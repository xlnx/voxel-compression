
#include <string>
#include <mapper.hpp>

using namespace std;

int main( int argc, char **argv )
{
	ifstream is( "a.mpeg", std::ios::ate | std::ios::binary );
	auto mpeg_len = is.tellg();
	vm::println( "{}", mpeg_len );
	vol::FstreamReader reader( is, 0, mpeg_len );

	ofstream os( "a.yuv", std::ios::binary );
	vol::FstreamWriter writer( os, 0, 0 );

	vol::decompress( reader, writer );
}
