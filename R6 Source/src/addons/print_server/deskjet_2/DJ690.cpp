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

#ifdef _DJ6xxPhoto

#ifdef PROTO
#include "../include/Header.h"
#include "../include/IO_defs.h"
#include "../Printer/DJ690.h"
#else
#include "Header.h"
#include "IO_defs.h"
#include "DJ690.h"
#endif


extern unsigned long ulMapCONQUEST_CMYK[ 9 * 9 * 9 ];
extern unsigned long ulMapCONQUEST_ClMlxx[ 9 * 9 * 9 ];
extern unsigned long ulMapVOLTAIRE_CCM_KCMY[ 9 * 9 * 9 ];
extern unsigned long ulMapVOLTAIRE_CCM_K[ 9 * 9 * 9 ];
//
// ** DeskJet690:Printer CLASS **
//
Mode690::Mode690()
// print mode for photo pen
: PrintMode( ulMapCONQUEST_CMYK, ulMapCONQUEST_ClMlxx )
{
   dyeCount=6;
   medium =  mediaSpecial;
//   theQuality = qualityPresentation;
   theQuality = qualityNormal;

   BaseResX = 600;
   for (int i=0; i < 6; i++)         
        ResolutionX[i]=600;

   CompatiblePens[0] = MDL_BOTH;

   strcpy(ModeName, "Photo");

}

DeskJet690::DeskJet690(SystemServices* pSS, BOOL proto)
	: Printer(pSS, NUM_DJ6XX_FONTS,proto)
// create two dummy font objects to be queried via EnumFont
{	
    
    if ((!proto) && (IOMode.bDevID))
      {
        constructor_error = VerifyPenInfo(); 
        CERRCHECK; 
      }
  
    pMode[DEFAULTMODE_INDEX] = new PrintMode( ulMapVOLTAIRE_CCM_KCMY );
    pMode[GRAYMODE_INDEX] = new GrayMode(ulMapVOLTAIRE_CCM_K);
    pMode[SPECIALMODE_INDEX] = new Mode690();

    ModeCount = 3;

	DBG1("DJ690 created\n");

}


Header690::Header690(Printer* p,PrintContext* pc)
	: Header(p,pc)
{  }

Header* DeskJet690::SelectHeader(PrintContext* pc)
{     
    return new Header690(this, pc);
}

DRIVER_ERROR Header690::Send()
{	DRIVER_ERROR err;

	StartSend();

	err = thePrinter->Send((const BYTE*)BlackExtractOff, 
						sizeof(BlackExtractOff)); // just pertains to 2-pen
	ERRCHECK;

	err = ConfigureRasterData();
	ERRCHECK;						
					
	err=Graphics();		// start raster graphics and set compression mode
	
return err;
}
DRIVER_ERROR DeskJet690::VerifyPenInfo()
{

    DRIVER_ERROR err=NO_ERROR;

	if(IOMode.bDevID == FALSE) 
        return err;

    PEN_TYPE ePen;

	err = ParsePenInfo(ePen);
	ERRCHECK;

	// check for the normal case
	if (ePen == BOTH_PENS || ePen == MDL_BOTH)
		return NO_ERROR;

DBG1("DeskJet690::VerifyPenInfo(): ePen is not BOTH_PENS or MDL_BOTH\n");

	// the 6XX printers are all two-pen, so trap
	// on any pen type that is not MDL_BOTH or
	// BOTH_PENS
	while ( (ePen != BOTH_PENS) && (ePen != MDL_BOTH)	)
	{
DBG1("DeskJet690::VerifyPenInfo(): in while loop\n");

		switch (ePen)
		{
            case MDL_PEN:	
            case BLACK_PEN:
				// black or photopen installed, need to install color pen
				pSS->DisplayPrinterStatus(DISPLAY_NO_COLOR_PEN);
				break;
			case COLOR_PEN:
				// color pen installed, need to install black pen
                // - use ambiguous message because of black or photo pen
				pSS->DisplayPrinterStatus(DISPLAY_NO_PEN_DJ600);
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

DRIVER_ERROR DeskJet690::ParsePenInfo(PEN_TYPE& ePen, BOOL QueryPrinter)
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


#endif  // _DJ690
