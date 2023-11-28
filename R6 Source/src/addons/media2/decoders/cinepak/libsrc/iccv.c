/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/iccv.c 3.34 1995/11/06 13:39:06 bog Exp $

 * (C) Copyright 1992-1995 Radius Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of Radius Inc. and is provided pursuant to a Software
 * License Agreement.  This code is the proprietary information of
 * Radius and is confidential in nature.  Its use and dissemination by
 * any party other than Radius is strictly limited by the confidential
 * information provisions of the Agreement referenced above.

 * $Log: iccv.c $
 * Revision 3.34  1995/11/06 13:39:06  bog
 * Check for proper bits/pixel for YUV playback.
 * 
 * Revision 3.33  1995/05/09 09:23:24  bog
 * Move WINVER back into the makefile.  Sigh.
 * Revision 3.32  1995/02/25  15:17:58  bog
 * ICDECOMPRESS_NULLFRAME means the frame is empty and we should not try to
 * decompress it.
 * 
 * Revision 3.31  1995/02/23  10:41:06  bog
 * Preclude non 0 mod 4 CLJR.
 * 
 * Revision 3.30  1995/02/22  14:44:37  bog
 * Uninitialized pEndingCodeBooks if first CVDecompressBegin in context.
 * 
 * Revision 3.29  1995/02/22  12:30:07  bog
 * 1.  Fix bytes per pixel by DibType table in DecompressBegin to include
 *     YUY2.
 * 2.  Fetch ending codebook from DecompressEnd for DecompressBegin.
 * 
 * Revision 3.28  1994/09/22  17:13:56  bog
 * 1.  Remove AllowN stuff.
 * 2.  Change DIBS_Xxx names to 4CCs.  Add UYVY.
 * 3.  Add check of icc->dwFlags for ICCOMPRESS_KEYFRAME.  The old way was
 *     incorrect but required for VidEdit.
 * 
 * Revision 3.27  1994/09/12  16:02:08  bog
 * Sometimes switching from no internal buffer to internal buffer would not
 * rebuild in DecompressBegin.
 * 
 * Revision 3.26  1994/09/08  13:54:03  bog
 * Check for biPlanes == 1 for valid decompressed target.
 * 
 * Revision 3.25  1994/07/29  17:21:51  bog
 * Disallow non 0 mod 4 YUV420Planar.
 * 
 * Revision 3.24  1994/07/29  15:31:27  bog
 * Weitek YUV420 works.
 * 
 * Revision 3.23  1994/07/22  16:26:50  bog
 * Allow YUV DIB types.
 * 
 * Revision 3.22  1994/07/13  16:22:09  bog
 * Look up StringFileInfo by first language/codepage in Translation,
 * not hard coded.
 * 
 * Revision 3.21  1994/07/13  13:22:44  bog
 * 1.  BI_1632 not valid if WIN32.
 * 2.  If frame size changes after a CompressBegin, just rebegin.  Premiere
 *     uses small frames to time compression.
 * 
 * Revision 3.20  1994/05/26  09:39:08  bog
 * We need mmreg.h in WIN16.
 * 
 * Revision 3.19  1994/05/26  09:19:07  bog
 * Remove unneeded reference to mmreg.h.
 * 
 * Revision 3.18  1994/05/25  16:55:21  bog
 * Remove UpsideDownPalette option.
 * 
 * Revision 3.17  1994/05/10  19:47:49  timr
 * Eliminate compiler warnings.
 * 
 * Revision 3.16  1994/05/09  09:26:13  bog
 * Black & White compression works.
 * 
 * Revision 3.15  1994/05/06  13:41:46  bog
 * Play back black and white movies.
 * 
 * Revision 3.14  1994/05/02  15:41:49  bog
 * Looking at wrong number of colors in DECOMPRESS_SET_PALETTE.
 * 
 * Revision 3.13  1994/04/30  10:53:01  unknown
 * Clean up frame-variable YStep for C version.
 * 
 * Revision 3.12  1994/04/29  12:44:09  bog
 * Add SYSTEM.INI switchable ability to show movies on non-Cinepak palette
 * upside down.  For debug.
 * 
 * Revision 3.11  1994/04/25  14:26:54  bog
 * NULL lpbi in DecompressSetPalette means go back to normal palette.
 * 
 * Revision 3.10  1994/04/09  11:56:31  bog
 * We must compute 0 mod 4 things off the source when decompression is doubled.
 * 
 * Revision 3.9  1994/03/29  13:20:11  bog
 * Allow YStep to change between frames of a movie.
 * 
 * Revision 3.8  1994/03/24  16:53:40  timr
 * For DCI, allow the DIB YStep to change without requiring End/Begin.
 * 
 * Revision 3.7  1994/03/04  14:02:08  bog
 * Must check DibType rather than pD->DStuff.DIBType after CVDecompressBegin().
 * 
 * Revision 3.6  1994/03/04  12:23:11  bog
 * Oops on test for CVDecompressSetPalette.
 * 
 * Revision 3.5  1994/03/04  11:56:02  bog
 * Don't invoke CVDecompressSetPalette unless Dither8.
 * 
 * Revision 3.4  1994/03/03  12:00:07  timr
 * Add code to support SET_PALETTE on non-x86 platforms.
 * 
 * Revision 3.3  1994/03/02  16:48:11  timr
 * Incorporate untested Chicago modifications.
 * 
 * Revision 3.2  1994/03/02  16:36:06  timr
 * Add support for ICM_DECOMPRESS_SET_PALETTE.
 * 
 * Revision 3.1  1994/01/25  11:03:00  bog
 * Remove SYSTEM.INI Decompress2X switch; enable always.
 * 
 * Revision 3.0  1993/12/10  14:48:53  timr
 * Minor changes to accomodate NT C-only codec.
 * 
 * Revision 2.31  1993/11/29  16:04:28  geoffs
 * Pixel doubling done for 8bpp decompress
 * 
 * Revision 2.30  1993/11/05  16:32:23  geoffs
 * Added PINSTINFO logging for Open() if DEBUG enabled
 * 
 * Revision 2.29  1993/11/05  11:36:12  bog
 * fDrawInitialized and fDecompressInitialized no longer used.
 * 
 * Revision 2.28  1993/11/04  22:24:43  bog
 * Remove acceleration hook; use VIDS.CVID instead.
 * 
 * Revision 2.27  1993/10/21  09:50:11  geoffs
 * Added CD-ROM padding for compressions
 * 
 * Revision 2.26  1993/10/18  13:50:40  geoffs
 * Spelling error for last revision caused compile hiccup
 * 
 * Revision 2.25  1993/10/18  13:46:52  geoffs
 * Treat BI_1632 and 8bpp as bitfieldless
 * 
 * Revision 2.24  1993/10/14  17:46:52  timr
 * Correct UNICODE boo-boos.  GetProcAddress can't handle it, but 
 * GetPrivateProfileString requires it.
 * 
 * Revision 2.23  1993/10/14  17:43:38  timr
 * RETURN with no parameters changed to RETURNVOID.  Also unwind overly
 * tricky use of comma operator which screwed up RETURN.
 * 
 * Revision 2.22  1993/10/13  10:54:10  bog
 * Fix RETURN macro; slightly broken DecompressedDibType.
 * 
 * Revision 2.21  1993/10/12  17:25:24  bog
 * RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
 * 
 * Revision 2.20  1993/10/12  11:22:52  geoffs
 * Before freeing flatSel, set its selector limit to 0
 * 
 * Revision 2.19  1993/10/05  13:16:09  geoffs
 * Additional changes to handle keyrates of 0 for frame size generation
 * 
 * Revision 2.18  1993/10/04  12:24:57  geoffs
 * Make sure Keyrate of 0 works for internal frame size generation during
 * compression
 * 
 * Revision 2.17  1993/10/01  12:36:45  geoffs
 * Now processing ICM_COMPRESSFRAMESINFO w/internal frame size generation
 * 
 * Revision 2.16  1993/09/23  17:21:52  geoffs
 * Now correctly processing status callback during compression
 * 
 * Revision 2.15  1993/09/09  09:19:40  geoffs
 * Add CompressFrames* stuff (but return ICERR_UNSUPPORTED for now)
 * 
 * Revision 2.14  1993/08/04  12:09:40  timr
 * Both compressor and decompressor now work on NT.
 * 
 * Revision 2.13  1993/07/24  14:34:15  bog
 * Bracket LoadLibrary with SetErrorMode to avoid File Not Found dialog.
 * 
 * Revision 2.12  1993/07/03  11:46:04  geoffs
 * TBYTE changed to TCHAR to make assembler happy
 * 
 * Revision 2.11  93/07/02  16:34:49  geoffs
 * Now compiles,runs under Windows NT
 * 
 * Revision 2.10  93/06/18  15:12:17  geoffs
 * Display file version, not product version, in About dialog box
 * 
 * Revision 2.9  93/06/17  11:38:25  geoffs
 * Fix up dialog box.
 * 
 * Revision 2.8  93/06/16  15:55:37  geoffs
 * Remove unused dialog box definition, minor update to version info stuff
 * 
 * Revision 2.7  93/06/14  11:40:59  bog
 * Get external acceleration working.
 * 
 * Revision 2.6  93/06/13  11:21:22  bog
 * Add hooks for playback acceleration.
 * 
 * Revision 2.5  93/06/09  17:59:04  bog
 * Support decompression to 32 bit DIBs.
 * 
 * Revision 2.4  93/06/09  14:31:00  bog
 * Initialize DStuff.Depth in DecompressBegin.
 * 
 * Revision 2.3  93/06/08  16:49:31  geoffs
 * Removed DRAW stuff, cleanup Decompress rectangles
 * 
 * Revision 2.2  93/06/07  18:43:29  bog
 * Straighten out YStep signs.
 * 
 * Revision 2.1  93/06/03  10:13:22  geoffs
 * First cut at rework for VFW 1.5
 * 
 * Revision 2.0  93/06/01  14:15:13  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.59  93/05/02  15:47:04  bog
 * Add LogRequested flag in system.ini [iccvid.drv] to note requested size and
 * quality.
 * 
 * Revision 1.58  93/04/21  15:49:12  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.57  93/03/12  11:40:10  geoffs
 * Don't compile local variables if making decompress-only driver
 * 
 * Revision 1.56  93/03/10  13:01:41  geoffs
 * Added ifdef's so that compression functions can be made to disappear if
 * decompress only target desired
 * 
 * Revision 1.55  93/02/26  14:51:58  geoffs
 * Changed visible strings containing CompactVideo to new name, Cinepak
 * 
 * Revision 1.54  93/02/25  11:24:09  geoffs
 * Decompress path is default render path, overridden from system.ini
 * 
 * Revision 1.53  93/02/25  08:49:45  geoffs
 * If auto-determination of render path, test correct variable
 * 
 * Revision 1.52  93/02/24  12:49:11  geoffs
 * Fixed up non-0mod4 playback problem when drawing directory to screen
 * 
 * Revision 1.51  93/02/18  15:27:02  geoffs
 * Oops -- processed {T,S}Quality incorrectly from system.ini
 * 
 * Revision 1.50  93/02/18  14:53:57  geoffs
 * Added quality override capability from system.ini
 * 
 * Revision 1.49  93/02/16  10:56:09  bog
 * Remove hinted size smoothing.
 * 
 * Revision 1.48  93/02/05  13:30:19  geoffs
 * Added hinted size smoothing during compression
 * 
 * Revision 1.47  93/02/03  09:49:58  bog
 * Make NEIGHBORS a SYSTEM.INI parameter.
 * 
 * Revision 1.46  93/02/02  10:36:52  geoffs
 * Added auto-determination of best path to choose for decompress
 * 
 * Revision 1.45  93/02/01  12:23:16  geoffs
 * Removed profiling, added system.ini flag for intercode speedups
 * 
 * Revision 1.44  93/01/28  10:17:17  geoffs
 * Fixed up nesting problem for debug printout
 * 
 * Revision 1.43  93/01/28  07:46:08  geoffs
 * Open function needs to treat the flags as containing bit flags and not values
 * 
 * Revision 1.42  93/01/27  17:19:22  geoffs
 * Debug nesting count was screwed up in Free
 * 
 * Revision 1.41  93/01/25  14:25:43  geoffs
 * Allow non 0 mod 4 frames.
 * 
 * Revision 1.40  93/01/21  15:49:22  geoffs
 * Added DEBUG code to beep when decompress called and compress is being done
 * 
 * Revision 1.39  93/01/21  10:37:07  geoffs
 * CVDecompress{Begin,End} now have same brackets as allocation of DIB for
 * bits buffer for decompression.
 * 
 * Revision 1.38  93/01/19  08:31:37  geoffs
 * Corrected the debug nesting count in CompressEnd, added overhead fudge
 * to CompressGetSize
 * 
 * Revision 1.37  93/01/16  16:11:14  geoffs
 * Allocated a flat selector in static data segment at Load time.
 * 
 * Revision 1.36  93/01/11  17:38:05  geoffs
 * Accommodate 2 quality parameters, CVCompressEnd now returns a void
 * 
 * Revision 1.35  93/01/10  15:36:20  geoffs
 * Added -> 8 bpp lookup table for CVCompressBegin
 * 
 * Revision 1.34  93/01/06  11:51:30  geoffs
 * Rationalized i/f's for CVCompress... and CVDecompress...
 * 
 * Revision 1.33  92/12/24  10:29:24  geoffs
 * Keyframe indication comes into Compress in the lpdwFlags, not the dwFlags
 * 
 * Revision 1.32  92/12/23  14:59:07  geoffs
 * Even when debug was turned off in system.ini we were outputting a
 * line of asterisks to the output logfile.
 * 
 * Revision 1.31  92/12/23  14:50:34  geoffs
 * Reconciled references to elements of the INSTINFO with the new
 * partitioning of elements in that struct. Added code in the compress
 * functions to handle calls to the lower level for actual compression.
 * These calls are #if'ed out right now because no lower level routines
 * exist yet.
 * 
 * Revision 1.30  92/12/22  14:22:26  geoffs
 * Added GetDefaultKeyframeRate function
 * 
 * Revision 1.29  92/12/22  14:14:03  geoffs
 * Added ...Quality functions
 * 
 * Revision 1.28  92/12/22  13:06:36  geoffs
 * New layering between API and CV implemented in Draw path
 * 
 * Revision 1.27  92/12/21  18:43:40  timr
 * Decompressor now works split.
 * 
 * Revision 1.26  92/12/21  11:37:47  bog
 * Split into IC and CV layers.
 * 
 * Revision 1.25  92/12/17  09:25:11  geoffs
 * Added lots of new stuff for improved debugging output. Now have ability
 * to define debugging output on/off and destination via keyword in the
 * system.ini file (in the [iccvid.drv] section).
 *
 * Revision 1.24  92/12/11  09:37:53  geoffs
 * When will it ever end! Yet more accommodations to MS kluges in the DRAW
 * path. It was working in Media Player but then it wouldn't work in VidEdit
 * because of palette problems. Now it works in both but only under funny
 * contrived conditions in the driver. It's still not quite correct.
 *
 * Revision 1.23  92/12/10  10:00:22  geoffs
 * Lots of fiddles having to do with DRAW palette realization, adjustment
 * to Microsoft codec kluges (ie: DrawEnd should not free up the internal
 * bits buffer), addition of lots of debug output.
 *
 * Revision 1.22  92/12/09  11:51:25  geoffs
 * Many changes:
 * 	1. Added logic to print debug log of functions called to a file
 * 	   named ICCVID.LOG in the Windows subdirectory. If compiled with
 * 	   DEBUG and the section [iccvid.drv] in system.ini has a keyword
 * 	   named Debug set to other than 0 then the log is produced.
 * 	2. Draw functionality has been simplified for palette handling.
 * 	3. Draw functionality now partially handles the stretch from source
 * 	   to output (ie: the stretch is noted but the contents of the output
 * 	   window is the src rect unstretched in a portion of the output rect).
 * 	   Need to have new bottom level draw routines which run dda's to
 * 	   stretch/reduce the contents.
 *
 * Revision 1.21  92/12/08  14:53:57  geoffs
 * Use logical palette handle passed in at DrawBegin time instead of making
 * our own logical palette
 *
 * Revision 1.20  92/12/08  14:36:39  geoffs
 * Added DRAW message functionality into codec
 *
 * Revision 1.19  92/12/02  10:52:28  geoffs
 * In CompressGetFormat, now return that bitcount == 24
 *
 * Revision 1.18  92/12/02  10:51:01  geoffs
 * Fixed up wild ptr upon successful Open
 *
 * Revision 1.17  92/12/01  12:11:00  geoffs
 * Don't allow Open if not being opened as a Video Compressor
 *
 * Revision 1.16  92/12/01  11:07:20  bog
 * 8 bpp dither path working; colors are a little strong.
 *
 * Revision 1.15  92/11/29  15:53:20  bog
 * Cleaned up 24bpp path.  Now it's faster.
 *
 * Revision 1.14  92/11/25  14:13:20  geoffs
 * Use SDK/INCLUDE version of compddk.h
 *
 * Revision 1.13  92/11/13  14:36:41  geoffs
 * Turn off VIDF_TEMPORAL flag
 *
 * Revision 1.12  92/11/11  13:00:17  geoffs
 * We now use the version resource to display info in dialog boxes
 *
 * Revision 1.11  92/11/07  17:00:53  bog
 * Problems with 2nd tile; USE32 works.
 *
 * Revision 1.10  92/11/05  16:57:08  geoffs
 * Clean up to windowsx.h; USE32 for Draw and Expand works.
 *
 * Revision 1.9  92/11/02  01:26:22  bog
 * CompactVideo healthy except that colors are wacked out.
 *
 * Revision 1.8  92/11/01  21:00:12  bog
 * DrawKey24 compiles.
 *
 * Revision 1.7  92/11/01  14:45:02  bog
 * First successful compile with Expand24 done.
 *
 * Revision 1.6  92/10/31  23:11:58  bog
 * Seems pretty healthy with empty asm bottom.
 *
 * Revision 1.5  92/10/31  16:11:41  geoffs
 * Undef NULL before include of windows.h
 *
 * Revision 1.4  92/10/31  15:41:09  bog
 * First successful compile.  Stubbed out bottom.
 *
 * Revision 1.3  92/10/31  12:08:30  bog
 * About to try explosive first compile of top level.
 *
 * Revision 1.2  92/10/28  13:39:49  geoffs
 * Change short description string, szName
 *
 * Revision 1.1  92/10/28  13:26:59  geoffs
 * Initial revision
 *
 *
 * CompactVideo Codec top level
 */

#define	_WINDOWS
#include <stdio.h>
#include <memory.h>		// included for _fmemcpy
#include <stdlib.h>
#include <time.h>
#ifndef __BEOS__
#ifdef	NULL
#undef	NULL
#endif
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#if	!defined(WIN32)
#include <mmreg.h>		// for BI_BITFIELDS
#endif
#include <commdlg.h>
#if	!defined(WIN32)
#include <ver.h>
#include <avifmt.h>
#endif

#include "compddk.h"
#include "config.h"
#include "iccv.h"
#include "cvcodec.h"

#ifdef	ABS
#undef	ABS
#endif
#define	ABS(a)		(((a) < 0) ? -(a) : (a))


#if	!defined(WIN32)
WORD flatSel = 0;			// flat-mapped selector to all memory
#endif
WORD TQuality;				// current temporal quality
WORD SQuality;				// current spatial quality

#ifdef	DEBUG
WORD wDebug = 0xffff;
UINT wSeq = 0;
UINT wDebugLevel = 0;
HFILE hfDebug = 0;
int iNest = -1;
TCHAR szDebug[1024];
#ifdef	SAFELOG
TCHAR szDebugName[1024];
#else
#define	szDebugName	szDebug
#endif
#endif

// system.ini option data
TCHAR szIniFile[] = TEXT("system.ini");
TCHAR szIniSection[] = MODNAME TEXT(".drv");
COMPRESSFLAGS CompressFlags;
TCHAR szIniFraction[] = TEXT("FractionInterCodeBook");
TCHAR szIniTQuality[] = TEXT("TemporalQuality");
TCHAR szIniSQuality[] = TEXT("SpatialQuality");
TCHAR szIniLogRequested[] = TEXT("LogRequested");

/*
  compression keyframe to interframe size ratio
 */
unsigned short KeyToInterRatioUpper;
unsigned short KeyToInterRatioLower;



/*****************************************************************************
 *
 * GetDefaultKeyframeRate() implements the ICM_GETDEFAULTKEYFRAMERATE message.
 *
 ****************************************************************************/
DWORD NEAR PASCAL GetDefaultKeyframeRate(LPDWORD lpdwKeyframeRate)
{
  *lpdwKeyframeRate = DFLT_KEYFRAMERATE;

  DPF2((2,FUNCNAME,(LPTSTR) TEXT("GetDefaultKeyframeRate"),DWKEYFRAME,*lpdwKeyframeRate));
  RETURN (ICERR_OK);
}

/*****************************************************************************
 *
 * GetQuality() implements the ICM_GETQUALITY message.
 *
 ****************************************************************************/
DWORD NEAR PASCAL GetQuality(PINSTINFO pI, LPDWORD lpdwQuality)
{
  if (pI) {
    *lpdwQuality = pI->CStuff.dwRequestedQuality;
    DPF2((3,FUNCNAME,(LPTSTR) TEXT("GetQuality"),INSTINFOPTR,pI,DWQUALITY,*lpdwQuality));
    RETURN (ICERR_OK);
  }

  DPF2((2,FUNCNAME,(LPSTR) TEXT("GetQuality"),INSTINFOPTR,pI));
  RETURN ((DWORD) ICERR_BADPARAM);
}

/*****************************************************************************
 *
 * GetDefaultQuality() implements the ICM_GETDEFAULTQUALITY message.
 *
 ****************************************************************************/
DWORD NEAR PASCAL GetDefaultQuality(LPDWORD lpdwQuality)
{
  *lpdwQuality = DFLT_QUALITY;

  DPF2((2,FUNCNAME,(LPTSTR) TEXT("GetDefaultQuality"),DWQUALITY,*lpdwQuality));
  RETURN (ICERR_OK);
}

/*****************************************************************************
 *
 * SetQuality() implements the ICM_SETQUALITY message.
 *
 ****************************************************************************/
DWORD NEAR PASCAL SetQuality(PINSTINFO pI, DWORD dwQuality)
{
  DPF2((3,FUNCNAME,(LPTSTR) TEXT("SetQuality"),INSTINFOPTR,pI,DWQUALITY,dwQuality));

  if (pI) {
    if (dwQuality == ICQUALITY_DEFAULT) dwQuality = DFLT_QUALITY;
    pI->CStuff.dwRequestedQuality = dwQuality;
    pI->CStuff.dwRealizedQuality = CV_QUALITY(dwQuality);
    RETURN (ICERR_OK);
  }

  RETURN ((DWORD) ICERR_BADPARAM);
}

#pragma mark -

/***************************************************************************
 *
 * ValidCompressedFormat(dwCompression, wBitCount)
 *
 * Returns
 *   0 if unrecognizable
 *   1 if Cinepak color movie format
 *   2 if Cinepak black & white movie format
 *
 ***************************************************************************/

int NEAR PASCAL ValidCompressedFormat(
  DWORD dwCompression,
  UINT wBitCount
) {
  BOOL ret;

  DPF2((3,FUNCNAME,(LPTSTR) TEXT("ValidCompressedFormat"),FCC,dwCompression,BITCOUNT,wBitCount));

  ret = (dwCompression != BI_CV) ? 0 : (
    ((wBitCount & 0x1f) == 8) ?
#if !defined(NOBLACKWHITE)
      2 :			// we do b&w movies
#else
      0 :			// black & white not recognized
#endif
      1
  );
  RETURN (ret);
}

/***************************************************************************
***************************************************************************/

typedef struct {
    DWORD rMask;
    DWORD gMask;
    DWORD bMask;
} BITFIELDMASKS, *PBITFIELDMASKS, FAR *LPBITFIELDMASKS;

DIBTYPE NEAR PASCAL DecompressedDibType(LPBITMAPINFOHEADER lpbi)
{
	DPF2((2,FUNCNAME,(LPTSTR) TEXT("DecompressedDibType"),BMIHPTR,lpbi));
	
	if (lpbi->biPlanes == 1) {
		
		switch (lpbi->biCompression) {
		
			case BI_RGB: {
				switch (lpbi->biBitCount) {
					case 8: {
						RETURN (Dither8);
						break;
					}
					case 15: {
						RETURN (RGB555);
						break;
					}
					case 16: {
						RETURN (RGB565);
						break;
					}
					case 24: {
						RETURN (RGB888);
						break;
					}
					case 32: {
						RETURN (RGBa8888);
						break;
					}
				}
				break;
			}
#if	!defined(WIN32)
			case BI_1632:		// BI-1632 like BI_BITFIELDS
#endif
			case BI_BITFIELDS: {	// 16bpp, RGB fields specified by masks...
			
				switch (lpbi->biBitCount) {
				
					case 8: {
						if (lpbi->biCompression == BI_1632) {
							RETURN (Dither8);
						}
						break;
					}
	
					case 16: {
						if (((LPBITFIELDMASKS) (lpbi + 1))->bMask == 0x0000001fL) {
							
							if (		// could be RGB565...
								(((LPBITFIELDMASKS) (lpbi + 1))->gMask == 0x000007e0L) &&
								(((LPBITFIELDMASKS) (lpbi + 1))->rMask == 0x0000f800L)) {
								RETURN (RGB565);
							}
							else if (		// could be RGB555...
								(((LPBITFIELDMASKS) (lpbi + 1))->gMask == 0x000003e0L) &&
								(((LPBITFIELDMASKS) (lpbi + 1))->rMask == 0x00007c00L)) {
								RETURN (RGB555);
							}
						}
						break;		// not a 16 bit bitfield we recognize...
					}
					case 24: {
						if (
							(((LPBITFIELDMASKS) (lpbi + 1))->bMask == 0x000000ffL) &&
							(((LPBITFIELDMASKS) (lpbi + 1))->gMask == 0x0000ff00L) &&
							(((LPBITFIELDMASKS) (lpbi + 1))->rMask == 0x00ff0000L)) {
							RETURN (RGB888);
						}
						break;
					}
					case 32: {
						if (
							(((LPBITFIELDMASKS) (lpbi + 1))->bMask == 0x000000ffL) &&
							(((LPBITFIELDMASKS) (lpbi + 1))->gMask == 0x0000ff00L) &&
							(((LPBITFIELDMASKS) (lpbi + 1))->rMask == 0x00ff0000L)
							) {
							RETURN (RGBa8888);
						}
						break;
					}
				}
	
				break;
			}
#if	(DIBS & DIBS_CPLA)
			case BI_CPLA: {
				RETURN((lpbi->biBitCount == 12) ? CPLA : NoGood);
			}
#endif
#if	(DIBS & DIBS_YUY2)
			case BI_YUY2: {
				RETURN((lpbi->biBitCount == 16) ? YUY2 : NoGood);
			}
#endif
#if	(DIBS & DIBS_UYVY)
			case BI_UYVY: {
				RETURN((lpbi->biBitCount == 16) ? UYVY : NoGood);
			}
#endif
#if	(DIBS & DIBS_CLJR)
			case BI_CLJR: {
				RETURN((lpbi->biBitCount == 8) ? CLJR : NoGood);
			}
#endif
		}
	}
	RETURN (NoGood);
}

#pragma mark -
/*****************************************************************************
 *
 * CompressQuery() handles the ICM_COMPRESSQUERY message
 *
 * This message basically asks, "Can you compress this into this?"
 *
 * We look at the input and output bitmap info headers and determine
 * if we can.
 *
 ****************************************************************************/
DWORD NEAR PASCAL CompressQuery(
				PINSTINFO pI,
				LPBITMAPINFOHEADER lpbiIn,
				LPBITMAPINFOHEADER lpbiOut
			       )
{
	DPF2((4,FUNCNAME,(LPTSTR) TEXT("CompressQuery"),INSTINFOPTR,pI,BMIHPTR,lpbiIn,BMIHPTR,lpbiOut));
	
#ifndef NOCOMPRESSION
	// determine if the input DIB data is in a format we like.
	if (!lpbiIn ||
		!((1 << DecompressedDibType(lpbiIn)) & CompressibleDIB))
	{
	printf("format error %08x, %08x\n",!lpbiIn,!((1 << DecompressedDibType(lpbiIn)) & CompressibleDIB));
		RETURN ((DWORD) ICERR_BADFORMAT);
	}
	
	//  are we being asked to query the output format also?
	if (lpbiOut &&			// querying the output format too
		(				// our compressed format?
		!ValidCompressedFormat(lpbiOut->biCompression,lpbiOut->biBitCount) ||
		(lpbiOut->biWidth != lpbiIn->biWidth) ||	// no stretch
		(lpbiOut->biHeight != lpbiIn->biHeight) ))
	{
	printf("output format error\n");
		RETURN ((DWORD) ICERR_BADFORMAT);
	}
	
	RETURN (ICERR_OK);
#else
	RETURN ((DWORD) ICERR_UNSUPPORTED);
#endif
}

/*****************************************************************************
 *
 * CompressBegin() implements ICM_COMPRESSBEGIN
 *
 * We're about to start compressing, initialize coprocessor, etc.
 *
 ****************************************************************************/
DWORD NEAR PASCAL CompressBegin(
				PINSTINFO pI,
				LPBITMAPINFOHEADER lpbiIn,
				LPBITMAPINFOHEADER lpbiOut
			       )
{
#ifndef	NOCOMPRESSION
	DWORD dwReturn;
	auto COMPRESSFLAGS LocalCompressFlags;// to include stuff from this movie
#endif

	DPF2((4,FUNCNAME,(LPTSTR) TEXT("CompressBegin"),INSTINFOPTR,pI,BMIHPTR,lpbiIn,BMIHPTR,lpbiOut));

#ifndef	NOCOMPRESSION
	// make sure that the input BITMAPINFOHEADER describes a bitmap
	// format that we understand
	if ((dwReturn = CompressQuery(pI,lpbiIn,lpbiOut)) != ICERR_OK)
	{
	printf("was errr\n");
		RETURN (dwReturn);
	}
	
	// if we've already been initialized then uninitialize and reinitialize
	// for the new format...
	if (pI->fCompressInitialized) CompressEnd(pI);
	
	// set some stuff in the context we'll need later on
	pI->CStuff.YStep = WIDTHBYTES((short)lpbiIn->biWidth * (short)(((lpbiIn->biBitCount+7)>>3)<<3));
	pI->CStuff.lPrevFrameNum = -2;	// no previous frame to start
	
	LocalCompressFlags = CompressFlags;
	LocalCompressFlags.BlackAndWhite = !!pI->fBlackAndWhite;
	
	// initialize the CV layer
	if (
		!(pI->CStuff.CContext = CVCompressBegin(
		(short) lpbiIn->biWidth,
		(short) lpbiIn->biHeight,
#ifdef __BEOS__
		(long) pI->CStuff.YStep,
#else
		- (long) pI->CStuff.YStep,
#endif
		DecompressedDibType(lpbiIn),
		&lpbiIn->biClrUsed,
		&pI->CStuff.ci,
		&pI->CStuff.icssp,
		LocalCompressFlags)))
	{
		RETURN ((DWORD) ICERR_MEMORY);
	}
	
	pI->CStuff.biWidth = lpbiIn->biWidth;// remember for size change
	pI->CStuff.biHeight = lpbiIn->biHeight;
	
	pI->fCompressInitialized = TRUE;
	
	RETURN (ICERR_OK);
#else
	RETURN ((DWORD) ICERR_UNSUPPORTED);
#endif
}

/*****************************************************************************
 *
 * Compress() implements ICM_COMPRESS
 *
 * Everything is set up; call the actual compression routine.
 *
 * Note:
 *
 * 1) We set the ckid in icc to a two-character code indicating how we
 *    compressed. This code will be returned to us at decompress time to
 *    allow us to pick a decompression algorithm to match. This is different
 *    from icc->fccHandler, which tells which driver to use!
 *
 ****************************************************************************/
DWORD NEAR PASCAL Compress(
	PINSTINFO pI,
	ICCOMPRESS FAR *icc,
	DWORD dwSize)
{
#ifndef	NOCOMPRESSION
	DWORD dwReturn;
	long lSize;
#if	!defined(WIN32)
	auto unsigned short sBits;	// selector to input DIB
#endif
	auto unsigned long oBits;	// offset into input DIB
#endif
	
	DPF2((4,FUNCNAME,(LPTSTR) TEXT("Compress"),INSTINFOPTR,pI,ICCOMPRESSPTR,icc,DWSIZE,dwSize));
	
#ifndef	NOCOMPRESSION
	/*
	  If we were called without a BEGIN, fake one.  It should never
	  happen.
	
	  If the size of the movie changed, force a BEGIN, too.  (The
	  CompressBegin does a CompressEnd).  Again, it should never happen.
	  But Premiere 1.1 does a quickie compression of an 8 scanline swath
	  to measure the likely time to compress the whole frame.  The easiest
	  thing to do is simply handle it.
	 */
	if (
		!pI->fCompressInitialized ||// if never called
		(icc->lpbiInput->biWidth != pI->CStuff.biWidth) ||// or size changed
		(icc->lpbiInput->biHeight != pI->CStuff.biHeight))
	{
		if ((dwReturn = CompressBegin(pI, icc->lpbiInput, icc->lpbiOutput))!=ICERR_OK)
		{
			RETURN (dwReturn);
		}
	}

	// make sure that the input BITMAPINFOHEADER describes a bitmap
	// format that we understand
	if ((dwReturn = CompressQuery(pI,icc->lpbiInput,icc->lpbiOutput))!=ICERR_OK)
	{
		RETURN (dwReturn);
	}

	//
	// actual compression here
	//
	
	// determine whether or not this frame s/b a keyframe
#if 0 /* XXXdbg */
	pI->CStuff.KeyFrame = (
		(icc->dwFlags & ICCOMPRESS_KEYFRAME) ||
		/*
		icc->lpdwFlags is supposed to be an output data area.  But
		VidEdit erroneously uses it to tell us whether a frame should be
		key or not.  So we have an extra, awkward way to note keyness.
		*/
		(icc->lpdwFlags && (*icc->lpdwFlags == AVIIF_KEYFRAME)) ||
		(pI->CStuff.lPrevFrameNum != (icc->lFrameNum - 1)));
#else
	pI->CStuff.KeyFrame = (icc->dwFlags & ICCOMPRESS_KEYFRAME);
#endif							  
	
	// calculate the -> the 0th scan of the incoming DIB
#if	!defined(WIN32)
	sBits = (unsigned short) (((unsigned long) icc->lpInput) >> 16);
	oBits = (((unsigned long) icc->lpInput) & 0x0000ffffL) +
		((pI->CStuff.biHeight - 1) * pI->CStuff.YStep);
#else
#ifdef __BEOS__		// this is pretty strange... (see the #ifdef __BEOS__ in CompressBegin
	oBits = (unsigned long) icc->lpInput;
#else
	oBits = (unsigned long) icc->lpInput +
		((pI->CStuff.biHeight - 1) * pI->CStuff.YStep);
#endif
#endif
	
	if (
		(lSize = CVCompress(	// returns size else neg if error
			pI->CStuff.CContext,	// compression context
			oBits,			// 32-bit offset to scan 0
#if !defined(WIN32)
			sBits,			// selector to input DIB
#endif
			icc->lpOutput,		// destination compressed frame
			&pI->CStuff.KeyFrame,	// in/out keyframe flag
			icc->dwFrameSize,	// frame size; 0 if don't care
			(unsigned short) (
				((unsigned short)SQuality != 0xffff)	// quality in .ini?
					? SQuality		// yes, use .ini value
					: 	// no -- use MS value
						((icc->dwQuality == ICQUALITY_DEFAULT)
							? CV_QUALITY(DFLT_QUALITY)
							: CV_QUALITY(icc->dwQuality))),
			(unsigned short) (
				((unsigned short)TQuality != 0xffff)	// quality in .ini?
					? TQuality		// yes, use .ini value
					:	// no -- use MS value
						((icc->dwQuality == ICQUALITY_DEFAULT)
							? CV_QUALITY(DFLT_QUALITY)
							: CV_QUALITY(icc->dwQuality))))) < 0)
	{				// some kind of error during compression...
		pI->CStuff.lPrevFrameNum = -2;	// next frame s/b keyframe
		RETURN ((DWORD)((lSize == CVCOMPRESS_ERR_ABORT) ? ICERR_ABORT : ICERR_ERROR));
	}
	pI->CStuff.lPrevFrameNum = icc->lFrameNum;// remember frame #
	
	// return the actual size of the compressed data
	icc->lpbiOutput->biSizeImage = (DWORD) lSize;
	
	// return the chunk id
	if (icc->lpckid) *icc->lpckid = TWOCC_CV;
	
	// set the AVI index flags
	if (icc->lpdwFlags)
	{
		*icc->lpdwFlags =
			AVIIF_TWOCC | ((pI->CStuff.KeyFrame) ? AVIIF_KEYFRAME : 0);
	}
	return lSize;
	RETURN (ICERR_OK);
#else
	RETURN ((DWORD) ICERR_UNSUPPORTED);
#endif
}

/*****************************************************************************
 *
 * CompressEnd() is called on ICM_COMPRESS_END
 *
 * This function is a chance to flush buffers, deinit hardware, etc.
 * after compressing a single frame.
 *
 ****************************************************************************/
DWORD NEAR PASCAL CompressEnd(PINSTINFO pI)
{
	DPF2((2,FUNCNAME,(LPTSTR) TEXT("CompressEnd"),INSTINFOPTR,pI));

#ifndef	NOCOMPRESSION
	// was there a corresponding CompressBegin bracket?
	if (!pI->fCompressInitialized) {
		RETURN ((DWORD) ICERR_ERROR);
	}
	
	CVCompressEnd(pI->CStuff.CContext);
	
	pI->CStuff.icssp.Status = NULL;
	pI->CStuff.ci.dwRate =
	pI->CStuff.ci.dwScale = 0;
	
	pI->fCompressInitialized = FALSE;
	
	RETURN (ICERR_OK);
#else
	RETURN ((DWORD) ICERR_UNSUPPORTED);
#endif
}

#pragma mark -

/*****************************************************************************
 *
 * DecompressQueryIndirect() implements internal analog to
 *	ICM_DECOMPRESS_QUERY but returns src,dest parameters adjusted
 * 
 ****************************************************************************/
LRESULT NEAR PASCAL DecompressQueryIndirect(
    PINSTINFO pI,
    DWORD dwFlags,
    LPBITMAPINFOHEADER lpbiSrc,
    LPVOID lpSrc,
    int xSrc,
    int ySrc,
    int far *lpdxSrc,
    int far *lpdySrc,
    LPBITMAPINFOHEADER lpbiDst,
    LPVOID lpDst,
    int xDst,
    int yDst,
    int far *lpdxDst,
    int far *lpdyDst
)
{
  DPF2((7,FUNCNAME,(LPTSTR) TEXT("DecompressQueryIndirect"),INSTINFOPTR,pI,DWVALUE,dwFlags,BMIHPTR,lpbiSrc,SRCDESTPARAMS,lpSrc,xSrc,ySrc,*lpdxSrc,*lpdySrc,BMIHPTR,lpbiDst,SRCDESTPARAMS,lpDst,xDst,yDst,*lpdxDst,*lpdyDst));

	//
	// determine if the input compressed DIB data is in a format we like.
	//
	if (!lpbiSrc || !ValidCompressedFormat(lpbiSrc->biCompression,lpbiSrc->biBitCount))
	{
		RETURN (ICERR_BADFORMAT);
	}

    //
    // allow (-1) as a default width/height
    //
    if (*lpdxSrc == -1) *lpdxSrc = (int) lpbiSrc->biWidth;
    if (*lpdySrc == -1) *lpdySrc = (int) lpbiSrc->biHeight;

	//
	//  we can't clip the source.
	//
	if (xSrc || ySrc) return (ICERR_BADPARAM);
	if ((*lpdxSrc != (int) lpbiSrc->biWidth) ||
		(*lpdySrc != (int) lpbiSrc->biHeight))
	{
		RETURN (ICERR_BADPARAM);
	}

	//
	// are we being asked to query just the input format?
	//
	if (lpbiDst) { // no -- check the output format too...
		DIBTYPE dibType;
		
		//
		// allow (-1) as a default width/height
		//
		if (*lpdxDst == -1) *lpdxDst = (int) lpbiDst->biWidth;
		if (*lpdyDst == -1) *lpdyDst = abs((int) lpbiDst->biHeight);

		//
		// make sure we can handle the format to decompress to.
		//
		if (((dibType = DecompressedDibType(lpbiDst)) == NoGood) ||
			!(((*lpdxDst == *lpdxSrc) && (*lpdyDst == *lpdySrc)) ||
			  ((dibType == Dither8) &&		// 8bpp
			   (*lpdxDst == (*lpdxSrc << 1)) &&	// 2:1 stretch only
			   (*lpdyDst == (*lpdySrc << 1)))) ||
			 (((dibType == CPLA) || (dibType == CLJR)) && (3 & (*lpdxDst | *lpdyDst))) || 
			 (((dibType == YUY2) || (dibType == UYVY)) && (1 & *lpdxDst))
		) {
		RETURN (ICERR_BADFORMAT);
		}
    }
    RETURN (ICERR_OK);
}

/*****************************************************************************
 *
 * DecompressBegin() implements ICM_DECOMPRESS_BEGIN
 *
 * See CompressBegin()
 *
 ****************************************************************************/
LRESULT NEAR PASCAL DecompressBegin(
  PINSTINFO pI,
  DWORD dwFlags,
  LPBITMAPINFOHEADER lpbiSrc,
  LPVOID lpSrc,
  int xSrc,
  int ySrc,
  int dxSrc,
  int dySrc,
  LPBITMAPINFOHEADER lpbiDst,
  LPVOID lpDst,
  int xDst,
  int yDst,
  int dxDst,
  int dyDst
) {
	auto LRESULT lReturn;
	
	auto DIBTYPE DibType;		// to compare
	auto short DepthInBytes;
	
	auto int Scale;
	
	static DepthsInBytes[] = {
		1,				// 8 bpp dithered
		2,				// 16 bpp RGB 555
		2,				// 16 bpp RGB 565
		3,				// 24 bpp RGB 888
		4,				// 32 bpp RGBa 8888
		1,				// CPLA Wietek YUV420 planar (really 12 bits)
		2,				// YUY2 various YUV422
		2,				// UYVY Cirrus YUV422
		1				// CLJR Cirrus packed YUV411
	};
	
	DPF2((7,FUNCNAME,(LPTSTR) TEXT("DecompressBegin"),INSTINFOPTR,pI,DWVALUE,dwFlags,BMIHPTR,lpbiSrc,SRCDESTPARAMS,lpSrc,xSrc,ySrc,dxSrc,dySrc,BMIHPTR,lpbiDst,SRCDESTPARAMS,lpDst,xDst,yDst,dxDst,dyDst));
	
	// make sure that the input,output BITMAPINFOHEADERs describe bitmap
	// formats that we can process...
	if ((lReturn = DecompressQueryIndirect(pI,dwFlags,
		lpbiSrc,lpSrc,xSrc,ySrc,&dxSrc,&dySrc,
		lpbiDst,lpDst,xDst,yDst,&dxDst,&dyDst)) != ICERR_OK)
	{
		RETURN (lReturn);
	}
	
	DibType = DecompressedDibType(lpbiDst);
	DepthInBytes = DepthsInBytes[DibType];
	
	Scale = dxDst / dxSrc;

	//
	// we decompress to an internal buffer if the movie is not 0 mod 4 in
	// both x and y
	//
	if ((dxSrc | dySrc) & 3)
	{
		auto short Width0Mod4;
		auto short Height0Mod4;
		
		auto BOOL bForceRealloc;
		
		bForceRealloc = FALSE;
		
		Width0Mod4 = ((dxSrc + 3) & ~3) * Scale;
		Height0Mod4 = ((dySrc + 3) & ~3) * Scale;
		
		/*
		must we re-initialize the lower levels?
		*/
		if (
			!pI->DStuff.DContext ||				// no context yet
			(pI->DStuff.Width0Mod4 != Width0Mod4) ||		// widths don't match
			(pI->DStuff.Height0Mod4 != Height0Mod4) ||	// heights don't match
			(pI->DStuff.DIBType != DibType) ||		// depths no match
			!(pI->DStuff.Flags & DECOMPRESS_USE_BUFFER)	// last didn't use buff
			)
		{
			auto void far *pEndingCodeBooks;// for holding the codebook frame
			
			if (pI->DStuff.DContext)
			{
				pEndingCodeBooks = CVDecompressEnd(
					pI->DStuff.DContext,
					TRUE,
					(pI->DStuff.nPalColors > 0) ? pI->DStuff.rgbqUser : 0
					);
				pI->DStuff.DContext = (void *) 0;
			}
			else
			{
				pEndingCodeBooks = 0;
			}
		
			if (			// reinitialize decompress
				!(
				pI->DStuff.DContext = CVDecompressBegin(
					Width0Mod4,
					Height0Mod4,
					DibType,
					(short) Scale,
					pEndingCodeBooks	// to load previous codebook
					)
				))
			{
				RETURN (ICERR_MEMORY);
			}
#ifndef __BEOS__	// no 8-bit dither in BeOS
			if (DibType == Dither8)
			{
				CVDecompressSetPalette (
					pI->DStuff.DContext,
					pI->DStuff.rgbqUser, 
					pI->DStuff.nPalColors 
				);
			}
#endif
	
			// internal 0Mod4 holding buffer always top down DIB
			pI->DStuff.YStep0Mod4 = ((long) Width0Mod4) * ((long) DepthInBytes);
			
			bForceRealloc = TRUE;
		}
			
		// now do allocation of DIB bits buffer...
		if (bForceRealloc ||					// must realloc (above)
			!(pI->DStuff.Flags & DECOMPRESS_USE_BUFFER) ||	// last didn't use buff
			!pI->DStuff.pBits					// no buffer yet
			)
		{
			
			if (pI->DStuff.pBits)
			{	// free up previous resources...
				GlobalFreePtr(pI->DStuff.pBits);
				pI->DStuff.pBits = (char far *) 0;
			}
			if (
				!(
				pI->DStuff.pBits = (char far *) GlobalAllocPtr(
				GMEM_MOVEABLE,
				pI->DStuff.YStep0Mod4 * (long) Height0Mod4
				)))
			{
				CVDecompressEnd(pI->DStuff.DContext, FALSE, 0);// free up any resources
				pI->DStuff.DContext = (void *) 0;
				RETURN (ICERR_MEMORY);
			}
	
			// remember dimensions of internal DIB
			pI->DStuff.Width0Mod4 = Width0Mod4;
			pI->DStuff.Height0Mod4 = Height0Mod4;
			
			/*
			cache 16:32 pointer to ul of internal holding DIB
			*/
#if	!defined(WIN32)
			pI->DStuff.s0Mod4 = SELECTOROF(pI->DStuff.pBits);
			pI->DStuff.o0Mod4 = (unsigned short) (unsigned long) pI->DStuff.pBits;
#else
			pI->DStuff.o0Mod4 = (unsigned long) pI->DStuff.pBits;
#endif
			
			// remember that we are to use the internal decompress buffer
			pI->DStuff.Flags |= DECOMPRESS_USE_BUFFER;
		}
			
	} else { // decompressing directly to destination...
		
		//
		// check for circumstances requiring us to reinitialize a context
		// at the lower levels
		//
		if (
			!pI->DStuff.DContext ||			// no context yet
			(pI->DStuff.WidthImage != dxDst) ||	// widths don't match
			(pI->DStuff.HeightImage != dyDst) ||	// heights don't match
			(pI->DStuff.DIBType != DibType) ||	// depths no match
			(pI->DStuff.Flags & DECOMPRESS_USE_BUFFER)// last used buffer
			)
		{
			auto void far *pEndingCodeBooks;// for holding the codebook frame
			
			if (pI->DStuff.DContext)
			{
				pEndingCodeBooks = CVDecompressEnd(
					pI->DStuff.DContext,TRUE,
					(pI->DStuff.nPalColors > 0) ? pI->DStuff.rgbqUser : 0);
				pI->DStuff.DContext = (void *) 0;
			}
			else
			{
				pEndingCodeBooks = 0;
			}
			
			if ( // reinitialize decompress
				!(
				pI->DStuff.DContext = CVDecompressBegin(
				(short) dxDst,
				(short) dyDst,
				DibType,
				(short) Scale,
				pEndingCodeBooks	// to load previous codebook
				)))
			{
				RETURN (ICERR_MEMORY);
			}
#ifndef __BEOS__	// no 8-bit dither in BeOS
			if (DibType == Dither8) {
				CVDecompressSetPalette (
				pI->DStuff.DContext,
				pI->DStuff.rgbqUser, 
				pI->DStuff.nPalColors 
				);
			}
#endif
		}
		// remember that we aren't using the internal decompress buffer
		pI->DStuff.Flags &= ~DECOMPRESS_USE_BUFFER;
	}
	// remember dimensions of image rectangle
	pI->DStuff.WidthImage = dxDst;
	pI->DStuff.HeightImage = dyDst;
	pI->DStuff.Depth = (short) lpbiDst->biBitCount;
	pI->DStuff.DepthInBytes = DepthInBytes;
	pI->DStuff.DIBType = DibType;
	pI->DStuff.biHeight = 0;	// must compute YStep
	
	RETURN (ICERR_OK);
}

/*****************************************************************************
 *
 * Decompress() implements ICM_DECOMPRESS
 *
 * See DecompressBegin()
 *
 ****************************************************************************/
LRESULT NEAR PASCAL Decompress(
  PINSTINFO pI,
  DWORD dwFlags,
  LPBITMAPINFOHEADER lpbiSrc,
  LPVOID lpSrc,
  int xSrc,
  int ySrc,
  int dxSrc,
  int dySrc,
  LPBITMAPINFOHEADER lpbiDst,
  LPVOID lpDst,
  int xDst,
  int yDst,
  int dxDst,
  int dyDst
) {
	auto unsigned long oFinal;	// 16:32 offset to ul pixel of final dest
#if	!defined(WIN32)
	auto unsigned short sFinal;
#endif
	
	DPF2((7,FUNCNAME,(LPTSTR) TEXT("Decompress"),INSTINFOPTR,pI,DWVALUE,dwFlags,BMIHPTR,lpbiSrc,SRCDESTPARAMS,lpSrc,xSrc,ySrc,dxSrc,dySrc,BMIHPTR,lpbiDst,SRCDESTPARAMS,lpDst,xDst,yDst,dxDst,dyDst));
	
	if (!pI->DStuff.DContext)
	{	// force Begin if none so far
		auto DWORD dwReturn;
		if ((dwReturn = DecompressBegin(pI,dwFlags,lpbiSrc,lpSrc,xSrc,ySrc,dxSrc,dySrc,lpbiDst,lpDst,xDst,yDst,dxDst,dyDst))!=ICERR_OK)
		{
			RETURN(dwReturn);
		}
	}

	/*
	We must calculate YStep if we never have before or if the
	destination has changed.
	*/
	if (
		(pI->DStuff.biWidth != lpbiDst->biWidth) ||
		(pI->DStuff.biHeight != (long) lpbiDst->biHeight)
		)
	{
		pI->DStuff.biWidth = lpbiDst->biWidth;
		pI->DStuff.biHeight = (long) lpbiDst->biHeight;
	
	/*
      pI->DStuff.YStepDIB is negative for normal DIBs and positive for
      TOPDOWN DIBs

      Traditional DIB (shown as seen on screen)
			  +-------------------------------+
			  |                               |
			  |                               |
	     increasing ^ |                               |
	       addr     | |  YStep negative               |
			  |                               |
			  |                               |
	    ptr to bits ->|<-(0,0) in DIB coords          |
			  +-------------------------------+

      TOPDOWN DIB
			  +-------------------------------+
	    ptr to bits ->|                               |
			  |                               |
			  |                               |
	     increasing | |  YStep positive               |
	       addr     v |                               |
			  |                               |
			  |                               |
			  |<-(0,0) in DIB coords          |
			  +-------------------------------+
     */
		pI->DStuff.YStepDIB =
#if	(DIBS & DIBS_CPLA)
			(lpbiDst->biCompression == BI_CPLA) ?
			((long) WIDTHBYTES(pI->DStuff.biWidth * 8)) :
#endif
			(long) WIDTHBYTES(pI->DStuff.biWidth * (((lpbiDst->biBitCount+7)>>3)<<3));
		
		if (((long) pI->DStuff.biHeight) >= 0)
		{// if normal DIB
			
			pI->DStuff.YStepDIB = -pI->DStuff.YStepDIB;
			pI->DStuff.o00DIB = 0L;
		} else {			// topdown DIB
			
			pI->DStuff.o00DIB =
			(-1 - ((long) pI->DStuff.biHeight)) * pI->DStuff.YStepDIB;
		}
	}
		
  /*
    compute -> ul pixel in final destination, either the direct target
    of CVDecompress or the source of ButtHeadCopyBits

    base of -> bits passed in to us +
    offset to 0th scan in destination + // (calculated above)
    offset into DIB to starting scan +
    offset into starting scan to starting pixel
   */
   
#if	!defined(WIN32)
	sFinal = SELECTOROF(lpDst);
	oFinal =
		((unsigned long) (unsigned short) (unsigned long) lpDst)
		+ pI->DStuff.o00DIB
		- ((yDst + pI->DStuff.HeightImage - 1) * pI->DStuff.YStepDIB)
		+ (xDst * pI->DStuff.DepthInBytes);
#else
	oFinal =
		((unsigned long) lpDst)
		+ pI->DStuff.o00DIB
		- ((yDst + pI->DStuff.HeightImage - 1) * pI->DStuff.YStepDIB)
		+ (xDst * pI->DStuff.DepthInBytes);
#endif

	if (!(dwFlags & ICDECOMPRESS_NULLFRAME)) {// if not empty
		if (!(pI->DStuff.Flags & DECOMPRESS_USE_BUFFER)) {// if direct...
			if (			// decompress into final destination
				!CVDecompress(
					pI->DStuff.DContext,	// decompression context
					lpSrc,		// frame to be decompressed
					oFinal,		// 32-bit offset to final DIB ul scan
#if !defined(WIN32)
					sFinal,		// selector to final DIB
#endif
					pI->DStuff.YStepDIB	// bytes from (x,y) to (x,y+1)
			)) {			// some error during decompress...
				RETURN ((DWORD) ICERR_ERROR);
			}
		}
		else
		{			// indirect through internal 0 mod 4 DIB
			
			if (			// decompress into internal DIB
				!CVDecompress(
					pI->DStuff.DContext,	// decompression context
					lpSrc,		// frame to be decompressed
					pI->DStuff.o0Mod4,	// 32-bit offset to 0 mod 4 DIB ul scan
#if !defined(WIN32)
					pI->DStuff.s0Mod4,	// selector to 0 mod 4 DIB
#endif
					pI->DStuff.YStep0Mod4	// bytes from (x,y) to (x,y+1)
			)) {			// some error during decompress...
				RETURN ((DWORD) ICERR_ERROR);
			}
			/*
			Now bitblt from internal DIB to final dest, as long as there's
			time
			*/
			if (!(dwFlags & ICDECOMPRESS_HURRYUP)) {
				ButtHeadCopyBits(
#if !defined(WIN32)
					pI->DStuff.o0Mod4,
					pI->DStuff.s0Mod4,
#else
					(unsigned char *) pI->DStuff.o0Mod4,
#endif
					pI->DStuff.YStep0Mod4,
#if !defined(WIN32)
					oFinal,
					sFinal,
#else
					(unsigned char *) oFinal,
#endif
					pI->DStuff.YStepDIB,
					(unsigned short) (pI->DStuff.WidthImage * pI->DStuff.DepthInBytes),
					(unsigned short) pI->DStuff.HeightImage
				);
			}
		}
	}
	RETURN (ICERR_OK);
}

/*****************************************************************************
 *
 * DecompressEnd() implements ICM_DECOMPRESS_END
 *
 * See CompressEnd()
 *
 ****************************************************************************/
DWORD NEAR PASCAL DecompressEnd(PINSTINFO pI)
{
  DPF2((2,FUNCNAME,(LPTSTR) TEXT("DecompressEnd"),INSTINFOPTR,pI));

	if (pI->DStuff.DContext)
		CVDecompressEnd(pI->DStuff.DContext,FALSE,NULL);
  RETURN(ICERR_OK);
}

#pragma mark -
#pragma mark  --------- not used ------------

#ifdef	DEBUG
/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to the debug screen and/or logfile. To
 * enable debug output, add the following to SYSTEM.INI :
 *
 * [MODNAME.drv]
 * Debug=?
 *
 *	where ? is 0:	to turn off debug output
 *		   1:	to enable all levels of debug to both debug screen
 *			& logfile
 *		   2:   to enable top-level debug only to both debug screen
 *			& logfile
 *		   3:   to enable all levels of debug to debug screen only
 *		   4:   to enable top-level debug to debug screen only
 *
 ****************************************************************************/
void FAR cdecl dprintf(LPTSTR szFormat, ...)
{
#if	!defined(WIN32)
//	SOMEDAY we should make this work on NT.
reswitch:

  switch (wDebug) {
    case 0xffff: {			// initialization call
      wDebug = GetPrivateProfileInt(szIniSection,TEXT("Debug"),0,szIniFile);
      if (wDebug && (wDebug < 3)) { // output to logfile also...
	GetWindowsDirectory(szDebugName,NUMCHARS(szDebug));
	lstrcat(szDebugName,TEXT("\\") MODNAME TEXT(".log"));
#if	!defined(WIN32)
	if ((hfDebug = _lopen(szDebugName,READ_WRITE)) != HFILE_ERROR)
#else
	if ((hfDebug = _lopen(szDebugName,OF_READWRITE)) != HFILE_ERROR)
#endif
	{
	  _llseek(hfDebug,0,2);		// seek to end of file
	}
	else {
	  hfDebug = _lcreat(szDebugName,0);	// create the new file
	}
	if (hfDebug) {
	  WORD wCnt;
	  for (wCnt = 80; wCnt--; ) _lwrite(hfDebug,TEXT("*"),1);
	  _lwrite(hfDebug,TEXT("\n"),1);
	}
      }
      goto reswitch;
    }
    case 4:
    case 2: {
      if (iNest) { // only top level calls
        wSeq++;
	break;
      }
    }
    case 3:
    case 1: {
      wsprintf(szDebug,MODNAME TEXT("%04d: ++++++++++"),wSeq++);
      wvsprintf(szDebug+NUMCHARS(MODNAME)+iNest+5,szFormat,(LPTSTR)(&szFormat+1));
      lstrcat(szDebug,TEXT("\r\n"));
      OutputDebugString(szDebug);
      if ((wDebug < 3) && hfDebug) _lwrite(hfDebug,szDebug,lstrlen(szDebug));

#ifdef	SAFELOG
      _lclose(hfDebug);
      hfDebug = _lopen(szDebugName,OF_READWRITE);
      _llseek(hfDebug,0,2);		// seek to end of file
#endif
      break;
    }
    default: {
      break;
    }
  }
#endif
}


/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to the debug screen and/or logfile. To
 * enable debug output, add the following to SYSTEM.INI :
 *
 * [MODNAME.drv]
 * Debug=?
 *
 *	where ? is 0:	to turn off debug output
 *		   1:	to enable all levels of debug to both debug screen
 *			& logfile
 *		   2:   to enable top-level debug only to both debug screen
 *			& logfile
 *		   3:   to enable all levels of debug to debug screen only
 *		   4:   to enable top-level debug to debug screen only
 *
 ****************************************************************************/

typedef char FAR *my_va_list;
#define my_va_start(ap,v) ap = (my_va_list)&v + sizeof(v)
#define my_va_arg(ap,t) ((t FAR *)(ap += sizeof(t)))[-1]
#define my_va_end(ap) ap = NULL

void FAR cdecl dprintf2(WORD wArgPairs, ...)
{
#if	!defined(WIN32)
  my_va_list marker;

reswitch:

  switch (wDebug) {
    case 0xffff: {			// initialization call
      wDebug = GetPrivateProfileInt(szIniSection,TEXT("Debug"),0,szIniFile);
      if (wDebug && (wDebug < 3)) { // output to logfile also...
	GetWindowsDirectory(szDebugName,NUMCHARS(szDebug));
	lstrcat(szDebugName,TEXT("\\") MODNAME TEXT(".log"));
#if	!defined(WIN32)
	if ((hfDebug = _lopen(szDebugName,READ_WRITE)) != HFILE_ERROR)
#else
	if ((hfDebug = _lopen(szDebugName,OF_READWRITE)) != HFILE_ERROR)
#endif
	{
	  _llseek(hfDebug,0,2);		// seek to end of file
	}
	else {
	  hfDebug = _lcreat(szDebugName,0);	// create the new file
	}
	if (hfDebug) {
	  WORD wCnt;
#if	0
	  for (wCnt = 80; wCnt--; ) _lwrite(hfDebug,TEXT("*"),1);
	  _lwrite(hfDebug,TEXT("\n*    "),6);
	  lstrcpy(szDebugName,"\n*    ");
	  _strdate(szDebugName+lstrlen(szDebugName));
	  lstrcat(szDebugName,"  ");
	  _strtime(szDebugName+lstrlen(szDebugName));
	  lstrcat(szDebugName,"\n");
	  _lwrite(hfDebug,szDebugName,lstrlen(szDebugName));
#endif
	  for (wCnt = 80; wCnt--; ) _lwrite(hfDebug,TEXT("*"),1);
	  _lwrite(hfDebug,TEXT("\n"),1);
	}
      }
      goto reswitch;
    }
    case 4:
    case 2: {
      if (iNest) { // only top level calls
        wSeq++;
	break;
      }
    }
    case 3:
    case 1: {
      UINT wCnt;
      UINT wLeaderLen;

      wsprintf(szDebug,MODNAME TEXT("%04d: "),wSeq++);
      for (wCnt = 0; wCnt < (UINT) iNest; wCnt++) lstrcat(szDebug,TEXT("+"));
      wLeaderLen = lstrlen(szDebug);

      my_va_start(marker,wArgPairs);
      while (wArgPairs--) {
	switch (my_va_arg(marker,WORD)) {
	  case BITCOUNT: {
	    wsprintf(
	    	     szDebug+wLeaderLen,
		     TEXT("\t\tBITCOUNT: %d\r\n"),
		     my_va_arg(marker,WORD)
		    );
	    break;
	  }
	  case BMIHPTR: {
	    LPBITMAPINFOHEADER lp = my_va_arg(marker,LPBITMAPINFOHEADER);

	    if (lp) {
	      wsprintf(
	    	       szDebug+wLeaderLen,
		       TEXT("\t\tBITMAPINFOHEADER: @0x%lx,(%d,%d,%d,%d,%d,'%4.4ls',%ld,%d)\r\n"),
		       lp,
		       (UINT) lp->biSize,
		       (UINT) lp->biWidth,
		       (UINT) lp->biHeight,
		       (UINT) lp->biPlanes,
		       (UINT) lp->biBitCount,
		       (LPSTR) &lp->biCompression,
		       (DWORD) lp->biSizeImage,
		       (UINT) lp->biClrUsed
		      );
	    }
	    else {
	      lstrcat(szDebug,TEXT("\t\tBITMAPINFOHEADER: NULL\r\n"));
	    }
	    break;
	  }
	  case BOOLEAN: {
	    wsprintf(
	    	     szDebug+wLeaderLen,
		     TEXT("\t\tBOOLEAN=%s\r\n"),
		     (LPTSTR) ((my_va_arg(marker,BOOL)) ? TEXT("TRUE") : TEXT("FALSE"))
		    );
	    break;
	  }
	  case DWSIZE: {
	    wsprintf(
	    	     szDebug+wLeaderLen,
		     TEXT("\t\tdwSize=%ld\r\n"),
		     my_va_arg(marker,DWORD)
		    );
	    break;
	  }
	  case DWVALUE: {
	    wsprintf(
	    	     szDebug+wLeaderLen,
		     TEXT("\t\tdwValue=0x%08lx\r\n"),
		     my_va_arg(marker,DWORD)
		    );
	    break;
	  }
	  case FCC: {
	    wsprintf(
	    	     szDebug+wLeaderLen,
		     TEXT("\t\tFCC: '%4.4ls'\r\n"),
		     (LPSTR) &(my_va_arg(marker,FOURCC))
		    );
	    break;
	  }
	  case FUNCNAME: {
	    wsprintf(szDebug+wLeaderLen,TEXT("%s(\r\n"),my_va_arg(marker,LPTSTR));
	    break;
	  }
	  case HDCPTR: {
	    wsprintf(
	    	     szDebug+wLeaderLen,
		     TEXT("\t\tHDC: 0x%x\r\n"),
		     my_va_arg(marker,HDC)
		    );
	    break;
	  }
	  case ICCOMPRESSPTR: {
	    LPICCOMPRESS lp = my_va_arg(marker,LPICCOMPRESS);

	    if (lp) {
	      wsprintf(
		       szDebug+wLeaderLen,
		       TEXT("\t\tICCOMPRESS: @0x%lx,(0x%lx,0x%lx,\r\n"
		       "\t\t\t@0x%lx(%d,%d,%d,%d,%d,'%4.4ls',%ld,%d),\r\n"
		       "\t\t\t@0x%lx,\r\n"
		       "\t\t\t@0x%lx(%d,%d,%d,%d,%d,'%4.4ls',%ld,%d),\r\n"
		       "\t\t\t@0x%lx,\r\n"
		       "\t\t\t%ld,%ld,%ld\r\n"),
		       lp,
		       lp->dwFlags,
		       *lp->lpdwFlags,
		       lp->lpbiOutput,
		       (UINT) lp->lpbiOutput->biSize,
		       (UINT) lp->lpbiOutput->biWidth,
		       (UINT) lp->lpbiOutput->biHeight,
		       (UINT) lp->lpbiOutput->biPlanes,
		       (UINT) lp->lpbiOutput->biBitCount,
		       (LPSTR) &lp->lpbiOutput->biCompression,
		       (DWORD) lp->lpbiOutput->biSizeImage,
		       (UINT) lp->lpbiOutput->biClrUsed,
		       lp->lpOutput,
		       lp->lpbiInput,
		       (UINT) lp->lpbiInput->biSize,
		       (UINT) lp->lpbiInput->biWidth,
		       (UINT) lp->lpbiInput->biHeight,
		       (UINT) lp->lpbiInput->biPlanes,
		       (UINT) lp->lpbiInput->biBitCount,
		       (LPSTR) &lp->lpbiInput->biCompression,
		       (DWORD) lp->lpbiInput->biSizeImage,
		       (UINT) lp->lpbiInput->biClrUsed,
		       lp->lpInput,
		       lp->lFrameNum,
		       lp->dwFrameSize,
		       lp->dwQuality
		      );
	    }
	    else {
	      lstrcat(szDebug,TEXT("\t\tICCOMPRESS: NULL\r\n"));
	    }
	    break;
	  }
	  case ICCFPTR: {
	    LPICCOMPRESSFRAMES lp = my_va_arg(marker,LPICCOMPRESSFRAMES);

	    if (lp) {
	      wsprintf(
		       szDebug+wLeaderLen,
		       TEXT("\t\tICCOMPRESSFRAMES: @0x%lx,(0x%lx,\r\n"
		       "\t\t\t@0x%lx(%d,%d,%d,%d,%d,'%4.4ls',%ld,%d),\r\n"
		       "\t\t\tid:0x%lx,\r\n"
		       "\t\t\t@0x%lx(%d,%d,%d,%d,%d,'%4.4ls',%ld,%d),\r\n"
		       "\t\t\tid:0x%lx,\r\n"
		       "\t\t\t%ld,%ld,%ld,%ld,%ld,%lu,%lu,\r\n"
		       "\t\t\t@0x%lx,@0x%lx)\r\n"),
		       lp,
		       lp->dwFlags,
		       lp->lpbiOutput,
		       (UINT) lp->lpbiOutput->biSize,
		       (UINT) lp->lpbiOutput->biWidth,
		       (UINT) lp->lpbiOutput->biHeight,
		       (UINT) lp->lpbiOutput->biPlanes,
		       (UINT) lp->lpbiOutput->biBitCount,
		       (LPSTR) &lp->lpbiOutput->biCompression,
		       (DWORD) lp->lpbiOutput->biSizeImage,
		       (UINT) lp->lpbiOutput->biClrUsed,
		       lp->lOutput,
		       lp->lpbiInput,
		       (UINT) lp->lpbiInput->biSize,
		       (UINT) lp->lpbiInput->biWidth,
		       (UINT) lp->lpbiInput->biHeight,
		       (UINT) lp->lpbiInput->biPlanes,
		       (UINT) lp->lpbiInput->biBitCount,
		       (LPSTR) &lp->lpbiInput->biCompression,
		       (DWORD) lp->lpbiInput->biSizeImage,
		       (UINT) lp->lpbiInput->biClrUsed,
		       lp->lInput,
		       lp->lStartFrame,
		       lp->lFrameCount,
		       lp->lQuality,
		       lp->lDataRate,
		       lp->lKeyRate,
		       lp->dwRate,
		       lp->dwScale,
		       lp->GetData,lp->PutData
		      );
	    }
	    else {
	      lstrcat(szDebug,TEXT("\t\tICCOMPRESSFRAMES: NULL\r\n"));
	    }
	    break;
	  }
	  case ICSSPPTR: {
	    LPICSETSTATUSPROC lp = my_va_arg(marker,LPICSETSTATUSPROC);

	    if (lp) {
	      wsprintf(
		       szDebug+wLeaderLen,
		       TEXT("\t\tICSETSTATUSPROC: @0x%lx,(0x%lx,\r\n"
		       "\t\t\t0x%lx,@0x%lx)\r\n"),
		       lp,
		       lp->dwFlags,
		       lp->lParam,
		       lp->Status
		      );
	    }
	    else {
	      lstrcat(szDebug,TEXT("\t\tICSETSTATUSPROC: NULL\r\n"));
	    }
	    break;
	  }
	  case ICOPENPTR: {
	    LPICOPEN lp = my_va_arg(marker,LPICOPEN);

	    if (lp) {
	      wsprintf(
		       szDebug+wLeaderLen,
		       TEXT("\t\tICOPEN: @0x%lx,(%ld,'%4.4ls','%4.4ls',0x%lx,"
		       "0x%lx,%ld)\r\n"),
		       lp,
		       lp->dwSize,
		       &lp->fccType,
		       &lp->fccHandler,
		       lp->dwVersion,
		       lp->dwFlags,
		       lp->dwError
		      );
	    }
	    else {
	      lstrcat(szDebug,TEXT("\t\tICOPEN: NULL\r\n"));
	    }
	    break;
	  }
	  case ICINFOPTR: {
	    LPICINFO lp = my_va_arg(marker,LPICINFO);

	    if (lp) {
	      wsprintf(
		       szDebug+wLeaderLen,
		       TEXT("\t\tICINFO: @0x%lx,(%ld,'%4.4ls','%4.4ls',0x%lx,0x%lx,"
		       "0x%lx,\r\n\t\t         '%s','%s',\r\n"
		       "\t\t         '%s')\r\n"),
		       lp,
		       lp->dwSize,
		       &lp->fccType,
		       &lp->fccHandler,
		       lp->dwFlags,
		       lp->dwVersion,
		       lp->dwVersionICM,
		       (LPSTR) lp->szName,
		       (LPSTR) lp->szDescription,
		       (LPSTR) lp->szDriver
		      );
	    }
	    else {
	      lstrcat(szDebug,TEXT("\t\tICINFO: NULL\r\n"));
	    }
	    break;
	  }
	  case INSTINFOPTR: {
	    PINSTINFO pI = my_va_arg(marker,PINSTINFO);

	    if (pI) {
	      wsprintf(
	    	       szDebug+wLeaderLen,
		       TEXT("\t\tPINSTINFO: @0x%x,(0x%lx)\r\n"),
		       pI,
		       pI->Flags
		      );
	    }
	    else {
	      lstrcat(szDebug,TEXT("\t\tPINSTINFO: NULL\r\n"));
	    }
	    break;
	  }
	  case STATEPTR: {
	    wsprintf(
		     szDebug+wLeaderLen,
		     TEXT("\t\tSTATE: @0x%lx\r\n"),
		     my_va_arg(marker,LPVOID)
		    );
	    break;
	  }
	  case DWKEYFRAME: {
	    wsprintf(
	    	     szDebug+wLeaderLen,
		     TEXT("\t\tKeyframeRate=%lu\r\n"),
		     my_va_arg(marker,DWORD)
		    );
	    break;
	  }
	  case DWQUALITY: {
	    wsprintf(
	    	     szDebug+wLeaderLen,
		     TEXT("\t\tQuality=%lu\r\n"),
		     my_va_arg(marker,DWORD)
		    );
	    break;
	  }
	  case SRCDESTPARAMS: {
	    LPVOID lpv;
	    int xSD,ySD,dxSD,dySD;

	    lpv = my_va_arg(marker,LPVOID);
	    xSD = my_va_arg(marker,int);
	    ySD = my_va_arg(marker,int);
	    dxSD = my_va_arg(marker,int);
	    dySD = my_va_arg(marker,int);

	    wsprintf(
	    	     szDebug+wLeaderLen,
		     TEXT(
		         "\t\t{Src,Dest}Params: lpBits=@0x%lx, "
			 "ul=(%d,%d), wh=(%d,%d)\r\n"
		     ),
		     lpv,
		     xSD,ySD,dxSD,dySD
		    );
	    break;
	  }
	}
	wLeaderLen = lstrlen(szDebug);
      }
      my_va_end(marker);
      lstrcat(szDebug,TEXT("\t\t);\r\n"));

      OutputDebugString(szDebug);
      if ((wDebug < 3) && hfDebug) _lwrite(hfDebug,szDebug,lstrlen(szDebug));

#ifdef	SAFELOG
      _lclose(hfDebug);
      hfDebug = _lopen(szDebugName,OF_READWRITE);
      _llseek(hfDebug,0,2);		// seek to end of file
#endif

      break;
    }

    default: {
      break;
    }
  }
#endif
}
#endif
#ifndef __BEOS__
/**********************************************************************/

void NEAR PASCAL WritePalette(
  PINSTINFO pI,
  HDC hdc,
  int Format,			// 1: color; 2: b&w
  LPBITMAPINFOHEADER lpbi
) {
  UINT wCnt;
  LPPALETTEENTRY lppe;
  RGBQUAD rgbq;

  DPF2((4,FUNCNAME,(LPTSTR) TEXT("WritePalette"),INSTINFOPTR,pI,HDCPTR,hdc,BMIHPTR,lpbi));

  // If we received a palette via SET_PALETTE, return it now.

  if (
#if !defined(NOBLACKWHITE)
    (Format == 1) &&		// if color movie
#endif
    (pI->DStuff.nPalColors > 0)
  ) {
    _fmemcpy (
	      ((LPBYTE) lpbi) + lpbi->biSize,
	      pI->DStuff.rgbqUser, 
	      pI->DStuff.nPalColors * sizeof(RGBQUAD)
	     );
    lpbi->biClrUsed = pI->DStuff.nPalColors;
    RETURNVOID;
  }

  // make identity palette by populating lowest and highest
  // colors same as the system palette's...
  GetSystemPaletteEntries(
			  hdc,
			  0,
			  256,
			  (LPPALETTEENTRY) (
					    ((LPBYTE) lpbi) +
					    lpbi->biSize
					   )
			 );

  // convert the low 10 system colors from PALETTEENTRY to
  // RGBQUAD format
  for (
       rgbq.rgbReserved = 0,
         wCnt = 0,
	   lppe = (LPPALETTEENTRY) (
				    ((LPBYTE) lpbi) +
				    lpbi->biSize
				   );
       wCnt < 10;
       wCnt++
      ) {
      rgbq.rgbRed = lppe->peRed;
      rgbq.rgbGreen = lppe->peGreen;
      rgbq.rgbBlue = lppe->peBlue;
      *((LPRGBQUAD) lppe)++ = rgbq;
  }

  // copy the standard palette RGBQUADs into the space between
  // the lower and upper system RGBQUADs
#if !defined(NOBLACKWHITE)
  if (Format == 1) {		// color movie
#endif

    while (wCnt < ((UINT) numStdPAL8 + 10)) {
      *((LPRGBQUAD) lppe)++ = stdPAL8[wCnt++ - 10];
    }

#if !defined(NOBLACKWHITE)
  } else {			// black & white movie
    /*
      Incoming Y is in 0..255.  The lookup table GreyByteLookup maps
      them linearly into palette indices in 10..245.  The palette
      indices are linearly mapped via the palette we return here back to
      0..255.
     */
    while (wCnt < 246) {
      ((LPRGBQUAD) lppe)->rgbRed =
	((LPRGBQUAD) lppe)->rgbGreen =
	((LPRGBQUAD) lppe)->rgbBlue =
	(((wCnt - 10) << 8) + (236 / 2))/ 236;
	
      ((LPRGBQUAD) lppe)++;
      wCnt++;
    }
  }
#endif
  for (
       rgbq.rgbRed = rgbq.rgbGreen = 0, rgbq.rgbBlue = 1;
       wCnt < 246;
       wCnt++, rgbq.rgbBlue++
      ) {
      *((LPRGBQUAD) lppe)++ = rgbq;
  }

  // convert the upper 10 system colors from PALETTEENTRY to
  // RGBQUAD format
  for (; wCnt < 256; wCnt++) {
      rgbq.rgbRed = lppe->peRed;
      rgbq.rgbGreen = lppe->peGreen;
      rgbq.rgbBlue = lppe->peBlue;
      *((LPRGBQUAD) lppe)++ = rgbq;
  }

  lpbi->biClrUsed = 256;// palette contains 256 entries

  RETURNVOID;
}
/*****************************************************************************
 *
 * SetStatusProc() implements ICM_SET_STATUS_PROC
 *
 * We are being given information that enables us to callback to a status
 * routine during compression
 *
 ****************************************************************************/
DWORD NEAR PASCAL SetStatusProc(
    PINSTINFO pI, LPICSETSTATUSPROC icssp, DWORD dwSize
)
{
    DPF2((4,FUNCNAME,(LPTSTR) TEXT("SetStatusProc"),INSTINFOPTR,pI,ICSSPPTR,icssp,DWSIZE,dwSize));

#ifndef	NOCOMPRESSION
    if (pI->fCompressInitialized) { // new info in midst of current stream...
	RETURN (
	    (CVSetStatusProc(pI->CStuff.CContext,icssp)) ?
		ICERR_OK : ICERR_ERROR
	);
    }

    //
    // outside of compression stream, latch data for later init...
    //
    pI->CStuff.icssp = *icssp;

    RETURN (ICERR_OK);
#else
    RETURN ((DWORD) ICERR_UNSUPPORTED);
#endif
}


/*****************************************************************************
 *
 * CompressGetSize() implements ICM_COMPRESS_GET_SIZE
 *
 * This function returns how much (upper bound) memory a compressed frame
 * will take.
 *
 ****************************************************************************/
DWORD NEAR PASCAL CompressGetSize(
				  PINSTINFO pI,
				  LPBITMAPINFOHEADER lpbiIn,
				  LPBITMAPINFOHEADER lpbiOut
				 )
{
#ifndef	NOCOMPRESSION
  DWORD dwWH;				// will be product of width,height
#endif

  DPF2((4,FUNCNAME,(LPTSTR) TEXT("CompressGetSize"),INSTINFOPTR,pI,BMIHPTR,lpbiIn,BMIHPTR,lpbiOut));

#ifndef	NOCOMPRESSION
  // formula for calculating min and max CV outputs:
  //
  //  min = ((Width * Height) >> 4) + (256 * 6);
  //  max = ((Width * Height) >> 2) + ((Width * Height) >> 6) + (256 * 6 * 6)
  //
  //  to the above we will also add in a constant 84 to take into account all
  //  the headers

  dwWH = lpbiIn->biWidth * lpbiIn->biHeight;

  RETURN ((dwWH >> 2) + (dwWH >> 6) + (256 * 6 * 6) + 84);
#else
  RETURN ((DWORD) ICERR_UNSUPPORTED);
#endif
}

/*****************************************************************************
 *
 * CompressGetFormat() implements ICM_GETFORMAT
 *
 * This message asks, "If I gave you this bitmap, how much memory would it
 * be compressed?"
 *
 * If the output bitmap info header is NULL, we just return how big the
 * header would be (header + palette, actually)
 *
 * Otherwise, we fill in the header, most importantly the biSizeImage.
 * This field must contain an upper bound on the size of the compressed
 * frame. A value that is too high here will result in inefficient
 * memory allocation at compression time, but will not be reflected
 * to the stored bitmap - the compression algorithm may chop biSizeImage
 * down to the actual amount with no ill effects.
 *
 ****************************************************************************/
DWORD NEAR PASCAL CompressGetFormat(
				    PINSTINFO pI,
				    LPBITMAPINFOHEADER lpbiIn,
				    LPBITMAPINFOHEADER lpbiOut
				   )
{
#ifndef	NOCOMPRESSION
  DWORD dwReturn;
#endif

  DPF2((4,FUNCNAME,(LPTSTR) TEXT("CompressGetFormat"),INSTINFOPTR,pI,BMIHPTR,lpbiIn,BMIHPTR,lpbiOut));

#ifndef	NOCOMPRESSION
  // make sure that the input BITMAPINFOHEADER describes a bitmap
  // format that we understand
  if ((dwReturn = CompressQuery(pI,lpbiIn,NULL)) != ICERR_OK) {
    RETURN (dwReturn);
  }

  // if the output BITMAPINFOHEADER has not been defined then the caller
  // just wants to know what the size of the BITMAPINFOHEADER s/b
  if (!lpbiOut) {
    RETURN (sizeof(BITMAPINFOHEADER));
  }

  // initialize the fields of the output BITMAPINFOHEADER
  lpbiOut->biSize = sizeof(BITMAPINFOHEADER);
  lpbiOut->biWidth = lpbiIn->biWidth;
  lpbiOut->biHeight = lpbiIn->biHeight;
  lpbiOut->biPlanes = 1;
  lpbiOut->biBitCount = 
    pI->fBlackAndWhite ? 40 : 24;
  lpbiOut->biCompression = BI_CV;
  lpbiOut->biSizeImage = CompressGetSize(pI,lpbiIn,lpbiOut);
  lpbiOut->biXPelsPerMeter =
  lpbiOut->biYPelsPerMeter =
  lpbiOut->biClrUsed =
  lpbiOut->biClrImportant = 0L;

  RETURN (ICERR_OK);
#else
  RETURN ((DWORD) ICERR_UNSUPPORTED);
#endif
}

/*****************************************************************************
 *
 * CompressFramesInfo() implements ICM_COMPRESS_FRAMES_INFO
 *
 * We are being given information that enables us to better guide the
 * process of compressing the stream to a given data rate,quality, etc...
 *
 ****************************************************************************/
DWORD NEAR PASCAL CompressFramesInfo(
    PINSTINFO pI, ICCOMPRESSFRAMES FAR *iccf, DWORD dwSize
)
{
#ifndef	NOCOMPRESSION
    COMPRESSINFO ci;
#endif

    DPF2((4,FUNCNAME,(LPTSTR) TEXT("CompressFramesInfo"),INSTINFOPTR,pI,ICCFPTR,iccf,DWSIZE,dwSize));

#ifndef	NOCOMPRESSION
    ci.dwFlags = iccf->dwFlags;
    ci.lQuality = iccf->lQuality;
    ci.lDataRate = iccf->lDataRate;
    ci.lKeyRate = iccf->lKeyRate;
    ci.dwRate = iccf->dwRate;
    ci.dwScale = iccf->dwScale;
    ci.dwPadding = 2048;
    ci.dwOverheadPerFrame = iccf->dwOverheadPerFrame;
    ci.ratioUpper = KeyToInterRatioUpper;
    ci.ratioLower = KeyToInterRatioLower;

    if (pI->fCompressInitialized) { // new info in midst of current stream...

	RETURN (
	    (CVCompressFramesInfo(pI->CStuff.CContext,&ci)) ?
		ICERR_OK : ICERR_ERROR
	);
    }

    //
    // outside of compression stream, latch data for later init...
    //
    pI->CStuff.ci = ci;

    RETURN (ICERR_OK);
#else
    RETURN ((DWORD) ICERR_UNSUPPORTED);
#endif
}

/*****************************************************************************
 *
 * CompressFrames() implements ICM_COMPRESS_FRAMES
 *
 * Being asked to compress a stream of frames (instead of just one at
 * a time)
 *
 ****************************************************************************/
DWORD NEAR PASCAL CompressFrames(
    PINSTINFO pI, ICCOMPRESSFRAMES FAR *iccf, DWORD dwSize
)
{
#ifndef	NOCOMPRESSION
#if 0
    int nFrame;
#endif
#endif

    DPF2((4,FUNCNAME,(LPTSTR) TEXT("CompressFrames"),INSTINFOPTR,pI,ICCFPTR,iccf,DWSIZE,dwSize));

#ifndef	NOCOMPRESSION
#if	1
    RETURN ((DWORD) ICERR_UNSUPPORTED);
#else
    //
    // if we've not yet initialized for compression, just latch the
    // ICCOMPRESSFRAMES data
    //
    if (!pI->fCompressInitialized) { // not yet init'ed for compression...
	pI->ci.lQuality = iccf->lQuality;
	pI->ci.lDataRate = iccf->lDataRate;
	pI->ci.lKeyRate = iccf->lKeyRate;
	pI->ci.dwRate = iccf->dwRate;
	pI->ci.dwScale = iccf->dwScale;
        pI->ci.ratioUpper = KeyToInterRatioUpper;
        pI->ci.ratioLower = KeyToInterRatioLower;
    }
    else { // inside compression begin/end brackets...
        if (!CVCompressFramesInfo(pI->CStuff.CContext,iccf)) {
	    RETURN (ICERR_ERROR);
	}
    }

    //
    // for each frame to compress, get the uncompressed frame data,
    // compress it, and store to the compressed destination buffer
    //
    for (nFrame = 0; nFrame < iccf->lFrameCount; nFrame++) {
    }
#endif
#else
    RETURN ((DWORD) ICERR_UNSUPPORTED);
#endif
}

/*****************************************************************************
 *
 * DecompressQuery() implements ICM_DECOMPRESS_QUERY
 *
 * See CompressQuery()
 * 
 ****************************************************************************/
LRESULT NEAR PASCAL DecompressQuery(
    PINSTINFO pI,
    DWORD dwFlags,
    LPBITMAPINFOHEADER lpbiSrc,
    LPVOID lpSrc,
    int xSrc,
    int ySrc,
    int dxSrc,
    int dySrc,
    LPBITMAPINFOHEADER lpbiDst,
    LPVOID lpDst,
    int xDst,
    int yDst,
    int dxDst,
    int dyDst
)
{
  DPF2((7,FUNCNAME,(LPTSTR) TEXT("DecompressQuery"),INSTINFOPTR,pI,DWVALUE,dwFlags,BMIHPTR,lpbiSrc,SRCDESTPARAMS,lpSrc,xSrc,ySrc,dxSrc,dySrc,BMIHPTR,lpbiDst,SRCDESTPARAMS,lpDst,xDst,yDst,dxDst,dyDst));

    //
    // determine if the input compressed DIB data is in a format we like.
    //
    if (
	!lpbiSrc ||
	!ValidCompressedFormat(lpbiSrc->biCompression,lpbiSrc->biBitCount)
    ) {
	RETURN (ICERR_BADFORMAT);
    }

    //
    // allow (-1) as a default width/height
    //
    if (dxSrc == -1) dxSrc = (int) lpbiSrc->biWidth;
    if (dySrc == -1) dySrc = (int) lpbiSrc->biHeight;

    //
    //  we can't clip the source.
    //
    if (xSrc || ySrc) return (ICERR_BADPARAM);
    if (
        (dxSrc != (int) lpbiSrc->biWidth) ||
	(dySrc != (int) lpbiSrc->biHeight)
    ) {
        RETURN (ICERR_BADPARAM);
    }

    //
    // are we being asked to query just the input format?
    //
    if (lpbiDst) { // no -- check the output format too...
      DIBTYPE dibType;

      //
      // allow (-1) as a default width/height
      //
      if (dxDst == -1) dxDst = (int) lpbiDst->biWidth;
      if (dyDst == -1) dyDst = abs((int) lpbiDst->biHeight);

      //
      // make sure we can handle the format to decompress to.
      //
      if (
	((dibType = DecompressedDibType(lpbiDst)) == NoGood) ||
	!(
	  ((dxDst == dxSrc) && (dyDst == dySrc)) ||
	  (
	    (dibType == Dither8) &&		// 8bpp
	    (dxDst == (dxSrc << 1)) &&	// 2:1 stretch only
	    (dyDst == (dySrc << 1))
	  )
	) || (
	  ((dibType == CPLA) || (dibType == CLJR)) &&
	  (3 & (dxDst | dyDst))
	) || (
	  ((dibType == YUY2) || (dibType == UYVY)) &&
	  (1 & dxDst)
	)
      ) {
	  RETURN (ICERR_BADFORMAT);
      }
    }

    RETURN (ICERR_OK);
}

/*****************************************************************************
 *
 * DecompressGetFormat() implements ICM_DECOMPRESS_GET_FORMAT
 *
 * See CompressGetFormat()
 *
 ****************************************************************************/
LRESULT NEAR PASCAL DecompressGetFormat(
  PINSTINFO pI,
  LPBITMAPINFOHEADER lpbiIn,
  LPBITMAPINFOHEADER lpbiOut
) {
  DPF2((4,FUNCNAME,(LPTSTR) TEXT("DecompressGetFormat"),INSTINFOPTR,pI,BMIHPTR,lpbiIn,BMIHPTR,lpbiOut));

  // make sure that the input BITMAPINFOHEADER describes a bitmap
  // format that we understand
  if (
      !lpbiIn ||
      !ValidCompressedFormat(lpbiIn->biCompression,lpbiIn->biBitCount)
     ) {
    RETURN (ICERR_BADFORMAT);
  }

  // if lpbiOut == NULL then return the size required to hold a output
  // format -- only return palette entries (if they exist in the input
  // DIB header) if we are decompressing to 8 bit palettized
  //
  if (!lpbiOut) {
    RETURN (sizeof(BITMAPINFOHEADER));
  }

  // return our preferred format...
  lpbiOut->biSize = sizeof(BITMAPINFOHEADER);
  lpbiOut->biWidth = lpbiIn->biWidth;
  lpbiOut->biHeight = lpbiIn->biHeight;
  lpbiOut->biPlanes = 1;
  lpbiOut->biBitCount = 24;
  lpbiOut->biCompression = BI_RGB;
  lpbiOut->biSizeImage = WIDTHBYTES(lpbiIn->biWidth * 24) * lpbiIn->biHeight;
  lpbiOut->biXPelsPerMeter =
  lpbiOut->biYPelsPerMeter =
  lpbiOut->biClrUsed =
  lpbiOut->biClrImportant = 0L;

  RETURN (ICERR_OK);
}

/*****************************************************************************
 *
 * DecompressGetPalette() implements ICM_GET_PALETTE
 *
 * This function has no Compress...() equivalent
 *
 * It is used to pull the palette from a frame in order to possibly do
 * a palette change.
 *
 ****************************************************************************/
LRESULT NEAR PASCAL DecompressGetPalette(
				         PINSTINFO pI,
				         LPBITMAPINFOHEADER lpbiIn,
				         LPBITMAPINFOHEADER lpbiOut
				        )
{
  int CompressedFormat;		// color or black & white

  DPF2((4,FUNCNAME,(LPTSTR) TEXT("DecompressGetPalette"),INSTINFOPTR,pI,BMIHPTR,lpbiIn,BMIHPTR,lpbiOut));

  //
  // determine if the input compressed DIB data and possible output format
  // are in formats we can understand.
  //
  if (
    !lpbiIn ||
    !(
      CompressedFormat = ValidCompressedFormat(
	lpbiIn->biCompression,
	lpbiIn->biBitCount
      )
    ) ||
    (
     lpbiOut &&
     (DecompressedDibType(lpbiOut) == NoGood)
    )
  ) {
    RETURN (ICERR_BADFORMAT);
  }

  // if lpbiOut == NULL then, return the size required to hold an output
  // format
  if (!lpbiOut) {
    RETURN (sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
  }

  // only honor the message if the output format specifies 8 bpp...
  if (lpbiOut->biBitCount == 8) { // return palette used for decompression...
    HDC hDC;

    if (hDC = GetDC((HWND) NULL)) {
      WritePalette(pI, hDC, CompressedFormat, lpbiOut);
      ReleaseDC((HWND) NULL,hDC);
      RETURN (ICERR_OK);
    }
  }

  RETURN (ICERR_ERROR);
}

/*****************************************************************************
 *
 * DecompressSetPalette() implements ICM_DECOMPRESS_SET_PALETTE
 *
 * This function has no Compress...() equivalent
 *
 * It is used to specify the application's desired palette.
 *
 ****************************************************************************/
LRESULT NEAR PASCAL DecompressSetPalette(
  PINSTINFO pI,
  LPBITMAPINFOHEADER lpbi
) {
  DPF2((3,FUNCNAME,(LPTSTR) TEXT("DecompressSetPalette"),INSTINFOPTR,pI,BMIHPTR,lpbi));

  if (lpbi) {

    if (lpbi->biBitCount != 8) {// If the DIB is not 8 bpp, reject it.
      RETURN (ICERR_BADFORMAT);
    }

    if (			// Allocate space to hold the palette.
      !pI->DStuff.rgbqUser &&
      !(
	pI->DStuff.rgbqUser = (LPRGBQUAD) GlobalAllocPtr(
	  GMEM_MOVEABLE | GMEM_ZEROINIT,
	  256 * sizeof (RGBQUAD)
	)
      )
    ) {
      RETURN (ICERR_MEMORY);
    }

    // Copy the incoming palette into the newly allocated space.
    pI->DStuff.nPalColors =
      ((lpbi->biClrUsed > 0) && (lpbi->biClrUsed < 256)) ?
      ((unsigned int) lpbi->biClrUsed) :
      256;
  
    _fmemcpy (
      pI->DStuff.rgbqUser, 
      ((LPBYTE) lpbi) + lpbi->biSize,
      pI->DStuff.nPalColors * sizeof(RGBQUAD)
    );

  } else {			// NULL lpbi means back to normal palette

    if (pI->DStuff.rgbqUser) {	// free up palette if any
      GlobalFreePtr(pI->DStuff.rgbqUser);
      pI->DStuff.rgbqUser = (LPRGBQUAD) 0;
    }
    pI->DStuff.nPalColors = 0;	// no palette any more
  }

  // If there is a DContext already, then a decompression is in progress.
  // Modify the palette translation in the context.
  if (
    pI->DStuff.DContext &&
    (pI->DStuff.DIBType == Dither8)
  ) {
    CVDecompressSetPalette(
      pI->DStuff.DContext,
      pI->DStuff.rgbqUser, 
      pI->DStuff.nPalColors 
    );
  }
  RETURN (ICERR_OK);
}

/*****************************************************************************
 *
 * Get() implements the ICM_GET message
 *
 ****************************************************************************/
LRESULT NEAR PASCAL Get(PINSTINFO pI, FOURCC fcc, DWORD dw)
{
  DPF2((4,FUNCNAME,(LPTSTR) TEXT("Get"),INSTINFOPTR,pI,FCC,fcc,DWVALUE,dw));

// Constants for ICM_GET:
#define ICM_DIB1632	mmioFOURCC('1','6','3','2') // handle 16:32 ->s?
#define ICM_DIB0032	mmioFOURCC('F','l','a','t') // handle 0:32 ->s?

  switch (fcc) {

#if	!defined(WIN32)
    case ICM_DIB1632: {
      RETURN (ICERR_OK);
    }
#endif

    case ICM_DIB0032:
    default: {
      RETURN (ICERR_UNSUPPORTED);
    }
  }
}

/*****************************************************************************
 *
 * Set() implements the ICM_SET message
 *
 ****************************************************************************/
LRESULT NEAR PASCAL Set(PINSTINFO pI, DWORD dw1, DWORD dw2)
{
  DPF2((4,FUNCNAME,(LPTSTR) TEXT("Set"),INSTINFOPTR,pI,DWVALUE,dw1,DWVALUE,dw2));

  RETURN (ICERR_UNSUPPORTED);
}
/*****************************************************************************
 *
 * Load() is called from the ICM_LOAD message.
 *
 * Tasks such as allocating global memory that is non-instance specific
 * or initializing coprocessor hardware may be performed here.
 *
 * Our simple case needs none of this.
 *
 ****************************************************************************/
BOOL NEAR PASCAL Load(void)
{
  unsigned short ui;

  DPF(TEXT("Load()"));

#if	!defined(WIN32)
  // our low-level code wants to make use of a selector which has been
  // allocated and setup so that it can address all of linear memory
  _asm {
	xor	ax,ax			// DPMI: alloc LDT descriptor
	mov	cx,1			// allocate just 1 selector
	int	031h			// allocate LDT descriptor
	mov	flatSel,ax 		// save returned value
	jnc	darnit
	jmp	fail_load		// failed -- return error...
darnit:	mov	bx,ax			// our selector
	mov	cx,0ffffh
	mov	dx,cx			// DX:CX is limit for all memory
	mov	ax,8			// DPMI: set segment limit
	int	031h			// set segment limit
  }
#endif

#ifdef __BEOS__
return (FALSE);
#else
  // read in override temporal,spatial quality values
  TQuality = (WORD) GetPrivateProfileInt(
  					 szIniSection,
					 szIniTQuality,
					 0xffff,
					 szIniFile
					);
  if (TQuality != (WORD)0xffff) {
    TQuality = CV_QUALITY(TQuality);
  }
  SQuality = (WORD) GetPrivateProfileInt(
  					 szIniSection,
					 szIniSQuality,
					 0xffff,
					 szIniFile
					);
  if (SQuality != (WORD)0xffff) {
    SQuality = CV_QUALITY(SQuality);
  }

  CompressFlags.FractionInterCodeBook = !!GetPrivateProfileInt(
							       szIniSection,
							       szIniFraction,
							       1,
							       szIniFile
							      );

  CompressFlags.LogRequested = !!GetPrivateProfileInt(
						       szIniSection,
						       szIniLogRequested,
						       0,
						       szIniFile
						      );

  // what is our keyframe to interframe size ratio if compressing?
  ui = (unsigned short) GetPrivateProfileInt(
      szIniSection, TEXT("KeyToInterRatioUpper"), DFLT_KEY_TO_INTER_RATIO_UPPER,
      szIniFile
  );
  KeyToInterRatioUpper = (unsigned short) (
      (ui > 32767) ? DFLT_KEY_TO_INTER_RATIO_UPPER : ((ui == 0) ? 1 : ui)
  );

  ui = (unsigned short) GetPrivateProfileInt(
      szIniSection, TEXT("KeyToInterRatioLower"), DFLT_KEY_TO_INTER_RATIO_LOWER,
      szIniFile
  );
  KeyToInterRatioLower = (unsigned short) (
      (ui > 32767) ? DFLT_KEY_TO_INTER_RATIO_LOWER : ((ui == 0) ? 1 : ui)
  );

  RETURN (TRUE);
#endif
#if	!defined(WIN32)
fail_load:

  RETURN (FALSE);
#endif
}

/*****************************************************************************
 *
 * Free() is called from the ICM_FREE message.
 *
 * It should totally reverse the effects of Load() in preparation for
 * the DRV being removed from memory.
 *
 ****************************************************************************/
void NEAR PASCAL Free()
{
  DPF(TEXT("Free()"));

#if	!defined(WIN32)
  if (flatSel) { // free up the flat-mapping selector...
    _asm {
	xor	cx,cx
	xor	dx,dx			// DX:CX is limit for no memory
	mov	ax,8			// DPMI: set segment limit
	mov	bx,flatSel		// BX is selector to work on
	int	031h			// set segment limit
	mov	ax,1			// DPMI: free LDT descriptor
	mov	bx,flatSel		// BX is selector to work on
	int	031h			// free LDT descriptor
    }
  }
#endif

#ifdef	DEBUG
  if (hfDebug) {
    _lclose(hfDebug);
    wDebug = 0xffff;
  }
#endif

  RETURNVOID;
}

/*****************************************************************************
 *
 * Open() is called from the ICM_OPEN message
 *
 * This message will be sent for a particular compress/decompress session.
 * Our code must verify that we are indeed being called as a video
 * compressor and create/initialize a state structure. The ICM will
 * give us back the pointer to that structure on every message dealing
 * with this session.
 *
 ****************************************************************************/
PINSTINFO NEAR PASCAL Open(ICOPEN FAR *icopen)
{
  auto PINSTINFO pI;

  //
  // refuse to open if we are not being opened as a Video compressor
  //
  if (icopen->fccType != ICTYPE_VIDEO) {
    icopen->dwError = (DWORD) ICERR_ERROR;
    DPF2((2,FUNCNAME,(LPTSTR) TEXT("Open"),ICOPENPTR,(LPICOPEN) icopen));
    RETURN (NULL);
  }

  // if one of the valid open modes...
  switch (icopen->dwFlags) {

    case ICMODE_QUERY:
    case ICMODE_COMPRESS:
    case ICMODE_DRAW:
    case ICMODE_DECOMPRESS:
    case ICMODE_FASTCOMPRESS:
    case ICMODE_FASTDECOMPRESS: {

      // allocate an instance and initialize its fields...
      if (pI = (PINSTINFO) LocalAlloc(LPTR, sizeof(INSTINFO))) {

	pI->Flags = icopen->dwFlags;// remember what we're doing

	icopen->dwError = ICERR_OK;
      }
      else { // the INSTINFO allocation failed...
	icopen->dwError = (DWORD) ICERR_MEMORY;
      }

      break;
    }

    default: { // not an open mode we recognize...
      pI = NULL;
      icopen->dwError = (DWORD) ICERR_ERROR;
      break;
    }
  }

  DPF2((3,FUNCNAME,(LPTSTR) TEXT("Open"),ICOPENPTR,(LPICOPEN) icopen,DWVALUE,(DWORD) (unsigned short) pI));
  RETURN (pI);
}

/*****************************************************************************
 *
 * Close() is called on the ICM_CLOSE message.
 *
 * This message is the complement to ICM_OPEN and marks the end
 * of a compress/decompress session. We kill any in-progress operations
 * (although this shouldn't be needed) and free our instance structure.
 *
 ****************************************************************************/
DWORD NEAR PASCAL Close(PINSTINFO pI)
{
  DPF2((2,FUNCNAME,(LPTSTR) TEXT("Close"),INSTINFOPTR,pI));

  // since the open flags were nothing more than "hints" we'll just go
  // ahead and call all the ...End functions and let the initialized flags
  // dictate whether or not cleanup operations take place...

  CompressEnd(pI);		// in case compression was done

  DecompressEnd(pI);		// in case decompression was done
  if (pI->DStuff.DContext) { // terminate any outstanding decompress bracket...
    CVDecompressEnd(pI->DStuff.DContext, FALSE, 0);
  }

  if (pI->DStuff.pBits) { // free up bits buffer memory
    GlobalFreePtr(pI->DStuff.pBits);
  }

  if (pI->DStuff.rgbqUser) {	// free up palette if any
    GlobalFreePtr(pI->DStuff.rgbqUser);
  }

  LocalFree((HLOCAL) pI);	// now safe to free up the INSTINFO

  RETURN (1L);
}

/*****************************************************************************
 *
 * QueryAbout() and About() handle the ICM_ABOUT message.
 *
 * QueryAbout() returns TRUE to indicate we support an about box.
 * About() displays the box.
 *
 ****************************************************************************/
BOOL NEAR PASCAL QueryAbout(void)
{
  DPF(TEXT("QueryAbout()"));

  RETURN (TRUE);
}

DWORD NEAR PASCAL About(HWND hwnd)
{
  DPF(TEXT("About()"));

  RETURN ((DWORD) DialogBox(ghModule,TEXT("About"),hwnd,AboutDlgProc));
}

/*****************************************************************************
 *
 * GetDriverVersionInfo() gets version info in the file's resources.
 *
 ****************************************************************************/
void NEAR PASCAL GetDriverVersionInfo(
				      LPTSTR lpRequested,
				      LPTSTR lpOutInfo
				     )
{
  DWORD dwVerInfoSize;
  DWORD dwVerHnd;
  TCHAR szBuf[_MAX_PATH];

  // assume no information is returned
  *lpOutInfo = '\0';

  // retrieve the fully qualified pathname of this module
  GetModuleFileName(ghModule,szBuf,NUMCHARS(szBuf));

  // attempt to locate the version info in the file's resources...
  if (dwVerInfoSize = GetFileVersionInfoSize(szBuf,&dwVerHnd)) {
    LPVOID lpVerInfo;		// -> to block to hold info

    // get a block big enough to hold version info
    if (lpVerInfo = (LPVOID) GlobalAllocPtr(GMEM_MOVEABLE,dwVerInfoSize)) {

      // get the file version first...
      if (GetFileVersionInfo(szBuf, dwVerHnd, dwVerInfoSize, lpVerInfo)) {
	LPVOID lpData;
#if	defined(WIN32)
	DWORD wCnt;
#else
	WORD wCnt;
#endif

	/*
	  First try to look up the string in the language/codepage
	  specified first in \VarFileInfo\Translation.  If that lookup
	  succeeds, then we're done.  If not, we try using
	  U. S. English/Windows.
	 */
	if (
	  (
	    VerQueryValue(
	      lpVerInfo,
	      TEXT("\\VarFileInfo\\Translation"),
	      &lpData,
	      &wCnt
	    ) &&
	    (wCnt >= 4) &&
	    (
	      wsprintf(
		szBuf,
		TEXT("\\StringFileInfo\\%04X%04X\\%s"),
		((WORD *) lpData)[0],
		((WORD *) lpData)[1],
		lpRequested
	      ),
	      VerQueryValue(lpVerInfo, szBuf, &lpData, &wCnt)
	    ) &&
	    wCnt
	  ) ||
	  (
	    (
	      lstrcpy(szBuf,TEXT("\\StringFileInfo\\040904E4\\")),
	      lstrcat(szBuf,lpRequested),
	      VerQueryValue(lpVerInfo, szBuf, &lpData, &wCnt)
	    ) &&
	    wCnt
	  )
	) {
	  lstrcpy(lpOutInfo, lpData);
	}
      }

      GlobalFreePtr(lpVerInfo);
    }
  }
}

/*****************************************************************************
 *
 * AboutDlgProc() is the dialog procedure for the About... box
 *
 ****************************************************************************/
BOOL CALLBACK _loadds AboutDlgProc(
				   HWND hDlg,
				   UINT msg,
				   WPARAM wParam,
				   LPARAM lParam
				  )
{
  static HBITMAP hbmSMAC;
  static BITMAP bmSMAC;

  switch (msg) {
    TCHAR szString[128];

    case WM_COMMAND: {
      switch (wParam) {
	case IDCANCEL:
	case IDOK: {
	  if (hbmSMAC) DeleteObject(hbmSMAC);
	  EndDialog(hDlg,(int) ICERR_OK);
	  break;
	}
      }
      break;
    }

    case WM_INITDIALOG: {

      // initialize title of About dialog box with product name
      GetDriverVersionInfo(TEXT("ProductName"),szString);
      SetWindowText(hDlg,szString);

      // display file description
      GetDriverVersionInfo(TEXT("FileDescription"),szString);
      SendDlgItemMessage(
			 hDlg,
			 ID_PRODUCTNAME,
			 WM_SETTEXT,
			 0,
			 (LPARAM)(LPSTR) szString
			);

      // display file version
      LoadString(ghModule,IDS_VERSION,szString,NUMCHARS(szString));
      GetDriverVersionInfo(TEXT("FileVersion"),&szString[lstrlen(szString)]);
      SendDlgItemMessage(
			 hDlg,
			 ID_PRODUCTVERSION,
			 WM_SETTEXT,
			 0,
			 (LPARAM)(LPSTR) szString
			);

      // display legal copyright info
      GetDriverVersionInfo(TEXT("LegalCopyright"),szString);
      SendDlgItemMessage(
			 hDlg,
			 ID_COPYRIGHT,
			 WM_SETTEXT,
			 0,
			 (LPARAM)(LPSTR) szString
			);

      // get a handle to the bitmap resource for the dialog box
      if (hbmSMAC = LoadBitmap(ghModule,TEXT("SMACLOGO"))) {
        GetObject(hbmSMAC,sizeof(BITMAP),&bmSMAC);
      }

      return (TRUE);
    }

    case WM_PAINT: { // paint the main window of the dialog box...
      PAINTSTRUCT ps;

      BeginPaint(hDlg,&ps);

      if (hbmSMAC) { // we have the bitmap loaded...
        HDC hdcMem;


	if (hdcMem = CreateCompatibleDC(ps.hdc)) { 
	  HBITMAP hbmOld;
	  RECT rc;

	  hbmOld = SelectObject(hdcMem,hbmSMAC);

	  //
	  // determine a rectangle in the dialog box to center the bitmap into
	  //

	  GetWindowRect(GetDlgItem(hDlg,ID_PRODUCTNAME),&rc);
	  ScreenToClient(hDlg,(POINT FAR *) &rc.left);
	  ScreenToClient(hDlg,(POINT FAR *) &rc.right);

	  rc.left = (rc.right - bmSMAC.bmWidth) >> 1;
	  rc.top = (rc.top - bmSMAC.bmHeight) >> 1;

	  //
	  // Bitblt our nice logo into the dialog box
	  //
	  BitBlt(
	    ps.hdc,
	    rc.left,
	    rc.top,
	    bmSMAC.bmWidth,
	    bmSMAC.bmHeight,
	    hdcMem,
	    0,
	    0,
	    SRCCOPY
	  );

	  SelectObject(hdcMem,hbmOld);
	  DeleteDC(hdcMem);

	}
      }

      EndPaint(hDlg,&ps);

      break;
    }

  }

  return (FALSE);
}

/*****************************************************************************
 *
 * QueryConfigure() and Configure() implement the ICM_CONFIGURE message.
 *
 * These functions put up a dialog that allows the user, if he so
 * chooses, to modify the configuration portion of our state info.
 *
 ****************************************************************************/
#if defined(NOBLACKWHITE)

BOOL NEAR PASCAL QueryConfigure(PINSTINFO pI)
{
  DPF2((2,FUNCNAME,(LPTSTR) TEXT("QueryConfigure"),INSTINFOPTR,pI));

  RETURN (FALSE);
}

DWORD NEAR PASCAL Configure(PINSTINFO pI, HWND hwnd)
{
  DPF2((2,FUNCNAME,(LPTSTR) TEXT("Configure"),INSTINFOPTR,pI));

  RETURN ((DWORD) ICERR_ERROR);
}

#else

BOOL NEAR PASCAL QueryConfigure(PINSTINFO pI)
{
  DPF2((2,FUNCNAME,(LPTSTR) TEXT("QueryConfigure"),INSTINFOPTR,pI));

  RETURN (TRUE);
}

DWORD NEAR PASCAL Configure(PINSTINFO pI, HWND hwnd)
{
  DPF2((2,FUNCNAME,(LPTSTR) TEXT("Configure"),INSTINFOPTR,pI));

  switch (
    DialogBoxParam(
      ghModule,
      TEXT("Configure"),
      hwnd,
      ConfigureDlgProc,
      (LPARAM) pI->fBlackAndWhite
    )
  ) {
    case 0: {			// 0 means force Color
      pI->fBlackAndWhite = 0;
      RETURN(TRUE);
    }
    case 1: {			// 1 means force Black & White
      pI->fBlackAndWhite = 1;
      RETURN(TRUE);
    }
    case 2: {			// 2 means no change
      RETURN(TRUE);
    }
  }
  RETURN(FALSE);		// could not start dialog
}

/*****************************************************************************
 *
 * ConfigureDlgProc() is the dialog procedure for the Configure... box
 *
 ****************************************************************************/
BOOL CALLBACK _loadds ConfigureDlgProc(
  HWND hDlg,
  UINT msg,
  WPARAM wParam,
  LPARAM lParam
) {
  switch (msg) {
    case WM_COMMAND: {
      switch (wParam) {
	case IDCANCEL: {
	  EndDialog(hDlg, 2);	// no change
	  break;
	}
	case IDOK: {
	  EndDialog(		// return 1 if Black & White checked else 0
	    hDlg,
	    (int) SendDlgItemMessage(hDlg, ID_BLACKANDWHITE, BM_GETCHECK, 0, 0)
	  );
	  break;
	}
      }
      break;
    }

    case WM_INITDIALOG: {	// lParam is pI
      TCHAR szString[128];

      // initialize title of Configure dialog box with product name
      GetDriverVersionInfo(TEXT("ProductName"),szString);
      SetWindowText(hDlg,szString);

      // display file description
      GetDriverVersionInfo(TEXT("FileDescription"),szString);
      SendDlgItemMessage(
	hDlg,
	ID_PRODUCTNAME,
	WM_SETTEXT,
	0,
	(LPARAM)(LPSTR) szString
      );

      // display file version
      LoadString(ghModule,IDS_VERSION,szString,NUMCHARS(szString));
      GetDriverVersionInfo(TEXT("FileVersion"),&szString[lstrlen(szString)]);
      SendDlgItemMessage(
	hDlg,
	ID_PRODUCTVERSION,
	WM_SETTEXT,
	0,
	(LPARAM)(LPSTR) szString
      );

      // display legal copyright info
      GetDriverVersionInfo(TEXT("LegalCopyright"),szString);
      SendDlgItemMessage(
	hDlg,
	ID_COPYRIGHT,
	WM_SETTEXT,
	0,
	(LPARAM)(LPSTR) szString
      );

      /*
	Set the Color and Black & White radio buttons state from
	fBlackAndWhite passed in lParam.
       */
      SendDlgItemMessage(
	hDlg,
	ID_BLACKANDWHITE,
	BM_SETCHECK,
	(WPARAM) lParam,	// got fBlackAndWhite in lParam
	0
      );
      SendDlgItemMessage(
	hDlg,
	ID_COLOR,
	BM_SETCHECK,
	(WPARAM) !lParam,	// got fBlackAndWhite in lParam
	0
      );
      return(TRUE);
    }

    case WM_PAINT: { // paint the main window of the dialog box...
      auto HBITMAP hbmSMAC;

      if (hbmSMAC = LoadBitmap(ghModule, TEXT("SMACLOGO"))) {
	auto BITMAP bmSMAC;
	auto PAINTSTRUCT ps;
        auto HDC hdcMem;

	GetObject(hbmSMAC, sizeof(BITMAP), &bmSMAC);

	BeginPaint(hDlg, &ps);

	if (hdcMem = CreateCompatibleDC(ps.hdc)) { 
	  auto HBITMAP hbmOld;
	  auto RECT rc;

	  hbmOld = SelectObject(hdcMem,hbmSMAC);

	  // draw the bitmap centered above the product name

	  GetWindowRect(GetDlgItem(hDlg, ID_PRODUCTNAME), &rc);
	  ScreenToClient(hDlg, (POINT FAR *) &rc.left);
	  ScreenToClient(hDlg, (POINT FAR *) &rc.right);

	  rc.left = (rc.right - bmSMAC.bmWidth) >> 1;
	  rc.top = (rc.top - bmSMAC.bmHeight) >> 1;

	  BitBlt(
	    ps.hdc,
	    rc.left,
	    rc.top,
	    bmSMAC.bmWidth,
	    bmSMAC.bmHeight,
	    hdcMem,
	    0,
	    0,
	    SRCCOPY
	  );
	  SelectObject(hdcMem, hbmOld);
	  DeleteDC(hdcMem);
	}
	EndPaint(hDlg,&ps);

	DeleteObject(hbmSMAC);
      }
      break;
    }
  }
  return (FALSE);
}
#endif


/*****************************************************************************
 *
 * GetState() implements the ICM_GETSTATE message.
 *
 * We copy our configuration information and return how many bytes it took.
 *
 ****************************************************************************/
DWORD NEAR PASCAL GetState(PINSTINFO pI, LPVOID pv, DWORD dwSize)
{
  DPF2((4,FUNCNAME,(LPTSTR) TEXT("GetState"),INSTINFOPTR,pI,STATEPTR,pv,DWSIZE,dwSize));

#if !defined(NOBLACKWHITE)

  /*
    If they've passed in a pointer, remember our Black & White vs. Color
    state in the dword it points to.
   */
  if (!pv || !dwSize) {		// they're asking how big the area needs to be
    RETURN(4L);
  }
  if (dwSize < 4) {		// too small to save it
    RETURN(0L);
  }
  *(DWORD far *) pv =		// save it
    pI->fBlackAndWhite ?
    mmioFOURCC('g', 'r', 'e', 'y') :
    mmioFOURCC('c', 'o', 'l', 'r');

  // return # of bytes copied
  RETURN (4L);

#else

  // return # of bytes copied
  RETURN (0L);

#endif
}

/*****************************************************************************
 *
 * SetState() implements the ICM_SETSTATE message.
 *
 * The ICM is giving us configuration information saved by GetState()
 * earlier.
 *
 ****************************************************************************/
DWORD NEAR PASCAL SetState(PINSTINFO pI, LPVOID pv, DWORD dwSize)
{
  DPF2((4,FUNCNAME,(LPSTR) "SetState",INSTINFOPTR,pI,STATEPTR,pv,DWSIZE,dwSize));

#if !defined(NOBLACKWHITE)

  /*
    If they've passed in a pointer, use the dword it points at as a
    boolean determining our Black & White vs. Color.  Otherwise, reset
    to the default of Color.
   */
  if (!pv) {

    pI->fBlackAndWhite = 0;	// reset to color

  } else {

    if (
      (dwSize < 4) ||
      (
	((*(DWORD far *) pv) != mmioFOURCC('g', 'r', 'e', 'y')) &&
	((*(DWORD far *) pv) != mmioFOURCC('c', 'o', 'l', 'r'))
      )
    ) {
      RETURN(0L);		// not our state
    }
    pI->fBlackAndWhite =
      (*((DWORD far *) pv)) == mmioFOURCC('g', 'r', 'e', 'y');
  }
  // return # of bytes copied
  RETURN (4L);

#else

  // return # of bytes copied
  RETURN (0L);

#endif
}
/*****************************************************************************
 *
 * GetInfo() implements the ICM_GETINFO message
 *
 * We just fill in the structure to tell the ICM what we can do. The flags
 * (none of which this sample supports) mean the following :
 *
 * VIDCF_QUALITY - we support the quality variable. This means we look at
 *                 dwQuality in the ICINFO structure when compressing and
 *                 make a concious decision to trade quality for space.
 *                 (higher values of dwQuality mean quality is more
 *                 important). dwQuality is set by the ICM.
 *
 * VIDCF_TEMPORAL - We do interframe compression. In this algorithm, not
 *                  every frame is a "key frame"; some frames depend on
 *                  other frames to be generated. An example of this might
 *                  be to store frame buffer differences until the
 *                  differences are big enough to no longer make this
 *                  worthwhile, then storing another complete frame and
 *                  starting over. In this case, the complete frames that
 *                  are stored are key frames and should be flagged as
 *                  such.
 *
 * VIDCF_DRAW -     We will draw the decompressed image on our own. This is
 *                  useful if the decompression is assisted by the video
 *                  hardware.
 *
 ****************************************************************************/
DWORD NEAR PASCAL GetInfo(PINSTINFO pI, ICINFO FAR *icinfo, DWORD dwSize)
{
#if	defined(WIN32) && !defined(UNICODE)
  TCHAR szName [NUMCHARS(icinfo->szName)];
  TCHAR szDescription [NUMCHARS(icinfo->szDescription)];
#endif
  if (icinfo) { // user wants information...

    // is user's buffer large enough to contain our data?
    if (dwSize < sizeof(ICINFO)) {
      RETURN (0L);
    }

    // fill user's buffer with the information...
    icinfo->dwSize = sizeof(ICINFO);
    icinfo->fccType = ICTYPE_VIDEO;
    icinfo->fccHandler = FOURCC_CV;
    icinfo->dwFlags = (
    		       VIDCF_CRUNCH |		// we can crunch to a size
		       VIDCF_COMPRESSFRAMES |	// want COMPRESSFRAMES msg
		       VIDCF_QUALITY |		// we support quality
		       VIDCF_TEMPORAL |		// we do temporal compression
		       VIDCF_FASTTEMPORALC	//   ... real fast!
		      );
    icinfo->dwVersion = VERSION_CV;
    icinfo->dwVersionICM = ICVERSION;
#if	defined(WIN32) && !defined(UNICODE)

    // For Chicago, szName and szDescription must be in Unicode.

    LoadString(ghModule,IDS_NAME,szName,NUMCHARS(szName));
    LoadString( 
      ghModule, 
      IDS_DESCRIPTION, 
      szDescription,
      NUMCHARS(szDescription)
    );
    MultiByteToWideChar (
      CP_ACP, 
      MB_PRECOMPOSED, 
      szName,
      -1,
      icinfo->szName,
      sizeof(icinfo->szName)/2
    );
    MultiByteToWideChar (
      CP_ACP, 
      MB_PRECOMPOSED, 
      szDescription,
      -1,
      icinfo->szDescription,
      sizeof(icinfo->szDescription)/2
    );
#else
    LoadString(ghModule,IDS_NAME,icinfo->szName,NUMCHARS(icinfo->szName));
    LoadString(
      ghModule,
      IDS_DESCRIPTION,
      icinfo->szDescription,
      NUMCHARS(icinfo->szDescription)
    );
#endif
  }
  DPF2((4,FUNCNAME,(LPTSTR) TEXT("GetInfo"),INSTINFOPTR,pI,ICINFOPTR,icinfo,DWSIZE,dwSize));

  RETURN (sizeof(ICINFO));
}


#endif

