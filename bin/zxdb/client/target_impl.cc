// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "garnet/bin/zxdb/client/target_impl.h"

#include <sstream>

#include "garnet/bin/zxdb/client/process_impl.h"
#include "garnet/bin/zxdb/client/session.h"
#include "garnet/bin/zxdb/client/system_impl.h"
#include "garnet/bin/zxdb/client/target_observer.h"
#include "garnet/public/lib/fxl/logging.h"
#include "garnet/public/lib/fxl/strings/string_printf.h"

namespace zxdb {

TargetImpl::TargetImpl(SystemImpl* system)
    : Target(system->session()),
      weak_thunk_(std::make_shared<WeakThunk<TargetImpl>>(this)) {}
TargetImpl::~TargetImpl() = default;

Target::State TargetImpl::GetState() const {
  return state_;
}

Process* TargetImpl::GetProcess() const {
  return process_.get();
}

const std::vector<std::string>& TargetImpl::GetArgs() const {
  return args_;
}

void TargetImpl::SetArgs(std::vector<std::string> args) {
  args_ = std::move(args);
}

void TargetImpl::Launch(LaunchCallback callback) {
  if (state_ != State::kStopped) {
    // TODO(brettw) issue callback asynchronously to avoid reentering caller.
    callback(this, Err("Can't launch, program is already running."));
    return;
  }
  if (args_.empty()) {
    // TODO(brettw) issue callback asynchronously to avoid reentering caller.
    callback(this, Err("No program specified to launch."));
    return;
  }

  debug_ipc::LaunchRequest request;
  request.argv = args_;
  session()->Send<debug_ipc::LaunchRequest, debug_ipc::LaunchReply>(
      request,
      [thunk = std::weak_ptr<WeakThunk<TargetImpl>>(weak_thunk_),
       callback = std::move(callback)](
           Session*, uint32_t transaction_id, const Err& err,
           debug_ipc::LaunchReply reply) {
        if (auto ptr = thunk.lock()) {
          ptr->thunk->OnLaunchReply(err, std::move(reply),
                                    std::move(callback));
        } else {
          // The reply that the process was launched came after the local
          // objects were destroyed.
          // TODO(brettw) handle this more gracefully. Maybe kill the remote
          // process?
          fprintf(stderr, "Warning: process launch race, extra process could "
              "be running.\n");
        }
      });

  for (auto& observer : observers())
    observer.DidChangeTargetState(this, State::kStopped);
}

void TargetImpl::OnLaunchReply(const Err& err, debug_ipc::LaunchReply reply,
                               LaunchCallback callback) {
  FXL_DCHECK(state_ = State::kStarting);
  FXL_DCHECK(!process_.get());  // Shouldn't have a process.
  if (err.has_error()) {
    // Error from transport.
    state_ = State::kStopped;
    callback(this, err);
  } else if (reply.status != 0) {
    // Error from launching.
    state_ = State::kStopped;
    callback(this, Err(fxl::StringPrintf(
            "Error launching, status = %d.", reply.status)));
  } else {
    state_ = State::kRunning;
    process_ = std::make_unique<ProcessImpl>(this, reply.process_koid);
    callback(this, Err());
  }

  for (auto& observer : observers())
    observer.DidChangeTargetState(this, State::kStarting);
}

}  // namespace zxdb
