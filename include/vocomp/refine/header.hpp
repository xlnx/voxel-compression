#pragma once

#include <VMUtils/modules.hpp>
#include <VMUtils/attributes.hpp>
#include <vocomp/index.hpp>
#include <vocomp/io.hpp>

namespace vol
{
VM_BEGIN_MODULE( refine )

using namespace std;

#pragma pack( push )
#pragma pack( 4 )

struct Header
{
	VM_DEFINE_ATTRIBUTE( uint64_t, version );
	VM_DEFINE_ATTRIBUTE( Idx, raw );
	VM_DEFINE_ATTRIBUTE( Idx, dim );
	VM_DEFINE_ATTRIBUTE( Idx, adjusted );
	VM_DEFINE_ATTRIBUTE( uint64_t, log_block_size );
	VM_DEFINE_ATTRIBUTE( uint64_t, block_size );
	VM_DEFINE_ATTRIBUTE( uint64_t, block_inner );
	VM_DEFINE_ATTRIBUTE( uint64_t, padding );
};

#pragma pack( pop )

VM_END_MODULE()

}  // namespace vol
