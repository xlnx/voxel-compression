#include <algorithm>
#include <varch/unarchive/unarchiver.hpp>
#include <varch/utils/linked_reader.hpp>
#include "idecoder.hpp"
#include "backends/nvdec/nvdecoder_async.hpp"
#include "backends/openh264/isvc_decoder_wrapper.hpp"

VM_BEGIN_MODULE( vol )

using namespace std;

struct UnarchiverImpl
{
	UnarchiverImpl( UnarchiverData &data, UnarchiverOptions const &opts ) :
	  data( data )
	{
		auto dec_opts = VideoDecompressOptions{}
						  .set_io_queue_size( opts.io_queue_size );
		try {
			decoder.reset( new NvDecoderAsync( dec_opts ) );
		} catch ( std::exception &e ) {
			decoder.reset( new IsvcDecoderWrapper( dec_opts ) );
		}
	}

public:
	void unarchive_to( std::vector<Idx> const &blocks_const,
					   std::function<void( Idx const &idx, VoxelStreamPacket const & )> const &consumer )
	{
		auto blocks = blocks_const;
		vector<int64_t> linked_block_offsets;
		auto reader = sort_and_get_reader( blocks, linked_block_offsets );
		int i = 0;
		int64_t curr_block_offset = 0;
		int64_t linked_read_pos = 0;
		int64_t block_bytes = data.header.block_size * data.header.block_size * data.header.block_size;
		decoder->decode(
		  reader,
		  [&]( Packet const &packet ) {
			  while ( i < linked_block_offsets.size() ) {
				  //   vm::println( ">>>>>>{} {} {} {} {} {}<<<<<<", i, sorted_blocks[ i ], linked_block_offsets.size(), linked_block_offsets[ i ], curr_block_offset, linked_read_pos );
				  int64_t inpacket_offset = linked_block_offsets[ i ] + curr_block_offset - linked_read_pos;
				  // buffer contains current block
				  if ( inpacket_offset >= 0 && inpacket_offset < packet.length ) {
					  int64_t inpacket_max_len = packet.length - inpacket_offset;
					  auto len = std::min( inpacket_max_len,
										   block_bytes - curr_block_offset );
					  VoxelStreamPacket blk_packet( packet, inpacket_offset );
					  blk_packet.offset = curr_block_offset;
					  blk_packet.length = len;
					  consumer( blocks[ i ], blk_packet );

					  curr_block_offset += len;
					  if ( curr_block_offset >= block_bytes ) {
						  curr_block_offset = 0;
						  i++;
					  } else {
						  break;
					  }
				  } else {
					  break;
				  }
			  }
			  linked_read_pos += packet.length;
		  } );
	}

	std::size_t unarchive_to( Idx const &idx, cufx::MemoryView1D<unsigned char> const &dst )
	{
		std::size_t len;
		unarchive_to(
		  { idx },
		  [&]( Idx const &, VoxelStreamPacket const &pkt ) {
			  len += pkt.length;
			  pkt.append_to( dst );
		  } );
		return len;
	}

public:
	LinkedReader sort_and_get_reader( vector<Idx> &blocks, vector<int64_t> &linked_block_offsets )
	{
		vector<map<Idx, BlockIndex>::const_iterator> sorted_blocks( blocks.size() );
		std::transform( blocks.begin(), blocks.end(), sorted_blocks.begin(),
						[this]( Idx const &idx ) { return data.block_idx.find( idx ); } );
		std::sort( sorted_blocks.begin(), sorted_blocks.end(),
				   [this]( auto const &x, auto const &y ) { return x->second < y->second; } );
		std::transform( sorted_blocks.begin(), sorted_blocks.end(), blocks.begin(),
						[this]( map<Idx, BlockIndex>::const_iterator it ) { return it->first; } );

		vector<vm::Arc<Reader>> readers;
		int frame_count = 0, prev = 0;
		for ( int i = 0; i < sorted_blocks.size(); ++i ) {
			auto &prev_block = sorted_blocks[ prev ]->second;
			auto &curr_block = sorted_blocks[ i ]->second;

			auto dframes = curr_block.first_frame - prev_block.first_frame;
			linked_block_offsets.emplace_back( ( frame_count + dframes ) * data.header.frame_size + curr_block.offset );

			if ( i == sorted_blocks.size() - 1 ||
				 sorted_blocks[ i + 1 ]->second.first_frame > curr_block.last_frame ) {
				auto beg = data.frame_offset[ prev_block.first_frame ];
				auto len = data.frame_offset[ curr_block.last_frame + 1 ] - beg;
				// vm::println( "{} -> {} = {}", sorted_blocks[ i ]->first, make_pair( beg, len ), make_pair( prev_block.first_frame, curr_block.last_frame + 1 ) );
				readers.emplace_back( vm::Arc<Reader>( new PartReader( data.content, beg, len ) ) );
				frame_count += curr_block.last_frame - prev_block.first_frame + 1;
				prev = i + 1;
			}
		}

		auto linked_reader = LinkedReader( readers );
		linked_reader.seek( 0 );
		return linked_reader;
	}

public:
	UnarchiverData &data;
	std::unique_ptr<IDecoder> decoder;
};

VM_EXPORT
{
	Unarchiver::Unarchiver( Reader & reader, UnarchiverOptions const &opts ) :
	  data( reader ),
	  _( new UnarchiverImpl( data, opts ) )
	{
	}

	Unarchiver::~Unarchiver()
	{
	}

	std::size_t Unarchiver::unarchive_to( Idx const &idx,
										  cufx::MemoryView1D<unsigned char> const &dst )
	{
		return _->unarchive_to( idx, dst );
	}

	// void Unarchiver::batch_unarchive( vector<Idx> const &blocks,
	// 								  std::function<void( Idx const &idx, VoxelStreamPacket const & )> const &consumer )
	// {
	// 	auto sorted_blocks = blocks;
	// 	vector<int64_t> linked_block_offsets;
	// 	auto reader = sort_and_get_reader( sorted_blocks, linked_block_offsets );
	// 	int i = 0;
	// 	int64_t curr_block_offset = 0;
	// 	int64_t linked_read_pos = 0;
	// 	int64_t block_bytes = header.block_size * header.block_size * header.block_size;
	// 	decomp.decode(
	// 	  reader,
	// 	  [&]( NvBitStreamPacket const &packet ) {
	// 		  while ( i < linked_block_offsets.size() ) {
	// 			  //   vm::println( ">>>>>>{} {} {} {} {} {}<<<<<<", i, sorted_blocks[ i ], linked_block_offsets.size(), linked_block_offsets[ i ], curr_block_offset, linked_read_pos );
	// 			  int64_t inpacket_offset = linked_block_offsets[ i ] + curr_block_offset - linked_read_pos;
	// 			  // buffer contains current block
	// 			  if ( inpacket_offset >= 0 && inpacket_offset < packet.length ) {
	// 				  int64_t inpacket_max_len = packet.length - inpacket_offset;
	// 				  auto len = std::min( inpacket_max_len,
	// 									   block_bytes - curr_block_offset );
	// 				  VoxelStreamPacket blk_packet( packet, inpacket_offset );
	// 				  blk_packet.offset = curr_block_offset;
	// 				  blk_packet.length = len;
	// 				  consumer( sorted_blocks[ i ], blk_packet );

	// 				  curr_block_offset += len;
	// 				  if ( curr_block_offset >= block_bytes ) {
	// 					  curr_block_offset = 0;
	// 					  i++;
	// 				  } else {
	// 					  break;
	// 				  }
	// 			  } else {
	// 				  break;
	// 			  }
	// 		  }
	// 		  linked_read_pos += packet.length;
	// 	  } );
	// }
}

VM_END_MODULE()
