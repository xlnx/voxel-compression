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
		for ( int i = 0; i < sorted_blocks.size(); ++i ) {
			auto &prev_block = sorted_blocks[ prev ]->second;
			auto &curr_block = sorted_blocks[ i ]->second;

			auto dframes = curr_block.first_frame - prev_block.first_frame;
			linked_block_offsets.emplace_back( ( frame_count + dframes ) * header.frame_size + curr_block.offset );

			if ( i == sorted_blocks.size() - 1 ||
				 sorted_blocks[ i + 1 ]->second.first_frame > curr_block.last_frame ) {
				auto beg = frame_offset[ prev_block.first_frame ];
				auto len = frame_offset[ curr_block.last_frame + 1 ] - beg;
				vm::println( "{} -> {} = {}", sorted_blocks[ i ]->first, make_pair( beg, len ), make_pair( prev_block.first_frame, curr_block.last_frame + 1 ) );
				readers.emplace_back( vm::Arc<Reader>( new PartReader( content, beg, len ) ) );
				frame_count += curr_block.last_frame - prev_block.first_frame + 1;
				prev = i + 1;
			}
		}

		auto linked_reader = LinkedReader( readers );
		linked_reader.seek( 0 );
		int i = 0;
		int64_t curr_block_offset = 0;
		int64_t linked_read_pos = 0;
		int64_t block_bytes = header.block_size * header.block_size * header.block_size;
		decomp.decompress(
		  linked_reader,
		  [&]( VideoStreamPacket const &packet ) {
			  while ( i < linked_block_offsets.size() ) {
				  //   vm::println( ">>>>>>{} {} {} {} {} {}<<<<<<", i, sorted_blocks[ i ]->first, linked_block_offsets.size(), linked_block_offsets[ i ], curr_block_offset, linked_read_pos );
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
