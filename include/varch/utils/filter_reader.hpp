#include <vector>
#include <numeric>
#include "io.hpp"

VM_BEGIN_MODULE( vol )

using namespace std;

VM_EXPORT
{
	struct FilterReader : Reader
	{
		FilterReader( size_t len, char filter = '\0' ) :
		  len( len ),
		  filter( filter ) {}

		void seek( size_t pos ) override { p = pos; }
		size_t tell() const override { return p; }
		size_t size() const override { return len; }
		size_t read( char *dst, size_t dlen ) override
		{
			auto nread = std::min( dlen, len - p );
			memset( dst, filter, sizeof( char ) * nread );
			return nread;
		}

	private:
		size_t p = 0, len;
		char filter;
	};
}

VM_END_MODULE()