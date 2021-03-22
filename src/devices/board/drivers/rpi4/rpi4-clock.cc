// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/binding.h>
#include <ddk/debug.h>
#include <ddk/metadata.h>
#include <ddk/metadata/clock.h>
#include <ddk/platform-defs.h>

#include <soc/broadcom/bcm2711.h>
#include <soc/broadcom/bcm2835-clk.h>

#include "rpi4.h"

namespace rpi4 {

zx_status_t Rpi4::ClockInit() {
  constexpr pbus_mmio_t clock_mmio = {
    .base = CLOCK_BASE,
    .length = CLOCK_SIZE,
  };

  constexpr clock_id_t clock_ids[] = {
    {BCM2835_CLOCK_TIMER},
    {BCM2835_CLOCK_PWM},
    {BCM2835_CLOCK_PCM},
    {BCM2835_CLOCK_AVEO},
    {BCM2835_CLOCK_GP0},
    {BCM2835_CLOCK_GP1},
    {BCM2835_CLOCK_GP2},
    {BCM2835_CLOCK_CAM0},
    {BCM2835_CLOCK_CAM1},
    {BCM2835_CLOCK_DSI0E},
    {BCM2835_CLOCK_DSI1E},
    {BCM2835_CLOCK_DSI0P},
    {BCM2835_CLOCK_DSI1P},
    {RPI_FIRMWARE_CLK_EMMC},
    {RPI_FIRMWARE_CLK_PIXEL},
    {RPI_FIRMWARE_CLK_PIXEL_BVB},
    {RPI_FIRMWARE_CLK_EMMC2},
  };

  const pbus_metadata_t clock_metadata = {
    .type = DEVICE_METADATA_CLOCK_IDS,
    .data_buffer = reinterpret_cast<const uint8_t*>(&clock_ids),
    .data_size = sizeof(clock_ids),
  };

  const zx_bind_inst_t root_match[] = {
    BI_MATCH(),
  };
  const zx_bind_inst_t clock_mailbox_match[] = {
    BI_ABORT_IF(NE, BIND_PROTOCOL, ZX_PROTOCOL_MAILBOX),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_VID, PDEV_VID_BROADCOM),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_PID, PDEV_PID_RPI4),
    BI_MATCH_IF(EQ, BIND_PLATFORM_DEV_DID, PDEV_DID_BCM_MAILBOX),
  };
  const device_fragment_part_t clock_mailbox_fragment[] = {
    {countof(root_match), root_match},
    {countof(clock_mailbox_match), clock_mailbox_match},
  };
  static const device_fragment_t clock_fragments[] = {
    {"clock-mailbox", countof(clock_mailbox_fragment), clock_mailbox_fragment},
  };

  pbus_dev_t dev = {};
  dev.name = "rpi4-clock";
  dev.vid = PDEV_VID_BROADCOM;
  dev.pid = PDEV_PID_RPI4;
  dev.did = PDEV_DID_BCM_CLK;
  dev.mmio_list = &clock_mmio;
  dev.mmio_count = 1;
  dev.metadata_list = &clock_metadata;
  dev.metadata_count = 1;

  auto status = pbus_.CompositeDeviceAdd(&dev, reinterpret_cast<uint64_t>(clock_fragments), countof(clock_fragments), 1);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: DeviceAdd failed %d", __func__, status);
    return status;
  }

  return ZX_OK;
}

}  // namespace board_vs680_evk
