//
//  Copyright (c) 2000, Hewlett-Packard Co.
//  All rights reserved.
//  
//  This software is licensed solely for use with HP products.  Redistribution
//  and use with HP products in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//  
//  -	Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//  -	Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//  -	Neither the name of Hewlett-Packard nor the names of its contributors
//      may be used to endorse or promote products derived from this software
//      without specific prior written permission.
//  -	Redistributors making defect corrections to source code grant to
//      Hewlett-Packard the right to use and redistribute such defect
//      corrections.
//  
//  This software contains technology licensed from third parties; use with
//  non-HP products is at your own risk and may require a royalty.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
//  CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL HEWLETT-PACKARD OR ITS CONTRIBUTORS
//  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
//  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
//  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
//  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//  DAMAGE.
//
#ifdef _DJ9xxVIP

#ifdef PROTO
#include "..\include\Header.h"
#else
#include "Header.h"
#endif

// copied from vob \di_research on 10/31/00
// MODIFICATIONS BY GE:
// 0. remove Windows header references
// 1. define assert
// 2. set iRastersReady, iRastersDelivered in submitrowtofilter
// 3. (constructor) allocate (and delete in destructor) buffers for fRowPtr 
//      (instead of setting it to input buffers, since we reuse input buffers)
// 4. copy data into fRowPtr in submitrowtofilter

#define assert ASSERT

#include "ernieplatform.h"
#include "filterhpa.h"

#if kGatherStats == 1
extern int blockStats[];
#endif

#if ((kMemWritesOptimize != 1) && (kMemWritesOptimize != 0))
#error "kMemWritesOptimize must be 0 or 1"
#endif


inline void AverageNRound(bool roundGreenDown, int &rFinal, int &r0, int &r1, int &gFinal, int &g0, int &g1, int &bFinal, int &b0, int &b1);
inline void AverageNRound(bool roundGreenDown, int &rFinal, int &r0, int &r1, int &gFinal, int &g0, int &g1, int &bFinal, int &b0, int &b1)
{
    // By rounding G in the other direction than R and B L* variations are minimized while mathematically alternate rounding is accomplished. EGW 2 Dec. 1999.
    if (roundGreenDown)
    {
        rFinal = (r0 + r1 + 1) / 2;
        gFinal = (g0 + g1) / 2;
        bFinal = (b0 + b1 + 1) / 2;
    }
    else
    {
        rFinal = (r0 + r1) / 2;
        gFinal = (g0 + g1 + 1) / 2;
        bFinal = (b0 + b1) / 2;
    }
}


// Filter1RawRow.  To be used to filter an odd row for which we don't have a pair,
// found at the bottom of bands that aren't divisable by 2.  This routine
// filters its row horizontally forming 4x1 and 2x1 blocks.
void TErnieFilter::Filter1RawRow(unsigned char *currPtr, int rowWidthInPixels, unsigned int *flagsPtr)
{
    assert(currPtr);
    assert(rowWidthInPixels > 0); 

    int R0, G0, B0, R1, G1, B1, lastR, lastG, lastB;
    const unsigned int maxErrorForFourPixels = fMaxErrorForTwoPixels / 2;
//    const unsigned int maxErrorForEightPixels = maxErrorForFourPixels / 2;

    int currPixel, lastPixel;
    bool lastPairAveraged = false;
    bool last2by2Averaged = false;


    for (int pixelNum = 0; pixelNum < rowWidthInPixels; pixelNum++)
    {
        if ((pixelNum & 0x03) == 0x00) // 0,4,8...
        {
            last2by2Averaged = false; // Reinitialize every four columns;
        }

        currPixel = get4Pixel(currPtr);

        flagsPtr[0] = (e11n|e11s);  // Initialize in case nothing is found for this column

        if (isWhite(currPixel))
        {
            flagsPtr[0] = eDone;
#if kGatherStats == 1
            blockStats[esWhiteFound]++;
#endif
        }
        
        // Currently we bail entirely if there is white. Later we may still do RLE on the non white pixel if one is present.
        if (flagsPtr[0] == (e11n|e11s))
        {           
            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
        
            // Can only horizontally average every other pixel, much like the 2x2 blocks.
            if (isOdd(pixelNum))
            {
                // do horizontal on current raster
                lastPixel = get4Pixel(currPtr,-1);
                lastR = GetRed(lastPixel);
                lastG = GetGreen(lastPixel);
                lastB = GetBlue(lastPixel);
                if ((fMaxErrorForTwoPixels >= 3) && (NewDeltaE(lastR, R1, lastG, G1, lastB, B1, fMaxErrorForTwoPixels)))
                {
                    /*   - -
                        |   | build 2x1
                         - -
                    */
                    int didNotBuild4by1 = true;
#if kGatherStats == 1
                    blockStats[es21nw]++;
#endif
                    AverageNRound(isOdd(pixelNum), lastR, lastR, R1, lastG, lastG, G1, lastB, lastB, B1);
                    if ((pixelNum >= 3) && (flagsPtr[-3] & e21nw)) // 4,5,6,7,12,13,14,15,20...
                    {
                        // Look for a 4x1
                        assert(!((flagsPtr[-3] | flagsPtr[-2] | flagsPtr[-1] | flagsPtr[0]) & eTheRest)); // no vertical blocks

                        lastPixel = get4Pixel(currPtr,-3);
                        R0 = GetRed(lastPixel);
                        G0 = GetGreen(lastPixel);
                        B0 = GetBlue(lastPixel);
                        if ((maxErrorForFourPixels >= 3) && (NewDeltaE(lastR, R0, lastG, G0, lastB, B0, maxErrorForFourPixels)))
                        {
                            /*   - - - -
                                |       | build 4x1
                                 - - - -
                            */
#if kGatherStats == 1
                            blockStats[es41ni]++;
#endif
                            didNotBuild4by1 = false;
                            AverageNRound((pixelNum & 0x04)== 0x04, lastR, lastR, R0, lastG, lastG, G0, lastB, lastB, B0); // 4,5,6,7,12,13,14,15,20...

                            currPixel = (lastR<<16) + (lastG<<8) + lastB;

#if kMemWritesOptimize == 0
                            put4Pixel(currPtr, -3, currPixel);
                            put4Pixel(currPtr, -2, currPixel);
                            put4Pixel(currPtr, -1, currPixel);
                            put4Pixel(currPtr, 0, currPixel);
#else
                            put4Pixel(currPtr, -3, currPixel);
#endif                      
                            flagsPtr[-3] = (flagsPtr[-3] & ~eNorths) | e41ni;
                            flagsPtr[-2] = (flagsPtr[-2] & ~eNorths) | e41n;
                            flagsPtr[-1] = (flagsPtr[-1] & ~eNorths) | e41n;
                            flagsPtr[0] = (flagsPtr[0] & ~eNorths) | e41n;
                        }
                    }

                    if (didNotBuild4by1) // Not a 4x1 so output 2x1.
                    {
                        assert(!((flagsPtr[-1] | flagsPtr[0]) & eTheRest)); // no vertical blocks

                        currPixel = (lastR<<16) + (lastG<<8) + lastB;

#if kMemWritesOptimize == 0
                        put4Pixel(currPtr, -1, currPixel);
                        put4Pixel(currPtr, 0, currPixel);
#else
                        put4Pixel(currPtr, -1, currPixel);
#endif
                        flagsPtr[-1] = (flagsPtr[-1] & ~eNorths) | e21nw;
                        flagsPtr[0] = (flagsPtr[0] & ~eNorths) | e21ne;
                    }
                }  // If DeltaE... Looking for two by one
            }  // IsOdd(pixelNum)
        }
        else // no flag bits set.
        {
            lastPairAveraged = false; // EGW Fixes bug on business graphics. 11/24/97
        }
        
        currPtr += eBufferedPixelWidthInBytes;
        flagsPtr++;
    }      // for each pixel...
}

// Filter2RawRows:  Looks filter two raw rows together to form blocks.  Vertical
// blocks are prefered over horizontal ones.  The routine will create 1x2 blocks
// before it will create 4x1's.  In total this routine will create 1x2, 2x2, 4x2,
// 4x1, and 2x1 blocks sizes, with the potential for two seperate 4x1's or 2x1's
// in the upper and lower rasters.
void TErnieFilter::Filter2RawRows(unsigned char *currPtr, unsigned char *upPtr, int rowWidthInPixels, unsigned int *flagsPtr)
{
    assert(currPtr);
    assert(upPtr);
    assert(rowWidthInPixels > 0); 

    int R0, G0, B0, R1, G1, B1, lastR, lastG, lastB;
    const unsigned int maxErrorForFourPixels = fMaxErrorForTwoPixels / 2;
    const unsigned int maxErrorForEightPixels = maxErrorForFourPixels / 2;

    int currPixel, upPixel, lastPixel;
    bool lastPairAveraged = false;
    bool last2by2Averaged = false;


    for (int pixelNum = 0; pixelNum < rowWidthInPixels; pixelNum++)
    {
        if ((pixelNum & 0x03) == 0x00) // 0,4,8...
        {
            last2by2Averaged = false; // Reinitialize every four columns;
        }

        upPixel = get4Pixel(upPtr);
        currPixel = get4Pixel(currPtr);

        flagsPtr[0] = (e11n|e11s);  // Initialize in case nothing is found for this column

        if (isWhite(upPixel) && isWhite(currPixel)) // both white?
        {
            flagsPtr[0] = eDone;
#if kGatherStats == 1
            blockStats[esWhiteFound]++;
#endif
        }
        
        // Do vertical average on the current 2 pixel high column

        // Currently we bail entirely if there is white. Later we may still do RLE on the non white pixel if one is present.
        if (flagsPtr[0] == (e11n|e11s))
        {           
            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
    
            R0 = GetRed(upPixel);
            G0 = GetGreen(upPixel);
            B0 = GetBlue(upPixel);
    
            if ((fMaxErrorForTwoPixels >= 3) && (NewDeltaE(R0, R1, G0, G1, B0, B1, fMaxErrorForTwoPixels)))
            {
                /*   _
                    | | build 1x2
                    | |
                     -
                */
                assert(flagsPtr[0] == (e11n|e11s));
                flagsPtr[0] = e12;
#if kGatherStats == 1
                blockStats[es12]++;
#endif
                R1 = GetRed(currPixel);
                G1 = GetGreen(currPixel);
                B1 = GetBlue(currPixel);
    
                R0 = GetRed(upPixel);
                G0 = GetGreen(upPixel);
                B0 = GetBlue(upPixel);

                AverageNRound(isOdd(pixelNum), R1, R1, R0, G1, G1, G0, B1, B1, B0);

                // look for a 2x2 block average on every other column
                if (isOdd(pixelNum))
                {   // It looks like we are at the end of a 2x2 block
                    if (lastPairAveraged)
                    {
                        // Last pair was averaged so it's ok to try to make a 2x2 block
                        if ((maxErrorForFourPixels >= 3) && (NewDeltaE(lastR, R1, lastG, G1,lastB, B1, maxErrorForFourPixels)))
                        {
                            /* - -
                              |   | build 2x2
                              |   |
                               - -
                            */
                            assert(flagsPtr[-1] == e12);
                            int didNotBuild4by2 = true;
#if kGatherStats == 1
                            blockStats[es22w]++;
#endif
                            flagsPtr[-1] = e22w;
                            flagsPtr[0] = e22e;

                            AverageNRound((pixelNum & 0x02) == 0x02, R1, R1, lastR, G1, G1, lastG, B1, B1, lastB); // 2,3,6,7... Alternate between rounding up and down for these 2x2 blocks

                            if ((pixelNum & 0x03) == 0x03)  // 3,7,11,15... Looking for a 4x2 block to average
                            {
                                if (last2by2Averaged)
                                {   
                                    /*   - -   - -
                                        |   | |   | We have two 2x2s.
                                        |   | |   | 
                                         - -   - -
                                    */
                                    
                                    lastPixel = get4Pixel(upPtr, -3); // Go back to previous 2x2 block and get the pixel
                                    lastR = GetRed(lastPixel);
                                    lastG = GetGreen(lastPixel);
                                    lastB = GetBlue(lastPixel);
                                    if ((maxErrorForEightPixels >= 3) && (NewDeltaE(lastR, R1, lastG, G1,lastB, B1, maxErrorForEightPixels)))
                                    {


                                        /* - - - -
                                          |       | build 4x2.
                                          |       | 
                                           - - - -
                                        */
#if kGatherStats == 1
                                        blockStats[es42i]++;
#endif
                                        didNotBuild4by2 = false;
                                        
                                        flagsPtr[-3] = e42i;
                                        flagsPtr[-2] = flagsPtr[-1] = flagsPtr[0] = e42;

                                        AverageNRound((pixelNum & 0x04) == 0x04, R1, R1, lastR, G1, G1, lastG, B1, B1, lastB); // 4,5,6,7,12,13,14,15,20... Alternate between rounding up down for these 4x2 blocks

                                        currPixel = (R1<<16) + (G1<<8) + B1;

#if kMemWritesOptimize == 0
                                        put4Pixel(upPtr, -3, currPixel);
                                        put4Pixel(upPtr, -2, currPixel);
                                        put4Pixel(upPtr, -1, currPixel);
                                        put4Pixel(upPtr,  0, currPixel);
                                        put4Pixel(currPtr, -3, currPixel);
                                        put4Pixel(currPtr, -2, currPixel);
                                        put4Pixel(currPtr, -1, currPixel);
                                        put4Pixel(currPtr, 0, currPixel);
#else
                                        put4Pixel(upPtr, -3, currPixel);
#endif              
                                    }
                                }

                                if (didNotBuild4by2)
                                {   // The first 2x2 block of this pair of 2x2 blocks wasn't averaged.
                                    /*    - -    - -
                                         |X X|  |   | not averaged block and averaged 2x2.
                                         |X X|  |   |
                                          - -    - -
                                    */

                                    last2by2Averaged = true;

                                    currPixel = (R1<<16) + (G1<<8) + B1;

#if kMemWritesOptimize == 0
                                    put4Pixel(upPtr, -1, currPixel);
                                    put4Pixel(upPtr, 0, currPixel);
                                    put4Pixel(currPtr, -1, currPixel);
                                    put4Pixel(currPtr, 0, currPixel);
#else
                                    put4Pixel(upPtr, -1, currPixel);
#endif
                                }
                            }
                            else  // Not looking for a 4x2 block yet so just output this 2x2 block for now.
                            {
                                /*    - -    - -
                                     |   |  |? ?| 1st 2x2 and maybe another later.
                                     |   |  |? ?|
                                      - -    - -
                                */
                                
                                last2by2Averaged = true;

                                currPixel = (R1<<16) + (G1<<8) + B1;

#if kMemWritesOptimize == 0
                                put4Pixel(upPtr, -1, currPixel);
                                put4Pixel(upPtr, 0, currPixel);
                                put4Pixel(currPtr, -1, currPixel);
                                put4Pixel(currPtr, 0, currPixel);
#else
                                put4Pixel(upPtr, -1, currPixel);
#endif
                            }
                        }
                        else  // The two averaged columns are not close enough in Delta E
                        {
                            /*  -    _
                               | |  | | 2 1x2 blocks
                               | |  | |
                                -    -
                            */
                            
                            last2by2Averaged = false;

                            currPixel = (R1<<16) + (G1<<8) + B1;

#if kMemWritesOptimize == 0
                            put4Pixel(upPtr, 0, currPixel);
                            put4Pixel(currPtr, 0, currPixel);
#else
                            put4Pixel(upPtr,0, currPixel);
#endif
                        }
                        lastR = R1;
                        lastG = G1;
                        lastB = B1;
                        lastPairAveraged = true;
                    }
                    else  // This is the right place for 2x2 averaging but the previous column wasn't averaged
                    {
                        /*     -
                            X | | Two non averaged pixels and a 1x2.
                            X | |
                               -
                        */
                        last2by2Averaged = false;
                        lastPairAveraged = true;
                        lastR = R1;
                        lastG = G1;
                        lastB = B1;

                        currPixel = (R1<<16) + (G1<<8) + B1;

#if kMemWritesOptimize == 0
                        put4Pixel(upPtr, 0, currPixel);
                        put4Pixel(currPtr, 0, currPixel);
#else
                        put4Pixel(upPtr, 0, currPixel);
#endif
                    }
                }
                else  // Not on the boundary for a 2x2 block, so just output current averaged 1x2 column
                {
                    /*    -
                         | | ?  1x2
                         | | ?
                          -
                    */
                    
                    lastPairAveraged = true;
                    lastR = R1;
                    lastG = G1;
                    lastB = B1;

                    currPixel = (R1<<16) + (G1<<8) + B1;

#if kMemWritesOptimize == 0
                    put4Pixel(upPtr, 0, currPixel);
                    put4Pixel(currPtr, 0, currPixel);
#else
                    put4Pixel(upPtr, 0, currPixel);
#endif
                }
            }
            else if (lastPairAveraged)
            {   // This is the case where we can't average current column and the last column was averaged.
                // Don't do anything if last pair was averaged and this one can't be

                /*    -
                     | | X 1x2 averaged block and two non averaged pixels.
                     | | X
                      -
                */

                lastPairAveraged = false;
            }
            else
             // can't vertically average current column so look for some horizontal averaging as a fallback
             // Only do it if the last pair wasn't averaged either because we don't want to mess up a vertical averaging
             // just to create a possible horizontal averaging.
            {
                // Can only horizontally average every other pixel, much like the 2x2 blocks.
                if (isOdd(pixelNum))
                {
                    // do horizontal averaging on previous raster
                    lastPixel = get4Pixel(upPtr,-1);
                    lastR = GetRed(lastPixel);
                    lastG = GetGreen(lastPixel);
                    lastB = GetBlue(lastPixel);
                    if (((fMaxErrorForTwoPixels >= 3)) && (NewDeltaE(lastR, R0, lastG, G0,lastB, B0, fMaxErrorForTwoPixels)))
                    {
                        /*   - -
                            |   | build upper 2x1
                             - -
                        */
#if kGatherStats == 1
                        blockStats[es21nw]++;
#endif
                        int didNotBuild4by1 = true;
                        
                        AverageNRound(isOdd(pixelNum), lastR, lastR, R0, lastG, lastG, G0, lastB, lastB, B0);

                        if ((pixelNum >= 3) && (flagsPtr[-3] & e21nw)) // 4,5,6,7,12,13,14,15,20...
                        {
                            assert(!((flagsPtr[-3] | flagsPtr[-2] | flagsPtr[-1] | flagsPtr[0]) & eTheRest)); // no vertical blocks

                            // Attempt an upper 4x1
                            lastPixel = get4Pixel(upPtr,-3);
                            R0 = GetRed(lastPixel);
                            G0 = GetGreen(lastPixel);
                            B0 = GetBlue(lastPixel);
                            if ( (maxErrorForFourPixels >= 3) && (NewDeltaE(lastR, R0, lastG, G0,lastB, B0, maxErrorForFourPixels)))
                            {
                                /*   - - - -
                                    |       | build upper 4x1
                                     - - - -
                                */
#if kGatherStats == 1
                                blockStats[es41ni]++;
#endif
                                didNotBuild4by1 = false;
                                AverageNRound((pixelNum & 0x04)== 0x04, lastR, lastR, R0, lastG, lastG, G0, lastB, lastB, B0); // 4,5,6,7,12,13,14,15,20...

                                currPixel = (lastR<<16) + (lastG<<8) + lastB;

#if kMemWritesOptimize == 0
                                put4Pixel(upPtr, -3, currPixel);
                                put4Pixel(upPtr, -2, currPixel);
                                put4Pixel(upPtr, -1, currPixel);
                                put4Pixel(upPtr, 0, currPixel);
#else
                                put4Pixel(upPtr, -3, currPixel);
#endif

                                assert(!((flagsPtr[-3] | flagsPtr[-2] | flagsPtr[-1] | flagsPtr[0]) & eTheRest)); // no vertical blocks

                                flagsPtr[-3] = (flagsPtr[-3] & ~eNorths) | e41ni;
                                flagsPtr[-2] = (flagsPtr[-2] & ~eNorths) | e41n;
                                flagsPtr[-1] = (flagsPtr[-1] & ~eNorths) | e41n;
                                flagsPtr[0] = (flagsPtr[0] & ~eNorths) | e41n;
                            }
                        }
                        
                        if (didNotBuild4by1) // Not an upper 4x1 so output upper 2x1.
                        {
                            currPixel = (lastR<<16) + (lastG<<8) + lastB;

#if kMemWritesOptimize == 0
                            put4Pixel(upPtr, -1, currPixel);
                            put4Pixel(upPtr, 0, currPixel);
#else
                            put4Pixel(upPtr, -1, currPixel);
#endif
                            assert(!((flagsPtr[-1] | flagsPtr[0]) & eTheRest)); // no vertical blocks
                            flagsPtr[-1] = (flagsPtr[-1] & ~eNorths) | e21nw;
                            flagsPtr[0] = (flagsPtr[0] & ~eNorths) | e21ne;
                        }
                    }
    
                    // do horizontal on current raster
                    lastPixel = get4Pixel(currPtr,-1);
                    lastR = GetRed(lastPixel);
                    lastG = GetGreen(lastPixel);
                    lastB = GetBlue(lastPixel);
                    if ((fMaxErrorForTwoPixels >= 3) && (NewDeltaE(lastR, R1, lastG, G1, lastB, B1, fMaxErrorForTwoPixels)))
                    {
                        /*   - -
                            |   | build lower 2x1
                             - -
                        */
                        int didNotBuild4by1 = true;
#if kGatherStats == 1
                        blockStats[es21sw]++;
#endif
                        AverageNRound(isOdd(pixelNum), lastR, lastR, R1, lastG, lastG, G1, lastB, lastB, B1);
                        if ((pixelNum >= 3) && (flagsPtr[-3] & e21sw)) // 4,5,6,7,12,13,14,15,20...
                        {
                            // Look for a lower 4x1
                            assert(!((flagsPtr[-3] | flagsPtr[-2] | flagsPtr[-1] | flagsPtr[0]) & eTheRest)); // no vertical blocks

                            lastPixel = get4Pixel(currPtr,-3);
                            R0 = GetRed(lastPixel);
                            G0 = GetGreen(lastPixel);
                            B0 = GetBlue(lastPixel);
                            if ((maxErrorForFourPixels >= 3) && (NewDeltaE(lastR, R0, lastG, G0, lastB, B0, maxErrorForFourPixels)))
                            {
                                /*   - - - -
                                    |       | build lower 4x1
                                     - - - -
                                */
#if kGatherStats == 1
                                blockStats[es41si]++;
#endif
                                didNotBuild4by1 = false;
                                AverageNRound((pixelNum & 0x04)== 0x04, lastR, lastR, R0, lastG, lastG, G0, lastB, lastB, B0); // 4,5,6,7,12,13,14,15,20...

                                currPixel = (lastR<<16) + (lastG<<8) + lastB;

#if kMemWritesOptimize == 0
                                put4Pixel(currPtr, -3, currPixel);
                                put4Pixel(currPtr, -2, currPixel);
                                put4Pixel(currPtr, -1, currPixel);
                                put4Pixel(currPtr, 0, currPixel);
#else
                                put4Pixel(currPtr, -3, currPixel);
#endif                              
                                flagsPtr[-3] = (flagsPtr[-3] & ~eSouths) | e41si;
                                flagsPtr[-2] = (flagsPtr[-2] & ~eSouths) | e41s;
                                flagsPtr[-1] = (flagsPtr[-1] & ~eSouths) | e41s;
                                flagsPtr[0] = (flagsPtr[0] & ~eSouths) | e41s;
                            }
                        }

                        if (didNotBuild4by1) // Not a lower 4x1 so output lower 2x1.
                        {
                            assert(!((flagsPtr[-1] | flagsPtr[0]) & eTheRest)); // no vertical blocks

                            currPixel = (lastR<<16) + (lastG<<8) + lastB;

#if kMemWritesOptimize == 0
                            put4Pixel(currPtr, -1, currPixel);
                            put4Pixel(currPtr, 0, currPixel);
#else
                            put4Pixel(currPtr, -1, currPixel);
#endif
                            
                            flagsPtr[-1] = (flagsPtr[-1] & ~eSouths) | e21sw;
                            flagsPtr[0] = (flagsPtr[0] & ~eSouths) | e21se;
                        }
                    }  // If DeltaE... Looking for two by one
                }  // IsOdd(pixelNum)
            }
        } 
        else // no flag bits set.
        {
            lastPairAveraged = false; // EGW Fixes bug on business graphics. 11/24/97
        }
        
        upPtr += eBufferedPixelWidthInBytes;
        currPtr += eBufferedPixelWidthInBytes;
        flagsPtr++;
    }  // for each pixel...
}

// Filter2PairsOfFilteredRows.  This routine takes 2 pairs of rows that
// have been through the Filter2RawRows routine and puts blocks together
// to make bigger blocks.  It prefers taking 2 high blocks and putting
// them together to make four high blocks, but as a last resort it will
// take try to take a 1 high blocks from the second and third rasters and
// create 2 high blocks.  The possible block sizes this routine could
// create are 8x4, 4x4, 2x4, and 1x4, and then with the second and third rasters
// 4x2, 2x2, and 1x2. 
void TErnieFilter::Filter2PairsOfFilteredRows(unsigned char *row1Ptr, unsigned char *row2Ptr, unsigned char *row3Ptr, unsigned char *row4Ptr)
{
    const unsigned int maxErrorForFourPixels = fMaxErrorForTwoPixels / 2;
    const unsigned int maxErrorForEightPixels = maxErrorForFourPixels / 2;
    const unsigned int maxErrorForSixteenPixels = maxErrorForEightPixels / 2;
    const unsigned int maxErrorForThirtyTwoPixels = maxErrorForSixteenPixels / 2;

    for (int pixelNum = 0; pixelNum < (fRowWidthInPixels-3);)  // Make sure we have four pixels to work with
    {
        int currPixel, upPixel;
        int R0, G0, B0, R1, G1, B1;

        if ((fPixelFilteredFlags[0][pixelNum] & e42i) && (fPixelFilteredFlags[1][pixelNum] & e42i))
        {
            /*  - - - -
               |       |
               |       |
                - - - -     We have two 4x2s.
                - - - - 
               |       |
               |       |
                - - - -
            */
            assert(fPixelFilteredFlags[0][pixelNum] == e42i && fPixelFilteredFlags[0][pixelNum+1] == e42 && fPixelFilteredFlags[0][pixelNum+2] == e42 && fPixelFilteredFlags[0][pixelNum+3] == e42);
            assert(fPixelFilteredFlags[1][pixelNum] == e42i && fPixelFilteredFlags[1][pixelNum+1] == e42 && fPixelFilteredFlags[1][pixelNum+2] == e42 && fPixelFilteredFlags[1][pixelNum+3] == e42);

            upPixel = get4Pixel(row1Ptr);
            currPixel = get4Pixel(row3Ptr);

            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
    
            R0 = GetRed(upPixel);
            G0 = GetGreen(upPixel);
            B0 = GetBlue(upPixel);
    
            if((maxErrorForSixteenPixels >= 3) &&(NewDeltaE(R1, R0, G1, G0, B1, B0, maxErrorForSixteenPixels)))
            {
                /*   - - - -
                    |       |
                    |       | build 4x4
                    |       |
                    |       |
                     - - - -
                */
#if kGatherStats == 1
                blockStats[es44ni]++;
#endif
                AverageNRound((pixelNum & 0x04) == 0x04, R1, R1, R0, G1, G1, G0, B1, B1, B0); // 4,5,6,7,12,13,14,15,20... Alternate between rounding up down

                currPixel = (R1<<16) + (G1<<8) + B1;

#if kMemWritesOptimize == 0
                put4Pixel(row1Ptr, 0, currPixel); 
                put4Pixel(row1Ptr, 1, currPixel);
                put4Pixel(row1Ptr, 2, currPixel);
                put4Pixel(row1Ptr, 3, currPixel);
                put4Pixel(row2Ptr, 0, currPixel);
                put4Pixel(row2Ptr, 1, currPixel);
                put4Pixel(row2Ptr, 2, currPixel);
                put4Pixel(row2Ptr, 3, currPixel);
                put4Pixel(row3Ptr, 0, currPixel);
                put4Pixel(row3Ptr, 1, currPixel);
                put4Pixel(row3Ptr, 2, currPixel);
                put4Pixel(row3Ptr, 3, currPixel);
                put4Pixel(row4Ptr, 0, currPixel);
                put4Pixel(row4Ptr, 1, currPixel);
                put4Pixel(row4Ptr, 2, currPixel);
                put4Pixel(row4Ptr, 3, currPixel);
#else
                put4Pixel(row1Ptr, 0, currPixel);
#endif
                row1Ptr += 4*eBufferedPixelWidthInBytes;
                row2Ptr += 4*eBufferedPixelWidthInBytes;
                row3Ptr += 4*eBufferedPixelWidthInBytes;
                row4Ptr += 4*eBufferedPixelWidthInBytes;

                fPixelFilteredFlags[0][pixelNum] = e44ni;
                fPixelFilteredFlags[0][pixelNum+1] = fPixelFilteredFlags[0][pixelNum+2] = fPixelFilteredFlags[0][pixelNum+3] = e44n;
                fPixelFilteredFlags[1][pixelNum] = e44si;
                fPixelFilteredFlags[1][pixelNum+1] = fPixelFilteredFlags[1][pixelNum+2] = fPixelFilteredFlags[1][pixelNum+3] = e44s;
                
                if ((pixelNum >= 4) && (fPixelFilteredFlags[1][pixelNum-4] & e44si)) // 4,5,6,7,12,13,14,15,20...
                {
                    /*   - - - -     - - - -
                        |       |   |       |
                        |       |   |       | We have two 4x4s.
                        |       |   |       |
                        |       |   |       |
                         - - - -     - - - -
                    */
                    assert(fPixelFilteredFlags[0][pixelNum-4] == e44ni && fPixelFilteredFlags[0][pixelNum-3] == e44n && fPixelFilteredFlags[0][pixelNum-2] == e44n && fPixelFilteredFlags[0][pixelNum-1] == e44n);
                    assert(fPixelFilteredFlags[1][pixelNum-4] == e44si && fPixelFilteredFlags[1][pixelNum-3] == e44s && fPixelFilteredFlags[1][pixelNum-2] == e44s && fPixelFilteredFlags[1][pixelNum-1] == e44s);

                    upPixel = get4Pixel(row1Ptr, -8);
            
                    R0 = GetRed(upPixel);
                    G0 = GetGreen(upPixel);
                    B0 = GetBlue(upPixel);
    
                    if( (maxErrorForThirtyTwoPixels >= 3) && (NewDeltaE(R1, R0, G1, G0, B1, B0, maxErrorForThirtyTwoPixels)))
                    {
                        /*   - - - - - - - -
                            |               |
                            |               | build 8x4
                            |               |
                            |               |
                             - - - - - - - -
                        */
#if kGatherStats == 1
                        blockStats[es84ni]++;
#endif
                        AverageNRound((pixelNum & 0x08) == 0x08, R1, R1, R0, G1, G1, G0, B1, B1, B0);
                        currPixel = (R1<<16) + (G1<<8) + B1;

#if kMemWritesOptimize == 0
                        put4Pixel(row1Ptr, -8, currPixel);
                        put4Pixel(row1Ptr, -7, currPixel);
                        put4Pixel(row1Ptr, -6, currPixel);
                        put4Pixel(row1Ptr, -5, currPixel); 
                        put4Pixel(row1Ptr, -4, currPixel); 
                        put4Pixel(row1Ptr, -3, currPixel); 
                        put4Pixel(row1Ptr, -2, currPixel); 
                        put4Pixel(row1Ptr, -1, currPixel); 
                        put4Pixel(row2Ptr, -8, currPixel); 
                        put4Pixel(row2Ptr, -7, currPixel); 
                        put4Pixel(row2Ptr, -6, currPixel); 
                        put4Pixel(row2Ptr, -5, currPixel); 
                        put4Pixel(row2Ptr, -4, currPixel); 
                        put4Pixel(row2Ptr, -3, currPixel); 
                        put4Pixel(row2Ptr, -2, currPixel); 
                        put4Pixel(row2Ptr, -1, currPixel); 
                        put4Pixel(row3Ptr, -8, currPixel); 
                        put4Pixel(row3Ptr, -7, currPixel); 
                        put4Pixel(row3Ptr, -6, currPixel); 
                        put4Pixel(row3Ptr, -5, currPixel); 
                        put4Pixel(row3Ptr, -4, currPixel); 
                        put4Pixel(row3Ptr, -3, currPixel); 
                        put4Pixel(row3Ptr, -2, currPixel); 
                        put4Pixel(row3Ptr, -1, currPixel); 
                        put4Pixel(row4Ptr, -8, currPixel); 
                        put4Pixel(row4Ptr, -7, currPixel); 
                        put4Pixel(row4Ptr, -6, currPixel); 
                        put4Pixel(row4Ptr, -5, currPixel); 
                        put4Pixel(row4Ptr, -4, currPixel); 
                        put4Pixel(row4Ptr, -3, currPixel); 
                        put4Pixel(row4Ptr, -2, currPixel); 
                        put4Pixel(row4Ptr, -1, currPixel); 
#else
                        put4Pixel(row1Ptr, -8, currPixel); 
#endif
                        fPixelFilteredFlags[0][pixelNum-4] = e84ni;
                        fPixelFilteredFlags[0][pixelNum-3] = fPixelFilteredFlags[0][pixelNum-2] = fPixelFilteredFlags[0][pixelNum-1] = fPixelFilteredFlags[0][pixelNum] = fPixelFilteredFlags[0][pixelNum+1] = fPixelFilteredFlags[0][pixelNum+2] = fPixelFilteredFlags[0][pixelNum+3] = e84n;
                        fPixelFilteredFlags[1][pixelNum-4] = e84si;
                        fPixelFilteredFlags[1][pixelNum-3] = fPixelFilteredFlags[1][pixelNum-2] = fPixelFilteredFlags[1][pixelNum-1] = fPixelFilteredFlags[1][pixelNum] = fPixelFilteredFlags[1][pixelNum+1] = fPixelFilteredFlags[1][pixelNum+2] = fPixelFilteredFlags[1][pixelNum+3] = e84s;
                    }
                }
            }
            else // could not build 4x4 so move forward past the stacked 4x2s.
            {
                row1Ptr += 4*eBufferedPixelWidthInBytes;
                row2Ptr += 4*eBufferedPixelWidthInBytes;
                row3Ptr += 4*eBufferedPixelWidthInBytes;
                row4Ptr += 4*eBufferedPixelWidthInBytes;
            }           
            pixelNum += 4;
        }
        else if ((fPixelFilteredFlags[0][pixelNum] & e22w) && (fPixelFilteredFlags[1][pixelNum] & e22w))
        {
            /*   - -
                |   |
                |   |
                 - -   we have 2 2x2s.
                 - -
                |   |
                |   |
                 - -
            */
            assert(fPixelFilteredFlags[0][pixelNum] == e22w && fPixelFilteredFlags[0][pixelNum+1] == e22e);
            assert(fPixelFilteredFlags[1][pixelNum] == e22w && fPixelFilteredFlags[1][pixelNum+1] == e22e);

            upPixel = get4Pixel(row1Ptr);
            currPixel = get4Pixel(row3Ptr);

            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
    
            R0 = GetRed(upPixel);
            G0 = GetGreen(upPixel);
            B0 = GetBlue(upPixel);
    
            if ((maxErrorForEightPixels >= 3) && (NewDeltaE(R1, R0, G1, G0, B1, B0, maxErrorForEightPixels)))
            {
                /*   - -
                    |   |
                    |   | build 2x4
                    |   |
                    |   |
                     - -
                */
#if kGatherStats == 1
                blockStats[es24nw]++;
#endif
                AverageNRound((pixelNum & 0x02) == 0x02, R1, R1, R0, G1, G1, G0, B1, B1, B0);

                currPixel = (R1<<16) + (G1<<8) + B1;

#if kMemWritesOptimize == 0
                put4Pixel(row1Ptr, 0, currPixel); 
                put4Pixel(row1Ptr, 1, currPixel); 
                put4Pixel(row2Ptr, 0, currPixel);
                put4Pixel(row2Ptr, 1, currPixel); 
                put4Pixel(row3Ptr, 0, currPixel); 
                put4Pixel(row3Ptr, 1, currPixel);
                put4Pixel(row4Ptr, 0, currPixel); 
                put4Pixel(row4Ptr, 1, currPixel);
#else
                put4Pixel(row1Ptr, 0, currPixel);
#endif              
                row1Ptr += 2*eBufferedPixelWidthInBytes;
                row2Ptr += 2*eBufferedPixelWidthInBytes;
                row3Ptr += 2*eBufferedPixelWidthInBytes;
                row4Ptr += 2*eBufferedPixelWidthInBytes;
                
                fPixelFilteredFlags[0][pixelNum] = e24nw;
                fPixelFilteredFlags[0][pixelNum+1] = e24ne;
                fPixelFilteredFlags[1][pixelNum] = e24sw;
                fPixelFilteredFlags[1][pixelNum+1] = e24se;
            } 
            else
            {
                row1Ptr += 2*eBufferedPixelWidthInBytes;
                row2Ptr += 2*eBufferedPixelWidthInBytes;
                row3Ptr += 2*eBufferedPixelWidthInBytes;
                row4Ptr += 2*eBufferedPixelWidthInBytes;
            }           
            pixelNum += 2;
        }
        else if ((fPixelFilteredFlags[0][pixelNum] & e12) && (fPixelFilteredFlags[1][pixelNum] & e12))
        {
            /*   -
                | |
                | | 
                 -  we have two 1x2s.
                 -
                | | 
                | | 
                 -              
            */
            assert(fPixelFilteredFlags[0][pixelNum] == e12);
            assert(fPixelFilteredFlags[1][pixelNum] == e12);
            
            upPixel = get4Pixel(row1Ptr);
            currPixel = get4Pixel(row3Ptr);

            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
    
            R0 = GetRed(upPixel);
            G0 = GetGreen(upPixel);
            B0 = GetBlue(upPixel);
    
            if ((maxErrorForFourPixels >= 3) && (NewDeltaE(R1, R0, G1, G0, B1, B0, maxErrorForFourPixels)))
            {
                /*   -
                    | |
                    | | build 1x4
                    | | 
                    | | 
                     -              
                */
#if kGatherStats == 1
                blockStats[es14n]++;
#endif
                AverageNRound((pixelNum & 0x01) == 0x01, R1, R1, R0, G1, G1, G0, B1, B1, B0);

                currPixel = (R1<<16) + (G1<<8) + B1;

#if kMemWritesOptimize == 0
                put4Pixel(row1Ptr, 0, currPixel);
                put4Pixel(row2Ptr, 0, currPixel);
                put4Pixel(row3Ptr, 0, currPixel);
                put4Pixel(row4Ptr, 0, currPixel);
#else
                put4Pixel(row1Ptr, 0, currPixel);
#endif              
                fPixelFilteredFlags[0][pixelNum] = e14n;
                fPixelFilteredFlags[1][pixelNum] = e14s;
            }
        
            row1Ptr += eBufferedPixelWidthInBytes;
            row2Ptr += eBufferedPixelWidthInBytes;
            row3Ptr += eBufferedPixelWidthInBytes;
            row4Ptr += eBufferedPixelWidthInBytes;
            
            pixelNum++;
        }
        else if ((fPixelFilteredFlags[0][pixelNum] & e41si) 
            && (fPixelFilteredFlags[1][pixelNum] & e41ni))
        {
            /*    - - - -
                 |       |
                  - - - -   We have two 4x1s.
                  - - - -
                 |       |
                  - - - -
            */

            upPixel = get4Pixel(row2Ptr);
            currPixel = get4Pixel(row3Ptr);

            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
    
            R0 = GetRed(upPixel);
            G0 = GetGreen(upPixel);
            B0 = GetBlue(upPixel);


            if ((maxErrorForEightPixels >= 3) && (NewDeltaE(R1, R0, G1, G0, B1, B0, maxErrorForEightPixels)))
            {

                /*    - - - -
                     |       |  build 4x2.
                     |       |
                      - - - -
                */
#if kGatherStats == 1
                blockStats[es42w]++;
#endif
                AverageNRound((pixelNum & 0x04) == 0x04, R1, R1, R0, G1, G1, G0, B1, B1, B0);

                currPixel = (R1<<16) + (G1<<8) + B1;

                // Note we write this block out now and do not delay the writes for the postprocessing step since we do not track this block.
                put4Pixel(row2Ptr, 0, currPixel);
                put4Pixel(row2Ptr, 1, currPixel);
                put4Pixel(row2Ptr, 2, currPixel);
                put4Pixel(row2Ptr, 3, currPixel);
                put4Pixel(row3Ptr, 0, currPixel); 
                put4Pixel(row3Ptr, 1, currPixel);
                put4Pixel(row3Ptr, 2, currPixel);
                put4Pixel(row3Ptr, 3, currPixel);

                fPixelFilteredFlags[0][pixelNum] = fPixelFilteredFlags[0][pixelNum] & ~e41si;
                fPixelFilteredFlags[0][pixelNum+1] = fPixelFilteredFlags[0][pixelNum+1] & ~e41s;
                fPixelFilteredFlags[0][pixelNum+2] = fPixelFilteredFlags[0][pixelNum+1] & ~e41s;
                fPixelFilteredFlags[0][pixelNum+3] = fPixelFilteredFlags[0][pixelNum+1] & ~e41s;

                fPixelFilteredFlags[1][pixelNum] = fPixelFilteredFlags[1][pixelNum] & ~e41ni;  // Note that we just formed a 2x2 in the middle of the filtered sets of rows (and do not remember it). We then remove the 2x1s that the 2x2 eliminated.
                fPixelFilteredFlags[1][pixelNum+1] = fPixelFilteredFlags[1][pixelNum+1] & ~e41n;
                fPixelFilteredFlags[1][pixelNum+2] = fPixelFilteredFlags[1][pixelNum+1] & ~e41n;
                fPixelFilteredFlags[1][pixelNum+3] = fPixelFilteredFlags[1][pixelNum+1] & ~e41n;
            }
            pixelNum += 4;
            
            row1Ptr += 4*eBufferedPixelWidthInBytes;
            row2Ptr += 4*eBufferedPixelWidthInBytes;
            row3Ptr += 4*eBufferedPixelWidthInBytes;
            row4Ptr += 4*eBufferedPixelWidthInBytes;
        }
        else if ((fPixelFilteredFlags[0][pixelNum] & e21sw) 
                && (fPixelFilteredFlags[1][pixelNum] & e21nw))
        {
            /*    - -
                 |   |
                  - -  We have two 2x1s.
                  - -
                 |   |
                  - -
            */
            assert(!((fPixelFilteredFlags[0][pixelNum] & e11s) | (fPixelFilteredFlags[0][pixelNum+1] & e11s)));
            assert(!((fPixelFilteredFlags[1][pixelNum] & e11n) | (fPixelFilteredFlags[1][pixelNum+1] & e11n)));

            upPixel = get4Pixel(row2Ptr);
            currPixel = get4Pixel(row3Ptr);

            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
    
            R0 = GetRed(upPixel);
            G0 = GetGreen(upPixel);
            B0 = GetBlue(upPixel);
    
            if ((maxErrorForFourPixels >= 3) && (NewDeltaE(R1, R0, G1, G0, B1, B0, maxErrorForFourPixels)))
            {
                /*    - -
                     |   |  build 2x2.
                     |   |
                      - -
                */
#if kGatherStats == 1
                blockStats[es22w]++;
#endif
                AverageNRound((pixelNum & 0x02) == 0x02, R1, R1, R0, G1, G1, G0, B1, B1, B0);

                currPixel = (R1<<16) + (G1<<8) + B1;

                // Note we write this block out now and do not delay the writes for the postprocessing step since we do not track this block.
                put4Pixel(row2Ptr, 0, currPixel);
                put4Pixel(row2Ptr, 1, currPixel);
                put4Pixel(row3Ptr, 0, currPixel); 
                put4Pixel(row3Ptr, 1, currPixel);

                fPixelFilteredFlags[0][pixelNum] = fPixelFilteredFlags[0][pixelNum] & ~e21sw;
                fPixelFilteredFlags[0][pixelNum+1] = fPixelFilteredFlags[0][pixelNum+1] & ~e21se;

                fPixelFilteredFlags[1][pixelNum] = fPixelFilteredFlags[1][pixelNum] & ~e21nw;  // Note that we just formed a 2x2 in the middle of the filtered sets of rows (and do not remember it). We then remove the 2x1s that the 2x2 eliminated.
                fPixelFilteredFlags[1][pixelNum+1] = fPixelFilteredFlags[1][pixelNum+1] & ~e21ne;
            }

            pixelNum += 2;

            row1Ptr += 2*eBufferedPixelWidthInBytes;
            row2Ptr += 2*eBufferedPixelWidthInBytes;
            row3Ptr += 2*eBufferedPixelWidthInBytes;
            row4Ptr += 2*eBufferedPixelWidthInBytes;
        }
        else if ((fPixelFilteredFlags[0][pixelNum] & e11s) 
                && (fPixelFilteredFlags[1][pixelNum] & e11n))
        {
            /*    - 
                 | |
                  -   We have two 1x1s.
                  -
                 | |
                  - 
            */

            upPixel = get4Pixel(row2Ptr);
            currPixel = get4Pixel(row3Ptr);

            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
    
            R0 = GetRed(upPixel);
            G0 = GetGreen(upPixel);
            B0 = GetBlue(upPixel);
    
            if ((fMaxErrorForTwoPixels >= 3) && (NewDeltaE(R1, R0, G1, G0, B1, B0, fMaxErrorForTwoPixels)))
            {
                /*    -
                     | |  build 1x2.
                     | |
                      -
                */
#if kGatherStats == 1
                blockStats[es12w]++;
#endif
                AverageNRound(isOdd(pixelNum), R1, R1, R0, G1, G1, G0, B1, B1, B0);

                currPixel = (R1<<16) + (G1<<8) + B1;

                // Note we write this block out now and do not delay the writes for the postprocessing step since we do not track this block.
                put4Pixel(row2Ptr, 0, currPixel);
                put4Pixel(row3Ptr, 0, currPixel); 

                fPixelFilteredFlags[0][pixelNum] = fPixelFilteredFlags[0][pixelNum] & ~e11s;

                fPixelFilteredFlags[1][pixelNum] = fPixelFilteredFlags[1][pixelNum] & ~e11n;  // Note that we just formed a 2x2 in the middle of the filtered sets of rows (and do not remember it). We then remove the 2x1s that the 2x2 eliminated.
            }

            pixelNum += 1;

            row1Ptr += eBufferedPixelWidthInBytes;
            row2Ptr += eBufferedPixelWidthInBytes;
            row3Ptr += eBufferedPixelWidthInBytes;
            row4Ptr += eBufferedPixelWidthInBytes;
        }
        else // Do no vertical filtering here.
        {
            pixelNum += 1;

            row1Ptr += eBufferedPixelWidthInBytes;
            row2Ptr += eBufferedPixelWidthInBytes;
            row3Ptr += eBufferedPixelWidthInBytes;
            row4Ptr += eBufferedPixelWidthInBytes;
        }   
    }
}

// Filter3FilteredRows.  This routine only exists for the case of the odd size band
// with three rasters left over.  I'm not sure how much extra benifit we really
// get from running this, but for now I'm leaving it in.  Since Ernie deals with
// block sizes that are powers of two its rather difficult to filter 3 rows together,
// about all I've been able to do is look for 1 high blocks in the second and third
// rasters to put together into 2 high blocks.  This routine will create 4x2, 2x2, and
// 1x2 blocks from those second and third rasters.
void TErnieFilter::Filter3FilteredRows(unsigned char *row1Ptr, unsigned char *row2Ptr, unsigned char *row3Ptr)
{
    const unsigned int maxErrorForFourPixels = fMaxErrorForTwoPixels / 2;
    const unsigned int maxErrorForEightPixels = maxErrorForFourPixels / 2;
//    const unsigned int maxErrorForSixteenPixels = maxErrorForEightPixels / 2;
//    const unsigned int maxErrorForThirtyTwoPixels = maxErrorForSixteenPixels / 2;

    for (int pixelNum = 0; pixelNum < (fRowWidthInPixels-3);)  // Make sure we have four pixels to work with
    {
        int currPixel, upPixel;
        int R0, G0, B0, R1, G1, B1;

        if ((fPixelFilteredFlags[0][pixelNum] & e41si) 
                && (fPixelFilteredFlags[1][pixelNum] & e41ni))
        {
            /*    - - - -
                 |       |
                  - - - -   We have two 4x1s.
                  - - - -
                 |       |
                  - - - -
            */
            assert(!((fPixelFilteredFlags[0][pixelNum] & e11s) | (fPixelFilteredFlags[0][pixelNum+1] & e11s)));
            assert(!((fPixelFilteredFlags[1][pixelNum] & e11n) | (fPixelFilteredFlags[1][pixelNum+1] & e11n)));

            upPixel = get4Pixel(row2Ptr);
            currPixel = get4Pixel(row3Ptr);

            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
    
            R0 = GetRed(upPixel);
            G0 = GetGreen(upPixel);
            B0 = GetBlue(upPixel);


            if ((maxErrorForEightPixels >= 3) && (NewDeltaE(R1, R0, G1, G0, B1, B0, maxErrorForEightPixels)))
            {

                /*    - - - -
                     |       |  build 4x2.
                     |       |
                      - - - -
                */
#if kGatherStats == 1
                blockStats[es42w]++;
#endif
                AverageNRound((pixelNum & 0x04) == 0x04, R1, R1, R0, G1, G1, G0, B1, B1, B0);

                currPixel = (R1<<16) + (G1<<8) + B1;

                // Note we write this block out now and do not delay the writes for the postprocessing step since we do not track this block.
                put4Pixel(row2Ptr, 0, currPixel);
                put4Pixel(row2Ptr, 1, currPixel);
                put4Pixel(row2Ptr, 2, currPixel);
                put4Pixel(row2Ptr, 3, currPixel);
                put4Pixel(row3Ptr, 0, currPixel); 
                put4Pixel(row3Ptr, 1, currPixel);
                put4Pixel(row3Ptr, 2, currPixel);
                put4Pixel(row3Ptr, 3, currPixel);

                fPixelFilteredFlags[0][pixelNum] = fPixelFilteredFlags[0][pixelNum] & ~e41si;
                fPixelFilteredFlags[0][pixelNum+1] = fPixelFilteredFlags[0][pixelNum+1] & ~e41s;
                fPixelFilteredFlags[0][pixelNum+2] = fPixelFilteredFlags[0][pixelNum+1] & ~e41s;
                fPixelFilteredFlags[0][pixelNum+3] = fPixelFilteredFlags[0][pixelNum+1] & ~e41s;

                fPixelFilteredFlags[1][pixelNum] = fPixelFilteredFlags[1][pixelNum] & ~e41ni;  // Note that we just formed a 2x2 in the middle of the filtered sets of rows (and do not remember it). We then remove the 2x1s that the 2x2 eliminated.
                fPixelFilteredFlags[1][pixelNum+1] = fPixelFilteredFlags[1][pixelNum+1] & ~e41n;
                fPixelFilteredFlags[1][pixelNum+2] = fPixelFilteredFlags[1][pixelNum+1] & ~e41n;
                fPixelFilteredFlags[1][pixelNum+3] = fPixelFilteredFlags[1][pixelNum+1] & ~e41n;
            }
            pixelNum += 4;
            
            row1Ptr += 4*eBufferedPixelWidthInBytes;
            row2Ptr += 4*eBufferedPixelWidthInBytes;
            row3Ptr += 4*eBufferedPixelWidthInBytes;
        }
        else if ((fPixelFilteredFlags[0][pixelNum] & e21sw) 
                && (fPixelFilteredFlags[1][pixelNum] & e21nw))
        {
            /*    - -
                 |   |
                  - -  We have two 2x1s.
                  - -
                 |   |
                  - -
            */
            assert(!((fPixelFilteredFlags[0][pixelNum] & e11s) | (fPixelFilteredFlags[0][pixelNum+1] & e11s)));
            assert(!((fPixelFilteredFlags[1][pixelNum] & e11n) | (fPixelFilteredFlags[1][pixelNum+1] & e11n)));

            upPixel = get4Pixel(row2Ptr);
            currPixel = get4Pixel(row3Ptr);

            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
    
            R0 = GetRed(upPixel);
            G0 = GetGreen(upPixel);
            B0 = GetBlue(upPixel);
    
            if ((maxErrorForFourPixels >= 3) && (NewDeltaE(R1, R0, G1, G0, B1, B0, maxErrorForFourPixels)))
            {
                /*    - -
                     |   |  build 2x2.
                     |   |
                      - -
                */
#if kGatherStats == 1
                blockStats[es22w]++;
#endif
                AverageNRound((pixelNum & 0x02) == 0x02, R1, R1, R0, G1, G1, G0, B1, B1, B0);

                currPixel = (R1<<16) + (G1<<8) + B1;

                // Note we write this block out now and do not delay the writes for the postprocessing step since we do not track this block.
                put4Pixel(row2Ptr, 0, currPixel);
                put4Pixel(row2Ptr, 1, currPixel);
                put4Pixel(row3Ptr, 0, currPixel); 
                put4Pixel(row3Ptr, 1, currPixel);

                fPixelFilteredFlags[0][pixelNum] = fPixelFilteredFlags[0][pixelNum] & ~e21sw;
                fPixelFilteredFlags[0][pixelNum+1] = fPixelFilteredFlags[0][pixelNum+1] & ~e21se;

                fPixelFilteredFlags[1][pixelNum] = fPixelFilteredFlags[1][pixelNum] & ~e21nw;  // Note that we just formed a 2x2 in the middle of the filtered sets of rows (and do not remember it). We then remove the 2x1s that the 2x2 eliminated.
                fPixelFilteredFlags[1][pixelNum+1] = fPixelFilteredFlags[1][pixelNum+1] & ~e21ne;
            }

            pixelNum += 2;

            row1Ptr += 2*eBufferedPixelWidthInBytes;
            row2Ptr += 2*eBufferedPixelWidthInBytes;
            row3Ptr += 2*eBufferedPixelWidthInBytes;
        }
        else if ((fPixelFilteredFlags[0][pixelNum] & e11s) 
                && (fPixelFilteredFlags[1][pixelNum] & e11n))
        {
            /*    - 
                 | |
                  -   We have two 1x1s.
                  -
                 | |
                  - 
            */

            upPixel = get4Pixel(row2Ptr);
            currPixel = get4Pixel(row3Ptr);

            R1 = GetRed(currPixel);
            G1 = GetGreen(currPixel);
            B1 = GetBlue(currPixel);
    
            R0 = GetRed(upPixel);
            G0 = GetGreen(upPixel);
            B0 = GetBlue(upPixel);
    
            if ((fMaxErrorForTwoPixels >= 3) && (NewDeltaE(R1, R0, G1, G0, B1, B0, fMaxErrorForTwoPixels)))
            {
                /*    -
                     | |  build 1x2.
                     | |
                      -
                */
#if kGatherStats == 1
                blockStats[es12w]++;
#endif
                AverageNRound(isOdd(pixelNum), R1, R1, R0, G1, G1, G0, B1, B1, B0);

                currPixel = (R1<<16) + (G1<<8) + B1;

                // Note we write this block out now and do not delay the writes for the postprocessing step since we do not track this block.
                put4Pixel(row2Ptr, 0, currPixel);
                put4Pixel(row3Ptr, 0, currPixel); 

                fPixelFilteredFlags[0][pixelNum] = fPixelFilteredFlags[0][pixelNum] & ~e11s;

                fPixelFilteredFlags[1][pixelNum] = fPixelFilteredFlags[1][pixelNum] & ~e11n;  // Note that we just formed a 2x2 in the middle of the filtered sets of rows (and do not remember it). We then remove the 2x1s that the 2x2 eliminated.
            }

            pixelNum += 1;

            row1Ptr += eBufferedPixelWidthInBytes;
            row2Ptr += eBufferedPixelWidthInBytes;
            row3Ptr += eBufferedPixelWidthInBytes;
        }
        else // Do no vertical filtering here.
        {
            pixelNum += 1;

            row1Ptr += eBufferedPixelWidthInBytes;
            row2Ptr += eBufferedPixelWidthInBytes;
            row3Ptr += eBufferedPixelWidthInBytes;
        }   
    }
}

#define NEWTEST true



#if 0
inline double CM_FastDelta(vector test, vector target) {
  double dl, da, db, result;

  dl = test[0] - target[0];
  da = test[1] - target[1];
  db = test[2] - target[2];

  double sl = 1.0;
  double sa = 5.0;
  double sb = 5.0;

  result = pow(pow(dl / sl, 2.0) + pow(da / sa, 2.0) + pow(db / sb, 2.0), 0.5);
  return result;

}
#endif


inline bool TErnieFilter::NewDeltaE(int dr0, int dr1, int dg0, int dg1, int db0, int db1, int tolerance)
{
    int Y0, Y1, dY, Cr0, Cr1, Cb0, Cb1, dCr, dCb;

    // new Delta E stuff from Jay

    Y0 = 5*dr0 + 9*dg0 + 2*db0;
    Y1 = 5*dr1 + 9*dg1 + 2*db1;

    dY = ABS(Y0 - Y1) >> 4;

    if(dY > tolerance) {
        return false;
    }
    else
    {
        Cr0 = (dr0 << 4) - Y0;
        Cr1 = (dr1 << 4) - Y1;
        dCr = ABS(Cr0 - Cr1) >> 5;
        if(dCr > tolerance)
        {
            return false;
        }
        else
        {
            Cb0 = (db0 << 4) - Y0;
            Cb1 = (db1 << 4) - Y1;
            dCb = ABS(Cb0 - Cb1) >> 6;
            if(dCb > tolerance)
            {
                return false;
            }
        }
    }
    return true;
}

#if 0
inline unsigned int TErnieFilter::DeltaE(int dr0, int dr1, int dg0, int dg1, int db0, int db1)
{
    // code changed implemented to take care of gradient transition
    // less aggressive filtering when a color is ramping up

    if (dr0 >= 248 && dr0 <= 254) return 0xFFFF;
        if (dr1 >= 248 && dr1 <= 254) return 0xFFFF;
        if (dg0 >= 248 && dg0 <= 254) return 0xFFFF;
        if (dg1 >= 248 && dg1 <= 254) return 0xFFFF;
        //if (db0 >= 248 && db0 <= 254) return 0xFFFF;
        //if (db1 >= 248 && db1 <= 254) return 0xFFFF;

        dr0 = ABS(dr1-dr0);
        dg0 = ABS(dg1-dg0);
        db0 = ABS(db1-db0);

        return (3*dr0 + dg0*4 + 2*db0)/8;
}

#endif


TErnieFilter::TErnieFilter(int rowWidthInPixels, pixelTypes pixelType, unsigned int maxErrorForTwoPixels, int bytesPerPixel)
: fOriginalPixelSize(bytesPerPixel)
{
    int index;
    assert((fOriginalPixelSize == 3) || (fOriginalPixelSize == 4));
    assert(rowWidthInPixels > 0);
    assert(pixelType == eBGRPixelData);
    
    fInternalBufferPixelSize = 4;
    
    fPixelOffsetIndex = 0;
    fRowWidthInPixels = rowWidthInPixels;
    fRowWidthInBytes = fRowWidthInPixels*fInternalBufferPixelSize;
    fMaxErrorForTwoPixels = maxErrorForTwoPixels;

    for (index = 0; index < 4; index++)
    {
        fRowBuf[index] = new unsigned int[rowWidthInPixels];
        assert(fRowBuf[index]);
        
        //fRowPtr[index] = NULL;
        fRowPtr[index] = new unsigned char[rowWidthInPixels*fOriginalPixelSize];
    }
    
    for (index = 0; index < 2; index++)
    {
        fPixelFilteredFlags[index] = new unsigned int[rowWidthInPixels];
        assert(fPixelFilteredFlags[index]);
    }

    // The least compressible image will be all raw pixels. Maximum compressed size is:
    // full size + a bloat of Cmd byte + 1 VLI byte per 255 pixels rounded up to nearest integer.
    
    int maxCompressionBufSize = fRowWidthInBytes + 1 + ((int)ceil(MAX((rowWidthInPixels-2)/255, 0)));

    fCompressionOutBuf = new unsigned char[maxCompressionBufSize];
    assert(fCompressionOutBuf);
    
    fNumberOfBufferedRows = 0;
    
    fPixelOffset[0] = 0;
    fPixelOffset[1] = 5;
    fPixelOffset[2] = 2;
    fPixelOffset[3] = 7;
    fPixelOffset[4] = 1;
    fPixelOffset[5] = 4;
    fPixelOffset[6] = 6;
    fPixelOffset[7] = 3;
}


TErnieFilter::~TErnieFilter()
{
    // Deallocate memory next.
    int index;
    
    for (index = 0; index < 4; index++)
    {
        delete [] fRowBuf[index];
        delete [] fRowPtr[index];
    }
    
    for (index = 0; index < 2; index++)
    {
        delete [] fPixelFilteredFlags[index];
    }

    delete [] fCompressionOutBuf;
    
//  delete [] fSeedRowBufOrigPtr;
}

void TErnieFilter::writeBufferedRows()
{
    int pixelIndex = 0;

    // We just have one lonely raster left.  Nothing
    // we can do but filter it horizontally.
    if( 1 == fNumberOfBufferedRows)
    {

        int offset2 = fPixelOffset[fPixelOffsetIndex];
        
        Filter1RawRow( (unsigned char*)(fRowBuf[0] + offset2),
                       fRowWidthInPixels - fPixelOffset[fPixelOffsetIndex],
                       fPixelFilteredFlags[0] + fPixelOffset[fPixelOffsetIndex]);

        
        unsigned char *rowPtr = fRowPtr[0];
        assert(rowPtr);
        pixelIndex = 0;
        do
        {
            memcpy(rowPtr, &fRowBuf[0][pixelIndex], 3);
            rowPtr += 3;
        } while (++pixelIndex < fRowWidthInPixels);
        
    }
    // If we've got a pair of rasters in the buffer, that pair
    // has already been filtered somewhat.  So lets just write them
    // out, some filtering is better than none.
    else if (2 == fNumberOfBufferedRows)
    {
        // Write the two rows back out.
        int k;
        for (k = 0; k < 2; k++)
        {
            unsigned char *rowPtr = fRowPtr[k];
            assert(rowPtr);
            pixelIndex = 0;
            do
            {
                memcpy(rowPtr, &fRowBuf[k][pixelIndex], 3);
                rowPtr += 3;
            } while (++pixelIndex < fRowWidthInPixels);
        }
    }
    // Okay, if we had three rasters in the buffer, the pair
    // should have already been written out above, so lets
    // just run the odd raster through Ernie with to
    // get the horizontal filtering.  [Need to look to see
    // if there's something more we can do with filtering
    // all three together.]
    else if (3 == fNumberOfBufferedRows)
    {
        
        int offset2 = fPixelOffset[fPixelOffsetIndex];
        
        Filter1RawRow( (unsigned char*)(fRowBuf[2] + offset2),
                       fRowWidthInPixels - fPixelOffset[fPixelOffsetIndex],
                       fPixelFilteredFlags[1] + fPixelOffset[fPixelOffsetIndex]);
        
        
        Filter3FilteredRows( (unsigned char*)fRowBuf[0],
                             (unsigned char*)fRowBuf[1],
                             (unsigned char*)fRowBuf[2]);

        int k;
        for (k = 0; k < 3; k++)
        {
            unsigned char *rowPtr = fRowPtr[k];
            assert(rowPtr);
            pixelIndex = 0;
            do
            {
                memcpy(rowPtr, &fRowBuf[k][pixelIndex], 3);
                rowPtr += 3;
            } while (++pixelIndex < fRowWidthInPixels);
        }
    }
}

void TErnieFilter::submitRowToFilter(unsigned char *rowPtr)
{
 //   fRowPtr[fNumberOfBufferedRows] = rowPtr; // Save where to put the filtered data back to.
    memcpy(fRowPtr[fNumberOfBufferedRows], rowPtr, fRowWidthInPixels*3);

    // Now reformat the pixel data from 24 bit to 32 bit pixels
    int pixelIndex = 0;
    unsigned int *RowPtrDest = fRowBuf[fNumberOfBufferedRows];
    do
    {
        RowPtrDest[pixelIndex] = *((unsigned int*)(rowPtr));
        rowPtr += fOriginalPixelSize;
    } while (++pixelIndex < (fRowWidthInPixels - 1));

    if(fOriginalPixelSize == 3)
    {
        // Now we have to deal with the last 24 bit pixel on the end
        // because we don't want to read past the end of the buffer.
        unsigned char byte1 = *rowPtr++;  // Little Endian blue, Big Endian red
        unsigned char byte2 = *rowPtr++;  // Green.
        unsigned char byte3 = *rowPtr++;  // Little Endian red, Big Endian blue

#if littleEndian
                              //  Red             Green          Blue
        RowPtrDest[pixelIndex] = (byte3 << 16) | (byte2 << 8) | (byte1);
#elif bigEndian
                              //  Red             Green          Blue
        RowPtrDest[pixelIndex] = (byte1 << 16) | (byte2 << 8) | (byte3);
#endif
    }
    else
    {
        RowPtrDest[pixelIndex] = *((unsigned int*)(rowPtr));
    }

    fNumberOfBufferedRows++; 

    iRastersReady=0;
    iRastersDelivered=0;
    
    // Next see about filtering & compression.
    // NOTE 1: as an optimization only do subsections of the raster at a time to stay in cache.
    // NOTE 2: Could filter the pixels left of the offset.
    if (2 == fNumberOfBufferedRows)
    {
        int offset2 = fPixelOffset[fPixelOffsetIndex];
        
        Filter2RawRows( (unsigned char*)(fRowBuf[1] + offset2),
                        (unsigned char*)(fRowBuf[0] + offset2),
                        fRowWidthInPixels - fPixelOffset[fPixelOffsetIndex],
                        fPixelFilteredFlags[0] + fPixelOffset[fPixelOffsetIndex]);
    }
    
    if (4 == fNumberOfBufferedRows)
    {
        int offset4 = fPixelOffset[fPixelOffsetIndex];
        Filter2RawRows( (unsigned char*)(fRowBuf[3] + offset4),
                        (unsigned char*)(fRowBuf[2] + offset4),
                        fRowWidthInPixels - fPixelOffset[fPixelOffsetIndex],
                        fPixelFilteredFlags[1] + fPixelOffset[fPixelOffsetIndex]);

        Filter2PairsOfFilteredRows( (unsigned char*)fRowBuf[0],
                                    (unsigned char*)fRowBuf[1],
                                    (unsigned char*)fRowBuf[2],
                                    (unsigned char*)fRowBuf[3]);

#if kMemWritesOptimize == 1
        // Writing the blocks out on a post processing step in this manner could leave the last 3 rows
        // unfiltered. This is a trade off we make for simplicity. The resulting loss in compression is small.
        WriteBlockPixels();
#endif

        fPixelOffsetIndex = (fPixelOffsetIndex + 1) % 8; // cycle the offset index.
        
        int k;
        for (k = 0; k < fPixelOffset[fPixelOffsetIndex]; k++) // Clear out the flags that we're offsetting past for this next iteration.
        {
            fPixelFilteredFlags[0][k] = eDone;
            fPixelFilteredFlags[1][k] = eDone;
        }

        // Write the four rows back out.
        for (k = 0; k < 4; k++)
        {
            unsigned char *rowPtr = fRowPtr[k];
            assert(rowPtr);
            pixelIndex = 0;
            do
            {
                memcpy(rowPtr, &fRowBuf[k][pixelIndex], fOriginalPixelSize);
                rowPtr += fOriginalPixelSize;
            } while (++pixelIndex < fRowWidthInPixels);
        }

        fNumberOfBufferedRows = 0;
        iRastersReady = 4;
    }
}


#if kMemWritesOptimize == 1


/*
At this point the color for the entire block is stored in the top left
corner of the block. This routine takes that pixel and smears it into the 
rest of the block.
*/
void TErnieFilter::WriteBlockPixels(void)
{
    unsigned char *row1Ptr = (unsigned char*)fRowBuf[0];
    unsigned char *row2Ptr = (unsigned char*)fRowBuf[1];
    unsigned char *row3Ptr = (unsigned char*)fRowBuf[2];
    unsigned char *row4Ptr = (unsigned char*)fRowBuf[3];

    for (int flagSet = 0; flagSet <= 1; flagSet++)
    {
        unsigned int *flagsPtr = fPixelFilteredFlags[0];
        unsigned char *rowA = (unsigned char*)fRowBuf[0];
        unsigned char *rowB = (unsigned char*)fRowBuf[1];
        
        if (flagSet == 1)
        {
            flagsPtr = fPixelFilteredFlags[1]; 
            rowA = (unsigned char*)fRowBuf[2]; 
            rowB = (unsigned char*)fRowBuf[3];
        }
        
        for (int rowIndex = 0; rowIndex < fRowWidthInPixels;)
        {
            unsigned int currentFlags = flagsPtr[rowIndex];
            
#ifndef NDEBUG /* only done for debug builds */
            int numberOfBitsSet = 0;
            unsigned int currentFlagsCopy = currentFlags & eTopLeftOfBlocks;
            while (currentFlagsCopy)
            {
                if (currentFlagsCopy & 1) numberOfBitsSet++;
                currentFlagsCopy >>= 1;
            }
            assert( (numberOfBitsSet <= 1) ||
                    ((numberOfBitsSet == 2) &&
                    (((currentFlags & eTopLeftOfBlocks) & ~(e21nw|e21sw|e41ni|e41si))==0)));
#endif

            if (currentFlags & eTopLeftOfBlocks) // Avoids doing a lot of checks if nothing is set.
            {
                unsigned int pixel;
                //  The three possible scenerios are:
                //  1: No top left of block bits are set.
                //  2: 1 top left block bit is set.
                //  3: 2 top left block bits are set. They are 21nw and 21sw.
                
                // Note: Due to possibly having two groups tracked by this flag we require the north checks to occur before the south checks.
                if (currentFlags & e22w)
                {
                    pixel = get4Pixel(rowA, rowIndex);
                    
                    put4Pixel(rowB, rowIndex, pixel);
                    rowIndex += 1;
                    put4Pixel(rowA, rowIndex, pixel);
                    put4Pixel(rowB, rowIndex, pixel);

                    rowIndex += 1;
                    continue;
                }
                
                if (currentFlags & e12)
                {
                    put4Pixel(rowB, rowIndex, get4Pixel(rowA, rowIndex));

                    rowIndex += 1;
                    continue;
                }
                
                if (currentFlags & e42i)
                {
                    pixel = get4Pixel(rowA, rowIndex);
                    
                    put4Pixel(rowB, rowIndex, pixel);
                    
                    rowIndex += 1;
                    put4Pixel(rowA, rowIndex, pixel);
                    put4Pixel(rowB, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(rowB, rowIndex, pixel);
                    put4Pixel(rowA, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(rowA, rowIndex, pixel);
                    put4Pixel(rowB, rowIndex, pixel);
                    
                    rowIndex += 1;
                    continue;
                }
                
                if (currentFlags & e84ni)
                {
                    pixel = get4Pixel(rowA, rowIndex);

                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;

                    continue;
                }

                if (currentFlags & e24nw)
                {
                    pixel = get4Pixel(row1Ptr, rowIndex);
                    
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);
                    
                    rowIndex += 1;
                    continue;
                }
                
                if (currentFlags & e44ni)
                {
                    pixel = get4Pixel(row1Ptr, rowIndex);
                    
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    put4Pixel(row1Ptr, rowIndex, pixel);
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    continue;
                }
                
                if (currentFlags & e14n)
                {
                    pixel = get4Pixel(row1Ptr, rowIndex);
                    
                    put4Pixel(row2Ptr, rowIndex, pixel);
                    put4Pixel(row3Ptr, rowIndex, pixel);
                    put4Pixel(row4Ptr, rowIndex, pixel);

                    rowIndex += 1;
                    continue;
                }
                
                if (currentFlags & e21nw)
                {
                    put4Pixel(rowA, rowIndex+1, get4Pixel(rowA, rowIndex));

                    if (!(currentFlags & (e21sw|e41si))) // if no south groups
                    {
                        rowIndex += 2;
                        continue;
                    }
                }
                
                if (currentFlags & e41ni)
                {
                    pixel = get4Pixel(rowA, rowIndex);
                    
                    put4Pixel(rowA, rowIndex+1, pixel);
                    put4Pixel(rowA, rowIndex+2, pixel);
                    put4Pixel(rowA, rowIndex+3, pixel);

                    if (!(currentFlags & (e21sw|e41si))) // if no south groups.
                    {
                        rowIndex += 2;
                        continue;
                    }
                }

                if (currentFlags & e21sw)
                {
                    put4Pixel(rowB, rowIndex+1, get4Pixel(rowB, rowIndex));

                    rowIndex += 2;
                    continue;
                }
                
                if (currentFlags & e41si)
                {
                    pixel = get4Pixel(rowB, rowIndex);
                    
                    put4Pixel(rowB, rowIndex+1, pixel);
                    put4Pixel(rowB, rowIndex+2, pixel);
                    put4Pixel(rowB, rowIndex+3, pixel);

                    rowIndex += 2;
                    continue;
                }
            }
            rowIndex += 1;
        }
    }
}

#endif // kMemWritesOptimize

#endif  // _DJ9xxVIP
