#include <vocomp/video/devices/encoder.hpp>

#include <nvcodec/NvEncoder.h>

namespace vol
{
VM_BEGIN_MODULE( video )

void Encoder::init( EncodeMethod method, EncodePreset preset )
{
	NV_ENC_CONFIG encode_cfg = { NV_ENC_CONFIG_VER };
	encode_cfg.profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;

	NV_ENC_INITIALIZE_PARAMS params = { NV_ENC_INITIALIZE_PARAMS_VER };

	decltype( NV_ENC_CODEC_H264_GUID ) *encode_guid;
	switch ( method ) {
	default:
		vm::eprintln( "unknown encode method, default to EncodeMethod::H264" );
	case EncodeMethod::H264:
		encode_guid = &NV_ENC_CODEC_H264_GUID;
		break;
	case EncodeMethod::HEVC:
		encode_guid = &NV_ENC_CODEC_HEVC_GUID;
		break;
	}

	decltype( NV_ENC_PRESET_DEFAULT_GUID ) *preset_guid;
	switch ( preset ) {
	default:
		vm::eprintln( "unknown encode method, default to EncodePreset::Default" );
	case EncodePreset::Default:
		preset_guid = &NV_ENC_PRESET_DEFAULT_GUID;
		break;
	case EncodePreset::HP:
		preset_guid = &NV_ENC_PRESET_HP_GUID;
		break;
	case EncodePreset::HQ:
		preset_guid = &NV_ENC_PRESET_HQ_GUID;
		break;
	case EncodePreset::BD:
		preset_guid = &NV_ENC_PRESET_BD_GUID;
		break;
	case EncodePreset::LowLatencyDefault:
		preset_guid = &NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
		break;
	case EncodePreset::LowLatencyHQ:
		preset_guid = &NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
		break;
	case EncodePreset::LowLatencyHP:
		preset_guid = &NV_ENC_PRESET_LOW_LATENCY_HP_GUID;
		break;
	case EncodePreset::LosslessDefault:
		preset_guid = &NV_ENC_PRESET_LOSSLESS_DEFAULT_GUID;
		break;
	case EncodePreset::LosslessHP:
		preset_guid = &NV_ENC_PRESET_LOSSLESS_HP_GUID;
		break;
	}

	params.encodeConfig = &encode_cfg;

	auto impl = reinterpret_cast<NvEncoder *>( get_nv_impl() );
	impl->CreateDefaultEncoderParams( &params, *encode_guid, *preset_guid );
	impl->CreateEncoder( &params );
	init_done = true;
}

Encoder::~Encoder()
{
	auto impl = reinterpret_cast<NvEncoder *>( get_nv_impl() );
	if ( init_done ) {
		impl->DestroyEncoder();
	}
}

void *Encoder::get_nv_impl()
{
	return nullptr;
}

void Encoder::get_pixel_format( void *dst, PixelFormat format )
{
	auto &pixel_format = *reinterpret_cast<decltype( NV_ENC_BUFFER_FORMAT_IYUV ) *>( dst );
	pixel_format = [&] {
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
	}();
}

VM_END_MODULE()

}  // namespace vol
