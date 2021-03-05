// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_BCM2835_MAILBOX_H_
#define SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_BCM2835_MAILBOX_H_

#define MAILBOX_SIZE   0x40    // per read/write pair
#define MAILBOX_COUNT  2

#define MAILBOX0_RD	0x00
#define MAILBOX0_POL	0x10
#define MAILBOX0_STA	0x18
#define MAILBOX0_CNF	0x1C
#define MAILBOX1_WRT	0x20
#define MAILBOX1_STA	0x38

#define MAILBOX_FULL	0x80000000
#define MAILBOX_EMPTY	0x40000000

#define MAILBOX_HAVEDATA_IRQEN	0x00000001

#define MAILBOX_MSG(chan, data28)	(((data28) & ~0xf) | ((chan) & 0xf))
#define MAILBOX_CHAN(msg)		((msg) & 0xf)
#define MAILBOX_DATA28(msg)		((msg) & ~0xf)
#define MAILBOX_CHAN_PROPERTY		8

#endif  // SRC_DEVICES_LIB_BROADCOM_INCLUDE_SOC_BCM2835_MAILBOX_H_
