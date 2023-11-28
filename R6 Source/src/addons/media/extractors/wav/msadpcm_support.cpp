/*  
	Source based on :

	 msadpcm.c

     (C) Copyright Microsoft Corp. 1993.  All rights reserved.

     You have a royalty-free right to use, modify, reproduce and 
     distribute the Sample Files (and/or any modified version) in 
     any way you find useful, provided that you agree that 
     Microsoft has no warranty obligations or liability for any 
     Sample Application Files which are modified. 
	 
     If you did not get this from Microsoft Sources, then it may not be the
     most current version.  This sample code in particular will be updated
     and include more documentation.  

     Sources are:
     	The MM Sys File Transfer BBS: The phone number is 206 936-4082.
	CompuServe: WINSDK forum, MDK section.
	Anonymous FTP from ftp.uu.net vendors\microsoft\multimedia
*/

#include "msadpcm_support.h"
#include <stdlib.h>

//---------------------------------------------------------------------------
//
//  the code below provides 'support' routines for building/verifying ADPCM
//  headers, etc.
//
//  the coefficient pairs that should be in the wave format header for
//  the Microsoft 4 Bit ADPCM algorithm. the code to copy the coefficients
//  into the wave format header is shown:
//
//      int16 gaiCoef1[] = { 256,  512,  0, 192, 240,  460,  392 };
//      int16 gaiCoef2[] = {   0, -256,  0,  64,   0, -208, -232 };
//
//      for (w = 0; w < MSADPCM_NUM_COEF; w++)
//      {
//          lpwfADPCM->aCoef[w].iCoef1 = gaiCoef1[w];
//          lpwfADPCM->aCoef[w].iCoef2 = gaiCoef2[w];
//      }
//
//---------------------------------------------------------------------------

static int16 gaiCoef1[] = { 256,  512,  0, 192, 240,  460,  392 };
static int16 gaiCoef2[] = {   0, -256,  0,  64,   0, -208, -232 };


/** BOOL FAR PASCAL adpcmIsValidFormat(LPWAVEFORMATEX lpwfx)
 *
 *  DESCRIPTION:
 *      
 *
 *  ARGUMENTS:
 *      (LPWAVEFORMATEX lpwfx)
 *
 *  RETURN (BOOL FAR PASCAL):
 *
 *
 *  NOTES:
 *
 ** */

bool adpcmIsValidFormat(WaveFormatEx *lpwfx)
{
    ADPCMWaveFormat   *lpwfADPCM;
    uint16            w;

    if (!lpwfx)
        return (false);

    if (lpwfx->wFormatTag != WAVE_FORMAT_ADPCM)
        return (false);

    if (lpwfx->wBitsPerSample != 4)
        return (false);

    if ((lpwfx->nChannels < 1) || (lpwfx->nChannels > MSADPCM_MAX_CHANNELS))
        return (false);


    //
    //  check coef's to see if it is Microsoft's standard 4 Bit ADPCM
    //
    lpwfADPCM = (ADPCMWaveFormat*)lpwfx;

    if (lpwfADPCM->wNumCoef != MSADPCM_NUM_COEF)
        return (false);

    for (w = 0; w < MSADPCM_NUM_COEF; w++)
    {
        if (lpwfADPCM->aCoef[w].iCoef1 != gaiCoef1[w])
            return (false);

        if (lpwfADPCM->aCoef[w].iCoef2 != gaiCoef2[w])
            return (false);
    }

    return (true);
} /* adpcmIsValidFormat() */


/** BOOL FAR PASCAL pcmIsValidFormat(LPWAVEFORMATEX lpwfx)
 *
 *  DESCRIPTION:
 *      
 *
 *  ARGUMENTS:
 *      (LPWAVEFORMATEX lpwfx)
 *
 *  RETURN (BOOL FAR PASCAL):
 *
 *
 *  NOTES:
 *
 ** */

bool pcmIsValidFormat(WaveFormatEx *lpwfx)
{
    if (!lpwfx)
        return (false);

    if (lpwfx->wFormatTag != WAVE_FORMAT_PCM)
        return (false);

    if ((lpwfx->wBitsPerSample != 8) && (lpwfx->wBitsPerSample != 16))
        return (false);

    if ((lpwfx->nChannels < 1) || (lpwfx->nChannels > MSADPCM_MAX_CHANNELS))
        return (false);

    return (true);
} /* pcmIsValidFormat() */


/** BOOL FAR PASCAL adpcmBuildFormatHeader(LPWAVEFORMATEX lpwfxSrc, LPWAVEFORMATEX lpwfxDst)
 *
 *  DESCRIPTION:
 *      
 *
 *  ARGUMENTS:
 *      (LPWAVEFORMATEX lpwfxSrc, LPWAVEFORMATEX lpwfxDst)
 *
 *  RETURN (BOOL FAR PASCAL):
 *
 *
 *  NOTES:
 *
 ** */

bool adpcmBuildFormatHeader(WaveFormatEx *lpwfxSrc, WaveFormatEx *lpwfxDst)
{
    ADPCMWaveFormat   *lpwfADPCM;
    uint16            wBlockAlign;
    uint16            wChannels;
    uint16            wBitsPerSample;
    uint16            wHeaderBytes;
    uint32            dw;
    uint16            w;

    //
    //  if the source format is PCM, then build an ADPCM destination format
    //  header... assuming the PCM format header is valid.
    //
    if (lpwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
    {
        if (!pcmIsValidFormat(lpwfxSrc))
            return (false);

        lpwfADPCM = (ADPCMWaveFormat*)lpwfxDst;

        //
        //  fill in destination header with appropriate ADPCM stuff based
        //  on source PCM header...
        //
        lpwfxDst->wFormatTag     = WAVE_FORMAT_ADPCM;
        lpwfxDst->nSamplesPerSec = lpwfxSrc->nSamplesPerSec;
        lpwfxDst->nChannels      = wChannels      = lpwfxSrc->nChannels;
        lpwfxDst->wBitsPerSample = wBitsPerSample = 4;

        //
        //  choose a block alignment that makes sense for the sample rate
        //  that the original PCM data is. basically, this needs to be
        //  some reasonable number to allow efficient streaming, etc.
        //
        //  don't let block alignment get too small...
        //
        wBlockAlign = 256 * wChannels;
        if (lpwfxSrc->nSamplesPerSec > 11025)
            wBlockAlign *= (uint16)(lpwfxSrc->nSamplesPerSec / 11000);

        lpwfxDst->nBlockAlign = wBlockAlign;

        //
        //  compute that 'samples per block' that will be in the encoded
        //  ADPCM data blocks. this is determined by subtracting out the
        //  'other info' contained in each block--a block is composed of
        //  a header followed by the encoded data.
        //
        //  the block header is composed of the following data:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first sample per channel
        //      2 byte second sample per channel
        //
        //  this gives us (7 * wChannels) bytes of header information that
        //  contains our first two full samples (so we add two below).
        //
        wHeaderBytes = (7 * wChannels);

        w = (wBlockAlign - wHeaderBytes) * 8;
        lpwfADPCM->wSamplesPerBlock = (w / (wBitsPerSample * wChannels)) + 2;

        //
        //  now compute the avg bytes per second (man this code bites!)
        //
        dw = (((uint32)wBitsPerSample * wChannels * 
                        (uint32)lpwfxDst->nSamplesPerSec) / 8);
        lpwfxDst->nAvgBytesPerSec = (uint16)(dw + wHeaderBytes + 
                        ((dw / wBlockAlign) * wHeaderBytes));

        //
        //  fill in the cbSize field of the extended wave format header.
        //  this number is the number of _EXTRA BYTES_ *after* the end
        //  of the WAVEFORMATEX structure that are need for the compression
        //  format.
        //
        //  for Microsoft's 4 Bit ADPCM format, this number is 32:
        //
        lpwfxDst->cbSize = sizeof(ADPCMWaveFormat) - sizeof(WaveFormatEx) +
                            (MSADPCM_NUM_COEF * sizeof(ADPCMCoefSet));

        //
        //  copy the Microsoft 4 Bit ADPCM coef's into the header
        //
        lpwfADPCM->wNumCoef = MSADPCM_NUM_COEF;
        for (w = 0; w < MSADPCM_NUM_COEF; w++)
        {
            lpwfADPCM->aCoef[w].iCoef1 = gaiCoef1[w];
            lpwfADPCM->aCoef[w].iCoef2 = gaiCoef2[w];
        }

        return (true);
    }

    //
    //  if the source format is ADPCM, then build an appropriate PCM header
    //
    else if (lpwfxSrc->wFormatTag == WAVE_FORMAT_ADPCM)
    {
        if (!adpcmIsValidFormat(lpwfxSrc))
            return (false);

        //
        //  fill in the info for our destination format...
        //
        lpwfxDst->wFormatTag     = WAVE_FORMAT_PCM;
        lpwfxDst->nSamplesPerSec = lpwfxSrc->nSamplesPerSec;
        lpwfxDst->nChannels      = wChannels = lpwfxSrc->nChannels;

        //
        //  NOTE: bits per sample can be 8 or 16.. most people don't have
        //  16 bit boards (yet!), so default to 8 bit decoding..
        //
#if 0
        lpwfxDst->wBitsPerSample = wBitsPerSample = 16;
#else
        lpwfxDst->wBitsPerSample = wBitsPerSample = 8;
#endif

        //
        //  set nAvgBytesPerSec and nBlockAlign
        //
        lpwfxDst->nBlockAlign     = (wBitsPerSample >> 3) << (wChannels >> 1);
        lpwfxDst->nAvgBytesPerSec = lpwfxDst->nSamplesPerSec * wBitsPerSample;

        return (true);
    }

    return (false);
} /* adpcmBuildFormatHeader() */


/** EOF: msadpcm.c **/
