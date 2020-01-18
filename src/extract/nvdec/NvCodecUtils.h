/*
* Copyright 2017-2018 NVIDIA Corporation.  All rights reserved.
*
* Please refer to the NVIDIA end user license agreement (EULA) associated
* with this source code for terms and conditions that govern your use of
* this software. Any use, reproduction, disclosure, or distribution of
* this software and related documentation outside the terms of the EULA
* is strictly prohibited.
*
*/

//---------------------------------------------------------------------------
//! \file NvCodecUtils.h
//! \brief Miscellaneous classes and error checking functions.
//!
//! Used by Transcode/Encode samples apps for reading input files, mutithreading, performance measurement or colorspace conversion while decoding.
//---------------------------------------------------------------------------

#pragma once

#include <iomanip>
#include <chrono>
#include <sys/stat.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <thread>
#include <fstream>
#include <VMUtils/fmt.hpp>

#ifdef __cuda_cuda_h__
inline bool check( CUresult e, int iLine, const char *szFile )
{
	if ( e != CUDA_SUCCESS ) {
		const char *szErrName = NULL;
		cuGetErrorName( e, &szErrName );
		vm::eprintln( "FATAL: CUDA driver API error {} at line {} in file {}",
					  szErrName, iLine, szFile );
		return false;
	}
	return true;
}
#endif

#ifdef __CUDA_RUNTIME_H__
inline bool check( cudaError_t e, int iLine, const char *szFile )
{
	if ( e != cudaSuccess ) {
		vm::eprintln( "FATAL: CUDA driver API error {} at line {} in file {}",
					  cudaGetErrorName( e ), iLine, szFile );
		return false;
	}
	return true;
}
#endif

inline bool check( int e, int iLine, const char *szFile )
{
	if ( e < 0 ) {
		vm::eprintln( "FATAL: General error {} at line {} in file {}",
					  e, iLine, szFile );
		return false;
	}
	return true;
}

#define ck( call ) check( call, __LINE__, __FILE__ )

/**
* @brief Wrapper class around std::thread
*/
class NvThread
{
public:
	NvThread() = default;
	NvThread( const NvThread & ) = delete;
	NvThread &operator=( const NvThread &other ) = delete;

	NvThread( std::thread &&thread ) :
	  t( std::move( thread ) )
	{
	}

	NvThread( NvThread &&thread ) :
	  t( std::move( thread.t ) )
	{
	}

	NvThread &operator=( NvThread &&other )
	{
		t = std::move( other.t );
		return *this;
	}

	~NvThread()
	{
		join();
	}

	void join()
	{
		if ( t.joinable() ) {
			t.join();
		}
	}

private:
	std::thread t;
};

#ifndef _WIN32
#define _stricmp strcasecmp
#define _stat64 stat64
#endif

/**
* @brief Utility class to allocate buffer memory. Helps avoid I/O during the encode/decode loop in case of performance tests.
*/
class BufferedFileReader
{
public:
	/**
    * @brief Constructor function to allocate appropriate memory and copy file contents into it
    */
	BufferedFileReader( const char *szFileName, bool bPartial = false )
	{
		struct _stat64 st;

		if ( _stat64( szFileName, &st ) != 0 ) {
			return;
		}

		nSize = st.st_size;
		while ( nSize ) {
			try {
				pBuf = new uint8_t[ (size_t)nSize ];
				if ( nSize != st.st_size ) {
					vm::eprintln( "WARNING: File is too large - only {.4}% is loaded",
								  100.0 * nSize / st.st_size );
				}
				break;
			} catch ( std::bad_alloc ) {
				if ( !bPartial ) {
					vm::eprintln( "ERROR: Failed to allocate memory in BufferedReader" );
					return;
				}
				nSize = ( uint32_t )( nSize * 0.9 );
			}
		}

		std::ifstream fpIn( szFileName, std::ifstream::in | std::ifstream::binary );
		if ( !fpIn ) {
			vm::eprintln( "ERROR: Unable to open input file: {}", szFileName );
			return;
		}

		std::streamsize nRead = fpIn.read( reinterpret_cast<char *>( pBuf ), nSize ).gcount();
		fpIn.close();

		assert( nRead == nSize );
	}
	~BufferedFileReader()
	{
		if ( pBuf ) {
			delete[] pBuf;
		}
	}
	bool GetBuffer( uint8_t **ppBuf, uint64_t *pnSize )
	{
		if ( !pBuf ) {
			return false;
		}

		*ppBuf = pBuf;
		*pnSize = nSize;
		return true;
	}

private:
	uint8_t *pBuf = NULL;
	uint64_t nSize = 0;
};

/**
* @brief Template class to facilitate color space conversion
*/
template <typename T>
class YuvConverter
{
public:
	YuvConverter( int nWidth, int nHeight ) :
	  nWidth( nWidth ),
	  nHeight( nHeight )
	{
		pQuad = new T[ nWidth * nHeight / 4 ];
	}
	~YuvConverter()
	{
		delete pQuad;
	}
	void PlanarToUVInterleaved( T *pFrame, int nPitch = 0 )
	{
		if ( nPitch == 0 ) {
			nPitch = nWidth;
		}
		T *puv = pFrame + nPitch * nHeight;
		if ( nPitch == nWidth ) {
			memcpy( pQuad, puv, nWidth * nHeight / 4 * sizeof( T ) );
		} else {
			for ( int i = 0; i < nHeight / 2; i++ ) {
				memcpy( pQuad + nWidth / 2 * i, puv + nPitch / 2 * i, nWidth / 2 * sizeof( T ) );
			}
		}
		T *pv = puv + ( nPitch / 2 ) * ( nHeight / 2 );
		for ( int y = 0; y < nHeight / 2; y++ ) {
			for ( int x = 0; x < nWidth / 2; x++ ) {
				puv[ y * nPitch + x * 2 ] = pQuad[ y * nWidth / 2 + x ];
				puv[ y * nPitch + x * 2 + 1 ] = pv[ y * nPitch / 2 + x ];
			}
		}
	}
	void UVInterleavedToPlanar( T *pFrame, int nPitch = 0 )
	{
		if ( nPitch == 0 ) {
			nPitch = nWidth;
		}
		T *puv = pFrame + nPitch * nHeight,
		  *pu = puv,
		  *pv = puv + nPitch * nHeight / 4;
		for ( int y = 0; y < nHeight / 2; y++ ) {
			for ( int x = 0; x < nWidth / 2; x++ ) {
				pu[ y * nPitch / 2 + x ] = puv[ y * nPitch + x * 2 ];
				pQuad[ y * nWidth / 2 + x ] = puv[ y * nPitch + x * 2 + 1 ];
			}
		}
		if ( nPitch == nWidth ) {
			memcpy( pv, pQuad, nWidth * nHeight / 4 * sizeof( T ) );
		} else {
			for ( int i = 0; i < nHeight / 2; i++ ) {
				memcpy( pv + nPitch / 2 * i, pQuad + nWidth / 2 * i, nWidth / 2 * sizeof( T ) );
			}
		}
	}

private:
	T *pQuad;
	int nWidth, nHeight;
};

/**
* @brief Utility class to measure elapsed time in seconds between the block of executed code
*/
class StopWatch
{
public:
	void Start()
	{
		t0 = std::chrono::high_resolution_clock::now();
	}
	double Stop()
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::high_resolution_clock::now().time_since_epoch() - t0.time_since_epoch() ).count() / 1.0e9;
	}

private:
	std::chrono::high_resolution_clock::time_point t0;
};

inline void CheckInputFile( const char *szInFilePath )
{
	std::ifstream fpIn( szInFilePath, std::ios::in | std::ios::binary );
	if ( fpIn.fail() ) {
		std::ostringstream err;
		err << "Unable to open input file: " << szInFilePath << std::endl;
		throw std::invalid_argument( err.str() );
	}
}

inline void ValidateResolution( int nWidth, int nHeight )
{
	if ( nWidth <= 0 || nHeight <= 0 ) {
		std::ostringstream err;
		err << "Please specify positive non zero resolution as -s WxH. Current resolution is " << nWidth << "x" << nHeight << std::endl;
		throw std::invalid_argument( err.str() );
	}
}

template <class COLOR32>
void Nv12ToColor32( uint8_t *dpNv12, int nNv12Pitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight, int iMatrix = 0 );
template <class COLOR64>
void Nv12ToColor64( uint8_t *dpNv12, int nNv12Pitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight, int iMatrix = 0 );

template <class COLOR32>
void P016ToColor32( uint8_t *dpP016, int nP016Pitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight, int iMatrix = 4 );
template <class COLOR64>
void P016ToColor64( uint8_t *dpP016, int nP016Pitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight, int iMatrix = 4 );

template <class COLOR32>
void YUV444ToColor32( uint8_t *dpYUV444, int nPitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight, int iMatrix = 0 );
template <class COLOR64>
void YUV444ToColor64( uint8_t *dpYUV444, int nPitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight, int iMatrix = 0 );

template <class COLOR32>
void YUV444P16ToColor32( uint8_t *dpYUV444, int nPitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight, int iMatrix = 4 );
template <class COLOR64>
void YUV444P16ToColor64( uint8_t *dpYUV444, int nPitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight, int iMatrix = 4 );

template <class COLOR32>
void Nv12ToColorPlanar( uint8_t *dpNv12, int nNv12Pitch, uint8_t *dpBgrp, int nBgrpPitch, int nWidth, int nHeight, int iMatrix = 0 );
template <class COLOR32>
void P016ToColorPlanar( uint8_t *dpP016, int nP016Pitch, uint8_t *dpBgrp, int nBgrpPitch, int nWidth, int nHeight, int iMatrix = 4 );

template <class COLOR32>
void YUV444ToColorPlanar( uint8_t *dpYUV444, int nPitch, uint8_t *dpBgrp, int nBgrpPitch, int nWidth, int nHeight, int iMatrix = 0 );
template <class COLOR32>
void YUV444P16ToColorPlanar( uint8_t *dpYUV444, int nPitch, uint8_t *dpBgrp, int nBgrpPitch, int nWidth, int nHeight, int iMatrix = 4 );

void Bgra64ToP016( uint8_t *dpBgra, int nBgraPitch, uint8_t *dpP016, int nP016Pitch, int nWidth, int nHeight, int iMatrix = 4 );

void ConvertUInt8ToUInt16( uint8_t *dpUInt8, uint16_t *dpUInt16, int nSrcPitch, int nDestPitch, int nWidth, int nHeight );
void ConvertUInt16ToUInt8( uint16_t *dpUInt16, uint8_t *dpUInt8, int nSrcPitch, int nDestPitch, int nWidth, int nHeight );

void ResizeNv12( unsigned char *dpDstNv12, int nDstPitch, int nDstWidth, int nDstHeight, unsigned char *dpSrcNv12, int nSrcPitch, int nSrcWidth, int nSrcHeight, unsigned char *dpDstNv12UV = nullptr );
void ResizeP016( unsigned char *dpDstP016, int nDstPitch, int nDstWidth, int nDstHeight, unsigned char *dpSrcP016, int nSrcPitch, int nSrcWidth, int nSrcHeight, unsigned char *dpDstP016UV = nullptr );

void ScaleYUV420( unsigned char *dpDstY, unsigned char *dpDstU, unsigned char *dpDstV, int nDstPitch, int nDstChromaPitch, int nDstWidth, int nDstHeight,
				  unsigned char *dpSrcY, unsigned char *dpSrcU, unsigned char *dpSrcV, int nSrcPitch, int nSrcChromaPitch, int nSrcWidth, int nSrcHeight, bool bSemiplanar );

#ifdef __cuda_cuda_h__
void ComputeCRC( uint8_t *pBuffer, uint32_t *crcValue, CUstream_st *outputCUStream );
#endif
