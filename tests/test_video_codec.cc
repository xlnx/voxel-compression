// #include <random>
// #include <gtest/gtest.h>
// #include <archive/video_compressor.hpp>
// #include <varch/video_decompressor.hpp>
// #include <varch/utils/unbounded_vector_writer.hpp>

// using namespace vol;
// using namespace std;
// using namespace __inner__;

// void add_blocks( vector<vector<char>> &blocks, size_t block_size, int nblocks = 1 )
// {
// 	static random_device rd;
// 	static uniform_int_distribution<char> dist( 0, 127 );
// 	for ( int i = 0; i != nblocks; ++i ) {
// 		vector<char> raw_nv12( block_size );
// 		std::transform( raw_nv12.begin(), raw_nv12.end(), raw_nv12.begin(),
// 						[&]( auto _ ) { return dist( rd ); } );
// 		blocks.emplace_back( std::move( raw_nv12 ) );
// 	}
// }

// void accept_blocks( VideoCompressor &video_compressor, vector<vector<char>> &blocks, vector<tuple<size_t, size_t, size_t>> const &idx )
// {
// 	for ( int i = 0; i != blocks.size(); ++i ) {
// 		auto &block = blocks[ i ];
// 		auto acc = video_compressor.accept(
// 		  vm::Arc<Reader>( new SliceReader( block.data(), block.size() ) ) );
// 		EXPECT_GE( acc.last_frame, acc.first_frame );
// 		EXPECT_EQ( acc.first_frame, std::get<0>( idx[ i ] ) );
// 		EXPECT_EQ( acc.last_frame, std::get<1>( idx[ i ] ) );
// 		EXPECT_EQ( acc.offset, std::get<2>( idx[ i ] ) );
// 	}
// 	blocks.clear();
// }

// TEST( test_video_codec, simple )
// {
// 	const auto width = 256;
// 	const auto height = 256;
// 	const auto frame_size = width * height * 3 / 2;
// 	const auto batch_frames = 2;
// 	const auto block_size = frame_size;  // exactly one block per frame

// 	vector<char> compressed;
// 	UnboundedVectorWriter compressed_writer( compressed );

// 	vector<vector<char>> raw_input_blocks;

// 	auto comp_opts = EncodeOptions{}
// 					   .set_encode_preset( EncodePreset::Default )
// 					   .set_width( width )
// 					   .set_height( height )
// 					   .set_batch_frames( batch_frames );
// 	VideoCompressor video_compressor( compressed_writer, comp_opts );
// 	ASSERT_EQ( video_compressor.frame_size(), frame_size );

// 	add_blocks( raw_input_blocks, block_size, 1 );
// 	accept_blocks( video_compressor, raw_input_blocks,
// 				   { { 0, 0, 0 } } );
// 	video_compressor.flush( true );
// 	ASSERT_EQ( video_compressor.frame_count(), 1 );

// 	add_blocks( raw_input_blocks, block_size, 3 );
// 	accept_blocks( video_compressor, raw_input_blocks,
// 				   { { 1, 1, 0 }, { 2, 2, 0 }, { 3, 3, 0 } } );
// 	video_compressor.flush( true );
// 	video_compressor.flush( true );
// 	ASSERT_EQ( video_compressor.frame_count(), 4 );

// 	video_compressor.wait();
// 	ASSERT_EQ( video_compressor.frame_count(), 4 );

// 	auto frame_offset = video_compressor.frame_offset();

// 	NvDecoderAsync deVideoCompressor;
// 	auto opts = DecodeOptions{}
// 				  .set_io_queue_size( 1 );
// 	auto data_ptr = compressed.data();

// 	{
// 		SliceReader first_frame(
// 		  data_ptr + frame_offset[ 0 ],
// 		  frame_offset[ 1 ] - frame_offset[ 0 ] );
// 		vector<size_t> buffer_size;
// 		deVideoCompressor.decode(
// 		  first_frame,
// 		  [&]( NvBitStreamPacket const &packet ) {
// 			  buffer_size.emplace_back( packet.length );
// 		  } );
// 		ASSERT_EQ( buffer_size, vector<size_t>{ frame_size } );
// 	}
// 	{
// 		SliceReader first_frame(
// 		  data_ptr + frame_offset[ 0 ],
// 		  frame_offset[ 1 ] - frame_offset[ 0 ] );
// 		vector<size_t> buffer_size;
// 		deVideoCompressor.decode(
// 		  first_frame,
// 		  [&]( NvBitStreamPacket const &packet ) {
// 			  buffer_size.emplace_back( packet.length );
// 		  } );
// 		ASSERT_EQ( buffer_size, vector<size_t>{ frame_size } );
// 	}
// 	{
// 		SliceReader frame_1_3(
// 		  data_ptr + frame_offset[ 1 ],
// 		  frame_offset[ 4 ] - frame_offset[ 1 ] );
// 		vector<size_t> buffer_size;
// 		deVideoCompressor.decode(
// 		  frame_1_3,
// 		  [&]( NvBitStreamPacket const &packet ) {
// 			  buffer_size.emplace_back( packet.length );
// 		  } );
// 		ASSERT_EQ( buffer_size, ( vector<size_t>{ frame_size, frame_size, frame_size } ) );
// 	}
// 	{
// 		SliceReader frame_0_3(
// 		  data_ptr + frame_offset[ 0 ],
// 		  frame_offset[ 4 ] - frame_offset[ 0 ] );
// 		vector<size_t> buffer_size;
// 		deVideoCompressor.decode(
// 		  frame_0_3,
// 		  [&]( NvBitStreamPacket const &packet ) {
// 			  buffer_size.emplace_back( packet.length );
// 		  } );
// 		ASSERT_EQ( buffer_size, ( vector<size_t>{ frame_size, frame_size, frame_size, frame_size } ) );
// 	}
// }

// TEST( test_video_codec, compress_large_frame )
// {
// 	const auto width = 1024;
// 	const auto height = 1024;
// 	const auto frame_size = width * height * 3 / 2;
// 	const auto batch_frames = 1;
// 	const auto block_size = 678;

// 	const auto nblocks = ( frame_size * batch_frames + block_size - 1 ) / block_size;

// 	vector<char> compressed;
// 	UnboundedVectorWriter compressed_writer( compressed );

// 	vector<vector<char>> raw_input_blocks;

// 	auto comp_opts = EncodeOptions{}
// 					   .set_encode_preset( EncodePreset::Default )
// 					   .set_width( width )
// 					   .set_height( height )
// 					   .set_batch_frames( batch_frames );
// 	VideoCompressor video_compressor( compressed_writer, comp_opts );
// 	ASSERT_EQ( video_compressor.frame_size(), frame_size );

// 	add_blocks( raw_input_blocks, block_size, nblocks - 1 );
// 	vector<tuple<size_t, size_t, size_t>> idx;
// 	for ( int i = 0; i != nblocks - 1; ++i ) {
// 		idx.emplace_back( 0, 0, block_size * i );
// 	}
// 	accept_blocks( video_compressor, raw_input_blocks, idx );
// 	video_compressor.flush( true );
// 	ASSERT_EQ( video_compressor.frame_count(), 0 );
// 	video_compressor.wait();
// 	ASSERT_EQ( video_compressor.frame_count(), 1 );

// 	add_blocks( raw_input_blocks, block_size, nblocks * 3 );
// 	idx.clear();
// 	for ( int i = 0; i != nblocks * 3; ++i ) {
// 		idx.emplace_back(
// 		  1 + block_size * i / frame_size,
// 		  1 + ( block_size * ( i + 1 ) + frame_size - 1 ) / frame_size - 1,
// 		  block_size * i % frame_size );
// 	}
// 	accept_blocks( video_compressor, raw_input_blocks, idx );
// 	video_compressor.wait();
// 	ASSERT_EQ( video_compressor.frame_count(), 5 );
// 	// vector<uint32_t> idx( video_compressor.frame_len() );
// 	// vm::println( "{}", idx );
// }
