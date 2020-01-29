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

VM_BEGIN_MODULE( vol )

struct NvDecoderAsyncImpl;

struct VideoDecompressOptions
{
	VM_DEFINE_ATTRIBUTE( EncodeMethod, encode );
	VM_DEFINE_ATTRIBUTE( unsigned, io_queue_size ) = 4;
};

struct NvBitStreamPacketMapSlot;

struct NvBitStreamPacketReleaseEvent
{
	bool poll() const { return idptr->load() != val; }

private:
	std::atomic_int64_t const *idptr;
	int64_t val;
	friend class NvBitStreamPacketMapSlot;
};

struct NvBitStreamPacketImpl;

struct NvBitStreamPacket : vm::NoCopy, vm::NoMove
{
	NvBitStreamPacket( NvBitStreamPacketImpl const &_ ) :
	  _( _ ) {}

	void copy_async( cufx::MemoryView1D<unsigned char> const &dst ) const
	{
		return copy_async( dst, 0, length );
	}
	void copy_async( cufx::MemoryView1D<unsigned char> const &dst, unsigned offset, unsigned length ) const;

public:
	unsigned length;
	unsigned id;
	NvBitStreamPacketReleaseEvent release_event;

private:
	NvBitStreamPacketImpl const &_;
};

struct NvDecoderAsync final : vm::NoCopy
{
	NvDecoderAsync( VideoDecompressOptions const &opts = VideoDecompressOptions{} );
	~NvDecoderAsync();

	void decompress( Reader &reader,
					 std::function<void( NvBitStreamPacket const & )> const &consumer );

private:
	vm::Box<NvDecoderAsyncImpl> _;
};

VM_END_MODULE()
