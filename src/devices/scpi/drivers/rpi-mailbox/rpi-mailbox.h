// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVICES_SCPI_DRIVERS_RPI_MAILBOX_H_
#define SRC_DEVICES_SCPI_DRIVERS_RPI_MAILBOX_H_

#include <fuchsia/hardware/mailbox/cpp/banjo.h>
#include <fuchsia/hardware/platform/device/c/banjo.h>
#include <lib/device-protocol/pdev.h>
#include <lib/device-protocol/platform-device.h>
#include <lib/mmio/mmio.h>
#include <lib/sync/completion.h>
#include <lib/zx/bti.h>
#include <lib/zx/interrupt.h>
#include <threads.h>

#include <optional>

#include <ddk/debug.h>
#include <ddk/driver.h>
#include <ddk/platform-defs.h>
#include <ddktl/device.h>

#include <soc/broadcom/bcm2835-mailbox.h>

namespace mailbox {

class RpiMailbox;
using DeviceType = ddk::Device<RpiMailbox, ddk::Unbindable>;

class RpiMailbox : public DeviceType, public ddk::MailboxProtocol<RpiMailbox, ddk::base_protocol> {
 public:
  DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(RpiMailbox);

  explicit RpiMailbox(zx_device_t* parent) : DeviceType(parent), pdev_(parent) {}

  static zx_status_t Create(void* ctx, zx_device_t* parent);

  // DDK Hooks.
  void DdkRelease();
  void DdkUnbind(ddk::UnbindTxn txn);

  // ZX_PROTOCOL_MAILBOX protocol.
  zx_status_t MailboxSendCommand(const mailbox_channel_t* channel, const mailbox_data_buf_t* mdata);

 private:
  zx_status_t InitPdev();
  zx_status_t Bind();

  ddk::PDev pdev_;
  zx::interrupt inth_[MAILBOX_COUNT];
  mtx_t mailbox_chan_lock_[MAILBOX_COUNT];

  std::optional<ddk::MmioBuffer> mailbox_mmio_;
  zx::bti bti_;
};

}  // namespace mailbox

#endif  // SRC_DEVICES_SCPI_DRIVERS_RPI_MAILBOX_H_
