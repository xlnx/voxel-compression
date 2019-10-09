#pragma once

#include "encoder.hpp"

namespace vol
{
VM_BEGIN_MODULE( video )

struct CudaEncoderImpl;

struct CudaEncoder : Encoder
{
	CudaEncoder( uint32_t width, uint32_t height, PixelFormat format );
	~CudaEncoder();
	void encode( Reader &reader, Writer &writer ) override;

protected:
	void *get_nv_impl() override;

private:
	vm::Box<CudaEncoderImpl> _;
};

VM_END_MODULE()

}  // namespace vol
