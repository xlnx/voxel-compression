#pragma once

#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include "io.hpp"

namespace vol
{
VM_BEGIN_MODULE( video )

struct CompressorImpl;

VM_EXPORT
{
	enum class EncodeMethod : char
	{
		H264,
		HEVC
	};
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

	struct CompressOptions
	{
		VM_DEFINE_ATTRIBUTE( EncodeMethod, encode_method ) = EncodeMethod::H264;
		VM_DEFINE_ATTRIBUTE( EncodePreset, encode_preset ) = EncodePreset::Default;
	};

	struct Compressor final : vm::NoCopy
	{
		Compressor( CompressOptions const & = CompressOptions{} );
		~Compressor();

		void compress( Reader &reader, Writer &writer );

	private:
		vm::Box<CompressorImpl> _;
	};
}

VM_END_MODULE()

}  // namespace vol
