#pragma once

#include "encoder.hpp"

namespace vol
{
VM_BEGIN_MODULE( video )

struct D3D9EncoderImpl;

struct D3D9Encoder : Encoder
{
	D3D9Encoder( uint32_t width, uint32_t height, PixelFormat format );
	~D3D9Encoder();
	void encode( Reader &reader, Writer &writer ) override;

protected:
	void *get_nv_impl() override;

private:
	vm::Box<D3D9EncoderImpl> _;
};

VM_END_MODULE()

}  // namespace vol
