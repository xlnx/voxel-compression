#pragma once

#include <functional>
#include <string>
#include <fstream>
#include <tuple>
#include <cstring>
#include <algorithm>

#include <VMUtils/fmt.hpp>
#include <VMUtils/modules.hpp>
#include <cudafx/driver/context.hpp>
#include <FFmpegDemuxer.h>
#include <NvDecoder.h>

VM_BEGIN_MODULE( vol )

using namespace std;

struct FnReader : FFmpegDemuxer::DataProvider
{
	FnReader( function<int( uint8_t *, int )> &&reader ) :
	  _( std::move( reader ) ) {}

	int GetData( uint8_t *pbuf, int nbuf ) override
	{
		return _( pbuf, nbuf );
	}

private:
	function<int( uint8_t *, int )> _;
};

struct FnWriter
{
	FnWriter( function<void( char const *, size_t )> &&writer ) :
	  _( std::move( writer ) ) {}

	void write( char const *src, size_t len ) { _( src, len ); }

private:
	function<void( char const *, size_t )> _;
};

VM_EXPORT
{
	struct Dim3
	{
		size_t x, y, z;

		friend ostream &operator<<( ostream &os, Dim3 const &_ )
		{
			vm::fprint( os, "{}", make_tuple( _.x, _.y, _.z ) );
			return os;
		}
	};

	struct SliceReader : FnReader
	{
		SliceReader( char const *src, size_t slen ) :
		  FnReader( [=]( uint8_t *dst, int dlen ) mutable -> int {
			  int nread = std::min( slen, size_t( dlen ) );
			  memcpy( dst, src, nread );
			  src += nread;
			  slen -= nread;
			  return nread;
		  } )
		{
		}
	};
	struct FstreamReader : FnReader
	{
		FstreamReader( ifstream &is, size_t offset, size_t slen ) :
		  FnReader( [=, &is]( uint8_t *dst, int dlen ) mutable -> int {
			  auto nread = std::min( slen, size_t( dlen ) );
			  nread = is.seekg( offset ).read( reinterpret_cast<char *>( dst ), nread ).gcount();
			  offset += nread;
			  slen -= nread;
			  return nread;
		  } )
		{
		}
	};
	struct SliceWriter : FnWriter
	{
		SliceWriter( char *dst, size_t dlen ) :
		  FnWriter( [=]( char const *src, size_t slen ) mutable {
			  memcpy( dst, src, slen );
			  dst += slen;
			  dlen -= slen;
		  } )
		{
		}
	};
	struct FstreamWriter : FnWriter
	{
		FstreamWriter( ofstream &os, size_t offset, size_t dlen ) :
		  FnWriter( [=, &os]( char const *src, size_t slen ) mutable {
			  os.seekp( offset ).write( src, slen );
			  offset += slen;
			  dlen -= slen;
		  } )
		{
		}
	};

	void decompress( FnReader & reader, FnWriter & writer )
	{
		cufx::drv::Context ctx( 0 );
		FFmpegDemuxer demuxer( &reader );
		NvDecoder dec( ctx, false, FFmpeg2NvCodecId( demuxer.GetVideoCodec() ) );

		int nVideoBytes = 0, nFrameReturned = 0, nFrame = 0;
		uint8_t *pVideo = NULL, **ppFrame;
		bool bDecodeOutSemiPlanar = false;

		do {
			demuxer.Demux( &pVideo, &nVideoBytes );
			dec.Decode( pVideo, nVideoBytes, &ppFrame, &nFrameReturned );
			if ( !nFrame && nFrameReturned ) {
				vm::println( "INFO: {}", dec.GetVideoInfo() );
			}
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
}

VM_END_MODULE()
