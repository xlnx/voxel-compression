#pragma once

#include <VMUtils/modules.hpp>
#include <VMUtils/concepts.hpp>
#include <vocomp/io.hpp>
#include <vocomp/video/compressor.hpp>
#include <nvcodec/NvEncoder.h>

namespace vol
{
VM_BEGIN_MODULE( video )

struct Encoder : vm::Dynamic, vm::NoCopy, vm::NoMove
{
	Encoder()
	{
		cfg.profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
		params.encodeConfig = &cfg;
	}

	virtual void encode( Reader &reader, std::vector<char> &block ) = 0;

	std::unique_ptr<NvEncoder> _;
	NV_ENC_CONFIG cfg = { NV_ENC_CONFIG_VER };
	NV_ENC_INITIALIZE_PARAMS params = { NV_ENC_INITIALIZE_PARAMS_VER };
};

inline decltype( NV_ENC_BUFFER_FORMAT_IYUV ) into_nv_format( PixelFormat format )
{
	switch ( format ) {
	default:
	case PixelFormat::IYUV: return NV_ENC_BUFFER_FORMAT_IYUV;
	case PixelFormat::YV12: return NV_ENC_BUFFER_FORMAT_YV12;
	case PixelFormat::NV12: return NV_ENC_BUFFER_FORMAT_NV12;
	case PixelFormat::YUV42010Bit: return NV_ENC_BUFFER_FORMAT_YUV420_10BIT;
	case PixelFormat::YUV444: return NV_ENC_BUFFER_FORMAT_YUV444;
	case PixelFormat::YUV44410Bit: return NV_ENC_BUFFER_FORMAT_YUV444_10BIT;
	case PixelFormat::ARGB: return NV_ENC_BUFFER_FORMAT_ARGB;
	case PixelFormat::ARGB10: return NV_ENC_BUFFER_FORMAT_ARGB10;
	case PixelFormat::AYUV: return NV_ENC_BUFFER_FORMAT_AYUV;
	case PixelFormat::ABGR: return NV_ENC_BUFFER_FORMAT_ABGR;
	case PixelFormat::ABGR10: return NV_ENC_BUFFER_FORMAT_ABGR10;
	}
}

inline decltype( NV_ENC_CODEC_H264_GUID ) *into_nv_encode( EncodeMethod method )
{
	switch ( method ) {
	default:
		vm::eprintln( "unknown encode method, default to EncodeMethod::H264" );
	case EncodeMethod::H264: return &NV_ENC_CODEC_H264_GUID;
	case EncodeMethod::HEVC: return &NV_ENC_CODEC_HEVC_GUID;
	}
}

inline decltype( NV_ENC_PRESET_DEFAULT_GUID ) *into_nv_preset( EncodePreset preset )
{
	switch ( preset ) {
	default:
		vm::eprintln( "unknown encode method, default to EncodePreset::Default" );
	case EncodePreset::Default: return &NV_ENC_PRESET_DEFAULT_GUID;
	case EncodePreset::HP: return &NV_ENC_PRESET_HP_GUID;
	case EncodePreset::HQ: return &NV_ENC_PRESET_HQ_GUID;
	case EncodePreset::BD: return &NV_ENC_PRESET_BD_GUID;
	case EncodePreset::LowLatencyDefault: return &NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
	case EncodePreset::LowLatencyHQ: return &NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
	case EncodePreset::LowLatencyHP: return &NV_ENC_PRESET_LOW_LATENCY_HP_GUID;
	case EncodePreset::LosslessDefault: return &NV_ENC_PRESET_LOSSLESS_DEFAULT_GUID;
	case EncodePreset::LosslessHP: return &NV_ENC_PRESET_LOSSLESS_HP_GUID;
	}
}

VM_END_MODULE()

}  // namespace vol
