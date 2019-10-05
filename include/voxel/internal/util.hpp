#pragma once

#include <VMUtils/fmt.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include <io.hpp>

namespace vol
{
VM_BEGIN_MODULE( voxel )

using namespace std;

struct BlockIndex
{
	VM_DEFINE_ATTRIBUTE( size_t, offset );
	VM_DEFINE_ATTRIBUTE( size_t, len );

	void write_to( Writer &writer ) const
	{
		uint64_t _[ 2 ] = { offset, len };
		writer.write( reinterpret_cast<char *>( _ ), sizeof( _ ) );
	}
	static BlockIndex read_from( Reader &reader )
	{
		uint64_t _[ 2 ];
		reader.read( reinterpret_cast<char *>( _ ), sizeof( _ ) );
		return BlockIndex{}
		  .set_offset( _[ 0 ] )
		  .set_len( _[ 1 ] );
	}

	friend ostream &operator<<( ostream &os, BlockIndex const &_ )
	{
		vm::fprint( os, "{}", make_pair( _.offset, _.len ) );
		return os;
	}
};

VM_EXPORT
{
	struct Idx
	{
		VM_DEFINE_ATTRIBUTE( size_t, x );
		VM_DEFINE_ATTRIBUTE( size_t, y );
		VM_DEFINE_ATTRIBUTE( size_t, z );

		void write_to( Writer &writer ) const
		{
			uint64_t _[ 3 ] = { x, y, z };
			writer.write( reinterpret_cast<char *>( _ ), sizeof( _ ) );
		}
		static Idx read_from( Reader &reader )
		{
			uint64_t _[ 3 ];
			reader.read( reinterpret_cast<char *>( _ ), sizeof( _ ) );
			return Idx{}
			  .set_x( _[ 0 ] )
			  .set_y( _[ 1 ] )
			  .set_z( _[ 2 ] );
		}

		size_t total() const { return x * y * z; }

		bool operator<( Idx const &other ) const
		{
			return x < other.x ||
				   x == other.x && ( y < other.y ||
									 y == other.y && z < other.z );
		}

		friend ostream &operator<<( ostream &os, Idx const &_ )
		{
			vm::fprint( os, "{}", make_tuple( _.x, _.y, _.z ) );
			return os;
		}
	};
}

struct CompressMeta final
{
	VM_DEFINE_ATTRIBUTE( uint64_t, block_count );
	VM_DEFINE_ATTRIBUTE( uint64_t, index_offset );
	VM_DEFINE_ATTRIBUTE( uint64_t, block_len );

	void write_to( Writer &writer ) const
	{
		writer.write( reinterpret_cast<char const *>( this ), sizeof( *this ) );
	}
	static CompressMeta read_from( Reader &reader )
	{
		CompressMeta _;
		reader.read( reinterpret_cast<char *>( &_ ), sizeof( _ ) );
		return _;
	}
};

VM_END_MODULE()

}  // namespace vol
