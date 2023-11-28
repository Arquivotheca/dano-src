#include <stdio.h>
#include <MediaFormats.h>
#include "Decoder.h"
#include "IMA4Decoder.h"
#include "ima4.h"

//#define DEBUG printf
#define DEBUG if (0) printf

/* Intel ADPCM step variation table */
static const int8 kIndexIMA4[] = {
    -1, -1, -1, -1, 2, 4, 6, 8
};

static const int16 kSteps[89] = {
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

void IMADecode4(const int8 *inp, char *outp, int32 channelCount,
				int32 sampCount, ima4_adpcm_state *state, bool isSwapped)
{
    int32	i,iv,neg;
	int32 pv = state->valprev;
	int32 index = state->index;
    int32 step = kSteps[index];
	int32 s0 = isSwapped ? 0 : 4;
	int32 s1 = isSwapped ? 4 : 0;
	uchar ib = 0;

    for ( i=0; i < sampCount; i++ )  {    
		if ( i&1 ) {
			iv = (ib >> s1) & 0xf;
		} else {
			ib = *inp++;
			iv = (ib >> s0) & 0xf;
		}
		neg = iv & 8;
		iv &= 7;
		index += kIndexIMA4[iv];
		iv <<= 1;
		iv |= 1;
		iv *= step;
		iv >>= 3;
		if ( neg )
			pv -= iv;
		else
			pv += iv;
		if ( pv > 32767 )
			pv = 32767;
		else if ( pv < -32768 )
			pv = -32768;
		if (sizeof(out_type) == 2) {
			*((int16*)outp) = pv;
			outp += 2*channelCount;
		} else {
			*((float*)outp) = (float)pv*(1.0/32767.0);
			outp += 4*channelCount;
		}
		if ( index < 0 ) 
			index = 0;
		if ( index > 88 ) 
			index = 88;
		step = kSteps[index];
    }
    state->valprev = pv;
    state->index = index;
}
