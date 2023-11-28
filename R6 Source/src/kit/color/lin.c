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
 * fut_lin computes the linear (1D) interpolation in the grid
 * table given (t) from the input tables.
 */

int32
fut_lin (fut_interp_t * iap)
{
	fut_generic_ptr_t cell;    /* ^ to base of interpolation cell */
	int32  frac;           /* fractional part of t */
	int32	t1, t2;		/* temporary variables */
					/* unpack arg list: */
	int32  t = iap->gt;

                                        /* compute base of 2-point segment in
                                           grid table, using t whole part */
	cell = (fut_generic_ptr_t) iap->grid;
	cell += (t >> WHOLE_SHIFT) * sizeof(fut_gtbldat_t);

					/* get values at grid points */

	t1 = GRID_VAL (cell);
	t2 = GRID_VAL ((cell += sizeof(fut_gtbldat_t)));

                                        /* get fractional parts of t and
                                           interpolate in t */
	frac = ((t >> FRAC_SHIFT) & FRAC_MASK);

#ifdef DEBUG
	{	/* compute intermediate values: */
		int32	q;
		q = (t2 - t1);
		q *= frac;
		q += ROUND_UP;
		q >>= FUT_INP_FRACBITS;
		q += t1;
		q = q;
	}
#endif /* DEBUG */

        return ((int)LINEAR_INTERPOLATE (t1, t2, frac));

} /* fut_lin */

