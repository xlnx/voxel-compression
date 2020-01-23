#include <fstream>
#include <gtest/gtest.h>
#define private public
#define protected public
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

	vector<Idx> blocks = { { 0, 0, 0 }, { 1, 0, 0 } };
	vector<unsigned char> buffer( 64 * 64 * 64 );
	vector<tuple<Idx, unsigned, unsigned, unsigned, unsigned, unsigned>> res;
	cufx::MemoryView1D<unsigned char> buffer_view( buffer.data(), buffer.size() );
	extractor.batch_extract( blocks, [&]( Idx const &idx, VoxelStreamPacket const &packet ) {
		res.emplace_back( idx, packet.offset, packet.inner_offset,
						  packet.length, packet._.id, packet._.length );
		// vm::println( "{}:{ <8} ({}, {})    #{}.{}",
		// 			 idx, packet.offset, packet.inner_offset,
		// 			 packet.length, packet._.id, packet._.length );
		packet.append_to( buffer_view );
	} );
	ASSERT_EQ( res,
			   ( vector<tuple<Idx, unsigned, unsigned, unsigned, unsigned, unsigned>>{
				 { { 0, 0, 0 }, 0, 0, 98304, 0, 98304 },
				 { { 0, 0, 0 }, 98304, 0, 98304, 1, 98304 },
				 { { 0, 0, 0 }, 98304 * 2, 0, 65536, 2, 98304 },
				 { { 1, 0, 0 }, 0, 65536, 32768, 2, 98304 },
				 { { 1, 0, 0 }, 32768, 0, 98304, 3, 98304 },
				 { { 1, 0, 0 }, 32768 + 98304, 0, 98304, 4, 98304 },
				 { { 1, 0, 0 }, 32768 + 98304 * 2, 0, 32768, 5, 98304 } } ) );
}
