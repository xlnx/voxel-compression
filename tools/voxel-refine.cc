#include <iostream>
#include <string>
#include <VMUtils/cmdline.hpp>
#include <vocomp/refine/refiner.hpp>
#include <vocomp/video/compressor.hpp>

int main( int argc, char **argv )
{
	cmdline::parser a;
	a.add<std::string>( "if", 'i', ".raw input filename", true );
	a.add<int>( "width", 'w', "width of the raw file", true );
	a.add<int>( "height", 'h', "height of the raw file", true );
	a.add<int>( "depth", 'd', "depth of the raw file", true );
	a.add<std::size_t>( "memlimit", 'm', "maximum memory limit in Gb", false, 128 );
	a.add<int>( "padding", 'p', "padding of the block, just support for 0, 1 or 2. the default value is 2", false, 2, cmdline::oneof<int>( 0, 1, 2 ) );
	a.add<int>( "side", 's', "the side length of the block, which is represented in logarithm. The Default value is 6.", false, 6, cmdline::oneof<int>( 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 ) );
	a.add<std::string>( "of", 'f', ".lvd output filename", true );

	//std::cout<<a.usage();
	a.parse_check( argc, argv );

	auto input = a.get<std::string>( "if" );
	auto output = a.get<std::string>( "of" );
	auto x = a.get<int>( "width" );
	auto y = a.get<int>( "height" );
	auto z = a.get<int>( "depth" );
	auto repeat = a.get<int>( "padding" );
	auto log = a.get<int>( "side" );
	auto mem = a.get<std::size_t>( "memlimit" );

	try {
		std::size_t block_size = 1 << log;
		auto width = block_size * block_size / 4;
		auto height = block_size;
		while ( width > 4096 ) {
			width = width >> 1;
		}
		while ( height > 4096 ) {
			height = height >> 1;
		}

		auto opts = vol::video::CompressOptions{}
					  .set_encode_method( vol::video::EncodeMethod::H264 )
					  .set_encode_preset( vol::video::EncodePreset::LowLatencyDefault )
					  .set_width( width )
					  .set_height( height )
					  .set_pixel_format( vol::video::PixelFormat::ARGB );
		vol::video::Compressor comp( opts );

		{
			auto opts = vol::refine::RefinerOptions{}
						  .set_x( x )
						  .set_y( y )
						  .set_z( z )
						  .set_log_block_size( log )
						  .set_input( input )
						  .set_output( output );
			vol::refine::Refiner refiner( opts );
			refiner.convert( comp, mem );
		}
	} catch ( std::exception &e ) {
		std::cout << e.what() << std::endl;
	}
}
