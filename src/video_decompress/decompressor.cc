#include <vocomp/video/decompressor.hpp>

#include <atomic>
#include <cudafx/driver/context.hpp>
#include <cudafx/memory.hpp>
#include <cudafx/transfer.hpp>
#include <cudafx/array.hpp>
#include <VMUtils/with.hpp>
#include <koi.hpp>
#include <nvcodec/nvcuvid.h>
#include <nvcodec/NvCodecUtils.h>
// #include "nvdec/NvDecoder.h"

namespace vol
{
VM_BEGIN_MODULE( video )

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

struct DecompressorImpl final : vm::NoCopy, vm::NoMove
{
	std::vector<uint32_t> &decode_header( Reader &reader )
	{
		uint32_t nframes;
		reader.read( reinterpret_cast<char *>( &nframes ), sizeof( nframes ) );
		thread_local std::vector<uint32_t> frame_len;
		frame_len.resize( nframes + 1 );
		reader.read( reinterpret_cast<char *>( frame_len.data() ),
					 sizeof( uint32_t ) * nframes );
		frame_len[ nframes ] = 0;
		return frame_len;
	}

	char *get_packet( uint32_t len )
	{
		thread_local auto _ = std::vector<char>( 1024 );
		if ( len > _.size() ) {
			_.resize( len );
		}
		return _.data();
	}

	void decompress( Reader &reader, Writer &writer )
	{
		// dec->on_picture_display.with(
		//   on_picture_display,
		//   [&] {
		auto &frame_len = decode_header( reader );
		for ( auto &len : frame_len ) {
			uint8_t **ppframe;
			int nframedec;

			auto packet = get_packet( len );
			reader.read( packet, len );

			throw std::runtime_error( "unimplemented" );
			// decode()
			// dec->Decode( reinterpret_cast<uint8_t *>( packet ), len, &ppframe, &nframedec );

			// for ( int i = 0; i < nframedec; i++ ) {
			// 	writer.write( reinterpret_cast<char *>( ppframe[ i ] ), dec->GetFrameSize() );
			// }
		}
		// } );
	}

	void decompress( Reader &reader, cufx::MemoryView1D<unsigned char> const &dst )
	{
		// assert(dst.size() == block_size)
		auto dp_dst = dst.ptr();
		vm::println("is_device: {}", dst.device_id().is_device());
		auto on_picture_display = [&]( CUdeviceptr dp_src, unsigned src_pitch, CUstream stream ) {
			vm::println( "picture display {} {}", (void*)dp_src, (void*)dp_dst );

			CUDA_MEMCPY2D m = {};
			m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
			m.srcPitch = src_pitch;
			m.dstMemoryType = dst.device_id().is_device() ? CU_MEMORYTYPE_DEVICE : CU_MEMORYTYPE_HOST;
			m.dstPitch = m.WidthInBytes = width;

			m.srcDevice = dp_src;
			m.dstDevice = (CUdeviceptr)(m.dstHost = dp_dst);
			m.Height = luma_height;
			CUDA_DRVAPI_CALL( cuMemcpy2DAsync( &m, stream ) );  //ck
			dp_dst += m.WidthInBytes * m.Height;

			m.srcDevice = dp_src + m.srcPitch * surface_height;
			m.dstDevice = (CUdeviceptr)(m.dstHost = dp_dst);
			m.Height = chroma_height;
			CUDA_DRVAPI_CALL( cuMemcpy2DAsync( &m, stream ) );  //ck
			dp_dst += m.WidthInBytes * m.Height;
		};
		this->on_picture_display.with(
		  on_picture_display, [&] {
			  auto &frame_len = decode_header( reader );
			  for ( auto &len : frame_len ) {
				  auto packet = get_packet( len );
				  reader.read( packet, len );
				  decode_and_advance( packet, len );
				//   dec->Decode( reinterpret_cast<uint8_t *>( packet ), len, nullptr, nullptr );
			  }
			  while ( pending_frames.load() ) {}
			  vm::println("decoded {} frames, {} bytes", frame_len.size() - 1, dp_dst - dst.ptr() );
		  } );
	}

public:
	DecompressorImpl( DecompressorOptions const &opts ):
		io_queue_size( opts.io_queue_size ),
		pending_frames( 0 ),
		stop(false),
		rt_thread( [this] {
			while (!this->stop.load()) {
				this->rt.run();
			}
		})
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
	~DecompressorImpl()
	{
		stop.store(true);
		rt_thread.join();
		if ( decoder ) {
			cuvidDestroyDecoder( decoder );
		}
		if ( parser ) {
			cuvidDestroyVideoParser( parser );
		}
		cuvidCtxLockDestroy( ctxlock );
	}

private:
	struct Slot: vm::NoCopy, vm::NoMove
	{
		Slot():
			ready( true )
		{
			CUDA_DRVAPI_CALL( cuStreamCreate( &stream, CU_STREAM_NON_BLOCKING ) );
		}
		~Slot()
		{
			cuStreamDestroy( stream );
		}

		CUstream stream;
		std::atomic<bool> ready;
	};

	void decode_and_advance( char *data, int len, uint32_t flags = 0 )
	{
		CUVIDSOURCEDATAPACKET packet = {};
		packet.payload = reinterpret_cast<unsigned char*>( data );
		packet.payload_size = len;
		packet.flags = flags;
		if ( !data || len == 0 ) {
			packet.flags |= CUVID_PKT_ENDOFSTREAM;
		}
		NVDEC_API_CALL( cuvidParseVideoData( parser, &packet ) );
	}
	int handle_video_sequence( CUVIDEOFORMAT *format )
	{
		if ( decoder ) return io_queue_size;

		if ( format->min_num_decode_surfaces > io_queue_size ) {
			io_queue_size = format->min_num_decode_surfaces;
			vm::println( "min_num_decode_surfaces > io_queue_size, resize to {}", io_queue_size );
		}
		vm::println( "io_queue_size = {}", io_queue_size );

		CUDA_DRVAPI_CALL( cuCtxPushCurrent( ctx ) );
		slots.reset( new Slot[ io_queue_size ] );
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
		info.ulNumOutputSurfaces = info.ulNumDecodeSurfaces = io_queue_size;   /* <---- */
		info.vidLock = ctxlock;

		surface_width = info.ulWidth = info.ulTargetWidth = format->coded_width;
		surface_height = info.ulHeight = info.ulTargetHeight = format->coded_height;

		width = format->display_area.right - format->display_area.left;
		luma_height = format->display_area.bottom - format->display_area.top;
		chroma_height = luma_height * [&]{
			switch ( format->chroma_format ) {
			case cudaVideoChromaFormat_Monochrome: return 0.0;
			default: return 0.5;
			case cudaVideoChromaFormat_422: return 1.0;
			case cudaVideoChromaFormat_444: return 1.0;
			}
		} ();
		chroma_plains = [&] {
			switch ( format->chroma_format ) {
			case cudaVideoChromaFormat_Monochrome: return 0;
			default: return 1;
			case cudaVideoChromaFormat_444: return 2;
			}
		} ();

		CUDA_DRVAPI_CALL( cuCtxPushCurrent( ctx ) );
		NVDEC_API_CALL( cuvidCreateDecoder( &decoder, &info ) );
		CUDA_DRVAPI_CALL( cuCtxPopCurrent( nullptr ) );

		vm::println( "setup decoder" );

		return io_queue_size;
	}
	int handle_picture_decode( CUVIDPICPARAMS *params )
	{
		NVDEC_API_CALL( cuvidDecodePicture( decoder, params ) );
		return 1;
	}
	int handle_picture_display( CUVIDPARSERDISPINFO *info )
	{
		const auto pid = info->picture_index;
		auto stream = slots[ pid ].stream;

		CUVIDPROCPARAMS params = {};
		params.progressive_frame = info->progressive_frame;
		params.second_field = info->repeat_first_field + 1;
		params.top_field_first = info->top_field_first;
		params.unpaired_field = info->repeat_first_field < 0;
		params.output_stream = stream;

		CUdeviceptr dp_src = 0;
		unsigned int src_pitch = 0;

		vm::println("pid = {}", pid);
		++pending_frames;
		bool ready = true;
		while ( !slots[ pid ].ready.compare_exchange_weak( ready, false ) ) {
			ready = true;
		}

		NVDEC_API_CALL( cuvidMapVideoFrame( decoder, pid, &dp_src, &src_pitch, &params ) );

		CUVIDGETDECODESTATUS stat = {};
		CUresult result = cuvidGetDecodeStatus( decoder, pid, &stat );
		if ( result == CUDA_SUCCESS && 
			( stat.decodeStatus == cuvidDecodeStatus_Error || 
			  stat.decodeStatus == cuvidDecodeStatus_Error_Concealed ) ) {
			vm::println( "decode error occurred" );
		}

		cuCtxPushCurrent( ctx );  //ck
		(*on_picture_display)( dp_src, src_pitch, stream );
		cuCtxPopCurrent( nullptr );  //ck
		rt.spawn(
			koi::future::poll_fn<void>(
				[this, pid, dp_src, stream](auto &_) -> koi::future::PollState {
					switch ( auto res = cuStreamQuery( stream ) ) {
					case CUDA_ERROR_NOT_READY: return koi::future::PollState::Pending;
					default: 
						NVDEC_API_CALL( cuvidUnmapVideoFrame( this->decoder, dp_src ) );
						this->slots[ pid ].ready.store( true );
						--pending_frames;
						return CUDA_SUCCESS == res ? koi::future::PollState::Ok : koi::future::PollState::Pruned;
					}
				}
			)
		 );
		return 1;
	}

private:
	static int CUDAAPI handle_video_sequence_proc( void *self, CUVIDEOFORMAT *params ) 
	{ 
		return reinterpret_cast<DecompressorImpl*>( self )->handle_video_sequence( params ); 
	}
	static int CUDAAPI handle_picture_decode_proc( void *self, CUVIDPICPARAMS *params )
	{ 
		return reinterpret_cast<DecompressorImpl*>( self )->handle_picture_decode( params ); 
	}
	static int CUDAAPI handle_picture_display_proc( void *self, CUVIDPARSERDISPINFO *params )
	{ 
		return reinterpret_cast<DecompressorImpl*>( self )->handle_picture_display( params ); 
	}

private:
	unsigned io_queue_size;

	int surface_width;
	int surface_height;

	int width;
	int luma_height;
	int chroma_height;
	int chroma_plains;

	vm::With<std::function<void( CUdeviceptr, unsigned, CUstream )>> on_picture_display;
	std::atomic<int> pending_frames;
	std::unique_ptr<Slot[]> slots;
	
	koi::runtime::current_thread::Runtime rt;
	std::atomic<bool> stop;
	std::thread rt_thread;

private:
	cufx::drv::Context ctx = 0;
	// std::unique_ptr<NvDecoder> dec;
	CUvideoctxlock ctxlock = nullptr;
	CUvideoparser parser = nullptr;
	CUvideodecoder decoder = nullptr;
};

VM_EXPORT
{
	Decompressor::Decompressor( DecompressorOptions const &opts ) :
	  _( new DecompressorImpl( opts ) )
	{
	}
	Decompressor::~Decompressor()
	{
	}
	void Decompressor::decompress( Reader & reader, Writer & writer )
	{
		_->decompress( reader, writer );
	}

	void Decompressor::decompress( Reader & reader, cufx::MemoryView1D<unsigned char> const &dst )
	{
		return _->decompress( reader, dst );
	}
}

VM_END_MODULE()

}  // namespace vol
