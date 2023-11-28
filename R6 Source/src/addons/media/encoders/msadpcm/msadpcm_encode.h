#ifndef _MSADPCM_ENCODE_H_
#define _MSADPCM_ENCODE_H_

#include <msmedia.h>

uint32 adpcmEncode4Bit(PCMWaveFormat *wfPCM, char *Src,
					   ADPCMWaveFormat *wfADPCM, char *Dst,
					   uint32 SrcLen);

#endif
