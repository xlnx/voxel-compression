#include <numeric>
#include <algorithm>
#include <wels/codec_api.h>
#include "isvc_encoder_wrapper.hpp"

VM_BEGIN_MODULE( vol )

struct IsvcEncoderWrapperImpl
{
	IsvcEncoderWrapperImpl( VideoCompressOptions const &opts ) :
	  width( opts.width ),
	  height( opts.height ),
	  frame_size( width * height * 3 / 2 )
	{
		WelsCreateSVCEncoder( &encoder );

		SEncParamBase param = {};
		param.iUsageType = CAMERA_VIDEO_REAL_TIME;
		param.fMaxFrameRate = 60;
		param.iPicWidth = width;
		param.iPicHeight = height;
		param.iTargetBitrate = 5000000;
		encoder->Initialize( &param );

		int trace_level = WELS_LOG_QUIET;
		encoder->SetOption( ENCODER_OPTION_TRACE_LEVEL, &trace_level );
		int video_format = videoFormatI420;
		encoder->SetOption( ENCODER_OPTION_DATAFORMAT, &video_format );

		pic.iPicWidth = width;
		pic.iPicHeight = height;
		pic.iColorFormat = video_format;
		pic.iStride[ 0 ] = pic.iPicWidth;
		pic.iStride[ 1 ] = pic.iStride[ 2 ] = pic.iPicWidth >> 1;
	}

	~IsvcEncoderWrapperImpl()
	{
		if ( encoder ) {
			encoder->Uninitialize();
			WelsDestroySVCEncoder( encoder );
		}
	}

public:
	void encode( Reader &reader, Writer &writer, std::vector<uint32_t> &frame_len )
	{
		thread_local std::vector<unsigned char> y_plane, uv_plane, nv12_plane;

		auto area = width * height;
		auto area_2 = area >> 1;

		y_plane.resize( area );
		uv_plane.resize( area_2 );
		nv12_plane.resize( area_2 );

		pic.pData[ 0 ] = y_plane.data();
		pic.pData[ 1 ] = uv_plane.data();
		pic.pData[ 2 ] = pic.pData[ 1 ] + area_2;

		while ( reader.read( reinterpret_cast<char *>( y_plane.data() ), y_plane.size() ) == y_plane.size() &&
				reader.read( reinterpret_cast<char *>( nv12_plane.data() ), nv12_plane.size() ) == nv12_plane.size() ) {
			perform_nv12_to_yuv( nv12_plane, uv_plane );

			if ( auto err = encoder->ForceIntraFrame( true ) ) {
				throw std::logic_error( "force idr frame failed" );
			}
			// vm::println( "#enc_src: { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} ...",
			// 			 int( buffer[ 0 ] ), int( buffer[ 1 ] ), int( buffer[ 2 ] ),
			// 			 int( buffer[ 3 ] ), int( buffer[ 4 ] ), int( buffer[ 5 ] ), int( buffer[ 6 ] ),
			// 			 int( buffer[ 7 ] ), int( buffer[ 8 ] ), int( buffer[ 9 ] ) );
			int rv = encoder->EncodeFrame( &pic, &info );
			if ( info.eFrameType != videoFrameTypeSkip ) {
				// std::cout << info.eFrameType << " " << info.iLayerNum << std::endl;
				std::vector<uint32_t> lens;
				for ( int i = 0; i != info.iLayerNum; ++i ) {
					auto &layerInfo = info.sLayerInfo[ i ];
					lens.emplace_back( std::accumulate( layerInfo.pNalLengthInByte,
														layerInfo.pNalLengthInByte + layerInfo.iNalCount, 0 ) );
				}
				uint32_t len = std::accumulate( lens.begin(), lens.end(), 0 );
				writer.write_typed( len );
				auto packet = info.sLayerInfo[ 1 ].pBsBuf - lens[ 0 ];
				// vm::println( "#enc_dst {} -> len {}: { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} ...",
				// 			 frame_len.size(), len, int( packet[ 96 + 0 ] ), int( packet[ 96 + 1 ] ), int( packet[ 96 + 2 ] ),
				// 			 int( packet[ 96 + 3 ] ), int( packet[ 96 + 4 ] ), int( packet[ 96 + 5 ] ), int( packet[ 96 + 6 ] ),
				// 			 int( packet[ 96 + 7 ] ), int( packet[ 96 + 8 ] ), int( packet[ 96 + 9 ] ) );
				for ( int i = 0; i != info.iLayerNum; ++i ) {
					auto &layerInfo = info.sLayerInfo[ i ];
					writer.write( reinterpret_cast<char *>( layerInfo.pBsBuf ), lens[ i ] );
				}
				frame_len.emplace_back( sizeof( len ) + len );
			}
		}
	}

	void perform_nv12_to_yuv( vector<unsigned char> &nv12_plane, vector<unsigned char> &yuv_plane )
	{
		auto *nv12 = nv12_plane.data();
		auto *nv12_end = nv12 + nv12_plane.size();
		auto *u = yuv_plane.data();
		auto *v = yuv_plane.data() + ( yuv_plane.size() >> 1 );

		while ( nv12 < nv12_end ) {
			*u++ = *nv12++;
			*v++ = *nv12++;
		}
	}

public:
	ISVCEncoder *encoder = nullptr;
	unsigned width, height, frame_size;
	SFrameBSInfo info = {};
	SSourcePicture pic = {};
};

IsvcEncoderWrapper::IsvcEncoderWrapper( VideoCompressOptions const &opts ) :
  _( new IsvcEncoderWrapperImpl( opts ) )
{
}

IsvcEncoderWrapper::~IsvcEncoderWrapper()
{
}

void IsvcEncoderWrapper::encode( Reader &reader, Writer &writer, std::vector<uint32_t> &frame_len )
{
	return _->encode( reader, writer, frame_len );
}

std::size_t IsvcEncoderWrapper::frame_size() const
{
	return _->frame_size;
}

VM_END_MODULE()
