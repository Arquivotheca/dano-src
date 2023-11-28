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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "msadpcm_encode.h"

//#define DEBUG	printf
#define DEBUG	if (0) printf


//  Fixed point Delta adaption table:
//      Next Delta = Delta * gaiP4[ this output ] / MSADPCM_PSCALE
static int16  gaiP4[] = { 230, 230, 230, 230, 307, 409, 512, 614,
                          768, 614, 512, 409, 307, 230, 230, 230 };

/** int16 NEAR PASCAL adpcmEncode4Bit_FirstDelta(int16 iCoef1, int16 iCoef2, int16 iP5, int16 iP4, int16 iP3, int16 iP2, int16 iP1)
 *
 *  DESCRIPTION:
 *      
 *
 *  ARGUMENTS:
 *      (int16 iCoef1, int16 iCoef2, int16 iP5, int16 iP4, int16 iP3, int16 iP2, int16 iP1)
 *
 *  RETURN (int16 NEAR PASCAL):
 *
 *
 *  NOTES:
 *
 **  */

static int16 adpcmEncode4Bit_FirstDelta(int16 iCoef1, int16 iCoef2, int16 iP5, int16 iP4, int16 iP3, int16 iP2, int16 iP1)
{
    int32    lTotal;
    int16   iRtn;
    int32    lTemp;

    //
    //  use average of 3 predictions
    //
    lTemp  = (((int32)iP5 * iCoef2) + ((int32)iP4 * iCoef1)) >> MSADPCM_CSCALE;
    lTotal = (lTemp > iP3) ? (lTemp - iP3) : (iP3 - lTemp);

    lTemp   = (((int32)iP4 * iCoef2) + ((int32)iP3 * iCoef1)) >> MSADPCM_CSCALE;
    lTotal += (lTemp > iP2) ? (lTemp - iP2) : (iP2 - lTemp);

    lTemp   = (((int32)iP3 * iCoef2) + ((int32)iP2 * iCoef1)) >> MSADPCM_CSCALE;
    lTotal += (lTemp > iP1) ? (lTemp - iP1) : (iP1 - lTemp);
    
    //
    //  optimal iDelta is 1/4 of prediction error
    //
    iRtn = (int16)(lTotal / 12);
    if (iRtn < MSADPCM_DELTA4_MIN)
        iRtn = MSADPCM_DELTA4_MIN;

    return (iRtn);
} /* adpcmEncode4Bit_FirstDelta() */


/** uint32 FAR PASCAL adpcmEncode4Bit(PcmWaveFormat* lpwfPCM, HPSTR hpSrc, LPADPCMWAVEFORMAT lpwfADPCM, HPSTR hpDst, uint32 dwSrcLen)
 *
 *  DESCRIPTION:
 *      
 *
 *  ARGUMENTS:
 *      (PcmWaveFormat* lpwfPCM, HPSTR hpSrc, LPADPCMWAVEFORMAT lpwfADPCM, HPSTR hpDst, uint32 dwSrcLen)
 *
 *  RETURN (uint32 FAR PASCAL):
 *
 *
 *  NOTES:
 *
 **  */

#define ENCODE_DELTA_LOOKAHEAD      5

uint32 adpcmEncode4Bit(PCMWaveFormat *lpwfPCM, char *hpSrc, ADPCMWaveFormat *lpwfADPCM,
					   char *hpDst, uint32 dwSrcLen)
{
    uint8   *lpSamplesBuf;
    uint8   *lpSamples;

    uint8   abBestPredictor[MSADPCM_MAX_CHANNELS];
    uint32  adwTotalError[MSADPCM_NUM_COEF][MSADPCM_MAX_CHANNELS];

    int16   aiSamples[ENCODE_DELTA_LOOKAHEAD][MSADPCM_MAX_CHANNELS];
    int16   aiFirstDelta[MSADPCM_NUM_COEF][MSADPCM_MAX_CHANNELS];
    int16   aiDelta[MSADPCM_MAX_CHANNELS];
    int16   aiSamp1[MSADPCM_MAX_CHANNELS];
    int16   aiSamp2[MSADPCM_MAX_CHANNELS];

    int32   iCoef1;
    int32   iCoef2;
    int32   iSamp1;
    int32   iSamp2;
    int32   iDelta;
    int32   iSample;
    int32   iOutput;

    int32   lSamp;

    uint32  dw;
    uint32  dwTotalConverted;
    uint32  dwInputPos;

    int32   lError;
    int32   lPrediction;

    uint32  wSamplesPerBlock;
    uint32  cbSample;
    uint32  wBlockHeaderBytes;
    uint32  wBlockSize;
    uint32  wNextWrite;
    uint32  wFirstNibble;
    uint32  n;
    uint8   m;
    uint8   i;
    uint8   bChannels;
    uint8   bBitsPerSample;

    //
    //  first copy some information into more accessible (cheaper and shorter)
    //  variables--and precompute some stuff...
    //
    wSamplesPerBlock    = lpwfADPCM->wSamplesPerBlock;
    bChannels           = (uint8)lpwfPCM->wf.nChannels;
    bBitsPerSample      = (uint8)lpwfPCM->wBitsPerSample;
    wBlockHeaderBytes   = bChannels * 7;
    dwInputPos          = 0L;
    dwTotalConverted    = 0L;

    //
    //  calculate the number of bytes per sample in the PCM data
    //
    cbSample = (bBitsPerSample >> 3) * bChannels;

    //
    //  we allocate and use a small buffer (<64k) to hold an entire block of
    //  samples to be encoded. the reason for this is mainly because we
    //  make 8 passes over the data per block and using huge pointers all
    //  of the time is expensive. so we put it in a <64k chunk and eliminate
    //  the huge pointer stuff
    //
//    lpSamplesBuf = (char*)malloc(wSamplesPerBlock * cbSample);
//    if (!lpSamplesBuf)
//        return (0L);

    //
    //  step through each block of PCM data and encode it to 4 bit ADPCM
    //
    for ( ; dwInputPos < dwSrcLen; dwInputPos += (wBlockSize * cbSample))
    {
        //
        //  determine how much data we should encode for this block--this
        //  will be wSamplesPerBlock until we hit the last chunk of PCM
        //  data that will not fill a complete block. so on the last block
        //  we only encode that amount of data remaining...
        //
        dw = (dwSrcLen - dwInputPos) / cbSample;
//      wBlockSize = (uint16)min((uint32)wSamplesPerBlock, dw);
        if ((uint32)wSamplesPerBlock > dw)
			wBlockSize = (uint16)dw;
		else
			wBlockSize = (uint16)wSamplesPerBlock;
	
        if (!wBlockSize)
            break;

        //
        //  copy the samples that we will be encoding into our small data
        //  samples buffer (this is faster than doing huge pointer arith
        //  all of the time)
        //
        lpSamplesBuf = (uint8*)hpSrc+dwInputPos;
//      memcpy(lpSamplesBuf, &hpSrc[dwInputPos], wBlockSize * cbSample);

        //
        //  find the optimal predictor for each channel: to do this, we
        //  must step through and encode using each coefficient set (one
        //  at a time) and determine which one has the least error from
        //  the original data. the one with the least error is then used
        //  for the final encode (the 8th pass done below).
        //
        //  NOTE: keeping the encoded data of the one that has the least
        //  error at all times is an obvious optimization that should be
        //  done. in this way, we only need to do 7 passes instead of 8.
        //
        for (i = 0; i < MSADPCM_NUM_COEF; i++)
        {
            //
            //  reset pointer to beginning of our temporary small data
            //  samples buffer
            //
            lpSamples = lpSamplesBuf;

            //
            //  copy the coefficient pair for the current coefficient set
            //  we are using into more convenient/cheaper variables
            //
            iCoef1 = lpwfADPCM->aCoef[i].iCoef1;
            iCoef2 = lpwfADPCM->aCoef[i].iCoef2;

            //
            //  we need to choose the first iDelta--to do this, we need
            //  to look at the first few samples. for convenience, we copy
            //  the first ENCODE_DELTA_LOOKAHEAD samples (converted to 
            //  16 bit samples) into a temporary buffer
            //
            for (n = 0; n < ENCODE_DELTA_LOOKAHEAD; n++)
            {
                for (m = 0; m < bChannels; m++)
                {
                    if (bBitsPerSample == 16) {
                        iSample = *(int16*)lpSamples;
                        lpSamples += 2;
                    }
                    else if (bBitsPerSample == 8) {
                    	iSample = ((int16)*lpSamples - 128) << 8;
                    	lpSamples += 1;
                    }
                    else {
                    	iSample = (int16)(*((float*)lpSamples)*32767.0);
                    	lpSamples += 4;
                    }

                    aiSamples[n][m] = iSample;
                }
            }

            //
            //  now choose the first iDelta and setup the first two samples
            //  based on the 16 samples we copied from the source data above
            //  for each channel
            //
            for (m = 0; m < bChannels; m++)
            {
                //
                //  reset total error calculation for new block
                //
                adwTotalError[i][m] = 0L;

                //
                //  first 2 samples will come from real data--compute
                //  starting from there
                //
                aiSamp1[m] = aiSamples[1][m];
                aiSamp2[m] = aiSamples[0][m];
                        
                //
                //  calculate initial iDelta
                //
                iDelta = adpcmEncode4Bit_FirstDelta(iCoef1, iCoef2,
                        aiSamples[0][m], aiSamples[1][m], aiSamples[2][m],
                        aiSamples[3][m], aiSamples[4][m]);
                aiDelta[m] = aiFirstDelta[i][m] = iDelta;
            }

            //
            //  step over first two complete samples--we took care of them
            //  above
            //
            lpSamples = &lpSamplesBuf[cbSample * 2];

            //
            //  now encode the rest of the PCM data in this block--note
            //  we start 2 samples ahead because the first two samples are
            //  simply copied into the ADPCM block header...
            //
            for (n = 2; n < wBlockSize; n++)
            {
                //
                //  each channel gets encoded independently... obviously.
                //
                for (m = 0; m < bChannels; m++)
                {
                    //
                    //  yes, copy into cheaper variables because we access
                    //  them a lot
                    //
                    iSamp1 = aiSamp1[m];
                    iSamp2 = aiSamp2[m];
                    iDelta = aiDelta[m];

                    //
                    //  calculate the prediction based on the previous two
                    //  samples
                    //
                    lPrediction = ((int32)iSamp1 * iCoef1 +
                                    (int32)iSamp2 * iCoef2) >> MSADPCM_CSCALE;

                    //
                    //  grab the sample (for the current channel) to encode
                    //  from the source and convert it to a 16 bit data if
                    //  necessary
                    //
                    if (bBitsPerSample == 16) {
                        iSample = *(int16*)lpSamples;
                        lpSamples += 2;
                    }
                    else if (bBitsPerSample == 8) {
                    	iSample = ((int16)*lpSamples - 128) << 8;
                    	lpSamples += 1;
                    }
                    else {
                    	iSample = (int16)(*((float*)lpSamples)*32767.0);
                    	lpSamples += 4;
                    }

                    //
                    //  encode it
                    //
                    lError = (int32)iSample - lPrediction;
                    iOutput = (int32)(lError / iDelta);
                    if (iOutput > MSADPCM_OUTPUT4_MAX)
                        iOutput = MSADPCM_OUTPUT4_MAX;
                    else if (iOutput < MSADPCM_OUTPUT4_MIN)
                        iOutput = MSADPCM_OUTPUT4_MIN;

                    lSamp = lPrediction + ((int32)iDelta * iOutput);
        
                    if (lSamp > 32767)
                        lSamp = 32767;
                    else if (lSamp < -32768)
                        lSamp = -32768;
        
                    //
                    //  compute the next iDelta
                    //
                    iDelta = (int32)((gaiP4[iOutput & 15] * (int32)iDelta) >> MSADPCM_PSCALE);
                    if (iDelta < MSADPCM_DELTA4_MIN)
                        iDelta = MSADPCM_DELTA4_MIN;
        
                    //
                    //  save updated values for this channel back into the
                    //  original arrays...
                    //
                    aiDelta[m] = iDelta;
                    aiSamp2[m] = iSamp1;
                    aiSamp1[m] = (int16)lSamp;

                    //
                    //  keep a running status on the error for the current
                    //  coefficient pair for this channel
                    //
                    lError = lSamp - iSample;
                    adwTotalError[i][m] += (lError * lError) >> 7;
                }
            }
        }


        //
        //  WHEW! we have now made 7 passes over the data and calculated
        //  the error for each--so it's time to find the one that produced
        //  the lowest error and use that predictor (this is for each
        //  channel of course)
        //
        for (m = 0; m < bChannels; m++)
        {
            abBestPredictor[m] = 0;
            dw = adwTotalError[0][m];
            for (i = 1; i < MSADPCM_NUM_COEF; i++)
            {
                if (adwTotalError[i][m] < dw)
                {
                    abBestPredictor[m] = i;
                    dw = adwTotalError[i][m];
                }
            }
        }    
        

        //
        //  reset pointer to beginning of our temporary samples buffer--
        //  we're going to make our final pass on the data with the optimal
        //  predictor that we found above
        //
        lpSamples = lpSamplesBuf;


        //
        //  grab first iDelta from our precomputed first deltas that we
        //  calculated above
        //
        for (m = 0; m < bChannels; m++)
        {
            i = abBestPredictor[m];
            aiDelta[m] = aiFirstDelta[i][m];
        }

        //
        //  get the first two samples from the source data (so we can write
        //  them into the ADPCM block header and use them in our prediction
        //  calculation when encoding the rest of the data).
        //
        if (bBitsPerSample == 16)
        {
            for (m = 0; m < bChannels; m++)
            {
                aiSamp2[m] = *(int16*)lpSamples;
                lpSamples++;
                lpSamples++;
            }
            for (m = 0; m < bChannels; m++)
            {
                aiSamp1[m] = *(int16*)lpSamples;
                lpSamples++;
                lpSamples++;
            }
        }
        else if (bBitsPerSample == 8)
        {
            for (m = 0; m < bChannels; m++)
            {
                aiSamp2[m] = ((int16)*lpSamples++ - 128) << 8;
            }
            for (m = 0; m < bChannels; m++)
            {
                aiSamp1[m] = ((int16)*lpSamples++ - 128) << 8;
            }
        }
        else
        {
            for (m = 0; m < bChannels; m++)
            {
                aiSamp2[m] = (int16)(*((float*)lpSamples)*32767.0);
                lpSamples += 4;
            }
            for (m = 0; m < bChannels; m++)
            {
                aiSamp1[m] = (int16)(*((float*)lpSamples)*32767.0);
                lpSamples += 4;
            }
        }

        //
        //  write the block header for the encoded data
        //
        //  the block header is composed of the following data:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first sample per channel
        //      2 byte second sample per channel
        //
        //  this gives us (7 * bChannels) bytes of header information
        //
        //  so first write the 1 byte predictor for each channel into the
        //  destination buffer
        //
        for (m = 0; m < bChannels; m++)
        {
            *hpDst++ = abBestPredictor[m];
DEBUG("Predictor[%d]: %d\n", m, abBestPredictor[m]);
        }

        //
        //  now write the 2 byte delta per channel...
        //
        for (m = 0; m < bChannels; m++)
        {
            *(int16*)hpDst = aiDelta[m];
            hpDst++;
            hpDst++;
DEBUG("Delta[%d]: %d\n", m, aiDelta[m]);
        }

        //
        //  finally, write the first two samples (2 bytes each per channel)
        //
        for (m = 0; m < bChannels; m++)
        {
            *(int16*)hpDst = aiSamp1[m];
            hpDst++;
            hpDst++;
DEBUG("Sample 1 [%d]: %d\n", m, aiSamp1[m]);
        }
        for (m = 0; m < bChannels; m++)
        {
            *(int16*)hpDst = aiSamp2[m];
            hpDst++;
            hpDst++;
DEBUG("Sample 2 [%d]: %d\n", m, aiSamp2[m]);
        }

        //
        //  the number of bytes that we have written to the destination
        //  buffer is (7 * bChannels)--so add this to our total number of
        //  bytes written to the destination.. for our return value.
        //
        dwTotalConverted += wBlockHeaderBytes;
        

        //
        //  we have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles). note that
        //  we start our count at 2 because we already wrote the first
        //  two samples into the destination buffer as part of the header
        //
        wFirstNibble = 1;
        for (n = 2; n < wBlockSize; n++)
        {
            //
            //  each channel gets encoded independently... obviously.
            //
            for (m = 0; m < bChannels; m++)
            {
                //
                //  use our chosen best predictor and grab the coefficient
                //  pair to use for this channel...
                //
                i = abBestPredictor[m];
                iCoef1 = lpwfADPCM->aCoef[i].iCoef1;
                iCoef2 = lpwfADPCM->aCoef[i].iCoef2;

                //
                //  copy into cheaper variables because we access them a lot
                //
                iSamp1 = aiSamp1[m];
                iSamp2 = aiSamp2[m];
                iDelta = aiDelta[m];

                //
                //  calculate the prediction based on the previous two samples
                //
                lPrediction = ((int32)iSamp1 * iCoef1 +
                                (int32)iSamp2 * iCoef2) >> MSADPCM_CSCALE;
//DEBUG("(%d * %d + %d * %d) >> %d -> %d\n",
//	  iSamp1, iCoef1, iSamp2, iCoef2, MSADPCM_CSCALE, lPrediction);

                //
                //  grab the sample to encode--convert it to 16 bit data if
                //  necessary...
                //
                if (bBitsPerSample == 16) {
                    iSample = *(int16*)lpSamples;
                    lpSamples += 2;
                }
                else if (bBitsPerSample == 8) {
                   	iSample = ((int16)*lpSamples - 128) << 8;
                   	lpSamples += 1;
                }
                else {
                   	iSample = (int16)(*((float*)lpSamples)*32767.0);
                   	lpSamples += 4;
                }

                //
                //  encode the sample
                //
                lError = (int32)iSample - lPrediction;
                iOutput = (int16)(lError / iDelta);
                if (iOutput > MSADPCM_OUTPUT4_MAX)
                    iOutput = MSADPCM_OUTPUT4_MAX;
                else if (iOutput < MSADPCM_OUTPUT4_MIN)
                    iOutput = MSADPCM_OUTPUT4_MIN;

                lSamp = lPrediction + ((int32)iDelta * iOutput);
            
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;

                //
                //  compute the next iDelta
                //
                iDelta = (int16)((gaiP4[iOutput&15] * (int32)iDelta) >> MSADPCM_PSCALE); 
                if (iDelta < MSADPCM_DELTA4_MIN)
                    iDelta = MSADPCM_DELTA4_MIN;

                //
                //  save updated values for this channel back into the
                //  original arrays...
                //
                aiDelta[m] = iDelta;
                aiSamp2[m] = iSamp1;
                aiSamp1[m] = (int16)lSamp;
DEBUG("index:%d, Cpred: %d, pred:%d, samp:%d, delta:%d, output:%d\n",
	  n, abBestPredictor[m], lPrediction, lSamp, iDelta, iOutput);

                //
                //  we have another nibble of encoded data--either combine
                //  this with the previous nibble and write out a full 
                //  byte, or save this nibble for the next nibble to be
                //  combined into a full byte and written to the destination
                //  buffer... uhg!
                //
                if (wFirstNibble)
                {
                    wNextWrite = (iOutput & 15) << 4;
                    wFirstNibble = 0;
                }
                else
                {
                    *hpDst++ = (uint8)(wNextWrite | (iOutput & 15));
                    dwTotalConverted += 1;
                    wFirstNibble++;
                }
            }
        }
    }

    //
    //  free the memory used for our small data buffer and return the number
    //  of bytes that we wrote into the destination buffer...
    //
//    free(lpSamplesBuf);

    return (dwTotalConverted);
} /* adpcmEncode4Bit() */
