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
 * given (y,z,t), fut_interp_lin3d0 computes the trilinear (3D) interpolation of
 * 12 bit input data in the grid table and produces 12 bit output.
 */

int
	fut_interp_lin3d0 (generic_int16_ptr_t rp, generic_int16_ptr_t zp,
						generic_int16_ptr_t yp, generic_int16_ptr_t xp,
						int32 n, fut_itbldat_ptr_t ztbl, fut_itbldat_ptr_t ytbl,
						fut_itbldat_ptr_t xtbl, fut_gtbldat_ptr_t gtbl, int32 nz,
						int32 ny, fut_otbldat_ptr_t otbl)
{
fut_generic_ptr_t cell;									/* ^ to base of interpolation cell */
longword_t	interpz, interpy, interpx;	/* interpolants */
int32	t1, t2, t3;												/* temporary results */
int32	int32fracy, int32fracz;						/* temporary fractions */
int32	sy, sx;

	sy = GDATSIZE * nz; 			/* single stride for y dimension */
	sx = sy * ny;							/* single stride for x dimension */
		
	while (--n >= 0) {
/* use input lookup tables to get cell coordinates and interpolants */
		interpz.HL = ztbl[*zp++];		/* z cell coordinate and interpolant */
		interpy.HL = ytbl[*yp++];		/* y cell coordinate and interpolant */
		interpx.HL = xtbl[*xp++];		/* x cell coordinate and interpolant */

/* find base of cell in grid table, using x,y,z whole parts and grid table base */
		cell = (fut_generic_ptr_t)gtbl;						/* grid table base */
		cell += interpz.shortword.H << GDATSHIFT; /* calculate cell base */
		cell += interpy.shortword.H * sy;
		cell += interpx.shortword.H * sx;

		int32fracz = (int32)interpz.shortword.L;
		int32fracy = (int32)interpy.shortword.L;

/* tri-linear interpolation */
		t1 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), int32fracz);

		cell += sy;
		t2 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), int32fracz);

   		t1 = LINEAR_INTERPOLATE (t1, t2, int32fracy);

		cell += sx;
		t2 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), int32fracz);

		cell -= sy;
		t3 = LINEAR_INTERPOLATE (GRID_VAL (cell), GRID_VAL (cell + GDATSIZE), int32fracz);

		t3 = LINEAR_INTERPOLATE (t3, t2, int32fracy);

		t1 = LINEAR_INTERPOLATE (t1, t3, (int32)interpx.shortword.L);

		if ( otbl != NULL ) {
			t1 = otbl[t1];								/* pass result through output table */
		}
		
		*rp++ = (int16) t1;							/* place result in output array */
	}

	return (1);

} /* fut_interp_lin3d0 */

