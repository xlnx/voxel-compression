#pragma once

#include "nvEncodeAPI.h"
#include <VMUtils/fmt.hpp>

inline std::string nvEncStatusToString( NVENCSTATUS e )
{
	const char *aszErrName[] = {
		"NV_ENC_SUCCESS",
		"NV_ENC_ERR_NO_ENCODE_DEVICE",
		"NV_ENC_ERR_UNSUPPORTED_DEVICE",
		"NV_ENC_ERR_INVALID_ENCODERDEVICE",
		"NV_ENC_ERR_INVALID_DEVICE",
		"NV_ENC_ERR_DEVICE_NOT_EXIST",
		"NV_ENC_ERR_INVALID_PTR",
		"NV_ENC_ERR_INVALID_EVENT",
		"NV_ENC_ERR_INVALID_PARAM",
		"NV_ENC_ERR_INVALID_CALL",
		"NV_ENC_ERR_OUT_OF_MEMORY",
		"NV_ENC_ERR_ENCODER_NOT_INITIALIZED",
		"NV_ENC_ERR_UNSUPPORTED_PARAM",
		"NV_ENC_ERR_LOCK_BUSY",
		"NV_ENC_ERR_NOT_ENOUGH_BUFFER",
		"NV_ENC_ERR_INVALID_VERSION",
		"NV_ENC_ERR_MAP_FAILED",
		"NV_ENC_ERR_NEED_MORE_INPUT",
		"NV_ENC_ERR_ENCODER_BUSY",
		"NV_ENC_ERR_EVENT_NOT_REGISTERD",
		"NV_ENC_ERR_GENERIC",
		"NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY",
		"NV_ENC_ERR_UNIMPLEMENTED",
		"NV_ENC_ERR_RESOURCE_REGISTER_FAILED",
		"NV_ENC_ERR_RESOURCE_NOT_REGISTERED",
		"NV_ENC_ERR_RESOURCE_NOT_MAPPED",
	};
	return aszErrName[ e ];
}
