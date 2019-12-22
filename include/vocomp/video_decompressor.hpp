#pragma once

#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include <cudafx/stream.hpp>
#include <cudafx/misc.hpp>
#include <cudafx/memory.hpp>
#include <cudafx/array.hpp>
#include <vocomp/utils/common.hpp>
#include <vocomp/utils/io.hpp>

VM_BEGIN_MODULE( vol )

struct VideoDecompressorImpl;

struct VideoDecompressOptions
{
	VM_DEFINE_ATTRIBUTE( EncodeMethod, encode );
	VM_DEFINE_ATTRIBUTE( unsigned, io_queue_size ) = 4;
};

using Buffer = cufx::MemoryView1D<unsigned char>;

struct VideoDecompressor final : vm::NoCopy
{
	VideoDecompressor( VideoDecompressOptions const &opts = VideoDecompressOptions{} );
	~VideoDecompressor();

	void decompress( Reader &reader,
					 std::function<void( Buffer const & )> const &consumer,
					 Buffer const &swap_buffer );

private:
	vm::Box<VideoDecompressorImpl> _;
};

VM_END_MODULE()
