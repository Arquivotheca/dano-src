/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/cv.h 2.17 1995/04/29 12:31:21 bog Exp $

 * (C) Copyright 1992-1995 Radius Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of Radius Inc. and is provided pursuant to a Software
 * License Agreement.  This code is the proprietary information of
 * Radius and is confidential in nature.  Its use and dissemination by
 * any party other than Radius is strictly limited by the confidential
 * information provisions of the Agreement referenced above.

 * $Log: cv.h $
 * Revision 2.17  1995/04/29 12:31:21  bog
 * Explicitly name the CodeBook types.
 * Revision 2.16  1995/02/22  12:37:06  bog
 * Add EndingCodeBooksFromContext so the decompression codebook tables
 * survive a destination depth change.
 * 
 * Revision 2.15  1994/10/20  17:27:47  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 2.14  1994/09/22  17:02:48  bog
 * Use 4CC for DIBS_xxx; add UYVY.
 * 
 * Revision 2.13  1994/07/22  16:23:50  bog
 * Allow YUV DIB types.
 * 
 * Revision 2.12  1994/05/09  14:02:19  timr
 * Add definitions for black & white codebooks.
 * 
 * Revision 2.11  1994/05/06  16:47:37  timr
 * Add kGreyBookBit.
 * 
 * Revision 2.10  1994/03/17  12:47:32  timr
 * Force byte writes in CopySwiz4.
 * 
 * Revision 2.9  1994/03/17  11:05:36  timr
 * Can't use eax in a Win16 inline asm.
 * 
 * Revision 2.8  1994/03/17  10:41:59  timr
 * Correct MIPS alignment faults.
 * 
 * Revision 2.7  1994/02/23  15:06:53  timr
 * Correct swiz2 for that tricky Alpha C compiler.
 * 
 * Revision 2.6  1994/01/13  09:55:07  geoffs
 * All C version needed different swiz2 definition
 * 
 * Revision 2.5  1993/10/21  14:07:17  geoffs
 * Updated LOGREQUESTED stuff to include more data
 * 
 * Revision 2.4  1993/10/12  17:23:50  bog
 * RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
 * 
 * Revision 2.3  1993/08/05  17:04:44  timr
 * Make swiz4 an inline function instead of a macro.
 * 
 * Revision 2.1  93/07/02  16:30:13  geoffs
 * Now compiles,runs under Windows NT
 * 
 * Revision 2.0  93/06/01  14:13:23  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.7  93/05/02  15:46:16  bog
 * Add LogRequested flag in system.ini [iccvid.drv] to note requested size and
 * quality.
 * 
 * Revision 1.6  93/04/21  15:46:48  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.5  93/01/21  11:02:09  timr
 * Qualify "Type" with another character so it passes H2INC.
 * 
 * Revision 1.4  93/01/10  15:35:16  geoffs
 * Removed P8LOOKUP definition and instead made -> lookup a void far *
 * 
 * Revision 1.3  93/01/10  15:29:10  geoffs
 * Added -> lookup table for 8 bpp into CVCompressBegin
 * 
 * Revision 1.2  92/12/21  14:00:45  bog
 * CVDecompress stuff now compiles.
 * 
 * Revision 1.1  92/12/21  11:45:37  bog
 * Initial revision
 * 
 *
 * Compact Video Codec structs and definitions
 */


#if	defined(WINCPK)
#include "w16_32.h"		// WIN32,WIN16 specific defines
#else
#include "Reference.h"		// non-Windows reference environment
#endif


#if	defined(WINCPK)	|| defined(__BEOS__)	// Swiz2 & Swiz4 in Reference.h if non-Win

#if	defined(NOASM) || defined(__BEOS__)	// NT non-x86

#  define	swiz2(hw)	(\
				 (((unsigned short) (hw) & 0xff) << 8) | \
				 (((unsigned short) (hw)) >> 8) \
				)
#ifdef __INTEL__
#define swiz2a(X) swiz2(X)
#else
#  define	swiz2a(hw)	(hw)
#endif
#elif	defined(WIN32)	// NT x86

// Defeat the "no return value" error.
#pragma warning (disable: 4035)

__inline unsigned short __swiz2 (unsigned short usin)
{
    _asm {
        mov	ax, usin
	xchg	ah, al
    }
}

#pragma warning (default: 4035)

#  define	swiz2(hw)	__swiz2((unsigned short) (hw))

#else			// Win16

#  define	swiz2(hw)	_rotl((unsigned short) (hw), 8)

#endif


#if	defined(NOASM) || defined(__BEOS__)	// NT non-x86

#ifdef __BEOS__
#define _CopySwiz4(D,S) \
	*((D)  ) = (BYTE)((S)>>24); \
	*((D)+1) = (BYTE)((S)>>16); \
	*((D)+2) = (BYTE)((S)>>8); \
	*((D)+3) = (BYTE)((S));
#else
__inline
void _CopySwiz4 (BYTE * dest, DWORD src)
{
  *(dest    ) = (BYTE)(src >> 24);
  *(dest + 1) = (BYTE)(src >> 16);
  *(dest + 2) = (BYTE)(src >> 8);
  *(dest + 3) = (BYTE)(src);
}
#endif

#  define	CopySwiz4(d,s)	_CopySwiz4((BYTE *)&(d), (DWORD)(s))

#elif	defined(WIN32)	// NT x86

// If we knew this was a 486, we could use bswap.
#pragma warning (disable: 4035)

__inline unsigned long __swiz4 (unsigned long ulin)
{
    _asm {
    	mov	eax, ulin
	xchg	ah, al
	rol	eax, 16
	xchg	ah, al
    }
}

#pragma warning (default: 4035)

#  define CopySwiz4(d,s)	(d = __swiz4((unsigned long)s))

#else			// Win16

// If we knew this was a 486, we could use bswap.
#pragma warning (disable: 4035)

__inline unsigned long __swiz4 (unsigned long ulin)
{
    _asm {
	mov	ax, word ptr ulin[2]
	xchg	ah, al
	mov	dx, word ptr ulin
	xchg	dh, dl
    }
}

#pragma warning (default: 4035)

#  define CopySwiz4(d,s)	(d = __swiz4((unsigned long)s))

#endif

#endif				// defined(WINCPK)


#define	DIBS_CPLA	2	// Weitek Cinepak YUV420 planar
#define	DIBS_YUY2	4	// various YUV422
#define	DIBS_UYVY	8	// Cirrus 5440
#define	DIBS_CLJR	16	// Cirrus packed YUV411

typedef enum {
  NoGood	= -1,		// badness from ValidDecompressedFormat
  Dither8	= 0,		// 8 bpp dithered
  RGB555	= 1,		// 16 bpp RGB 555
  RGB565	= 2,		// 16 bpp RGB 565
  RGB888	= 3,		// 24 bpp RGB 888
  RGBa8888	= 4,		// 32 bpp RGBa 8888
  CPLA		= 5,		// Weitek Cinepak YUV420 planar
  YUY2		= 6,		// various YUV422
  UYVY		= 7,		// Cirrus 5440
  CLJR		= 8		// Cirrus packed YUV411
} DIBTYPE;


/*
 * CV stream definition
 */

#define	kMaxTileCount		3

#define	kFrameType		0x00
#define	kKeyFrameType		(0x00 + kFrameType)
#define	kInterFrameType		(0x01 + kFrameType)
#define	kEmptyFrameType		(0x02 + kFrameType)

#define	kTileType		0x10
#define	kKeyTileType		(0x00 + kTileType)
#define	kInterTileType		(0x01 + kTileType)

#define	kCodeBookType		0x20
#define	kPartialBookBit		0x01
#define	kSmoothBookBit		0x02
#define	kGreyBookBit		0x04
#define	kFullDBookType		\
  (kCodeBookType)
#define	kPartialDBookType	\
  (kCodeBookType | kPartialBookBit)
#define	kFullSBookType		\
  (kCodeBookType | kSmoothBookBit)
#define	kPartialSBookType	\
  (kCodeBookType | kPartialBookBit | kSmoothBookBit)
#define	kFullDGreyBookType	\
  (kCodeBookType | kGreyBookBit)
#define	kPartialDGreyBookType	\
  (kCodeBookType | kPartialBookBit | kGreyBookBit)
#define	kFullSGreyBookType	\
  (kCodeBookType | kSmoothBookBit | kGreyBookBit)
#define	kPartialSGreyBookType	\
  (kCodeBookType | kPartialBookBit | kSmoothBookBit | kGreyBookBit)

#define	kCodesType		0x30
#define	kIntraCodesType		(0x00 + kCodesType)
#define	kInterCodesType		(0x01 + kCodesType)
#define	kAllSmoothCodesType	(0x02 + kCodesType)

#define	kLogRequestedType	0xfd


#ifdef	WIN32
#pragma	pack(1)
#endif
#ifdef __BEOS__
#pragma options align=packed
#endif

typedef union {
  unsigned char sType;
  unsigned long SwizSize;	/* chunk size (bytes) including SIZETYPE */
} SIZETYPE;

typedef struct {		/* Mac rectangle as seen by x86 */
  short SwizTop;
  short SwizLeft;
  short SwizBottom;
  short SwizRight;
} SWIZRECT;

typedef unsigned char INDEX;

typedef struct {
  SIZETYPE SizeType;
  INDEX Index[1];
} INDICES;

typedef struct {
  unsigned char Y[4];		/* (4*G + 2*R + B) / 7:  ul, ur, ll, lr */
  signed char U;		/* (B - Y) / 2 */
  signed char V;		/* (R - Y) / 2 */
} CODE;

typedef struct {
  SIZETYPE SizeType;
  CODE Code[1];
} CODEBOOK;

typedef struct {
  unsigned char Y[4];		/* (4*G + 2*R + B) / 7:  ul, ur, ll, lr */
} CODEBW;

typedef struct {
  SIZETYPE SizeType;
  CODEBW Code[1];
} CODEBOOKBW;

typedef struct {
  SIZETYPE SizeType;
  SWIZRECT SwizRect;		/* rectangle covered by tile */
  CODEBOOK Book[1];
} TILE;

typedef struct {		// iff system.ini [ICCVID.drv] LogRequested=<nz>
  SIZETYPE SizeType;
  unsigned long SwizHintedSizeReq;	// requested hinted size
  unsigned short SwizSQuality;
  unsigned short SwizTQuality;
  unsigned long SwizHintedSizeCalc;	// calc'ed hinted size
  unsigned long SwizDetailError;	// relative to input
  unsigned long SwizSmoothError;	// relative to input
} LOGREQUESTEDTILE;

typedef struct {
  SIZETYPE SizeType;
  short SwizWidth;
  short SwizHeight;
  short SwizNumTiles;
  TILE Tile[1];
} FRAME;

#ifdef	WIN32
#pragma	pack()
#endif
#ifdef __BEOS__
#pragma options align=reset
#endif
