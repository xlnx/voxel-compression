#pragma once

#include <VMUtils/nonnull.hpp>
#include <cudafx/memory.hpp>
#include <varch/utils/common.hpp>
#include <varch/video_decompressor.hpp>

VM_BEGIN_MODULE( vol )

VM_EXPORT
{
	// struct BlockConsumer : vm::NoCopy
	// {
	// 	virtual void consume( cufx::MemoryView1D<unsigned char> const &data,
	// 						  Idx const &idx,
	// 						  std::size_t offset ) = 0;
	// 	virtual void wait() {}
	// 	virtual cufx::MemoryView1D<unsigned char> swap_buffer() const = 0;
	// };

	struct VoxelStreamPacket : vm::NoMove, vm::NoCopy
	{
		VoxelStreamPacket( VideoStreamPacket const &_, unsigned inner_offset ) :
		  _( _ ),
		  inner_offset( inner_offset ) {}

	public:
		void append_to( cufx::MemoryView1D<unsigned char> const &buffer ) const
		{
			if ( buffer.size() < offset + length ) {
				throw std::logic_error(
				  vm::fmt( "insufficient buffer size: {} < {}", buffer.size(), offset + length ) );
			}
			_.copy_async( buffer.slice( offset, length ), inner_offset, length );
		}

	public:
		unsigned offset, length;

	private:
		VideoStreamPacket const &_;
		unsigned inner_offset;
	};

	struct UnarchiverOptions
	{
		VM_DEFINE_ATTRIBUTE( unsigned, io_queue_size ) = 4;
	};

	struct Unarchiver final : vm::NoCopy
	{
		Unarchiver( Reader &reader, UnarchiverOptions const &opts = {} ) :
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
		void batch_unarchive( std::vector<Idx> const &blocks,
							std::function<void( Idx const &idx, VoxelStreamPacket const & )> const &consumer );

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
