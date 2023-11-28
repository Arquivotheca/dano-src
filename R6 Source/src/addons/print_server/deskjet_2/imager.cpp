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
//  Filename     :  imager.cpp
//
//  Module       :  Open Source Imaging
//
//  Description  :  This file contains the constructor and destructor for
//                  the Imager class, which performs color-matching and 
//                  halftoning.
//
// Detailed Description:
//
// The only member functions needed are Process(inputRaster)
// and Restart (used to skip white space and for new page).
// 
// Configurability required in Slimhost driver is reflected in the
// parameters to the constructor:
// 1. SystemServices encapsulates memory management for platform-independence
// 2. PrintMode contains info on resolution and other properties
// 3. iInputWidth tells how many pixels input per plane
// 4. iNumRows is 1 except for mixed-resolution cases 
// 5. HiResFactor is for boosting base resolution, e.g. 2 if 600 dpi grid
//      (base res assumed to be 300)
//
// These structures, together with the variable StartPlane (designating
// K or C in the fixed ordering KCMY), are accessed by the Translator
// component of the driver, in order to properly package the data in
// the printer command language.
//============================================================================

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif

#include "imager.h"

Imager::Imager( SystemServices* pSys, PrintMode* pPM,
                unsigned int iInputWidth,            
                unsigned int iNumRows[],        			 
                unsigned int HiResFactor                
			  )
	:  ColorPlaneCount(pPM->dyeCount), InputWidth(iInputWidth), 
       ResBoost(HiResFactor), pSS(pSys),
	   nNextRaster(0), cmap(pPM->cmap), 
       fBlackFEDResPtr(pPM->BlackFEDTable),   
       fColorFEDResPtr(pPM->ColorFEDTable),
       iColor(0), iRow(0), iPlane(0)
{
	unsigned int i;
    int j,k,PlaneSize;
	constructor_error=NO_ERROR;
	ASSERT(cmap.ulMap1!=NULL);
	StartPlane=K;	    // most common case
   
	if (ColorPlaneCount==3)		// CMY pen
	  {
		StartPlane=C;
		NumRows[K]=ColorDepth[K]=OutputWidth[K]=0;
	  }

    EndPlane=Y;         // most common case
    if (ColorPlaneCount==6)
        EndPlane=Mlight;
    if (ColorPlaneCount==1)
        EndPlane=K;

    AdjustedInputWidth=InputWidth;
    if (AdjustedInputWidth % 8)
        AdjustedInputWidth += 8 - (AdjustedInputWidth % 8);
 
	// init arrays
	for (i=StartPlane; i < (ColorPlaneCount+StartPlane); i++)
	  {
		ColorDepth[i]= pPM->ColorDepth[i];
        NumRows[i]=iNumRows[i];

        OutputWidth[i]= AdjustedInputWidth * NumRows[i] * ResBoost;

	  }
	for (;i < MAXCOLORPLANES; i++)
		NumRows[i]=ColorDepth[i]=OutputWidth[i]=0;
		
	///////////////////////////////////////////////////////////////////////////
	for (i=0; i <= Mlight; i++)
      {
        Contone[i]=NULL;
        ErrBuff[i]=NULL;
      }
        
    for (i=StartPlane; i <= EndPlane; i++)
      {
        Contone[i] = (BYTE*)pSS->AllocMem(OutputWidth[i]);
        if (Contone[i]==NULL)
            goto MemoryError;
        ErrBuff[i] = (short*)pSS->AllocMem((OutputWidth[i]+2)*sizeof(short));
        if (ErrBuff[i]==NULL)
            goto MemoryError;
      }


   
    Restart();  // this zeroes buffers and sets nextraster counter

	// allocate output buffers
    for (i=0; i < MAXCOLORPLANES; i++)
	  for (j=0; j < MAXCOLORROWS; j++)
        for (k=0; k < MAXCOLORDEPTH; k++)
			ColorPlane[i][j][k]=NULL;

	for (i=StartPlane; i < (ColorPlaneCount+StartPlane); i++)
       for (j=0; j < NumRows[i]; j++)
	    for (k=0; k < ColorDepth[i]; k++)
			  {
                PlaneSize= OutputWidth[i]/8 + // doublecheck ... should already be divisble by 8
                            ((OutputWidth[i] % 8)!=0);
				ColorPlane[i][j][k] = (BYTE*) pSS->AllocMem(PlaneSize);			
				if (ColorPlane[i][j]==NULL)
					goto MemoryError;
				memset(ColorPlane[i][j][k], 0, PlaneSize);
			  }

	return;	

MemoryError:
	constructor_error=ALLOCMEM_ERROR;

	FreeBuffers();
		
	for (i=0; i < ColorPlaneCount; i++)
        for (j=0; j < NumRows[i]; j++)
	    	for (k=0; k < ColorDepth[i]; k++)
                if (ColorPlane[i][j][k])
			        pSS->FreeMemory(ColorPlane[i][j][k]);
}

Imager::~Imager()
{
	DBG1("destroying Imager \n");

	FreeBuffers();
	
	for (int i=0; i < MAXCOLORPLANES; i++)
        for (int j=0; j < NumRows[i]; j++)
		    for (int k=0; k < ColorDepth[i]; k++)
			    if (ColorPlane[i][j][k])
			        pSS->FreeMemory(ColorPlane[i][j][k]);
}

void Imager::Restart()
{
	nNextRaster = 0;

    for (unsigned int i=StartPlane; i <= EndPlane; i++)
      {
        memset(Contone[i], 0, OutputWidth[i]);
        memset(ErrBuff[i], 0, (OutputWidth[i]+2)*sizeof(short));
      }
}

void Imager::FreeBuffers()
{
    for (unsigned int i=StartPlane; i <= EndPlane; i++)
      {
        pSS->FreeMemory(Contone[i]);
        pSS->FreeMemory(ErrBuff[i]);
      }
}

// dumb horizontal doubling (tripling, etc.) for resolution-boost prior to halftoning
void Imager::PixelMultiply(unsigned char* buffer, unsigned int width, unsigned int factor)
{ 
    if (factor==1)
        return;

    for (int j = (int)(width-1); j >=0; j--) 
        for (unsigned int k=0; k<factor; k++)
            buffer[j*factor+k] = buffer[j];
        
}

BYTE* Imager::NextOutputRaster()
{
	if (iRastersReady == 0)
		return NULL;

    if (iColor == (ColorPlaneCount+StartPlane))
        return NULL;
    
    if (iPlane == ColorDepth[iColor])
    {
        iPlane = 0;
        iRow++;
        return NextOutputRaster();
    }

    if (iRow == NumRows[iColor])
    {
        iRow = 0;
        iColor++;
        return NextOutputRaster();
    }
    
    iRastersDelivered++;
    iRastersReady--;
   return ColorPlane[iColor][iRow][iPlane++];
}

BOOL Imager::LastPlane()
{   
      return ((iColor == (ColorPlaneCount+StartPlane-1)) &&
              (iRow == (unsigned int)(NumRows[iColor] - 1)) &&
              (iPlane == ColorDepth[iColor])        // was pre-incremented
              );           
}

BOOL Imager::FirstPlane()
{   
      return ((iColor == StartPlane) &&
              (iRow == 0) &&
              (iPlane == 1)        // was pre-incremented
              );           
}

unsigned int Imager::GetOutputWidth()
// return size of data in the plane being delivered (depends on iRastersDelivered)
// (will be used in connection with compression seedrow)
{
    unsigned int colorplane, tmp;
    // figure out which colorplane we're on
    unsigned int raster=iRastersDelivered;
    // we come after increment of iRastersDelivered
    if (raster>0)
        raster--;

    tmp = (unsigned int)(NumRows[0]*ColorDepth[0]);
    if (raster < tmp)
        colorplane=0;
//    else if (iRastersDelivered < (NumRows[0]*ColorDepth[0] + NumRows[1]*ColorDepth[1]))
//            colorplane=1;
    // have to count up to possible 6th plane;
    // but we'll save code by assuming sizes of C,M,Y (Cl,Ml) are all same
         else colorplane=1;

    int temp = (OutputWidth[colorplane] / 8) + ((OutputWidth[colorplane]%8)!=0);
    return temp;
}

unsigned int Imager::GetMaxOutputWidth()
// This is needed by Configure, since the output-width for Imager is variable
// depending on the colorplane
{
    unsigned int max=0;
    for (unsigned int i=StartPlane; i <= EndPlane; i++)
        if (OutputWidth[i] > max)
            max = OutputWidth[i];

    return (max / 8) + ((max%8)!=0);
}

unsigned int Imager::PlaneCount()
{
 unsigned int count=0;

     for (int i=0; i < MAXCOLORPLANES; i++)
         count += NumRows[i] * ColorDepth[i];
 
  return count;
}
