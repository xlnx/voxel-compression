#pragma once

#include "../../idecoder.hpp"

VM_BEGIN_MODULE( vol )

struct IsvcDecoderWrapperImpl;

struct IsvcDecoderWrapper : IDecoder
{
	IsvcDecoderWrapper( VideoDecompressOptions const &opts = VideoDecompressOptions{} );
	~IsvcDecoderWrapper();

	void decode( Reader &reader,
				 std::function<void( Packet const & )> const &consumer ) override;

private:
	vm::Box<IsvcDecoderWrapperImpl> _;
};

VM_END_MODULE()
