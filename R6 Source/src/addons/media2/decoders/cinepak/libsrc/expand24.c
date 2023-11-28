// ex:set sw=2 ts=8 wm=0:
// $Header: u:/rcs/cv/rcs/expand24.c 3.5 1994/05/11 18:42:56 unknown Exp $

// (C) Copyright 1992-1993 SuperMac Technology, Inc.
// All rights reserved

// This source code and any compilation or derivative thereof is the sole
// property of SuperMac Technology, Inc. and is provided pursuant to a
// Software License Agreement.  This code is the proprietary information
// of SuperMac Technology and is confidential in nature.  Its use and
// dissemination by any party other than SuperMac Technology is strictly
// limited by the confidential information provisions of the Agreement
// referenced above.

// $Log: expand24.c $
// Revision 3.5  1994/05/11 18:42:56  unknown
// (bog)  Get running after grey scale movie changes.
//Revision 3.4  1994/05/09  16:29:40  timr
//Play back black and white movies.
//
//Revision 3.3  1994/03/17  10:42:39  timr
//Correct MIPS alignment faults.
//
//Revision 3.2  1994/03/03  12:01:40  timr
//Add code to support SET_PALETTE on non-x86 platforms.
//
//Revision 3.1  1994/01/13  02:15:34  geoffs
//Fix up DEBUG stuff for all C version
//
//Revision 3.0  1993/12/10  14:24:15  timr
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

unsigned char Bounding24 [] = {
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000, // underflow
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,

	0x000,0x001,0x002,0x003,0x004,0x005,0x006,0x007,
	0x008,0x009,0x00a,0x00b,0x00c,0x00d,0x00e,0x00f,
	0x010,0x011,0x012,0x013,0x014,0x015,0x016,0x017,
	0x018,0x019,0x01a,0x01b,0x01c,0x01d,0x01e,0x01f,
	0x020,0x021,0x022,0x023,0x024,0x025,0x026,0x027,
	0x028,0x029,0x02a,0x02b,0x02c,0x02d,0x02e,0x02f,
	0x030,0x031,0x032,0x033,0x034,0x035,0x036,0x037,
	0x038,0x039,0x03a,0x03b,0x03c,0x03d,0x03e,0x03f,
	0x040,0x041,0x042,0x043,0x044,0x045,0x046,0x047,
	0x048,0x049,0x04a,0x04b,0x04c,0x04d,0x04e,0x04f,
	0x050,0x051,0x052,0x053,0x054,0x055,0x056,0x057,
	0x058,0x059,0x05a,0x05b,0x05c,0x05d,0x05e,0x05f,
	0x060,0x061,0x062,0x063,0x064,0x065,0x066,0x067,
	0x068,0x069,0x06a,0x06b,0x06c,0x06d,0x06e,0x06f,
	0x070,0x071,0x072,0x073,0x074,0x075,0x076,0x077,
	0x078,0x079,0x07a,0x07b,0x07c,0x07d,0x07e,0x07f,
	0x080,0x081,0x082,0x083,0x084,0x085,0x086,0x087,
	0x088,0x089,0x08a,0x08b,0x08c,0x08d,0x08e,0x08f,
	0x090,0x091,0x092,0x093,0x094,0x095,0x096,0x097,
	0x098,0x099,0x09a,0x09b,0x09c,0x09d,0x09e,0x09f,
	0x0a0,0x0a1,0x0a2,0x0a3,0x0a4,0x0a5,0x0a6,0x0a7,
	0x0a8,0x0a9,0x0aa,0x0ab,0x0ac,0x0ad,0x0ae,0x0af,
	0x0b0,0x0b1,0x0b2,0x0b3,0x0b4,0x0b5,0x0b6,0x0b7,
	0x0b8,0x0b9,0x0ba,0x0bb,0x0bc,0x0bd,0x0be,0x0bf,
	0x0c0,0x0c1,0x0c2,0x0c3,0x0c4,0x0c5,0x0c6,0x0c7,
	0x0c8,0x0c9,0x0ca,0x0cb,0x0cc,0x0cd,0x0ce,0x0cf,
	0x0d0,0x0d1,0x0d2,0x0d3,0x0d4,0x0d5,0x0d6,0x0d7,
	0x0d8,0x0d9,0x0da,0x0db,0x0dc,0x0dd,0x0de,0x0df,
	0x0e0,0x0e1,0x0e2,0x0e3,0x0e4,0x0e5,0x0e6,0x0e7,
	0x0e8,0x0e9,0x0ea,0x0eb,0x0ec,0x0ed,0x0ee,0x0ef,
	0x0f0,0x0f1,0x0f2,0x0f3,0x0f4,0x0f5,0x0f6,0x0f7,
	0x0f8,0x0f9,0x0fa,0x0fb,0x0fc,0x0fd,0x0fe,0x0ff,

	0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff, //overflow
	0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,
	0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,
	0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,
	0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,
	0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,
	0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,
	0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff
};





void far
ExpandDetailCodeBook24 (
    CODEBOOK * pCBin,
    DCODEBOOK * pCB,
    unsigned char * unused
) {
    int cbType = pCBin->SizeType.sType;
    CODE * pC = pCBin->Code;
#ifndef	NOBLACKWHITE
    CODEBW * pCbw = ((CODEBOOKBW *) pCBin)->Code;
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

		deltaB = pC->U * 2;			// -255..255
		deltaR = pC->V * 2;			// -255..255
		deltaG = - (pC->V + pC->U / 2);	// -191..191

		pD->Blue0in0  = Bounding24 [64+ (short) pC->Y[0] + deltaB];
		pD->Green0in0 = Bounding24 [64+ (short) pC->Y[0] + deltaG];
		pD->Red0in0   =
		pD->Red0in1   = Bounding24 [64+ (short) pC->Y[0] + deltaR];

		pD->Blue1in0  =
		pD->Blue1in1  = Bounding24 [64+ (short) pC->Y[1] + deltaB];
		pD->Green1in1 = Bounding24 [64+ (short) pC->Y[1] + deltaG];
		pD->Red1in1   = Bounding24 [64+ (short) pC->Y[1] + deltaR];

		pD->Blue2in2  = Bounding24 [64+ (short) pC->Y[2] + deltaB];
		pD->Green2in2 = Bounding24 [64+ (short) pC->Y[2] + deltaG];
		pD->Red2in2   =
		pD->Red2in3   = Bounding24 [64+ (short) pC->Y[2] + deltaR];

		pD->Blue3in2  =
		pD->Blue3in3  = Bounding24 [64+ (short) pC->Y[3] + deltaB];
		pD->Green3in3 = Bounding24 [64+ (short) pC->Y[3] + deltaG];
		pD->Red3in3   = Bounding24 [64+ (short) pC->Y[3] + deltaR];

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
		    deltaB = pC->U * 2;			// -255..255
		    deltaR = pC->V * 2;			// -255..255
		    deltaG = - (pC->V + pC->U / 2);		// -191..191

		    pD->Blue0in0  = Bounding24 [64+ (short) pC->Y[0] + deltaB];
		    pD->Green0in0 = Bounding24 [64+ (short) pC->Y[0] + deltaG];
		    pD->Red0in0   =
		    pD->Red0in1   = Bounding24 [64+ (short) pC->Y[0] + deltaR];

		    pD->Blue1in0  =
		    pD->Blue1in1  = Bounding24 [64+ (short) pC->Y[1] + deltaB];
		    pD->Green1in1 = Bounding24 [64+ (short) pC->Y[1] + deltaG];
		    pD->Red1in1   = Bounding24 [64+ (short) pC->Y[1] + deltaR];

		    pD->Blue2in2  = Bounding24 [64+ (short) pC->Y[2] + deltaB];
		    pD->Green2in2 = Bounding24 [64+ (short) pC->Y[2] + deltaG];
		    pD->Red2in2   =
		    pD->Red2in3   = Bounding24 [64+ (short) pC->Y[2] + deltaR];

		    pD->Blue3in2  =
		    pD->Blue3in3  = Bounding24 [64+ (short) pC->Y[3] + deltaB];
		    pD->Green3in3 = Bounding24 [64+ (short) pC->Y[3] + deltaG];
		    pD->Red3in3   = Bounding24 [64+ (short) pC->Y[3] + deltaR];

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

// We take the incoming YYYYUV at *pCbw and develop a 2x2 patch:
//
//     RGB0  RGB1
//     RGB2  RGB3
//
// as seen on the screen.

		pD->Blue0in0  =
		pD->Green0in0 =
		pD->Red0in0   =
		pD->Red0in1   = pCbw->Y[0];

		pD->Blue1in0  =
		pD->Blue1in1  =
		pD->Green1in1 =
		pD->Red1in1   = pCbw->Y[1];

		pD->Blue2in2  =
		pD->Green2in2 =
		pD->Red2in2   =
		pD->Red2in3   = pCbw->Y[2];

		pD->Blue3in2  =
		pD->Blue3in3  =
		pD->Green3in3 =
		pD->Red3in3   = pCbw->Y[3];

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
		    pD->Blue0in0  =
		    pD->Green0in0 =
		    pD->Red0in0   =
		    pD->Red0in1   = pCbw->Y[0];

		    pD->Blue1in0  =
		    pD->Blue1in1  =
		    pD->Green1in1 =
		    pD->Red1in1   = pCbw->Y[1];

		    pD->Blue2in2  =
		    pD->Green2in2 =
		    pD->Red2in2   =
		    pD->Red2in3   = pCbw->Y[2];

		    pD->Blue3in2  =
		    pD->Blue3in3  =
		    pD->Green3in3 =
		    pD->Red3in3   = pCbw->Y[3];

		    pCbw++;
		}
		pD++;
	    }
	    break;
	}
#endif

#ifdef	DEBUG
        default: {
	    DebugBreak();
	}
#endif
    }
}



void far
ExpandSmoothCodeBook24 (
    CODEBOOK * pCBin,
    DCODEBOOK * pCB,
    unsigned char * unused
) {
    int cbType = pCBin->SizeType.sType;
    CODE * pC = pCBin->Code;
#ifndef	NOBLACKWHITE
    CODEBW * pCbw = ((CODEBOOKBW *) pCBin)->Code;
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

		deltaB = pC->U * 2;			// -255..255
		deltaR = pC->V * 2;			// -255..255
		deltaG = - (pC->V + pC->U / 2);	// -191..191

		pD->Blue0_0  =
		pD->Blue0_3  = Bounding24 [64+ (short) pC->Y[0] + deltaB];
		pD->Green0_1 = Bounding24 [64+ (short) pC->Y[0] + deltaG];
		pD->Red0_2   = Bounding24 [64+ (short) pC->Y[0] + deltaR];

		pD->Blue1_5  = Bounding24 [64+ (short) pC->Y[1] + deltaB];
		pD->Green1_6 = Bounding24 [64+ (short) pC->Y[1] + deltaG];
		pD->Red1_4  =
		pD->Red1_7   = Bounding24 [64+ (short) pC->Y[1] + deltaR];

		pD->Blue2_8  =
		pD->Blue2_b  = Bounding24 [64+ (short) pC->Y[2] + deltaB];
		pD->Green2_9 = Bounding24 [64+ (short) pC->Y[2] + deltaG];
		pD->Red2_a   = Bounding24 [64+ (short) pC->Y[2] + deltaR];

		pD->Blue3_d  = Bounding24 [64+ (short) pC->Y[3] + deltaB];
		pD->Green3_e = Bounding24 [64+ (short) pC->Y[3] + deltaG];
		pD->Red3_c   =
		pD->Red3_f   = Bounding24 [64+ (short) pC->Y[3] + deltaR];

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
		    deltaB = pC->U * 2;			// -255..255
		    deltaR = pC->V * 2;			// -255..255
		    deltaG = - (pC->V + pC->U / 2);		// -191..191

		    pD->Blue0_0  =
		    pD->Blue0_3  = Bounding24 [64+ (short) pC->Y[0] + deltaB];
		    pD->Green0_1 = Bounding24 [64+ (short) pC->Y[0] + deltaG];
		    pD->Red0_2   = Bounding24 [64+ (short) pC->Y[0] + deltaR];

		    pD->Blue1_5  = Bounding24 [64+ (short) pC->Y[1] + deltaB];
		    pD->Green1_6 = Bounding24 [64+ (short) pC->Y[1] + deltaG];
		    pD->Red1_4  =
		    pD->Red1_7   = Bounding24 [64+ (short) pC->Y[1] + deltaR];

		    pD->Blue2_8  =
		    pD->Blue2_b  = Bounding24 [64+ (short) pC->Y[2] + deltaB];
		    pD->Green2_9 = Bounding24 [64+ (short) pC->Y[2] + deltaG];
		    pD->Red2_a   = Bounding24 [64+ (short) pC->Y[2] + deltaR];

		    pD->Blue3_d  = Bounding24 [64+ (short) pC->Y[3] + deltaB];
		    pD->Green3_e = Bounding24 [64+ (short) pC->Y[3] + deltaG];
		    pD->Red3_c   =
		    pD->Red3_f   = Bounding24 [64+ (short) pC->Y[3] + deltaR];

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

// We take the incoming YYYYUV at *pC and develop a 4x4 patch:
//
//     RGB0 RGB0  RGB1 RGB1
//     RGB0 RGB0  RGB1 RGB1
//     RGB2 RGB2  RGB3 RGB3
//     RGB2 RGB2  RGB3 RGB3
//
// as seen on the screen.

		pD->Blue0_0  =
		pD->Blue0_3  =
		pD->Green0_1 =
		pD->Red0_2   = pCbw->Y[0];

		pD->Blue1_5  =
		pD->Green1_6 =
		pD->Red1_4   =
		pD->Red1_7   = pCbw->Y[1];

		pD->Blue2_8  =
		pD->Blue2_b  =
		pD->Green2_9 =
		pD->Red2_a   = pCbw->Y[2];

		pD->Blue3_d  =
		pD->Green3_e =
		pD->Red3_c   =
		pD->Red3_f   = pCbw->Y[3];

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
		    pD->Blue0_0  =
		    pD->Blue0_3  =
		    pD->Green0_1 =
		    pD->Red0_2   = pCbw->Y[0];

		    pD->Blue1_5  =
		    pD->Green1_6 =
		    pD->Red1_4   =
		    pD->Red1_7   = pCbw->Y[1];

		    pD->Blue2_8  =
		    pD->Blue2_b  =
		    pD->Green2_9 =
		    pD->Red2_a   = pCbw->Y[2];

		    pD->Blue3_d  =
		    pD->Green3_e =
		    pD->Red3_c   =
		    pD->Red3_f   = pCbw->Y[3];

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
