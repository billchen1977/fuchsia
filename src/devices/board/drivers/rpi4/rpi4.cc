// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/devices/board/drivers/rpi4/rpi4.h"

#include <zircon/status.h>
#include <zircon/threads.h>

#include <ddk/debug.h>
#include <ddk/platform-defs.h>
#include <fbl/alloc_checker.h>

#include "src/devices/board/drivers/rpi4/rpi4-bind.h"

namespace rpi4 {

int Rpi4::Thread() {
  zxlogf(INFO, "%s: Board %s Start", __func__, board_info_.board_name);

  auto status = GpioInit();
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: GpioInit() failed: %s", __func__, zx_status_get_string(status));
    return thrd_error;
  }

  status = MailboxInit();
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: MailboxInit() failed: %s", __func__, zx_status_get_string(status));
    return thrd_error;
  }

  return ZX_OK;
}

zx_status_t Rpi4::Start() {
  int rc = thrd_create_with_name(
      &thread_, [](void* arg) -> int { return reinterpret_cast<Rpi4*>(arg)->Thread(); }, this,
      "rpi4-start-thread");
  if (rc != thrd_success) {
    return ZX_ERR_INTERNAL;
  }
  return ZX_OK;
}

zx_status_t Rpi4::Create(void* ctx, zx_device_t* parent) {
  ddk::PBusProtocolClient pbus(parent);
  if (!pbus.is_valid()) {
    zxlogf(ERROR, "%s: Failed to get ZX_PROTOCOL_PBUS", __func__);
    return ZX_ERR_NO_RESOURCES;
  }

  pdev_board_info_t board_info;
  zx_status_t status = pbus.GetBoardInfo(&board_info);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: Failed to get board info: %s", __func__, zx_status_get_string(status));
    return status;
  }

  fbl::AllocChecker ac;
  auto board = fbl::make_unique_checked<Rpi4>(&ac, parent, pbus, board_info);
  if (!ac.check()) {
    return ZX_ERR_NO_MEMORY;
  }

  if ((status = board->DdkAdd("rpi4", DEVICE_ADD_NON_BINDABLE)) != ZX_OK) {
    zxlogf(ERROR, "%s: DdkAdd failed %s", __func__, zx_status_get_string(status));
    return status;
  }

  if ((status = board->Start()) != ZX_OK) {
    return status;
  }

  __UNUSED auto* dummy = board.release();
  return ZX_OK;
}

static zx_driver_ops_t rpi4_driver_ops = []() {
  zx_driver_ops_t ops = {};
  ops.version = DRIVER_OPS_VERSION;
  ops.bind = Rpi4::Create;
  return ops;
}();

}  // namespace rpi4

ZIRCON_DRIVER(rpi4_bus, rpi4::rpi4_driver_ops, "zircon", "0.1");
