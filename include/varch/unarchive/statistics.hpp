#pragma once

#include <vector>
#include <numeric>
#include <algorithm>
#include <varch/unarchive/unarchiver.hpp>

VM_BEGIN_MODULE( vol )

struct StatisticsCollectorImpl;

VM_EXPORT
{
	struct BasicStatistics
	{
		double avg, max, min;

		template <typename T>
		void compute_from( T const *src, std::size_t len )
		{
			this->avg = accumulate( src, src + len, 0.0 ) / len;
			this->max = *max_element( src, src + len );
			this->min = *min_element( src, src + len );
		}
		template <typename T>
		void compute_from( std::vector<T> const &vec )
		{
			compute_from( vec.data(), vec.size() );
		}
	};

	struct Statistics
	{
		BasicStatistics src, raw, diff;
	};

	struct StatisticsCollector : vm::NoCopy, vm::NoMove
	{
		StatisticsCollector( Unarchiver &unarchiver, std::string const &raw_file = "" );
		~StatisticsCollector();

	public:
		/* compute statistics of a single block */
		void compute_into( Idx const &idx, Statistics &dst );

	private:
		vm::Box<StatisticsCollectorImpl> _;
	};
}

VM_END_MODULE()
