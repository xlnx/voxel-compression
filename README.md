# Voxel Codec

## Compression:

```cpp

// check tools

```

## Decompression

```cpp

ifstream is( "256-3.h264.vol", std::ios::ate | std::ios::binary );
auto ilen = is.tellg();

vol::StreamReader reader( is, 0, ilen );
vol::voxel::Decompressor decomp( reader );
auto block_dim = vol::voxel::Idx{}
  .set_x(256)
  .set_y(256)
  .set_z(256);			// (256, 256, 256)

{
	ofstream os( "256-3.raw.0.1.0", std::ios::binary );
	vol::UnboundedStreamWriter writer( os );

	// ofstream os( "256-3.raw.0.1.0", std::ios::binary );
	// vol::StreamWriter writer( os, 0, block_dim.total() );

	// vector<char> s( block_dim.total() );
	// vol::SliceWriter writer( s.data(), s.size() );

	auto idx = vol::voxel::Idx{}
	  .set_x( 0 )
	  .set_y( 1 )
	  .set_z( 0 );
	decomp.get( idx, writer );
}


```
