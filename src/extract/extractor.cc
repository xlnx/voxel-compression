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
								   shared_ptr<BlockConsumer> const &consumer )
	{
		if ( !blocks.size() ) return;
		auto swap_buffer = consumer->swap_buffer();
		vector<map<Idx, BlockIndex>::const_iterator> sorted_blocks;
		std::transform( blocks.begin(), blocks.end(), sorted_blocks.begin(),
						[this]( Idx const &idx ) { return block_idx.find( idx ); } );
		std::sort( sorted_blocks.begin(), sorted_blocks.end(),
				   [this]( auto const &x, auto const &y ) { return x->second < y->second; } );
		vector<vm::Arc<Reader>> readers;
		int prev = 0;
		for ( int i = 1; i < sorted_blocks.size(); ++i ) {
			if ( sorted_blocks[ i ]->second.first_frame >
				 sorted_blocks[ i - 1 ]->second.last_frame ) {
				auto beg = frame_offset[ sorted_blocks[ prev ]->second.first_frame ];
				auto end = frame_offset[ sorted_blocks[ i - 1 ]->second.last_frame + 1 ];
				readers.emplace_back( vm::Arc<Reader>( new PartReader( content, beg, end ) ) );
			}
		}
		auto beg = frame_offset[ sorted_blocks[ prev ]->second.first_frame ];
		auto end = frame_offset[ sorted_blocks.back()->second.last_frame + 1 ];
		readers.emplace_back( vm::Arc<Reader>( new PartReader( content, beg, end ) ) );
		auto linked_reader = LinkedReader( readers );
		// decomp.decompress( linked_reader, buffer_consumer, swap_buffer );
	}
}

VM_END_MODULE()

}  // namespace vol
