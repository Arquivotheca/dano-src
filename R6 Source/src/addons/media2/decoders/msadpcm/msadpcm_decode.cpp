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

#include <support2/ByteOrder.h>

#include "msadpcm_decode.h"

#include <stdio.h>

//#define DEBUG	printf
#define DEBUG	if (0) printf

//  Fixed point Delta adaption table:
//      Next Delta = Delta * gaiP4[ this output ] / MSADPCM_PSCALE
static int16  gaiP4[] = { 230, 230, 230, 230, 307, 409, 512, 614,
                          768, 614, 512, 409, 307, 230, 230, 230 };

/** uint32 FAR PASCAL adpcmDecode4Bit(LPADPCMWAVEFORMAT lpwfADPCM, HPSTR hpSrc, PcmWaveFormat* lpwfPCM, HPSTR hpDst, uint32 dwSrcLen)
 *
 *  DESCRIPTION:
 *      
 *
 *  ARGUMENTS:
 *      (LPADPCMWAVEFORMAT lpwfADPCM, HPSTR hpSrc, PcmWaveFormat* lpwfPCM, HPSTR hpDst, uint32 dwSrcLen)
 *
 *  RETURN (uint32 FAR PASCAL):
 *
 *
 *  NOTES:
 *
 **  */

uint32 adpcmDecode4Bit(ADPCMWaveFormat *lpwfADPCM, const char *hpSrc, PCMWaveFormat *lpwfPCM, char *hpDst, uint32 dwSrcLen)
{
    int16			iInput, iNextInput=0, iFirstNibble, iDelta;
    int16			aiSamp1[MSADPCM_MAX_CHANNELS];
    int16			aiSamp2[MSADPCM_MAX_CHANNELS];
    int16			aiCoef1[MSADPCM_MAX_CHANNELS];
    int16			aiCoef2[MSADPCM_MAX_CHANNELS];
    int16			aiDelta[MSADPCM_MAX_CHANNELS];
    int32			lSamp, lPrediction;
    uint8			bPredictors, bChannels, bBitsPerSample, m;
    uint16			n, wSamplesPerBlock, wBlockHeaderBytes;
    uint32			dwTotalPos, dwDecoded;
    ADPCMCoefSet	*lpCoefSet;

    //
    //  put some commonly used info in more accessible variables and init
    //  the wBlockHeaderBytes, dwDecoded and dwTotalPos vars...
    //
    lpCoefSet           = &lpwfADPCM->aCoef[0];
    bPredictors         = (uint8)lpwfADPCM->wNumCoef;
    bChannels           = (uint8)lpwfADPCM->wfx.nChannels;
    bBitsPerSample      = (uint8)lpwfPCM->wBitsPerSample;
    wSamplesPerBlock    = lpwfADPCM->wSamplesPerBlock;
    wBlockHeaderBytes   = bChannels * 7;
    dwDecoded           = 0L;
    dwTotalPos          = 0L;


    //
    //  step through each byte of ADPCM data and decode it to the requested
    //  PCM format (8 or 16 bit).
    //
    while (dwTotalPos < dwSrcLen)
    {
        //
        //  the first thing we need to do with each block of ADPCM data is
        //  to read the header which consists of 7 bytes of data per channel.
        //  so our first check is to make sure that we have _at least_
        //  enough input data for a complete ADPCM block header--if there
        //  is not enough data for the header, then exit.
        //
        //  the header looks like this:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first sample per channel
        //      2 byte second sample per channel
        //
        //  this gives us (7 * bChannels) bytes of header information. note
        //  that as int32 as there is _at least_ (7 * bChannels) of header
        //  info, we will grab the two samples from the header (and if no
        //  data exists following the header we will exit in the decode
        //  loop below).
        //
        dwTotalPos += wBlockHeaderBytes;
        if (dwTotalPos > dwSrcLen)
            goto adpcmDecode4BitExit;
            
        //
        //  grab and validate the predictor for each channel
        //
        for (m = 0; m < bChannels; m++)
        {
            uint8    bPredictor;

            bPredictor = (uint8)(*hpSrc++);
DEBUG("Predictor[%d]: %d\n", m, bPredictor);
            if (bPredictor >=  bPredictors)
            {
                //
                //  the predictor is out of range--this is considered a
                //  fatal error with the ADPCM data, so we fail by returning
                //  zero bytes decoded
                //
                dwDecoded = 0;
                goto adpcmDecode4BitExit;
            }

            //
            //  get the coefficients for the predictor index
            //
            aiCoef1[m] = lpCoefSet[bPredictor].iCoef1;
            aiCoef2[m] = lpCoefSet[bPredictor].iCoef2;
        }
        
        //
        //  get the starting delta for each channel
        //
        for (m = 0; m < bChannels; m++)
        {
            aiDelta[m] = B_LENDIAN_TO_HOST_INT16(*(int16*)hpSrc);
            hpSrc++;
            hpSrc++;
DEBUG("Delta[%d]: %d\n", m, aiDelta[m]);
       }

        //
        //  get the sample just previous to the first encoded sample per
        //  channel
        //
        for (m = 0; m < bChannels; m++)
        {
            aiSamp1[m] = B_LENDIAN_TO_HOST_INT16(*(int16*)hpSrc);
            hpSrc++;
            hpSrc++;
DEBUG("Sample 1 [%d]: %d\n", m, aiSamp1[m]);
        }

        //
        //  get the sample previous to aiSamp1[x] per channel
        //
        for (m = 0; m < bChannels; m++)
        {
            aiSamp2[m] = B_LENDIAN_TO_HOST_INT16(*(int16*)hpSrc);
            hpSrc++;
            hpSrc++;
DEBUG("Sample 2 [%d]: %d\n", m, aiSamp2[m]);
        }


        //
        //  write out first 2 samples for each channel.
        //
        //  NOTE: the samples are written to the destination PCM buffer
        //  in the _reverse_ order that they are in the header block:
        //  remember that aiSamp2[x] is the _previous_ sample to aiSamp1[x].
        //
        if (bBitsPerSample == (uint8)8)
        {
            for (m = 0; m < bChannels; m++)
            {
                *hpDst++ = (char)((aiSamp2[m] >> 8) + 128);
            }
            for (m = 0; m < bChannels; m++)
            {
                *hpDst++ = (char)((aiSamp1[m] >> 8) + 128);
            }
        }
        else
        {
            for (m = 0; m < bChannels; m++)
            {
                *(int16*)hpDst = aiSamp2[m];
                hpDst++;
                hpDst++;
            }
            for (m = 0; m < bChannels; m++)
            {
                *(int16*)hpDst = aiSamp1[m];
                hpDst++;
                hpDst++;
            }
        }

        //
        //  we have decoded the first two samples for this block, so add
        //  two to our decoded count
        //
        dwDecoded += 2;


        //
        //  we now need to decode the 'data' section of the ADPCM block.
        //  this consists of packed 4 bit nibbles.
        //
        //  NOTE: we start our count for the number of data bytes to decode
        //  at 2 because we have already decoded the first 2 samples in
        //  this block.
        //
        iFirstNibble = 1;
        for (n = 2; n < wSamplesPerBlock; n++)
        {
            for (m = 0; m < bChannels; m++)
            {
                if (iFirstNibble)
                {
                    //
                    //  we need to grab the next byte to decode--make sure
                    //  that there is a byte for us to grab before continue
                    //
                    dwTotalPos++;
                    if (dwTotalPos > dwSrcLen)
                        goto adpcmDecode4BitExit;

                    //
                    //  grab the next two nibbles and create sign extended
                    //  integers out of them:
                    //
                    //      iInput is the first nibble to decode
                    //      iNextInput will be the next nibble decoded
                    //
                    iNextInput  = (int16)*hpSrc++;
                    iInput      = iNextInput >> 4;
//                  iNextInput  = (iNextInput << 12) >> 12;  // Thanks Marco !
                    iNextInput  = iNextInput << 12;
                    iNextInput  = iNextInput >> 12;

                    iFirstNibble = 0;
                }
                else
                {
                    //
                    //  put the next sign extended nibble into iInput and
                    //  decode it--also set iFirstNibble back to 1 so we
                    //  will read another byte from the source stream on
                    //  the next iteration...
                    //
                    iInput = iNextInput;
                    iFirstNibble = 1;
                }


                //
                //  compute the next Adaptive Scale Factor (ASF) and put
                //  this value in aiDelta for the current channel.
                //
                iDelta = aiDelta[m];
                aiDelta[m] = (int16)((gaiP4[iInput & 15] * (int32)iDelta) >> MSADPCM_PSCALE);
                if (aiDelta[m] < MSADPCM_DELTA4_MIN)
                    aiDelta[m] = MSADPCM_DELTA4_MIN;

                //
                //  decode iInput (the sign extended 4 bit nibble)--there are
                //  two steps to this:
                //
                //  1.  predict the next sample using the previous two
                //      samples and the predictor coefficients:
                //
                //      Prediction = (aiSamp1[channel] * aiCoef1[channel] + 
                //              aiSamp2[channel] * aiCoef2[channel]) / 256;
                //
                //  2.  reconstruct the original PCM sample using the encoded
                //      sample (iInput), the Adaptive Scale Factor (aiDelta)
                //      and the prediction value computed in step 1 above.
                //
                //      Sample = (iInput * aiDelta[channel]) + Prediction;
                //
                lPrediction = (((int32)aiSamp1[m] * aiCoef1[m]) +
                                ((int32)aiSamp2[m] * aiCoef2[m])) >> MSADPCM_CSCALE;
                lSamp = ((int32)iInput * iDelta) + lPrediction;
DEBUG("index:%d, pred:%ld, samp:%ld, delta:%d, input:%d\n",
	  n, lPrediction, lSamp, iDelta, iInput);
                //
                //  now we need to clamp lSamp to [-32768..32767]--this value
                //  will then be a valid 16 bit sample.
                //
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;
        
                //
                //  lSamp contains the decoded iInput sample--now write it
                //  out to the destination buffer
                //
                if (bBitsPerSample == (uint8)8)
                {
                    *hpDst++ = (char)(((int16)lSamp >> 8) + 128);
                }
                else
                {
                    *(int16*)hpDst = (int16)lSamp;
                    hpDst++;
                    hpDst++;
                }

                //
                //  ripple our previous samples down making the new aiSamp1
                //  equal to the sample we just decoded
                //
                aiSamp2[m] = aiSamp1[m];
                aiSamp1[m] = (int16)lSamp;
            }

            //
            //  we have decoded one more complete sample
            //
            dwDecoded++;
        }
    }

    //
    //  we're done decoding the input data. dwDecoded contains the number
    //  of complete _SAMPLES_ that were decoded. we need to return the
    //  number of _BYTES_ decoded. so calculate the number of bytes per
    //  sample and multiply that with dwDecoded...
    //
adpcmDecode4BitExit:

    return (dwDecoded * ((bBitsPerSample >> (uint8)3) * bChannels));
} /* adpcmDecode4Bit() */
