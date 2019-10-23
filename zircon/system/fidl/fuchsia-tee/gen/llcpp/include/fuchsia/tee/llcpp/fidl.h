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
#include <lib/zx/vmo.h>
#include <zircon/fidl.h>

namespace llcpp {

namespace fuchsia {
namespace tee {

struct Uuid;
enum class ReturnOrigin : uint32_t {
  COMMUNICATION = 0u,
  TRUSTED_OS = 1u,
  TRUSTED_APPLICATION = 2u,
};


struct OsRevision;
struct OsInfo;
struct Empty;
enum class Direction : uint32_t {
  INPUT = 0u,
  OUTPUT = 1u,
  INOUT = 2u,
};


struct Value;
struct Buffer;
struct Parameter;
struct ParameterSet;
struct OpResult;
class Device;



struct Uuid {
  static constexpr const fidl_type_t* Type = nullptr;
  static constexpr uint32_t MaxNumHandles = 0;
  static constexpr uint32_t PrimarySize = 16;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

  uint32_t time_low = {};

  uint16_t time_mid = {};

  uint16_t time_hi_and_version = {};

  ::fidl::Array<uint8_t, 8> clock_seq_and_node = {};
};



struct OsRevision {
  static constexpr const fidl_type_t* Type = nullptr;
  static constexpr uint32_t MaxNumHandles = 0;
  static constexpr uint32_t PrimarySize = 8;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

  uint32_t major = {};

  uint32_t minor = {};
};

extern "C" const fidl_type_t fuchsia_tee_OsInfoTable;

struct OsInfo {
  static constexpr const fidl_type_t* Type = &fuchsia_tee_OsInfoTable;
  static constexpr uint32_t MaxNumHandles = 0;
  static constexpr uint32_t PrimarySize = 28;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

  ::llcpp::fuchsia::tee::Uuid uuid = {};

  ::llcpp::fuchsia::tee::OsRevision revision = {};

  bool is_global_platform_compliant = {};
};



struct Empty {
  static constexpr const fidl_type_t* Type = nullptr;
  static constexpr uint32_t MaxNumHandles = 0;
  static constexpr uint32_t PrimarySize = 1;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

  uint8_t __reserved = {};
};

extern "C" const fidl_type_t fuchsia_tee_ValueTable;

struct Value {
  static constexpr const fidl_type_t* Type = &fuchsia_tee_ValueTable;
  static constexpr uint32_t MaxNumHandles = 0;
  static constexpr uint32_t PrimarySize = 32;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

  ::llcpp::fuchsia::tee::Direction direction = {};

  uint64_t a = {};

  uint64_t b = {};

  uint64_t c = {};
};

extern "C" const fidl_type_t fuchsia_tee_BufferTable;

struct Buffer {
  static constexpr const fidl_type_t* Type = &fuchsia_tee_BufferTable;
  static constexpr uint32_t MaxNumHandles = 1;
  static constexpr uint32_t PrimarySize = 24;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

  ::llcpp::fuchsia::tee::Direction direction = {};

  ::zx::vmo vmo = {};

  uint64_t offset = {};

  uint64_t size = {};
};

extern "C" const fidl_type_t fuchsia_tee_ParameterTable;

struct Parameter {
  enum class Tag : fidl_union_tag_t {
    kEmpty = 0,
    kBuffer = 1,
    kValue = 2,
    Invalid = ::std::numeric_limits<::fidl_union_tag_t>::max(),
  };

  Parameter();
  ~Parameter();

  Parameter(Parameter&& other) {
    tag_ = Tag::Invalid;
    if (this != &other) {
      MoveImpl_(std::move(other));
    }
  }

  Parameter& operator=(Parameter&& other) {
    if (this != &other) {
      MoveImpl_(std::move(other));
    }
    return *this;
  }

  bool has_invalid_tag() const { return tag_ == Tag::Invalid; }

  bool is_empty() const { return tag_ == Tag::kEmpty; }

  static Parameter WithEmpty(::llcpp::fuchsia::tee::Empty&& val) {
    Parameter result;
    result.set_empty(std::move(val));
    return result;
  }

  ::llcpp::fuchsia::tee::Empty& mutable_empty();

  template <typename T>
  std::enable_if_t<std::is_convertible<T, ::llcpp::fuchsia::tee::Empty>::value && std::is_copy_assignable<T>::value>
  set_empty(const T& v) {
    mutable_empty() = v;
  }

  template <typename T>
  std::enable_if_t<std::is_convertible<T, ::llcpp::fuchsia::tee::Empty>::value && std::is_move_assignable<T>::value>
  set_empty(T&& v) {
    mutable_empty() = std::move(v);
  }

  ::llcpp::fuchsia::tee::Empty const & empty() const { return empty_; }

  bool is_buffer() const { return tag_ == Tag::kBuffer; }

  static Parameter WithBuffer(::llcpp::fuchsia::tee::Buffer&& val) {
    Parameter result;
    result.set_buffer(std::move(val));
    return result;
  }

  ::llcpp::fuchsia::tee::Buffer& mutable_buffer();

  template <typename T>
  std::enable_if_t<std::is_convertible<T, ::llcpp::fuchsia::tee::Buffer>::value && std::is_copy_assignable<T>::value>
  set_buffer(const T& v) {
    mutable_buffer() = v;
  }

  template <typename T>
  std::enable_if_t<std::is_convertible<T, ::llcpp::fuchsia::tee::Buffer>::value && std::is_move_assignable<T>::value>
  set_buffer(T&& v) {
    mutable_buffer() = std::move(v);
  }

  ::llcpp::fuchsia::tee::Buffer const & buffer() const { return buffer_; }

  bool is_value() const { return tag_ == Tag::kValue; }

  static Parameter WithValue(::llcpp::fuchsia::tee::Value&& val) {
    Parameter result;
    result.set_value(std::move(val));
    return result;
  }

  ::llcpp::fuchsia::tee::Value& mutable_value();

  template <typename T>
  std::enable_if_t<std::is_convertible<T, ::llcpp::fuchsia::tee::Value>::value && std::is_copy_assignable<T>::value>
  set_value(const T& v) {
    mutable_value() = v;
  }

  template <typename T>
  std::enable_if_t<std::is_convertible<T, ::llcpp::fuchsia::tee::Value>::value && std::is_move_assignable<T>::value>
  set_value(T&& v) {
    mutable_value() = std::move(v);
  }

  ::llcpp::fuchsia::tee::Value const & value() const { return value_; }

  Tag which() const { return tag_; }

  static constexpr const fidl_type_t* Type = &fuchsia_tee_ParameterTable;
  static constexpr uint32_t MaxNumHandles = 1;
  static constexpr uint32_t PrimarySize = 40;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

 private:
  void Destroy();
  void MoveImpl_(Parameter&& other);
  static void SizeAndOffsetAssertionHelper();
  Tag tag_;
  union {
    ::llcpp::fuchsia::tee::Empty empty_;
    ::llcpp::fuchsia::tee::Buffer buffer_;
    ::llcpp::fuchsia::tee::Value value_;
  };
};

extern "C" const fidl_type_t fuchsia_tee_ParameterSetTable;

struct ParameterSet {
  static constexpr const fidl_type_t* Type = &fuchsia_tee_ParameterSetTable;
  static constexpr uint32_t MaxNumHandles = 4;
  static constexpr uint32_t PrimarySize = 168;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

  uint16_t count = {};

  ::fidl::Array<::llcpp::fuchsia::tee::Parameter, 4> parameters = {};
};

extern "C" const fidl_type_t fuchsia_tee_OpResultTable;

struct OpResult {
  static constexpr const fidl_type_t* Type = &fuchsia_tee_OpResultTable;
  static constexpr uint32_t MaxNumHandles = 4;
  static constexpr uint32_t PrimarySize = 184;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

  uint64_t return_code = {};

  ::llcpp::fuchsia::tee::ReturnOrigin return_origin = {};

  ::llcpp::fuchsia::tee::ParameterSet parameter_set = {};
};

extern "C" const fidl_type_t fuchsia_tee_DeviceGetOsInfoResponseTable;
extern "C" const fidl_type_t fuchsia_tee_DeviceOpenSessionRequestTable;
extern "C" const fidl_type_t fuchsia_tee_DeviceOpenSessionResponseTable;
extern "C" const fidl_type_t fuchsia_tee_DeviceInvokeCommandRequestTable;
extern "C" const fidl_type_t fuchsia_tee_DeviceInvokeCommandResponseTable;
extern "C" const fidl_type_t fuchsia_tee_DeviceCloseSessionRequestTable;

class Device final {
  Device() = delete;
 public:
  static constexpr char Name[] = "fuchsia.tee.Device";

  struct GetOsInfoResponse final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    ::llcpp::fuchsia::tee::OsInfo info;

    static constexpr const fidl_type_t* Type = &fuchsia_tee_DeviceGetOsInfoResponseTable;
    static constexpr uint32_t MaxNumHandles = 0;
    static constexpr uint32_t PrimarySize = 48;
    static constexpr uint32_t MaxOutOfLine = 0;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kResponse;
  };
  using GetOsInfoRequest = ::fidl::AnyZeroArgMessage;

  struct OpenSessionResponse final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    uint32_t session_id;
    ::llcpp::fuchsia::tee::OpResult op_result;

    static constexpr const fidl_type_t* Type = &fuchsia_tee_DeviceOpenSessionResponseTable;
    static constexpr uint32_t MaxNumHandles = 4;
    static constexpr uint32_t PrimarySize = 208;
    static constexpr uint32_t MaxOutOfLine = 0;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kResponse;
  };
  struct OpenSessionRequest final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    ::llcpp::fuchsia::tee::Uuid trusted_app;
    ::llcpp::fuchsia::tee::ParameterSet parameter_set;

    static constexpr const fidl_type_t* Type = &fuchsia_tee_DeviceOpenSessionRequestTable;
    static constexpr uint32_t MaxNumHandles = 4;
    static constexpr uint32_t PrimarySize = 200;
    static constexpr uint32_t MaxOutOfLine = 0;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kRequest;
    using ResponseType = OpenSessionResponse;
  };

  struct InvokeCommandResponse final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    ::llcpp::fuchsia::tee::OpResult op_result;

    static constexpr const fidl_type_t* Type = &fuchsia_tee_DeviceInvokeCommandResponseTable;
    static constexpr uint32_t MaxNumHandles = 4;
    static constexpr uint32_t PrimarySize = 200;
    static constexpr uint32_t MaxOutOfLine = 0;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kResponse;
  };
  struct InvokeCommandRequest final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    uint32_t session_id;
    uint32_t command_id;
    ::llcpp::fuchsia::tee::ParameterSet parameter_set;

    static constexpr const fidl_type_t* Type = &fuchsia_tee_DeviceInvokeCommandRequestTable;
    static constexpr uint32_t MaxNumHandles = 4;
    static constexpr uint32_t PrimarySize = 192;
    static constexpr uint32_t MaxOutOfLine = 0;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kRequest;
    using ResponseType = InvokeCommandResponse;
  };

  using CloseSessionResponse = ::fidl::AnyZeroArgMessage;
  struct CloseSessionRequest final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    uint32_t session_id;

    static constexpr const fidl_type_t* Type = &fuchsia_tee_DeviceCloseSessionRequestTable;
    static constexpr uint32_t MaxNumHandles = 0;
    static constexpr uint32_t PrimarySize = 24;
    static constexpr uint32_t MaxOutOfLine = 0;
    static constexpr bool HasFlexibleEnvelope = false;
    static constexpr ::fidl::internal::TransactionalMessageKind MessageKind =
        ::fidl::internal::TransactionalMessageKind::kRequest;
  };


  // Collection of return types of FIDL calls in this interface.
  class ResultOf final {
    ResultOf() = delete;
   private:
    template <typename ResponseType>
    class GetOsInfo_Impl final : private ::fidl::internal::OwnedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::OwnedSyncCallBase<ResponseType>;
     public:
      GetOsInfo_Impl(zx::unowned_channel _client_end);
      ~GetOsInfo_Impl() = default;
      GetOsInfo_Impl(GetOsInfo_Impl&& other) = default;
      GetOsInfo_Impl& operator=(GetOsInfo_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class OpenSession_Impl final : private ::fidl::internal::OwnedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::OwnedSyncCallBase<ResponseType>;
     public:
      OpenSession_Impl(zx::unowned_channel _client_end, ::llcpp::fuchsia::tee::Uuid trusted_app, ::llcpp::fuchsia::tee::ParameterSet parameter_set);
      ~OpenSession_Impl() = default;
      OpenSession_Impl(OpenSession_Impl&& other) = default;
      OpenSession_Impl& operator=(OpenSession_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class InvokeCommand_Impl final : private ::fidl::internal::OwnedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::OwnedSyncCallBase<ResponseType>;
     public:
      InvokeCommand_Impl(zx::unowned_channel _client_end, uint32_t session_id, uint32_t command_id, ::llcpp::fuchsia::tee::ParameterSet parameter_set);
      ~InvokeCommand_Impl() = default;
      InvokeCommand_Impl(InvokeCommand_Impl&& other) = default;
      InvokeCommand_Impl& operator=(InvokeCommand_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class CloseSession_Impl final : private ::fidl::internal::OwnedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::OwnedSyncCallBase<ResponseType>;
     public:
      CloseSession_Impl(zx::unowned_channel _client_end, uint32_t session_id);
      ~CloseSession_Impl() = default;
      CloseSession_Impl(CloseSession_Impl&& other) = default;
      CloseSession_Impl& operator=(CloseSession_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };

   public:
    using GetOsInfo = GetOsInfo_Impl<GetOsInfoResponse>;
    using OpenSession = OpenSession_Impl<OpenSessionResponse>;
    using InvokeCommand = InvokeCommand_Impl<InvokeCommandResponse>;
    using CloseSession = CloseSession_Impl<CloseSessionResponse>;
  };

  // Collection of return types of FIDL calls in this interface,
  // when the caller-allocate flavor or in-place call is used.
  class UnownedResultOf final {
    UnownedResultOf() = delete;
   private:
    template <typename ResponseType>
    class GetOsInfo_Impl final : private ::fidl::internal::UnownedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::UnownedSyncCallBase<ResponseType>;
     public:
      GetOsInfo_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);
      ~GetOsInfo_Impl() = default;
      GetOsInfo_Impl(GetOsInfo_Impl&& other) = default;
      GetOsInfo_Impl& operator=(GetOsInfo_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class OpenSession_Impl final : private ::fidl::internal::UnownedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::UnownedSyncCallBase<ResponseType>;
     public:
      OpenSession_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _request_buffer, ::llcpp::fuchsia::tee::Uuid trusted_app, ::llcpp::fuchsia::tee::ParameterSet parameter_set, ::fidl::BytePart _response_buffer);
      ~OpenSession_Impl() = default;
      OpenSession_Impl(OpenSession_Impl&& other) = default;
      OpenSession_Impl& operator=(OpenSession_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class InvokeCommand_Impl final : private ::fidl::internal::UnownedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::UnownedSyncCallBase<ResponseType>;
     public:
      InvokeCommand_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _request_buffer, uint32_t session_id, uint32_t command_id, ::llcpp::fuchsia::tee::ParameterSet parameter_set, ::fidl::BytePart _response_buffer);
      ~InvokeCommand_Impl() = default;
      InvokeCommand_Impl(InvokeCommand_Impl&& other) = default;
      InvokeCommand_Impl& operator=(InvokeCommand_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };
    template <typename ResponseType>
    class CloseSession_Impl final : private ::fidl::internal::UnownedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::UnownedSyncCallBase<ResponseType>;
     public:
      CloseSession_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _request_buffer, uint32_t session_id, ::fidl::BytePart _response_buffer);
      ~CloseSession_Impl() = default;
      CloseSession_Impl(CloseSession_Impl&& other) = default;
      CloseSession_Impl& operator=(CloseSession_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
      using Super::operator->;
      using Super::operator*;
    };

   public:
    using GetOsInfo = GetOsInfo_Impl<GetOsInfoResponse>;
    using OpenSession = OpenSession_Impl<OpenSessionResponse>;
    using InvokeCommand = InvokeCommand_Impl<InvokeCommandResponse>;
    using CloseSession = CloseSession_Impl<CloseSessionResponse>;
  };

  class SyncClient final {
   public:
    explicit SyncClient(::zx::channel channel) : channel_(std::move(channel)) {}
    ~SyncClient() = default;
    SyncClient(SyncClient&&) = default;
    SyncClient& operator=(SyncClient&&) = default;

    const ::zx::channel& channel() const { return channel_; }

    ::zx::channel* mutable_channel() { return &channel_; }

    // Allocates 64 bytes of message buffer on the stack. No heap allocation necessary.
    ResultOf::GetOsInfo GetOsInfo();

    // Caller provides the backing storage for FIDL message via request and response buffers.
    UnownedResultOf::GetOsInfo GetOsInfo(::fidl::BytePart _response_buffer);

    // Allocates 408 bytes of message buffer on the stack. No heap allocation necessary.
    ResultOf::OpenSession OpenSession(::llcpp::fuchsia::tee::Uuid trusted_app, ::llcpp::fuchsia::tee::ParameterSet parameter_set);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    UnownedResultOf::OpenSession OpenSession(::fidl::BytePart _request_buffer, ::llcpp::fuchsia::tee::Uuid trusted_app, ::llcpp::fuchsia::tee::ParameterSet parameter_set, ::fidl::BytePart _response_buffer);

    // Allocates 392 bytes of message buffer on the stack. No heap allocation necessary.
    ResultOf::InvokeCommand InvokeCommand(uint32_t session_id, uint32_t command_id, ::llcpp::fuchsia::tee::ParameterSet parameter_set);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    UnownedResultOf::InvokeCommand InvokeCommand(::fidl::BytePart _request_buffer, uint32_t session_id, uint32_t command_id, ::llcpp::fuchsia::tee::ParameterSet parameter_set, ::fidl::BytePart _response_buffer);

    // Allocates 40 bytes of message buffer on the stack. No heap allocation necessary.
    ResultOf::CloseSession CloseSession(uint32_t session_id);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    UnownedResultOf::CloseSession CloseSession(::fidl::BytePart _request_buffer, uint32_t session_id, ::fidl::BytePart _response_buffer);

   private:
    ::zx::channel channel_;
  };

  // Methods to make a sync FIDL call directly on an unowned channel, avoiding setting up a client.
  class Call final {
    Call() = delete;
   public:

    // Allocates 64 bytes of message buffer on the stack. No heap allocation necessary.
    static ResultOf::GetOsInfo GetOsInfo(zx::unowned_channel _client_end);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    static UnownedResultOf::GetOsInfo GetOsInfo(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);

    // Allocates 408 bytes of message buffer on the stack. No heap allocation necessary.
    static ResultOf::OpenSession OpenSession(zx::unowned_channel _client_end, ::llcpp::fuchsia::tee::Uuid trusted_app, ::llcpp::fuchsia::tee::ParameterSet parameter_set);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    static UnownedResultOf::OpenSession OpenSession(zx::unowned_channel _client_end, ::fidl::BytePart _request_buffer, ::llcpp::fuchsia::tee::Uuid trusted_app, ::llcpp::fuchsia::tee::ParameterSet parameter_set, ::fidl::BytePart _response_buffer);

    // Allocates 392 bytes of message buffer on the stack. No heap allocation necessary.
    static ResultOf::InvokeCommand InvokeCommand(zx::unowned_channel _client_end, uint32_t session_id, uint32_t command_id, ::llcpp::fuchsia::tee::ParameterSet parameter_set);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    static UnownedResultOf::InvokeCommand InvokeCommand(zx::unowned_channel _client_end, ::fidl::BytePart _request_buffer, uint32_t session_id, uint32_t command_id, ::llcpp::fuchsia::tee::ParameterSet parameter_set, ::fidl::BytePart _response_buffer);

    // Allocates 40 bytes of message buffer on the stack. No heap allocation necessary.
    static ResultOf::CloseSession CloseSession(zx::unowned_channel _client_end, uint32_t session_id);

    // Caller provides the backing storage for FIDL message via request and response buffers.
    static UnownedResultOf::CloseSession CloseSession(zx::unowned_channel _client_end, ::fidl::BytePart _request_buffer, uint32_t session_id, ::fidl::BytePart _response_buffer);

  };

  // Messages are encoded and decoded in-place when these methods are used.
  // Additionally, requests must be already laid-out according to the FIDL wire-format.
  class InPlace final {
    InPlace() = delete;
   public:

    static ::fidl::DecodeResult<GetOsInfoResponse> GetOsInfo(zx::unowned_channel _client_end, ::fidl::BytePart response_buffer);

    static ::fidl::DecodeResult<OpenSessionResponse> OpenSession(zx::unowned_channel _client_end, ::fidl::DecodedMessage<OpenSessionRequest> params, ::fidl::BytePart response_buffer);

    static ::fidl::DecodeResult<InvokeCommandResponse> InvokeCommand(zx::unowned_channel _client_end, ::fidl::DecodedMessage<InvokeCommandRequest> params, ::fidl::BytePart response_buffer);

    static ::fidl::DecodeResult<CloseSessionResponse> CloseSession(zx::unowned_channel _client_end, ::fidl::DecodedMessage<CloseSessionRequest> params, ::fidl::BytePart response_buffer);

  };

  // Pure-virtual interface to be implemented by a server.
  class Interface {
   public:
    Interface() = default;
    virtual ~Interface() = default;
    using _Outer = Device;
    using _Base = ::fidl::CompleterBase;

    class GetOsInfoCompleterBase : public _Base {
     public:
      void Reply(::llcpp::fuchsia::tee::OsInfo info);
      void Reply(::fidl::BytePart _buffer, ::llcpp::fuchsia::tee::OsInfo info);
      void Reply(::fidl::DecodedMessage<GetOsInfoResponse> params);

     protected:
      using ::fidl::CompleterBase::CompleterBase;
    };

    using GetOsInfoCompleter = ::fidl::Completer<GetOsInfoCompleterBase>;

    virtual void GetOsInfo(GetOsInfoCompleter::Sync _completer) = 0;

    class OpenSessionCompleterBase : public _Base {
     public:
      void Reply(uint32_t session_id, ::llcpp::fuchsia::tee::OpResult op_result);
      void Reply(::fidl::BytePart _buffer, uint32_t session_id, ::llcpp::fuchsia::tee::OpResult op_result);
      void Reply(::fidl::DecodedMessage<OpenSessionResponse> params);

     protected:
      using ::fidl::CompleterBase::CompleterBase;
    };

    using OpenSessionCompleter = ::fidl::Completer<OpenSessionCompleterBase>;

    virtual void OpenSession(::llcpp::fuchsia::tee::Uuid trusted_app, ::llcpp::fuchsia::tee::ParameterSet parameter_set, OpenSessionCompleter::Sync _completer) = 0;

    class InvokeCommandCompleterBase : public _Base {
     public:
      void Reply(::llcpp::fuchsia::tee::OpResult op_result);
      void Reply(::fidl::BytePart _buffer, ::llcpp::fuchsia::tee::OpResult op_result);
      void Reply(::fidl::DecodedMessage<InvokeCommandResponse> params);

     protected:
      using ::fidl::CompleterBase::CompleterBase;
    };

    using InvokeCommandCompleter = ::fidl::Completer<InvokeCommandCompleterBase>;

    virtual void InvokeCommand(uint32_t session_id, uint32_t command_id, ::llcpp::fuchsia::tee::ParameterSet parameter_set, InvokeCommandCompleter::Sync _completer) = 0;

    class CloseSessionCompleterBase : public _Base {
     public:
      void Reply();

     protected:
      using ::fidl::CompleterBase::CompleterBase;
    };

    using CloseSessionCompleter = ::fidl::Completer<CloseSessionCompleterBase>;

    virtual void CloseSession(uint32_t session_id, CloseSessionCompleter::Sync _completer) = 0;

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
    static void GetOsInfoRequest(const ::fidl::DecodedMessage<Device::GetOsInfoRequest>& _msg);
    static void GetOsInfoResponse(const ::fidl::DecodedMessage<Device::GetOsInfoResponse>& _msg);
    static void OpenSessionRequest(const ::fidl::DecodedMessage<Device::OpenSessionRequest>& _msg);
    static void OpenSessionResponse(const ::fidl::DecodedMessage<Device::OpenSessionResponse>& _msg);
    static void InvokeCommandRequest(const ::fidl::DecodedMessage<Device::InvokeCommandRequest>& _msg);
    static void InvokeCommandResponse(const ::fidl::DecodedMessage<Device::InvokeCommandResponse>& _msg);
    static void CloseSessionRequest(const ::fidl::DecodedMessage<Device::CloseSessionRequest>& _msg);
    static void CloseSessionResponse(const ::fidl::DecodedMessage<Device::CloseSessionResponse>& _msg);
  };
};

}  // namespace tee
}  // namespace fuchsia
}  // namespace llcpp

namespace fidl {

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Uuid> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::tee::Uuid>);
static_assert(offsetof(::llcpp::fuchsia::tee::Uuid, time_low) == 0);
static_assert(offsetof(::llcpp::fuchsia::tee::Uuid, time_mid) == 4);
static_assert(offsetof(::llcpp::fuchsia::tee::Uuid, time_hi_and_version) == 6);
static_assert(offsetof(::llcpp::fuchsia::tee::Uuid, clock_seq_and_node) == 8);
static_assert(sizeof(::llcpp::fuchsia::tee::Uuid) == ::llcpp::fuchsia::tee::Uuid::PrimarySize);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::OsRevision> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::tee::OsRevision>);
static_assert(offsetof(::llcpp::fuchsia::tee::OsRevision, major) == 0);
static_assert(offsetof(::llcpp::fuchsia::tee::OsRevision, minor) == 4);
static_assert(sizeof(::llcpp::fuchsia::tee::OsRevision) == ::llcpp::fuchsia::tee::OsRevision::PrimarySize);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::OsInfo> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::tee::OsInfo>);
static_assert(offsetof(::llcpp::fuchsia::tee::OsInfo, uuid) == 0);
static_assert(offsetof(::llcpp::fuchsia::tee::OsInfo, revision) == 16);
static_assert(offsetof(::llcpp::fuchsia::tee::OsInfo, is_global_platform_compliant) == 24);
static_assert(sizeof(::llcpp::fuchsia::tee::OsInfo) == ::llcpp::fuchsia::tee::OsInfo::PrimarySize);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Empty> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::tee::Empty>);
static_assert(offsetof(::llcpp::fuchsia::tee::Empty, __reserved) == 0);
static_assert(sizeof(::llcpp::fuchsia::tee::Empty) == ::llcpp::fuchsia::tee::Empty::PrimarySize);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Value> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::tee::Value>);
static_assert(offsetof(::llcpp::fuchsia::tee::Value, direction) == 0);
static_assert(offsetof(::llcpp::fuchsia::tee::Value, a) == 8);
static_assert(offsetof(::llcpp::fuchsia::tee::Value, b) == 16);
static_assert(offsetof(::llcpp::fuchsia::tee::Value, c) == 24);
static_assert(sizeof(::llcpp::fuchsia::tee::Value) == ::llcpp::fuchsia::tee::Value::PrimarySize);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Buffer> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::tee::Buffer>);
static_assert(offsetof(::llcpp::fuchsia::tee::Buffer, direction) == 0);
static_assert(offsetof(::llcpp::fuchsia::tee::Buffer, vmo) == 4);
static_assert(offsetof(::llcpp::fuchsia::tee::Buffer, offset) == 8);
static_assert(offsetof(::llcpp::fuchsia::tee::Buffer, size) == 16);
static_assert(sizeof(::llcpp::fuchsia::tee::Buffer) == ::llcpp::fuchsia::tee::Buffer::PrimarySize);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Parameter> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::tee::Parameter>);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::ParameterSet> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::tee::ParameterSet>);
static_assert(offsetof(::llcpp::fuchsia::tee::ParameterSet, count) == 0);
static_assert(offsetof(::llcpp::fuchsia::tee::ParameterSet, parameters) == 8);
static_assert(sizeof(::llcpp::fuchsia::tee::ParameterSet) == ::llcpp::fuchsia::tee::ParameterSet::PrimarySize);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::OpResult> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::tee::OpResult>);
static_assert(offsetof(::llcpp::fuchsia::tee::OpResult, return_code) == 0);
static_assert(offsetof(::llcpp::fuchsia::tee::OpResult, return_origin) == 8);
static_assert(offsetof(::llcpp::fuchsia::tee::OpResult, parameter_set) == 16);
static_assert(sizeof(::llcpp::fuchsia::tee::OpResult) == ::llcpp::fuchsia::tee::OpResult::PrimarySize);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Device::GetOsInfoResponse> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::tee::Device::GetOsInfoResponse> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::tee::Device::GetOsInfoResponse)
    == ::llcpp::fuchsia::tee::Device::GetOsInfoResponse::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::tee::Device::GetOsInfoResponse, info) == 16);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Device::OpenSessionRequest> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::tee::Device::OpenSessionRequest> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::tee::Device::OpenSessionRequest)
    == ::llcpp::fuchsia::tee::Device::OpenSessionRequest::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::tee::Device::OpenSessionRequest, trusted_app) == 16);
static_assert(offsetof(::llcpp::fuchsia::tee::Device::OpenSessionRequest, parameter_set) == 32);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Device::OpenSessionResponse> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::tee::Device::OpenSessionResponse> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::tee::Device::OpenSessionResponse)
    == ::llcpp::fuchsia::tee::Device::OpenSessionResponse::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::tee::Device::OpenSessionResponse, session_id) == 16);
static_assert(offsetof(::llcpp::fuchsia::tee::Device::OpenSessionResponse, op_result) == 24);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Device::InvokeCommandRequest> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::tee::Device::InvokeCommandRequest> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::tee::Device::InvokeCommandRequest)
    == ::llcpp::fuchsia::tee::Device::InvokeCommandRequest::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::tee::Device::InvokeCommandRequest, session_id) == 16);
static_assert(offsetof(::llcpp::fuchsia::tee::Device::InvokeCommandRequest, command_id) == 20);
static_assert(offsetof(::llcpp::fuchsia::tee::Device::InvokeCommandRequest, parameter_set) == 24);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Device::InvokeCommandResponse> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::tee::Device::InvokeCommandResponse> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::tee::Device::InvokeCommandResponse)
    == ::llcpp::fuchsia::tee::Device::InvokeCommandResponse::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::tee::Device::InvokeCommandResponse, op_result) == 16);

template <>
struct IsFidlType<::llcpp::fuchsia::tee::Device::CloseSessionRequest> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::tee::Device::CloseSessionRequest> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::tee::Device::CloseSessionRequest)
    == ::llcpp::fuchsia::tee::Device::CloseSessionRequest::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::tee::Device::CloseSessionRequest, session_id) == 16);

}  // namespace fidl
