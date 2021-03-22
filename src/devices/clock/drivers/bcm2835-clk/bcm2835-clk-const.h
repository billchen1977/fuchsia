// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVICES_CLOCK_DRIVERS_BCM2835_CLK_BCM2835_CLK_CONST_H_
#define SRC_DEVICES_CLOCK_DRIVERS_BCM2835_CLK_BCM2835_CLK_CONST_H_

#include "bcm2835-clk-internal.h"

static const struct bcm2835_pll_ana_bits bcm2835_ana_default = {
  .mask0 = 0,
  .set0 = 0,
  .mask1 = A2W_PLL_KI_MASK | A2W_PLL_KP_MASK,
  .set1 = (2 << A2W_PLL_KI_SHIFT) | (8 << A2W_PLL_KP_SHIFT),
  .mask3 = A2W_PLL_KA_MASK,
  .set3 = (2 << A2W_PLL_KA_SHIFT),
  .fb_prediv_mask = BIT(14),
};

static const struct bcm2835_pll_ana_bits bcm2835_ana_pllh = {
  .mask0 = A2W_PLLH_KA_MASK | A2W_PLLH_KI_LOW_MASK,
  .set0 = (2 << A2W_PLLH_KA_SHIFT) | (2 << A2W_PLLH_KI_LOW_SHIFT),
  .mask1 = A2W_PLLH_KI_HIGH_MASK | A2W_PLLH_KP_MASK,
  .set1 = (6 << A2W_PLLH_KP_SHIFT),
  .mask3 = 0,
  .set3 = 0,
  .fb_prediv_mask = BIT(11),
};

/* assignment helper macros for different clock types */
#define _REGISTER(f, ...) { .type = f, \
			     __VA_ARGS__ }
#define REGISTER_PLL(...)      _REGISTER(CLK_TYPE_PLL,	\
					  .data.pll_data = {__VA_ARGS__})
#define REGISTER_PLL_DIV(...)  _REGISTER(CLK_TYPE_PLL_DIV, \
					  .data.pll_divider_data = {__VA_ARGS__})
#define REGISTER_CLK(...)	_REGISTER(CLK_TYPE_CLK,	\
					  .data.clock_data = {__VA_ARGS__})
#define REGISTER_FW(id)	_REGISTER(CLK_TYPE_FW,		\
                                         .data.fw_clk_id = id)

/* parent mux arrays plus helper macros */

/* main oscillator parent mux */
static const uint32_t bcm2835_clock_osc_parents[] = {
  (uint32_t)-1,	//"gnd",
  0,			//"xosc",
  (uint32_t)-1,	//"testdebug0",
  (uint32_t)-1,	//"testdebug1"
};

#define REGISTER_OSC_CLK(...)	REGISTER_CLK(			\
	.num_mux_parents = countof(bcm2835_clock_osc_parents),	\
	.parents = bcm2835_clock_osc_parents,				\
	__VA_ARGS__)

/* main peripherial parent mux */
static const uint32_t bcm2835_clock_per_parents[] = {
  (uint32_t)-1,	//"gnd",
  0,			//"xosc",
  (uint32_t)-1,	//"testdebug0",
  (uint32_t)-1,	//"testdebug1",
  BCM2835_PLLA_PER,	//"plla_per",
  BCM2835_PLLC_PER,	//"pllc_per",
  BCM2835_PLLD_PER,	//"plld_per",
  BCM2835_PLLH_AUX,	//"pllh_aux",
};

#define REGISTER_PER_CLK(...)	REGISTER_CLK(			\
	.num_mux_parents = countof(bcm2835_clock_per_parents),	\
	.parents = bcm2835_clock_per_parents,				\
	__VA_ARGS__)

/* main vpu parent mux */
static const uint32_t bcm2835_clock_vpu_parents[] = {
  (uint32_t)-1,	//"gnd",
  0,			//"xosc",
  (uint32_t)-1,	//"testdebug0",
  (uint32_t)-1,	//"testdebug1",
  BCM2835_PLLA_CORE,	//"plla_core",
  BCM2835_PLLC_CORE0,	//"pllc_core0",
  BCM2835_PLLD_CORE,	//"plld_core",
  BCM2835_PLLH_AUX,	//"pllh_aux",
  BCM2835_PLLC_CORE1,	//"pllc_core1",
  BCM2835_PLLC_CORE2,	//"pllc_core2",
};

#define REGISTER_VPU_CLK(s, ...)	REGISTER_CLK(			\
	.num_mux_parents = countof(bcm2835_clock_vpu_parents),	\
	.parents = bcm2835_clock_vpu_parents,				\
	__VA_ARGS__)

/*
 * DSI parent clocks.  The DSI byte/DDR/DDR2 clocks come from the DSI
 * analog PHY.  The _inv variants are generated internally to cprman,
 * but we don't use them so they aren't hooked up.
 */
static const uint32_t bcm2835_clock_dsi0_parents[] = {
  (uint32_t)-1,	//"gnd",
  0,			//"xosc",
  (uint32_t)-1,	//"testdebug0",
  (uint32_t)-1,	//"testdebug1",
  (uint32_t)-1,	//"dsi0_ddr",
  (uint32_t)-1,	//"dsi0_ddr_inv",
  (uint32_t)-1,	//"dsi0_ddr2",
  (uint32_t)-1,	//"dsi0_ddr2_inv",
  (uint32_t)-1,	//"dsi0_byte",
  (uint32_t)-1,	//"dsi0_byte_inv",
};

static const uint32_t bcm2835_clock_dsi1_parents[] = {
  (uint32_t)-1,	//"gnd",
  0,			//"xosc",
  (uint32_t)-1,	//"testdebug0",
  (uint32_t)-1,	//"testdebug1",
  (uint32_t)-1,	//"dsi1_ddr",
  (uint32_t)-1,	//"dsi1_ddr_inv",
  (uint32_t)-1,	//"dsi1_ddr2",
  (uint32_t)-1,	//"dsi1_ddr2_inv",
  (uint32_t)-1,	//"dsi1_byte",
  (uint32_t)-1,	//"dsi1_byte_inv",
};

#define REGISTER_DSI0_CLK(...)	REGISTER_CLK(			\
	.num_mux_parents = countof(bcm2835_clock_dsi0_parents),	\
	.parents = bcm2835_clock_dsi0_parents,			\
	__VA_ARGS__)

#define REGISTER_DSI1_CLK(s, ...)	REGISTER_CLK(			\
	.num_mux_parents = countof(bcm2835_clock_dsi1_parents),	\
	.parents = bcm2835_clock_dsi1_parents,			\
	__VA_ARGS__)

/*
 * the real definition of all the pll, pll_dividers and clocks
 * these make use of the above REGISTER_* macros
 */
const struct bcm2835_clk_desc bcm2835_clk_descs[] = {
	/* the PLL + PLL dividers */

	/*
	 * PLLA is the auxiliary PLL, used to drive the CCP2
	 * (Compact Camera Port 2) transmitter clock.
	 *
	 * It is in the PX LDO power domain, which is on when the
	 * AUDIO domain is on.
	 */
	[BCM2835_PLLA]		= REGISTER_PLL(
		.cm_ctrl_reg = CM_PLLA,
		.a2w_ctrl_reg = A2W_PLLA_CTRL,
		.frac_reg = A2W_PLLA_FRAC,
		.ana_reg_base = A2W_PLLA_ANA0,
		.reference_enable_mask = A2W_XOSC_CTRL_PLLA_ENABLE,
		.lock_mask = CM_LOCK_FLOCKA,

		.ana = &bcm2835_ana_default,

		.min_rate = 600000000u,
		.max_rate = 2400000000u,
		.max_fb_rate = BCM2835_MAX_FB_RATE),
	[BCM2835_PLLA_CORE]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLA,
		.cm_reg = CM_PLLA,
		.a2w_reg = A2W_PLLA_CORE,
		.load_mask = CM_PLLA_LOADCORE,
		.hold_mask = CM_PLLA_HOLDCORE,
		.fixed_divider = 1),
	[BCM2835_PLLA_PER]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLA,
		.cm_reg = CM_PLLA,
		.a2w_reg = A2W_PLLA_PER,
		.load_mask = CM_PLLA_LOADPER,
		.hold_mask = CM_PLLA_HOLDPER,
		.fixed_divider = 1),
	[BCM2835_PLLA_DSI0]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLA,
		.cm_reg = CM_PLLA,
		.a2w_reg = A2W_PLLA_DSI0,
		.load_mask = CM_PLLA_LOADDSI0,
		.hold_mask = CM_PLLA_HOLDDSI0,
		.fixed_divider = 1),
	[BCM2835_PLLA_CCP2]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLA,
		.cm_reg = CM_PLLA,
		.a2w_reg = A2W_PLLA_CCP2,
		.load_mask = CM_PLLA_LOADCCP2,
		.hold_mask = CM_PLLA_HOLDCCP2,
		.fixed_divider = 1),

	/* PLLB is used for the ARM's clock. */
	[BCM2835_PLLB]		= REGISTER_PLL(
		.cm_ctrl_reg = CM_PLLB,
		.a2w_ctrl_reg = A2W_PLLB_CTRL,
		.frac_reg = A2W_PLLB_FRAC,
		.ana_reg_base = A2W_PLLB_ANA0,
		.reference_enable_mask = A2W_XOSC_CTRL_PLLB_ENABLE,
		.lock_mask = CM_LOCK_FLOCKB,

		.ana = &bcm2835_ana_default,

		.min_rate = 600000000u,
		.max_rate = 3000000000u,
		.max_fb_rate = BCM2835_MAX_FB_RATE),
	[BCM2835_PLLB_ARM]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLB,
		.cm_reg = CM_PLLB,
		.a2w_reg = A2W_PLLB_ARM,
		.load_mask = CM_PLLB_LOADARM,
		.hold_mask = CM_PLLB_HOLDARM,
		.fixed_divider = 1),

	/*
	 * PLLC is the core PLL, used to drive the core VPU clock.
	 *
	 * It is in the PX LDO power domain, which is on when the
	 * AUDIO domain is on.
	 */
	[BCM2835_PLLC]		= REGISTER_PLL(
		.cm_ctrl_reg = CM_PLLC,
		.a2w_ctrl_reg = A2W_PLLC_CTRL,
		.frac_reg = A2W_PLLC_FRAC,
		.ana_reg_base = A2W_PLLC_ANA0,
		.reference_enable_mask = A2W_XOSC_CTRL_PLLC_ENABLE,
		.lock_mask = CM_LOCK_FLOCKC,

		.ana = &bcm2835_ana_default,

		.min_rate = 600000000u,
		.max_rate = 3000000000u,
		.max_fb_rate = BCM2835_MAX_FB_RATE),
	[BCM2835_PLLC_CORE0]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLC,
		.cm_reg = CM_PLLC,
		.a2w_reg = A2W_PLLC_CORE0,
		.load_mask = CM_PLLC_LOADCORE0,
		.hold_mask = CM_PLLC_HOLDCORE0,
		.fixed_divider = 1),
	[BCM2835_PLLC_CORE1]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLC,
		.cm_reg = CM_PLLC,
		.a2w_reg = A2W_PLLC_CORE1,
		.load_mask = CM_PLLC_LOADCORE1,
		.hold_mask = CM_PLLC_HOLDCORE1,
		.fixed_divider = 1),
	[BCM2835_PLLC_CORE2]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLC,
		.cm_reg = CM_PLLC,
		.a2w_reg = A2W_PLLC_CORE2,
		.load_mask = CM_PLLC_LOADCORE2,
		.hold_mask = CM_PLLC_HOLDCORE2,
		.fixed_divider = 1,),
	[BCM2835_PLLC_PER]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLC,
		.cm_reg = CM_PLLC,
		.a2w_reg = A2W_PLLC_PER,
		.load_mask = CM_PLLC_LOADPER,
		.hold_mask = CM_PLLC_HOLDPER,
		.fixed_divider = 1),

	/*
	 * PLLD is the display PLL, used to drive DSI display panels.
	 *
	 * It is in the PX LDO power domain, which is on when the
	 * AUDIO domain is on.
	 */
	[BCM2835_PLLD]		= REGISTER_PLL(
		.cm_ctrl_reg = CM_PLLD,
		.a2w_ctrl_reg = A2W_PLLD_CTRL,
		.frac_reg = A2W_PLLD_FRAC,
		.ana_reg_base = A2W_PLLD_ANA0,
		.reference_enable_mask = A2W_XOSC_CTRL_DDR_ENABLE,
		.lock_mask = CM_LOCK_FLOCKD,

		.ana = &bcm2835_ana_default,

		.min_rate = 600000000u,
		.max_rate = 2400000000u,
		.max_fb_rate = BCM2835_MAX_FB_RATE),
	[BCM2835_PLLD_CORE]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLD,
		.cm_reg = CM_PLLD,
		.a2w_reg = A2W_PLLD_CORE,
		.load_mask = CM_PLLD_LOADCORE,
		.hold_mask = CM_PLLD_HOLDCORE,
		.fixed_divider = 1),
	/*
	 * VPU firmware assumes that PLLD_PER isn't disabled by the ARM core.
	 * Otherwise this could cause firmware lookups. That's why we mark
	 * it as critical.
	 */
	[BCM2835_PLLD_PER]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLD,
		.cm_reg = CM_PLLD,
		.a2w_reg = A2W_PLLD_PER,
		.load_mask = CM_PLLD_LOADPER,
		.hold_mask = CM_PLLD_HOLDPER,
		.fixed_divider = 1),
	[BCM2835_PLLD_DSI0]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLD,
		.cm_reg = CM_PLLD,
		.a2w_reg = A2W_PLLD_DSI0,
		.load_mask = CM_PLLD_LOADDSI0,
		.hold_mask = CM_PLLD_HOLDDSI0,
		.fixed_divider = 1),
	[BCM2835_PLLD_DSI1]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLD,
		.cm_reg = CM_PLLD,
		.a2w_reg = A2W_PLLD_DSI1,
		.load_mask = CM_PLLD_LOADDSI1,
		.hold_mask = CM_PLLD_HOLDDSI1,
		.fixed_divider = 1),

	/*
	 * PLLH is used to supply the pixel clock or the AUX clock for the
	 * TV encoder.
	 *
	 * It is in the HDMI power domain.
	 */
	[BCM2835_PLLH]		= REGISTER_PLL(
		.cm_ctrl_reg = CM_PLLH,
		.a2w_ctrl_reg = A2W_PLLH_CTRL,
		.frac_reg = A2W_PLLH_FRAC,
		.ana_reg_base = A2W_PLLH_ANA0,
		.reference_enable_mask = A2W_XOSC_CTRL_PLLC_ENABLE,
		.lock_mask = CM_LOCK_FLOCKH,

		.ana = &bcm2835_ana_pllh,

		.min_rate = 600000000u,
		.max_rate = 3000000000u,
		.max_fb_rate = BCM2835_MAX_FB_RATE),
	[BCM2835_PLLH_RCAL]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLH,
		.cm_reg = CM_PLLH,
		.a2w_reg = A2W_PLLH_RCAL,
		.load_mask = CM_PLLH_LOADRCAL,
		.hold_mask = 0,
		.fixed_divider = 10),
	[BCM2835_PLLH_AUX]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLH,
		.cm_reg = CM_PLLH,
		.a2w_reg = A2W_PLLH_AUX,
		.load_mask = CM_PLLH_LOADAUX,
		.hold_mask = 0,
		.fixed_divider = 1),
	[BCM2835_PLLH_PIX]	= REGISTER_PLL_DIV(
		.source_pll = BCM2835_PLLH,
		.cm_reg = CM_PLLH,
		.a2w_reg = A2W_PLLH_PIX,
		.load_mask = CM_PLLH_LOADPIX,
		.hold_mask = 0,
		.fixed_divider = 10),

	/* the clocks */

	/* clocks with oscillator parent mux */

	/* One Time Programmable Memory clock.  Maximum 10Mhz. */
	[BCM2835_CLOCK_OTP]	= REGISTER_OSC_CLK(
		.ctl_reg = CM_OTPCTL,
		.div_reg = CM_OTPDIV,
		.int_bits = 4,
		.frac_bits = 0,
		.tcnt_mux = 6),
	/*
	 * Used for a 1Mhz clock for the system clocksource, and also used
	 * bythe watchdog timer and the camera pulse generator.
	 */
	[BCM2835_CLOCK_TIMER]	= REGISTER_OSC_CLK(
		.ctl_reg = CM_TIMERCTL,
		.div_reg = CM_TIMERDIV,
		.int_bits = 6,
		.frac_bits = 12),
	/*
	 * Clock for the temperature sensor.
	 * Generally run at 2Mhz, max 5Mhz.
	 */
	[BCM2835_CLOCK_TSENS]	= REGISTER_OSC_CLK(
		.ctl_reg = CM_TSENSCTL,
		.div_reg = CM_TSENSDIV,
		.int_bits = 5,
		.frac_bits = 0),
	[BCM2835_CLOCK_TEC]	= REGISTER_OSC_CLK(
		.ctl_reg = CM_TECCTL,
		.div_reg = CM_TECDIV,
		.int_bits = 6,
		.frac_bits = 0),

	/* clocks with vpu parent mux */
	[BCM2835_CLOCK_H264]	= REGISTER_VPU_CLK(
		.ctl_reg = CM_H264CTL,
		.div_reg = CM_H264DIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 1),
	[BCM2835_CLOCK_ISP]	= REGISTER_VPU_CLK(
		.ctl_reg = CM_ISPCTL,
		.div_reg = CM_ISPDIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 2),

	/*
	 * Secondary SDRAM clock.  Used for low-voltage modes when the PLL
	 * in the SDRAM controller can't be used.
	 */
	[BCM2835_CLOCK_SDRAM]	= REGISTER_VPU_CLK(
		.ctl_reg = CM_SDCCTL,
		.div_reg = CM_SDCDIV,
		.int_bits = 6,
		.frac_bits = 0,
		.tcnt_mux = 3),

	/*
	 * CLOCK_V3D is used for v3d clock. Controlled by firmware, see
	 * clk-raspberrypi.c.
	 */

	/*
	 * VPU clock.  This doesn't have an enable bit, since it drives
	 * the bus for everything else, and is special so it doesn't need
	 * to be gated for rate changes.  It is also known as "clk_audio"
	 * in various hardware documentation.
	 */
	[BCM2835_CLOCK_VPU]	= REGISTER_VPU_CLK(
		.ctl_reg = CM_VPUCTL,
		.div_reg = CM_VPUDIV,
		.int_bits = 12,
		.frac_bits = 8,
		.is_vpu_clock = true,
		.tcnt_mux = 5),

	/* clocks with per parent mux */
	[BCM2835_CLOCK_AVEO]	= REGISTER_PER_CLK(
		.ctl_reg = CM_AVEOCTL,
		.div_reg = CM_AVEODIV,
		.int_bits = 4,
		.frac_bits = 0,
		.tcnt_mux = 38),
	[BCM2835_CLOCK_CAM0]	= REGISTER_PER_CLK(
		.ctl_reg = CM_CAM0CTL,
		.div_reg = CM_CAM0DIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 14),
	[BCM2835_CLOCK_CAM1]	= REGISTER_PER_CLK(
		.ctl_reg = CM_CAM1CTL,
		.div_reg = CM_CAM1DIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 15),
	[BCM2835_CLOCK_DFT]	= REGISTER_PER_CLK(
		.ctl_reg = CM_DFTCTL,
		.div_reg = CM_DFTDIV,
		.int_bits = 5,
		.frac_bits = 0),
	[BCM2835_CLOCK_DPI]	= REGISTER_PER_CLK(
		.ctl_reg = CM_DPICTL,
		.div_reg = CM_DPIDIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 17),

	/* Arasan EMMC clock */
	[BCM2835_CLOCK_EMMC]	= REGISTER_PER_CLK(
		.ctl_reg = CM_EMMCCTL,
		.div_reg = CM_EMMCDIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 39),

	/* EMMC2 clock (only available for BCM2711) */
	[BCM2711_CLOCK_EMMC2]	= REGISTER_PER_CLK(
		.ctl_reg = CM_EMMC2CTL,
		.div_reg = CM_EMMC2DIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 42),

	/* General purpose (GPIO) clocks */
	[BCM2835_CLOCK_GP0]	= REGISTER_PER_CLK(
		.ctl_reg = CM_GP0CTL,
		.div_reg = CM_GP0DIV,
		.int_bits = 12,
		.frac_bits = 12,
		.is_mash_clock = true,
		.tcnt_mux = 20),
	[BCM2835_CLOCK_GP1]	= REGISTER_PER_CLK(
		.ctl_reg = CM_GP1CTL,
		.div_reg = CM_GP1DIV,
		.int_bits = 12,
		.frac_bits = 12,
		.is_mash_clock = true,
		.tcnt_mux = 21),
	[BCM2835_CLOCK_GP2]	= REGISTER_PER_CLK(
		.ctl_reg = CM_GP2CTL,
		.div_reg = CM_GP2DIV,
		.int_bits = 12,
		.frac_bits = 12),

	/* HDMI state machine */
	[BCM2835_CLOCK_HSM]	= REGISTER_PER_CLK(
		.ctl_reg = CM_HSMCTL,
		.div_reg = CM_HSMDIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 22),
	[BCM2835_CLOCK_PCM]	= REGISTER_PER_CLK(
		.ctl_reg = CM_PCMCTL,
		.div_reg = CM_PCMDIV,
		.int_bits = 12,
		.frac_bits = 12,
		.is_mash_clock = true,
		.low_jitter = true,
		.tcnt_mux = 23),
	[BCM2835_CLOCK_PWM]	= REGISTER_PER_CLK(
		.ctl_reg = CM_PWMCTL,
		.div_reg = CM_PWMDIV,
		.int_bits = 12,
		.frac_bits = 12,
		.is_mash_clock = true,
		.tcnt_mux = 24),
	[BCM2835_CLOCK_SLIM]	= REGISTER_PER_CLK(
		.ctl_reg = CM_SLIMCTL,
		.div_reg = CM_SLIMDIV,
		.int_bits = 12,
		.frac_bits = 12,
		.is_mash_clock = true,
		.tcnt_mux = 25),
	[BCM2835_CLOCK_SMI]	= REGISTER_PER_CLK(
		.ctl_reg = CM_SMICTL,
		.div_reg = CM_SMIDIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 27),
	[BCM2835_CLOCK_UART]	= REGISTER_PER_CLK(
		.ctl_reg = CM_UARTCTL,
		.div_reg = CM_UARTDIV,
		.int_bits = 10,
		.frac_bits = 12,
		.tcnt_mux = 28),

	/* TV encoder clock.  Only operating frequency is 108Mhz.  */
	[BCM2835_CLOCK_VEC]	= REGISTER_PER_CLK(
		.ctl_reg = CM_VECCTL,
		.div_reg = CM_VECDIV,
		.int_bits = 4,
		.frac_bits = 0,
		.tcnt_mux = 29),

	/* dsi clocks */
	[BCM2835_CLOCK_DSI0E]	= REGISTER_PER_CLK(
		.ctl_reg = CM_DSI0ECTL,
		.div_reg = CM_DSI0EDIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 18),
	[BCM2835_CLOCK_DSI1E]	= REGISTER_PER_CLK(
		.ctl_reg = CM_DSI1ECTL,
		.div_reg = CM_DSI1EDIV,
		.int_bits = 4,
		.frac_bits = 8,
		.tcnt_mux = 19),
	[BCM2835_CLOCK_DSI0P]	= REGISTER_DSI0_CLK(
		.ctl_reg = CM_DSI0PCTL,
		.div_reg = CM_DSI0PDIV,
		.int_bits = 0,
		.frac_bits = 0,
		.tcnt_mux = 12),
	[BCM2835_CLOCK_DSI1P]	= REGISTER_DSI1_CLK(
		.ctl_reg = CM_DSI1PCTL,
		.div_reg = CM_DSI1PDIV,
		.int_bits = 0,
		.frac_bits = 0,
		.tcnt_mux = 13),
	[RPI_FIRMWARE_CLK_EMMC] = REGISTER_FW(RPI_FIRMWARE_EMMC_CLK_ID),
	[RPI_FIRMWARE_CLK_UART] = REGISTER_FW(RPI_FIRMWARE_UART_CLK_ID),
	[RPI_FIRMWARE_CLK_ARM] = REGISTER_FW(RPI_FIRMWARE_ARM_CLK_ID),
	[RPI_FIRMWARE_CLK_CORE] = REGISTER_FW(RPI_FIRMWARE_CORE_CLK_ID),
	[RPI_FIRMWARE_CLK_V3D] = REGISTER_FW(RPI_FIRMWARE_V3D_CLK_ID),
	[RPI_FIRMWARE_CLK_H264] = REGISTER_FW(RPI_FIRMWARE_H264_CLK_ID),
	[RPI_FIRMWARE_CLK_ISP] = REGISTER_FW(RPI_FIRMWARE_ISP_CLK_ID),
	[RPI_FIRMWARE_CLK_SDRAM] = REGISTER_FW(RPI_FIRMWARE_SDRAM_CLK_ID),
	[RPI_FIRMWARE_CLK_PIXEL] = REGISTER_FW(RPI_FIRMWARE_PIXEL_CLK_ID),
	[RPI_FIRMWARE_CLK_PWM] = REGISTER_FW(RPI_FIRMWARE_PWM_CLK_ID),
	[RPI_FIRMWARE_CLK_HEVC] = REGISTER_FW(RPI_FIRMWARE_HEVC_CLK_ID),
	[RPI_FIRMWARE_CLK_EMMC2] = REGISTER_FW(RPI_FIRMWARE_EMMC2_CLK_ID),
	[RPI_FIRMWARE_CLK_M2MC] = REGISTER_FW(RPI_FIRMWARE_M2MC_CLK_ID),
	[RPI_FIRMWARE_CLK_PIXEL_BVB] = REGISTER_FW(RPI_FIRMWARE_PIXEL_BVB_CLK_ID),
};

#endif  // SRC_DEVICES_CLOCK_DRIVERS_BCM2835_CLK_BCM2835_CLK_CONST_H_
