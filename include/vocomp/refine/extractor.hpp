#pragma once

#include <vocomp/index.hpp>
#include <vocomp/video/decompressor.hpp>
#include <VMUtils/nonnull.hpp>
#include <cudafx/memory.hpp>
#include "header.hpp"

namespace vol
{
VM_BEGIN_MODULE( refine )

VM_EXPORT
{
	struct BlockConsumer : vm::NoCopy
	{
		virtual void consume( cufx::MemoryView1D<unsigned char> const &data,
							  Idx const &idx,
							  std::size_t offset ) = 0;
		virtual void wait() {}
		virtual cufx::MemoryView1D<unsigned char> swap_buffer() const;
	};

	struct Extractor final : vm::NoCopy
	{
		Extractor( Reader &reader, video::DecompressorOptions const &opts ) :
		  content( reader, sizeof( Header ), reader.size() - sizeof( Header ) ),
		  decomp( opts )
		{
			reader.read_typed( header );
			uint64_t meta_offset;
			reader.seek( reader.size() - sizeof( meta_offset ) );
			reader.read_typed( meta_offset );
			reader.seek( meta_offset );
			reader.read_typed( frame_offset );
			reader.read_typed( block_idx );
			content.seek( 0 );
		}
		// block_idx ->
		void batch_extract( std::vector<Idx> const &blocks,
							std::shared_ptr<BlockConsumer> const &consumer );

		auto raw() const { return header.raw; }
		auto dim() const { return header.dim; }
		auto adjusted() const { return header.adjusted; }
		auto log_block_size() const { return header.log_block_size; }
		auto block_size() const { return header.block_size; }
		auto block_inner() const { return header.block_inner; }
		auto padding() const { return header.padding; }

	private:
		Header header;
		PartReader content;
		vector<uint64_t> frame_offset;
		map<Idx, BlockIndex> block_idx;
		video::Decompressor decomp;
	};
}

VM_END_MODULE()

}  // namespace vol
