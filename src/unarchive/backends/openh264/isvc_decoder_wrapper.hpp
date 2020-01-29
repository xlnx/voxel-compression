#pragma once

#include "../../idecoder.hpp"

VM_BEGIN_MODULE( vol )

struct IsvcDecoderWrapperImpl;

struct IsvcDecoderWrapper : IDecoder
{
	IsvcDecoderWrapper( DecodeOptions const &opts = DecodeOptions{} );
	~IsvcDecoderWrapper();

	void decode( Reader &reader,
				 std::function<void( Packet const & )> const &consumer ) override;

private:
	vm::Box<IsvcDecoderWrapperImpl> _;
};

VM_END_MODULE()
