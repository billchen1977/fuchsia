// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIB_FIDL_ASYNC_CPP_BIND_H_
#define LIB_FIDL_ASYNC_CPP_BIND_H_

#include <lib/async/wait.h>
#include <lib/fidl-async/cpp/channel_transaction.h>
#include <lib/fidl/llcpp/transaction.h>
#include <lib/fit/function.h>
#include <lib/zx/channel.h>
#include <zircon/fidl.h>

namespace fidl {

template <typename Interface>
using OnChannelClosedFn = fit::callback<void(Interface*)>;

namespace internal {

using TypeErasedDispatchFn = bool (*)(void*, fidl_msg_t*, ::fidl::Transaction*);

using TypeErasedOnChannelClosedFn = fit::callback<void(void*)>;

zx_status_t TypeErasedBind(async_dispatcher_t* dispatcher, zx::channel channel, void* impl,
                           TypeErasedDispatchFn dispatch_fn,
                           TypeErasedOnChannelClosedFn on_channel_closed_fn);

class SimpleBinding : private async_wait_t {
 public:
  SimpleBinding(async_dispatcher_t* dispatcher, zx::channel channel, void* impl,
                TypeErasedDispatchFn dispatch_fn, TypeErasedOnChannelClosedFn on_channel_closed_fn);

  ~SimpleBinding();

  static void MessageHandler(async_dispatcher_t* dispatcher, async_wait_t* wait, zx_status_t status,
                             const zx_packet_signal_t* signal);

 private:
  friend fidl::internal::ChannelTransaction;
  friend zx_status_t BeginWait(std::unique_ptr<SimpleBinding>* unique_binding);

  zx::unowned_channel channel() const { return zx::unowned_channel(async_wait_t::object); }

  async_dispatcher_t* dispatcher_;
  void* interface_;
  TypeErasedDispatchFn dispatch_fn_;
  TypeErasedOnChannelClosedFn on_channel_closed_fn_;
};

// Attempts to attach the binding onto the async dispatcher.
// Upon success, the binding ownership is transferred to the async dispatcher.
// Upon failure, the binding ownership stays in the |unique_ptr|.
// Returns the status of |async_begin_wait|.
zx_status_t BeginWait(std::unique_ptr<SimpleBinding>* unique_binding);

}  // namespace internal

// (Deprecated) Binds an implementation of a low-level C++ server interface to
// |channel| using |dispatcher|.
//
// Note: This function is preserved for backwards compatibility reasons only.
// It would not dispatch a second message on a channel if there were already
// another message being handled or whose response deferred asynchronously.
// New clients should use the |fidl::BindServer| API from
// `zircon/system/ulib/fidl/include/lib/fidl/llcpp/server.h`.
//
// This function adds an |async_wait_t| to the given |dispatcher| that waits
// asynchronously for new messages to arrive on |channel|. When a message
// arrives, the dispatch function corresponding to the interface is called
// on one of the threads associated with the |dispatcher|.
//
// Typically, the dispatch function is generated by the low-level C++ backend
// for FIDL interfaces. These dispatch functions decode the |fidl_msg_t| and
// call into the implementation of the FIDL interface, via its C++ vtable.
//
// When an error occurs in the server implementation as part of handling the message, it may call
// |Close(error)| on the completer to indicate the error condition to the dispatcher.
// The dispatcher will send an epitaph with this error code before closing the channel.
//
// Returns whether |fidl::BindSingleInFlightOnly| was able to begin waiting on the given |channel|.
// Upon any error, |channel| is closed and the binding is terminated.
//
// The |dispatcher| takes ownership of the channel. Shutting down the |dispatcher|
// results in |channel| being closed.
//
// It is safe to shutdown the dispatcher at any time.
//
// It is unsafe to destroy the dispatcher from within a dispatch function.
// It is unsafe to destroy the dispatcher while any fidl::Transaction objects are alive.
template <typename Interface>
zx_status_t BindSingleInFlightOnly(async_dispatcher_t* dispatcher, zx::channel channel,
                                   Interface* impl) {
  return internal::TypeErasedBind(dispatcher, std::move(channel), impl,
                                  &Interface::_EnclosingProtocol::TypeErasedDispatch, nullptr);
}

// As above, but will invoke |on_channel_close_fn| on |impl| when either end of the channel
// is closed.
template <typename Interface>
zx_status_t BindSingleInFlightOnly(async_dispatcher_t* dispatcher, zx::channel channel,
                                   Interface* impl,
                                   OnChannelClosedFn<Interface> on_channel_close_fn) {
  return internal::TypeErasedBind(dispatcher, std::move(channel), impl,
                                  &Interface::_EnclosingProtocol::TypeErasedDispatch,
                                  [fn = std::move(on_channel_close_fn)](void* impl) mutable {
                                    fn(static_cast<Interface*>(impl));
                                  });
}

// As above, but will destroy |impl| when either end of the channel is closed.
template <typename Interface>
zx_status_t BindSingleInFlightOnly(async_dispatcher_t* dispatcher, zx::channel channel,
                                   std::unique_ptr<Interface> impl) {
  OnChannelClosedFn<Interface> fn = [](Interface* impl) { delete impl; };
  return BindSingleInFlightOnly(dispatcher, std::move(channel), impl.release(), std::move(fn));
}

}  // namespace fidl

#endif  // LIB_FIDL_ASYNC_CPP_BIND_H_
