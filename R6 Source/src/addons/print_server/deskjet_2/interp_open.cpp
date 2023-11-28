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
//  Filename     :  interp_open
//
//  Module       :  Open Source Imaging
//
//  Description  :  Main entry point for the Imager_Open class.
//                  Do all processing for next raster.
//                  Colormatch RGB to KCMY, then halftone for each color. 
//
//============================================================================
#ifdef PROTO
#include "../../include/Header.h"
#else
#include "Header.h"
#endif

#include "imaging.h" 

BOOL Imager_Open::Process(unsigned char* pbyInputRGBRaster, unsigned int size)
{
    unsigned int i;
    int j;
    if (pbyInputRGBRaster==NULL)
    {
        Restart();
        return FALSE;   // no output
    }

    // increment current raster
    ++nNextRaster;
    if ( -1 == nNextRaster )
        nNextRaster = 0;
    
    fRasterOdd        = ( 1 & nNextRaster ) ? 0 : 1;
    
    ColorMatch( InputWidth, // ASSUMES ALL INPUTWIDTHS EQUAL
        cmap.ulMap1,pbyInputRGBRaster,
        Contone[K],Contone[C],Contone[M],Contone[Y] );
    
    if (EndPlane > Y)
        ColorMatch( InputWidth, 
        cmap.ulMap2,    // 2nd map is for lighter inks
        pbyInputRGBRaster,
        (unsigned char*)NULL,           // don't need black again
        Contone[Clight],Contone[Mlight],
        (unsigned char*)NULL            // don't need yellow again
        );
    
    for (i=StartPlane; i <= EndPlane; i++)
    {
        if (OutputWidth[i] > AdjustedInputWidth)
        {
            int factor = NumRows[i] * ResBoost;        
            PixelMultiply(Contone[i], InputWidth, factor);
        }
        
        fDitherParms[i].fNumPix = OutputWidth[i];
        fDitherParms[i].fInput = Contone[i];  
        fDitherParms[i].fErr = ErrBuff[i];
        fDitherParms[i].fErr++; // This is for serpentine
        fDitherParms[i].fSymmetricFlag = HPTRUE;   // Symmetric only
        if (i == K)
            fDitherParms[i].fFEDResPtr = fBlackFEDResPtr;
        else
            fDitherParms[i].fFEDResPtr = fColorFEDResPtr;
        fDitherParms[i].fRasterEvenOrOdd = fRasterOdd;
        fDitherParms[i].fHifipe = ColorDepth[i]>1;
        
        for (j=0; j < NumRows[i]; j++)
        {
            fDitherParms[i].fOutput1 = ColorPlane[i][j][0];
            fDitherParms[i].fOutput2 = ColorPlane[i][j][1];
            
            HTEDiffOpen  ((THTDitherParmsPtr) fDitherParms, i);            
        }       
    }
 iColor = StartPlane;
 iRow = iPlane = 0;
 iRastersReady = PlaneCount();
 iRastersDelivered = 0;
return TRUE;   // one raster in, one raster out
}


#define DOCALC(a, b, d)     a + ( ( ( (long)b - (long)a ) * d) >> 5)

void Imager_Open::Interpolate(const unsigned long *start, const unsigned long i,
                              unsigned char r,unsigned char g,unsigned char b,
                              unsigned char *blackout, unsigned char *cyanout, 
                              unsigned char *magentaout, unsigned char *yellowout,
                              HPBool firstPixelInRow)
{
    static unsigned char prev_red = 255, prev_green = 255, prev_blue = 255;
    
    unsigned long c0,c1,c2,c3,c4,c5,c6,c7;
    unsigned char diff_red, diff_green, diff_blue;
    static unsigned char bcyan, bmagenta, byellow, bblack;
    unsigned char cyan[8], magenta[8],yellow[8],black[8];
    
    c0=(*start);
    c1=*(start+1);
    c2=*(start+9);
    c3=*(start+10);
    c4=*(start+81);
    c5=*(start+82);
    c6=*(start+90);
    c7=*(start+91);
    
    if(firstPixelInRow || ( (prev_red != r) || (prev_green != g) || (prev_blue != b) ))
    {
        // update cache info
        prev_red = r;
        prev_green = g;
        prev_blue = b;
        
        cyan[0]=GetCyanValue(c0);
        cyan[1]=GetCyanValue(c1);
        cyan[2]=GetCyanValue(c2);
        cyan[3]=GetCyanValue(c3);
        cyan[4]=GetCyanValue(c4);
        cyan[5]=GetCyanValue(c5);
        cyan[6]=GetCyanValue(c6);
        cyan[7]=GetCyanValue(c7);
        
        magenta[0] = GetMagentaValue(c0);
        magenta[1] = GetMagentaValue(c1);
        magenta[2] = GetMagentaValue(c2);
        magenta[3] = GetMagentaValue(c3);
        magenta[4] = GetMagentaValue(c4);
        magenta[5] = GetMagentaValue(c5);
        magenta[6] = GetMagentaValue(c6);
        magenta[7] = GetMagentaValue(c7);
        
        yellow[0] = GetYellowValue(c0);
        yellow[1] = GetYellowValue(c1);
        yellow[2] = GetYellowValue(c2);
        yellow[3] = GetYellowValue(c3);
        yellow[4] = GetYellowValue(c4);
        yellow[5] = GetYellowValue(c5);
        yellow[6] = GetYellowValue(c6);
        yellow[7] = GetYellowValue(c7);
        
        black[0] = GetBlackValue(c0);
        black[1] = GetBlackValue(c1);
        black[2] = GetBlackValue(c2);
        black[3] = GetBlackValue(c3);
        black[4] = GetBlackValue(c4);
        black[5] = GetBlackValue(c5);
        black[6] = GetBlackValue(c6);
        black[7] = GetBlackValue(c7);
        
        ////////////////this is the 8 bit 9cube operation /////////////
        diff_red = r&0x1f;
        diff_green = g&0x1f;
        diff_blue = b&0x1f;
        
        bcyan   =   DOCALC( (DOCALC( (DOCALC( cyan[0], cyan[4], diff_red)), (DOCALC( cyan[2], cyan[6], diff_red)), diff_green)),
            (DOCALC( (DOCALC( cyan[1], cyan[5], diff_red)), (DOCALC( cyan[3], cyan[7], diff_red)), diff_green)),
            diff_blue); 
        bmagenta =  DOCALC( (DOCALC( (DOCALC( magenta[0], magenta[4], diff_red)), (DOCALC( magenta[2], magenta[6], diff_red)), diff_green)),
            (DOCALC( (DOCALC( magenta[1], magenta[5], diff_red)), (DOCALC( magenta[3], magenta[7], diff_red)), diff_green)),
            diff_blue);
        byellow =   DOCALC( (DOCALC( (DOCALC( yellow[0], yellow[4], diff_red)), (DOCALC( yellow[2], yellow[6], diff_red)), diff_green)),
            (DOCALC( (DOCALC( yellow[1], yellow[5], diff_red)), (DOCALC( yellow[3], yellow[7], diff_red)), diff_green)),
            diff_blue);
        bblack  =   DOCALC( (DOCALC( (DOCALC( black[0], black[4], diff_red)), (DOCALC( black[2], black[6], diff_red)), diff_green)),
            (DOCALC( (DOCALC( black[1], black[5], diff_red)), (DOCALC( black[3], black[7], diff_red)), diff_green)),
            diff_blue);
    }
    
    if(cyanout)
        *(cyanout + i)      =   bcyan;
    
    if(magentaout)
        *(magentaout + i)   =   bmagenta;
    
    if(yellowout)
        *(yellowout + i)    =   byellow;
    
    if(blackout)
        *(blackout + i)     =   bblack;
    
}

