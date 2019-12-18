#include <vocomp/refine/refiner.hpp>

#include <thread>
#include <VMat/geometry.h>
#include <VMat/numeric.h>
#include <VMUtils/timer.hpp>
#include <VMFoundation/rawreader.h>
#include <vocomp/index.hpp>
#include <vocomp/unbounded_io.hpp>

namespace vol
{
VM_BEGIN_MODULE( refine )

using namespace vm;
using namespace std;

using Voxel = char;

struct RefinerImpl final : vm::NoCopy, vm::NoMove
{
private:
	size_t log_block_size, block_size, block_inner, padding;
	Idx raw, dim, adjusted;

	const size_t nvoxels_per_block;
	const int ncols, nrows, nslices;
	int ncols_per_stride, nrows_per_stride, stride_interval;
	int nblocks_per_stride, nrow_iters;
	size_t buffer_size;

	RawReaderIO input;
	ofstream output;

	vol::UnboundedStreamWriter body_writer;
	video::Compressor video_compressor;

	vector<char> read_buffer, write_buffer;
	atomic<size_t> read_blocks = 0;
	size_t written_blocks = 0;
	vm::Timer t;

	map<Idx, BlockIndex> block_idx;

public:
	RefinerImpl( RefinerOptions const &opts ) :
	  log_block_size( opts.log_block_size ),
	  block_size( 1 << opts.log_block_size ),
	  block_inner( block_size - 2 * opts.padding ),
	  padding( opts.padding ),
	  raw{ Idx{}.set_x( opts.x ).set_y( opts.y ).set_z( opts.z ) },
	  dim( Idx{}
			 .set_x( RoundUpDivide( raw.x, block_inner ) )
			 .set_y( RoundUpDivide( raw.y, block_inner ) )
			 .set_z( RoundUpDivide( raw.z, block_inner ) ) ),
	  adjusted( Idx{}
				  .set_x( dim.x * block_size )
				  .set_y( dim.y * block_size )
				  .set_z( dim.z * block_size ) ),
	  nvoxels_per_block( block_size * block_size * block_size ),
	  ncols( dim.x ),
	  nrows( dim.y ),
	  nslices( dim.z ),
	  input( opts.input, Size3( raw.x, raw.y, raw.z ), sizeof( Voxel ) ),
	  output( opts.output, ios::binary ),
	  body_writer( output, sizeof( Header ) ),
	  video_compressor( body_writer, opts.compress_opts )
	{
		if ( padding < 0 || padding > 2 ) {
			throw runtime_error( "unsupported padding" );
		}
		if ( not output.is_open() ) {
			throw runtime_error( "can not open output file" );
		}

		size_t gb_to_bytes = size_t( 1024 ) /*Mb*/ * 1024 /*Kb*/ * 1024 /*Bytes*/;
		size_t block_size_in_bytes = sizeof( Voxel ) * nvoxels_per_block;
		size_t mem_size_in_bytes = opts.suggest_mem_gb * gb_to_bytes;
		int nblocks_in_mem = mem_size_in_bytes / block_size_in_bytes / 2 /*two buffers*/;
		if ( not nblocks_in_mem ) {
			throw runtime_error( "total memory < block size" );
		}

		vm::println( "raw: {}", raw );
		vm::println( "block_size: {}", block_size );
		vm::println( "block_inner: {}", block_inner );
		vm::println( "padding: {}", padding );
		vm::println( "dim: {}", dim );
		vm::println( "adjusted: {}", adjusted );

		// const int maxBlocksPerStride = 2;
		// nblocks_in_mem = std::min( nblocks_in_mem, maxBlocksPerStride );
		nrows_per_stride = std::min( nblocks_in_mem / ncols, nrows );
		ncols_per_stride = ncols;
		stride_interval = 1;
		if ( not nrows_per_stride ) { /*nblocks_in_mem < nBlocksPerRow*/
			nrows_per_stride = 1;
			ncols_per_stride = nblocks_in_mem;
			stride_interval = RoundUpDivide( ncols, ncols_per_stride );
		}

		nblocks_per_stride = ncols_per_stride * nrows_per_stride;
		vm::println( "stride size: {} x {} = {} block(s)",
					 ncols_per_stride, nrows_per_stride, nblocks_per_stride );

		nrow_iters = RoundUpDivide( nrows, nrows_per_stride );

		vm::println( "total strides: {}", nrow_iters * stride_interval * nslices );

		// since read_buffer is no larger than write_buffer
		buffer_size = nvoxels_per_block * nblocks_per_stride;
	}
	~RefinerImpl()
	{
	}

	void stride_read_task( int slice, int it, int rep )
	{
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

		size_t dx = 0, dy = 0, dz = 0;
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
			for ( size_t dep = 0; dep < region_size.z; ++dep ) {
				auto slice_dst = dst + dep * raw_region_size.x * raw_region_size.y;
				auto slice_src = src + dep * region_size.x * region_size.y;
				for ( size_t i = 0; i < region_size.y; ++i ) {
					memcpy(
					  slice_dst + dz * raw_region_size.x * raw_region_size.y +
						( i + dy ) * raw_region_size.x + dx, /*x'_i*/
					  slice_src + i * region_size.x,		 /*x_i*/
					  region_size.x * sizeof( Voxel ) );
				}
			}
			write_buffer.swap( read_buffer );
		}
		for ( int yb = 0; yb < stride_size.y; yb++ ) {
			for ( int xb = 0; xb < stride_size.x; xb++ ) {
				const int dblkid = xb + yb * stride_size.x;
				const auto dst = write_buffer.data() + dblkid * nvoxels_per_block;
				const auto src = read_buffer.data() + xb * block_inner + yb * block_inner * raw_region_size.x;

				for ( size_t dep = 0; dep < block_size; ++dep ) {
					auto slice_dst = dst + dep * block_size * block_size;
					auto slice_src = src + dep * raw_region_size.x * raw_region_size.y;
					for ( size_t row = 0; row < block_size; ++row ) {
						memcpy(
						  slice_dst + row * block_size,
						  slice_src + row * raw_region_size.x,
						  block_size * sizeof( Voxel ) );
					}
				}
				++read_blocks;

				const auto blkid = blkid_base + dblkid;
				const auto idx = Idx{}
								   .set_x( blkid % dim.x )
								   .set_y( blkid / dim.x % dim.y )
								   .set_z( blkid / ( dim.x * dim.y ) % dim.z );
				block_idx[ idx ] = video_compressor.accept(
				  vm::Arc<Reader>( new SliceReader( dst, nvoxels_per_block ) ) );
			}
		}
		video_compressor.flush( true );
		// vm::println( "{}", video_compressor.frame_len() );
		vm::println( "handled {} blocks", read_blocks );
	}

	bool convert()
	{
		t.start();

		vm::println( "allocing buffers: {} byte(s) x 2 = {} Mb",
					 buffer_size, buffer_size / 1024 /*Kb*/ / 1024 /*Mb*/ );
		read_buffer.resize( nvoxels_per_block * nblocks_per_stride );
		write_buffer.resize( nvoxels_per_block * nblocks_per_stride );

		{
			vm::Timer::Scoped t( [&]( auto dt ) {
				vm::println( "total convert time: {}", dt.s() );
			} );

			atomic<bool> should_stop( false );
			for ( int slice = 0; slice < nslices; slice++ ) {
				for ( int it = 0; it < nrow_iters; ++it ) {
					for ( int rep = 0; rep < stride_interval; ++rep ) {
						stride_read_task( slice, it, rep );
					}
				}
			}
			video_compressor.wait();
		}

		vector<char>{}.swap( read_buffer );
		vector<char>{}.swap( write_buffer );

		uint64_t meta_offset = body_writer.tell();
		body_writer.write_typed( video_compressor.frame_offset() );
		body_writer.write_typed( block_idx );
		body_writer.write_typed( meta_offset );

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
		writer.write_typed( header );

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
	bool Refiner::convert()
	{
		return _->convert();
	}
}

VM_END_MODULE()

}  // namespace vol
