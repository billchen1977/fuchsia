// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/binding.h>
#include <ddk/debug.h>
#include <ddk/metadata.h>
#include <ddk/metadata/clock.h>
#include <ddk/platform-defs.h>

#include <soc/broadcom/bcm2711.h>

#include "rpi4.h"

namespace rpi4 {

zx_status_t Rpi4::DisplayInit() {
  const pbus_mmio_t display_mmios[] = {
    {
      .base = SMI_BASE,
      .length = SMI_SIZE,
    },
  };

  const pbus_irq_t display_irqs[] = {
    {
      .irq = GIC_SPI_SMI,
      .mode = ZX_INTERRUPT_MODE_EDGE_HIGH,
    },
  };

  const pbus_bti_t display_bti = {
    .iommu_index = 0,
    .bti_id = BTI_DISPLAY,
  };

  const zx_bind_inst_t root_match[] = {
    BI_MATCH(),
  };
  const zx_bind_inst_t display_mailbox_match[] = {
    BI_ABORT_IF(NE, BIND_PROTOCOL, ZX_PROTOCOL_MAILBOX),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_VID, PDEV_VID_BROADCOM),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_PID, PDEV_PID_RPI4),
    BI_MATCH_IF(EQ, BIND_PLATFORM_DEV_DID, PDEV_DID_BCM_MAILBOX),
  };
  const zx_bind_inst_t display_sysmem_match[] = {
    BI_MATCH_IF(EQ, BIND_PROTOCOL, ZX_PROTOCOL_SYSMEM),
  };
  const device_fragment_part_t display_mailbox_fragment[] = {
    {countof(root_match), root_match},
    {countof(display_mailbox_match), display_mailbox_match},
  };
  const device_fragment_part_t display_sysmem_fragment[] = {
    {countof(root_match), root_match},
    {countof(display_sysmem_match), display_sysmem_match},
 };
  static const device_fragment_t display_fragments[] = {
    {"display-mailbox", countof(display_mailbox_fragment), display_mailbox_fragment},
    {"display-sysmem", countof(display_sysmem_fragment), display_sysmem_fragment},
  };

  pbus_dev_t dev = {};
  dev.name = "vc4-display";
  dev.vid = PDEV_VID_BROADCOM;
  dev.pid = PDEV_PID_RPI4;
  dev.did = PDEV_DID_BCM_DISPLAY;
  dev.mmio_list = display_mmios;
  dev.mmio_count = countof(display_mmios);
  dev.irq_list = display_irqs;
  dev.irq_count = countof(display_irqs);
  dev.bti_list = &display_bti;
  dev.bti_count = 1;

  auto status = pbus_.CompositeDeviceAdd(&dev, reinterpret_cast<uint64_t>(display_fragments), countof(display_fragments), 1);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: DeviceAdd failed %d", __func__, status);
    return status;
  }

  return ZX_OK;
}

}  // namespace rpi4
