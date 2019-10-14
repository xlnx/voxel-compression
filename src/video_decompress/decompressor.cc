#include <vocomp/video/decompressor.hpp>

#include <nvcodec/NvDecoder.h>
#include <nvcodec/FFmpegDemuxer.h>
#include <cudafx/driver/context.hpp>

namespace vol
{
VM_BEGIN_MODULE( video )

struct ReaderWrapper : FFmpegDemuxer::DataProvider
{
	ReaderWrapper( Reader *_ ) :
	  _( _ )
	{
	}

	int GetData( uint8_t *pbuf, int nbuf ) override
	{
		auto nread = _->read( reinterpret_cast<char *>( pbuf ), nbuf );
		if ( !nread ) {
			return AVERROR_EOF;
		}
		return nread;
	}

	Reader *_;
};

struct DecompressorImpl final : vm::NoCopy, vm::NoMove
{
	DecompressorImpl( Reader &hint ) :
	  wrapper( &hint ),
	  demuxer( &wrapper ),
	  dec( ctx, false, FFmpeg2NvCodecId( demuxer.GetVideoCodec() ) )
	{
	}

	void decompress( Reader &reader, Writer &writer )
	{
		wrapper._ = &reader;
		int nVideoBytes = 0, nFrameReturned = 0, nFrame = 0;
		uint8_t *pVideo = NULL, **ppFrame;
		bool bDecodeOutSemiPlanar = false;

		do {
			demuxer.Demux( &pVideo, &nVideoBytes );
			dec.Decode( pVideo, nVideoBytes, &ppFrame, &nFrameReturned );
			// if ( !nFrame && nFrameReturned ) {
			// 	vm::println( "INFO: {}", dec.GetVideoInfo() );
			// }
			bDecodeOutSemiPlanar = ( dec.GetOutputFormat() == cudaVideoSurfaceFormat_NV12 ) || ( dec.GetOutputFormat() == cudaVideoSurfaceFormat_P016 );

			for ( int i = 0; i < nFrameReturned; i++ ) {
				// if ( bOutPlanar && bDecodeOutSemiPlanar ) {
				// 	ConvertSemiplanarToPlanar( ppFrame[ i ], dec.GetWidth(), dec.GetHeight(), dec.GetBitDepth() );
				// }
				writer.write( reinterpret_cast<char *>( ppFrame[ i ] ), dec.GetFrameSize() );
			}
			nFrame += nFrameReturned;
		} while ( nVideoBytes );
	}

private:
	cufx::drv::Context ctx = 0;
	ReaderWrapper wrapper;
	FFmpegDemuxer demuxer;
	NvDecoder dec;
};

VM_EXPORT
{
	Decompressor::Decompressor( Reader & hint ) :
	  _( new DecompressorImpl( hint ) )
	{
	}
	Decompressor::~Decompressor()
	{
	}
	void Decompressor::decompress( Reader & reader, Writer & writer )
	{
		_->decompress( reader, writer );
	}
}

VM_END_MODULE()

}  // namespace vol
