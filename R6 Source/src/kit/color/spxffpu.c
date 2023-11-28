/*********************************************************************/
/*
	Contains:	This module contains conversion routines.

				Created by lsh, October 18, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, 
	            all rights reserved.

	Changes:
	
	SCCS Revision:
		@(#)spxffpu.c	1.21	11/25/97

	To Do:
*/
/*********************************************************************/


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
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1997                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"

#if defined (KPMAC)
#include <fp.h>
#else
#include <math.h>
#endif


#include "fut.h"
#include "kcmptlib.h"
#include "attrcipg.h"
#include <string.h>

#if defined (KPMAC68K) && !defined (SP_FPU)
/************************************************************************/
/* For the macintosh find out if there is a floating point processor.	*/
/*	for everybody else there is not.									*/
/************************************************************************/
static void 
SpGetFPU (KpInt32_p SpFPUType)
{
	OSErr	myErr;
	KpInt32_t	myFPUType;
	
	myErr = Gestalt(gestaltFPUType, &myFPUType);
	if (myFPUType == gestaltNoFPU)
		*SpFPUType = SpNoFPU;
	else
		*SpFPUType = SpFoundFPU;
}
#endif

#if !defined (SP_FPU)
/************************************************************/
/*	Return gestalt status for whether or not FPU is present */
/************************************************************/
KpInt32_t SpIsFPUpresent ()
{
	KpInt32_t	SpFPUType = SpNoFPU;
	
#if defined (KPMAC68K)
	SpGetFPU(&SpFPUType);
#endif

	return (SpFPUType);
}

#endif

#if !defined (KPMAC68K)
/************************************************************/
/* If this is not a KPMAC68K these functions do not exist. */
/************************************************************/
KpInt32_t LAB_to_uvLFPU (
				PTRefNum_t	FAR *aPT,
				KpInt32_t	Render,
				SpParadigm_t ParadigmType,
				KpInt32_t	GridSize)
{
	SPArgUsed (aPT);
	SPArgUsed (Render);
	SPArgUsed (ParadigmType);
	SPArgUsed (GridSize);
	return (0);

}
KpInt32_t UVL_to_labFPU (
				PTRefNum_t	FAR *aPT,
				KpInt32_t	Render,
				SpParadigm_t ParadigmType,
				KpInt32_t	GridSize)
{
	SPArgUsed (aPT);
	SPArgUsed (Render);
	SPArgUsed (ParadigmType);
	SPArgUsed (GridSize);
	return (0);
}

SpStatus_t SPAPI SpXformCreateMatTagsFromPTFPU (
				SpProfile_t		Profile,
				PTRefNum_t		RefNum)
{
	SPArgUsed (Profile);
	SPArgUsed (RefNum);

	/* return error - should never call this function */
	return (SpStatNotImp);	
}

SpStatus_t SPAPI SpXformCreateMatTagsFromXformFPU (
				SpProfile_t		Profile,
				SpXform_t		Xform)
{
	SPArgUsed (Profile);
	SPArgUsed (Xform);

	/* return error - should never call this function */
	return (SpStatNotImp);	
}

#endif 	/* ! KPMAC68K */

/***********************************************************************
 ***** Floating Poing Magic for 68K
 *****
 ***** This file is compiled twice when building the 68K sprofile
 ***** library.  It is compiled in the main sprofile project without
 ***** SP_FPU being defined.  It is also compiled on its own with
 ***** spfp68K_glue.c with SP_FPU defined.  The end result is 
 ***** two functions for each function in this file.  One function xxxFPU
 ***** used the floating point processor and 96-bit doubles.  The other
 ***** function xxxnoFPU uses the emulation functions and 80-bit doubles.
 **********************************************************************/

#if defined (SP_FPU)
#define sppow( x, y ) ( powFPU( x , y) )
#define splog( x ) ( logFPU( x ) )
#define spexp( x ) ( expFPU ( x ) )
#define spsqrt( x ) ( sqrtFPU ( x ) )
#define spfabs( x ) ( fabsFPU ( x ) )
#define spatan2( x, y ) ( atan2FPU ( x , y) )
#define sptan( x ) ( tanFPU ( x ) )
#else
#define sppow( x, y )  (pow( x, y ) )
#define splog( x ) ( log( x ) )
#define spexp( x ) ( exp ( x ) )
#define spsqrt( x ) ( sqrt ( x ) )
#define spfabs( x ) ( fabs ( x ) )
#define spatan2( x, y ) ( atan2 ( x , y) )
#define sptan( x ) ( tan ( x ) )
#endif

/***********************************************************************
 ***** BEGIN COMMON DEFINITIONS & CODE FOR LAB<->UVL STUFF
 ***********************************************************************/
/* flare.h - definitions to shape shadows by adding `flare' */

#define	YMIN	0.00392156862745	/* Y_min = 1/255, a la DM8000 */
#define YFLARE	0.00393700787402	/* Y_min/(1 + Y_min) */
#define	DMAX	2.40654018043395	/* D_max = -(log Y_min) */
#define	TANPT	0.01065992873906	/* eY_min */
#define	MAPPT	0.18046425546277	/* (log e)/D_max */
/*#define	SLOPE	16.9292178100213	* MAPPT/TANPT * */

/* general.h */

		/* Convenient Notations */
#if !defined (M_E)
#define M_E             2.7182818284590452354
#define M_LOG10E        0.43429448190325182765
#define M_LN10          2.30258509299404568402
#define M_PI    		3.14159265358979323846
#define M_PI_2          1.57079632679489661923
#define M_PI_4          0.78539816339744830962
#endif

#define	OK		0			/* successful completion */
#define	ERR		-1			/* error return */
#define NIL		-1			/* unassigned index */

#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))	/* maximum of 2 values */
#define MIN(a, b)	((a) < (b) ? (a) : (b))	/* minimum of 2 values */
#endif

/* restricts t to interval [low, high] */
#define RESTRICT(t, low, high)	\
	( MAX((low), MIN((high), (t))) )

/* quantizes t to nearest (short) integer in given scale */
#define QUANT(t, scale)		\
	( (short)((double)(scale) * RESTRICT((t), 0.0, 1.0) + 0.5) )

/* (approximate) inverse of QUANT() */
#define DEQUANT(t, scale)	\
	( (double)(t)/(double)(scale) )

#define	POW(x, power)	( ((x) > 0.0) ? spexp ((power) * splog ((x))) \
			 	      : sppow ((x), (power)) )
/* not currently used 
#define	ANTILOG(x)	( spexp ((x) * M_LN10) )
*/

#define H(t)			\
	( ((t) <= 0.008856) ? 9.033 * (t) \
			    : 1.16 * POW((t), 1.0/3.0) - 0.16 )
	/* CIE visual-response function */

#define H_inverse(t)		\
	( ((t) <= 0.08) ? (t)/9.033 \
			      : POW(((t) + 0.16)/1.16, 3.0) )
	/* inverse of H() */

#define	XWHITE	0.96819				/* Prophecy uvL = [87 196 255] */
#define	YWHITE	1.0000
#define	ZWHITE	0.82830

#define	REFLECTANCE	0.85		/* reflectance of nominal substrate */
#define NEUTRALGRID8	0.57142857142857	/* 4/7; neutral grid point */
#define	NEUTRALGRID16	0.53333333333333	/* 8/15; neutral grid point */

/* Local Prototypes */

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static void CalcItblFPU (
				KcmHandle	itblH,
				KpInt32_t	gridSize,
				double		(*ifun) (double, double),
				double		neutralgrid)
#else
static void CalcItblnoFPU (
				KcmHandle	itblH,
				KpInt32_t	gridSize,
				double		(*ifun) (double, double),
				double		neutralgrid)
#endif
{
	KpInt32_t			count;
	double				val;
	double				norm;
	fut_itbldat_ptr_t	itblPtr;

	itblPtr = (fut_itbldat_ptr_t)lockBuffer (itblH);

	norm = (double) ((gridSize - 1) << FUT_INP_DECIMAL_PT);
	for (count = 0; count<FUT_INPTBL_ENT; count++ ) {
		val = (*ifun) (((double)count / (double) (FUT_INPTBL_ENT-1)),
							neutralgrid);
		if ((val < 0.0) || (val > 1.0)) {
			unlockBuffer (itblH);
			return;
		}
		val = val * norm + .5;

				/* clip value to 1 less than norm ( see note above) */
		if ( val >= norm )
			val = norm - 1;

		*itblPtr++ = (fut_itbldat_t) val;

	}
	/*
	 * set the very last (generally invisible) input table entry to the
	 * value of the previous one.  This will perform automatic clipping
	 * of input greater than 4080 to the valid gridspace, which is defined
	 * only for input in the range (0, 255) or (0<<4, 255<<4).
	 */

	*itblPtr = *(itblPtr-1);

	unlockBuffer ((KcmHandle)itblH);
}

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static void CalcGrid3FPU (
#else
static void CalcGrid3noFPU (
#endif
			   KcmHandle	gtbl_1_H,
			   KcmHandle	gtbl_2_H,
			   KcmHandle	gtbl_3_H,
			   KpInt32_t	nDim,
			   void			(*gmap) (fut_gtbldat_t,
									fut_gtbldat_t,
									fut_gtbldat_t,
									fut_gtbldat_t FAR *,
									fut_gtbldat_t FAR *,
									fut_gtbldat_t FAR *,
									KpBool_t, double),
				double		neutralgrid,
				KpBool_t		isAbsolute)
{
	fut_gtbldat_t	input[3], output [3];
	fut_gtbldat_t	FAR *grid [3];
	KpInt32_t		i, j, k, ic;

	grid[0] = (fut_gtbldat_t *) lockBuffer (gtbl_1_H);
	grid[1] = (fut_gtbldat_t *) lockBuffer (gtbl_2_H);
	grid[2] = (fut_gtbldat_t *)lockBuffer (gtbl_3_H);

	for ( k=0; k<nDim; k++ ) {
		input[0] = (fut_gtbldat_t) ((FUT_GRD_MAXVAL*k)/(nDim-1));	/* x */
		for ( j=0; j<nDim; j++ ) {
			input[1] = (fut_gtbldat_t) ((FUT_GRD_MAXVAL*j)/(nDim-1));/* y */
			for ( i=0; i<nDim; i++ ) {
				input[2] = (fut_gtbldat_t) ((FUT_GRD_MAXVAL*i)/(nDim-1));/* z */
				(*gmap)(input[0], input[1], input[2],
						&output[0], &output[1], &output[2],
						isAbsolute, neutralgrid);	/* remap values */
				for ( ic=0; ic<3; ic++) {	/* loop over channels */
					if ( (output [(int)ic]) > FUT_GRD_MAXVAL ) {
						unlockBuffer ((KcmHandle)gtbl_1_H);
						unlockBuffer ((KcmHandle)gtbl_2_H);
						unlockBuffer ((KcmHandle)gtbl_3_H);
						return;
					}
					*grid[(int)ic]++ = output[(int)ic];		/* populate grid */
				}									/* next channel */
			}										/* next z value */
		}											/* next y value */
	}												/* next x value */
	unlockBuffer ((KcmHandle)gtbl_1_H);
	unlockBuffer ((KcmHandle)gtbl_2_H);
	unlockBuffer ((KcmHandle)gtbl_3_H);
}

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static void CalcOtblFPU (
#else
static void CalcOtblnoFPU (
#endif
				KcmHandle otblH,
				fut_otbldat_t (*ofun) (fut_gtbldat_t, KpBool_t),
				KpBool_t isAbsolute)
{
	KpInt32_t			count;
	fut_otbldat_t		val;
	fut_otbldat_ptr_t	otblPtr;

	otblPtr = (fut_otbldat_ptr_t)lockBuffer (otblH);

	for (count = 0; count < FUT_OUTTBL_ENT; count++ ) {
		val = (*ofun) ((fut_gtbldat_t)count, isAbsolute);
		if ( (unsigned) val > FUT_GRD_MAXVAL ) {
			unlockBuffer ((KcmHandle)otblH);
			return;
		}
		*otblPtr++ = val;
	}
	unlockBuffer ((KcmHandle)otblH);
}



/***********************************************************************
 ***** BEGIN UVL->LAB CODE
 ***********************************************************************/

/*--------------------------------------------------------------------------------
 *  Definitions for uv-mapping
 *--------------------------------------------------------------------------------
 */
#define U0	0.34117647058824 /* 0.33725490196078	/* 86/255; approximately, D50 */
#define V0	0.76862745098039	/* 196/255;       "           */
#define UGRID0	0.33333333333333	/* 5/15; neutral grid point */
#define	VGRID0	0.60000000000000	/* 9/15;          "         */
#define SLOPE	0.33333333333333	/* 1/3; slope at neutral point */
#define SLOPEINV	3				/* 1/slope ; slope at neutral point */

#define	AU0	-2.07058823529412 /* -2.03529411764706	/* coefficient of quadratic term for low u  */
		/* = -(U0/UGRID0 - SLOPE)/UGRID0 */
#define AU1	0.98235294117647 /* 0.99117647058823	/*                    "              high u */
		/* = ((1.0 - U0)/(1.0 - UGRID0) - SLOPE)/(1.0 - UGRID0) */
#define AV0	-1.57952069716776	/*                    "              low v  */
		/* = -(V0/VGRID0 - SLOPE)/VGRID0 */
#define AV1	0.61274509803922	/*                    "              high v */
		/* = ((1.0 - V0)/(1.0 - VGRID0) - SLOPE)/(1.0 - VGRID0) */


#define	REMAP(x, a)	( (spsqrt (1.0 + 4.0 * (a) * (x) * (SLOPEINV * SLOPEINV)) - 1.0) \
				* (0.5 * SLOPE / (a)) )
#define	UNMAP(x, a)	( (SLOPE + (a) * (x)) * (x) )

/*---------------------------------------------------------------------*/
#if defined (SP_FPU)
static void gridUVL_LABFPU (
#else
static void gridUVL_LABnoFPU (
#endif
				double u,
				double v,
				double l,
				double *pL,
				double *pa,
				double *pb,
				KpBool_t isAbsolute)
{
	double	x, y, z, p;
	double	delta, a, astar, bstar;

	SPArgUsed(isAbsolute);

     /* Decode S-shaped functions of u and v:  */
	delta = u - UGRID0;			/* f(u) -> u */
	a = (delta > 0.0) ? AU1 : AU0;
	delta = UNMAP (delta, a);
	u = U0 + delta;

	delta = v - VGRID0;			/* f(v) -> v */
	a = (delta > 0.0) ? AV1 : AV0;
	delta = UNMAP (delta, a);
	v = V0 + delta;

     /* Compute CIE 1931 tristimulus values:  */
	y = H_inverse (l);			/* L* -> Y */
	
	u = 0.070 + 0.40996784565916 * u;			/* CIE 1976 u' */
	v = 0.165 + 0.41986827661910 * v;			/* CIE 1976 v' */
	x = 2.25 * (u / v) * y;
	z = ((3.0 - 0.75 * u) / v - 5.0) * y;

     /* Scale to white point:  */
	x /= XWHITE;
	y /= YWHITE;
	z /= ZWHITE;

	p = H (y);
	
     /* Convert to CIELAB:  */
	astar = (H(x) - p) / 0.00232;	/* CIE 1976 a* */
	bstar = (p - H(z)) / 0.00580;	/* CIE 1976 b* */

     /* Encode CIELAB for L* in [0, 100] and a* & b* in [-200, 200]:  */

/*	   case 1:	- (L*)/100 */
	*pL = RESTRICT (p, 0.0, 1.0);

/*	   case 2:	- (1/2)[1 + (a*)/200] */
	p = 0.5 * (1.0 + 0.005 * astar);
	*pa = RESTRICT (p, 0.0, 1.0);

/*	   case 3:	- (1/2)[1 + (b*)/200] */
	p = 0.5 * (1.0 + 0.005 * bstar);
	*pb = RESTRICT (p, 0.0, 1.0);
}

/*---------------------------------------------------------------------
 *  xfun, yfun, zfun -- input mappings (lab)
 *---------------------------------------------------------------------
 */
#if defined (SP_FPU)
static double xfun_uFPU (
#else
static double xfun_unoFPU (
#endif
				double	u,
				double neutralgrid)
{
	double	delta, a;

	SPArgUsed(neutralgrid);

	delta = u - U0;
	a = (delta > 0.0) ? AU1 : AU0;
	delta = REMAP (delta, a);
	u = UGRID0 + delta;
	return RESTRICT (u, 0.0, 1.0);
}

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static double yfun_vFPU (
#else
static double yfun_vnoFPU (
#endif
				double	v,
				double	neutralgrid)
{
	double	delta, a;

	SPArgUsed(neutralgrid);

	delta = v - V0;
	a = (delta > 0.0) ? AV1 : AV0;
	delta = REMAP (delta, a);
	v = VGRID0 + delta;
	return RESTRICT (v, 0.0, 1.0);
}

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static double zfun_uvZFPU (
#else
static double zfun_uvZnoFPU (
#endif
				double	l,
				double	neutralgrid) 
{
	SPArgUsed(neutralgrid);

	return l;
}

/*----------------------------------------------------------------------------
 *  outfun -- rescale and clip a* and b* and convert to signed representation
 *----------------------------------------------------------------------------
 */
#if defined (SP_FPU)
static double outfunlab_LFPU (
#else
static double outfunlab_LnoFPU (
#endif
				double	p, KpBool_t isAbsolute)
{
	SPArgUsed(isAbsolute);
	
	p = H_inverse (p);		/* Y */
	p = (255.0 * p - 1.0) / 254.0;
	p = H (p);			/* (L*)/100, in [-0.0356, 1] */
	return RESTRICT (p, 0.0, 1.0);
}

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static double outfun_abFPU (
#else
static double outfun_abnoFPU (
#endif
				double	p)
{
	p = 400.0 * (p - 0.5);	/* CIE 1976 a*, b*, in [-200, 200] */
	p = RESTRICT (p, -128.0, 127.0);	/* clip to [-128, 127] */
	p = p + 128.0;		/* -> [0, 255] */
	p /= 255.0;		/* from [0, 255] to [0, 1] */

	return RESTRICT (p, 0.0, 1.0);
			/* L* in [0, 100]; a* & b* in [-128, 127] */
}

/*********** FPU MACROS ******/
 #if defined(SP_FPU)			/* using the FPU? */
 #define CalcItbl(itblH, gridSize, ifun,neutralgrid) ( CalcItblFPU (itblH, gridSize, ifun, neutralgrid) )
 #define CalcGrid3( gtbl_1_H, gtbl_2_H, gtbl_3_H, nDim, gmap, neutralgrid, isAbsolute) \
 			( CalcGrid3FPU ( gtbl_1_H, gtbl_2_H, gtbl_3_H, nDim, gmap, neutralgrid, isAbsolute))
#define CalcOtbl(otblH, ofun,isAbsolute) ( CalcOtblFPU (otblH, ofun, isAbsolute) )

#define gridUVL_LAB(u,  v, l, pL, pa, pb, isAbsolute) ( gridUVL_LABFPU (u, v, l, pL, pa, pb, isAbsolute) )
#define xfun_u xfun_uFPU
#define yfun_v yfun_vFPU
#define zfun_uvZ zfun_uvZFPU
#define outfunlab_L(p,  isAbsolute) ( outfunlab_LFPU ( p, isAbsolute) )
#define outfun_ab(p) ( outfun_abFPU (p) )
#else							/* all other programming environments */

#define CalcItbl(itblH, gridSize, ifun, neutralgrid) ( CalcItblnoFPU (itblH, gridSize, ifun, neutralgrid) )
#define CalcGrid3( gtbl_1_H, gtbl_2_H, gtbl_3_H, nDim, gmap, neutralgrid, isAbsolute) \
		( CalcGrid3noFPU ( gtbl_1_H, gtbl_2_H, gtbl_3_H, nDim, gmap, neutralgrid, isAbsolute) )
#define CalcOtbl(otblH, ofun,isAbsolute) (CalcOtblnoFPU (otblH, ofun, isAbsolute))
#define gridUVL_LAB(u, v, l, pL, pa, pb, isAbsolute) (gridUVL_LABnoFPU (u, v, l, pL, pa, pb, isAbsolute))
#define xfun_u xfun_unoFPU
#define yfun_v yfun_vnoFPU
#define zfun_uvZ zfun_uvZnoFPU
#define outfunlab_L(p, isAbsolute) ( outfunlab_LnoFPU ( p, isAbsolute) )
#define outfun_ab(p) ( outfun_abnoFPU (p) )
#endif

/*-------------------------------------------------------------------
 *   umap -- computation of a Output from UVL
 *-------------------------------------------------------------------
 */
#if defined (SP_FPU)
static fut_otbldat_t aLABmapFPU (
#else
static fut_otbldat_t aLABmapnoFPU (
#endif
				fut_gtbldat_t	a,
				KpBool_t isAbsolute)
{
	double	aa;		/* unquantized u' */

	SPArgUsed(isAbsolute);

     /* Unquantize and linearize u':  */
	aa = (double) DEQUANT (a, FUT_GRD_MAXVAL);
	aa = outfun_ab (aa);
	return QUANT (aa, FUT_MAX_PEL12);	/* Prophecy u */
}

/*-------------------------------------------------------------------
 *   vmap -- computation of b output from UVL
 *-------------------------------------------------------------------
 */
#if defined (SP_FPU)
static fut_otbldat_t bLABmapFPU (
#else
static fut_otbldat_t bLABmapnoFPU (
#endif
				fut_gtbldat_t	b,
				KpBool_t isAbsolute)
{
	double	bb;		/* unquantized v' */

	SPArgUsed(isAbsolute);

     /* Unquantize and linearize v':  */
	bb = (double) DEQUANT (b, FUT_GRD_MAXVAL);
	bb = outfun_ab (bb);

	return QUANT (bb, FUT_MAX_PEL12);	/* Prophecy v */
}

/*-------------------------------------------------------------------
 *   Lmap -- computations of L output from UVL
 *-------------------------------------------------------------------
 */
#if defined (SP_FPU)
static fut_otbldat_t LLABmapFPU (
#else
static fut_otbldat_t LLABmapnoFPU (
#endif
				fut_gtbldat_t	l,
				KpBool_t isAbsolute)
{
	double	ll;

     /* Remap Y and convert to (quantized) L*:  */
	ll = (double)DEQUANT (l, FUT_GRD_MAXVAL);
	ll = outfunlab_L(ll, isAbsolute);

	return QUANT (ll, FUT_MAX_PEL12); 	/* Prophecy L */
}

/*-------------------------------------------------------------------
 *  gridcalc -- computation of LAB from uvL
 *-------------------------------------------------------------------
 */
#if defined (SP_FPU)
static void gridcalcLABFPU (
#else
static void gridcalcLABnoFPU (
#endif
				fut_gtbldat_t gu,
				fut_gtbldat_t gv,
				fut_gtbldat_t gL,
				fut_gtbldat_t *gl,
				fut_gtbldat_t *ga,
				fut_gtbldat_t *gb,
				KpBool_t isAbsolute,
				double neutralgrid)
{
	double	l, a, b;		/* CIE 1931 X, Y, Z */
	double	u, v, L;		/* CIE 1976 u', v', L' */

	SPArgUsed(neutralgrid);

	u = (double) gu / (double) FUT_GRD_MAXVAL;
	v = (double) gv / (double) FUT_GRD_MAXVAL;
	L = (double) gL / (double) FUT_GRD_MAXVAL;

	gridUVL_LAB (u, v, L, &l, &a, &b, isAbsolute);     /* Calculate:  */

	*gl = QUANT (l, FUT_GRD_MAXVAL);
	*ga = QUANT (a, FUT_GRD_MAXVAL);
	*gb = QUANT (b, FUT_GRD_MAXVAL);
}
/*********** FPU MACROS ******/
#if defined (SP_FPU)
#define aLABmap aLABmapFPU
#define bLABmap bLABmapFPU
#define LLABmap LLABmapFPU
#define gridcalcLAB gridcalcLABFPU
#else
#define aLABmap aLABmapnoFPU
#define bLABmap bLABmapnoFPU
#define LLABmap LLABmapnoFPU
#define gridcalcLAB gridcalcLABnoFPU
#endif

/*-------------------------------------------------------------------*/
#if defined (SP_FPU)
KpInt32_t UVL_to_labFPU (
				PTRefNum_t	FAR *aPT,
				KpInt32_t	Render,
				SpParadigm_t ParadigmType,
				KpInt32_t	GridSize)
#else
KpInt32_t UVL_to_labnoFPU (
				PTRefNum_t	FAR *aPT,
				KpInt32_t	Render,
				SpParadigm_t ParadigmType,
				KpInt32_t	GridSize)
#endif
{
	PTRefNum_t	thePT;
	PTErr_t		thePTErr;
	KcmHandle	itbl_u;		/* pointers to input tables */
	KcmHandle	itbl_v;
	KcmHandle	itbl_L;
	KcmHandle	gtbl_a;		/* pointers to grid tables */
	KcmHandle	gtbl_b;
	KcmHandle	gtbl_L;
	KcmHandle	otbl_a;		/* pointers to output tables */
	KcmHandle	otbl_b;
	KcmHandle	otbl_L;
	KpInt32_t	nDim;		/* Number of Dimensions */
	int			result;		/* return value */
	KpInt32_t	dim[4];
	int			i;
	KpBool_t		isAbsolute;
	double		neutralgrid; /* igored right now for uvl->lab */

	/* set neutral grid point based on selected grid size */
	if (GridSize == 8)
		neutralgrid=NEUTRALGRID8;
	else
		neutralgrid=NEUTRALGRID16;

	/* First check if the user wants to use fixed conversions */
	if (useFixed())
	{
		result = getPTFromFile(SpDir2LAB, Render, aPT);
		displayWarning("Using Fixed Conversion PTs!\n");
		return result;
	}

	/* determine which paradigm algoritm to use */
	isAbsolute = SpWhichParadigm(ParadigmType, Render);

	/* set up the dimensions */
	for (i=0;i<4;i++) dim[i] = GridSize;

    /* Compute (shared) input tables:  */
	/* NOW we call the driver for a fut to fill in! */

	thePTErr = PTNewEmpty (3, dim, 3, &thePT);
	if (thePTErr != KCP_SUCCESS)
		return -1;

/* now get the input tables and fill'er up */
	thePTErr = PTGetItbl (thePT, -1, FUT_XCHAN, (KcmHandle *)&itbl_u);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut(thePT);
		return -1;
	}
	CalcItbl (itbl_u, GridSize, xfun_u, neutralgrid);

	thePTErr = PTGetItbl (thePT, -1, FUT_YCHAN, (KcmHandle *) &itbl_v);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}
	CalcItbl (itbl_v, GridSize, yfun_v, neutralgrid);

	thePTErr = PTGetItbl (thePT, -1, FUT_ZCHAN, (KcmHandle *) &itbl_L);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}
	CalcItbl (itbl_L, GridSize, zfun_uvZ, neutralgrid);

     /* Compute channel-specific tables:  */

	/* Now lets use the soon to be new to the fut library call
		fut_calc_grid3 shall we? Ahhh but first we must already
		have a set of grid tables to calculate into. */

	thePTErr  = PTGetGtbl (thePT, FUT_XCHAN, &nDim, dim, (KcmHandle *) &gtbl_L);
	thePTErr |= PTGetGtbl (thePT, FUT_YCHAN, &nDim, dim, (KcmHandle *) &gtbl_a);
	thePTErr |= PTGetGtbl (thePT, FUT_ZCHAN, &nDim, dim, (KcmHandle *) &gtbl_b);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}

	CalcGrid3 (gtbl_L, gtbl_a, gtbl_b, GridSize, gridcalcLAB, neutralgrid, isAbsolute);

	/* Finally the Output tables */

	thePTErr = PTGetOtbl (thePT, FUT_XCHAN, (KcmHandle *) &otbl_L);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}
	CalcOtbl (otbl_L, LLABmap, isAbsolute);

	thePTErr = PTGetOtbl (thePT, FUT_YCHAN, (KcmHandle *) &otbl_a);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}
	CalcOtbl(otbl_a, aLABmap, isAbsolute);

	thePTErr = PTGetOtbl (thePT, FUT_ZCHAN, (KcmHandle *) &otbl_b);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}
	CalcOtbl (otbl_b, bLABmap, isAbsolute);

	*aPT = thePT;
	return 0;
}

/***********************************************************************
 ***** BEGIN LAB->UVL CODE
 ***********************************************************************/

/*---------------------------------------------------------------------
 *  Definitions for a* and b* mappings
 *---------------------------------------------------------------------
 */
#define	NEUTRALBYTE	0.50196078431373	/* 128/255; a* or b* = 0.0 */
#define APOWR		3.0 			/* exponent of a* mapping function */
#define ADENM		19.08553692318767 	/* exp(APOWR) - 1 */
#define	AEXP(x)		((spexp(APOWR * (x)) - 1.0) / ADENM)
#define ALOG(x)		(splog(ADENM * (x) + 1.0) / APOWR)
#define BPOWR		2.0 			/* exponent of b* mapping function */
#define BDENM		6.38905609893065 	/* exp(BPOWR) - 1 */
#define	BEXP(x)		((spexp(BPOWR * (x)) - 1.0) / BDENM)
#define BLOG(x)		(splog(BDENM * (x) + 1.0) / BPOWR)

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static void gridLAB_UVLFPU (
#else
static void gridLAB_UVLnoFPU (
#endif
				double l,
				double a,
				double b,
				double *pu,
				double *pv,
				double *pL,
				KpBool_t isAbsolute,
				double neutralgrid)
{
	double	delta;
	double	x, y, z;
	double	u, v;		/* output quantities */
	double	denom;		/* denominator of u', v' */

	SPArgUsed(isAbsolute);
	
     /* Linearize L*:  */
	y = H_inverse (l);		/* CIE 1931 Y, in [0, 1] */

     /* Undo grid mapping of a* and b*:  */
	delta = a - neutralgrid;
	if (delta < 0.0)
	   a = NEUTRALBYTE * ALOG (a / neutralgrid);
	else
	   a = 1.0 - (1.0 - NEUTRALBYTE) * ALOG ((1.0 - a) / (1.0 - neutralgrid));

	delta = b - neutralgrid;
	if (delta < 0.0)
	   b = NEUTRALBYTE * BLOG (b / neutralgrid);
	else
	   b = 1.0 - (1.0 - NEUTRALBYTE) * BLOG ((1.0 - b) / (1.0 - neutralgrid));

     /* Shift a* and b* to standard domain:  */
	a = 255.0 * a - 128.0;		/* CIE 1976 a* */
	b = 255.0 * b - 128.0;		/* CIE 1976 b* */

     /* Rescale a* and b*:  */
	a = 0.00232 * a;		/* H(X/X_n) - H(Y/Y_n) */
	b = 0.00580 * b;		/* H(Y/Y_n) - H(Z/Z_n) */

     /* Compress luminance for Prophecy uvL:  */
	y = (254.0 * y + 1.0) / 255.0;

     /* Separate X and Z channels:  */
	l = H (y);			/* H(Y/Y_n)*/ 
	x = a + l;			/* H(X/X_n) */
	z = l - b;			/* H(Z/Z_n) */

     /* Linearize X and Z:  */
	x = H_inverse (x);		/* X/X_n */
	z = H_inverse (z);		/* Z/Z_n */

/* Compute grid-table entry:  */

/* Scale XYZ:  */
	x *= XWHITE;			/* X */
	y *= YWHITE;			/* Y */
	z *= ZWHITE;			/* Z */

/* Compute arctan u':  */
	denom = x + 15.0 * y + 3.0 * z;
	u = spfabs (denom);		/* to improve interpolant */
	u = spatan2 (4.0 * x, u);	/* arctan u', in [-PI, PI] */

/* Encode over extended range:  */
	u = (u + M_PI_2) / M_PI;	/* [-PI/2, +PI/2] -> [0, 1] */
	*pu = RESTRICT (u, 0.0, 1.0);
	   
/* Compute arctan v':  */
	v = spatan2 (9.0 * y, denom);	/* arctan v', in [-PI, PI] */

/* Encode over extended range:  */
	v = v / M_PI;			/* [0, PI] -> [0, 1] */
	*pv = RESTRICT (v, 0.0, 1.0);

	*pL = l;
}

/***** Floating Point Macros ********/
#if defined (SP_FPU)
#define gridLAB_UVL(l, a, b, pu, pv, pL, isAbsolute, neutralgrid) \
				( gridLAB_UVLFPU (l, a,  b, pu, pv, pL, isAbsolute, neutralgrid) )
#else
#define gridLAB_UVL(l, a, b, pu, pv, pL, isAbsolute, neutralgrid) \
				( gridLAB_UVLnoFPU (l, a,  b, pu, pv, pL, isAbsolute, neutralgrid) )
#endif

/*-------------------------------------------------------------------
 *  gridcalc -- computation of uvL from LAB
 *-------------------------------------------------------------------
 */
#if defined (SP_FPU)
static void gridcalcUVLFPU (
#else
static void gridcalcUVLnoFPU (
#endif
				fut_gtbldat_t gl,
				fut_gtbldat_t ga,
				fut_gtbldat_t gb,
				fut_gtbldat_t *gu,
				fut_gtbldat_t *gv,
				fut_gtbldat_t *gL,
				KpBool_t isAbsolute,
				double neutralgrid)
{
	double	l, a, b;		/* CIE 1931 X, Y, Z */
	double	u, v, L;		/* CIE 1976 u', v', L' */

	l = (double) gl / (double) FUT_GRD_MAXVAL;
	a = (double) ga / (double) FUT_GRD_MAXVAL;
	b = (double) gb / (double) FUT_GRD_MAXVAL;

	gridLAB_UVL (l, a, b, &u, &v, &L, isAbsolute, neutralgrid);     /* Calculate:  */

	*gu = QUANT (u, FUT_GRD_MAXVAL);
	*gv = QUANT (v, FUT_GRD_MAXVAL);
	*gL = QUANT (L, FUT_GRD_MAXVAL);
}

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static double outfun_uFPU (
#else
static double outfun_unoFPU (
#endif
				double	p)
{
	double	uu;

/* u' */
/* Requantize u' to interval [0.07, 0.48]:  */
	uu = M_PI * p - M_PI_2;		/* arctan u' */
	uu = RESTRICT (uu, 0.0, M_PI_4);
	uu = sptan (uu);			/* CIE 1976 u', in [0, 1] */
	uu = (uu - 0.070) / 0.40996784565916;
	return RESTRICT (uu, 0.0, 1.0);
}

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static double outfun_vFPU (
				double	p)
#else
static double outfun_vnoFPU (
				double	p)
#endif
{
	double	 vv;

/* v' */
/* Requantize v' to interval [0.165, 0.585]:  */
	vv = M_PI *  p;			/* arctan v' */
	vv = RESTRICT (vv, 0.0, M_PI_4);
	vv = sptan (vv);			/* CIE 1976 v', in [0, 1] */
	vv = (vv - 0.165) / 0.41986827661910;
	return RESTRICT (vv, 0.0, 1.0);
}

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static double outfunuvl_LFPU (
#else
static double outfunuvl_LnoFPU (
#endif
				double	p,
				KpBool_t isAbsolute)
{
	SPArgUsed(isAbsolute);

	return RESTRICT (p, 0.0, 1.0);
}

/***** Floating Point Macros ********/
#if defined (SP_FPU)
#define gridcalcUVL gridcalcUVLFPU
#define outfun_u(p) ( outfun_uFPU(p) )
#define outfun_v(p) ( outfun_vFPU(p) )
#define outfunuvl_L(p, isAbsolute) ( outfunuvl_LFPU (p,isAbsolute) )
#else
#define gridcalcUVL  gridcalcUVLnoFPU
#define outfun_u(p) ( outfun_unoFPU(p) )
#define outfun_v(p) ( outfun_vnoFPU(p) )
#define outfunuvl_L(p, isAbsolute) ( outfunuvl_LnoFPU (p,isAbsolute) )
#endif
/*-------------------------------------------------------------------
 *   umap -- computation of u Output from LAB
 *-------------------------------------------------------------------
 */
#if defined (SP_FPU)
static fut_otbldat_t uUVLmapFPU (
#else
static fut_otbldat_t uUVLmapnoFPU (
#endif
				fut_gtbldat_t	u,
				KpBool_t isAbsolute)
{
	double	uu;		/* unquantized u' */

	SPArgUsed(isAbsolute);

     /* Unquantize and linearize u':  */
	uu = (double) DEQUANT (u, FUT_GRD_MAXVAL);
	uu = outfun_u (uu);
	return QUANT (uu, FUT_MAX_PEL12);	/* Prophecy u */
}

/*-------------------------------------------------------------------
 *   vmap -- computation of v output from LAB
 *-------------------------------------------------------------------
 */
#if defined (SP_FPU)
static fut_otbldat_t vUVLmapFPU (
#else
static fut_otbldat_t vUVLmapnoFPU (
#endif
				fut_gtbldat_t	v,
				KpBool_t isAbsolute)
{
	double	vv;		/* unquantized v' */

	SPArgUsed(isAbsolute);

     /* Unquantize and linearize v':  */
	vv = (double) DEQUANT (v, FUT_GRD_MAXVAL);
	vv = outfun_v (vv);

	return QUANT (vv, FUT_MAX_PEL12);	/* Prophecy v */
}

/*-------------------------------------------------------------------
 *   Lmap -- computations of L output from LAB
 *-------------------------------------------------------------------
 */
#if defined (SP_FPU)
static fut_otbldat_t LUVLmapFPU (
#else
static fut_otbldat_t LUVLmapnoFPU (
#endif
				fut_gtbldat_t	y,
				KpBool_t isAbsolute)
{
	double	yy;

     /* Remap Y and convert to (quantized) L*:  */
	yy = (double) DEQUANT (y, FUT_GRD_MAXVAL);
	yy = outfunuvl_L (yy, isAbsolute);

	return QUANT (yy, FUT_MAX_PEL12); 	/* Prophecy L */
}


/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static double xfun_LFPU (					/* L */
#else
static double xfun_LnoFPU (					/* L */
#endif
				double	x,
				double	neutralgrid)
{
	SPArgUsed(neutralgrid);

	return (x);
}

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static double yfun_aFPU (					/* a* */
#else
static double yfun_anoFPU (					/* a* */
#endif
				double	y,
				double	neutralgrid)
{
	double	delta;

	delta = y - NEUTRALBYTE;
	if (delta < 0.0)
	   y = neutralgrid * AEXP (y / NEUTRALBYTE);
	else
	   y = 1.0 - (1.0 - neutralgrid) * AEXP ((1.0 - y) / (1.0 - NEUTRALBYTE));
	return RESTRICT (y, 0.0, 1.0);
}

/*----------------------------------------------------------------------*/
#if defined (SP_FPU)
static double zfun_bFPU (					/* b* */
#else
static double zfun_bnoFPU (					/* b* */
#endif
				double	z,
				double	neutralgrid)
{
	double	delta;

	delta = z - NEUTRALBYTE;
	if (delta < 0.0)
	   z = neutralgrid * BEXP (z / NEUTRALBYTE);
	else
	   z = 1.0 - (1.0 - neutralgrid) * BEXP ((1.0 - z) / (1.0 - NEUTRALBYTE));
	return RESTRICT (z, 0.0, 1.0);
}

 /***** Floating Point Macros ********/
#if defined (SP_FPU)
#define uUVLmap uUVLmapFPU
#define vUVLmap vUVLmapFPU
#define LUVLmap LUVLmapFPU
#define xfun_L xfun_LFPU
#define yfun_a yfun_aFPU
#define zfun_b zfun_bFPU
#else
#define uUVLmap uUVLmapnoFPU
#define vUVLmap vUVLmapnoFPU
#define LUVLmap LUVLmapnoFPU
#define xfun_L xfun_LnoFPU
#define yfun_a yfun_anoFPU
#define zfun_b zfun_bnoFPU
#endif

/*-------------------------------------------------------------------*/


#if defined(SP_FPU)			/* using the FPU? */
KpInt32_t LAB_to_uvLFPU (
				PTRefNum_t	FAR *aPT,
				KpInt32_t	Render,
				SpParadigm_t ParadigmType,
				KpInt32_t	GridSize)
#else							/* all other programming environments */
KpInt32_t LAB_to_uvLnoFPU (
				PTRefNum_t	FAR *aPT,
				KpInt32_t	Render,
				SpParadigm_t ParadigmType,
				KpInt32_t	GridSize)
#endif
{
	PTRefNum_t			thePT;
	PTErr_t				thePTErr;
	KcmHandle			itbl_l;	/* pointers to input tables */
	KcmHandle			itbl_a;
	KcmHandle			itbl_b;
	KcmHandle			gtbl_u;	/* pointers to grid tables */
	KcmHandle			gtbl_v;
	KcmHandle			gtbl_L;
	KcmHandle			otbl_u;	/* pointers to output tables */
	KcmHandle			otbl_v;
	KcmHandle			otbl_L;
	KpInt32_t			nDim;		/* Number of Dimensions */
	int					result;		/* return value */
	KpInt32_t			dim[4];
	int					i;
	KpBool_t				isAbsolute;
	double				neutralgrid;

	SPArgUsed(ParadigmType);

	/* First check if the user wants to use fixed conversions */
	if (useFixed())
	{
		result = getPTFromFile(SpDir2UVL, Render, aPT);
		displayWarning("Using Fixed Conversion PTs!\n");
		return result;
	}

	isAbsolute = KPFALSE;

	/* setup the dimensions */
	for (i=0;i<4;i++) dim[i] = GridSize;

	/* setup neutralgrid - dependent on the GridSize */
	neutralgrid = (double)(GridSize / 2) / (double)(GridSize - 1);

    /* Compute (shared) input tables:  */
	/* NOW we call the driver for a fut to fill in! */

	thePTErr = PTNewEmpty (3, dim, 3, &thePT);
	if (thePTErr != KCP_SUCCESS)
		return -1;

	/* now get the input tables and fill'er up */

	thePTErr = PTGetItbl (thePT, -1, FUT_XCHAN, (KcmHandle *) &itbl_l);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}
	CalcItbl (itbl_l, GridSize, xfun_L, neutralgrid);

	thePTErr = PTGetItbl (thePT, -1, FUT_YCHAN, (KcmHandle *) &itbl_a);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}
	CalcItbl (itbl_a, GridSize, yfun_a, neutralgrid);

	thePTErr = PTGetItbl (thePT, -1, FUT_ZCHAN, (KcmHandle *) &itbl_b);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}
	CalcItbl (itbl_b, GridSize, zfun_b, neutralgrid);

    /* Compute channel-specific tables:  */

	/* Now lets use the soon to be new to the fut library call
		fut_calc_grid3 shall we? Ahhh but first we must already
		have a set of grid tables to calculate into. */

	thePTErr  = PTGetGtbl (thePT, FUT_XCHAN, &nDim, dim, (KcmHandle *) &gtbl_u);
	thePTErr |= PTGetGtbl (thePT, FUT_YCHAN, &nDim, dim, (KcmHandle *) &gtbl_v);
	thePTErr |= PTGetGtbl (thePT, FUT_ZCHAN, &nDim, dim, (KcmHandle *) &gtbl_L);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}

	CalcGrid3 (gtbl_u, gtbl_v, gtbl_L, GridSize, gridcalcUVL, neutralgrid, isAbsolute);

	/* Finally the Output tables */

	thePTErr = PTGetOtbl (thePT, FUT_XCHAN, (KcmHandle *) &otbl_u);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}
	CalcOtbl (otbl_u, uUVLmap, isAbsolute);

	thePTErr = PTGetOtbl (thePT, FUT_YCHAN, (KcmHandle *) &otbl_v);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut(thePT);
		return -1;
	}
	CalcOtbl(otbl_v, vUVLmap, isAbsolute);

	thePTErr = PTGetOtbl (thePT, FUT_ZCHAN, (KcmHandle *) &otbl_L);
	if (thePTErr != KCP_SUCCESS) {
		PTCheckOut (thePT);
		return -1;
	}
	CalcOtbl (otbl_L, LUVLmap, isAbsolute);

	*aPT = thePT;
	return 0;
}

/***** Floating Point Macros ********/
/***** for Shaper Matrix Code *******/
#if defined (SP_FPU)
#define HCIE HCIEFPU
#define BuBvBL2XYZ BuBvBL2XYZFPU
#define SuSvSL2XYZ SuSvSL2XYZFPU
#define NormXYZtoLab NormXYZtoLabFPU
#define ComputeLab ComputeLabFPU
#define SolveMat SolveMatFPU
#define ComputeShaper ComputeShaperFPU
#define ComputeShaperEx ComputeShaperExFPU

#define ComputeLabError ComputeLabErrorFPU
#define NewSearchDirection NewSearchDirectionFPU
#define SearchLab SearchLabFPU
#define ComputeMatrix ComputeMatrixFPU
#define ComputeMatrixEx ComputeMatrixExFPU
#define PostNormalize PostNormalizeFPU
#define ComputeShaperMatrix ComputeShaperMatrixFPU
#define ComputeShaperMatrixEx ComputeShaperMatrixExFPU
#define	BXYZ2BLab BXYZ2BLabFPU
#define Lab2NormXYZ Lab2NormXYZFPU
#define	US12XYZ2US12Lab US12XYZ2US12LabFPU
#define	MakeGamma MakeGammaFPU
#endif

/*------------------------------------------------------------------------*/
#if defined (SP_FPU)
static double HCIEFPU (double t)
#else
static double HCIE (double t)
#endif
{
	const double CIE_exp = 0.333333333333;
	
	/* CIE visual-response function from pre-normalized XYZ */

	if (t <= 0.008856)
		return 9.033 * t;
	else
		return (1.16 * POW (t, CIE_exp)) - 0.16;
}
/* ---------------------------------------------------------------------- */
#if defined (SP_FPU)
double MakeGammaFPU (double g, double x)
#else
double MakeGamma (double g, double x)
#endif
{
	return POW (x, g);
}

/* ---------------------------------------------------------------------- */
#if defined (SP_FPU)
static void BuBvBL2XYZFPU (
#else 
static void BuBvBL2XYZ (
#endif
				KpUInt8_t	Bu,
				KpUInt8_t	Bv,
				KpUInt8_t	BL,
				double		FAR *XPtr,
				double		FAR *YPtr,
				double		FAR *ZPtr)
{
	double	u, v, L;
	double	X, Y, Z;

	u = 0.41 * (double)Bu / 255.0 + .07;
	v = 0.42 * (double)Bv / 255.0 + .165;
	L = (double)BL / 2.55;

/* convert u',v', L* to X, Y, Z */
	Y = L / 100.0;
	if (Y <= 0.08)
		Y = Y / 9.033;
	else {
		Y = (Y + 0.16) / 1.16;
		Y = Y * Y * Y;
	}

	Y *= 100;		/* official normalization */
	X = (9 * u * Y) / (4 * v);
	Z = ((3 - 0.75 * u) / v - 5) * Y;

	*XPtr = X;
	*YPtr = Y;
	*ZPtr = Z;
}
/* ---------------------------------------------------------------------- */
#if defined (SP_FPU)
static void NormXYZtoLabFPU (
#else
static void NormXYZtoLab (
#endif
				double	X,
				double	Y,
				double	Z,
				double	FAR *L,
				double	FAR *a,
				double	FAR *b)
{
/* Converts normalized XYZ to Lab */
	double	fx, fy, fz;

	fx = HCIE (X);
	fy = HCIE (Y);
	fz = HCIE (Z);
	*a = 431.0 * (fx-fy);
	*b = 172.4 * (fy-fz);
	*L = 100 * fy;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 * This function will convert byte encoded XYZ into byte encoded Lab
 *
 * INPUT VARIABLES
 *      KpInt32_t numPels       - number of pels to convert (# of XYZ triplets)
 *      KpUInt8_t       *bXYZ   - pointer to byte encoded XYZ data (interleave format)
 *      KpUInt8_t       *bLab   - pointer to bytes encoded Lab buffer (interleave format)
 *
 * OUTPUT VARIABLES
 *      KpUInt8_t       *bLab   - fills buffer with byte encoded Lab data
 *
 *
 * AUTHOR
 * stanp
 *
 *------------------------------------------------------------------*/
#if defined (SP_FPU)
static void BXYZ2BLabFPU(
#else
static void BXYZ2BLab(
#endif
		KpInt32_t numPels, KpUInt8_t  *bXYZ, KpUInt8_t *bLab)
{
        int i;
        double  X,Y,Z,L,a,b;
        KpUInt8_t       *input, *output;
 
        input = bXYZ;
        output = bLab;
 
        for (i = 0; i < numPels; i++)
        {
                /* convert byte XYZ to doubles */
                X = (double)*input++;
                Y = (double)*input++;
                Z = (double)*input++;
 
                X = X/255.0;
                Y = Y/255.0;
                Z = Z/255.0;
                /* convert XYZ to Lab */
                NormXYZtoLab(X, Y, Z, &L, &a, &b);
 
                /* convert double Lab to byte Lab */
 
                *output++ = (KpUInt8_t)(L * 2.55 + 0.5);
                *output++ = (KpUInt8_t)(a + 128.0 + 0.5);
                *output++ = (KpUInt8_t)(b + 128.0 + 0.5);
        }
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 * This function will convert UShort12 encoded XYZ into UShort12 encoded Lab
 *
 * INPUT VARIABLES
 *      KpInt32_t numPels       - number of pels to convert (# of XYZ triplets)
 *      KpUInt16_t      *pXYZ   - pointer to UShort12 encoded XYZ data (interleave f
ormat)
 *      KpUInt16_t      *pLab   - pointer to UShort12 encoded Lab buffer (interleave
 format)
 *
 * OUTPUT VARIABLES
 *      KpUInt16_t      *pLab   - fills buffer with byte encoded Lab data
 *
 *
 * AUTHOR
 * stanp
 *
 *------------------------------------------------------------------*/
#if defined (SP_FPU)
static void US12XYZ2US12LabFPU (
#else
static void US12XYZ2US12Lab (
#endif
				KpInt32_t	numPels, 
			    KpUInt16_t	*pXYZ, 
			    KpUInt16_t	*pLab)
{
	int i;
	double  X,Y,Z,L,a,b;
	KpUInt16_t      *input, *output;
 
	input = pXYZ;
	output = pLab;
 
	for (i = 0; i < numPels; i++)
	{
		/* convert UShort12 XYZ to doubles */
		X = (double)*input++;
		Y = (double)*input++;
		Z = (double)*input++;
 
		X = X/4080.0;
		Y = Y/4080.0;
		Z = Z/4080.0;
 
		/* convert XYZ to Lab */
		NormXYZtoLab(X, Y, Z, &L, &a, &b);
  
		/* convert double Lab to UShort12 Lab */
 
		*output++ = (KpUInt16_t)(L * 40.80 + 0.5);
		*output++ = (KpUInt16_t)((a + 128.0) * 16.0 + 0.5);
		*output++ = (KpUInt16_t)((b + 128.0) * 16.0 + 0.5);
	}
}

static const double  aStarFactor = (500.0 / 1.16);
static const double  bStarFactor = (200.0 / 1.16);
static const double y2lStarSlope     = 9.03296296296296;
static const double     minXYZ  = -0.5;
static const double     maxXYZ  = 200.0;

static double KdsColorLstar2NormY(double val)
{
        double temp;
 
        if (val <= 0.08) {
                return(val / y2lStarSlope);
        } else {
                temp = (val + 0.16) / 1.16;
                return(temp * temp * temp);
        }
}

static double ClipXYZ(double xyz)
{
        if (xyz > maxXYZ) {
                xyz = maxXYZ;
        } else if (xyz < minXYZ) {
                xyz = minXYZ;
        }
        return(xyz);
}

/* ---------------------------------------------------------------------- */
#if defined (SP_FPU)
static void SuSvSL2XYZFPU (
#else 
static void SuSvSL2XYZ (
#endif
				KpUInt16_t	Bu,
				KpUInt16_t	Bv,
				KpUInt16_t	BL,
				double		FAR *XPtr,
				double		FAR *YPtr,
				double		FAR *ZPtr)
{
	double	u, v, L;
	double	X, Y, Z;

	u = 0.41 * (double)Bu / 4080.0 + .07;
	v = 0.42 * (double)Bv / 4080.0 + .165;
	L = (double)BL / 40.80;

/* convert u',v', L* to X, Y, Z */
	Y = L / 100.0;
	if (Y <= 0.08)
		Y = Y / 9.033;
	else {
		Y = (Y + 0.16) / 1.16;
		Y = Y * Y * Y;
	}

	Y *= 100;		/* official normalization */
	X = (9 * u * Y) / (4 * v);
	Z = ((3 - 0.75 * u) / v - 5) * Y;

	*XPtr = X;
	*YPtr = Y;
	*ZPtr = Z;
}

/* ---------------------------------------------------------------------- */

#if defined (SP_FPU)
static void Lab2NormXYZFPU(
#else
static void Lab2NormXYZ(
#endif
			double  L,
			double  a,
			double  b,
			double  FAR *X,
			double  FAR *Y,
			double  FAR *Z)
{
double  fx, fy, fz;
 
	fy = L / 100.0;
	fx = a / aStarFactor + fy;
	fz = b / (-bStarFactor) + fy;
 
	*X = ClipXYZ(KdsColorLstar2NormY(fx));
	*Y = ClipXYZ(KdsColorLstar2NormY(fy));
	*Z = ClipXYZ(KdsColorLstar2NormY(fz));
}


/* ---------------------------------------------------------------------- */
#if defined (SP_FPU)
static void ComputeLabFPU (
#else
static void ComputeLab (
#endif
				double	A[6],
				double	R,
				double	G,
				double	B,
				double	FAR *L,
				double	FAR *a,
				double	FAR *b)
{
/*
 * computes LAB from the parameter matrix
 * The matrix is normalized so that the resulting XYZ's
 * are each normalized to 1
 */
	
	double	ColorMatrix [3] [3], RGB [3], XYZ [3];
	int		i,j;
	
/* reconstruct the matrix */
	ColorMatrix [0] [0] = 1.0 - A [0] - A [1];	/* X */
	ColorMatrix [1] [0] = A [0];
	ColorMatrix [2] [0] = A [1];
	ColorMatrix [0] [1] = A [2];				/* Y */
	ColorMatrix [1] [1] = 1.0 - A [2] - A [3];
	ColorMatrix [2] [1] = A [3];
	ColorMatrix [0] [2] = A [4];				/* Z */
	ColorMatrix [1] [2] = A [5];
	ColorMatrix [2] [2] = 1.0 - A [4] - A [5];
	
/* Compute */
	RGB [0] = R;		RGB [1] = G;		RGB [2] = B;

/* Compute normalized XYZ */
	for (i = 0; i < 3; i++) {	/* loop over XYZ */
		XYZ [i] = 0;
		for (j = 0; j < 3; j++)
			XYZ [i] += RGB [j] * ColorMatrix [j] [i];
	}

/* Compute Lab */
	NormXYZtoLab (XYZ [0], XYZ [1], XYZ [2], L, a, b); 
}	

/* ---------------------------------------------------------------------- */
#if defined (SP_FPU)
SpStatus_t SolveMatFPU (
#else
SpStatus_t SolveMat (
#endif
				double	FAR *FAR *mat,
				int		DimR,
				int		DimC)
{
	int		i, r, c;
	double	pivot;
	double	factor;
	const	double limit = 1.e-6;

	for (i = 0; i < DimR; i++) {
		pivot = mat [i] [i];
		if ((pivot > -limit) && (pivot < limit))	/* check for singularity */
			return SpStatOutOfRange;

	/* set the pivot point to 1.0 */
		for (c = 0; c < DimC; c++)
			mat [i] [c] /= pivot;

	/* set the zeros in the columns of the non-pivot rows */
		for (r = 0; r < DimR; r++) {
			if (r == i)
				continue;

			factor = mat [r] [i];
			for (c = 0; c < DimC; c++)
				mat [r] [c] -= factor * mat [i] [c];
		}
	}
	return SpStatSuccess;
}


/* ------------------------------------------------------------------------ */
#if defined (SP_FPU)
static SpStatus_t ComputeShaperFPU (
#else
static SpStatus_t ComputeShaper (
#endif
				PTRefNum_t	pt,
				double		FAR *shaper [3],
				double		wht [3])
{
	KpUInt16_t	*Pels, *p, avg_u, avg_v;
	int			i,j;
	SpStatus_t	spErr = SpStatSuccess;
	int			center = SHAPERCURVESIZE / 2;
	int			ml = center, mh = center;
	int32		sum [2], N;
	
/* Allocate memory */
	Pels = SpMalloc (3*SHAPERCURVESIZE * sizeof(*Pels));
	if (NULL == Pels)
		return SpStatMemory;
	
/* Compute ramp */
	for (i = 0; i < 3; i++)			/* Loop over colors */
		for (j = 0, p = Pels+i; j < SHAPERCURVESIZE; j++, p += 3)
			*p = (KpUInt16_t)(j*16);

/* Transform colors */
	spErr = Transform12BPels (pt, Pels, SHAPERCURVESIZE);
	if (spErr != SpStatSuccess) {
		SpFree (Pels);
		return spErr;
	}
	
/*
 * Find limits of monotonic region
 * (Note - because of truncation, constancy does not mean clipping)
 */

/* go down until finding a reversal */
	for (i = center-1, p = Pels+2+3*i;
			(i >=0) && (*p <= *(p+3));
					i--, p -= 3)
		ml = i;			/* no reversal yet */

/* head back up, until function starts rising */
	for (i = ml+1,p = Pels+2+3*i;
			(i < center) && (*p == *(p-3));
					i++, p += 3)
		ml = i;			/* is still constant */
		
/* go up until finding a reversal */
	for (i = center, p = Pels+2+3*i;
			i < SHAPERCURVESIZE && *p >= *(p-3);
					i++, p += 3)
		mh = i;			/* is still going up */

/* head back down, until function starts falling */
	for (i = mh-1, p = Pels+2+3*i;
			i >= center && *p == *(p+3);
					i--, p -= 3)
		mh = i;			/* is still constant */
	
/* Compute effective white value - avg u,v and max L */
	sum[0] = 0;
	sum[1] = 0;
	for (i = center, p = Pels+3*i; i <= mh; i++, p += 3) {
		sum [0] += *p;		/* sum u */
		sum [1] += *(p+1);	/* sum v */
	}
	N = mh-center+1;
			
	avg_u = (KpUInt16_t) ((sum [0] + N/2) / N);
	avg_v = (KpUInt16_t) ((sum [1] + N/2) / N);
	SuSvSL2XYZ (avg_u, avg_v, Pels [2 + mh*3], &wht [0], &wht [1], &wht [2]);

	for (i = 0; i < 3; i++)	{		/* better safe than sorry */
		if (wht [i] <= 0) {
			SpFree (Pels);		/* clean up */
			return SpStatOutOfRange;	/* can't go on */
		}
	}
	
/* Within monotonic region, shaper = XYZ value/white */
/* loop over monotonic region */
	for (i = ml, p = Pels+3*ml;
			i <= mh;
					i++, p += 3) {
		SuSvSL2XYZ (p [0], p [1], p [2],
					&shaper [0] [i], &shaper [1] [i], &shaper [2] [i]);

		for (j = 0; j < 3; j++)		/* loop over color */
			shaper [j] [i] /= wht [j];		/* normalize */
	}
	
/* Extend the edges */
	for (i = 0; i < ml; i++)
		for (j = 0; j < 3; j++)
			shaper [j] [i] = shaper [j] [ml];	/* extend to left */
	for (i = mh+1; i < SHAPERCURVESIZE; i++)
		for (j = 0; j < 3; j++)
			shaper [j] [i] = shaper [j] [mh];	/* extend to right */
			
/* Clean up */
	SpFree (Pels);
	return spErr;
}

/* ---------------------------------------------------------------------- */
#if defined (SP_FPU)
static double ComputeLabErrorFPU (
#else
static double ComputeLabError (
#endif
				double	A [6],
				double	FAR *shaped_rgb [3],
				double	FAR *Lab_grid [3],
				int		totalGrid)
{
/* Compute value and error */
	
	int		i, j;
	double	Lab [3], delta, error;
	
/* Compute error */
	error = 0;
	for (i = 0; i < totalGrid; i++)	{	/* Loop over data points */

	/* Compute the Lab value */
		ComputeLab (A, shaped_rgb [0] [i], shaped_rgb [1] [i],
					shaped_rgb [2] [i], &Lab [0], &Lab [1], &Lab [2]);
		for (j = 0; j < 3 ; j++)	{
			delta = Lab_grid [j] [i] - Lab [j];
			error += delta*delta;
		}
	}		/* Next data value */

/* Average error */
	error /= (double)(3 * totalGrid);
	return error;
}

/* ---------------------------------------------------------------------- */
/*	Use Levenberg-Marquard method for determining a new search direction */
#if defined (SP_FPU)
static KpBool_t NewSearchDirectionFPU (
#else
static KpBool_t NewSearchDirection (
#endif
				double	A [6],
				double	FAR *shaped_rgb [3],
				double	FAR *Lab_grid [3],
				int		Npts,
				double	deltaA [6])
{
	double	*JJ[6], jjBuf[42], J[6][3], maxDiag, diagLimit;
	int		ia, ja, kPt, i;
	double	Lab[3], newLab[3];
	double	newA[6], delta[6];
	
	const double delLimit = 0.00001;
	const double LM = 0.025;
	
/* Assign pointer array */
	for (ia = 0; ia < 6; ia++)
		JJ [ia] = jjBuf + ia*7;

	for (i =0; i < 42; i++)
		jjBuf [i] = 0;
	
/* Set up delta values for dirivative */
	for (ia = 0; ia < 6; ia++) {
		delta [ia] = .001 * A [ia];
		if (delta [ia] < 0)
			delta [ia] = -delta [ia];
		if (delta [ia] < delLimit)
			delta [ia] = delLimit;
	}
		
/* Compute the Jacobian arrays */
	for (kPt = 0; kPt < Npts; kPt++) {		/* Loop over points */
		ComputeLab (A, shaped_rgb [0] [kPt], shaped_rgb [1] [kPt],
					shaped_rgb [2] [kPt], &Lab [0], &Lab [1], &Lab [2]);

	/* Compute the dirivatives at this point */
		for (ia = 0; ia < 6; ia++)	{
			for (ja = 0; ja < 6; ja++)		/* Set up incremental A */
				newA [ja] = A [ja];

			newA [ia] += delta [ia];
			ComputeLab (newA,
						shaped_rgb [0] [kPt],
						shaped_rgb [1] [kPt],
						shaped_rgb [2] [kPt],
						&newLab [0], &newLab [1], &newLab [2]);

			for (i = 0; i < 3; i++) {
			/* dirivative at this point */
				J [ia] [i] = (newLab [i]-Lab [i]) / delta [ia];
			}
		}

	/* Add contribution to the J2 array */
		for (ia = 0; ia < 6; ia++)	{		/* Loop over rows of JJ */

		/* dirivative squared */
			for (ja = 0; ja < 6; ja++)
				for (i = 0; i < 3; i++)
					JJ [ia] [ja] += J [ia] [i] * J [ja] [i];

		/* error in function */
			for (i = 0; i < 3; i++)
				JJ [ia] [6] += J [ia] [i] * (Lab_grid [i] [kPt]-Lab [i]);

		}		/* Next row of J2 array */	

	}		/* next grid point */
	
/* Normalize */
	for (ia = 0; ia < 6; ia++)
		for (ja = 0; ja < 7; ja++)
			JJ [ia] [ja] /= (double)(3*Npts);
			
/* Adjust diagonal value */
	maxDiag = 0;
	for (ia = 0; ia < 6; ia++)
		if (JJ [ia] [ia]>maxDiag)
			maxDiag = JJ [ia] [ia];

	if (maxDiag <= 1.e-6)
		return KPFALSE;

	diagLimit = maxDiag * 0.01;
	for (ia = 0; ia < 6; ia++)
		JJ [ia] [ia] += LM * (JJ [ia] [ia] > diagLimit
												? JJ [ia] [ia] : diagLimit);
	
/* invert J2 to determine new search direction */
	if (SolveMat (JJ, 6, 7) != SpStatSuccess)
		return KPFALSE;

	for (ia = 0; ia < 6; ia++)
		deltaA [ia] = (1+LM)*JJ [ia] [6];
	
	return KPTRUE;
}

/* ---------------------------------------------------------------------- */
#if defined (SP_FPU)
static SpStatus_t SearchLabFPU (
#else
static SpStatus_t SearchLab (
#endif
				double	A [6],
				double	FAR *shaped_rgb [3],
				double	FAR *Lab_grid [3],
				int		totalGrid)
{
	int			i, istep;
	double		labError, labError0, newError;
	KpBool_t	notDone = KPTRUE, goingDown;
	double		newA [6], deltaA [6], scale, lastScale = 1.0;

	const int	NSteps = 5;
	const double tolerance = 1.0;

/* Compute initial error */
	labError0 = ComputeLabError (A, shaped_rgb, Lab_grid, totalGrid);
	if (labError0 < tolerance)
		return SpStatSuccess;
	
	labError = labError0;

/* keep looking until error stops going down */
	do {
		/* Get a new search direction */
		notDone = NewSearchDirection (A, shaped_rgb, Lab_grid,
											totalGrid, deltaA);
		if (notDone) {
		/*
		 * Head off in the new direction taking smaller steps,
		 * until error goes down
		 */

		/*keep trying, with smaller steps*/
			for (istep = 1, goingDown = KPFALSE, scale=1.0;
					(istep <= NSteps) && !goingDown;
							istep++, scale *= 0.5) {
				for (i = 0; i < 6; i++)
					newA [i] = A [i] + scale * deltaA [i];	/* try this one */
				newError = ComputeLabError (newA, shaped_rgb,
												Lab_grid, totalGrid);
				goingDown = (KpBool_t) (labError-newError > tolerance);
				if (goingDown) {	/* it got better */
					labError = newError;
					lastScale = scale;
				}
			}		/* end of steps along search path */

		/* it did not get much better */
			if (labError0 - labError < tolerance)
				notDone = KPFALSE;		/* no more to do */
			else {
				labError0 = labError;
				for (i = 0; i < 6; i++)
					A [i] += lastScale * deltaA [i];		/* update A */
				notDone = (KpBool_t) (labError0 > tolerance);
			}		/* End of linear search */
		}
	} while (notDone);		/*try another direction*/

	return SpStatSuccess;
}

/* ------------------------------------------------------------------------ */
#if defined (SP_FPU)
static SpStatus_t ComputeMatrixFPU (
#else
static SpStatus_t ComputeMatrix (
#endif
				PTRefNum_t	pt,
				double		FAR *shaper [3],
				double		wht [3],
				double		ColorMatrix [3] [3])
{
	int			ml, mh;
	KpUInt8_t	*rgbgrid, *rgbValue, *rcsValue, rv,gv, bv;
	double		*shaped_rgb[3];
	double		*xyz_grid[3];
	const int	gridSize = 5, totalGrid = gridSize * gridSize * gridSize;
	int			i, j, k;
	KpBool_t	memOK;
	SpStatus_t	spErr;
	double		cBuf[18], *correlation[3], norm;
	double		A[6];

	xyz_grid[0] = 0;
	xyz_grid[1] = 0;
	xyz_grid[2] = 0;
	shaped_rgb[0] = 0;
	shaped_rgb[1] = 0;
	shaped_rgb[2] = 0;

	correlation [0] = cBuf;
	correlation [1] = cBuf+6;
	correlation [2] = cBuf+12;
	
/* find limits on shaper for monotonic behavior in Y */
	for (ml = 0;
			(ml < SHAPERCURVESIZE-1) && (shaper [2] [ml] == shaper [2] [ml+1]);
					ml++)
		;
	for (mh = SHAPERCURVESIZE-1;
			(mh > 1) && (shaper [2] [mh] == shaper [2] [mh-1]);
					mh--)
		;
	
/* Allocate memory */
	rgbgrid = SpMalloc (3 * totalGrid * sizeof (*rgbgrid));
	if (NULL == rgbgrid)
		return SpStatMemory;

	for (i = 0, memOK = KPTRUE;
			(i < 3) && memOK;
					i++) {		
		shaped_rgb [i] = SpMalloc (totalGrid * sizeof (double));
		memOK &= (NULL != shaped_rgb[i]);
		if (memOK) {
			xyz_grid [i] = SpMalloc (totalGrid * sizeof (double));
			memOK &= (NULL != xyz_grid [i]);
		}
	}	/* next color */

/* check for memory failure */
	if (!memOK) {
		SpFree (rgbgrid);
		for (i = 0; i < 3; i++) {
			SpFree (shaped_rgb [i]);
			SpFree (xyz_grid [i]);
		}
		return SpStatMemory;
	}
	
/* Fill rgb grid */
	rgbValue = rgbgrid;
	for (i = 0; i < gridSize; i++) {						/* r values */
		rv = (KpUInt8_t) ( (ml * gridSize + i * (mh-ml)) / gridSize );	
		for (j = 0; j < gridSize; j++)	{					/* g values */
			gv = (KpUInt8_t) ( (ml * gridSize + j * (mh-ml)) / gridSize );	
			for (k = 0; k < gridSize; k++) {				/* b values */
				bv = (KpUInt8_t) ( (ml * gridSize + k * (mh-ml)) / gridSize );
				*rgbValue++ = rv;
				*rgbValue++ = gv;
				*rgbValue++ = bv;
			}
		}
	}
	
/* Compute shaped values */
	rgbValue = rgbgrid;
	for (i = 0; i < totalGrid; i++) {	/* loop over gridpoints */
		for (j = 0; j < 3; j++) {		/* loop over colors */

		/* look up color in the shaper */
			shaped_rgb [j] [i] = shaper [j] [*rgbValue++];
		}
	}

/* Transform the grid */
	spErr = TransformPels (pt, rgbgrid, totalGrid);
	if (spErr != SpStatSuccess) {
		SpFree (rgbgrid);
		for (i = 0; i < 3; i++)	{
			SpFree (shaped_rgb [i]);
			SpFree (xyz_grid [i]);
		}
		return spErr;
	}
	
/*--------------------------------*/
/* Compute transformed XYZ values */
/*--------------------------------*/
	for (i = 0, rcsValue = rgbgrid;
			i < totalGrid;
					i++, rcsValue += 3) {	/* loop over gridpoints */
		BuBvBL2XYZ (rcsValue [0], rcsValue [1], rcsValue [2],
					&xyz_grid [0] [i], &xyz_grid [1] [i], &xyz_grid [2] [i]);
	}

/* Release initial grid values */
	SpFree (rgbgrid);
	
/* Compute initial estimate */
	
/* Correlation matrix's - pack into 3x6 array */
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)	{
			correlation [i] [j] = 0;
			correlation [i] [j+3] = 0;
			for (k = 0; k < totalGrid; k++)	{	/* loop over grid points */
				correlation [i] [j] += shaped_rgb [i] [k] * shaped_rgb [j] [k];
				correlation [i] [j+3] += shaped_rgb [i] [k] * xyz_grid [j] [k];
			}
		}
	}

/* solve RGB_RGB * ColorMatrix = RGB_XYZ */
	spErr = SolveMat (correlation, 3,6);		
	if (spErr != SpStatSuccess) {
		for (i = 0; i < 3; i++) {
			SpFree (shaped_rgb [i]);
			SpFree (xyz_grid [i]);
		}
		return spErr;
	}	

	for (i = 0; i < 3; i++)				/* loop over rgb */
		for (j = 0; j < 3; j++)			/* loop over XYZ */
			ColorMatrix [i] [j] = correlation [i] [j+3];

/* Normalize matrix */
	for (i = 0; i < 3; i++)	{		/* loop over XYZ */
		norm = 0;
		for (j = 0; j < 3; j++)
			norm += ColorMatrix [j] [i];	/* sum over RGB */
		for (j = 0; j < 3; j++)				/* loop over RGB */
			ColorMatrix [j] [i] /= norm;	/* normalize to unit sum */
	}

/*-----------------------*/
/* Perform search in Lab */
/*-----------------------*/
	
/* Convert result vestor to Lab */
	for (k = 0; k < totalGrid; k++)		/* loop over grid points */
		NormXYZtoLab (xyz_grid [0] [k] / wht [0],
						xyz_grid [1] [k] / wht [1],
						xyz_grid [2] [k] / wht [2],
						&xyz_grid[0][k], &xyz_grid[1][k], &xyz_grid[2][k]);
	
/* Encode normalized matrix	 */
	A [0] = ColorMatrix [1] [0];
	A [1] = ColorMatrix [2] [0];
	A [2] = ColorMatrix [0] [1];
	A [3] = ColorMatrix [2] [1];
	A [4] = ColorMatrix [0] [2];
	A [5] = ColorMatrix [1] [2];
	
/* Call non-linear search routine */
	spErr = SearchLab (A, shaped_rgb, xyz_grid, totalGrid);
	
	if (spErr == SpStatSuccess) {		/* it turned out OK */
		/* reconstruct and normalize the matrix */
		ColorMatrix [0] [0] = wht [0] * (1.0 - A [0] - A [1]);	/* X */
		ColorMatrix [1] [0] = wht [0] * A [0];
		ColorMatrix [2] [0] = wht [0] * A [1];
		ColorMatrix [0] [1] = wht [1] * A [2];					/* Y */
		ColorMatrix [1] [1] = wht [1] * (1.0 - A [2] - A [3]);
		ColorMatrix [2] [1] = wht [1] * A [3];
		ColorMatrix [0] [2] = wht [2] * A [4];					/* Z */
		ColorMatrix [1] [2] = wht [2] * A [5];
		ColorMatrix [2] [2] = wht [2] * (1.0 - A [4] - A [5]);
	}
		
/* Clean up */
	for (i = 0; i < 3; i++)	{
		SpFree (shaped_rgb [i]);
		SpFree (xyz_grid [i]);
	}
	
	return spErr;
}
/*------------------------------------------------------------------------*/
static SpStatus_t Transform12BitPelsEx (
				SpXform_t	Xform,
				KpUInt16_t	FAR *Pels,
				int			nPels)
{
/*
 * Transform the pel array using the xform
 * The array is assumes to be in plane-sequential order
 * RGB input, Lab or XYZ output
 */

	SpPixelLayout_t	layout;
	SpStatus_t		status;
	int			i;

	layout.SampleType = SpSampleType_UShort12;
	layout.NumCols = nPels;
	layout.NumRows = 1;
	layout.OffsetColumn = 3 * sizeof(*Pels);
	layout.OffsetRow = nPels * 3 * sizeof (*Pels);
	layout.NumChannels = 3;

	for (i = 0; i < 3; i++) {
		layout.BaseAddrs [i] = (void FAR *) (Pels+i);
	}

	status = SpEvaluate(Xform, &layout, &layout, NULL, NULL);
	return status;
}

/* ------------------------------------------------------------------------ */
#if defined (SP_FPU)
static SpStatus_t PostNormalizeFPU (
#else
static SpStatus_t PostNormalize (
#endif
		double		FAR *shaper [3],
		double		ColorMatrix [3] [3])
{
	KpInt16_t i, j;
	double maxShaper, YWhite = 0; 


	/* for each of the 3 shapers, find the largest value */
	/* if it is <= 1.0 - everything is ok */
	/* if not, divide shaper by this value, and */
	/* multiply the corresponding matrix row by the same value */
	/* i = r,g, or b shapers */
	for (i=0; i<3; i++) {
		maxShaper = 0;
		/* find the largest value in the shaper */
		for (j=0; j<256; j++) {
			if (maxShaper < shaper[i][j])
				maxShaper = shaper[i][j];
		}

		/* all shaper values must be less than 1.0 to fit in a 16-bit integer ! */
	
		/* divide each shaper element by the largest value to normalize */
		for (j=0; j<256; j++) {
			shaper[i][j] /= maxShaper;
			if (shaper[i][j] >= 1.0) {
				shaper[i][j] = 0.99999999; /* this must be less than 1 !!! */
			}
		}

		/* multiply the corresponding matrix row by this value */
		for (j=0; j<3; j++)	{
			ColorMatrix[i][j] *= maxShaper;
		}
	}
	
	/* normalize the matrix so that the Y column sums to 1.0 */
	for (i = 0; i < 3; i++) {
		YWhite += ColorMatrix[i][1];
	}

	YWhite = 1.0/YWhite;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			ColorMatrix[i][j] *= YWhite;
		}
	}

	return SpStatSuccess;
}

/* ---------------------------------------------------------------------- */
static SpStatus_t TransformPelsEx (
				SpXform_t	Xform,
				KpUInt8_t	FAR *Pels,
				int			nPels)
{
/*
 * Transform the pel array using the xform
 * The array is assumes to be in plane-sequential order
 * RGB input, Lab or XYZ output
 */

	SpPixelLayout_t	layout;
	SpStatus_t		status;
	int			i;

	layout.SampleType = SpSampleType_UByte;
	layout.NumCols = nPels;
	layout.NumRows = 1;
	layout.OffsetColumn = 3 * sizeof(*Pels);
	layout.OffsetRow = nPels * 3 * sizeof (*Pels);
	layout.NumChannels = 3;

	for (i = 0; i < 3; i++) {
		layout.BaseAddrs [i] = (void FAR *) (Pels+i);
	}

	status = SpEvaluate(Xform, &layout, &layout, NULL, NULL);
	return status;
}
/*-----------------------------------------------------------------------*/
#if defined (SP_FPU)
static SpStatus_t ComputeMatrixExFPU (
#else
static SpStatus_t ComputeMatrixEx (
#endif
				SpXform_t	xform,
				double		FAR *shaper [3],
				double		wht [3],
				double		ColorMatrix [3] [3])
{
	int			ml, mh;
	KpUInt8_t	*rgbgrid, *rgbValue, *rcsValue, rv,gv, bv;
	double		*shaped_rgb[3];
	double		*xyz_grid[3];
	const int	gridSize = 5, totalGrid = gridSize * gridSize * gridSize;
	int			i, j, k;
	KpBool_t	memOK;
	SpStatus_t	spErr;
	SpXformDesc_t	Desc;
	double		cBuf[18], *correlation[3], norm;
	double		A[6];
	double		temp_L, temp_a, temp_b;

	xyz_grid[0] = 0;
	xyz_grid[1] = 0;
	xyz_grid[2] = 0;
	shaped_rgb[0] = 0;
	shaped_rgb[1] = 0;
	shaped_rgb[2] = 0;

	correlation [0] = cBuf;
	correlation [1] = cBuf+6;
	correlation [2] = cBuf+12;
	
/* find limits on shaper for monotonic behavior in Y */
	for (ml = 0;
			(ml < SHAPERCURVESIZE-1) && (shaper [2] [ml] == shaper [2] [ml+1]);
					ml++)
		;
	for (mh = SHAPERCURVESIZE-1;
			(mh > 1) && (shaper [2] [mh] == shaper [2] [mh-1]);
					mh--)
		;
	
/* Allocate memory */
	rgbgrid = SpMalloc (3 * totalGrid * sizeof (*rgbgrid));

	if (NULL == rgbgrid)
		return SpStatMemory;

	for (i = 0, memOK = KPTRUE;
			(i < 3) && memOK;
					i++) {		
		shaped_rgb [i] = SpMalloc (totalGrid * sizeof (double));
		memOK &= (NULL != shaped_rgb[i]);
		if (memOK) {
			xyz_grid [i] = SpMalloc (totalGrid * sizeof (double));
			memOK &= (NULL != xyz_grid [i]);
		}
	}	/* next color */

/* check for memory failure */
	if (!memOK) {
		SpFree (rgbgrid);
		for (i = 0; i < 3; i++) {
			SpFree (shaped_rgb [i]);
			SpFree (xyz_grid [i]);
		}

		return SpStatMemory;
	}
	
/* Fill rgb grid */
	rgbValue = rgbgrid;
	for (i = 0; i < gridSize; i++) {						/* r values */
		rv = (KpUInt8_t) ( (ml * gridSize + i * (mh-ml)) / gridSize );	
		for (j = 0; j < gridSize; j++)	{					/* g values */
			gv = (KpUInt8_t) ( (ml * gridSize + j * (mh-ml)) / gridSize );	
			for (k = 0; k < gridSize; k++) {				/* b values */
				bv = (KpUInt8_t) ( (ml * gridSize + k * (mh-ml)) / gridSize );
				*rgbValue++ = rv;
				*rgbValue++ = gv;
				*rgbValue++ = bv;
			}
		}
	}
	
/* Compute shaped values */
	rgbValue = rgbgrid;
	for (i = 0; i < totalGrid; i++) {	/* loop over gridpoints */
		for (j = 0; j < 3; j++) {		/* loop over colors */

		/* look up color in the shaper */
			shaped_rgb [j] [i] = shaper [j] [*rgbValue++];
		}
	}

/* Transform the grid */
	spErr = TransformPelsEx (xform, rgbgrid, totalGrid);
	if (spErr != SpStatSuccess) {
		SpFree (rgbgrid);
		for (i = 0; i < 3; i++)	{
			SpFree (shaped_rgb [i]);
			SpFree (xyz_grid [i]);
		}
		return spErr;
	}
	
	/* if output color space is XYZ, convert pels to Lab */
	spErr = SpXformGetDesc( xform, &Desc);
	if (spErr != SpStatSuccess)
	{
			SpFree (rgbgrid);
		for (i = 0; i < 3; i++)	{
			SpFree (shaped_rgb [i]);
			SpFree (xyz_grid [i]);
		}
		return spErr;
	}

	if (Desc.SpaceOut == SpSpaceXYZ)
	{
		BXYZ2BLab(totalGrid, rgbgrid, rgbgrid);
	}
	
/*--------------------------------*/
/* Compute transformed XYZ values */
/*--------------------------------*/
	for (i = 0, rcsValue = rgbgrid;i < totalGrid;i++, rcsValue += 3) 
	{	/* loop over gridpoints */
						temp_L = (double) (rcsValue[0]) / 2.55;
						temp_a = (double) (rcsValue[1]) - 128.0;
						temp_b = (double) (rcsValue[2]) - 128.0;
		Lab2NormXYZ(temp_L, temp_a, temp_b, 
			&xyz_grid [0] [i], &xyz_grid [1] [i], &xyz_grid [2] [i]);
	}

/* Release initial grid values */
	SpFree (rgbgrid);
	
/* Compute initial estimate */
	
/* Correlation matrix's - pack into 3x6 array */
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)	{
			correlation [i] [j] = 0;
			correlation [i] [j+3] = 0;
			for (k = 0; k < totalGrid; k++)	{	/* loop over grid points */
				correlation [i] [j] += shaped_rgb [i] [k] * shaped_rgb [j] [k];
				correlation [i] [j+3] += shaped_rgb [i] [k] * xyz_grid [j] [k];
			}
		}
	}

/* solve RGB_RGB * ColorMatrix = RGB_XYZ */
	spErr = SolveMat (correlation, 3,6);		
	if (spErr != SpStatSuccess) {
		for (i = 0; i < 3; i++) {
			SpFree (shaped_rgb [i]);
			SpFree (xyz_grid [i]);
		}
		return spErr;
	}	

	for (i = 0; i < 3; i++)				/* loop over rgb */
		for (j = 0; j < 3; j++)			/* loop over XYZ */
			ColorMatrix [i] [j] = correlation [i] [j+3];

/* Normalize matrix */
	for (i = 0; i < 3; i++)	{		/* loop over XYZ */
		norm = 0;
		for (j = 0; j < 3; j++)
			norm += ColorMatrix [j] [i];	/* sum over RGB */
		for (j = 0; j < 3; j++)				/* loop over RGB */
			ColorMatrix [j] [i] /= norm;	/* normalize to unit sum */
	}

/*-----------------------*/
/* Perform search in Lab */
/*-----------------------*/
	
/* Convert result vestor to Lab */
	for (k = 0; k < totalGrid; k++)		/* loop over grid points */
		NormXYZtoLab (xyz_grid [0] [k] / wht [0],
						xyz_grid [1] [k] / wht [1],
						xyz_grid [2] [k] / wht [2],
						&xyz_grid[0][k], &xyz_grid[1][k], &xyz_grid[2][k]);
	
/* Encode normalized matrix	 */
	A [0] = ColorMatrix [1] [0];
	A [1] = ColorMatrix [2] [0];
	A [2] = ColorMatrix [0] [1];
	A [3] = ColorMatrix [2] [1];
	A [4] = ColorMatrix [0] [2];
	A [5] = ColorMatrix [1] [2];
	
/* Call non-linear search routine */
	spErr = SearchLab (A, shaped_rgb, xyz_grid, totalGrid);
	
	if (spErr == SpStatSuccess) {		/* it turned out OK */
		/* reconstruct and normalize the matrix */
		ColorMatrix [0] [0] = wht [0] * (1.0 - A [0] - A [1]);	/* X */
		ColorMatrix [1] [0] = wht [0] * A [0];
		ColorMatrix [2] [0] = wht [0] * A [1];
		ColorMatrix [0] [1] = wht [1] * A [2];					/* Y */
		ColorMatrix [1] [1] = wht [1] * (1.0 - A [2] - A [3]);
		ColorMatrix [2] [1] = wht [1] * A [3];
		ColorMatrix [0] [2] = wht [2] * A [4];					/* Z */
		ColorMatrix [1] [2] = wht [2] * A [5];
		ColorMatrix [2] [2] = wht [2] * (1.0 - A [4] - A [5]);
	}
		
/* Clean up */
	for (i = 0; i < 3; i++)	{
		SpFree (shaped_rgb [i]);
		SpFree (xyz_grid [i]);
	}
	
	return spErr;
}
/* ------------------------------------------------------------------------ */
#if defined (SP_FPU)
static SpStatus_t ComputeShaperMatrixFPU (
#else
static SpStatus_t ComputeShaperMatrix (
#endif
				PTRefNum_t	pt,
				double		FAR *shaper [3],
				double		ColorMatrix [3] [3])
{
	double	wht [3];
	SpStatus_t	error;
		
	error = ComputeShaper (pt, shaper, wht);
	if (error == SpStatSuccess)
	{
		error = ComputeMatrix (pt, shaper, wht, ColorMatrix);
		if (error == SpStatSuccess)
			error = PostNormalize(shaper, ColorMatrix);
	}
	return error;
}
/*------------------------------------------------------------------------*/
#if defined (SP_FPU)
static SpStatus_t ComputeShaperExFPU (
#else
static SpStatus_t ComputeShaperEx (
#endif
				SpXform_t	xform,
				double		FAR *shaper [3],
				double		wht [3])
{
	KpUInt16_t	*Pels, *p, avg_a, avg_b;
	int			i,j;
	SpStatus_t	spErr = SpStatSuccess;
	SpXformDesc_t	Desc;
	int			center = SHAPERCURVESIZE / 2;
	int			ml = center, mh = center;
	int32		sum [2], N;
	double		temp_a, temp_b, temp_L;
	
/* Allocate memory */
	Pels = SpMalloc (3*SHAPERCURVESIZE * sizeof(*Pels));
	if (NULL == Pels)
		return SpStatMemory;
	
/* Compute ramp */
	for (i = 0; i < 3; i++)			/* Loop over colors */
		for (j = 0, p = Pels+i; j < SHAPERCURVESIZE ; j++, p += 3)
	{
			*p = (KpUInt16_t)(j * 16);
	}

/* Transform colors */
	spErr = Transform12BitPelsEx (xform, Pels, SHAPERCURVESIZE);
	if (spErr != SpStatSuccess) {
		SpFree (Pels);
		return spErr;
	}
	
/* if output color space is XYZ, convert pels to Lab */
	spErr = SpXformGetDesc( xform, &Desc);
	if (spErr != SpStatSuccess)
	{	
		SpFree (Pels);
		return spErr;
	}

	if (Desc.SpaceOut == SpSpaceXYZ)
	{
		US12XYZ2US12Lab(SHAPERCURVESIZE, Pels, Pels);
	}
	
/*
 * Find limits of monotonic region
 * (Note - because of truncation, constancy does not mean clipping)
 */

/* go down until finding a reversal */
	for (i = center-1, p = Pels+3*i;
			(i >=0) && (*p <= *(p+3));
					i--, p -= 3)
		ml = i;			/* no reversal yet */

/* head back up, until function starts rising */
	for (i = ml+1,p = Pels+3*i;
			(i < center) && (*p == *(p-3));
					i++, p += 3)
		ml = i;			/* is still constant */
		
/* go up until finding a reversal */
	for (i = center, p = Pels+3*i;
			i < SHAPERCURVESIZE && *p >= *(p-3);
					i++, p += 3)
		mh = i;			/* is still going up */

/* head back down, until function starts falling */
	for (i = mh-1, p = Pels+3*i;
			i >= center && (*p == *(p+3));
					i--, p -= 3)
		mh = i;			/* is still constant */
	
/* Compute effective white value - max L and avg a,b */
	sum[0] = 0;
	sum[1] = 0;
	for (i = center, p = Pels+1+3*i; i <= mh; i++, p += 3) {
		sum [0] += *p;		/* sum a */
		sum [1] += *(p+1);	/* sum b */
	}
	N = mh-center+1;
			
	avg_a = (KpUInt16_t) ((sum [0] + N/2) / N);
	avg_b = (KpUInt16_t) ((sum [1] + N/2) / N);
	temp_a = (double)(avg_a)/16.0 - 128.0;
	temp_b = (double)(avg_b)/16.0 - 128.0;
	temp_L = (double)(Pels [mh*3])/40.80;

	Lab2NormXYZ(temp_L, temp_a, temp_b, &wht [0], &wht [1], &wht [2]);

	for (i = 0; i < 3; i++)	{			/* better safe than sorry */
		if (wht [i] <= 0) {
			SpFree (Pels);				/* clean up */
			return SpStatOutOfRange;	/* can't go on */
		}
	}
	
/* Within monotonic region, shaper = XYZ value/white */
/* loop over monotonic region */
	for (i = ml, p = Pels+3*ml;i <= mh;i++, p += 3)
	{
		temp_a = (double)(p[1])/16.0 - 128.0;
		temp_b = (double)(p[2])/16.0 - 128.0;
		temp_L = (double)(p[0]) / 40.80;

		Lab2NormXYZ (temp_L, temp_a, temp_b,
					&shaper [0] [i], &shaper [1] [i], &shaper [2] [i]);

		for (j = 0; j < 3; j++)	/* loop over color */
		{
			shaper [j] [i] /= wht [j];		/* normalize */
			if (shaper [j] [i] < 0.0)
				shaper [j] [i] = 0.0;		/* don't allow < 0.0 */

		}
	}
	
/* Extend the edges */
	for (i = 0; i < ml; i++)
		for (j = 0; j < 3; j++)
			shaper [j] [i] = shaper [j] [ml];	/* extend to left */
	for (i = mh+1; i < SHAPERCURVESIZE; i++)
		for (j = 0; j < 3; j++)
			shaper [j] [i] = shaper [j] [mh];	/* extend to right */
			
/* Clean up */
	SpFree (Pels);
	return spErr;
}
/*----------------------------------------------------------------------*/
/*	ComputeShaperMatrixEx is called by SpXformCreateMatTagsFromXform    */
#if defined (SP_FPU)
static SpStatus_t ComputeShaperMatrixExFPU (
#else
static SpStatus_t ComputeShaperMatrixEx (
#endif
				SpXform_t	xform,
				double		FAR *shaper [3],
				double		ColorMatrix [3] [3])
{
	double	wht [3];
	SpStatus_t	error;
		
	error = ComputeShaperEx (xform, shaper, wht);
	if (error == SpStatSuccess)
	{
		error = ComputeMatrixEx (xform, shaper, wht, ColorMatrix);
		if (error == SpStatSuccess)
			error = PostNormalize(shaper, ColorMatrix);
	}
	return error;
}


/*------------------------------------------------------------------*/

#if defined (SP_FPU)
SpStatus_t	SPAPI SpXformCreateMatTagsFromPTFPU (
#else
SpStatus_t	SPAPI SpXformCreateMatTagsFromPTnoFPU (
#endif
				SpProfile_t		Profile,
				PTRefNum_t		RefNum)
{
	SpStatus_t		Status;
	SpTagValue_t	Tag;
	KpInt32_t		SpaceIn, SpaceOut;
	KpInt32_t		SenseIn;
	SpStatus_t		spStat;
	double			*Shaper [3];
	double			r [SHAPERCURVESIZE];
	double			g [SHAPERCURVESIZE];
	double			b [SHAPERCURVESIZE];
	double			ColorMatrix [3] [3];
	SpCurve_t		TRC;
	KpUInt16_t		TRCData [SHAPERCURVESIZE];
	int				i;

/* check that we are dealing with an RGB to RCS PT */
	SpaceIn = SpGetKcmAttrInt (RefNum, KCM_SPACE_IN);
	SpaceOut = SpGetKcmAttrInt (RefNum, KCM_SPACE_OUT);
	SenseIn = SpGetKcmAttrInt (RefNum, KCM_MEDIUM_SENSE_IN);
	if ((SpaceIn != KCM_RGB)
			|| (SpaceOut != KCM_RCS)
			|| (SenseIn == KCM_NEGATIVE)) {
		PTCheckOut (RefNum);
		return SpStatOutOfRange;
	}

/* compute tag data */
	Shaper [0] = r;
	Shaper [1] = g;
	Shaper [2] = b;

	spStat = ComputeShaperMatrix (RefNum, Shaper, ColorMatrix);
	if (spStat != SpStatSuccess)
		return spStat;

/* save the colorant tags */
	Tag.TagType = Sp_AT_XYZ;

	Tag.TagId = SpTagRedColorant;
	Tag.Data.XYZ.X = KpF15d16FromDouble ((ColorMatrix [0] [0]));
	Tag.Data.XYZ.Y = KpF15d16FromDouble ((ColorMatrix [0] [1]));
	Tag.Data.XYZ.Z = KpF15d16FromDouble ((ColorMatrix [0] [2]));
	Status = SpTagSet (Profile, &Tag);
	if (SpStatSuccess == Status) {
		Tag.TagId = SpTagGreenColorant;
		Tag.Data.XYZ.X = KpF15d16FromDouble ((ColorMatrix [1] [0]));
		Tag.Data.XYZ.Y = KpF15d16FromDouble ((ColorMatrix [1] [1]));
		Tag.Data.XYZ.Z = KpF15d16FromDouble ((ColorMatrix [1] [2]));
		Status = SpTagSet (Profile, &Tag);
	}
	if (SpStatSuccess == Status) {
		Tag.TagId = SpTagBlueColorant;
		Tag.Data.XYZ.X = KpF15d16FromDouble ((ColorMatrix [2] [0]));
		Tag.Data.XYZ.Y = KpF15d16FromDouble ((ColorMatrix [2] [1]));
		Tag.Data.XYZ.Z = KpF15d16FromDouble ((ColorMatrix [2] [2]));
		Status = SpTagSet (Profile, &Tag);
	}

/* save reponse curve tags */
	TRC.Count = SHAPERCURVESIZE;
	TRC.Data = TRCData;
	Tag.Data.Curve = TRC;

	Tag.TagType = Sp_AT_Curve;
	if (SpStatSuccess == Status) {
		Tag.TagId = SpTagRedTRC;
		for (i = 0; i < SHAPERCURVESIZE; i++)
			TRCData [i] = (KpUInt16_t)KpF15d16FromDouble (r [i]);
		Status = SpTagSet (Profile, &Tag);
	}
	if (SpStatSuccess == Status) {
		Tag.TagId = SpTagGreenTRC;
		for (i = 0; i < SHAPERCURVESIZE; i++)
			TRCData [i] = (KpUInt16_t)KpF15d16FromDouble (g [i]);
		Status = SpTagSet (Profile, &Tag);
	}
	if (SpStatSuccess == Status) {
		Tag.TagId = SpTagBlueTRC;
		for (i = 0; i < SHAPERCURVESIZE; i++)
			TRCData [i] = (KpUInt16_t)KpF15d16FromDouble (b [i]);
		Status = SpTagSet (Profile, &Tag);
	}

	return Status;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 * This function will generate TRC curve tags and colorant tags from 
 * a given xform and insert them into a given profile.
 *
 * INPUT VARIABLES
 *	SpProfile_t	Profile	- The profile to receive the tags
 *	SpXform_t	Xform	- The xform used to generate the tags
 *
 * OUTPUT VARIABLES
 *	None
 *
 * RETURNS
 *	SpStatus_t - SpStatSuccess or a sprofile error
 *
 * AUTHOR
 * stanp
 *
 *------------------------------------------------------------------*/
 
#if defined (SP_FPU)
SpStatus_t	SPAPI SpXformCreateMatTagsFromXformFPU (
#else
SpStatus_t	SPAPI SpXformCreateMatTagsFromXformnoFPU (
#endif
		SpProfile_t	Profile,
		SpXform_t	Xform)
{
	SpStatus_t	Status;
	SpTagValue_t	Tag;
	SpStatus_t	spStat;
	SpXformDesc_t	Desc;
	double		*Shaper [3];
	double		r [SHAPERCURVESIZE];
	double		g [SHAPERCURVESIZE];
	double		b [SHAPERCURVESIZE];
	double		ColorMatrix [3] [3];
	SpCurve_t	TRC;
	KpUInt16_t	TRCData [SHAPERCURVESIZE];
	int		i;
/*	PTRefNum_t	refNum;  */
 
	/* check that we are dealing with an RGB to PCS xform */
	spStat = SpXformGetDesc( Xform, &Desc);
	if (spStat != SpStatSuccess)
		return spStat;
 
	if ((Desc.SpaceIn != SpSpaceRGB) || 
	    (Desc.SpaceOut != SpSpaceXYZ && 
	     Desc.SpaceOut != SpSpaceLAB )) 
	{
		return SpStatOutOfRange;
	}
 
	/* compute tag data */
	Shaper [0] = r;
	Shaper [1] = g;
	Shaper [2] = b;
/*	spStat = SpXformGetRefNum (Xform, &refNum);
	if (spStat != SpStatSuccess)
		return spStat;
*/
	spStat = ComputeShaperMatrixEx (/*refNum*/ Xform, Shaper, ColorMatrix);
	if (spStat != SpStatSuccess)
		return spStat;
 
	/* save the colorant tags */
	Tag.TagType = Sp_AT_XYZ;
 
	Tag.TagId = SpTagRedColorant;
	Tag.Data.XYZ.X = KpF15d16FromDouble (ColorMatrix [0] [0]);
	Tag.Data.XYZ.Y = KpF15d16FromDouble (ColorMatrix [0] [1]);
	Tag.Data.XYZ.Z = KpF15d16FromDouble (ColorMatrix [0] [2]);
	Status = SpTagSet (Profile, &Tag);

	if (SpStatSuccess == Status) 
	{
		Tag.TagId = SpTagGreenColorant;
		Tag.Data.XYZ.X = KpF15d16FromDouble (ColorMatrix [1] [0]);
		Tag.Data.XYZ.Y = KpF15d16FromDouble (ColorMatrix [1] [1]);
		Tag.Data.XYZ.Z = KpF15d16FromDouble (ColorMatrix [1] [2]);
		Status = SpTagSet (Profile, &Tag);
	}

	if (SpStatSuccess == Status) 
	{
		Tag.TagId = SpTagBlueColorant;
		Tag.Data.XYZ.X = KpF15d16FromDouble (ColorMatrix [2] [0]);
		Tag.Data.XYZ.Y = KpF15d16FromDouble (ColorMatrix [2] [1]);
		Tag.Data.XYZ.Z = KpF15d16FromDouble (ColorMatrix [2] [2]);
		Status = SpTagSet (Profile, &Tag);
	}
 
	/* save reponse curve tags */
	TRC.Count = SHAPERCURVESIZE;
	TRC.Data = TRCData;
	Tag.Data.Curve = TRC;
 
	Tag.TagType = Sp_AT_Curve;
	if (SpStatSuccess == Status) 
	{
		Tag.TagId = SpTagRedTRC;
		for (i = 0; i < SHAPERCURVESIZE; i++)
			TRCData [i] = (KpUInt16_t)KpF15d16FromDouble (r [i]);

		Status = SpTagSet (Profile, &Tag);
	}

	if (SpStatSuccess == Status) 
	{
		Tag.TagId = SpTagGreenTRC;
		for (i = 0; i < SHAPERCURVESIZE; i++)
			TRCData [i] = (KpUInt16_t)KpF15d16FromDouble (g [i]);

		Status = SpTagSet (Profile, &Tag);
	}

	if (SpStatSuccess == Status) 
	{
		Tag.TagId = SpTagBlueTRC;
		for (i = 0; i < SHAPERCURVESIZE; i++)
			TRCData [i] = (KpUInt16_t)KpF15d16FromDouble (b [i]);

		Status = SpTagSet (Profile, &Tag);
	}
 
	return Status;
}



