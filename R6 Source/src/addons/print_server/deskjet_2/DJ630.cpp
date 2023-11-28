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

#ifdef _DJ630

#ifdef PROTO
#include "../include/Header.h"
#include "../include/IO_defs.h"
#include "../Printer/DJ630.h"
#else
#include "Header.h"
#include "IO_defs.h"
#include "DJ630.h"
#endif

extern unsigned long ulMapCONQUEST_CMYK[ 9 * 9 * 9 ];
extern unsigned long ulMapCONQUEST_ClMlxx[ 9 * 9 * 9 ];
extern unsigned long ulMapVOLTAIRE_CCM_KCMY[ 9 * 9 * 9 ];
extern unsigned long ulMapVOLTAIRE_CCM_K[ 9 * 9 * 9 ];
extern unsigned long ulMapVOLTAIRE_CCM_CMY[ 9 * 9 * 9 ];


DeskJet630::DeskJet630(SystemServices* pSS, BOOL proto)
	: Printer(pSS, NUM_DJ6XX_FONTS,proto)
{	
    
    if ((!proto) && (IOMode.bDevID))
      {
        constructor_error = VerifyPenInfo(); 
        CERRCHECK; 
      }
  
    pMode[DEFAULTMODE_INDEX]		= new Mode630Color();
    
    pMode[SPECIALMODE_INDEX + 0]	= new PrintMode(ulMapVOLTAIRE_CCM_KCMY);
    pMode[SPECIALMODE_INDEX + 1]	= new Mode630Photo();
    pMode[GRAYMODE_INDEX]			= new GrayMode630(ulMapVOLTAIRE_CCM_K);

    ModeCount = 4;

	DBG1("DJ630 created\n");

}

GrayMode630::GrayMode630(unsigned long *map)
: GrayMode(map)
{
    CompatiblePens[1] = DUMMY_PEN;
}

DRIVER_ERROR DeskJet630::VerifyPenInfo()
{

    DRIVER_ERROR err=NO_ERROR;

	if(IOMode.bDevID == FALSE) 
        return err;

    PEN_TYPE ePen;

	err = ParsePenInfo(ePen);
	ERRCHECK;

	// check for the normal case
	if (ePen == BOTH_PENS || ePen == MDL_BOTH || ePen == COLOR_PEN)
		return NO_ERROR;

DBG1("DeskJet630::VerifyPenInfo(): ePen is not BOTH_PENS, MDL_BOTH, or COLOR_PEN\n");

	// the 630 printers require the color pen to be installed, so trap
	// on any pen type that does not include the color pen
	while ( (ePen != BOTH_PENS) && (ePen != MDL_BOTH) && (ePen != COLOR_PEN) )
	{
DBG1("DeskJet630::VerifyPenInfo(): in while loop\n");

		switch (ePen)
		{
            case MDL_PEN:	
            case BLACK_PEN:
				// black or photopen installed, need to install color pen
				pSS->DisplayPrinterStatus(DISPLAY_NO_COLOR_PEN);
				break;
			case NO_PEN:
				// neither pen installed
			default:
				pSS->DisplayPrinterStatus(DISPLAY_NO_PENS);
				break;
		}

		if (pSS->BusyWait(500) == JOB_CANCELED)
			return JOB_CANCELED;

		err =  ParsePenInfo(ePen);
		ERRCHECK;
	}

	pSS->DisplayPrinterStatus(DISPLAY_PRINTING);

	return NO_ERROR;

}

DRIVER_ERROR DeskJet630::ParsePenInfo(PEN_TYPE& ePen, BOOL QueryPrinter)
{
	char* str;
    DRIVER_ERROR err = SetPenInfo(str, QueryPrinter);
    ERRCHECK;

    str++;    // skip $
	// parse penID
	PEN_TYPE temp_pen1;
	// check pen1, assume it is black or MDL, pen2 is color
	switch (str[0])
	{
		// check for MDL in case someone wedged one in there
        case 'M': temp_pen1 = MDL_PEN; break; // (M)ulti-Dye load pen
		case 'C': temp_pen1 = BLACK_PEN; break; // (C)andide black
		default:  temp_pen1 = NO_PEN; break;
	}

	// now check pen2

	int i=2;
	while(str[i]!='$') i++; // handles variable length penIDs
	i++;

	if(str[i]=='R')	// we have the (R)obinhood color pen, 
					// check what pen1 was
	{
		if (temp_pen1 == BLACK_PEN)
				ePen = BOTH_PENS;
		else 
		{
			if (temp_pen1 == MDL_PEN)
				ePen = MDL_BOTH;
			else
				ePen = COLOR_PEN;
		}
	}
	else // no color pen, just set what pen1 was
		ePen = temp_pen1;

	return NO_ERROR;
}


Mode630Photo::Mode630Photo()
// print mode for photo pen
: PrintMode(ulMapCONQUEST_CMYK,ulMapCONQUEST_ClMlxx)
{
   dyeCount=6;
   medium =  mediaSpecial;
   theQuality = qualityNormal;


   BaseResX = 600;
   for (int i=0; i < 6; i++)         
        ResolutionX[i]=600;

   CompatiblePens[0] = MDL_BOTH;

   strcpy(ModeName, "Photo");

}

Mode630Color::Mode630Color()
: PrintMode(ulMapVOLTAIRE_CCM_CMY)
{
   dyeCount=3;
   CompatiblePens[0] = COLOR_PEN;   // only color pen allowed


   strcpy(ModeName, "Color");
}

Header630::Header630(Printer* p,PrintContext* pc)
	: Header(p,pc)
{  }

Header* DeskJet630::SelectHeader(PrintContext* pc)
{     
    return new Header630(this,pc);
}

DRIVER_ERROR Header630::Send()
{	DRIVER_ERROR err;

	StartSend();

    // don't disable black extraction if we are in the color pen only mode
    // since the printer may actually have both pens so we want the printer
    // to print the K with the K pen if it actually has one
    if (thePrintMode->dyeCount != 3)
    {
	    err = thePrinter->Send((const BYTE*)BlackExtractOff, 
						    sizeof(BlackExtractOff)); // just pertains to 2-pen
	    ERRCHECK;
    }

	err = ConfigureRasterData();
	ERRCHECK;						
					
	err=Graphics();		// start raster graphics and set compression mode
	
return err;
}


#endif  // _DJ630
