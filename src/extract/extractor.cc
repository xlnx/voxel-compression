#include <algorithm>
#include <vocomp/extractor.hpp>
#include <vocomp/utils/linked_reader.hpp>

VM_BEGIN_MODULE( vol )

using namespace std;

VM_EXPORT
{
	void Extractor::batch_extract( vector<Idx> const &blocks,
								   std::function<void( Idx const &idx, VoxelStreamPacket const & )> const &consumer )
	{
		if ( !blocks.size() ) return;
		vector<map<Idx, BlockIndex>::const_iterator> sorted_blocks( blocks.size() );
		std::transform( blocks.begin(), blocks.end(), sorted_blocks.begin(),
						[this]( Idx const &idx ) { return block_idx.find( idx ); } );
		std::sort( sorted_blocks.begin(), sorted_blocks.end(),
				   [this]( auto const &x, auto const &y ) { return x->second < y->second; } );

		vector<vm::Arc<Reader>> readers;
		vector<int64_t> linked_block_offsets;
		int frame_count = 0, prev = 0;
		for ( int i = 0; i < sorted_blocks.size() - 1; ++i ) {
			if ( sorted_blocks[ i + 1 ]->second.first_frame >
				 sorted_blocks[ i ]->second.last_frame ) {
				auto beg = frame_offset[ sorted_blocks[ prev ]->second.first_frame ];
				auto end = frame_offset[ sorted_blocks[ i ]->second.last_frame + 1 ];
				readers.emplace_back( vm::Arc<Reader>( new PartReader( content, beg, end ) ) );
				frame_count += sorted_blocks[ i ]->second.last_frame - sorted_blocks[ prev ]->second.first_frame + 1;
				prev = i + 1;
			}
			auto inframe_offset = sorted_blocks[ i ]->second.offset;
			auto dframes = sorted_blocks[ i ]->second.first_frame - sorted_blocks[ prev ]->second.first_frame;
			linked_block_offsets.emplace_back( ( frame_count + dframes ) * header.frame_size + inframe_offset );
		}
		auto beg = frame_offset[ sorted_blocks[ prev ]->second.first_frame ];
		auto end = frame_offset[ sorted_blocks.back()->second.last_frame + 1 ];
		readers.emplace_back( vm::Arc<Reader>( new PartReader( content, beg, end ) ) );
		auto inframe_offset = sorted_blocks.back()->second.offset;
		auto dframes = sorted_blocks.back()->second.first_frame - sorted_blocks[ prev ]->second.first_frame;
		linked_block_offsets.emplace_back( ( frame_count + dframes ) * header.frame_size + inframe_offset );

		auto linked_reader = LinkedReader( readers );
		int i = 0;
		int64_t curr_block_offset = 0;
		int64_t linked_read_pos = 0;
		int64_t block_bytes = header.block_size * header.block_size * header.block_size;
		decomp.decompress(
		  linked_reader,
		  [&]( VideoStreamPacket const &packet ) {
			  while ( i < linked_block_offsets.size() ) {
				  int64_t inpacket_offset = linked_block_offsets[ i ] + curr_block_offset - linked_read_pos;
				  // buffer contains current block
				  if ( inpacket_offset >= 0 && inpacket_offset < packet.length ) {
					  int64_t inpacket_max_len = packet.length - inpacket_offset;
					  auto len = std::min( inpacket_max_len,
										   block_bytes - curr_block_offset );
					  VoxelStreamPacket blk_packet( packet, inpacket_offset );
					  blk_packet.offset = curr_block_offset;
					  blk_packet.length = len;
					  consumer( sorted_blocks[ i ]->first, blk_packet );

					  curr_block_offset += len;
					  if ( curr_block_offset >= block_bytes ) {
						  curr_block_offset = 0;
						  i++;
					  } else {
						  break;
					  }
				  } else {
					  break;
				  }
			  }
			  linked_read_pos += packet.length;
		  } );
	}
}

VM_END_MODULE()
