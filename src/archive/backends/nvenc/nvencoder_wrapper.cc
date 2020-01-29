#include "nvencoder_wrapper.hpp"

VM_BEGIN_MODULE( vol )

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

NvEncoderWrapper::NvEncoderWrapper( EncodeOptions const &opts )
{
	static NV_ENC_INITIALIZE_PARAMS params = { NV_ENC_INITIALIZE_PARAMS_VER };
	static NV_ENC_CONFIG cfg = { NV_ENC_CONFIG_VER };

	if ( _ == nullptr ) {
		cfg.profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
		params.encodeConfig = &cfg;

		_.reset( new NvEncoderCuda( ctx,
									opts.width,
									opts.height,
									NV_ENC_BUFFER_FORMAT_NV12 ) );
		_->CreateDefaultEncoderParams( &params,
									   NV_ENC_CODEC_H264_GUID,
									   *into_nv_preset( opts.encode_preset ) );
		_->CreateEncoder( &params );
		_->Allocate();
	} else {
		_->m_eBufferFormat = NV_ENC_BUFFER_FORMAT_NV12;
		_->m_nWidth = opts.width;
		_->m_nHeight = opts.height;
		_->CreateDefaultEncoderParams( &params,
									   NV_ENC_CODEC_H264_GUID,
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

	static auto params = [] {
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
			// vm::println( "#enc_src: { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} ...",
			// 			 int( pHostFrame[ 0 ] ), int( pHostFrame[ 1 ] ), int( pHostFrame[ 2 ] ),
			// 			 int( pHostFrame[ 3 ] ), int( pHostFrame[ 4 ] ), int( pHostFrame[ 5 ] ), int( pHostFrame[ 6 ] ),
			// 			 int( pHostFrame[ 7 ] ), int( pHostFrame[ 8 ] ), int( pHostFrame[ 9 ] ) );
			const NvEncInputFrame *encoderInputFrame = _.GetNextInputFrame();
			NvEncoderCuda::CopyToDeviceFrame( ctx, pHostFrame.get(), 0, (CUdeviceptr)encoderInputFrame->inputPtr,
											  (int)encoderInputFrame->pitch,
											  _.GetEncodeWidth(),
											  _.GetEncodeHeight(),
											  CU_MEMORYTYPE_HOST,
											  encoderInputFrame->bufferFormat,
											  encoderInputFrame->chromaOffsets,
											  encoderInputFrame->numChromaPlanes );
			_.EncodeFrame( vPacket, &params );
		} else {
			_.EndEncode( vPacket );
		}

		nFrame += (int)vPacket.size();

		for ( auto &packet : vPacket ) {
			auto packet_begin = reinterpret_cast<char *>( packet.data() );
			uint32_t len = packet.size();
			out.write( reinterpret_cast<char *>( &len ), sizeof( len ) );
			out.write( packet_begin, packet.size() );
			// vm::println( "#enc_dst {} -> len {}: { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} ...",
			// 			 frame_len.size(), packet.size(), int( packet[ 48 + 0 ] ), int( packet[ 48 + 1 ] ), int( packet[ 48 + 2 ] ),
			// 			 int( packet[ 48 + 3 ] ), int( packet[ 48 + 4 ] ), int( packet[ 48 + 5 ] ), int( packet[ 48 + 6 ] ),
			// 			 int( packet[ 48 + 7 ] ), int( packet[ 48 + 8 ] ), int( packet[ 48 + 9 ] ) );
			frame_len.emplace_back( sizeof( len ) + packet.size() );
		}

		if ( nRead != nFrameSize ) break;
	}
}

std::size_t NvEncoderWrapper::frame_size() const
{
	return _->GetFrameSize();
}

VM_END_MODULE()
