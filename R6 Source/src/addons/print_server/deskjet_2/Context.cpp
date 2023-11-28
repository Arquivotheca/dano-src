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

// PrintContext
//
// The PrintContext is the second item created in the
// driver calling sequence, following SystemServices.
// It provides the interface for inquiring about
// supported devices, and for setting all optional
// properties of the imaging pipeline as determined by
// given devices.

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif


#ifdef PROTO
	extern int argColor;
    extern int argProprietary;
#endif

extern PAPER_SIZE MediaSizeToPaper(MediaSize msize);
extern MediaSize PaperToMediaSize(PAPER_SIZE psize);

////////////////////////////////////////////////////////////////////
// PrintContext constructor
// In the normal case where bidirectional communication
// was established by SystemServices, successful
// construction results in instantiation of the proper
// Printer class; otherwise client will have to complete
// the process with a subsequent call to SelectDevice.
// The last two parameters are optional. In the case where
// InputPixelsPerRow has the default setting of zero,
// the value of printablewith will be used.
//
PrintContext::PrintContext( SystemServices * pSysServ, 
                            unsigned int InputPixelsPerRow,
                            unsigned int OutputPixelsPerRow,                                                         
                            PAPER_SIZE ps)
    : pSS(pSysServ), thePrinter((Printer*)NULL), CurrentMode((PrintMode*)NULL),
      CurrentModeIndex(DEFAULTMODE_INDEX), InputRes(300),
      InputWidth(InputPixelsPerRow), OutputWidth(OutputPixelsPerRow), 
      thePaperSize(ps)
	     
{
#ifdef CAPTURE
    Capture_PrintContext(InputPixelsPerRow,OutputPixelsPerRow,ps,pSS->IOMode);
#endif

    DR = pSS->DR;

    InitPSMetrics();    // create array of paper sizes 

    if (!pSS->IOMode.bDevID)     // SystemServices couldn't establish good DevID
    {
        constructor_error = setpixelsperrow(InputWidth,OutputWidth); 
 
        return;             // leave in incomplete state
    }
    
    if ( (constructor_error=DR->SelectDevice(pSS->strModel,&(pSS->VIPVersion),pSS->strPens,pSS)) != NO_ERROR)
	  {
        if (constructor_error == UNSUPPORTED_PRINTER)
          {
			pSS->DisplayPrinterStatus(DISPLAY_PRINTER_NOT_SUPPORTED);
            //wait to read message
			while (pSS->BusyWait(500) != JOB_CANCELED) ;   // nothing.....
            return;
          }
        else { DBG1("PrintContext - error in SelectDevice\n"); return; }
	  }

    // Device selected... now instantiate a printer object 
	if ( (constructor_error = DR->InstantiatePrinter(thePrinter,pSS)) != NO_ERROR) 
	  { DBG1("PrintContext - error in InstantiatePrinter\n"); return; }

    thePrinter->SetHelpType(pSS->strModel);

	pSS->AdjustIO(thePrinter->IOMode);

    PAPER_SIZE p = thePrinter->MandatoryPaperSize();
    if (p != UNSUPPORTED_SIZE)
        thePaperSize = p;

    if (SelectDefaultMode())
        constructor_error = setpixelsperrow(InputWidth,OutputWidth);
    else constructor_error = SYSTEM_ERROR;
}



DRIVER_ERROR PrintContext::SetMode(unsigned int ModeIndex) 
{ 
    if (ModeIndex>=GetModeCount())
        return INDEX_OUT_OF_RANGE;

    CurrentModeIndex=ModeIndex; 
    CurrentMode = thePrinter->GetMode(ModeIndex); 
    if (CurrentMode==NULL)
        return SYSTEM_ERROR;
 return NO_ERROR;
}

BOOL PrintContext::SelectDefaultMode()
// return TRUE if mode set okay
{
    BOOL modeOK[MAX_PRINTMODES];
    unsigned int maxmodes = GetModeCount();

    unsigned int i;
    // loop through entire modeOK array to make sure all of it gets initialized, even
    // the entries beyond maxmodes since may access those indexes below
    for (i=0; i < MAX_PRINTMODES; i++)
    {   
        modeOK[i]=FALSE;
        if (i < maxmodes)
        {
            if (SetMode(i) == NO_ERROR)
            {
                if ((ModeAgreesWithHardware(FALSE)) && 
	                (setpixelsperrow(InputWidth,OutputWidth)==NO_ERROR))
                    modeOK[i]=TRUE;
            }
        }
    }

    //
    // now look to find a compatible mode to use as default
    //

    // first preference is the default mode (if compatible)
    if (modeOK[DEFAULTMODE_INDEX])
    {
        SetMode(DEFAULTMODE_INDEX);
        return TRUE;
    }

    // next preference is a special mode that is compatible
    for (i=SPECIALMODE_INDEX; i<MAX_PRINTMODES; i++)
    {
        if (modeOK[i])
        {
            SetMode(i);
            return TRUE;
        }
    }

    // last preference is grayscale
    if (modeOK[GRAYMODE_INDEX])
    {
        SetMode(GRAYMODE_INDEX);
        return TRUE;
    }

    return FALSE;
}

char* PrintContext::GetModeName()
{
  if (CurrentMode==NULL)
    return (char*)NULL;
  return CurrentMode->ModeName;
}


BOOL PrintContext::ModeAgreesWithHardware(BOOL QueryPrinter)
// no check for null printer
{
    BOOL agree=FALSE;
    PEN_TYPE ePen;

    if (pSS->IOMode.bDevID==FALSE)
        return TRUE;
 
    if (thePrinter->ParsePenInfo(ePen,QueryPrinter) != NO_ERROR)
        return FALSE;

    for (int i=0; i < MAX_COMPATIBLE_PENS; i++)
        if (ePen == CurrentMode->CompatiblePens[i])
            agree = TRUE;

    return agree;
}

DRIVER_ERROR PrintContext::SetInputResolution(unsigned int Res)
{
#ifdef CAPTURE
Capture_SetInputResolution(Res);
#endif
    if ((Res!=300) && (Res!=600))
        return ILLEGAL_RESOLUTION;

    if (CurrentMode)
        if ((Res > CurrentMode->BaseResX) || (Res > CurrentMode->BaseResY))
            return ILLEGAL_RESOLUTION;

    InputRes=Res;
  
  return NO_ERROR;
}

void PrintContext::InitPSMetrics()
{
// this array is directly linked to PAPER_SIZE enum
// note: fPrintablePageY is related to fPrintableStartY to allow for a 2/3" bottom margin.
//  If fPrintableStartY is altered, fPrintablePageY should be correspondingly updated.

PSM[LETTER].fPhysicalPageX = (float)8.5;
PSM[LETTER].fPhysicalPageY = (float)11.0;
PSM[LETTER].fPrintablePageX = (float)8.0;
PSM[LETTER].fPrintablePageY = (float)10.0;
PSM[LETTER].fPrintableStartY = (float)0.3333;

PSM[A4].fPhysicalPageX = (float)8.27;
PSM[A4].fPhysicalPageY = (float)11.69;
PSM[A4].fPrintablePageX = (float)8.0;
PSM[A4].fPrintablePageY = (float)10.7;
PSM[A4].fPrintableStartY = (float)0.3333;

PSM[LEGAL].fPhysicalPageX = (float)8.5;
PSM[LEGAL].fPhysicalPageY = (float)14.0;
PSM[LEGAL].fPrintablePageX = (float)8.0;
PSM[LEGAL].fPrintablePageY = (float)13.0;
PSM[LEGAL].fPrintableStartY = (float)0.3333;

// Corresponds to 4x6" photo paper used in the 9xx series photo tray.
// The apparent 1/8" bottom margin is allowed because of a pull-off tab on the media.
PSM[PHOTO_SIZE].fPhysicalPageX = (float)4.0;
PSM[PHOTO_SIZE].fPhysicalPageY = (float)6.0;
PSM[PHOTO_SIZE].fPrintablePageX = (float)3.75;
PSM[PHOTO_SIZE].fPrintablePageY = (float)5.75;
PSM[PHOTO_SIZE].fPrintableStartY = (float)0.125;
}
   

PrintContext::~PrintContext()
{ 
#ifdef CAPTURE
Capture_dPrintContext();
#endif
DBG1("deleting PrintContext\n");

    if (thePrinter)
        delete thePrinter;

}
///////////////////////////////////////////////////////////////////////
// Functions to report on device-dependent properties.
// Note that mixed-case versions of function names are used
// for the client API; lower-case versions are for calls
// made by the driver itself, to avoid the CAPTURE instrumentation.
///////////////////////////////////////////////////////////////////////
float PrintContext::PhysicalPageSizeX()   // returned in inches
{ 
	if (thePrinter==NULL)
		return 0.0;
	return PSM[ thePaperSize ].fPhysicalPageX; 
}

float PrintContext::PhysicalPageSizeY()   // returned in inches
{ 
	if (thePrinter==NULL)
		return 0.0;
	return PSM[ thePaperSize ].fPhysicalPageY; 
}

float PrintContext::PrintableWidth()	// returned in inches
// for external use
{ 
	return printablewidth();
}

/////////////////////////////////////////////////////////////////////
// NOTE ON RESOLUTIONS: These functions access ResolutionX[C],
//  where C is the conventional index for Cyan. The assumption 
//  is that Res[C]=Res[M]=Res[Y], AND that Res[K]>=Res[C]
/////////////////////////////////////////////////////////////////////

float PrintContext::printablewidth()	
// for internal use
{ 
	if (thePrinter==NULL)
		return 0.0;
	return PSM[ thePaperSize ].fPrintablePageX;  
}

float PrintContext::PrintableHeight()     // returned in inches
// for external use
{ 
	return printableheight();
}

float PrintContext::printableheight()	
// for internal use
{ 
	if (thePrinter==NULL)
		return 0.0;
	return PSM[ thePaperSize ].fPrintablePageY; 
}


float PrintContext::PrintableStartX() // returned in inches
{ 
	if (thePrinter==NULL)
		return 0;
    // this return value centers the printable page horizontally on the physical page
    float physwidth =  PSM[ thePaperSize ].fPhysicalPageX;
    float printable =  PSM[ thePaperSize ].fPrintablePageX;
	return ((physwidth - printable) / (float)2.0 ); 
}

float PrintContext::PrintableStartY() // returned in inches
{ 
	if (thePrinter==NULL)
		return 0;
	return PSM[ thePaperSize ].fPrintableStartY;
}


unsigned int PrintContext::printerunitsY()
// internal version
{ 
	if (thePrinter==NULL)
		return 0;
	return CurrentMode->ResolutionY[C]; 
}

///////////////////////////////////////////////////////////////////////////////////////

DRIVER_ERROR PrintContext::PerformPrinterFunction(PRINTER_FUNC eFunc)
{

	if (thePrinter==NULL)
        return NO_PRINTER_SELECTED;

	if (eFunc==CLEAN_PEN)
	{
		thePrinter->Flush();
	    return thePrinter->CleanPen();
	}
		
	DBG1("PerformPrinterFunction: Unknown function\n");
	return UNSUPPORTED_FUNCTION;
				
}


	
PRINTER_TYPE PrintContext::EnumDevices(unsigned int& currIdx) const 
{ return DR->EnumDevices(currIdx); }



BOOL PrintContext::PrinterFontsAvailable(unsigned int PrintModeIndex)
{ 
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
    PrintMode* pPM;
	if (thePrinter==NULL) 
        return SYSTEM_ERROR;	// called too soon
    pPM = thePrinter->GetMode(PrintModeIndex);
	return pPM->bFontCapable;
#else
    return FALSE;
#endif
}


DRIVER_ERROR PrintContext::SelectDevice(const PRINTER_TYPE Model)
// used by client when SystemServices couldn't get DevID
// this is the place where printer gets instantiated for unidi
{
#ifdef CAPTURE
Capture_SelectDevice(Model);
#endif
    DRIVER_ERROR err;

    if (thePrinter)     // if printer exists due to bidi or previous call
        delete thePrinter;

    err = DR->SelectDevice(Model);
    ERRCHECK;
   
    if ( (err = DR->InstantiatePrinter(thePrinter,pSS)) != NO_ERROR) 
	  { DBG1("PrintContext - error in InstantiatePrinter\n"); return err; }

    const char* model;
    if (strlen(pSS->strModel) > 0)  // if bidi so strModel got set in SS
        model = pSS->strModel;
    else model = PrintertypeToString(Model);    // at least give something

    thePrinter->SetHelpType(model);

	pSS->AdjustIO(thePrinter->IOMode);

    PAPER_SIZE p = thePrinter->MandatoryPaperSize();
    if (p != UNSUPPORTED_SIZE)
        thePaperSize = p;

    if (SelectDefaultMode())
        err = setpixelsperrow(InputWidth,OutputWidth);
    else err = WARN_MODE_MISMATCH;


 return err;
}

///////////////////////////////////////////////////////////////////////////
DRIVER_ERROR PrintContext::setpixelsperrow(unsigned int InputPixelsPerRow,
                                           unsigned int OutputPixelsPerRow)
// internal version without printer check
{
    unsigned int baseres;
    float printwidth;

	if (CurrentMode==NULL)
        baseres = 300;
    else baseres = CurrentMode->BaseResX;
    if (thePrinter==NULL)
        printwidth = 8.0;
    else printwidth = printablewidth();

//    PageWidth = (unsigned int)((float)baseres * printwidth);
	
	if ((InputPixelsPerRow==0) && (OutputPixelsPerRow==0))
       InputPixelsPerRow=PageWidth;             // by convention
  
    if (OutputPixelsPerRow==0)            
        OutputPixelsPerRow=PageWidth;           // by convention

//    if (OutputPixelsPerRow > PageWidth)
//       return ILLEGAL_COORDS;  
 
	PageWidth = OutputPixelsPerRow;

    if (InputPixelsPerRow>OutputPixelsPerRow)
        return ILLEGAL_COORDS;      // no downscaling

    // new value is legal
    InputWidth = InputPixelsPerRow;
    OutputWidth = OutputPixelsPerRow;
return NO_ERROR;
}

DRIVER_ERROR PrintContext::SetPixelsPerRow(unsigned int InputPixelsPerRow,
                                           unsigned int OutputPixelsPerRow)
// set new in/out width
// assumes PageWidth is already valid
{
#ifdef CAPTURE
Capture_SetPixelsPerRow(InputPixelsPerRow, OutputPixelsPerRow);
#endif
    if (thePrinter==NULL)
        return NO_PRINTER_SELECTED;

   return setpixelsperrow(InputPixelsPerRow, OutputPixelsPerRow);
}

unsigned int PrintContext::EffectiveResolutionX()
{
    if (CurrentMode==NULL)
        return 0;

    return CurrentMode->BaseResX;
}

unsigned int PrintContext::EffectiveResolutionY()
{
    if (CurrentMode==NULL)
        return 0;

    return CurrentMode->BaseResY;
}

unsigned int PrintContext::GetModeCount()
{
  if (thePrinter==NULL)
      return 0;

  return thePrinter->GetModeCount();
}

DRIVER_ERROR PrintContext::SelectPrintMode(const unsigned int index)
{
#ifdef CAPTURE
Capture_SelectPrintMode(index);
#endif

    if (thePrinter==NULL)
        return NO_PRINTER_SELECTED;

    unsigned int count = GetModeCount();
    if (index > (count-1))
        return INDEX_OUT_OF_RANGE;

    CurrentMode= thePrinter->GetMode(index);
    CurrentModeIndex = index;

    if ((InputRes > CurrentMode->BaseResX) || (InputRes > CurrentMode->BaseResY))
            return ILLEGAL_RESOLUTION;

    if (!ModeAgreesWithHardware(FALSE))
        return WARN_MODE_MISMATCH;
    // notice that requested mode is set even if it is wrong for the pen

    return setpixelsperrow(InputWidth,OutputWidth);
 
}

DRIVER_ERROR PrintContext::SetPaperSize(PAPER_SIZE ps)
{ 
#ifdef CAPTURE
Capture_SetPaperSize(ps);
#endif
    if (thePrinter==NULL)
        return NO_PRINTER_SELECTED;

    if ((thePrinter->MandatoryPaperSize() != UNSUPPORTED_SIZE) && 
        (ps != thePaperSize))
        return ILLEGAL_PAPERSIZE;
    thePaperSize = ps;

return setpixelsperrow(InputWidth,OutputWidth);
}

PAPER_SIZE PrintContext::GetPaperSize() 
{ 
    return thePaperSize; 
}
    
PRINTER_TYPE PrintContext::SelectedDevice() 
{ 
    if (thePrinter==NULL) 
        return UNSUPPORTED; 
    return (PRINTER_TYPE)DR->device; 
}
     
const char* PrintContext::PrinterModel()
{
    if ((pSS==NULL) || (thePrinter==NULL))
        return (const char*)NULL;

    return pSS->strModel;
}

const char* PrintContext::PrintertypeToString(PRINTER_TYPE pt)
{
    return ModelName[pt];
}

HELP_TYPE PrintContext::GetHelpType()
{ 
    if (thePrinter==NULL) 
        return HELP_UNSUPPORTED;
    else return  thePrinter->help; 
}
//////////////////////////////////////////////////////////////
DRIVER_ERROR PrintContext::SendPrinterReadyData(BYTE* stream, unsigned int size)
{
    if (stream==NULL)
        return NULL_POINTER;

    if (thePrinter==NULL)
        return NO_PRINTER_SELECTED;

    return thePrinter->Send(stream,size);
}

void PrintContext::Flush(int FlushSize) { if(thePrinter != NULL) thePrinter->Flush(FlushSize); }
