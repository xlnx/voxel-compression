#include <video/compressor.hpp>

#include <nv/NvEncoder.h>
#include <nv/NvEncoderCuda.h>
#include <cudafx/driver/context.hpp>

namespace vol
{
VM_BEGIN_MODULE( video )

using namespace std;

struct CompressorImpl final : vm::NoCopy, vm::NoMove
{
	CompressorImpl( CompressOptions const &_ ) :
	  pEnc( new NvEncoderCuda( ctx, 1024, 1024, NV_ENC_BUFFER_FORMAT_IYUV ) )
	{
		NV_ENC_CONFIG encode_cfg = { NV_ENC_CONFIG_VER };
		encode_cfg.profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;

		NV_ENC_INITIALIZE_PARAMS params = { NV_ENC_INITIALIZE_PARAMS_VER };

		decltype( NV_ENC_CODEC_H264_GUID ) *encode_guid;
		switch ( _.encode_method ) {
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
		switch ( _.encode_preset ) {
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
		pEnc->CreateDefaultEncoderParams( &params, *encode_guid, *preset_guid );

		pEnc->CreateEncoder( &params );
	}
	~CompressorImpl()
	{
		pEnc->DestroyEncoder();
	}

	void compress( Reader &reader, Writer &writer )
	{
		int nFrameSize = pEnc->GetFrameSize();
		std::unique_ptr<uint8_t[]> pHostFrame( new uint8_t[ nFrameSize ] );
		int nFrame = 0;

		while ( true ) {
			// Load the next frame from disk
			std::streamsize nRead = reader.read( reinterpret_cast<char *>( pHostFrame.get() ), nFrameSize );
			// For receiving encoded packets
			std::vector<std::vector<uint8_t>> vPacket;
			if ( nRead == nFrameSize ) {
				const NvEncInputFrame *encoderInputFrame = pEnc->GetNextInputFrame();
				NvEncoderCuda::CopyToDeviceFrame( ctx, pHostFrame.get(), 0, (CUdeviceptr)encoderInputFrame->inputPtr,
												  (int)encoderInputFrame->pitch,
												  pEnc->GetEncodeWidth(),
												  pEnc->GetEncodeHeight(),
												  CU_MEMORYTYPE_HOST,
												  encoderInputFrame->bufferFormat,
												  encoderInputFrame->chromaOffsets,
												  encoderInputFrame->numChromaPlanes );
				pEnc->EncodeFrame( vPacket );
			} else {
				pEnc->EndEncode( vPacket );
			}

			nFrame += (int)vPacket.size();

			for ( std::vector<uint8_t> &packet : vPacket ) {
				writer.write( reinterpret_cast<char *>( packet.data() ), packet.size() );
			}

			if ( nRead != nFrameSize ) break;
		}

		// vm::println( "Total frames encoded: {}", nFrame );
	}

private:
	cufx::drv::Context ctx = 0;
	vm::Box<NvEncoderCuda> pEnc;
};

VM_EXPORT
{
	Compressor::Compressor( CompressOptions const &_ ) :
	  _( new CompressorImpl( _ ) )
	{
	}
	Compressor::~Compressor()
	{
	}
	void Compressor::compress( Reader & reader, Writer & writer )
	{
		_->compress( reader, writer );
	}
}

VM_END_MODULE()

}  // namespace vol
