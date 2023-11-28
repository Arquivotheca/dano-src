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

// global_types.h
//
// Definitions and structures needed by applications
//
// This file does not include C++ class definitions etc. so it can
// be included by calling C or C++ source files

#ifndef GLOBAL_TYPES_H
#define GLOBAL_TYPES_H

#include "models.h"

// ** Defines

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef BOOL
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#endif

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#ifndef LOWORD
#define LOWORD(l)   ((WORD) (l))
#endif

#ifndef HIWORD
#define HIWORD(l)   ((WORD) (((DWORD) (l) >> 16) & 0xFFFF))
#endif

#ifndef ABS
#define ABS(x)      ( ((x)<0) ? -(x) : (x) )
#endif

#ifdef BLACK_PEN
#undef BLACK_PEN
#endif

#ifdef NO_ERROR
#undef NO_ERROR
#endif

typedef int DRIVER_ERROR;

typedef int MediaType;		// for use in connection with PCL media-type command
							// values are PCL codes
#define mediaAuto -1
#define mediaPlain 0
#define mediaBond 0
#define mediaSpecial 2
#define mediaGlossy 3
#define mediaTransparency 4

typedef int MediaSize;		// for use in connection with PCL media-size command
							// values are PCL codes
#define sizeUSLetter 2
#define sizeUSLegal 3
#define sizeA4 26
#define sizeNum10Env 81
#define sizePhoto   99  // BOGUS NUMBER I MADE UP
	
typedef int MediaSource;	// for use in connection with PCL media-source command
							// values are PCL codes		
#define sourceTray1 1
//#define sourceTray2 4		// no second tray in current supported list
#define sourceManual 2
#define sourceManualEnv 3
	
typedef int Quality;		// for use in connection with PCL quality-mode command
							// values are PCL codes	
#define qualityPresentation 1
#define qualityNormal 0
#define qualityDraft -1


#define COURIER_INDEX 1
#define CGTIMES_INDEX 2
#define LETTERGOTHIC_INDEX 3
#define UNIVERS_INDEX 4


#define MAX_CHAR_SET 5
#define MAX_POINTSIZES 5

#define GRAYMODE_INDEX 0
#define DEFAULTMODE_INDEX 1
#define SPECIALMODE_INDEX 2
#define EXTRASPECIALMODE_INDEX 3

#define MAXCOLORDEPTH 3
#define MAXCOLORPLANES	6	// current max anticipated, 6 for 690 photopen
#define MAXCOLORROWS 2      // multiple of high-to-low for mixed-resolution cases


// ** JOB related structures/enums

typedef enum { BLACK_PEN, COLOR_PEN, BOTH_PENS, MDL_PEN, MDL_BOTH, NO_PEN, DUMMY_PEN } PEN_TYPE;
#define MAX_PEN_TYPE 4      // base-0, ending with MDL_BOTH (NOT NO_PEN)

// the PAPER_SIZE enum is directly supported by PSM in PrintContext
typedef enum { UNSUPPORTED_SIZE =-1, LETTER = 0, A4 = 1, LEGAL = 2, PHOTO_SIZE = 3} PAPER_SIZE; 

typedef enum { CLEAN_PEN = 0 } PRINTER_FUNC;


// ** TEXT related structures/enums

typedef enum {

	WHITE_TEXT,
	CYAN_TEXT,
	MAGENTA_TEXT,
	BLUE_TEXT,
	YELLOW_TEXT,
	GREEN_TEXT,
	RED_TEXT,
	BLACK_TEXT

} TEXTCOLOR;


typedef enum {
// currently only portrait fonts are supported

	PORTRAIT,
	LANDSCAPE,
	BOTH

} TEXTORIENT;

#define MAX_FONT_SIZES 10 	// max # of fonts to be realized at one time 

// ** I/O related stuff
#define TIMEOUTVAL 		500		// in msec, ie 0.5 sec

typedef WORD PORTID;
typedef void * PORTHANDLE;

typedef enum 
{
	COMPATIBILITY,
	NIBBLE,
	ECP
} MODE1284;

//////////////////////////////////////////////////////////////////////////////////////
//  values of DRIVER_ERROR
// first of 2 hex digits indicates category

// general or system errors

#define NO_ERROR			    0x0	    // everything okay	
#define JOB_CANCELED		    0x1		// CANCEL chosen by user
#define SYSTEM_ERROR		    0x2     // something bad that should not have happened		
#define ALLOCMEM_ERROR          0x3     // failed to allocate memory
#define NO_PRINTER_SELECTED     0x4     // indicates improper calling sequence or unidi
#define INDEX_OUT_OF_RANGE      0x5     // what it says
#define ILLEGAL_RESOLUTION      0x6     // tried to set resolution at unacceptable value
#define NULL_POINTER            0x7     // supplied ptr was null
// build-related
// (items either absent from current build, or just bad index from client code)

#define UNSUPPORTED_PRINTER     0x10    // selected printer-type unsupported in build
#define UNSUPPORTED_PEN         0x11    // selected pen-type unsupported
#define TEXT_UNSUPPORTED        0x12    // no text allowed in current build
#define GRAPHICS_UNSUPPORTED    0x13    // no graphics allowed in current build
#define UNSUPPORTED_FONT        0x14    // font selection failed
#define	ILLEGAL_COORDS          0x15    // bad (x,y) passed to TextOut
#define UNSUPPORTED_FUNCTION    0x16    // bad selection for PerformPrinterFunction
#define ILLEGAL_PAPERSIZE       0x17    // papersize illegal for given hardware

// I/O related 
			
#define IO_ERROR                0x20                                            
#define BAD_DEVICE_ID		    0x21


// WARNINGS
// convention is that values < 0 can be ignored (at user's peril)
#define WARN_MODE_MISMATCH      -1      // printmode selection incompatible with pen, tray, etc.
#define WARN_DUPLEX             -2      // duplexer installed; our driver can't use it

///////////////////////////////////////////////////////////////////////////////////////

// ** Printer Status return values


typedef enum {    // used for DisplayPrinterStatus

	DISPLAY_PRINTING,
	DISPLAY_PRINTING_COMPLETE,
	DISPLAY_PRINTING_CANCELED,
	DISPLAY_OFFLINE,
	DISPLAY_BUSY,
	DISPLAY_OUT_OF_PAPER,
	DISPLAY_TOP_COVER_OPEN,
	DISPLAY_ERROR_TRAP,
	DISPLAY_NO_PRINTER_FOUND,
	DISPLAY_NO_PEN_DJ400,
	DISPLAY_NO_PEN_DJ600,
	DISPLAY_NO_COLOR_PEN,
	DISPLAY_NO_BLACK_PEN,
	DISPLAY_NO_PENS,
	DISPLAY_PHOTO_PEN_WARN,
	DISPLAY_PRINTER_NOT_SUPPORTED,
	DISPLAY_COMM_PROBLEM,
	DISPLAY_CANT_ID_PRINTER,
	ACCEPT_DEFAULT		// internal driver use only

} DISPLAY_STATUS;

// ** move this to models.h
const char ModelName[10][11]={"DJ400","DJ540","DJ600","DJ6xx","DJ6xxPhoto",
                              "DJ8xx","DJ9xx","DJ9xxVIP","DJ630","AP2100"};  
 
// ** move these to internal.h
// items from wtv_interp.h
#define NUMBER_PLANES   3 // 3 for RGB 4 for alphaRGB
#define NUMBER_RASTERS  3 // The number of Rasters to Buffer

// ** these don't seem to be used anymore - delete
// until resolution issue resolved
#define	PRINTABLEREGIONX 2400
#define PRINTABLEREGIONY 3000

//
// ** move these to intenal.h
struct fOptSubSig
{
    float pi;
    const float *means;
};
struct fOptClassSig
{
    int nsubclasses;
    float variance;
    float inv_variance;
    float cnst;
    struct fOptSubSig *OptSubSig;
};
struct fOptSigSet
{
    int nbands;
	struct fOptClassSig *OptClassSig;
};
typedef struct
{
		int				Width;
		int				ScaleFactorMultiplier;
		int				ScaleFactorDivisor;
//		int				CallerAlloc;		 //	Who does the memory alloc.
		int             Remainder;           // For use in non integer scaling cases
		int             Repeat;				 // When to send an extra output raster
		int             RastersinBuffer;     // # of currently buffered rasters 
		unsigned char*  Bufferpt[NUMBER_RASTERS];
		int				BufferSize;
		unsigned char*	Buffer;
		struct fOptSigSet OS;
		struct fOptSubSig rsOptSubSigPtr1[45];
		struct fOptClassSig OCS;
		float **joint_means;
		float ***filter;
		float filterPtr1[45];
		float filterPtr2[45][9];
		float joint_meansPtr1[45];

} RESSYNSTRUCT;


#endif // GLOBAL_TYPES_H
