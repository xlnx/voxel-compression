#pragma once

#include <vocomp/voxel/internal/util.hpp>
#include <VMUtils/nonnull.hpp>
#include "internal/header.hpp"

namespace vol
{
VM_BEGIN_MODULE( refine )

struct ExtractorImpl;

VM_EXPORT
{
	struct Extractor final : vm::NoCopy
	{
		Extractor( Reader &reader, Pipe &pipe );
		~Extractor();
		bool extract( voxel::Idx idx, Writer &writer );

		auto raw() const { return _raw; }
		auto dim() const { return _dim; }
		auto adjusted() const { return _adjusted; }
		auto log_block_size() const { return _log_block_size; }
		auto block_size() const { return _block_size; }
		auto block_inner() const { return _block_inner; }
		auto padding() const { return _padding; }

	private:
		voxel::Idx _raw;
		voxel::Idx _dim;
		voxel::Idx _adjusted;
		size_t _log_block_size;
		size_t _block_size;
		size_t _block_inner;
		size_t _padding;

	private:
		vm::Box<ExtractorImpl> _;
	};
}

VM_END_MODULE()

}  // namespace vol
