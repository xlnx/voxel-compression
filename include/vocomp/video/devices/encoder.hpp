#pragma once

#include <VMUtils/modules.hpp>
#include <VMUtils/concepts.hpp>
#include <vocomp/io.hpp>

namespace vol
{
VM_BEGIN_MODULE( video )

VM_EXPORT
{
	enum class EncodeMethod : char
	{
		H264,
		HEVC
	};
	enum class EncodePreset : char
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
	enum class PixelFormat : char
	{
		IYUV,
		YV12,
		NV12,
		YUV42010Bit,
		YUV444,
		YUV44410Bit,
		ARGB,
		ARGB10,
		AYUV,
		ABGR,
		ABGR10
	};
}

struct Encoder : vm::Dynamic, vm::NoCopy
{
	~Encoder();
	void init( EncodeMethod method, EncodePreset preset );
	virtual void encode( Reader &reader, Writer &writer ) = 0;

protected:
	static void get_pixel_format( void *dst, PixelFormat format );
	virtual void *get_nv_impl();
};

VM_END_MODULE()

}  // namespace vol
