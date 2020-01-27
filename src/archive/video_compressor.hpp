#pragma once

#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include <varch/utils/common.hpp>
#include <varch/utils/io.hpp>

VM_BEGIN_MODULE( vol )

struct VideoCompressorImpl;

struct VideoCompressor final : vm::NoCopy
{
	VideoCompressor( Writer &out, VideoCompressOptions const &_ = VideoCompressOptions{} );
	~VideoCompressor();

	BlockIndex accept( vm::Arc<Reader> &&reader );
	void flush( bool wait = false );
	void wait();
	uint32_t frame_size() const;
	std::vector<uint64_t> const &frame_offset() const;
	uint32_t frame_count() const { return frame_offset().size() - 1; }

private:
	vm::Box<VideoCompressorImpl> _;
};

VM_END_MODULE()
