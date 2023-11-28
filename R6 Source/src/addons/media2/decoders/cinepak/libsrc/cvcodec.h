/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/cvcodec.h 2.12 1995/02/22 12:37:57 bog Exp $

 * (C) Copyright 1992-1995 Radius Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of Radius Inc. and is provided pursuant to a Software
 * License Agreement.  This code is the proprietary information of
 * Radius and is confidential in nature.  Its use and dissemination by
 * any party other than Radius is strictly limited by the confidential
 * information provisions of the Agreement referenced above.

 * $Log: cvcodec.h $
 * Revision 2.12  1995/02/22 12:37:57  bog
 * Add EndingCodeBooksFromContext so the decompression codebook tables
 * survive a destination depth change.
 * Revision 2.11  1994/09/08  13:52:55  bog
 * Access to FRAME and TILE should be huge except in WriteTile.
 * 
 * Revision 2.10  1994/05/09  17:26:01  bog
 * Black & White compression works.
 * 
 * Revision 2.9  1994/03/29  13:19:39  bog
 * Allow YStep to change between frames of a movie.
 * 
 * Revision 2.8  1994/03/02  16:09:04  timr
 * Add CVDecompressSetPalette.
 * 
 * Revision 2.7  1993/11/29  08:04:16  geoffs
 * Pixel doubling done for 8bpp decompress
 * 
 * Revision 2.6  1993/10/12  17:24:25  bog
 * RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
 * 
 * Revision 2.5  1993/10/01  12:36:18  geoffs
 * Now processing ICM_COMPRESSFRAMESINFO w/internal frame size generation
 * 
 * Revision 2.4  1993/09/23  17:22:56  geoffs
 * Now correctly processing status callback during compression
 * 
 * Revision 2.3  1993/07/02  16:23:31  geoffs
 * Now compiles,runs under Windows NT
 * 
 * Revision 2.2  93/06/14  11:34:29  bog
 * Remove CVFAR; we will clone for Thun acceleration.
 * 
 * Revision 2.1  93/06/13  11:20:49  bog
 * Add hooks for playback acceleration.
 * 
 * Revision 2.0  93/06/01  14:13:28  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.13  93/05/02  15:46:45  bog
 * Add LogRequested flag in system.ini [iccvid.drv] to note requested size and
 * quality.
 * 
 * Revision 1.12  93/04/21  15:46:54  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.11  93/02/18  14:34:28  bog
 * NEIGHBORS no longer a SYSTEM.INI parameter.
 * 
 * Revision 1.10  93/02/03  09:49:46  bog
 * Make NEIGHBORS a SYSTEM.INI parameter.
 * 
 * Revision 1.9  93/01/31  13:10:28  bog
 * Add logic to refine only vectors and codebook entries that are unlocked on
 * interframes.  Peter calls it "MSEFractional".
 * 
 * Revision 1.8  93/01/11  17:35:57  geoffs
 * Accommodate 2 quality parameters, CVCompressEnd now returns a void
 *
 * Revision 1.7  93/01/11  10:21:28  geoffs
 * Add , to parameter in CVCompressBegin
 *
 * Revision 1.6  93/01/10  15:35:18  geoffs
 * Removed P8LOOKUP definition and instead made -> lookup a void far *
 *
 * Revision 1.5  93/01/10  15:29:12  geoffs
 * Added -> lookup table for 8 bpp into CVCompressBegin
 *
 * Revision 1.4  93/01/06  11:52:17  geoffs
 * Rationalized i/f's for CVCompress... and CVDecompress...
 *
 * Revision 1.3  93/01/06  10:07:33  bog
 * Compressor interface changes.  More to come, I'm sure.
 *
 * Revision 1.2  92/12/23  14:49:02  bog
 * Added Depth to CVCompressBegin, anticipating other than 24bpp incoming.
 *
 * Revision 1.1  92/12/21  11:36:12  bog
 * Initial revision
 *
 *
 * Compact Video Codec external interface
 */


typedef struct {
  unsigned short FractionInterCodeBook : 1;// TRUE iff fraction refine on inter
  unsigned short LogRequested : 1;// TRUE iff remembering requested size & qual
#if !defined(NOBLACKWHITE)
  unsigned short BlackAndWhite : 1;// TRUE iff bloack & white, not color
#endif
} COMPRESSFLAGS;

extern int CVSetStatusProc(	// init for ICSETSTATUSPROC
  void *pC,			// -> compression context
  ICSETSTATUSPROC far *pICSSP	// -> set status proc info
);
extern int CVCompressFramesInfo(
  void *pC,			// -> compression context
  COMPRESSINFO far *pCI		// -> compress frames info data
);
extern void *CVCompressBegin(	// returns compression context or 0
  short Width,			// width (pixels) of rect in source bitmap
  short Height,			// height (pixels) of rect in source bitmap
  long YStep,			// bytes from (x,y) to (x,y+1) in source DIB
  DIBTYPE DibType,		// type of source DIB
  void far *pLookup,		// -> lookup table for 8 bpp
  COMPRESSINFO far *pCI,	// -> compress frames info data
  ICSETSTATUSPROC far *pICSSP,	// -> set status proc info
  COMPRESSFLAGS Flags		// session flags
);
extern void CVCompressEnd(
  void *pContext		// compression context
);
extern long CVCompress(		// returns size else neg if error
  void *pContext,		// compression context
  unsigned long oBits,		// 32 bit offset of base of input DIB
#if	!defined(WIN32)
  unsigned short sBits,		// selector to input DIB
#endif
  void huge *pOutput,		// destination compressed frame
  char *pKeyFrame,		// in/out keyframe flag
  long FrameSize,		// desired frame size; 0 if don't care
  unsigned short SQuality,	// 0..1023 spatial quality slider
  unsigned short TQuality	// 0..1023 temporal quality slider
);
#define	CVCOMPRESS_ERR_ERROR	-1	// generic error from CVCompress
#define	CVCOMPRESS_ERR_ABORT	-2	// user abort from CVCompress

extern void *CVDecompressBegin(// returns decompression context or 0
  short Width,			// 0 mod 4 width (pixels) of dest DIB
  short Height,			// 0 mod 4 height (scanlines) of dest DIB
  DIBTYPE DibType,		// type of dest DIB
  short Scale,			// dest:src stretching ratio (integer only)
  void far *pInitialCodeBooks	// from CVDecompressEnd:  starting codebooks
);
extern void far *CVDecompressEnd(// nz if no error
  void *pContext,		// decompression context
  BOOL ReturnEndingCodeBooks,	// TRUE if we are to return pEndingCodeBooks
  LPRGBQUAD lpRgbQ		// palette to use for pEndingCodeBooks if 8 bit
);
extern int CVDecompress(	// nz if no error
  void *pContext,		// decompression context
  void far *pInput,		// frame to be decompressed
  unsigned long oBits,		// 32 bit offset of base of dest DIB
#if	!defined(WIN32)
  unsigned short sBits,		// selector to output DIB
#endif
  long YStep			// bytes from (x,y) to (x,y+1) in dest DIB
);
extern int CVDecompressSetPalette(
  void *pContext,		// decompression context
  LPRGBQUAD,			// new palette
  int				// number of colors in new palette
);

extern RGBQUAD far stdPAL8[];	// standard 8bpp cv dither palette
extern WORD FAR numStdPAL8;
