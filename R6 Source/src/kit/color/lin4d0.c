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
 * fut_interp_lin4d8 computes the trilinear (3D) interpolation of
 * 12 bit input data in the grid table and produces 12 bit output.
 */

int
	fut_interp_lin4d0 (generic_int16_ptr_t rp, generic_int16_ptr_t tp,
				generic_int16_ptr_t zp, generic_int16_ptr_t yp,
				generic_int16_ptr_t xp,
				int32 n, fut_itbldat_ptr_t ttbl, fut_itbldat_ptr_t ztbl,
				fut_itbldat_ptr_t ytbl, fut_itbldat_ptr_t xtbl,
				fut_gtbldat_ptr_t gtbl, int32 nt, int32 nz, int32 ny,
				fut_otbldat_ptr_t otbl)
{
fut_generic_ptr_t cell;							/* ^ to base of interpolation cell */
longword_t	interpt, interpz, interpy, interpx;	/* interpolants */
int32  frac;					/* fractional part of x, y, z, or t */
int32	t0, t1, t2, t3;	/* temporary results */
int32	t4, t5, t6, t7;	/* more temporary results */
int32	sz, sy, sx;

	sz = GDATSIZE * nt; 			/* single stride for z dimension */
	sy = sz * nz;							/* single stride for y dimension */
	sx = sy * ny;							/* single stride for x dimension */

	while (--n >= 0) {
/* use input lookup tables to get cell coordinates and interpolants */
		interpt.HL = ttbl[*tp++];		/* t cell coordinate and interpolant */
		interpz.HL = ztbl[*zp++];		/* z cell coordinate and interpolant */
		interpy.HL = ytbl[*yp++];		/* y cell coordinate and interpolant */
		interpx.HL = xtbl[*xp++];		/* x cell coordinate and interpolant */

/* compute base of 8-corner cube in grid table, using x,y,z,t whole parts */
		cell = (fut_generic_ptr_t)gtbl;						/* grid table base */
		cell += interpt.shortword.H << GDATSHIFT; /* calculate cell base */
		cell += interpz.shortword.H * sz;
		cell += interpy.shortword.H * sy;
		cell += interpx.shortword.H * sx;

/* get fractional parts of t and interpolate in t */

		frac = (int32)interpt.shortword.L;

		t0 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), frac);
		cell += sz;
		t1 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), frac);
		cell += sy;
		t3 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), frac);
		cell -= sz;
		t2 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), frac);
		cell += sx;
		t6 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), frac);
		cell -= sy;
		t4 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), frac);
		cell += sz;
		t5 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), frac);
		cell += sy;
		t7 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), frac);

	/* get fractional parts of z and interpolate in z */

		frac = (int32)interpz.shortword.L;
		t0 = LINEAR_INTERPOLATE (t0, t1, frac);
		t1 = LINEAR_INTERPOLATE (t2, t3, frac);
		t2 = LINEAR_INTERPOLATE (t4, t5, frac);
		t3 = LINEAR_INTERPOLATE (t6, t7, frac);

  /* get fractional parts of y and interpolate in y */

		frac = (int32)interpy.shortword.L;
		t0 = LINEAR_INTERPOLATE (t0, t1, frac);
		t1 = LINEAR_INTERPOLATE (t2, t3, frac);

  /* get fractional parts of x and interpolate in x */

		frac = (int32)interpx.shortword.L;
		t0 = LINEAR_INTERPOLATE (t0, t1, frac);

		if ( otbl != NULL ) {
			t0 = otbl[t0];				/* pass result through output table */
		}

		*rp++ = (int16) (t0);	/* place result in output array */
	}

	return (1);

} /* fut_interp_lin4d0 */

