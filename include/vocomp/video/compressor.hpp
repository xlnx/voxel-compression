#pragma once

#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include "method.hpp"
#include "../io.hpp"

namespace vol
{
VM_BEGIN_MODULE( video )

struct CompressorImpl;

VM_EXPORT
{
	enum class EncodePreset : char
	{
		Default,
		HP,
		HQ,
		BD,
		LowLatencyDefault,
		LowLatencyHQ,
		LowLatencyHP,
		LosslessDefault,
		LosslessHP
	};
	enum class PixelFormat : char
	{
		IYUV,
		YV12,
		NV12,
		YUV42010Bit,
		YUV444,
		YUV44410Bit,
		ARGB,
		ARGB10,
		AYUV,
		ABGR,
		ABGR10
	};
	enum class CompressDevice
	{
		Cuda,	/* cuda sdk required */
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
		~Compressor();

		void transfer( Reader &reader, Writer &writer ) override;

	private:
		vm::Box<CompressorImpl> _;
	};
}

VM_END_MODULE()

}  // namespace vol
