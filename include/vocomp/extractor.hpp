#pragma once

#include <VMUtils/nonnull.hpp>
#include <cudafx/memory.hpp>
#include <vocomp/utils/common.hpp>
#include <vocomp/video_decompressor.hpp>

VM_BEGIN_MODULE( vol )

VM_EXPORT
{
	struct BlockConsumer : vm::NoCopy
	{
		virtual void consume( cufx::MemoryView1D<unsigned char> const &data,
							  Idx const &idx,
							  std::size_t offset ) = 0;
		virtual void wait() {}
		virtual cufx::MemoryView1D<unsigned char> swap_buffer() const = 0;
	};

	struct ExtractorOptions
	{
		VM_DEFINE_ATTRIBUTE( unsigned, io_queue_size ) = 4;
	};

	struct Extractor final : vm::NoCopy
	{
		Extractor( Reader &reader, ExtractorOptions const &opts = {} ) :
		  content( reader, sizeof( Header ), reader.size() - sizeof( Header ) ),
		  decomp( [this, &reader, &opts] {
			  reader.read_typed( header );
			  return VideoDecompressOptions{}
				.set_encode( header.encode_method )
				.set_io_queue_size( opts.io_queue_size );
		  }() )
		{
			vm::println( "header: {}", header );
			uint64_t meta_offset;
			content.seek( content.size() - sizeof( meta_offset ) );
			content.read_typed( meta_offset );
			content.seek( meta_offset );
			content.read_typed( frame_offset );
			content.read_typed( block_idx );
			content.seek( 0 );
		}
		// block_idx ->
		void batch_extract( std::vector<Idx> const &blocks,
							BlockConsumer &consumer );

		auto raw() const { return header.raw; }
		auto dim() const { return header.dim; }
		auto adjusted() const { return header.adjusted; }
		auto log_block_size() const { return header.log_block_size; }
		auto block_size() const { return header.block_size; }
		auto block_inner() const { return header.block_inner; }
		auto padding() const { return header.padding; }
		auto frame_size() const { return header.frame_size; }

	private:
		Header header;
		PartReader content;
		vector<uint64_t> frame_offset;
		map<Idx, BlockIndex> block_idx;
		VideoDecompressor decomp;
	};
}

VM_END_MODULE()
