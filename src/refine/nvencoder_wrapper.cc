#include "nvencoder_wrapper.hpp"

VM_BEGIN_MODULE( vol )

inline NV_ENC_BUFFER_FORMAT into_nv_format( PixelFormat format )
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

inline GUID const *into_nv_encode( EncodeMethod method )
{
	switch ( method ) {
	default:
		vm::eprintln( "unknown encode method, default to EncodeMethod::H264" );
	case EncodeMethod::H264: return &NV_ENC_CODEC_H264_GUID;
	case EncodeMethod::HEVC: return &NV_ENC_CODEC_HEVC_GUID;
	}
}

inline GUID const *into_nv_preset( EncodePreset preset )
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

std::unique_ptr<NvEncoder> NvEncoderWrapper::_;
cufx::drv::Context NvEncoderWrapper::ctx = 0;

NvEncoderWrapper::NvEncoderWrapper( VideoCompressOptions const &opts )
{
	static NV_ENC_INITIALIZE_PARAMS params = { NV_ENC_INITIALIZE_PARAMS_VER };
	static NV_ENC_CONFIG cfg = { NV_ENC_CONFIG_VER };

	if ( _ == nullptr ) {
		cfg.profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
		params.encodeConfig = &cfg;

		_.reset( new NvEncoderCuda( ctx,
									opts.width,
									opts.height,
									into_nv_format( opts.pixel_format ) ) );
		_->CreateDefaultEncoderParams( &params,
									   *into_nv_encode( opts.encode_method ),
									   *into_nv_preset( opts.encode_preset ) );
		_->CreateEncoder( &params );
		_->Allocate();
	} else {
		_->m_eBufferFormat = into_nv_format( opts.pixel_format );
		_->m_nWidth = opts.width;
		_->m_nHeight = opts.height;
		_->CreateDefaultEncoderParams( &params,
									   *into_nv_encode( opts.encode_method ),
									   *into_nv_preset( opts.encode_preset ) );

		NV_ENC_RECONFIGURE_PARAMS reconfigure_params;

		reconfigure_params.version = NV_ENC_RECONFIGURE_PARAMS_VER;
		reconfigure_params.resetEncoder = 1;
		reconfigure_params.reInitEncodeParams = params;

		_->Reconfigure( &reconfigure_params );
		_->Allocate();
	}
}

NvEncoderWrapper::~NvEncoderWrapper()
{
	_->Deallocate();
}

void NvEncoderWrapper::encode( Reader &reader, Writer &out, std::vector<uint32_t> &frame_len )
{
	auto &_ = *this->_;

	int nFrameSize = _.GetFrameSize();
	std::unique_ptr<uint8_t[]> pHostFrame( new uint8_t[ nFrameSize ] );
	int nFrame = 0;

	thread_local auto params = [] {
		NV_ENC_PIC_PARAMS params;
		params.encodePicFlags = NV_ENC_PIC_FLAG_OUTPUT_SPSPPS |
								NV_ENC_PIC_FLAG_FORCEIDR;
		return params;
	}();

	while ( true ) {
		// For receiving encoded packets
		std::vector<std::vector<uint8_t>> vPacket;
		// Load the next frame from disk
		auto nRead = reader.read( reinterpret_cast<char *>( pHostFrame.get() ), nFrameSize );
		if ( nRead == nFrameSize ) {
			const NvEncInputFrame *encoderInputFrame = _.GetNextInputFrame();
			NvEncoderCuda::CopyToDeviceFrame( ctx, pHostFrame.get(), 0, (CUdeviceptr)encoderInputFrame->inputPtr,
											  (int)encoderInputFrame->pitch,
											  _.GetEncodeWidth(),
											  _.GetEncodeHeight(),
											  CU_MEMORYTYPE_HOST,
											  encoderInputFrame->bufferFormat,
											  encoderInputFrame->chromaOffsets,
											  encoderInputFrame->numChromaPlanes );
			_.EncodeFrame( vPacket, nFrame ? nullptr : &params );
		} else {
			_.EndEncode( vPacket );
		}

		nFrame += (int)vPacket.size();

		for ( auto &packet : vPacket ) {
			auto packet_begin = reinterpret_cast<char *>( packet.data() );
			uint32_t len = packet.size();
			out.write( reinterpret_cast<char *>( &len ), sizeof( len ) );
			out.write( packet_begin, packet.size() );
			frame_len.emplace_back( sizeof( len ) + packet.size() );
		}

		if ( nRead != nFrameSize ) break;
	}
}

VM_END_MODULE()
