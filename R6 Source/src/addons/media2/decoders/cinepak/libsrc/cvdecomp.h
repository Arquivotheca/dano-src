/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/cvdecomp.h 2.24 1995/02/22 12:38:08 bog Exp $

 * (C) Copyright 1992-1995 Radius Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of Radius Inc. and is provided pursuant to a Software
 * License Agreement.  This code is the proprietary information of
 * Radius and is confidential in nature.  Its use and dissemination by
 * any party other than Radius is strictly limited by the confidential
 * information provisions of the Agreement referenced above.

 * $Log: cvdecomp.h $
 * Revision 2.24  1995/02/22 12:38:08  bog
 * Add EndingCodeBooksFromContext so the decompression codebook tables
 * survive a destination depth change.
 * Revision 2.23  1994/10/20  17:35:00  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 2.22  1994/09/22  17:04:15  bog
 * Use 4CC for DIBS_xxx; add UYVY.
 * 
 * Revision 2.21  1994/08/30  09:30:00  bog
 * Fix up non-ASM, non-X86 #ifdef stuff from MOTION changes.
 * 
 * Revision 2.20  1994/07/29  15:31:09  bog
 * Weitek YUV420 works.
 * 
 * Revision 2.19  1994/07/22  16:24:10  bog
 * Change fixup mechanism to include Weitek YUV.
 * 
 * Revision 2.18  1994/05/25  12:36:27  timr
 * Use MoveMemory instead of CopyMemory where overlap can occur.
 * 
 * Revision 2.17  1994/04/30  10:51:01  unknown
 * (bog)  Clean up DECOMPRESS_SET_PALETTE for C version.
 *
 * Revision 2.16  1994/03/29  13:20:07  bog
 * Allow YStep to change between frames of a movie.
 *
 * Revision 2.15  1994/03/24  16:55:28  timr
 * For DCI, allow the DIB YStep to change without requiring End/Begin.
 *
 * Revision 2.14  1994/03/03  12:00:46  timr
 * Add code to support SET_PALETTE on non-x86 platforms.
 *
 * Revision 2.13  1994/03/02  16:16:35  timr
 * Add code to support _SET_PALETTE message.
 *
 * Revision 2.12  1994/02/23  15:07:45  timr
 * Rework the switches code to make it more sensible.
 *
 * Revision 2.11  1994/01/13  11:17:45  geoffs
 * Added back extern declarations for Expand,Draw{Base,Table} labels
 *
 * Revision 2.10  1994/01/13  10:18:56  geoffs
 * Must pass height down to draw*.c routines for all C version
 *
 * Revision 2.9  1993/12/10  14:47:02  timr
 * Incorporate changes to accomodate NT C-only codec.
 *
 * Revision 2.8  1993/11/29  16:04:24  geoffs
 * Pixel doubling done for 8bpp decompress
 *
 * Revision 2.7  1993/10/12  17:24:43  bog
 * RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
 *
 * Revision 2.6  1993/07/02  16:30:21  geoffs
 * Now compiles,runs under Windows NT
 *
 * Revision 2.5  93/06/14  11:34:50  bog
 * Remove CVFAR; we will clone for Thun acceleration.
 *
 * Revision 2.4  93/06/13  11:21:12  bog
 * Add hooks for playback acceleration.
 *
 * Revision 2.3  93/06/09  17:58:35  bog
 * Support decompression to 32 bit DIBs.
 *
 * Revision 2.2  93/06/02  13:27:49  bog
 * Ensure things still work if the incoming frame crosses a segment boundary.
 *
 * Revision 2.1  93/06/01  14:48:46  bog
 * Compiled, flat decompress assembler.
 *
 * Revision 2.0  93/06/01  14:13:45  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 *
 * Revision 1.5  93/04/21  15:47:18  bog
 * Fix up copyright and disclaimer.
 *
 * Revision 1.4  93/01/15  14:52:51  timr
 * Incorporate 16 bit decompressor.
 *
 * Revision 1.3  93/01/06  11:51:49  geoffs
 * Rationalized i/f's for CVCompress... and CVDecompress...
 *
 * Revision 1.2  92/12/21  18:42:26  bog
 * Decompressor now works split.
 *
 * Revision 1.1  92/12/21  11:36:15  bog
 * Initial revision
 *
 *
 * Compact Video Codec structs and definitions
 */

#if	defined(WINCPK)

#include "w16_32.h"		// WIN32,WIN16 specific defines

#endif

#ifdef __BEOS__
#include "beos.h"
#endif
/*
 * CV decompressor declarations and structures
 */

typedef struct {
  unsigned char Blue;
  unsigned char Green;
  unsigned char Red;
} RGB24;

#if	defined(WINCPK) | defined(__BEOS__)

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
typedef struct {
  unsigned char Blue;
  unsigned char Green;
  unsigned char Red;
  unsigned char alpha;
} RGB32;
#else
typedef struct {
  unsigned char alpha;
  unsigned char Red;
  unsigned char Green;
  unsigned char Blue;
} RGB32;
#endif

#else

typedef struct {
  unsigned char alpha;
  unsigned char Red;
  unsigned char Green;
  unsigned char Blue;
} RGB32;

#endif

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
typedef struct {
  unsigned short Blue  : 5;
  unsigned short Green : 6;
  unsigned short Red   : 5;
} RGB16;

typedef struct {
  unsigned short Blue  : 5;
  unsigned short Green : 5;
  unsigned short Red   : 5;
  unsigned short alpha : 1;
} RGB15;
#else
typedef struct {
  unsigned short Red   : 5;
  unsigned short Green : 6;
  unsigned short Blue  : 5;
} RGB16;

typedef struct {
  unsigned short alpha : 1;
  unsigned short Red   : 5;
  unsigned short Green : 5;
  unsigned short Blue  : 5;
} RGB15;
#endif

#if	defined(WINCPK) || defined(__BEOS__)

typedef struct {
  unsigned long V  : 6;
  unsigned long U  : 6;
  unsigned long Y0 : 5;
  unsigned long Y1 : 5;
  unsigned long Y2 : 5;
  unsigned long Y3 : 5;
} CLJR32;

typedef union {			// codebook table entry format

  struct {
    unsigned char Index00;	// ordered so dword refs can be used to
    unsigned char Index01;	// build 4 dithered 2x2 patches:
    unsigned char Index10;
    unsigned char Index11;	// +-------------------+
    unsigned char Index02;	// | 00 | 01 | 10 | 11 |
    unsigned char Index03;	// |-------------------|
    unsigned char Index12;	// | 02 | 03 | 12 | 13 |
    unsigned char Index13;	// |-------------------|
    unsigned char Index20;	// | 20 | 21 | 30 | 31 |
    unsigned char Index21;	// |-------------------|
    unsigned char Index30;	// | 22 | 23 | 32 | 33 |
    unsigned char Index31;	// +-------------------+
    unsigned char Index22;
    unsigned char Index23;
    unsigned char Index32;
    unsigned char Index33;
  } u0;	//  Detail8;
#define Index00 u0.Index00
#define Index01 u0.Index01
#define Index10 u0.Index10
#define Index11 u0.Index11
#define Index02 u0.Index02
#define Index03 u0.Index03
#define Index12 u0.Index12
#define Index13 u0.Index13
#define Index20 u0.Index20
#define Index21 u0.Index21
#define Index30 u0.Index30
#define Index31 u0.Index31
#define Index22 u0.Index22
#define Index23 u0.Index23
#define Index32 u0.Index32
#define Index33 u0.Index33
  struct {
    unsigned char From8Detail00;
    unsigned char From8Detail01;
    unsigned char From8Detail02;
    unsigned char From8Detail03;
    unsigned char From8Detail12;
    unsigned char From8Detail13;
    unsigned char From8Detail10;
    unsigned char From8Detail11;
    unsigned char From8Detail20;
    unsigned char From8Detail21;
    unsigned char From8Detail22;
    unsigned char From8Detail23;
    unsigned char From8Detail32;
    unsigned char From8Detail33;
    unsigned char From8Detail30;
    unsigned char From8Detail31;
  } u1;
#define From8Detail00 u1.From8Detail00
#define From8Detail01 u1.From8Detail01
#define From8Detail02 u1.From8Detail02
#define From8Detail03 u1.From8Detail03
#define From8Detail12 u1.From8Detail12
#define From8Detail13 u1.From8Detail13
#define From8Detail10 u1.From8Detail10
#define From8Detail11 u1.From8Detail11
#define From8Detail20 u1.From8Detail20
#define From8Detail21 u1.From8Detail21
#define From8Detail22 u1.From8Detail22
#define From8Detail23 u1.From8Detail23
#define From8Detail32 u1.From8Detail32
#define From8Detail33 u1.From8Detail33
#define From8Detail30 u1.From8Detail30
#define From8Detail31 u1.From8Detail31
  struct {
    unsigned char Index0;	// ordered so dword refs can be used to
    unsigned char Index1;	// build dithered 4x4 patch:
    unsigned char Index2;
    unsigned char Index3;	// +---------------+
    unsigned char Index4;	// | 0 | 1 | 2 | 3 |
    unsigned char Index5;	// |---------------|
    unsigned char Index6;	// | 4 | 5 | 6 | 7 |
    unsigned char Index7;	// |---------------|
    unsigned char Index8;	// | 8 | 9 | a | b |
    unsigned char Index9;	// |---------------|
    unsigned char Indexa;	// | c | d | b | f |
    unsigned char Indexb;	// +---------------+
    unsigned char Indexc;
    unsigned char Indexd;
    unsigned char Indexe;
    unsigned char Indexf;
  } u2;	//  Smooth8;
#define Index0 u2.Index0
#define Index1 u2.Index1
#define Index2 u2.Index2
#define Index3 u2.Index3
#define Index4 u2.Index4
#define Index5 u2.Index5
#define Index6 u2.Index6
#define Index7 u2.Index7
#define Index8 u2.Index8
#define Index9 u2.Index9
#define Indexa u2.Indexa
#define Indexb u2.Indexb
#define Indexc u2.Indexc
#define Indexd u2.Indexd
#define Indexe u2.Indexe
#define Indexf u2.Indexf

  struct {
    RGB15 ul15;			// ul : ur
    RGB15 ur15;
    RGB15 ll15;			// ll : lr
    RGB15 lr15;
  } u3;	//  Detail15;
#define ul15 u3.ul15
#define ur15 u3.ur15
#define ll15 u3.ll15
#define lr15 u3.lr15
  struct {
    RGB15 ull15;		// ull : ulr : url : urr
    RGB15 ulr15;
    RGB15 url15;		// ull : ulr : url : urr
    RGB15 urr15;
    RGB15 lll15;		// lll : llr : lrl : lrr
    RGB15 llr15;
    RGB15 lrl15;		// lll : llr : lrl : lrr
    RGB15 lrr15;
  } u4;	//  Smooth15;
#define ull15 u4.ull15
#define ulr15 u4.ulr15
#define url15 u4.url15
#define urr15 u4.urr15
#define lll15 u4.lll15
#define llr15 u4.llr15
#define lrl15 u4.lrl15
#define lrr15 u4.lrr15

  struct {
    RGB16 ul;			// ul : ur
    RGB16 ur;
    RGB16 ll;			// ll : lr
    RGB16 lr;
  } u5;	//  Detail16;
#define ul u5.ul
#define ur u5.ur
#define ll u5.ll
#define lr u5.lr
  struct {
    RGB16 ull;			// ull : ulr : url : urr
    RGB16 ulr;
    RGB16 url;			// ull : ulr : url : urr
    RGB16 urr;
    RGB16 lll;			// lll : llr : lrl : lrr
    RGB16 llr;
    RGB16 lrl;			// lll : llr : lrl : lrr
    RGB16 lrr;
  } u6;	//  Smooth16;
#define ull u6.ull
#define ulr u6.ulr
#define url u6.url
#define urr u6.urr
#define lll u6.lll
#define llr u6.llr
#define lrl u6.lrl
#define lrr u6.lrr

  struct {			// for efficiency in accessing Smooth1[56]
    unsigned long dul;
    unsigned long dur;
    unsigned long dll;
    unsigned long dlr;
  } u7;	//  Smooth15_16_fake
#define dul u7.dul
#define dur u7.dur
#define dll u7.dll
#define dlr u7.dlr
  struct {
    unsigned char Blue0in0;	// ordered so dword refs can be used to
    unsigned char Green0in0;	// build 2x2 RGB24 patch
    unsigned char Red0in0;
    unsigned char Blue1in0;	// +-----------------------------+
    unsigned char Red0in1;	// | B0 | G0 | R0 | B1 | G1 | R1 |
    unsigned char Blue1in1;	// |-----------------------------|
    unsigned char Green1in1;	// | B2 | G2 | R2 | B3 | G3 | R3 |
    unsigned char Red1in1;	// +-----------------------------+
    unsigned char Blue2in2;
    unsigned char Green2in2;
    unsigned char Red2in2;
    unsigned char Blue3in2;
    unsigned char Red2in3;
    unsigned char Blue3in3;
    unsigned char Green3in3;
    unsigned char Red3in3;
  } u8;	//  Detail24;
#define Blue0in0 u8.Blue0in0
#define Green0in0 u8.Green0in0
#define Red0in0 u8.Red0in0
#define Blue1in0 u8.Blue1in0
#define Red0in1 u8.Red0in1
#define Blue1in1 u8.Blue1in1
#define Green1in1 u8.Green1in1
#define Red1in1 u8.Red1in1
#define Blue2in2 u8.Blue2in2
#define Green2in2 u8.Green2in2
#define Red2in2 u8.Red2in2
#define Blue3in2 u8.Blue3in2
#define Red2in3 u8.Red2in3
#define Blue3in3 u8.Blue3in3
#define Green3in3 u8.Green3in3
#define Red3in3 u8.Red3in3
  struct {
    unsigned char Blue0_0;	// ordered so dword refs can be used to
    unsigned char Green0_1;	// build 4x4 RGB24 patch
    unsigned char Red0_2;
    unsigned char Blue0_3;	// +---------------------------+
    unsigned char Red1_4;	// | RGB0 | RGB0 | RGB1 | RGB1 |
    unsigned char Blue1_5;	// |---------------------------|
    unsigned char Green1_6;	// | RGB0 | RGB0 | RGB1 | RGB1 |
    unsigned char Red1_7;	// |---------------------------|
    unsigned char Blue2_8;	// | RGB2 | RGB2 | RGB3 | RGB3 |
    unsigned char Green2_9;	// |---------------------------|
    unsigned char Red2_a;	// | RGB2 | RGB2 | RGB3 | RGB3 |
    unsigned char Blue2_b;	// +---------------------------+
    unsigned char Red3_c;
    unsigned char Blue3_d;
    unsigned char Green3_e;
    unsigned char Red3_f;
  } u9;	//  Smooth24;
#define Blue0_0 u9.Blue0_0
#define Green0_1 u9.Green0_1
#define Red0_2 u9.Red0_2
#define Blue0_3 u9.Blue0_3
#define Red1_4 u9.Red1_4
#define Blue1_5 u9.Blue1_5
#define Green1_6 u9.Green1_6
#define Red1_7 u9.Red1_7
#define Blue2_8 u9.Blue2_8
#define Green2_9 u9.Green2_9
#define Red2_a u9.Red2_a
#define Blue2_b u9.Blue2_b
#define Red3_c u9.Red3_c
#define Blue3_d u9.Blue3_d
#define Green3_e u9.Green3_e
#define Red3_f u9.Red3_f

  struct {
    RGB32 Pixel0;	// 00 01      00 00 01 01
    RGB32 Pixel1;	//            00 00 01 01
    RGB32 Pixel2;	// 02 03      02 02 03 03
    RGB32 Pixel3;	//            02 02 03 03
  } u10;	//  Detail32;	// and Smooth32;
#define Pixel0 u10.Pixel0
#define Pixel1 u10.Pixel1
#define Pixel2 u10.Pixel2
#define Pixel3 u10.Pixel3
  struct {
    CLJR32 Cljr[4];		// CLJR Cirrus Logic AcuPak
  } u11;
#define Cljr u11.Cljr
  struct {
  	unsigned long L0;
  	unsigned long L1;
  	unsigned long L2;
  	unsigned long L3;
  } u12;
#define L0 u12.L0
#define L1 u12.L1
#define L2 u12.L2
#define L3 u12.L3

} DENTRY;

#else			// reference version

typedef struct {		// codebook table entry format

  RGB32 Pixel0;		// 00 01      00 00 01 01
  RGB32 Pixel1;		//            00 00 01 01
  RGB32 Pixel2;		// 02 03      02 02 03 03
  RGB32 Pixel3;		//            02 02 03 03

} DENTRY;

#endif

typedef struct {
  DENTRY Entry[256];
} DCODEBOOK;

typedef struct {
  DCODEBOOK CodeBook[2];
} CODEBOOKS;

#if	!defined(NOASM)

typedef enum {
  iCodebook = 0,		// ref to current codebook
  iYStep1,			// YStep*1
  iYStep2,			// YStep*2
  iYStep3,			// YStep*3
  iYStep4,			// YStep*4
  iYStep5,			// YStep*5
  iYStep6,			// YStep*6
  iYStep7,			// YStep*7
  iYDelta,			// YStep*8 - Width
  iMotion,			// self
  iWidth4,			// Width/4
  iWidth8,			// Width/8
  iTEXT32,			// stuff in TEXT32 that doesn't move
  iYScan0,			// offset to Y frame
  iUScan0,			// offset to U frame
  iUScan1,			// offset to U (0,1)
  iVScan0,			// offset to V frame
  iVScan1,			// offset to V (0,1)
  nFixups			// number of fixup types
} IFIXUP;

#define	bCodebook	(1L << iCodebook)
#define	bYStep1		(1L << iYStep1)
#define	bYStep2		(1L << iYStep2)
#define	bYStep3		(1L << iYStep3)
#define	bYStep4		(1L << iYStep4)
#define	bYStep5		(1L << iYStep5)
#define	bYStep6		(1L << iYStep6)
#define	bYStep7		(1L << iYStep7)
#define	bYDelta		(1L << iYDelta)
#define	bMotion		(1L << iMotion)
#define	bWidth4		(1L << iWidth4)
#define	bWidth8		(1L << iWidth8)
#define	bTEXT32		(1L << iTEXT32)
#define	bYScan0		(1L << iYScan0)
#define	bUScan0		(1L << iUScan0)
#define	bUScan1		(1L << iUScan1)
#define	bVScan0		(1L << iVScan0)
#define	bVScan1		(1L << iVScan1)

// 18*4=72
typedef struct {
  unsigned long vFixup[nFixups];// current or desired fixup value
} VFIXUP;

extern VFIXUP far InitialFixup;	// starting value of fixups

typedef struct {

  unsigned short nBytes;	// length of moving section

  unsigned short nRefs[nFixups];// number of refs for each fixup

  unsigned short oRef[1];	// table of refs

} MOTIONTABLE;

#endif

struct _DCONTEXT;


// Externals.

#if	!defined(EXPAND8_C)

extern int far nRgbLookup;
extern unsigned char far RgbLookup [];
extern RGBQUAD far stdPAL8 [];

#endif


// Function prototype typedefs.

#if	defined(NOASM)

#define	EXPANDCODEBOOK_PROTO	\
	CODEBOOK *,\
	DCODEBOOK *, \
	unsigned char *

typedef void (far *PEXPANDCODEBOOK)(
  CODEBOOK *,			// -> input codebook
  DCODEBOOK *,			// -> where to put translate table
  unsigned char *		// -> RgbLookup
);

#define	DRAWFRAME_PROTO	\
	struct _DCONTEXT *,	\
	unsigned char *,\
	unsigned char *,\
	short

typedef void (far *PDRAWFRAME)(
  struct _DCONTEXT *,		// -> decompress context
  unsigned char *,		// -> incoming compressed indices
  unsigned char *,		// -> DIB bits to fill
  short				// height of tile
);

#else

#define EXPANDCODEBOOK_PROTO	\
	unsigned long,	\
	unsigned long

typedef void (far *PEXPANDCODEBOOK)(
  unsigned long,		// -> input codebook
  unsigned long 		// -> where to put translate table
);

#if	defined(WIN32)

#if defined(__BEOS__)
#define	DRAWFRAME_PROTO	\
	unsigned long, \
	unsigned long,	\
	unsigned long,	\
	short
#else
#define	DRAWFRAME_PROTO	\
	unsigned long,	\
	unsigned long,	\
	short
#endif

#else

#define	DRAWFRAME_PROTO	\
	unsigned long,	\
	unsigned long,	\
	unsigned short,	\
	short

#endif

typedef void (far *PDRAWFRAME)(
#ifdef __BEOS__
  unsigned long,		// fixup table
#endif
  unsigned long,		// flat -> incoming compressed indices
  unsigned long,		// offset for DIB bits to fill
#if	!defined(WIN32)
  unsigned short,		// selector for DIB bits to fill
#endif
  short				// height of tile
);

#endif

typedef CODE (*PCODEFROM)(
  DENTRY far *pCodeBook,	// codebook slot to inverse transform
  LPRGBQUAD lpRgbQ		// user palette if 8 bit
);

#if	!defined(NOASM)

typedef struct {
  char far *pBase;		// base of code to copy
  MOTIONTABLE *pTable;		// -> table for chunk to copy
} MOTION;

#endif

// 16+28=44 for asm, 28 for noasm
typedef struct _DISPATCH {

#if	!defined(NOASM)

  MOTION Expand;		// expand codebook
  MOTION Draw;			// draw frame

#endif

  PEXPANDCODEBOOK pExpandDetailCodeBook;
  PEXPANDCODEBOOK pExpandSmoothCodeBook;
  PDRAWFRAME	pDrawKey;
  PDRAWFRAME	pDrawSmooth;
  PDRAWFRAME	pDrawInter;
  PCODEFROM pDetailCodeFrom;
  PCODEFROM pSmoothCodeFrom;

} DISPATCH;


typedef struct _DCONTEXT {

  DISPATCH Dispatch;		// which functions to hit

  CODEBOOKS far *pCodeBook;	// translate code to pixels
  unsigned long oCodeBook;	// flat offset to codebooks

  FRAME far *pInitialCodeBooks;	// inverse color transformed codebooks

// offset 12+44=56 for asm, 12+28=40 for noasm
  short Width;			// width of holding DIB in pixels
  short Height;			// height of holding DIB in scanlines
  short DepthInBytes;		// bytes per pixel
  short Scale;			// dest:src stretch factor (2x only for now)

#if	defined(NOASM)

  unsigned char far *pRgbLookup;// -> live RgbLookup table

// offset 40+12=52
  CODEBOOKS far *pThisCodeBook;	// -> this tile's codebook pair
  long YStep;			// YStep for this frame

#else
// offset 20+44=64
  VFIXUP Desired;		// desired fixup values
// offset 64+72=136
  VFIXUP Actual;		// actual fixup values currently in built code
  unsigned long FixupSws;	// 1L << i(IFIXUP) on if corresp nRef nz

#endif

} DCONTEXT;


/*
  bottom level decompression
 */

#if	!defined(NOASM)
extern unsigned char far Expand8Base;	// base of code to copy
extern unsigned char far Draw8Base;	// base of code to copy
extern MOTIONTABLE Expand8Table;	// -> table for chunk to copy
extern MOTIONTABLE Draw8Table;		// -> table for chunk to copy
#endif
void far ExpandDetailCodeBook8	(EXPANDCODEBOOK_PROTO);
void far ExpandSmoothCodeBook8	(EXPANDCODEBOOK_PROTO);
void far DrawKey8		(DRAWFRAME_PROTO);
void far DrawSmooth8		(DRAWFRAME_PROTO);
void far DrawInter8		(DRAWFRAME_PROTO);
CODE DetailCodeFromDither8(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
CODE SmoothCodeFromDither8(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);

#if	!defined(NOASM)
extern unsigned char far Expand15Base;	// base of code to copy
extern unsigned char far Draw15Base;	// base of code to copy
extern MOTIONTABLE Expand15Table;	// -> table for chunk to copy
extern MOTIONTABLE Draw15Table;		// -> table for chunk to copy
#endif
void far ExpandDetailCodeBook15	(EXPANDCODEBOOK_PROTO);
void far ExpandSmoothCodeBook15	(EXPANDCODEBOOK_PROTO);
void far DrawKey15		(DRAWFRAME_PROTO);
void far DrawSmooth15		(DRAWFRAME_PROTO);
void far DrawInter15		(DRAWFRAME_PROTO);
CODE DetailCodeFromRGB555(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
CODE SmoothCodeFromRGB555(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);

#if	!defined(NOASM)
extern unsigned char far Expand16Base;	// base of code to copy
extern unsigned char far Draw16Base;	// base of code to copy
extern MOTIONTABLE Expand16Table;	// -> table for chunk to copy
extern MOTIONTABLE Draw16Table;		// -> table for chunk to copy
#endif
void far ExpandDetailCodeBook16	(EXPANDCODEBOOK_PROTO);
void far ExpandSmoothCodeBook16	(EXPANDCODEBOOK_PROTO);
// RGB565 Draw use RGB555 Draw
CODE DetailCodeFromRGB565(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
CODE SmoothCodeFromRGB565(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);

#if	!defined(NOASM)
extern unsigned char far Expand24Base;	// base of code to copy
extern unsigned char far Draw24Base;	// base of code to copy
extern MOTIONTABLE Expand24Table;	// -> table for chunk to copy
extern MOTIONTABLE Draw24Table;		// -> table for chunk to copy
#endif
void far ExpandDetailCodeBook24	(EXPANDCODEBOOK_PROTO);
void far ExpandSmoothCodeBook24	(EXPANDCODEBOOK_PROTO);
void far DrawKey24		(DRAWFRAME_PROTO);
void far DrawSmooth24		(DRAWFRAME_PROTO);
void far DrawInter24		(DRAWFRAME_PROTO);
CODE DetailCodeFromRGB888(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
#define	SmoothCodeFromRGB888	DetailCodeFromRGB888

#if	!defined(NOASM)
extern unsigned char far Expand32Base;	// base of code to copy
extern unsigned char far Draw32Base;	// base of code to copy
extern MOTIONTABLE Expand32Table;	// -> table for chunk to copy
extern MOTIONTABLE Draw32Table;		// -> table for chunk to copy
#endif
void far ExpandCodeBook32	(EXPANDCODEBOOK_PROTO);
void far DrawKey32		(DRAWFRAME_PROTO);
void far DrawSmooth32		(DRAWFRAME_PROTO);
void far DrawInter32		(DRAWFRAME_PROTO);
CODE DetailCodeFromRGBa8888(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
#define	SmoothCodeFromRGBa8888	DetailCodeFromRGBa8888

#if	(DIBS & DIBS_CPLA)
#if	!defined(NOASM)
extern unsigned char far ExpandCPLABase;// base of code to copy
extern unsigned char far DrawCPLABase;	// base of code to copy
extern MOTIONTABLE ExpandCPLATable;	// -> table for chunk to copy
extern MOTIONTABLE DrawCPLATable;	// -> table for chunk to copy
#endif
void far ExpandDetailCodeBookCPLA	(EXPANDCODEBOOK_PROTO);
void far ExpandSmoothCodeBookCPLA	(EXPANDCODEBOOK_PROTO);
void far DrawKeyCPLA			(DRAWFRAME_PROTO);
void far DrawSmoothCPLA			(DRAWFRAME_PROTO);
void far DrawInterCPLA			(DRAWFRAME_PROTO);
CODE DetailCodeFromCPLA(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
CODE SmoothCodeFromCPLA(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
#endif

#if	(DIBS & DIBS_YUY2)
#if	!defined(NOASM)
extern unsigned char far ExpandYUY2Base;// base of code to copy
extern unsigned char far DrawYUY2Base;	// base of code to copy
extern MOTIONTABLE ExpandYUY2Table;	// -> table for chunk to copy
extern MOTIONTABLE DrawYUY2Table;	// -> table for chunk to copy
#endif
void far ExpandDetailCodeBookYUY2	(EXPANDCODEBOOK_PROTO);
void far ExpandSmoothCodeBookYUY2	(EXPANDCODEBOOK_PROTO);
// YUY2 Draw use RGB555 Draw
CODE DetailCodeFromYUY2(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
CODE SmoothCodeFromYUY2(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
#endif

#if	(DIBS & DIBS_UYVY)
#if	!defined(NOASM)
extern unsigned char far ExpandUYVYBase;// base of code to copy
extern unsigned char far DrawUYVYBase;	// base of code to copy
extern MOTIONTABLE ExpandUYVYTable;	// -> table for chunk to copy
extern MOTIONTABLE DrawUYVYTable;	// -> table for chunk to copy
#endif
void far ExpandDetailCodeBookUYVY	(EXPANDCODEBOOK_PROTO);
void far ExpandSmoothCodeBookUYVY	(EXPANDCODEBOOK_PROTO);
// UYVY Draw use RGB555 Draw
CODE DetailCodeFromUYVY(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
CODE SmoothCodeFromUYVY(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
#endif

#if	(DIBS & DIBS_CLJR)
#if	!defined(NOASM)
extern unsigned char far ExpandCLJRBase;// base of code to copy
extern unsigned char far DrawCLJRBase;	// base of code to copy
extern MOTIONTABLE ExpandCLJRTable;	// -> table for chunk to copy
extern MOTIONTABLE DrawCLJRTable;	// -> table for chunk to copy
#endif
void far ExpandDetailCodeBookCLJR	(EXPANDCODEBOOK_PROTO);
void far ExpandSmoothCodeBookCLJR	(EXPANDCODEBOOK_PROTO);
void far DrawKeyCLJR			(DRAWFRAME_PROTO);
void far DrawSmoothCLJR			(DRAWFRAME_PROTO);
void far DrawInterCLJR			(DRAWFRAME_PROTO);
CODE DetailCodeFromCLJR(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
CODE SmoothCodeFromCLJR(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);
#endif

#if	!defined(NOASM)
extern unsigned char far Expand8DoubledBase;	// base of code to copy
extern unsigned char far Draw8DoubledBase;	// base of code to copy
extern MOTIONTABLE Expand8DoubledTable;	// -> table for chunk to copy
extern MOTIONTABLE Draw8DoubledTable;		// -> table for chunk to copy
#endif
void far ExpandDetailCodeBook8Doubled	(EXPANDCODEBOOK_PROTO);
void far ExpandSmoothCodeBook8Doubled	(EXPANDCODEBOOK_PROTO);
void far DrawKey8Doubled	(DRAWFRAME_PROTO);
void far DrawSmooth8Doubled	(DRAWFRAME_PROTO);
void far DrawInter8Doubled	(DRAWFRAME_PROTO);
#define	DetailCodeFromDither8Doubled	SmoothCodeFromDither8
CODE SmoothCodeFromDither8Doubled(DENTRY far *pCodeBook, LPRGBQUAD lpRgbQ);

DCONTEXT *CVDecompressBegin(	// returns decompression context or 0
  short Width,			// 0 mod 4 width (pixels) of holding DIB
  short Height,			// 0 mod 4 height (scanlines) of holding DIB
  DIBTYPE DibType,		// type of destination DIB
  short Scale,			// dest:src stretching ratio (integer only)
  FRAME far *pInitialCodeBooks	// from CVDecompressEnd:  starting codebooks
);
FRAME far *CVDecompressEnd(	// nz if no error
  DCONTEXT *pContext,		// decompression context
  BOOL ReturnEndingCodeBooks,	// TRUE if we are to return pEndingCodeBooks
  LPRGBQUAD lpRgbQ		// palette to use for pEndingCodeBooks if 8 bit
);
int CVDecompress(		// nz if no error
  DCONTEXT *pD,			// decompression context
  FRAME huge *pFrame,		// frame to be decompressed
  unsigned long oBits,		// 32 bit offset of base of dest DIB
#if	!defined(WIN32)
  unsigned short sBits,		// selector to output DIB
#endif
  long YStep			// bytes from (x,y) to (x,y+1) in dest DIB
);
FRAME far *EndingCodeBooksFromContext(// return Cinepak codebook load frame
  DCONTEXT *pD,			// decompression context
  LPRGBQUAD lpRgbQ		// palette to use for pEndingCodeBooks if 8 bit
);

#if	defined(WIN32)
#  if	!defined(BigCopy)
#    define BigCopy(s,d,l)	MoveMemory((d),(s),(l))
#  endif
#else
void BigCopy(void far * pS, void far * pD, unsigned long sz);
#endif


void DecompressFixup(
  DCONTEXT *,			// decompress context
  unsigned long			// bitswitches of items to fix
);


#if	defined(WINCPK) || defined(__BEOS__)

/*
  In the incoming frame, there might be a segment boundary buried inside
  the header on an atom.  C pointers, even huge ones, won't correctly
  fiddle things to work right.  These macros do it all with huge char
  pointers.
 */

#define	AddrInHugeStruct(ptr, type, element) ( \
  ((unsigned char huge *) (ptr)) + offsetof(type, element) \
)

#define	ByteInHugeStruct(ptr, type, element) ( \
  AddrInHugeStruct((ptr), type, element)[0] \
)

#define	WordInHugeStruct(ptr, type, element) ( \
  (((unsigned short) AddrInHugeStruct((ptr), type, element)[1]) << 8) | \
  ((unsigned short) AddrInHugeStruct((ptr), type, element)[0]) \
)

#define	SwizWordInHugeStruct(ptr, type, element) ( \
  (((unsigned short) AddrInHugeStruct((ptr), type, element)[0]) << 8) | \
  ((unsigned short) AddrInHugeStruct((ptr), type, element)[1]) \
)

#define	SwizSizeInHugeStruct(ptr, type, element) ( \
  (((long) AddrInHugeStruct((ptr), type, element)[1]) << 16) | \
  (((long) AddrInHugeStruct((ptr), type, element)[2]) << 8) | \
  ((long) AddrInHugeStruct((ptr), type, element)[3]) \
)

#endif


#if	defined(NOASM)

// These structures and macros manage the bit switch dwords found in the
// incoming data stream.

typedef struct {
    unsigned long mask;		// last bit tested
    unsigned long bits;		// current switch word
} SWITCHCONTEXT;


#if	defined(WINCPK)

__inline
unsigned long swAdvance (SWITCHCONTEXT * sc, BYTE ** bfr)
{
    sc->mask = 0x80000000;
    sc->bits =  ((*bfr) [0] << 24) |
		((*bfr) [1] << 16) |
		((*bfr) [2] << 8) |
		((*bfr) [3]);

    (*bfr) += 4;
    return sc->mask & sc->bits;
}

#else
#if defined(__BEOS__)
static unsigned long swAdvance (SWITCHCONTEXT * sc, BYTE ** bfr)
{
    sc->mask = 0x80000000;
    sc->bits =  ((*bfr) [0] << 24) |
		((*bfr) [1] << 16) |
		((*bfr) [2] << 8) |
		((*bfr) [3]);

    (*bfr) += 4;
    return sc->mask & sc->bits;
}
#else
unsigned long swAdvance(SWITCHCONTEXT * sc, BYTE ** bfr);

#endif
#endif


#define DECLARE_SWITCHES()	SWITCHCONTEXT swC

#define INIT_SWITCHES()		swC.mask = 0;

#define NEXT_SWITCH(bfr)  \
    ( (swC.mask >>= 1) ? \
	    swC.mask & swC.bits : \
	    swAdvance (&swC, (BYTE **) &bfr) )


#endif
