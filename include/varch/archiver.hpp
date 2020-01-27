#pragma once

#include <cstring>
#include <fstream>
#include <atomic>
#include <ciso646>
#include <VMUtils/nonnull.hpp>
#include <varch/utils/common.hpp>

VM_BEGIN_MODULE( vol )

struct ArchiverImpl;

VM_EXPORT
{
	struct ArchiverOptions
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

	struct Archiver final : vm::NoCopy
	{
		Archiver( ArchiverOptions const &opts );
		~Archiver();
		bool convert();

	private:
		vm::Box<ArchiverImpl> _;
	};
}

VM_END_MODULE()
