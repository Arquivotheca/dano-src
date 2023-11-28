// ex:set sw=2 ts=8 wm=0:
// $Header: u:/rcs/cv/rcs/expand16.c 3.4 1994/05/11 18:42:44 unknown Exp $

// (C) Copyright 1992-1993 SuperMac Technology, Inc.
// All rights reserved

// This source code and any compilation or derivative thereof is the sole
// property of SuperMac Technology, Inc. and is provided pursuant to a
// Software License Agreement.  This code is the proprietary information
// of SuperMac Technology and is confidential in nature.  Its use and
// dissemination by any party other than SuperMac Technology is strictly
// limited by the confidential information provisions of the Agreement
// referenced above.

// $Log: expand16.c $
// Revision 3.4  1994/05/11 18:42:44  unknown
// (bog)  Get running after grey scale movie changes.
//Revision 3.3  1994/05/09  16:29:34  timr
//Play back black and white movies.
//
//Revision 3.2  1994/03/17  10:42:28  timr
//Correct MIPS alignment faults.
//
//Revision 3.1  1994/03/03  12:01:48  timr
//Add code to support SET_PALETTE on non-x86 platforms.
//
//Revision 3.0  1993/12/10  14:24:13  timr
//Initial revision, NT C-only version.
//



#define	_WINDOWS
#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>

#include "cv.h"
#include "cvdecomp.h"

#ifdef __BEOS__
#include <ByteOrder.h>
#endif

unsigned char ScaleTable5 [] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// underflow
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
	0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
	0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,
	0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,
	0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
	0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,
	0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,
	0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,
	0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,
	0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,
	0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
	0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
	0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
	0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
	0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,
	0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,
	0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
	0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,
	0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,
	0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
	0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
	0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,

	0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,	// overflow
	0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,
	0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,
	0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,
	0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,
	0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,
	0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,
	0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f
};

unsigned char ScaleTable6 [] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// underflow
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

	0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,
	0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,
	0x04,0x04,0x04,0x04,0x05,0x05,0x05,0x05,
	0x06,0x06,0x06,0x06,0x07,0x07,0x07,0x07,
	0x08,0x08,0x08,0x08,0x09,0x09,0x09,0x09,
	0x0a,0x0a,0x0a,0x0a,0x0b,0x0b,0x0b,0x0b,
	0x0c,0x0c,0x0c,0x0c,0x0d,0x0d,0x0d,0x0d,
	0x0e,0x0e,0x0e,0x0e,0x0f,0x0f,0x0f,0x0f,
	0x10,0x10,0x10,0x10,0x11,0x11,0x11,0x11,
	0x12,0x12,0x12,0x12,0x13,0x13,0x13,0x13,
	0x14,0x14,0x14,0x14,0x15,0x15,0x15,0x15,
	0x16,0x16,0x16,0x16,0x17,0x17,0x17,0x17,
	0x18,0x18,0x18,0x18,0x19,0x19,0x19,0x19,
	0x1a,0x1a,0x1a,0x1a,0x1b,0x1b,0x1b,0x1b,
	0x1c,0x1c,0x1c,0x1c,0x1d,0x1d,0x1d,0x1d,
	0x1e,0x1e,0x1e,0x1e,0x1f,0x1f,0x1f,0x1f,
	0x20,0x20,0x20,0x20,0x21,0x21,0x21,0x21,
	0x22,0x22,0x22,0x22,0x23,0x23,0x23,0x23,
	0x24,0x24,0x24,0x24,0x25,0x25,0x25,0x25,
	0x26,0x26,0x26,0x26,0x27,0x27,0x27,0x27,
	0x28,0x28,0x28,0x28,0x29,0x29,0x29,0x29,
	0x2a,0x2a,0x2a,0x2a,0x2b,0x2b,0x2b,0x2b,
	0x2c,0x2c,0x2c,0x2c,0x2d,0x2d,0x2d,0x2d,
	0x2e,0x2e,0x2e,0x2e,0x2f,0x2f,0x2f,0x2f,
	0x30,0x30,0x30,0x30,0x31,0x31,0x31,0x31,
	0x32,0x32,0x32,0x32,0x33,0x33,0x33,0x33,
	0x34,0x34,0x34,0x34,0x35,0x35,0x35,0x35,
	0x36,0x36,0x36,0x36,0x37,0x37,0x37,0x37,
	0x38,0x38,0x38,0x38,0x39,0x39,0x39,0x39,
	0x3a,0x3a,0x3a,0x3a,0x3b,0x3b,0x3b,0x3b,
	0x3c,0x3c,0x3c,0x3c,0x3d,0x3d,0x3d,0x3d,
	0x3e,0x3e,0x3e,0x3e,0x3f,0x3f,0x3f,0x3f,

	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,	// overflow
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
	0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
};




void far
ExpandDetailCodeBook16 (
    CODEBOOK * pCBin,
    DCODEBOOK * pCB,
    unsigned char * unused
) {
    int cbType = pCBin->SizeType.sType;
    CODE * pC = pCBin->Code;
#ifndef	NOBLACKWHITE
    CODEBW * pCbw = ((CODEBOOKBW *)pCBin)->Code;
#endif
    DENTRY * pD = pCB->Entry;
    DECLARE_SWITCHES ();
    register signed short deltaB;
    register signed short deltaR;
    register signed short deltaG;

    int iNumCodes;

    iNumCodes = swiz2 (B_LENDIAN_TO_HOST_INT32(pCBin->SizeType.SwizSize) >> 16) - sizeof (SIZETYPE);

    if (!iNumCodes)
        return;

    switch (cbType) {
        case kFullDBookType: {
	    for (; iNumCodes; iNumCodes -= sizeof(CODE))
	    {

// We take the incoming YYYYUV at *pC and develop a 2x2 patch:
//
//     RGB0  RGB1
//     RGB2  RGB3
//
// as seen on the screen.
//
// The YUV is scaled:
//   Ri = Yi + V*2
//   Gi = Yi - (V+U/2)
//   Bi = Yi + U*2
//
// We put V*2 in deltaR, -(V+U/2) in deltaG, and U*2 in deltaB.

		deltaB = pC->U<<1;			// -255..255
		deltaR = pC->V<<1;			// -255..255
		deltaG = - (pC->V + (pC->U>>1));	// -191..191

		pD->ul.Blue  = ScaleTable5 [64+ (short) pC->Y[0] + deltaB];
		pD->ul.Green = ScaleTable6 [64+ (short) pC->Y[0] + deltaG];
		pD->ul.Red   = ScaleTable5 [64+ (short) pC->Y[0] + deltaR];

		pD->ur.Blue  = ScaleTable5 [64+ (short) pC->Y[1] + deltaB];
		pD->ur.Green = ScaleTable6 [64+ (short) pC->Y[1] + deltaG];
		pD->ur.Red   = ScaleTable5 [64+ (short) pC->Y[1] + deltaR];

		pD->ll.Blue  = ScaleTable5 [64+ (short) pC->Y[2] + deltaB];
		pD->ll.Green = ScaleTable6 [64+ (short) pC->Y[2] + deltaG];
		pD->ll.Red   = ScaleTable5 [64+ (short) pC->Y[2] + deltaR];

		pD->lr.Blue  = ScaleTable5 [64+ (short) pC->Y[3] + deltaB];
		pD->lr.Green = ScaleTable6 [64+ (short) pC->Y[3] + deltaG];
		pD->lr.Red   = ScaleTable5 [64+ (short) pC->Y[3] + deltaR];

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
		    deltaB = pC->U<<1;			// -255..255
		    deltaR = pC->V<<1;			// -255..255
		    deltaG = - (pC->V + (pC->U>>1));		// -191..191

		    pD->ul.Blue  = ScaleTable5 [64+ (short) pC->Y[0] + deltaB];
		    pD->ul.Green = ScaleTable6 [64+ (short) pC->Y[0] + deltaG];
		    pD->ul.Red   = ScaleTable5 [64+ (short) pC->Y[0] + deltaR];

		    pD->ur.Blue  = ScaleTable5 [64+ (short) pC->Y[1] + deltaB];
		    pD->ur.Green = ScaleTable6 [64+ (short) pC->Y[1] + deltaG];
		    pD->ur.Red   = ScaleTable5 [64+ (short) pC->Y[1] + deltaR];

		    pD->ll.Blue  = ScaleTable5 [64+ (short) pC->Y[2] + deltaB];
		    pD->ll.Green = ScaleTable6 [64+ (short) pC->Y[2] + deltaG];
		    pD->ll.Red   = ScaleTable5 [64+ (short) pC->Y[2] + deltaR];

		    pD->lr.Blue  = ScaleTable5 [64+ (short) pC->Y[3] + deltaB];
		    pD->lr.Green = ScaleTable6 [64+ (short) pC->Y[3] + deltaG];
		    pD->lr.Red   = ScaleTable5 [64+ (short) pC->Y[3] + deltaR];

		    pC++;
		}
		pD++;
	    }
	    break;
	}

#ifndef	NOBLACKWHITE
        case kFullDBookType + kGreyBookBit: {
	    for (; iNumCodes; iNumCodes -= sizeof(CODEBW))
	    {

// We take the incoming YYYYUV at *pC and develop a 2x2 patch:
//
//     RGB0  RGB1
//     RGB2  RGB3
//
// as seen on the screen.

		pD->ul.Blue  = pCbw->Y[0] >> 3;
		pD->ul.Green = pCbw->Y[0] >> 2;
		pD->ul.Red   = pCbw->Y[0] >> 3;

		pD->ur.Blue  = pCbw->Y[1] >> 3;
		pD->ur.Green = pCbw->Y[1] >> 2;
		pD->ur.Red   = pCbw->Y[1] >> 3;

		pD->ll.Blue  = pCbw->Y[2] >> 3;
		pD->ll.Green = pCbw->Y[2] >> 2;
		pD->ll.Red   = pCbw->Y[2] >> 3;

		pD->lr.Blue  = pCbw->Y[3] >> 3;
		pD->lr.Green = pCbw->Y[3] >> 2;
		pD->lr.Red   = pCbw->Y[3] >> 3;

		pD++;
		pCbw++;
	    }
	    break;
	}

	case kPartialDBookType + kGreyBookBit: {
	    int i;
	    INIT_SWITCHES ();

	    // There will always be 256 bits of switches.

	    for (i = 0; i < 256; i++)
	    {
		if (NEXT_SWITCH (pCbw))
		{
		    pD->ul.Blue  = pCbw->Y[0] >> 3;
		    pD->ul.Green = pCbw->Y[0] >> 2;
		    pD->ul.Red   = pCbw->Y[0] >> 3;

		    pD->ur.Blue  = pCbw->Y[1] >> 3;
		    pD->ur.Green = pCbw->Y[1] >> 2;
		    pD->ur.Red   = pCbw->Y[1] >> 3;

		    pD->ll.Blue  = pCbw->Y[2] >> 3;
		    pD->ll.Green = pCbw->Y[2] >> 2;
		    pD->ll.Red   = pCbw->Y[2] >> 3;

		    pD->lr.Blue  = pCbw->Y[3] >> 3;
		    pD->lr.Green = pCbw->Y[3] >> 2;
		    pD->lr.Red   = pCbw->Y[3] >> 3;

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



void far
ExpandSmoothCodeBook16 (
    CODEBOOK * pCBin,
    DCODEBOOK * pCB,
    unsigned char * unused
) {
    int cbType = pCBin->SizeType.sType;
    CODE * pC = pCBin->Code;
#ifndef	NOBLACKWHITE
    CODEBW * pCbw = ((CODEBOOKBW *)pCBin)->Code;
#endif
    DENTRY * pD = pCB->Entry;
    DECLARE_SWITCHES ();
    register signed short deltaB;
    register signed short deltaR;
    register signed short deltaG;

    int iNumCodes;

    iNumCodes = swiz2 (B_LENDIAN_TO_HOST_INT32(pCBin->SizeType.SwizSize) >> 16) - sizeof (SIZETYPE);

    if (!iNumCodes)
        return;

    switch (cbType) {
        case kFullSBookType: {
	    for (; iNumCodes; iNumCodes -= sizeof(CODE))
	    {

// We take the incoming YYYYUV at *pC and develop a 4x4 patch:
//
//     RGB0 RGB0  RGB1 RGB1
//     RGB0 RGB0  RGB1 RGB1
//     RGB2 RGB2  RGB3 RGB3
//     RGB2 RGB2  RGB3 RGB3
//
// as seen on the screen.
//
// The YUV is scaled:
//   Ri = Yi + V*2
//   Gi = Yi - (V+U/2)
//   Bi = Yi + U*2
//
// We put V*2 in deltaR, -(V+U/2) in deltaG, and U*2 in deltaB.

		deltaB = pC->U<<1;                 // -255..255
		deltaR = pC->V<<1;                 // -255..255
		deltaG = - (pC->V + (pC->U>>1));     // -191..191

		pD->ull.Blue  = ScaleTable5 [64+ (short) pC->Y[0] + deltaB];
		pD->ull.Green = ScaleTable6 [64+ (short) pC->Y[0] + deltaG];
		pD->ull.Red   = ScaleTable5 [64+ (short) pC->Y[0] + deltaR];
		pD->ulr = pD->ull;

		pD->url.Blue  = ScaleTable5 [64+ (short) pC->Y[1] + deltaB];
		pD->url.Green = ScaleTable6 [64+ (short) pC->Y[1] + deltaG];
		pD->url.Red   = ScaleTable5 [64+ (short) pC->Y[1] + deltaR];
		pD->urr = pD->url;

		pD->lll.Blue  = ScaleTable5 [64+ (short) pC->Y[2] + deltaB];
		pD->lll.Green = ScaleTable6 [64+ (short) pC->Y[2] + deltaG];
		pD->lll.Red   = ScaleTable5 [64+ (short) pC->Y[2] + deltaR];
		pD->llr = pD->lll;

		pD->lrl.Blue  = ScaleTable5 [64+ (short) pC->Y[3] + deltaB];
		pD->lrl.Green = ScaleTable6 [64+ (short) pC->Y[3] + deltaG];
		pD->lrl.Red   = ScaleTable5 [64+ (short) pC->Y[3] + deltaR];
		pD->lrr = pD->lrl;

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
		    deltaB = pC->U<<1;			// -255..255
		    deltaR = pC->V<<1;			// -255..255
		    deltaG = - (pC->V + (pC->U>>1));		// -191..191

		    pD->ull.Blue  = ScaleTable5 [64+ (short) pC->Y[0] + deltaB];
		    pD->ull.Green = ScaleTable6 [64+ (short) pC->Y[0] + deltaG];
		    pD->ull.Red   = ScaleTable5 [64+ (short) pC->Y[0] + deltaR];
		    pD->ulr = pD->ull;

		    pD->url.Blue  = ScaleTable5 [64+ (short) pC->Y[1] + deltaB];
		    pD->url.Green = ScaleTable6 [64+ (short) pC->Y[1] + deltaG];
		    pD->url.Red   = ScaleTable5 [64+ (short) pC->Y[1] + deltaR];
		    pD->urr = pD->url;

		    pD->lll.Blue  = ScaleTable5 [64+ (short) pC->Y[2] + deltaB];
		    pD->lll.Green = ScaleTable6 [64+ (short) pC->Y[2] + deltaG];
		    pD->lll.Red   = ScaleTable5 [64+ (short) pC->Y[2] + deltaR];
		    pD->llr = pD->lll;

		    pD->lrl.Blue  = ScaleTable5 [64+ (short) pC->Y[3] + deltaB];
		    pD->lrl.Green = ScaleTable6 [64+ (short) pC->Y[3] + deltaG];
		    pD->lrl.Red   = ScaleTable5 [64+ (short) pC->Y[3] + deltaR];
		    pD->lrr = pD->lrl;

		    pC++;
		}
		pD++;
	    }
	    break;
	}

#ifndef	NOBLACKWHITE
        case kFullSBookType + kGreyBookBit: {
	    for (; iNumCodes; iNumCodes -= sizeof(CODEBW))
	    {

// We take the incoming YYYYUV at *pC and develop a 4x4 patch:
//
//     RGB0 RGB0  RGB1 RGB1
//     RGB0 RGB0  RGB1 RGB1
//     RGB2 RGB2  RGB3 RGB3
//     RGB2 RGB2  RGB3 RGB3
//
// as seen on the screen.

		pD->ull.Blue  = pCbw->Y[0] >> 3;
		pD->ull.Green = pCbw->Y[0] >> 2;
		pD->ull.Red   = pCbw->Y[0] >> 3;
		pD->ulr = pD->ull;

 		pD->url.Blue  = pCbw->Y[1] >> 3;
		pD->url.Green = pCbw->Y[1] >> 2;
		pD->url.Red   = pCbw->Y[1] >> 3;
		pD->urr = pD->url;

		pD->lll.Blue  = pCbw->Y[2] >> 3;
		pD->lll.Green = pCbw->Y[2] >> 2;
		pD->lll.Red   = pCbw->Y[2] >> 3;
		pD->llr = pD->lll;

		pD->lrl.Blue  = pCbw->Y[3] >> 3;
		pD->lrl.Green = pCbw->Y[3] >> 2;
		pD->lrl.Red   = pCbw->Y[3] >> 3;
		pD->lrr = pD->lrl;

		pD++;
		pCbw++;
	    }
	    break;
	}

	case kPartialSBookType + kGreyBookBit: {
	    int i;
	    INIT_SWITCHES ();

	    // There will always be 256 bits of switches.

	    for (i = 0; i < 256; i++)
	    {
		if (NEXT_SWITCH (pCbw))
		{
		    pD->ull.Blue  = pCbw->Y[0] >> 3;
		    pD->ull.Green = pCbw->Y[0] >> 2;
		    pD->ull.Red   = pCbw->Y[0] >> 3;
		    pD->ulr = pD->ull;
	
		    pD->url.Blue  = pCbw->Y[1] >> 3;
		    pD->url.Green = pCbw->Y[1] >> 2;
		    pD->url.Red   = pCbw->Y[1] >> 3;
		    pD->urr = pD->url;
	
		    pD->lll.Blue  = pCbw->Y[2] >> 3;
		    pD->lll.Green = pCbw->Y[2] >> 2;
		    pD->lll.Red   = pCbw->Y[2] >> 3;
		    pD->llr = pD->lll;
	
		    pD->lrl.Blue  = pCbw->Y[3] >> 3;
		    pD->lrl.Green = pCbw->Y[3] >> 2;
		    pD->lrl.Red   = pCbw->Y[3] >> 3;
		    pD->lrr = pD->lrl;
	
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

