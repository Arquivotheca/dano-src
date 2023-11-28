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

#ifdef _DJ9xx

#ifdef PROTO
#include "../imaging/resources.h"
#include "../include/Header.h"
#include "../include/IO_defs.h"
#include "../Printer/DJ895.h"
#include "../Printer/broadway.h"
extern int argProprietary;
extern BOOL argPhotoTray;
#else
#include "Header.h"
#include "IO_defs.h"
#include "DJ895.h"
#include "broadway.h"
extern BYTE* GetHT3x3_4();
extern BYTE* GetHT6x6_4_970();
#endif

extern unsigned long ulMapBROADWAY_KCMY[ 9 * 9 * 9 ];
extern unsigned long ulMapBROADWAY_Gossimer_Normal_KCMY[ 9 * 9 * 9 ]; 
extern unsigned long ulMapVOLTAIRE_CCM_K[ 9 * 9 * 9 ];


Broadway::Broadway(SystemServices* pSS, BOOL proto)
: Printer(pSS,NUM_DJ6XX_FONTS,proto)
{
    if (IOMode.bDevID)
    {
		bCheckForCancelButton = TRUE;
        constructor_error = VerifyPenInfo();
        CERRCHECK;
    }

    ModeCount=3;

    pMode[DEFAULTMODE_INDEX] = new BroadwayMode1();
    pMode[SPECIALMODE_INDEX] = new BroadwayMode2();
    pMode[GRAYMODE_INDEX] = new GrayMode(ulMapVOLTAIRE_CCM_K);
  

}

Broadway::~Broadway()
{ }

BroadwayMode1::BroadwayMode1()
: PrintMode(ulMapBROADWAY_KCMY)
// 600x600x1 K
// 300x300x2 CMY
{
    
    ColorDepth[K]=1;  // 600x600x1 K          
    
    for (int i=1; i < 4; i++) 
        ColorDepth[i]=2;    // 300x300x2 CMY
          
    ResolutionX[K]=ResolutionY[K]=600;

    MixedRes = TRUE;

#ifdef PROTO
    if (!argProprietary)  
        ColorFEDTable = (BYTE*) HT300x3004level_open;
    else
        ColorFEDTable = (BYTE*) HT300x3004level_prop;
#else
    ColorFEDTable = GetHT3x3_4();
#endif
}

BroadwayMode2::BroadwayMode2()
: PrintMode(ulMapBROADWAY_Gossimer_Normal_KCMY) 
// 600x600x1 K
// 600x600x2 CMY
{
    int i;
    ColorDepth[K]=1;  // 600x600x1 K          
    
    for (i=1; i < 4; i++) 
        ColorDepth[i]=2;    // 300x300x2 CMY

    for (i=0; i < 4; i++) 
        ResolutionX[i]=ResolutionY[i]=600;

    BaseResX = BaseResY = 600;
    MixedRes = FALSE;

    medium = mediaGlossy;

#ifdef PROTO
    if (!argProprietary)
        ColorFEDTable = (BYTE*) HT600x6004level970_open;
    else 
        ColorFEDTable = (BYTE*) HT600x6004level970_prop;
#else
    ColorFEDTable = GetHT6x6_4_970();
#endif

    strcpy(ModeName, "Photo");
    bFontCapable=FALSE;
}


BOOL Broadway::UseGUIMode(unsigned int PrintModeIndex)
{
  return (PrintModeIndex==SPECIALMODE_INDEX);
}

Compressor* Broadway::CreateCompressor(unsigned int RasterSize)
{
	return new Mode2(pSS,RasterSize);
}

Header900::Header900(Printer* p,PrintContext* pc)
	: Header895(p,pc)
{ }

Header* Broadway::SelectHeader(PrintContext* pc)
{ 
	return new Header900(this,pc); 
}

DRIVER_ERROR Header900::Send()
{
	DRIVER_ERROR err;
	//BOOL bDuplex = FALSE;

	StartSend();

	// this code will look for the duplexer enabled in the device ID and send the right
	// escape to the printer to enable duplexing.  At this time, however, we are not
	// going to support duplexing.  One, it is not supported with PCL3, which we need
	// for device font support.  Second, we don't have the resources to reformat the page
	// for book duplexing and can only do tablet.

    /*BYTE bDevIDBuff[DevIDBuffSize];
    err = theTranslator->pSS->GetDeviceID(bDevIDBuff, DevIDBuffSize, TRUE);
    ERRCHECK;

    // look for duplex code in bDevIDBuff
	Duplex = DuplexEnabled(bDevIDBuff);

	if(bDuplex)
    {
        err = thePrinter->Send((const BYTE*)EnableDuplex,sizeof(EnableDuplex));
	    ERRCHECK;
    }*/

	err = ConfigureRasterData();
	ERRCHECK;						
					
	err=Graphics();		// start raster graphics and set compression mode

	return err;
}   

BOOL Header900::DuplexEnabled(BYTE* bDevIDBuff)
{
	char* pStrVstatus = NULL;
	char* pStrDuplex = NULL;
	char* pStrSemicolon = NULL;

	if((pStrVstatus = strstr((char*)bDevIDBuff + 2,"VSTATUS:"))) 
		pStrVstatus += 8;
	else
		return FALSE;

	pStrDuplex = pStrVstatus;
	pStrSemicolon = pStrVstatus;

	// now parse VSTATUS parameters to find if we are in simplex or duplex
	if (!(pStrSemicolon = strstr((char*)pStrVstatus,";")))
		return FALSE;

	if ( (pStrDuplex = strstr((char*)pStrVstatus,"DP")) )
		if(pStrDuplex < pStrSemicolon)
			return TRUE;
	if ( (pStrDuplex = strstr((char*)pStrVstatus,"SM")) )
		if(pStrDuplex < pStrSemicolon)
			return FALSE;

	DBG1("didn't find SM or DP!!\n");
	return FALSE;
}



BOOL Broadway::PhotoTrayInstalled(BOOL QueryPrinter)
{
#ifdef PROTO
    return argPhotoTray;
#endif

    DRIVER_ERROR err;
	char* pStrVstatus = NULL;
	char* pStrPhotoTray = NULL;
	char* pStrSemicolon = NULL;

    BYTE bDevIDBuff[DevIDBuffSize];
    
    err=pSS->GetDeviceID(bDevIDBuff, DevIDBuffSize, QueryPrinter);
    if (err!=NO_ERROR)
        return FALSE;

	if((pStrVstatus = strstr((char*)bDevIDBuff + 2,"VSTATUS:"))) 
		pStrVstatus += 8;
	else
		return FALSE;

	pStrPhotoTray = pStrVstatus;
	pStrSemicolon = pStrVstatus;

	// now parse VSTATUS parameters to find if we are in simplex or duplex
	if (!(pStrSemicolon = strstr((char*)pStrVstatus,";")))
		return FALSE;

	if ( (pStrPhotoTray = strstr((char*)pStrVstatus,"PH")) )
		if(pStrPhotoTray < pStrSemicolon)
			return TRUE;
	if ( (pStrPhotoTray = strstr((char*)pStrVstatus,"NR")) )
		if(pStrPhotoTray < pStrSemicolon)
			return FALSE;

	DBG1("didn't find PH or NR!!\n");
	return FALSE;
}

PAPER_SIZE Broadway::MandatoryPaperSize()
{ 
    if (PhotoTrayInstalled(FALSE)) 
        return PHOTO_SIZE; 
    else return UNSUPPORTED_SIZE;   // code for "nothing mandatory"
}


DISPLAY_STATUS Broadway::ParseError(BYTE status_reg)
{
	DBG1("DJ9XX, parsing error info\n");
	
	DRIVER_ERROR err = NO_ERROR;
	BYTE DevIDBuffer[DevIDBuffSize];

	if(IOMode.bDevID)
	{
		// If a bi-di cable was plugged in and everything was OK, let's see if it's still
		// plugged in and everything is OK
		err = pSS->GetDeviceID(DevIDBuffer, DevIDBuffSize, TRUE);
		if(err != NO_ERROR)
			// job was bi-di but now something's messed up, probably cable unplugged
			return DISPLAY_COMM_PROBLEM;
	}

	// check for errors we can detect from the status reg
	if (IOMode.bStatus)
    {
		// Although DJ8XX is OOP, printer continues taking data and BUSY bit gets
		// set.  See IO_defs.h
	    if ( DJ8XX_IS_OOP(status_reg) )
	    {
		    DBG1("Out Of Paper\n");
		    return DISPLAY_OUT_OF_PAPER;
	    }

		// DJ8XX doesn't go offline, so SELECT bit is set even when paper jammed.
		// See IO_defs.h
	    if ( DJ8XX_JAMMED_OR_TRAPPED(status_reg) )
	    {
		    DBG1("Jammed or trapped\n");
		    return DISPLAY_ERROR_TRAP;
	    }
    }
	
	if (IOMode.bDevID)
    {
        if ( TopCoverOpen(status_reg) )
	    {
		    DBG1("Top Cover Open\n");
		    return DISPLAY_TOP_COVER_OPEN;
	    }

		// VerifyPenInfo will handle prompting the user
        // if this is a problem
        err = VerifyPenInfo();

		if(err != NO_ERROR)
			// VerifyPenInfo returned an error, which can only happen when ToDevice
			// or GetDeviceID returns an error. Either way, it's BAD_DEVICE_ID or
			// IO_ERROR, both unrecoverable.  This is probably due to the printer 
			// being turned off during printing, resulting in us not being able to 
			// power it back on in VerifyPenInfo, since the buffer still has a 
			// partial raster in it and we can't send the power-on command.
			return DISPLAY_COMM_PROBLEM;
    }

	// don't know what the problem is-
    //  Is the PrinterAlive?  
	if (pSS->PrinterIsAlive())
    {
		iTotal_SLOW_POLL_Count += iMax_SLOW_POLL_Count;
#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
		printf("iTotal_SLOW_POLL_Count = %d\n",iTotal_SLOW_POLL_Count);
#endif
		// -Note that iTotal_SLOW_POLL_Count is a multiple of 
		//  iMax_SLOW_POLL_Count allowing us to check this
		//  on an absolute time limit - not relative to the number
		//  of times we happen to have entered ParseError.
		// -Also note that we have different thresholds for uni-di & bi-di.
        if( 
            ((IOMode.bDevID == FALSE) && (iTotal_SLOW_POLL_Count >= 60)) ||
            ((IOMode.bDevID == TRUE)  && (iTotal_SLOW_POLL_Count >= 120)) 
          )
			return DISPLAY_BUSY;
		else return DISPLAY_PRINTING;
	}
	else
		return DISPLAY_COMM_PROBLEM;
}

DRIVER_ERROR Broadway::VerifyPenInfo()
{

	DRIVER_ERROR err=NO_ERROR;

	if(IOMode.bDevID == FALSE) 
        return err;

    PEN_TYPE ePen;

	err = ParsePenInfo(ePen);

    if(err == UNSUPPORTED_PEN) // probably Power Off - pens couldn't be read
    {
        DBG1("DJ9xx::Need to do a POWER ON to get penIDs\n");

		// have to delay for Broadway or the POWER ON will be ignored
        if (pSS->BusyWait((DWORD)2000) == JOB_CANCELED)
            return JOB_CANCELED;

		DWORD length=sizeof(Venice_Power_On);
		err = pSS->ToDevice(Venice_Power_On,&length);
        ERRCHECK;

		err = pSS->FlushIO();
        ERRCHECK;

        // give the printer some time to power up
        if (pSS->BusyWait((DWORD)1000) == JOB_CANCELED)
            return JOB_CANCELED;

	    err = ParsePenInfo(ePen);
    }

    ERRCHECK;

	// check for the normal case
	if (ePen == BOTH_PENS)
		return NO_ERROR;

	while ( ePen != BOTH_PENS 	)
	{

		switch (ePen)
		{
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

DRIVER_ERROR Broadway::ParsePenInfo(PEN_TYPE& ePen, BOOL QueryPrinter)
{
	char* str;
    DRIVER_ERROR err = SetPenInfo(str, QueryPrinter);
    ERRCHECK;


	// parse penID
	PEN_TYPE temp_pen1;
	// check pen1, assume it is black, pen2 is color
	switch (str[1])
	{
		case 'H': temp_pen1 = BLACK_PEN; break; // (H)obbes black
        case 'X': return UNSUPPORTED_PEN; 
		default:  temp_pen1 = NO_PEN; break;
	}

	// now check pen2

	int i=2;
	while(str[i]!='$') i++; // handles variable length penIDs
	i++;

	// need to be more forgiving of the color pen type because of
    // the unknown chinookID for broadway
    // we can't guarantee the (F)lash color pen, but we can make sure
    // the pen is not (X)Undefined, (A)Missing or (M)onet
    if(str[i]!='X' && str[i]!='A' && str[i]!='M') 
	// check what pen1 was
	{
		if (temp_pen1 == BLACK_PEN)
				ePen = BOTH_PENS;
		else 
		{
				ePen = COLOR_PEN;
		}
	}
	else // no color pen, just set what pen1 was
		ePen = temp_pen1;

	return NO_ERROR;
}

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
Font* Broadway::RealizeFont(const int index,const BYTE bSize,
						   const TEXTCOLOR eColor,
						   const BOOL bBold,const BOOL bItalic,
						   const BOOL bUnderline)

{ 
    
    return Printer::RealizeFont(index,bSize,eColor,bBold,bItalic,bUnderline);
}
#endif




#endif  // _DJ930 || _DJ950 || _DJ970
