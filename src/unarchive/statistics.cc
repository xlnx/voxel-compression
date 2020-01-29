#include <VMat/geometry.h>
#include <VMat/numeric.h>
#include <VMFoundation/rawreader.h>
#include <varch/unarchive/statistics.hpp>

VM_BEGIN_MODULE( vol )

using namespace vm;
using namespace std;

struct StatisticsCollectorImpl
{
	StatisticsCollectorImpl( Unarchiver &unarchiver, std::string const &raw_file ) :
	  unarchiver( unarchiver )
	{
		if ( raw_file != "" ) {
			auto raw = unarchiver.raw();
			raw_input.reset( new RawReaderIO( raw_file, Size3( raw.x, raw.y, raw.z ), sizeof( char ) ) );
		}
	}

	void compute_into( Idx const &idx, Statistics &dst )
	{
		const auto I = Vec3i( idx.x, idx.y, idx.z );
		const auto N = Size3( unarchiver.block_size() );

		vector<unsigned char> buffer( N.Prod() );

		unarchiver.unarchive_to( idx, buffer );
		dst.src.compute_from( buffer );

		if ( raw_input ) {
			const auto N_i = Vec3i( unarchiver.block_inner() );
			const auto P = unarchiver.padding();

			vector<unsigned char> raw_buffer( N.Prod() );
			vector<unsigned char> diff_buffer( N.Prod() );

			raw_input->readRegion( N_i * I - Vec3i( P ), N, raw_buffer.data() );
			for ( int i = 0; i != N.Prod(); ++i ) {
				diff_buffer[ i ] = std::abs( raw_buffer[ i ] - buffer[ i ] );
			}
			dst.raw.compute_from( raw_buffer );
		}

		// double s = 0, a = 0, m = 0;

		// for ( int i = 0; i != buffer.size(); ++i ) {
		// 	if ( raw_buffer.data() ) {
		// 		auto dt = double( buffer[ i ] - raw_buffer[ i ] );
		// 		a += raw_buffer[ i ];
		// 	}
		// 	m = std::max( std::abs( dt ), m );
		// 	s += dt * dt;
		// }
		// s = std::sqrt( s / buffer.size() );
		// a /= buffer.size();
	}

public:
	Unarchiver &unarchiver;
	std::shared_ptr<RawReaderIO> raw_input;
};

VM_EXPORT
{
	StatisticsCollector::StatisticsCollector( Unarchiver & unarchiver, std::string const &raw_file ) :
	  _( new StatisticsCollectorImpl( unarchiver, raw_file ) )
	{
	}

	void StatisticsCollector::compute_into( Idx const &idx, Statistics &dst )
	{
		return _->compute_into( idx, dst );
	}
}

VM_END_MODULE()
