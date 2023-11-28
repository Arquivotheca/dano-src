/*
	File:		xfers.h

	Contains:	definitions and prototypes for xfers.c and clients

	Written by:	The Boston White Sox

	Copyright:	1993, by Eastman Kodak Company (all rights reserved)

	Change History (most recent first):

			11/21/93	pgt	port to Win32
			11/17/93	RFP	xfers.h:  separate header file
			11/16/93	RFP	Non-re-entrant, for now:  NUMXFER = 1
			11/11/93	RFP	xfers.c:  Re-entrant version for DLL
			11/09/93	RFP	New version for ColorSync profiles
		 	11/18/92	RFP	gamxfer.c:  Coarse-fine version for gammafut
							(from xfer.c in transferfut)

	Windows Revision Level:
		$Workfile$
		$Logfile$
		$Revision$
		$Date$
		$Author$

	SCCS Revision:
		@(#)xfers.h	1.4 11/29/93

	To Do:		

*/
/*********************************************************************/


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

#ifndef XFERS_H
#define XFERS_H

#include "kcmsos.h"
#include "matdata.h"

#define NUMFINE 25

typedef struct xfer_s			/* transfer-table object */
{
	double	nonlinear[NUMFINE];	/* nonlinear (e.g., device) coordinates */
	double	linear[NUMFINE];	/* linear (e.g., radiant flux) coordinates */
	double	FAR* from;		/* source selector */
	double	FAR* to;		/* destination selector */
} xfer_t;

PTErr_t init_xfer ARGS((xfer_t FAR* xferp, ResponseRecord FAR* rrp));
PTErr_t set_xfer ARGS((xfer_t FAR* xferp, int source, int dest));
double xfer ARGS((xfer_t FAR* xferp, double inval, int *hint));

PTErr_t init_xferFPU ARGS((xfer_t FAR* xferp, ResponseRecord FAR* rrp));
PTErr_t set_xferFPU ARGS((xfer_t FAR* xferp, int source, int dest));
double xferFPU ARGS((xfer_t FAR* xferp, double inval, int *hint));

/********  CURRENTLY POINTLESS  ********
void free_xfer ARGS((xfer_t FAR* xferp));
****************************************/

#endif	/* XFERS_H */

