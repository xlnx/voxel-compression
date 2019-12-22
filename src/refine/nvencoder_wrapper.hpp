#pragma once

#include <VMUtils/modules.hpp>
#include <VMUtils/concepts.hpp>
#include <cudafx/driver/context.hpp>
#include <vocomp/utils/common.hpp>
#include <vocomp/utils/unbounded_io.hpp>
#include "nvenc/NvEncoder.h"
#include "nvenc/NvEncoderCuda.h"

// #include <vocomp/video/VideoCompressor.hpp>

VM_BEGIN_MODULE( vol )

struct NvEncoderWrapper : vm::Dynamic, vm::NoCopy, vm::NoMove
{
	NvEncoderWrapper( VideoCompressOptions const &opts );
	~NvEncoderWrapper();
	void encode( Reader &reader, Writer &out, std::vector<uint32_t> &frame_len );

	cufx::drv::Context ctx = 0;
	std::unique_ptr<NvEncoder> _;
	NV_ENC_CONFIG cfg = { NV_ENC_CONFIG_VER };
	NV_ENC_INITIALIZE_PARAMS params = { NV_ENC_INITIALIZE_PARAMS_VER };
};

VM_END_MODULE()
