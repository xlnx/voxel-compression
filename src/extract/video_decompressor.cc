#include <vocomp/video_decompressor.hpp>

#include <atomic>
#include <cudafx/driver/context.hpp>
#include <cudafx/memory.hpp>
#include <cudafx/transfer.hpp>
#include <cudafx/array.hpp>
#include <VMUtils/with.hpp>
#include "nvdec/nvcuvid.h"
#include "nvdec/NvCodecUtils.h"
// #include "nvdec/NvDecoder.h"

VM_BEGIN_MODULE( vol )

/**
* @brief Exception class for error reporting from the decode API.
*/
class NVDECException : public std::exception
{
public:
	NVDECException( const std::string &errorStr, const CUresult errorCode ) :
	  m_errorString( errorStr ),
	  m_errorCode( errorCode ) {}

	virtual ~NVDECException() throw() {}
	virtual const char *what() const throw() { return m_errorString.c_str(); }
	CUresult getErrorCode() const { return m_errorCode; }
	const std::string &getErrorString() const { return m_errorString; }
	static NVDECException makeNVDECException( const std::string &errorStr, const CUresult errorCode,
											  const std::string &functionName, const std::string &fileName, int lineNo );

private:
	std::string m_errorString;
	CUresult m_errorCode;
};

inline NVDECException NVDECException::makeNVDECException( const std::string &errorStr, const CUresult errorCode, const std::string &functionName,
														  const std::string &fileName, int lineNo )
{
	std::ostringstream errorLog;
	errorLog << functionName << " : " << errorStr << " at " << fileName << ":" << lineNo << std::endl;
	NVDECException exception( errorLog.str(), errorCode );
	return exception;
}

#define NVDEC_THROW_ERROR( errorStr, errorCode )                                                           \
	do {                                                                                                   \
		throw NVDECException::makeNVDECException( errorStr, errorCode, __FUNCTION__, __FILE__, __LINE__ ); \
	} while ( 0 )

#define NVDEC_API_CALL( cuvidAPI )                                                                                   \
	do {                                                                                                             \
		CUresult errorCode = cuvidAPI;                                                                               \
		if ( errorCode != CUDA_SUCCESS ) {                                                                           \
			std::ostringstream errorLog;                                                                             \
			const char *szErrName = NULL;                                                                            \
			cuGetErrorString( errorCode, &szErrName );                                                               \
			errorLog << #cuvidAPI << " returned error " << errorCode << " " << szErrName;                            \
			throw NVDECException::makeNVDECException( errorLog.str(), errorCode, __FUNCTION__, __FILE__, __LINE__ ); \
		}                                                                                                            \
	} while ( 0 )

#define CUDA_DRVAPI_CALL( call )                                                                                 \
	do {                                                                                                         \
		CUresult err__ = call;                                                                                   \
		if ( err__ != CUDA_SUCCESS ) {                                                                           \
			const char *szErrName = NULL;                                                                        \
			cuGetErrorName( err__, &szErrName );                                                                 \
			std::ostringstream errorLog;                                                                         \
			errorLog << "CUDA driver API error " << szErrName;                                                   \
			throw NVDECException::makeNVDECException( errorLog.str(), err__, __FUNCTION__, __FILE__, __LINE__ ); \
		}                                                                                                        \
	} while ( 0 )

struct VideoDecompressorImpl;

/* one slot <-> one temporary devptr processing task */
struct VideoStreamPacketMapSlot
{
	VideoStreamPacketMapSlot() :
	  id( -1 )
	{
		CUDA_DRVAPI_CALL( cuStreamCreate( &stream, CU_STREAM_NON_BLOCKING ) );
	}
	~VideoStreamPacketMapSlot()
	{
		cuStreamDestroy( stream );
	}

public:
	/* add one devptr to fill this slot */
	VideoStreamPacketReleaseEvent map( CUvideodecoder decoder, int pid, CUdeviceptr *dp_src,
									   unsigned *src_pitch, CUVIDPROCPARAMS *params )
	{
		VideoStreamPacketReleaseEvent evt;
		NVDEC_API_CALL( cuvidMapVideoFrame( decoder, pid, dp_src, src_pitch, params ) );
		dp = *dp_src;
		id.store( evt.val = counter() );
		evt.idptr = &id;
		return evt;
	}
	/* wait for stream tasks and pop devptr */
	bool unmap( CUvideodecoder decoder )
	{
		if ( dp ) {
			cuStreamSynchronize( stream );
			NVDEC_API_CALL( cuvidUnmapVideoFrame( decoder, dp ) );
			dp = 0;
			id.store( -1 );
			return true;
		}
		return false;
	}

private:
	static int64_t counter()
	{
		static std::atomic_int64_t i64( 0 );
		return i64.fetch_add( 1 );
	}

private:
	CUdeviceptr dp = 0;
	CUstream stream;
	atomic_int64_t id;
	friend class VideoDecompressorImpl;
};

struct VideoDecompressorImpl final : vm::NoCopy, vm::NoMove
{
	uint8_t *get_packet( uint32_t len )
	{
		thread_local auto _ = std::vector<uint8_t>( 1024 );
		if ( len > _.size() ) {
			_.resize( len );
		}
		return _.data();
	}

	/* async decompress procedure */
	void decompress( Reader &reader,
					 std::function<void( VideoStreamPacket const & )> const &consumer );

public:
	VideoDecompressorImpl( VideoDecompressOptions const &opts ) :
	  io_queue_size( opts.io_queue_size )
	{
		NVDEC_API_CALL( cuvidCtxLockCreate( &ctxlock, ctx ) );

		CUVIDPARSERPARAMS params = {};
		params.pfnSequenceCallback = handle_video_sequence_proc;
		params.pfnDecodePicture = handle_picture_decode_proc;
		params.pfnDisplayPicture = handle_picture_display_proc;
		params.ulMaxNumDecodeSurfaces = io_queue_size;
		params.ulMaxDisplayDelay = 2;  // 2..4, increase pipelining
		params.pUserData = this;
		params.CodecType = [&] {
			switch ( opts.encode ) {
			default: return cudaVideoCodec_H264;
			case EncodeMethod::HEVC: return cudaVideoCodec_HEVC;
			}
		}();
		NVDEC_API_CALL( cuvidCreateVideoParser( &parser, &params ) );
	}
	~VideoDecompressorImpl()
	{
		if ( decoder ) {
			cuvidDestroyDecoder( decoder );
		}
		if ( parser ) {
			cuvidDestroyVideoParser( parser );
		}
		cuvidCtxLockDestroy( ctxlock );
	}

private:
	void decode_and_advance( uint8_t *data, int len, uint32_t flags = 0 );

private:
	/* nvidia video parser callbacks */
	int handle_video_sequence( CUVIDEOFORMAT *format );
	int handle_picture_decode( CUVIDPICPARAMS *params );
	int handle_picture_display( CUVIDPARSERDISPINFO *info );

	static int CUDAAPI handle_video_sequence_proc( void *self, CUVIDEOFORMAT *params )
	{
		return reinterpret_cast<VideoDecompressorImpl *>( self )->handle_video_sequence( params );
	}
	static int CUDAAPI handle_picture_decode_proc( void *self, CUVIDPICPARAMS *params )
	{
		return reinterpret_cast<VideoDecompressorImpl *>( self )->handle_picture_decode( params );
	}
	static int CUDAAPI handle_picture_display_proc( void *self, CUVIDPARSERDISPINFO *params )
	{
		return reinterpret_cast<VideoDecompressorImpl *>( self )->handle_picture_display( params );
	}

public:
	unsigned io_queue_size;
	unsigned packet_id;

	int surface_width;
	int surface_height;

	int width;
	int luma_height;
	int chroma_height;
	int chroma_plains;

private:
	using Consumer = std::function<void( VideoStreamPacket const & )>;

	vm::With<Consumer> consumer;
	unique_ptr<VideoStreamPacketMapSlot[]> slots;

private:
	cufx::drv::Context ctx = 0;
	// std::unique_ptr<NvDecoder> dec;
	CUvideoctxlock ctxlock = nullptr;
	CUvideoparser parser = nullptr;
	CUvideodecoder decoder = nullptr;
};

struct VideoStreamPacketImpl
{
	CUdeviceptr dp_src;
	unsigned src_pitch;
	CUstream stream;
	VideoDecompressorImpl *decomp;
};

void VideoStreamPacket::copy_async( cufx::MemoryView1D<unsigned char> const &dst, unsigned offset, unsigned length ) const
{
	auto dp_dst = dst.ptr();
	auto dp_dst_end = dp_dst + dst.size();

	if ( dst.size() < length ) {
		throw 2;
	}

	struct Rect
	{
		CUdeviceptr src;
		int height;
	};
	Rect rect[ 2 ] = { { _.dp_src, _.decomp->luma_height },
					   { _.dp_src + _.src_pitch * _.decomp->surface_height, _.decomp->chroma_height } };
	if ( _.decomp->surface_height == _.decomp->luma_height ) {
		rect[ 0 ].height += _.decomp->chroma_height;
		rect[ 1 ].height = 0;
	}
	unsigned pos = 0, copied = 0;
	for ( int i = 0; i != 2; ++i ) {
		int src_offset = 0;
		if ( pos < offset ) {
			auto new_pos = std::min( offset, pos + _.decomp->width * rect[ i ].height );
			src_offset = new_pos - pos;
			pos = new_pos;
		}
		int nbytes = std::min( _.decomp->width * rect[ i ].height - src_offset, int( length - copied ) );
		if ( nbytes != 0 ) {
			if ( _.src_pitch == _.decomp->width ) {
				// vm::println( "copy!! to {}", (void *)( dp_dst + copied ) );
				CUDA_DRVAPI_CALL( cuMemcpyAsync( ( CUdeviceptr )( dp_dst + copied ),
												 rect[ i ].src + src_offset, nbytes, _.stream ) );
				copied += nbytes;
			} else {
				int width = _.decomp->width;
				int pitch = _.src_pitch;
				int y_offset = ( src_offset + width - 1 ) / width;
				int y_length = ( src_offset + nbytes ) / width;

				int front = y_offset * width - src_offset;
				int back = src_offset + nbytes - y_length * width;

				if ( front != 0 ) {
					// vm::println( "copy to {}", (void *)( dp_dst + copied ) );
					CUDA_DRVAPI_CALL( cuMemcpyAsync( ( CUdeviceptr )( dp_dst + copied ),
													 rect[ i ].src + src_offset, front, _.stream ) );
					copied += front;
				}
				if ( y_length > y_offset ) {
					CUDA_MEMCPY2D m = {};
					m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
					m.srcPitch = _.src_pitch;
					m.dstMemoryType = dst.device_id().is_device() ? CU_MEMORYTYPE_DEVICE : CU_MEMORYTYPE_HOST;
					m.dstPitch = m.WidthInBytes = width;

					m.srcDevice = rect[ i ].src + y_offset * pitch;
					m.dstDevice = ( CUdeviceptr )( m.dstHost = dp_dst + copied );
					m.Height = y_length - y_offset;

					// vm::println( "copy to {}", (void *)( dp_dst + copied ) );
					CUDA_DRVAPI_CALL( cuMemcpy2DAsync( &m, _.stream ) );  //ck
					copied += m.WidthInBytes * m.Height;
				}
				if ( back != 0 ) {
					// vm::println( "copy to {}", (void *)( dp_dst + copied ) );
					CUDA_DRVAPI_CALL( cuMemcpyAsync( ( CUdeviceptr )( dp_dst + copied ),
													 y_length * width, back, _.stream ) );
					copied += back;
				}
			}
		}
	}
	CUDA_DRVAPI_CALL( cuStreamSynchronize( _.stream ) );
	// vm::println( "copied {} -> buffer {}", copied, (void *)dp_dst );
	// if ( copied > length ) {
	// 	throw std::logic_error( vm::fmt( "copied {} > length {}", copied, length ) );
	// }
}

void VideoDecompressorImpl::decompress( Reader &reader, Consumer const &consumer )
{
	packet_id = 0;
	this->consumer.with(
	  consumer,
	  [&] {
		  uint32_t frame_len;
		  while ( reader.read( reinterpret_cast<char *>( &frame_len ), sizeof( uint32_t ) ) ) {
			  auto packet = get_packet( frame_len );
			  reader.read( reinterpret_cast<char *>( packet ), frame_len );
			  //   vm::println( "#dec_src: { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} { >#x2} ...", int( packet[ 0 ] ), int( packet[ 1 ] ), int( packet[ 2 ] ),
			  // 			   int( packet[ 3 ] ), int( packet[ 4 ] ), int( packet[ 5 ] ), int( packet[ 6 ] ),
			  // 			   int( packet[ 7 ] ), int( packet[ 8 ] ), int( packet[ 9 ] ) );
			  decode_and_advance( packet, frame_len );
		  }
		  decode_and_advance( nullptr, 0 );
		  //   vm::println( "io_queue_size = {}, slots = {}, this = {}", io_queue_size, slots.get(), this );
		  if ( slots != nullptr ) {
			  //   vm::println( "slots != nullptr" );
			  for ( int i = 0; i != io_queue_size; ++i ) {
				  slots[ i ].unmap( decoder );
			  }
		  }
	  } );
}

void VideoDecompressorImpl::decode_and_advance( uint8_t *data, int len, uint32_t flags )
{
	CUVIDSOURCEDATAPACKET packet = {};
	packet.payload = data;
	packet.payload_size = len;
	packet.flags = flags;
	if ( !data || len == 0 ) {
		packet.flags |= CUVID_PKT_ENDOFSTREAM;
	}
	NVDEC_API_CALL( cuvidParseVideoData( parser, &packet ) );
}

int VideoDecompressorImpl::handle_picture_display( CUVIDPARSERDISPINFO *info )
{
	const auto pid = info->picture_index;

	CUVIDPROCPARAMS params = {};
	params.progressive_frame = info->progressive_frame;
	params.second_field = info->repeat_first_field + 1;
	params.top_field_first = info->top_field_first;
	params.unpaired_field = info->repeat_first_field < 0;
	params.output_stream = slots[ pid ].stream;

	CUdeviceptr dp_src = 0;
	unsigned int src_pitch = 0;

	// vm::println( "pid = {}", pid );
	slots[ pid ].unmap( decoder );
	auto evt = slots[ pid ].map( decoder, pid, &dp_src, &src_pitch, &params );
	// vm::println( "map {}", dp_src );

	CUVIDGETDECODESTATUS stat = {};
	CUresult result = cuvidGetDecodeStatus( decoder, pid, &stat );
	if ( result == CUDA_SUCCESS &&
		 ( stat.decodeStatus == cuvidDecodeStatus_Error ||
		   stat.decodeStatus == cuvidDecodeStatus_Error_Concealed ) ) {
		vm::println( "decode error occurred" );
	}

	CUDA_DRVAPI_CALL( cuCtxPushCurrent( ctx ) );  //ck

	{
		VideoStreamPacketImpl packet_impl{ dp_src, src_pitch, slots[ pid ].stream, this };
		VideoStreamPacket packet( packet_impl );
		packet.length = width * ( luma_height + chroma_height );
		packet.id = packet_id++;
		packet.release_event = evt;
		( *consumer )( packet );
	}

	CUDA_DRVAPI_CALL( cuCtxPopCurrent( nullptr ) );  //ck

	return 1;
}

int VideoDecompressorImpl::handle_video_sequence( CUVIDEOFORMAT *format )
{
	if ( decoder ) return io_queue_size;

	if ( format->min_num_decode_surfaces > io_queue_size ) {
		io_queue_size = format->min_num_decode_surfaces;
		vm::println( "min_num_decode_surfaces > io_queue_size, resize to {}", io_queue_size );
	}

	CUDA_DRVAPI_CALL( cuCtxPushCurrent( ctx ) );
	slots.reset( new VideoStreamPacketMapSlot[ io_queue_size ] );
	CUDA_DRVAPI_CALL( cuCtxPopCurrent( nullptr ) );

	CUVIDDECODECAPS caps = {};
	caps.eCodecType = format->codec;
	caps.eChromaFormat = format->chroma_format;
	caps.nBitDepthMinus8 = format->bit_depth_luma_minus8;
	CUDA_DRVAPI_CALL( cuCtxPushCurrent( ctx ) );
	NVDEC_API_CALL( cuvidGetDecoderCaps( &caps ) );
	CUDA_DRVAPI_CALL( cuCtxPopCurrent( nullptr ) );

	if ( !caps.bIsSupported ) {
		throw std::runtime_error( "current codec is not supported on this GPU" );
	}

	if ( format->coded_width > caps.nMaxWidth ||
		 format->coded_height > caps.nMaxHeight ) {
		throw std::runtime_error( "unsupported resolution" );
		// vm::fmt("{}: {}x{}\n") );
	}

	if ( ( format->coded_width >> 4 ) * ( format->coded_height >> 4 ) > caps.nMaxMBCount ) {
		throw std::runtime_error( "unsupported mbcount" );
	}

	CUVIDDECODECREATEINFO info = {};
	info.CodecType = format->codec;
	info.ChromaFormat = format->chroma_format;
	if ( format->chroma_format == cudaVideoChromaFormat_420 ) {
		if ( format->bit_depth_luma_minus8 ) {
			info.OutputFormat = cudaVideoSurfaceFormat_P016;
		} else {
			info.OutputFormat = cudaVideoSurfaceFormat_NV12;
		}
	} else if ( format->chroma_format == cudaVideoChromaFormat_444 ) {
		if ( format->bit_depth_luma_minus8 ) {
			info.OutputFormat = cudaVideoSurfaceFormat_YUV444_16Bit;
		} else {
			info.OutputFormat = cudaVideoSurfaceFormat_YUV444;
		}
	} else {
		throw std::runtime_error( "unsupported chroma format" );
	}
	info.bitDepthMinus8 = format->bit_depth_luma_minus8;
	if ( format->progressive_sequence ) {
		info.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;
	} else {
		info.DeinterlaceMode = cudaVideoDeinterlaceMode_Adaptive;
	}
	info.ulCreationFlags = cudaVideoCreate_PreferCUVID;
	info.ulNumOutputSurfaces = info.ulNumDecodeSurfaces = io_queue_size; /* <---- */
	info.vidLock = ctxlock;

	surface_width = info.ulWidth = info.ulTargetWidth = format->coded_width;
	surface_height = info.ulHeight = info.ulTargetHeight = format->coded_height;

	width = format->display_area.right - format->display_area.left;
	luma_height = format->display_area.bottom - format->display_area.top;
	chroma_height = luma_height * [&] {
		switch ( format->chroma_format ) {
		case cudaVideoChromaFormat_Monochrome: return 0.0;
		default: return 0.5;
		case cudaVideoChromaFormat_422: return 1.0;
		case cudaVideoChromaFormat_444: return 1.0;
		}
	}();
	chroma_plains = [&] {
		switch ( format->chroma_format ) {
		case cudaVideoChromaFormat_Monochrome: return 0;
		default: return 1;
		case cudaVideoChromaFormat_444: return 2;
		}
	}();

	CUDA_DRVAPI_CALL( cuCtxPushCurrent( ctx ) );
	NVDEC_API_CALL( cuvidCreateDecoder( &decoder, &info ) );
	CUDA_DRVAPI_CALL( cuCtxPopCurrent( nullptr ) );

	return io_queue_size;
}

int VideoDecompressorImpl::handle_picture_decode( CUVIDPICPARAMS *params )
{
	NVDEC_API_CALL( cuvidDecodePicture( decoder, params ) );
	return 1;
}

VideoDecompressor::VideoDecompressor( VideoDecompressOptions const &opts ) :
  _( new VideoDecompressorImpl( opts ) )
{
}
VideoDecompressor::~VideoDecompressor()
{
}
void VideoDecompressor::decompress( Reader &reader,
									std::function<void( VideoStreamPacket const & )> const &consumer )
{
	_->decompress( reader, consumer );
}

VM_END_MODULE()
