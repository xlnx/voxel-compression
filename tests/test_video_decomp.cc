#include <random>
#include <gtest/gtest.h>
#include <vocomp/video/compressor.hpp>
#include <vocomp/video/decompressor.hpp>

using namespace vol;
using namespace video;

TEST( test_video_decomp, test_video_decomp )
{
	const auto width = 256;
	const auto height = 256;
	const auto nframes = 4;

	std::random_device rd;
	std::uniform_int_distribution<char> dist( 0, 127 );
	std::vector<char> raw_nv12( width * height * nframes * 3 / 2, 0 );
	std::transform( raw_nv12.begin(), raw_nv12.end(), raw_nv12.begin(),
					[&]( auto _ ) { return dist( rd ); } );
	std::vector<char> compressed( raw_nv12.size(), 0 );
	SliceReader raw_nv12_reader( raw_nv12.data(), raw_nv12.size() );
	SliceWriter compressed_writer( compressed.data(), compressed.size() );

	auto comp_opts = CompressOptions{}
					   .set_encode_method( EncodeMethod::H264 )
					   .set_encode_preset( EncodePreset::Default )
					   .set_pixel_format( PixelFormat::NV12 )
					   .set_width( width )
					   .set_height( height );
	Compressor compressor( comp_opts );
	compressor.transfer( raw_nv12_reader, compressed_writer );
	EXPECT_EQ( compressor.frame_count(), nframes * 2 );

	EXPECT_GT( raw_nv12.size(), compressed_writer.tell() );

	SliceReader compressed_reader( compressed.data(), compressed_writer.tell() );
	auto decomp_opts = DecompressorOptions{}
						 .set_encode( EncodeMethod::H264 );
	Decompressor decompressor( decomp_opts );

	std::vector<char> decompressed( raw_nv12.size() * 3, 0 );
	SliceWriter decompressed_writer( decompressed.data(), decompressed.size() );
	decompressor.decompress( compressed_reader, decompressed_writer );

	EXPECT_EQ( compressed_reader.tell(), compressed_writer.tell() );
	EXPECT_EQ( decompressed_writer.tell(), raw_nv12.size() * 2 );

	std::vector<int> diff( width * height, 0 );
	int max_diff = 0;
	for ( int i = 0; i != width * height; ++i ) {
		diff[ i ] = (unsigned char)decompressed[ i ] - (unsigned char)raw_nv12[ i ];
		EXPECT_NEAR( diff[ i ], 0, 20 );
		max_diff = std::max( std::abs( diff[ i ] ), max_diff );
	}
	vm::println( "max_diff: {}", max_diff );
}
