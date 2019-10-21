#pragma once

#include "encoder.hpp"
#include <cudafx/driver/context.hpp>
#include <nvcodec/NvEncoderCuda.h>

namespace vol
{
VM_BEGIN_MODULE( video )

struct CudaEncoder : Encoder
{
	CudaEncoder( uint32_t width, uint32_t height, PixelFormat format )
	{
		_.reset( new NvEncoderCuda( ctx, width, height, into_nv_format( format ) ) );
	}
	void encode( Reader &reader, std::vector<char> &block ) override
	{
		auto &_ = *this->_;

		int nFrameSize = _.GetFrameSize();
		std::unique_ptr<uint8_t[]> pHostFrame( new uint8_t[ nFrameSize ] );
		int nFrame = 0;

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
				_.EncodeFrame( vPacket );
			} else {
				_.EndEncode( vPacket );
			}

			nFrame += (int)vPacket.size();

			for ( auto &packet : vPacket ) {
				auto packet_begin = reinterpret_cast<char *>( packet.data() );
				block.insert( block.end(), packet_begin, packet_begin + packet.size() );
			}

			if ( nRead != nFrameSize ) break;
		}
		// vm::println( "Total frames encoded: {}", nFrame );
	}

	cufx::drv::Context ctx = 0;
};

VM_END_MODULE()

}  // namespace vol
