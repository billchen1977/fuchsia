// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_DEBUG_DEBUG_AGENT_MOCK_PROCESS_HANDLE_H_
#define SRC_DEVELOPER_DEBUG_DEBUG_AGENT_MOCK_PROCESS_HANDLE_H_

#include <string>

#include "src/developer/debug/debug_agent/mock_thread_handle.h"
#include "src/developer/debug/debug_agent/process_handle.h"
#include "src/developer/debug/shared/mock_memory.h"

namespace debug_agent {

class MockProcessHandle final : public ProcessHandle {
 public:
  struct MemoryWrite {
    MemoryWrite(uint64_t a, std::vector<uint8_t> d) : address(a), data(std::move(d)) {}

    uint64_t address = 0;
    std::vector<uint8_t> data;
  };

  explicit MockProcessHandle(zx_koid_t process_koid, std::string name = std::string());

  void set_name(std::string n) { name_ = std::move(n); }

  // Sets the threads. These will be copied since we need to return a new unique_ptr for each call
  // to GetChildThreads().
  void set_threads(std::vector<MockThreadHandle> threads) { threads_ = std::move(threads); }

  // Use to set mcoked memory values to read. The MockMemory is only used for ReadMemory calls.
  // WriteMemory calls come out in memory_writes().
  debug_ipc::MockMemory& mock_memory() { return mock_memory_; }
  std::vector<MemoryWrite>& memory_writes() { return memory_writes_; }

  // ProcessHandle implementation.
  const zx::process& GetNativeHandle() const override { return null_handle_; }
  zx::process& GetNativeHandle() override { return null_handle_; }
  zx_koid_t GetKoid() const override { return process_koid_; }
  std::string GetName() const override { return name_; }
  std::vector<std::unique_ptr<ThreadHandle>> GetChildThreads() const override;
  int64_t GetReturnCode() const override;
  std::vector<debug_ipc::AddressRegion> GetAddressSpace(uint64_t address) const override;
  std::vector<debug_ipc::Module> GetModules(uint64_t dl_debug_addr) const override;
  zx_status_t ReadMemory(uintptr_t address, void* buffer, size_t len,
                         size_t* actual) const override;
  zx_status_t WriteMemory(uintptr_t address, const void* buffer, size_t len,
                          size_t* actual) override;
  std::vector<debug_ipc::MemoryBlock> ReadMemoryBlocks(uint64_t address,
                                                       uint32_t size) const override;

 private:
  // Always null, for returning only from the getters above.
  // TODO(brettw) Remove this when the ThreadHandle no longer exposes a zx::thread getter.
  static zx::process null_handle_;

  zx_koid_t process_koid_;
  std::string name_;

  std::vector<MockThreadHandle> threads_;

  debug_ipc::MockMemory mock_memory_;
  std::vector<MemoryWrite> memory_writes_;
};

}  // namespace debug_agent

#endif  // SRC_DEVELOPER_DEBUG_DEBUG_AGENT_MOCK_PROCESS_HANDLE_H_
