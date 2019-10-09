#pragma once

#include "encoder.hpp"

namespace vol
{
VM_BEGIN_MODULE( video )

struct GLEncoderImpl;

struct GLEncoder : Encoder
{
	GLEncoder( uint32_t width, uint32_t height, PixelFormat format );
	~GLEncoder();
	void encode( Reader &reader, Writer &writer ) override;

protected:
	void *get_nv_impl() override;

private:
	vm::Box<GLEncoderImpl> _;
};

VM_END_MODULE()

}  // namespace vol
