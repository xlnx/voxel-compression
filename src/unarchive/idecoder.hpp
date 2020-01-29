#pragma once

#include <functional>
#include <VMUtils/modules.hpp>
#include <VMUtils/concepts.hpp>
#include <varch/utils/common.hpp>
#include <varch/utils/unbounded_io.hpp>

VM_BEGIN_MODULE( vol )

struct IDecoder : vm::Dynamic, vm::NoCopy, vm::NoMove
{
	virtual void decode( Reader &reader,
						 std::function<void( Packet const & )> const &consumer ) = 0;
};

VM_END_MODULE()
