// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/debug.h>
#include <ddk/platform-defs.h>

#include <soc/broadcom/bcm2711.h>
#include <soc/broadcom/bcm2835-mailbox.h>

#include "rpi4.h"

namespace rpi4 {

zx_status_t Rpi4::MailboxInit() {
  const pbus_mmio_t mailbox_mmio = {
    .base = MAILBOX_BASE,
    .length = MAILBOX_SIZE * MAILBOX_COUNT,
  };

  const pbus_irq_t mailbox_irqs[] = {
    {
      .irq = GIC_SPI_MAILBOX0,
      .mode = ZX_INTERRUPT_MODE_EDGE_HIGH,
    },
    {
      .irq = GIC_SPI_MAILBOX1,
      .mode = ZX_INTERRUPT_MODE_EDGE_HIGH,
    },
  };

  const pbus_bti_t mailbox_bti = {
    .iommu_index = 0,
    .bti_id = BTI_MAILBOX,
  };

  pbus_dev_t mailbox_dev = {};
  mailbox_dev.name = "mailbox";
  mailbox_dev.vid = PDEV_VID_BROADCOM;
  mailbox_dev.pid = PDEV_PID_RPI4;
  mailbox_dev.did = PDEV_DID_BCM_MAILBOX;
  mailbox_dev.mmio_list = &mailbox_mmio;
  mailbox_dev.mmio_count = 1;
  mailbox_dev.irq_list = mailbox_irqs;
  mailbox_dev.irq_count = countof(mailbox_irqs);
  mailbox_dev.bti_list = &mailbox_bti;
  mailbox_dev.bti_count = 1;

  auto status = pbus_.DeviceAdd(&mailbox_dev);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: ProtocolDeviceAdd failed: %d", __func__, status);
    return status;
  }

  return ZX_OK;
}

}  // namespace rpi4
