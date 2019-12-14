#include <gtest/gtest.h>
#include "../linked_reader.hpp"

using namespace vol;
using namespace vol::video;
using namespace std;

TEST( test_linked_reader, simple )
{
	string a = "123456789";
	string b = "abcdef";
	string c = "QWERTY";
	vector<vm::Arc<Reader>> readers;
	readers.emplace_back( vm::Arc<Reader>( new SliceReader( a.c_str(), a.length() ) ) );
	readers.emplace_back( vm::Arc<Reader>( new SliceReader( b.c_str(), b.length() ) ) );
	readers.emplace_back( vm::Arc<Reader>( new SliceReader( c.c_str(), c.length() ) ) );
	LinkedReader reader( readers );
	string d;
	d.resize( 16 );
	EXPECT_EQ( 16, reader.read( const_cast<char *>( d.data() ), d.length() ) );
	EXPECT_EQ( "123456789abcdefQ", d );
	EXPECT_EQ( 21, reader.size() );
	EXPECT_EQ( 16, reader.tell() );

	reader.seek( 8 );
	EXPECT_EQ( 13, reader.read( const_cast<char *>( d.data() ), d.length() ) );
	EXPECT_EQ( "9abcdefQWERTYefQ", d );
	EXPECT_EQ( 21, reader.size() );
	EXPECT_EQ( 21, reader.tell() );

	EXPECT_EQ( 0, reader.read( const_cast<char *>( d.data() ), d.length() ) );
}