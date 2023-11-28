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

#include "debug.h"

#ifdef CAPTURE
#ifdef PROTO
#include "../include/Header.h"
#include "../debug/script.h"
#else
#include "Header.h"
#include "script.h"
#endif
//////////////////////////////////////////////////////////////
// Capture functions belonging to API

SystemServices* copySS;


void PrintContext::Capture_PrintContext(unsigned int InputPixelsPerRow, 
                                        unsigned int OutputPixelsPerRow,
                                        PAPER_SIZE ps, 
                                        IO_MODE IOMode)
{

    copySS=pSS;
    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokPrintContext);
    pS->PutDebugInt(InputPixelsPerRow);
    pS->PutDebugInt(OutputPixelsPerRow);
    pS->PutDebugByte(ps);
    pS->PutDebugByte(IOMode.bDevID);
	pS->PutDebugByte(IOMode.bStatus);
    if (!IOMode.bDevID)
        return;
    
    // need to simulate bidi, remember model and pens
    pS->PutDebugString((const char*)pSS->strModel,strlen(pSS->strModel));
    pS->PutDebugString((const char*)pSS->strPens,strlen(pSS->strPens));	

}


void PrintContext::Capture_RealizeFont(const unsigned int ptr,
                                       const unsigned int index,
									   const BYTE bSize,
						               const TEXTCOLOR eColor,
						               const BOOL bBold,
						               const BOOL bItalic,
						               const BOOL bUnderline)
{

    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokRealizeFont);
    pS->PutDebugInt(ptr);
	pS->PutDebugByte(index);
	pS->PutDebugByte(bSize);
	pS->PutDebugByte(eColor);
	pS->PutDebugByte(bBold);
	pS->PutDebugByte(bItalic);
	pS->PutDebugByte(bUnderline);

}

void PrintContext::Capture_SetPixelsPerRow(unsigned int InputPixelsPerRow,
                                           unsigned int OutputPixelsPerRow)
{

    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokSetPixelsPerRow);
	pS->PutDebugInt(InputPixelsPerRow);
    pS->PutDebugInt(OutputPixelsPerRow);
}

void PrintContext::Capture_SetInputResolution(unsigned int Res)
{

    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokSetRes);
	pS->PutDebugInt(Res);

}


void PrintContext::Capture_SelectDevice(const PRINTER_TYPE Model)
{

    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokSelectDevice);
	pS->PutDebugByte(Model);

}

void PrintContext::Capture_SelectPrintMode(unsigned int modenum)
{

    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokSelectPrintMode);
	pS->PutDebugByte(modenum);
}

void PrintContext::Capture_SetPaperSize(PAPER_SIZE ps)
{

    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokSetPaperSize);
	pS->PutDebugByte(ps);
}




void Job::Capture_Job(PrintContext* pPC)
{

    if (! thePrintContext->pSS->Capturing)
        return;

    Scripter *pS = thePrintContext->pSS->pScripter;

	pS->PutDebugToken(tokJob);

}

void Job::Capture_SendRasters(BYTE* ImageData)
{

    if (! thePrintContext->pSS->Capturing)
        return;

    Scripter *pS = thePrintContext->pSS->pScripter;

	pS->PutDebugToken(tokSendRasters);
    unsigned int len=0;
	if (ImageData != NULL)
        len= thePrintContext->OutputWidth*3;
	pS->PutDebugStream(ImageData, len);

}

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
void Job::Capture_TextOut(const char* pTextString, 
					      unsigned int iLenString,
					      const Font& font, 
					      unsigned int iAbsX, 
					      unsigned int iAbsY)
{
    SystemServices* pSS = thePrintContext->pSS;

    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokTextOut);
	pS->PutDebugInt((int)&font);
	pS->PutDebugString(pTextString,iLenString);
	pS->PutDebugInt(iAbsX);
	pS->PutDebugInt(iAbsY);
}
#endif

void Job::Capture_NewPage()
{
    SystemServices* pSS = thePrintContext->pSS;
    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokNewPage);

}


void Job::Capture_dJob()
{
    SystemServices* pSS = thePrintContext->pSS;
    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokdJob);	

}

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
void Font::Capture_dFont(const unsigned int ptr)
{

    if (! copySS->Capturing)
        return;

    Scripter *pS = copySS->pScripter;

	pS->PutDebugToken(tokdFont);
	pS->PutDebugInt(ptr);
}
#endif

void PrintContext::Capture_dPrintContext()
{

    if (! pSS->Capturing)
        return;

    Scripter *pS = pSS->pScripter;

	pS->PutDebugToken(tokdPrintContext);

}


#endif
