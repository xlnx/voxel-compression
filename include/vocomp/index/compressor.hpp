#pragma once

#include <map>

#include "../unbounded_io.hpp"
#include "index.hpp"

namespace vol
{
VM_BEGIN_MODULE( index )

using namespace std;

VM_EXPORT
{
	template <typename Voxel = char>
	struct Compressor final : vm::NoCopy, vm::NoMove
	{
		Compressor( Idx const &block_dim,
					UnboundedWriter &writer,
					Pipe &_ ) :
		  block_dim( block_dim ),
		  block_len( block_dim.total() * sizeof( Voxel ) / sizeof( char ) ),
		  writer( writer ),
		  _( _ )
		{
		}
		~Compressor()
		{
			writer.seek( fend );
			uint64_t n = 0;
			for ( auto &e : index ) {
				++n;
				auto &idx = e.first;
				auto &blk = e.second;
				idx.write_to( writer );
				blk.write_to( writer );
			}

			auto meta = CompressMeta{}
						  .set_block_count( n )
						  .set_index_offset( fend )
						  .set_block_len( block_len );
			meta.write_to( writer );
		}

		template <typename R>  // R: Reader
		void put( Idx const &idx, R &reader )
		{
			if ( index.find( idx ) != index.end() ) {
				throw runtime_error( vm::fmt( "duplicate entry: {}", idx ) );
			}
			auto a = writer.tell();
			// auto x = reader.tell();
			_.transfer( reader, writer );
			auto b = writer.tell();
			// auto y = reader.tell();
			fend = std::max( b, fend );
			auto len = b - a;
			// vm::println( "put {} with len {} to {}", idx, y - x, make_pair( a, len ) );
			index[ idx ] = BlockIndex{}
							 .set_offset( a )
							 .set_len( len );
		}
		void put( Idx const &idx, istream &is, size_t offset = 0 )
		{
			StreamReader reader( is, offset, block_len );
			put( idx, reader );
		}
		void put( Idx const &idx, Voxel const *block )
		{
			auto ptr = reinterpret_cast<char const *>( block );
			SliceReader reader( ptr, 0, block_len );
			put( idx, reader );
		}

	private:
		Idx block_dim;
		size_t block_len;
		size_t fend = 0;
		UnboundedWriter &writer;
		Pipe &_;
		std::map<Idx, BlockIndex> index;
	};
}

VM_END_MODULE()

}  // namespace vol
