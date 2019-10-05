#pragma once

#include <map>

#include <video/decompressor.hpp>
#include <voxel/internal/util.hpp>

namespace vol
{
VM_BEGIN_MODULE( voxel )

using namespace std;

VM_EXPORT
{
	template <typename Voxel = char>
	struct Decompressor final : vm::NoCopy, vm::NoMove
	{
		Decompressor( Reader &reader )
		{
			auto len = reader.size();
			reader.seek( len - sizeof( CompressMeta ) );
			auto meta = CompressMeta::read_from( reader );
			reader.seek( meta.index_offset );
			vm::println( "total blocks: {}", meta.block_count );
			for ( int i = 0; i != meta.block_count; ++i ) {
				auto idx = Idx::read_from( reader );
				auto blk = BlockIndex::read_from( reader );
				index.insert( make_pair( idx, blk ) );
			}
		}

	private:
		std::map<Idx, BlockIndex> index;
	};
}

VM_END_MODULE()

}  // namespace vol
