/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/expand32.c 3.6 1994/10/20 17:40:04 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the sole
 * property of SuperMac Technology, Inc. and is provided pursuant to a
 * Software License Agreement.  This code is the proprietary information
 * of SuperMac Technology and is confidential in nature.  Its use and
 * dissemination by any party other than SuperMac Technology is strictly
 * limited by the confidential information provisions of the Agreement
 * referenced above.

 * $Log: expand32.c $
 * Revision 3.6  1994/10/20 17:40:04  bog
 * Modifications to support Sunnyvale Reference version.
 * Revision 3.5  1994/05/11  18:43:06  unknown
 * (bog)  Get running after grey scale movie changes.
 *
 * Revision 3.4  1994/05/09  18:26:31  unknown
 * Cast typo.
 *
 * Revision 3.3  1994/05/09  16:29:46  timr
 * Play back black and white movies.
 *
 * Revision 3.2  1994/03/17  10:42:43  timr
 * Correct MIPS alignment faults.
 *
 * Revision 3.1  1994/03/03  12:01:17  timr
 * Add code to support SET_PALETTE on non-x86 platforms.
 *
 * Revision 3.0  1993/12/10  14:24:16  timr
 * Initial revision, NT C-only version.
 */



#if	defined(WINCPK)

#define	_WINDOWS
#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>

#endif
#ifdef __BEOS__
#include <ByteOrder.h>
#endif

#include "cv.h"
#include "cvdecomp.h"



extern unsigned char Bounding24[];// in expand24.c

extern char *buffer;
extern char *obuffer;

void far
ExpandCodeBook32 (
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
#if	defined(WINCPK) || defined(__BEOS__)

    iNumCodes = swiz2 (B_LENDIAN_TO_HOST_INT32(pCBin->SizeType.SwizSize) >> 16) - sizeof (SIZETYPE);

#else

    iNumCodes = (pCBin->SizeType.SwizSize & 0x00ffffff) - sizeof (SIZETYPE);

#endif
    if (!iNumCodes)
        return;

    switch (cbType) {
	case kFullDBookType:
	case kFullSBookType: {
		int z=0;
	    for (; iNumCodes; iNumCodes -= sizeof(CODE))
	    {

// For detail, we take the incoming YYYYUV at *pC and develop a 2x2 patch:
//
//     RGB0  RGB1
//     RGB2  RGB3
//
// as seen on the screen.

// For smooth, we take the incoming YYYYUV at *pC and develop a 4x4 patch:
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
		deltaB = pC->U<<1;			// -255..255
		deltaR = pC->V<<1;			// -255..255
		deltaG = - (pC->V + (pC->U>>1));	// -191..191

		pD->Pixel0.Blue  = Bounding24 [64+ (short) pC->Y[0] + deltaB];
		pD->Pixel0.Green = Bounding24 [64+ (short) pC->Y[0] + deltaG];
		pD->Pixel0.Red   = Bounding24 [64+ (short) pC->Y[0] + deltaR];

		pD->Pixel1.Blue  = Bounding24 [64+ (short) pC->Y[1] + deltaB];
		pD->Pixel1.Green = Bounding24 [64+ (short) pC->Y[1] + deltaG];
		pD->Pixel1.Red   = Bounding24 [64+ (short) pC->Y[1] + deltaR];

		pD->Pixel2.Blue  = Bounding24 [64+ (short) pC->Y[2] + deltaB];
		pD->Pixel2.Green = Bounding24 [64+ (short) pC->Y[2] + deltaG];
		pD->Pixel2.Red   = Bounding24 [64+ (short) pC->Y[2] + deltaR];

		pD->Pixel3.Blue  = Bounding24 [64+ (short) pC->Y[3] + deltaB];
		pD->Pixel3.Green = Bounding24 [64+ (short) pC->Y[3] + deltaG];
		pD->Pixel3.Red   = Bounding24 [64+ (short) pC->Y[3] + deltaR];

//if (++z==15) pD->Pixel0.Red=pD->Pixel0.Green=pD->Pixel0.Blue=255;
		pD++;
		pC++;
	    }
	    break;
	}

	case kPartialDBookType:
	case kPartialSBookType: {
	    int i;
	    INIT_SWITCHES ();

	    // There will always be 256 bits of switches.

	    for (i = 0; i < 256; i++)
	    {
		if (NEXT_SWITCH (pC))
		{
		    deltaB = pC->U * 2;			// -255..255
		    deltaR = pC->V * 2;			// -255..255
		    deltaG = - (pC->V + pC->U / 2);	// -191..191

		    pD->Pixel0.Blue  = Bounding24 [64+ (short) pC->Y[0] + deltaB];
		    pD->Pixel0.Green = Bounding24 [64+ (short) pC->Y[0] + deltaG];
		    pD->Pixel0.Red   = Bounding24 [64+ (short) pC->Y[0] + deltaR];

		    pD->Pixel1.Blue  = Bounding24 [64+ (short) pC->Y[1] + deltaB];
		    pD->Pixel1.Green = Bounding24 [64+ (short) pC->Y[1] + deltaG];
		    pD->Pixel1.Red   = Bounding24 [64+ (short) pC->Y[1] + deltaR];

		    pD->Pixel2.Blue  = Bounding24 [64+ (short) pC->Y[2] + deltaB];
		    pD->Pixel2.Green = Bounding24 [64+ (short) pC->Y[2] + deltaG];
		    pD->Pixel2.Red   = Bounding24 [64+ (short) pC->Y[2] + deltaR];

		    pD->Pixel3.Blue  = Bounding24 [64+ (short) pC->Y[3] + deltaB];
		    pD->Pixel3.Green = Bounding24 [64+ (short) pC->Y[3] + deltaG];
		    pD->Pixel3.Red   = Bounding24 [64+ (short) pC->Y[3] + deltaR];

		    pC++;
		}
		pD++;
	    }
	    break;
	}

#ifndef	NOBLACKWHITE
	case kFullDBookType + kGreyBookBit:
	case kFullSBookType + kGreyBookBit: {
//	printf("Gray! %02x %02x %02x %02x\n",pCbw->Y[0],pCbw->Y[1],pCbw->Y[2],pCbw->Y[3]);
	    for (; iNumCodes; iNumCodes -= sizeof(CODEBW))
	    {
//	printf("Gray! %02x %02x %02x %02x\n",pCbw->Y[0],pCbw->Y[1],pCbw->Y[2],pCbw->Y[3]);

// For detail, we take the incoming YYYYUV at *pC and develop a 2x2 patch:
//
//     RGB0  RGB1
//     RGB2  RGB3
//
// as seen on the screen.

// For smooth, we take the incoming YYYYUV at *pC and develop a 4x4 patch:
//
//     RGB0 RGB0  RGB1 RGB1
//     RGB0 RGB0  RGB1 RGB1
//     RGB2 RGB2  RGB3 RGB3
//     RGB2 RGB2  RGB3 RGB3
//
// as seen on the screen.

		pD->Pixel0.Blue  =
		pD->Pixel0.Green =
		pD->Pixel0.Red   = pCbw->Y[0];

		pD->Pixel1.Blue  =
		pD->Pixel1.Green =
		pD->Pixel1.Red   = pCbw->Y[1];

		pD->Pixel2.Blue  =
		pD->Pixel2.Green =
		pD->Pixel2.Red   = pCbw->Y[2];

		pD->Pixel3.Blue  =
		pD->Pixel3.Green =
		pD->Pixel3.Red   = pCbw->Y[3];

		pD++;
		pCbw++;
	    }
	    break;
	}

	case kPartialDBookType + kGreyBookBit:
	case kPartialSBookType + kGreyBookBit: {
	    int i;
	    INIT_SWITCHES ();
//printf("partial grey\n");
	    // There will always be 256 bits of switches.

	    for (i = 0; i < 256; i++)
	    {
		if (NEXT_SWITCH (pCbw))
		{
                    pD->Pixel0.Blue  =
		    pD->Pixel0.Green =
		    pD->Pixel0.Red   = pCbw->Y[0];
	
		    pD->Pixel1.Blue  =
		    pD->Pixel1.Green =
		    pD->Pixel1.Red   = pCbw->Y[1];
	
		    pD->Pixel2.Blue  =
		    pD->Pixel2.Green =
		    pD->Pixel2.Red   = pCbw->Y[2];
	
		    pD->Pixel3.Blue  =
		    pD->Pixel3.Green =
		    pD->Pixel3.Red   = pCbw->Y[3];

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
