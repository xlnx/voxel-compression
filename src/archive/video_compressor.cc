#include <numeric>
#include <thread>
#include <condition_variable>
#include <varch/utils/linked_reader.hpp>
#include <varch/utils/padded_reader.hpp>
#include <varch/utils/filter_reader.hpp>
#include <varch/utils/self_owned_reader.hpp>
#include "backends/nvenc/nvencoder_wrapper.hpp"
#ifdef VARCH_OPENH264_CODEC
#include "backends/openh264/isvc_encoder_wrapper.hpp"
#endif
#include "video_compressor.hpp"

VM_BEGIN_MODULE( vol )

using namespace std;

struct VideoCompressorImpl
{
	VideoCompressorImpl( Writer &out, EncodeOptions const &opts ) :
	  out( out )
	{
		static mutex mut;
		unique_lock<mutex> lk( mut );
		switch ( opts.device ) {
		case ComputeDevice::Cuda:
			encoder.reset( new NvEncoderWrapper( opts ) );
			break;
		case ComputeDevice::Cpu:
		CPU:
#ifdef VARCH_OPENH264_CODEC
			encoder.reset( new IsvcEncoderWrapper( opts ) );
#else
			throw std::logic_error( "please recompile with openh264 codec support" );
#endif
			break;
		default:
			try {
				encoder.reset( new NvEncoderWrapper( opts ) );
			} catch ( std::exception &e ) {
				goto CPU;
			}
		}
		nframe_batch = opts.batch_frames;
		frame_size = encoder->frame_size();
		worker.reset( new thread( [this] { work_loop(); } ) );
	}

	~VideoCompressorImpl()
	{
		should_stop = true;
		{
			unique_lock<mutex> input_lk( input_mut );
			input_cv.notify_one();
		}
		worker->join();
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
				// vm::println( "acquired {} readers", readers.size() );
				input_readers.swap( readers );
				auto nframes = total_size / frame_size;
				nframes_size = nframes * frame_size;
				size_t len = 0;
				for ( auto &reader : input_readers ) {
					// vm::println( "{}", reader.get() );
					len += reader->size() - reader->tell();
					if ( len > nframes_size ) {
						readers.emplace_back( reader );
						// vm::println( "!!{}", readers.back().get() );
						// vm::println( "!!{}", readers.size() );
					}
				}
				// vm::println( "saved {} readers", readers.size() );
				total_size -= nframes_size;
				emitted_frames += nframes;
			}
			{
				unique_lock<mutex> work_lk( work_mut );
				if ( input_readers.size() ) {
					auto offset = input_readers[ 0 ]->tell();
					auto linked_reader = LinkedReader( input_readers );
					auto part_reader = PartReader( linked_reader, offset, nframes_size );
					part_reader.seek( 0 );
					vector<uint32_t> frame_len;
					// vm::println( "encode with {} blocks", input_readers.size() );
					this->encoder->encode( part_reader, out, frame_len );
					for ( auto &len : frame_len ) {
						frame_offset.emplace_back( frame_offset.back() + len );
					}
				}
				finish_cv.notify_one();
			}
		}
	}

	// std::future<>
	BlockIndex accept( vm::Arc<Reader> const &reader )
	{
		unique_lock<mutex> input_lk( input_mut );
		BlockIndex idx;
		idx.first_frame = emitted_frames + total_size / frame_size;
		idx.offset = total_size % frame_size;
		total_size += reader->size();
		idx.last_frame = emitted_frames + ( total_size + frame_size - 1 ) / frame_size - 1;
		// vm::print( "{} ", make_tuple( idx.first_frame, idx.last_frame, idx.offset ) );
		readers.emplace_back( reader );
		// vm::println( "@{} {} {}", readers.size(), readers[ 0 ].get(), reader.get() );
		if ( total_size >= frame_size * nframe_batch ) {
			// vm::println( "notified {} / {}", total_size, frame_size * nframe_batch );
			input_cv.notify_one();
		}
		return idx;
	}

	// make data in all readers owned by this VideoCompressor
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
			// vm::println( "flushed {} / {}", total_size, frame_size * nframe_batch );
			input_cv.notify_one();
		}
		input_mut.unlock();
		if ( wait ) {
			finish_cv.wait( work_lk );
		}
		// vm::println( "{}", frame_offset );
	}

	void wait()
	{
		input_mut.lock();
		if ( readers.size() ) {
			auto nframes_padded = ( total_size + frame_size - 1 ) / frame_size;
			auto padded_size = nframes_padded * frame_size;
			if ( padded_size != total_size ) {
				readers.emplace_back( new FilterReader( padded_size - total_size ) );
				// vm::println( "@{}", readers.size() );
				total_size = padded_size;
			}
		}
		should_flush = true;
		// vm::println( "wait" );
		input_cv.notify_one();
		unique_lock<mutex> work_lk( work_mut );
		input_mut.unlock();
		finish_cv.wait( work_lk );
	}

public:
	// EncodeOptions opts;
	Writer &out;
	shared_ptr<IEncoder> encoder;
	vector<vm::Arc<Reader>> readers;
	size_t total_size = 0;
	size_t frame_size, nframe_batch;
	size_t emitted_frames = 0;
	vector<uint64_t> frame_offset = { 0 };
	bool should_stop = false, should_flush = false;

	mutex input_mut, work_mut;
	condition_variable finish_cv, input_cv;
	unique_ptr<thread> worker;
};

VideoCompressor::VideoCompressor( Writer &out, EncodeOptions const &opts ) :
  _( new VideoCompressorImpl( out, opts ) )
{
}

VideoCompressor::~VideoCompressor()
{
}

BlockIndex VideoCompressor::accept( vm::Arc<Reader> &&reader )
{
	return _->accept( std::move( reader ) );
}
void VideoCompressor::flush( bool wait )
{
	_->flush( wait );
}
void VideoCompressor::wait()
{
	_->wait();
}
uint32_t VideoCompressor::frame_size() const
{
	return _->frame_size;
}
vector<uint64_t> const &VideoCompressor::frame_offset() const
{
	return _->frame_offset;
}

VM_END_MODULE()
