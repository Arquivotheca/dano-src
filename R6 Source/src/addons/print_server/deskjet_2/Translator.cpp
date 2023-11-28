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
#include "../imaging/imager.h"
#else
#include "Header.h"
#include "imager.h"
#endif


RasterSender::RasterSender(Printer* pP, PrintContext* pPC, 
                       Job* pJob,Imager* pImager)
	: thePrinter(pP), thePrintContext(pPC), theJob(pJob),theImager(pImager)
{ 
    constructor_error=NO_ERROR;
}

RasterSender::~RasterSender()
{
}

////////////////////////////////////////////////////////////////////////
BOOL RasterSender::Process(BYTE* InputRaster, unsigned int size) 
{ 
    DRIVER_ERROR err=NO_ERROR;
    BOOL bOutput=FALSE;
    if (InputRaster)
    {
        err=SendRaster(InputRaster,size); 
        if (err==NO_ERROR)
            bOutput=TRUE;
    }
    else err=theJob->SendCAPy(); 
   
    myphase->err = err;

  return bOutput;
}


DRIVER_ERROR RasterSender::SendRaster(BYTE* InputRaster,unsigned int size)
{ 
    char scratch[20];
    DRIVER_ERROR err;
    BOOL lastplane,firstplane;
	    
    if (theImager)
    {
        lastplane = theImager->LastPlane();
        firstplane = theImager->FirstPlane();
    }
    else lastplane=firstplane=TRUE;

    if (firstplane)      
            err=theJob->SendCAPy();

	int scratchLen;
	if (lastplane)					
		scratchLen = sprintf(scratch, "%c%c%c%d%c", '\033', '*', 'b', size, 'W');											
	else					
		scratchLen = sprintf(scratch, "%c%c%c%d%c", '\033', '*', 'b', size, 'V');											
					
	err = thePrinter->Send((const BYTE*)scratch,scratchLen);
	ERRCHECK;
		
	err = thePrinter->Send(InputRaster,size);
	ERRCHECK;

  return err;
}




  
