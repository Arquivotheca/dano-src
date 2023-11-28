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

#ifndef IMAGING_OPEN_H
#define IMAGING_OPEN_H
//===========================================================================
//
//  Filename     :  imager_open
//
//  Module       :  Open Source Imaging
//
//  Description  :  Imager_Open class
//
//============================================================================

#ifdef PROTO
#include "../imager.h"
#else
#include "imager.h"
#endif


////////////////////////////////////////////////////////////////////////////
// Imager
//
// Encapsulation of buffers and data needed by Imager color-matching and
// halftoning code.

class Imager_Open : public Imager
{
public:
	Imager_Open(SystemServices* pSys, 
		PrintMode* pPM,
		unsigned int iInputWidth,            
		unsigned int iNumRows[],        // for mixed-res cases			
		unsigned int HiResFactor        // when base-res is multiple of 300            
		);
	virtual ~Imager_Open();
	
	virtual BOOL Process(BYTE* pbyInputRGBRaster=NULL, unsigned int size=0);			  		
	
	
protected:
	
	typedef struct THTDitherParms 
	{
		HPUInt16    fNumPix;            // Dirty Pixels to be dithered
		HPBytePtr   fInput;             // Pixel array to dither 
		HPBytePtr   fOutput1;           // Output raster binary & hifipe plane 1
		HPBytePtr   fOutput2;           // Output raster hifipe plane 2 (2-bit)
		HPBytePtr   fOutput3;           // Output raster hifipe plane 3 (3-bit)
		
		HPCBytePtr  fFEDResPtr;			// brkpnt table
		
		kSpringsErrorTypePtr    fErr;            // Current error buffer
		HPInt16                 fRasterEvenOrOdd;// Serpentine (Forward/Backward)
		
		HPBool                  fSymmetricFlag;   // Are we symmetric
		
		HPBool                  fHifipe;          // Are we doing Hifipe?
	} THTDitherParms, ENVPTR(THTDitherParmsPtr);
	
	void Interpolate(const unsigned long *start,const unsigned long i,
		unsigned char r,unsigned char g,unsigned char b,
		unsigned char *blackout, unsigned char *cyanout, 
		unsigned char *magentaout, unsigned char *yellowout, HPBool);
	
	
    HPByte HPRand()
    {
        HPByte randomNum;
		
        randomNum = pSS->GetRandomNumber();
        randomNum = randomNum % 74;
        randomNum += 5;
        return(randomNum);
    }
	
    void HTEDiffOpen   (THTDitherParmsPtr ditherParmsPtr,
		HPUInt16          count);
	
	THTDitherParms  fDitherParms[6];

    THTDitherParmsPtr       ditherParms;
    kSpringsErrorType       tone;
    kSpringsErrorTypePtr    diffusionErrorPtr;
    kSpringsErrorType       tmpShortStore;
    HPUInt8                 rasterByte1, rasterByte2, rasterByte3; 
    HPUInt8                 level;
    HPInt16                 pixelCount;
    HPInt16                 thValue;
    
    kSpringsErrorTypePtr    errPtr; 
    HPUInt16                numLoop;
    
    HPBytePtr               inputPtr;    
    HPBytePtr               outputPtr1, outputPtr2, outputPtr3;
    
    HPCBytePtr               fedResTbl;
    HPCBytePtr               fedResPtr;
    
    HPBool                  symmetricFlag;
    
    HPBool                  doNext8Pixels;
    
    HPBool                  hifipe;

    void BACKWARD_FED( HPInt16 thresholdValue, unsigned int bitMask );
    void FORWARD_FED( HPInt16 thresholdValue, unsigned int bitMask );
};

#endif // IMAGING_OPEN_H
