#include <vocomp/refine/refiner.hpp>

#include <VMat/geometry.h>
#include <VMat/numeric.h>
#include <VMUtils/timer.hpp>
#include <VMUtils/compile_spec.hpp>
#include <VMUtils/threadpool.hpp>
#include <VMFoundation/rawreader.h>
#include <vocomp/index/compressor.hpp>

namespace vol
{
VM_BEGIN_MODULE( refine )

using namespace ysl;
using namespace std;

using Voxel = char;

template <typename Voxel>
struct Buffer
{
	unique_ptr<Voxel[]> buffer;
	pair<int, size_t> stride;
	condition_variable cond_notify_read_compute;
	condition_variable cond_notify_write;
	mutex mtx;
	bool ready = false;
	Buffer( size_t nVoxels )
	{
		buffer.reset( new Voxel[ nVoxels * sizeof( Voxel ) ] );
	}
};

struct RefinerImpl final : vm::NoCopy, vm::NoMove
{
private:
	index::Idx raw;
	size_t log_block_size, block_size, block_inner, padding;
	index::Idx dim;
	index::Idx adjusted;

	RawReaderIO input;
	ofstream output;

private:
	int ncols_per_stride, nrows_per_stride, stride_interval;
	int ncols, nrows, nslices;
	size_t nvoxels_per_block;
	size_t nblocks_per_stride;
	size_t nrow_iters;
	size_t buffer_size;
	size_t read_blocks, written_blocks;
	shared_ptr<Buffer<Voxel>> read_buffer, write_buffer;

	vm::ThreadPool pool = thread::hardware_concurrency();

public:
	RefinerImpl( RefinerOptions const &opts ) :
	  raw{ index::Idx{}.set_x( opts.x ).set_y( opts.y ).set_z( opts.z ) },
	  log_block_size( opts.log_block_size ),
	  block_size( 1 << opts.log_block_size ),
	  block_inner( block_size - 2 * opts.padding ),
	  padding( opts.padding ),
	  dim( index::Idx{}
			 .set_x( RoundUpDivide( raw.x, block_inner ) )
			 .set_y( RoundUpDivide( raw.y, block_inner ) )
			 .set_z( RoundUpDivide( raw.z, block_inner ) ) ),
	  adjusted( index::Idx{}
				  .set_x( dim.x * block_size )
				  .set_y( dim.y * block_size )
				  .set_z( dim.z * block_size ) ),
	  input( opts.input, Size3( raw.x, raw.y, raw.z ), sizeof( Voxel ) ),
	  output( opts.output, ios::binary ),
	  nvoxels_per_block( block_size * block_size * block_size )
	{
		ncols = dim.x;
		nrows = dim.y;
		nslices = dim.z;

		if ( padding < 0 || padding > 2 ) {
			cout << "Unsupported padding\n";
			return;
		}

		vm::println( "raw: {}", raw );
		vm::println( "block_size: {}", block_size );
		vm::println( "block_inner: {}", block_inner );
		vm::println( "padding: {}", padding );
		vm::println( "dim: {}", dim );
		vm::println( "adjusted: {}", adjusted );

		if ( not output.is_open() ) {
			throw runtime_error( "can not open lvd file" );
		}
	}
	~RefinerImpl()
	{
		auto header = Header{}
						.set_log_block_size( log_block_size )
						.set_block_size( block_size )
						.set_block_inner( block_inner )
						.set_padding( padding );
		header.raw
		  .set_x( raw.x )
		  .set_y( raw.y )
		  .set_z( raw.z );
		header.dim
		  .set_x( dim.x )
		  .set_y( dim.y )
		  .set_z( dim.z );
		header.adjusted
		  .set_x( adjusted.x )
		  .set_y( adjusted.y )
		  .set_z( adjusted.z );
		StreamWriter writer( output, 0, sizeof( Header ) );
		header.write_to( writer );
	}

	VM_NO_INLINE bool convert( ConvertOptions const &opts )
	{
		auto pipe = opts.pipe;
		if ( not pipe ) {
			pipe = make_shared<vol::Copy>();
		}

		vol::UnboundedStreamWriter writer( output, sizeof( Header ) );
		index::Compressor<> comp( index::Idx{}
									.set_x( block_size )
									.set_y( block_size )
									.set_z( block_size ),
								  writer, *pipe );

		{
			size_t gb_to_bytes = size_t( 1024 ) /*Mb*/ * 1024 /*Kb*/ * 1024 /*Bytes*/;
			size_t block_size_in_bytes = sizeof( Voxel ) * nvoxels_per_block;
			size_t mem_size_in_bytes = opts.suggest_mem_gb * gb_to_bytes;
			int nblocks_in_mem = mem_size_in_bytes / block_size_in_bytes / 2 /*two buffers*/;
			if ( not nblocks_in_mem ) {
				throw runtime_error( "total memory < block size" );
			}
			// const int maxBlocksPerStride = 2;
			// nblocks_in_mem = min( nblocks_in_mem, maxBlocksPerStride );
			nrows_per_stride = min( nblocks_in_mem / ncols, nrows );
			ncols_per_stride = ncols;
			stride_interval = 1;
			if ( not nrows_per_stride ) { /*nblocks_in_mem < nBlocksPerRow*/
				nrows_per_stride = 1;
				ncols_per_stride = nblocks_in_mem;
				stride_interval = ysl::RoundUpDivide( ncols, ncols_per_stride );
			}
		}
		nblocks_per_stride = ncols_per_stride * nrows_per_stride;
		vm::println( "stride size: {} x {} = {} block(s)",
					 ncols_per_stride, nrows_per_stride, nblocks_per_stride );

		nrow_iters = ysl::RoundUpDivide( nrows, nrows_per_stride );

		vm::println( "total strides: {}", nrow_iters * stride_interval * nslices );

		// since read_buffer is no larger than write_buffer
		buffer_size = nvoxels_per_block * nblocks_per_stride;
		vm::println( "allocing buffers: {} byte(s) x 2 = {} Mb",
					 buffer_size, buffer_size / 1024 /*Kb*/ / 1024 /*Mb*/ );

		read_buffer = make_shared<Buffer<Voxel>>(
		  nvoxels_per_block * nblocks_per_stride );
		write_buffer = make_shared<Buffer<Voxel>>(
		  nvoxels_per_block * nblocks_per_stride );

		read_blocks = 0;
		written_blocks = 0;

		{
			vm::Timer::Scoped t( [&]( auto dt ) {
				vm::println( "total convert time: {}", dt.s() );
			} );

			vm::ThreadPool read_thread( 1 );
			vm::ThreadPool write_thread( 1 );

			for ( int slice = 0; slice < nslices; slice++ ) {
				for ( int it = 0; it < nrow_iters; ++it ) {
					for ( int rep = 0; rep < stride_interval; ++rep ) {
						read_thread.AppendTask( [=] { read_task( slice, it, rep ); } );
						write_thread.AppendTask( [=, &opts, &comp] { write_task( opts, comp ); } );
					}
				}
			}
		}

		read_buffer = nullptr;
		write_buffer = nullptr;

		return true;
	}

private:
	VM_NO_INLINE void read_one_batch( Vec3i &region_start, Size3 &region_size )
	{
		input.readRegion(
		  region_start, region_size,
		  reinterpret_cast<unsigned char *>( read_buffer->buffer.get() ) );
	}

	struct ReadTaskParams
	{
		Vec2i stride_start;
		Size2 stride_size;

		Vec3i region_start;
		Size3 region_size;

		Vec3i raw_region_start;
		Size3 raw_region_size;

		size_t dx = 0, dy = 0, dz = 0;
	};

	VM_NO_INLINE void adjust_memory_layout( ReadTaskParams const &_ )
	{
		memset( write_buffer->buffer.get(), 0,
				sizeof( Voxel ) * nvoxels_per_block * nblocks_per_stride );
		auto dst = write_buffer->buffer.get();
		auto src = read_buffer->buffer.get();
		for ( size_t dep = 0; dep < _.region_size.z; ++dep ) {
			auto slice_dst = dst + dep * _.raw_region_size.x * _.raw_region_size.y;
			auto slice_src = src + dep * _.region_size.x * _.region_size.y;
			for ( size_t i = 0; i < _.region_size.y; ++i ) {
				memcpy(
				  slice_dst + _.dz * _.raw_region_size.x * _.raw_region_size.y +
					( i + _.dy ) * _.raw_region_size.x + _.dx, /*x'_i*/
				  slice_src + i * _.region_size.x,			   /*x_i*/
				  _.region_size.x * sizeof( Voxel ) );
			}
		}
		write_buffer->buffer.swap( read_buffer->buffer );
	}

	VM_NO_INLINE void blocklify_one_block( ReadTaskParams const &_, int xb, int yb )
	{
		const int idx = xb + yb * _.stride_size.x;
		const auto dst = write_buffer->buffer.get() +
						 idx * nvoxels_per_block;
		const auto src = read_buffer->buffer.get() +
						 xb * block_inner +
						 yb * block_inner * _.raw_region_size.x;

		for ( size_t dep = 0; dep < block_size; ++dep ) {
			auto slice_dst = dst + dep * block_size * block_size;
			auto slice_src = src + dep * _.raw_region_size.x * _.raw_region_size.y;
			for ( size_t row = 0; row < block_size; ++row ) {
				memcpy(
				  slice_dst + row * block_size,
				  slice_src + row * _.raw_region_size.x,
				  block_size * sizeof( Voxel ) );
			}
		}
		++read_blocks;
	}

	VM_NO_INLINE void blocklify_one_batch( ReadTaskParams const &_ )
	{
		for ( int yb = 0; yb < _.stride_size.y; yb++ ) {
			for ( int xb = 0; xb < _.stride_size.x; xb++ ) {
				pool.AppendTask( [=, &_] { blocklify_one_block( _, xb, yb ); } );
			}
		}
		pool.Wait();
		vm::println( "{} of {} complete", read_blocks, dim.total() );
	}

	VM_NO_INLINE void read_task( int slice, int it, int rep )
	{
		ReadTaskParams _;
		_.stride_start = Vec2i( rep * ncols_per_stride, it * nrows_per_stride );
		_.stride_size = Size2(
		  std::min( ncols - _.stride_start.x, ncols_per_stride ),
		  std::min( nrows - _.stride_start.y, nrows_per_stride ) );

		/* left bottom corner of region, if padding > 0 this coord might be < 0 */
		_.region_start = Vec3i(
		  _.stride_start.x * block_inner - padding,
		  _.stride_start.y * block_inner - padding,
		  slice * block_inner - padding );
		/* whole region size includes padding, might overflow */
		_.region_size = Size3(
		  _.stride_size.x * block_inner + padding * 2,
		  _.stride_size.y * block_inner + padding * 2,
		  block_inner + padding * 2 );
		_.raw_region_start = _.region_start;
		_.raw_region_size = _.region_size;

		/* overflow = bbbbbb -> -x,x,-y,y,-z,z */
		int overflow = 0;
		/* region overflows: x1 > X */
		if ( _.region_size.x + _.region_start.x > raw.x ) {
			_.region_size.x = raw.x - _.region_start.x;
			overflow |= 0b010000;
		}
		/* region overflows: y1 > Y */
		if ( _.region_size.y + _.region_start.y > raw.y ) {
			_.region_size.y = raw.y - _.region_start.y;
			overflow |= 0b000100;
		}
		/* region overflows: z1 > Z */
		if ( _.region_size.z + _.region_start.z > raw.z ) {
			_.region_size.z = raw.z - _.region_start.z;
			overflow |= 0b000001;
		}
		/* region overflows: x0 < 0, must be a padding */
		if ( _.region_start.x < 0 ) {
			_.region_size.x -= padding;
			_.region_start.x = 0;
			overflow |= 0b100000;
		}
		/* region overflows: y0 < 0, must be a padding */
		if ( _.region_start.y < 0 ) {
			_.region_size.y -= padding;
			_.region_start.y = 0;
			overflow |= 0b001000;
		}
		/* region overflows: x0 < 0, must be a padding */
		if ( _.region_start.z < 0 ) {
			_.region_size.z -= padding;
			_.region_start.z = 0;
			overflow |= 0b000010;
		}

		/* let x_i = first voxel of line i
				   x_i = len * i
				   -> x'_i = slen * i + dx? * 1 + slen * dy?*/
		if ( overflow & 0b100000 ) {
			_.dx = padding;
		}
		if ( overflow & 0b001000 ) {
			_.dy = padding;
		}
		if ( overflow & 0b000010 ) {
			_.dz = padding;
		}

		{
			unique_lock<mutex> lk( read_buffer->mtx );
			read_buffer->cond_notify_read_compute.wait(
			  lk, [&] { return not read_buffer->ready; } );

			vm::println( "read stride: {} {}", _.stride_start, _.stride_size );
			vm::println( "read region(raw): {} {}", _.raw_region_start, _.raw_region_size );
			vm::println( "read region: {} {}", _.region_start, _.region_size );

			vm::println( "overflow: {#x}", overflow );
			vm::println( "dxy: {}", Vec2i( _.dx, _.dy ) );

			/* always read region into buffer[0..] */
			read_one_batch( _.region_start, _.region_size );
			read_buffer->ready = true;  //flag

			unique_lock<mutex> lk2( write_buffer->mtx );
			write_buffer->cond_notify_read_compute.wait(
			  lk2, [&] { return not write_buffer->ready; } );

			/* transfer overflowed into correct position */
			if ( overflow ) {
				adjust_memory_layout( _ );
			}

			blocklify_one_batch( _ );

			// compute finished
			read_buffer->ready = false;  // dirty, prepare for next read
			write_buffer->stride = make_pair(
			  slice * dim.x * dim.y +
				it * ncols * nrows_per_stride +
				rep * ncols_per_stride,
			  _.stride_size.Prod() );
			write_buffer->ready = true;  // ready to write
		}

		read_buffer->cond_notify_read_compute.notify_one();  // notify to read next section
		write_buffer->cond_notify_write.notify_one();		 // notify to the write thread to write into the disk
	}

	VM_NO_INLINE void write_task( ConvertOptions const &opts, index::Compressor<> &comp )
	{
		{
			unique_lock<mutex> lk( write_buffer->mtx );
			write_buffer->cond_notify_write.wait( lk, [&] { return write_buffer->ready; } );

			const auto [ index, nblocks ] = write_buffer->stride;

			const auto one_block = nvoxels_per_block * sizeof( Voxel );
			const auto offset = one_block * index;
			const auto nbytes = one_block * nblocks;
			///TODO:: write to file
			vm::println( "write {} byte(s) to disk:", nbytes );
			{
				vm::Timer::Scoped t( [&]( auto dt ) {
					vm::println( "Write To Disk Finished. Time: {}", dt.ms() );
				} );
				auto buf = reinterpret_cast<char const *>( write_buffer->buffer.get() );
				for ( int i = 0; i != nblocks; ++i ) {
					auto j = index + i;
					SliceReader inner( buf + one_block * i, one_block );

					struct PaddedReader : Reader
					{
						PaddedReader( Reader &_, size_t len, char fill = 0 ) :
						  _( _ ),
						  len( len ),
						  fill( fill )
						{
						}
						void seek( size_t pos ) override
						{
							_.seek( pos );
						}
						size_t tell() const override
						{
							return _.tell() + p;
						}
						size_t size() const override
						{
							return len;
						}
						size_t read( char *dst, size_t dlen ) override
						{
							auto nread = _.read( dst, dlen );
							auto remain = len - _.size() - p;
							if ( nread < dlen && remain > 0 ) {
								auto nfill = std::min( dlen - nread, remain );
								p += nfill;
								memset( dst + nread, fill, sizeof( fill ) * nfill );
								nread += nfill;
							}
							return nread;
						}

					private:
						Reader &_;
						size_t len;
						size_t p = 0;
						char fill;
					};

					auto idx = index::Idx{}
								 .set_x( j % dim.x )
								 .set_y( j / dim.x % dim.y )
								 .set_z( j / ( dim.x * dim.y ) % dim.z );

					if ( opts.frame_len ) {
						auto nframes = RoundUpDivide( inner.size(), opts.frame_len );
						PaddedReader reader( inner, opts.frame_len * nframes );
						comp.put( idx, reader );
					} else {
						comp.put( idx, inner );
					}
				}
			}
			written_blocks += dim.x * dim.y;

			float cur_percent = written_blocks * 1.0 / dim.total();
			// size_t seconds = t.eval_remaining_time( cur_percent ) / 1000000;
			// const int hh = seconds / 3600;
			// const int mm = ( seconds - hh * 3600 ) / 60;
			// const int ss = int( seconds ) % 60;
			// printf( "%20lld blocks finished, made up %.2f%%. Estimated remaining time: %02d:%02d:%02d\n",
			// 		written_blocks, written_blocks  * 100.0 / dim.total(), hh, mm, ss );
			write_buffer->ready = false;  // prepare for next compute
		}
		write_buffer->cond_notify_read_compute.notify_one();  // notify to the read thread for the next read and computation
	}
};

VM_EXPORT
{
	Refiner::Refiner( RefinerOptions const &opts ) :
	  _( new RefinerImpl( opts ) )
	{
	}
	Refiner::~Refiner()
	{
	}
	bool Refiner::convert( ConvertOptions const &opts )
	{
		return _->convert( opts );
	}
}

VM_END_MODULE()

}  // namespace vol
