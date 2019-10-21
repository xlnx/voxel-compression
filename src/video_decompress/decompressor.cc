#include <vocomp/video/decompressor.hpp>

#include <nvcodec/NvDecoder.h>
#include <cudafx/driver/context.hpp>

namespace vol
{
VM_BEGIN_MODULE( video )

struct DecompressorImpl final : vm::NoCopy, vm::NoMove
{
	DecompressorImpl( EncodeMethod encode )
	{
		dec.reset(
		  new NvDecoder(
			ctx, false,
			[&] {
				switch ( encode ) {
				case EncodeMethod::H264: return cudaVideoCodec_H264;
				case EncodeMethod::HEVC: return cudaVideoCodec_HEVC;
				default: throw std::runtime_error( "unknown encoding" );
				}
			}() ) );
	}

	void decompress( Reader &reader, Writer &writer )
	{
		thread_local auto packet = std::vector<char>( 1024 );

		int totaldec = 0;
		uint8_t **ppframe;
		int nframedec;

		uint32_t nframes;
		reader.read( reinterpret_cast<char *>( &nframes ), sizeof( nframes ) );
		thread_local std::vector<uint32_t> frame_len;
		frame_len.resize( nframes + 1 );
		reader.read( reinterpret_cast<char *>( frame_len.data() ),
					 sizeof( uint32_t ) * nframes );
		frame_len[ nframes ] = 0;

		for ( auto &len : frame_len ) {
			if ( len > packet.size() ) {
				packet.resize( len );
			}
			reader.read( packet.data(), len );
			dec->Decode( reinterpret_cast<uint8_t *>( packet.data() ), len, &ppframe, &nframedec );

			for ( int i = 0; i < nframedec; i++ ) {
				writer.write( reinterpret_cast<char *>( ppframe[ i ] ), dec->GetFrameSize() );
			}
			totaldec += nframedec;
		}
	}

private:
	cufx::drv::Context ctx = 0;
	std::unique_ptr<NvDecoder> dec;
};

VM_EXPORT
{
	Decompressor::Decompressor( EncodeMethod encode ) :
	  _( new DecompressorImpl( encode ) )
	{
	}
	Decompressor::~Decompressor()
	{
	}
	void Decompressor::decompress( Reader & reader, Writer & writer )
	{
		_->decompress( reader, writer );
	}
}

VM_END_MODULE()

}  // namespace vol
