// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVICES_CLOCK_DRIVERS_BCM2835_CLK_BCM2835_CLK_INTERNAL_H_
#define SRC_DEVICES_CLOCK_DRIVERS_BCM2835_CLK_BCM2835_CLK_INTERNAL_H_

#ifndef BIT
# define BIT(b)                (1UL << (b))
#endif

#ifndef GENMASK
# define GENMASK(b, s)         (((1UL << (s)) - 1) << (b))
#endif

#define CM_PASSWORD		0x5a000000

#define CM_GNRICCTL		0x000
#define CM_GNRICDIV		0x004
# define CM_DIV_FRAC_BITS	12
# define CM_DIV_FRAC_MASK	GENMASK(0, CM_DIV_FRAC_BITS)

#define CM_VPUCTL		0x008
#define CM_VPUDIV		0x00c
#define CM_SYSCTL		0x010
#define CM_SYSDIV		0x014
#define CM_PERIACTL		0x018
#define CM_PERIADIV		0x01c
#define CM_PERIICTL		0x020
#define CM_PERIIDIV		0x024
#define CM_H264CTL		0x028
#define CM_H264DIV		0x02c
#define CM_ISPCTL		0x030
#define CM_ISPDIV		0x034
#define CM_V3DCTL		0x038
#define CM_V3DDIV		0x03c
#define CM_CAM0CTL		0x040
#define CM_CAM0DIV		0x044
#define CM_CAM1CTL		0x048
#define CM_CAM1DIV		0x04c
#define CM_CCP2CTL		0x050
#define CM_CCP2DIV		0x054
#define CM_DSI0ECTL		0x058
#define CM_DSI0EDIV		0x05c
#define CM_DSI0PCTL		0x060
#define CM_DSI0PDIV		0x064
#define CM_DPICTL		0x068
#define CM_DPIDIV		0x06c
#define CM_GP0CTL		0x070
#define CM_GP0DIV		0x074
#define CM_GP1CTL		0x078
#define CM_GP1DIV		0x07c
#define CM_GP2CTL		0x080
#define CM_GP2DIV		0x084
#define CM_HSMCTL		0x088
#define CM_HSMDIV		0x08c
#define CM_OTPCTL		0x090
#define CM_OTPDIV		0x094
#define CM_PCMCTL		0x098
#define CM_PCMDIV		0x09c
#define CM_PWMCTL		0x0a0
#define CM_PWMDIV		0x0a4
#define CM_SLIMCTL		0x0a8
#define CM_SLIMDIV		0x0ac
#define CM_SMICTL		0x0b0
#define CM_SMIDIV		0x0b4
/* no definition for 0x0b8  and 0x0bc */
#define CM_TCNTCTL		0x0c0
# define CM_TCNT_SRC1_SHIFT		12
#define CM_TCNTCNT		0x0c4
#define CM_TECCTL		0x0c8
#define CM_TECDIV		0x0cc
#define CM_TD0CTL		0x0d0
#define CM_TD0DIV		0x0d4
#define CM_TD1CTL		0x0d8
#define CM_TD1DIV		0x0dc
#define CM_TSENSCTL		0x0e0
#define CM_TSENSDIV		0x0e4
#define CM_TIMERCTL		0x0e8
#define CM_TIMERDIV		0x0ec
#define CM_UARTCTL		0x0f0
#define CM_UARTDIV		0x0f4
#define CM_VECCTL		0x0f8
#define CM_VECDIV		0x0fc
#define CM_PULSECTL		0x190
#define CM_PULSEDIV		0x194
#define CM_SDCCTL		0x1a8
#define CM_SDCDIV		0x1ac
#define CM_ARMCTL		0x1b0
#define CM_AVEOCTL		0x1b8
#define CM_AVEODIV		0x1bc
#define CM_EMMCCTL		0x1c0
#define CM_EMMCDIV		0x1c4
#define CM_EMMC2CTL		0x1d0
#define CM_EMMC2DIV		0x1d4

/* General bits for the CM_*CTL regs */
# define CM_ENABLE			BIT(4)
# define CM_KILL			BIT(5)
# define CM_GATE_BIT			6
# define CM_GATE			BIT(CM_GATE_BIT)
# define CM_BUSY			BIT(7)
# define CM_BUSYD			BIT(8)
# define CM_FRAC			BIT(9)
# define CM_SRC_SHIFT			0
# define CM_SRC_BITS			4
# define CM_SRC_MASK			0xf
# define CM_SRC_GND			0
# define CM_SRC_OSC			1
# define CM_SRC_TESTDEBUG0		2
# define CM_SRC_TESTDEBUG1		3
# define CM_SRC_PLLA_CORE		4
# define CM_SRC_PLLA_PER		4
# define CM_SRC_PLLC_CORE0		5
# define CM_SRC_PLLC_PER		5
# define CM_SRC_PLLC_CORE1		8
# define CM_SRC_PLLD_CORE		6
# define CM_SRC_PLLD_PER		6
# define CM_SRC_PLLH_AUX		7
# define CM_SRC_PLLC_CORE1		8
# define CM_SRC_PLLC_CORE2		9

#define CM_OSCCOUNT			0x100

#define CM_PLLA			0x104
# define CM_PLL_ANARST			BIT(8)
# define CM_PLLA_HOLDPER		BIT(7)
# define CM_PLLA_LOADPER		BIT(6)
# define CM_PLLA_HOLDCORE		BIT(5)
# define CM_PLLA_LOADCORE		BIT(4)
# define CM_PLLA_HOLDCCP2		BIT(3)
# define CM_PLLA_LOADCCP2		BIT(2)
# define CM_PLLA_HOLDDSI0		BIT(1)
# define CM_PLLA_LOADDSI0		BIT(0)

#define CM_PLLC			0x108
# define CM_PLLC_HOLDPER		BIT(7)
# define CM_PLLC_LOADPER		BIT(6)
# define CM_PLLC_HOLDCORE2		BIT(5)
# define CM_PLLC_LOADCORE2		BIT(4)
# define CM_PLLC_HOLDCORE1		BIT(3)
# define CM_PLLC_LOADCORE1		BIT(2)
# define CM_PLLC_HOLDCORE0		BIT(1)
# define CM_PLLC_LOADCORE0		BIT(0)

#define CM_PLLD			0x10c
# define CM_PLLD_HOLDPER		BIT(7)
# define CM_PLLD_LOADPER		BIT(6)
# define CM_PLLD_HOLDCORE		BIT(5)
# define CM_PLLD_LOADCORE		BIT(4)
# define CM_PLLD_HOLDDSI1		BIT(3)
# define CM_PLLD_LOADDSI1		BIT(2)
# define CM_PLLD_HOLDDSI0		BIT(1)
# define CM_PLLD_LOADDSI0		BIT(0)

#define CM_PLLH			0x110
# define CM_PLLH_LOADRCAL		BIT(2)
# define CM_PLLH_LOADAUX		BIT(1)
# define CM_PLLH_LOADPIX		BIT(0)

#define CM_LOCK			0x114
# define CM_LOCK_FLOCKH		BIT(12)
# define CM_LOCK_FLOCKD		BIT(11)
# define CM_LOCK_FLOCKC		BIT(10)
# define CM_LOCK_FLOCKB		BIT(9)
# define CM_LOCK_FLOCKA		BIT(8)

#define CM_EVENT		0x118
#define CM_DSI1ECTL		0x158
#define CM_DSI1EDIV		0x15c
#define CM_DSI1PCTL		0x160
#define CM_DSI1PDIV		0x164
#define CM_DFTCTL		0x168
#define CM_DFTDIV		0x16c

#define CM_PLLB			0x170
# define CM_PLLB_HOLDARM		BIT(1)
# define CM_PLLB_LOADARM		BIT(0)

#define A2W_PLLA_CTRL		0x1100
#define A2W_PLLC_CTRL		0x1120
#define A2W_PLLD_CTRL		0x1140
#define A2W_PLLH_CTRL		0x1160
#define A2W_PLLB_CTRL		0x11e0
# define A2W_PLL_CTRL_PRST_DISABLE	BIT(17)
# define A2W_PLL_CTRL_PWRDN		BIT(16)
# define A2W_PLL_CTRL_PDIV_MASK	0x000007000
# define A2W_PLL_CTRL_PDIV_SHIFT	12
# define A2W_PLL_CTRL_NDIV_MASK	0x0000003ff
# define A2W_PLL_CTRL_NDIV_SHIFT	0

#define A2W_PLLA_ANA0		0x1010
#define A2W_PLLC_ANA0		0x1030
#define A2W_PLLD_ANA0		0x1050
#define A2W_PLLH_ANA0		0x1070
#define A2W_PLLB_ANA0		0x10f0

#define A2W_PLL_KA_SHIFT		7
#define A2W_PLL_KA_MASK		GENMASK(7, 3)
#define A2W_PLL_KI_SHIFT		19
#define A2W_PLL_KI_MASK		GENMASK(19, 3)
#define A2W_PLL_KP_SHIFT		15
#define A2W_PLL_KP_MASK		GENMASK(15, 4)

#define A2W_PLLH_KA_SHIFT		19
#define A2W_PLLH_KA_MASK		GENMASK(19, 3)
#define A2W_PLLH_KI_LOW_SHIFT		22
#define A2W_PLLH_KI_LOW_MASK		GENMASK(22, 2)
#define A2W_PLLH_KI_HIGH_SHIFT	0
#define A2W_PLLH_KI_HIGH_MASK		GENMASK(0, 1)
#define A2W_PLLH_KP_SHIFT		1
#define A2W_PLLH_KP_MASK		GENMASK(1, 4)

#define A2W_XOSC_CTRL		0x1190
# define A2W_XOSC_CTRL_PLLB_ENABLE	BIT(7)
# define A2W_XOSC_CTRL_PLLA_ENABLE	BIT(6)
# define A2W_XOSC_CTRL_PLLD_ENABLE	BIT(5)
# define A2W_XOSC_CTRL_DDR_ENABLE	BIT(4)
# define A2W_XOSC_CTRL_CPR1_ENABLE	BIT(3)
# define A2W_XOSC_CTRL_USB_ENABLE	BIT(2)
# define A2W_XOSC_CTRL_HDMI_ENABLE	BIT(1)
# define A2W_XOSC_CTRL_PLLC_ENABLE	BIT(0)

#define A2W_PLLA_FRAC		0x1200
#define A2W_PLLC_FRAC		0x1220
#define A2W_PLLD_FRAC		0x1240
#define A2W_PLLH_FRAC		0x1260
#define A2W_PLLB_FRAC		0x12e0
# define A2W_PLL_FRAC_BITS		20
# define A2W_PLL_FRAC_MASK		GENMASK(0, A2W_PLL_FRAC_BITS)

#define A2W_PLL_CHANNEL_DISABLE	BIT(8)
#define A2W_PLL_DIV_BITS		8
#define A2W_PLL_DIV_SHIFT		0
#define A2W_PLL_DIV_MASK		GENMASK(A2W_PLL_DIV_SHIFT, A2W_PLL_DIV_BITS)

#define A2W_PLLA_DSI0		0x1300
#define A2W_PLLA_CORE		0x1400
#define A2W_PLLA_PER		0x1500
#define A2W_PLLA_CCP2		0x1600

#define A2W_PLLC_CORE2		0x1320
#define A2W_PLLC_CORE1		0x1420
#define A2W_PLLC_PER		0x1520
#define A2W_PLLC_CORE0		0x1620

#define A2W_PLLD_DSI0		0x1340
#define A2W_PLLD_CORE		0x1440
#define A2W_PLLD_PER		0x1540
#define A2W_PLLD_DSI1		0x1640

#define A2W_PLLH_AUX		0x1360
#define A2W_PLLH_RCAL		0x1460
#define A2W_PLLH_PIX		0x1560
#define A2W_PLLH_STS		0x1660

#define A2W_PLLH_CTRLR		0x1960
#define A2W_PLLH_FRACR		0x1a60
#define A2W_PLLH_AUXR		0x1b60
#define A2W_PLLH_RCALR		0x1c60
#define A2W_PLLH_PIXR		0x1d60
#define A2W_PLLH_STSR		0x1e60

#define A2W_PLLB_ARM		0x13e0
#define A2W_PLLB_SP0		0x14e0
#define A2W_PLLB_SP1		0x15e0
#define A2W_PLLB_SP2		0x16e0

#define LOCK_TIMEOUT_NS		100000000
#define BCM2835_MAX_FB_RATE	1750000000u

#define BCM2835_OSC_RATE	19200000u
#define BCM2711_OSC_RATE	54000000u

#define SOC_BCM2835		BIT(0)
#define SOC_BCM2711		BIT(1)
#define SOC_ALL			(SOC_BCM2835 | SOC_BCM2711)

#define VCMSG_ID_CORE_CLOCK     4

enum {
  CLK_TYPE_PLL = 1,
  CLK_TYPE_PLL_DIV,
  CLK_TYPE_CLK,
  CLK_TYPE_FW,
};

struct bcm2835_pll_ana_bits {
  uint32_t mask0;
  uint32_t set0;
  uint32_t mask1;
  uint32_t set1;
  uint32_t mask3;
  uint32_t set3;
  uint32_t fb_prediv_mask;
};

struct bcm2835_pll_data {
  uint32_t cm_ctrl_reg;
  uint32_t a2w_ctrl_reg;
  uint32_t frac_reg;
  uint32_t ana_reg_base;
  uint32_t reference_enable_mask;
  /* Bit in CM_LOCK to indicate when the PLL has locked. */
  uint32_t lock_mask;

  const struct bcm2835_pll_ana_bits *ana;

  uint32_t min_rate;
  uint32_t max_rate;
  /*
   * Highest rate for the VCO before we have to use the
   * pre-divide-by-2.
   */
  uint32_t max_fb_rate;
};

struct bcm2835_pll_divider_data {
  uint32_t source_pll;

  uint32_t cm_reg;
  uint32_t a2w_reg;

  uint32_t load_mask;
  uint32_t hold_mask;
  uint32_t fixed_divider;
};

struct bcm2835_clock_data {
  uint32_t num_mux_parents;
  const uint32_t *parents;

  uint32_t ctl_reg;
  uint32_t div_reg;

  /* Number of integer bits in the divider */
  uint32_t int_bits;
  /* Number of fractional bits in the divider */
  uint32_t frac_bits;

  bool is_vpu_clock;
  bool is_mash_clock;
  bool low_jitter;

  uint32_t tcnt_mux;
};

struct bcm2835_clk_desc {
  int type;
  //const void *data;
  union {
    struct bcm2835_pll_data pll_data;
    struct bcm2835_pll_divider_data pll_divider_data;
    struct bcm2835_clock_data clock_data;
    uint32_t fw_clk_id;
  } data;
};

extern const struct bcm2835_clk_desc bcm2835_clk_descs[];

#endif  // SRC_DEVICES_CLOCK_DRIVERS_BCM2835_CLK_BCM2835_CLK_INTERNAL_H_
