/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/dibtoyuv.h 2.3 1993/10/12 17:26:53 geoffs Exp $

 * (C) Copyright 1992-1993 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: dibtoyuv.h $
 * Revision 2.3  1993/10/12 17:26:53  geoffs
 * RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
 * Revision 2.2  1993/08/04  19:06:54  timr
 * Both compressor and decompressor now run on NT.
 * 
 * Revision 2.1  93/06/10  09:18:36  geoffs
 * Add 32 bit DIB support
 * 
 * Revision 2.0  93/06/01  14:14:05  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.13  93/04/21  15:47:42  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.12  93/02/18  14:11:43  geoffs
 * Increased the CODEMAXSIZE for generated code
 * 
 * Revision 1.11  93/02/16  14:48:32  geoffs
 * Added recreate of smooth vectors from detail vectors
 * 
 * Revision 1.10  93/01/27  13:13:53  geoffs
 * Added more pointer checking code, fixed up gpf in 2nd pass vertical filter
 * 
 * Revision 1.9  93/01/27  08:01:24  geoffs
 * Added debug check for pointer ranges; added fix so that last input scan + 1 not processed
 * 
 * Revision 1.8  93/01/25  21:46:35  geoffs
 * Non 0 mod 4 input.
 * 
 * Revision 1.7  93/01/25  14:24:59  geoffs
 * Allow non 0 mod 4 frames.
 * 
 * Revision 1.6  93/01/16  16:12:36  geoffs
 * Added code to checksum detail,smooth vectors and display
 * 
 * Revision 1.5  93/01/13  17:22:46  geoffs
 * Will use globally available flatSel so cut out code here to alloc one
 * 
 * Revision 1.4  93/01/13  10:37:26  geoffs
 * The detail and smooth lists appear to be outputting consistent data now
 * 
 * Revision 1.3  93/01/11  18:44:55  geoffs
 * Have all the fragment code in now, but not yet fully debugged
 * 
 * Revision 1.2  93/01/11  09:40:39  geoffs
 * Don't get impatient -- we're almost there. Have only the 2nd vertical
 * filtering pass on UV to go.
 * 
 * Revision 1.1  93/01/10  17:01:09  geoffs
 * 75% of the way towards a complete compiled code version
 * 
 *
 * CompactVideo Codec Convert DIB to YYYYUV
 */

#ifndef	DEBUG
#define	CODEMAXSIZE	(7*1024)	// assume no more than this for code
#else
#define	CODEMAXSIZE	(12*1024)	// assume no more than this for code
#endif

typedef unsigned char	FIXUPTYPE;	// type of fixup
#define	FIXUP_EOT		255	// end of fixup table
#define	FIXUP_WORKYSTEP		0	// instruction value * work buffer ystep
#define	FIXUP_DSREL32		1	// DS relative 32-bit fixup
#define	FIXUP_JMPLP1		2	// save offset to loop1 fixup
#define	FIXUP_STOREPC0		3	// temporarily save PC
#define	FIXUP_USE_STOREDPC0	4	// fixup with temporarily saved PC
#define	FIXUP_USE_STOREDPC1	5	// fixup with temporarily saved PC
#define	FIXUP_INTERU2		6	// store -> U2 intermediate
#define	FIXUP_INTERV2		7	// store -> V2 intermediate
#define	FIXUP_WORKPLUS		8	// store -> work buffer + offset to scan
#define	FIXUP_WORKWIDTH		9	// workystep >> instruction value
#define	FIXUP_UVOFF		10	// offset of V from U intermediate
#define	FIXUP_SRCYSTEP		11	// add src ystep * instruction value
#define	FIXUP_SRCWIDTH		12	// src width >> instruction value

#ifdef	DEBUG
typedef unsigned short	FIXUPLOC;	// offset of fixup from fragment start
#else
typedef unsigned char	FIXUPLOC;	// offset of fixup from fragment start
#endif

typedef struct tagFRAGFIXUP {
  FIXUPTYPE fixupType;			// kind of fixup
  FIXUPLOC fixupPCRelOffset;		// offset of fixup from fragment start
} FRAGFIXUP;

typedef struct tagFRAGINFO {
#ifndef	WIN32
  unsigned short fragOffset;		// from base of FRAGTEXT32 segment
#else
  unsigned long fragOffset;		// from base of FRAGTEXT32 segment
#endif
  unsigned short fragLength;		// length of fragment
  FRAGFIXUP fragFixups[1];		// last entry has FIXUPTYPE == FIXUP_EOT
} FRAGINFO;

typedef struct tagCOMPILEDATA {
  unsigned long oPrivate;		// DS-relative offset of private area
  unsigned long oInterU2;		// -> U2 intermediate
  unsigned long oInterV2;		// -> V2 intermediate
  unsigned long oWork;			// -> work buffer scans
  unsigned char far *storedPC0;		// 1-level deep stored PC
  unsigned char far *storedPC1;		// 1-level deep stored PC
  unsigned long far *pJmpToLoop1;	// -> jmp to loop1 fixup location
  long DIBYStep;			// step in bytes to next source scan
  unsigned short DIBWidth;		// width in pixels of source scan
  unsigned short workYStep;		// width in bytes of a work buffer scan
} COMPILEDATA;				// remembered compilation data

typedef struct tagPTRPARTS {
  unsigned short offset;		// offset of 16:16 far pointer
  unsigned short selector;		// selector of 16:16 far pointer
} PTRPARTS;

typedef struct tagDIBTOYUVPRIVATE {

  unsigned long filler0;		// was routine called to load segment

  union {
    void (far *pDIBToYUV)(void);	// -> compiled function
    PTRPARTS parts;			// access to offset,selector
  } pCode;

  union {
    void (far *pRecreateSmoothFromDetail)(void);// -> compiled function
    PTRPARTS parts;			// access to offset,selector
  } pRecreateCode;

  unsigned short tileHeight;		// # scans in current tile

  unsigned char hzSwizzle;		// used for nerp'ing through VECTOR
  unsigned char filler;			// pad to DWORD

  unsigned long oBits;			// 32-bit offset of tile data

  unsigned long oDetail;		// 32-bit offset of detail list
  unsigned long oDetail_0Mod4;		// current 0 mod 4 detail ->
  unsigned long oDetail_1Mod4;		// current 1 mod 4 detail ->
  unsigned long oDetail_2Mod4;		// current 2 mod 4 detail ->
  unsigned long oDetail_3Mod4;		// current 3 mod 4 detail ->

  unsigned long oSmooth;		// 32-bit offset of smooth list
  unsigned long oSmooth_0Mod2;		// current 0 mod 2 smooth ->
  unsigned long oSmooth_1Mod2;		// current 1 mod 2 smooth ->
  unsigned long oSmoothUV;		// 32-bit offset of smooth U2 list

  unsigned long oInterU2Work;		// working 32-bit offset of U2
  unsigned long oInterV2Work;		// working 32-bit offset of V2

  unsigned long oWork;			// 32-bit offset of work buffer
  unsigned long oWS0Current;		// 32-bit offset of current workscan 0
  unsigned long oWS0Effective;		// 32-bit offset of effective workscan 0
  unsigned long oWS1Current;		// 32-bit offset of current workscan 1
  unsigned long oWS2Current;		// 32-bit offset of current workscan 2
  unsigned long oWS3Current;		// 32-bit offset of current workscan 3
  unsigned long oWS3Effective;		// 32-bit offset of effective workscan 3

  unsigned long srcWidth;		// actual width in pixels of input
  unsigned long cntHLoop;		// loop counter for horizontal filters

  unsigned long srcHeight;		// actual height in scans of input
  unsigned long cntVLoop;		// loop counter for vertical filter

  unsigned long firstTime;		// first time flag for code

  unsigned long lookUp8[256];		// 8 bit palettized lookup table

  unsigned char divBy7[256 * 7];	// the divide by 7 table

#ifdef	DEBUG			// for pointer range checking
  unsigned long bitsBase;		// linear base of input bits
  unsigned long bitsLimit;		// linear limit of input bits
  long bitsYStep;			// ystep in bits

  unsigned long privBase;		// linear base of private work areas
  unsigned long privLimit;		// linear limit of private work areas

  unsigned long detailBase;		// linear base of detail vector list
  unsigned long detailLimit;		// linear limit of detail vector list

  unsigned long smoothBase;		// linear base of smooth vector list
  unsigned long smoothLimit;		// linear limit of smooth vector list
#endif
} DIBTOYUVPRIVATE;

typedef struct tagP8LOOKUP {
  unsigned long palNumEntries;		// colors in lookup
  unsigned long filler0;		// unused
  unsigned long palPalEntry[1];		// the RGB entries
} P8LOOKUP;


extern void far baseFRAGTEXT32(void);	// something at the base of FRAGTEXT32

#define	FRAGDECLARE(procName)	extern FRAGINFO far procName

FRAGDECLARE(cEntry);

FRAGDECLARE(H1331Init);
FRAGDECLARE(H1331Start);
FRAGDECLARE(H1331FetchPixel8);
FRAGDECLARE(H1331FetchPixel15);
FRAGDECLARE(H1331FetchPixel16);
FRAGDECLARE(H1331FetchPixel24);
FRAGDECLARE(H1331FetchPixel32);
FRAGDECLARE(H1331ToYUV);
FRAGDECLARE(H1331StoreY0);
FRAGDECLARE(H1331StoreY1);
FRAGDECLARE(H431);
FRAGDECLARE(H1331B);
FRAGDECLARE(H1331C);
FRAGDECLARE(H1331Loop);
FRAGDECLARE(H1331End);

FRAGDECLARE(V1331EarlyOut);

FRAGDECLARE(V1331FetchY2Dest);
FRAGDECLARE(V1331FetchU2Dest);
FRAGDECLARE(V1331FetchV2Dest);
FRAGDECLARE(V1331Body);
#ifdef	DEBUG
FRAGDECLARE(V1331Body_Y2);
#endif
FRAGDECLARE(V1331IncY2Dest);
FRAGDECLARE(V1331IncU2V2Dest);
FRAGDECLARE(V1331Loop);
FRAGDECLARE(V1331StoreY2Dest);
FRAGDECLARE(V1331StoreU2Dest);
FRAGDECLARE(V1331StoreV2Dest);

FRAGDECLARE(HVLoop);

FRAGDECLARE(U2V2HInit);
FRAGDECLARE(U2V2HStart);
FRAGDECLARE(U2V2H431);
FRAGDECLARE(U2V2H1331B);
FRAGDECLARE(U2V2H1331C);
FRAGDECLARE(U2V2HLoop);
FRAGDECLARE(U2V2HEnd);

FRAGDECLARE(U2V2VBody);

FRAGDECLARE(cExit);

FRAGDECLARE(rY_H1331Init);
FRAGDECLARE(rY_H1331Start);
FRAGDECLARE(rY_H1331End);
FRAGDECLARE(rY_H431);
FRAGDECLARE(rY_H1331B);
FRAGDECLARE(rY_H1331C);
FRAGDECLARE(rY_V1331Body);

FRAGDECLARE(rUV_H1331Init);
FRAGDECLARE(rUV_H1331Start);
FRAGDECLARE(rUV_H1331End);
FRAGDECLARE(rUV_H431);
FRAGDECLARE(rUV_H1331B);
FRAGDECLARE(rUV_H1331C);
FRAGDECLARE(rUV_V1331Body);
