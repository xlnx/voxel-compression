#pragma once

#include <atomic>
#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include <cudafx/stream.hpp>
#include <cudafx/misc.hpp>
#include <cudafx/memory.hpp>
#include <cudafx/array.hpp>
#include <varch/utils/common.hpp>
#include <varch/utils/io.hpp>
#include "../../idecoder.hpp"

VM_BEGIN_MODULE( vol )

struct NvDecoderAsyncImpl;

struct NvBitStreamPacketMapSlot;

struct NvBitStreamPacketImpl;

struct NvBitStreamPacketReleaseEvent
{
	bool poll() const { return idptr->load() != val; }

private:
	std::atomic_int64_t const *idptr;
	int64_t val;
	friend class NvBitStreamPacketMapSlot;
};

struct NvBitStreamPacket : Packet
{
	NvBitStreamPacket( NvBitStreamPacketImpl const &_ ) :
	  _( _ ) {}

	void copy_to( cufx::MemoryView1D<unsigned char> const &dst, unsigned offset, unsigned length ) const;

public:
	NvBitStreamPacketReleaseEvent release_event;

private:
	NvBitStreamPacketImpl const &_;
};

struct NvDecoderAsync final : IDecoder
{
	NvDecoderAsync( VideoDecompressOptions const &opts = VideoDecompressOptions{} );
	~NvDecoderAsync();

	void decode( Reader &reader,
				 std::function<void( Packet const & )> const &consumer ) override
	{
		decode( reader, [&]( NvBitStreamPacket const &pkt ) { consumer( pkt ); } );
	}
	void decode( Reader &reader,
				 std::function<void( NvBitStreamPacket const & )> const &consumer );

private:
	vm::Box<NvDecoderAsyncImpl> _;
};

VM_END_MODULE()
