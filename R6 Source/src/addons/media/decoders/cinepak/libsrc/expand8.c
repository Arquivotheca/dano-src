// ex:set sw=2 ts=8 wm=0:
// $Header: u:/rcs/cv/rcs/expand8.c 3.6 1994/05/12 12:48:22 timr Exp $

// (C) Copyright 1992-1993 SuperMac Technology, Inc.
// All rights reserved

// This source code and any compilation or derivative thereof is the sole
// property of SuperMac Technology, Inc. and is provided pursuant to a
// Software License Agreement.  This code is the proprietary information
// of SuperMac Technology and is confidential in nature.  Its use and
// dissemination by any party other than SuperMac Technology is strictly
// limited by the confidential information provisions of the Agreement
// referenced above.

// $Log: expand8.c $
// Revision 3.6  1994/05/12 12:48:22  timr
// Shift by 6 in the table instead of in the code.
//Revision 3.5  1994/05/11  18:43:15  unknown
//(bog)  Get running after grey scale movie changes.
//
//Revision 3.4  1994/05/09  14:02:50  timr
//Add black & white movie support.
//
//Revision 3.3  1994/03/17  10:42:46  timr
//Correct MIPS alignment faults.
//
//Revision 3.2  1994/03/03  12:00:58  timr
//Add code to support SET_PALETTE on non-x86 platforms.
//
//Revision 3.1  1994/02/23  07:12:26  timr
//Typo in smooth patch expansion.
//
//Revision 3.0  1993/12/10  06:24:18  timr
//Initial revision, NT C-only version.
//



#define	_WINDOWS
#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>

#define	EXPAND8_C

#include "cv.h"
#include "cvdecomp.h"

#include "palette.h"

#ifndef	NOBLACKWHITE
#include "greypal.h"
#endif

#ifdef __BEOS__
#include <ByteOrder.h>
#endif

/*
; Our 8 bpp dither scheme divides YUV space into prisms.  There are 16
; slots in Y and 8 in U and V.  Of the 16*8*8=1024 possible colors, only
; 232 are valid in RGB space; this number is conveniently less than the
; 236 slots we have available in a Windows palette since Windows
; reserves 20 colors.

; The dither happens as we quantize, when we map Y[0..255] into 0..15,
; U[0..255] into 0..7 and Y[0..255] into 0..7.  There are 4 versions of
; each of these mappings.  Version A is (Y1,U2,V0), B is (Y3,U1,V2), C
; is (Y0,U3,V1) and D is (Y2,U0,V3).  Across a scanline, we simply cycle
; through A,B,C,D,... and start each scanline with A,B,C,D,... so that
; we dither the same on 4x4 patches:

;   YUV
;    120 312 031 203
;    031 203 120 312
;    312 031 203 120
;    203 120 312 031

;   Y
;    1   3   0   2
;    0   2   1   3
;    3   0   2   1
;    2   1   3   0

;   U
;     2   1   3   0
;     3   0   2   1
;     1   3   0   2
;     0   2   1   3

;   V
;      0   2   1   3
;      1   3   0   2
;      2   1   3   0
;      3   0   2   1

; Once we have quantized the YUV into a 0..1023 index, we look up the
; dithered color in our palette table.  This produces an index into our
; standard dither palette.

; We save 2x2 detail pre dithered for the 4 possible positions in the
; destination 4x4.
*/

typedef void (*pSetFunction) (CODE *, DENTRY *, unsigned char *);

RGBQUAD stdPAL8[256];
extern QWORD ULookup [];
extern QWORD VLookup [];
extern QWORD YLookup [];

#define nY 16
#define nU 8
#define nV 8
#define YUVLOOKUPSIZE (nY*nU*nV)
const int nRgbLookup=YUVLOOKUPSIZE;
unsigned char RgbLookup [YUVLOOKUPSIZE];
unsigned char GreyByteLookup[256];
unsigned char GreyDwordDither[256][4];

void
SetDetail8 (CODE * pC, DENTRY * pD, unsigned char * pRgbLookup)
{
    // The [YUV]Lookup entries are stored with the least significant two
    // words in the "R" entry.  This is backwards from what we really 
    // want.  We swap the dwords within the subvert structure.

    union {
	struct {
	    unsigned long Y0L;
	    unsigned long Y0R;
	    unsigned long Y1L;
	    unsigned long Y1R;
	    unsigned long Y2L;
	    unsigned long Y2R;
	    unsigned long Y3L;
	    unsigned long Y3R;
	} u0;
#define Y0L u0.Y0L
#define Y0R u0.Y0R
#define Y1L u0.Y1L
#define Y1R u0.Y1R
#define Y2L u0.Y2L
#define Y2R u0.Y2R
#define Y3L u0.Y3L
#define Y3R u0.Y3R
	struct {
	    unsigned short index [16];
	} u1;
#define index u1.index
    } subvert;

    QWORD tUV;

    // Combine the U and V values, then mix in our 4 Ys.

    // tUV = 00 U2 V0 00 U1 V2 00 U3 V1 00 U0 V3
    tUV.L = 
        ULookup [(unsigned char) pC->U].L | 
	VLookup [(unsigned char) pC->V].L;
    tUV.R = 
	ULookup [(unsigned char) pC->U].R | 
	VLookup [(unsigned char) pC->V].R;

    subvert.Y0R = tUV.R | YLookup [pC->Y[0]].R;
    subvert.Y0L = tUV.L | YLookup [pC->Y[0]].L;
    subvert.Y1R = tUV.R | YLookup [pC->Y[1]].R;
    subvert.Y1L = tUV.L | YLookup [pC->Y[1]].L;
    subvert.Y2R = tUV.R | YLookup [pC->Y[2]].R;
    subvert.Y2L = tUV.L | YLookup [pC->Y[2]].L;
    subvert.Y3R = tUV.R | YLookup [pC->Y[3]].R;
    subvert.Y3L = tUV.L | YLookup [pC->Y[3]].L;

    // subvert now contains roughly the matrix product:
    // [ Yn1 ]
    // [ Yn3 ] X [ U2V0 U1V2 U3V1 U0V3 ]
    // [ Yn0 ]
    // [ Yn2 ]

    // Now distribute the 16 combined values to the codebook.

    pD->Index00 = pRgbLookup [subvert.index [ 0]];	// Y01 U2 V0
    pD->Index01 = pRgbLookup [subvert.index [ 5]];	// Y13 U1 V2
    pD->Index02 = pRgbLookup [subvert.index [10]];	// Y20 U3 V1
    pD->Index03 = pRgbLookup [subvert.index [15]];	// Y32 U0 V3
    pD->Index10 = pRgbLookup [subvert.index [ 2]];	// Y00 U3 V1
    pD->Index11 = pRgbLookup [subvert.index [ 7]];	// Y12 U0 V3
    pD->Index12 = pRgbLookup [subvert.index [ 8]];	// Y21 U2 V0
    pD->Index13 = pRgbLookup [subvert.index [13]];	// Y33 U1 V2
    pD->Index20 = pRgbLookup [subvert.index [ 1]];	// Y03 U1 V2
    pD->Index21 = pRgbLookup [subvert.index [ 6]];	// Y10 U3 V1
    pD->Index22 = pRgbLookup [subvert.index [11]];	// Y22 U0 V3
    pD->Index23 = pRgbLookup [subvert.index [12]];	// Y31 U2 V0
    pD->Index30 = pRgbLookup [subvert.index [ 3]];	// Y02 U0 V3
    pD->Index31 = pRgbLookup [subvert.index [ 4]];	// Y11 U2 V0
    pD->Index32 = pRgbLookup [subvert.index [ 9]];	// Y23 U1 V2
    pD->Index33 = pRgbLookup [subvert.index [14]];	// Y30 U3 V1
}


void
SetSmooth8 (CODE * pC, DENTRY * pD, unsigned char * pRgbLookup)
{
    union {
	struct {
	    unsigned long bY0L;
	    unsigned long bY0R;
	    unsigned long bY1L;
	    unsigned long bY1R;
	    unsigned long bY2L;
	    unsigned long bY2R;
	    unsigned long bY3L;
	    unsigned long bY3R;
	} u0;
#define bY0L u0.bY0L
#define bY0R u0.bY0R
#define bY1L u0.bY1L
#define bY1R u0.bY1R
#define bY2L u0.bY2L
#define bY2R u0.bY2R
#define bY3L u0.bY3L
#define bY3R u0.bY3R
	struct {
	    unsigned short bindex [16];
	} u1;
#define bindex u1.bindex
    } subvert;

    QWORD tUV;

    // Combine the U and V values, then mix in our 4 Ys.

    // tUV = 00 U2 V0 00 U1 V2 00 U3 V1 00 U0 V3
    tUV.L = 
        ULookup [(unsigned char) pC->U].L | 
	VLookup [(unsigned char) pC->V].L;
    tUV.R = 
	ULookup [(unsigned char) pC->U].R | 
	VLookup [(unsigned char) pC->V].R;

    subvert.bY0L = tUV.L | YLookup [pC->Y[0]].L;
    subvert.bY0R = tUV.R | YLookup [pC->Y[0]].R;
    subvert.bY1L = tUV.L | YLookup [pC->Y[1]].L;
    subvert.bY1R = tUV.R | YLookup [pC->Y[1]].R;
    subvert.bY2L = tUV.L | YLookup [pC->Y[2]].L;
    subvert.bY2R = tUV.R | YLookup [pC->Y[2]].R;
    subvert.bY3L = tUV.L | YLookup [pC->Y[3]].L;
    subvert.bY3R = tUV.R | YLookup [pC->Y[3]].R;

    // subvert now contains roughly the matrix product:
    // [ Yn1 ]
    // [ Yn3 ] X [ U2V0 U1V2 U3V1 U0V3 ]
    // [ Yn0 ]
    // [ Yn2 ]

    // Now distribute the 16 combined values to the codebook.

    pD->Index0 = pRgbLookup [subvert.bindex [ 0]];	// Y01 U2 V0
    pD->Index1 = pRgbLookup [subvert.bindex [ 1]];	// Y03 U1 V2
    pD->Index2 = pRgbLookup [subvert.bindex [ 6]];	// Y10 U3 V1
    pD->Index3 = pRgbLookup [subvert.bindex [ 7]];	// Y12 U0 V3
    pD->Index4 = pRgbLookup [subvert.bindex [ 2]];	// Y00 U3 V1
    pD->Index5 = pRgbLookup [subvert.bindex [ 3]];	// Y02 U0 V3
    pD->Index6 = pRgbLookup [subvert.bindex [ 4]];	// Y11 U2 V0
    pD->Index7 = pRgbLookup [subvert.bindex [ 5]];	// Y13 U1 V2
    pD->Index8 = pRgbLookup [subvert.bindex [ 9]];	// Y23 U1 V2
    pD->Index9 = pRgbLookup [subvert.bindex [10]];	// Y20 U3 V1
    pD->Indexa = pRgbLookup [subvert.bindex [15]];	// Y32 U0 V3
    pD->Indexb = pRgbLookup [subvert.bindex [12]];	// Y31 U2 V0
    pD->Indexc = pRgbLookup [subvert.bindex [11]];	// Y22 U0 V3
    pD->Indexd = pRgbLookup [subvert.bindex [ 8]];	// Y21 U2 V0
    pD->Indexe = pRgbLookup [subvert.bindex [13]];	// Y33 U1 V2
    pD->Indexf = pRgbLookup [subvert.bindex [14]];	// Y30 U3 V1
}


#ifndef	NOBLACKWHITE

void
SetDetailGrey8 (CODEBW * pC, DENTRY * pD)
{
    pD->Index00 = 
    pD->Index10 =
    pD->Index20 =
    pD->Index30 = GreyByteLookup [pC->Y[0]];
    pD->Index01 = 
    pD->Index11 =
    pD->Index21 =
    pD->Index31 = GreyByteLookup [pC->Y[1]];
    pD->Index02 = 
    pD->Index12 =
    pD->Index22 =
    pD->Index32 = GreyByteLookup [pC->Y[2]];
    pD->Index03 = 
    pD->Index13 =
    pD->Index23 =
    pD->Index33 = GreyByteLookup [pC->Y[3]];
}


void
SetSmoothGrey8 (CODEBW * pC, DENTRY * pD)
{
    pD->Index0 = GreyDwordDither [pC->Y[0]][0];
    pD->Index1 = GreyDwordDither [pC->Y[0]][1];
    pD->Index4 = GreyDwordDither [pC->Y[0]][2];
    pD->Index5 = GreyDwordDither [pC->Y[0]][3];
    pD->Index2 = GreyDwordDither [pC->Y[1]][0];
    pD->Index3 = GreyDwordDither [pC->Y[1]][1];
    pD->Index6 = GreyDwordDither [pC->Y[1]][2];
    pD->Index7 = GreyDwordDither [pC->Y[1]][3];
    pD->Index8 = GreyDwordDither [pC->Y[2]][0];
    pD->Index9 = GreyDwordDither [pC->Y[2]][1];
    pD->Indexc = GreyDwordDither [pC->Y[2]][2];
    pD->Indexd = GreyDwordDither [pC->Y[2]][3];
    pD->Indexa = GreyDwordDither [pC->Y[3]][0];
    pD->Indexb = GreyDwordDither [pC->Y[3]][1];
    pD->Indexe = GreyDwordDither [pC->Y[3]][2];
    pD->Indexf = GreyDwordDither [pC->Y[3]][3];
}

#endif


void far
ExpandDetailCodeBook8 (
    CODEBOOK * pCBin,
    DCODEBOOK * pCB,
    unsigned char * pRgbLookup
) {
    int cbType = pCBin->SizeType.sType;
    CODE * pC = pCBin->Code;
#ifndef	NOBLACKWHITE
    CODEBW * pCbw = ((CODEBOOKBW *) pCBin)->Code;
#endif
    DENTRY * pD = pCB->Entry;
    DECLARE_SWITCHES ();

    int iNumCodes;
    
    iNumCodes = swiz2 (B_LENDIAN_TO_HOST_INT32(pCBin->SizeType.SwizSize) >> 16) - sizeof (SIZETYPE);

    if (!iNumCodes)
        return;

    switch (cbType) {
	case kFullDBookType: {
	    for (; iNumCodes; iNumCodes -= sizeof(CODE))
	    {
		SetDetail8 (pC, pD, pRgbLookup);
		pD++;
		pC++;
	    }
	    break;
	}

	case kPartialDBookType: {
	    int i;
	    INIT_SWITCHES ();

	    // There will always be 256 bits of switches.

	    for (i = 0; i < 256; i++)
	    {
		if (NEXT_SWITCH (pC))
		{
		    SetDetail8 (pC, pD, pRgbLookup);
		    pC++;
		}
		pD++;
	    }
	    break;
	}

#ifndef	NOBLACKWHITE
	case kFullDBookType | kGreyBookBit: {

	    for (; iNumCodes; iNumCodes -= sizeof(CODEBW))
	    {
		SetDetailGrey8 (pCbw, pD);
		pD++;
		pCbw++;
	    }
	    break;
	}

	case kPartialDBookType | kGreyBookBit: {
	    int i;
	    INIT_SWITCHES ();

	    // There will always be 256 bits of switches.

	    for (i = 0; i < 256; i++)
	    {
		if (NEXT_SWITCH (pCbw))
		{
		    SetDetailGrey8 (pCbw, pD);
		    pCbw++;
		}
		pD++;
	    }
	    break;
	}
#endif

#ifdef	DEBUG
	default:
	    DebugBreak ();
#endif
    }

}



void far
ExpandSmoothCodeBook8 (
    CODEBOOK * pCBin,
    DCODEBOOK * pCB,
    unsigned char * pRgbLookup
) {
    int cbType = pCBin->SizeType.sType;
    CODE * pC = pCBin->Code;
#ifndef	NOBLACKWHITE
    CODEBW * pCbw = ((CODEBOOKBW *) pCBin)->Code;
#endif
    DENTRY * pD = pCB->Entry;
    DECLARE_SWITCHES ();

    int iNumCodes;
    
    iNumCodes = swiz2 (B_LENDIAN_TO_HOST_INT32(pCBin->SizeType.SwizSize) >> 16) - sizeof (SIZETYPE);

    if (!iNumCodes)
        return;

    switch (cbType) {
        case kFullSBookType: {
	    for (; iNumCodes; iNumCodes -= sizeof(CODE))
	    {
		SetSmooth8 (pC, pD, pRgbLookup);
		pD++;
		pC++;
	    }
	    break;
	}

	case kPartialSBookType: {
	    int i;
	    INIT_SWITCHES ();

	    // There will always be 256 bits of switches.

	    for (i = 0; i < 256; i++)
	    {
		if (NEXT_SWITCH (pC))
		{
		    SetSmooth8 (pC, pD, pRgbLookup);
		    pC++;
		}
		pD++;
	    }
	    break;
	}

#ifndef	NOBLACKWHITE
        case kFullSBookType | kGreyBookBit: {
	    for (; iNumCodes; iNumCodes -= sizeof(CODEBW))
	    {
		SetSmoothGrey8 (pCbw, pD);
		pD++;
		pCbw++;
	    }
	    break;
	}

	case kPartialSBookType | kGreyBookBit: {
	    int i;
	    INIT_SWITCHES ();

	    // There will always be 256 bits of switches.

	    for (i = 0; i < 256; i++)
	    {
		if (NEXT_SWITCH (pCbw))
		{
		    SetSmoothGrey8 (pCbw, pD);
		    pCbw++;
		}
		pD++;
	    }
	    break;
	}
#endif

#ifdef	DEBUG
 	default: {
	    DebugBreak ();
	}
#endif
    }
}
