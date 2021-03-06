// WARNING: This file is machine generated by fidlgen.

#include <fuchsia/sysinfo/llcpp/fidl.h>
#include <memory>

namespace llcpp {

namespace fuchsia {
namespace sysinfo {

namespace {

[[maybe_unused]]
constexpr uint64_t kDevice_GetRootJob_Ordinal = 0x650877d700000000lu;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetRootJobResponseTable;
[[maybe_unused]]
constexpr uint64_t kDevice_GetHypervisorResource_Ordinal = 0x3868a16b00000000lu;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetHypervisorResourceResponseTable;
[[maybe_unused]]
constexpr uint64_t kDevice_GetBoardName_Ordinal = 0x68768b6d00000000lu;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetBoardNameResponseTable;
[[maybe_unused]]
constexpr uint64_t kDevice_GetInterruptControllerInfo_Ordinal = 0x5f8bb9e400000000lu;
extern "C" const fidl_type_t fuchsia_sysinfo_DeviceGetInterruptControllerInfoResponseTable;

}  // namespace
template <>
Device::ResultOf::GetRootJob_Impl<Device::GetRootJobResponse>::GetRootJob_Impl(zx::unowned_channel _client_end) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetRootJobRequest>();
  ::fidl::internal::AlignedBuffer<_kWriteAllocSize> _write_bytes_inlined;
  auto& _write_bytes_array = _write_bytes_inlined;
  uint8_t* _write_bytes = _write_bytes_array.view().data();
  memset(_write_bytes, 0, GetRootJobRequest::PrimarySize);
  ::fidl::BytePart _request_bytes(_write_bytes, _kWriteAllocSize, sizeof(GetRootJobRequest));
  ::fidl::DecodedMessage<GetRootJobRequest> _decoded_request(std::move(_request_bytes));
  Super::SetResult(
      Device::InPlace::GetRootJob(std::move(_client_end), Super::response_buffer()));
}

Device::ResultOf::GetRootJob Device::SyncClient::GetRootJob() {
  return ResultOf::GetRootJob(zx::unowned_channel(this->channel_));
}

Device::ResultOf::GetRootJob Device::Call::GetRootJob(zx::unowned_channel _client_end) {
  return ResultOf::GetRootJob(std::move(_client_end));
}

template <>
Device::UnownedResultOf::GetRootJob_Impl<Device::GetRootJobResponse>::GetRootJob_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer) {
  FIDL_ALIGNDECL uint8_t _write_bytes[sizeof(GetRootJobRequest)] = {};
  ::fidl::BytePart _request_buffer(_write_bytes, sizeof(_write_bytes));
  memset(_request_buffer.data(), 0, GetRootJobRequest::PrimarySize);
  _request_buffer.set_actual(sizeof(GetRootJobRequest));
  ::fidl::DecodedMessage<GetRootJobRequest> _decoded_request(std::move(_request_buffer));
  Super::SetResult(
      Device::InPlace::GetRootJob(std::move(_client_end), std::move(_response_buffer)));
}

Device::UnownedResultOf::GetRootJob Device::SyncClient::GetRootJob(::fidl::BytePart _response_buffer) {
  return UnownedResultOf::GetRootJob(zx::unowned_channel(this->channel_), std::move(_response_buffer));
}

Device::UnownedResultOf::GetRootJob Device::Call::GetRootJob(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer) {
  return UnownedResultOf::GetRootJob(std::move(_client_end), std::move(_response_buffer));
}

zx_status_t Device::SyncClient::GetRootJob_Deprecated(int32_t* out_status, ::zx::job* out_job) {
  return Device::Call::GetRootJob_Deprecated(zx::unowned_channel(this->channel_), out_status, out_job);
}

zx_status_t Device::Call::GetRootJob_Deprecated(zx::unowned_channel _client_end, int32_t* out_status, ::zx::job* out_job) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetRootJobRequest>();
  FIDL_ALIGNDECL uint8_t _write_bytes[_kWriteAllocSize] = {};
  auto& _request = *reinterpret_cast<GetRootJobRequest*>(_write_bytes);
  _request._hdr.ordinal = kDevice_GetRootJob_Ordinal;
  ::fidl::BytePart _request_bytes(_write_bytes, _kWriteAllocSize, sizeof(GetRootJobRequest));
  ::fidl::DecodedMessage<GetRootJobRequest> _decoded_request(std::move(_request_bytes));
  auto _encode_request_result = ::fidl::Encode(std::move(_decoded_request));
  if (_encode_request_result.status != ZX_OK) {
    return _encode_request_result.status;
  }
  constexpr uint32_t _kReadAllocSize = ::fidl::internal::ClampedMessageSize<GetRootJobResponse>();
  FIDL_ALIGNDECL uint8_t _read_bytes[_kReadAllocSize];
  ::fidl::BytePart _response_bytes(_read_bytes, _kReadAllocSize);
  auto _call_result = ::fidl::Call<GetRootJobRequest, GetRootJobResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(_response_bytes));
  if (_call_result.status != ZX_OK) {
    return _call_result.status;
  }
  auto _decode_result = ::fidl::Decode(std::move(_call_result.message));
  if (_decode_result.status != ZX_OK) {
    return _decode_result.status;
  }
  auto& _response = *_decode_result.message.message();
  *out_status = std::move(_response.status);
  *out_job = std::move(_response.job);
  return ZX_OK;
}

::fidl::DecodeResult<Device::GetRootJobResponse> Device::SyncClient::GetRootJob_Deprecated(::fidl::BytePart _response_buffer, int32_t* out_status, ::zx::job* out_job) {
  return Device::Call::GetRootJob_Deprecated(zx::unowned_channel(this->channel_), std::move(_response_buffer), out_status, out_job);
}

::fidl::DecodeResult<Device::GetRootJobResponse> Device::Call::GetRootJob_Deprecated(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer, int32_t* out_status, ::zx::job* out_job) {
  FIDL_ALIGNDECL uint8_t _write_bytes[sizeof(GetRootJobRequest)] = {};
  ::fidl::BytePart _request_buffer(_write_bytes, sizeof(_write_bytes));
  auto& _request = *reinterpret_cast<GetRootJobRequest*>(_request_buffer.data());
  _request._hdr.ordinal = kDevice_GetRootJob_Ordinal;
  _request_buffer.set_actual(sizeof(GetRootJobRequest));
  ::fidl::DecodedMessage<GetRootJobRequest> _decoded_request(std::move(_request_buffer));
  auto _encode_request_result = ::fidl::Encode(std::move(_decoded_request));
  if (_encode_request_result.status != ZX_OK) {
    return ::fidl::DecodeResult<GetRootJobResponse>(_encode_request_result.status, _encode_request_result.error);
  }
  auto _call_result = ::fidl::Call<GetRootJobRequest, GetRootJobResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(_response_buffer));
  if (_call_result.status != ZX_OK) {
    return ::fidl::DecodeResult<GetRootJobResponse>(_call_result.status, _call_result.error);
  }
  auto _decode_result = ::fidl::Decode(std::move(_call_result.message));
  if (_decode_result.status != ZX_OK) {
    return _decode_result;
  }
  auto& _response = *_decode_result.message.message();
  *out_status = std::move(_response.status);
  *out_job = std::move(_response.job);
  return _decode_result;
}

::fidl::DecodeResult<Device::GetRootJobResponse> Device::InPlace::GetRootJob(zx::unowned_channel _client_end, ::fidl::BytePart response_buffer) {
  constexpr uint32_t _write_num_bytes = sizeof(GetRootJobRequest);
  ::fidl::internal::AlignedBuffer<_write_num_bytes> _write_bytes;
  ::fidl::BytePart _request_buffer = _write_bytes.view();
  _request_buffer.set_actual(_write_num_bytes);
  ::fidl::DecodedMessage<GetRootJobRequest> params(std::move(_request_buffer));
  params.message()->_hdr = {};
  params.message()->_hdr.ordinal = kDevice_GetRootJob_Ordinal;
  auto _encode_request_result = ::fidl::Encode(std::move(params));
  if (_encode_request_result.status != ZX_OK) {
    return ::fidl::DecodeResult<Device::GetRootJobResponse>::FromFailure(
        std::move(_encode_request_result));
  }
  auto _call_result = ::fidl::Call<GetRootJobRequest, GetRootJobResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(response_buffer));
  if (_call_result.status != ZX_OK) {
    return ::fidl::DecodeResult<Device::GetRootJobResponse>::FromFailure(
        std::move(_call_result));
  }
  return ::fidl::Decode(std::move(_call_result.message));
}

template <>
Device::ResultOf::GetHypervisorResource_Impl<Device::GetHypervisorResourceResponse>::GetHypervisorResource_Impl(zx::unowned_channel _client_end) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetHypervisorResourceRequest>();
  ::fidl::internal::AlignedBuffer<_kWriteAllocSize> _write_bytes_inlined;
  auto& _write_bytes_array = _write_bytes_inlined;
  uint8_t* _write_bytes = _write_bytes_array.view().data();
  memset(_write_bytes, 0, GetHypervisorResourceRequest::PrimarySize);
  ::fidl::BytePart _request_bytes(_write_bytes, _kWriteAllocSize, sizeof(GetHypervisorResourceRequest));
  ::fidl::DecodedMessage<GetHypervisorResourceRequest> _decoded_request(std::move(_request_bytes));
  Super::SetResult(
      Device::InPlace::GetHypervisorResource(std::move(_client_end), Super::response_buffer()));
}

Device::ResultOf::GetHypervisorResource Device::SyncClient::GetHypervisorResource() {
  return ResultOf::GetHypervisorResource(zx::unowned_channel(this->channel_));
}

Device::ResultOf::GetHypervisorResource Device::Call::GetHypervisorResource(zx::unowned_channel _client_end) {
  return ResultOf::GetHypervisorResource(std::move(_client_end));
}

template <>
Device::UnownedResultOf::GetHypervisorResource_Impl<Device::GetHypervisorResourceResponse>::GetHypervisorResource_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer) {
  FIDL_ALIGNDECL uint8_t _write_bytes[sizeof(GetHypervisorResourceRequest)] = {};
  ::fidl::BytePart _request_buffer(_write_bytes, sizeof(_write_bytes));
  memset(_request_buffer.data(), 0, GetHypervisorResourceRequest::PrimarySize);
  _request_buffer.set_actual(sizeof(GetHypervisorResourceRequest));
  ::fidl::DecodedMessage<GetHypervisorResourceRequest> _decoded_request(std::move(_request_buffer));
  Super::SetResult(
      Device::InPlace::GetHypervisorResource(std::move(_client_end), std::move(_response_buffer)));
}

Device::UnownedResultOf::GetHypervisorResource Device::SyncClient::GetHypervisorResource(::fidl::BytePart _response_buffer) {
  return UnownedResultOf::GetHypervisorResource(zx::unowned_channel(this->channel_), std::move(_response_buffer));
}

Device::UnownedResultOf::GetHypervisorResource Device::Call::GetHypervisorResource(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer) {
  return UnownedResultOf::GetHypervisorResource(std::move(_client_end), std::move(_response_buffer));
}

zx_status_t Device::SyncClient::GetHypervisorResource_Deprecated(int32_t* out_status, ::zx::resource* out_resource) {
  return Device::Call::GetHypervisorResource_Deprecated(zx::unowned_channel(this->channel_), out_status, out_resource);
}

zx_status_t Device::Call::GetHypervisorResource_Deprecated(zx::unowned_channel _client_end, int32_t* out_status, ::zx::resource* out_resource) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetHypervisorResourceRequest>();
  FIDL_ALIGNDECL uint8_t _write_bytes[_kWriteAllocSize] = {};
  auto& _request = *reinterpret_cast<GetHypervisorResourceRequest*>(_write_bytes);
  _request._hdr.ordinal = kDevice_GetHypervisorResource_Ordinal;
  ::fidl::BytePart _request_bytes(_write_bytes, _kWriteAllocSize, sizeof(GetHypervisorResourceRequest));
  ::fidl::DecodedMessage<GetHypervisorResourceRequest> _decoded_request(std::move(_request_bytes));
  auto _encode_request_result = ::fidl::Encode(std::move(_decoded_request));
  if (_encode_request_result.status != ZX_OK) {
    return _encode_request_result.status;
  }
  constexpr uint32_t _kReadAllocSize = ::fidl::internal::ClampedMessageSize<GetHypervisorResourceResponse>();
  FIDL_ALIGNDECL uint8_t _read_bytes[_kReadAllocSize];
  ::fidl::BytePart _response_bytes(_read_bytes, _kReadAllocSize);
  auto _call_result = ::fidl::Call<GetHypervisorResourceRequest, GetHypervisorResourceResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(_response_bytes));
  if (_call_result.status != ZX_OK) {
    return _call_result.status;
  }
  auto _decode_result = ::fidl::Decode(std::move(_call_result.message));
  if (_decode_result.status != ZX_OK) {
    return _decode_result.status;
  }
  auto& _response = *_decode_result.message.message();
  *out_status = std::move(_response.status);
  *out_resource = std::move(_response.resource);
  return ZX_OK;
}

::fidl::DecodeResult<Device::GetHypervisorResourceResponse> Device::SyncClient::GetHypervisorResource_Deprecated(::fidl::BytePart _response_buffer, int32_t* out_status, ::zx::resource* out_resource) {
  return Device::Call::GetHypervisorResource_Deprecated(zx::unowned_channel(this->channel_), std::move(_response_buffer), out_status, out_resource);
}

::fidl::DecodeResult<Device::GetHypervisorResourceResponse> Device::Call::GetHypervisorResource_Deprecated(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer, int32_t* out_status, ::zx::resource* out_resource) {
  FIDL_ALIGNDECL uint8_t _write_bytes[sizeof(GetHypervisorResourceRequest)] = {};
  ::fidl::BytePart _request_buffer(_write_bytes, sizeof(_write_bytes));
  auto& _request = *reinterpret_cast<GetHypervisorResourceRequest*>(_request_buffer.data());
  _request._hdr.ordinal = kDevice_GetHypervisorResource_Ordinal;
  _request_buffer.set_actual(sizeof(GetHypervisorResourceRequest));
  ::fidl::DecodedMessage<GetHypervisorResourceRequest> _decoded_request(std::move(_request_buffer));
  auto _encode_request_result = ::fidl::Encode(std::move(_decoded_request));
  if (_encode_request_result.status != ZX_OK) {
    return ::fidl::DecodeResult<GetHypervisorResourceResponse>(_encode_request_result.status, _encode_request_result.error);
  }
  auto _call_result = ::fidl::Call<GetHypervisorResourceRequest, GetHypervisorResourceResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(_response_buffer));
  if (_call_result.status != ZX_OK) {
    return ::fidl::DecodeResult<GetHypervisorResourceResponse>(_call_result.status, _call_result.error);
  }
  auto _decode_result = ::fidl::Decode(std::move(_call_result.message));
  if (_decode_result.status != ZX_OK) {
    return _decode_result;
  }
  auto& _response = *_decode_result.message.message();
  *out_status = std::move(_response.status);
  *out_resource = std::move(_response.resource);
  return _decode_result;
}

::fidl::DecodeResult<Device::GetHypervisorResourceResponse> Device::InPlace::GetHypervisorResource(zx::unowned_channel _client_end, ::fidl::BytePart response_buffer) {
  constexpr uint32_t _write_num_bytes = sizeof(GetHypervisorResourceRequest);
  ::fidl::internal::AlignedBuffer<_write_num_bytes> _write_bytes;
  ::fidl::BytePart _request_buffer = _write_bytes.view();
  _request_buffer.set_actual(_write_num_bytes);
  ::fidl::DecodedMessage<GetHypervisorResourceRequest> params(std::move(_request_buffer));
  params.message()->_hdr = {};
  params.message()->_hdr.ordinal = kDevice_GetHypervisorResource_Ordinal;
  auto _encode_request_result = ::fidl::Encode(std::move(params));
  if (_encode_request_result.status != ZX_OK) {
    return ::fidl::DecodeResult<Device::GetHypervisorResourceResponse>::FromFailure(
        std::move(_encode_request_result));
  }
  auto _call_result = ::fidl::Call<GetHypervisorResourceRequest, GetHypervisorResourceResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(response_buffer));
  if (_call_result.status != ZX_OK) {
    return ::fidl::DecodeResult<Device::GetHypervisorResourceResponse>::FromFailure(
        std::move(_call_result));
  }
  return ::fidl::Decode(std::move(_call_result.message));
}

template <>
Device::ResultOf::GetBoardName_Impl<Device::GetBoardNameResponse>::GetBoardName_Impl(zx::unowned_channel _client_end) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetBoardNameRequest>();
  ::fidl::internal::AlignedBuffer<_kWriteAllocSize> _write_bytes_inlined;
  auto& _write_bytes_array = _write_bytes_inlined;
  uint8_t* _write_bytes = _write_bytes_array.view().data();
  memset(_write_bytes, 0, GetBoardNameRequest::PrimarySize);
  ::fidl::BytePart _request_bytes(_write_bytes, _kWriteAllocSize, sizeof(GetBoardNameRequest));
  ::fidl::DecodedMessage<GetBoardNameRequest> _decoded_request(std::move(_request_bytes));
  Super::SetResult(
      Device::InPlace::GetBoardName(std::move(_client_end), Super::response_buffer()));
}

Device::ResultOf::GetBoardName Device::SyncClient::GetBoardName() {
  return ResultOf::GetBoardName(zx::unowned_channel(this->channel_));
}

Device::ResultOf::GetBoardName Device::Call::GetBoardName(zx::unowned_channel _client_end) {
  return ResultOf::GetBoardName(std::move(_client_end));
}

template <>
Device::UnownedResultOf::GetBoardName_Impl<Device::GetBoardNameResponse>::GetBoardName_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer) {
  FIDL_ALIGNDECL uint8_t _write_bytes[sizeof(GetBoardNameRequest)] = {};
  ::fidl::BytePart _request_buffer(_write_bytes, sizeof(_write_bytes));
  memset(_request_buffer.data(), 0, GetBoardNameRequest::PrimarySize);
  _request_buffer.set_actual(sizeof(GetBoardNameRequest));
  ::fidl::DecodedMessage<GetBoardNameRequest> _decoded_request(std::move(_request_buffer));
  Super::SetResult(
      Device::InPlace::GetBoardName(std::move(_client_end), std::move(_response_buffer)));
}

Device::UnownedResultOf::GetBoardName Device::SyncClient::GetBoardName(::fidl::BytePart _response_buffer) {
  return UnownedResultOf::GetBoardName(zx::unowned_channel(this->channel_), std::move(_response_buffer));
}

Device::UnownedResultOf::GetBoardName Device::Call::GetBoardName(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer) {
  return UnownedResultOf::GetBoardName(std::move(_client_end), std::move(_response_buffer));
}

::fidl::DecodeResult<Device::GetBoardNameResponse> Device::SyncClient::GetBoardName_Deprecated(::fidl::BytePart _response_buffer, int32_t* out_status, ::fidl::StringView* out_name) {
  return Device::Call::GetBoardName_Deprecated(zx::unowned_channel(this->channel_), std::move(_response_buffer), out_status, out_name);
}

::fidl::DecodeResult<Device::GetBoardNameResponse> Device::Call::GetBoardName_Deprecated(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer, int32_t* out_status, ::fidl::StringView* out_name) {
  FIDL_ALIGNDECL uint8_t _write_bytes[sizeof(GetBoardNameRequest)] = {};
  ::fidl::BytePart _request_buffer(_write_bytes, sizeof(_write_bytes));
  auto& _request = *reinterpret_cast<GetBoardNameRequest*>(_request_buffer.data());
  _request._hdr.ordinal = kDevice_GetBoardName_Ordinal;
  _request_buffer.set_actual(sizeof(GetBoardNameRequest));
  ::fidl::DecodedMessage<GetBoardNameRequest> _decoded_request(std::move(_request_buffer));
  auto _encode_request_result = ::fidl::Encode(std::move(_decoded_request));
  if (_encode_request_result.status != ZX_OK) {
    return ::fidl::DecodeResult<GetBoardNameResponse>(_encode_request_result.status, _encode_request_result.error);
  }
  auto _call_result = ::fidl::Call<GetBoardNameRequest, GetBoardNameResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(_response_buffer));
  if (_call_result.status != ZX_OK) {
    return ::fidl::DecodeResult<GetBoardNameResponse>(_call_result.status, _call_result.error);
  }
  auto _decode_result = ::fidl::Decode(std::move(_call_result.message));
  if (_decode_result.status != ZX_OK) {
    return _decode_result;
  }
  auto& _response = *_decode_result.message.message();
  *out_status = std::move(_response.status);
  *out_name = std::move(_response.name);
  return _decode_result;
}

::fidl::DecodeResult<Device::GetBoardNameResponse> Device::InPlace::GetBoardName(zx::unowned_channel _client_end, ::fidl::BytePart response_buffer) {
  constexpr uint32_t _write_num_bytes = sizeof(GetBoardNameRequest);
  ::fidl::internal::AlignedBuffer<_write_num_bytes> _write_bytes;
  ::fidl::BytePart _request_buffer = _write_bytes.view();
  _request_buffer.set_actual(_write_num_bytes);
  ::fidl::DecodedMessage<GetBoardNameRequest> params(std::move(_request_buffer));
  params.message()->_hdr = {};
  params.message()->_hdr.ordinal = kDevice_GetBoardName_Ordinal;
  auto _encode_request_result = ::fidl::Encode(std::move(params));
  if (_encode_request_result.status != ZX_OK) {
    return ::fidl::DecodeResult<Device::GetBoardNameResponse>::FromFailure(
        std::move(_encode_request_result));
  }
  auto _call_result = ::fidl::Call<GetBoardNameRequest, GetBoardNameResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(response_buffer));
  if (_call_result.status != ZX_OK) {
    return ::fidl::DecodeResult<Device::GetBoardNameResponse>::FromFailure(
        std::move(_call_result));
  }
  return ::fidl::Decode(std::move(_call_result.message));
}

template <>
Device::ResultOf::GetInterruptControllerInfo_Impl<Device::GetInterruptControllerInfoResponse>::GetInterruptControllerInfo_Impl(zx::unowned_channel _client_end) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetInterruptControllerInfoRequest>();
  ::fidl::internal::AlignedBuffer<_kWriteAllocSize> _write_bytes_inlined;
  auto& _write_bytes_array = _write_bytes_inlined;
  uint8_t* _write_bytes = _write_bytes_array.view().data();
  memset(_write_bytes, 0, GetInterruptControllerInfoRequest::PrimarySize);
  ::fidl::BytePart _request_bytes(_write_bytes, _kWriteAllocSize, sizeof(GetInterruptControllerInfoRequest));
  ::fidl::DecodedMessage<GetInterruptControllerInfoRequest> _decoded_request(std::move(_request_bytes));
  Super::SetResult(
      Device::InPlace::GetInterruptControllerInfo(std::move(_client_end), Super::response_buffer()));
}

Device::ResultOf::GetInterruptControllerInfo Device::SyncClient::GetInterruptControllerInfo() {
  return ResultOf::GetInterruptControllerInfo(zx::unowned_channel(this->channel_));
}

Device::ResultOf::GetInterruptControllerInfo Device::Call::GetInterruptControllerInfo(zx::unowned_channel _client_end) {
  return ResultOf::GetInterruptControllerInfo(std::move(_client_end));
}

template <>
Device::UnownedResultOf::GetInterruptControllerInfo_Impl<Device::GetInterruptControllerInfoResponse>::GetInterruptControllerInfo_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer) {
  FIDL_ALIGNDECL uint8_t _write_bytes[sizeof(GetInterruptControllerInfoRequest)] = {};
  ::fidl::BytePart _request_buffer(_write_bytes, sizeof(_write_bytes));
  memset(_request_buffer.data(), 0, GetInterruptControllerInfoRequest::PrimarySize);
  _request_buffer.set_actual(sizeof(GetInterruptControllerInfoRequest));
  ::fidl::DecodedMessage<GetInterruptControllerInfoRequest> _decoded_request(std::move(_request_buffer));
  Super::SetResult(
      Device::InPlace::GetInterruptControllerInfo(std::move(_client_end), std::move(_response_buffer)));
}

Device::UnownedResultOf::GetInterruptControllerInfo Device::SyncClient::GetInterruptControllerInfo(::fidl::BytePart _response_buffer) {
  return UnownedResultOf::GetInterruptControllerInfo(zx::unowned_channel(this->channel_), std::move(_response_buffer));
}

Device::UnownedResultOf::GetInterruptControllerInfo Device::Call::GetInterruptControllerInfo(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer) {
  return UnownedResultOf::GetInterruptControllerInfo(std::move(_client_end), std::move(_response_buffer));
}

::fidl::DecodeResult<Device::GetInterruptControllerInfoResponse> Device::SyncClient::GetInterruptControllerInfo_Deprecated(::fidl::BytePart _response_buffer, int32_t* out_status, InterruptControllerInfo** out_info) {
  return Device::Call::GetInterruptControllerInfo_Deprecated(zx::unowned_channel(this->channel_), std::move(_response_buffer), out_status, out_info);
}

::fidl::DecodeResult<Device::GetInterruptControllerInfoResponse> Device::Call::GetInterruptControllerInfo_Deprecated(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer, int32_t* out_status, InterruptControllerInfo** out_info) {
  FIDL_ALIGNDECL uint8_t _write_bytes[sizeof(GetInterruptControllerInfoRequest)] = {};
  ::fidl::BytePart _request_buffer(_write_bytes, sizeof(_write_bytes));
  auto& _request = *reinterpret_cast<GetInterruptControllerInfoRequest*>(_request_buffer.data());
  _request._hdr.ordinal = kDevice_GetInterruptControllerInfo_Ordinal;
  _request_buffer.set_actual(sizeof(GetInterruptControllerInfoRequest));
  ::fidl::DecodedMessage<GetInterruptControllerInfoRequest> _decoded_request(std::move(_request_buffer));
  auto _encode_request_result = ::fidl::Encode(std::move(_decoded_request));
  if (_encode_request_result.status != ZX_OK) {
    return ::fidl::DecodeResult<GetInterruptControllerInfoResponse>(_encode_request_result.status, _encode_request_result.error);
  }
  auto _call_result = ::fidl::Call<GetInterruptControllerInfoRequest, GetInterruptControllerInfoResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(_response_buffer));
  if (_call_result.status != ZX_OK) {
    return ::fidl::DecodeResult<GetInterruptControllerInfoResponse>(_call_result.status, _call_result.error);
  }
  auto _decode_result = ::fidl::Decode(std::move(_call_result.message));
  if (_decode_result.status != ZX_OK) {
    return _decode_result;
  }
  auto& _response = *_decode_result.message.message();
  *out_status = std::move(_response.status);
  *out_info = std::move(_response.info);
  return _decode_result;
}

::fidl::DecodeResult<Device::GetInterruptControllerInfoResponse> Device::InPlace::GetInterruptControllerInfo(zx::unowned_channel _client_end, ::fidl::BytePart response_buffer) {
  constexpr uint32_t _write_num_bytes = sizeof(GetInterruptControllerInfoRequest);
  ::fidl::internal::AlignedBuffer<_write_num_bytes> _write_bytes;
  ::fidl::BytePart _request_buffer = _write_bytes.view();
  _request_buffer.set_actual(_write_num_bytes);
  ::fidl::DecodedMessage<GetInterruptControllerInfoRequest> params(std::move(_request_buffer));
  params.message()->_hdr = {};
  params.message()->_hdr.ordinal = kDevice_GetInterruptControllerInfo_Ordinal;
  auto _encode_request_result = ::fidl::Encode(std::move(params));
  if (_encode_request_result.status != ZX_OK) {
    return ::fidl::DecodeResult<Device::GetInterruptControllerInfoResponse>::FromFailure(
        std::move(_encode_request_result));
  }
  auto _call_result = ::fidl::Call<GetInterruptControllerInfoRequest, GetInterruptControllerInfoResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(response_buffer));
  if (_call_result.status != ZX_OK) {
    return ::fidl::DecodeResult<Device::GetInterruptControllerInfoResponse>::FromFailure(
        std::move(_call_result));
  }
  return ::fidl::Decode(std::move(_call_result.message));
}


bool Device::TryDispatch(Interface* impl, fidl_msg_t* msg, ::fidl::Transaction* txn) {
  if (msg->num_bytes < sizeof(fidl_message_header_t)) {
    zx_handle_close_many(msg->handles, msg->num_handles);
    txn->Close(ZX_ERR_INVALID_ARGS);
    return true;
  }
  fidl_message_header_t* hdr = reinterpret_cast<fidl_message_header_t*>(msg->bytes);
  switch (hdr->ordinal) {
    case kDevice_GetRootJob_Ordinal:
    {
      auto result = ::fidl::DecodeAs<GetRootJobRequest>(msg);
      if (result.status != ZX_OK) {
        txn->Close(ZX_ERR_INVALID_ARGS);
        return true;
      }
      impl->GetRootJob(
        Interface::GetRootJobCompleter::Sync(txn));
      return true;
    }
    case kDevice_GetHypervisorResource_Ordinal:
    {
      auto result = ::fidl::DecodeAs<GetHypervisorResourceRequest>(msg);
      if (result.status != ZX_OK) {
        txn->Close(ZX_ERR_INVALID_ARGS);
        return true;
      }
      impl->GetHypervisorResource(
        Interface::GetHypervisorResourceCompleter::Sync(txn));
      return true;
    }
    case kDevice_GetBoardName_Ordinal:
    {
      auto result = ::fidl::DecodeAs<GetBoardNameRequest>(msg);
      if (result.status != ZX_OK) {
        txn->Close(ZX_ERR_INVALID_ARGS);
        return true;
      }
      impl->GetBoardName(
        Interface::GetBoardNameCompleter::Sync(txn));
      return true;
    }
    case kDevice_GetInterruptControllerInfo_Ordinal:
    {
      auto result = ::fidl::DecodeAs<GetInterruptControllerInfoRequest>(msg);
      if (result.status != ZX_OK) {
        txn->Close(ZX_ERR_INVALID_ARGS);
        return true;
      }
      impl->GetInterruptControllerInfo(
        Interface::GetInterruptControllerInfoCompleter::Sync(txn));
      return true;
    }
    default: {
      return false;
    }
  }
}

bool Device::Dispatch(Interface* impl, fidl_msg_t* msg, ::fidl::Transaction* txn) {
  bool found = TryDispatch(impl, msg, txn);
  if (!found) {
    zx_handle_close_many(msg->handles, msg->num_handles);
    txn->Close(ZX_ERR_NOT_SUPPORTED);
  }
  return found;
}


void Device::Interface::GetRootJobCompleterBase::Reply(int32_t status, ::zx::job job) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetRootJobResponse>();
  FIDL_ALIGNDECL uint8_t _write_bytes[_kWriteAllocSize] = {};
  auto& _response = *reinterpret_cast<GetRootJobResponse*>(_write_bytes);
  _response._hdr.ordinal = kDevice_GetRootJob_Ordinal;
  _response.status = std::move(status);
  _response.job = std::move(job);
  ::fidl::BytePart _response_bytes(_write_bytes, _kWriteAllocSize, sizeof(GetRootJobResponse));
  CompleterBase::SendReply(::fidl::DecodedMessage<GetRootJobResponse>(std::move(_response_bytes)));
}

void Device::Interface::GetRootJobCompleterBase::Reply(::fidl::BytePart _buffer, int32_t status, ::zx::job job) {
  if (_buffer.capacity() < GetRootJobResponse::PrimarySize) {
    CompleterBase::Close(ZX_ERR_INTERNAL);
    return;
  }
  auto& _response = *reinterpret_cast<GetRootJobResponse*>(_buffer.data());
  _response._hdr.ordinal = kDevice_GetRootJob_Ordinal;
  _response.status = std::move(status);
  _response.job = std::move(job);
  _buffer.set_actual(sizeof(GetRootJobResponse));
  CompleterBase::SendReply(::fidl::DecodedMessage<GetRootJobResponse>(std::move(_buffer)));
}

void Device::Interface::GetRootJobCompleterBase::Reply(::fidl::DecodedMessage<GetRootJobResponse> params) {
  params.message()->_hdr = {};
  params.message()->_hdr.ordinal = kDevice_GetRootJob_Ordinal;
  CompleterBase::SendReply(std::move(params));
}


void Device::Interface::GetHypervisorResourceCompleterBase::Reply(int32_t status, ::zx::resource resource) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetHypervisorResourceResponse>();
  FIDL_ALIGNDECL uint8_t _write_bytes[_kWriteAllocSize] = {};
  auto& _response = *reinterpret_cast<GetHypervisorResourceResponse*>(_write_bytes);
  _response._hdr.ordinal = kDevice_GetHypervisorResource_Ordinal;
  _response.status = std::move(status);
  _response.resource = std::move(resource);
  ::fidl::BytePart _response_bytes(_write_bytes, _kWriteAllocSize, sizeof(GetHypervisorResourceResponse));
  CompleterBase::SendReply(::fidl::DecodedMessage<GetHypervisorResourceResponse>(std::move(_response_bytes)));
}

void Device::Interface::GetHypervisorResourceCompleterBase::Reply(::fidl::BytePart _buffer, int32_t status, ::zx::resource resource) {
  if (_buffer.capacity() < GetHypervisorResourceResponse::PrimarySize) {
    CompleterBase::Close(ZX_ERR_INTERNAL);
    return;
  }
  auto& _response = *reinterpret_cast<GetHypervisorResourceResponse*>(_buffer.data());
  _response._hdr.ordinal = kDevice_GetHypervisorResource_Ordinal;
  _response.status = std::move(status);
  _response.resource = std::move(resource);
  _buffer.set_actual(sizeof(GetHypervisorResourceResponse));
  CompleterBase::SendReply(::fidl::DecodedMessage<GetHypervisorResourceResponse>(std::move(_buffer)));
}

void Device::Interface::GetHypervisorResourceCompleterBase::Reply(::fidl::DecodedMessage<GetHypervisorResourceResponse> params) {
  params.message()->_hdr = {};
  params.message()->_hdr.ordinal = kDevice_GetHypervisorResource_Ordinal;
  CompleterBase::SendReply(std::move(params));
}


void Device::Interface::GetBoardNameCompleterBase::Reply(int32_t status, ::fidl::StringView name) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetBoardNameResponse>();
  FIDL_ALIGNDECL uint8_t _write_bytes[_kWriteAllocSize];
  GetBoardNameResponse _response = {};
  _response._hdr.ordinal = kDevice_GetBoardName_Ordinal;
  _response.status = std::move(status);
  _response.name = std::move(name);
  auto _linearize_result = ::fidl::Linearize(&_response, ::fidl::BytePart(_write_bytes,
                                                                          _kWriteAllocSize));
  if (_linearize_result.status != ZX_OK) {
    CompleterBase::Close(ZX_ERR_INTERNAL);
    return;
  }
  CompleterBase::SendReply(std::move(_linearize_result.message));
}

void Device::Interface::GetBoardNameCompleterBase::Reply(::fidl::BytePart _buffer, int32_t status, ::fidl::StringView name) {
  if (_buffer.capacity() < GetBoardNameResponse::PrimarySize) {
    CompleterBase::Close(ZX_ERR_INTERNAL);
    return;
  }
  GetBoardNameResponse _response = {};
  _response._hdr.ordinal = kDevice_GetBoardName_Ordinal;
  _response.status = std::move(status);
  _response.name = std::move(name);
  auto _linearize_result = ::fidl::Linearize(&_response, std::move(_buffer));
  if (_linearize_result.status != ZX_OK) {
    CompleterBase::Close(ZX_ERR_INTERNAL);
    return;
  }
  CompleterBase::SendReply(std::move(_linearize_result.message));
}

void Device::Interface::GetBoardNameCompleterBase::Reply(::fidl::DecodedMessage<GetBoardNameResponse> params) {
  params.message()->_hdr = {};
  params.message()->_hdr.ordinal = kDevice_GetBoardName_Ordinal;
  CompleterBase::SendReply(std::move(params));
}


void Device::Interface::GetInterruptControllerInfoCompleterBase::Reply(int32_t status, InterruptControllerInfo* info) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetInterruptControllerInfoResponse>();
  FIDL_ALIGNDECL uint8_t _write_bytes[_kWriteAllocSize];
  GetInterruptControllerInfoResponse _response = {};
  _response._hdr.ordinal = kDevice_GetInterruptControllerInfo_Ordinal;
  _response.status = std::move(status);
  _response.info = std::move(info);
  auto _linearize_result = ::fidl::Linearize(&_response, ::fidl::BytePart(_write_bytes,
                                                                          _kWriteAllocSize));
  if (_linearize_result.status != ZX_OK) {
    CompleterBase::Close(ZX_ERR_INTERNAL);
    return;
  }
  CompleterBase::SendReply(std::move(_linearize_result.message));
}

void Device::Interface::GetInterruptControllerInfoCompleterBase::Reply(::fidl::BytePart _buffer, int32_t status, InterruptControllerInfo* info) {
  if (_buffer.capacity() < GetInterruptControllerInfoResponse::PrimarySize) {
    CompleterBase::Close(ZX_ERR_INTERNAL);
    return;
  }
  GetInterruptControllerInfoResponse _response = {};
  _response._hdr.ordinal = kDevice_GetInterruptControllerInfo_Ordinal;
  _response.status = std::move(status);
  _response.info = std::move(info);
  auto _linearize_result = ::fidl::Linearize(&_response, std::move(_buffer));
  if (_linearize_result.status != ZX_OK) {
    CompleterBase::Close(ZX_ERR_INTERNAL);
    return;
  }
  CompleterBase::SendReply(std::move(_linearize_result.message));
}

void Device::Interface::GetInterruptControllerInfoCompleterBase::Reply(::fidl::DecodedMessage<GetInterruptControllerInfoResponse> params) {
  params.message()->_hdr = {};
  params.message()->_hdr.ordinal = kDevice_GetInterruptControllerInfo_Ordinal;
  CompleterBase::SendReply(std::move(params));
}


}  // namespace sysinfo
}  // namespace fuchsia
}  // namespace llcpp
