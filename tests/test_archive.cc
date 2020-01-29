#include <fstream>
#include <gtest/gtest.h>
#define private public
#define protected public
#include <VMat/geometry.h>
#include <VMat/numeric.h>
#include <VMFoundation/rawreader.h>
#include <varch/archive/archiver.hpp>
#include <varch/unarchive/unarchiver.hpp>

using namespace vm;
using namespace std;
using namespace vol;

void compress_256( string const &raw_input_file, string const &h264_output_file )
{
	auto opts = vol::ArchiverOptions{}
				  .set_x( 256 )
				  .set_y( 256 )
				  .set_z( 256 )
				  .set_log_block_size( 6 )	// 64
				  .set_padding( 0 )
				  .set_suggest_mem_gb( 4 )
				  .set_input( raw_input_file )
				  .set_output( h264_output_file );
	opts.compress_opts
	  .set_encode_preset( EncodePreset::Default )
	  .set_width( 1024 )
	  .set_height( 1024 )
	  .set_batch_frames( 4 );
	{
		Archiver archiver( opts );
		archiver.convert();
	}
}

bool compare_block( Unarchiver &unarchiver, string const &raw_input_file, Idx const &idx )
{
	const auto N = unarchiver.block_size();
	const auto N_3 = N * N * N;
	const auto raw = unarchiver.raw();

	vector<unsigned char> buffer( N_3 );

	RawReaderIO raw_input( raw_input_file, Size3( raw.x, raw.y, raw.z ), sizeof( char ) );
	vector<unsigned char> src_buffer( N_3 );

	unarchiver.unarchive_to( idx, buffer );

	auto begin = Vec3i( idx.x, idx.y, idx.z ) * N;
	raw_input.readRegion( begin, Size3( N, N, N ), src_buffer.data() );
	double s = 0, a = 0, m = 0;
	for ( int i = 0; i != buffer.size(); ++i ) {
		auto dt = double( buffer[ i ] - src_buffer[ i ] );
		a += src_buffer[ i ];
		m = std::max( std::abs( dt ), m );
		s += dt * dt;
	}
	s = std::sqrt( s / buffer.size() );
	a /= buffer.size();
	vm::println( "block = {}, stddev = {}, avg = {}, maxdiff = {}", idx, s, a, m );

	const double threshold = 15;
	if ( s < threshold ) { return true; }

	println( "given:" );
	for ( int i = 0; i != buffer.size() / ( N * N ); ++i ) {
		// vm::print( "{ >#x2} ", int( std::abs( buffer[ i ] - src_buffer[ i ] ) ) );
		vm::print( "{ >#x2} ", int( buffer[ i ] ) );
		if ( i % 16 == 15 ) {
			vm::println( "" );
		}
	}
	println( "expected:" );
	for ( int i = 0; i != buffer.size() / ( N * N ); ++i ) {
		vm::print( "{ >#x2} ", int( src_buffer[ i ] ) );
		if ( i % 16 == 15 ) {
			vm::println( "" );
		}
	}
	vm::println( "" );
	return false;
}

void decode_256( string const &raw_input_file, string const &h264_output_file )
{
	ifstream is( h264_output_file, ios::binary );
	is.seekg( 0, is.end );
	vm::println( "file size: {}", is.tellg() );
	StreamReader reader( is, 0, is.tellg() );
	reader.seek( 0 );
	ASSERT_EQ( reader.tell(), 0 );
	Unarchiver unarchiver( reader );
	EXPECT_EQ( unarchiver.raw(), ( Idx{ 256, 256, 256 } ) );
	EXPECT_EQ( unarchiver.dim(), ( Idx{ 4, 4, 4 } ) );
	EXPECT_EQ( unarchiver.adjusted(), ( Idx{ 256, 256, 256 } ) );
	EXPECT_EQ( unarchiver.log_block_size(), 6 );
	EXPECT_EQ( unarchiver.block_size(), 64 );
	EXPECT_EQ( unarchiver.block_inner(), 64 );
	EXPECT_EQ( unarchiver.padding(), 0 );

	for ( uint32_t i = 0; i != 4; ++i ) {
		for ( uint32_t j = 0; j != 4; ++j ) {
			for ( uint32_t k = 0; k != 4; ++k ) {
				ASSERT_TRUE( compare_block( unarchiver, raw_input_file, { i, j, k } ) );
			}
		}
	}
}

TEST( test_archive, aneurism )
{
	auto raw_input_file = "./test_data/aneurism_256x256x256_uint8.raw";
	auto h264_output_file = "./test.aneurism_256x256x256_uint8.h264";
	compress_256( raw_input_file, h264_output_file );
	decode_256( raw_input_file, h264_output_file );
}

TEST( test_archive, urandom )
{
	auto raw_input_file = "./test_data/urandom_256x256x256_uint8.raw";
	auto h264_output_file = "./test.urandom_256x256x256_uint8.h264";
	compress_256( raw_input_file, h264_output_file );
	decode_256( raw_input_file, h264_output_file );
}
