#pragma once

#include <VMUtils/fmt.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include <vocomp/io.hpp>

VM_BEGIN_MODULE( vol )

using namespace std;

#pragma pack( push )
#pragma pack( 4 )

VM_EXPORT
{
	struct BlockIndex
	{
		VM_DEFINE_ATTRIBUTE( uint32_t, first_frame );
		VM_DEFINE_ATTRIBUTE( uint32_t, last_frame );
		VM_DEFINE_ATTRIBUTE( uint64_t, offset );

		bool operator<( BlockIndex const &other ) const
		{
			return first_frame < other.first_frame ||
				   first_frame == other.first_frame && ( offset < other.offset );
		}

		bool operator==( BlockIndex const &other ) const
		{
			return first_frame == other.first_frame &&
				   last_frame == other.last_frame &&
				   offset == other.offset;
		}

		friend ostream &operator<<( ostream &os, BlockIndex const &_ )
		{
			vm::fprint( os, "{{ f0: {}, f1: {}, offset:{} }}", _.first_frame, _.last_frame, _.offset );
			return os;
		}
	};

	struct Idx
	{
		VM_DEFINE_ATTRIBUTE( uint32_t, x );
		VM_DEFINE_ATTRIBUTE( uint32_t, y );
		VM_DEFINE_ATTRIBUTE( uint32_t, z );

		uint64_t total() const { return (uint64_t)x * y * z; }

		bool operator<( Idx const &other ) const
		{
			return x < other.x ||
				   x == other.x && ( y < other.y ||
									 y == other.y && z < other.z );
		}
		bool operator==( Idx const &other ) const
		{
			return x == other.x && y == other.y && z == other.z;
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
};

#pragma pack( pop )

VM_END_MODULE()
