/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/cvcompre.h 3.18 1995/10/02 11:49:28 bog Exp $

 * (C) Copyright 1992-1995 Radius Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of Radius Inc. and is provided pursuant to a Software
 * License Agreement.  This code is the proprietary information of
 * Radius and is confidential in nature.  Its use and dissemination by
 * any party other than Radius is strictly limited by the confidential
 * information provisions of the Agreement referenced above.

 * $Log: cvcompre.h $
 * Revision 3.18  1995/10/02 11:49:28  bog
 * Separate CClass and VClass parameters for MatchAndRefine for
 * clarity.
 * 
 * Revision 3.17  1995/03/14  08:24:04  bog
 * 1.  No one was looking at the remembered previous frame's smooth
 *     vectors, so there is no point in remembering them.
 * 2.  We update the previous frame's remembered detail vectors to be as
 *     refined (quantized) rather than as incoming, improving the decision
 *     about what to update in the next frame.
 * 
 * Revision 3.16  1995/02/22  12:38:00  bog
 * Add EndingCodeBooksFromContext so the decompression codebook tables
 * survive a destination depth change.
 * 
 * Revision 3.15  1994/12/14  10:49:56  bog
 * Make the frame size computations all long.
 * 
 * Revision 3.14  1994/11/08  10:38:50  bog
 * MIPS requires 8 byte alignment on jmp_buf.
 * 
 * Revision 3.13  1994/10/23  17:22:28  bog
 * Try to allow big frames.
 * 
 * Revision 3.12  1994/10/20  17:31:19  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 3.11  1994/09/08  13:53:36  bog
 * Access to FRAME and TILE should be huge except in WriteTile.
 * 
 * Revision 3.10  1994/08/30  17:33:02  bog
 * Make macro a bit more readable.
 * 
 * Revision 3.9  1994/08/16  13:19:38  timr
 * Values in the FrameSizes log can be > 32768; make them unsigned.
 * 
 * Revision 3.8  1994/05/26  09:46:50  bog
 * Dword align EXTCODE.
 * 
 * Revision 3.7  1994/05/25  12:35:55  timr
 * Use MoveMemory instead of CopyMemory where overlap can occur.
 * 
 * Revision 3.6  1994/05/11  18:40:33  unknown
 * Dword alignment in struct.
 *
 * Revision 3.5  1994/05/09  17:26:02  bog
 * Black & White compression works.
 *
 * Revision 3.4  1994/05/01  23:39:00  unknown
 * Move nCodes based max neighbors calculation down to MatchAndRefine().
 *
 * Revision 3.3  1994/05/01  16:07:03  unknown
 * Build the Neighbors list only in the first two rounds in ConvergeCodeBook.
 * In subsequent rounds, just reuse the last one built.
 *
 * Revision 3.2  1994/04/30  11:26:35  bog
 * Add nCodes basis for Neighbors range.  Seems more reasonable than nVectors.
 *
 * Revision 3.1  1994/04/06  16:34:20  bog
 * Add NeighborsRange to SYSTEM.INI stuff.  Maybe only experimental.
 *
 * Revision 3.0  1993/12/10  14:30:19  timr
 * Incorporate changes to accomodate C-only NT version of codec.
 *
 * Revision 2.16  1993/10/21  14:07:26  geoffs
 * Updated LOGREQUESTED stuff to include more data
 *
 * Revision 2.15  1993/10/21  09:50:03  geoffs
 * Added CD-ROM padding for compressions
 *
 * Revision 2.14  1993/10/12  17:24:34  bog
 * RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
 *
 * Revision 2.13  1993/10/06  16:38:09  timr
 * Squares table should be long for NT, short for Windows.
 *
 * Revision 2.12  1993/10/05  13:15:39  geoffs
 * Additional changes to handle keyrates of 0 for frame size generation
 *
 * Revision 2.11  1993/10/04  16:56:49  timr
 * Get rid of a stupid conditional compilation.
 *
 * Revision 2.10  1993/10/03  09:11:50  geoffs
 * Slightly more correct,optimal frame size generation
 *
 * Revision 2.9  1993/10/01  12:36:26  geoffs
 * Now processing ICM_COMPRESSFRAMESINFO w/internal frame size generation
 *
 * Revision 2.8  1993/09/24  14:07:37  geoffs
 * Have an approximation of progress for compression in call to Status callback
 *
 * Revision 2.7  1993/09/23  17:22:58  geoffs
 * Now correctly processing status callback during compression
 *
 * Revision 2.6  1993/08/04  19:10:29  timr
 * Both compressor and decompressor now work on NT.
 *
 * Revision 2.5  1993/07/24  13:03:18  timr
 * Fix some ifdefs.
 *
 * Revision 2.4  1993/07/06  08:50:50  geoffs
 * Based,far #defines, dword bdry padding, etc...
 *
 * Revision 2.3  93/07/03  11:44:49  geoffs
 * Fixes needed to compile WIN16
 *
 * Revision 2.2  93/07/02  16:30:16  geoffs
 * Now compiles,runs under Windows NT
 *
 * Revision 2.1  93/06/08  15:50:05  bog
 * Remove refs to ButtHeadCopyBits.
 *
 * Revision 2.0  93/06/01  14:13:36  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 *
 * Revision 1.31  93/05/02  15:46:58  bog
 * Add LogRequested flag in system.ini [iccvid.drv] to note requested size and
 * quality.
 *
 * Revision 1.30  93/04/21  15:47:05  bog
 * Fix up copyright and disclaimer.
 *
 * Revision 1.29  93/03/10  15:17:12  bog
 * Enable insertion of key frames.
 *
 * Revision 1.28  93/02/18  21:10:20  bog
 * pC->Flags.Neighbors is now pC->Neighbors.
 *
 * Revision 1.27  93/02/16  09:48:00  bog
 * Add RecerateSmoothFromDetail() definition.
 *
 * Revision 1.26  93/02/03  09:49:51  bog
 * Make NEIGHBORS a SYSTEM.INI parameter.
 *
 * Revision 1.25  93/02/01  14:09:30  bog
 * Performance highest when NEIGHBORS is largest.
 *
 * Revision 1.24  93/01/31  13:10:34  bog
 * Add logic to refine only vectors and codebook entries that are unlocked on
 * interframes.  Peter calls it "MSEFractional".
 *
 * Revision 1.23  93/01/27  23:03:36  bog
 * Get interframes working.
 *
 * Revision 1.22  93/01/27  08:00:05  geoffs
 * Added guard block in low DS to check against writes with nullish ->s
 *
 * Revision 1.21  93/01/25  22:28:33  geoffs
 * Parameter to SplitCodeBook should be VECTORBOOK *, not far *.
 *
 * Revision 1.20  93/01/25  14:24:16  geoffs
 * Match() needs valid Neighbor[] count.
 *
 * Revision 1.19  93/01/22  08:45:09  bog
 * Make Neighbor.Who into __base .pWho.
 *
 * Revision 1.18  93/01/21  18:43:15  timr
 * Make the H2INC stuff conditional on the symbol H2INC defines.
 *
 * Revision 1.17  93/01/21  12:29:04  bog
 * Back out MATCHERROR Match().
 *
 * Revision 1.16  93/01/21  12:10:50  bog
 * Qualify "Type"s in preparation for H2INC.
 *
 * Revision 1.15  93/01/20  13:55:53  geoffs
 * For now, make calls to MessageBox drop on the floor
 *
 * Revision 1.14  93/01/19  15:00:01  bog
 * HintedSize now unsigned long.
 *
 * Revision 1.13  93/01/19  09:48:52  geoffs
 * Replaced references to _fmemcpy with calls to internal BigCopy instead
 *
 * Revision 1.12  93/01/18  21:47:05  bog
 * Handle CodeSize and CodeBookSize.
 *
 * Revision 1.11  93/01/18  20:16:10  bog
 * Split the bottom level stuff out so it can be used by inter codebooks.
 *
 * Revision 1.10  93/01/17  15:53:31  bog
 * Compute distance using a squares table rather than multiplying.
 *
 * Revision 1.9  93/01/16  16:08:23  bog
 * Compressed first frame!
 *
 * Revision 1.8  93/01/13  09:33:34  timr
 * Restore DWORD alignment to CCONTEXT./
 *
 * Revision 1.7  93/01/12  17:13:27  bog
 * TILEs now have VECTORs instead of YYYYUVs.
 *
 * Revision 1.6  93/01/12  10:42:57  bog
 * Tiles now hold VECTORs, not YYYYUVs.
 *
 * Revision 1.5  93/01/12  09:07:18  timr
 * Manipulate the detail and smooth patch lists as VECTORs instead of
 * YYYYUVs.  This avoids a translation step later.
 *
 * Revision 1.4  93/01/11  17:37:16  geoffs
 * Accommodate 2 quality parameters, CVCompressEnd now returns a void
 *
 * Revision 1.3  93/01/11  09:45:52  bog
 * First halting steps towards integration.
 *
 * Revision 1.2  93/01/06  11:52:48  geoffs
 * Rationalized i/f's for CVCompress... and CVDecompress...
 *
 * Revision 1.1  93/01/06  10:12:34  bog
 * Initial revision
 *
 *
 * Compact Video Codec Compressor Internal Interfaces
 */

#if	defined(WINCPK)

#include <setjmp.h>
#ifdef	_H2INC
#include	"cv.h"
#endif
#include "w16_32.h"		// WIN32,WIN16 specific defines

#endif				// defined (WINCPK)

#ifdef __BEOS__
#include "beos.h"
#endif

typedef struct {		//  -------------
  unsigned char y[4];		// | y[0] | y[1] |  u,v filtered for
  unsigned char u;		// |------+------|   for whole patch
  unsigned char v;		// | y[2] | y[3] |
} YYYYUV;			//  -------------

typedef struct {		// long version, for accumulating for mean
  unsigned long y[4];
  unsigned long u;
  unsigned long v;
} LYYYYUV;

typedef struct {
  YYYYUV yuv;			// luma, chroma
  unsigned char Code;		// of matching entry in current codebook
  unsigned char Class;		// used to build balanced codebook
} VECTOR;

#define	NEIGHBORS	52	// keep track of this many nearest neighbors

typedef struct {
  unsigned short HowFar;	// distance to a neighbor; ffff is infinite
#ifdef	WIN32
  unsigned short padToDwordBdry;// for WIN32 dword boundary alignment
#endif
  struct EXTCODE BASED_SELF *pWho;// who that neighbor is
} NEIGHBOR;

typedef struct {
  int VRangeBottom;		// lower bound of range:  vectors
  int CRangeBottom;		// lower bound of range:  codes
  int nNeighbors;		// default number of neighbors in this range
  TCHAR *szIniNeighbors;	// SYSTEM.INI string
} NEIGHBORSRANGE[8];

typedef struct {		// codebook entry
  YYYYUV yuv;			// luma, chroma
  unsigned short Age;		// frames since 1st use; ffff is bad code
} CCODE;

typedef struct {		// compressor codebook
  CCODE far *pCodes;		// codebook entries
  short nCodes;			// number of entries in codebook
  short filler;
} CCODEBOOK;

typedef struct EXTCODE {	// extended codebook entry
  YYYYUV yuv;			// luma, chroma
  unsigned char Code;		// this entry's code
  unsigned char Flags;		// various flags
#define	fINVALID	1	// iff this code is not valid now
#define	fUNLOCKED	2	// iff this code changeable in interframe
#define	fUPDATED	4	// iff this code changed this interframe
  struct EXTCODE BASED_SELF *pHole;// chain of holes to fill
#if	!defined(WIN32)
  short filler;
#endif
  long nMatches;		// number of vectors that match
  LYYYYUV lyuv;			// for computing mean for next iteration
  unsigned long Error;		// total error for all matching codes
  NEIGHBOR Neighbor[NEIGHBORS];	// nearest neighbors list
} EXTCODE;

typedef struct {
  unsigned short FractionInterCodeBook : 1;// TRUE iff fraction refine on inter
  unsigned short LogRequested : 1;// TRUE iff remembering requested size & qual
#if !defined(NOBLACKWHITE)
  unsigned short BlackAndWhite : 1;// TRUE iff bloack & white, not color
#endif
} COMPRESSFLAGS;

typedef struct {		// compressor extended codebook with vectors
  EXTCODE far *pECodes;		// extended compressor codebook entries
  VECTOR huge *pVectors;	// array of vectors for detail/smooth patches
  short nCodes;			// number of extended codebook entries
  short nUpdateCodes;		// number of updates allowed if inter
  long nVectors;		// number of vectors (not patches)
  unsigned long Converged;	// convergence factor for LBG
  unsigned long TotalError;	// error relative to input
} VECTORBOOK;

typedef enum {			// indices into CODEBOOK
  Detail = 0,
  Smooth = 1
} CCODEBOOKINDEX;

typedef struct TILECONTEXT {	// !! WATCH DWORD ALIGNMENT !!
  struct TILECONTEXT *pNextTile;// -> next tile; 0 if none
  short TileNum;		// number of this tile, 0..(kMaxTileCount-1)

  short Height;			// 0 mod 4 height of this tile

  short DIBHeight;		// number of scanlines that go in this tile

  unsigned char tType;		// tile type
#if	defined(WIN32)
  unsigned char filler[1];
#else
  unsigned char filler[3];
#endif

  long nPatches;		// number of 4x4 patches in this tile

  VECTOR huge *pDetail;		// DIB translated to YYYYUV

  CCODEBOOK PBook[2];		// previous codebook detail then smooth

} TILECONTEXT;


typedef struct {
    DWORD dwFlags;
    LONG lQuality;
    LONG lDataRate;
    LONG lKeyRate;
    DWORD dwRate;
    DWORD dwScale;
    DWORD dwPadding;
    DWORD dwOverheadPerFrame;
    unsigned short ratioUpper;
    unsigned short ratioLower;
} COMPRESSINFO, *PCOMPRESSINFO, FAR *LPCOMPRESSINFO;


typedef struct {		// !! WATCH DWORD ALIGNMENT !!

#if	defined(WINCPK)
  /*
    used for Status callback and CompressFramesInfo
   */
  jmp_buf envAbort;		// setjmp/longjmp env for aborts
#ifndef	WIN32
  short filler3;		// for WIN16 alignment
#endif
#endif

  /*
    frame and tile dimensions
   */
  long DIBYStep;		// bytes from (x,y) to (x,y+1) in source DIB
  short DIBWidth;		// width in pixels of rectangle to compress
  short DIBHeight;		// height in scanlines of frame to compress
  DIBTYPE DIBType;		// type of DIB
#if defined(WIN32)
  short filler0;		// sizeof(enum) is 4
#endif

  short FrameWidth;		// 0 mod 4 width of compressed frame
  short FrameHeight;		// 0 mod 4 height of compressed frame

  unsigned char fType;		// frame type
  unsigned char filler1;

  long nPixels;			// number of pixels in compressed frame

  long Difference;		// average temporal difference per pixel
  long LastDifference;		// Difference on last frame
  long bRecompressingToKey;	// TRUE if recursing on CVCompress

  short FramesSinceKey;		// number of frames since key frame

  short Neighbors;		// max neighbors we remember
  short CurrNeighbors;		// how many we actually are remembering

  COMPRESSFLAGS Flags;		// session flags

  /*
    variables used by LBG
   */
  short huge *pDiffList;	// errors between this tile and prev
  short huge *pDetailList;	// errors between detail and smooth
  long DiffCount;		// number of 1s in pDiffList
  long DetailCount;		// number of 1s in pDetailList

  VECTORBOOK VBook[2];		// detail then smooth

  short NeighborsRange[8];	// nNeighbors based on nVectors
  short NeighborsMethod;	// based on nVectors or nCodes
  short NeighborsRounds;	// number of rounds to call MakeNeighbors()

  /*
    variables private to DIBToYUV
   */
  void far *pDIBToYUVPrivate;	// DIBToYUV private data

  /*
    variables per tile
   */
  TILECONTEXT *pT[kMaxTileCount];
  short nTiles;			// number of tiles in this frame

  short filler2;

  /*
    used for Status callback and CompressFramesInfo
   */
  struct {
    short fValidICCF;		// TRUE: doing our own internal crunching
    short FPU;			// frames per crunch unit
    long BPU;			// bytes per crunch unit
    long PadSize;		// size of padding unit, 0 if not padding
    long OverheadPerFrame;	// byte overhead per frame if padding
    unsigned short ratioUpper;	// keysize/intersize ratio multiplicand
    unsigned short ratioLower;	// keysize/intersize ratio dividend
#define	DFLT_KEYFRAMERATE	15
#define	MAX_FRAME_HISTORY	100
#define	MAX_FRAMESIZE		0xffff
    struct tagFRAMESIZES {	// where we keep history of sizes of frames
      long InterSize;
      long KeySize;
    } FrameSizes[MAX_FRAME_HISTORY];
    short InterframesLeft;	// remaining interframes before next key
    short nNew;			// where we add in newest FrameSizes entry
    long BytesInFirstCU;	// generated bytes in 1st of 2 CU periods
  } iccf;			// compress frames info to be remembered
  struct {
    LPARAM lParam;
#if	defined(WINCPK)
    LONG (CALLBACK *Status) (LPARAM lParam,UINT message,LONG l);
#endif
    short nMatchAndRefineCalls;	// calls to MatchAndRefine this compress
    short nMaxMatchAndRefineCalls;// worst case or previous # calls
  } icssp;

} CCONTEXT;

void CalcFrameSize(
  CCONTEXT *pC,			// -> compression context
  unsigned long HintedSize,	// requested hinted size
  unsigned long far *HintedSizeOut,// calc'ed hinted size
  unsigned long far *RecompressHintedSizeOut,// calc'ed size if recompress
  unsigned short far *SQuality,	// use quality
  unsigned short far *TQuality	// use quality
);

void UpdateFrameSize(
  CCONTEXT *pC,			// -> compression context
  long FrameSize,		// size of just compressed frame
  BOOL bKeyframe		// TRUE: generated frame was key
);

int CVSetStatusProc(		// init for ICSETSTATUSPROC
  CCONTEXT *pC,			// -> compression context
  ICSETSTATUSPROC far *pICSSP	// -> set status proc info
);

int CVCompressFramesInfo(	// init for ICCOMPRESSFRAMES
  CCONTEXT *pC,			// -> compression context
  COMPRESSINFO far *pCI		// -> compress frames info data
);

CCONTEXT *CVCompressBegin(	// returns compression context or 0
  short Width,			// width (pixels) of rect in source bitmap
  short Height,			// height (pixels) of rect in source bitmap
  long YStep,			// bytes from (x,y) to (x,y+1) in source DIB
  DIBTYPE DibType,		// type of source DIB
  void far *pLookup,		// -> lookup table for 8 bpp
  COMPRESSINFO far *pCI,	// -> compress frames info data
  ICSETSTATUSPROC far *pICSSP,	// -> set status proc info
  COMPRESSFLAGS Flags		// session flags
);

void CVCompressEnd(
  CCONTEXT *pContext		// compression context
);

long CVCompress(		// returns size else neg if error
  CCONTEXT *pContext,		// compression context
  unsigned long oBits,		// 32 bit offset of base of input DIB
#if	!defined(WIN32)
  unsigned short sBits,		// selector to input DIB
#endif
  FRAME huge *pF,		// destination compressed frame
  char *pKeyFrame,		// in/out keyframe flag
  unsigned long FrameSize,	// desired frame size; 0 if don't care
  unsigned short SQuality,	// 0..1023 spatial quality slider
  unsigned short TQuality	// 0..1023 temporal quality slider
);
#define	CVCOMPRESS_ERR_ERROR	-1	// generic error from CVCompress
#define	CVCOMPRESS_ERR_ABORT	-2	// user abort from CVCompress

long CompressTile(		// returns tile size or neg if error
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  unsigned long oBits,		// 32 bit offset of base of input tile
#if	!defined(WIN32)
  unsigned short sBits,		// selector to input tile
#endif
  TILE huge *pTile,		// where to build the compressed tile
  unsigned long HintedSize,	// if caller gave hint at desired size
  unsigned short SQuality,	// 0..1023 spatial quality slider
  unsigned short TQuality	// 0..1023 temporal quality slider
);

CCONTEXT *UnwindCompressBegin(	// returns 0
  CCONTEXT *pC			// compression context
);

void DIBToVectors(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  unsigned long oBits,		// 32 bit offset of base of input tile in DIB
#if	!defined(WIN32)
  unsigned short sBits,		// selector to input DIB
#endif
  unsigned long HintedSize,	// if caller gave hint at desired size
  unsigned short SQuality,	// 0..1023 spatial quality slider
  unsigned short TQuality	// 0..1023 temporal quality slider
);

void DIBToYUV(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  unsigned long oBits		// 32 bit offset of base of input tile in DIB
#if	!defined(WIN32)
  ,unsigned short sBits		// selector to input DIB
#endif
);

int DIBToYUVBegin(		// return 0 if error
  CCONTEXT *pC,			// compression context
  void far *pLookup		// -> 8 bpp lookup table?
);

void DIBToYUVEnd(
  CCONTEXT *pC			// compression context
);

unsigned long DifferenceList(	// returns total difference
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT		// tile context
);

void DetailList(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT		// tile context
);

void RateControl(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  unsigned long HintedSize,	// if caller gave hint at desired size
  unsigned short SQuality,	// 0..1023 spatial quality slider
  unsigned short TQuality	// 0..1023 temporal quality slider
);

void RecreateSmoothFromDetail(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT		// current tile context
);

void GenerateVectors(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT		// tile context
);

void CodeBook(			// build or adapt and then refine a codebook
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  VECTORBOOK *pVB,		// current codebook
  CCODEBOOK *pPB		// prev codebook
);

void RememberQuantized(		// remember quantized vectors for next frame
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT		// tile context
);

void KeyCodeBook(		// build and refine a new codebook
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  VECTORBOOK *pVB		// current codebook
);

void InterCodeBook(		// adapt and refine an old codebook
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  VECTORBOOK *pVB,		// current codebook
  CCODEBOOK *pPB		// prev codebook
);

short SplitCodeBook(		// returns # entries used up
  VECTORBOOK *pVB,		// vectors and codebook to fill out
  short iFirstCode,		// index of first candidate codebook entry
  long iFirstVector,		// index of first vector in class
  unsigned char Class,		// vector class to search
  EXTCODE far * far *ppHoles,	// ->-> first hole to fill
  short nHoles,			// number of holes to fill
  unsigned char IgnFlags	// ignore entries with any of these bits set
);

unsigned long ConvergeCodeBook(	// converge a new codebook; return last error
  CCONTEXT *pC,			// compression context
  VECTORBOOK *pVB,		// current codebook
  unsigned char VClass,		// vector class to match; 0xff means any
  unsigned char CClass		// any bit on here must be on in EC Flags
);

unsigned long MatchAndRefine(	// LBG refine codebook
  CCONTEXT *pC,			// compression context
  VECTORBOOK *pVB,		// current codebook
  int RecomputeNeighbors,	// TRUE if allowed to recompute Neighbors list
  unsigned char VClass,		// vector class to match; 0xff means any
  unsigned char CClass		// any bit on here must be on in EC Flags
);

unsigned long Match(		// find & remember nearest code, return error
  VECTOR huge *pV,		// vector to match
  EXTCODE far *pEC,		// -> base of code table
  short nCodes,			// number of codes against which to match
  unsigned short nNeighbors,	// number of entries in the Neighbor[] table
  unsigned char Flags		// whether to look at codes that are locked
);

short MakeNeighbors(		// build distance tables in codebook
  EXTCODE far *pE,		// codebook and extension
  int nCodes,			// number of entries (Win16 ? short : long)
  unsigned short nNeighbors,	// maximum Neighbors in list
  unsigned char Flags		// whether to look at codes that are locked
);

long WriteTile(			// write tile to frame; return size or err
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  TILE huge *pTile		// where to build the compressed tile
);

#if	!defined(WIN32)
unsigned long muldiv32(		// 32x32 mul = 64 result, divided by 32 value
  unsigned long top1,		// multiplier 1
  unsigned long top2,		// multiplier 2
  unsigned long bottom		// divider
);
#else
#  define muldiv32(m1,m2,d)	MulDiv(m1,m2,d)
#endif

/*
  Square of the distance between two YYYYUVs, as an unsigned long
 */
extern unsigned int Squares[511];// squares of -255..255

#define	LongDistance(a, b) ( \
  ((unsigned long) Squares[255 + ((short) (a).y[0]) - ((short) (b).y[0])]) + \
  ((unsigned long) Squares[255 + ((short) (a).y[1]) - ((short) (b).y[1])]) + \
  ((unsigned long) Squares[255 + ((short) (a).y[2]) - ((short) (b).y[2])]) + \
  ((unsigned long) Squares[255 + ((short) (a).y[3]) - ((short) (b).y[3])]) + \
  ((unsigned long) (Squares[255 + ((short) (a).u) - ((short) (b).u)] << 2)) + \
  ((unsigned long) (Squares[255 + ((short) (a).v) - ((short) (b).v)] << 2)) \
)

#define	LongSharpness(a, b) ( \
  ((unsigned long) Squares[255 + (a) - ((short) (b).y[0])]) + \
  ((unsigned long) Squares[255 + (a) - ((short) (b).y[1])]) + \
  ((unsigned long) Squares[255 + (a) - ((short) (b).y[2])]) + \
  ((unsigned long) Squares[255 + (a) - ((short) (b).y[3])]) \
)

#ifdef WIN32
#  ifndef BigCopy
#    ifdef __BEOS__
#      define BigCopy(s,d,l)	memmove((d),(s),(l))
#    else
#      define BigCopy(s,d,l)	MoveMemory((d),(s),(l))
#    endif
#  endif
#else
void BigCopy(void far * pS, void far * pD, unsigned long sz);
#endif

// make sure we haven't written into our own data segment with null ->s
#if	defined(DEBUG) && !defined(WIN32)
#define	CHECKF0AD(str)	{\
  extern unsigned short F0ADNearBase[256];\
  unsigned long cs;\
  int i;\
  for (cs = 0, i = 0; i < 256; i++) {\
    cs += F0ADNearBase[i];\
  }\
  if (cs != (256L * (unsigned) 0xf0ad)) {\
    MessageBox(NULL,"F0AD problem",str,MB_OK);\
  }\
}
#else
#define	CHECKF0AD(str)
#endif

#if	defined(WINCPK)
//
// macros to implement callbacks to MSVIDEO status,yield functions
// during compression
//

#define	VFW_STATUS_CALLBACK(pc,msg,l)	( \
  (pc)->icssp.Status \
  ? ( \
    (*((pc)->icssp.Status)) ((pc)->icssp.lParam, (msg), (l)) \
    ? (longjmp((pc)->envAbort,1),1) : 0 \
  ) : 0 \
)

#define	VFW_YIELD(pc)		VFW_STATUS_CALLBACK((pc),ICSTATUS_YIELD,0)
#define	VFW_STATUS(pc,amt) 	VFW_STATUS_CALLBACK((pc),ICSTATUS_STATUS,(amt))
#define	VFW_START(pc)		VFW_STATUS_CALLBACK((pc),ICSTATUS_START,0)
#define	VFW_END(pc)		VFW_STATUS_CALLBACK((pc),ICSTATUS_END,0)

#endif

#ifdef __BEOS__
#define	VFW_YIELD(pc)
#define	VFW_STATUS(pc,amt)
#define	VFW_START(pc)
#define	VFW_END(pc)
#endif
