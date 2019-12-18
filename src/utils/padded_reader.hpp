#include <vector>
#include <numeric>
#include <vocomp/io.hpp>

VM_BEGIN_MODULE( vol )

using namespace std;

VM_EXPORT
{
	struct PaddedReader : Reader
	{
		PaddedReader( Reader &_, size_t len, char fill = 0 ) :
		  _( _ ),
		  len( len ),
		  fill( fill )
		{
		}
		void seek( size_t pos ) override
		{
			_.seek( pos );
		}
		size_t tell() const override
		{
			return _.tell() + p;
		}
		size_t size() const override
		{
			return len;
		}
		size_t read( char *dst, size_t dlen ) override
		{
			auto nread = _.read( dst, dlen );
			auto remain = len - _.size() - p;
			if ( nread < dlen && remain > 0 ) {
				auto nfill = std::min( dlen - nread, remain );
				p += nfill;
				memset( dst + nread, fill, sizeof( fill ) * nfill );
				nread += nfill;
			}
			return nread;
		}

	private:
		Reader &_;
		size_t len;
		size_t p = 0;
		char fill;
	};
}

VM_END_MODULE()
