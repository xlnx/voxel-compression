#pragma once

#include "encoder.hpp"
#include <nvcodec/NvEncoderGL.h>
#include <GL/gl.h>

namespace vol
{
VM_BEGIN_MODULE( video )

struct GLEncoder : Encoder
{
	GLEncoder( uint32_t width, uint32_t height, PixelFormat format ) :
	  width( width ),
	  height( height )
	{
		_.reset( new NvEncoderGL( width, height, into_nv_format( format ) ) );
	}
	void encode( Reader &reader, std::vector<char> &block ) override
	{
		auto &_ = *this->_;

		int nFrameSize = _.GetFrameSize();
		std::unique_ptr<uint8_t[]> pHostFrame( new uint8_t[ nFrameSize ] );
		int nFrame = 0;

		while ( true ) {
			auto nRead = reader.read( reinterpret_cast<char *>( pHostFrame.get() ), nFrameSize );
			const NvEncInputFrame *encoderInputFrame = _.GetNextInputFrame();

			NV_ENC_INPUT_RESOURCE_OPENGL_TEX *pResource = (NV_ENC_INPUT_RESOURCE_OPENGL_TEX *)encoderInputFrame->inputPtr;
			glBindTexture( pResource->target, pResource->texture );
			glTexSubImage2D( pResource->target, 0, 0, 0,
							 width, height * 3 / 2,
							 GL_RED, GL_UNSIGNED_BYTE, pHostFrame.get() );
			glBindTexture( pResource->target, 0 );

			std::vector<std::vector<uint8_t>> vPacket;
			if ( nRead == nFrameSize ) {
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
	}

	uint32_t width, height;
};

VM_END_MODULE()

}  // namespace vol
