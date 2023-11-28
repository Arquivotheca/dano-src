#ifndef _MSADPCM_SUPPORT_H_
#define _MSADPCM_SUPPORT_H_

#include <msmedia.h>

//  support functions
bool pcmIsValidFormat(WaveFormatEx *wfx);
bool adpcmIsValidFormat(WaveFormatEx *wfx);
bool adpcmBuildFormatHeader(WaveFormatEx *wfxSrc, WaveFormatEx *wfxDst);

#endif
