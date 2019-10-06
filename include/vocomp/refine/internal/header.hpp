#pragma once

#include <VMUtils/modules.hpp>
#include <VMUtils/attributes.hpp>
#include "../../io.hpp"

namespace vol
{
VM_BEGIN_MODULE( refine )

using namespace std;

struct Header
{
	struct
	{
		VM_DEFINE_ATTRIBUTE( uint64_t, x );
		VM_DEFINE_ATTRIBUTE( uint64_t, y );
		VM_DEFINE_ATTRIBUTE( uint64_t, z );
	} raw;
	struct
	{
		VM_DEFINE_ATTRIBUTE( uint64_t, x );
		VM_DEFINE_ATTRIBUTE( uint64_t, y );
		VM_DEFINE_ATTRIBUTE( uint64_t, z );
	} dim;
	struct
	{
		VM_DEFINE_ATTRIBUTE( uint64_t, x );
		VM_DEFINE_ATTRIBUTE( uint64_t, y );
		VM_DEFINE_ATTRIBUTE( uint64_t, z );
	} adjusted;
	VM_DEFINE_ATTRIBUTE( uint64_t, log_block_size );
	VM_DEFINE_ATTRIBUTE( uint64_t, block_size );
	VM_DEFINE_ATTRIBUTE( uint64_t, block_inner );
	VM_DEFINE_ATTRIBUTE( uint64_t, padding );

	void write_to( Writer &writer ) const
	{
		writer.write( reinterpret_cast<char const *>( this ), sizeof( *this ) );
	}
	static Header read_from( Reader &reader )
	{
		Header _;
		reader.read( reinterpret_cast<char *>( &_ ), sizeof( _ ) );
		return _;
	}
};

VM_END_MODULE()

}  // namespace vol

