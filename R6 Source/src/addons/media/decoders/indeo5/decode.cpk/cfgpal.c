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
 * This code was in part derived from 'gentab'.
 *
 * gentab -	generates palette tables for the IRV-type color converter
 * 			based on parameters for:
 *
 * 			Number of UV points
 * 			Number of Y points
 * 			Y step
 *
 * tabs set to 4
 * developed by Mike Keith and Stephen Wood, 6/95
 * modified by Tom Walsh, Summer / Fall 1995, Spring 1996
 *
 */

#ifdef DEBUG
static char cfgpal_rcsid[] = "@(#) $Id";
static char cfgpal_vcsid[] = "@(#) $Workfile:   cfgpal.c  $ $Revision:   1.1  $";
#endif /* DEBUG */

#include "datatype.h"
#include "pia_main.h"	/* pointer check */
#include "yrgb8.h"

typedef struct { int U; int V; } UVpoint;
typedef struct { int du, dv; } ChromaDitherVector;

static int FindPaletteStructure(BGR_ENTRY *pEntries, int iFirst, int iLast,
	UVpoint *puvpUVTable, int *piNumUV, int *piYStep, int *piYMin, int *piYMax);

static int ComputeChromaDitherVectorTable(ChromaDitherVector *cdv,
	int uvspacing, int num);

static void BuildChrominanceDitherTable(ChromaDitherVector *pCDV, int iNumVects,
	UVpoint *pUV, int iNumUVPoints, int iUStart, int iVStart, int iUEnd,
	int iVEnd, int iPalStart, int iPalNumY, unsigned long *pCDT);

static void BuildLumaTruncClampDitherTable(int iYMin, int iYMax, int iYStep,
	unsigned char *pYT);

#ifdef C_PORT_KIT
typedef struct t_palcfg {
	/* input values */
	I32 iSetPal, iFirst, iLast, iSetDither, iDither;
	int iNumVects;
	BGR_ENTRY rgbqPalette[256];
	MUTEX_HANDLE mConfigMutex;

	/* derived values */
	int iYMin, iYMax, iYStep, ChromaSpacing;
	UVpoint uvpPoints[256];
	int iNumUV;
	ChromaDitherVector cdv[16];

	/* 'output' - state */
	unsigned long *OutputChrominanceDitherTable;
	unsigned char *OutputLumaTruncClampDitherTable;

} g_LPC;
static g_LPC g_LastPaletteConfiguration;
#else /* C_PORT_KIT */
static struct t_palcfg {
	/* input values */
	I32 iSetPal, iFirst, iLast, iSetDither, iDither;
	int iNumVects;
	BGR_ENTRY rgbqPalette[256];
	MUTEX_HANDLE mConfigMutex;

	/* derived values */
	int iYMin, iYMax, iYStep, ChromaSpacing;
	UVpoint uvpPoints[256];
	int iNumUV;
	ChromaDitherVector cdv[16];

	/* 'output' - state */
	unsigned long *OutputChrominanceDitherTable;
	unsigned char *OutputLumaTruncClampDitherTable;

} g_LastPaletteConfiguration;
#endif /* C_PORT_KIT */


#ifdef C_PORT_KIT
static int SetupConfiguration(g_LPC *g_cfg, int iSetPal, int iFirst,
	int iLast, BGR_ENTRY *pPal, int iSetDither, int iDither);
#else /* C_PORT_KIT */
static int SetupConfiguration(struct t_palcfg *g_cfg, int iSetPal, int iFirst,
	int iLast, BGR_ENTRY *pPal, int iSetDither, int iDither);
#endif /* C_PORT_KIT */

static BGR_ENTRY TheDefaultPalette[256];

/*
 * Note: These should match between the compile time palette generator,
 * the authoring time palette optimizer, and the runtime palette analyzer.
 *
 * Formulas based on CCIR601 compliant calculations:
 * U, V range = [16..240]
 * Y range    = [16..235]
 * RGB range  = [ 0..255]
 *
 * Y = (0.257 * R) + (0.504 * G) + (0.098 * B) + 16.
 * U = (-0.148 * R) + (-0.291 * G) + (0.439 * B) + 128.
 * V = (0.439 * R) + (-0.368 * G) + (-0.071 * B) + 128.
 * R = (1.164 * (Y - 16.)) + (-0.001 * (U - 128.)) + (1.596 * (V - 128.))
 * G = (1.164 * (Y - 16.)) + (-0.391 * (U - 128.)) + (-0.813 * (V - 128.))
 * B = (1.164 * (Y - 16.)) + (2.017 * (U - 128.)) + (0.001 * (V - 128.))
 */
#define Yfrom(R,G,B) (int)(( 0.257*R)+( 0.504*G)+( 0.098*B)+16)
#define Ufrom(R,G,B) (int)((-0.148*R)+(-0.291*G)+( 0.439*B)+128)
#define Vfrom(R,G,B) (int)(( 0.439*R)+(-0.368*G)+(-0.071*B)+128)

/*
 * Expose the configuration parameters.
 */
void
ColorOutReleasePaletteConfiguration(void){
	HiveEndCriticalSection(g_LastPaletteConfiguration.mConfigMutex);
}
BGR_ENTRY const *
ColorOutGetPaletteConfiguration(I32 *iFirst, I32 *iLast){
	if(HiveBeginCriticalSection(g_LastPaletteConfiguration.mConfigMutex, 0) != PIA_S_OK)
		return NULL;
	*iFirst = g_LastPaletteConfiguration.iFirst;
	*iLast = g_LastPaletteConfiguration.iLast;
	return g_LastPaletteConfiguration.rgbqPalette;
}
/* Place correct dither vectors in vector table.
 * Return -1 on error, 0 on success
 * Makes a chroma dither vector table for non-ditherred output by setting
 * the first entry to zero.
 */
static int
ComputeChromaDitherVectorTable(ChromaDitherVector *cdv, int uvspacing, int num){

	int d[8];
	int i;

	if(!cdv) return -1;
	switch(num){case 1: case 4: case 8: case 16: break; default: return -1; break;}
	if(num == 1){
		cdv[0].du = cdv[0].dv = 0;
		return 0;
	}

	d[0] = uvspacing/num/2;
	d[1] = uvspacing/num+d[0];
	for(i = 2; i < num/2; i++) d[i] = uvspacing/num+d[i-1];

	switch(num){
		case 4:
			/* {  -D0,  -D0 },
			 * {  +D0,  +D0 },
			 * {  -D1,  +D1 },
			 * {  +D1,  -D1 },
			 */
			/*   0 1 2 3
			 *   2 3 0 1
			 *   1 0 3 2
			 *   3 2 1 0
			 */
			cdv[0].du = -1 * d[0]; cdv[0].dv = -1 * d[0];
			cdv[1].du = +1 * d[0]; cdv[1].dv = +1 * d[0];
			cdv[2].du = -1 * d[1]; cdv[2].dv = +1 * d[1];
			cdv[3].du = +1 * d[1]; cdv[3].dv = -1 * d[1];
			break;
		case 8:
			/* { -D3, +D3 },
			 * { -D2, -D2 },
			 * { -D1, +D0 },
			 * { +D2, +D2 },
			 * { -D0, +D1 },
			 * { +D1, -D0 },
			 * { +D0, -D1 },
			 * { +D3, -D3 },
			 */
			/*   0 4 1 5
			 *   6 2 7 3
			 *   1 5 0 4
			 *   7 3 6 2
			 */
			cdv[0].du = -1 * d[3]; cdv[0].dv = +1 * d[3];
			cdv[1].du = -1 * d[2]; cdv[1].dv = -1 * d[2];
			cdv[2].du = -1 * d[1]; cdv[2].dv = +1 * d[0];
			cdv[3].du = +1 * d[2]; cdv[3].dv = +1 * d[2];
			cdv[4].du = -1 * d[0]; cdv[4].dv = +1 * d[1];
			cdv[5].du = +1 * d[1]; cdv[5].dv = -1 * d[0];
			cdv[6].du = +1 * d[0]; cdv[6].dv = -1 * d[1];
			cdv[7].du = +1 * d[3]; cdv[7].dv = -1 * d[3];
			/* it is possible to make 0/1, 2/3, 4/5, 6/7 near
			 * to eachother, but make 0/1 and 2/3 far from each
			 * other, and likewise with 4/5 and 6/7.
			 * Here is a first pass at these groupings.  I was
			 * able to get groups with members about 9 apart,
			 * and non-members about 18 apart.  The 6/7 group
			 * actualy isn't a group, but is rather two vectors
			 * each of which is about 18 away from the other
			 * groups / members.
			 * This succeeded in reducing the strong diagonal
			 * cross hatching, but in it's place substituted a
			 * brick like pattern of offset rectangles.
			 * { +D0, +D1 },
			 * { +D1, +D2 },
			 * { -D2, -D1 },
			 * { -D3, -D2 },
			 * { +D3, +D0 },
			 * { +D2, -D0 },
			 * { -D0, -D3 },
			 * { -D1, +D3 },
			 */
			break;
		case 16:
			/*
			 * { -D0, +D0 },
			 * { -D1, +D2 },
			 * { -D7, +D1 },
			 * { +D7, -D6 },
			 * { +D6, +D5 },
			 * { +D5, -D0 },
			 * { +D0, -D7 },
			 * { -D5, -D5 },
			 * { +D1, +D7 },
			 * { +D2, -D4 },
			 * { -D4, -D1 },
			 * { -D2, +D6 },
			 * { -D3, +D3 },
			 * { -D6, +D4 },
			 * { +D3, -D2 },
			 * { +D4, -D3 },
			 */
			/*   0 A 2 8
			 *   E 4 C 6
			 *   3 9 1 B
			 *   D 7 F 5
			 */
			cdv[ 0].du = -1 * d[0]; cdv[ 0].dv = +1 * d[0];
			cdv[ 1].du = -1 * d[1]; cdv[ 1].dv = +1 * d[2];
			cdv[ 2].du = -1 * d[7]; cdv[ 2].dv = +1 * d[1];
			cdv[ 3].du = +1 * d[7]; cdv[ 3].dv = -1 * d[6];
			cdv[ 4].du = +1 * d[6]; cdv[ 4].dv = +1 * d[5];
			cdv[ 5].du = +1 * d[5]; cdv[ 5].dv = -1 * d[0];
			cdv[ 6].du = +1 * d[0]; cdv[ 6].dv = -1 * d[7];
			cdv[ 7].du = -1 * d[5]; cdv[ 7].dv = -1 * d[5];
			cdv[ 8].du = +1 * d[1]; cdv[ 8].dv = +1 * d[7];
			cdv[ 9].du = +1 * d[2]; cdv[ 9].dv = -1 * d[4];
			cdv[10].du = -1 * d[4]; cdv[10].dv = -1 * d[1];
			cdv[11].du = -1 * d[2]; cdv[11].dv = +1 * d[6];
			cdv[12].du = -1 * d[3]; cdv[12].dv = +1 * d[3];
			cdv[13].du = -1 * d[6]; cdv[13].dv = +1 * d[4];
			cdv[14].du = +1 * d[3]; cdv[14].dv = -1 * d[2];
			cdv[15].du = +1 * d[4]; cdv[15].dv = -1 * d[3];
			break;
	}
	return 0;
}
/* Given a palette, with iFirst through iLast inclusive as the area of
 * interest, determine the U/V plane locations included in the palette,
 * the y step size, and the y min and max values.
 * Store the found UV points in the table pointed to, and return the number
 * of entries stored.
 *
 * The palette is expected to have IVI structure:  All luma levels for a given
 * chroma point are grouped together.  They are in ascending order.  There are
 * no repeated luma levels (delta is always one or more).  The chroma points
 * should be, for best quality, sorted in order of increasing saturation.  Each
 * chroma point has the same number of luma levels, and in the middle of the
 * 'ramp' of luma, these levels are the same (near the ends, clamping prevents
 * this since the color spaces are not commensurate.)
 *
 * For compatibility with future upgrades, points not in the central 128x128
 * region of the plane are recorded as provided.  In some circumstances they
 * may be used.  Possible upgrades in which the UV range is expanded may be
 * able to use them more effectively.
 *
 * Return value will be the discoverred UV grid spacing.  When there is only
 * one chroma point present, the return value will be 0, which will allow the
 * caller to populate the chroma dither table with the single chroma point.
 * Normally, this will be the minimum delta U or delta V, and will set the
 * chroma dither vector length.  The assumption is that the grid is 'square',
 * and the U and V elements of the dither vectors are not adjusted to account
 * for a non-square rectangular grid.
 *
 * Errors are indicated by returning -1.
 *
 * Output of the uv point array assumes there to be enough space allocated by
 * the caller.  Note that this could be quite large for the general version.
 *
 * Possible reasonable restriction, but not enforced here due to insufficient
 * information about whether there are useful palettes which violate this...
 *	if(NumLumaLevels < 8) return -1;
 *
 */
static int
FindPaletteStructure(BGR_ENTRY *pEntries, int iFirst, int iLast,
		UVpoint *puvpUVTable, int *piNumUV,
		int *piYStep, int *piYMin, int *piYMax){
	int i, luma[256], FirstYMax, MinChromaSpacing;
	int NumChromaPoints, NumLumaLevels, NumPaletteEntries;
	int MaxLuma, MinLuma, CenterLuma, LumaStepSize, PointOfInterest;

	if(!pEntries||!puvpUVTable||!piYStep||!piYMin||!piYMax) return -1;

	for(i = iFirst; i <= iLast; i++) /* could find one ramp if need be */
		luma[i] = Yfrom(pEntries[i].u8R, pEntries[i].u8G,
				    pEntries[i].u8B);
	for(FirstYMax = iFirst+1; FirstYMax <= iLast; FirstYMax++)
		if(luma[FirstYMax] <= luma[FirstYMax-1]) break;
	FirstYMax--;	/* index of first entry with max Y */
	NumPaletteEntries = iLast - iFirst + 1;
	NumLumaLevels = FirstYMax - iFirst + 1;
	NumChromaPoints = NumPaletteEntries / NumLumaLevels;

	if(NumChromaPoints * NumLumaLevels != NumPaletteEntries) return -1;

	if(NumLumaLevels > 4){
		for(i=0,CenterLuma=0,LumaStepSize=0; i < NumChromaPoints; i++){
			PointOfInterest=i*NumLumaLevels+iFirst+NumLumaLevels/2;
			CenterLuma += luma[PointOfInterest];
			LumaStepSize+=luma[PointOfInterest+1]-
				      luma[PointOfInterest];
		}
		CenterLuma = (CenterLuma+NumChromaPoints-1)/NumChromaPoints;
		LumaStepSize = (LumaStepSize+NumChromaPoints-1)/NumChromaPoints;
		MinLuma = CenterLuma - (NumLumaLevels/2) * LumaStepSize;
		MaxLuma = (LumaStepSize*(NumLumaLevels-1)) + MinLuma;

	}
	else{
		for(i=0, MinLuma=256, MaxLuma=0; i < NumChromaPoints; i++){
			PointOfInterest = i*NumLumaLevels+iFirst;
			MinLuma = MIN(MinLuma,luma[PointOfInterest]);
			if(i)MaxLuma = MAX(MaxLuma,luma[PointOfInterest-1]);
		}
		PointOfInterest = i*NumLumaLevels+iFirst;
		MaxLuma = MAX(MaxLuma, luma[PointOfInterest-1]);
		LumaStepSize=(MaxLuma-MinLuma+NumLumaLevels-1)/NumLumaLevels;
	}
	for(i = 0; i < NumChromaPoints; i++){
		PointOfInterest = i*NumLumaLevels+NumLumaLevels/2+iFirst;
		puvpUVTable[i].U = Ufrom(pEntries[PointOfInterest].u8R,
					pEntries[PointOfInterest].u8G,
					pEntries[PointOfInterest].u8B);
		puvpUVTable[i].V = Vfrom(pEntries[PointOfInterest].u8R,
					pEntries[PointOfInterest].u8G,
					pEntries[PointOfInterest].u8B);
		puvpUVTable[i].U += 2; puvpUVTable[i].U &= ~3;/* mod 4 */
		puvpUVTable[i].V += 2; puvpUVTable[i].V &= ~3;
	}
	*piYStep = LumaStepSize; *piYMin = MinLuma; *piYMax = MaxLuma;
	*piNumUV = NumChromaPoints;
	if(NumChromaPoints == 1) return 0;
	for(i = 1, MinChromaSpacing = 257; i < NumChromaPoints; i++){
		int DeltaU, DeltaV;

		DeltaU = ABS(puvpUVTable[i].U-puvpUVTable[i-1].U);
		if(DeltaU) MinChromaSpacing = MIN(MinChromaSpacing, DeltaU);
		DeltaV = ABS(puvpUVTable[i].V-puvpUVTable[i-1].V);
		if(DeltaV) MinChromaSpacing = MIN(MinChromaSpacing, DeltaV);
	}
	if(MinChromaSpacing == 257) return -1;	/* >1 ramp w/same point */
	if(MinChromaSpacing) return MinChromaSpacing;
	return -1;
}

/* Given the table of dither vectors, the number of dither vectors to use, and
 * the range of values within the chroma plane to consider, produce the chroma
 * dither table.  It is assumed that the storage pointed to is adequate to hold
 * the dither table.  This table increases in size as the truncation and clamp
 * of chrominance is modified to cover more of the plane.
 * The number of luma steps and start of area of interest in the palette is
 * needed in order to load the dither table with the correct palette indices.
 * For the first version, this should be called with a pointer to 16K of space:
 * with iUStart = iVStart = 64, iUEnd = iVEnd = 192, iNumVects = {1,4,8,16}.
 * The plane is examined in the area of interest at a granularity of four in
 * U and V.  Thus: 192-64=128. 128/4=32.  32 U * 32 V * 16 elements = 16384.
 *
 * First version eliminates randomized chroma match point search.  This can
 * be added by changing Order[].  Order should then provide a saturation sorted
 * version of the chroma points in the palette.  Segmentation by saturation is
 * then needed.  This segmentation is encapsulated in Order[].
 *
 * Non-ditherred output through the dither table is available by calling with
 * the number of dither vectors set to one.  If the chroma dither vector table
 * is set with du and dv of first entry to zero, we look for nearest matches.
 *
 * Note that the dither table entry subscripted by U and V as noted above is
 * actually a 16 byte entry (for YVU9) expressing the 16 chroma selections,
 * of which at most 'iNumVects' are unique.  Each of these selections is the
 * first palette index for the chosen chrominance.  The luminance is used to
 * add an offset to this, choosing which of the luma ramp entries should be
 * used.  In other color formats, there would be smaller entries, in particular
 * for a 2x2 subsampled chroma space, we would have 4 byte entries, and as
 * a result we could cover more of the UV plane with the same size table.
 *
 * For efficiency reasons we view the dither table as unsigned long, which
 * allows us to express write coalescing in C.
 *
 */

/*#include <stdlib.h>*/	/* for [s]rand() */
#define getrand(min,max) ((rand()%(int)(((max)+1)-(min)))+(min))
static int Order[256];
static void newinitrandsearch(){int i;/*srand(1)*/;for(i=0;i<256;i++)Order[i]=i;}

static void
BuildChrominanceDitherTable(ChromaDitherVector *pCDV, int iNumVects,
			UVpoint *pUV, int iNumUVPoints,
			int iUStart, int iVStart,
			int iUEnd, int iVEnd,
			int iPalStart, int iPalNumY,
			unsigned long *pCDT){

	int ThisU, ThisV;

	newinitrandsearch();

	for(ThisU = iUStart; ThisU < iUEnd; ThisU+=4){
		for(ThisV = iVStart; ThisV < iVEnd; ThisV+=4){
			int ThisVect, Matches[16];
			unsigned long *p;

			for(ThisVect = 0; ThisVect < iNumVects; ThisVect++){
				int Uprime, Vprime, MinDisp, ThisPt, MinPt;

				Uprime = ThisU + pCDV[ThisVect].du;
				Vprime = ThisV + pCDV[ThisVect].dv;
				Uprime = MAX(0, MIN(255, Uprime));
				Vprime = MAX(0, MIN(255, Vprime));
				MinDisp = 255 * 255 + 1;
				MinPt = 0;
				for(ThisPt = 0; ThisPt < iNumUVPoints; ThisPt++){
					int Udisp, Vdisp, ThisDisp;

					Udisp = Uprime - pUV[Order[ThisPt]].U;
					Vdisp = Vprime - pUV[Order[ThisPt]].V;
					ThisDisp = Udisp*Udisp+Vdisp*Vdisp;
					if(ThisDisp < MinDisp){
						MinDisp = ThisDisp;
						MinPt = ThisPt;
					}
				}
				Matches[ThisVect] = MinPt*iPalNumY+iPalStart;
			}
			p = pCDT+
				((ThisU-iUStart)/4)*((iVEnd-iVStart)/4)*(16/sizeof(*pCDT))+
				((ThisV-iVStart)/4)*(16/sizeof(*pCDT));
			switch(iNumVects){
				case 1: {
					int OutRow;
					OutRow = Matches[0];
					OutRow |= OutRow<<8;
					OutRow |= OutRow<<16;
					*p++ = OutRow;
					*p++ = OutRow;
					*p++ = OutRow;
					*p++ = OutRow;
					} break;
				case 4:
  *p++ = Matches[0]<<24|Matches[1]<<16|Matches[2]<<8|Matches[3];
  *p++ = Matches[2]<<24|Matches[3]<<16|Matches[0]<<8|Matches[1];
  *p++ = Matches[1]<<24|Matches[0]<<16|Matches[3]<<8|Matches[2];
  *p++ = Matches[3]<<24|Matches[2]<<16|Matches[1]<<8|Matches[0];
  					break;
				case 8:
  *p++ = Matches[0]<<24|Matches[4]<<16|Matches[1]<<8|Matches[5];
  *p++ = Matches[6]<<24|Matches[2]<<16|Matches[7]<<8|Matches[3];
  *p++ = Matches[1]<<24|Matches[5]<<16|Matches[0]<<8|Matches[4];
  *p++ = Matches[7]<<24|Matches[3]<<16|Matches[6]<<8|Matches[2];
  					break;
				case 16:
  *p++ = Matches[ 0]<<24|Matches[10]<<16|Matches[ 2]<<8|Matches[ 8];
  *p++ = Matches[14]<<24|Matches[ 4]<<16|Matches[12]<<8|Matches[ 6];
  *p++ = Matches[ 3]<<24|Matches[ 9]<<16|Matches[ 1]<<8|Matches[11];
  *p++ = Matches[13]<<24|Matches[ 7]<<16|Matches[15]<<8|Matches[ 5];
  					break;
			}
		}
	}
}
/* Build table at passed address, using the luma range and precision
 * information passed in.  The table accomplishes reduction of precision,
 * clamping, and dithering of luma using one table lookup.
 * *(pYT + 0) to *(pYT + 255) will be initialized.
 */
static void
BuildLumaTruncClampDitherTable(int iYMin, int iYMax, int iYStep,
		unsigned char *pYT){
	int Y, divisor;

	for(Y = 0; Y < (iYMin-1); Y++){
		*pYT++ = 0;
		*pYT++ = 0;
		*pYT++ = 0;
		*pYT++ = 0;
	}
	if(iYStep == 0)
		divisor = 1;
	else
		divisor = iYStep;
	for(Y = (iYMin-1); Y < iYMax; Y++){
		*pYT++ = ((Y+1)-iYMin)/divisor;
		*pYT++ = ((Y+1)-iYMin+(iYStep*1/4))/divisor;
		*pYT++ = ((Y+1)-iYMin+(iYStep*2/4))/divisor;
		*pYT++ = ((Y+1)-iYMin+(iYStep*3/4))/divisor;
	}

	if(iYStep < 2)
		divisor = 1;
	else
		divisor = iYStep - 1;

	for(Y = iYMax; Y < 256; Y++){
		*pYT++ = (iYMax-iYMin)/divisor;
		*pYT++ = (iYMax-iYMin)/divisor;
		*pYT++ = (iYMax-iYMin)/divisor;
		*pYT++ = (iYMax-iYMin)/divisor;
	}
}

void
DeInitPaletteConfiguration(){
	if(HiveBeginCriticalSection(g_LastPaletteConfiguration.mConfigMutex,0)){
		g_LastPaletteConfiguration.iSetPal = 0;
		g_LastPaletteConfiguration.iSetDither = 0;
		/*HiveEndCriticalSection(g_LastPaletteConfiguration.mConfigMutex);*/
		HiveFreeMutex(g_LastPaletteConfiguration.mConfigMutex);
	}
}

void
InitPaletteConfiguration(){
	int i;
	extern int CP_EntryCount;
	extern unsigned char CP_PalTable[];
	extern unsigned long CP_DitherTable[];
	extern unsigned char CP_ICDY[];

	g_LastPaletteConfiguration.iSetPal = 0;
	g_LastPaletteConfiguration.iSetDither = 0;
	g_LastPaletteConfiguration.OutputChrominanceDitherTable = CP_DitherTable;
	g_LastPaletteConfiguration.OutputLumaTruncClampDitherTable = CP_ICDY;
	for(i = 10; i < 10+CP_EntryCount; i++){
		TheDefaultPalette[i].u8B = CP_PalTable[i*4+2-10*4];
		TheDefaultPalette[i].u8G = CP_PalTable[i*4+1-10*4];
		TheDefaultPalette[i].u8R = CP_PalTable[i*4+0-10*4];
		TheDefaultPalette[i].u8Reserved = CP_PalTable[i*4+3-10*4];
	}
	g_LastPaletteConfiguration.iFirst = 10;
	g_LastPaletteConfiguration.iLast = 10 + CP_EntryCount - 1;
	for(i = 10; i < 10+CP_EntryCount; i++)
		g_LastPaletteConfiguration.rgbqPalette[i] = TheDefaultPalette[i];
	g_LastPaletteConfiguration.mConfigMutex = HiveCreateMutex((PU8)"Truth");
} /* InitPaletteConfiguration */

/* Set up the palette configuration information based on request.
 * Return -1 on error, 0 on no error.  A request for palette change
 * with a null pointer to the palette entries will be interpreted as
 * a request to reset the palette configuration to the default values.
 * A dither setting request will be honored at initial use.
 */
static int
#ifdef C_PORT_KIT
SetupConfiguration(g_LPC *g_cfg,
			int iSetPal, int iFirst, int iLast, BGR_ENTRY *pPal,
			int iSetDither, int iDither)
#else /* C_PORT_KIT */
SetupConfiguration(struct t_palcfg *g_cfg,
			int iSetPal, int iFirst, int iLast, BGR_ENTRY *pPal,
			int iSetDither, int iDither)
#endif /* C_PORT_KIT */
{
	extern int CP_EntryCount;
	int i;

	if( ((iSetPal)&&(pPal == NULL)) ||						/* reset request */
		((!iSetPal)&&(!g_LastPaletteConfiguration.iSetPal))	/* or never set */
	  ){
		pPal = TheDefaultPalette;
		iFirst = 10;
		iLast = 10 + CP_EntryCount - 1;
		iSetPal = 1;
		if(!iSetDither){
			iSetDither = 1;
			iDither = 2;
		}
	}
	if(iSetPal){
		g_LastPaletteConfiguration.iSetPal = 1;
		g_LastPaletteConfiguration.iFirst = iFirst;
		g_LastPaletteConfiguration.iLast = iLast;
		for(i = iFirst; i <= iLast; i++)
			g_LastPaletteConfiguration.rgbqPalette[i] = *(pPal+i);
	}
	if(iSetDither){
		g_LastPaletteConfiguration.iSetDither = 1;
		g_LastPaletteConfiguration.iDither = iDither;
	}
	if(!g_LastPaletteConfiguration.iSetDither){
		g_LastPaletteConfiguration.iSetDither = 1;
		g_LastPaletteConfiguration.iDither = 2;
	}
	switch(g_LastPaletteConfiguration.iDither){
		case 0: g_LastPaletteConfiguration.iNumVects = 1; break;
		case 1: g_LastPaletteConfiguration.iNumVects = 4; break;
		case 2: g_LastPaletteConfiguration.iNumVects = 8; break;
		case 3: g_LastPaletteConfiguration.iNumVects = 16; break;
		default: return -1;
	}
	return 0;
}


/* Externally visible PIA function will, given a palette as a set of U32s
 * with 0xXRGB as order of color elements and iFirst through iLast inclusive
 * the entries of interest, and the indication iDither to select the degree
 * of chrominance dithering, generate luma clamp dither table, chroma dither
 * table, and optionally a palette (as a cross check of our computations.)
 *
 * The luma dither, clamp, and precision reduction is a function of:
 *		luma step size, minimum luma, maximum luma
 * The paletet is a function of:
 *		luma step size, minimum luma, maximum luma, chroma points.
 * The chroma dither table is a function of:
 *		chroma step size, chroma points.
 *
 * The chroma truncation and clamping arrays are invariant, a function of
 * the chroma precision and range selected.  The size of the chroma dither
 * table would need to be changed to match any change to these tables.
 *
 * Steps, loosely:
 *	o SetupConfiguration()
 *	o FindPaletteStructure()
 *	o ComputeChromaDitherVectorTable()
 *	o BuildChrominanceDitherTable()
 *	o BuildLumaTruncClampDitherTable()
 *
 * Input -
 *	iSetPal - indicates whether this call conveys information
 *		about a new palette, non-zero indicates that it does.
 * 	iFirst, iLast, and pPal - describe palette.  *(pPal+iLast) is last
 *		entry of interest.  if pPal is NULL while iSetPal is nonzero,
 *		the function will reset to defaults.
 *	iSetDither - indicates whether this call conveys a request to set
 *		the degree of dither.  non-zero indicates that it does.
 *	iDither - describes degree of dither requested.
 *
 * Return PIA_S_OK if no errors are found.  Return PIA_S_ERROR for any error.
 *
 */
PIA_RETURN_STATUS
ColorOutSetPaletteConfiguration(I32 iSetPal, I32 iFirst, I32 iLast,
		PTR_BGR_ENTRY pPal, I32 iSetDither, I32 iDither){

	if(iSetPal){
		if(iFirst > iLast) return PIA_S_ERROR;
		if(iFirst < 0) return PIA_S_ERROR;
		if(iLast > 255) return PIA_S_ERROR;
		if(pPal)
			if(HiveGlobalPtrCheck(pPal,(iLast+1)*sizeof(BGR_ENTRY))!=PIA_S_OK)
				return PIA_S_ERROR;
	}
	if(iSetDither){
		switch(iDither){
			case 0:
			case 1:
			case 2:
			case 3:
				break;
			default:
				return PIA_S_ERROR;
				break;
		}
	}

	if(HiveBeginCriticalSection(g_LastPaletteConfiguration.mConfigMutex, 0) != PIA_S_OK)
		return PIA_S_ERROR;

	if(SetupConfiguration(&g_LastPaletteConfiguration,
			iSetPal, iFirst, iLast, pPal, iSetDither, iDither)){
		HiveEndCriticalSection(g_LastPaletteConfiguration.mConfigMutex);
		return PIA_S_ERROR;
	}
	g_LastPaletteConfiguration.ChromaSpacing =
		FindPaletteStructure(g_LastPaletteConfiguration.rgbqPalette,
			g_LastPaletteConfiguration.iFirst,
			g_LastPaletteConfiguration.iLast,
			g_LastPaletteConfiguration.uvpPoints,
			&g_LastPaletteConfiguration.iNumUV,
			&g_LastPaletteConfiguration.iYStep,
			&g_LastPaletteConfiguration.iYMin,
			&g_LastPaletteConfiguration.iYMax);

	if(g_LastPaletteConfiguration.ChromaSpacing == -1){
		HiveEndCriticalSection(g_LastPaletteConfiguration.mConfigMutex);
		return PIA_S_ERROR;
	}

	/*if(ChromaSpacing == 0)	OPTIMIZATION FOR "GREYSCALE??" */
	/*	... */

	if(ComputeChromaDitherVectorTable(g_LastPaletteConfiguration.cdv,
			g_LastPaletteConfiguration.ChromaSpacing,
			g_LastPaletteConfiguration.iNumVects) == -1){
		HiveEndCriticalSection(g_LastPaletteConfiguration.mConfigMutex);
		return PIA_S_ERROR;
	}

	{
		int numluma;

		if(g_LastPaletteConfiguration.iYStep)
			numluma = ((g_LastPaletteConfiguration.iYMax-
				    g_LastPaletteConfiguration.iYMin)/
				   g_LastPaletteConfiguration.iYStep)+1;
		else
			numluma = 1;

		BuildChrominanceDitherTable(g_LastPaletteConfiguration.cdv,
		g_LastPaletteConfiguration.iNumVects,
		g_LastPaletteConfiguration.uvpPoints,
		g_LastPaletteConfiguration.iNumUV,
		64, 64, 192, 192,
		g_LastPaletteConfiguration.iFirst,
		numluma,
		g_LastPaletteConfiguration.OutputChrominanceDitherTable);
	}
	BuildLumaTruncClampDitherTable(g_LastPaletteConfiguration.iYMin,
		g_LastPaletteConfiguration.iYMax,
		g_LastPaletteConfiguration.iYStep,
		g_LastPaletteConfiguration.OutputLumaTruncClampDitherTable);


	HiveEndCriticalSection(g_LastPaletteConfiguration.mConfigMutex);

	return PIA_S_OK;
}


#if 0
#define getrand(min,max) ((rand()%(int)(((max)+1)-(min)))+(min))
static int searchorder[sizeof(UVpt)/sizeof(UVpt[0])];
static int checksearch[sizeof(UVpt)/sizeof(UVpt[0])];
static void initrandsearch(){ srand(1); }

/*
 * Improve this.  It currently has hard coded (in effect) interesting groups,
 * and will fail to search what it thinks is uninteresting groups...
 *
 * perhaps:
 *	if(max > currentgrouptop)
 *		for(i = min; i < min(max,currentgrouptop); i++)
 */
static void
setrandsearch(maxuvpt){
	int i;
	int trials;

	for(i = 0; i <= maxuvpt; i++){ checksearch[i] = 0; }

	searchorder[0] = 0;	/* search grey first */
	if(maxuvpt >= 4){
		for(i = 1; i <= 4; i++){
			searchorder[i] = getrand(1, 4);
			trials = 0;
			while(checksearch[searchorder[i]] == 1){
				searchorder[i] = getrand(1, 4);
				trials++;
				if(trials > 10000000){
					fprintf(stderr, "Bad Search: 1-4\n");
					exit(-1);
				}
			}
			checksearch[searchorder[i]] = 1;
		}
	}

	if(maxuvpt >= 8){
		for(i = 5; i <= 8; i++){
			searchorder[i] = getrand(5, 8);
			trials = 0;
			while(checksearch[searchorder[i]] == 1){
				searchorder[i] = getrand(5, 8);
				trials++;
				if(trials > 10000000){
					fprintf(stderr, "Bad Search: 5-8\n");
					exit(-1);
				}
			}
			checksearch[searchorder[i]] = 1;
		}
	}

	if(maxuvpt == 11){	/* particular to red coverage.. */
		for(i = 9; i <= 10; i++){
			searchorder[i] = getrand(9, 10);
			trials = 0;
			while(checksearch[searchorder[i]] == 1){
				searchorder[i] = getrand(9, 10);
				trials++;
				if(trials > 10000000){
					fprintf(stderr, "Bad Search: 9-10\n");
					exit(-1);
				}
			}
			checksearch[searchorder[i]] = 1;
		}
	}

	if(maxuvpt >= 12){
		for(i = 9; i <= 12; i++){
			searchorder[i] = getrand(9, 12);
			trials = 0;
			while(checksearch[searchorder[i]] == 1){
				searchorder[i] = getrand(9, 12);
				trials++;
				if(trials > 10000000){
					fprintf(stderr, "Bad Search: 11-12\n");
					exit(-1);
				}
			}
			checksearch[searchorder[i]] = 1;
		}
	}

	if(maxuvpt > 13){
		for(i = 13; i <= maxuvpt; i++){
			searchorder[i] = getrand(13, maxuvpt);
			trials = 0;
			while(checksearch[searchorder[i]] == 1){
				searchorder[i] = getrand(13, maxuvpt);
				trials++;
				if(trials > 10000000){
					fprintf(stderr, "Bad Search: 13-?\n");
					exit(-1);
				}
			}
			checksearch[searchorder[i]] = 1;
		}
	}
}
#endif
#if 0	/* Snippet showing use of randomized visit order */
	for (U_trunc = 0; U_trunc < 32; ++U_trunc) {
		for (V_trunc = 0; V_trunc < 32; ++V_trunc) {

			int UVx[MAX_DITHER_VECTS], LastPointConsidered[MAX_DITHER_VECTS];
 			int position;

			setrandsearch(number_of_UV - 1);

			for (position = 0; position < num_dither_vectors; ++position) {
				int UV_dist, this_dist;
				int U_search, V_search;
				ChromaDitherVector *cdv = g_LastPaletteConfiguration.cdv;

				U_search = UOFF+(U_trunc<<2)+cdv[position].du;
				U_search = CLAMP(U_search);

				V_search = VOFF+(V_trunc<<2)+cdv[position].dv;
				V_search = CLAMP(V_search);

				UV_dist = MILLION; /* max = 2*128*128 */
   	
				/* find the closest point in the UV selections */
				for (i = 0; i < number_of_UV; ++i) {
					this_dist = (U_search - UVpt[searchorder[i]].U) *
								(U_search - UVpt[searchorder[i]].U) +
								(V_search - UVpt[searchorder[i]].V) *
								(V_search - UVpt[searchorder[i]].V);
					if (this_dist < UV_dist) {
						UV_dist = this_dist;
						UVx[position] = searchorder[i];
					}
				} /* for i... */
			} /* for position... */

			/* adjust to get past reserved colors and account for Y */
			for(position = 0; position < num_dither_vectors; position++){
				LastPointConsidered[position] = UVx[position];
				UVx[position] = 10 + (UVx[position] * number_of_Y);
			}
		}
	}
#endif
