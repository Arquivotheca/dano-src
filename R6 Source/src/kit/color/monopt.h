/*
 * @(#)monopt.h	1.5 97/12/22

	Contains: prototypes and defines for the monochrome transforms

	Written by:	Color Processor group

*/

/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1997                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#ifndef MONOPT_H
#define MONOPT_H

#define RRECORD_DATA_SIZE 65536

/* function prototypes */

PTErr_t calcOtblLSN ARGS((fut_otbldat_ptr_t table, ResponseRecord* rrp));
PTErr_t calcOtblLS1 ARGS((fut_otbldat_ptr_t table, double gamma));
PTErr_t calcOtblLN ARGS((fut_otbldat_ptr_t table, ResponseRecord* rrp));
PTErr_t calcOtblL1 ARGS((fut_otbldat_ptr_t table, double gamma));

PTErr_t makeForwardXformMono ARGS((ResponseRecord FAR*	grayTRC,
									KpUInt32_t			gridsize,
									fut_ptr_t *		theFut));
PTErr_t makeInverseXformMono ARGS((ResponseRecord FAR*	grayTRC,
									KpUInt32_t 		gridsize,
									fut_ptr_t *		theFut));

fut_gtbldat_t gtblFunc ARGS((double *args));
fut_otbldat_t otblFunc ARGS((fut_gtbldat_t q));
#endif

