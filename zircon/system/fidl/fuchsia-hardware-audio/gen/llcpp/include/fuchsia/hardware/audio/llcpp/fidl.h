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
#include <zircon/fidl.h>

namespace llcpp {

namespace fuchsia {
namespace hardware {
namespace audio {

class Device;

extern "C" const fidl_type_t fuchsia_hardware_audio_DeviceGetChannelResponseTable;

class Device final {
  Device() = delete;
 public:

  struct GetChannelResponse final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    ::zx::channel ch;

    static constexpr const fidl_type_t* Type = &fuchsia_hardware_audio_DeviceGetChannelResponseTable;
    static constexpr uint32_t MaxNumHandles = 1;
    static constexpr uint32_t PrimarySize = 24;
    static constexpr uint32_t MaxOutOfLine = 0;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kResponse;
  };
  using GetChannelRequest = ::fidl::AnyZeroArgMessage;


  // Collection of return types of FIDL calls in this interface.
  class ResultOf final {
    ResultOf() = delete;
   private:
    template <typename ResponseType>
    class GetChannel_Impl final : private ::fidl::internal::OwnedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::OwnedSyncCallBase<ResponseType>;
     public:
      GetChannel_Impl(zx::unowned_channel _client_end);
      ~GetChannel_Impl() = default;
      GetChannel_Impl(GetChannel_Impl&& other) = default;
      GetChannel_Impl& operator=(GetChannel_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };

   public:
    using GetChannel = GetChannel_Impl<GetChannelResponse>;
  };

  // Collection of return types of FIDL calls in this interface,
  // when the caller-allocate flavor or in-place call is used.
  class UnownedResultOf final {
    UnownedResultOf() = delete;
   private:
    template <typename ResponseType>
    class GetChannel_Impl final : private ::fidl::internal::UnownedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::UnownedSyncCallBase<ResponseType>;
     public:
      GetChannel_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);
      ~GetChannel_Impl() = default;
      GetChannel_Impl(GetChannel_Impl&& other) = default;
      GetChannel_Impl& operator=(GetChannel_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };

   public:
    using GetChannel = GetChannel_Impl<GetChannelResponse>;
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
    ResultOf::GetChannel GetChannel();

    // Caller provides the backing storage for FIDL message via request and response buffers.
    UnownedResultOf::GetChannel GetChannel(::fidl::BytePart _response_buffer);

   private:
    ::zx::channel channel_;
  };

  // Methods to make a sync FIDL call directly on an unowned channel, avoiding setting up a client.
  class Call final {
    Call() = delete;
   public:

    // Allocates 40 bytes of message buffer on the stack. No heap allocation necessary.
    static ResultOf::GetChannel GetChannel(zx::unowned_channel _client_end);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    static UnownedResultOf::GetChannel GetChannel(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);

  };

  // Messages are encoded and decoded in-place when these methods are used.
  // Additionally, requests must be already laid-out according to the FIDL wire-format.
  class InPlace final {
    InPlace() = delete;
   public:

    static ::fidl::DecodeResult<GetChannelResponse> GetChannel(zx::unowned_channel _client_end, ::fidl::BytePart response_buffer);

  };

  // Pure-virtual interface to be implemented by a server.
  class Interface {
   public:
    Interface() = default;
    virtual ~Interface() = default;
    using _Outer = Device;
    using _Base = ::fidl::CompleterBase;

    class GetChannelCompleterBase : public _Base {
     public:
      void Reply(::zx::channel ch);
      void Reply(::fidl::BytePart _buffer, ::zx::channel ch);
      void Reply(::fidl::DecodedMessage<GetChannelResponse> params);

     protected:
      using ::fidl::CompleterBase::CompleterBase;
    };

    using GetChannelCompleter = ::fidl::Completer<GetChannelCompleterBase>;

    virtual void GetChannel(GetChannelCompleter::Sync _completer) = 0;

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
    static void GetChannelRequest(const ::fidl::DecodedMessage<Device::GetChannelRequest>& _msg);
    static void GetChannelResponse(const ::fidl::DecodedMessage<Device::GetChannelResponse>& _msg);
  };
};

}  // namespace audio
}  // namespace hardware
}  // namespace fuchsia
}  // namespace llcpp

namespace fidl {

template <>
struct IsFidlType<::llcpp::fuchsia::hardware::audio::Device::GetChannelResponse> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::hardware::audio::Device::GetChannelResponse> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::hardware::audio::Device::GetChannelResponse)
    == ::llcpp::fuchsia::hardware::audio::Device::GetChannelResponse::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::hardware::audio::Device::GetChannelResponse, ch) == 16);

}  // namespace fidl
