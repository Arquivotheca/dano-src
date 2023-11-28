#ifndef _MSADPCM_DECODE_H_
#define _MSADPCM_DECODE_H_

#include <msmedia.h>

uint32 adpcmDecode4Bit(ADPCMWaveFormat *wfADPCM, const char *Src,
					   PCMWaveFormat *wfPCM, char *Dst,
					   uint32 SrcLen);

#endif
