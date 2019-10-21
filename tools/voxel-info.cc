#include <fstream>
#include <VMUtils/fmt.hpp>
#include <vocomp/refine/extractor.hpp>
#include <vocomp/io.hpp>

using namespace std;

int main( int argc, char **argv )
{
	try {
		ifstream in( argv[ 1 ], std::ios::ate | std::ios::binary );
		auto len = in.tellg();
		vol::StreamReader reader( in, 0, len );
		vol::refine::Extractor e( reader );

		if ( argc <= 2 ) {
			vm::println( "{>16}: {}", "Size", e.raw() );
			vm::println( "{>16}: {}", "Padded Size", e.adjusted() );
			vm::println( "{>16}: {}", "Grid Size", e.dim() );
			vm::println( "{>16}: {} = 2^{}", "Block Size", e.block_size(), e.log_block_size() );
			vm::println( "{>16}: {}", "Padding", e.padding() );
		} else {
			vm::println( "{}", e.index() );
		}

	} catch ( exception &e ) {
		vm::eprintln( "{}", e.what() );
	}
}
