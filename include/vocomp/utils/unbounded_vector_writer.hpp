#include <vector>
#include <numeric>
#include <vocomp/utils/io.hpp>
#include <vocomp/utils/unbounded_io.hpp>

VM_BEGIN_MODULE( vol )

using namespace std;

VM_EXPORT
{
	struct UnboundedVectorWriter : UnboundedWriter
	{
		UnboundedVectorWriter( vector<char> &data ) :
		  data( data )
		{
		}
		void seek( size_t pos ) override
		{
			idx = pos;
		}
		size_t tell() const override
		{
			return idx;
		}
		void write( char const *src, size_t slen ) override
		{
			if ( data.size() < slen + idx ) {
				if ( data.capacity() < slen + idx ) {
					data.resize( slen + idx );
				} else {
					vector<char> data_buf;
					data_buf.reserve( ( idx + slen ) * 2 );
					data_buf.resize( idx + slen );
					data.swap( data_buf );
					memcpy( data.data(), data_buf.data(), idx * sizeof( char ) );
				}
			}
			memcpy( data.data() + idx, src, slen );
			idx += slen;
		}

	private:
		vector<char> &data;
		int idx = 0;
	};
}

VM_END_MODULE()
