/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/dib2yuv2.h 3.1 1994/10/20 17:37:56 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: dib2yuv2.h $
 * Revision 3.1  1994/10/20 17:37:56  bog
 * Modifications to support Sunnyvale Reference version.
 * Revision 3.0  1993/12/10  14:23:58  timr
 * Initial revision, NT C-only version.
 *
 *
 * CompactVideo Codec Convert DIB to YYYYUV
 *

 * This C implementation of DYUVFRAG is implemented as a threaded interpreter.
 * Following are the fragments which the interpreter understands:
 */

typedef enum _FRAGMENT {
    cEntry,
    H1331Init,
    H1331Start,
    H1331End,
//  H134,
    H1331FetchPixel8,
    H1331FetchPixel15,
    H1331FetchPixel16,
    H1331FetchPixel24,
    H1331FetchPixel32,
    H1331ToYUV,
    H1331StoreY0,
    H1331StoreY1,
    H431,
    H1331B,
    H1331C,
    H1331Loop,
    V1331EarlyOut,
    V1331FetchY2Dest,
    V1331FetchU2Dest,
    V1331FetchV2Dest,
    V1331Body,
    V1331Body_Y2,
    V1331IncY2Dest,
    V1331IncU2V2Dest,
    V1331Loop,
    V1331StoreY2Dest,
    V1331StoreU2Dest,
    V1331StoreV2Dest,
    HVLoop,
    U2V2HInit,
    U2V2HStart,
    U2V2HEnd,
//  U2V2H134,
    U2V2H431,
    U2V2H1331B,
    U2V2H1331C,
    U2V2HLoop,
    U2V2VBody,
    cExit,
    rY_H1331Init,
    rY_H1331Start,
    rY_H1331End,
//  rY_H134,
    rY_H431,
    rY_H1331B,
    rY_H1331C,
    rY_V1331Body,
    rUV_H1331Init,
    rUV_H1331Start,
    rUV_H1331End,
//  rUV_H134,
    rUV_H431,
    rUV_H1331B,
    rUV_H1331C,
    rUV_V1331Body,
    MAXFRAGMENT,
    JUMP = 0xc000		// force enum to 16 bit from byte
}			FRAGMENT;

#define	CODEMAXSIZE	(256)		// assume no more than this for code



#if	defined(WINCPK) || defined(__BEOS__)

typedef struct _RGB {
    unsigned char B;
    unsigned char G;
    unsigned char R;
}			RGB;

#else
typedef struct _RGB {
    unsigned char filler;
    unsigned char R;
    unsigned char G;
    unsigned char B;
}			RGB;

#endif

typedef struct _RGBA {
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned char zero;
}			RGBA;

typedef struct _YUV {
    unsigned char V;
    unsigned char U;
    unsigned char Y;
}			YUV;

typedef struct tagPTRPARTS {
  unsigned short offset;		// offset of 16:16 far pointer
  unsigned short selector;		// selector of 16:16 far pointer
} PTRPARTS;

typedef struct tagCOMPILEDATA {
  LPBYTE oInterU2;			// -> U2 intermediate
  LPBYTE oInterV2;			// -> V2 intermediate
  LPBYTE oWork;				// -> work buffer scans
  long DIBYStep;			// step in bytes to next source scan
  unsigned short DIBWidth;		// width in pixels of source scan
  unsigned short workYStep;		// width in bytes of a work buffer scan
} COMPILEDATA;				// remembered compilation data

typedef struct tagDIBTOYUVPRIVATE {

  COMPILEDATA cd;

  unsigned short ofsDIBToYUV;		// index of DIBtoYUV instructions
  unsigned short ofsRecreate;		// index of Recreate instructions

  unsigned short tileHeight;		// # scans in current tile
  unsigned short filler;

  LPBYTE oBits;				// 32-bit offset of tile data
  LPBYTE oDetail;			// 32-bit offset of detail list
  LPBYTE oSmooth;			// 32-bit offset of smooth list

  unsigned long srcWidth;		// actual width in pixels of input
  unsigned long srcHeight;		// actual height in scans of input

  RGBA FAR * lookUp8;			// 8 bit palettized lookup table

  unsigned char divBy7[256 * 7];	// the divide by 7 table

} DIBTOYUVPRIVATE;

typedef struct tagDIBTOYUVAREA {
  DIBTOYUVPRIVATE dp;
  FRAGMENT f [CODEMAXSIZE];
#if	defined(WINCPK)
  BYTE	 rest [0];
#else
  BYTE	 rest [1];
#endif
} DIBTOYUVAREA;

typedef struct tagP8LOOKUP {
  unsigned long palNumEntries;		// colors in lookup
  unsigned long filler0;		// unused
  unsigned long palPalEntry[1];		// the RGB entries
} P8LOOKUP;
