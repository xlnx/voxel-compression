#include <fstream>
#include <video/compressor.hpp>
#include <voxel/compressor.hpp>
#include <unbounded_io.hpp>
#include "cxxopts.hpp"

using namespace std;
using namespace vol;

int main( int argc, char **argv )
{
	cxxopts::Options options( "voprod", "Produce compressed random voxel data" );
	options.add_options()(
	  "o,output", "put compressed data into <file>", cxxopts::value<string>() )(
	  "h,help", "print this help message" )(
	  "x,block_x", "block.x", cxxopts::value<size_t>()->default_value( "1024" ) )(
	  "y,block_y", "block.y", cxxopts::value<size_t>()->default_value( "1024" ) )(
	  "z,block_z", "block.z", cxxopts::value<size_t>()->default_value( "1024" ) )(
	  "X,dim_x", "dim.x", cxxopts::value<size_t>()->default_value( "2" ) )(
	  "Y,dim_y", "dim.y", cxxopts::value<size_t>()->default_value( "2" ) )(
	  "Z,dim_z", "dim.z", cxxopts::value<size_t>()->default_value( "2" ) );

	auto opts = options.parse( argc, argv );
	if ( opts.count( "h" ) ) {
		vm::println( "{}", options.help() );
		return 0;
	}

	auto out = opts[ "o" ].as<string>();
	ofstream os( out, std::ios::binary );
	UnboundedStreamWriter writer( os );

	auto dim = voxel::Idx{}
				 .set_x( opts[ "X" ].as<size_t>() )
				 .set_y( opts[ "Y" ].as<size_t>() )
				 .set_z( opts[ "Z" ].as<size_t>() );
	auto block = voxel::Idx{}
				   .set_x( opts[ "x" ].as<size_t>() )
				   .set_y( opts[ "y" ].as<size_t>() )
				   .set_z( opts[ "z" ].as<size_t>() );

	{
		video::Compressor video;
		voxel::Compressor<> comp( dim, writer, video );

		auto buf = std::unique_ptr<char[]>( new char[ block.total() ] );

		voxel::Idx idx;
		for ( idx.z = 0; idx.z != dim.z; ++idx.z ) {
			for ( idx.y = 0; idx.y != dim.y; ++idx.y ) {
				for ( idx.x = 0; idx.x != dim.x; ++idx.x ) {
					SliceReader reader( buf.get(), block.total() );
					comp.put( idx, reader );
				}
			}
		}
	}
}
