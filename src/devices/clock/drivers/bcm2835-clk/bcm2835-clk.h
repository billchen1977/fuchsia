// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVICES_CLOCK_DRIVERS_BCM2835_CLK_BCM2835_CLK_H_
#define SRC_DEVICES_CLOCK_DRIVERS_BCM2835_CLK_BCM2835_CLK_H_

#include <fuchsia/hardware/clockimpl/cpp/banjo.h>
#include <fuchsia/hardware/mailbox/cpp/banjo.h>
#include <lib/mmio/mmio.h>
#include <lib/zircon-internal/thread_annotations.h>

#include <ddktl/device.h>
#include <fbl/mutex.h>
#include <soc/broadcom/bcm2835-clk.h>

#include "bcm2835-clk-internal.h"

namespace clk {

class Bcm2835Clk;
using DeviceType = ddk::Device<Bcm2835Clk, ddk::Unbindable>;

class Bcm2835Clk : public DeviceType, public ddk::ClockImplProtocol<Bcm2835Clk, ddk::base_protocol> {
 public:
  static zx_status_t Create(void* ctx, zx_device_t* parent);

  // Clock Protocol Implementation
  zx_status_t ClockImplEnable(uint32_t index);
  zx_status_t ClockImplDisable(uint32_t index);
  zx_status_t ClockImplIsEnabled(uint32_t index, bool* out_enabled);

  zx_status_t ClockImplSetRate(uint32_t index, uint64_t hz);
  zx_status_t ClockImplQuerySupportedRate(uint32_t id, uint64_t max_rate, uint64_t* out_best_rate);
  zx_status_t ClockImplGetRate(uint32_t index, uint64_t* out_current_rate);

  zx_status_t ClockImplSetInput(uint32_t index, uint32_t idx);
  zx_status_t ClockImplGetNumInputs(uint32_t index, uint32_t* out);
  zx_status_t ClockImplGetInput(uint32_t index, uint32_t* out);

  // Device Protocol Implementation.
  void DdkUnbind(ddk::UnbindTxn txn);
  void DdkRelease() { delete this; }

 protected:
  Bcm2835Clk(zx_device_t* parent, ddk::MmioBuffer mmio, ddk::MailboxProtocolClient mailbox)
      : DeviceType(parent),
        mmio_(std::move(mmio)),
        mailbox_(std::move(mailbox)) {}

 private:
  bool bcm2835_pll_is_on(uint32_t index);
  uint64_t bcm2835_pll_round_rate(uint32_t index, uint64_t rate);
  uint64_t bcm2835_pll_get_rate(uint32_t index);
  zx_status_t bcm2835_pll_off(uint32_t index);
  zx_status_t bcm2835_pll_on(uint32_t index);
  void bcm2835_pll_write_ana(uint32_t ana_reg_base, uint32_t *ana);
  zx_status_t bcm2835_pll_set_rate(uint32_t index, uint64_t rate);

  bool bcm2835_pll_divider_is_on(uint32_t index);
  uint64_t bcm2835_pll_divider_round_rate(uint32_t index, uint64_t rate);
  uint64_t bcm2835_pll_divider_get_rate(uint32_t index);
  zx_status_t bcm2835_pll_divider_off(uint32_t index);
  zx_status_t bcm2835_pll_divider_on(uint32_t index);
  zx_status_t bcm2835_pll_divider_set_rate(uint32_t index, uint64_t rate);

  bool bcm2835_clock_is_on(uint32_t index);
  uint64_t bcm2835_clock_get_parent_rate(uint32_t index);
  uint64_t bcm2835_clock_get_rate(uint32_t index);
  zx_status_t bcm2835_clock_wait_busy(const struct bcm2835_clock_data *data);
  zx_status_t bcm2835_clock_off(uint32_t index);
  zx_status_t bcm2835_clock_on(uint32_t index);
  zx_status_t bcm2835_clock_set_rate(uint32_t index, uint64_t rate);
  uint64_t bcm2835_clock_choose_div_and_prate(const struct bcm2835_clock_data *data, int parent, uint64_t rate);
  uint64_t bcm2835_clock_determine_rate(uint32_t index, uint64_t rate);
  void bcm2835_clock_set_parent(uint32_t index, uint32_t idx);
  uint32_t bcm2835_clock_get_parent(uint32_t index);
  
  zx_status_t raspberrypi_clock_property(uint32_t id, uint32_t tag, uint32_t *val);
  zx_status_t raspberrypi_fw_get_enable(uint32_t id, bool *enable);
  zx_status_t raspberrypi_fw_set_enable(uint32_t id, bool enable);
  zx_status_t raspberrypi_fw_get_rate(uint32_t id, uint64_t *rate);
  zx_status_t raspberrypi_fw_set_rate(uint32_t id, uint64_t rate);

  fbl::Mutex mmio_lock_;
  ddk::MmioBuffer mmio_;
  ddk::MailboxProtocolClient mailbox_;
};

}  // namespace clk

#endif  // SRC_DEVICES_CLOCK_DRIVERS_BCM2835_CLK_BCM2835_CLK_H_
