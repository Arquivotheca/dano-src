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

#ifdef _DJ600
#ifdef PROTO
#include "../include/Header.h"
#include "../include/IO_defs.h"
#include "../Printer/DJ6XX.h"
#include "../Printer/DJ600.h"
#else
#include "Header.h"
#include "IO_defs.h"
#include "DJ6XX.h"
#include "DJ600.h"
#endif


extern unsigned long ulMapVOLTAIRE_CCM_K[ 9 * 9 * 9 ];
extern unsigned long ulMapVOLTAIRE_CCM_CMY[ 9 * 9 * 9 ];
//
// ** DeskJet600:Printer CLASS **
//

Mode600::Mode600()
: PrintMode( ulMapVOLTAIRE_CCM_CMY )
{
   dyeCount=3;
   CompatiblePens[0] = COLOR_PEN;   // only color pen allowed
}

DeskJet600::DeskJet600(SystemServices* pSS, BOOL proto)
	: DeskJet6XX(pSS, NUM_DJ6XX_FONTS,proto)
{ 
    if ((!proto) && (IOMode.bDevID))
      {
        constructor_error = VerifyPenInfo(); 
        CERRCHECK; 
      }

    pMode[DEFAULTMODE_INDEX] = new Mode600();
    pMode[GRAYMODE_INDEX] = new GrayMode(ulMapVOLTAIRE_CCM_K);


DBG1("DeskJet 600 created\n");
}

 
Header600::Header600(Printer* p,PrintContext* pc)
	: Header6XX(p,pc)
{ }

DRIVER_ERROR Header600::Send()
// Sends 600-style header to printer.
{	DRIVER_ERROR err;

	StartSend();

	if (dyeCount==3)	// color pen
	  {
	  	err = ConfigureRasterData();
		ERRCHECK;						
	  }
	else				// black pen
	  {
		err=Simple();			// set color mode and resolution
		ERRCHECK;
	  }
	
	err=Graphics();		// start raster graphics and set compression mode
	
return err;
}

Header* DeskJet600::SelectHeader(PrintContext* pc)
{
	return new Header600(this,pc);
}

DRIVER_ERROR DeskJet600::VerifyPenInfo() 
{
	DRIVER_ERROR err=NO_ERROR;

	if(IOMode.bDevID == FALSE) 
        return err;

    PEN_TYPE ePen;

	err = ParsePenInfo(ePen);
	ERRCHECK;


	if (ePen == BLACK_PEN || ePen == COLOR_PEN)	
	// pen was recognized
	{
		return NO_ERROR;
	}

	// BLACK_PEN and COLOR_PEN are the only valid pens, so loop and
	// display error message until user cancels or a valid pen installed
	while(ePen != BLACK_PEN && ePen != COLOR_PEN)
	{
		pSS->DisplayPrinterStatus(DISPLAY_NO_PEN_DJ600);

		if(pSS->BusyWait(500) == JOB_CANCELED)
		{
			return JOB_CANCELED;
		}

		err = ParsePenInfo(ePen);
		ERRCHECK;
	}
		
	pSS->DisplayPrinterStatus(DISPLAY_PRINTING);

	// The 600 will report OFFLINE for a while after the
	// pen has been installed.  Let's wait for it to
	// come online and not confuse the user with a potentially
	// bogus OFFLINE message

	if (pSS->BusyWait((DWORD)1000) == JOB_CANCELED)
		return JOB_CANCELED;

    return NO_ERROR;

}

DRIVER_ERROR DeskJet600::ParsePenInfo(PEN_TYPE& ePen, BOOL QueryPrinter) 
{
	char* c;
    DRIVER_ERROR err = SetPenInfo(c, QueryPrinter);
    ERRCHECK;
       
    c++;    // skip $
	// parse penID
	
	if(c[0] == 'R') 		// (R)obinhood color
		ePen = COLOR_PEN;
	else if(c[0] == 'C')	// (C)andide black
			ePen = BLACK_PEN;
	     else ePen = NO_PEN;

	return NO_ERROR;
}

#endif  // _DJ600
