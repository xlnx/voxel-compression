#pragma once

#include "../../iencoder.hpp"

VM_BEGIN_MODULE( vol )

struct IsvcEncoderWrapperImpl;

struct IsvcEncoderWrapper : IEncoder
{
	IsvcEncoderWrapper( EncodeOptions const &opts );
	~IsvcEncoderWrapper();

	void encode( Reader &reader, Writer &writer,
				 std::vector<uint32_t> &frame_len ) override;
	std::size_t frame_size() const override;

private:
	vm::Box<IsvcEncoderWrapperImpl> _;
};

VM_END_MODULE()
