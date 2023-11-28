/***************************************************************
  PROPRIETARY NOTICE: The software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on
  their designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1991-1994 Eastman Kodak Company.
  As an unpublished work pursuant to Title 17 of the United
  States Code.  All rights reserved.
****************************************************************
*/

#include "fut.h"
#include "fut_util.h"		/* internal interface file */
#include "interp.h"		/* interpolation macros */

/*
 * fut_quadlin computes the quadlinear (4D) interpolation in the grid
 * table given (x,y,z,t) from the input tables.
 */

int32
	fut_quadlin (struct fut_interp_s * iap)
{
	fut_generic_ptr_t cell;		/* ^ to base of interpolation cell */
	int32  frac;				/* fractional part of x, y, z, or t */
	int32	t0, t1, t2, t3;	/* temporary results */
	int32	t4, t5, t6, t7;	/* more temporary results */
	int32	fmask = FRAC_MASK;
					/* FRAC_SHIFT probably is 0 */

  /* compute base of 8-corner cube in grid table, using x,y,z,t whole parts */
	cell = (fut_generic_ptr_t) iap->grid;
	cell += ((int32)iap->gt >> WHOLE_SHIFT) * sizeof(fut_gtbldat_t);
	cell += ((int32)iap->gz >> WHOLE_SHIFT) * (int32)iap->sz;
	cell += ((int32)iap->gy >> WHOLE_SHIFT) * (int32)iap->sy;
	cell += ((int32)iap->gx >> WHOLE_SHIFT) * (int32)iap->sx;

   /* get fractional parts of t and interpolate in t */

	frac = (((int32)iap->gt >> FRAC_SHIFT) & fmask);

        t0 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);
	cell += iap->sz;
        t1 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);
	cell += iap->sy;
        t3 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);
	cell -= iap->sz;
        t2 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);
	cell += iap->sx;
        t6 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);
	cell -= iap->sy;
        t4 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);
	cell += iap->sz;
        t5 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);
	cell += iap->sy;
        t7 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);

	/* get fractional parts of z and interpolate in z */

	frac = (((int32)iap->gz >> FRAC_SHIFT) & fmask);
        t0 = LINEAR_INTERPOLATE (t0, t1, frac);
        t1 = LINEAR_INTERPOLATE (t2, t3, frac);
        t2 = LINEAR_INTERPOLATE (t4, t5, frac);
        t3 = LINEAR_INTERPOLATE (t6, t7, frac);

  /* get fractional parts of y and interpolate in y */

	frac = ((iap->gy >> FRAC_SHIFT) & fmask);
        t0 = LINEAR_INTERPOLATE (t0, t1, frac);
        t1 = LINEAR_INTERPOLATE (t2, t3, frac);

  /* get fractional parts of x and interpolate in x */

	frac = ((iap->gx >> FRAC_SHIFT) & fmask);
        t0 = LINEAR_INTERPOLATE (t0, t1, frac);

	return (t0);

} /* fut_quadlin */


