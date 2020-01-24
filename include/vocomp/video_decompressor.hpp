#pragma once

#include <atomic>
#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include <cudafx/stream.hpp>
#include <cudafx/misc.hpp>
#include <cudafx/memory.hpp>
#include <cudafx/array.hpp>
#include <vocomp/utils/common.hpp>
#include <vocomp/utils/io.hpp>

VM_BEGIN_MODULE( vol )

struct VideoDecompressorImpl;

struct VideoDecompressOptions
{
	VM_DEFINE_ATTRIBUTE( EncodeMethod, encode );
	VM_DEFINE_ATTRIBUTE( unsigned, io_queue_size ) = 4;
};

struct VideoStreamPacketMapSlot;

struct VideoStreamPacketReleaseEvent
{
	bool poll() const { return idptr->load() != val; }

private:
	std::atomic_int64_t const *idptr;
	int64_t val;
	friend class VideoStreamPacketMapSlot;
};

struct VideoStreamPacketImpl;

struct VideoStreamPacket : vm::NoCopy, vm::NoMove
{
	VideoStreamPacket( VideoStreamPacketImpl const &_ ) :
	  _( _ ) {}

	void copy_async( cufx::MemoryView1D<unsigned char> const &dst ) const
	{
		return copy_async( dst, 0, length );
	}
	void copy_async( cufx::MemoryView1D<unsigned char> const &dst, unsigned offset, unsigned length ) const;

public:
	unsigned length;
	unsigned id;
	VideoStreamPacketReleaseEvent release_event;

private:
	VideoStreamPacketImpl const &_;
};

struct VideoDecompressor final : vm::NoCopy
{
	VideoDecompressor( VideoDecompressOptions const &opts = VideoDecompressOptions{} );
	~VideoDecompressor();

	void decompress( Reader &reader,
					 std::function<void( VideoStreamPacket const & )> const &consumer );

private:
	vm::Box<VideoDecompressorImpl> _;
};

VM_END_MODULE()
