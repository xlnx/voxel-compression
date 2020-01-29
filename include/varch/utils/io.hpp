#pragma once

#include <functional>
#include <fstream>
#include <vector>
#include <map>
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
		template <typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
		size_t read_typed( T &dst )
		{
			return read( reinterpret_cast<char *>( &dst ), sizeof( T ) );
		}
		template <typename T>
		size_t read_typed( std::vector<T> &vec )
		{
			uint64_t len;
			auto nread = read_typed( len );
			vec.resize( len );
			nread += read( reinterpret_cast<char *>( vec.data() ), sizeof( T ) * len );
			return nread;
		}
		template <typename K, typename V>
		size_t read_typed( std::map<K, V> &map )
		{
			uint64_t len;
			auto nread = read_typed( len );
			for ( uint64_t i = 0; i != len; ++i ) {
				std::pair<K, V> entry;
				nread += read_typed( entry.first );
				nread += read_typed( entry.second );
				map.insert( entry );
			}
			return nread;
		}
	};

	struct Writer : RandomIO
	{
		virtual void write( char const *src, size_t len ) = 0;
		template <typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
		void write_typed( T const &src )
		{
			write( reinterpret_cast<char const *>( &src ), sizeof( T ) );
		}
		template <typename T>
		void write_typed( std::vector<T> const &vec )
		{
			uint64_t len = vec.size();
			write_typed( len );
			write( reinterpret_cast<char const *>( vec.data() ), sizeof( T ) * len );
		}
		template <typename K, typename V>
		void write_typed( std::map<K, V> &map )
		{
			uint64_t len = map.size();
			write_typed( len );
			for ( auto &entry : map ) {
				write_typed( entry.first );
				write_typed( entry.second );
			}
		}
	};

	struct PartReader : Reader
	{
		PartReader( Reader &_, size_t offset, size_t len ) :
		  _( _ ),
		  offset( offset ),
		  len( len )
		{
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
}

VM_END_MODULE()
