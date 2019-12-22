#pragma once

#include <cstring>
#include <fstream>
#include <atomic>
#include <ciso646>
#include <VMUtils/nonnull.hpp>
#include <vocomp/utils/common.hpp>

VM_BEGIN_MODULE( vol )

struct RefinerImpl;

VM_EXPORT
{
	struct RefinerOptions
	{
		VM_DEFINE_ATTRIBUTE( size_t, x );
		VM_DEFINE_ATTRIBUTE( size_t, y );
		VM_DEFINE_ATTRIBUTE( size_t, z );
		VM_DEFINE_ATTRIBUTE( size_t, log_block_size );
		VM_DEFINE_ATTRIBUTE( size_t, padding );
		VM_DEFINE_ATTRIBUTE( string, input );
		VM_DEFINE_ATTRIBUTE( string, output );
		VM_DEFINE_ATTRIBUTE( VideoCompressOptions, compress_opts );
		VM_DEFINE_ATTRIBUTE( size_t, suggest_mem_gb ) = 128;
	};

	struct Refiner final : vm::NoCopy
	{
		Refiner( RefinerOptions const &opts );
		~Refiner();
		bool convert();

	private:
		vm::Box<RefinerImpl> _;
	};
}

VM_END_MODULE()
