// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bcm2835-gpio.h"

#include <fuchsia/hardware/platform/bus/cpp/banjo.h>
#include <lib/device-protocol/pdev.h>

#include <ddk/debug.h>
#include <ddk/metadata.h>
#include <ddk/platform-defs.h>
#include <fbl/alloc_checker.h>
#include <fbl/array.h>
#include <fbl/auto_lock.h>
#include <fbl/mutex.h>

#include "src/devices/gpio/drivers/bcm2835-gpio/bcm2835-gpio-bind.h"

namespace gpio {

zx_status_t Bcm2835Gpio::Create(void* ctx, zx_device_t* parent) {
  Bcm2835GpioMetadata gpio_metadata = {};
  size_t actual;
  zx_status_t status = device_get_metadata(parent, DEVICE_METADATA_PRIVATE, &gpio_metadata,
                                           sizeof(gpio_metadata), &actual);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: Failed to get metadata: %d", __FILE__, status);
    return status;
  }
  if (actual != sizeof(gpio_metadata)) {
    zxlogf(ERROR, "%s: Unexpected metadata size", __FILE__);
    return ZX_ERR_INTERNAL;
  }

  ddk::PDev pdev(parent);
  if (!pdev.is_valid()) {
    zxlogf(ERROR, "%s: Failed to get ZX_PROTOCOL_PLATFORM_DEVICE", __FILE__);
    return ZX_ERR_NO_RESOURCES;
  }

  pdev_device_info_t device_info = {};
  if ((status = pdev.GetDeviceInfo(&device_info)) != ZX_OK) {
    zxlogf(ERROR, "%s: Failed to get device info: %d", __FILE__, status);
    return status;
  }

  std::optional<ddk::MmioBuffer> mmio;
  if ((status = pdev.MapMmio(0, &mmio)) != ZX_OK) {
    zxlogf(ERROR, "%s: Failed to map pinmux MMIO: %d", __FILE__, status);
    return status;
  }

  fbl::AllocChecker ac;

  fbl::Array<zx::interrupt> port_interrupts(new (&ac) zx::interrupt[device_info.irq_count],
                                            device_info.irq_count);
  if (!ac.check()) {
    zxlogf(ERROR, "%s: Allocation failed", __FILE__);
    return ZX_ERR_NO_RESOURCES;
  }

  for (uint32_t i = 0; i < port_interrupts.size(); i++) {
    zx::interrupt interrupt;
    if ((status = pdev.GetInterrupt(i, &interrupt)) != ZX_OK) {
      zxlogf(ERROR, "%s: Failed to get interrupt: %d", __FILE__, status);
      return status;
    }
    port_interrupts[i] = std::move(interrupt);
  }

  auto device = fbl::make_unique_checked<Bcm2835Gpio>(&ac, parent, *std::move(mmio),
                                                      std::move(port_interrupts), gpio_metadata);
  if (!ac.check()) {
    zxlogf(ERROR, "%s: Failed to allocate device memory", __FILE__);
    return ZX_ERR_NO_MEMORY;
  }

  if ((status = device->Init()) != ZX_OK) {
    zxlogf(ERROR, "%s: Init failed: %d", __FILE__, status);
    return status;
  }

  if ((status = device->Bind()) != ZX_OK) {
    zxlogf(ERROR, "%s: Bind failed: %d", __FILE__, status);
    device->Shutdown();
    return status;
  }

  __UNUSED auto* dummy = device.release();
  return ZX_OK;
}

zx_status_t Bcm2835Gpio::Init() {
  zx_status_t status = zx::port::create(ZX_PORT_BIND_TO_INTERRUPT, &port_);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s zx_port_create failed %d", __FUNCTION__, status);
    return status;
  }

  uint32_t port_key = 0;
  for (const zx::interrupt& port_interrupt : port_interrupts_) {
    status = port_interrupt.bind(port_, port_key++, 0 /*options*/);
    if (status != ZX_OK) {
      zxlogf(ERROR, "%s zx_interrupt_bind failed %d", __FUNCTION__, status);
      return status;
    }
  }

  const size_t interrupt_count = gpio_metadata_.pin_count;
  fbl::AllocChecker ac;
  gpio_interrupts_ = fbl::Array(new (&ac) zx::interrupt[interrupt_count], interrupt_count);
  if (!ac.check()) {
    return ZX_ERR_NO_MEMORY;
  }

  auto cb = [](void* arg) -> int { return reinterpret_cast<Bcm2835Gpio*>(arg)->Thread(); };
  int rc = thrd_create_with_name(&thread_, cb, this, "bcm2835-gpio-thread");
  if (rc != thrd_success) {
    return ZX_ERR_INTERNAL;
  }

  return ZX_OK;
}

zx_status_t Bcm2835Gpio::Bind() {
  ddk::PBusProtocolClient pbus(parent());
  if (!pbus.is_valid()) {
    zxlogf(ERROR, "%s: Failed to get ZX_PROTOCOL_PLATFORM_BUS", __FILE__);
    return ZX_ERR_NO_RESOURCES;
  }

  zx_status_t status;
  if ((status = DdkAdd("bcm2835-gpio")) != ZX_OK) {
    zxlogf(ERROR, "%s: DdkAdd failed: %d", __FILE__, status);
    return status;
  }

  gpio_impl_protocol_t gpio_proto = {.ops = &gpio_impl_protocol_ops_, .ctx = this};
  status = pbus.RegisterProtocol(ddk_proto_id_, reinterpret_cast<uint8_t*>(&gpio_proto),
                                 sizeof(gpio_proto));
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: Failed to register ZX_PROTOCOL_GPIO_IMPL: %d", __FILE__, __LINE__);
    return status;
  }

  return ZX_OK;
}

int Bcm2835Gpio::Thread() {
  while (1) {
    zx_port_packet_t packet;
    zx_status_t status = port_.wait(zx::time::infinite(), &packet);
    if (status != ZX_OK) {
      zxlogf(ERROR, "%s port wait failed: %d", __FUNCTION__, status);
      return thrd_error;
    }

    if (packet.key == (uint64_t)-1) {
      zxlogf(INFO, "As370Gpio thread terminating");
      return thrd_success;
    }
    if (packet.key >= gpio_metadata_.bank_count) {
      zxlogf(WARNING, "%s received interrupt from invalid port", __FUNCTION__);
      continue;
    }

    for (uint32_t index = gpio_metadata_.bank_pin[packet.key]; index < gpio_metadata_.bank_pin[packet.key + 1]; index++) {
      if (mmio_.GetBit<uint32_t>(index % 32, GPIO_GPEDS0 + index / 32)) {
        fbl::AutoLock al(&mmio_lock_);
        if (gpio_interrupts_[index]) {
          status = gpio_interrupts_[index].trigger(0, zx::time(packet.interrupt.timestamp));
          if (status != ZX_OK) {
            zxlogf(ERROR, "%s zx_interrupt_trigger failed %d", __func__, status);
          }
        }
        // Clear the interrupt.
        mmio_.SetBit<uint32_t>(index % 32, GPIO_GPEDS0 + index / 32 * 4);
      }
    }
    port_interrupts_[packet.key].ack();
  }
  return thrd_success;
}

zx_status_t Bcm2835Gpio::GpioImplConfigIn(uint32_t index, uint32_t flags) {
  if (index >= gpio_metadata_.pin_count) {
    return ZX_ERR_INVALID_ARGS;
  }

  const uint32_t pull_mode = flags & GPIO_PULL_MASK;

  {
    fbl::AutoLock al(&mmio_lock_);
    mmio_.ModifyBits32(GPIO_GPFSEL_INPUT, (index % 10) * 3, 3, GPIO_GPFSEL0 + index / 10 * 4);

    switch (pull_mode) {
      case GPIO_NO_PULL:
        mmio_.ModifyBits32(GPIO_GPPUD_NONE, (index % 16) * 2, 2, GPIO_GPPUD0 + index / 16 * 4);
        return ZX_OK;
      case GPIO_PULL_UP:
        mmio_.ModifyBits32(GPIO_GPPUD_UP, (index % 16) * 2, 2, GPIO_GPPUD0 + index / 16 * 4);
        return ZX_OK;
      case GPIO_PULL_DOWN:
        mmio_.ModifyBits32(GPIO_GPPUD_DOWN, (index % 16) * 2, 2, GPIO_GPPUD0 + index / 16 * 4);
        return ZX_OK;
    }
  }

  return ZX_OK;
}

zx_status_t Bcm2835Gpio::GpioImplConfigOut(uint32_t index, uint8_t initial_value) {
  if (index >= gpio_metadata_.pin_count) {
    return ZX_ERR_INVALID_ARGS;
  }

  {
    fbl::AutoLock al(&mmio_lock_);
    mmio_.ModifyBits32(GPIO_GPFSEL_OUTPUT, (index % 10) * 3, 3, GPIO_GPFSEL0 + index / 10 * 4);
    mmio_.SetBit<uint32_t>(index % 32, (initial_value ? GPIO_GPSET0 : GPIO_GPCLR0) + index / 32 * 4);
  }

  return ZX_OK;
}

zx_status_t Bcm2835Gpio::GpioImplSetAltFunction(uint32_t index, uint64_t function) {
  if (index >= gpio_metadata_.pin_count) {
    return ZX_ERR_INVALID_ARGS;
  }

  if (function < 2 || function >= 8) {
    return ZX_ERR_OUT_OF_RANGE;
  }

  {
    fbl::AutoLock al(&mmio_lock_);
    mmio_.ModifyBits32(function, (index % 10) * 3, 3, GPIO_GPFSEL0 + index / 10 * 4);
  }

  return ZX_OK;
}

zx_status_t Bcm2835Gpio::GpioImplRead(uint32_t index, uint8_t* out_value) {
  if (index >= gpio_metadata_.pin_count) {
    return ZX_ERR_INVALID_ARGS;
  }

  *out_value = mmio_.GetBit<uint32_t>(index % 32, GPIO_GPLEV0 + index / 32 * 4);

  return ZX_OK;
}

zx_status_t Bcm2835Gpio::GpioImplWrite(uint32_t index, uint8_t value) {
  if (index >= gpio_metadata_.pin_count) {
    return ZX_ERR_INVALID_ARGS;
  }

  {
    fbl::AutoLock al(&mmio_lock_);
    mmio_.SetBit<uint32_t>(index % 32, (value ? GPIO_GPSET0 : GPIO_GPCLR0) + index / 32 * 4);
  }

  return ZX_OK;
}

zx_status_t Bcm2835Gpio::GpioImplGetInterrupt(uint32_t index, uint32_t flags, zx::interrupt* out_irq) {
  if (index >= gpio_metadata_.pin_count) {
    return ZX_ERR_INVALID_ARGS;
  }

  fbl::AutoLock al(&mmio_lock_);

  if (gpio_interrupts_[index]) {
    zxlogf(ERROR, "%s interrupt %u already exists", __FUNCTION__, index);
    return ZX_ERR_ALREADY_EXISTS;
  }

  zx::interrupt irq;
  zx_status_t status = zx::interrupt::create(zx::resource(), index, ZX_INTERRUPT_VIRTUAL, &irq);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s zx::interrupt::create failed %d ", __FUNCTION__, status);
    return status;
  }
  status = irq.duplicate(ZX_RIGHT_SAME_RIGHTS, out_irq);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s interrupt.duplicate failed %d ", __FUNCTION__, status);
    return status;
  }

  mmio_.SetBit<uint32_t>(index % 32, GPIO_GPEDS0 + index / 32 * 4);

  switch (flags & ZX_INTERRUPT_MODE_MASK) {
    case ZX_INTERRUPT_MODE_EDGE_LOW:
      mmio_.SetBit<uint32_t>(index % 32, GPIO_GPFEN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPREN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPLEN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPHEN0 + index / 32 * 4);
      break;
    case ZX_INTERRUPT_MODE_EDGE_HIGH:
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPFEN0 + index / 32 * 4);
      mmio_.SetBit<uint32_t>(index % 32, GPIO_GPREN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPLEN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPHEN0 + index / 32 * 4);
      break;
    case ZX_INTERRUPT_MODE_LEVEL_LOW:
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPFEN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPREN0 + index / 32 * 4);
      mmio_.SetBit<uint32_t>(index % 32, GPIO_GPLEN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPHEN0 + index / 32 * 4);
      break;
    case ZX_INTERRUPT_MODE_LEVEL_HIGH:
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPFEN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPREN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPLEN0 + index / 32 * 4);
      mmio_.SetBit<uint32_t>(index % 32, GPIO_GPHEN0 + index / 32 * 4);
      break;
    case ZX_INTERRUPT_MODE_EDGE_BOTH:
      mmio_.SetBit<uint32_t>(index % 32, GPIO_GPFEN0 + index / 32 * 4);
      mmio_.SetBit<uint32_t>(index % 32, GPIO_GPREN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPLEN0 + index / 32 * 4);
      mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPHEN0 + index / 32 * 4);
      break;
    default:
      return ZX_ERR_INVALID_ARGS;
  }
  gpio_interrupts_[index] = std::move(irq);

  zxlogf(DEBUG, "%s INT %u enabled", __FUNCTION__, index);
  return ZX_OK;
}

zx_status_t Bcm2835Gpio::GpioImplReleaseInterrupt(uint32_t index) {
  if (index >= gpio_metadata_.pin_count) {
    return ZX_ERR_INVALID_ARGS;
  }

  fbl::AutoLock al(&mmio_lock_);

  if (!gpio_interrupts_[index]) {
    zxlogf(ERROR, "%s interrupt %u not enabled", __FUNCTION__, index);
    return ZX_ERR_NOT_FOUND;
  }

  mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPFEN0 + index / 32 * 4);
  mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPREN0 + index / 32 * 4);
  mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPLEN0 + index / 32 * 4);
  mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPHEN0 + index / 32 * 4);
  mmio_.SetBit<uint32_t>(index % 32, GPIO_GPEDS0 + index / 32 * 4);

  gpio_interrupts_[index].destroy();
  gpio_interrupts_[index].reset();

  zxlogf(DEBUG, "%s INT %u disabled", __FUNCTION__, index);
  return ZX_OK;
}

zx_status_t Bcm2835Gpio::GpioImplSetPolarity(uint32_t index, uint32_t polarity) {
  if (index >= gpio_metadata_.pin_count) {
    return ZX_ERR_INVALID_ARGS;
  }

  fbl::AutoLock al(&mmio_lock_);

  if (!gpio_interrupts_[index]) {
    zxlogf(ERROR, "%s interrupt %u not enabled", __FUNCTION__, index);
    return ZX_ERR_NOT_FOUND;
  }

  if (polarity == GPIO_POLARITY_HIGH) {
    mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPFEN0 + index / 32 * 4);
    mmio_.SetBit<uint32_t>(index % 32, GPIO_GPREN0 + index / 32 * 4);
  } else {
    mmio_.SetBit<uint32_t>(index % 32, GPIO_GPFEN0 + index / 32 * 4);
    mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPREN0 + index / 32 * 4);
  }
  mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPLEN0 + index / 32 * 4);
  mmio_.ClearBit<uint32_t>(index % 32, GPIO_GPHEN0 + index / 32 * 4);
  mmio_.SetBit<uint32_t>(index % 32, GPIO_GPEDS0 + index / 32 * 4);

  return ZX_OK;
}

void Bcm2835Gpio::Shutdown() {
  zx_port_packet packet = {(uint64_t)-1, ZX_PKT_TYPE_USER, ZX_OK, {}};
  port_.queue(&packet);
  thrd_join(thread_, nullptr);
}

void Bcm2835Gpio::DdkUnbind(ddk::UnbindTxn txn) {
  Shutdown();
  txn.Reply();
}

}  // namespace gpio

static constexpr zx_driver_ops_t bcm2835_gpio_driver_ops = []() {
  zx_driver_ops_t ops = {};
  ops.version = DRIVER_OPS_VERSION;
  ops.bind = gpio::Bcm2835Gpio::Create;
  return ops;
}();

ZIRCON_DRIVER(bcm2835_gpio, bcm2835_gpio_driver_ops, "zircon", "0.1");
