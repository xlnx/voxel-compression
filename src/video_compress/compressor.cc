#include <numeric>
#include <thread>
#include <condition_variable>
#include <vocomp/video/compressor.hpp>
#include "../utils/linked_reader.hpp"
#include "../utils/padded_reader.hpp"
#include "../utils/self_owned_reader.hpp"

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
	  }() )
	{
		_->_->CreateDefaultEncoderParams( &_->params,
										  *into_nv_encode( opts.encode_method ),
										  *into_nv_preset( opts.encode_preset ) );
		_->_->CreateEncoder( &_->params );
		_->_->Allocate();
		nframe_batch = opts.batch_frames;
		frame_size = _->_->GetFrameSize();
		worker.reset( new thread( [this] { work_loop(); } ) );
	}

	void work_loop()
	{
		while ( !should_stop ) {
			vector<vm::Arc<Reader>> input_readers;
			size_t nframes_size = 0;
			{
				unique_lock<mutex> input_lk( input_mut );
				input_cv.wait(
				  input_lk,
				  [this] { return should_stop ||
								  should_flush ||
								  total_size >= frame_size * nframe_batch; } );
				if ( should_stop ) return;
				if ( should_flush ) {
					should_flush = false;
				}
				input_readers.swap( readers );
				nframes_size = total_size / frame_size * frame_size;
				size_t len = 0;
				for ( auto &reader : input_readers ) {
					len += reader->size() - reader->tell();
					if ( len > nframes_size ) {
						readers.emplace_back( reader );
					}
				}
				total_size -= nframes_size;
			}
			{
				unique_lock<mutex> work_lk( work_mut );
				if ( input_readers.size() ) {
					auto linked_reader = LinkedReader( input_readers );
					auto part_reader = PartReader( linked_reader, 0, nframes_size );
					this->_->encode( part_reader, out, frame_len );
				}
				finish_cv.notify_one();
			}
		}
	}

	// std::future<>
	void accept( vm::Arc<Reader> &&reader )
	{
		unique_lock<mutex> input_lk( input_mut );
		total_size += reader->size();
		readers.emplace_back( std::move( reader ) );
		if ( total_size >= frame_size * nframe_batch ) {
			input_cv.notify_one();
		}
	}

	// make data in all readers owned by this compressor
	void flush( bool wait = false )
	{
		input_mut.lock();
		unique_lock<mutex> work_lk( work_mut );
		vector<vm::Arc<Reader>> copied_readers;
		if ( readers.size() ) {
			for ( auto &reader : readers ) {
				copied_readers.emplace_back( new SelfOwnedReader( *reader ) );
			}
			copied_readers.swap( readers );
		}
		if ( wait ) {
			should_flush = true;
			input_cv.notify_one();
		}
		input_mut.unlock();
		if ( wait ) {
			finish_cv.wait( work_lk );
		}
	}

	void wait()
	{
		input_mut.lock();
		if ( readers.size() ) {
			auto nframes_padded = ( total_size + frame_size - 1 ) / frame_size;
			auto padded_size = nframes_padded * frame_size;
			if ( padded_size != total_size ) {
				struct Padding : PaddedReader
				{
					Padding( size_t size ) :
					  PaddedReader( [this]() -> Reader & {
						  reader = unique_ptr<Reader>( new SliceReader( nullptr, 0 ) );
						  return *reader;
					  }(),
									size ) {}

				private:
					unique_ptr<Reader> reader;
				};
				readers.emplace_back( new Padding( padded_size - total_size ) );
				total_size = padded_size;
			}
		}
		should_flush = true;
		input_cv.notify_one();
		unique_lock<mutex> work_lk( work_mut );
		input_mut.unlock();
		finish_cv.wait( work_lk );
	}

	~CompressorImpl()
	{
		_->_->Deallocate();
		_->_->DestroyEncoder();
		should_stop = true;
		{
			unique_lock<mutex> input_lk( input_mut );
			input_cv.notify_one();
		}
		worker->join();
	}

	// CompressOptions opts;
	Writer &out;
	vm::Box<Encoder> _;
	vector<vm::Arc<Reader>> readers;
	size_t total_size = 0;
	size_t frame_size, nframe_batch;
	vector<uint32_t> frame_len;
	bool should_stop = false, should_flush = false;

	mutex input_mut, work_mut;
	condition_variable finish_cv, input_cv;
	unique_ptr<thread> worker;
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

	void Compressor::accept( vm::Arc<Reader> && reader )
	{
		_->accept( std::move( reader ) );
	}
	void Compressor::flush( bool wait )
	{
		_->flush( wait );
	}
	void Compressor::wait()
	{
		_->wait();
	}
	uint32_t Compressor::frame_size() const
	{
		return _->frame_size;
	}
	vector<uint32_t> const &Compressor::frame_len() const
	{
		return _->frame_len;
	}
}

VM_END_MODULE()

}  // namespace vol
