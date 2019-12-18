#include <fstream>
#include "cxxopts.hpp"
#include <VMUtils/fmt.hpp>
#include <vocomp/refine/extractor.hpp>
#include <vocomp/io.hpp>

using namespace std;

int main( int argc, char **argv )
{
	// cxxopts::Options options( "voxel-info", "Print voxel info" );
	// options.add_options()(
	//   "i,input", "input compressed file", cxxopts::value<string>() )(
	//   "h,help", "print this help message" )(
	//   "l,list", "list indices" );

	// auto opts = options.parse( argc, argv );
	// if ( opts.count( "h" ) || !opts.count( "i" ) ) {
	// 	vm::println( "{}", options.help() );
	// 	return 0;
	// }

	// try {
	// 	ifstream in( opts[ "i" ].as<string>(), std::ios::ate | std::ios::binary );
	// 	auto len = in.tellg();
	// 	vol::StreamReader reader( in, 0, len );
	// 	vol::refine::Extractor e( reader );

	// 	if ( !opts.count( "l" ) ) {
	// 		vm::println( "{>16}: {}", "Size", e.raw() );
	// 		vm::println( "{>16}: {}", "Padded Size", e.adjusted() );
	// 		vm::println( "{>16}: {}", "Grid Size", e.dim() );
	// 		vm::println( "{>16}: {} = 2^{}", "Block Size", e.block_size(), e.log_block_size() );
	// 		vm::println( "{>16}: {}", "Padding", e.padding() );
	// 	} else {
	// 		vm::println( "{}", e.index() );
	// 	}

	// } catch ( exception &e ) {
	// 	vm::eprintln( "{}", e.what() );
	// }
}
