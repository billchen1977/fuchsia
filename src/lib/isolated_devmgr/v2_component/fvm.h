// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_LIB_ISOLATED_DEVMGR_V2_COMPONENT_FVM_H_
#define SRC_LIB_ISOLATED_DEVMGR_V2_COMPONENT_FVM_H_

#include <lib/zx/status.h>
#include <zircon/device/block.h>

#include <array>
#include <optional>
#include <string>

namespace isolated_devmgr {

struct FvmOptions {
  std::string_view name = "fs-test-partition";

  // If not set, a test GUID type is used.
  std::optional<std::array<uint8_t, BLOCK_GUID_LEN>> type;
};

// Formats the given block device to be managed by FVM, and start up an FVM instance.
//
// Returns that path to the FVM device.
zx::status<std::string> CreateFvmInstance(const std::string& device_path, int slice_size);

// Formats the given block device to be FVM managed, and create a new partition on the device.
//
// Returns the path to the newly created block device.
zx::status<std::string> CreateFvmPartition(const std::string& device_path, int slice_size,
                                           const FvmOptions& options = {});

// Binds the FVM driver to the given device.
zx::status<> BindFvm(int fd);

}  // namespace isolated_devmgr

#endif  // SRC_LIB_ISOLATED_DEVMGR_V2_COMPONENT_FVM_H_
