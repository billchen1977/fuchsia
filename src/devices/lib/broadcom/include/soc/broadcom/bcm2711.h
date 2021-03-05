// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_BCM2711_H_
#define SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_BCM2711_H_

#define BCM_PERIPH_BASE         0xfe000000U
#define ARM_BASE                (BCM_PERIPH_BASE + 0xb000)
#define MAILBOX_BASE            (ARM_BASE + 0x880)
#define GPIO_BASE               (BCM_PERIPH_BASE + 0x200000)

/* Interrupt */
#define GIC_SPI_BASE            32
#define GIC_SPI_MAILBOX0        (GIC_SPI_BASE + 33)
#define GIC_SPI_MAILBOX1        (GIC_SPI_BASE + 35)
#define GIC_SPI_GPIO0           (GIC_SPI_BASE + 113)
#define GIC_SPI_GPIO1           (GIC_SPI_BASE + 114)
#define GIC_SPI_GPIO2           (GIC_SPI_BASE + 115)
#define GIC_SPI_GPIO3           (GIC_SPI_BASE + 116)

/* GPIO */
#define GPIO_PIN_COUNT          58
#define GPIO_BANK_COUNT         3
#define GPIO_BANK0_PIN_COUNT	 28
#define GPIO_BANK1_PIN_COUNT    18
#define GPIO_BANK2_PIN_COUNT    12

#endif  // SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_BCM2711_H_
