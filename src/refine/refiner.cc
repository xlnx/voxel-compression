#include <vocomp/refine/refiner.hpp>

#include <thread>
#include <VMat/geometry.h>
#include <VMat/numeric.h>
#include <VMUtils/timer.hpp>
#include <VMFoundation/rawreader.h>
#include <vocomp/index/compressor.hpp>
#include <koi.hpp>

namespace vol
{
VM_BEGIN_MODULE( refine )

using namespace ysl;

using Voxel = char;

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

struct RefinerImpl final : vm::NoCopy, vm::NoMove
{
private:
	index::Idx raw;
	size_t log_block_size, block_size, block_inner, padding;
	index::Idx dim;
	index::Idx adjusted;

	RawReaderIO input;
	ofstream output;

	// vm::ThreadExchange_Pool exchange_pool = thread::hardware_concurrency();

	koi::runtime::Runtime rt;
	koi::runtime::Runtime exchange_pool;
	koi::runtime::Runtime write_pool;

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
	  output( opts.output, std::ios::binary )
	{
		if ( padding < 0 || padding > 2 ) {
			std::cout << "Unsupported padding\n";
			return;
		}

		vm::println( "raw: {}", raw );
		vm::println( "block_size: {}", block_size );
		vm::println( "block_inner: {}", block_inner );
		vm::println( "padding: {}", padding );
		vm::println( "dim: {}", dim );
		vm::println( "adjusted: {}", adjusted );

		if ( not output.is_open() ) {
			throw std::runtime_error( "can not open lvd file" );
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

	bool convert( ConvertOptions const &opts )
	{
		auto pipe = opts.pipe;
		if ( not pipe ) {
			pipe = std::make_shared<vol::Copy>();
		}

		vol::UnboundedStreamWriter writer( output, sizeof( Header ) );
		index::Compressor<> comp( index::Idx{}
									.set_x( block_size )
									.set_y( block_size )
									.set_z( block_size ),
								  writer, *pipe );

		auto t = vm::Timer();
		t.start();

		const std::size_t nvoxels_per_block = block_size * block_size * block_size;

		int ncols = dim.x;
		int nrows = dim.y;
		int nslices = dim.z;

		const auto [ ncols_per_stride, nrows_per_stride, stride_interval ] = [&] {
			std::size_t gb_to_bytes = std::size_t( 1024 ) /*Mb*/ * 1024 /*Kb*/ * 1024 /*Bytes*/;
			std::size_t block_size_in_bytes = sizeof( Voxel ) * nvoxels_per_block;
			std::size_t mem_size_in_bytes = opts.suggest_mem_gb * gb_to_bytes;
			int nblocks_in_mem = mem_size_in_bytes / block_size_in_bytes / 2 /*two buffers*/;
			if ( not nblocks_in_mem ) {
				throw std::runtime_error( "total memory < block size" );
			}
			// const int maxBlocksPerStride = 2;
			// nblocks_in_mem = std::min( nblocks_in_mem, maxBlocksPerStride );
			int nrows_per_stride = std::min( nblocks_in_mem / ncols, nrows );
			int ncols_per_stride = ncols;
			int stride_interval = 1;
			if ( not nrows_per_stride ) { /*nblocks_in_mem < nBlocksPerRow*/
				nrows_per_stride = 1;
				ncols_per_stride = nblocks_in_mem;
				stride_interval = ysl::RoundUpDivide( ncols, ncols_per_stride );
			}
			return std::make_tuple( ncols_per_stride, nrows_per_stride, stride_interval );
		}();
		const int nblocks_per_stride = ncols_per_stride * nrows_per_stride;
		vm::println( "stride size: {} x {} = {} block(s)",
					 ncols_per_stride, nrows_per_stride, nblocks_per_stride );

		const int nrow_iters = ysl::RoundUpDivide( nrows, nrows_per_stride );

		vm::println( "total strides: {}", nrow_iters * stride_interval * nslices );

		// since read_buffer is no larger than write_buffer
		const std::size_t buffer_size = nvoxels_per_block * nblocks_per_stride;
		vm::println( "allocing buffers: {} byte(s) x 2 = {} Mb",
					 buffer_size, buffer_size / 1024 /*Kb*/ / 1024 /*Mb*/ );
		vector<char> read_buffer( nvoxels_per_block * nblocks_per_stride );
		vector<char> write_buffer( nvoxels_per_block * nblocks_per_stride );

		std::atomic<size_t> read_blocks = 0;
		std::size_t written_blocks = 0;

		auto stride_read_task = [&, this]( int slice, int it, int rep ) {
			const auto blkid_base = slice * dim.x * dim.y +
									it * ncols * nrows_per_stride +
									rep * ncols_per_stride;

			const Vec2i stride_start( rep * ncols_per_stride, it * nrows_per_stride );
			const Size2 stride_size(
			  std::min( ncols - stride_start.x, ncols_per_stride ),
			  std::min( nrows - stride_start.y, nrows_per_stride ) );

			/* left bottom corner of region, if padding > 0 this coord might be < 0 */
			Vec3i region_start(
			  stride_start.x * block_inner - padding,
			  stride_start.y * block_inner - padding,
			  slice * block_inner - padding );
			/* whole region size includes padding, might overflow */
			Size3 region_size(
			  stride_size.x * block_inner + padding * 2,
			  stride_size.y * block_inner + padding * 2,
			  block_inner + padding * 2 );
			auto raw_region_size = region_size;
			auto raw_region_start = region_start;

			/* overflow = bbbbbb -> -x,x,-y,y,-z,z */
			int overflow = 0;
			/* region overflows: x1 > X */
			if ( region_size.x + region_start.x > raw.x ) {
				region_size.x = raw.x - region_start.x;
				overflow |= 0b010000;
			}
			/* region overflows: y1 > Y */
			if ( region_size.y + region_start.y > raw.y ) {
				region_size.y = raw.y - region_start.y;
				overflow |= 0b000100;
			}
			/* region overflows: z1 > Z */
			if ( region_size.z + region_start.z > raw.z ) {
				region_size.z = raw.z - region_start.z;
				overflow |= 0b000001;
			}
			/* region overflows: x0 < 0, must be a padding */
			if ( region_start.x < 0 ) {
				region_size.x -= padding;
				region_start.x = 0;
				overflow |= 0b100000;
			}
			/* region overflows: y0 < 0, must be a padding */
			if ( region_start.y < 0 ) {
				region_size.y -= padding;
				region_start.y = 0;
				overflow |= 0b001000;
			}
			/* region overflows: x0 < 0, must be a padding */
			if ( region_start.z < 0 ) {
				region_size.z -= padding;
				region_start.z = 0;
				overflow |= 0b000010;
			}

			std::size_t dx = 0, dy = 0, dz = 0;
			/* let x_i = first voxel of line i
				   x_i = len * i
				   -> x'_i = slen * i + dx? * 1 + slen * dy?*/
			if ( overflow & 0b100000 ) {
				dx = padding;
			}
			if ( overflow & 0b001000 ) {
				dy = padding;
			}
			if ( overflow & 0b000010 ) {
				dz = padding;
			}

			vm::println( "read stride: {} {}", stride_start, stride_size );
			vm::println( "read region(raw): {} {}", raw_region_start, raw_region_size );
			vm::println( "read region: {} {}", region_start, region_size );

			vm::println( "overflow: {#x}", overflow );
			vm::println( "dxy: {}", Vec2i( dx, dy ) );

			/* always read region into buffer[0..] */
			input.readRegion(
			  region_start, region_size,
			  reinterpret_cast<unsigned char *>( read_buffer.data() ) );

			/* transfer overflowed into correct position */
			if ( overflow ) {
				memset( write_buffer.data(), 0,
						sizeof( Voxel ) * nvoxels_per_block * nblocks_per_stride );
				auto dst = write_buffer.data();
				auto src = read_buffer.data();
				for ( std::size_t dep = 0; dep < region_size.z; ++dep ) {
					auto slice_dst = dst + dep * raw_region_size.x * raw_region_size.y;
					auto slice_src = src + dep * region_size.x * region_size.y;
					for ( std::size_t i = 0; i < region_size.y; ++i ) {
						memcpy(
						  slice_dst + dz * raw_region_size.x * raw_region_size.y +
							( i + dy ) * raw_region_size.x + dx, /*x'_i*/
						  slice_src + i * region_size.x,		 /*x_i*/
						  region_size.x * sizeof( Voxel ) );
					}
				}
				write_buffer.swap( read_buffer );
			}

			auto blocklify_task = [&]( int xb, int yb ) {
				const int blkid = xb + yb * stride_size.x;
				const auto dst = write_buffer.data() +
								 blkid * nvoxels_per_block;
				const auto src = read_buffer.data() +
								 xb * block_inner + yb * block_inner * raw_region_size.x;

				for ( std::size_t dep = 0; dep < block_size; ++dep ) {
					auto slice_dst = dst + dep * block_size * block_size;
					auto slice_src = src + dep * raw_region_size.x * raw_region_size.y;
					for ( std::size_t row = 0; row < block_size; ++row ) {
						memcpy(
						  slice_dst + row * block_size,
						  slice_src + row * raw_region_size.x,
						  block_size * sizeof( Voxel ) );
					}
				}

				++read_blocks;
			};
			const int REV_INTERVAL = std::max( nblocks_per_stride / 20, 1 );
			atomic<int> interval( 0 );
			auto write_task = [&]( SliceReader block, size_t blkid ) {
				//vm::println( "write block {}", blkid );
				const auto one_block = nvoxels_per_block * sizeof( Voxel );

				{
					auto s = writer.tell();
					vm::Timer::Scoped t( [&]( auto dt ) {
						auto t = writer.tell();
						interval++;
						if ( interval.load() == REV_INTERVAL ) {
							vm::println( "written {} bytes in {}", t - s, dt.s() );
							interval.store( 0 );
						}
					} );
					auto buf = reinterpret_cast<char const *>( write_buffer.data() );
					auto idx = index::Idx{}
								 .set_x( blkid % dim.x )
								 .set_y( blkid / dim.x % dim.y )
								 .set_z( blkid / ( dim.x * dim.y ) % dim.z );

					if ( opts.frame_len ) {
						auto nframes = RoundUpDivide( block.size(), opts.frame_len );
						PaddedReader reader( block, opts.frame_len * nframes );
						comp.put( idx, reader );
					} else {
						comp.put( idx, block );
					}
				}
				++written_blocks;

				if ( interval.load() == 0 ) {
					float cur_percent = written_blocks * 1.0 / dim.total();
					size_t seconds = t.eval_remaining_time( cur_percent ) / 1000000;
					const int hh = seconds / 3600;
					const int mm = ( seconds - hh * 3600 ) / 60;
					const int ss = int( seconds ) % 60;
					printf( "%20lld blocks finished, made up %.2f%%. Estimated remaining time: %02d:%02d:%02d\n",
							written_blocks, written_blocks * 100.0 / dim.total(), hh, mm, ss );
				}
			};
			auto exchange_sink = koi::future::sink();
			auto write_sink = koi::future::sink();
			auto exchange_handle = exchange_sink.handle();
			auto write_handle = write_sink.handle();
			for ( int yb = 0; yb < stride_size.y; yb++ ) {
				for ( int xb = 0; xb < stride_size.x; xb++ ) {
					exchange_pool.spawn(
					  koi::future::lazy(
						[&, xb, yb] {
							blocklify_task( xb, yb );
						} )
						.then(
						  [&, xb, yb] {
							  // write one block
							  const size_t blkid = xb + yb * stride_size.x;
							  const auto dst = write_buffer.data() +
											   blkid * nvoxels_per_block;
							  write_pool.spawn(
								koi::future::lazy(
								  [&, dst, blkid] {
									  write_task(
										SliceReader( dst, nvoxels_per_block ),
										blkid + blkid_base );
								  } )
								  .gather( write_handle ) );
						  } )
						.gather( exchange_handle ) );
				}
			}
			exchange_pool.spawn(
			  std::move( exchange_sink )
				.then( [&]( bool ok ) {
					vm::println( "gather: {}", ok );
				} ) );

			exchange_pool.spawn(
			  std::move( write_sink ) );
			exchange_pool.run();

			vm::println( "handled {} blocks", read_blocks );
		};

		{
			vm::Timer::Scoped t( [&]( auto dt ) {
				vm::println( "total convert time: {}", dt.s() );
			} );

			atomic<bool> should_stop( false );
			auto w = thread( [&] {
				while ( !should_stop.load() ) { write_pool.run(); }
			} );
			for ( int slice = 0; slice < nslices; slice++ ) {
				for ( int it = 0; it < nrow_iters; ++it ) {
					for ( int rep = 0; rep < stride_interval; ++rep ) {
						rt.spawn(
						  koi::future::lazy(
							[&, slice, it, rep] {
								stride_read_task( slice, it, rep );
							} ) );
					}
				}
			}
			rt.run();
			should_stop.store( true );
			w.join();
		}

		return true;
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
