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
		Decompressor( Reader &reader, Pipe &_ ) :
		  reader( reader ),
		  _( _ )
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

		template <typename W>  // W: Writer
		void get( Idx const &idx, W &writer )
		{
			if ( index.find( idx ) == index.end() ) {
				return;
			}
			auto &blk = index[ idx ];
			PartReader part( reader, blk.offset, blk.len );
			_.transfer( part, writer );
		}
		void get( Idx const &idx, ostream &os, size_t offset = 0 )
		{
			StreamWriter writer( os, offset, block_len );
			get( idx, writer );
		}
		void get( Idx const &idx, Voxel *block )
		{
			auto ptr = reinterpret_cast<car *>( block );
			SliceWriter writer( ptr, 0, block_len );
			put( idx, reader );
		}

	private:
		std::map<Idx, BlockIndex> index;
		Reader &reader;
		size_t block_len;
		Pipe &_;
	};
}

VM_END_MODULE()

}  // namespace vol
