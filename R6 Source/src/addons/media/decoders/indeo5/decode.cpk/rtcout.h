/************************************************************************
*                                                                       *
*               INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                       *
*    This listing is supplied under the terms of a license agreement    *
*      with INTEL Corporation and may not be copied nor disclosed       *
*        except in accordance with the terms of that agreement.         *
*                                                                       *
*************************************************************************
*                                                                       *
*               Copyright (C) 1994-1997 Intel Corp.                     *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/
/*************************************************************************
 *                                                                       *
 *              INTEL CORPORATION PROPRIETARY INFORMATION                *
 *                                                                       *
 *      This software is supplied under the terms of a license           *
 *      agreement or nondisclosure agreement with Intel Corporation      *
 *      and may not be copied or disclosed except in accordance          *
 *      with the terms of that agreement.                                *
 *                                                                       *
 *************************************************************************/

/*************************************************************************
 * RTCC.H -- function declarations and definitions needed in decompress
 *              routines (decproc.c and decproci.c).  
 *************************************************************************/

#ifdef __RTCOUT_H__
#pragma message("rtcout.h: multiple inclusion")
#else /* __RTCOUT_H__ */
#define __RTCOUT_H__


/* Possible uCCFlags bits */
#define CCF_DOING_ALT_LINE		0x01
#define CCF_REFRESH_ALT_LINES	0x02
#define CCF_VU_DATA_IS_YVU12	0x04
#define CCF_XRGB16_1555			0x10
#define CCF_RGB16_565			0x20
#define CCF_RGB16_655			0x40
#define CCF_RGB16_664			0x80

/* constants */
#define ZOOM_NONE 2
#define ZOOM_X2   4

typedef struct {
	PIA_ENUM	nPaletteType;		  	/* PT_FIXED, PT_CONFIGURABLE, */
										/* or PT_ACTIVE */
	Boo			bPalette;			  	/* Is the palette valid? */
	Boo			bPalTablesBuilt;		/* Dynamic CLUT tables built? */
	Boo			bPaletteSwitch; 		/* First frame with a new palette? */
	Boo			bNeedsUpdate;			/* First frame of new cc mode */

	U32			uZoomX, uZoomY;			/* Amount to Zoom by */

	RectSt		rColorConvert;			/* Rectangle to color conver */
	RectSt		rViewRect;

	TRANSPARENCY_KIND	htkTransparencyKind;
	PU8			pLDMask;

	PTR_TRANSPARENCY_MASK  pExtTransMask;/* external mask */
	PPIA_Boolean	pbExtTransMaskUpdated;	/* Is the external mask updated or not? */

	Boo			bAltlineBlackFill;		/* true if need to clear alt */
										/* lines - reset to true when */
										/* begin is called */

} RTCOutInst, *pRTCOutInst;

typedef RTCOutInst PRIVATE_COLOROUT_DATA;
typedef PRIVATE_COLOROUT_DATA * PTR_PRIVATE_COLOROUT_DATA;
#define PRIVATE_COLOROUT_DATA_DEFINED


#define mac_abs(n) ((n) < 0 ? -(n) : (n))

#include "ccflags.h"

I32 C_YVU9toYUY2(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	U32 uWantsFill, U32 uFillValue, U32 uZoomX, U32 uZoomY, U32 uCCFlags);

I32 C_YVU9toUYVY(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	U32 uWantsFill, U32 uFillValue, U32 uZoomX, U32 uZoomY, U32 uCCFlags);

I32 C_YVU9toYV12(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	U32 uWantsFill, U32 uFillValue, U32 uZoomX, U32 uZoomY, U32 uCCFlags);

void YVU9_to_XRGB32(U32 uRows, U32 uCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	Boo bWantsFill, U32 uFillValue, COLOR_FORMAT eFormat
);

#endif /* __RTCOUT_H__ */
