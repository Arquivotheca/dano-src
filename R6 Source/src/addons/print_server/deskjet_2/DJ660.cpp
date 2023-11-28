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

#ifdef _DJ6xx

#ifdef PROTO
#include "../include/Header.h"
#include "../include/IO_defs.h"
#include "../Printer/DJ6XX.h"
#include "../Printer/DJ660.h"
#else
#include "Header.h"
#include "IO_defs.h"
#include "DJ6XX.h"
#include "DJ660.h"
#endif


extern unsigned long ulMapVOLTAIRE_CCM_KCMY[ 9 * 9 * 9 ];
extern unsigned long ulMapVOLTAIRE_CCM_K[ 9 * 9 * 9 ];
//
// ** DeskJet660:Printer CLASS **
//

DeskJet660::DeskJet660(SystemServices* pSS,BOOL proto)
	: DeskJet6XX(pSS,NUM_DJ6XX_FONTS,proto)
// create two dummy font objects to be queried via EnumFont
{
DBG1("DeskJet660::VerifyPenInfo(): called\n");
	
    if ((!proto) && (IOMode.bDevID))
      {
        constructor_error = VerifyPenInfo(); 
        CERRCHECK; 
      }


    pMode[DEFAULTMODE_INDEX] = new PrintMode( ulMapVOLTAIRE_CCM_KCMY );
    pMode[GRAYMODE_INDEX] = new GrayMode(ulMapVOLTAIRE_CCM_K);

	  
DBG1("DeskJet 660 created\n");
}


Header* DeskJet660::SelectHeader(PrintContext* pc)
{ 
	return new Header6XX(this,pc); 
}

DRIVER_ERROR DeskJet660::VerifyPenInfo()
{

    DRIVER_ERROR err=NO_ERROR;

	if(IOMode.bDevID == FALSE) 
        return err;

    PEN_TYPE ePen;

	err = ParsePenInfo(ePen);
	ERRCHECK;


	// check for the normal case
	if (ePen == BOTH_PENS)
		return NO_ERROR;


	// the 6XX printers are all two-pen, so trap
	// on any pen type that is not BOTH_PENS
	while ( ePen != BOTH_PENS )
	{

		switch (ePen)
		{
            case MDL_BOTH:
            case MDL_PEN:
                // user shouldn't be able to get photopen in a non-690...
				pSS->DisplayPrinterStatus(DISPLAY_PHOTO_PEN_WARN);
				break;
            case BLACK_PEN:
				// black pen installed, need to install color pen
				pSS->DisplayPrinterStatus(DISPLAY_NO_COLOR_PEN);
				break;
			case COLOR_PEN:
				// color pen installed, need to install black pen
				pSS->DisplayPrinterStatus(DISPLAY_NO_BLACK_PEN);
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

DRIVER_ERROR DeskJet660::ParsePenInfo(PEN_TYPE& ePen, BOOL QueryPrinter)
{
	char* str;
    DRIVER_ERROR err = SetPenInfo(str, QueryPrinter);
    ERRCHECK;

    str++;  // skip $
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

#endif  // _DJ660
