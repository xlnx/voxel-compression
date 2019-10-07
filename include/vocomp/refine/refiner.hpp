#pragma once

#include <cstring>
#include <fstream>
#include <atomic>
#include <ciso646>
#include <VMUtils/nonnull.hpp>
#include "internal/header.hpp"

namespace vol
{
VM_BEGIN_MODULE( refine )

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
	};

	struct ConvertOptions
	{
		VM_DEFINE_ATTRIBUTE(std::shared_ptr<vol::Pipe>, pipe);
		VM_DEFINE_ATTRIBUTE(size_t, suggest_mem_gb) = 128;
		VM_DEFINE_ATTRIBUTE(size_t, frame_len) = 0;
	};

	struct Refiner final : vm::NoCopy
	{
		Refiner( RefinerOptions const &opts );
		~Refiner();
		bool convert( ConvertOptions const &opts = ConvertOptions{} );

	private:
		vm::Box<RefinerImpl> _;
	};
}

VM_END_MODULE()

}  // namespace vol
