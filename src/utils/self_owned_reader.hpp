#include <vector>
#include <numeric>
#include <vocomp/io.hpp>

namespace vol
{
VM_BEGIN_MODULE( video )

using namespace std;

VM_EXPORT
{
	struct SelfOwnedReader : SliceReader
	{
		SelfOwnedReader( Reader &reader, vector<char> buf = {} ) :
		  SliceReader( [this, &reader, &buf] {
			  buf.resize( reader.size() );
			  reader.read( buf.data(), buf.size() );
			  return SliceReader( buf.data(), buf.size() );
		  }() )
		{
			buf.swap( data );
		}

	private:
		vector<char> data;
	};
}

VM_END_MODULE()

}  // namespace vol