#pragma once

#include <cudafx/driver/context.hpp>
#include "../../iencoder.hpp"
#include "NvEncoder.h"
#include "NvEncoderCuda.h"

VM_BEGIN_MODULE( vol )

struct NvEncoderWrapper : IEncoder
{
	NvEncoderWrapper( EncodeOptions const &opts );
	~NvEncoderWrapper();

	void encode( Reader &reader, Writer &out, std::vector<uint32_t> &frame_len ) override;
	std::size_t frame_size() const override;

private:
	static cufx::drv::Context ctx;
	static std::unique_ptr<NvEncoder> _;
};

VM_END_MODULE()
