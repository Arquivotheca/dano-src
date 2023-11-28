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
#include "../imaging/open/imaging.h"
#include "../imaging/closed/imaging.h"
#include "../scaling/scaler_open.h"
#include "../scaling/scaler_prop.h"
#else
#include "Header.h"
#include "imaging.h"
#include "scaler_open.h"
#include "scaler_prop.h"
#endif

extern float frac(float f);
extern BOOL Proprietary();

#ifndef PROTO
extern Imager* Create_Imager( SystemServices* pSys, PrintMode* pPM,
                unsigned int iInputWidth, unsigned int iNumRows[], unsigned int HiResFactor );
extern Scaler* Create_Scaler(SystemServices* pSys,int inputwidth,
							 int numerator,int denominator);

#endif

    
#define Bytes(x) ((x/8)+(x%8))
//////////////////////////////////////////////////////////////////////////
// The Job object receives data for a contiguous set of pages targeted at
// a single printer, with a single group of settings encapsulated in the
// PrintContext. 
// At least one page will be ejected for a job, if any data at all 
// is printed (no half-page jobs). If settings are to be changed, this
// must be done between jobs.
//
// The Job constructor is responsible for retrieving or instantiating,
// and aligning, the various components of the system:
//
// 1. the Printer, retrieved from PrintContext
// 2. the color processor (Imager)
// 3. the Translator (with its components GraphicsTranslator and TextTranslator)
// 4. the TextManager
//
// The Job constructor (via SetupTranslator) also calls Translator::SendHeader 
// to put the printer into the desired PCL state.

Job::Job(PrintContext* pPC)
	: thePrintContext(pPC), pSS(pPC->pSS),        
	  thePipeline(NULL), pText(NULL), pSender(NULL), pImager(NULL), 
      pResSynth(NULL), pReplicator(NULL), pErnie(NULL),
      CurrentMode(pPC->CurrentMode), skipcount(0), RowsInput(0), 
      BlankRaster(NULL), DataSent(FALSE)
#ifdef USAGE_LOG
	, UText(0), UTextCount(0)
#endif
{ 
#ifdef CAPTURE
	Capture_Job(pPC);
#endif
    unsigned int i;
    constructor_error = NO_ERROR;

    if (!thePrintContext->PrinterSelected())
      {
        constructor_error = NO_PRINTER_SELECTED; 
        return;
      }
    thePrinter = thePrintContext->thePrinter;

    pHead = thePrinter->SelectHeader(thePrintContext);
    CNEWCHECK(pHead);

    // set ResBoost, numrows, RowMultiple ///////////////////////////
    InputDPI = thePrintContext->InputResolution();

    for (i=0; i<MAXCOLORPLANES; i++)
        if (CurrentMode->MixedRes)
            numrows[i] =  CurrentMode->ResolutionY[i] / InputDPI;
        else numrows[i]=1;

    ResBoost = CurrentMode->BaseResX / InputDPI;
    if (ResBoost==0) ResBoost=1;                    // safety
    RowMultiple = CurrentMode->BaseResY / InputDPI;
    if (RowMultiple==0) RowMultiple=1;              // safety
  
    // set up blank raster used by SendRasters
    constructor_error=setblankraster();
    CERRCHECK;

     // flush any garbage out of the printer's buffer
    constructor_error=thePrinter->Flush();
    CERRCHECK;
    // send the PCL header
    constructor_error=pHead->Send();
    CERRCHECK;
    // set CAPy after sending header in case header used it
	CAPy=pHead->CAPy;


	constructor_error=Configure();
    CERRCHECK;

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

	pText = new TextTranslator(thePrinter, pHead->QualityCode(),
                               thePrintContext->CurrentMode->dyeCount);
    CNEWCHECK(pText);

    theTextManager = NULL;
    if (CurrentMode->bFontCapable)
    {
        unsigned int pixelsX = (unsigned int)(thePrintContext->printablewidth() * 
                                CurrentMode->BaseResX);
        unsigned int pixelsY = (unsigned int)(thePrintContext->printableheight() * 
                                CurrentMode->BaseResY);

	    theTextManager=new TextManager( pText, pixelsX,pixelsY );
	    CNEWCHECK(theTextManager);
	    constructor_error = theTextManager->constructor_error;
	    CERRCHECK;
    }
#endif

    if (!thePrintContext->ModeAgreesWithHardware(TRUE))
        constructor_error = WARN_MODE_MISMATCH;	

#ifdef USAGE_LOG
	UHeader[0]='\0';
#endif
}

DRIVER_ERROR Job::SetupColor()
// Subroutine of Job constructor to handle Imager initialization.
// OutputWidth set in InitScaler
{  

#ifdef PROTO
    if (!Proprietary())
        pImager = new Imager_Open(thePrintContext->pSS, CurrentMode,
                                  OutputWidth, numrows,ResBoost);
    else
        pImager = new Imager_Prop(thePrintContext->pSS, CurrentMode,
                                  OutputWidth, numrows,ResBoost);
#else
    pImager = Create_Imager(thePrintContext->pSS, CurrentMode,
                                  OutputWidth, numrows,ResBoost);
#endif


	NEWCHECK(pImager);
	return pImager->constructor_error;

}


Job::~Job()
{ 
#ifdef CAPTURE
	Capture_dJob();
#endif
	
	// Client isn't required to call NewPage at end of last page, so
	// we may need to eject a page now.
	if (DataSent)
		newpage();


	// Tell printer that job is over.
	pHead->EndJob();
		  
// Delete the 4 components created in Job constructor.
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
DBG1("deleting TextManager\n");
	if (theTextManager)
		delete theTextManager;
#endif

DBG1("deleting RasterSender\n");
	if (pSender)
		delete pSender;
DBG1("deleting umpqua\n");
	if (pImager)
		delete pImager;

    if (BlankRaster)
        pSS->FreeMemory(BlankRaster);

    if (pReplicator)
        delete pReplicator;
    if (pResSynth)
        delete pResSynth;

    if (thePipeline)
        delete thePipeline;
    if (pHead)
        delete pHead;
    if (pErnie)
        delete pErnie;
    if (theCompressor)
        delete theCompressor;
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
    if (pText)
        delete pText;
#endif // defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

DBG1("done with ~Job\n");

}

DRIVER_ERROR Job::SendCAPy() 
{ 
    return pHead->SendCAPy(CAPy++); 
}
///////////////////////////////////////////////////////////////////////////////////
//
DRIVER_ERROR Job::SendRasters(BYTE* ImageData)		// RGB24 data for one raster
// This is how graphical data is sent to the driver.

// We do not check for rasters where data is supplied but happens 
// to be all blank (white); caller should have used NULL in this case.
{
#ifdef CAPTURE
	Capture_SendRasters(ImageData);
#endif

 return sendrasters(ImageData);
}

// internal entry point, used by newpage
DRIVER_ERROR Job::sendrasters(BYTE* ImageData)
{
	DRIVER_ERROR err=NO_ERROR;
 
    if (thePipeline==NULL)
        return SYSTEM_ERROR;

    static float fcount=0.0;
    static unsigned int icount=0;

    // need to put out some data occasionally in case of all-text page,
    // so printer will go ahead and start printing the text before end of page
    if (ImageData==NULL)
    {
        skipcount++;
        if (skipcount >= 200)
        {
            skipcount=0;     
            ImageData=BlankRaster;
        }
        err=thePipeline->Flush();
        ERRCHECK;
    }
    else 
    {
        skipcount=0;
        DataSent=TRUE;		// so we know we need a formfeed when job ends
        fcount=0.0;
        icount=0;
    }

    RowsInput++;        // needed in case res > 300

    for (unsigned int k=0; k < RowMultiple; k++)     // for cases where res=RowMultiple*300

       if (ImageData)
           err=thePipeline->Execute(ImageData,thePrintContext->InputWidth*3);
       else 
       {           
           fcount += ScaleFactor;
           
           int loop = (unsigned int)fcount - icount;

           for (int i=0; i < loop; i++)
               SendCAPy();

           icount += loop;
       }

 return err;
}

DRIVER_ERROR Job::newpage()
// (for internal use, called by external NewPage)
{
    sendrasters();     // flush pipeline


	DRIVER_ERROR err = pHead->FormFeed();
	ERRCHECK;

    if (pImager)
// re-init the Imager buffer
	    pImager->Restart();	

// reset vertical cursor counter
    if (thePrinter->UseGUIMode(thePrintContext->CurrentPrintMode()) )
// Venice in GUImode doesn't accept top-margin setting, so we use CAP for topmargin
      { 
        unsigned int ResMultiple = (InputDPI / 300);
        // see Header895::StartSend() for computation to get 88
        CAPy = 88 / ResMultiple;
        err=pHead->SendCAPy( CAPy );
        ERRCHECK;
      }
	else CAPy=0;

    skipcount = RowsInput = 0;

// reset flag used to see if formfeed needed
	DataSent=FALSE;

    if (!thePrintContext->ModeAgreesWithHardware(TRUE))
        return WARN_MODE_MISMATCH;

#ifdef USAGE_LOG
//	theTranslator->PrintDotCount(UHeader);
//	theTranslator->NextPage();
	UTextCount=UText=0;
#endif

	return NO_ERROR;
}

DRIVER_ERROR Job::NewPage()
// External entry point
{
#ifdef CAPTURE
	Capture_NewPage();
#endif
	return newpage();
}

DRIVER_ERROR Job::InitScaler()
// sets pResSynth & pReplicator
{
    unsigned int numerator, denominator;
	OutputWidth = thePrintContext->OutputWidth;
	unsigned int InputWidth = thePrintContext->InputWidth;

    if ((OutputWidth % InputWidth)==0)
    {
        numerator = OutputWidth / InputWidth;
        denominator = 1;
    }
    else
    {
        numerator = OutputWidth;
        denominator = InputWidth;
    }

    // ResBoost is for horizontal, RowMultiple for vertical
    // if they are the same, we can just up the scalefactor
    if ((ResBoost==RowMultiple) && ResBoost>1)
    {
        numerator *= ResBoost;        
        OutputWidth *= ResBoost;
        ResBoost=RowMultiple=1;
        thePrintContext->setpixelsperrow(InputWidth, OutputWidth);
        DRIVER_ERROR err = setblankraster();
        ERRCHECK;
    }

    ScaleFactor = (float)numerator / (float)denominator;

// Two paths: if ResSynth included (proprietary path), then use it for the first doubling;
//            otherwise do it all in PixelReplication phase
//  but don't use ResSynth anyway if printer-res=600 and scale<4 (i.e. original-res<150), 
//  or printer-res=300 and scale<2.33 (i.e. original-res<133)
    BOOL RSok;
    if (thePrintContext->EffectiveResolutionX()>300)
        // I don't know about 1200dpi, so I'm doing it this way
        RSok = ScaleFactor >= 4.0f;
    else RSok = ScaleFactor >= 2.33f;

    if ((!Proprietary())  || !RSok )
    {
        pResSynth=NULL;
        pReplicator = new Scaler_Open(pSS,InputWidth,numerator,denominator);
        NEWCHECK(pReplicator);
        return pReplicator->constructor_error;
    }

// Proprietary path and scalefactor>=2, so break it up into 2 parts
// first give ResSynth a factor of 2, which is all it really does anyway.
#ifdef PROTO
    pResSynth = new Scaler_Prop(pSS,InputWidth,2,1);   
#else
    pResSynth = Create_Scaler(pSS,InputWidth,2,1); 
#endif
	NEWCHECK(pResSynth);
    if (pResSynth->constructor_error != NO_ERROR)
        return pResSynth->constructor_error;

// now replicate the rest of the way -- only half as much left
    pReplicator = new Scaler_Open(pSS,2*InputWidth,numerator,2*denominator);   
	NEWCHECK(pReplicator);
    return pReplicator->constructor_error;

}

DRIVER_ERROR Job::Configure()
// mode has been set -- now set up rasterwidths and pipeline
{  
   DRIVER_ERROR err;
   Pipeline* p=NULL;
   unsigned int width;
   BOOL useRS=FALSE;

   err = InitScaler();		// create pReplicator and maybe pResSynth
   ERRCHECK;

   if ((CurrentMode->Config.bResSynth) && Proprietary() 
       && pResSynth)        // pResSynth==NULL if no scaling required
   {
       p = new Pipeline(pResSynth);
       NEWCHECK(p);
       if (thePipeline)
          thePipeline->AddPhase(p);
       else thePipeline=p;
       useRS=TRUE;
   }

#ifdef _DJ9xxVIP
   if (CurrentMode->Config.bErnie)
   {
       // create Ernie (need pixelwidth for constructor)
       if (p)
           width = p->GetOutputWidth() / 3;     // GetOutputWidth returns # of bytes
       else width = thePrintContext->InputWidth;

// calculate Ernie threshold value
//Normal: threshold = (resolution) * (0.0876) - 2
// roughly: image at original 300 implies threshold=24; 600=>48, 150=>12, 75=>6
// to get resolution of "original" image, divide target resolution by scalefactor
       float scale = (float)thePrintContext->OutputWidth / (float)thePrintContext->InputWidth;       
       float original_res = ((float)thePrintContext->EffectiveResolutionX()) / scale;
       if (useRS && (scale >= 2.0f))
           // image already doubled by ResSynth so consider the resolution as of now
            original_res *= 2.0f;
       float fthreshold = original_res * 0.0876f;
       int threshold = (int)fthreshold - 2;
 
       pErnie = new TErnieFilter(width, eBGRPixelData, threshold);
       p = new Pipeline(pErnie);
       NEWCHECK(p);
       if (thePipeline)
          thePipeline->AddPhase(p);
       else thePipeline=p;
   }
#endif

   if (CurrentMode->Config.bPixelReplicate)
   {
       // create Replicator
       p = new Pipeline(pReplicator);
       NEWCHECK(p);
       if (thePipeline)
          thePipeline->AddPhase(p);
       else thePipeline=p;
   }
  	
   if (CurrentMode->Config.bColorImage)
   {
	   err = SetupColor();		// create pImager pointee
	   ERRCHECK;
       p = new Pipeline(pImager);
       NEWCHECK(p);
       if (thePipeline)
          thePipeline->AddPhase(p);
       else thePipeline=p;
   }

   if (CurrentMode->Config.bCompress)
   {
      if (p)
         width = p->GetMaxOutputWidth();
      else width = thePrintContext->InputWidth;
      unsigned int SeedBufferSize;
      if (pImager)      // if data is halftoned-output
                        // format is 1 bit-per-pixel for each ink,drop,pass
          SeedBufferSize = MAXCOLORPLANES * MAXCOLORDEPTH * MAXCOLORROWS * width;
      else              // VIP data is just RGB24 here
          SeedBufferSize = width;

	  theCompressor = thePrinter->CreateCompressor(SeedBufferSize);
      NEWCHECK(theCompressor);
      err=theCompressor->constructor_error;
	  ERRCHECK;

       p = new Pipeline(theCompressor);
       NEWCHECK(p);
       if (thePipeline)
          thePipeline->AddPhase(p);
       else thePipeline=p;
   }
    NEWCHECK(p);	


   // always end pipeline with RasterSender
   // create RasterSender object

   pSender = new RasterSender(thePrinter,thePrintContext,this,pImager);
   NEWCHECK(pSender);
   err=pSender->constructor_error;
   ERRCHECK;

   p = new Pipeline(pSender);

   if (thePipeline)
	  thePipeline->AddPhase(p);
   else thePipeline=p;

  return NO_ERROR;
}
    
DRIVER_ERROR Job::setblankraster()
{
    if (BlankRaster)
        pSS->FreeMemory(BlankRaster);

    BlankRaster = (BYTE*)pSS->AllocMem(thePrintContext->OutputWidth*3);
    NEWCHECK(BlankRaster);

    for (unsigned int i=0; i < thePrintContext->OutputWidth*3; i++)
        BlankRaster[i]=0xFF;

  return NO_ERROR;
}
//////////////////////////////////////////////////////////////////////
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
DRIVER_ERROR Job::TextOut(const char* pTextString, unsigned int iLenString, 
				  const Font& font, int iAbsX, int iAbsY ) 
// This is how ASCII data is sent to the driver.
// Everything is really handled by the TextManager, including error checking.
{
#ifdef CAPTURE
	Capture_TextOut(pTextString, iLenString, font, iAbsX, iAbsY);
#endif


	DRIVER_ERROR err=theTextManager->TextOut(pTextString,iLenString,font,iAbsX,iAbsY);
	ERRCHECK;

	DataSent=TRUE;

#ifdef USAGE_LOG
	if (iLenString > UTextSize)
	  {	iLenString = UTextSize-1;
		UHeader[iLenString]='\0';
	  }
	if (UTextCount<2)
	  {
		strcpy(UHeader,pTextString);
		UText += iLenString;
	  }
	if (UTextCount==1)
		UHeader[UText]='\0';
	UTextCount++;
	
#endif

	return err;
}
#endif	
///////////////////////////////////////////////////////////
// Pipeline management

Pipeline::Pipeline(Processor* E)
    : next(NULL), prev(NULL)
{
    Exec=E;
    Exec->myphase = this;
}

void Pipeline::AddPhase(Pipeline* newp)
{ 
    Pipeline* p = this;
    while (p->next)
        p=p->next;
    p->next = newp;
    newp->prev = p;
}

Pipeline::~Pipeline()
{ 
    if (next)
        delete next;
}

BOOL Pipeline::Process(BYTE* raster, unsigned int size)
{
    return Exec->Process(raster,size);
}

DRIVER_ERROR Pipeline::Execute(BYTE* InputRaster, unsigned int size)
{
    BYTE* raster;
    err=NO_ERROR;

    if (Process(InputRaster,size)        // true if output ready; may set err
        && (err==NO_ERROR))         
      if (next)
        while ( (raster=NextOutputRaster()) )
        {
            err=next->Execute(raster,GetOutputWidth());
            ERRCHECK;
        }
  return err;  
}

DRIVER_ERROR Pipeline::Flush()
{
    BYTE* raster;
    err=NO_ERROR;

    Exec->Flush();

    if (next && (err==NO_ERROR))
    {
      while ( (raster=NextOutputRaster()) )
      {
        err=next->Execute(raster,GetOutputWidth());
        ERRCHECK;
      }
    // one more to continue flushing downstream
      err=next->Flush();                         
    }

  return err;  
}


Processor::Processor()
: iRastersReady(0), iRastersDelivered(0), myphase(NULL)
{ }

Processor::~Processor()
{ }
