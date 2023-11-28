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

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif

extern BYTE EscCopy(BYTE *dest,const char *s,int num, char end);
extern BYTE EscAmplCopy(BYTE *dest, int num, char end);
extern PAPER_SIZE MediaSizeToPaper(MediaSize msize);
extern MediaSize PaperToMediaSize(PAPER_SIZE psize);


float frac(float f)
// Returns fractional part of float
{ int g=(int)f;
  float fp= f - (float)g;
  return fp;
}
//////////////////////////////////////////////////////////////////////////////////////
// Header constructor
// Each printer model has a routine SelectHeader to call the constructor as needed.
// The various PCL options we support are collected and initialized here.
//
Header::Header(Printer* p,PrintContext* pc)
	:  CAPy(0), thePrinter(p), thePrintContext(pc)
{ 
    thePrintMode = thePrintContext->CurrentMode;

    dyeCount = thePrintMode->dyeCount;

    DBG1("In Header constructor\n");
    for (int i=0; i<MAXCOLORPLANES; i++)
	  {
		ResolutionX[i] = thePrintMode->ResolutionX[i];
		ResolutionY[i] = thePrintMode->ResolutionY[i];
	  }

    
    // Set up internal data members with proper escape sequence data, for easy output later.
    // Get values from PrintContext
	SetMediaType(thePrintContext->CurrentMode->medium);
	SetMediaSize( thePrintContext->thePaperSize );
	SetMediaSource(sourceTray1);                    // hardcoded for now
	SetQuality(thePrintContext->CurrentMode->theQuality);

    SetSimpleColor();

}


//////////////////////////////////////////////////////////////////////////
// routines to set internal data members

void Header::SetMediaType(MediaType mtype)
// Sets value of mediatype and associated counter mtcount
{	
	mtcount=EscAmplCopy((BYTE*)mediatype,mtype,'M'); 
}

void Header::SetMediaSize(PAPER_SIZE psize)
// Sets value of PCL::mediasize and associated counter mscount
{        
	int msizeCode;

	msizeCode = PaperToMediaSize(psize);
 
	mscount=EscAmplCopy((BYTE*)mediasize,msizeCode,'A'); 
}

void Header::SetMediaSource(MediaSource msource)
// Sets value of PCL::mediasource and associated counter msrccount
{
#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
    DBG1("Setting MediaSource to ");
    switch(msource)
	{
	    case sourceTray1: DBG1("Tray1\n"); break;
	    case sourceManual: DBG1("Manual\n"); break;
	    case sourceManualEnv: DBG1("ManualEnv\n"); break;
	    default: DBG1("<out of range, using> Tray1\n"); break;
	}
#endif

	ASSERT( (msource==sourceTray1) || (msource==sourceManual) || 
			(msource==sourceManualEnv) );

	if ( (msource!=sourceTray1) && (msource!=sourceManual) && 
			(msource!=sourceManualEnv) )
		msource=sourceTray1;

	msrccount=EscAmplCopy((BYTE*)mediasource,msource,'H');
}

void Header::SetQuality(Quality qual)
// Sets value of PCL::quality and associated counter qualcount
{
#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
    DBG1("Setting Quality to ");
    switch(qual)
	{
	    case qualityPresentation: DBG1("Presentation\n"); break;
	    case qualityNormal: DBG1("Normal\n"); break;
	    case qualityDraft: DBG1("Draft\n"); break;
	    default: DBG1("<out of range, using> Normal\n"); break;
	}
#endif
    
	ASSERT( (qual==qualityPresentation) || (qual==qualityNormal)
			|| (qual==qualityDraft) );

	if ( (qual!=qualityPresentation) && (qual!=qualityNormal)
			&& (qual!=qualityDraft) )
		qual=qualityNormal;
	
	qualcount=EscCopy((BYTE*)quality,"*o",qual,'M');
}

BYTE Header::QualityCode()
{ return quality[3]; }

void Header::SetSimpleColor()
// Sets value of SimpleColor and associated counter sccount
// Only applicable to one-pen products.
// SimpleColor command tells printer how many color planes; alternative to
// configure raster data in 6xx.
{ 
    DBG1("Setting SimpleColor mode\n");
    int planes= 0 - dyeCount;
	sccount=EscCopy((BYTE*)SimpleColor,"*r",planes,'U'); 
}


DRIVER_ERROR Header::StartSend()
// common items gathered for code savings
{
	DRIVER_ERROR err;
	err = thePrinter->Send((const BYTE*)Reset,sizeof(Reset));
	ERRCHECK;

	err = thePrinter->Send((const BYTE*)UEL,sizeof(UEL));
	ERRCHECK;

	err = thePrinter->Send((const BYTE*)EnterLanguage,sizeof(EnterLanguage));
	ERRCHECK;

	err = thePrinter->Send((const BYTE*)PCL3,sizeof(PCL3));
	ERRCHECK;

	err = thePrinter->Send((const BYTE*)&LF,1);
	ERRCHECK;
		
	err=Modes();			// Set media source, type, size and quality modes.
	ERRCHECK;	
				
	err=Margins();			// set margins
	
	return err;
}

///////////////////////////////////////////////////////////////////////////
DRIVER_ERROR Header::ConfigureRasterData()
// This is the more sophisticated way of setting color and resolution info.
//
// NOTE: Will need to be overridden for DJ5xx.
{ 
	
#define CRDSIZE 41	// 5 + 6 (max#dyes) * 6	
char buff[CRDSIZE]; 
char *out=buff;


	// begin the CRD command
	memcpy(out,crdStart,sizeof(crdStart) );
	out += sizeof(crdStart);

	// now set up the "#W" part, where #= number of data bytes in the command
	// #= 6 per color (2 each for horiz. res., vert. res., and color levels)
	//	  + 2 for format and dye count
	char sBuffer[3];
	int num = dyeCount * MAXCOLORPLANES + 2;
	sprintf( sBuffer, "%d", num );
	*out++ = sBuffer[0]; 
	if (num > 9)
		*out++ = sBuffer[1];
	*out++ = 'W';

	*out++ = crdFormat;
	*out++ = (BYTE)dyeCount;

	int start=K;	    // most common case
    if (dyeCount==3)    // CMY pen
        start=C;

	for (unsigned int color = start; color < (start + dyeCount); color++)
	 {	
		*out++ = ResolutionX[color]/256;
		*out++ = ResolutionX[color]%256;
		*out++ = ResolutionY[color]/256;
		*out++ = ResolutionY[color]%256;

		int depth=ColorLevels(color);

		*out++ = depth/256;
		*out++ = depth%256;
	 }
    DRIVER_ERROR error= thePrinter->Send((const BYTE*) buff, out-buff);
    return error;
}


#define unprintable (float).04
#define granularity 48

DRIVER_ERROR Header::Margins()
// Some code shared between different header flavors.
{
#define MARGINSIZE (5 * 6)	// each item can be 6
BYTE buff[MARGINSIZE]; 
BYTE *out=buff;
// Calculate top margin.
// This is given in terms of text line spacing, the smallest available 
// granularity of which is 48.
// There also seems to be a starting unprintable distance, which is
//  .04 for printers checked so far. (Empirical check; 600 manual says zero.)
// So our formula is: marginvalue = granularity * (startY/verticalres - unprintable)

	float tmInches = (float)( thePrintContext->PrintableStartY() ) / 
					 (float)( thePrintContext->printerunitsY() );
	float AdjustedStartY = tmInches - unprintable;
	float fTopMargin = (float)granularity * AdjustedStartY;
	// now round result up or down
	int iTopMargin = (int) fTopMargin;	
	if (frac(fTopMargin) > (float).5)
		iTopMargin++;


	out += EscAmplCopy(out,0, 'L');				// turn perforation-skip OFF
	out += EscAmplCopy(out,granularity,'D');	// set line spacing (for top margin)
	out += EscAmplCopy(out,iTopMargin,'E');		// set top margin
	out += EscAmplCopy(out,6,'D');				// reset to default in case of 
												//		simple text mode
			
	// set CAPx -- this must be done prior to grafStart (start of raster graphics)
	// to fix the graphics margin
	out += EscCopy(out,"*p", 0, 'X'); 

	DRIVER_ERROR error= thePrinter->Send((const BYTE*) buff,(out-buff) );

return error;
}

DRIVER_ERROR Header::Graphics()
{
	DRIVER_ERROR error= thePrinter->Send((const BYTE*)grafStart, sizeof(grafStart) );
	if (error!=NO_ERROR)
		return error;	

	if (thePrintMode->Config.bCompress)
		{	
			error= thePrinter->Send((const BYTE*)grafMode9, sizeof(grafMode9) );
			if (error!=NO_ERROR)
				return error;
			error= thePrinter->Send((const BYTE*)SeedSame, sizeof(SeedSame) );			
	    }
	else
		error= thePrinter->Send((const BYTE*)grafMode0, sizeof(grafMode0) );
		
return error;
#define GRAPHICSIZE (sizeof(grafStart)+sizeof(grafMode9)+sizeof(SeedSame))
}


DRIVER_ERROR Header::Simple()
// Items pertaining to "simple color" mode, including resolution and width.
// (This is an alternative method to the CRD command.)
// This sequence is used when in text-only mode, so reference no graphics data.
{
#define SIMPLESIZE 21	// max 7 for each of 3
	BYTE buff[SIMPLESIZE];
	BYTE *out = buff;

	int resolution = thePrintContext->CurrentMode->BaseResX;
	int gwidth = thePrintContext->PageWidth;

	memcpy(out, SimpleColor, sccount); 
    out += sccount; 

	// set resolution
	out += EscCopy(out,"*t", resolution, 'R');

	// set graphics width
	out += EscCopy(out,"*r", gwidth, 'S'); 

	
	DRIVER_ERROR error= thePrinter->Send((const BYTE*) buff,(out-buff) );

return error;

}

DRIVER_ERROR Header::Modes()
// Set media source, type, size and quality modes.
{
#define MODESIZE 6*4
	BYTE buff[MODESIZE];
	BYTE *out = buff;

	memcpy(out,mediasource, msrccount  );
	out += msrccount;
	memcpy(out,mediatype, mtcount );
	out += mtcount;
    if (thePrintContext->thePaperSize != PHOTO_SIZE)
    {
	    memcpy(out,mediasize,mscount );
	    out += mscount;
    }
	memcpy(out,quality, qualcount );
	out += qualcount;

DRIVER_ERROR error= thePrinter->Send((const BYTE*) buff,(out-buff) );

return error;
}
////////////////////////////////////////////////////////////////////



DRIVER_ERROR Header::EndJob()
{		
	DRIVER_ERROR err = thePrinter->Send((const BYTE*)Reset,sizeof(Reset));
	ERRCHECK;

	return thePrinter->Send((const BYTE*)UEL,sizeof(UEL));
}


DRIVER_ERROR Header::SendCAPy(unsigned int iAbsY)
{ int len; BYTE temp[10];

	len= EscCopy(temp,"*p",iAbsY,'Y');
	
 return thePrinter->Send( temp, len);
} 

unsigned int Header::ColorLevels(unsigned int ColorPlane) 
{
    // convert from number of bits to levels
	int bits=thePrintContext->CurrentMode->ColorDepth[ColorPlane];
	int res=1;
	for (int i=0; i<bits; i++)
		res=res*2;
	
	return  res;
}

DRIVER_ERROR Header::FormFeed()
{
	BYTE FF=12;
	return thePrinter->Send((const BYTE*)&FF,1);
}
