/*
 * @(#)calcgtbl.c	1.7 97/12/22

	Contains:	calcGtbl3

	Written by:	The Boston White Sox

	Copyright:	1993, by Eastman Kodak Company (all rights reserved)

	Change History (most recent first):

			11/21/93	PGT	Port to WIN32
			11/16/93	RFP	calcGtbl.c:  separate source files
			11/16/93	RFP	makeFwdXform.c:  no call-backs
			11/08/93	RFP	makeFwdMatrix.c:  developed on SunOS

*/

/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

/*
 *	General definitions
 */

#include <math.h>
#include "kcmsos.h"
#include "fut.h"
#include "csmatrix.h"
#include "makefuts.h"

/*-------------------------------------------------------------------------------
 *  calcGtbl3 -- calculate 3 grid tables from a (3 x 3) matrix
 *-------------------------------------------------------------------------------
 */
#if defined(KCP_FPU)			/* using the FPU? */
void calcGtbl3FPU (fut_gtbldat_ptr_t *tables, int32 *gridSizes, 
				double **rows, bool xrange)
#else							/* all other programming environments */
void calcGtbl3noFPU (fut_gtbldat_ptr_t *tables, int32 *gridSizes, 
				double **rows, bool xrange)
#endif
{
	double		input[3];
	int32		i, j, k, row, col;
	double		scale, temp, onethird;

     /* Set up for extended range:  */
	scale = (xrange) ? (double)FUT_GRD_MAXVAL : (double)FUT_MAX_PEL12;
	onethird = 1.0 / 3.0;

     /* Loop over 3D grid, converting indices to floating-point input variables:  */	
	for (k = 0; k < gridSizes[0]; k++)
	{
		input[0] = (double)k / (double)(gridSizes[0] - 1);
		for (j = 0; j < gridSizes[1]; j++)
		{
			input[1] = (double)j / (double)(gridSizes[1] - 1);
			for (i = 0; i < gridSizes[2]; i++)
			{
				input[2] = (double)i / (double)(gridSizes[2] - 1);

			     /* Loop over output variables (matrix rows):  */
				for (row = 0; row < 3; row++)
				{

				     /* Multiply input vector by row of matrix:  */
					temp = 0.0;
					for (col = 0; col < 3; col++)
						temp += rows[row][col] * input[col];

				     /* Add offset for extended range, if required:  */
					if (xrange)
						temp += onethird;

				     /* Quantize for 12-bit grid table and insert:  */
					*(tables[row]++) = QUANT (temp, scale);
				}
			}
		}
	}
}

