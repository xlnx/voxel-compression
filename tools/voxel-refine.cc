#include <iostream>
#include <string>
#include <VMUtils/cmdline.hpp>
#include <vocomp/refine/refiner.hpp>
#ifdef VOCOMP_BUILD_VIDEO_COMPRESS
#include <vocomp/video/compressor.hpp>
#endif

using namespace std;

int main( int argc, char **argv )
{
	cmdline::parser a;
	a.add<string>( "if", 'i', ".raw input filename", true );
	a.add<int>( "x", 'x', "raw.x", true );
	a.add<int>( "y", 'y', "raw.y", true );
	a.add<int>( "z", 'z', "raw.z", true );
	a.add<size_t>( "memlimit", 'm', "maximum memory limit in gb", false, 128 );
	a.add<int>( "padding", 'p', "padding of the block, just support for 0, 1 or 2. the default value is 2", false, 2, cmdline::oneof<int>( 0, 1, 2 ) );
	a.add<int>( "side", 's', "the side length of the block, which is represented in logarithm. The Default value is 6.", false, 6, cmdline::oneof<int>( 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 ) );
	a.add<string>( "compression", 'c', "block compression method: h264/hevc/lz4/none", false, "h264", cmdline::oneof<string>( "h264", "hevc", "lz4", "none" ) );
	a.add<string>( "device", 'd', "video compression device: default/cuda/graphics", false, "default", cmdline::oneof<string>( "default", "cuda", "graphics" ) );
	a.add<string>( "of", 'o', "output filename", true );

	//cout<<a.usage();
	a.parse_check( argc, argv );

	auto input = a.get<string>( "if" );
	auto output = a.get<string>( "of" );
	auto x = a.get<int>( "x" );
	auto y = a.get<int>( "y" );
	auto z = a.get<int>( "z" );
	auto padding = a.get<int>( "padding" );
	auto log = a.get<int>( "side" );
	auto comp = a.get<string>( "compression" );
	auto dev = a.get<string>( "device" );
	auto mem = a.get<size_t>( "memlimit" );

	try {
		size_t frame_len = 0;
		shared_ptr<vol::Pipe> pipe;

		if ( comp == "h264" || comp == "hevc" ) {
#ifdef VOCOMP_BUILD_VIDEO_COMPRESS
			size_t block_size = 1 << log;
			auto width = block_size * block_size / 2;
			auto height = block_size;
			if ( height < 64 ) {
				width *= 64 / height;
				height = 64;
			}
			while ( width > 4096 ) {
				width = width >> 1;
			}
			while ( height > 4096 ) {
				height = height >> 1;
			}
			frame_len = width * height * 3 / 2;

			auto opts = vol::video::CompressOptions{}
						  .set_encode_preset( vol::video::EncodePreset::Default )
						  .set_width( width )
						  .set_height( height )
						  .set_pixel_format( vol::video::PixelFormat::NV12 );
			if ( comp == "h264" ) {
				opts.set_encode_method( vol::video::EncodeMethod::H264 );
			} else {
				opts.set_encode_method( vol::video::EncodeMethod::HEVC );
			}
			if ( dev == "cuda" ) {
				opts.set_device( vol::video::CompressDevice::Cuda );
			} else if ( dev == "graphics" ) {
				opts.set_device( vol::video::CompressDevice::Graphics );
			}
			pipe = make_shared<vol::video::Compressor>( opts );
#else
			throw runtime_error( "this tool is built without video compression support" );
#endif
		} else if ( comp == "lz4" ) {
			throw runtime_error( "unimplemented lz4" );
		} else if ( comp != "none" ) {
			throw "unreachable";
		}

		vm::println( "using compression method: {}", comp );
		output = vm::fmt( "{}.{}.comp", output, comp );

		{
			auto opts = vol::refine::RefinerOptions{}
						  .set_x( x )
						  .set_y( y )
						  .set_z( z )
						  .set_log_block_size( log )
						  .set_padding( padding )
						  .set_input( input )
						  .set_output( output );
			vol::refine::Refiner refiner( opts );
			{
				auto opts = vol::refine::ConvertOptions{}
							  .set_frame_len( frame_len )
							  .set_suggest_mem_gb( mem )
							  .set_pipe( pipe );
				refiner.convert( opts );
			}

			vm::println( "written to {}", output );
		}
	} catch ( exception &e ) {
		vm::eprintln( "{}", e.what() );
	}
}
