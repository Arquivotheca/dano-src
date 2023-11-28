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
 * fut_bilin computes the bilinear (2D) interpolation in the grid
 * table given (z,t) from the input tables.
 */

int32
fut_bilin (fut_interp_t	* iap)
{
	fut_generic_ptr_t	cell;         /* ^ to base of interpolation cell */
	int32  frac;           /* fractional part of z, t */
	int32	t1, t2;         /* temporary results */
					/* unpack arg list: */
	int32  t = (int32)iap->gt;
	int32  z = (int32)iap->gz;
	int32  sz = iap->sz;	/* stride in z direction */

                                        /* compute base of 4-corner square in
                                           grid table, using z,t whole parts */

	cell =  (fut_generic_ptr_t)iap->grid;
	cell += (t >> WHOLE_SHIFT) * sizeof(fut_gtbldat_t);
	cell += (z >> WHOLE_SHIFT) * sz;

                                        /* get fractional parts of t and
                                           interpolate in t */

	frac = ((t >> FRAC_SHIFT) & FRAC_MASK);

        t1 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);

	cell += sz;
        t2 = LINEAR_INTERPOLATE (GRID_VAL (cell),
                                 GRID_VAL (cell + sizeof(fut_gtbldat_t)),
                                 frac);

                                        /* get fractional parts of z and
                                           interpolate in z */

	frac = ((z >> FRAC_SHIFT) & FRAC_MASK);

	return ((int)LINEAR_INTERPOLATE(t1, t2, frac));

} /* fut_bilin */

