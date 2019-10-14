#pragma once

#include <map>

#include "index.hpp"

namespace vol
{
VM_BEGIN_MODULE( index )

using namespace std;

VM_EXPORT
{
	template <typename Voxel = char>
	struct Decompressor final : vm::NoCopy, vm::NoMove
	{
		Decompressor( Reader &reader ) :
		  reader( reader )
		{
			auto len = reader.size();
			reader.seek( len - sizeof( CompressMeta ) );
			auto meta = CompressMeta::read_from( reader );
			block_len = meta.block_len;
			reader.seek( meta.index_offset );
			vm::println( "total blocks: {}", meta.block_count );
			for ( int i = 0; i != meta.block_count; ++i ) {
				auto idx = Idx::read_from( reader );
				auto blk = BlockIndex::read_from( reader );
				index.insert( make_pair( idx, blk ) );
			}
		}

		PartReader get( Idx const &idx )
		{
			if ( index.find( idx ) == index.end() ) {
				throw std::runtime_error( vm::fmt( "invalid block id: {}", idx ) );
			}
			auto &blk = index[ idx ];
			return PartReader( reader, blk.offset, blk.len );
		}

	private:
		std::map<Idx, BlockIndex> index;
		Reader &reader;
		size_t block_len;
	};
}

VM_END_MODULE()

}  // namespace vol
