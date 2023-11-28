/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/endingco.c 1.5 1995/05/09 09:23:22 bog Exp $

 * (C) Copyright 1995 Radius Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of Radius Inc. and is provided pursuant to a Software
 * License Agreement.  This code is the proprietary information of
 * Radius and is confidential in nature.  Its use and dissemination by
 * any party other than Radius is strictly limited by the confidential
 * information provisions of the Agreement referenced above.

 * $Log: endingco.c $
 * Revision 1.5  1995/05/09 09:23:22  bog
 * Move WINVER back into the makefile.  Sigh.
 * Revision 1.4  1995/02/24  16:30:28  bog
 * 1.  Needed to save Detail codebooks as Detail, not Smooth.
 * 2.  Forgot to shift up from CLJR to Cinepak YUV.
 * 
 * Revision 1.3  1995/02/24  08:08:59  bog
 * Clean up Detail/Smooth control.
 * 
 * Revision 1.2  1995/02/24  07:58:13  bog
 * Was incorrectly stepping through for each of {Detail, Smooth}.
 * 
 * Revision 1.1  1995/02/22  12:36:05  bog
 * Initial revision
 * 
 *
 * Cinepak  Build CodeBooks frame from CODEBOOKS.
 */


#if	defined(WINCPK)

#define	_WINDOWS
#include <stdio.h>
#include <memory.h>		// included for _fmemcpy
#include <stdlib.h>
#include <stddef.h>
#if	defined(NULL)
#undef	NULL
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>

#endif

#include "cv.h"
#include "cvdecomp.h"


typedef struct {
  unsigned char Y[4];
  unsigned char U;
  unsigned char V;
} CCIR601CODE;


/**********************************************************************
 *
 * YFromRgbQ()
 *
 * Convert an RGBQUAD to Cinepak Y:
 *
 *   round((Red*2 + Green*4 + Blue)/7)
 *
 **********************************************************************/

unsigned char YFromRgbQ(
  LPRGBQUAD pRgbQ		// RGB value to convert
) {
  auto unsigned int t;

  t = (((pRgbQ->rgbGreen << 1) + pRgbQ->rgbRed) << 1) + pRgbQ->rgbBlue;
  return((t + 3 + (t & 1)) / 7);
}


/**********************************************************************
 *
 * CodeFromRgbQ()
 *
 * Convert 4 RGBQUADs to Cinepak Code form.
 *
 **********************************************************************/

CODE CodeFromRgbQ(
  LPRGBQUAD pRgbQ		// RGBQUAD ul, ur, ll, lr
) {
  auto CODE YyyyUV;		// hold return value
  auto int Y;

  YyyyUV.Y[0] = YFromRgbQ(pRgbQ);
  YyyyUV.Y[1] = YFromRgbQ(pRgbQ + 1);
  YyyyUV.Y[2] = YFromRgbQ(pRgbQ + 2);
  YyyyUV.Y[3] = YFromRgbQ(pRgbQ + 3);

  Y = YyyyUV.Y[0] + YyyyUV.Y[1] + YyyyUV.Y[2] + YyyyUV.Y[3];

  YyyyUV.U = (
    (
      (int) (
	pRgbQ[0].rgbBlue
	+ pRgbQ[1].rgbBlue
	+ pRgbQ[2].rgbBlue
	+ pRgbQ[3].rgbBlue
      )
    ) - Y + 4
  ) >> 3;
  YyyyUV.V = (
    (
      (int) (
	pRgbQ[0].rgbRed
	+ pRgbQ[1].rgbRed
	+ pRgbQ[2].rgbRed
	+ pRgbQ[3].rgbRed
      )
    ) - Y + 4
  ) >> 3;

  return(YyyyUV);
}


/**********************************************************************
 *
 * CodeFromCCIR601()
 *
 * Convert CCIR601 YUV to Cinepak YUV
 *
 **********************************************************************/

CODE CodeFromCCIR601(
  CCIR601CODE Ccir		// CCIR601 YYYYUV
) {
  auto CODE YyyyUV;		// hold return value
  auto int i;

  YyyyUV.U = (signed char) (
    min(
      max(
	-128 << 7,
	125 * ((int) Ccir.U) + ((int) Ccir.V) - 16060
      ),
      127 << 7
    ) >> 7
  );
  YyyyUV.V = (signed char) (
    min(
      max(
	-128 << 7,
	-4 * ((int) Ccir.U) + 103 * ((int) Ccir.V) - 12611
      ),
      127 << 7
    ) >> 7
  );

  for (i = 0; i< 4; i++) {
    YyyyUV.Y[i] = (unsigned char) (
      (
	(unsigned int) min(
	  max(
	    0 << 7,
	    (8 * ((int) Ccir.U) - ((int) Ccir.V) - 3286)
	    + (long) (149U * (unsigned int) Ccir.Y[i])
	  ),
	  255 << 7
	)
      ) >> 7
    );
  }

  return(YyyyUV);
}


/**********************************************************************
 *
 * DetailCodeFromDither8()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE DetailCodeFromDither8(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto RGBQUAD RgbQ[4];		// for 2x2 patch

  /*
    In the case of 8 bit DIBs, detail and smooth codebooks are not
    identical.  Detail codebooks present 2x2 patches predithered to the
    4 possible positions, with the bytes ordered for ease in the draw
    loops:

      ------------+-----------+-----------+------------
      |00|01|02|03|12|13|10|11|20|21|22|23|32|33|30|31|
      ------------+-----------+-----------+------------

    These correspond to screen patches that are 2x2 for detail, dithered
    according to position:

      +-------------------+
      | 00 | 01 | 10 | 11 |
      |-------------------|
      | 02 | 03 | 12 | 13 |
      |-------------------|
      | 20 | 21 | 30 | 31 |
      |-------------------|
      | 22 | 23 | 32 | 33 |
      +-------------------+
   */
  if (!lpRgbQ){			// if using standard palette
    lpRgbQ = stdPAL8 - 10;
  }

  RgbQ[0].rgbRed = (
    lpRgbQ[pCodeBook->From8Detail00].rgbRed
    + lpRgbQ[pCodeBook->From8Detail10].rgbRed
    + lpRgbQ[pCodeBook->From8Detail20].rgbRed
    + lpRgbQ[pCodeBook->From8Detail30].rgbRed
    + 2
  ) >> 2;
  RgbQ[0].rgbGreen = (
    lpRgbQ[pCodeBook->From8Detail00].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail10].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail20].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail30].rgbGreen
    + 2
  ) >> 2;
  RgbQ[0].rgbBlue = (
    lpRgbQ[pCodeBook->From8Detail00].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail10].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail20].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail30].rgbBlue
    + 2
  ) >> 2;

  RgbQ[1].rgbRed = (
    lpRgbQ[pCodeBook->From8Detail01].rgbRed
    + lpRgbQ[pCodeBook->From8Detail11].rgbRed
    + lpRgbQ[pCodeBook->From8Detail21].rgbRed
    + lpRgbQ[pCodeBook->From8Detail31].rgbRed
    + 2
  ) >> 2;
  RgbQ[1].rgbGreen = (
    lpRgbQ[pCodeBook->From8Detail01].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail11].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail21].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail31].rgbGreen
    + 2
  ) >> 2;
  RgbQ[1].rgbBlue = (
    lpRgbQ[pCodeBook->From8Detail01].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail11].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail21].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail31].rgbBlue
    + 2
  ) >> 2;

  RgbQ[2].rgbRed = (
    lpRgbQ[pCodeBook->From8Detail02].rgbRed
    + lpRgbQ[pCodeBook->From8Detail12].rgbRed
    + lpRgbQ[pCodeBook->From8Detail22].rgbRed
    + lpRgbQ[pCodeBook->From8Detail32].rgbRed
    + 2
  ) >> 2;
  RgbQ[2].rgbGreen = (
    lpRgbQ[pCodeBook->From8Detail02].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail12].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail22].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail32].rgbGreen
    + 2
  ) >> 2;
  RgbQ[2].rgbBlue = (
    lpRgbQ[pCodeBook->From8Detail02].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail12].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail22].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail32].rgbBlue
    + 2
  ) >> 2;

  RgbQ[3].rgbRed = (
    lpRgbQ[pCodeBook->From8Detail03].rgbRed
    + lpRgbQ[pCodeBook->From8Detail13].rgbRed
    + lpRgbQ[pCodeBook->From8Detail23].rgbRed
    + lpRgbQ[pCodeBook->From8Detail33].rgbRed
    + 2
  ) >> 2;
  RgbQ[3].rgbGreen = (
    lpRgbQ[pCodeBook->From8Detail03].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail13].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail23].rgbGreen
    + lpRgbQ[pCodeBook->From8Detail33].rgbGreen
    + 2
  ) >> 2;
  RgbQ[3].rgbBlue = (
    lpRgbQ[pCodeBook->From8Detail03].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail13].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail23].rgbBlue
    + lpRgbQ[pCodeBook->From8Detail33].rgbBlue
    + 2
  ) >> 2;

  return(CodeFromRgbQ(RgbQ));
}


/**********************************************************************
 *
 * SmoothCodeFromDither8()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE SmoothCodeFromDither8(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto RGBQUAD RgbQ[4];		// for 2x2 patch

  /*
    Smooth codebooks simply contain the 16 bytes properly dithered:

      --------+-------+-------+--------
      |0|1|2|3|4|5|6|7|8|9|a|b|c|d|e|f|
      --------+-------+-------+--------


    and 4x4 for smooth:

      +---------------+
      | 0 | 1 | 2 | 3 |
      |---------------|
      | 4 | 5 | 6 | 7 |
      |---------------|
      | 8 | 9 | a | b |
      |---------------|
      | c | d | e | f |
      +---------------+
   */
  if (!lpRgbQ){			// if using standard palette
    lpRgbQ = stdPAL8 - 10;
  }

  RgbQ[0].rgbRed = (
    lpRgbQ[pCodeBook->Index0].rgbRed
    + lpRgbQ[pCodeBook->Index1].rgbRed
    + lpRgbQ[pCodeBook->Index4].rgbRed
    + lpRgbQ[pCodeBook->Index5].rgbRed
    + 2
  ) >> 2;
  RgbQ[0].rgbGreen = (
    lpRgbQ[pCodeBook->Index0].rgbGreen
    + lpRgbQ[pCodeBook->Index1].rgbGreen
    + lpRgbQ[pCodeBook->Index4].rgbGreen
    + lpRgbQ[pCodeBook->Index5].rgbGreen
    + 2
  ) >> 2;
  RgbQ[0].rgbBlue = (
    lpRgbQ[pCodeBook->Index0].rgbBlue
    + lpRgbQ[pCodeBook->Index1].rgbBlue
    + lpRgbQ[pCodeBook->Index4].rgbBlue
    + lpRgbQ[pCodeBook->Index5].rgbBlue
    + 2
  ) >> 2;

  RgbQ[1].rgbRed = (
    lpRgbQ[pCodeBook->Index2].rgbRed
    + lpRgbQ[pCodeBook->Index3].rgbRed
    + lpRgbQ[pCodeBook->Index6].rgbRed
    + lpRgbQ[pCodeBook->Index7].rgbRed
    + 2
  ) >> 2;
  RgbQ[1].rgbGreen = (
    lpRgbQ[pCodeBook->Index2].rgbGreen
    + lpRgbQ[pCodeBook->Index3].rgbGreen
    + lpRgbQ[pCodeBook->Index6].rgbGreen
    + lpRgbQ[pCodeBook->Index7].rgbGreen
    + 2
  ) >> 2;
  RgbQ[1].rgbBlue = (
    lpRgbQ[pCodeBook->Index2].rgbBlue
    + lpRgbQ[pCodeBook->Index3].rgbBlue
    + lpRgbQ[pCodeBook->Index6].rgbBlue
    + lpRgbQ[pCodeBook->Index7].rgbBlue
    + 2
  ) >> 2;

  RgbQ[2].rgbRed = (
    lpRgbQ[pCodeBook->Index8].rgbRed
    + lpRgbQ[pCodeBook->Index9].rgbRed
    + lpRgbQ[pCodeBook->Indexc].rgbRed
    + lpRgbQ[pCodeBook->Indexd].rgbRed
    + 2
  ) >> 2;
  RgbQ[2].rgbGreen = (
    lpRgbQ[pCodeBook->Index8].rgbGreen
    + lpRgbQ[pCodeBook->Index9].rgbGreen
    + lpRgbQ[pCodeBook->Indexc].rgbGreen
    + lpRgbQ[pCodeBook->Indexd].rgbGreen
    + 2
  ) >> 2;
  RgbQ[2].rgbBlue = (
    lpRgbQ[pCodeBook->Index8].rgbBlue
    + lpRgbQ[pCodeBook->Index9].rgbBlue
    + lpRgbQ[pCodeBook->Indexc].rgbBlue
    + lpRgbQ[pCodeBook->Indexd].rgbBlue
    + 2
  ) >> 2;

  RgbQ[3].rgbRed = (
    lpRgbQ[pCodeBook->Indexa].rgbRed
    + lpRgbQ[pCodeBook->Indexb].rgbRed
    + lpRgbQ[pCodeBook->Indexe].rgbRed
    + lpRgbQ[pCodeBook->Indexf].rgbRed
    + 2
  ) >> 2;
  RgbQ[3].rgbGreen = (
    lpRgbQ[pCodeBook->Indexa].rgbGreen
    + lpRgbQ[pCodeBook->Indexb].rgbGreen
    + lpRgbQ[pCodeBook->Indexe].rgbGreen
    + lpRgbQ[pCodeBook->Indexf].rgbGreen
    + 2
  ) >> 2;
  RgbQ[3].rgbBlue = (
    lpRgbQ[pCodeBook->Indexa].rgbBlue
    + lpRgbQ[pCodeBook->Indexb].rgbBlue
    + lpRgbQ[pCodeBook->Indexe].rgbBlue
    + lpRgbQ[pCodeBook->Indexf].rgbBlue
    + 2
  ) >> 2;

  return(CodeFromRgbQ(RgbQ));
}


/**********************************************************************
 *
 * SmoothCodeFromDither8Doubled()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE SmoothCodeFromDither8Doubled(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto RGBQUAD RgbQ[4];		// for 2x2 patch

  /*
    Smooth codebooks have the 4 orientations of each color saved as they
    appear on the top scanline of each 4x4 block of color:

      ------------+-----------+-----------+------------
      |02|03|00|01|12|13|10|11|22|23|20|21|32|33|30|31|
      ------------+-----------+-----------+------------

    and 8x8 for smooth:

      +-------------------+-------------------+
      | 00 | 01 | 02 | 03 | 10 | 11 | 12 | 13 |
      |-------------------|-------------------|
      | 02 | 03 | 00 | 01 | 12 | 13 | 10 | 11 |
      |-------------------|-------------------|
      | 01 | 02 | 03 | 00 | 11 | 12 | 13 | 10 |
      |-------------------|-------------------|
      | 03 | 00 | 01 | 02 | 13 | 10 | 11 | 12 |
      |-------------------+-------------------|
      | 20 | 21 | 22 | 23 | 30 | 31 | 32 | 33 |
      |-------------------|-------------------|
      | 22 | 23 | 20 | 21 | 32 | 33 | 30 | 31 |
      |-------------------|-------------------|
      | 21 | 22 | 23 | 20 | 31 | 32 | 33 | 30 |
      |-------------------|-------------------|
      | 23 | 20 | 21 | 22 | 33 | 30 | 31 | 32 |
      +-------------------+-------------------+
   */
  if (!lpRgbQ){			// if using standard palette
    lpRgbQ = stdPAL8 - 10;
  }

  RgbQ[0].rgbRed = (
    lpRgbQ[pCodeBook->Index0].rgbRed
    + lpRgbQ[pCodeBook->Index1].rgbRed
    + lpRgbQ[pCodeBook->Index2].rgbRed
    + lpRgbQ[pCodeBook->Index3].rgbRed
    + 2
  ) >> 2;
  RgbQ[0].rgbGreen = (
    lpRgbQ[pCodeBook->Index0].rgbGreen
    + lpRgbQ[pCodeBook->Index1].rgbGreen
    + lpRgbQ[pCodeBook->Index2].rgbGreen
    + lpRgbQ[pCodeBook->Index3].rgbGreen
    + 2
  ) >> 2;
  RgbQ[0].rgbBlue = (
    lpRgbQ[pCodeBook->Index0].rgbBlue
    + lpRgbQ[pCodeBook->Index1].rgbBlue
    + lpRgbQ[pCodeBook->Index2].rgbBlue
    + lpRgbQ[pCodeBook->Index3].rgbBlue
    + 2
  ) >> 2;

  RgbQ[1].rgbRed = (
    lpRgbQ[pCodeBook->Index4].rgbRed
    + lpRgbQ[pCodeBook->Index5].rgbRed
    + lpRgbQ[pCodeBook->Index6].rgbRed
    + lpRgbQ[pCodeBook->Index7].rgbRed
    + 2
  ) >> 2;
  RgbQ[1].rgbGreen = (
    lpRgbQ[pCodeBook->Index4].rgbGreen
    + lpRgbQ[pCodeBook->Index5].rgbGreen
    + lpRgbQ[pCodeBook->Index6].rgbGreen
    + lpRgbQ[pCodeBook->Index7].rgbGreen
    + 2
  ) >> 2;
  RgbQ[1].rgbBlue = (
    lpRgbQ[pCodeBook->Index4].rgbBlue
    + lpRgbQ[pCodeBook->Index5].rgbBlue
    + lpRgbQ[pCodeBook->Index6].rgbBlue
    + lpRgbQ[pCodeBook->Index7].rgbBlue
    + 2
  ) >> 2;

  RgbQ[2].rgbRed = (
    lpRgbQ[pCodeBook->Index8].rgbRed
    + lpRgbQ[pCodeBook->Index9].rgbRed
    + lpRgbQ[pCodeBook->Indexa].rgbRed
    + lpRgbQ[pCodeBook->Indexb].rgbRed
    + 2
  ) >> 2;
  RgbQ[2].rgbGreen = (
    lpRgbQ[pCodeBook->Index8].rgbGreen
    + lpRgbQ[pCodeBook->Index9].rgbGreen
    + lpRgbQ[pCodeBook->Indexa].rgbGreen
    + lpRgbQ[pCodeBook->Indexb].rgbGreen
    + 2
  ) >> 2;
  RgbQ[2].rgbBlue = (
    lpRgbQ[pCodeBook->Index8].rgbBlue
    + lpRgbQ[pCodeBook->Index9].rgbBlue
    + lpRgbQ[pCodeBook->Indexa].rgbBlue
    + lpRgbQ[pCodeBook->Indexb].rgbBlue
    + 2
  ) >> 2;

  RgbQ[3].rgbRed = (
    lpRgbQ[pCodeBook->Indexc].rgbRed
    + lpRgbQ[pCodeBook->Indexd].rgbRed
    + lpRgbQ[pCodeBook->Indexe].rgbRed
    + lpRgbQ[pCodeBook->Indexf].rgbRed
    + 2
  ) >> 2;
  RgbQ[3].rgbGreen = (
    lpRgbQ[pCodeBook->Indexc].rgbGreen
    + lpRgbQ[pCodeBook->Indexd].rgbGreen
    + lpRgbQ[pCodeBook->Indexe].rgbGreen
    + lpRgbQ[pCodeBook->Indexf].rgbGreen
    + 2
  ) >> 2;
  RgbQ[3].rgbBlue = (
    lpRgbQ[pCodeBook->Indexc].rgbBlue
    + lpRgbQ[pCodeBook->Indexd].rgbBlue
    + lpRgbQ[pCodeBook->Indexe].rgbBlue
    + lpRgbQ[pCodeBook->Indexf].rgbBlue
    + 2
  ) >> 2;

  return(CodeFromRgbQ(RgbQ));
}


/**********************************************************************
 *
 * DetailCodeFromRGB555()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE DetailCodeFromRGB555(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto RGBQUAD RgbQ[4];		// for 2x2 patch

  RgbQ[0].rgbRed = (unsigned char) ((pCodeBook->ul15.Red << 3) + 4);
  RgbQ[0].rgbGreen = (unsigned char) ((pCodeBook->ul15.Green << 3) + 4);
  RgbQ[0].rgbBlue = (unsigned char) ((pCodeBook->ul15.Blue << 3) + 4);

  RgbQ[1].rgbRed = (unsigned char) ((pCodeBook->ur15.Red << 3) + 4);
  RgbQ[1].rgbGreen = (unsigned char) ((pCodeBook->ur15.Green << 3) + 4);
  RgbQ[1].rgbBlue = (unsigned char) ((pCodeBook->ur15.Blue << 3) + 4);

  RgbQ[2].rgbRed = (unsigned char) ((pCodeBook->ll15.Red << 3) + 4);
  RgbQ[2].rgbGreen = (unsigned char) ((pCodeBook->ll15.Green << 3) + 4);
  RgbQ[2].rgbBlue = (unsigned char) ((pCodeBook->ll15.Blue << 3) + 4);

  RgbQ[3].rgbRed = (unsigned char) ((pCodeBook->lr15.Red << 3) + 4);
  RgbQ[3].rgbGreen = (unsigned char) ((pCodeBook->lr15.Green << 3) + 4);
  RgbQ[3].rgbBlue = (unsigned char) ((pCodeBook->lr15.Blue << 3) + 4);

  return(CodeFromRgbQ(RgbQ));
}


/**********************************************************************
 *
 * SmoothCodeFromRGB555()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE SmoothCodeFromRGB555(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto RGBQUAD RgbQ[4];		// for 2x2 patch

  RgbQ[0].rgbRed = (unsigned char) ((pCodeBook->ull15.Red << 3) + 4);
  RgbQ[0].rgbGreen = (unsigned char) ((pCodeBook->ull15.Green << 3) + 4);
  RgbQ[0].rgbBlue = (unsigned char) ((pCodeBook->ull15.Blue << 3) + 4);

  RgbQ[1].rgbRed = (unsigned char) ((pCodeBook->url15.Red << 3) + 4);
  RgbQ[1].rgbGreen = (unsigned char) ((pCodeBook->url15.Green << 3) + 4);
  RgbQ[1].rgbBlue = (unsigned char) ((pCodeBook->url15.Blue << 3) + 4);

  RgbQ[2].rgbRed = (unsigned char) ((pCodeBook->lll15.Red << 3) + 4);
  RgbQ[2].rgbGreen = (unsigned char) ((pCodeBook->lll15.Green << 3) + 4);
  RgbQ[2].rgbBlue = (unsigned char) ((pCodeBook->lll15.Blue << 3) + 4);

  RgbQ[3].rgbRed = (unsigned char) ((pCodeBook->lrl15.Red << 3) + 4);
  RgbQ[3].rgbGreen = (unsigned char) ((pCodeBook->lrl15.Green << 3) + 4);
  RgbQ[3].rgbBlue = (unsigned char) ((pCodeBook->lrl15.Blue << 3) + 4);

  return(CodeFromRgbQ(RgbQ));
}


/**********************************************************************
 *
 * DetailCodeFromRGB565()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE DetailCodeFromRGB565(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto RGBQUAD RgbQ[4];		// for 2x2 patch

  RgbQ[0].rgbRed = (unsigned char) ((pCodeBook->ul.Red << 3) + 4);
  RgbQ[0].rgbGreen = (unsigned char) ((pCodeBook->ul.Green << 2) + 2);
  RgbQ[0].rgbBlue = (unsigned char) ((pCodeBook->ul.Blue << 3) + 4);

  RgbQ[1].rgbRed = (unsigned char) ((pCodeBook->ur.Red << 3) + 4);
  RgbQ[1].rgbGreen = (unsigned char) ((pCodeBook->ur.Green << 2) + 2);
  RgbQ[1].rgbBlue = (unsigned char) ((pCodeBook->ur.Blue << 3) + 4);

  RgbQ[2].rgbRed = (unsigned char) ((pCodeBook->ll.Red << 3) + 4);
  RgbQ[2].rgbGreen = (unsigned char) ((pCodeBook->ll.Green << 2) + 2);
  RgbQ[2].rgbBlue = (unsigned char) ((pCodeBook->ll.Blue << 3) + 4);

  RgbQ[3].rgbRed = (unsigned char) ((pCodeBook->lr.Red << 3) + 4);
  RgbQ[3].rgbGreen = (unsigned char) ((pCodeBook->lr.Green << 2) + 2);
  RgbQ[3].rgbBlue = (unsigned char) ((pCodeBook->lr.Blue << 3) + 4);

  return(CodeFromRgbQ(RgbQ));
}


/**********************************************************************
 *
 * SmoothCodeFromRGB565()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE SmoothCodeFromRGB565(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto RGBQUAD RgbQ[4];		// for 2x2 patch

  RgbQ[0].rgbRed = (unsigned char) ((pCodeBook->ull.Red << 3) + 4);
  RgbQ[0].rgbGreen = (unsigned char) ((pCodeBook->ull.Green << 2) + 2);
  RgbQ[0].rgbBlue = (unsigned char) ((pCodeBook->ull.Blue << 3) + 4);

  RgbQ[1].rgbRed = (unsigned char) ((pCodeBook->url.Red << 3) + 4);
  RgbQ[1].rgbGreen = (unsigned char) ((pCodeBook->url.Green << 2) + 2);
  RgbQ[1].rgbBlue = (unsigned char) ((pCodeBook->url.Blue << 3) + 4);

  RgbQ[2].rgbRed = (unsigned char) ((pCodeBook->lll.Red << 3) + 4);
  RgbQ[2].rgbGreen = (unsigned char) ((pCodeBook->lll.Green << 2) + 2);
  RgbQ[2].rgbBlue = (unsigned char) ((pCodeBook->lll.Blue << 3) + 4);

  RgbQ[3].rgbRed = (unsigned char) ((pCodeBook->lrl.Red << 3) + 4);
  RgbQ[3].rgbGreen = (unsigned char) ((pCodeBook->lrl.Green << 2) + 2);
  RgbQ[3].rgbBlue = (unsigned char) ((pCodeBook->lrl.Blue << 3) + 4);

  return(CodeFromRgbQ(RgbQ));
}


/**********************************************************************
 *
 * DetailCodeFromRGB888()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE DetailCodeFromRGB888(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto RGBQUAD RgbQ[4];		// for 2x2 patch

  /*
    In the case of 24 bit DIBs, detail and smooth codebooks are not
    identical.  We organize the saved codebooks so that they make best
    use of dword alignment.  Detail:

      ------------+-----------+-----------+------------
      |B0|G0|R0|B1|R0|B1|G1|R1|B2|G2|R2|B3|R2|B3|G3|R3|
      ------------+-----------+-----------+------------

    Smooth:

      ------------+-----------+-----------+------------
      |B0|G0|R0|B0|R1|B1|G1|R1|B2|G2|R2|B2|R3|B3|G3|R3|
      ------------+-----------+-----------+------------

    These correspond to screen patches that are 2x2 for detail:

      +-------------+
      | RGB0 | RGB1 |
      |-------------|
      | RGB2 | RGB3 |
      +-------------+

    and 4x4 for smooth:

      +---------------------------+
      | RGB0 | RGB0 | RGB1 | RGB1 |
      |---------------------------|
      | RGB0 | RGB0 | RGB1 | RGB1 |
      |---------------------------|
      | RGB2 | RGB2 | RGB3 | RGB3 |
      |---------------------------|
      | RGB2 | RGB2 | RGB3 | RGB3 |
      +---------------------------+
   */
  RgbQ[0] = ((LPRGBQUAD) pCodeBook)[0];

  RgbQ[1].rgbRed = pCodeBook->Red1in1;
  RgbQ[1].rgbGreen = pCodeBook->Green1in1;
  RgbQ[1].rgbBlue = pCodeBook->Blue1in1;

  RgbQ[2] = ((LPRGBQUAD) pCodeBook)[2];

  RgbQ[3].rgbRed = pCodeBook->Red3in3;
  RgbQ[3].rgbGreen = pCodeBook->Green3in3;
  RgbQ[3].rgbBlue = pCodeBook->Blue3in3;

  return(CodeFromRgbQ(RgbQ));
}


/**********************************************************************
 *
 * DetailCodeFromRGBa8888()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE DetailCodeFromRGBa8888(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  /*
    In the case of 32 bit DIBs, detail and smooth codebooks are
    identical:

      ------------+-----------+-----------+------------
      |B0|G0|R0|00|R0|B1|G1|00|B2|G2|R2|00|R2|B3|G3|00|
      ------------+-----------+-----------+------------

    These correspond to screen patches that are 2x2 for detail:

      +-------------+
      | RGB0 | RGB1 |
      |-------------|
      | RGB2 | RGB3 |
      +-------------+

    and 4x4 for smooth:

      +---------------------------+
      | RGB0 | RGB0 | RGB1 | RGB1 |
      |---------------------------|
      | RGB0 | RGB0 | RGB1 | RGB1 |
      |---------------------------|
      | RGB2 | RGB2 | RGB3 | RGB3 |
      |---------------------------|
      | RGB2 | RGB2 | RGB3 | RGB3 |
      +---------------------------+
   */
  return(CodeFromRgbQ((LPRGBQUAD) pCodeBook));
}


/**********************************************************************
 *
 * DetailCodeFromCPLA()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE DetailCodeFromCPLA(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto CODE Code;

  /*
    In the case of YUV 4:2:0, detail and smooth codebooks are not
    identical.  We organize the saved codebooks so that they make best
    use of dword alignment.  Detail:

      ------------+-----------+-----------+------------
      |Y0|Y1|Y2|Y3|Y2|Y3|Y0|Y1| U| V|--|--|--|--|--|--|
      ------------+-----------+-----------+------------

    These correspond to screen patches that are 2x2 for detail:

      +-------------+
      | RGB0 | RGB1 |
      |-------------|
      | RGB2 | RGB3 |
      +-------------+
   */
  Code.Y[0] = pCodeBook->Index0;
  Code.Y[1] = pCodeBook->Index1;
  Code.Y[2] = pCodeBook->Index2;
  Code.Y[3] = pCodeBook->Index3;
  Code.U = pCodeBook->Index8;
  Code.V = pCodeBook->Index9;

  return(Code);
}


/**********************************************************************
 *
 * SmoothCodeFromCPLA()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE SmoothCodeFromCPLA(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto CODE Code;

  /*
    Smooth:

      ------------+-----------+-----------+------------
      |Y0|Y0|Y1|Y1|Y2|Y2|Y3|Y3| U| U| V| V|--|--|--|--|
      ------------+-----------+-----------+------------

    and 4x4 for smooth:

      +---------------------------+
      | RGB0 | RGB0 | RGB1 | RGB1 |
      |---------------------------|
      | RGB0 | RGB0 | RGB1 | RGB1 |
      |---------------------------|
      | RGB2 | RGB2 | RGB3 | RGB3 |
      |---------------------------|
      | RGB2 | RGB2 | RGB3 | RGB3 |
      +---------------------------+
   */
  Code.Y[0] = pCodeBook->Index0;
  Code.Y[1] = pCodeBook->Index2;
  Code.Y[2] = pCodeBook->Index4;
  Code.Y[3] = pCodeBook->Index6;
  Code.U = pCodeBook->Index8;
  Code.V = pCodeBook->Indexa;

  return(Code);
}


/**********************************************************************
 *
 * DetailCodeFromYUY2()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE DetailCodeFromYUY2(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto CCIR601CODE Code;

  /*
    We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
    convert to CCIR601 YUV422 and remember it in the table:

    +-----------+-----------+-----------+-----------+
    |Y0|U |Y1|V |Y2|U |Y3|V |  |  |  |  |  |  |  |  |
    +-----------+-----------+-----------+-----------+
   */

  Code.Y[0] = pCodeBook->Index0;
  Code.Y[1] = pCodeBook->Index2;
  Code.Y[2] = pCodeBook->Index4;
  Code.Y[3] = pCodeBook->Index6;
  Code.U = pCodeBook->Index1;
  Code.V = pCodeBook->Index3;

  return(CodeFromCCIR601(Code));
}


/**********************************************************************
 *
 * SmoothCodeFromYUY2()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE SmoothCodeFromYUY2(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto CCIR601CODE Code;

  /*
    We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
    convert to CCIR601 YUV422 and remember it in the table:

    +-----------+-----------+-----------+-----------+
    |Y0|U |Y0|V |Y1|U |Y1|V |Y2|U |Y2|V |Y3|U |Y3|V |
    +-----------+-----------+-----------+-----------+
   */

  Code.Y[0] = pCodeBook->Index0;
  Code.Y[1] = pCodeBook->Index4;
  Code.Y[2] = pCodeBook->Index8;
  Code.Y[3] = pCodeBook->Indexc;
  Code.U = pCodeBook->Index1;
  Code.V = pCodeBook->Index3;

  return(CodeFromCCIR601(Code));
}


/**********************************************************************
 *
 * DetailCodeFromUYVY()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE DetailCodeFromUYVY(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto CCIR601CODE Code;

  /*
    We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
    convert to CCIR601 YUV422 and remember it in the table:

    +-----------+-----------+-----------+-----------+
    |U |Y0|V |Y1|U |Y2|V |Y3|  |  |  |  |  |  |  |  |
    +-----------+-----------+-----------+-----------+
   */

  Code.Y[0] = pCodeBook->Index1;
  Code.Y[1] = pCodeBook->Index3;
  Code.Y[2] = pCodeBook->Index5;
  Code.Y[3] = pCodeBook->Index7;
  Code.U = pCodeBook->Index0;
  Code.V = pCodeBook->Index2;

  return(CodeFromCCIR601(Code));
}


/**********************************************************************
 *
 * SmoothCodeFromUYVY()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE SmoothCodeFromUYVY(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto CCIR601CODE Code;

  /*
    We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
    convert to CCIR601 YUV422 and remember it in the table:

    +-----------+-----------+-----------+-----------+
    |U |Y0|V |Y0|U |Y1|V |Y1|U |Y2|V |Y2|U |Y3|V |Y3|
    +-----------+-----------+-----------+-----------+
   */

  Code.Y[0] = pCodeBook->Index1;
  Code.Y[1] = pCodeBook->Index5;
  Code.Y[2] = pCodeBook->Index9;
  Code.Y[3] = pCodeBook->Indexd;
  Code.U = pCodeBook->Index0;
  Code.V = pCodeBook->Index2;

  return(CodeFromCCIR601(Code));
}


/**********************************************************************
 *
 * DetailCodeFromCLJR()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE DetailCodeFromCLJR(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto CCIR601CODE Code;

  /*
    We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
    convert to CCIR601 YUV411 as CLJR dwords and remember it in the
    table:

        31 27 26 22 21 17 16 12 11   6 5    0
       +-----+-----+-----+-----+------+------+
    0: |   0 |   0 |  y1 |  y0 |  u/2 |  v/2 |
       +-----+-----+-----+-----+------+------+
    4: |  y1 |  y0 |   0 |   0 |  u/2 |  v/2 |
       +-----+-----+-----+-----+------+------+
    8: |   0 |   0 |  y3 |  y2 |  u/2 |  v/2 |
       +-----+-----+-----+-----+------+------+
   12: |  y3 |  y2 |   0 |   0 |  u/2 |  v/2 |
       +-----+-----+-----+-----+------+------+
   */

  Code.Y[0] = (((unsigned char) pCodeBook->Cljr[0].Y0) << 3) + 4;
  Code.Y[1] = (((unsigned char) pCodeBook->Cljr[0].Y1) << 3) + 4;
  Code.Y[2] = (((unsigned char) pCodeBook->Cljr[3].Y2) << 3) + 4;
  Code.Y[3] = (((unsigned char) pCodeBook->Cljr[3].Y3) << 3) + 4;
  Code.U = (((unsigned char) pCodeBook->Cljr[0].U) << 3) + 4;
  Code.V = (((unsigned char) pCodeBook->Cljr[0].V) << 3) + 4;

  return(CodeFromCCIR601(Code));
}


/**********************************************************************
 *
 * SmoothCodeFromCLJR()
 *
 * Convert a decompression codebook entry back to Cinepak Code form.
 *
 **********************************************************************/

CODE SmoothCodeFromCLJR(
  DENTRY far *pCodeBook,	// codebook entry
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto CCIR601CODE Code;

  /*

    We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
    convert to CCIR601 YUV411 as CLJR dwords and remember it in the
    table:

        31 27 26 22 21 17 16 12 11   6 5    0
       +-----+-----+-----+-----+------+------+
    0: |  y1 |  y1 |  y0 |  y0 |   u  |   v  |
       +-----+-----+-----+-----+------+------+
    4: |  y3 |  y3 |  y2 |  y2 |   u  |   v  |
       +-----+-----+-----+-----+------+------+
   */

  Code.Y[0] = (((unsigned char) pCodeBook->Cljr[0].Y0) << 3) + 4;
  Code.Y[1] = (((unsigned char) pCodeBook->Cljr[0].Y2) << 3) + 4;
  Code.Y[2] = (((unsigned char) pCodeBook->Cljr[1].Y0) << 3) + 4;
  Code.Y[3] = (((unsigned char) pCodeBook->Cljr[1].Y2) << 3) + 4;
  Code.U = (((unsigned char) pCodeBook->Cljr[0].U) << 2) + 2;
  Code.V = (((unsigned char) pCodeBook->Cljr[0].V) << 2) + 2;

  return(CodeFromCCIR601(Code));
}


/**********************************************************************
 *
 * EndingCodeBooksFromContext()
 *
 * Inverse translate CODEBOOKS to a valid Cinepak frame so that we can
 * re-interpret it in a new DIB format.
 *
 * Returns zero if error
 *
 **********************************************************************/

FRAME far *EndingCodeBooksFromContext(
  DCONTEXT *pD,			// decompression context
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
) {
  auto FRAME far *pFrame;
  auto DENTRY far *pEntry;
  auto int i;
  auto TILE far *pTile;

  /*
    We build a FRAME with kMaxTileCount tiles in it.  (We don't know how
    many tiles there actually are in the movie.)

    FRAME
      TILE
	CODEBOOK
	  CODE * 255
      TILE
	CODEBOOK
	  CODE * 255
      TILE
	CODEBOOK
	  CODE * 255

    FRAME includes one TILE, which includes one CODEBOOK, which includes
    one CODE.
   */
  if (
    !(
      pFrame = (FRAME far *) GlobalAllocPtr(
	GMEM_MOVEABLE,				// no GMEM_ZEROINIT needed
	sizeof(FRAME)				// includes 1 TILE
	+ (kMaxTileCount - 1) * sizeof(TILE)	// includes 1 CODEBOOK
	+ kMaxTileCount * sizeof(CODEBOOK)	// 2nd CODEBOOK
	+ kMaxTileCount * 2 * 255 * sizeof(CODE)
      )
    )
  ) {
    return(0);			// if no mem, can't preserve across Begin/End
  }
  /*
    Initialize FRAME
   */
  CopySwiz4(
    pFrame->SizeType.SwizSize,
    (
      sizeof(FRAME)
      + (kMaxTileCount - 1) * sizeof(TILE)
      + kMaxTileCount * sizeof(CODEBOOK)
      + kMaxTileCount * 2 * 255 * sizeof(CODE)
    ) | (
      ((unsigned long) kKeyFrameType) << 24
    )
  );
  pFrame->SwizWidth = (short) swiz2(320);
  pFrame->SwizHeight = (short) swiz2(240);
  pFrame->SwizNumTiles = (short) swiz2(kMaxTileCount);

  pEntry = pD->pCodeBook->CodeBook->Entry;

  /*
    for each TILE
   */
  for (
    i = kMaxTileCount,
      pTile = pFrame->Tile;
    i--;
    pTile = (TILE far *) (
      ((char far *) pTile)
      + sizeof(TILE)
      + sizeof(CODEBOOK)
      + 2 * 255 * sizeof(CODE)
    )
  ) {
    auto CODEBOOK far *pCodeBook;
    auto int j;

    /*
      Initialize TILE
     */
    CopySwiz4(
      pTile->SizeType.SwizSize,
      (
	sizeof(TILE)
	+ sizeof(CODEBOOK)
	+ 2 * 255 * sizeof(CODE)
      ) | (
	((unsigned long) kKeyTileType) << 24
      )
    );
    pTile->SwizRect.SwizTop = (short) swiz2(0);
    pTile->SwizRect.SwizLeft = (short) swiz2(0);
    pTile->SwizRect.SwizBottom = (short) swiz2(120);
    pTile->SwizRect.SwizRight = (short) swiz2(240);

    /*
      for {Detail, Smooth}
     */
    for (
      j = 2,
	pCodeBook = pTile->Book;
      j--;
      pCodeBook = (CODEBOOK far *) &pCodeBook->Code[256]
    ) {
      auto PCODEFROM pCodeFrom;
      auto CODE far *pCode;
      auto int k;

      if (j) {			// if Detail

	CopySwiz4(
	  pCodeBook->SizeType.SwizSize,
	  (
	    sizeof(CODEBOOK) + 255 * sizeof(CODE)
	  ) | (
	    ((unsigned long) kFullDBookType) << 24
	  )
	);
	pCodeFrom = pD->Dispatch.pDetailCodeFrom;

      } else {			// Smooth

	CopySwiz4(
	  pCodeBook->SizeType.SwizSize,
	  (
	    sizeof(CODEBOOK) + 255 * sizeof(CODE)
	  ) | (
	    ((unsigned long) kFullSBookType) << 24
	  )
	);
	pCodeFrom = pD->Dispatch.pSmoothCodeFrom;
      }
      /*
	for each of 256 DENTRYs
       */
      for (
	pCode = pCodeBook->Code,
	  k = 256;
	k--;
	*pCode++ = (*pCodeFrom)(pEntry++, lpRgbQ)
      );
    }
  }
  return(pFrame);
}
