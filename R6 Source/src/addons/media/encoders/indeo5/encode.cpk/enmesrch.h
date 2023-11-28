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
*               Copyright (c) 1994-1997 Intel Corp.		                *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

#ifdef INCLUDE_NESTING_CHECK
#ifdef __ENMESRCH_H__
#pragma message("***** ENMESRCH.H Included Multiple Times")
#endif
#endif

#ifndef __ENMESRCH_H__ 
#define __ENMESRCH_H__

/* this header file depends on the following header files 
* #ifdef SIMULATOR 
* #include "encsmdef.h"
* #include "decsmdef.h"
* #endif
* #include "enme.h"
* #include "indeo5.h"
* #include "ensyntax.h"
* #include "encsmdef.h"
*/

/*************************************************************

DESCRIPTION: motion analysis, motion compensation routines

*************************************************************/

		/* Error Measure */
#define ME_MSE 0        /* mean square error */
#define ME_MAD 1        /* mean absolute error */

		/* Search Tactic */
#define ME_FULL 0       /* exhaustive search */
#define ME_GRAD 1       /* PLV2 gradient search method */

/* Resolutions */
#define INTEGER_PEL_RES 16
#define HALF_PEL_RES 8

/* Find best match for Targ Block in Ref image */
Dbl EncMeRect(
	PCEncMeCntxSt pMeContext,
	RectSt rMbRect,			/* Macro block rectangle */
	PMcVectorSt pvMcVect, 	/* Starting and returned motion vector */
	jmp_buf jbEnv);

/* Return err of match for given Vect */

Dbl
EncMeRectErr(PCMatrixSt pRef,
			 PCMatrixSt pTarg,
			 RectSt		Rect,
			 I32		iMeasure,		
			 McVectorSt	Vect);

I32
EncMeRectErrFast(PCMatrixSt pcRef,
			     PCMatrixSt pcmTarg,
			     RectSt		Rect,
			     I32 		iMaxErr,		
			     McVectorSt	Vect);


#endif
