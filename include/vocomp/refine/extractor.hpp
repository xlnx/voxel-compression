#pragma once

#include <vocomp/index/index.hpp>
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
		Extractor( Reader &reader );
		~Extractor();
		PartReader extract( index::Idx idx );

		auto raw() const { return _raw; }
		auto dim() const { return _dim; }
		auto adjusted() const { return _adjusted; }
		auto log_block_size() const { return _log_block_size; }
		auto block_size() const { return _block_size; }
		auto block_inner() const { return _block_inner; }
		auto padding() const { return _padding; }
		auto &index() const { return _index; }
		
	private:
		index::Idx _raw;
		index::Idx _dim;
		index::Idx _adjusted;
		size_t _log_block_size;
		size_t _block_size;
		size_t _block_inner;
		size_t _padding;
		std::map<index::Idx, index::__inner__::BlockIndex> &_index;

	private:
		vm::Box<ExtractorImpl> _;
	};
}

VM_END_MODULE()

}  // namespace vol
