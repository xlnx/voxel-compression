#include <gtest/gtest.h>
#include "../unbounded_vector_writer.hpp"

using namespace vol;
using namespace std;

TEST( test_unbounded_vector_writer, simple )
{
	vector<char> vec;
	UnboundedVectorWriter writer( vec );
	vector<string> data = {
		"123456789",
		"abcdef",
		"QWERTY"
	};
	writer.write( data[ 0 ].c_str(), data[ 0 ].length() );
	EXPECT_STREQ( vec.data(), "123456789" );
	EXPECT_EQ( writer.tell(), 9 );
	writer.write( data[ 1 ].c_str(), data[ 1 ].length() );
	EXPECT_STREQ( vec.data(), "123456789abcdef" );
	EXPECT_EQ( writer.tell(), 15 );
	writer.seek( 12 );
	writer.write( data[ 2 ].c_str(), data[ 2 ].length() );
	EXPECT_STREQ( vec.data(), "123456789abcQWERTY" );
}
