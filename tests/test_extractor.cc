#include <fstream>
#include <gtest/gtest.h>
#define private public
#define protected public
#include <vocomp/refiner.hpp>
#include <vocomp/extractor.hpp>
#include <VMat/geometry.h>
#include <VMat/numeric.h>
#include <VMFoundation/rawreader.h>

using namespace vm;
using namespace std;
using namespace vol;

void compress_256( string const &raw_input_file, string const &h264_output_file )
{
	auto opts = vol::RefinerOptions{}
				  .set_x( 256 )
				  .set_y( 256 )
				  .set_z( 256 )
				  .set_log_block_size( 6 )  // 64
				  .set_padding( 0 )
				  .set_suggest_mem_gb( 4 )
				  .set_input( raw_input_file )
				  .set_output( h264_output_file );
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
}

void decompress_256( string const &raw_input_file, string const &h264_output_file,
					 Idx const &block_idx )
{
	ifstream is( h264_output_file, ios::binary );
	is.seekg( 0, is.end );
	vm::println( "file size: {}", is.tellg() );
	StreamReader reader( is, 0, is.tellg() );
	reader.seek( 0 );
	ASSERT_EQ( reader.tell(), 0 );
	Extractor extractor( reader );
	EXPECT_EQ( extractor.raw(), ( Idx{ 256, 256, 256 } ) );
	EXPECT_EQ( extractor.dim(), ( Idx{ 4, 4, 4 } ) );
	EXPECT_EQ( extractor.adjusted(), ( Idx{ 256, 256, 256 } ) );
	EXPECT_EQ( extractor.log_block_size(), 6 );
	EXPECT_EQ( extractor.block_size(), 64 );
	EXPECT_EQ( extractor.block_inner(), 64 );
	EXPECT_EQ( extractor.padding(), 0 );

	vector<Idx> blocks = { block_idx };

	vector<unsigned char> buffer( 64 * 64 * 64 );
	vector<tuple<Idx, unsigned, unsigned, unsigned, unsigned, unsigned>> res;
	cufx::MemoryView1D<unsigned char> buffer_view( buffer.data(), buffer.size() );
	extractor.batch_extract( blocks, [&]( Idx const &idx, VoxelStreamPacket const &packet ) {
		res.emplace_back( idx, packet.offset, packet.inner_offset,
						  packet.length, packet._.id, packet._.length );
		packet.append_to( buffer_view );
	} );

	ASSERT_GT( res.size(), 0 );

	RawReaderIO raw_input( raw_input_file, Size3( 256, 256, 256 ), sizeof( char ) );
	vector<unsigned char> src_buffer( 64 * 64 * 64 );

	auto begin = Vec3i( block_idx.x, block_idx.y, block_idx.z ) * 64;
	raw_input.readRegion( begin, Size3( 64, 64, 64 ), src_buffer.data() );
	// const auto slice_idx = 58 * 64 + 8;
	// const auto slice_size = buffer.size() / 64 / 64;
	// vm::println( "{}", (void *)buffer.data() );
	// for ( int i = slice_idx * slice_size, j = 0; i != ( slice_idx + 1 ) * slice_size; ++i, j = ( j + 1 ) % 8 ) {
	int dt = 0;
	for ( int i = 0; i != buffer.size(); ++i ) {
		// vm::print( "{}_{} ", int( buffer[ i ] ), int( src_buffer[ i ] ) );
		dt = std::max( std::abs( buffer[ i ] - src_buffer[ i ] ), dt );
	}
	vm::println( "max diff: {}", dt );
	if ( dt > 30 ) {
		for ( int i = 0; i != buffer.size() / ( 64 * 64 ); ++i ) {
			vm::print( "{ >3} ", int( buffer[ i ] ) );
			if ( i % 16 == 15 ) {
				vm::println( "" );
			}
		}
		vm::println( "" );
		for ( int i = 0; i != buffer.size() / ( 64 * 64 ); ++i ) {
			vm::print( "{ >3} ", int( src_buffer[ i ] ) );
			if ( i % 16 == 15 ) {
				vm::println( "" );
			}
		}
	}
	ASSERT_LT( dt, 30 );
}

TEST( test_extractor, simple )
{
	// auto raw_input_file = "./test_data/aneurism_256x256x256_uint8.raw";
	// auto h264_output_file = "./test.aneurism_256x256x256_uint8.h264";
	// auto opts = vol::RefinerOptions{}
	// 			  .set_x( 256 )
	// 			  .set_y( 256 )
	// 			  .set_z( 256 )
	// 			  .set_log_block_size( 6 )  // 64
	// 			  .set_padding( 2 )
	// 			  .set_suggest_mem_gb( 4 )
	// 			  .set_input( raw_input_file )
	// 			  .set_output( h264_output_file );
	// opts.compress_opts
	//   .set_encode_preset( EncodePreset::Default )
	//   .set_pixel_format( PixelFormat::NV12 )
	//   .set_width( 256 )
	//   .set_height( 256 )
	//   .set_batch_frames( 4 );
	// {
	// 	Refiner refiner( opts );
	// 	refiner.convert();
	// }

	// ifstream is( h264_output_file, ios::binary );
	// is.seekg( 0, is.end );
	// vm::println( "file size: {}", is.tellg() );
	// StreamReader reader( is, 0, is.tellg() );
	// reader.seek( 0 );
	// ASSERT_EQ( reader.tell(), 0 );
	// Extractor extractor( reader );
	// EXPECT_EQ( extractor.raw(), ( Idx{ 256, 256, 256 } ) );
	// EXPECT_EQ( extractor.dim(), ( Idx{ 5, 5, 5 } ) );
	// EXPECT_EQ( extractor.adjusted(), ( Idx{ 320, 320, 320 } ) );
	// EXPECT_EQ( extractor.log_block_size(), 6 );
	// EXPECT_EQ( extractor.block_size(), 64 );
	// EXPECT_EQ( extractor.block_inner(), 60 );
	// EXPECT_EQ( extractor.padding(), 2 );

	// vector<Idx> blocks = { { 0, 0, 0 }, { 2, 2, 2 } };

	// vector<unsigned char> buffer( 64 * 64 * 64 );
	// vector<tuple<Idx, unsigned, unsigned, unsigned, unsigned, unsigned>> res;
	// cufx::MemoryView1D<unsigned char> buffer_view( buffer.data(), buffer.size() );
	// extractor.batch_extract( blocks, [&]( Idx const &idx, VoxelStreamPacket const &packet ) {
	// 	res.emplace_back( idx, packet.offset, packet.inner_offset,
	// 					  packet.length, packet._.id, packet._.length );
	// 	if ( idx != Idx{ 0, 0, 0 } ) {
	// 		packet.append_to( buffer_view );
	// 	}
	// } );
	// // ASSERT_EQ( res,
	// // 		   ( vector<tuple<Idx, unsigned, unsigned, unsigned, unsigned, unsigned>>{
	// // 			 { { 0, 0, 0 }, 0, 0, 98304, 0, 98304 },
	// // 			 { { 0, 0, 0 }, 98304, 0, 98304, 1, 98304 },
	// // 			 { { 0, 0, 0 }, 98304 * 2, 0, 65536, 2, 98304 },
	// // 			 { { 2, 2, 2 }, 0, 0, 98304, 3, 98304 },
	// // 			 { { 2, 2, 2 }, 98304, 0, 98304, 4, 98304 },
	// // 			 { { 2, 2, 2 }, 98304 * 2, 0, 65536, 5, 98304 } } ) );

	// RawReaderIO raw_input( raw_input_file, Size3( 256, 256, 256 ), sizeof( char ) );
	// vector<unsigned char> src_buffer( 64 * 64 * 64 );
	// raw_input.readRegion( Vec3i( 118, 118, 118 ), Size3( 64, 64, 64 ), src_buffer.data() );
	// const auto slice_idx = 58 * 64 + 8;
	// const auto slice_size = buffer.size() / 64 / 64;
	// vm::println( "{}", (void *)buffer.data() );
	// for ( int i = slice_idx * slice_size, j = 0; i != ( slice_idx + 1 ) * slice_size; ++i, j = ( j + 1 ) % 8 ) {
	// 	// vm::print( "{} ", int( src_buffer[ i ] ) );
	// 	vm::print( "{}  ", make_pair( int( buffer[ i ] ), int( src_buffer[ i ] ) ) );
	// 	if ( j == 7 ) {
	// 		vm::println( "" );
	// 	}
	// 	// ASSERT_LT( std::abs( buffer[ i ] - src_buffer[ i ] ), 100 );
	// }
}

// TEST( test_extractor, aneurism )
// {
// 	auto raw_input_file = "./test_data/aneurism_256x256x256_uint8.raw";
// 	auto h264_output_file = "./test.aneurism_256x256x256_uint8.h264";
// 	compress_256( raw_input_file, h264_output_file );
// 	decompress_256( raw_input_file, h264_output_file, { 1, 0, 0 } );
// 	decompress_256( raw_input_file, h264_output_file, { 0, 0, 0 } );
// 	decompress_256( raw_input_file, h264_output_file, { 2, 0, 0 } );
// 	decompress_256( raw_input_file, h264_output_file, { 0, 2, 0 } );
// }

TEST( test_extractor, urandom )
{
	auto raw_input_file = "./test_data/urandom_256x256x256_uint8.raw";
	auto h264_output_file = "./test.urandom_256x256x256_uint8.h264";
	compress_256( raw_input_file, h264_output_file );
	// decompress_256( raw_input_file, h264_output_file, { 0, 0, 0 } );
	// decompress_256( raw_input_file, h264_output_file, { 1, 0, 0 } );
	// decompress_256( raw_input_file, h264_output_file, { 2, 0, 0 } );
	decompress_256( raw_input_file, h264_output_file, { 3, 0, 0 } );
	decompress_256( raw_input_file, h264_output_file, { 0, 1, 0 } );
	// decompress_256( raw_input_file, h264_output_file, { 3, 3, 3 } );
}
