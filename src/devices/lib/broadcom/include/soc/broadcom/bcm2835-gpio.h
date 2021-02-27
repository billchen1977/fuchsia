// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_BCM2835_GPIO_REGS_H_
#define SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_BCM2835_GPIO_REGS_H_

#define GPIO_GPFSEL0   0x00
#define GPIO_GPFSEL1   0x04
#define GPIO_GPFSEL2   0x08
#define GPIO_GPFSEL3   0x0C
#define GPIO_GPFSEL4   0x10
#define GPIO_GPFSEL5   0x14
#define GPIO_GPSET0    0x1C
#define GPIO_GPSET1    0x20
#define GPIO_GPCLR0    0x28
#define GPIO_GPCLR1    0x2C
#define GPIO_GPLEV0    0x34
#define GPIO_GPLEV1    0x38
#define GPIO_GPEDS0    0x40
#define GPIO_GPEDS1    0x44
#define GPIO_GPREN0    0x4C
#define GPIO_GPREN1    0x50
#define GPIO_GPFEN0    0x58
#define GPIO_GPFEN1    0x5C
#define GPIO_GPHEN0    0x64
#define GPIO_GPHEN1    0x68
#define GPIO_GPLEN0    0x70
#define GPIO_GPLEN1    0x74
#define GPIO_GPAREN0   0x7C
#define GPIO_GPAREN1   0x80
#define GPIO_GPAFEN0   0x88
#define GPIO_GPAFEN1   0x8C
#define GPIO_GPPUD0    0xe4
#define GPIO_GPPUD1    0xe8
#define GPIO_GPPUD2    0xec
#define GPIO_GPPUD3    0xf0

#define GPIO_GPFSEL_INPUT   0x0
#define GPIO_GPFSEL_OUTPUT  0x1
#define GPIO_GPFSEL_ALT0    0x4
#define GPIO_GPFSEL_ALT1    0x5
#define GPIO_GPFSEL_ALT2    0x6
#define GPIO_GPFSEL_ALT3    0x7
#define GPIO_GPFSEL_ALT4    0x3
#define GPIO_GPFSEL_ALT5    0x2

#define GPIO_GPPUD_NONE     0x0
#define GPIO_GPPUD_UP       0x1
#define GPIO_GPPUD_DOWN     0x2

typedef struct Bcm2835GpioMetadata {
  uint32_t pin_count;
  uint32_t bank_count;
  uint8_t bank_pin[8];
} Bcm2835GpioMetadata;

#endif  // SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_BCM2835_GPIO_REGS_H_
