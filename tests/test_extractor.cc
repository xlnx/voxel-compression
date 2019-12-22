#include <fstream>
#include <gtest/gtest.h>
#include <vocomp/refiner.hpp>
#include <vocomp/extractor.hpp>

using namespace std;
using namespace vol;

TEST( test_extractor, simple )
{
	auto opts = vol::RefinerOptions{}
				  .set_x( 256 )
				  .set_y( 256 )
				  .set_z( 256 )
				  .set_log_block_size( 6 )  // 64
				  .set_padding( 2 )
				  .set_suggest_mem_gb( 4 )
				  .set_input( "test_data/aneurism_256x256x256_uint8.raw" )
				  .set_output( "test.aneurism_256x256x256_uint8.h264" );
	opts.compress_opts
	  .set_encode_preset( EncodePreset::Default )
	  .set_pixel_format( PixelFormat::NV12 )
	  .set_width( 256 )
	  .set_height( 256 )
	  .set_batch_frames( 4 );
	{
		Refiner refiner( opts );
		refiner.convert();
	}
	ifstream is( "./test.aneurism_256x256x256_uint8.h264", ios::binary );
	is.seekg( 0, is.end );
	vm::println( "file size: {}", is.tellg() );
	StreamReader reader( is, 0, is.tellg() );
	reader.seek( 0 );
	ASSERT_EQ( reader.tell(), 0 );
	Extractor extractor( reader );
	EXPECT_EQ( extractor.raw(), ( Idx{ 256, 256, 256 } ) );
	EXPECT_EQ( extractor.dim(), ( Idx{ 5, 5, 5 } ) );
	EXPECT_EQ( extractor.adjusted(), ( Idx{ 320, 320, 320 } ) );
	EXPECT_EQ( extractor.log_block_size(), 6 );
	EXPECT_EQ( extractor.block_size(), 64 );
	EXPECT_EQ( extractor.block_inner(), 60 );
	EXPECT_EQ( extractor.padding(), 2 );
	struct Consumer : BlockConsumer
	{
		Consumer( size_t buffer_size ) :
		  buffer( buffer_size ),
		  _( buffer.data(), buffer.size() )
		{
		}

		void consume( cufx::MemoryView1D<unsigned char> const &data,
					  Idx const &idx,
					  std::size_t offset ) override
		{
			res.emplace_back( idx, data.size(), offset );
		}
		void wait() override
		{
			vm::println( "wait" );
		}
		cufx::MemoryView1D<unsigned char> swap_buffer() const override { return _; }

		vector<unsigned char> buffer;
		vector<tuple<Idx, size_t, size_t>> res;
		cufx::MemoryView1D<unsigned char> _;
	};
	vector<Idx> blocks = { { 0, 0, 0 }, { 1, 0, 0 } };
	vm::println( "swap_buffer: {}", extractor.frame_size() * 3 );
	Consumer consumer( extractor.frame_size() * 4 );
	extractor.batch_extract( blocks, consumer );
	ASSERT_EQ( consumer.res,
			   ( vector<tuple<Idx, size_t, size_t>>{
				 { { 0, 0, 0 }, 262144, 0 },
				 { { 1, 0, 0 }, 98304, 0 },
				 { { 1, 0, 0 }, 163840, 98304 } } ) );
	// extractor.batch_extract()
}
