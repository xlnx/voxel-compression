#pragma once

#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include "devices/encoder.hpp"

namespace vol
{
VM_BEGIN_MODULE( video )

struct CompressorImpl;

VM_EXPORT
{
	enum class CompressDevice
	{
		CUDA,	 /* cuda sdk required */
		Graphics /* D3D9 for windows and GL for linux */
	};

	struct CompressOptions
	{
		CompressOptions();
		
		VM_DEFINE_ATTRIBUTE( CompressDevice, device );
		VM_DEFINE_ATTRIBUTE( EncodeMethod, encode_method ) = EncodeMethod::H264;
		VM_DEFINE_ATTRIBUTE( EncodePreset, encode_preset ) = EncodePreset::Default;
		VM_DEFINE_ATTRIBUTE( unsigned, width ) = 1024;
		VM_DEFINE_ATTRIBUTE( unsigned, height ) = 1024;
		VM_DEFINE_ATTRIBUTE( PixelFormat, pixel_format ) = PixelFormat::IYUV;
	};

	struct Compressor final : Pipe, vm::NoCopy
	{
		Compressor( CompressOptions const &_ = CompressOptions{} );

		void transfer( Reader &reader, Writer &writer ) override
		{
			_->encode( reader, writer );
		}

	private:
		vm::Box<Encoder> _;
	};
}

VM_END_MODULE()

}  // namespace vol
