#include <numeric>
#include <thread>
#include <condition_variable>
#include <vocomp/video/compressor.hpp>
#include <koi.hpp>

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

using namespace std;

struct LinkedReader : Reader
{
	LinkedReader( vector<vm::Box<Reader>> &&readers ) :
	  _( std::move( readers ) )
	{
	}

	void seek( size_t pos ) override
	{
	}
	size_t tell() const override
	{
		return _[ idx ]->tell() +
			   std::accumulate(
				 _.begin(), _.begin() + idx, 0,
				 []( size_t len, vm::Box<Reader> const &reader ) { return len + reader->size(); } );
	}
	size_t size() const override
	{
		return std::accumulate(
		  _.begin(), _.end(), 0,
		  []( size_t len, vm::Box<Reader> const &reader ) { return len + reader->size(); } );
	}
	size_t read( char *dst, size_t dlen ) override
	{
		size_t len = 0;
		while ( true ) {
			len += _[ idx ]->read( dst + len, dlen );
			if ( len < dlen && idx < _.size() ) {
				if ( ++idx == _.size() ) {
					break;
				}
			} else {
				break;
			}
		}
		return len;
	}

private:
	vector<vm::Box<Reader>> _;
	size_t idx = 0;
};

struct CompressorImpl
{
	CompressorImpl( Writer &out, CompressOptions const &opts ) :
	  //   opts( opts ),
	  out( out ),
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
	  }() ),
	  worker( [this, &out]() {
		  while ( !should_stop ) {
			  vector<vm::Box<Reader>> input_readers;
			  {
				  unique_lock<mutex> _( mut );
				  cv.wait( _, [this] { return should_stop || !readers.empty(); } );
				  if ( should_stop ) return;
				  input_readers.swap( readers );
			  }
			  auto linked_reader = LinkedReader( std::move( input_readers ) );
			  _->encode( linked_reader, out, frame_len );
		  }
	  } )
	{
		_->_->CreateDefaultEncoderParams( &_->params,
										  *into_nv_encode( opts.encode_method ),
										  *into_nv_preset( opts.encode_preset ) );
		_->_->CreateEncoder( &_->params );
		_->_->Allocate();
	}

	// std::future<>
	void accept( vm::Box<Reader> &&reader )
	{
		unique_lock<mutex> _( mut );
		readers.emplace_back( std::move( reader ) );
		cv.notify_one();
	}

	void wait()
	{
		if ( readers.size() ) {
		}
	}

	~CompressorImpl()
	{
		_->_->Deallocate();
		_->_->DestroyEncoder();
		should_stop = true;
		cv.notify_one();
		worker.join();
	}

	// CompressOptions opts;
	Writer &out;
	vm::Box<Encoder> _;
	vector<vm::Box<Reader>> readers;
	vector<uint32_t> frame_len;
	bool should_stop = false;
	condition_variable cv;
	mutex mut;
	thread worker;
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

	Compressor::Compressor( Writer & out, CompressOptions const &opts ) :
	  _( new CompressorImpl( out, opts ) )
	{
	}

	Compressor::~Compressor()
	{
	}

	void Compressor::accept( vm::Box<Reader> && reader )
	{
		_->accept( std::move( reader ) );
	}
	void Compressor::wait()
	{
		_->wait();
	}
}

VM_END_MODULE()

}  // namespace vol
