#include <algorithm>
#include <vocomp/refine/extractor.hpp>
#include "../utils/linked_reader.hpp"

namespace vol
{
VM_BEGIN_MODULE( refine )

using namespace std;

VM_EXPORT
{
	void Extractor::batch_extract( vector<Idx> const &blocks,
								   BlockConsumer &consumer )
	{
		if ( !blocks.size() ) return;
		auto swap_buffer = consumer.swap_buffer();
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
		  [&]( video::Buffer const &buffer ) {
			  while ( i < linked_block_offsets.size() ) {
				  int64_t inbuffer_offset = linked_block_offsets[ i ] + curr_block_offset - linked_read_pos;
				  // buffer contains current block
				  if ( inbuffer_offset >= 0 && inbuffer_offset < buffer.size() ) {
					  int64_t inbuffer_max_len = buffer.size() - inbuffer_offset;
					  auto len = std::min( inbuffer_max_len,
										   block_bytes - curr_block_offset );
					  consumer.consume(
						buffer.slice( inbuffer_offset, len ),
						sorted_blocks[ i ]->first,
						curr_block_offset );
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
			  linked_read_pos += buffer.size();
			  consumer.wait();
		  },
		  swap_buffer );
	}
}

VM_END_MODULE()

}  // namespace vol
