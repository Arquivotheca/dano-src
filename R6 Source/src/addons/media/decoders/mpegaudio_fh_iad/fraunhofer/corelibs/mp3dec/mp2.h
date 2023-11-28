
#if !defined(__MP2_H__)
#define __MP2_H__

#include "mpeg.h"
#include "mpegheader.h"
#include "l2table.h"
#include "bitstream.h"

#define MP2_SCALE_BLOCK 12

// ------------------------------------------------------------------------ //
// layer 1/2
// ------------------------------------------------------------------------ //

int mpgaCalcJSBound(
	const CMpegHeader& header);
	
int mpgaPickBitAllocTable(
	const CMpegHeader& header);

// ------------------------------------------------------------------------ //
// layer 1
// ------------------------------------------------------------------------ //

void mp1ReadBitAlloc(
	CBitStream& bs,
	unsigned int outBitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound);

void mp1ReadScale(
	CBitStream& bs,
	unsigned int outScaleIndex[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound);
	
void mp1BufferSample(
	CBitStream& bs,
	unsigned int outSample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound);
	
#if defined(ENABLE_FLOATING_POINT)
void mp1Dequantize(
	float outSpectrum[2][SSLIMIT][SBLIMIT],
	unsigned int sample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound);

void mp1Denormalize(
	float ioSpectrum[2][SSLIMIT][SBLIMIT],
	unsigned int scaleIndex[2][3][SBLIMIT],
	const CMpegHeader& header,
	int jsbound);
#endif

// fixed-point, channel-interleaved
// dequantize/denormalize are merged

#if defined(ENABLE_FIXED_POINT)
void mp1Dequantize(
	int32 outSpectrum[SSLIMIT*SBLIMIT*2],
	unsigned int sample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	unsigned int scaleIndex[2][3][SBLIMIT],
	const CMpegHeader& header,
	int jsbound);
#endif

// ------------------------------------------------------------------------ //
// layer 2
// ------------------------------------------------------------------------ //

void mp2ReadBitAlloc(
	CBitStream& bs,
	unsigned int outBitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table);

void mp2ReadScale(
	CBitStream& bs,
	unsigned int outScaleIndex[2][3][SBLIMIT],
	unsigned int outScaleSel[2][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table);
	
void mp2BufferSample(
	CBitStream& bs,
	unsigned int outSample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table);
	
#if defined(ENABLE_FLOATING_POINT)
void mp2Dequantize(
	float outSpectrum[2][SSLIMIT][SBLIMIT],
	unsigned int sample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table);

void mp2Denormalize(
	float ioSpectrum[2][SSLIMIT][SBLIMIT],
	unsigned int scaleIndex[2][3][SBLIMIT],
	unsigned int seg,
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table);
#endif

// fixed-point, channel-interleaved
// dequantize/denormalize are merged
#if defined(ENABLE_FIXED_POINT)
void mp2Dequantize(
	int32 outSpectrum[SSLIMIT*SBLIMIT*2],
	unsigned int sample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	unsigned int scaleIndex[2][3][SBLIMIT],
	unsigned int seg,
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table);
#endif


#endif //__MP2_H__
