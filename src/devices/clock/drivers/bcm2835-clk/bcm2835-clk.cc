// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/hardware/clockimpl/c/banjo.h>
#include <fuchsia/hardware/platform/bus/c/banjo.h>
#include <fuchsia/hardware/platform/bus/cpp/banjo.h>
#include <fuchsia/hardware/platform/device/c/banjo.h>
#include <lib/device-protocol/pdev.h>

#include <numeric>

#include <ddk/platform-defs.h>
#include <fbl/auto_lock.h>

#include <soc/broadcom/bcm2835-clk.h>
#include <soc/broadcom/rpi-clk.h>

#include "bcm2835-clk.h"
#include "bcm2835-clk-const.h"

#include "src/devices/clock/drivers/bcm2835-clk/bcm2835-clk-bind.h"

namespace clk {

zx_status_t Bcm2835Clk::Create(void* ctx, zx_device_t* parent) {
  ddk::PDev pdev = ddk::PDev::FromFragment(parent);
  if (!pdev.is_valid()) {
    zxlogf(ERROR, "%s: failed to get pdev", __FILE__);
    return ZX_ERR_NO_RESOURCES;
  }

  pdev_device_info_t info;
  auto status = pdev.GetDeviceInfo(&info);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: GetDeviceInfo failed: %d", __FILE__, status);
    return status;
  }

  std::optional<ddk::MmioBuffer> mmio;
  status = pdev.MapMmio(0, &mmio);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: failed to map mmio %d", __FILE__, status);
    return status;
  }

  ddk::MailboxProtocolClient mailbox(parent, "clock-mailbox");
  if (!mailbox.is_valid()) {
    zxlogf(ERROR, "%s: could not get mailbox fragment", __FILE__);
    return ZX_ERR_NO_RESOURCES;
  }

  std::unique_ptr<Bcm2835Clk> device(
      new Bcm2835Clk(parent, *std::move(mmio), std::move(mailbox)));

  zx_device_prop_t props[] = {
      {BIND_PLATFORM_DEV_VID, 0, info.vid},
      {BIND_PLATFORM_DEV_PID, 0, info.pid},
      {BIND_PLATFORM_DEV_DID, 0, info.did},
  };

  status = device->DdkAdd(ddk::DeviceAddArgs("bcm2835-clk").set_props(props).set_proto_id(ZX_PROTOCOL_CLOCK_IMPL));
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: DdkAdd failed %d", __FILE__, status);
    return status;
  }

  // Intentially leak, devmgr owns the memory now.
  __UNUSED auto* unused = device.release();

  return ZX_OK;
}

zx_status_t Bcm2835Clk::ClockImplEnable(uint32_t index) {
  if (index >= BCM2835_CLOCK_COUNT) {
    return ZX_ERR_INVALID_ARGS;
  }
  
  switch (bcm2835_clk_descs[index].type) {
    case CLK_TYPE_PLL:
      return bcm2835_pll_on(index);
    case CLK_TYPE_PLL_DIV:
      return bcm2835_pll_divider_on(index);
    case CLK_TYPE_CLK:
      return bcm2835_clock_on(index);
    case CLK_TYPE_FW:
      return raspberrypi_fw_set_enable(bcm2835_clk_descs[index].data.fw_clk_id, true);
  }
  return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t Bcm2835Clk::ClockImplDisable(uint32_t index) {
  if (index >= BCM2835_CLOCK_COUNT) {
    return ZX_ERR_INVALID_ARGS;
  }
  
  switch (bcm2835_clk_descs[index].type) {
    case CLK_TYPE_PLL:
      return bcm2835_pll_off(index);
    case CLK_TYPE_PLL_DIV:
      return bcm2835_pll_divider_off(index);
    case CLK_TYPE_CLK:
      return bcm2835_clock_off(index);
    case CLK_TYPE_FW:
      return raspberrypi_fw_set_enable(bcm2835_clk_descs[index].data.fw_clk_id, false);
  }
  return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t Bcm2835Clk::ClockImplIsEnabled(uint32_t index, bool* out_enabled) {
  if (index >= BCM2835_CLOCK_COUNT) {
    return ZX_ERR_INVALID_ARGS;
  }
  
  switch (bcm2835_clk_descs[index].type) {
    case CLK_TYPE_PLL:
      *out_enabled = bcm2835_pll_is_on(index);
      return ZX_OK;
    case CLK_TYPE_PLL_DIV:
      *out_enabled = bcm2835_pll_divider_is_on(index);
      return ZX_OK;
    case CLK_TYPE_CLK:
      *out_enabled = bcm2835_clock_is_on(index);
      return ZX_OK;
    case CLK_TYPE_FW:
      return raspberrypi_fw_get_enable(bcm2835_clk_descs[index].data.fw_clk_id, out_enabled);
  }
  return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t Bcm2835Clk::ClockImplQuerySupportedRate(uint32_t index, uint64_t max_rate,
                                                    uint64_t* out_best_rate) {
  if (index >= BCM2835_CLOCK_COUNT) {
    return ZX_ERR_INVALID_ARGS;
  }
  
  switch (bcm2835_clk_descs[index].type) {
    case CLK_TYPE_PLL:
      *out_best_rate = bcm2835_pll_round_rate(index, max_rate);
      return ZX_OK;
    case CLK_TYPE_PLL_DIV:
      *out_best_rate = bcm2835_pll_divider_round_rate(index, max_rate);
      return ZX_OK;
    case CLK_TYPE_CLK:
      *out_best_rate = bcm2835_clock_determine_rate(index, max_rate);
      return ZX_OK;
  }
  return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t Bcm2835Clk::ClockImplGetRate(uint32_t index, uint64_t* out_current_rate) {
  if (index >= BCM2835_CLOCK_COUNT) {
    return ZX_ERR_INVALID_ARGS;
  }
  
  switch (bcm2835_clk_descs[index].type) {
    case CLK_TYPE_PLL:
      *out_current_rate = bcm2835_pll_get_rate(index);
      return ZX_OK;
    case CLK_TYPE_PLL_DIV:
      *out_current_rate = bcm2835_pll_divider_get_rate(index);
      return ZX_OK;
    case CLK_TYPE_CLK:
      *out_current_rate = bcm2835_clock_get_rate(index);
      return ZX_OK;
    case CLK_TYPE_FW:
      return raspberrypi_fw_get_rate(bcm2835_clk_descs[index].data.fw_clk_id, out_current_rate);
  }
  return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t Bcm2835Clk::ClockImplSetRate(uint32_t index, uint64_t hz) {
  if (index >= BCM2835_CLOCK_COUNT) {
    return ZX_ERR_INVALID_ARGS;
  }
  
  switch (bcm2835_clk_descs[index].type) {
    case CLK_TYPE_PLL:
      return bcm2835_pll_set_rate(index, hz);
    case CLK_TYPE_PLL_DIV:
      return bcm2835_pll_divider_set_rate(index, hz);
    case CLK_TYPE_CLK:
      return bcm2835_clock_set_rate(index, hz);
    case CLK_TYPE_FW:
      return raspberrypi_fw_set_rate(bcm2835_clk_descs[index].data.fw_clk_id, hz);
  }
  return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t Bcm2835Clk::ClockImplSetInput(uint32_t index, uint32_t idx) {
  if (index >= BCM2835_CLOCK_COUNT) {
    return ZX_ERR_INVALID_ARGS;
  }
  
  if (bcm2835_clk_descs[index].type == CLK_TYPE_CLK) {
    if (idx >= bcm2835_clk_descs[index].data.clock_data.num_mux_parents) {
      return ZX_ERR_INVALID_ARGS;
    }

    bcm2835_clock_set_parent(index, idx);
    return ZX_OK;
  }
  return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t Bcm2835Clk::ClockImplGetNumInputs(uint32_t index, uint32_t* out) {
  if (index >= BCM2835_CLOCK_COUNT) {
    return ZX_ERR_INVALID_ARGS;
  }
  
  if (bcm2835_clk_descs[index].type == CLK_TYPE_CLK) {
    return bcm2835_clk_descs[index].data.clock_data.num_mux_parents;
  }
  return ZX_ERR_NOT_SUPPORTED;
}

zx_status_t Bcm2835Clk::ClockImplGetInput(uint32_t index, uint32_t* out) {
  if (index >= BCM2835_CLOCK_COUNT) {
    return ZX_ERR_INVALID_ARGS;
  }
  
  if (bcm2835_clk_descs[index].type == CLK_TYPE_CLK) {
    *out = bcm2835_clock_get_parent(index);
    return ZX_OK;
  }
  return ZX_ERR_NOT_SUPPORTED;
}

void Bcm2835Clk::DdkUnbind(ddk::UnbindTxn txn) {
  fbl::AutoLock lock(&mmio_lock_);
  mmio_.reset();
  txn.Reply();
}

}  // namespace clk

static constexpr zx_driver_ops_t bcm2835_clk_ops = []() {
  zx_driver_ops_t ops = {};
  ops.version = DRIVER_OPS_VERSION;
  ops.bind = clk::Bcm2835Clk::Create;
  return ops;
}();

ZIRCON_DRIVER(bcm2835_clk, bcm2835_clk_ops, "zircon", "0.1");
