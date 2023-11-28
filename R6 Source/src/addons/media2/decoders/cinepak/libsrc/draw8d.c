// ex:set sw=2 ts=8 wm=0:
// $Header: u:/rcs/cv/rcs/draw8d.c 3.4 1994/09/26 17:07:02 bog Exp $

// (C) Copyright 1992-1993 SuperMac Technology, Inc.
// All rights reserved

// This source code and any compilation or derivative thereof is the sole
// property of SuperMac Technology, Inc. and is provided pursuant to a
// Software License Agreement.  This code is the proprietary information
// of SuperMac Technology and is confidential in nature.  Its use and
// dissemination by any party other than SuperMac Technology is strictly
// limited by the confidential information provisions of the Agreement
// referenced above.

// $Log: draw8d.c $
// Revision 3.4  1994/09/26 17:07:02  bog
// _lrotr is declared in stdlib.h.
//Revision 3.3  1994/09/09  16:14:04  bog
//Fetch alignment problems discovered on PPC.  Dither now matches .ASM, too.
//
//Revision 3.2  1994/04/30  10:51:24  unknown
//(bog)  Clean up DECOMPRESS_SET_PALETTE for C version.
//
//Revision 3.1  1994/01/13  09:58:23  geoffs
//Now works on MIPS
//
//Revision 3.0  1993/12/10  14:24:09  timr
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




/**********************************************************************

; An incoming frame is composed of up to three tiles.  A tile is a
; horizontal band of the image.  Although the data format allows the
; possibility of tiles being arbitrary rectangles on the image, the
; current implementation on the Mac and PC will barf if the tiles are
; anything other than rectangles with a common left edge, abutting in Y,
; that sequentially cover the frame.

; A tile has two parts:

;   1.  Codebooks
;   2.  Code stream

; Incoming codebook entries are in a 2x2 YUV format:

;   -----------------
;   |Y3|Y2|Y1|Y0|U|V|
;   -----------------

; There are two kinds of codebooks.  A detail codebook entry covers a
; 2x2 patch:


;   +---------+
;   | Y0 | Y1 |
;   |---------|    with the U and V coloring the entire patch.
;   | Y2 | Y3 |
;   +---------+

; A smooth codebook entry covers a 4x4 patch:

;   +-------------------+
;   | Y0 | Y0 | Y1 | Y1 |
;   |-------------------|
;   | Y0 | Y0 | Y1 | Y1 |
;   |-------------------|  with U and V again coloring the whole patch.
;   | Y2 | Y2 | Y3 | Y3 |
;   |-------------------|
;   | Y2 | Y2 | Y3 | Y3 |
;   +-------------------+

; The internal codebook (translation table) has room for 512 16-byte
; entries.  The first 256 are for detail codebook entries and the 2nd
; 256 are for smooth entries.
;
; The entries are stored in a form convenient to the decompressor for
; the particular depth to which we are decompressing.  Some of the
; decompressors have internal formats that differ between the two
; groups.

; In the case of 8 bit pixel doubled DIBs, detail and smooth codebooks
; are not identical.  In the blocks below, the pixels are shown as CP, with C
; color and P position.
  
; Detail codebooks simply contain the 16 bytes properly dithered:

;   ------------+-----------+-----------+------------
;   |00|01|10|11|02|03|12|13|20|21|30|31|22|23|32|33|
;   ------------+-----------+-----------+------------

; Smooth codebooks have the 4 orientations of each color saved as they
; appear on the top scanline of each 4x4 block of color:

;   ------------+-----------+-----------+------------
;   |00|01|02|03|10|11|12|13|20|21|22|23|30|31|32|33|
;   ------------+-----------+-----------+------------

; These correspond to screen patches that are 4x4 for detail:

;   +-------------------+
;   | 00 | 01 | 10 | 11 |
;   |-------------------|
;   | 02 | 03 | 12 | 13 |
;   |-------------------|
;   | 20 | 21 | 30 | 31 |
;   |-------------------|
;   | 22 | 23 | 32 | 33 |
;   +-------------------+

; and 8x8 for smooth:

;   +-------------------+-------------------+
;   | 00 | 01 | 02 | 03 | 10 | 11 | 12 | 13 |
;   |-------------------|-------------------|
;   | 02 | 03 | 00 | 01 | 12 | 13 | 10 | 11 |
;   |-------------------|-------------------|
;   | 01 | 02 | 03 | 00 | 11 | 12 | 13 | 10 |
;   |-------------------|-------------------|
;   | 03 | 00 | 01 | 02 | 13 | 10 | 11 | 12 |
;   |-------------------+-------------------|
;   | 20 | 21 | 22 | 23 | 30 | 31 | 32 | 33 |
;   |-------------------|-------------------|
;   | 22 | 23 | 20 | 21 | 32 | 33 | 30 | 31 |
;   |-------------------|-------------------|
;   | 21 | 22 | 23 | 20 | 31 | 32 | 33 | 30 |
;   |-------------------|-------------------|
;   | 23 | 20 | 21 | 22 | 33 | 30 | 31 | 32 |
;   +-------------------+-------------------+

; There are two kinds of codebook streams that fill entries in the
; internal codebooks.
;
; We maintain a current codebook for each tile.  On a key frame, the
; first tile contains a key codebook, kFull[DS]BookType.  Subsequent
; tiles in the key frame can contain partial codebooks that are
; updates to the previous tile.  Frames other than key frames can
; contain tiles with partial codebooks, too.  Those update the
; corresponding codebook from the previous frame.

; A key codebook is used to replace a codebook with a new one.  It
; simply consists of the sequence of 6 byte entries; there is no
; interleaved control bit stream.  The number of entries is determined
; from the size field of the SizeType dword at the front of the stream.

; A partial codebook is used to update the entries in a codebook without
; replacing them all.  The stream is interleaved from two separate
; streams.  The first is a control bit stream and the second is the
; stream of the 6-byte codebook entries.  The first dword after the
; SizeType dword has in it 32 bit switches that control whether the
; codebook should be updated from the next 6 bytes in the stream.  When
; those 32 bits run out, the next dword continues the control stream.
; Thus the control stream and the codebook bytes are interleaved.


; The code stream itself is also interleaved from two separate streams.
; When a code stream contains the control bit stream, the first dword
; has in it 32 bit switches that control interpretation of subsequent
; bytes.  When those 32 bits run out, the next dword continues the
; control stream.  Note that the interleaving continues through the
; entire code stream for a tile; there is no flush at scanline breaks.

; Sequencing through the code stream occurs on 4x4 clumps of pixels; we
; thus process 4 scanlines at a time.  If a clump is a detail clump,
; four bytes in the stream specify the four 2x2 patches making up the
; clump (I<i> are the bytes in the stream):

;   +---------------------------+
;   | I0          | I1          |
;   |   Y00 | Y01 |   Y10 | Y11 |
;   |   --------- |   --------- |
;   |   Y02 | Y03 |   Y11 | Y11 |
;   |---------------------------|
;   | I2          | I3          |
;   |   Y20 | Y21 |   Y30 | Y31 |
;   |   --------- |   --------- |
;   |   Y22 | Y23 |   Y31 | Y33 |
;   |---------------------------|

; A smooth clump paints the 4x4 patch from a single byte in the code
; stream:

;   +-------------------+
;   | Y0 | Y0 | Y1 | Y1 |
;   |-------------------|
;   | Y0 | Y0 | Y1 | Y1 |
;   |-------------------|
;   | Y2 | Y2 | Y3 | Y3 |
;   |-------------------|
;   | Y2 | Y2 | Y3 | Y3 |
;   +-------------------+

; The bytes in the code byte stream are indices into the codebook.  A
; detail code byte indexes into the detail codebook and a smooth byte
; indexes into the smooth codebook.

; There are three kinds of code stream.  A key code stream
; (kIntraCodesType) describes a key tile.  An all-smooth code stream
; (kAllSmoothCodesType) describes a key tile that is composed of only
; smooth codes.  A partial code stream (kInterCodesType) contains
; differences from the previous frame.

; In a key code stream, the bits in the control bit stream describe
; whether the corresponding 4x4 clump ought to be a detail clump,
; requiring four bytes of the code byte stream to specify the colors, or
; a smooth clump, requiring but a single byte to describe the colors of
; the clump.

; In an all-smooth code stream, there is no control stream.  All code
; bytes are smooth indices.

; In a partial code stream, a bit in the control stream indicates
; whether or not the corresponding clump should be changed.  If zero,
; the clump is left unchanged from the previous frame.  If one, the next
; bit in the control bit stream describes whether the clump is a detail
; clump, with four detail index bytes, or smooth, with only one smooth
; index byte.


**********************************************************************/

void far DrawKey8Doubled(
  DCONTEXT *,			// -> decompress context
  unsigned char *,		// flat -> incoming compressed indices
  unsigned char *, 		// offset for DIB bits to fill
  short				// height of tile
);
void far DrawSmooth8Doubled(
  DCONTEXT *,			// -> decompress context
  unsigned char *,		// flat -> incoming compressed indices
  unsigned char *, 		// offset for DIB bits to fill
  short				// height of tile
);
void far DrawInter8Doubled(
  DCONTEXT *,			// -> decompress context
  unsigned char *,		// flat -> incoming compressed indices
  unsigned char *, 		// offset for DIB bits to fill
  short				// height of tile
);





// Drawing a normal key frame.

void far
DrawKey8Doubled (
    DCONTEXT * pDC,		// -> decompress context
    unsigned char * pIx,	// -> incoming compress indices
    unsigned char * pBits, 	// -> DIB bits to fill
    short Height		// height of tile
)
{
    int nVPatches = Height >> 3;
    int cWidth;
    long yStep1 = pDC->YStep;
    long yStep2 = pDC->YStep * 2;
    long yStep3 = pDC->YStep * 3;
    long yStep4 = pDC->YStep * 4;
    long yStep5 = pDC->YStep * 5;
    long yStep6 = pDC->YStep * 6;
    long yStep7 = pDC->YStep * 7;
    long yStep8 = pDC->YStep * 8;
    unsigned long YDelta = yStep8 - pDC->Width;

    DENTRY * peDetail = pDC->pThisCodeBook->CodeBook [0].Entry;
    DENTRY * peSmooth = pDC->pThisCodeBook->CodeBook [1].Entry;

    DECLARE_SWITCHES ();

    INIT_SWITCHES ();

    // For each set of four scanlines...

    while (nVPatches--)
    {
        // For each 8x8 patch within the swath...

	for (cWidth = 0; cWidth < pDC->Width / 8; cWidth++)
	{
	    if (NEXT_SWITCH (pIx))	// bit set means DETAIL
	    {
		// We place the 64 corresponding indices:

		//   000  001  010  011   100  101  110  111 
		//   002  003  012  013   102  103  112  113 
		//   020  021  030  031   120  121  130  131 
		//   022  023  032  033   122  123  132  133 

		//   200  201  210  211   300  301  310  311 
		//   202  203  212  213   302  303  312  313 
		//   220  221  230  231   320  321  330  331 
		//   222  223  232  233   322  323  332  333

		// We copy four pixels at a time.

		*(RGB32*)(pBits         ) = peDetail[pIx[0]].Pixel0;
		*(RGB32*)(pBits  +yStep1) = peDetail[pIx[0]].Pixel1;
		*(RGB32*)(pBits  +yStep2) = peDetail[pIx[0]].Pixel2;
		*(RGB32*)(pBits  +yStep3) = peDetail[pIx[0]].Pixel3;
		*(RGB32*)(pBits+4       ) = peDetail[pIx[1]].Pixel0;
		*(RGB32*)(pBits+4+yStep1) = peDetail[pIx[1]].Pixel1;
		*(RGB32*)(pBits+4+yStep2) = peDetail[pIx[1]].Pixel2;
		*(RGB32*)(pBits+4+yStep3) = peDetail[pIx[1]].Pixel3;
		*(RGB32*)(pBits  +yStep4) = peDetail[pIx[2]].Pixel0;
		*(RGB32*)(pBits  +yStep5) = peDetail[pIx[2]].Pixel1;
		*(RGB32*)(pBits  +yStep6) = peDetail[pIx[2]].Pixel2;
		*(RGB32*)(pBits  +yStep7) = peDetail[pIx[2]].Pixel3;
		*(RGB32*)(pBits+4+yStep4) = peDetail[pIx[3]].Pixel0;
		*(RGB32*)(pBits+4+yStep5) = peDetail[pIx[3]].Pixel1;
		*(RGB32*)(pBits+4+yStep6) = peDetail[pIx[3]].Pixel2;
		*(RGB32*)(pBits+4+yStep7) = peDetail[pIx[3]].Pixel3;

		pIx += 4;
	    }
	    else			// bit clear means SMOOTH
	    {
		// We place the 64 corresponding indices:

		//   00  01  02  03   10  11  12  13 
		//   02  03  00  01   12  13  10  11 
		//   01  02  03  00   11  12  13  10 
		//   03  00  01  02   13  10  11  12 

		//   20  21  22  23   30  31  32  33 
		//   22  23  20  21   32  33  30  31 
		//   21  22  23  20   31  32  33  30 
		//   23  20  21  22   33  30  31  32

		// We copy four pixels at a time.

		*(DWORD *)(pBits  +yStep2) = _lrotr(
		  *(DWORD *)(pBits         ) = _lrotr(
		    *(DWORD *)(pBits  +yStep3) = _lrotr(
		      *(DWORD *)(pBits  +yStep1) = peSmooth[pIx[0]].dul,
		      8
		    ),
		    8
		  ),
		  8
		);
		*(DWORD *)(pBits+4+yStep2) = _lrotr(
		  *(DWORD *)(pBits+4       ) = _lrotr(
		    *(DWORD *)(pBits+4+yStep3) = _lrotr(
		      *(DWORD *)(pBits+4+yStep1) = peSmooth[pIx[0]].dur,
		      8
		    ),
		    8
		  ),
		  8
		);
		*(DWORD *)(pBits  +yStep6) = _lrotr(
		  *(DWORD *)(pBits  +yStep4) = _lrotr(
		    *(DWORD *)(pBits  +yStep7) = _lrotr(
		      *(DWORD *)(pBits  +yStep5) = peSmooth[pIx[0]].dll,
		      8
		    ),
		    8
		  ),
		  8
		);
		*(DWORD *)(pBits+4+yStep6) = _lrotr(
		  *(DWORD *)(pBits+4+yStep4) = _lrotr(
		    *(DWORD *)(pBits+4+yStep7) = _lrotr(
		      *(DWORD *)(pBits+4+yStep5) = peSmooth[pIx[0]].dlr,
		      8
		    ),
		    8
		  ),
		  8
		);

		pIx ++;
	    }
	    pBits += 8;
	}

	pBits += YDelta;
    }
}




// Drawing an all-smooth key frame.

void far
DrawSmooth8Doubled (
    DCONTEXT * pDC,		// -> decompress context
    unsigned char * pIx,	// -> incoming compress indices
    unsigned char * pBits,	// -> DIB bits to fill
    short Height		// height of tile
)
{
    int nVPatches = Height >> 3;
    int cWidth;
    long yStep1 = pDC->YStep;
    long yStep2 = pDC->YStep * 2;
    long yStep3 = pDC->YStep * 3;
    long yStep4 = pDC->YStep * 4;
    long yStep5 = pDC->YStep * 5;
    long yStep6 = pDC->YStep * 6;
    long yStep7 = pDC->YStep * 7;
    long yStep8 = pDC->YStep * 8;
    unsigned long YDelta = yStep8 - pDC->Width;

    DENTRY * peSmooth = pDC->pThisCodeBook->CodeBook [1].Entry;

    while (nVPatches--)
    {
	for (cWidth = 0; cWidth < pDC->Width/8; cWidth++)
	{
	    *(DWORD *)(pBits  +yStep2) = _lrotr(
	      *(DWORD *)(pBits         ) = _lrotr(
		*(DWORD *)(pBits  +yStep3) = _lrotr(
		  *(DWORD *)(pBits  +yStep1) = peSmooth[pIx[0]].dul,
		  8
		),
		8
	      ),
	      8
	    );
	    *(DWORD *)(pBits+4+yStep2) = _lrotr(
	      *(DWORD *)(pBits+4       ) = _lrotr(
		*(DWORD *)(pBits+4+yStep3) = _lrotr(
		  *(DWORD *)(pBits+4+yStep1) = peSmooth[pIx[0]].dur,
		  8
		),
		8
	      ),
	      8
	    );
	    *(DWORD *)(pBits  +yStep6) = _lrotr(
	      *(DWORD *)(pBits  +yStep4) = _lrotr(
		*(DWORD *)(pBits  +yStep7) = _lrotr(
		  *(DWORD *)(pBits  +yStep5) = peSmooth[pIx[0]].dll,
		  8
		),
		8
	      ),
	      8
	    );
	    *(DWORD *)(pBits+4+yStep6) = _lrotr(
	      *(DWORD *)(pBits+4+yStep4) = _lrotr(
		*(DWORD *)(pBits+4+yStep7) = _lrotr(
		  *(DWORD *)(pBits+4+yStep5) = peSmooth[pIx[0]].dlr,
		  8
		),
		8
	      ),
	      8
	    );

	    pBits += 8;
	    pIx ++;
	}

	pBits += YDelta;
    }
}


// Drawing an interframe.

void far
DrawInter8Doubled (
    DCONTEXT * pDC,		// -> decompress context
    unsigned char * pIx,	// -> incoming compress indices
    unsigned char * pBits,	// -> DIB bits to fill
    short Height		// height of tile
)
{
    int nVPatches = Height >> 3;
    int cWidth;
    long yStep1 = pDC->YStep;
    long yStep2 = pDC->YStep * 2;
    long yStep3 = pDC->YStep * 3;
    long yStep4 = pDC->YStep * 4;
    long yStep5 = pDC->YStep * 5;
    long yStep6 = pDC->YStep * 6;
    long yStep7 = pDC->YStep * 7;
    long yStep8 = pDC->YStep * 8;
    unsigned long YDelta = yStep8 - pDC->Width;
    DECLARE_SWITCHES ();

    DENTRY * peDetail = pDC->pThisCodeBook->CodeBook [0].Entry;
    DENTRY * peSmooth = pDC->pThisCodeBook->CodeBook [1].Entry;

    INIT_SWITCHES ();

    while (nVPatches--)
    {
	for (cWidth = 0; cWidth < pDC->Width/8; cWidth++)
	{
	    if (NEXT_SWITCH (pIx))	// bit set means INCLUDED
	    {
		if (NEXT_SWITCH (pIx))	// bit set means DETAIL
		{
		    *(RGB32*)(pBits         ) = peDetail[pIx[0]].Pixel0;
		    *(RGB32*)(pBits  +yStep1) = peDetail[pIx[0]].Pixel1;
		    *(RGB32*)(pBits  +yStep2) = peDetail[pIx[0]].Pixel2;
		    *(RGB32*)(pBits  +yStep3) = peDetail[pIx[0]].Pixel3;
		    *(RGB32*)(pBits+4       ) = peDetail[pIx[1]].Pixel0;
		    *(RGB32*)(pBits+4+yStep1) = peDetail[pIx[1]].Pixel1;
		    *(RGB32*)(pBits+4+yStep2) = peDetail[pIx[1]].Pixel2;
		    *(RGB32*)(pBits+4+yStep3) = peDetail[pIx[1]].Pixel3;
		    *(RGB32*)(pBits  +yStep4) = peDetail[pIx[2]].Pixel0;
		    *(RGB32*)(pBits  +yStep5) = peDetail[pIx[2]].Pixel1;
		    *(RGB32*)(pBits  +yStep6) = peDetail[pIx[2]].Pixel2;
		    *(RGB32*)(pBits  +yStep7) = peDetail[pIx[2]].Pixel3;
		    *(RGB32*)(pBits+4+yStep4) = peDetail[pIx[3]].Pixel0;
		    *(RGB32*)(pBits+4+yStep5) = peDetail[pIx[3]].Pixel1;
		    *(RGB32*)(pBits+4+yStep6) = peDetail[pIx[3]].Pixel2;
		    *(RGB32*)(pBits+4+yStep7) = peDetail[pIx[3]].Pixel3;

		    pIx += 4;
		}
		else			// bit clear means SMOOTH
		{
		    *(DWORD *)(pBits  +yStep2) = _lrotr(
		      *(DWORD *)(pBits         ) = _lrotr(
			*(DWORD *)(pBits  +yStep3) = _lrotr(
			  *(DWORD *)(pBits  +yStep1) = peSmooth[pIx[0]].dul,
			  8
			),
			8
		      ),
		      8
		    );
		    *(DWORD *)(pBits+4+yStep2) = _lrotr(
		      *(DWORD *)(pBits+4       ) = _lrotr(
			*(DWORD *)(pBits+4+yStep3) = _lrotr(
			  *(DWORD *)(pBits+4+yStep1) = peSmooth[pIx[0]].dur,
			  8
			),
			8
		      ),
		      8
		    );
		    *(DWORD *)(pBits  +yStep6) = _lrotr(
		      *(DWORD *)(pBits  +yStep4) = _lrotr(
			*(DWORD *)(pBits  +yStep7) = _lrotr(
			  *(DWORD *)(pBits  +yStep5) = peSmooth[pIx[0]].dll,
			  8
			),
			8
		      ),
		      8
		    );
		    *(DWORD *)(pBits+4+yStep6) = _lrotr(
		      *(DWORD *)(pBits+4+yStep4) = _lrotr(
			*(DWORD *)(pBits+4+yStep7) = _lrotr(
			  *(DWORD *)(pBits+4+yStep5) = peSmooth[pIx[0]].dlr,
			  8
			),
			8
		      ),
		      8
		    );

		    pIx ++;
		}
	    }

	    pBits += 8;
	}

	pBits += YDelta;
    }
}
