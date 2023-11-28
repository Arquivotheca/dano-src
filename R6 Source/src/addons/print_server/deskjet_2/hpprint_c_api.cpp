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

// hpprint_c_api.cpp
// 'C' interface functions to APDK external C++ interfaces
// For use when calling environment is written in 'C' not C++
// (a C++ compiler is still required, however)

#ifdef _APDK_C_API

#ifdef PROTO
#include "../include/Header.h"
#include "../api/hpprint_c_api.h"
#else
#include "Header.h"
#include "hpprint_c_api.h"
#endif

//////////////////////////////////////////
// 'C' interface to Job class
//

DRIVER_ERROR C_Job_Create(JobHandle *phNewJob, PrintContextHandle hPrintContext)
{
    Job *pJob = new Job((PrintContext *)hPrintContext);
    if (pJob)
    {
        *phNewJob = (JobHandle) pJob;
        return(pJob->constructor_error);
    }
    else
    {
        return(ALLOCMEM_ERROR);
    }
}    

void C_Job_Destroy(JobHandle hJob)
{
    if (hJob)
    {
        delete (Job *)hJob;
    }
}    

DRIVER_ERROR C_Job_SendRasters(JobHandle hJob, BYTE* ImageData)
{
    return(((Job *)hJob)->SendRasters(ImageData));
}    

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)	
DRIVER_ERROR C_Job_TextOut(JobHandle hJob, const char* pTextString, 
                                  unsigned int iLenString, const FontHandle hFont, 
                                  int iAbsX, int iAbsY )
{
    return(((Job *)hJob)->TextOut(pTextString, iLenString, *((Font *)hFont), iAbsX, iAbsY));
}
#endif

DRIVER_ERROR C_Job_NewPage(JobHandle hJob)
{
    return(((Job *)hJob)->NewPage());
}


//////////////////////////////////////////
// 'C' interface to PrintContext class
//

DRIVER_ERROR C_PrintContext_Create(PrintContextHandle *phNewPrintContext,
                                          SystemServicesHandle hSysServ,
                                          unsigned int InputPixelsPerRow,
                                          unsigned int OutputPixelsPerRow,
                                          PAPER_SIZE ps)
{
    PrintContext *pPC = new PrintContext((SystemServices *)hSysServ, InputPixelsPerRow, 
                                         OutputPixelsPerRow, ps);
    if (pPC)
    {
        *phNewPrintContext = (PrintContextHandle) pPC;
        return(pPC->constructor_error);
    }
    else
    {
        return(ALLOCMEM_ERROR);
    }
}        	

void C_PrintContext_Destroy(PrintContextHandle hPrintContext)
{
    if (hPrintContext)
    {
        delete (PrintContext *)hPrintContext;
    }
}

void C_PrintContext_Flush(PrintContextHandle hPrintContext, int FlushSize)
{
    ((PrintContext *)hPrintContext)->Flush(FlushSize);
}

DRIVER_ERROR C_PrintContext_SelectDevice(PrintContextHandle hPrintContext, const PRINTER_TYPE Model)
{
    return(((PrintContext *)hPrintContext)->SelectDevice(Model));
    
}

unsigned int C_PrintContext_GetModeCount(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->GetModeCount());   
}

DRIVER_ERROR C_PrintContext_SelectPrintMode(PrintContextHandle hPrintContext, const unsigned int index)
{
    return(((PrintContext *)hPrintContext)->SelectPrintMode(index));
}

unsigned int C_PrintContext_CurrentPrintMode(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->CurrentPrintMode());
}

char* C_PrintContext_GetModeName(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->GetModeName());
}

PRINTER_TYPE C_PrintContext_SelectedDevice(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->SelectedDevice());
}

HELP_TYPE C_PrintContext_GetHelpType(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->GetHelpType());
}

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
ReferenceFontHandle C_PrintContext_EnumFont(PrintContextHandle hPrintContext, int * iCurrIdx)
{
    return((ReferenceFontHandle)((PrintContext *)hPrintContext)->EnumFont(*iCurrIdx));
}

FontHandle C_PrintContext_RealizeFont(PrintContextHandle hPrintContext,
                                             const int index, const BYTE bSize, const TEXTCOLOR eColor,
							                 const BOOL bBold, const BOOL bItalic, const BOOL bUnderline)
{
    return((FontHandle)((PrintContext *)hPrintContext)->RealizeFont(index, bSize, eColor, bBold, bItalic, bUnderline));
}
#endif

PRINTER_TYPE C_PrintContext_EnumDevices(const PrintContextHandle hPrintContext, unsigned int * currIdx)
{
    return(((PrintContext *)hPrintContext)->EnumDevices(*currIdx));
}

DRIVER_ERROR C_PrintContext_PerformPrinterFunction(PrintContextHandle hPrintContext, PRINTER_FUNC eFunc)
{
    return(((PrintContext *)hPrintContext)->PerformPrinterFunction(eFunc));
}

DRIVER_ERROR C_PrintContext_SetPaperSize(PrintContextHandle hPrintContext, PAPER_SIZE ps)
{
    return(((PrintContext *)hPrintContext)->SetPaperSize(ps));
}

DRIVER_ERROR C_PrintContext_SetPixelsPerRow(PrintContextHandle hPrintContext,
                                                   unsigned int InputPixelsPerRow,
                                                   unsigned int OutputPixelsPerRow)
{
    return(((PrintContext *)hPrintContext)->SetPixelsPerRow(InputPixelsPerRow, OutputPixelsPerRow));
}

BOOL C_PrintContext_PrinterSelected(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->PrinterSelected());
}

BOOL C_PrintContext_PrinterFontsAvailable(PrintContextHandle hPrintContext, unsigned int PrintModeIndex)
{
    return(((PrintContext *)hPrintContext)->PrinterFontsAvailable(PrintModeIndex));
}

unsigned int C_PrintContext_InputPixelsPerRow(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->InputPixelsPerRow());
}

unsigned int C_PrintContext_OutputPixelsPerRow(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->OutputPixelsPerRow());
}

PAPER_SIZE C_PrintContext_GetPaperSize(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->GetPaperSize());
}

const char* C_PrintContext_PrinterModel(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->PrinterModel());
}

const char* C_PrintContext_PrintertypeToString(PrintContextHandle hPrintContext, PRINTER_TYPE pt)
{
    return(((PrintContext *)hPrintContext)->PrintertypeToString(pt));
}

unsigned int C_PrintContext_InputResolution(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->InputResolution());
}

DRIVER_ERROR C_PrintContext_SetInputResolution(PrintContextHandle hPrintContext, unsigned int Res)
{
    return(((PrintContext *)hPrintContext)->SetInputResolution(Res));
}

unsigned int C_PrintContext_EffectiveResolutionX(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->EffectiveResolutionX());
}

unsigned int C_PrintContext_EffectiveResolutionY(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->EffectiveResolutionY());
}

float C_PrintContext_PrintableWidth(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->PrintableWidth());
}

float C_PrintContext_PrintableHeight(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->PrintableHeight());
}

float C_PrintContext_PhysicalPageSizeX(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->PhysicalPageSizeX());
}

float C_PrintContext_PhysicalPageSizeY(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->PhysicalPageSizeY());
}

float C_PrintContext_PrintableStartX(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->PrintableStartX());
}

float C_PrintContext_PrintableStartY(PrintContextHandle hPrintContext)
{
    return(((PrintContext *)hPrintContext)->PrintableStartY());
}

DRIVER_ERROR C_PrintContext_SendPrinterReadyData(PrintContextHandle hPrintContext, BYTE* stream, unsigned int size)
{
    return(((PrintContext *)hPrintContext)->SendPrinterReadyData(stream, size));
}


//////////////////////////////////////////
// 'C' interface to Font class
//
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

void C_Font_Destroy(FontHandle hFont)
{
    if (hFont)
    {
        delete (Font *)hFont;
    }
}

DRIVER_ERROR C_Font_GetTextExtent(FontHandle hFont,
                                  const char* pTextString, const int iLenString,
								  int * iHeight, int * iWidth)
{
    return(((Font *)hFont)->GetTextExtent(pTextString, iLenString, *iHeight, *iWidth));
}

const char* C_Font_GetName(const FontHandle hFont)
{
    return(((Font *)hFont)->GetName());
}

BOOL C_Font_IsBoldAllowed(const FontHandle hFont)
{
    return(((Font *)hFont)->IsBoldAllowed());
}

BOOL C_Font_IsItalicAllowed(const FontHandle hFont)
{
    return(((Font *)hFont)->IsItalicAllowed());
}

BOOL C_Font_IsUnderlineAllowed(const FontHandle hFont)
{
    return(((Font *)hFont)->IsUnderlineAllowed());
}

BOOL C_Font_IsColorAllowed(const FontHandle hFont)
{
    return(((Font *)hFont)->IsColorAllowed());
}

BOOL C_Font_IsProportional(const FontHandle hFont)
{
    return(((Font *)hFont)->IsProportional());
}

BOOL C_Font_HasSerif(const FontHandle hFont)
{
    return(((Font *)hFont)->HasSerif());
}

BYTE C_Font_GetPitch(const FontHandle hFont, const BYTE pointsize)
{
    return(((Font *)hFont)->GetPitch(pointsize));
}

int C_Font_Get_iPointsize(const FontHandle hFont)
{
    return(((Font *)hFont)->iPointsize);
}

void C_Font_Set_iPointsize(const FontHandle hFont, int iPointsize)
{
    ((Font *)hFont)->iPointsize = iPointsize;
}

BOOL C_Font_Get_bBold(const FontHandle hFont)
{
    return(((Font *)hFont)->bBold);
}

void C_Font_Set_bBold(const FontHandle hFont, BOOL bBold)
{
    ((Font *)hFont)->bBold = bBold;
}

BOOL C_Font_Get_bItalic(const FontHandle hFont)
{
    return(((Font *)hFont)->bItalic);
}

void C_Font_Set_bItalic(const FontHandle hFont, BOOL bItalic)
{
    ((Font *)hFont)->bItalic = bItalic;
}

BOOL C_Font_Get_bUnderline(const FontHandle hFont)
{
    return(((Font *)hFont)->bUnderline);
}

void C_Font_Set_bUnderline(const FontHandle hFont, BOOL bUnderline)
{
    ((Font *)hFont)->bUnderline = bUnderline;
}

TEXTCOLOR C_Font_Get_eColor(const FontHandle hFont)
{
    return(((Font *)hFont)->eColor);
}

void C_Font_Set_eColor(const FontHandle hFont, TEXTCOLOR eColor)
{
    ((Font *)hFont)->eColor = eColor;
}

int C_Font_Get_iPitch(const FontHandle hFont)
{
    return(((Font *)hFont)->iPitch);
}

void C_Font_Set_iPitch(const FontHandle hFont, int iPitch)
{
    ((Font *)hFont)->iPitch = iPitch;
}

int C_Font_Index(FontHandle hFont)
{
    return(((Font *)hFont)->Index());
}


#endif  // defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

#endif  // _APDK_C_API
