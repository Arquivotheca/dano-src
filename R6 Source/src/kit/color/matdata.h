/*
	File:		matdata.h

	Contains:	interface definitions for make{Forward|Inverse}XformFromMatrix

	Written by:	The Boston White Sox

	Copyright:	1993-1995, by Eastman Kodak Company (all rights reserved)

	Change History (most recent first):

*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1995                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/




#ifndef MATRIXDATA_H
#define MATRIXDATA_H

#include "kcmptlib.h"
#include "fut.h"
#include "fut_util.h"

/*-----------------------------------------------------------
 *  MATRIXDATA -- internal data format 
 *-----------------------------------------------------------
 */

typedef struct tagMATRIXDATA {
	u_int16	dim;				/* dimension (= 3) */
	double	FAR* FAR* matrix;		/* i.e., matrix[dim][dim] */
	ResponseRecord_h response;	/* i.e., response[dim] */
} MATRIXDATA, FAR* LPMATRIXDATA;


/*-----------------------------------------------------------
 *  function prototypes
 *-----------------------------------------------------------
 */

PTErr_t makeProfileXformFPU ARGS((
			FixedXYZColor_p rXYZ, FixedXYZColor_p gXYZ, FixedXYZColor_p bXYZ, 
			ResponseRecord_p rTRC, ResponseRecord_p gTRC, ResponseRecord_p bTRC, 
			u_int32 gridsize, bool invert, newMGmode_p newMGmodeP, fut_ptr_t FAR* theFut));

PTErr_t makeProfileXformNoFPU ARGS((
			FixedXYZColor_p rXYZ, FixedXYZColor_p gXYZ, FixedXYZColor_p bXYZ, 
			ResponseRecord_p rTRC, ResponseRecord_p gTRC, ResponseRecord_p bTRC, 
			u_int32 gridsize, bool invert, newMGmode_p newMGmodeP, fut_ptr_t FAR* theFut));

PTErr_t makeOutputMatrixXformFPU 
			ARGS((Fixed_p matrix, u_int32 gridsize, fut_ptr_t *theFut));

PTErr_t makeOutputMatrixXformNoFPU 
			ARGS((Fixed_p matrix, u_int32 gridsize, fut_ptr_t *theFut));

PTErr_t makeForwardXformFromMatrixFPU ARGS((
			LPMATRIXDATA mdata, u_int32 gridsize, KpUInt32_t interpMode, fut_ptr_t * theFut));

PTErr_t makeForwardXformFromMatrixNoFPU ARGS((
			LPMATRIXDATA mdata, u_int32 gridsize, KpUInt32_t interpMode, fut_ptr_t * theFut));

PTErr_t makeInverseXformFromMatrixFPU ARGS((
			LPMATRIXDATA mdata, u_int32 gridsize, KpUInt32_t interpMode, fut_ptr_t * theFut));

PTErr_t makeInverseXformFromMatrixNoFPU ARGS((
			LPMATRIXDATA mdata, u_int32 gridsize, KpUInt32_t interpMode, fut_ptr_t * theFut));

void makeMonotonic ARGS((KpUInt32_t count, KpUInt16_p table));

void makeInverseMonotonic ARGS((KpUInt32_t count, KpUInt16_p table));

KpInt32_t
	kcpIsFPUpresent (void);

/*-----------------------------------------------------------
 *  useful macros
 *-----------------------------------------------------------
 */

#define	SCALEFIXED	65536.0			/* 2^16 */
#define	SCALEDOT16	65536.0			/* 2^16 */
#define	SCALEDOT15	32768.0			/* 2^15 */
#define	SCALEDOT8	  256.0			/* 2^8  */
	/*
		This scale factor is applied to the X, Y, and Z values prior to
		the 12-bit quantization so that, when shifted left 4 bits,
		it will produce an unsigned 16-bit number with one integer bit,
		as per the ColorSync open profile format.  It is *approximately* 
		1/2:  the exact number is 2^15/(4080 << 4), since 1.0 is represented 
		as 2^15 and FUT_MAX_PEL12 (= 4080) is the quantization scale factor.
	*/


#endif	/* MATRIXDATA_H */

