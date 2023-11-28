/*********************************************************************/
/*
	Contains:	This module contains code to calculate colorant
				values and set colorant and TRC curve tags

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1997 by Eastman Kodak Company

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile $
		$Logfile $
		$Revision $
		$Date $
		$Author $

	SCCS Revision:
		@(#)spcrctag.c	1.4 12/4/97

	To Do:
*/
/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** PROPRIETARY NOTICE:     The  software  information   contained ***
 *** herein is the  sole property of  Eastman Kodak Company  and is ***
 *** provided to Eastman Kodak users under license for use on their ***
 *** designated  equipment  only.  Reproduction of  this matter  in ***
 *** whole  or in part  is forbidden  without the  express  written ***
 *** consent of Eastman Kodak Company.                              ***
 ***                                                                ***
 *** COPYRIGHT (c) Eastman Kodak Company, 1997                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/
#if defined (KPMAC)
#include <fp.h>
#else
#include <math.h>
#endif

#include "sprof-pr.h"


#define	X_D50		         0.9642		/* standard D50 white point */
#define	Y_D50		         1.0000
#define	Z_D50		         0.8249

typedef struct {
	double	x, y, z;
} xyz_t;

static KpInt32_t xyz2cone (SpMatrix_p src, SpMatrix_p dest);
static KpInt32_t cone2xyz (SpMatrix_p src, SpMatrix_p dest);

static KpBool_t AdaptXYZ( xyz_t *rXYZ, xyz_t *gXYZ, xyz_t *bXYZ,
                          xyz_t *adaptRed, xyz_t *adaptGreen, xyz_t *adaptBlue);

/*----------------------------------------------------------------------*/
static void Set_xy (
				KpF15d16_t		x,
				KpF15d16_t		y, 
				xyz_t			*Prim)
{
	Prim->x = KpF15d16ToDouble(x);
	Prim->y = KpF15d16ToDouble(y);
	Prim->z = 1.0 - Prim->x - Prim->y;
}

/* ---------------------------------------------------------------------- */
static SpStatus_t SetXYZ (
					SpProfile_t	Profile,
					SpTagId_t	TagId,
					double		X,
					double		Y,
					double		Z)
{
	SpTagValue_t	Tag;
	SpStatus_t		Status;

	Tag.TagId = TagId;
	Tag.TagType = Sp_AT_XYZ;
	Tag.Data.XYZ.X = KpF15d16FromDouble (X);
	Tag.Data.XYZ.Y = KpF15d16FromDouble (Y);
	Tag.Data.XYZ.Z = KpF15d16FromDouble (Z);
	
	Status = SpTagSet (Profile, &Tag);
	
	return Status;
}

/* ---------------------------------------------------------------------- */
static SpStatus_t SetResponse (SpProfile_t	Profile, 
							   SpTagId_t	TagId,
							   KpF15d16_t	Gamma)
{
	SpTagValue_t	Tag;
	double			TempData [5];
	double			sum, p, p2, p3;
	KpUInt16_t		CurveData[256];
	double			ScaleFactor, ScaledVal, TempVal;
	SpStatus_t		Status;
	KpInt32_t		i, j, k, m, index1, index2;

	KpInt16_t		n = 5;  /* number of temporary points calculated */


	/** set up the tag  **/
	Tag.TagId = TagId;
	Tag.TagType = Sp_AT_Curve;
	Tag.Data.Curve.Count = 256;
	Tag.Data.Curve.Data = CurveData;
	
	/** calculate five points on the curve  **/
	TempData [0] = 0.0;
	
	/* value at 1/4 of range */
	TempVal = KpF15d16ToDouble(Gamma);
	TempData [1] = MakeGamma(TempVal, .25); 

	/* value at 1/2 of range */
	TempData [2] = MakeGamma(TempVal, .50);

	/* value at 3/4 of range */
	TempData [3] = exp(log(TempData [2]) * log(0.75)/log(0.50));
	
	TempData [4] = 1.0;

	/** interpolate up to 256 points **/
	ScaleFactor = (n-1) / 255.0;

	/* loop over the curve data array */
	for (i = 0; i < 256; i++) {
		
		ScaledVal = ((double)i)*ScaleFactor;
		
		/* find interpolation interval */
		j = 1;
		while (ScaledVal >= (double)j) {
			j++;
		}	/* j <= ScaledVal < n-1 */

		/* calculate 4-point Lagrangian interpolant  */
		index1 = MAX( (j-2), (0) );	/* pick end points in [0, n-1] */
		index2 = MIN( (j+1), (n-1) );

		sum = 0.0;
		for (k = index1; k <= index2; k++) { 
			
			p = TempData[k];
			p2 = k;
			for (m = index1; m <= index2; m++) {
				if (m != k) {
					p3 = m;
					p *= (ScaledVal - p3) / (p2 -p3);
				}
			}
			sum += p;
		}
		
		/* insert the value into the TRC */
		CurveData[i] = (KpUInt16_t)KpF15d16FromDouble(sum);
	}
	
	CurveData[255] = 65535U;   /* KpF15d16FromDouble(1.0) = 0  */

	/** set the tag **/
	Status = SpTagSet (Profile, &Tag);

	return Status;

}

/* ---------------------------------------------------------------------- */

static KpBool_t AdaptXYZ( xyz_t *rXYZ, xyz_t *gXYZ, xyz_t *bXYZ,
                          xyz_t *adaptRed, xyz_t *adaptGreen, xyz_t *adaptBlue)
{
SpMatrix_t	inputXYZ, curWP, adaptedXYZ;
SpMatrix_t	newWP, scale, newWPcone, curWPcone, inputCone, adaptedCone;
KpUInt32_t	row, col;
KpInt32_t	error1, error2;
		

	/* Create matrix from Profile tags:  */
	inputXYZ.nRows = 3;
	inputXYZ.nCols = 3;
	inputXYZ.coef[0][0] = rXYZ->x;
	inputXYZ.coef[0][1] = gXYZ->x;
	inputXYZ.coef[0][2] = bXYZ->x;
	inputXYZ.coef[1][0] = rXYZ->y;
	inputXYZ.coef[1][1] = gXYZ->y;
	inputXYZ.coef[1][2] = bXYZ->y;
	inputXYZ.coef[2][0] = rXYZ->z;
	inputXYZ.coef[2][1] = gXYZ->z;
	inputXYZ.coef[2][2] = bXYZ->z;

	/* Adjust given matrix by applying chromatic adaptation from current white point to D50  */
	adaptedXYZ.nRows = 3;	/* set up resultant matrix */
	adaptedXYZ.nCols = 3;

	curWP.nRows = 3;		/* set up vector for current white point */
	curWP.nCols = 1;
	SpMatZero (&curWP);

	curWP.coef[0][0] = inputXYZ.coef[0][0] + inputXYZ.coef[0][1] + inputXYZ.coef[0][2];	/* = X_RGB */
	if (curWP.coef[0][0] <= 0.0) {
		return KPFALSE;
	}

	curWP.coef[1][0] = inputXYZ.coef[1][0] + inputXYZ.coef[1][1] + inputXYZ.coef[1][2];	/* = Y_RGB */
	if (curWP.coef[1][0] <= 0.0) {
		return KPFALSE;
	}

	curWP.coef[2][0] = inputXYZ.coef[2][0] + inputXYZ.coef[2][1] + inputXYZ.coef[2][2];	/* = Z_RGB */
	if (curWP.coef[2][0] <= 0.0) {
		return KPFALSE;
	}

	
	
	newWP.nRows = 3;		/* set up new white point vector */
	newWP.nCols = 1;

	newWP.coef[0][0] = X_D50;	/* which is D50 */
	newWP.coef[1][0] = Y_D50;
	newWP.coef[2][0] = Z_D50;

	scale.nRows = 3;			/* set up scaling vector */
	scale.nCols = 1;

	/* convert new WP to cone primaries */
	error1 = xyz2cone (&newWP, &newWPcone);	

	/* convert current WP to cone primaries */
	error2 = xyz2cone (&curWP, &curWPcone);

	if ((error1 != 1) || (error2 != 1)) {
		return KPFALSE;
	}

	/* calculate ratios */
	error1 = SpMatDotDiv (&newWPcone, &curWPcone, &scale);
	if (error1 != 1) {
		return KPFALSE;
	}

	/* replicate scale vector to 2nd and 3rd columns */
	for (col = 1; col < 3; col++) {
		for (row = 0; row < 3; row++) {
			scale.coef[row][col] = scale.coef[row][0];
		}
	}

	scale.nCols = 3;			/* has 3 columns now */
		
	/* convert input XYZs to cone primaries */
	error1 = xyz2cone (&inputXYZ, &inputCone);
	if (error1 != 1) {
		return KPFALSE;
	}
		
	/* adapt cone primaries */
	error1 = SpMatDotMul (&inputCone, &scale, &adaptedCone);
	if (error1 != 1) {
		return KPFALSE;
	}

	/* convert input XYZs to cone primaries */
	error1 = cone2xyz (&adaptedCone, &adaptedXYZ);
	if (error1 != 1) {
		return KPFALSE;
	}
	
	/* convert back to d15f16 data format */
	adaptRed->x		= adaptedXYZ.coef[0][0];	
	adaptGreen->x	= adaptedXYZ.coef[0][1];	
	adaptBlue->x	= adaptedXYZ.coef[0][2];	
	adaptRed->y		= adaptedXYZ.coef[1][0];	
	adaptGreen->y	= adaptedXYZ.coef[1][1];	
	adaptBlue->y	= adaptedXYZ.coef[1][2];	
	adaptRed->z		= adaptedXYZ.coef[2][0];	
	adaptGreen->z	= adaptedXYZ.coef[2][1];	
	adaptBlue->z	= adaptedXYZ.coef[2][2];	

	return KPTRUE;
}

/*
 * Convert tristimulus values to the rho, gamma, beta
 * cone primaries.  See Hunt, Measuring Colour pg. 71.
 */
static KpInt32_t xyz2cone (SpMatrix_p src,	SpMatrix_p dest)
{
	SpMatrix_t toCone = {3, 3,
						0.40024,	0.70760,	-0.08081,
						-0.22630,	1.16532,	 0.04570,
						0.0,		0.0,		 0.91822};
	KpInt32_t	error;

	error = SpMatMul (&toCone, src, dest);
	
	return error;
}

/*
 * Convert tristimulus values to the rho, gamma, beta
 * cone primaries.  See Hunt, Measuring Colour pg. 71.
 */
static KpInt32_t cone2xyz (SpMatrix_p src, SpMatrix_p dest)
{
	SpMatrix_t fromCone = {3, 3,
					1.85993638745584,	-1.12938161858009,	 0.21989740959619,
					0.36119143624177,	 0.63881246328504,	-0.00000637059684,
					0.0,				 0.0,				 1.08906362309686};
	KpInt32_t	error;

	error = SpMatMul (&fromCone, src, dest);
	
	return error;
}

/* ---------------------------------------------------------------------- */
SpStatus_t SPAPI SpTagCreateColorantRC (SpProfile_t		Profile, 
										SpPhosphor_t	FAR *PData,
										KpF15d16XYZ_t	FAR *WhitePoint,
										KpF15d16_t		Gamma)
{
	double			Row0 [4];
	double			Row1 [4];
	double			Row2 [4];
	double			*Mat [3];
	double			Nr, Ng, Nb;
	xyz_t			primary1, primary2, primary3;
	xyz_t			adaptedPrimary1, adaptedPrimary2, adaptedPrimary3;
	xyz_t			Primary1, Primary2, Primary3;
	SpStatus_t		Status;
	
	/* copy the passed in data to the local static variables */
	Set_xy(PData->red.x, PData->red.y, &Primary1); 
	Set_xy(PData->green.x, PData->green.y, &Primary2); 
	Set_xy(PData->blue.x, PData->blue.y, &Primary3); 

	/* initialize matrix */
	Mat [0] = Row0;
	Mat [1] = Row1;
	Mat [2] = Row2;


	Row0 [0] = Primary1.x; 
	Row0 [1] = Primary2.x;
	Row0 [2] = Primary3.x;
	Row0 [3] = KpF15d16ToDouble(WhitePoint->X) / KpF15d16ToDouble(WhitePoint->Y);

	Row1 [0] = Primary1.y;
	Row1 [1] = Primary2.y;
	Row1 [2] = Primary3.y;
	Row1 [3] = KpF15d16ToDouble(WhitePoint->Y) /  KpF15d16ToDouble(WhitePoint->Y); /*  i.e. 1.0 */

	Row2 [0] = Primary1.z;
	Row2 [1] = Primary2.z;
	Row2 [2] = Primary3.z;
	Row2 [3] = KpF15d16ToDouble(WhitePoint->Z) / KpF15d16ToDouble(WhitePoint->Y);

	Status = SolveMat (Mat, 3, 4);
	if (Status != SpStatSuccess) {
		return Status;
	}

	Nr = Mat [0] [3];
	Ng = Mat [1] [3];
	Nb = Mat [2] [3];

	/*
	 * Do adaptation
	 */
	primary1.x = Nr * Primary1.x;
	primary1.y = Nr * Primary1.y;
	primary1.z = Nr * Primary1.z;
	primary2.x = Ng * Primary2.x;
	primary2.y = Ng * Primary2.y;
	primary2.z = Ng * Primary2.z;
	primary3.x = Nb * Primary3.x;
	primary3.y = Nb * Primary3.y;
	primary3.z = Nb * Primary3.z;

	AdaptXYZ(&primary1, &primary2, &primary3, &adaptedPrimary1,
						&adaptedPrimary2, &adaptedPrimary3);

	/* Set red colorant  */
	Status = SetXYZ (Profile, SpTagRedColorant,
					 adaptedPrimary1.x, adaptedPrimary1.y, adaptedPrimary1.z);
	if (Status != SpStatSuccess)  {
		return Status;
	}
	
	/* set green colorant */
	Status = SetXYZ (Profile, SpTagGreenColorant,
					adaptedPrimary2.x, adaptedPrimary2.y, adaptedPrimary2.z);
	if (Status != SpStatSuccess)  {
		return Status;
	}

	/* set blue colorant */
	Status = SetXYZ (Profile, SpTagBlueColorant,
					adaptedPrimary3.x, adaptedPrimary3.y, adaptedPrimary3.z);
	if (Status != SpStatSuccess)  {
		return Status;
	}

	/* set red gamma */
	Status = SetResponse(Profile, SpTagRedTRC, Gamma);

	/* set green gamma  */
	Status = SetResponse(Profile, SpTagGreenTRC, Gamma);

	/* set blue gamma */
	Status = SetResponse(Profile, SpTagBlueTRC, Gamma);

	return Status;
}




