/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/iccv.h 3.12 1994/12/14 10:50:05 bog Exp $

 * (C) Copyright 1992-1993 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: iccv.h $
 * Revision 3.12  1994/12/14 10:50:05  bog
 * Default key frame every 15 frames; default quality 100%.
 * Revision 3.11  1994/10/05  10:38:57  bog
 * Fix 4CC for CLJR.
 * 
 * Revision 3.10  1994/09/22  17:03:29  bog
 * Use 4CC for DIBS_xxx; add UYVY.
 * 
 * Revision 3.9  1994/07/29  17:22:21  bog
 * Disallow non 0 mod 4 YUV420Planar.
 * 
 * Revision 3.8  1994/07/29  15:31:39  bog
 * Weitek YUV420 works.
 * 
 * Revision 3.7  1994/07/22  16:27:26  bog
 * Declare 4CCs and DIBType for YUV DIBs.
 * 
 * Revision 3.6  1994/07/13  13:24:41  bog
 * 1.  BI_1632 not valid if WIN32.
 * 2.  If frame size changes after a CompressBegin, just rebegin.  Premiere
 *     uses small frames to time compression.
 * 
 * Revision 3.5  1994/05/25  12:36:53  timr
 * Use MoveMemory instead of CopyMemory where overlap can occur.
 * 
 * Revision 3.4  1994/05/09  17:26:09  bog
 * Black & White compression works.
 *
 * Revision 3.3  1994/05/06  13:42:01  bog
 * Play back black and white movies.
 *
 * Revision 3.2  1994/03/29  13:20:23  bog
 * Allow YStep to change between frames of a movie.
 *
 * Revision 3.1  1994/03/02  16:24:01  timr
 * Add support for ICM_DECOMPRESS_SET_PALETTE.
 *
 * Revision 3.0  1993/12/10  14:48:02  timr
 * Minor changes to accomodate NT C-only codec.
 *
 * Revision 2.21  1993/11/29  16:04:45  geoffs
 * Pixel doubling done for 8bpp decompress
 *
 * Revision 2.20  1993/11/05  11:36:46  bog
 * fDrawInitialized and fDecompressInitialized no longer used.
 *
 * Revision 2.19  1993/11/04  22:24:59  bog
 * Remove acceleration hook; use VIDS.CVID instead.
 *
 * Revision 2.18  1993/10/28  08:07:56  bog
 * Make default Key to Inter 2.1:1.
 *
 * Revision 2.17  1993/10/21  09:50:27  geoffs
 * Added CD-ROM padding for compressions
 *
 * Revision 2.16  1993/10/14  17:42:47  timr
 * Fix RETURN macro again.  "return" can't be second part of a comma
 * operator.  Also need separate macros for 0 and 1 parameter.
 *
 * Revision 2.15  1993/10/14  16:18:22  geoffs
 * Change key to inter default ratio to 2.5 to 1
 *
 * Revision 2.14  1993/10/13  10:54:50  bog
 * Fix RETURN macro; slightly broken DecompressedDibType.
 *
 * Revision 2.13  1993/10/12  17:25:39  bog
 * RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
 *
 * Revision 2.12  1993/10/05  13:16:25  geoffs
 * Additional changes to handle keyrates of 0 for frame size generation
 *
 * Revision 2.11  1993/10/01  12:37:00  geoffs
 * Now processing ICM_COMPRESSFRAMESINFO w/internal frame size generation
 *
 * Revision 2.10  1993/09/23  17:23:03  geoffs
 * Now correctly processing status callback during compression
 *
 * Revision 2.9  1993/09/22  10:28:48  geoffs
 * Default keyframe rate changed to 7 as per MS request
 *
 * Revision 2.8  1993/09/09  09:18:42  geoffs
 * Add CompressFrames* declarations
 *
 * Revision 2.7  1993/08/04  12:06:57  timr
 * Both compressor and decompressor now run on NT.
 *
 * Revision 2.6  93/07/02  16:30:25  geoffs
 * Now compiles,runs under Windows NT
 *
 * Revision 2.5  93/06/17  11:38:55  geoffs
 * Fix up dialog box.
 *
 * Revision 2.4  93/06/17  08:51:19  timr
 * Add symbols for stringtable resources.
 *
 * Revision 2.3  93/06/13  11:21:45  bog
 * Add hooks for playback acceleration.
 *
 * Revision 2.2  93/06/08  16:49:49  geoffs
 * Removed DRAW stuff, cleanup Decompress rectangles
 *
 * Revision 2.1  93/06/03  10:13:40  geoffs
 * First cut at rework for VFW 1.5
 *
 * Revision 2.0  93/06/01  14:15:34  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 *
 * Revision 1.34  93/04/21  15:49:40  bog
 * Fix up copyright and disclaimer.
 *
 * Revision 1.33  93/02/18  14:54:12  geoffs
 * Added quality override capability from system.ini
 *
 * Revision 1.32  93/02/18  14:13:57  geoffs
 * Changed default quality to be 100%
 *
 * Revision 1.31  93/02/16  10:58:09  bog
 * Remove hinted size smoothing.
 *
 * Revision 1.30  93/02/05  13:30:39  geoffs
 * Added hinted size smoothing during compression
 *
 * Revision 1.29  93/02/02  10:37:07  geoffs
 * Added auto-determination of best path to choose for decompress
 *
 * Revision 1.28  93/01/25  14:26:37  geoffs
 * Allow non 0 mod 4 frames.
 *
 * Revision 1.27  93/01/21  10:37:51  geoffs
 * CVDecompress{Begin,End} now have same brackets as allocation of DIB for
 * bits buffer for decompression.
 *
 * Revision 1.26  93/01/12  10:44:06  bog
 * CV quality computation incorrect.
 *
 * Revision 1.25  93/01/06  11:51:44  geoffs
 * Rationalized i/f's for CVCompress... and CVDecompress...
 *
 * Revision 1.24  92/12/24  10:29:37  geoffs
 * Keyframe indication comes into Compress in the lpdwFlags, not the dwFlags
 *
 * Revision 1.23  92/12/23  14:49:33  geoffs
 * Partitioned the INSTINFO struct into Compress and {Decompress,Draw}
 * sections (ie: variables for each in separate substructs).
 *
 * Revision 1.22  92/12/22  14:22:39  geoffs
 * Added GetDefaultKeyframeRate function
 *
 * Revision 1.21  92/12/22  14:14:16  geoffs
 * Added ...Quality functions
 *
 * Revision 1.20  92/12/22  13:06:32  geoffs
 * New layering between API and CV implemented in Draw path
 *
 * Revision 1.19  92/12/21  18:43:55  timr
 * Decompressor now works split.
 *
 * Revision 1.18  92/12/21  11:38:03  bog
 * Split into IC and CV layers.
 *
 * Revision 1.17  92/12/17  09:25:56  geoffs
 * Added lots of new stuff for improved debugging output. Now have ability
 * to define debugging output on/off and destination via keyword in the
 * system.ini file (in the [iccvid.drv] section).
 *
 * Revision 1.16  92/12/11  09:39:06  geoffs
 * When will it ever end! Yet more accommodations to MS kluges in the DRAW
 * path. It was working in Media Player but then it wouldn't work in VidEdit
 * because of palette problems. Now it works in both but only under funny
 * contrived conditions in the driver. It's still not quite correct.
 *
 * Revision 1.15  92/12/10  10:01:28  geoffs
 * Lots of fiddles having to do with DRAW palette realization, adjustment
 * to Microsoft codec kluges (ie: DrawEnd should not free up the internal
 * bits buffer), addition of lots of debug output.
 *
 * Revision 1.14  92/12/09  11:54:34  geoffs
 * Made changes to DPF macro, added macro called RETURN.
 *
 * Revision 1.13  92/12/08  14:36:48  geoffs
 * Added DRAW message functionality into codec
 *
 * Revision 1.12  92/12/02  10:48:39  geoffs
 * Added support for 8 bit dithering
 *
 * Revision 1.11  92/11/29  15:53:12  bog
 * Cleaned up 24bpp path.  Now it's faster.
 *
 * Revision 1.10  92/11/13  14:00:10  geoffs
 * Changed CompactVideo FOURCC code to 'cvid'
 *
 * Revision 1.9  92/11/11  13:00:23  geoffs
 * We now use the version resource to display info in dialog boxes
 *
 * Revision 1.8  92/11/07  17:01:03  bog
 * Problems with 2nd tile; USE32 works.
 *
 * Revision 1.7  92/11/05  16:56:43  bog
 * USE32 segment for Draw and Expand works.
 *
 * Revision 1.6  92/11/01  21:00:23  bog
 * DrawKey24 compiles.
 *
 * Revision 1.5  92/11/01  14:45:12  bog
 * First successful compile with Expand24 done.
 *
 * Revision 1.4  92/10/31  23:12:31  bog
 * Seems pretty healthy with empty asm bottom.
 *
 * Revision 1.3  92/10/31  15:41:32  bog
 * First successful compile.  Stubbed out bottom.
 *
 * Revision 1.2  92/10/31  12:09:02  bog
 * About to try explosive first compile of top level.
 *
 * Revision 1.1  92/10/28  13:27:03  geoffs
 * Initial revision
 *
 *
 * Compact Video Codec structs and definitions
 */

#include "w16_32.h"		// WIN32,WIN16 specific defines

#ifdef __BEOS__
#include "compddk.h"
#endif

typedef	ICOPEN			*PICOPEN;
typedef	ICOPEN NEAR		*NPICOPEN;
typedef	ICOPEN FAR		*LPICOPEN;
typedef	ICINFO			*PICINFO;
typedef	ICINFO NEAR		*NPICINFO;
typedef	ICINFO FAR		*LPICINFO;
typedef	ICDECOMPRESS		*PICDECOMPRESS;
typedef	ICDECOMPRESS NEAR	*NPICDECOMPRESS;
typedef	ICDECOMPRESS FAR	*LPICDECOMPRESS;
typedef	ICCOMPRESS		*PICCOMPRESS;
typedef	ICCOMPRESS NEAR		*NPICCOMPRESS;
typedef	ICCOMPRESS FAR		*LPICCOMPRESS;
typedef ICCOMPRESSFRAMES	*PICCOMPRESSFRAMES;
typedef ICCOMPRESSFRAMES NEAR	*NPICCOMPRESSFRAMES;
typedef ICCOMPRESSFRAMES FAR	*LPICCOMPRESSFRAMES;
typedef	ICDRAWBEGIN		*PICDRAWBEGIN;
typedef	ICDRAWBEGIN NEAR	*NPICDRAWBEGIN;
typedef	ICDRAWBEGIN FAR		*LPICDRAWBEGIN;
typedef	ICDRAW			*PICDRAW;
typedef	ICDRAW NEAR		*NPICDRAW;
typedef	ICDRAW FAR		*LPICDRAW;
typedef	ICSETSTATUSPROC FAR	*LPICSETSTATUSPROC;

#define	MODNAME			TEXT("ICCVID")
#define	FOURCC_CV		mmioFOURCC('c','v','i','d')
#define	TWOCC_CV		cktypeDIBcompressed
#define	BI_CV			FOURCC_CV
#define	VERSION_CV		0x00010000L

// Weitek Cinepak YUV420 planar
#define	BI_CPLA			mmioFOURCC('C','P','L','A')
#define	DIBS_CPLA		2

// various YUV422
#define	BI_YUY2			mmioFOURCC('Y','U','Y','2')
#define	DIBS_YUY2		4

// Cirrus 5440
#define	BI_UYVY			mmioFOURCC('U','Y','V','Y')
#define	DIBS_UYVY		8

// Cirrus packed YUV411
#define	BI_CLJR			mmioFOURCC('C','L','J','R')
#define	DIBS_CLJR		16

#define	RENDER_CONDITIONAL	-1	// decompress via best path
#define	RENDER_DECOMPRESS	0	// decompress via DECOMPRESS path
#define	RENDER_DRAW		1	// decompress via DRAW path

#define	DRAWPATH_BITBLT		0	// do DRAW via BitBlt
#define	DRAWPATH_DEVICE		1	// do DRAW via SetDIBitsToDevice

#ifdef DEBUG
extern void FAR CDECL dprintf(LPTSTR, ...);
#define	DPF			iNest++,dprintf
extern void FAR CDECL dprintf2(WORD, ...);
#define	DPF2(X)			iNest++,dprintf2 X
#define	FUNCNAME		0
#define	ICOPENPTR		1
#define	INSTINFOPTR		2
#define	ICINFOPTR		3
#define	DWSIZE			4
#define	BMIHPTR			5
#define	STATEPTR		6
#define	FCC			7
#define	BITCOUNT		8
#define	HDCPTR			9
#define	ICDECOMPRESSPTR		10
#define	ICCOMPRESSPTR		11
#define	BOOLEAN			12
#define	ICDRAWBEGINPTR		13
#define	ICDRAWPTR		14
#define	DWKEYFRAME		15
#define	DWQUALITY		16
#define	DWVALUE			17
#define	SRCDESTPARAMS		18
#define	ICCFPTR			19
#define	ICSSPPTR		20
#define	RETURNVOID		{iNest--;return;}
#define	RETURN(k)		{iNest--;return(k);}
#else
#ifdef __BEOS__
#define DPF(X)
#define DPF2(X) 
#else
#define	DPF			/ ## /
#define	DPF2			/ ## /
#endif
#define RETURNVOID		return
#define	RETURN			return
#endif

// String resource IDs.

#define	IDS_NAME		12001
#define	IDS_DESCRIPTION		12002
#define	IDS_VERSION		12003

#define	WIDTHBYTES(bits)	(((((unsigned) (bits)) + 31) & ~31) >> 3)

#define	DFLT_QUALITY		10000L		// scale 0...10000
#define	CV_QUALITY(a)	( \
  (unsigned short) ((1023L * (unsigned long) (a))/10000L) \
)

#define	DFLT_KEYFRAMERATE	15		// every 15 frames


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
#define	CompressibleDIB		0x007f // BEOS 0x001f


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
#define	DFLT_KEY_TO_INTER_RATIO_UPPER	21
#define	DFLT_KEY_TO_INTER_RATIO_LOWER	10


typedef struct {

  unsigned long Flags;

  char fCompressInitialized;	/* Compress initialization */
#if !defined(NOBLACKWHITE)
  char fBlackAndWhite;		// TRUE iff compression is black & white

  char filler0[2];
#else
  char filler0[3];
#endif

  int last_keyframe;

  struct { // Compress: info needing to be remembered
    void *CContext;		// compression context

    void *filler1;

    long biWidth;		// as it came in BITMAPINFOHEADER
    long biHeight;

    DWORD YStep;		// caller's bytes from (x,y+1) to (x,y)

    long lPrevFrameNum;		// previous frame compressed
    char KeyFrame;		// in:s/b or not keyframe, out:was or not

    char filler2;
    char filler3;
    char filler4;

    DWORD dwRequestedQuality;	// quality requested for compression
    DWORD dwRealizedQuality;	// quality used internally for compression

    COMPRESSINFO ci;		// latched data when outside of compress stream
    ICSETSTATUSPROC icssp;	// set status proc info
  } CStuff;

  struct { // Decompress, DecompressEx: info needing to be remembered
    void *DContext;		// decompression context

    int nPalColors;             // number of colors in users palette
    LPRGBQUAD rgbqUser;         // -> RGBs in users palette; null if default

    unsigned long Flags;	// decompression flags
#define	DECOMPRESS_USE_BUFFER	1	// 1:using decompression buffer

    /*
      common to all views of destinations
     */
    short Depth;		// pixel depth in bits
    short DepthInBytes;		// ... in bytes

    DIBTYPE DIBType;		// type of DIB
    DIBTYPE filler7;

    /*
      stuff for actual image rectangle
     */
    short WidthImage;		// destination rectangle width
    short HeightImage;		// destination rectangle height

    /*
      stuff for 0 mod 4 rectangle we decompress into
     */
    short Width0Mod4;		// compression rectangle width
    short Height0Mod4;		// compression rectangle height

    char far *pBits;		// -> 0 mod 4 rectangle as returned by alloc

#if	!defined(WIN32)
    unsigned short s0Mod4;	// selector of 0 mod 4 rectangle

    short filler8;
#endif
    unsigned long o0Mod4;	// offset to ul of 0 mod 4 rectangle

    long YStep0Mod4;		// top to bottom step in 0 mod 4 rectangle

    /*
      stuff for final destination DIB
     */
    long biWidth;		// from BITMAPINFOHEADER
    long biHeight;
    long YStepDIB;		// top to bottom step in destination DIB
    unsigned long o00DIB;	// offset to (0,0) in destination DIB

  } DStuff;

	ICDECOMPRESSEX px;
	ICCOMPRESS icc;
} INSTINFO, *PINSTINFO;

// from ICCV.C ...
BOOL NEAR PASCAL Load(void);
void NEAR PASCAL Free(void);
PINSTINFO NEAR PASCAL Open(ICOPEN FAR *);
DWORD NEAR PASCAL Close(PINSTINFO);
DWORD NEAR PASCAL GetState(PINSTINFO,LPVOID,DWORD);
DWORD NEAR PASCAL SetState(PINSTINFO,LPVOID,DWORD);
DWORD NEAR PASCAL GetInfo(PINSTINFO,ICINFO FAR *,DWORD);
LRESULT NEAR PASCAL Get(PINSTINFO,FOURCC,DWORD);
LRESULT NEAR PASCAL Set(PINSTINFO,DWORD,DWORD);
DWORD NEAR PASCAL GetDefaultKeyframeRate(LPDWORD);
DWORD NEAR PASCAL GetQuality(PINSTINFO,LPDWORD);
DWORD NEAR PASCAL GetDefaultQuality(LPDWORD);
DWORD NEAR PASCAL SetQuality(PINSTINFO,DWORD);
BOOL NEAR PASCAL QueryAbout(void);
DWORD NEAR PASCAL About(HWND);
BOOL NEAR PASCAL QueryConfigure(PINSTINFO);
DWORD NEAR PASCAL Configure(PINSTINFO,HWND);
DWORD NEAR PASCAL CompressBegin(
  PINSTINFO, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER
);
DWORD NEAR PASCAL CompressQuery(
  PINSTINFO, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER
);
DWORD NEAR PASCAL CompressGetFormat(
  PINSTINFO, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER
);
DWORD NEAR PASCAL SetStatusProc(PINSTINFO,LPICSETSTATUSPROC,DWORD);
DWORD NEAR PASCAL CompressFramesInfo(PINSTINFO,LPICCOMPRESSFRAMES,DWORD);
DWORD NEAR PASCAL CompressFrames(PINSTINFO,LPICCOMPRESSFRAMES,DWORD);
DWORD NEAR PASCAL Compress(PINSTINFO,LPICCOMPRESS,DWORD);
DWORD NEAR PASCAL CompressGetSize(
  PINSTINFO,
  LPBITMAPINFOHEADER,
  LPBITMAPINFOHEADER
);
DWORD NEAR PASCAL CompressEnd(PINSTINFO);
LRESULT NEAR PASCAL DecompressBegin(
    PINSTINFO, DWORD,
    LPBITMAPINFOHEADER, LPVOID, int, int, int, int,
    LPBITMAPINFOHEADER, LPVOID, int, int, int, int
);
LRESULT NEAR PASCAL DecompressQuery(
    PINSTINFO, DWORD,
    LPBITMAPINFOHEADER, LPVOID, int, int, int, int,
    LPBITMAPINFOHEADER, LPVOID, int, int, int, int
);
LRESULT NEAR PASCAL DecompressGetFormat(
  PINSTINFO, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER
);
LRESULT NEAR PASCAL DecompressGetPalette(
  PINSTINFO, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER
);
LRESULT NEAR PASCAL DecompressSetPalette(
  PINSTINFO, LPBITMAPINFOHEADER
);
LRESULT NEAR PASCAL Decompress(
    PINSTINFO, DWORD,
    LPBITMAPINFOHEADER, LPVOID, int, int, int, int,
    LPBITMAPINFOHEADER, LPVOID, int, int, int, int
);
DWORD NEAR PASCAL DecompressEnd(PINSTINFO);
BOOL CALLBACK _loadds AboutDlgProc(HWND,UINT,WPARAM,LPARAM);
#if !defined(NOBLACKWHITE)
BOOL CALLBACK _loadds ConfigureDlgProc(HWND, UINT, WPARAM, LPARAM);
#endif

// from DRVPROC.C ...
extern HMODULE ghModule;

// from BIGCOPY.ASM
#ifdef WIN32
#  ifndef BigCopy
#    define BigCopy(s,d,l)	MoveMemory((d),(s),(l))
#  endif
#else
void BigCopy(void far * pS, void far * pD, unsigned long sz);
#endif

void ButtHeadCopyBits(
#if	!defined(WIN32)
    unsigned long,
    unsigned short,
#else
    unsigned char *,
#endif
    long,
#if	!defined(WIN32)
    unsigned long,
    unsigned short,
#else
    unsigned char *,
#endif
    long,
    unsigned short,
    unsigned short
);
