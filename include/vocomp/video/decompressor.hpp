#pragma once

#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include "method.hpp"
#include "../io.hpp"

namespace vol
{
VM_BEGIN_MODULE( video )

struct DecompressorImpl;

VM_EXPORT
{
	struct DecompressorOptions
	{
		VM_DEFINE_ATTRIBUTE( EncodeMethod, encode ) = EncodeMethod::H264;
	};

	struct Decompressor final : Pipe, vm::NoCopy
	{
		Decompressor( DecompressorOptions const &opts = DecompressorOptions{} );
		~Decompressor();

		void decompress( Reader &reader, Writer &writer );
		void transfer( Reader &reader, Writer &writer ) override
		{
			decompress( reader, writer );
		}

	private:
		vm::Box<DecompressorImpl> _;
	};
}

VM_END_MODULE()

}  // namespace vol
