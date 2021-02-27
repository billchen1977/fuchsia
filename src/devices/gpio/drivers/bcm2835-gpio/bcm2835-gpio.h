// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVICES_GPIO_DRIVERS_BCM2835_GPIO_BCM2835_GPIO_H_
#define SRC_DEVICES_GPIO_DRIVERS_BCM2835_GPIO_BCM2835_GPIO_H_

#include <fuchsia/hardware/gpioimpl/cpp/banjo.h>
#include <lib/mmio/mmio.h>
#include <threads.h>

#include <ddktl/device.h>
#include <fbl/array.h>
#include <fbl/mutex.h>

#include <soc/broadcom/bcm2835-gpio.h>

namespace gpio {

class Bcm2835Gpio : public ddk::Device<Bcm2835Gpio, ddk::Unbindable>, 
                    public ddk::GpioImplProtocol<Bcm2835Gpio, ddk::base_protocol> {
 public:
  static zx_status_t Create(void* ctx, zx_device_t* parent);

  Bcm2835Gpio(zx_device_t* parent, ddk::MmioBuffer mmio_gpio,
              fbl::Array<zx::interrupt> port_interrupts, 
              const Bcm2835GpioMetadata& gpio_metadata)
      : ddk::Device<Bcm2835Gpio, ddk::Unbindable>(parent),
        mmio_(std::move(mmio_gpio)),
        port_interrupts_(std::move(port_interrupts)),
        gpio_metadata_(gpio_metadata) {}

  zx_status_t GpioImplConfigIn(uint32_t index, uint32_t flags);
  zx_status_t GpioImplConfigOut(uint32_t index, uint8_t initial_value);
  zx_status_t GpioImplSetAltFunction(uint32_t index, uint64_t function);
  zx_status_t GpioImplRead(uint32_t index, uint8_t* out_value);
  zx_status_t GpioImplWrite(uint32_t index, uint8_t value);
  zx_status_t GpioImplGetInterrupt(uint32_t index, uint32_t flags, zx::interrupt* out_irq);
  zx_status_t GpioImplReleaseInterrupt(uint32_t index);
  zx_status_t GpioImplSetPolarity(uint32_t index, uint32_t polarity);
  zx_status_t GpioImplSetDriveStrength(uint32_t index, uint64_t ua, uint64_t* out_actual_ua) {
    return ZX_ERR_NOT_SUPPORTED;
  }

  void DdkUnbind(ddk::UnbindTxn txn);
  void DdkRelease() { delete this; }

  zx_status_t Init();
  void Shutdown();

 private:
  zx_status_t Bind();
  int Thread();

  thrd_t thread_;
  ddk::MmioBuffer mmio_;
  fbl::Mutex mmio_lock_;
  fbl::Array<zx::interrupt> port_interrupts_;
  fbl::Array<zx::interrupt> gpio_interrupts_;
  zx::port port_;
  Bcm2835GpioMetadata gpio_metadata_;
};

}  // namespace gpio

#endif  // SRC_DEVICES_GPIO_DRIVERS_BCM2835_GPIO_BCM2835_GPIO_H_
