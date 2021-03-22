// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bcm2835-clk.h"

#include <fuchsia/hardware/mailbox/cpp/banjo.h>

#include <soc/broadcom/bcm2835-mailbox.h>
#include <soc/broadcom/rpi-clk.h>
#include <soc/broadcom/rpi-firmware.h>

namespace clk {

zx_status_t Bcm2835Clk::raspberrypi_clock_property(uint32_t id, uint32_t tag, uint32_t *val)
{
  struct raspberrypi_firmware_clk_prop msg = {
    .id = id,
    .val = *val,
  };

  mailbox_data_buf_t mdata;
  mdata.cmd = tag;
  mdata.tx_buffer = reinterpret_cast<const uint8_t*>(&msg);
  mdata.tx_size = sizeof(msg);

  mailbox_channel_t channel;
  channel.mailbox = 0;
  channel.rx_buffer = reinterpret_cast<const uint8_t*>(&msg);
  channel.rx_size = sizeof(msg);

  zx_status_t status = mailbox_.SendCommand(&channel, &mdata);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: mailbox_send_command failed - error status %d", __FILE__, status);
    return status;
  }

  *val = msg.val;
  return ZX_OK;
}

zx_status_t Bcm2835Clk::raspberrypi_fw_get_enable(uint32_t id, bool* enable)
{
  uint32_t val = 0;
  zx_status_t status = raspberrypi_clock_property(id, RPI_FIRMWARE_GET_CLOCK_STATE, &val);
  if (status != ZX_OK) {
    return status;
  }

  *enable = val & RPI_FIRMWARE_STATE_ENABLE;
  return ZX_OK;
}

zx_status_t Bcm2835Clk::raspberrypi_fw_set_enable(uint32_t id, bool enable)
{
  uint32_t val = 0;
  zx_status_t status = raspberrypi_clock_property(id, RPI_FIRMWARE_GET_CLOCK_STATE, &val);
  if (status != ZX_OK) {
    return status;
  }

  if (!(val & RPI_FIRMWARE_STATE_ENABLE)) {
    val |= RPI_FIRMWARE_STATE_ENABLE;
    return raspberrypi_clock_property(id, RPI_FIRMWARE_SET_CLOCK_STATE, &val);
  }
  return ZX_OK;
}

zx_status_t Bcm2835Clk::raspberrypi_fw_get_rate(uint32_t id, uint64_t* rate)
{
  uint32_t val = 0;
  zx_status_t status = raspberrypi_clock_property(id, RPI_FIRMWARE_GET_CLOCK_RATE, &val);
  if (status != ZX_OK) {
    return status;
  }

  *rate = (uint64_t) val;
  return ZX_OK;
}

zx_status_t Bcm2835Clk::raspberrypi_fw_set_rate(uint32_t id, uint64_t rate)
{
  uint32_t val = (uint32_t)rate;
  return raspberrypi_clock_property(id, RPI_FIRMWARE_SET_CLOCK_RATE, &val);
}

}  // namespace clk
