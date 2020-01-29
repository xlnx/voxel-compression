#pragma once

#include <VMUtils/nonnull.hpp>
#include <cudafx/memory.hpp>
#include <varch/utils/common.hpp>

VM_BEGIN_MODULE( vol )

struct UnarchiverImpl;

struct UnarchiverData
{
	UnarchiverData( Reader &reader ) :
	  content( reader, sizeof( Header ), reader.size() - sizeof( Header ) )
	{
		reader.seek( 0 );
		reader.read_typed( header );
		vm::println( "header: {}", header );

		uint64_t meta_offset;
		content.seek( content.size() - sizeof( meta_offset ) );
		content.read_typed( meta_offset );
		content.seek( meta_offset );
		content.read_typed( frame_offset );
		content.read_typed( block_idx );
		content.seek( 0 );
	}

public:
	Header header;
	PartReader content;
	vector<uint64_t> frame_offset;
	map<Idx, BlockIndex> block_idx;
};

VM_EXPORT
{
	struct VoxelStreamPacket : vm::NoMove, vm::NoCopy
	{
		VoxelStreamPacket( Packet const &packet, unsigned inner_offset ) :
		  packet( packet ),
		  inner_offset( inner_offset ) {}

	public:
		void append_to( cufx::MemoryView1D<unsigned char> const &buffer ) const
		{
			if ( buffer.size() < offset + length ) {
				throw std::logic_error(
				  vm::fmt( "insufficient buffer size: {} < {}", buffer.size(), offset + length ) );
			}
			packet.copy_to( buffer.slice( offset, length ), inner_offset, length );
		}

	public:
		unsigned offset, length;

	private:
		Packet const &packet;
		unsigned inner_offset;
	};

	struct Unarchiver final : vm::NoCopy, vm::NoMove
	{
		Unarchiver( Reader &reader, DecodeOptions const &opts = {} );
		~Unarchiver();

	public:
		std::size_t unarchive_to( Idx const &idx,
								  cufx::MemoryView1D<unsigned char> const &dst );
		// // block_idx ->
		// void batch_unarchive( std::vector<Idx> const &blocks,
		// 					  std::function<void( Idx const &idx, VoxelStreamPacket const & )> const &consumer );
		// std::size_t unarchive_to( Idx const &block,
		// 							cufx::MemoryView1D<unsigned char> const &buffer )
		// {
		// 	std::size_t nbytes;
		// 	batch_unarchive( { block },
		// 					 [&]( Idx const &, VoxelStreamPacket const &pkt ) {
		// 						 pkt.append_to( buffer );
		// 						 nbytes += pkt.length;
		// 					 } );
		// 	return nbytes;
		// }

	public:
		auto raw() const { return data.header.raw; }
		auto dim() const { return data.header.dim; }
		auto adjusted() const { return data.header.adjusted; }
		auto log_block_size() const { return data.header.log_block_size; }
		auto block_size() const { return data.header.block_size; }
		auto block_inner() const { return data.header.block_inner; }
		auto padding() const { return data.header.padding; }
		auto frame_size() const { return data.header.frame_size; }

	private:
		UnarchiverData data;
		vm::Box<UnarchiverImpl> _;
	};
}

VM_END_MODULE()
