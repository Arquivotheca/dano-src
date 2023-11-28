/*
 * @(#)futiotbl.c	2.21 98/01/02

	Contains:	Table access functions for futs.

	Written by:	Drivin' Team

	These routines provide access to the individual tables of a fut.
	Since there is little protection against abuse, great care should
	be exercised when using them.

	 8 Dec 93	HTR		Mod PTNewEmpty, PTNewEmptySep, PTGet?tbl to check for
						 valid ptrs

 */
/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) 1992-1995 Eastman Kodak Company                  ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "fut.h"
#include "fut_util.h"
#include "fut_io.h"
#include "kcmptlib.h"
#include "kcms_sys.h"
#include "kcpfut.h"
#include "kcptmgrd.h"
#include "kcptmgr.h"

/* prototypes */
static PTErr_t getTbl(int32, PTRefNum_t,
				int32, int32, int32_long_ptr, int32_long_ptr, KcmHandle FAR*);


PTErr_t
	PTNewEmpty (int32				ndim,
				int32_long_ptr		dim,
				int32				nchan,
				PTRefNum_long_ptr_t	PTRefNum)
{
PTErr_t	errnum;
fut_ptr_t fut;

/* Check for valid ptrs */
	if (Kp_IsBadWritePtr(PTRefNum, sizeof(*PTRefNum)))
		return (KCP_BAD_PTR);
	if (Kp_IsBadReadPtr(dim, (KpUInt32_t)(ndim * sizeof(*dim))))
		return (KCP_BAD_PTR);

	fut = fut_new_empty (ndim, dim, nchan);
	if (fut == NULL) {
		errnum = KCP_FAILURE;
	}
	else {
		if ( fut->idstr != NULL ) {
			fut_free_idstr (fut->idstr);
			fut->idstr = NULL;
		}
		errnum = fut2PT (fut, PTRefNum);	/* make into PT */
	}

	return (errnum);
}


PTErr_t
	PTNewEmptySep (	int32				nchan,
					int32_long_ptr		dim,
					PTRefNum_long_ptr_t	PTRefNum)
{
PTErr_t	errnum;
fut_ptr_t fut;
int32 iomask = 0;
int32 i1;
fut_itbl_ptr_t itbl;
fut_gtbl_ptr_t gtbl;
fut_otbl_ptr_t otbl;

	if ((nchan > FUT_NICHAN) || (nchan > FUT_NOCHAN)) {
		return KCP_FAILURE;
	}
	
/* Check for valid ptrs */
	if (Kp_IsBadWritePtr(PTRefNum, sizeof(*PTRefNum)))
		return (KCP_BAD_PTR);
	if (Kp_IsBadReadPtr(dim, (KpUInt32_t)(nchan * sizeof(*dim))))
		return (KCP_BAD_PTR);

	fut = fut_new (0);	/* make a fut with nothing in it */
	if ( fut->idstr != NULL ) {
		fut_free_idstr (fut->idstr);
		fut->idstr = NULL;
	}
	for (i1 = 0; i1 < nchan; i1++) {
		iomask = FUT_IN(FUT_BIT(i1)) | FUT_OUT(FUT_BIT(i1));

		itbl = fut_new_itbl(dim[i1], fut_iramp);							/* make input table */
		gtbl = fut_new_gtblA(iomask, FUT_NULL_GFUN, dim);					/* make grid table */
		otbl = fut_new_otbl(fut_oramp);										/* make output tables */

		if ( ! fut_defchan(fut, iomask, itbl, gtbl, otbl) ) {
			fut_free (fut);
			return (KCP_FAILURE);
		}

/* since tables are shared in the fut library,
 * free the original tables since they are still 'ours' */
		fut_free_itbl (itbl);
		fut_free_gtbl (gtbl);
		fut_free_otbl (otbl);
	}

	errnum = fut2PT (fut, PTRefNum);	/* make into PT */
	
	return (errnum);
}


/* convert a fut into a checked in and actiavted PT */
PTErr_t
	fut2PT (fut_ptr_t			fut,
			PTRefNum_long_ptr_t	PTRefNum)
{
PTErr_t	errnum;
KcmGenericPtr	PTHdr = NULL;
KcmHandle	PTHdrH, PTDataH;

	errnum = (fut != FUT_NULL ? KCP_SUCCESS : KCP_FAILURE);

	if (errnum == KCP_SUCCESS) {

		PTHdr = allocBufferPtr(sizeof(fut_hdr_t));	/* get buffer for resultant info header */
		if (PTHdr == NULL) {
			errnum = KCP_NO_CHECKIN_MEM;
			diagWindow("fut2PT: allocBufferPtr failed", errnum);
			goto ErrOut;
		}

		if (!fut_io_encode (fut, PTHdr)) {	/* make the info header */
			errnum = KCP_FAILURE;
			diagWindow("fut2PT: fut_io_encode failed", errnum);
			goto ErrOut;
		}
				
		PTDataH = (KcmHandle)fut_unlock_fut(fut);
		if (PTDataH == NULL) {
			errnum = KCP_FAILURE;
			diagWindow("fut2PT: fut_unlock_fut fut failed", errnum);
			goto ErrOut;
		}

		PTHdrH = unlockBufferPtr(PTHdr);	/* unlock the header buffer */
		if (PTHdrH == NULL) {
			errnum = KCP_FAILURE;
			diagWindow("fut2PT: unlockBufferPtr failed(PTHdr)", errnum);
			goto ErrOut;
		}

		/* checkin and activate */
		errnum = PTNewPT(PTHdrH, PTDataH, PTRefNum);
		if (errnum != KCP_SUCCESS) {
			diagWindow("fut2PT: PTNewPT failed", errnum);
			goto ErrOut;
		}
	}

	return(errnum);


ErrOut:
	if (PTHdr != NULL) {
		freeBufferPtr(PTHdr);
	}
	
	if (fut != FUT_NULL) {
		fut_free(fut);
	}

	return (errnum);
}


PTErr_t
	PTGetItbl (	PTRefNum_t		PTRefNum,
				int32			ochan,
				int32			ichan,
				KcmHandle FAR*	itblDat)
{
PTErr_t	errnum;
int32_long_ptr nDim = NULL;
int32_long_ptr dimList = NULL;

/* Check for valid ptr */
	if (Kp_IsBadWritePtr(itblDat, sizeof(*itblDat)))
		return (KCP_BAD_PTR);

	errnum = getTbl(FUT_IMAGIC, PTRefNum, ochan, ichan, nDim, dimList, itblDat);

	return (errnum);
}


PTErr_t 
	PTGetGtbl (	PTRefNum_t		PTRefNum,
				int32			ochan,
				int32_long_ptr	nDim,
				int32_long_ptr	dimList,
				KcmHandle FAR*	gtblDat)
{
PTErr_t	errnum;
int32 ichan = -1;

/* Check for valid ptrs */
	if (Kp_IsBadWritePtr(gtblDat, sizeof(*gtblDat)))
		return (KCP_BAD_PTR);
	if (Kp_IsBadWritePtr(nDim, sizeof(*nDim)))
		return (KCP_BAD_PTR);
/* dimList must have at least 4 entries */
	if (Kp_IsBadWritePtr(dimList, (u_int32) 4 * sizeof(*dimList)))
		return (KCP_BAD_PTR);

	errnum = getTbl(FUT_GMAGIC, PTRefNum, ochan, ichan, nDim, dimList, gtblDat);

	return (errnum);
}


PTErr_t
	PTGetOtbl (	PTRefNum_t		PTRefNum,
				int32			ochan,
				KcmHandle FAR*	otblDat)
{
PTErr_t	errnum;
int32 ichan = -1;
int32_long_ptr nDim = NULL, dimList = NULL;

/* Check for valid ptr */
	if (Kp_IsBadWritePtr(otblDat, sizeof(*otblDat)))
		return (KCP_BAD_PTR);

	errnum = getTbl(FUT_OMAGIC, PTRefNum, ochan, ichan, nDim, dimList, otblDat);

	return (errnum);
}


/* Get a fut table
 * if the PT is active:
 *		get the fut address, lock it, get the requested table,
 *		get the handle to the table, unlock the fut
 * Returns:
 *   KCP_SUCCESS or table failure state 
 */
static PTErr_t
	getTbl(	int32			tblSel,
			PTRefNum_t		PTRefNum,
			int32			ochan,
			int32			ichan,
			int32_long_ptr	nDim,
			int32_long_ptr	dimList,
			KpHandle_t FAR*	tblH)
{
PTErr_t		errnum, errnum1;
KpGenericPtr_t	PTHdr;
PTAddr_t FAR* PTHdrH, FAR* PTDataH;
KpGenericPtr_t tblP;
fut_ptr_t fut;
fut_gtbl_ptr_t gtbl;
int i1, fut_err = 0;

	errnum = PTGetPTInfo(PTRefNum, &PTHdrH, NULL, &PTDataH);	/* get PT info */

	if ((errnum == KCP_PT_ACTIVE) || (errnum == KCP_SERIAL_PT)) {
		fut = fut_lock_fut((KpHandle_t)PTDataH);	/* lock fut and get pointer */
		PTHdr = lockBuffer((KpHandle_t)PTHdrH);		/* lock header and get pointer */

		switch (tblSel) {
			case FUT_IMAGIC:
				fut_err = fut_get_itbl (fut, ochan, ichan, (fut_itbldat_h)&tblP);
				break;
				
			case FUT_GMAGIC:
				fut_err = fut_get_gtbl (fut, ochan, (fut_gtbldat_h)&tblP);
				if (fut_err == 1) {
					gtbl = fut->chan[ochan]->gtbl;	/* get grid table structure */

					for (i1 = 0, *nDim = 0; i1 < FUT_NICHAN; i1++) {	/* find the active dimensions of the grid */
						if (gtbl->size[i1] > 1) {
							dimList[*nDim] = gtbl->size[i1];		/* return dimension sizes */
							(*nDim)++;													/* return # of input variables */
						}
					}
				}
				break;
				
			case FUT_OMAGIC:
				fut_err = fut_get_otbl (fut, ochan, (fut_otbldat_h)&tblP);
				break;
		}
		
		if (fut_err == 1) {
			*tblH = getHandleFromPtr(tblP);	/* return handle to data */
		
			if (!fut_io_encode (fut, PTHdr)) {	/* make the info header */
				errnum = KCP_FAILURE;
				diagWindow("getTbl: fut_io_encode failed", errnum);
			}
			else {				
				errnum = KCP_SUCCESS;
			}
		}
		else {
			errnum += KCP_NO_OUTTABLE +1;
		}

		errnum1 = unlockPT(PTHdrH, fut);
		if (errnum1 != KCP_SUCCESS) {
			diagWindow("getTbl: unlockPT failed", errnum);
		}
	}

	return (errnum);
}

