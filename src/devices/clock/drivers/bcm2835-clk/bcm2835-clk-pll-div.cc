// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bcm2835-clk.h"

#include <algorithm>

#include <fbl/auto_lock.h>

namespace clk {

bool Bcm2835Clk::bcm2835_pll_divider_is_on(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.pll_divider_data;

  fbl::AutoLock al(&mmio_lock_);
  return !(mmio_.Read32(data->a2w_reg) & A2W_PLL_CHANNEL_DISABLE);
}

uint64_t Bcm2835Clk::bcm2835_pll_divider_round_rate(uint32_t index, uint64_t rate)
{
  auto data = &bcm2835_clk_descs[index].data.pll_divider_data;
  uint64_t parent_rate = bcm2835_pll_get_rate(data->source_pll);
  uint32_t div, max_div = 1 << A2W_PLL_DIV_BITS;

  div = (parent_rate + rate - 1) / rate;
  div = std::min(div, max_div);
  return parent_rate / div;
}

uint64_t Bcm2835Clk::bcm2835_pll_divider_get_rate(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.pll_divider_data;
  uint64_t parent_rate = bcm2835_pll_get_rate(data->source_pll);

  fbl::AutoLock al(&mmio_lock_);
  uint32_t div = mmio_.Read32(data->a2w_reg) & A2W_PLL_DIV_MASK;
  if (div == 0)
    div = 1 << A2W_PLL_DIV_BITS;
  return parent_rate / div;
}

zx_status_t Bcm2835Clk::bcm2835_pll_divider_off(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.pll_divider_data;

  fbl::AutoLock al(&mmio_lock_);
  mmio_.ModifyBits32(data->hold_mask, data->load_mask | data->hold_mask, data->cm_reg);
  mmio_.SetBits32(A2W_PLL_CHANNEL_DISABLE, data->a2w_reg);
  return ZX_OK;
}

zx_status_t Bcm2835Clk::bcm2835_pll_divider_on(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.pll_divider_data;

  fbl::AutoLock al(&mmio_lock_);
  mmio_.ClearBits32(A2W_PLL_CHANNEL_DISABLE, data->a2w_reg);
  mmio_.ClearBits32(data->hold_mask, data->cm_reg);
  return ZX_OK;
}

zx_status_t Bcm2835Clk::bcm2835_pll_divider_set_rate(uint32_t index, uint64_t rate)
{
  auto data = &bcm2835_clk_descs[index].data.pll_divider_data;
  uint64_t parent_rate = bcm2835_pll_get_rate(data->source_pll);
  uint32_t div, max_div = 1 << A2W_PLL_DIV_BITS;

  div = (parent_rate + rate - 1) / rate;
  div = std::min(div, max_div);
  if (div == max_div)
    div = 0;

  fbl::AutoLock al(&mmio_lock_);
  mmio_.Write32(div, data->a2w_reg);
  mmio_.SetBits32(data->load_mask, data->cm_reg);
  mmio_.ClearBits32(data->load_mask, data->cm_reg);
  return ZX_OK;
}

}  // namespace clk
