#pragma once

#include "encoder.hpp"

namespace vol
{
VM_BEGIN_MODULE( video )

struct D3D9Encoder : Encoder
{
	D3D9Encoder( uint32_t width, uint32_t height, PixelFormat format )
	{
	}
	~D3D9Encoder()
	{
	}
	void encode( Reader &reader, std::vector<char> &frames, std::vector<uint32_t> &frame_len ) override
	{
	}
};

VM_END_MODULE()

}  // namespace vol
