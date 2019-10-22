#include <vocomp/video/compressor.hpp>

#ifdef VOCOMP_ENABLE_CUDA
#include "devices/cuda_encoder.hpp"
#endif
#if defined( WIN32 ) && defined( VOCOMP_ENABLE_D3D9 )
#include "devices/d3d9_encoder.hpp"
#endif
#if defined( __linux__ ) && defined( VOCOMP_ENABLE_GL )
#include "devices/gl_encoder.hpp"
#endif

namespace vol
{
VM_BEGIN_MODULE( video )

struct CompressorImpl
{
	CompressorImpl( CompressOptions const &opts ) :
	  opts( opts ),
	  _( [&]() -> Encoder * {
		  switch ( opts.device ) {
		  case CompressDevice::Cuda:
#ifdef VOCOMP_ENABLE_CUDA
			  return new CudaEncoder( opts.width, opts.height, opts.pixel_format );
#else
			  throw std::runtime_error( "current compressor is compiled without cuda support." );
#endif
		  default:
		  case CompressDevice::Graphics:
#if defined( WIN32 ) && defined( VOCOMP_ENABLE_D3D9 )
			  return new D3D9Encoder( opts.width, opts.height, opts.pixel_format );
#elif defined( __linux__ ) && defined( VOCOMP_ENABLE_GL )
			  return new GLEncoder( opts.width, opts.height, opts.pixel_format );
#else
			  throw std::runtime_error( "no supported compression device" );
#endif
		  }
	  }() )
	{
		_->_->CreateDefaultEncoderParams( &_->params,
										  *into_nv_encode( opts.encode_method ),
										  *into_nv_preset( opts.encode_preset ) );
		_->_->CreateEncoder( &_->params );
		_->_->Allocate();
	}

	uint32_t transfer( Reader &reader, Writer &writer )
	{
		_->_->CreateEncoder( &_->params );

		thread_local std::vector<char> frames;
		thread_local std::vector<uint32_t> frame_len;
		frames.clear();
		frame_len.clear();

		_->encode( reader, frames, frame_len );

		uint32_t nframes = frame_len.size();
		writer.write( reinterpret_cast<char *>( &nframes ), sizeof( uint32_t ) );
		writer.write( reinterpret_cast<char *>( frame_len.data() ),
					  sizeof( uint32_t ) * nframes );
		writer.write( frames.data(), frames.size() );

		return nframes;
	}

	~CompressorImpl()
	{
		_->_->Deallocate();
		_->_->DestroyEncoder();
	}

	CompressOptions opts;
	vm::Box<Encoder> _;
};

VM_EXPORT
{
	CompressOptions::CompressOptions() :
	  device(
#ifdef VOCOMP_ENABLE_CUDA
		CompressDevice::Cuda
#else
		CompressDevice::Graphics
#endif
	  )
	{
	}

	Compressor::Compressor( CompressOptions const &_ ) :
	  _( new CompressorImpl( _ ) )
	{
	}

	Compressor::~Compressor()
	{
	}

	void Compressor::transfer( Reader & reader, Writer & writer )
	{
		this->nframes = _->transfer( reader, writer );
	}
}

VM_END_MODULE()

}  // namespace vol
