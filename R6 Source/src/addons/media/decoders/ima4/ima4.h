#ifndef _IMA4_H
#define _IMA4_H

#include <SupportDefs.h>

struct ima4_adpcm_state{
    int16	valprev;	/* Previous output value */
    int32	index;		/* Index into stepsize table */
};

void IMADecode4(const int8 *inp, char *outp, int32 channelCount,
				int32 sampCount, ima4_adpcm_state *state, bool isSwapped);

#endif
