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

#ifndef IMAGER_H
#define IMAGER_H
//===========================================================================
//
//  Filename     :  imager.h
//
//  Module       :  Open Source Imaging
//
//  Description  :  This file contains the class declaration for Imaging.
//
//===========================================================================

#ifndef HPTYPES_H
#include "hptypes.h"
#endif

// used to encourage consistent ordering of color planes
#define K   0
#define C   1
#define M   2
#define Y   3
#define Clight  4
#define Mlight  5

#define RANDSEED 77

////////////////////////////////////////////////////////////////////////////
// UMPQUA
//
// Encapsulation of buffers and data needed by Imager color-matching and
// halftoning code.

class Imager : public Processor
{
public:
    Imager(SystemServices* pSys, 
        PrintMode* pPM,
        unsigned int iInputWidth,            
        unsigned int iNumRows[],        // for mixed-res cases          
        unsigned int HiResFactor        // when base-res is multiple of 300            
        );
    virtual ~Imager();
    
    virtual BOOL Process(BYTE* pbyInputRGBRaster=NULL, unsigned int size=0)=0; 
    void Flush() { }    // halftoning has no pending output
    
    DRIVER_ERROR constructor_error;
    
    virtual void Restart();         // set up for new page or blanks

    // items required by Processor
    unsigned int GetOutputWidth();
    unsigned int GetMaxOutputWidth();
    BYTE* NextOutputRaster();
    BOOL LastPlane();  
    BOOL FirstPlane();
    unsigned int PlaneSize() 
        { return OutputWidth[iColor]/8 + (OutputWidth[iColor] % 8); }
	

    // items needed by GraphicsTranslator
    unsigned int ColorPlaneCount;
    unsigned int InputWidth;    // # of pixels input per colorplane
    unsigned int OutputWidth[MAXCOLORPLANES];   // # of pixels output per colorplane
    unsigned char ColorDepth[MAXCOLORPLANES];
    // how many rows needed relative to base resolution -- all 1 unless mixed-res
    unsigned char NumRows[MAXCOLORPLANES];      
    // color plane data
    // for current interface, we must maintain mapping of
    //  0=K, 1=C, 2=M, 3=Y
    BYTE* ColorPlane[MAXCOLORPLANES][MAXCOLORROWS][MAXCOLORDEPTH];
    
    unsigned int StartPlane;    // since planes are ordered KCMY, if no K, this is 1
    unsigned int EndPlane;      // usually Y, could be Mlight
    unsigned int ResBoost;
    
protected:
    
    SystemServices* pSS;    // needed for memory management
    
    void FreeBuffers();
    void ColorMatch( unsigned long width, const unsigned long *map, unsigned char *rgb,
        unsigned char *kplane, unsigned char *cplane, unsigned char *mplane, 
        unsigned char *yplane );
    
    virtual void Interpolate(const unsigned long *start,const unsigned long i,
        unsigned char r,unsigned char g,unsigned char b,
        unsigned char *blackout, unsigned char *cyanout, 
        unsigned char *magentaout, unsigned char *yellowout, HPBool)=0;
    
    inline unsigned char GetYellowValue(unsigned long cmyk)
    { return( ((unsigned char)((cmyk)>>24) & 0xFF) ); }                        
    
    inline unsigned char GetMagentaValue(unsigned long cmyk)
    { return( ((unsigned char)((cmyk)>>16) & 0xFF) ); }
    
    inline unsigned char GetCyanValue(unsigned long cmyk)
    { return( ((unsigned char)(((int)(cmyk))>>8) & 0xFF) ); }
    
    inline unsigned char GetBlackValue(unsigned long cmyk)
    { return( ((unsigned char)(cmyk) & 0xFF) ); }
    
    HPBool Forward16PixelsNonWhite(HPBytePtr inputPtr)
    {
//        return ((*(HPUInt32Ptr)(inputPtr) != 0x0) || (*(((HPUInt32Ptr)(inputPtr)) + 1) != 0x0)  ||
//            (*(((HPUInt32Ptr)(inputPtr)) + 2) != 0x0) || (*(((HPUInt32Ptr)(inputPtr)) + 3) != 0x0));
	for (int i=0; i < 16; i++)
	{
		if ((*inputPtr++)!=0)
			return TRUE;
	}

	return FALSE;
    }
    
    HPBool Backward16PixelsNonWhite(HPBytePtr inputPtr)
    {
//        return ((*(HPUInt32Ptr)(inputPtr) != 0x0) || (*(((HPUInt32Ptr)(inputPtr)) - 1) != 0x0)  ||
//            (*(((HPUInt32Ptr)(inputPtr)) - 2) != 0x0) || (*(((HPUInt32Ptr)(inputPtr)) - 3) != 0x0));
	inputPtr--;
	for (int i=0; i < 16; i++)
	{
		if ((*inputPtr--)!=0)
			return TRUE;
	}

	return FALSE;
    }
    
    
    // containing byte-per-pixel CMYK values
    unsigned char* Contone[6];
    // buffers used during halftoning for error terms
    short* ErrBuff[6];
    
    short   nNextRaster;
    ColorMap cmap;
    
    short          fRasterOdd; 
    unsigned char* fBlackFEDResPtr;
    unsigned char* fColorFEDResPtr;
    
    unsigned int AdjustedInputWidth;    // InputWidth padded to be divisible by 8
    void PixelMultiply(unsigned char* buffer, unsigned int width, unsigned int factor);

    unsigned int iColor, iRow, iPlane;
    unsigned int PlaneCount();          // tells how many layers (colors,hifipe,multirow)
    
};

#endif // IMAGER_H
