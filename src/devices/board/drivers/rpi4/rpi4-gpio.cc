// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/debug.h>
#include <ddk/metadata.h>
#include <ddk/metadata/gpio.h>
#include <ddk/platform-defs.h>

#include <soc/broadcom/bcm2711.h>
#include <soc/broadcom/bcm2835-gpio.h>

#include "rpi4.h"

namespace rpi4 {

zx_status_t Rpi4::GpioInit() {
  const Bcm2835GpioMetadata private_metadata = {
    .pin_count = GPIO_PIN_COUNT,
    .bank_pin = {
      0,
      GPIO_BANK0_PIN_COUNT,
      GPIO_BANK0_PIN_COUNT + GPIO_BANK1_PIN_COUNT,
      GPIO_BANK0_PIN_COUNT + GPIO_BANK0_PIN_COUNT + GPIO_BANK2_PIN_COUNT,
    },
  };

  const pbus_mmio_t gpio_mmio = {
    .base = GPIO_BASE,
    .length = GPIO_SIZE,
  };

  const pbus_irq_t gpio_irqs[] = {
    {
      .irq = GIC_SPI_GPIO0,
      .mode = ZX_INTERRUPT_MODE_LEVEL_HIGH,
    },
    {
      .irq = GIC_SPI_GPIO1,
      .mode = ZX_INTERRUPT_MODE_LEVEL_HIGH,
    },
    {
      .irq = GIC_SPI_GPIO2,
      .mode = ZX_INTERRUPT_MODE_LEVEL_HIGH,
    },
  };

  const gpio_pin_t gpio_pins[] = {
      {0},  {1},  {2},  {3},  {4},  {5},  {6},  {7},  {8},  {9},
      {10}, {11}, {12}, {13}, {14}, {15}, {16}, {17}, {18}, {19},
      {20}, {21}, {22}, {23}, {24}, {25}, {26}, {27}, {28}, {29},
      {30}, {31}, {32}, {33}, {34}, {35}, {36}, {37}, {38}, {39},
      {40}, {41}, {42}, {43}, {44}, {45}, {46}, {47}, {48}, {49},
      {50}, {51}, {52}, {53}, {54}, {55}, {56}, {57}, 
  };

  const pbus_metadata_t gpio_metadata[] = {
    {
      .type = DEVICE_METADATA_GPIO_PINS,
      .data_buffer = reinterpret_cast<const uint8_t*>(&gpio_pins),
      .data_size = sizeof(gpio_pins),
    },
    {
      .type = DEVICE_METADATA_PRIVATE,
      .data_buffer = reinterpret_cast<const uint8_t*>(&private_metadata),
      .data_size = sizeof(private_metadata),
    },
  };

  pbus_dev_t gpio_dev = {};
  gpio_dev.name = "gpio";
  gpio_dev.vid = PDEV_VID_BROADCOM;
  gpio_dev.pid = PDEV_PID_RPI4;
  gpio_dev.did = PDEV_DID_BCM_GPIO;
  gpio_dev.mmio_list = &gpio_mmio;
  gpio_dev.mmio_count = 1;
  gpio_dev.irq_list = gpio_irqs;
  gpio_dev.irq_count = countof(gpio_irqs);
  gpio_dev.metadata_list = gpio_metadata;
  gpio_dev.metadata_count = countof(gpio_metadata);

  auto status = pbus_.ProtocolDeviceAdd(ZX_PROTOCOL_GPIO_IMPL, &gpio_dev);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: ProtocolDeviceAdd failed: %d", __func__, status);
    return status;
  }
  gpio_impl_ = ddk::GpioImplProtocolClient(parent());
  if (!gpio_impl_.is_valid()) {
    zxlogf(ERROR, "%s: device_get_protocol failed", __func__);
    return ZX_ERR_INTERNAL;
  }
  return ZX_OK;
}

}  // namespace rpi4
