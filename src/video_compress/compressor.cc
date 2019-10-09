#include <vocomp/video/compressor.hpp>

#ifdef VOCOMP_ENABLE_CUDA
#include <vocomp/video/devices/cuda_encoder.hpp>
#endif
#if defined( WIN32 ) && defined( VOCOMP_ENABLE_D3D9 )
#include <vocomp/video/devices/d3d9_encoder.hpp>
#endif
#if defined( __linux__ ) && defined( VOCOMP_ENABLE_GL )
#include <vocomp/video/devices/gl_encoder.hpp>
#endif

namespace vol
{
VM_BEGIN_MODULE( video )

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
	  _( [&]() -> Encoder * {
		  switch ( _.device ) {
		  case CompressDevice::Cuda:
#ifdef VOCOMP_ENABLE_CUDA
			  return new CudaEncoder( _.width, _.height, _.pixel_format );
#else
			  throw std::runtime_error( "current compressor is compiled without cuda support." );
#endif
		  default:
		  case CompressDevice::Graphics:
#if defined( WIN32 ) && defined( VOCOMP_ENABLE_D3D9 )
			  return new D3D9Encoder( _.width, _.height, _.pixel_format );
#elif defined( __linux__ ) && defined( VOCOMP_ENABLE_GL )
			  return new GLEncoder( _.width, _.height, _.pixel_format );
#else
			  throw std::runtime_error( "no supported compression device" );
#endif
		  }
	  }() )
	{
		this->_->init( _.encode_method, _.encode_preset );
	}
}

VM_END_MODULE()

}  // namespace vol
