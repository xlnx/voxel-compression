#pragma once

#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include <cudafx/stream.hpp>
#include <cudafx/misc.hpp>
#include <cudafx/memory.hpp>
#include <cudafx/array.hpp>
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
		VM_DEFINE_ATTRIBUTE( EncodeMethod, encode );
		VM_DEFINE_ATTRIBUTE( unsigned, io_queue_size ) = 4;
	};

	using Buffer = cufx::MemoryView1D<unsigned char>;

	struct Decompressor final : vm::NoCopy
	{
		Decompressor( DecompressorOptions const &opts = DecompressorOptions{} );
		~Decompressor();

		void decompress( Reader &reader,
						 std::function<void( Buffer const & )> const &consumer,
						 Buffer const &swap_buffer );

	private:
		vm::Box<DecompressorImpl> _;
	};
}

VM_END_MODULE()

}  // namespace vol
