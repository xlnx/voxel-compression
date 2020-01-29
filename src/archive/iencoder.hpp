#pragma once

#include <VMUtils/modules.hpp>
#include <VMUtils/concepts.hpp>
#include <varch/utils/common.hpp>
#include <varch/utils/unbounded_io.hpp>

VM_BEGIN_MODULE( vol )

struct IEncoder : vm::Dynamic, vm::NoCopy, vm::NoMove
{
	virtual void encode( Reader &reader, Writer &writer,
						 std::vector<uint32_t> &frame_len ) = 0;
	virtual std::size_t frame_size() const = 0;
};

VM_END_MODULE()
