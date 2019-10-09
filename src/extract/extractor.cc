#include <vocomp/refine/extractor.hpp>
#include <vocomp/index/decompressor.hpp>

namespace vol
{
VM_BEGIN_MODULE( refine )

using namespace std;

struct ExtractorImpl final : vm::NoCopy, vm::NoMove
{
	ExtractorImpl( Reader &reader, Pipe &pipe ) :
	  content( reader, sizeof( Header ), reader.size() - sizeof( Header ) ),
	  decomp( content, pipe )
	{
	}
	bool extract( index::Idx idx, Writer &writer )
	{
		decomp.get( idx, writer );
		return true;
	}

private:
	PartReader content;
	index::Decompressor<> decomp;
};

VM_EXPORT
{
	Extractor::Extractor( Reader & reader, Pipe & pipe ) :
	  _( new ExtractorImpl( reader, pipe ) )
	{
		PartReader header_reader( reader, 0, sizeof( Header ) );
		auto header = Header::read_from( header_reader );
		_raw
		  .set_x( header.raw.x )
		  .set_y( header.raw.y )
		  .set_z( header.raw.z );
		_dim
		  .set_x( header.dim.x )
		  .set_y( header.dim.y )
		  .set_z( header.dim.z );
		_adjusted
		  .set_x( header.adjusted.x )
		  .set_y( header.adjusted.y )
		  .set_z( header.adjusted.z );
		_log_block_size = header.log_block_size;
		_block_size = header.block_size;
		_block_inner = header.block_inner;
		_padding = header.padding;
	}
	Extractor::~Extractor()
	{
	}
	bool Extractor::extract( index::Idx idx, Writer & writer )
	{
		return _->extract( idx, writer );
	}
}

VM_END_MODULE()

}  // namespace vol
