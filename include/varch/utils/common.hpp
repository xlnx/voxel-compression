#pragma once

#include <VMUtils/fmt.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include <VMUtils/modules.hpp>
#include <cudafx/memory.hpp>
#include "io.hpp"

VM_BEGIN_MODULE( vol )

using namespace std;

#pragma pack( push )
#pragma pack( 4 )

VM_EXPORT
{
	enum class EncodePreset : uint32_t
	{
		Default,
		HP,
		HQ,
		BD,
		LowLatencyDefault,
		LowLatencyHQ,
		LowLatencyHP,
		LosslessDefault,
		LosslessHP
	};
	enum class ComputeDevice : uint32_t
	{
		Default = 0,
		Cuda, /* cuda runtime & nvidia driver >= 418 */
		Cpu	  /* openh264 libs required */
	};

	struct EncodeOptions
	{
		VM_DEFINE_ATTRIBUTE( ComputeDevice, device ) = ComputeDevice::Default;
		VM_DEFINE_ATTRIBUTE( EncodePreset, encode_preset ) = EncodePreset::Default;
		VM_DEFINE_ATTRIBUTE( unsigned, width ) = 1024;
		VM_DEFINE_ATTRIBUTE( unsigned, height ) = 1024;
		VM_DEFINE_ATTRIBUTE( unsigned, batch_frames ) = 64;
	};
	struct DecodeOptions
	{
		VM_DEFINE_ATTRIBUTE( ComputeDevice, device ) = ComputeDevice::Default;
		VM_DEFINE_ATTRIBUTE( unsigned, io_queue_size ) = 4;
	};

	struct BlockIndex
	{
		VM_DEFINE_ATTRIBUTE( uint32_t, first_frame );
		VM_DEFINE_ATTRIBUTE( uint32_t, last_frame );
		VM_DEFINE_ATTRIBUTE( uint64_t, offset );

		bool operator<( BlockIndex const &other ) const
		{
			return first_frame < other.first_frame ||
				   first_frame == other.first_frame && ( offset < other.offset );
		}

		bool operator==( BlockIndex const &other ) const
		{
			return first_frame == other.first_frame &&
				   last_frame == other.last_frame &&
				   offset == other.offset;
		}

		friend ostream &operator<<( ostream &os, BlockIndex const &_ )
		{
			vm::fprint( os, "{{ f0: {}, f1: {}, offset:{} }}", _.first_frame, _.last_frame, _.offset );
			return os;
		}
	};

	struct Idx
	{
		VM_DEFINE_ATTRIBUTE( uint32_t, x );
		VM_DEFINE_ATTRIBUTE( uint32_t, y );
		VM_DEFINE_ATTRIBUTE( uint32_t, z );

		uint64_t total() const { return (uint64_t)x * y * z; }

		bool operator<( Idx const &other ) const
		{
			return x < other.x ||
				   x == other.x && ( y < other.y ||
									 y == other.y && z < other.z );
		}
		bool operator==( Idx const &other ) const
		{
			return x == other.x && y == other.y && z == other.z;
		}
		bool operator!=( Idx const &other ) const
		{
			return !( *this == other );
		}

		friend ostream &operator<<( ostream &os, Idx const &_ )
		{
			vm::fprint( os, "{}", make_tuple( _.x, _.y, _.z ) );
			return os;
		}
	};

	struct Packet : vm::Dynamic, vm::NoCopy, vm::NoMove
	{
		void copy_to( cufx::MemoryView1D<unsigned char> const &dst ) const
		{
			return copy_to( dst, 0, length );
		}
		virtual void copy_to( cufx::MemoryView1D<unsigned char> const &dst,
							  unsigned offset, unsigned length ) const = 0;

	public:
		unsigned length, id;
	};
}

struct Header
{
	VM_DEFINE_ATTRIBUTE( uint64_t, version );
	VM_DEFINE_ATTRIBUTE( Idx, raw );
	VM_DEFINE_ATTRIBUTE( Idx, dim );
	VM_DEFINE_ATTRIBUTE( Idx, adjusted );
	VM_DEFINE_ATTRIBUTE( uint64_t, log_block_size );
	VM_DEFINE_ATTRIBUTE( uint64_t, block_size );
	VM_DEFINE_ATTRIBUTE( uint64_t, block_inner );
	VM_DEFINE_ATTRIBUTE( uint64_t, padding );
	VM_DEFINE_ATTRIBUTE( uint64_t, encode_method ) = 0;
	VM_DEFINE_ATTRIBUTE( uint64_t, frame_size );

	friend std::ostream &operator<<( std::ostream &os, Header const &header )
	{
		vm::fprint( os, "version: {}\nraw: {}\ndim: {}\nadjusted: {}\n"
						"log_block_size: {}\nblock_size: {}\nblock_inner: {}\n"
						"padding: {}\nframe_size: {}",
					header.version,
					header.raw,
					header.dim,
					header.adjusted,
					header.log_block_size,
					header.block_size,
					header.block_inner,
					header.padding,
					header.frame_size );
		return os;
	}
};

#pragma pack( pop )

VM_END_MODULE()
