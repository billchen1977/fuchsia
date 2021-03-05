// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "rpi-mailbox.h"

#include <fbl/algorithm.h>
#include <fbl/alloc_checker.h>
#include <fbl/auto_lock.h>
#include <lib/zircon-internal/align.h>
#include <lib/zx/vmar.h>
#include <lib/zx/vmo.h>

#include <soc/broadcom/rpi-firmware.h>

#include "src/devices/scpi/drivers/rpi-mailbox/rpi-mailbox-bind.h"

namespace mailbox {

void RpiMailbox::DdkUnbind(ddk::UnbindTxn txn) { txn.Reply(); }

void RpiMailbox::DdkRelease() { delete this; }

zx_status_t RpiMailbox::MailboxSendCommand(const mailbox_channel_t* channel,
                                           const mailbox_data_buf_t* mdata) {
  if (!channel || !mdata) {
    return ZX_ERR_INVALID_ARGS;
  }

  if (channel->mailbox >= MAILBOX_COUNT || channel->rx_size & 3 || mdata->tx_size & 3) {
    return ZX_ERR_INVALID_ARGS;
  }

  fbl::AutoLock mailbox_lock(&mailbox_chan_lock_[channel->mailbox]);

  zx::vmo vmo;
  zx::pmt pmt;
  zx_paddr_t phys;
  uint32_t* vbuf;
  uint32_t wdata, rdata;
  uint32_t size = 6 * sizeof(uint32_t) + mdata->tx_size;
  zx_status_t status = zx::vmo::create_contiguous(bti_, ZX_ROUNDUP_PAGE_SIZE(size), 0, &vmo);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: could not create VMO: %d", __FILE__, status);
    return status;
  }

  status = bti_.pin(ZX_BTI_PERM_READ | ZX_BTI_PERM_WRITE, vmo, 0, ZX_ROUNDUP_PAGE_SIZE(size),
                    &phys, 1, &pmt);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: could not pin VMO: %d", __FILE__, status);
    return status;
  }

  status = zx::vmar::root_self()->map(ZX_VM_PERM_READ | ZX_VM_PERM_WRITE, 0, vmo, 0,
                                      ZX_ROUNDUP_PAGE_SIZE(size), reinterpret_cast<zx_vaddr_t*>(&vbuf));
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: could not map VMO: %d", __FILE__, status);
    goto end;
  }

  vbuf[0] = size;
  vbuf[1] = RPI_FIRMWARE_STATUS_REQUEST;
  vbuf[2] = mdata->cmd;        // tag
  vbuf[3] = mdata->tx_size;    // buf_size
  vbuf[4] = 0;                 // req_resp_size
  memcpy(&vbuf[5], mdata->tx_buffer, mdata->tx_size);
  vbuf[5 + mdata->tx_size / 4] = RPI_FIRMWARE_PROPERTY_END;
  vmo.op_range(ZX_VMO_OP_CACHE_CLEAN, 0, size, nullptr, 0);

  while (mailbox_mmio_->Read32(channel->mailbox * MAILBOX_SIZE + MAILBOX1_STA) & MAILBOX_FULL);
  wdata = MAILBOX_MSG(MAILBOX_CHAN_PROPERTY, phys);
  mailbox_mmio_->Write32(wdata, channel->mailbox * MAILBOX_SIZE + MAILBOX1_WRT);

  mailbox_mmio_->Write32(MAILBOX_HAVEDATA_IRQEN, channel->mailbox * MAILBOX_SIZE + MAILBOX0_CNF);
  status = inth_[channel->mailbox].wait(nullptr);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: zx_interrupt_wait failed: %d", __FILE__, status);
    goto end;
  }
  mailbox_mmio_->Write32(0, channel->mailbox * MAILBOX_SIZE + MAILBOX0_CNF);

  while (mailbox_mmio_->Read32(channel->mailbox * MAILBOX_SIZE + MAILBOX0_STA) & MAILBOX_EMPTY);
  rdata = mailbox_mmio_->Read32(channel->mailbox * MAILBOX_SIZE + MAILBOX0_RD);
  if (rdata != wdata) {
    zxlogf(ERROR, "%s: protocol mismatch 0x%08x != 0x%08x", __FILE__, rdata, wdata);
    status = ZX_ERR_INTERNAL;
    goto end;
  }

  vmo.op_range(ZX_VMO_OP_CACHE_INVALIDATE, 0, size, nullptr, 0);
  if (vbuf[1] == RPI_FIRMWARE_STATUS_SUCCESS && vbuf[4] == (RPI_FIRMWARE_STATUS_SUCCESS | channel->rx_size)) {
    memcpy((void *)channel->rx_buffer, &vbuf[5], channel->rx_size);
    status = ZX_OK;
  } else {
    status = ZX_ERR_INTERNAL;
  }

end:
  zx::vmar::root_self()->unmap((uintptr_t)vbuf, ZX_ROUNDUP_PAGE_SIZE(size));
  pmt.unpin();
  return status;
}

zx_status_t RpiMailbox::Bind() {
  zx_status_t status;
  if ((status = DdkAdd("rpi-mailbox")) != ZX_OK) {
    zxlogf(ERROR, "%s: DdkAdd failed: %d", __FILE__, status);
    return status;
  }

  return ZX_OK;
}

zx_status_t RpiMailbox::InitPdev() {
  if (!pdev_.is_valid()) {
    return ZX_ERR_NO_RESOURCES;
  }

  zx_status_t status = pdev_.MapMmio(0, &mailbox_mmio_);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: could not map mailbox mmio: %d", __FILE__, status);
    return status;
  }

  status = pdev_.GetBti(0, &bti_);
  if (status != ZX_OK) {
    zxlogf(ERROR, "%s: could not get BTI handle: %d", __FILE__, status);
    return status;
  }

  for (uint32_t i = 0; i < MAILBOX_COUNT; i++) {
    status = pdev_.GetInterrupt(i, &inth_[i]);
    if (status != ZX_OK) {
      zxlogf(ERROR, "%s: could not map interrupt: %d", __FILE__, status);
      return status;
    }

    mtx_init(&mailbox_chan_lock_[i], mtx_plain);
  }

  return status;
}

zx_status_t RpiMailbox::Create(void* ctx, zx_device_t* parent) {
  fbl::AllocChecker ac;
  auto mailbox_device = fbl::make_unique_checked<RpiMailbox>(&ac, parent);
  if (!ac.check()) {
    return ZX_ERR_NO_MEMORY;
  }

  zx_status_t status = mailbox_device->InitPdev();
  if (status != ZX_OK) {
    return status;
  }

  status = mailbox_device->Bind();
  if (status != ZX_OK) {
    zxlogf(ERROR, "rpi-mailbox driver failed to get added: %d", status);
    return status;
  } else {
    zxlogf(INFO, "rpi-mailbox driver added");
  }

  // mailbox_device intentionally leaked as it is now held by DevMgr
  __UNUSED auto ptr = mailbox_device.release();

  return ZX_OK;
}

}  // namespace mailbox

static constexpr zx_driver_ops_t rpi_mailbox_ops = []() {
  zx_driver_ops_t ops = {};
  ops.version = DRIVER_OPS_VERSION;
  ops.bind = mailbox::RpiMailbox::Create;
  return ops;
}();

ZIRCON_DRIVER(rpi_mailbox, rpi_mailbox_ops, "zircon", "0.1");
