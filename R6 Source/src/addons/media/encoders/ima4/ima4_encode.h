#ifndef _IMA4_ENCODE_H_
#define _IMA4_ENCODE_H_

#include <SupportDefs.h>

enum {
	IMA4_LUINT8 = 1,
	IMA4_LINT16 = 2,
	IMA4_LINT32 = 3,
	IMA4_LFLOAT = 4,
	IMA4_BUINT8 = 5,
	IMA4_BINT16 = 6,
	IMA4_BINT32 = 7,
	IMA4_BFLOAT = 8
};

enum {
	IMA4_SAMPLE_COUNT = 64,
	IMA4_PACKET_SIZE = 34
};

// if sampCount < packetLength, pad with zeroes
int32 IMAEncode4(char *inp, char *outp, int32 channelCount,
				 int32 sampCount, int32 format, int32 *pv, int32 *pi,
				 int16 *convertBuffer);

int32 IMAQTHeader(char* outp, int32* pv, int32* pi);

// reads one sample per channel from (inp); pv and pi are expected to be
// arrays of size (channelCount)
int32 IMAWavHeader(char* inp, char* outp, int32 channelCount,
	int32 format, int32* pv, int32* pi);
	
#endif
