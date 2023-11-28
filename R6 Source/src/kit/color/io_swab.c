/*
  File:         io_swab.c          @(#)io_swab.c	13.5 06/08/95

  Contains:
  This file contains functions to handle architecture dependent byte swapping.

*/
/***************************************************************
  PROPRIETARY NOTICE: The software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on
  their designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1991-1995 Eastman Kodak Company.
  As an unpublished work pursuant to Title 17 of the United
  States Code.  All rights reserved.
****************************************************************
*/

#include "fut.h"
#include "fut_util.h"             /* internal interface file */
#include "fut_io.h"

/*
 * fut_swab_hdr and fut_swab_[iog]tbl swaps bytes in a fut_hdr_t and
 * fut_[iog]tbl_t structures to convert between DEC and IBM byte ordering.
 */
void
fut_swab_hdr (fut_hdr_ptr_t hdr)
{
	int            i;
	chan_hdr_ptr_t	chan;

	Kp_swab32 ((fut_generic_ptr_t)&hdr->magic,     1);
	Kp_swab32 ((fut_generic_ptr_t)&hdr->version,   1);
	Kp_swab32 ((fut_generic_ptr_t)&hdr->idstr_len, 1);
	Kp_swab32 ((fut_generic_ptr_t)&hdr->order,     1);
	Kp_swab32 ( (fut_generic_ptr_t)hdr->icode,     FUT_NCHAN);

	for (i=0, chan=hdr->chan; i<FUT_NCHAN; ++i, chan++) {
		Kp_swab16 ( (fut_generic_ptr_t)chan->size,  FUT_NCHAN);
		Kp_swab32 ( (fut_generic_ptr_t)chan->icode, FUT_NCHAN);
		Kp_swab32 ((fut_generic_ptr_t)&chan->ocode, 1);
		Kp_swab32 ((fut_generic_ptr_t)&chan->gcode, 1);
	}

	Kp_swab32 ((fut_generic_ptr_t)&hdr->more,      1);

} /* fut_swab_hdr */

void
fut_swab_itbl (fut_itbl_ptr_t itbl)
{
	Kp_swab32 ((fut_generic_ptr_t)&itbl->magic, 1);
	Kp_swab32 ((fut_generic_ptr_t)&itbl->ref,   1);
	Kp_swab32 ((fut_generic_ptr_t)&itbl->id,    1);
	Kp_swab32 ((fut_generic_ptr_t)&itbl->size,  1);
	Kp_swab32 ((fut_generic_ptr_t) itbl->tbl,   FUT_INPTBL_ENT+1);

	/* Kp_swab32 ((fut_generic_ptr_t)&itbl->tbl, 1);       Never do this! */

} /* fut_swab_itbl */

void
fut_swab_otbl (fut_otbl_ptr_t otbl)
{
	Kp_swab32 ((fut_generic_ptr_t)&otbl->magic, 1);
	Kp_swab32 ((fut_generic_ptr_t)&otbl->ref,   1);
	Kp_swab32 ((fut_generic_ptr_t)&otbl->id,    1);
	Kp_swab16 ((fut_generic_ptr_t) otbl->tbl,   FUT_OUTTBL_ENT);

	/* Kp_swab32 (&otbl->tbl, 1);  Never do this! */

} /* fut_swab_otbl */

void
fut_swab_gtbl (fut_gtbl_ptr_t gtbl)
{
	int32   tbl_size = gtbl->tbl_size;

					/* If gtbl is currently byte reversed,
					   we must swap tbl_size to determine
					   size of grid table */
	if ( gtbl->magic == FUT_CIGAMG )
		Kp_swab32 ((fut_generic_ptr_t)&tbl_size, 1);


	Kp_swab32 ((fut_generic_ptr_t)&gtbl->magic,    1);
	Kp_swab32 ((fut_generic_ptr_t)&gtbl->ref,      1);
	Kp_swab32 ((fut_generic_ptr_t)&gtbl->id,       1);
	Kp_swab16 ((fut_generic_ptr_t) gtbl->tbl,      tbl_size / (int32)sizeof(int16));
	Kp_swab32 ((fut_generic_ptr_t)&gtbl->tbl_size, 1);
	Kp_swab16 ((fut_generic_ptr_t) gtbl->size,     FUT_NCHAN);

	/* Kp_swab32 (&gtbl->tbl, 1);  Never do this! */

} /* fut_swab_gtbl */

