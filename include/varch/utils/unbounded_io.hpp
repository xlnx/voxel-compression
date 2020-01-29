#pragma once

#include "io.hpp"

VM_BEGIN_MODULE( vol )

using namespace std;

VM_EXPORT
{
	struct UnboundedWriter : Writer
	{
		size_t size() const override
		{
			return -1;
		}
	};

	struct UnboundedStreamWriter : UnboundedWriter
	{
		UnboundedStreamWriter( ostream &os, size_t offset = 0 ) :
		  os( os ),
		  offset( offset )
		{
			os.seekp( offset );
		}

		void seek( size_t pos ) override
		{
			os.seekp( pos + offset );
		}
		size_t tell() const override
		{
			return size_t( os.tellp() ) - offset;
		}
		void write( char const *src, size_t slen ) override
		{
			os.write( src, slen );
		}

	private:
		ostream &os;
		size_t offset;
	};
}

VM_END_MODULE()
