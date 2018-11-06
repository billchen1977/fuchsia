# Copyright 2018 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS := $(LOCAL_DIR)/bt-hci-broadcom.c

MODULE_STATIC_LIBS := system/ulib/ddk system/ulib/sync

MODULE_LIBS := system/ulib/driver system/ulib/zircon system/ulib/c

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-bt-hci \
    system/banjo/ddk-protocol-serial \

ifeq ($(call TOBOOL,$(INTERNAL_ACCESS)),true)
MODULE_FIRMWARE := bluetooth/bcm4345c4/BCM4345C5.hcd
endif

include make/module.mk
