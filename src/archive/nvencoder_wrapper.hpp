#pragma once

#include <VMUtils/modules.hpp>
#include <VMUtils/concepts.hpp>
#include <cudafx/driver/context.hpp>
#include <varch/utils/common.hpp>
#include <varch/utils/unbounded_io.hpp>
#include "nvenc/NvEncoder.h"
#include "nvenc/NvEncoderCuda.h"

// #include <varch/video/VideoCompressor.hpp>

VM_BEGIN_MODULE( vol )

struct NvEncoderWrapper : vm::Dynamic, vm::NoCopy, vm::NoMove
{
	NvEncoderWrapper( VideoCompressOptions const &opts );
	~NvEncoderWrapper();

	void encode( Reader &reader, Writer &out, std::vector<uint32_t> &frame_len );

	static cufx::drv::Context ctx;
	static std::unique_ptr<NvEncoder> _;
};

VM_END_MODULE()
