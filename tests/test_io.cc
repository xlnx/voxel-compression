#include <gtest/gtest.h>
#include <vocomp/utils/linked_reader.hpp>
#include <vocomp/utils/padded_reader.hpp>
#include <vocomp/utils/unbounded_vector_writer.hpp>

using namespace vol;
using namespace std;

TEST( test_io, linked_reader )
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

TEST( test_io, padded_reader )
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

TEST( test_io, unbounded_vector_writer )
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
