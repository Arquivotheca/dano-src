// ex:set sw=2 ts=8 wm=0:
// $Header: u:/rcs/cv/rcs/expand15.c 3.4 1994/05/11 18:42:18 unknown Exp $

// (C) Copyright 1992-1993 SuperMac Technology, Inc.
// All rights reserved

// This source code and any compilation or derivative thereof is the sole
// property of SuperMac Technology, Inc. and is provided pursuant to a
// Software License Agreement.  This code is the proprietary information
// of SuperMac Technology and is confidential in nature.  Its use and
// dissemination by any party other than SuperMac Technology is strictly
// limited by the confidential information provisions of the Agreement
// referenced above.

// $Log: expand15.c $
// Revision 3.4  1994/05/11 18:42:18  unknown
// (bog)  Get running after grey scale movie changes.
//Revision 3.3  1994/05/09  16:29:23  timr
//Play back black and white movies.
//
//Revision 3.2  1994/03/17  10:42:21  timr
//Correct MIPS alignment faults.
//
//Revision 3.1  1994/03/03  12:01:54  timr
//Add code to support SET_PALETTE on non-x86 platforms.
//
//Revision 3.0  1993/12/10  14:24:11  timr
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

extern unsigned char ScaleTable5 [];




void far
ExpandDetailCodeBook15 (
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

		pD->ul15.Blue  = ScaleTable5 [64+ (short) pC->Y[0] + deltaB];
		pD->ul15.Green = ScaleTable5 [64+ (short) pC->Y[0] + deltaG];
		pD->ul15.Red   = ScaleTable5 [64+ (short) pC->Y[0] + deltaR];

		pD->ur15.Blue  = ScaleTable5 [64+ (short) pC->Y[1] + deltaB];
		pD->ur15.Green = ScaleTable5 [64+ (short) pC->Y[1] + deltaG];
		pD->ur15.Red   = ScaleTable5 [64+ (short) pC->Y[1] + deltaR];

		pD->ll15.Blue  = ScaleTable5 [64+ (short) pC->Y[2] + deltaB];
		pD->ll15.Green = ScaleTable5 [64+ (short) pC->Y[2] + deltaG];
		pD->ll15.Red   = ScaleTable5 [64+ (short) pC->Y[2] + deltaR];

		pD->lr15.Blue  = ScaleTable5 [64+ (short) pC->Y[3] + deltaB];
		pD->lr15.Green = ScaleTable5 [64+ (short) pC->Y[3] + deltaG];
		pD->lr15.Red   = ScaleTable5 [64+ (short) pC->Y[3] + deltaR];
		
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

		    pD->ul15.Blue  = ScaleTable5 [64+ (short) pC->Y[0] + deltaB];
		    pD->ul15.Green = ScaleTable5 [64+ (short) pC->Y[0] + deltaG];
		    pD->ul15.Red   = ScaleTable5 [64+ (short) pC->Y[0] + deltaR];

		    pD->ur15.Blue  = ScaleTable5 [64+ (short) pC->Y[1] + deltaB];
		    pD->ur15.Green = ScaleTable5 [64+ (short) pC->Y[1] + deltaG];
		    pD->ur15.Red   = ScaleTable5 [64+ (short) pC->Y[1] + deltaR];

		    pD->ll15.Blue  = ScaleTable5 [64+ (short) pC->Y[2] + deltaB];
		    pD->ll15.Green = ScaleTable5 [64+ (short) pC->Y[2] + deltaG];
		    pD->ll15.Red   = ScaleTable5 [64+ (short) pC->Y[2] + deltaR];

		    pD->lr15.Blue  = ScaleTable5 [64+ (short) pC->Y[3] + deltaB];
		    pD->lr15.Green = ScaleTable5 [64+ (short) pC->Y[3] + deltaG];
		    pD->lr15.Red   = ScaleTable5 [64+ (short) pC->Y[3] + deltaR];

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

		pD->ul15.Blue  =
		pD->ul15.Green =
		pD->ul15.Red   = pCbw->Y[0] >> 3;

		pD->ur15.Blue  =
		pD->ur15.Green =
		pD->ur15.Red   = pCbw->Y[1] >> 3;

		pD->ll15.Blue  =
		pD->ll15.Green =
		pD->ll15.Red   = pCbw->Y[2] >> 3;

		pD->lr15.Blue  =
		pD->lr15.Green =
		pD->lr15.Red   = pCbw->Y[3] >> 3;

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
		    pD->ul15.Blue  =
		    pD->ul15.Green =
		    pD->ul15.Red   = pCbw->Y[0] >> 3;

		    pD->ur15.Blue  =
		    pD->ur15.Green =
		    pD->ur15.Red   = pCbw->Y[1] >> 3;

		    pD->ll15.Blue  =
		    pD->ll15.Green =
		    pD->ll15.Red   = pCbw->Y[2] >> 3;

		    pD->lr15.Blue  =
		    pD->lr15.Green =
		    pD->lr15.Red   = pCbw->Y[3] >> 3;

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
ExpandSmoothCodeBook15 (
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

		pD->ull15.Blue  = ScaleTable5 [64+ (short) pC->Y[0] + deltaB];
		pD->ull15.Green = ScaleTable5 [64+ (short) pC->Y[0] + deltaG];
		pD->ull15.Red   = ScaleTable5 [64+ (short) pC->Y[0] + deltaR];
		pD->ulr15 = pD->ull15;

		pD->url15.Blue  = ScaleTable5 [64+ (short) pC->Y[1] + deltaB];
		pD->url15.Green = ScaleTable5 [64+ (short) pC->Y[1] + deltaG];
		pD->url15.Red   = ScaleTable5 [64+ (short) pC->Y[1] + deltaR];
		pD->urr15 = pD->url15;

		pD->lll15.Blue  = ScaleTable5 [64+ (short) pC->Y[2] + deltaB];
		pD->lll15.Green = ScaleTable5 [64+ (short) pC->Y[2] + deltaG];
		pD->lll15.Red   = ScaleTable5 [64+ (short) pC->Y[2] + deltaR];
		pD->llr15 = pD->lll15;

		pD->lrl15.Blue  = ScaleTable5 [64+ (short) pC->Y[3] + deltaB];
		pD->lrl15.Green = ScaleTable5 [64+ (short) pC->Y[3] + deltaG];
		pD->lrl15.Red   = ScaleTable5 [64+ (short) pC->Y[3] + deltaR];
		pD->lrr15 = pD->lrl15;

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

		    pD->ull15.Blue  = ScaleTable5 [64+ (short) pC->Y[0] + deltaB];
		    pD->ull15.Green = ScaleTable5 [64+ (short) pC->Y[0] + deltaG];
		    pD->ull15.Red   = ScaleTable5 [64+ (short) pC->Y[0] + deltaR];
		    pD->ulr15 = pD->ull15;

		    pD->url15.Blue  = ScaleTable5 [64+ (short) pC->Y[1] + deltaB];
		    pD->url15.Green = ScaleTable5 [64+ (short) pC->Y[1] + deltaG];
		    pD->url15.Red   = ScaleTable5 [64+ (short) pC->Y[1] + deltaR];
		    pD->urr15 = pD->url15;

		    pD->lll15.Blue  = ScaleTable5 [64+ (short) pC->Y[2] + deltaB];
		    pD->lll15.Green = ScaleTable5 [64+ (short) pC->Y[2] + deltaG];
		    pD->lll15.Red   = ScaleTable5 [64+ (short) pC->Y[2] + deltaR];
		    pD->llr15 = pD->lll15;

		    pD->lrl15.Blue  = ScaleTable5 [64+ (short) pC->Y[3] + deltaB];
		    pD->lrl15.Green = ScaleTable5 [64+ (short) pC->Y[3] + deltaG];
		    pD->lrl15.Red   = ScaleTable5 [64+ (short) pC->Y[3] + deltaR];
		    pD->lrr15 = pD->lrl15;

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

		pD->ull15.Blue  =
		pD->ull15.Green =
		pD->ull15.Red   = pCbw->Y[0] >> 3;
		pD->ulr15 = pD->ull15;

 		pD->url15.Blue  =
		pD->url15.Green =
		pD->url15.Red   = pCbw->Y[1] >> 3;
		pD->urr15 = pD->url15;

		pD->lll15.Blue  =
		pD->lll15.Green =
		pD->lll15.Red   = pCbw->Y[2] >> 3;
		pD->llr15 = pD->lll15;

		pD->lrl15.Blue  =
		pD->lrl15.Green =
		pD->lrl15.Red   = pCbw->Y[3] >> 3;
		pD->lrr15 = pD->lrl15;

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
		    pD->ull15.Blue  =
		    pD->ull15.Green =
		    pD->ull15.Red   = pCbw->Y[0] >> 3;
		    pD->ulr15 = pD->ull15;
	
		    pD->url15.Blue  =
		    pD->url15.Green =
		    pD->url15.Red   = pCbw->Y[1] >> 3;
		    pD->urr15 = pD->url15;

		    pD->lll15.Blue  =
		    pD->lll15.Green =
		    pD->lll15.Red   = pCbw->Y[2] >> 3;
		    pD->llr15 = pD->lll15;
	
		    pD->lrl15.Blue  =
		    pD->lrl15.Green =
		    pD->lrl15.Red   = pCbw->Y[3] >> 3;
		    pD->lrr15 = pD->lrl15;
	
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
