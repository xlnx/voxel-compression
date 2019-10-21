#pragma once

#include <VMUtils/modules.hpp>

namespace vol
{
VM_BEGIN_MODULE( video )

VM_EXPORT
{
	enum class EncodeMethod : char
	{
		H264,
		HEVC
	};
}

VM_END_MODULE()

}  // namespace vol
