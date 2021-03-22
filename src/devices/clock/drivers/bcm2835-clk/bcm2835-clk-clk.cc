// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bcm2835-clk.h"

#include <algorithm>
#include <thread>

#include <fbl/auto_lock.h>
#include <zircon/time.h>

namespace clk {

static uint32_t bcm2835_clock_choose_div(const struct bcm2835_clock_data *data,
				          uint64_t rate,
				          uint64_t parent_rate,
				          bool round_up)
{
  uint32_t unused_frac_mask = GENMASK(0, CM_DIV_FRAC_BITS - data->frac_bits);
  uint64_t temp = (uint64_t)parent_rate << CM_DIV_FRAC_BITS;
  uint64_t rem;
  uint32_t div, mindiv, maxdiv;

  div = temp / rate;
  rem = temp % rate;

  /* Round up and mask off the unused bits */
  if (round_up && ((div & unused_frac_mask) != 0 || rem != 0))
    div += unused_frac_mask + 1;
  div &= ~unused_frac_mask;

  /* different clamping limits apply for a mash clock */
  if (data->is_mash_clock) {
    /* clamp to min divider of 2 */
    mindiv = 2 << CM_DIV_FRAC_BITS;
    /* clamp to the highest possible integer divider */
    maxdiv = GENMASK(CM_DIV_FRAC_BITS, data->int_bits);
  } else {
    /* clamp to min divider of 1 */
    mindiv = 1 << CM_DIV_FRAC_BITS;
    /* clamp to the highest possible fractional divider */
    maxdiv = GENMASK(CM_DIV_FRAC_BITS - data->frac_bits, data->int_bits + data->frac_bits);
  }

  /* apply the clamping  limits */
  div = std::max(div, mindiv);
  div = std::min(div, maxdiv);

  return div;
}

static uint64_t bcm2835_clock_rate_from_divisor(const struct bcm2835_clock_data *data,
					          uint64_t parent_rate,
					          uint32_t div)
{
  if (data->int_bits == 0 && data->frac_bits == 0)
    return parent_rate;

  /*
   * The divisor is a 12.12 fixed point field, but only some of
   * the bits are populated in any given clock.
   */
  div >>= CM_DIV_FRAC_BITS - data->frac_bits;
  div &= (1 << (data->int_bits + data->frac_bits)) - 1;

  if (div == 0)
    return 0;

  return (parent_rate << data->frac_bits) / div;
}

static bool bcm2835_clk_is_pllc(uint32_t index)
{
  return (index >= BCM2835_PLLC_CORE0) && (index <= BCM2835_PLLC_PER);
}

bool Bcm2835Clk::bcm2835_clock_is_on(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.clock_data;

  return mmio_.Read32(data->ctl_reg) & CM_ENABLE;
}

uint64_t Bcm2835Clk::bcm2835_clock_get_parent_rate(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.clock_data;
  uint32_t parent = bcm2835_clock_get_parent(index);
  uint64_t parent_rate;
  if (parent >= data->num_mux_parents)
    return 0;
  parent = data->parents[parent];
  if (parent == (uint32_t)-1)
    return 0;
  else if (parent == 0)
    parent_rate = BCM2711_OSC_RATE;
  else
    ClockImplGetRate(parent, &parent_rate);

  return parent_rate;
}

uint64_t Bcm2835Clk::bcm2835_clock_get_rate(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.clock_data;
  uint64_t parent_rate = bcm2835_clock_get_parent_rate(index);

  if (data->int_bits == 0 && data->frac_bits == 0)
    return parent_rate;

  uint32_t div = mmio_.Read32(data->div_reg);
  return bcm2835_clock_rate_from_divisor(data, parent_rate, div);
}

zx_status_t Bcm2835Clk::bcm2835_clock_wait_busy(const struct bcm2835_clock_data *data)
{
  zx_time_t deadline = zx_deadline_after(ZX_NSEC(LOCK_TIMEOUT_NS));
  while (!(mmio_.Read32(data->ctl_reg) & CM_BUSY)) {
    if (deadline <= zx_clock_get_monotonic()) {
      zxlogf(ERROR, "%s: Couldn't lock PLL", __func__);
      return ZX_ERR_TIMED_OUT;
    }

    std::this_thread::yield();
  }
  return ZX_OK;
}

zx_status_t Bcm2835Clk::bcm2835_clock_off(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.clock_data;

  {
    fbl::AutoLock al(&mmio_lock_);
    mmio_.ClearBits32(CM_ENABLE, data->ctl_reg);
  }

  /* BUSY will remain high until the divider completes its cycle. */
  return bcm2835_clock_wait_busy(data);
}

zx_status_t Bcm2835Clk::bcm2835_clock_on(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.clock_data;

  fbl::AutoLock al(&mmio_lock_);
  mmio_.SetBits32(CM_ENABLE | CM_GATE, data->ctl_reg);
  return ZX_OK;
}

zx_status_t Bcm2835Clk::bcm2835_clock_set_rate(uint32_t index, uint64_t rate)
{
  auto data = &bcm2835_clk_descs[index].data.clock_data;
  uint64_t parent_rate = bcm2835_clock_get_parent_rate(index);
  uint32_t div = bcm2835_clock_choose_div(data, rate, parent_rate, false);
  uint32_t ctl;

  /* If the clock is running, we have to pause clock generation while
   * updating the control and div regs.  This is glitchless (no clock
   * signals generated faster than the rate) but each reg access is two
   * OSC cycles so the clock will slow down for a moment.
   */
  bcm2835_clock_off(index);

  fbl::AutoLock al(&mmio_lock_);
  ctl = mmio_.Read32(data->ctl_reg);

  if (div & CM_DIV_FRAC_MASK)
    ctl |= CM_FRAC;
  else
    ctl &= ~CM_FRAC;
  mmio_.Write32(ctl, data->ctl_reg);

  mmio_.Write32(div, data->div_reg);
  return ZX_OK;
}

uint64_t Bcm2835Clk::bcm2835_clock_choose_div_and_prate(const struct bcm2835_clock_data *data,
                                                        int parent, uint64_t rate)
{
  uint64_t parent_rate;
  uint64_t best_rate;
  uint32_t div;

  if (parent == 0)
    parent_rate = BCM2711_OSC_RATE;
  else
    ClockImplGetRate(parent, &parent_rate);

  div = bcm2835_clock_choose_div(data, rate, parent_rate, true);

  best_rate = bcm2835_clock_rate_from_divisor(data, parent_rate, div);

  if (data->low_jitter && (div & CM_DIV_FRAC_MASK)) {
    uint64_t high, low;
    uint32_t int_div = div & ~CM_DIV_FRAC_MASK;

    high = bcm2835_clock_rate_from_divisor(data, parent_rate, int_div);
    int_div += CM_DIV_FRAC_MASK + 1;
    low = bcm2835_clock_rate_from_divisor(data, parent_rate, int_div);

    /*
     * Return a value which is the maximum deviation
     * below the ideal rate, for use as a metric.
     */
    return best_rate - std::max(best_rate - low, high - best_rate);
  }
  return best_rate;
}

uint64_t Bcm2835Clk::bcm2835_clock_determine_rate(uint32_t index, uint64_t rate)
{
  auto data = &bcm2835_clk_descs[index].data.clock_data;
  uint64_t r, best_rate = 0;

  bool current_parent_is_pllc = bcm2835_clk_is_pllc(index);

  /*
   * Select parent clock that results in the closest but lower rate
   */
  for (uint32_t i = 0; i < data->num_mux_parents; ++i) {
    uint32_t parent = data->parents[i];
    if (parent == (uint32_t)-1)
      continue;

    /*
     * Don't choose a PLLC-derived clock as our parent
     * unless it had been manually set that way.  PLLC's
     * frequency gets adjusted by the firmware due to
     * over-temp or under-voltage conditions, without
     * prior notification to our clock consumer.
     */
    if (bcm2835_clk_is_pllc(parent) && !current_parent_is_pllc)
      continue;

    r = bcm2835_clock_choose_div_and_prate(data, parent, rate);
    if (r > best_rate && r <= rate)
      best_rate = r;
  }

  return best_rate;
}

void Bcm2835Clk::bcm2835_clock_set_parent(uint32_t index, uint32_t idx)
{
  auto data = &bcm2835_clk_descs[index].data.clock_data;
  uint32_t src = (idx << CM_SRC_SHIFT) & CM_SRC_MASK;

  fbl::AutoLock al(&mmio_lock_);
  mmio_.Write32(src, data->ctl_reg);
}

uint32_t Bcm2835Clk::bcm2835_clock_get_parent(uint32_t index)
{
  auto data = &bcm2835_clk_descs[index].data.clock_data;
  uint32_t src;

  fbl::AutoLock al(&mmio_lock_);
  src = mmio_.Read32(data->ctl_reg);
  return (src & CM_SRC_MASK) >> CM_SRC_SHIFT;
}

}  // namespace clk
