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
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

/*
 *  Tabs set to 4
 *
 *  icconv.h
 *
 *  DESCRIPTION:
 *  ColorIn module include file.
 */

#ifndef __ICCONV_H__
#define __ICCONV_H__ 

typedef struct {
  /* The following data can be passed in/out through HIVE */
	PIA_Boolean  bBuildColorRange;		  /* Input: request to build range from color analysis
									   * typically only ture for the first frame
									   */
	PIA_Boolean  bBuildBitmask;			/* If true: build the bitmask from the color range
									 * otherwise, the bitmask is supplied by the user
									 */
	PIA_Boolean bAlphaChannel;      /* The bit mask is from alpha channel */

	COLOR_RANGE crTransColorRange;  /*  either user specifies the range or built from
									   * the first frame analysis
									   */
	PTR_TRANSPARENCY_MASK pTransparencyMask;   /* either given by the user or built by the ColorIn */
	BGR_ENTRY bgrRepresentativeColor;	/*   output from color analysis	
										 * Transparency analysis does not need this, but it
										 * will be passed to the encoder module
										 */
} PassDownColorInParmsSt, *pPassDownColorInParmsSt;

typedef PassDownColorInParmsSt * PTR_PASS_DOWN_COLORIN_PARMS;

/* Persistent context for input color conversion routines */

typedef struct {
	
	PIA_Boolean bFirstTime;
	U32 uPrevYAvg;
    PU8	pu8TempBuffer;		/* Temp Buffer to put full V/U data before up-sampling to YVU9/12 */
	U32 uMaxTempSize;

	PU8 pu8OutputData;   /* output buffer for color converted data */
} ColorInPrivateData, *PColorInPrivateData;

typedef ColorInPrivateData * PTR_PRIVATE_COLORIN_DATA;

#define PRIVATE_COLORIN_DATA_DEFINED

#endif
