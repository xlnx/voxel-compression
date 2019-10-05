#include <fstream>
#include <video/compressor.hpp>
#include <voxel/compressor.hpp>
#include <unbounded_io.hpp>

using namespace std;
using namespace vol;

int main( int argc, char **argv )
{
	auto in = argv[ 1 ];
	auto out = argv[ 2 ];

	ifstream is( in, std::ios::ate | std::ios::binary );
	ofstream os( out, std::ios::binary );

	{
		video::Compressor video;
		voxel::Compressor<> comp( voxel::Dim3{}, writer, video );

		for ( int i = 0; i != 1; ++i ) {
			auto idx = voxel::S3{};
			comp.put( idx, reader );
		}
	}
}
