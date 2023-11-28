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

/* functions for accessing fut tables.
 */

#include "fut.h"
#include "fut_util.h"		/* internal interface file */

/* local procedures */
static int fut_get_itbldat ARGS((fut_itbl_ptr_t, fut_itbldat_ptr_t*));
static int has_chan ARGS((fut_ptr_t, int));


/* get an input table of a fut 
 */
int
  fut_get_itbl (fut_ptr_t fut, int ochan, int ichan, fut_itbldat_ptr_t* itblDat)
{
int theReturn = -1;		/* assume the worst */

	if (ichan < FUT_NICHAN) {
		if (ochan == -1) {
			if ( IS_FUT(fut) ) {	/* defined? */
				theReturn = fut_get_itbldat (fut->itbl[ichan], itblDat);
			}
		} else {
			if ((theReturn = has_chan(fut, ochan)) == 1) {
				theReturn = fut_get_itbldat (fut->chan[ochan]->itbl[ichan], itblDat);
			}
		}
		fut->modNum++;	/* increase modification level */
	}
	return (theReturn);

} /* fut_get_itbl */


/* get the data pointer of an input table of a fut 
 */
static int
  fut_get_itbldat (fut_itbl_ptr_t itbl, fut_itbldat_ptr_t* itblDat)
{
int theReturn = -2;	/* assuem table does not exist */

	if ( IS_ITBL(itbl) ) {	/* defined? */
		if (itbl->id <= 0) {
			itbl->id = fut_unique_id();	/* assume table gets changed, give it a real id */
		}
		*itblDat = itbl->tbl;					/* return input table */
		theReturn = 1;
	}
		
	return (theReturn);

} /* fut_get_itbldat */


/* get a grid table of a fut 
 */
int
  fut_get_gtbl (fut_ptr_t fut, int ochan, fut_gtbldat_ptr_t* gtblDat)
{
int theReturn;
fut_gtbl_ptr_t gtbl;

	if ((theReturn = has_chan(fut, ochan)) == 1) {
		gtbl = fut->chan[ochan]->gtbl;
		if (gtbl->id <= 0) {
			gtbl->id = fut_unique_id();	/* assume table gets changed, give it a real id */
		}
		*gtblDat = gtbl->tbl;					/* return output table */
	}
	
	fut->modNum++;	/* increase modification level */
		
	return (theReturn);
	
} /* fut_get_gtbl */



/* get an output table of a fut 
 */
int
  fut_get_otbl (fut_ptr_t fut, int ochan, fut_otbldat_ptr_t* otblDat)
{
int theReturn;
fut_otbl_ptr_t otbl;

	if ((theReturn = has_chan(fut, ochan)) == 1) {
		otbl = fut->chan[ochan]->otbl;
		if (otbl->id <= 0) {
			otbl->id = fut_unique_id();	/* assume table gets changed, give it a real id */
		}
		*otblDat = otbl->tbl;					/* return output table */
	}
	
	fut->modNum++;	/* increase modification level */
		
	return (theReturn);
	
} /* fut_get_otbl */



/* return 1 if channel is present, -1 if it is not */
static int 
	has_chan (fut_ptr_t fut, int chan)
{
int theReturn = -1;

	if ( IS_FUT(fut) &&
			(chan >= 0) &&
			(chan < FUT_NOCHAN) &&
			IS_CHAN(fut->chan[chan]) ) {
		theReturn = 1;											/* output channel exists */
	}
	
	return (theReturn);
}

