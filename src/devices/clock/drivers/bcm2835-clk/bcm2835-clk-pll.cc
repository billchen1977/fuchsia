// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bcm2835-clk.h"

#include <thread>

#include <fbl/auto_lock.h>

namespace clk {

static uint32_t bcm2835_pll_get_prediv_mask(const struct bcm2835_pll_data *data)
{
  /*
   * On BCM2711 there isn't a pre-divisor available in the PLL feedback
   * loop. Bits 13:14 of ANA1 (PLLA,PLLB,PLLC,PLLD) have been re-purposed
   * for to for VCO RANGE bits.
  */
  return 0;
  //return data->ana->fb_prediv_mask;
}

static void bcm2835_pll_choose_ndiv_and_fdiv(uint64_t rate,
					       uint64_t parent_rate,
					       uint32_t *ndiv, uint32_t *fdiv)
{
  uint64_t div = (rate << A2W_PLL_FRAC_BITS) / parent_rate;

  *ndiv = (uint32_t)(div >> A2W_PLL_FRAC_BITS);
  *fdiv = (uint32_t)(div & ((1 << A2W_PLL_FRAC_BITS) - 1));
}

static uint64_t bcm2835_pll_rate_from_divisors(uint64_t parent_rate,
					         uint32_t ndiv, uint32_t fdiv, uint32_t pdiv)
{
  if (pdiv == 0)
    return 0;

  uint64_t rate = parent_rate * ((ndiv << A2W_PLL_FRAC_BITS) + fdiv) / pdiv;
  return rate >> A2W_PLL_FRAC_BITS;
}

bool Bcm2835Clk::bcm2835_pll_is_on(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.pll_data;

  fbl::AutoLock al(&mmio_lock_);
  return mmio_.Read32(data->a2w_ctrl_reg) & A2W_PLL_CTRL_PRST_DISABLE;
}

uint64_t Bcm2835Clk::bcm2835_pll_round_rate(uint32_t index, uint64_t rate)
{
  auto data = &bcm2835_clk_descs[index].data.pll_data;
  uint64_t parent_rate = BCM2711_OSC_RATE;
  uint32_t ndiv, fdiv;

  if (rate < data->min_rate)
    rate = data->min_rate;
  else if (rate > data->max_rate)
    rate = data->max_rate;

  bcm2835_pll_choose_ndiv_and_fdiv(rate, parent_rate, &ndiv, &fdiv);

  return bcm2835_pll_rate_from_divisors(parent_rate, ndiv, fdiv, 1);
}

uint64_t Bcm2835Clk::bcm2835_pll_get_rate(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.pll_data;
  uint64_t parent_rate = BCM2711_OSC_RATE;
  uint32_t a2wctrl;
  uint32_t ndiv, pdiv, fdiv;
  bool using_prediv;

  {
    fbl::AutoLock al(&mmio_lock_);

    fdiv = mmio_.Read32(data->frac_reg) & A2W_PLL_FRAC_MASK;
    a2wctrl = mmio_.Read32(data->a2w_ctrl_reg);
    ndiv = (a2wctrl & A2W_PLL_CTRL_NDIV_MASK) >> A2W_PLL_CTRL_NDIV_SHIFT;
    pdiv = (a2wctrl & A2W_PLL_CTRL_PDIV_MASK) >> A2W_PLL_CTRL_PDIV_SHIFT;
    using_prediv = mmio_.Read32(data->ana_reg_base + 4) & bcm2835_pll_get_prediv_mask(data);
  }

  if (using_prediv) {
    ndiv *= 2;
    fdiv *= 2;
  }

  return bcm2835_pll_rate_from_divisors(parent_rate, ndiv, fdiv, pdiv);
}

zx_status_t Bcm2835Clk::bcm2835_pll_off(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.pll_data;

  fbl::AutoLock al(&mmio_lock_);
  mmio_.Write32(CM_PLL_ANARST, data->cm_ctrl_reg);
  mmio_.SetBits32(A2W_PLL_CTRL_PWRDN, data->a2w_ctrl_reg);
  return ZX_OK;
}

zx_status_t Bcm2835Clk::bcm2835_pll_on(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.pll_data;

  fbl::AutoLock al(&mmio_lock_);
  mmio_.ClearBits32(A2W_PLL_CTRL_PWRDN, data->a2w_ctrl_reg);
  mmio_.ClearBits32(CM_PLL_ANARST, data->cm_ctrl_reg);

  /* Wait for the PLL to lock. */
  if (data->lock_mask) {
    zx_time_t deadline = zx_deadline_after(ZX_NSEC(LOCK_TIMEOUT_NS));
    while (!(mmio_.Read32(CM_LOCK) & data->lock_mask)) {
      if (deadline <= zx_clock_get_monotonic()) {
        mmio_lock_.Release();
        zxlogf(ERROR, "%s: Couldn't lock PLL", __func__);
	return ZX_ERR_TIMED_OUT;
      }

      std::this_thread::yield();
    }
  }

  mmio_.SetBits32(A2W_PLL_CTRL_PRST_DISABLE, data->a2w_ctrl_reg);

  return ZX_OK;
}

void Bcm2835Clk::bcm2835_pll_write_ana(uint32_t ana_reg_base, uint32_t *ana)
{
  /*
   * ANA register setup is done as a series of writes to
   * ANA3-ANA0, in that order.  This lets us write all 4
   * registers as a single cycle of the serdes interface (taking
   * 100 xosc clocks), whereas if we were to update ana0, 1, and
   * 3 individually through their partial-write registers, each
   * would be their own serdes cycle.
   */
  for (int i = 3; i >= 0; i--)
    mmio_.Write32(ana[i], ana_reg_base + i * 4);
}

zx_status_t Bcm2835Clk::bcm2835_pll_set_rate(uint32_t index, uint64_t rate)
{
  auto data = &bcm2835_clk_descs[index].data.pll_data;
  uint64_t parent_rate = BCM2711_OSC_RATE;
  uint32_t prediv_mask = bcm2835_pll_get_prediv_mask(data);
  bool was_using_prediv, use_fb_prediv, do_ana_setup_first;
  uint32_t ndiv, fdiv, a2w_ctl;
  uint32_t ana[4];
  int i;

  if (rate > data->max_fb_rate) {
    use_fb_prediv = true;
    rate /= 2;
  } else {
    use_fb_prediv = false;
  }

  bcm2835_pll_choose_ndiv_and_fdiv(rate, parent_rate, &ndiv, &fdiv);

  fbl::AutoLock al(&mmio_lock_);

  for (i = 3; i >= 0; i--)
    ana[i] = mmio_.Read32(data->ana_reg_base + i * 4);

  was_using_prediv = ana[1] & prediv_mask;

  ana[0] &= ~data->ana->mask0;
  ana[0] |= data->ana->set0;
  ana[1] &= ~data->ana->mask1;
  ana[1] |= data->ana->set1;
  ana[3] &= ~data->ana->mask3;
  ana[3] |= data->ana->set3;

  if (was_using_prediv && !use_fb_prediv) {
    ana[1] &= ~prediv_mask;
    do_ana_setup_first = true;
  } else if (!was_using_prediv && use_fb_prediv) {
    ana[1] |= prediv_mask;
    do_ana_setup_first = false;
  } else {
    do_ana_setup_first = true;
  }

  /* Unmask the reference clock from the oscillator. */
  mmio_.SetBits32(data->reference_enable_mask, A2W_XOSC_CTRL);

  if (do_ana_setup_first)
    bcm2835_pll_write_ana(data->ana_reg_base, ana);

  /* Set the PLL multiplier from the oscillator. */
  mmio_.Write32(fdiv, data->frac_reg);

  a2w_ctl = mmio_.Read32(data->a2w_ctrl_reg);
  a2w_ctl &= ~A2W_PLL_CTRL_NDIV_MASK;
  a2w_ctl |= ndiv << A2W_PLL_CTRL_NDIV_SHIFT;
  a2w_ctl &= ~A2W_PLL_CTRL_PDIV_MASK;
  a2w_ctl |= 1 << A2W_PLL_CTRL_PDIV_SHIFT;
  mmio_.Write32(a2w_ctl, data->a2w_ctrl_reg);

  if (!do_ana_setup_first)
    bcm2835_pll_write_ana(data->ana_reg_base, ana);

  return ZX_OK;
}

}  // namespace clk
