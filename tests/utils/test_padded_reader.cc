#include <gtest/gtest.h>
#include <vocomp/utils/padded_reader.hpp>

using namespace vol;
using namespace std;

TEST( test_padded_reader, simple )
{
	string a = "123456789";
	SliceReader slice_reader( a.c_str(), a.length() );
	PaddedReader reader( slice_reader, 12, '_' );
	string d;
	d.resize( 16 );
	EXPECT_EQ( 12, reader.read( const_cast<char *>( d.data() ), d.length() ) );
	EXPECT_STREQ( "123456789___", d.c_str() );
	EXPECT_EQ( 12, reader.size() );
	EXPECT_EQ( 12, reader.tell() );
}
