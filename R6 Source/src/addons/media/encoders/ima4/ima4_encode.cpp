#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ByteOrder.h>
#include <Debug.h>
#include "ima4_encode.h"

#if DEBUG
#define PRINTF	printf
#else
#define PRINTF	if (0) printf
#endif

// Intel ADPCM step variation table
static const int8 kIndexIMA4[] = { -1, -1, -1, -1, 2, 4, 6, 8 };
static const int32 kSteps[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static void readValues(char *&inp, int16* outp, int32 count, int32 format, int32 channelCount) {
	int32		i;
	
	switch (format) {
	case IMA4_LUINT8 :
	case IMA4_BUINT8 :
		for (i=0; i<count; i++) {
			outp[i] = (((int16)((uint8*)inp)[0])-128)<<8;
			inp += channelCount;
		}
		break;
	case IMA4_LINT16 :
		for (i=0; i<count; i++) {
			outp[i] = B_LENDIAN_TO_HOST_INT16(((int16*)inp)[0]);
			inp += 2*channelCount;
		}
		break;
	case IMA4_LINT32 :
		for (i=0; i<count; i++) {
			outp[i] = B_LENDIAN_TO_HOST_INT32(((int16*)inp)[0])>>16;
			inp += 4*channelCount;
		}
		break;
	case IMA4_LFLOAT :
		for (i=0; i<count; i++) {
			outp[i] = (int16)(B_LENDIAN_TO_HOST_FLOAT(((float*)inp)[0])*32767.0);
			inp += 4*channelCount;
		}
		break;
	case IMA4_BINT16 :
		for (i=0; i<count; i++) {
			outp[i] = B_BENDIAN_TO_HOST_INT16(((int16*)inp)[0]);
			inp += 2*channelCount;
		}
		break;
	case IMA4_BINT32 :
		for (i=0; i<count; i++) {
			outp[i] = B_BENDIAN_TO_HOST_INT32(((int16*)inp)[0])>>16;
			inp += 4*channelCount;
		}
		break;
	case IMA4_BFLOAT :
		for (i=0; i<count; i++) {
			outp[i] = (int16)(B_BENDIAN_TO_HOST_FLOAT(((float*)inp)[0])*32767.0);
			inp += 4*channelCount;
		}
		break;
	}
}

int32 IMAEncode4(char *inp, char *outp, int32 channelCount,
				 int32 sampCount, int32 format, int32 *pv, int32 *pi,
				 int16 *convertBuffer) {
	int32			i, i_count, abs_delta, delta, index, prev_value;
	int32			frac, b_data, step;

	int32 output_count = 0;
	
	// convert the values in usable format
	readValues(inp, convertBuffer, sampCount, format, channelCount);
	
	// get the last coded value and step predictor 
	index = *pi;
	prev_value = *pv;
	
	for (i=0; i<sampCount; i++) {
		// current delta step
		step = kSteps[index];
		// real delta and absolute delta
		delta = ((int32)convertBuffer[i])-prev_value;
		if (delta < 0)
			abs_delta = -delta;
		else
			abs_delta = delta;
		// calculate the best approximation possible with the current step
		frac = ((((abs_delta<<3)+(step>>1))/step)-1)>>1;
		if (frac < 0)
			frac = 0;
		else if (frac > 7)
			frac = 7;
		// update the index used to calculate the next step
		index += kIndexIMA4[frac];
		if ( index < 0 ) 
			index = 0;
		if ( index > 88 ) 
			index = 88;
		// calculate the approximate value that will be encoded.
		abs_delta = (step*(frac*2+1))>>3;
		if (delta < 0) {
			prev_value -= abs_delta;
			frac |= 8;
		}
		else
			prev_value += abs_delta;
		if (prev_value > 32767 )
			prev_value = 32767;
		else if (prev_value < -32768 )
			prev_value = -32768;		
		// Nibble packing and writing
		if (i&1) {
			b_data += frac<<4;
			*outp++ = b_data;
			output_count++;
		}
		else
			b_data = frac;
	}
	
	// write the last nibble alone if any (should never happen hopefully)
	if (sampCount&1) {
		*outp++ = b_data;
		output_count++;
	}
	
	// return the new "last used value and last used step"
	*pv = prev_value;
	*pi = index;
	
	// return the count of written bytes.
	return output_count;
}

int32 
IMAQTHeader(char* outp, int32* pv, int32* pi)
{
	int32 prev_value = (int32)((int16)((*pv)&0xff80));
	*((int16*)outp) = B_HOST_TO_BENDIAN_INT16(prev_value + *pi);
	*pv = prev_value;
	return 2;
}

int32 
IMAWavHeader(char *inp, char *outp, int32 channelCount, int32 format, int32 *pv, int32 *pi)
{
	ASSERT(channelCount <= 2);
	int16 val[2];
	readValues(inp, val, channelCount, format, 1);
	
	int32 written = 0;
	for(int32 i = 0; i < channelCount; i++, written += 4)
	{
		int32 index = pi[i];
		// current delta step
		int32 step = kSteps[index];
		// real delta and absolute delta
		int32 delta = ((int32)val[i])-pv[i];
		int32 abs_delta = (delta < 0) ? -delta : delta;
		// calculate the best approximation possible with the current step
		int32 frac = ((((abs_delta<<3)+(step>>1))/step)-1)>>1;
		if (frac < 0)
			frac = 0;
		else if (frac > 7)
			frac = 7;
		// update the index used to calculate the next step
		index += kIndexIMA4[frac];
		if ( index < 0 ) 
			index = 0;
		if ( index > 88 ) 
			index = 88;

		pv[i] = val[i];
		pi[i] = index;
		
		*((int16*)outp) = B_HOST_TO_LENDIAN_INT16(pv[i]);
		outp += 2;
		*outp++ = int8(pi[i]);
		*outp++ = 0;
	}
	
	return written;
}


