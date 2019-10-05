#pragma once

#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include "io.hpp"

namespace vol
{
VM_BEGIN_MODULE( video )

struct DecompressorImpl;

VM_EXPORT
{
	struct Decompressor final : vm::NoCopy
	{
		Decompressor();
		~Decompressor();

		void decompress( Reader &reader, Writer &writer );

	private:
		vm::Box<DecompressorImpl> _;
	};
}

VM_END_MODULE()

}  // namespace vol
