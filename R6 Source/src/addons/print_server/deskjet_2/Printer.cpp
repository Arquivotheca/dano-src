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
#include "../include/IO_defs.h"
#include "../imaging/resources.h"
#else
#include "Header.h"
#include "IO_defs.h"
#include "resources.h"
#endif



//
// ** Printer CLASS **
    
#ifndef PROTO
extern BYTE* GetHTBinary();
#else
extern int argProprietary;
#endif

Printer::Printer(SystemServices* pSys,
                 int numfonts, BOOL proto)
	:  constructor_error(NO_ERROR), IOMode(pSys->IOMode), 
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS) 
iNumFonts(numfonts),
#else
iNumFonts(0),  
#endif	
    bCheckForCancelButton(FALSE), ulBytesSentSinceCancelCheck(0), pSS(pSys),
    InSlowPollMode(0), iTotal_SLOW_POLL_Count(0), 
    iMax_SLOW_POLL_Count(DEFAULT_SLOW_POLL_COUNT),
    ErrorTerminationState(FALSE),  
    ModeCount(2)
{ 	
	int i=0; // counter

    for (i=0; i < MAX_PRINTMODES; i++)
        pMode[i]=NULL;

    if (IOMode.bDevID)
       iMax_SLOW_POLL_Count = DEFAULT_SLOW_POLL_BIDI;

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)         
    // create dummy font objects to be queried via EnumFont
    // these fonts used by all except DJ400

    for (i=0; i<=MAX_PRINTER_FONTS; i++)
        fontarray[i] = NULL;

#ifdef _COURIER
    fontarray[COURIER_INDEX] = new Courier();
	CNEWCHECK(fontarray[COURIER_INDEX]);
#endif
#ifdef _CGTIMES
	fontarray[CGTIMES_INDEX] = new CGTimes();
	CNEWCHECK(fontarray[CGTIMES_INDEX]);
#endif
#ifdef _LTRGOTHIC
	fontarray[LETTERGOTHIC_INDEX] = new LetterGothic();
	CNEWCHECK(fontarray[LETTERGOTHIC_INDEX]);
#endif
#ifdef _UNIVERS
	fontarray[UNIVERS_INDEX] = new Univers();
	CNEWCHECK(fontarray[UNIVERS_INDEX]);
#endif

#endif  // 
}


Printer::~Printer()
{ 
    for (int i=0; i < MAX_PRINTMODES; i++)
        if (pMode[i])
            delete pMode[i];


    if (ErrorTerminationState) pSS->AbortIO();

#ifdef _COURIER
    delete fontarray[COURIER_INDEX];
#endif
#ifdef _CGTIMES
    delete fontarray[CGTIMES_INDEX];
#endif
#ifdef _LTRGOTHIC
    delete fontarray[LETTERGOTHIC_INDEX];
#endif
#ifdef _UNIVERS
    delete fontarray[UNIVERS_INDEX];
#endif

}

////////////////////////////////////////////////////////////////////////////
Compressor* Printer::CreateCompressor(unsigned int RasterSize)
{
	return new Mode9(pSS,RasterSize);   // most printers use mode 9
}

////////////////////////////////////////////////////////////////////////////
// ** API functions
//


DRIVER_ERROR Printer::Flush(int FlushSize) // default = MAX_RASTERSIZE
// flush possible leftover garbage -- 
// default call will send one (maximal) raster's worth of zeroes
{	
	DRIVER_ERROR err=NO_ERROR;
#define chunksize 1000
	BYTE zero[chunksize];

	memset(zero, 0, chunksize);
	int chunks = (FlushSize / chunksize) + 1;
	for (int i=0; i < chunks; i++)
    {
		if ((err = Send( zero, chunksize)) != NO_ERROR)
			return err;
    }
    return err;
}


/*
 * 	Function name: ParseError
 *
 *	Owner: Darrell Walker
 *
 *	Purpose:  To determine what error state the printer is in.
 *
 *	Called by: Send()
 *
 *	Parameters on entry: status_reg is the contents of the centronics
 *						status register (at the time the error was
 *						detected)
 *
 *	Parameters on exit:	unchanged
 *
 *	Return Values: The proper DISPLAY_STATUS to reflect the printer
 *				error state.
 *
 */

DISPLAY_STATUS Printer::ParseError(BYTE status_reg)
{
	DBG1("Printer: parsing error info\n");

	DRIVER_ERROR err = NO_ERROR;
	BYTE DevIDBuffer[DevIDBuffSize];

	if(IOMode.bDevID)
	{
		// If a bi-di cable was plugged in and everything was OK, let's see if it's still
		// plugged in and everything is OK
		err = pSS->GetDeviceID(DevIDBuffer, DevIDBuffSize, TRUE);
		if(err != NO_ERROR)
			// job was bi-di but now something's messed up, probably cable unplugged
			// or printer turned off during print job
			return DISPLAY_COMM_PROBLEM;
	}

	// check for errors we can detect from the status reg

	if (IOMode.bStatus)
    {
	    if ( DEVICE_IS_OOP(status_reg) )
	    {
		    DBG1("Out Of Paper\n");
		    return DISPLAY_OUT_OF_PAPER;
	    }

	    if ( DEVICE_JAMMED_OR_TRAPPED(status_reg) )
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
        //  if this is a problem
        VerifyPenInfo();
    }
	  
	// don't know what the problem is-
    //  Is the PrinterAlive?  
	if (pSS->PrinterIsAlive())  // <- This is only viable if bStatus is TRUE
    {
        iTotal_SLOW_POLL_Count += iMax_SLOW_POLL_Count;

        // -Note that iTotal_SLOW_POLL_Count is a multiple of 
        //  iMax_SLOW_POLL_Count allowing us to check this
        //  on an absolute time limit - not relative to the number
        //  of times we happen to have entered ParseError.
        // -Also note that we have different thresholds for uni-di & bi-di.

        // REVISIT these counts - they are relative to the speed through
        // the send loop aren't they?  They may be too long!
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



DRIVER_ERROR Printer::Send(const BYTE* pWriteBuff)
{
	int len=strlen((const char*)pWriteBuff);
	return Send(pWriteBuff,len);
}

#if 0
/*
 * 	Function name: Printer::Send
 *
 *	Owner: Darrell Walker
 *
 *	Purpose:  Encapsulate error handling generated by performing I/O
 *
 *	Called by:
 *
 *	Calls made: WritePort(), GetStatus(), BusyWait(), ParseError(),
 *				DisplayPrinterStatus(), YieldToSystem()
 *
 *	Parameters on entry: pJob is a pointer to the current JOBSTRUCT,
 *						pWriteBuff is a pointer to the data the
 *						caller wishes to send to the pritner,
 *						wWriteCount is the number of bytes of
 *						pWriteBuff to send
 *
 *	Parameters on exit:	Unchanged
 *
 *	Side effects: Sends data to the printer, may update print dialog,
 *				may change pJob->InSlowPollMode, 
 *				pJob->ErrorTerminationState
 *
 *	Return Values: NO_ERROR or JOB_CANCELED or IO_ERROR
 *
 *  Comments: (TJL) This routine now has functionality to attempt iterating
 *      through wWriteCount bytes of data (until we hit slow poll mode)
 *      before sending PCL cleanup code.  This still leaves the possibility of
 *      prematurely exiting with an incomplete raster, but gives I/O a fighting chance
 *      of completing the raster while also protecting from a bogged down slow poll phase
 *      which would prevent a timely exit from CANCEL.  A JobCanceled flag is used
 *      because JOB_CANCELED cannot be maintained by write_error through the while
 *      loop since it will be overwritten by next ToDevice.
 *
 */

DRIVER_ERROR Printer::Send( const BYTE* pWriteBuff, DWORD dwWriteCount)

{
	DRIVER_ERROR 	write_error = NO_ERROR;
	DWORD 		    residual = dwWriteCount;
	const BYTE *	pWritePos = NULL;
	BOOL			error_displayed = FALSE;
	BYTE			status_reg = 0;
    DISPLAY_STATUS	eDisplayStatus = DISPLAY_PRINTING;
    BOOL            JobCanceled = FALSE;  // see comments in function header
 
////////////////////////////////////////////////////////////////
#ifdef NULL_IO
	// test imaging speed independent of printer I/O, will not
	// send any data to the device
	return NO_ERROR;
#endif
////////////////////////////////////////////////////////////////

	if (ErrorTerminationState) 	// don't try any more IO if we previously
		return JOB_CANCELED;	// terminated in an error state

	if (bCheckForCancelButton && 
	    (ulBytesSentSinceCancelCheck >= CANCEL_BUTTON_CHECK_THRESHOLD) )
	{
		ulBytesSentSinceCancelCheck = 0;
		char* tmpStr;
		BYTE DevIDBuffer[DevIDBuffSize];
		DRIVER_ERROR tmpErr = pSS->GetDeviceID(DevIDBuffer, DevIDBuffSize, TRUE);
		if(tmpErr)
			return tmpErr;
		if((tmpStr = strstr((char*)DevIDBuffer + 2,"CNCL")))
		{
			// Since the printer is now just throwing data away, we can bail
            // immediately w/o worrying about finishing the raster so the
            // end-of-job FF will work.
            ErrorTerminationState = TRUE;
			pSS->DisplayPrinterStatus(DISPLAY_PRINTING_CANCELED);
            return JOB_CANCELED;
		}
	}

	while (residual > 0)	// while still data to send in this request
	{
		DWORD prev_residual = residual;  	// WritePort overwrites request
									        // count, need to save

		pWritePos = (const BYTE *) &(pWriteBuff[dwWriteCount-residual]);
        write_error = pSS->ToDevice(pWritePos, &residual);

		if (residual == 0) // no more data to send this time
		{
			// did we want to transition out of slow poll?
			if ( (InSlowPollMode != 0) &&
				(prev_residual > MIN_XFER_FOR_SLOW_POLL) )
			{
				InSlowPollMode = 0;
                iTotal_SLOW_POLL_Count = 0;
		    }
			break;	// out of while loop
		}


		// if we are here, WritePort() was not able to
		// send the full request so start looking for errors

        // decide whether we've waited long enough to check for an error
        if (InSlowPollMode > iMax_SLOW_POLL_Count )
		{ 
            if (JobCanceled == TRUE)
            // Well, I/O didn't finish in time to meet the CANCEL request and avoid
            // the SlowPoll threshold.  We have to bail for prompt cancel response.
            {
                DBG1("Send(SlowPoll): Premature return w/ JOB_CANCELED\n");
                ErrorTerminationState = TRUE;
                return JOB_CANCELED;
            }

			DBG1("Printer slow poll times exceeded\n");
			// reset counter so we will not pop it next time
			InSlowPollMode = 1;
			write_error = IO_ERROR;
		}
		else
		{
			write_error = NO_ERROR;
		}

		// are we in slow poll mode?  If so, track our count
		if ( (prev_residual - residual) <= MIN_XFER_FOR_SLOW_POLL)
		{
			InSlowPollMode++;

#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
			if (InSlowPollMode == 1)
				printf("entering slow poll mode\n");
			else
				printf("still in slow poll mode, count = %d\n", 
									InSlowPollMode);
#endif
			// give the printer some time to process
            if (pSS->BusyWait((DWORD)1000) == JOB_CANCELED)
            {
                DBG1("Send: JOB_CANCELED\n");
                JobCanceled = TRUE;
				pSS->DisplayPrinterStatus(DISPLAY_PRINTING_CANCELED);
            }
		}
		else
		{
			// still busy, but taking enough data that
			// we are not in slow poll mode
			DBG1("Partial Send but not slow poll mode\n");
			InSlowPollMode = 0;
            iTotal_SLOW_POLL_Count = 0;
		}
			

        if (write_error != NO_ERROR) // slow poll times exceeded
        // the printer isn't taking data so let's see what's wrong...
	    {
            DBG1("Parsing possible error state...\n");
		    error_displayed = TRUE;

            // go get the status of the printer
		    if (IOMode.bStatus)
                pSS->GetStatusInfo(&status_reg);

		    // determine the error
		    eDisplayStatus = ParseError(status_reg);
				    
		    switch (eDisplayStatus)
		    {
     		    case DISPLAY_PRINTING_CANCELED:

					    // user canceled in an error state,
					    // so we don't want to attempt any
					    // further communication with the printer

					    ErrorTerminationState = TRUE;

					    pSS->DisplayPrinterStatus(eDisplayStatus);
					    return JOB_CANCELED;

     		    case DISPLAY_ERROR_TRAP:
     		    case DISPLAY_COMM_PROBLEM:
					    // these are unrecoverable cases
					    // don't let any more of this job
					    // be sent to the printer

					    ErrorTerminationState = TRUE;
					    pSS->DisplayPrinterStatus(eDisplayStatus);

					    // wait for user to cancel the job,
					    // otherwise they might miss the 
					    // error message

					    while (pSS->BusyWait(500) != JOB_CANCELED) 
							    // nothing....
						    ;

					    return IO_ERROR;

     		    case DISPLAY_TOP_COVER_OPEN:

				    pSS->DisplayPrinterStatus(DISPLAY_TOP_COVER_OPEN);

				    // wait for top cover to close
				    while ( eDisplayStatus == DISPLAY_TOP_COVER_OPEN)
				    {
					    if (pSS->BusyWait((DWORD)500) == JOB_CANCELED)
                        // although we'll leave an incomplete job in the printer,
                        // we really need to bail for proper CANCEL response.
					    {
						    ErrorTerminationState = TRUE;
						    return JOB_CANCELED;
					    }

		                if (IOMode.bStatus)
					        pSS->GetStatusInfo(&status_reg);

					    eDisplayStatus = ParseError(status_reg);
				    }

                    pSS->DisplayPrinterStatus(DISPLAY_PRINTING);

                    // Wait for printer to come back online
				    if (pSS->BusyWait((DWORD)1000) == JOB_CANCELED)
                    // Since the top_cover error HAS been handled, we have
                    // the opportunity to finish the raster before we hit
                    // the next slowpoll threshold.
				    {
                        DBG1("Send: JOB_CANCELED\n");
                        JobCanceled = TRUE;
			            pSS->DisplayPrinterStatus(DISPLAY_PRINTING_CANCELED);
                    }

				    break;

     		    case DISPLAY_OUT_OF_PAPER:

				    pSS->DisplayPrinterStatus(DISPLAY_OUT_OF_PAPER);

				    // wait for the user to add paper and
				    // press resume
				    while ( eDisplayStatus == DISPLAY_OUT_OF_PAPER)
				    {
					    if (pSS->BusyWait((DWORD)500) == JOB_CANCELED)
                        // although we'll leave an incomplete job in the printer,
                        // we really need to bail for proper CANCEL response.
					    {
						    ErrorTerminationState = TRUE;
						    return JOB_CANCELED;
					    }

                        if (IOMode.bStatus)
                            pSS->GetStatusInfo(&status_reg);

                        eDisplayStatus = ParseError(status_reg);
				    }

				    pSS->DisplayPrinterStatus(DISPLAY_PRINTING);

				    break;

                case DISPLAY_BUSY:

                    if (pSS->BusyWait((DWORD)5000) == JOB_CANCELED)
                    {
                        ErrorTerminationState = TRUE;
                        return JOB_CANCELED;
                    }

                    pSS->DisplayPrinterStatus(DISPLAY_BUSY);

                    break;

			    // other cases need no special handling, display
			    // the error and try to continue
			    default:
					    pSS->DisplayPrinterStatus(eDisplayStatus);
					    break;
		    }// switch
		} // if

        // give printer time to digest the data and check for 'cancel' before
        // the next iteration of the loop
        if (pSS->BusyWait((DWORD)200) == JOB_CANCELED)
        {
            DBG1("Send: JOB_CANCELED\n");
            JobCanceled = TRUE;
			pSS->DisplayPrinterStatus(DISPLAY_PRINTING_CANCELED);
        }

    }	// while (residual > 0)

    // restore my JOB_CANCELED error
    if (JobCanceled == TRUE)
    {
        DBG1("Send: Clean return w/ JOB_CANCELED\n");
        // ensure that display still says we're cancelling
	    pSS->DisplayPrinterStatus(DISPLAY_PRINTING_CANCELED);
        return JOB_CANCELED;
    }
    else
    {
        // ensure any error message has been cleared
	    pSS->DisplayPrinterStatus(DISPLAY_PRINTING);
        if (bCheckForCancelButton)
        {
            ulBytesSentSinceCancelCheck += dwWriteCount;
        }
        return NO_ERROR;
    }
}
#endif

BOOL Printer::TopCoverOpen(BYTE /*status_reg*/)
{
    char * pStr;
    BYTE bDevIDBuff[DevIDBuffSize];

    if(IOMode.bDevID == FALSE) 
        return FALSE;

    DRIVER_ERROR err = pSS->GetDeviceID(bDevIDBuff, DevIDBuffSize, TRUE);
    if (err != NO_ERROR) 
        return FALSE;

    if( (pStr=strstr((char*)bDevIDBuff+2,"VSTATUS:")) ) //  find the VSTATUS area
    {
        pStr+=8;        
        // now parse VSTATUS parameters
        // looking for UP for open, DN for closed
        if (strstr((char*)pStr,"UP"))
            return TRUE;
        if (strstr((char*)pStr,"DN"))
            return FALSE;

        DBG1("didn't find UP or DN!!\n");
        return FALSE;
    }
    else if ( (pStr=strstr((char*)bDevIDBuff+2,";S:00")) )  // binary encoded device ID status (version 0)
    {
        if ( (*(pStr+5) == '9') )
        {
            return TRUE;  
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;  // if we can't find VSTATUS or binary status field, assume top is not open        
    }
}

DRIVER_ERROR Printer::CleanPen()
{
DBG1("Printer::ClearPen() called\n");
    const BYTE PEN_CLEAN_PML[]={0x1B,0x45,0x1B,0x26,0x62,0x31,0x36,0x57,
				0x50,0x4D,0x4C,0x20,  // EscE Esc&b16WPML{space}
				0x04,0x00,0x06,0x01,0x04,0x01,0x05,0x01,
				0x01,0x04,0x01,0x64}; // PML Marking-Agent-Maintenance=100

    DWORD length=sizeof(PEN_CLEAN_PML);
	return pSS->ToDevice(PEN_CLEAN_PML,&length);
}

PrintMode* Printer::GetMode(unsigned int index)
{
    if (index >= ModeCount)
        return NULL;

    return pMode[index];    
}


PrintMode::PrintMode(unsigned long *map1,unsigned long *map2)
{
    cmap.ulMap1 = map1;
    cmap.ulMap2 = map2;

    BaseResX = BaseResY = 300;
    MixedRes= FALSE;

	// default setting
	for (int i=0; i < MAXCOLORPLANES; i++)
	  {
		ResolutionX[i] = BaseResX;
		ResolutionY[i] = BaseResY;
        ColorDepth[i]=1;
	  }

    medium = mediaPlain;
    theQuality = qualityNormal;
    dyeCount=4;

    Config.bResSynth=TRUE;
    Config.bErnie=FALSE;
    Config.bPixelReplicate=TRUE;
    Config.bColorImage=TRUE;
    Config.bCompress=TRUE;
                             
#ifdef PROTO
     if (!argProprietary)
     {
        BlackFEDTable=(BYTE*) HTBinary_open;
        ColorFEDTable=(BYTE*) HTBinary_open;
     }
     else
     {
        BlackFEDTable=(BYTE*) HTBinary_prop;
        ColorFEDTable=(BYTE*) HTBinary_prop;
     }
#else
    BlackFEDTable=GetHTBinary();
    ColorFEDTable=GetHTBinary();
#endif

    // set for most common cases
    bFontCapable = TRUE;    
    CompatiblePens[0] = BOTH_PENS;
    CompatiblePens[1] = DUMMY_PEN;
    CompatiblePens[2] = DUMMY_PEN;

    strcpy(ModeName,"Normal");
    
}


GrayMode::GrayMode(unsigned long *map)
    : PrintMode(map)
// grayscale uses econo, 300, 1 bit
{
    ColorDepth[K] = 1;
    dyeCount = 1;
    CompatiblePens[1] = BLACK_PEN;  // accept either black or both

    strcpy(ModeName,"Grayscale");
}

DRIVER_ERROR Printer::SetPenInfo(char*& pStr, BOOL QueryPrinter)
{
    DRIVER_ERROR err;

    if (QueryPrinter)
    {
        // read the DevID into the stored strDevID
        err = pSS->GetDeviceID(pSS->strDevID, DevIDBuffSize, TRUE);
        ERRCHECK;

        // update the static values of the pens
        err = pSS->DR->ParseDevIDString((const char*)(pSS->strDevID),pSS->strModel,&(pSS->VIPVersion),pSS->strPens);
        ERRCHECK;

        if( (pStr = strstr((char*)pSS->strDevID+2,"VSTATUS:")) )   //  find the VSTATUS area
        {
            pStr += 8;            
        }
        else if ( (pStr = strstr((char*)pSS->strDevID+2,";S:00")) )  // binary encoded device ID status (version 0)
        {
            pStr += 19;     // get to the number of pens field
        }
        else 
            return BAD_DEVICE_ID;  // - code should never reach this point
    }
    else pStr = pSS->strPens;
    
  return NO_ERROR;
}
