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

	void transfer( Reader &reader, Writer &writer )
	{
		_->_->CreateEncoder( &_->params );
		{
			_->encode( reader, writer );
		}
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
		_->transfer( reader, writer );
	}
}

VM_END_MODULE()

}  // namespace vol
