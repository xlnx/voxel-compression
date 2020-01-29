#include <iostream>
#include <string>
#include <VMUtils/cmdline.hpp>
#include <varch/archive/archiver.hpp>

#ifdef WIN32
#include <windows.h>
unsigned long long get_system_memory()
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof( status );
	GlobalMemoryStatusEx( &status );
	return status.ullTotalPhys;
}
#else
#include <unistd.h>
unsigned long long get_system_memory()
{
	long pages = sysconf( _SC_PHYS_PAGES );
	long page_size = sysconf( _SC_PAGE_SIZE );
	return pages * page_size;
}
#endif

using namespace std;
using namespace vol;

int main( int argc, char **argv )
{
	auto system_memory_gb = get_system_memory() / 1024 /*kb*/ / 1024 /*mb*/ / 1024 /*gb*/;

	cmdline::parser a;
	a.add<string>( "if", 'i', ".raw input filename", true );
	a.add<int>( "x", 'x', "raw.x", true );
	a.add<int>( "y", 'y', "raw.y", true );
	a.add<int>( "z", 'z', "raw.z", true );
	a.add<size_t>( "memlimit", 'm', "maximum memory limit in gb", false, system_memory_gb / 2 );
	a.add<int>( "padding", 'p', "block padding", false, 2, cmdline::oneof<int>( 0, 1, 2 ) );
	a.add<int>( "side", 's', "block size in log(voxel)", false, 6, cmdline::oneof<int>( 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 ) );
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
	auto dev = a.get<string>( "device" );
	auto mem = a.get<size_t>( "memlimit" );

	try {
		auto opts = ArchiverOptions{}
					  .set_x( x )
					  .set_y( y )
					  .set_z( z )
					  .set_log_block_size( log )
					  .set_padding( padding )
					  .set_suggest_mem_gb( mem )
					  .set_input( input );

		auto &compress_opts = opts.compress_opts;
		compress_opts = VideoCompressOptions{}
						  .set_encode_preset( EncodePreset::Default )
						  .set_width( 1024 )
						  .set_height( 1024 )
						  .set_batch_frames( 16 );
		if ( dev == "cuda" ) {
			compress_opts.set_device( CompressDevice::Cuda );
		} else if ( dev == "graphics" ) {
			compress_opts.set_device( CompressDevice::Graphics );
		}

		opts.set_output( vm::fmt( "{}.h264", output ) );

		{
			Archiver archiver( opts );

			archiver.convert();

			vm::println( "written to {}", output );
		}
	} catch ( exception &e ) {
		vm::eprintln( "{}", e.what() );
	}
}
