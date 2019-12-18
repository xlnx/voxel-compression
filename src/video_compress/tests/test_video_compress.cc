#include <random>
#include <gtest/gtest.h>
#include <vocomp/video/compressor.hpp>
#include "../../utils/unbounded_vector_writer.hpp"

using namespace vol;
using namespace std;
using namespace video;

void add_blocks( vector<vector<char>> &blocks, size_t block_size, int nblocks = 1 )
{
	static random_device rd;
	static uniform_int_distribution<char> dist( 0, 127 );
	for ( int i = 0; i != nblocks; ++i ) {
		vector<char> raw_nv12( block_size );
		std::transform( raw_nv12.begin(), raw_nv12.end(), raw_nv12.begin(),
						[&]( auto _ ) { return dist( rd ); } );
		blocks.emplace_back( std::move( raw_nv12 ) );
	}
}

void accept_blocks( Compressor &compressor, vector<vector<char>> &blocks, vector<tuple<size_t, size_t, size_t>> const &idx )
{
	for ( int i = 0; i != blocks.size(); ++i ) {
		auto &block = blocks[ i ];
		auto acc = compressor.accept(
		  vm::Arc<Reader>( new SliceReader( block.data(), block.size() ) ) );
		EXPECT_GE( acc.last_frame, acc.first_frame );
		EXPECT_EQ( acc.first_frame, std::get<0>( idx[ i ] ) );
		EXPECT_EQ( acc.last_frame, std::get<1>( idx[ i ] ) );
		EXPECT_EQ( acc.offset, std::get<2>( idx[ i ] ) );
	}
	blocks.clear();
}

TEST( test_video_compress, compress_simple )
{
	const auto width = 256;
	const auto height = 256;
	const auto frame_size = width * height * 3 / 2;
	const auto batch_frames = 2;
	const auto block_size = frame_size;  // exactly one block per frame

	vector<char> compressed;
	UnboundedVectorWriter compressed_writer( compressed );

	vector<vector<char>> raw_input_blocks;

	auto comp_opts = CompressOptions{}
					   .set_encode_method( EncodeMethod::H264 )
					   .set_encode_preset( EncodePreset::Default )
					   .set_pixel_format( PixelFormat::IYUV )
					   .set_width( width )
					   .set_height( height )
					   .set_batch_frames( batch_frames );
	Compressor compressor( compressed_writer, comp_opts );
	ASSERT_EQ( compressor.frame_size(), frame_size );

	add_blocks( raw_input_blocks, block_size, 1 );
	accept_blocks( compressor, raw_input_blocks,
				   { { 0, 0, 0 } } );
	compressor.flush( true );
	ASSERT_EQ( compressor.frame_count(), 1 );

	add_blocks( raw_input_blocks, block_size, 3 );
	accept_blocks( compressor, raw_input_blocks,
				   { { 1, 1, 0 }, { 2, 2, 0 }, { 3, 3, 0 } } );
	compressor.flush( true );
	compressor.flush( true );
	ASSERT_EQ( compressor.frame_count(), 4 );

	compressor.wait();
	ASSERT_EQ( compressor.frame_count(), 4 );
}

TEST( test_video_compression, compress_large_frame )
{
	const auto width = 1024;
	const auto height = 1024;
	const auto frame_size = width * height * 3 / 2;
	const auto batch_frames = 1;
	const auto block_size = 678;

	const auto nblocks = ( frame_size * batch_frames + block_size - 1 ) / block_size;

	vector<char> compressed;
	UnboundedVectorWriter compressed_writer( compressed );

	vector<vector<char>> raw_input_blocks;

	auto comp_opts = CompressOptions{}
					   .set_encode_method( EncodeMethod::H264 )
					   .set_encode_preset( EncodePreset::Default )
					   .set_pixel_format( PixelFormat::IYUV )
					   .set_width( width )
					   .set_height( height )
					   .set_batch_frames( batch_frames );
	Compressor compressor( compressed_writer, comp_opts );
	ASSERT_EQ( compressor.frame_size(), frame_size );

	add_blocks( raw_input_blocks, block_size, nblocks - 1 );
	vector<tuple<size_t, size_t, size_t>> idx;
	for ( int i = 0; i != nblocks - 1; ++i ) {
		idx.emplace_back( 0, 0, block_size * i );
	}
	accept_blocks( compressor, raw_input_blocks, idx );
	compressor.flush( true );
	ASSERT_EQ( compressor.frame_count(), 0 );
	compressor.wait();
	ASSERT_EQ( compressor.frame_count(), 1 );

	add_blocks( raw_input_blocks, block_size, nblocks * 3 );
	idx.clear();
	for ( int i = 0; i != nblocks * 3; ++i ) {
		idx.emplace_back(
		  1 + block_size * i / frame_size,
		  1 + ( block_size * ( i + 1 ) + frame_size - 1 ) / frame_size - 1,
		  block_size * i % frame_size );
	}
	accept_blocks( compressor, raw_input_blocks, idx );
	compressor.wait();
	ASSERT_EQ( compressor.frame_count(), 5 );
	// vector<uint32_t> idx( compressor.frame_len() );
	// vm::println( "{}", idx );
}
