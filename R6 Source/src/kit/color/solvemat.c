/*********************************************************************/
/*
	Contains:	This module contains a simultaneous matrix solver
				Created by PGT, Oct 26, 1993 (port to Win32)
				From JJG and RFP

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SOLVEMAT.c  $
		$Logfile:   O:\pc_src\lib\csmatrix\solvemat.c_v  $
		$Revision:   1.1  $
		$Date:   05 Nov 1993 18:33:04  $
		$Author:   pgt  $

	SCCS Revision:
		@(#)solvemat.c	1.11 12/22/97

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1997                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/
/* solvemat.c - solvemat */

#include <stdlib.h>
#include <math.h>
#include "csmatrix.h"


/*--------------------------------------------------------------------
 * DESCRIPTION
 * This function solves a set of simultaneous linear equations, given
 * the matrix of coefficients and the vector of constant
 * terms.  Uses Gauss-Jordan elimination with full
 * pivoting.  Matrix is replaced by its inverse, and
 * the constant vector is replaced by the vector of
 * results.
 *
 * AUTHOR
 * JJG, RFP
 *
 * DATE CREATED
 * ??
 *
 *------------------------------------------------------------------*/

int32
	solvemat (	int			n,
			double FAR* FAR*	a,
			double FAR*		b)
{
	int i,j,k;					/* Loop indices		*/
	int row = 0,col = 0;				/* Row & col for pivot	*/
	int *piv,*rindx = NULL,*cindx = NULL;		/* Bookkeeping vectors	*/
	int retval;					/* Return value TRUE => error */

	double big;					/* Big value for pivot	*/
	double temp;				/* Temp for swaps, etc. */
	double *ptrtemp;			/* Temp for column swaps */
	double pivinv;				/* Inverse of pivot	*/

		/* DEBUGGING CODE */

#ifdef DEBUG_SOLVEMAT
  FILE	*fp, *fopen();
  fp = fopen ("solvemat.dbg", "a+");
  if (fp == NULL)
	error ("%s:  Can't open solvemat.dbg", "solvemat");
  fprintf (fp, "Input matrix A:  \n");
  for (i = 0; i < n; i++)
  for (j = 0; j < n; j++)
	fprintf (fp, "%10d%10d %20.10lf\n", i, j, a[i][j]);
  fprintf (fp, "\nInput vector B:  \n");
  for (i = 0; i < n; i++)
	fprintf (fp, "%10d %20.10lf\n", i, b[i]);
#endif

  /*
   * Allocate space for bookkeeping vectors
   */
	retval = KPTRUE;			/* assume allocation failure */
	/*
	 * Test each allocation for success
	 */
	if( (piv = (int *) ALLOC(n,sizeof(int))) != NULL )
	{
		if( (rindx = (int *) ALLOC(n,sizeof(int))) != NULL )
		{
			if( (cindx = (int *) ALLOC(n,sizeof(int))) == NULL )
			{
				DALLOC(piv);
				DALLOC(rindx);
				goto quit;
			}
		}
		else
		{
			DALLOC(piv);
			goto quit;
		}
	}  
	else
		goto quit;



	for (j = 0; j < n; j++)			/* Initialize!		*/
		piv[j] = 0;

	for (i=0; i<n; i++)
	{								/* Need to pivot n times */
		big = 0.0;					/* Search whole matrix	*/
		for (j=0; j<n; j++)
		{							/* for element of max	*/
			if (piv[j] != 1)		/* abs value.  This is	*/
				for (k=0; k<n; k++)
				{					/* the pivot, which must*/
					if (piv[k] == 0)
					{				/* be moved to the diagonal */
						temp = fabs(a[k][j]);
						big = (temp >= big) ? (row=j, col=k, temp) : big;
					}
					else if (piv[k] > 1)
					{					/* Matrix is singular */
						retval = KPTRUE;	/* Must not pivot same */
						goto quit;		/* row twice */
					}
				}
		}
		piv[col]++;
		
		if (row != col)
		{
			for (j=0; j<n; j++)
			{						/* Interchange matrix rows to bring */
				temp = a[j][row];	/* pivot to diagonal*/
				a[j][row] = a[j][col];
				a[j][col] = temp;
			}
			temp = b[row];			/* Same for the constant vector */
			b[row] = b[col];
			b[col] = temp;
		}
		
		rindx[i] = row;				/* Keep track of implicit column */
		cindx[i] = col;				/* interchanges needed on matrix */
		
		if (a[col][col] == 0.0)
		{							/* Zero pivot => singular */
			retval = KPTRUE;
			goto quit;
		}
		pivinv = 1.0/a[col][col];	/* Inverse of pivot */
		a[col][col] = 1.0;			/* Replace diag element with 1.0 */
		
		for (j=0; j<n; j++) 
			a[j][col] *= pivinv;	/* Normalize pivot row */
		b[col] *= pivinv;			/* for matrix and vector */
		
		for (j=0; j<n; j++)
		{
			if (j == col) continue;	/* Skip pivot row */
			temp = a[col][j];		/* Replace off-diag matrix */
			a[col][j] = 0.0;		/* element with zero */
		
			for (k=0; k<n; k++) 
				a[k][j] -= a[k][col]*temp;	/* Reduce jth row */
			b[j] -= b[col]*temp;			/* of matrix & vector */
		}
		
	}						/* End i loop */

	for (i=n-1; i>=0; i--) 		/* Undo matrix column interchanges */
		if (rindx[i] != cindx[i])
		{		/* Only important if inverse matrix */
			ptrtemp = a[rindx[i]];		/* itself is needed */
			a[rindx[i]] = a[cindx[i]];
			a[cindx[i]] = ptrtemp;
		}

	retval = KPFALSE;						/* Normal completion */

	quit:
		DALLOC(piv);					/* Free up bookkeeping space */
		DALLOC(rindx);
		DALLOC(cindx);

			/* DEBUGGING CODE */
#ifdef DEBUG_SOLVEMAT
  fprintf (fp, "Output matrix A:  \n");
  for (i = 0; i < n; i++)
  for (j = 0; j < n; j++)
	fprintf (fp, "%10d%10d %20.10lf\n", i, j, a[i][j]);
  fprintf (fp, "\nOutput vector B:  \n");
  for (i = 0; i < n; i++)
	fprintf (fp, "%10d %20.10lf\n", i, b[i]);
  fclose (fp);
#endif

	return(retval);			/* Return error indication */

}

