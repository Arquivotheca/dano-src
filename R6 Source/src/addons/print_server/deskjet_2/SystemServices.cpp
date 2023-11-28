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

// SystemServices
//
// Contains constructor, destructor and higher-level members
// of the class.
//
// The SystemServices object is used to encapsulate
// memory-management, I/O, and other utilities provided by
// the operating system or host system supporting the driver.
// It is an abstract base class, requiring implementation of
// the necessary routines by the host.
//
// Creation of SystemServices is the first step in the calling
// sequence for the driver, followed by creation of the PrintContext
// and the Job.
// The derived class constructor must include a call to the member
// function InitDeviceComm, which establishes communication with
// the printer if possible.
//
#ifdef PROTO
#include "../include/Header.h"
#include "../include/IO_defs.h"
#include "../debug/script.h"
#else
#include "Header.h"
#include "IO_defs.h"
#ifdef CAPTURE
#include "script.h"
#endif // CAPTURE
#endif



// Constructor instantiates the DeviceRegistry.
// Real work done in InitDeviceComm, called from derived
// class constructor.
SystemServices::SystemServices()
    : constructor_error(NO_ERROR)
#if defined(CAPTURE) || defined(PROTO)
    , Capturing(FALSE), pScripter(NULL)
#endif
{

    strModel[0] = strPens[0] = '\0';
    VIPVersion = 0;
#ifdef PROTO
	DR = new ProtoRegistry();     
#else
	DR = new DeviceRegistry();
#endif
	// DR can't fail

	IOMode.bDevID=FALSE;
	IOMode.bStatus=FALSE;
    IOMode.bUSB=FALSE;

}

SystemServices::~SystemServices()
{ 

    delete DR;

#if defined(CAPTURE) || defined(PROTO)
    if (pScripter)
        delete pScripter;
#endif

DBG1("deleting SystemServices\n");
}

///////////////////////////////////////////////////////////////////
// Function to determine whether printer is responsive.
// Calls host-supplied GetStatusInfo
//
// NOTE: This implementation is appropriate for Parallel bus only.
//
BOOL SystemServices::PrinterIsAlive()
{
	BYTE status_reg;
    
    // Technically, this function should not even be
    // called if IOMode.bStatus is known to be FALSE
    if( GetStatusInfo(&status_reg) == FALSE )
    {
        DBG1("PrinterIsAlive: No Status-Byte Available (Default = TRUE)\n");
        return TRUE;
    }

#define DJ6XX_OFF		(0xF8)
#define DJ400_OFF		(0xC0)
// sometimes the DJ400 reports a status byte of C8 when it's turned off
#define DJ400_OFF_BOGUS	(0xC8)
#define DEVICE_IS_OK(reg) (!((reg == DJ6XX_OFF) || (reg == DJ400_OFF) || (reg == DJ400_OFF_BOGUS)))

#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
	printf("status reg is 0x%02x\n",status_reg);
	if (DEVICE_IS_OK(status_reg))
		DBG1("PrinterIsAlive: returning TRUE\n");
	else
		DBG1("PrinterIsAlive: returning FALSE\n");
#endif

	return (DEVICE_IS_OK(status_reg));
}

// Same as host-supplied FreeMem, with extra safety check
// for null pointer.
DRIVER_ERROR SystemServices::FreeMemory(void *ptr)
{
	if (ptr == NULL)
		return SYSTEM_ERROR;

//		printf("FreeMemory freeing %p\n",ptr);
	FreeMem((BYTE*)ptr);

 return NO_ERROR;
}

DRIVER_ERROR SystemServices::GetDeviceID(BYTE* strID, int iSize, BOOL bQuery)
{
    if (iSize < 3)  // must have at least enough space for count bytes and NULL terminator
    {
        return(SYSTEM_ERROR);
    }
    
    if (bQuery) 
    {
        // initialize the first 3 bytes to NULL (1st 2 bytes may be binary count of string
        // length so need to clear them as well as the "real" start of the string)
        // so that if ReadDeviceID() does nothing with buffer we won't act upon what
        // was in the buffer before calling it
        strID[0] = strID[1] = strID[2] = '\0';
    
        // get the string    
        DRIVER_ERROR ret;
        if ((ret = ReadDeviceID(strID, iSize)) != NO_ERROR)
        {
            return ret;
        } 
    }  
    else
    {
        // for use when string doesn't have to be re-fetched from printer
        
        if (DevIDBuffSize > iSize)
        {
		    return SYSTEM_ERROR;
        }

        // the first 2 bytes may be binary so could be 0 (NULL) so can use strcpy
        // (could get strlen of strDevID if start @ strDevID+2 and then add 2
        //  if do this it wouldn't require that caller's buffer be >=
        //  DevIDBuffSize, only that is it longer that actual devID string read)  
        memcpy(strID, strDevID, DevIDBuffSize);
    }
    
    
    // check the read (or copied) DeviceID string for validity
    
    // check what may be the binary count of the string length (some platforms return
    // the raw DeviceID in which the 1st 2 bytes are a binary count of the string length,
    // other platforms strip off thes count bytes)
    // if they are a binary count they shouldn't be zero, and if they aren't a binary
    // count they also shouldn't be zero (NULL) since that would mean end of string
    if ((strID[0] == '\0') && (strID[1] == '\0'))
    {
        return BAD_DEVICE_ID;
    }
    
    // look for the existence of either of the defined manufacturer fields in the string
    // (need to look starting at strID[0] and at strID[2] since the first 2 bytes may or
    //  may not be binary count bytes, one of which could be a binary 0 (NULL) which strstr()
    //  will interpret as the end of string)
    if (!strstr((const char*)strID, "MFG:") &&
        !strstr((const char*)strID+2, "MFG:") &&
        !strstr((const char*)strID, "MANUFACTURER:") &&
        !strstr((const char*)strID+2, "MANUFACTURER:"))
    {
        return BAD_DEVICE_ID;
    }
    
    return NO_ERROR;
}

////////////////////////////////////////////////////////////////////
DRIVER_ERROR SystemServices::InitDeviceComm()
// Must be called from derived class constructor.
// (Base class must be constructed before system calls
//  below can be made.)
// Opens the port, looks for printer and 
// dialogues with user if none found;
// then attempts to read and parse device ID string --
// if successful, sets IOMode.bDevID to TRUE (strings stored
// for retrieval by PrintContext).
// Returns an error only if user cancelled. Otherwise
// no error even if unidi.
//
// Calls: OpenPort,PrinterIsAlive,DisplayPrinterStatus,BusyWait,
//   GetDeviceID,DeviceRegistry::ParseDevIDString.
// Sets:    hPort,IOMode, strModel, strPens
{
    DRIVER_ERROR err = NO_ERROR;
    BOOL ErrorDisplayed = FALSE;
    BYTE temp;

    // Check whether this system supports passing back a status-byte
    if( GetStatusInfo(&temp) == FALSE )
    {
        DBG1("InitDeviceComm:  No Status-Byte Available\n");
    }
    else IOMode.bStatus = TRUE;
    
	// Check whether we can get a DeviceID - this may
    // still fail if the device is just turned off
    err = GetDeviceID(strDevID, DevIDBuffSize, TRUE);

    if ( err == NO_ERROR )
    {
        DBG1("InitDeviceComm:  DevID request successful\n");
        IOMode.bDevID = TRUE;
    }


    // PrinterIsAlive is arbitrary if we can't get the status-byte.
    // This check is also critical so a true uni-di system does not sit
    // in a loop informing the user to turn on the printer.
    if ( IOMode.bStatus == TRUE )
    {
        // Make sure a printer is there, turned on and connected
	    // before we go any further.  This takes some additional checking
	    // due to the fact that the 895 returns a status byte of F8 when
	    // it's out of paper, the same as a 600 when it's turned off.
        // 895 can get a devID even when 'off' so we'll key off that logic.
	    if ( (err != NO_ERROR) && (PrinterIsAlive() == FALSE) )
	    {
		    // Printer is actually turned off
		    while(PrinterIsAlive() == FALSE)
		    {
			    DBG1("PrinterIsAlive returned FALSE\n");
			    ErrorDisplayed = TRUE;
			    DisplayPrinterStatus(DISPLAY_NO_PRINTER_FOUND);

			    if(BusyWait(500) == JOB_CANCELED)
				    return JOB_CANCELED;
		    }
		    if(ErrorDisplayed == TRUE)
		    {
			    DisplayPrinterStatus(DISPLAY_PRINTING);
			    // if they just turned on/connected the printer,
			    // delay a bit to let it initialize
			    if(BusyWait(2000) == JOB_CANCELED)
				    return JOB_CANCELED;

			    err = GetDeviceID(strDevID, DevIDBuffSize, TRUE);
                if ( err == NO_ERROR )
                {
                    DBG1("InitDeviceComm:  DevID request successful\n");
                    IOMode.bDevID = TRUE;
                }
		    }
	    }
	    // else... we have 8xx/9xx with an out-of-paper error
        // which we will catch in the I/O handling

    }
    
    if (err!=NO_ERROR)
    {
        DBG1("InitDeviceComm:  No DeviceID Available\n");
        return NO_ERROR;
    }

    err = DR->ParseDevIDString((const char*)strDevID, strModel, &VIPVersion, strPens);

    if (err!=NO_ERROR)
    {
        // The DevID we got is actually garbage!
        DBG1("InitDeviceComm:  The DevID string is invalid!\n");
        IOMode.bDevID=FALSE;
    }

	return NO_ERROR;
}


// This function is only relevant for DeskJet 400 support and is therefore
// basically 'stubbed out' since the DJ400 is now supported only on some
// grandfather-ed systems.  Systems that DO support the DJ400 MUST implement
// this function in their derived SystemServices class.
DRIVER_ERROR SystemServices::GetECPStatus(BYTE *pStatusString,int *pECPLength, int ECPChannel)
{
    pStatusString = NULL;
    *pECPLength = 0;

    return UNSUPPORTED_FUNCTION;
}
