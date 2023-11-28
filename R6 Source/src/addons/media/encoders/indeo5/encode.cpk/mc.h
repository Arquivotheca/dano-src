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

#ifdef INCLUDE_NESTING_CHECK
#ifdef __MC_H__
#pragma message("***** MC.H Included Multiple Times")
#endif
#endif

#ifndef __MC_H__ 
#define __MC_H__


/* This file depends on the following :
#include <setjmp.h>
#include <datatype.h>
*/

/* Verify inclusion of required files */
#ifndef __DATATYPE_H__
#pragma message("mc.h requires datatype.h")
#endif

/*************************************************************

DESCRIPTION: motion compensation routines

*************************************************************/

/* returns displacement in pels and fraction
*/
#define McModRes(Div, quo, rem) \
	{*(quo) = (Div) >> MC_RES; *(rem) = (Div) & MC_MASK;}


/* Average of two values currently used in the averaging of forward
   and backward motion vectors.
*/
#define AVG(x, y) (((x) + (y)) >> 1)
#define DIV_ROUND(x,d)     ((x)>0?((x)+(d)/2)/(d):-(((-(x))+(d)/2)/(d)))  

/*   The displacement vector of a block points to the source of that
block. Thus, eg, c > 0 means that the source of the block begins at a
larger column, so is to the right of current block. This is the PLV and
MPEG convention. These vectors are described by the following structure:
*/
typedef struct _McVectorSt {
	I32 r, c;		/* row displ, column displ */
} McVectorSt, *PMcVectorSt;

typedef const McVectorSt *PCMcVectorSt;

/*   The units of displacement are 1/16 pixel; the unit of the PB/DB.
*/
#define MC_RES  4               /* # bits of precision       */
#define MC_UNIT (1 << MC_RES)   /* # steps in a pixel        */
#define MC_MASK (MC_UNIT - 1)   /* test for fractional displ */

/* apply displacement vector to rectangle in matrix
*/
void
McRectInterp(
	MatrixSt mRef,    	/* reference pic */
	MatrixSt mComp,   	/* Compensated picture */
	RectSt rRect,     	/* rectangle of interest */
	McVectorSt Vector,  /* displacement vector */
	jmp_buf jbEnv);


/* Verifies a displaced rect lies within image. Returns TRUE iff the
rect at Rect+Vect lies at least Margin pixels from any boundary of Im.
*/
PIA_Boolean
McVectOk(
	PCMatrixSt pIn,
	RectSt cell,
	McVectorSt d,
	I32 margin);

#endif /* __MC_H__ */

