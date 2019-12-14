#include <vector>
#include <numeric>
#include <vocomp/io.hpp>

namespace vol
{
VM_BEGIN_MODULE( video )

using namespace std;

VM_EXPORT
{
	struct LinkedReader : Reader
	{
		LinkedReader( vector<vm::Arc<Reader>> const &readers ) :
		  _( readers )
		{
		}

		void seek( size_t pos ) override
		{
			size_t len = 0;
			for ( int i = 0; i != _.size(); ++i ) {
				auto size = _[ i ]->size();
				if ( len + size > pos ) {
					idx = i;
					_[ idx ]->seek( pos - len );
					for ( int j = i + 1; j < _.size(); ++j ) {
						_[ j ]->seek( 0 );
					}
					return;
				}
				len += size;
			}
		}
		size_t tell() const override
		{
			if ( idx == _.size() ) return size();
			return _[ idx ]->tell() +
				   accumulate(
					 _.begin(), _.begin() + idx, 0,
					 []( size_t len, vm::Arc<Reader> const &reader ) { return len + reader->size(); } );
		}
		size_t size() const override
		{
			return accumulate(
			  _.begin(), _.end(), 0,
			  []( size_t len, vm::Arc<Reader> const &reader ) { return len + reader->size(); } );
		}
		size_t read( char *dst, size_t dlen ) override
		{
			if ( idx == _.size() ) return 0;
			size_t len = 0;
			while ( true ) {
				len += _[ idx ]->read( dst + len, dlen - len );
				if ( len < dlen && idx < _.size() ) {
					if ( ++idx == _.size() ) {
						break;
					}
				} else {
					break;
				}
			}
			return len;
		}

	private:
		vector<vm::Arc<Reader>> _;
		size_t idx = 0;
	};
}

VM_END_MODULE()

}  // namespace vol