// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_RPI_DISPLAY_H_
#define SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_RPI_DISPLAY_H_

#include "rpi-firmware.h"

#ifndef BIT
# define BIT(b)                (1UL << (b))
#endif

#ifndef GENMASK
# define GENMASK(b, s)         (((1UL << (s)) - 1) << (b))
#endif

#define PLANES_PER_CRTC	8

struct get_display_cfg {
  uint32_t max_pixel_clock[4];  //Max pixel clock for each display
};

struct set_plane {
  uint8_t display;
  uint8_t plane_id;
  uint8_t vc_image_type;
  int8_t layer;

  uint16_t width;
  uint16_t height;

  uint16_t pitch;
  uint16_t vpitch;

  uint32_t src_x;	/* 16p16 */
  uint32_t src_y;	/* 16p16 */

  uint32_t src_w;	/* 16p16 */
  uint32_t src_h;	/* 16p16 */

  int16_t dst_x;
  int16_t dst_y;

  uint16_t dst_w;
  uint16_t dst_h;

  uint8_t alpha;
  uint8_t num_planes;
  uint8_t is_vu;
  uint8_t color_encoding;

  uint32_t planes[4];  /* DMA address of each plane */

  uint32_t transform;
};

/* Values for the transform field */
#define TRANSFORM_NO_ROTATE	0
#define TRANSFORM_ROTATE_180	BIT(1)
#define TRANSFORM_FLIP_HRIZ	BIT(16)
#define TRANSFORM_FLIP_VERT	BIT(17)

struct mailbox_set_plane {
  //struct rpi_firmware_property_tag_header tag;
  struct set_plane plane;
};

struct mailbox_blank_display {
  struct rpi_firmware_property_tag_header tag1;
  uint32_t display;
  struct rpi_firmware_property_tag_header tag2;
  uint32_t blank;
};

struct mailbox_display_pwr {
  //struct rpi_firmware_property_tag_header tag;
  uint32_t display;
  uint32_t state;
};

struct mailbox_get_edid {
  //struct rpi_firmware_property_tag_header tag;
  uint32_t block;
  uint32_t display_number;
  uint8_t edid[128];
};

struct set_timings {
  uint8_t display;
  uint8_t padding;
  uint16_t video_id_code;

  uint32_t clock;		/* in kHz */

  uint16_t hdisplay;
  uint16_t hsync_start;

  uint16_t hsync_end;
  uint16_t htotal;

  uint16_t hskew;
  uint16_t vdisplay;

  uint16_t vsync_start;
  uint16_t vsync_end;

  uint16_t vtotal;
  uint16_t vscan;

  uint16_t vrefresh;
  uint16_t padding2;

  uint32_t flags;
#define  TIMINGS_FLAGS_H_SYNC_POS	BIT(0)
#define  TIMINGS_FLAGS_H_SYNC_NEG	0
#define  TIMINGS_FLAGS_V_SYNC_POS	BIT(1)
#define  TIMINGS_FLAGS_V_SYNC_NEG	0
#define  TIMINGS_FLAGS_INTERLACE	BIT(2)

#define TIMINGS_FLAGS_ASPECT_MASK	GENMASK(4, 4)
#define TIMINGS_FLAGS_ASPECT_NONE	(0 << 4)
#define TIMINGS_FLAGS_ASPECT_4_3	(1 << 4)
#define TIMINGS_FLAGS_ASPECT_16_9	(2 << 4)
#define TIMINGS_FLAGS_ASPECT_64_27	(3 << 4)
#define TIMINGS_FLAGS_ASPECT_256_135	(4 << 4)

/* Limited range RGB flag. Not set corresponds to full range. */
#define TIMINGS_FLAGS_RGB_LIMITED	BIT(8)
/* DVI monitor, therefore disable infoframes. Not set corresponds to HDMI. */
#define TIMINGS_FLAGS_DVI		BIT(9)
/* Double clock */
#define TIMINGS_FLAGS_DBL_CLK		BIT(10)
};

struct mailbox_set_mode {
  //struct rpi_firmware_property_tag_header tag;
  struct set_timings timings;
};


/* The firmware delivers a vblank interrupt to us through the SMI
 * hardware, which has only this one register.
 */
#define SMICS 0x0
#define SMIDSW0 0x14
#define SMIDSW1 0x1C
#define SMICS_INTERRUPTS (BIT(9) | BIT(10) | BIT(11))

/* Flag to denote that the firmware is giving multiple display callbacks */
#define SMI_NEW 0xabcd0000


#endif  // SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_RPI_DISPLAY_H_
