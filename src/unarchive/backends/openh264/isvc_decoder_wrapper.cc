#include <wels/codec_api.h>
#include "isvc_decoder_wrapper.hpp"

VM_BEGIN_MODULE( vol )

struct BitStreamPacket : Packet
{
	void copy_to( cufx::MemoryView1D<unsigned char> const &dst,
				  unsigned offset, unsigned length ) const override
	{
		if ( !length ) return;
		if ( dst.device_id().is_device() ) {
			throw std::logic_error( "invalid copy to device memory" );
		}
		int end_offset = offset + length;
		auto hp = dst.ptr();
		for ( int i = 0; i != 2; ++i ) {
			int beg = std::max( int( offset ) - byte_offset[ i ], 0 );
			int end = std::min( end_offset - byte_offset[ i ],
								byte_offset[ i + 1 ] - byte_offset[ i ] );
			int len = end - beg;
			if ( len > 0 ) {
				auto w = width[ i ];
				auto h = height[ i ];
				auto s = stride[ i ];

				int beg_j = ( beg + w - 1 ) / w;
				int beg_off = beg % w;
				int end_j = end / w;
				int end_off = end % w;
				// vm::println( "{} {} {} {}", (void *)yuv[ i ], w, h, s );
				auto src = nv12[ i ] + beg_j * s;
				if ( beg_off ) {
					src -= s;
					// vm::println( "# {} -> {}", (void *)hp, w - beg_off );
					memcpy( hp, src, w - beg_off );
					hp += w - beg_off;
					src += s;
				}
				// vm::println( "# {}, {} -> {}", beg_j, end_j, w );
				for ( int j = beg_j; j < end_j; ++j ) {
					memcpy( hp, src, w );
					hp += w;
					src += s;
				}
				if ( end_off ) {
					// vm::println( "# {} -> {}", (void *)hp, end_off );
					memcpy( hp, src, end_off );
					hp += end_off;
					src += s;
				}
			}
		}
		// vm::println( "copied {}", hp - dst.ptr() );
	}

	void init()
	{
		width[ 0 ] = info.UsrData.sSystemBuffer.iWidth;
		height[ 0 ] = info.UsrData.sSystemBuffer.iHeight;
		width[ 1 ] = info.UsrData.sSystemBuffer.iWidth;
		height[ 1 ] = info.UsrData.sSystemBuffer.iHeight >> 1;
		stride[ 0 ] = info.UsrData.sSystemBuffer.iStride[ 0 ];
		stride[ 1 ] = width[ 1 ];
		byte_offset[ 0 ] = 0;
		byte_offset[ 1 ] = width[ 0 ] * height[ 0 ];
		byte_offset[ 2 ] = length = byte_offset[ 1 ] + width[ 1 ] * height[ 1 ];

		uv_buf.resize( byte_offset[ 2 ] - byte_offset[ 1 ] );
		nv12[ 0 ] = yuv[ 0 ];
		nv12[ 1 ] = uv_buf.data();

		int w = width[ 1 ] >> 1;
		int h = height[ 1 ];
		int s = info.UsrData.sSystemBuffer.iStride[ 1 ];
		auto *u = yuv[ 1 ], *v = yuv[ 2 ];
		auto *dst = uv_buf.data();
		for ( int i = 0; i != h; ++i, u += s, v += s ) {
			for ( int j = 0; j != w; ++j, dst += 2 ) {
				dst[ 0 ] = u[ j ];
				dst[ 1 ] = v[ j ];
			}
		}
	}

public:
	unsigned char *yuv[ 3 ];
	unsigned char *nv12[ 2 ];
	vector<unsigned char> uv_buf;
	int width[ 2 ];
	int height[ 2 ];
	int stride[ 2 ];
	int byte_offset[ 3 ];
	SBufferInfo info = {};
};

struct IsvcDecoderWrapperImpl
{
	IsvcDecoderWrapperImpl( DecodeOptions const &opts )
	{
		WelsCreateDecoder( &decoder );

		SDecodingParam param = {};
		param.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
		//for Parsing only, the assignment is mandatory
		// param.bParseOnly = true;
		decoder->Initialize( &param );
	}

	~IsvcDecoderWrapperImpl()
	{
		decoder->Uninitialize();
		WelsDestroyDecoder( decoder );
	}

public:
	void decode( Reader &reader,
				 std::function<void( Packet const & )> const &consumer )
	{
		out.id = 0;
		uint32_t frame_len;
		while ( reader.read_typed( frame_len ) ) {
			auto packet = get_packet( frame_len );
			reader.read( reinterpret_cast<char *>( packet ), frame_len );
			// vm::println( "#dec_src: { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} ...",
			// 			 int( packet[ 96 + 0 ] ), int( packet[ 96 + 1 ] ), int( packet[ 96 + 2 ] ),
			// 			 int( packet[ 96 + 3 ] ), int( packet[ 96 + 4 ] ), int( packet[ 96 + 5 ] ), int( packet[ 96 + 6 ] ),
			// 			 int( packet[ 96 + 7 ] ), int( packet[ 96 + 8 ] ), int( packet[ 96 + 9 ] ) );
			out.info.iBufferStatus = 0;
			decoder->DecodeFrameNoDelay( packet, frame_len, out.yuv, &out.info );
			consume_packet( out, consumer );
		}
		do {
			out.info.iBufferStatus = 0;
			decoder->FlushFrame( out.yuv, &out.info );
		} while ( consume_packet( out, consumer ) );
	}

	bool consume_packet( BitStreamPacket &out,
						 std::function<void( Packet const & )> const &consumer )
	{
		if ( out.info.iBufferStatus == 1 ) {
			out.init();
			out.id += 1;
			consumer( out );
			return true;
		}
		return false;
	}

	unsigned char *get_packet( uint32_t len )
	{
		thread_local auto _ = std::vector<unsigned char>( 1024 );
		if ( len > _.size() ) {
			_.resize( len );
		}
		return _.data();
	}

public:
	ISVCDecoder *decoder = nullptr;
	BitStreamPacket out;
};

IsvcDecoderWrapper::IsvcDecoderWrapper( DecodeOptions const &opts ) :
  _( new IsvcDecoderWrapperImpl( opts ) )
{
}

IsvcDecoderWrapper::~IsvcDecoderWrapper()
{
}

void IsvcDecoderWrapper::decode( Reader &reader,
								 std::function<void( Packet const & )> const &consumer )
{
	_->decode( reader, consumer );
}

VM_END_MODULE()
