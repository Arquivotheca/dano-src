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

//===========================================================================
//
//  Filename     : HTFED.CPP
//
//  Module       : Open Source Imaging
//
//  Description  : Error Diffusion Halftoning
//
//============================================================================

//============================================================================
// Header file dependencies                                                              
//============================================================================ 
#ifndef HPTYPES_H
#ifdef PROTO
#include "../hptypes.h"
#else
#include "hptypes.h"
#endif
#endif


//#ifndef FED_H
//#include "htfed.h"
//#endif

#ifdef PROTO
#include "../../include/Header.h"
#else
#include "Header.h"
#endif

#include "imaging.h"

//=============================================================================
// C functions
//============================================================================= 

void Imager_Open::HTEDiffOpen  (THTDitherParmsPtr ditherParmsPtr,
                                HPUInt16          count)
{   
   

    ditherParms = ditherParmsPtr+count;        
    errPtr = ditherParms->fErr; 
    numLoop = ditherParms->fNumPix;
    inputPtr = ditherParms->fInput;    
    fedResTbl = ditherParms->fFEDResPtr;
    symmetricFlag = ditherParms->fSymmetricFlag;
    doNext8Pixels = HPTRUE;
    hifipe = ditherParms->fHifipe;    
    outputPtr1 = ditherParms->fOutput1;
    
    outputPtr1 = ditherParms->fOutput1;
    
    if (hifipe)
    {      
        outputPtr1 = ditherParms->fOutput1;
        outputPtr2 = ditherParms->fOutput2;
        outputPtr3 = ditherParms->fOutput3;
    }
    
    diffusionErrorPtr = errPtr;
    
    rasterByte1 = 0;
    rasterByte2 = 0;
    rasterByte3 = 0;  
    
    if( ditherParms->fRasterEvenOrOdd == 1 ) 
    {         
        tmpShortStore = *diffusionErrorPtr;  
        
        *diffusionErrorPtr = 0;
        
        for (pixelCount = numLoop + 8; (pixelCount -= 8) > 0; ) 
        {            
            if (pixelCount > 16) // if next 16 pixels are white, skip 8
            {
                doNext8Pixels = Forward16PixelsNonWhite(inputPtr);
            } 
            else 
            {
                doNext8Pixels = HPTRUE;
            }
            
            if (doNext8Pixels)
            {
                thValue = HPRand();
                FORWARD_FED( thValue, 0x80 );
                thValue = HPRand();
                FORWARD_FED( thValue, 0x40 );
                thValue = HPRand();
                FORWARD_FED( thValue, 0x20 );
                thValue = HPRand();
                FORWARD_FED( thValue, 0x10 );
                thValue = HPRand();
                FORWARD_FED( thValue, 0x08 );
                thValue = HPRand();
                FORWARD_FED( thValue, 0x04 );
                thValue = HPRand();
                FORWARD_FED( thValue, 0x02 );
                thValue = HPRand();
                FORWARD_FED( thValue, 0x01 );
                
                *outputPtr1++ = rasterByte1;   
                rasterByte1 = 0; 
                
                if (hifipe)
                {      
                    *outputPtr2++ = rasterByte2;
                    rasterByte2 = 0;  
                }
            }
            else  // Do white space skipping
            {
                inputPtr += 8;
                *outputPtr1++ = 0;
                if (hifipe)
                {      
                    *outputPtr2++ = 0;
                }
                *diffusionErrorPtr++ = 0;
                *diffusionErrorPtr++ = 0;
                *diffusionErrorPtr++ = 0;
                *diffusionErrorPtr++ = 0;
                *diffusionErrorPtr++ = 0;
                *diffusionErrorPtr++ = 0;
                *diffusionErrorPtr++ = 0;
                *diffusionErrorPtr++ = 0;
                rasterByte1 = 0;
                rasterByte2 = 0;
                tmpShortStore = 0;
            }
        } // for pixelCount
    }                                                                                                          
    else 
    {
        rasterByte1 = 0;
        rasterByte2 = 0;
        inputPtr  += ( numLoop-1 );
        outputPtr1 += ( numLoop/8 - 1 ); 
        outputPtr2 += ( numLoop/8 - 1 );
        diffusionErrorPtr += ( numLoop-1 ); 
        
        tmpShortStore = *diffusionErrorPtr;  
        
        *diffusionErrorPtr = 0;
        
        for (pixelCount = numLoop + 8; (pixelCount -= 8) > 0; ) 
        {            
            if (pixelCount > 16) // if next 16 pixels are white, skip 8
            {
                doNext8Pixels = Backward16PixelsNonWhite(inputPtr);
            } 
            else 
            {
                doNext8Pixels = HPTRUE;
            }
            
            if (doNext8Pixels)
            {
                thValue = HPRand();
                BACKWARD_FED( thValue, 0x01 );
                thValue = HPRand();
                BACKWARD_FED( thValue, 0x02 );
                thValue = HPRand();
                BACKWARD_FED( thValue, 0x04 );
                thValue = HPRand();
                BACKWARD_FED( thValue, 0x08 );
                thValue = HPRand();
                BACKWARD_FED( thValue, 0x10 );
                thValue = HPRand();
                BACKWARD_FED( thValue, 0x20 );
                thValue = HPRand();
                BACKWARD_FED( thValue, 0x40 );
                thValue = HPRand();
                BACKWARD_FED( thValue, 0x80 );
                
                *outputPtr1-- = rasterByte1;  
                rasterByte1 = 0; 
                
                if (hifipe)
                {      
                    *outputPtr2-- = rasterByte2;
                    rasterByte2 = 0;  
                }
            }
            else  // Do white space skipping
            {
                inputPtr -= 8;
                *outputPtr1-- = 0;
                if (hifipe)
                {
                    *outputPtr2-- = 0;
                }
                *diffusionErrorPtr-- = 0;
                *diffusionErrorPtr-- = 0;
                *diffusionErrorPtr-- = 0;
                *diffusionErrorPtr-- = 0;
                *diffusionErrorPtr-- = 0;
                *diffusionErrorPtr-- = 0;
                *diffusionErrorPtr-- = 0;
                *diffusionErrorPtr-- = 0;
                rasterByte1 = 0;
                rasterByte2 = 0;
                tmpShortStore = 0;
            }
        }
    }
}

//////////////////////////////////////////////////////////
void Imager_Open::FORWARD_FED( HPInt16 thresholdValue, unsigned int bitMask )
{
    tone = (*inputPtr++ );
    fedResPtr = fedResTbl + (tone << 2);
    level = *fedResPtr++;
    if (tone != 0)
    {
    tone = ( tmpShortStore + (HPInt16)(*fedResPtr++) );
    if (tone >= thresholdValue)
        {
        tone -= 255;
        level++;
        }
        switch (level)
        {
            case 0:
            break;
            case 1:
            rasterByte1 |= bitMask;
            break;
            case 2:
            rasterByte2 |= bitMask;
            break;
            case 3:
            rasterByte2 |= bitMask; rasterByte1 |= bitMask;
            break;
            case 4:
            rasterByte3 |= bitMask;
            break;
            case 5:
            rasterByte3 |= bitMask; rasterByte1 |= bitMask;
            break;
            case 6:
            rasterByte3 |= bitMask; rasterByte2 |= bitMask;
            break;
            case 7:
            rasterByte3 |= bitMask; rasterByte2 |= bitMask; rasterByte1 |= bitMask;
            break;
        }
    }
    else
    {
    tone = tmpShortStore;
    }
    *diffusionErrorPtr++ = tone >> 1;
    tmpShortStore = *diffusionErrorPtr + (tone - (tone >> 1));
}

void Imager_Open::BACKWARD_FED( HPInt16 thresholdValue, unsigned int bitMask )
{
    tone = (*inputPtr-- );
    fedResPtr = fedResTbl + (tone << 2);
    level = *fedResPtr++;
    if (tone != 0)
    {
    tone = ( tmpShortStore + (HPInt16)(*fedResPtr++) );
    if (tone >= thresholdValue)
        {
        tone -= 255;
        level++;
        }
        switch (level)
        {
            case 0:
            break;
            case 1:
            rasterByte1 |= bitMask;
            break;
            case 2:
            rasterByte2 |= bitMask;
            break;
            case 3:
            rasterByte2 |= bitMask; rasterByte1 |= bitMask;
            break;
            case 4:
            rasterByte3 |= bitMask;
            break;
            case 5:
            rasterByte3 |= bitMask; rasterByte1 |= bitMask;
            break;
            case 6:
            rasterByte3 |= bitMask; rasterByte2 |= bitMask;
            break;
            case 7:
            rasterByte3 |= bitMask; rasterByte2 |= bitMask; rasterByte1 |= bitMask;
            break;
        }
    }
    else
    {
    tone = tmpShortStore;
    }
    *diffusionErrorPtr-- = tone >> 1;
    tmpShortStore = *diffusionErrorPtr + (tone - (tone >> 1));

}
