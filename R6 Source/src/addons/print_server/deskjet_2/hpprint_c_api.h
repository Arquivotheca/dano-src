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

// hpprint_c_api.h
// 'C' interface functions to APDK external C++ interfaces
// For use when calling environment is written in 'C' not C++
// (a C++ compiler is still required, however; and there must
// be a derived SystemServices class defined for the host environment)

#ifndef HPPRINT_C_API_H
#define HPPRINT_C_API_H

#include "global_types.h"

typedef void * JobHandle;
typedef void * PrintContextHandle;
typedef void * FontHandle;
typedef void * ReferenceFontHandle;
typedef void * SystemServicesHandle;

//////////////////////////////////////////
// 'C' interface to Job class
//

#ifdef __cplusplus
extern "C" {
#endif

extern DRIVER_ERROR C_Job_Create(JobHandle *phNewJob, PrintContextHandle hPrintContext);

extern void C_Job_Destroy(JobHandle hJob);

extern DRIVER_ERROR C_Job_SendRasters(JobHandle hJob, BYTE* ImageData);

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)	
extern DRIVER_ERROR C_Job_TextOut(JobHandle hJob, const char* pTextString, 
                                  unsigned int iLenString, const FontHandle hFont, 
                                  int iAbsX, int iAbsY );
#endif

extern DRIVER_ERROR C_Job_NewPage(JobHandle hJob);


//////////////////////////////////////////
// 'C' interface to PrintContext class
//

extern DRIVER_ERROR C_PrintContext_Create(PrintContextHandle *phNewPrintContext,
                                          SystemServicesHandle hSysServ,
                                          unsigned int InputPixelsPerRow,
                                          unsigned int OutputPixelsPerRow,
                                          PAPER_SIZE ps);        	

extern void C_PrintContext_Destroy(PrintContextHandle hPrintContext);

extern void C_PrintContext_Flush(PrintContextHandle hPrintContext, int FlushSize);

extern DRIVER_ERROR C_PrintContext_SelectDevice(PrintContextHandle hPrintContext, const PRINTER_TYPE Model);

extern unsigned int C_PrintContext_GetModeCount(PrintContextHandle hPrintContext);

extern DRIVER_ERROR C_PrintContext_SelectPrintMode(PrintContextHandle hPrintContext, const unsigned int index);

extern unsigned int C_PrintContext_CurrentPrintMode(PrintContextHandle hPrintContext);

extern char* C_PrintContext_GetModeName(PrintContextHandle hPrintContext);

extern PRINTER_TYPE C_PrintContext_SelectedDevice(PrintContextHandle hPrintContext);

extern HELP_TYPE C_PrintContext_GetHelpType(PrintContextHandle hPrintContext);

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
extern ReferenceFontHandle C_PrintContext_EnumFont(PrintContextHandle hPrintContext, int * iCurrIdx);

extern FontHandle C_PrintContext_RealizeFont(PrintContextHandle hPrintContext,
                                             const int index, const BYTE bSize, const TEXTCOLOR eColor,
							                 const BOOL bBold, const BOOL bItalic, const BOOL bUnderline);
#endif

extern PRINTER_TYPE C_PrintContext_EnumDevices(const PrintContextHandle hPrintContext, unsigned int * currIdx);

extern DRIVER_ERROR C_PrintContext_PerformPrinterFunction(PrintContextHandle hPrintContext, PRINTER_FUNC eFunc);

extern DRIVER_ERROR C_PrintContext_SetPaperSize(PrintContextHandle hPrintContext, PAPER_SIZE ps);

extern DRIVER_ERROR C_PrintContext_SetPixelsPerRow(PrintContextHandle hPrintContext,
                                                   unsigned int InputPixelsPerRow,
                                                   unsigned int OutputPixelsPerRow);

extern BOOL C_PrintContext_PrinterSelected(PrintContextHandle hPrintContext);

extern BOOL C_PrintContext_PrinterFontsAvailable(PrintContextHandle hPrintContext, unsigned int PrintModeIndex);

extern unsigned int C_PrintContext_InputPixelsPerRow(PrintContextHandle hPrintContext);

extern unsigned int C_PrintContext_OutputPixelsPerRow(PrintContextHandle hPrintContext);

extern PAPER_SIZE C_PrintContext_GetPaperSize(PrintContextHandle hPrintContext);

extern const char* C_PrintContext_PrinterModel(PrintContextHandle hPrintContext);

extern const char* C_PrintContext_PrintertypeToString(PrintContextHandle hPrintContext, PRINTER_TYPE pt);

extern unsigned int C_PrintContext_InputResolution(PrintContextHandle hPrintContext);

extern DRIVER_ERROR C_PrintContext_SetInputResolution(PrintContextHandle hPrintContext, unsigned int Res);

extern unsigned int C_PrintContext_EffectiveResolutionX(PrintContextHandle hPrintContext);

extern unsigned int C_PrintContext_EffectiveResolutionY(PrintContextHandle hPrintContext);

extern float C_PrintContext_PrintableWidth(PrintContextHandle hPrintContext);

extern float C_PrintContext_PrintableHeight(PrintContextHandle hPrintContext);

extern float C_PrintContext_PhysicalPageSizeX(PrintContextHandle hPrintContext);

extern float C_PrintContext_PhysicalPageSizeY(PrintContextHandle hPrintContext);

extern float C_PrintContext_PrintableStartX(PrintContextHandle hPrintContext);

extern float C_PrintContext_PrintableStartY(PrintContextHandle hPrintContext);

extern DRIVER_ERROR C_PrintContext_SendPrinterReadyData(PrintContextHandle hPrintContext, BYTE* stream, unsigned int size);


//////////////////////////////////////////
// 'C' interface to Font class
//
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

extern void C_Font_Destroy(FontHandle hFont);

extern DRIVER_ERROR C_Font_GetTextExtent(FontHandle hFont,
                                         const char* pTextString, const int iLenString,
								         int * iHeight, int * iWidth);

extern const char* C_Font_GetName(const FontHandle hFont);

extern BOOL C_Font_IsBoldAllowed(const FontHandle hFont);

extern BOOL C_Font_IsItalicAllowed(const FontHandle hFont);

extern BOOL C_Font_IsUnderlineAllowed(const FontHandle hFont);

extern BOOL C_Font_IsColorAllowed(const FontHandle hFont);

extern BOOL C_Font_IsProportional(const FontHandle hFont);

extern BOOL C_Font_HasSerif(const FontHandle hFont);

extern BYTE C_Font_GetPitch(const FontHandle hFont, const BYTE pointsize);

extern int C_Font_Get_iPointsize(const FontHandle hFont);

extern void C_Font_Set_iPointsize(const FontHandle hFont, int iPointsize);

extern BOOL C_Font_Get_bBold(const FontHandle hFont);

extern void C_Font_Set_bBold(const FontHandle hFont, BOOL bBold);

extern BOOL C_Font_Get_bItalic(const FontHandle hFont);

extern void C_Font_Set_bItalic(const FontHandle hFont, BOOL bItalic);

extern BOOL C_Font_Get_bUnderline(const FontHandle hFont);

extern void C_Font_Set_bUnderline(const FontHandle hFont, BOOL bUnderline);

extern TEXTCOLOR C_Font_Get_eColor(const FontHandle hFont);

extern void C_Font_Set_eColor(const FontHandle hFont, TEXTCOLOR eColor);

extern int C_Font_Get_iPitch(const FontHandle hFont);

extern void C_Font_Set_iPitch(const FontHandle hFont, int iPitch);

extern int C_Font_Index(FontHandle hFont);

#endif  // defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

#ifdef __cplusplus
}
#endif

#endif  // HPPRINT_C_API_H
