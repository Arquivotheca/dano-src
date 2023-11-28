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
 * fut_trilin computes the trilinear (3D) interpolation in the grid
 * table given (y,z,t) from the input tables.
 */

int32
fut_trilin (fut_interp_t * iap)
{
	fut_generic_ptr_t cell;		/* ^ to base of interpolation cell */
	int32  frac;				/* fractional part of y, z, or t */
	int32	t1, t2, t3, t4;	/* temporary results */
	int32	fmask = FRAC_MASK;
					/* FRAC_SHIFT probably is 0 */

                                        /* compute base of 8-corner cube in
                                           grid table, using y,z,t
                                           whole parts */
	cell = (fut_generic_ptr_t) iap->grid;
	cell += ((int32)iap->gt >> WHOLE_SHIFT) * sizeof(fut_gtbldat_t);
	cell += ((int32)iap->gz >> WHOLE_SHIFT) * (int32)iap->sz;
	cell += ((int32)iap->gy >> WHOLE_SHIFT) * (int32)iap->sy;

                                        /* get fractional parts of t and
                                           interpolate in t */

	frac = (((int32)iap->gt >> FRAC_SHIFT) & fmask);

        t1 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);
	cell += iap->sz;
        t2 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);
	cell += iap->sy;
        t4 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);
	cell -= iap->sz;
        t3 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);

                                        /* get fractional parts of z and
                                           interpolate in z */

	frac = ((iap->gz >> FRAC_SHIFT) & fmask);

        t1 = LINEAR_INTERPOLATE (t1, t2, frac);
        t3 = LINEAR_INTERPOLATE (t3, t4, frac);

                                        /* get fractional parts of y and
                                           interpolate in y */

	frac = ((iap->gy >> FRAC_SHIFT) & fmask);

	return ((int)LINEAR_INTERPOLATE(t1, t3, frac));

} /* fut_trilin */

ÿ