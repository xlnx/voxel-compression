#pragma once

#include <VMUtils/modules.hpp>
#include "../io.hpp"
#include "../video/decompressor.hpp"

VM_BEGIN_MODULE( vol )

using namespace std;

VM_EXPORT
{
	struct PipeFactory
	{
		static Pipe *create( const string &file_name )
		{
			auto p1 = file_name.find_last_of( '.' );
			auto p2 = file_name.find_last_of( '.', p1 - 1 );
			auto method = file_name.substr( p2 + 1, p1 - p2 - 1 );

			if ( method == "h264" || method == "hevc" ) {
				auto opts = vol::video::DecompressorOptions{};
				if ( method == "h264" ) {
					opts.encode = video::EncodeMethod::H264;
				} else {
					opts.encode = video::EncodeMethod::HEVC;
				}
				return new video::Decompressor( opts );
			} else if ( method == "none" ) {
				return new Copy();
			} else {
				throw std::logic_error( vm::fmt( "unrecognized decompression method: {}", method ) );
			}
		}
	};
}

VM_END_MODULE()
