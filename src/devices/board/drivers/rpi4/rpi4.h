// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVICES_BOARD_DRIVERS_RPI4_RPI4_H_
#define SRC_DEVICES_BOARD_DRIVERS_RPI4_RPI4_H_

#include <fuchsia/hardware/gpioimpl/cpp/banjo.h>
#include <fuchsia/hardware/platform/bus/cpp/banjo.h>
#include <threads.h>

#include <ddktl/device.h>

namespace rpi4 {

// BTI IDs for our devices
enum {
  BTI_SYSMEM,
  BTI_MAILBOX,
  BTI_DISPLAY,
};

class Rpi4;
using Rpi4Type = ddk::Device<Rpi4>;

// This is the main class for the Rpi4 platform bus driver.
class Rpi4 : public Rpi4Type {
 public:
  explicit Rpi4(zx_device_t* parent, const ddk::PBusProtocolClient& pbus, const pdev_board_info_t& board_info)
      : Rpi4Type(parent), pbus_(pbus), board_info_(board_info) {}

  static zx_status_t Create(void* ctx, zx_device_t* parent);

  // Device protocol implementation.
  void DdkRelease() { delete this; }

 private:
  zx_status_t Start();
  int Thread();

  zx_status_t ClockInit();
  zx_status_t GpioInit();
  zx_status_t MailboxInit();
  zx_status_t SysmemInit();
  zx_status_t DisplayInit();

  const ddk::PBusProtocolClient pbus_;
  const pdev_board_info_t board_info_;
  ddk::GpioImplProtocolClient gpio_impl_;

  thrd_t thread_;
};

}  // namespace rpi4

#endif  // SRC_DEVICES_BOARD_DRIVERS_RPI4_RPI4_H_
