#pragma once

#include <functional>
#include <fstream>
#include <cstring>
#include <algorithm>

#include <VMUtils/fmt.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/modules.hpp>

VM_BEGIN_MODULE( vol )

using namespace std;

VM_EXPORT
{
	struct RandomIO : vm::Dynamic
	{
		virtual void seek( size_t pos ) = 0;
		virtual size_t tell() const = 0;
		virtual size_t size() const = 0;
	};

	struct Reader : RandomIO
	{
		virtual size_t read( char *dst, size_t len ) = 0;
	};

	struct Writer : RandomIO
	{
		virtual void write( char const *src, size_t len ) = 0;
	};

	struct PartReader : Reader
	{
		PartReader( Reader &_, size_t offset, size_t len ) :
		  _( _ ),
		  offset( offset ),
		  len( len )
		{
			_.seek( offset );
		}

		void seek( size_t pos ) override
		{
			_.seek( pos + offset );
		}
		size_t tell() const override
		{
			return _.tell() - offset;
		}
		size_t size() const override
		{
			return len;
		}
		size_t read( char *dst, size_t dlen ) override
		{
			return _.read( dst, std::min( dlen, len - tell() ) );
		}

	private:
		Reader &_;
		size_t offset;
		size_t len;
	};
	struct SliceReader : Reader
	{
		SliceReader( char const *src, size_t slen ) :
		  src( src ),
		  slen( slen )
		{
		}

		void seek( size_t pos ) override
		{
			p = pos;
		}
		size_t tell() const override
		{
			return p;
		}
		size_t size() const override
		{
			return slen;
		}
		size_t read( char *dst, size_t dlen ) override
		{
			auto nread = std::min( slen - p, dlen );
			memcpy( dst, src + p, nread );
			p += nread;
			return nread;
		}

	private:
		char const *src;
		size_t p = 0;
		size_t slen;
	};
	struct StreamReader : Reader
	{
		StreamReader( istream &is, size_t offset, size_t slen ) :
		  is( is ),
		  offset( offset ),
		  send( slen + offset )
		{
			is.seekg( offset );
		}

		void seek( size_t pos ) override
		{
			is.seekg( pos + offset );
		}
		size_t tell() const override
		{
			return size_t( is.tellg() ) - offset;
		}
		size_t size() const override
		{
			return send - offset;
		}
		size_t read( char *dst, size_t dlen ) override
		{
			auto nread = std::min( send - is.tellg(), dlen );
			nread = is.read( dst, nread ).gcount();
			return nread;
		}

	private:
		istream &is;
		size_t offset;
		size_t send;
	};

	struct SliceWriter : Writer
	{
		SliceWriter( char *dst, size_t dlen ) :
		  dst( dst ),
		  dlen( dlen )
		{
		}

		void seek( size_t pos ) override
		{
			p = pos;
		}
		size_t tell() const override
		{
			return p;
		}
		size_t size() const override
		{
			return dlen;
		}
		void write( char const *src, size_t slen ) override
		{
			auto nwrite = std::min( slen, dlen - p );
			memcpy( dst + p, src, nwrite );
			p += nwrite;
		}

	private:
		char *dst;
		size_t p = 0;
		size_t dlen;
	};
	struct StreamWriter : Writer
	{
		StreamWriter( ostream &os, size_t offset, size_t dlen ) :
		  os( os ),
		  offset( offset ),
		  dend( dlen + offset )
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
		size_t size() const override
		{
			return dend - offset;
		}
		void write( char const *src, size_t slen ) override
		{
			auto nwrite = std::min( slen, dend - os.tellp() );
			os.write( src, nwrite );
		}

	private:
		ostream &os;
		size_t offset;
		size_t dend;
	};

	struct Pipe : vm::Dynamic
	{
		virtual void transfer( Reader &reader, Writer &writer ) = 0;
	};

	struct Copy : Pipe
	{
		virtual void transfer( Reader &reader, Writer &writer ) override
		{
			static char _[ 4096 ];
			while ( auto nread = reader.read( _, sizeof( _ ) ) ) {
				writer.write( _, nread );
			}
		}
	};
}

VM_END_MODULE()
