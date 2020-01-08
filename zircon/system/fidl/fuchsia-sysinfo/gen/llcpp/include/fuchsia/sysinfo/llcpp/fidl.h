// WARNING: This file is machine generated by fidlgen.

#pragma once

#include <lib/fidl/internal.h>
#include <lib/fidl/txn_header.h>
#include <lib/fidl/llcpp/array.h>
#include <lib/fidl/llcpp/coding.h>
#include <lib/fidl/llcpp/connect_service.h>
#include <lib/fidl/llcpp/service_handler_interface.h>
#include <lib/fidl/llcpp/string_view.h>
#include <lib/fidl/llcpp/sync_call.h>
#include <lib/fidl/llcpp/traits.h>
#include <lib/fidl/llcpp/transaction.h>
#include <lib/fidl/llcpp/vector_view.h>
#include <lib/fit/function.h>
#include <lib/zx/channel.h>
#include <lib/zx/resource.h>
#include <zircon/fidl.h>

namespace llcpp {

namespace fuchsia {
namespace sysinfo {

class Device;
enum class InterruptControllerType : uint32_t {
  UNKNOWN = 0u,
  APIC = 1u,
  GIC_V2 = 2u,
  GIC_V3 = 3u,
};


struct InterruptControllerInfo;

extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetHypervisorResourceRequestTable;
extern "C" const fidl_type_t v1_fuchsia_sysinfo_DeviceGetHypervisorResourceRequestTable;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetHypervisorResourceResponseTable;
extern "C" const fidl_type_t v1_fuchsia_sysinfo_DeviceGetHypervisorResourceResponseTable;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetBoardNameRequestTable;
extern "C" const fidl_type_t v1_fuchsia_sysinfo_DeviceGetBoardNameRequestTable;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetBoardNameResponseTable;
extern "C" const fidl_type_t v1_fuchsia_sysinfo_DeviceGetBoardNameResponseTable;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetBoardRevisionRequestTable;
extern "C" const fidl_type_t v1_fuchsia_sysinfo_DeviceGetBoardRevisionRequestTable;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetBoardRevisionResponseTable;
extern "C" const fidl_type_t v1_fuchsia_sysinfo_DeviceGetBoardRevisionResponseTable;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetInterruptControllerInfoRequestTable;
extern "C" const fidl_type_t v1_fuchsia_sysinfo_DeviceGetInterruptControllerInfoRequestTable;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetInterruptControllerInfoResponseTable;
extern "C" const fidl_type_t v1_fuchsia_sysinfo_DeviceGetInterruptControllerInfoResponseTable;

class Device final {
  Device() = delete;
 public:

  struct GetHypervisorResourceResponse final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    int32_t status;
    ::zx::resource resource;

    static constexpr const fidl_type_t* Type = &v1_fuchsia_sysinfo_DeviceGetHypervisorResourceResponseTable;
    static constexpr const fidl_type_t* AltType = &fuchsia_sysinfo_DeviceGetHypervisorResourceResponseTable;
    static constexpr uint32_t MaxNumHandles = 1;
    static constexpr uint32_t PrimarySize = 24;
    static constexpr uint32_t MaxOutOfLine = 0;
    static constexpr uint32_t AltPrimarySize = 24;
    static constexpr uint32_t AltMaxOutOfLine = 0;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr bool ContainsUnion = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kResponse;
  };
  using GetHypervisorResourceRequest = ::fidl::AnyZeroArgMessage;

  struct GetBoardNameResponse final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    int32_t status;
    ::fidl::StringView name;

    static constexpr const fidl_type_t* Type = &v1_fuchsia_sysinfo_DeviceGetBoardNameResponseTable;
    static constexpr const fidl_type_t* AltType = &fuchsia_sysinfo_DeviceGetBoardNameResponseTable;
    static constexpr uint32_t MaxNumHandles = 0;
    static constexpr uint32_t PrimarySize = 40;
    static constexpr uint32_t MaxOutOfLine = 32;
    static constexpr uint32_t AltPrimarySize = 40;
    static constexpr uint32_t AltMaxOutOfLine = 32;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr bool ContainsUnion = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kResponse;
  };
  using GetBoardNameRequest = ::fidl::AnyZeroArgMessage;

  struct GetBoardRevisionResponse final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    int32_t status;
    uint32_t revision;

    static constexpr const fidl_type_t* Type = &v1_fuchsia_sysinfo_DeviceGetBoardRevisionResponseTable;
    static constexpr const fidl_type_t* AltType = &fuchsia_sysinfo_DeviceGetBoardRevisionResponseTable;
    static constexpr uint32_t MaxNumHandles = 0;
    static constexpr uint32_t PrimarySize = 24;
    static constexpr uint32_t MaxOutOfLine = 0;
    static constexpr uint32_t AltPrimarySize = 24;
    static constexpr uint32_t AltMaxOutOfLine = 0;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr bool ContainsUnion = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kResponse;
  };
  using GetBoardRevisionRequest = ::fidl::AnyZeroArgMessage;

  struct GetInterruptControllerInfoResponse final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    int32_t status;
    ::llcpp::fuchsia::sysinfo::InterruptControllerInfo* info;

    static constexpr const fidl_type_t* Type = &v1_fuchsia_sysinfo_DeviceGetInterruptControllerInfoResponseTable;
    static constexpr const fidl_type_t* AltType = &fuchsia_sysinfo_DeviceGetInterruptControllerInfoResponseTable;
    static constexpr uint32_t MaxNumHandles = 0;
    static constexpr uint32_t PrimarySize = 32;
    static constexpr uint32_t MaxOutOfLine = 8;
    static constexpr uint32_t AltPrimarySize = 32;
    static constexpr uint32_t AltMaxOutOfLine = 8;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr bool ContainsUnion = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kResponse;
  };
  using GetInterruptControllerInfoRequest = ::fidl::AnyZeroArgMessage;


  // Collection of return types of FIDL calls in this interface.
  class ResultOf final {
    ResultOf() = delete;
   private:
    template <typename ResponseType>
    class GetHypervisorResource_Impl final : private ::fidl::internal::OwnedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::OwnedSyncCallBase<ResponseType>;
     public:
      GetHypervisorResource_Impl(::zx::unowned_channel _client_end);
      ~GetHypervisorResource_Impl() = default;
      GetHypervisorResource_Impl(GetHypervisorResource_Impl&& other) = default;
      GetHypervisorResource_Impl& operator=(GetHypervisorResource_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class GetBoardName_Impl final : private ::fidl::internal::OwnedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::OwnedSyncCallBase<ResponseType>;
     public:
      GetBoardName_Impl(::zx::unowned_channel _client_end);
      ~GetBoardName_Impl() = default;
      GetBoardName_Impl(GetBoardName_Impl&& other) = default;
      GetBoardName_Impl& operator=(GetBoardName_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class GetBoardRevision_Impl final : private ::fidl::internal::OwnedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::OwnedSyncCallBase<ResponseType>;
     public:
      GetBoardRevision_Impl(::zx::unowned_channel _client_end);
      ~GetBoardRevision_Impl() = default;
      GetBoardRevision_Impl(GetBoardRevision_Impl&& other) = default;
      GetBoardRevision_Impl& operator=(GetBoardRevision_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class GetInterruptControllerInfo_Impl final : private ::fidl::internal::OwnedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::OwnedSyncCallBase<ResponseType>;
     public:
      GetInterruptControllerInfo_Impl(::zx::unowned_channel _client_end);
      ~GetInterruptControllerInfo_Impl() = default;
      GetInterruptControllerInfo_Impl(GetInterruptControllerInfo_Impl&& other) = default;
      GetInterruptControllerInfo_Impl& operator=(GetInterruptControllerInfo_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };

   public:
    using GetHypervisorResource = GetHypervisorResource_Impl<GetHypervisorResourceResponse>;
    using GetBoardName = GetBoardName_Impl<GetBoardNameResponse>;
    using GetBoardRevision = GetBoardRevision_Impl<GetBoardRevisionResponse>;
    using GetInterruptControllerInfo = GetInterruptControllerInfo_Impl<GetInterruptControllerInfoResponse>;
  };

  // Collection of return types of FIDL calls in this interface,
  // when the caller-allocate flavor or in-place call is used.
  class UnownedResultOf final {
    UnownedResultOf() = delete;
   private:
    template <typename ResponseType>
    class GetHypervisorResource_Impl final : private ::fidl::internal::UnownedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::UnownedSyncCallBase<ResponseType>;
     public:
      GetHypervisorResource_Impl(::zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);
      ~GetHypervisorResource_Impl() = default;
      GetHypervisorResource_Impl(GetHypervisorResource_Impl&& other) = default;
      GetHypervisorResource_Impl& operator=(GetHypervisorResource_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class GetBoardName_Impl final : private ::fidl::internal::UnownedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::UnownedSyncCallBase<ResponseType>;
     public:
      GetBoardName_Impl(::zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);
      ~GetBoardName_Impl() = default;
      GetBoardName_Impl(GetBoardName_Impl&& other) = default;
      GetBoardName_Impl& operator=(GetBoardName_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class GetBoardRevision_Impl final : private ::fidl::internal::UnownedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::UnownedSyncCallBase<ResponseType>;
     public:
      GetBoardRevision_Impl(::zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);
      ~GetBoardRevision_Impl() = default;
      GetBoardRevision_Impl(GetBoardRevision_Impl&& other) = default;
      GetBoardRevision_Impl& operator=(GetBoardRevision_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class GetInterruptControllerInfo_Impl final : private ::fidl::internal::UnownedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::UnownedSyncCallBase<ResponseType>;
     public:
      GetInterruptControllerInfo_Impl(::zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);
      ~GetInterruptControllerInfo_Impl() = default;
      GetInterruptControllerInfo_Impl(GetInterruptControllerInfo_Impl&& other) = default;
      GetInterruptControllerInfo_Impl& operator=(GetInterruptControllerInfo_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };

   public:
    using GetHypervisorResource = GetHypervisorResource_Impl<GetHypervisorResourceResponse>;
    using GetBoardName = GetBoardName_Impl<GetBoardNameResponse>;
    using GetBoardRevision = GetBoardRevision_Impl<GetBoardRevisionResponse>;
    using GetInterruptControllerInfo = GetInterruptControllerInfo_Impl<GetInterruptControllerInfoResponse>;
  };

  class SyncClient final {
   public:
    explicit SyncClient(::zx::channel channel) : channel_(std::move(channel)) {}
    ~SyncClient() = default;
    SyncClient(SyncClient&&) = default;
    SyncClient& operator=(SyncClient&&) = default;

    const ::zx::channel& channel() const { return channel_; }

    ::zx::channel* mutable_channel() { return &channel_; }

    // Allocates 40 bytes of message buffer on the stack. No heap allocation necessary.
    ResultOf::GetHypervisorResource GetHypervisorResource();

    // Caller provides the backing storage for FIDL message via request and response buffers.
    UnownedResultOf::GetHypervisorResource GetHypervisorResource(::fidl::BytePart _response_buffer);

    // Allocates 88 bytes of message buffer on the stack. No heap allocation necessary.
    ResultOf::GetBoardName GetBoardName();

    // Caller provides the backing storage for FIDL message via request and response buffers.
    UnownedResultOf::GetBoardName GetBoardName(::fidl::BytePart _response_buffer);

    // Allocates 40 bytes of message buffer on the stack. No heap allocation necessary.
    ResultOf::GetBoardRevision GetBoardRevision();

    // Caller provides the backing storage for FIDL message via request and response buffers.
    UnownedResultOf::GetBoardRevision GetBoardRevision(::fidl::BytePart _response_buffer);

    // Allocates 56 bytes of message buffer on the stack. No heap allocation necessary.
    ResultOf::GetInterruptControllerInfo GetInterruptControllerInfo();

    // Caller provides the backing storage for FIDL message via request and response buffers.
    UnownedResultOf::GetInterruptControllerInfo GetInterruptControllerInfo(::fidl::BytePart _response_buffer);

   private:
    ::zx::channel channel_;
  };

  // Methods to make a sync FIDL call directly on an unowned channel, avoiding setting up a client.
  class Call final {
    Call() = delete;
   public:

    // Allocates 40 bytes of message buffer on the stack. No heap allocation necessary.
    static ResultOf::GetHypervisorResource GetHypervisorResource(::zx::unowned_channel _client_end);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    static UnownedResultOf::GetHypervisorResource GetHypervisorResource(::zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);

    // Allocates 88 bytes of message buffer on the stack. No heap allocation necessary.
    static ResultOf::GetBoardName GetBoardName(::zx::unowned_channel _client_end);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    static UnownedResultOf::GetBoardName GetBoardName(::zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);

    // Allocates 40 bytes of message buffer on the stack. No heap allocation necessary.
    static ResultOf::GetBoardRevision GetBoardRevision(::zx::unowned_channel _client_end);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    static UnownedResultOf::GetBoardRevision GetBoardRevision(::zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);

    // Allocates 56 bytes of message buffer on the stack. No heap allocation necessary.
    static ResultOf::GetInterruptControllerInfo GetInterruptControllerInfo(::zx::unowned_channel _client_end);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    static UnownedResultOf::GetInterruptControllerInfo GetInterruptControllerInfo(::zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);

  };

  // Messages are encoded and decoded in-place when these methods are used.
  // Additionally, requests must be already laid-out according to the FIDL wire-format.
  class InPlace final {
    InPlace() = delete;
   public:

    static ::fidl::DecodeResult<GetHypervisorResourceResponse> GetHypervisorResource(::zx::unowned_channel _client_end, ::fidl::BytePart response_buffer);

    static ::fidl::DecodeResult<GetBoardNameResponse> GetBoardName(::zx::unowned_channel _client_end, ::fidl::BytePart response_buffer);

    static ::fidl::DecodeResult<GetBoardRevisionResponse> GetBoardRevision(::zx::unowned_channel _client_end, ::fidl::BytePart response_buffer);

    static ::fidl::DecodeResult<GetInterruptControllerInfoResponse> GetInterruptControllerInfo(::zx::unowned_channel _client_end, ::fidl::BytePart response_buffer);

  };

  // Pure-virtual interface to be implemented by a server.
  class Interface {
   public:
    Interface() = default;
    virtual ~Interface() = default;
    using _Outer = Device;
    using _Base = ::fidl::CompleterBase;

    class GetHypervisorResourceCompleterBase : public _Base {
     public:
      void Reply(int32_t status, ::zx::resource resource);
      void Reply(::fidl::BytePart _buffer, int32_t status, ::zx::resource resource);
      void Reply(::fidl::DecodedMessage<GetHypervisorResourceResponse> params);

     protected:
      using ::fidl::CompleterBase::CompleterBase;
    };

    using GetHypervisorResourceCompleter = ::fidl::Completer<GetHypervisorResourceCompleterBase>;

    virtual void GetHypervisorResource(GetHypervisorResourceCompleter::Sync _completer) = 0;

    class GetBoardNameCompleterBase : public _Base {
     public:
      void Reply(int32_t status, ::fidl::StringView name);
      void Reply(::fidl::BytePart _buffer, int32_t status, ::fidl::StringView name);
      void Reply(::fidl::DecodedMessage<GetBoardNameResponse> params);

     protected:
      using ::fidl::CompleterBase::CompleterBase;
    };

    using GetBoardNameCompleter = ::fidl::Completer<GetBoardNameCompleterBase>;

    virtual void GetBoardName(GetBoardNameCompleter::Sync _completer) = 0;

    class GetBoardRevisionCompleterBase : public _Base {
     public:
      void Reply(int32_t status, uint32_t revision);
      void Reply(::fidl::BytePart _buffer, int32_t status, uint32_t revision);
      void Reply(::fidl::DecodedMessage<GetBoardRevisionResponse> params);

     protected:
      using ::fidl::CompleterBase::CompleterBase;
    };

    using GetBoardRevisionCompleter = ::fidl::Completer<GetBoardRevisionCompleterBase>;

    virtual void GetBoardRevision(GetBoardRevisionCompleter::Sync _completer) { _completer.Close(ZX_ERR_NOT_SUPPORTED); }

    class GetInterruptControllerInfoCompleterBase : public _Base {
     public:
      void Reply(int32_t status, ::llcpp::fuchsia::sysinfo::InterruptControllerInfo* info);
      void Reply(::fidl::BytePart _buffer, int32_t status, ::llcpp::fuchsia::sysinfo::InterruptControllerInfo* info);
      void Reply(::fidl::DecodedMessage<GetInterruptControllerInfoResponse> params);

     protected:
      using ::fidl::CompleterBase::CompleterBase;
    };

    using GetInterruptControllerInfoCompleter = ::fidl::Completer<GetInterruptControllerInfoCompleterBase>;

    virtual void GetInterruptControllerInfo(GetInterruptControllerInfoCompleter::Sync _completer) = 0;

  };

  // Attempts to dispatch the incoming message to a handler function in the server implementation.
  // If there is no matching handler, it returns false, leaving the message and transaction intact.
  // In all other cases, it consumes the message and returns true.
  // It is possible to chain multiple TryDispatch functions in this manner.
  static bool TryDispatch(Interface* impl, fidl_msg_t* msg, ::fidl::Transaction* txn);

  // Dispatches the incoming message to one of the handlers functions in the interface.
  // If there is no matching handler, it closes all the handles in |msg| and closes the channel with
  // a |ZX_ERR_NOT_SUPPORTED| epitaph, before returning false. The message should then be discarded.
  static bool Dispatch(Interface* impl, fidl_msg_t* msg, ::fidl::Transaction* txn);

  // Same as |Dispatch|, but takes a |void*| instead of |Interface*|. Only used with |fidl::Bind|
  // to reduce template expansion.
  // Do not call this method manually. Use |Dispatch| instead.
  static bool TypeErasedDispatch(void* impl, fidl_msg_t* msg, ::fidl::Transaction* txn) {
    return Dispatch(static_cast<Interface*>(impl), msg, txn);
  }


  // Helper functions to fill in the transaction header in a |DecodedMessage<TransactionalMessage>|.
  class SetTransactionHeaderFor final {
    SetTransactionHeaderFor() = delete;
   public:
    static void GetHypervisorResourceRequest(const ::fidl::DecodedMessage<Device::GetHypervisorResourceRequest>& _msg);
    static void GetHypervisorResourceResponse(const ::fidl::DecodedMessage<Device::GetHypervisorResourceResponse>& _msg);
    static void GetBoardNameRequest(const ::fidl::DecodedMessage<Device::GetBoardNameRequest>& _msg);
    static void GetBoardNameResponse(const ::fidl::DecodedMessage<Device::GetBoardNameResponse>& _msg);
    static void GetBoardRevisionRequest(const ::fidl::DecodedMessage<Device::GetBoardRevisionRequest>& _msg);
    static void GetBoardRevisionResponse(const ::fidl::DecodedMessage<Device::GetBoardRevisionResponse>& _msg);
    static void GetInterruptControllerInfoRequest(const ::fidl::DecodedMessage<Device::GetInterruptControllerInfoRequest>& _msg);
    static void GetInterruptControllerInfoResponse(const ::fidl::DecodedMessage<Device::GetInterruptControllerInfoResponse>& _msg);
  };
};

constexpr uint8_t SYSINFO_BOARD_NAME_LEN = 32u;

extern "C" const fidl_type_t fuchsia_sysinfo_InterruptControllerInfoTable;
extern "C" const fidl_type_t v1_fuchsia_sysinfo_InterruptControllerInfoTable;

struct InterruptControllerInfo {
  static constexpr const fidl_type_t* Type = &v1_fuchsia_sysinfo_InterruptControllerInfoTable;
  static constexpr uint32_t MaxNumHandles = 0;
  static constexpr uint32_t PrimarySize = 4;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

  ::llcpp::fuchsia::sysinfo::InterruptControllerType type = {};
};

}  // namespace sysinfo
}  // namespace fuchsia
}  // namespace llcpp

namespace fidl {

template <>
struct IsFidlType<::llcpp::fuchsia::sysinfo::Device::GetHypervisorResourceResponse> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::sysinfo::Device::GetHypervisorResourceResponse> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::sysinfo::Device::GetHypervisorResourceResponse)
    == ::llcpp::fuchsia::sysinfo::Device::GetHypervisorResourceResponse::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::sysinfo::Device::GetHypervisorResourceResponse, status) == 16);
static_assert(offsetof(::llcpp::fuchsia::sysinfo::Device::GetHypervisorResourceResponse, resource) == 20);

template <>
struct IsFidlType<::llcpp::fuchsia::sysinfo::Device::GetBoardNameResponse> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::sysinfo::Device::GetBoardNameResponse> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::sysinfo::Device::GetBoardNameResponse)
    == ::llcpp::fuchsia::sysinfo::Device::GetBoardNameResponse::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::sysinfo::Device::GetBoardNameResponse, status) == 16);
static_assert(offsetof(::llcpp::fuchsia::sysinfo::Device::GetBoardNameResponse, name) == 24);

template <>
struct IsFidlType<::llcpp::fuchsia::sysinfo::Device::GetBoardRevisionResponse> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::sysinfo::Device::GetBoardRevisionResponse> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::sysinfo::Device::GetBoardRevisionResponse)
    == ::llcpp::fuchsia::sysinfo::Device::GetBoardRevisionResponse::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::sysinfo::Device::GetBoardRevisionResponse, status) == 16);
static_assert(offsetof(::llcpp::fuchsia::sysinfo::Device::GetBoardRevisionResponse, revision) == 20);

template <>
struct IsFidlType<::llcpp::fuchsia::sysinfo::Device::GetInterruptControllerInfoResponse> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::sysinfo::Device::GetInterruptControllerInfoResponse> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::sysinfo::Device::GetInterruptControllerInfoResponse)
    == ::llcpp::fuchsia::sysinfo::Device::GetInterruptControllerInfoResponse::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::sysinfo::Device::GetInterruptControllerInfoResponse, status) == 16);
static_assert(offsetof(::llcpp::fuchsia::sysinfo::Device::GetInterruptControllerInfoResponse, info) == 24);

template <>
struct IsFidlType<::llcpp::fuchsia::sysinfo::InterruptControllerInfo> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::sysinfo::InterruptControllerInfo>);
static_assert(offsetof(::llcpp::fuchsia::sysinfo::InterruptControllerInfo, type) == 0);
static_assert(sizeof(::llcpp::fuchsia::sysinfo::InterruptControllerInfo) == ::llcpp::fuchsia::sysinfo::InterruptControllerInfo::PrimarySize);

}  // namespace fidl
