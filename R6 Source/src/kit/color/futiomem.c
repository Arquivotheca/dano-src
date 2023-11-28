/*
 * @(#)futiomem.c	2.56 97/12/22

	Contains:	functions to read and write binary fut "memory" files.

	Written by:	Drivin' Team

	Copyright:	(c) 1991-1997 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

		 <4>	 12/4/91	blh		take out diagwindows for non-fatal errors
		 <3>	 12/3/91	gbp		modify for new fut library
		 <2>	11/15/91	blh		fix product version attribute propagation
		 <1>	11/13/91	blh		first checked in

These routines intended for use with a system which maintains its own fut
database.  The external database is responsible for loading and storing a
block of memory which contains all of the fut information.  It needs to know
only the location and size of this block.  The fut_io_mem routines are based
on the fut_io disk file routines and translate the external memory block into
the internal fut structure format needed for function evaluation.

To support architecture dependent byte ordering, byte swapping is performed as
neccessary when reading a file, by checking the byte ordering of the "magic"
numbers.  The "standard" byte ordering is Most Significant Byte First
(e.g. Sun, Macintosh) but this default can be overridden (see below).
 */

/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) 1992-1997 Eastman Kodak Company                  ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "kcms_sys.h"

#if defined (KPMAC68K)
#include <Gestalt.h>
#else
#define gestaltNoFPU 0
#endif

#include <string.h>

#include "fut.h"
#include "fut_util.h"
#include "fut_io.h"
#include "kcmptlib.h"
#include "kcpfut.h"
#include "kcptmgrd.h"
#include "kcptmgr.h"
#include "attrib.h"
#include "attrcipg.h"
#include "matdata.h"

/* prototypes */
static int32 fut_size_chan ARGS((fut_chan_ptr_t, chan_hdr_ptr_t));
static int32 fut_size_itbl ARGS((fut_itbl_ptr_t));
static int32 fut_size_otbl ARGS((fut_otbl_ptr_t));
static int32 fut_size_gtbl ARGS((fut_gtbl_ptr_t));
static PTErr_t lockPT ARGS((KcmHandle, KcmHandle, fut_hdr_ptr_t FAR*, fut_ptr_t FAR*));
static PTErr_t stripCopyright ARGS((char_p, char_p FAR*));

/*
 * TpReadHdr reads the header information from an open binary fut file into
 * futio header structure.  This information describes shared and identity
 * (ramp) tables and indicates the content of the remaining part of the file,
 * but does not set up the input, output, or grid tables.
 * It returns a pointer to the loaded fut header.
 */

PTErr_t
	TpReadHdr (	threadGlobals_p threadGlobalsP,
				KpFd_p			fd,
				KcmHandle*	PTHdr,
				PTType_p	formatP)
{
PTErr_t errnum;
fut_hdr_ptr_t futHdr;
int32		ret, hdrSize;

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}

	hdrSize = (int32)sizeof (fut_hdr_t);

	/* get header memory */
	futHdr = (fut_hdr_ptr_t) allocBufferPtr (hdrSize);
	if (futHdr == NULL) {
		errnum = KCP_NO_CHECKIN_MEM;
		diagWindow("TpReadHdr: allocBufferPtr failed", errnum);
		goto ErrOut;
	}

	ret = Kp_read (fd, (KpGenericPtr_t)&futHdr->magic, sizeof (int32));
	if (ret != 1) {
		errnum = KCP_INVAL_PT_BLOCK;
/*		diagWindow("TpReadHdr: Kp_read failed", errnum); */
		goto ErrOut;
	}

	if ((futHdr->magic == FUT_CIGAM) || (futHdr->magic == FUT_MAGIC) ) {
		ret = fut_read_futhdr (fd, futHdr);	/* read in the header */
	}
	else {
#if (FUT_MSBF == 0)						/* swap bytes if necessary */
		Kp_swab32 ((KpGenericPtr_t)&futHdr->magic, 1);
#endif

		switch (futHdr->magic) {
		case MF1_TBL_ID:	/* 8 bit matrix fut */
		case MF2_TBL_ID:	/* 16 bit matrix fut */
			ret = fut_readMFutHdr (fd, futHdr);	/* read the matrix fut header */
			futHdr->idstr_len = 0;
			break;
		
		default:
			errnum = KCP_INVAL_PT_BLOCK;	/* unknown type */
/*			diagWindow("TpReadHdr: unknown table type", errnum); */
			goto ErrOut;
		}
	}

	if (ret != 1) {
		errnum = KCP_INVAL_PT_BLOCK;
/* 		diagWindow("TpReadHdr: error in header data", errnum); */
		goto ErrOut;
	}

	*formatP = futHdr->magic;	/* return type of PT */

	*PTHdr = unlockBufferPtr((KcmGenericPtr)futHdr);	/* return handle to header info */
	if (*PTHdr == NULL) {
		errnum = KCP_MEM_UNLOCK_ERR;
		diagWindow ("TpReadHdr: unlockBufferPtr failed", errnum);
		goto ErrOut;
	}

	return KCP_SUCCESS;

ErrOut:
	if (futHdr != NULL) {
		freeBufferPtr ((KcmGenericPtr)futHdr);
	}

	return (errnum);

} /* TpReadHdr */



/*
 * verify that two header info blocks are the same
 */
PTErr_t
	TpCompareHdr (	threadGlobals_p threadGlobalsP, 
					KcmHandle	PTHdr1,
					KcmHandle	PTHdr2)
{
PTErr_t errnum = KCP_SUCCESS;
int32 i1, i2;
chan_hdr_ptr_t chanf, chani;
fut_hdr_ptr_t futHdr1, futHdr2;

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}

	if (PTHdr1 == PTHdr2) {
		return (KCP_SUCCESS);
	}

	futHdr1 = (fut_hdr_ptr_t)lockBuffer (PTHdr1);	/* get header pointer 1 */
	if (futHdr1 == NULL) {
		errnum = KCP_MEM_LOCK_ERR;
		diagWindow ("TpCompareHdr: lockBuffer futHdr1 failed", errnum);
		goto GetOut;
	}

	futHdr2 = (fut_hdr_ptr_t)lockBuffer (PTHdr2);	/* get header pointer 2 */
	if (futHdr2 == NULL) {
		errnum = KCP_MEM_LOCK_ERR;
		diagWindow ("TpCompareHdr: lockBuffer futHdr2 failed", errnum);
		goto GetOut;
	}

	if (	(futHdr1->magic != futHdr2->magic)
		||	(futHdr1->version != futHdr2->version)
	   	||	(futHdr1->order != futHdr2->order)) {
		errnum = KCP_INCON_PT;						/* no match */
		diagWindow ("TpCompareHdr: magic, version, or order failed", errnum);
		goto GetOut;
	}

	switch (futHdr1->magic) {
	case FUT_CIGAM: /* fut with bytes reversed */
	case FUT_MAGIC: /* fut with bytes in correct order */

		/* note: 	    (futHdr1->idstr_len != futHdr2->idstr_len)   ||
 		* removed since it may change when attribute size changes
 		*/

		for (i1 = 0; i1 < FUT_NCHAN; i1++) {
			if (futHdr1->icode[i1] != futHdr2->icode[i1]) {
				errnum = KCP_INCON_PT;					/* no match */
				diagWindow ("TpCompareHdr: icode failed", errnum);
				goto GetOut;
			}
		}

		for (i1=0, chanf=futHdr1->chan, chani=futHdr2->chan; i1<FUT_NCHAN; ++i1, chanf++, chani++) {
			if (chanf->gcode != chani->gcode) {
				errnum = KCP_INCON_PT;					/* no match */
				diagWindow ("TpCompareHdr: chan->gcode failed", errnum);
				goto GetOut;
			}

			if (chanf->gcode != 0) {		/* if the channel is defined */
				for (i2 = 0; i2 < FUT_NCHAN; i2++) {
					if (chanf->icode[i2] != chani->icode[i2]) {
						errnum = KCP_INCON_PT;			/* no match */
						diagWindow ("TpCompareHdr: chan->icode failed", errnum);
						goto GetOut;
					}

				/* if this input is defined */
					if (chanf->icode[i2] != FUTIO_NULL) {
						if (chanf->size[i2] != chani->size[i2]) {
							errnum = KCP_INCON_PT;		/* no match */
							diagWindow ("TpCompareHdr: chan->size failed", errnum);
							goto GetOut;
						}
					}
				}

				if (chanf->ocode != chani->ocode) {
					errnum = KCP_INCON_PT;				/* no match */
					diagWindow ("TpCompareHdr: chan->ocode failed", errnum);
					goto GetOut;
				}
			}
		}

		if (futHdr1->more != futHdr2->more) {
			errnum = KCP_INCON_PT;						/* no match */
			diagWindow ("TpCompareHdr: more failed", errnum);
			goto GetOut;
		}

		break;

	case MF1_TBL_ID:	/* 8 bit matrix fut */
	case MF2_TBL_ID:	/* 16 bit matrix fut */
		for (i1 = 0; i1 < 2; i1++) {
			if (futHdr1->icode[i1] != futHdr2->icode[i1]) {
				errnum = KCP_INCON_PT;						/* no match */
				diagWindow ("TpCompareHdr: more failed", errnum);
				goto GetOut;
			}
		}
		break;
		
	default:
		return (KCP_INVAL_PT_BLOCK);		/* unknown type */
	}


GetOut:
	if ( ! unlockBuffer (PTHdr1)) {
		errnum = KCP_MEM_UNLOCK_ERR;
		diagWindow ("TpCompareHdr: unlockBuffer futHdr 1 failed", errnum);
	}
	if ( ! unlockBuffer (PTHdr2)) {
		errnum = KCP_MEM_UNLOCK_ERR;
		diagWindow ("TpCompareHdr: unlockBuffer futHdr 2 failed", errnum);
	}

	return errnum;

}	/* TpCompareHdr */


/*
 * lock and return pointers to the header and fut of a PT
 */
static PTErr_t
	lockPT (	KcmHandle			PTHdr,
				KcmHandle			PTData,
				fut_hdr_ptr_t FAR*	futHdrP,
				fut_ptr_t FAR*		futP)
{
PTErr_t errnum = KCP_SUCCESS;
fut_hdr_ptr_t futHdr;

/* get fut pointer */
	*futP = fut_lock_fut ((KpHandle_t)PTData);
	if (*futP == FUT_NULL) {
		errnum = KCP_PTERR_2;
		diagWindow ("lockPT: fut_lock_fut failed", errnum);
		return errnum;
	}

	if ( ! IS_FUT (*futP) ) {	/* check for valid fut */
		errnum = KCP_NOT_FUT;
		diagWindow ("lockPT: IS_FUT failed", errnum);
		goto GetOut;
	}

/* get header pointer of checked in PT */
	futHdr = (fut_hdr_ptr_t)lockBuffer (PTHdr);
	if (futHdr == NULL) {
		errnum = KCP_MEM_LOCK_ERR;
		diagWindow ("lockPT: lockBuffer futHdr failed", errnum);
		goto GetOut;
	}

	*futHdrP = futHdr;		/* return pointer to caller */

GetOut:
	if (errnum != KCP_SUCCESS) {
		(void) unlockPT (PTHdr, *futP);
	}

	return errnum;

}	/* lockPT */


/*
 * unlock header and data of a PT
 */
PTErr_t
	unlockPT (	KcmHandle	PTHdr,
				fut_ptr_t	fut)
{
PTErr_t errnum = KCP_SUCCESS;

	if (fut_unlock_fut (fut) == NULL) {
		errnum = KCP_PTERR_1;
		diagWindow ("unlockPT: fut_unlock_fut failed", errnum);
	}
	else {
		if ( ! unlockBuffer (PTHdr)) {
			errnum = KCP_MEM_UNLOCK_ERR;
			diagWindow ("unlockPT: unlockBuffer PTHdr failed", errnum);
		}
	}

	return errnum;

}	/* unlockPT */



/*
 * TpFreeHdr releases the header of a checked in fut.
 */
PTErr_t
	TpFreeHdr (	threadGlobals_p threadGlobalsP, 
				KcmHandle	PTHdr)
{

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}

	freeBuffer (PTHdr);		/* free the header info structure */

	return (KCP_SUCCESS);

}	/* TpFreeHdr */

/*
 * TpReadData reads a fut from a memory block and returns a handle to a newly allocated fut
 */
PTErr_t
	TpReadData(	threadGlobals_p threadGlobalsP, 
				KpFd_p			fd,
				PTType_t	format,
				PTRefNum_t	PTRefNum,
				KcmHandle	PTHdr,
				KcmHandle*	PTData)
{
PTErr_t errnum = KCP_SUCCESS;
fut_ptr_t fut = NULL, theFutFromMatrix = NULL, newFut = NULL, lab2xyzFut = NULL, finalFut = NULL;
fut_hdr_ptr_t futHdr;
Fixed_t	matrix[MF_MATRIX_DIM * MF_MATRIX_DIM];
int32	ret, iomask;
char	ENUM_String[20];

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}

	futHdr = (fut_hdr_ptr_t)lockBuffer (PTHdr);	/* get buffer pointer */
	if (futHdr == NULL) {
		errnum = KCP_MEM_LOCK_ERR;
		diagWindow ("TpReadData: lockBuffer failed", errnum);
		goto GetOut;
	}

	switch (format) {
	case FUT_CIGAM: /* fut with bytes reversed */
	case FUT_MAGIC: /* fut with bytes in correct order */
		fut = fut_alloc_fut ();	/* allocate a new fut structure */
		if (fut == FUT_NULL ) {
			errnum = KCP_NO_ACTIVATE_MEM;
			diagWindow ("TpReadData: fut_alloc_fut failed", errnum);
			return errnum;
		}

		if (fut_read_tbls (fd, fut, futHdr) != 1) {	/* read fut tables */
			errnum = KCP_PT_DATA_READ_ERR;
/*			diagWindow ("TpReadData: fut_read_tbls failed", errnum); */
		}

		if ((errnum == KCP_SUCCESS) && (fut_io_decode (fut, futHdr)) == 0) {
			errnum = KCP_PTERR_0;
/* 			diagWindow ("TpReadData: fut_io_decode failed", errnum); */
		}

		break;

	case PTTYPE_MFT1:
	case PTTYPE_MFT2:
		ret = fut_readMFutTbls (fd, &fut, futHdr, matrix);	/* read matrix fut tables */
		if (ret == 1) {
			if ( fut->idstr != NULL ) {
				fut_free_idstr (fut->idstr);
				fut->idstr = NULL;
			}
			if (isIdentityMatrix (matrix, MF_MATRIX_DIM) != 1) {
				if (gestaltNoFPU == kcpIsFPUpresent()) {
					ret = makeOutputMatrixXformNoFPU ((Fixed_p)&matrix, 8, &theFutFromMatrix);
				} else {
					ret = makeOutputMatrixXformFPU ((Fixed_p)&matrix, 8, &theFutFromMatrix);
				}
				if (ret != 1) {
					errnum = KCP_INCON_PT;
					goto GetOut;
				}
				else {
					iomask = fut_iomask ("([xyzt])");
					/* get the Lab to XYZ fut */  
					lab2xyzFut = get_lab2xyz (threadGlobalsP->processGlobalsP->iGP->PTCubeSize);
					newFut = fut_comp (theFutFromMatrix, lab2xyzFut, iomask);			
					fut_free (theFutFromMatrix);	/* free intermediate futs */
					fut_free (lab2xyzFut);

					if (newFut != FUT_NULL) {
						finalFut = fut_comp (fut, newFut, iomask);
					}			
					fut_free (fut);
					fut_free (newFut);
					fut = finalFut;

					/* set the input color space attribute to Lab */
					/* Set to KCM_CIE_LAB */
					KpItoa(KCM_CIE_LAB, ENUM_String);
					errnum = PTSetAttribute (PTRefNum, KCM_SPACE_IN, ENUM_String);
					if (errnum != KCP_SUCCESS) {
						goto GetOut;
					}

					/* set the input composition attribute to Lab */
					errnum = PTSetAttribute (PTRefNum, KCM_IN_CHAIN_CLASS_2, "6");
					if (errnum != KCP_SUCCESS) {
						goto GetOut;
					}

				}
			}

			if ((fut == FUT_NULL) || !fut_io_encode (fut, futHdr)) {	/* make the info header */
				errnum = KCP_INCON_PT;
				diagWindow("TpReadData: fut_io_encode failed", errnum);
				goto GetOut;
			}
		}
		else {
			errnum = KCP_NO_ACTIVATE_MEM;
		}

		break;

	default:
		break;
	}


GetOut:
	if ((errnum != KCP_SUCCESS) && (fut != FUT_NULL)) {
		fut_free (fut);
	}
	else {

	/* return handle to fut to caller */
		*PTData = (KcmHandle)fut_unlock_fut (fut);
	}

	if ( ! unlockBuffer (PTHdr)) {
		errnum = KCP_MEM_UNLOCK_ERR;
		diagWindow ("TpReadData: unlockBuffer failed", errnum);
		return errnum;
	}

	return errnum;

} /* TpReadData */


/*
 * TpFreeData frees all of the memory allocated for a fut.
 */
PTErr_t
	TpFreeData(	threadGlobals_p threadGlobalsP, 
				KcmHandle PTData)
{
	PTErr_t errnum = KCP_SUCCESS;
	fut_ptr_t fut;

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}

	if ((fut = fut_free_futH ((KpHandle_t)PTData)) != FUT_NULL) {
		errnum = KCP_PTERR_2;
		diagWindow ("TpFreeData: unlockBuffer failed", errnum);
	}

	return errnum;

} /* TpFreeData */

/*
 * TpWriteHdr writes the header of a fut to an external memory block.
 */
PTErr_t
	TpWriteHdr(	threadGlobals_p threadGlobalsP, 
				KpFd_p			fd,
				PTType_t	format,
				KcmHandle	PTHdr,
				int32		attrSize)
{
PTErr_t errnum = KCP_SUCCESS;
fut_hdr_ptr_t futHdr;

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}

	switch (format) {
	case PTTYPE_FUTF:	/* get buffer pointer */
		futHdr = (fut_hdr_ptr_t)lockBuffer (PTHdr);
		if (futHdr == NULL) {
			errnum = KCP_MEM_LOCK_ERR;
			diagWindow ("TpWriteHdr: lockBuffer failed", errnum);
			return errnum;
		}

		futHdr->idstr_len = attrSize;		/* insert size of attributes */
	
		if (fut_write_hdr (fd, futHdr) == 0) {	/* and write out the header */
			errnum = KCP_PT_HDR_WRITE_ERR;
			diagWindow ("TpWriteHdr: fut_write_hdr failed", errnum);
		}

		if ( ! unlockBuffer (PTHdr)) {
			errnum = KCP_MEM_UNLOCK_ERR;
			diagWindow ("TpWriteHdr: unlockBuffer failed", errnum);
		}

		break;

	case PTTYPE_MFT1:
	case PTTYPE_MFT2:
		errnum = KCP_SUCCESS;
		break;

	default:
		errnum = KCP_INVAL_PTTYPE;
		break;
	}
	
	return errnum;

} /* TpWriteHdr */


/*
 * TpWriteData writes the table data of a fut to an external memory block.
 */
PTErr_t
	TpWriteData(	threadGlobals_p threadGlobalsP, 
					KpFd_p			fd,
					PTType_t	format,
					KcmHandle	PTHdr,
					KcmHandle	PTData)
{
PTErr_t errnum,  errnum1;
fut_hdr_ptr_t futHdr;
fut_ptr_t fut;

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}

/* make sure the PT header and fut info match */
	errnum = lockPT (PTHdr, PTData, &futHdr, &fut);
	if (errnum != KCP_SUCCESS) {
		diagWindow ("TpWriteData: lockPT failed", errnum);
		return errnum;
	}

/* write out the tables */
	switch (format) {
	case PTTYPE_FUTF:
		if (fut_write_tbls (fd, fut, futHdr) == 0) {
			errnum = KCP_PT_DATA_WRITE_ERR;
			diagWindow ("TpWriteData: fut_write_tbls failed", errnum);
		}
		break;

	case PTTYPE_MFT1:
	case PTTYPE_MFT2:
		if (fut_writeMFut_Kp (fd, fut, NULL, format, FUT_OUTTBL_ENT) != 1) {
			errnum = KCP_PT_DATA_WRITE_ERR;
			diagWindow ("TpWriteData: fut_writeMFut failed", errnum);
		}

		break;

	default:
		errnum = KCP_INVAL_PTTYPE;
		break;
	}

	errnum1 = unlockPT (PTHdr, fut);
	if (errnum1 != KCP_SUCCESS) {
		diagWindow ("TpWriteData: unlockPT failed", errnum1);
	}

	if (errnum != KCP_SUCCESS) {
		return (errnum);
	} else {
		return (errnum1);
	}

} /* TpWriteData */

/*
 * TpGetDataSize returns the size in bytes of fut
 */

int32
	TpGetDataSize (	threadGlobals_p threadGlobalsP, 
					KcmHandle	PTHdr,
					KcmHandle	PTData,
					PTType_t	format)
{
int32		size = 0, futRet, LUTDimensions, inputChans, outputChans, gTableEntries, iTableEntries;
kcpindex_t	i1;
PTErr_t		errnum;
fut_hdr_ptr_t	futHdr;
fut_ptr_t		fut;
KpInt32_t		ret;

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return (0);
	}

/* make sure the PT header and fut info match */
	errnum = lockPT (PTHdr, PTData, &futHdr, &fut);
	if (errnum != KCP_SUCCESS) {
		diagWindow ("TpGetDataSize: lockPT failed", errnum);
		return (0);
	}

	switch (format) {
	case PTTYPE_FUTF:
		/* add up size of the input tables */
		for ( i1=0; i1<FUT_NICHAN; i1++ ) {
			if ( futHdr->icode[i1] == FUTIO_UNIQUE )
				size += fut_size_itbl (fut->itbl[i1]);
		}

		/* add up size of the output channels */
		for ( i1=0; i1<FUT_NOCHAN; i1++ ) {
			if ( fut->chan[i1] != 0 )
				size += fut_size_chan (fut->chan[i1], & futHdr->chan[i1]);
		}

		break;

	case PTTYPE_MFT1:
	case PTTYPE_MFT2:
		futRet = fut_mfutInfo (fut, &LUTDimensions, &inputChans, &outputChans);
		if (futRet != 1) {
			return (0);
		}

		gTableEntries = fut->chan[0]->gtbl->tbl_size / (int32)sizeof (fut_gtbldat_t);

		if (format == PTTYPE_MFT2) {
			ret = fut_getItblFlag (fut, (KpUInt32_t *)&size);
			if (ret == 0) {
				return (-1);
			}

			if (size > MF1_TBL_ENT)
				iTableEntries = size;
			else
				iTableEntries = MF1_TBL_ENT;
		}
		else
			iTableEntries = MF1_TBL_ENT;

		size = inputChans * iTableEntries;			/* total input table entries */

		size += outputChans * gTableEntries;	/* plus total grid table entries */

		if (format == PTTYPE_MFT1) {
			size += outputChans * MF1_TBL_ENT;		/* plus total output table entries */
			size *= sizeof (u_int8);				/* mult by bytes in each entry */
		}
		else if (format == PTTYPE_MFT2) {
			size += outputChans * MF2_MAX_TBL_ENT;	/* plus total output table entries */
			size *= sizeof (u_int16);				/* mult by bytes in each entry */

			size += 2 * sizeof (u_int16);	/* plus size of table counters */
		}

		break;

	default:
		return (0);

	}

 	errnum = unlockPT (PTHdr, fut);
	if (errnum != KCP_SUCCESS) {
		diagWindow ("TpGetDataSize: unlockPT failed", errnum);
		return (0);
	}

	return (size);

} /* TpGetDataSize */

/*
 * calculate the signed 32-bit fut cyclical redundancy check (CRC)
 */
 
PTErr_t
	TpCalCrc (	threadGlobals_p threadGlobalsP, 
				KcmHandle	PTHdr,
				KcmHandle	PTData,
				int32 FAR*	crc32)
{
PTErr_t			errnum;
fut_hdr_ptr_t	futHdr;
fut_ptr_t		fut;

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}

/* make sure the PT header and fut info match */
	errnum = lockPT (PTHdr, PTData, &futHdr, &fut);
	if (errnum != KCP_SUCCESS) {
		diagWindow ("TpWriteData: lockPT failed", errnum);
		return errnum;
	}

/* calculate the crc */
	fut_cal_crc (fut, crc32);

	errnum = unlockPT (PTHdr, fut);
	if (errnum != KCP_SUCCESS) {
		diagWindow ("TpWriteData: unlockPT failed", errnum);
		return errnum;
	}

	return errnum;
}

/*
 * fut_size_chan returns the size in bytes of a channel
 */

static int32
	fut_size_chan (	fut_chan_ptr_t	chan,
					chan_hdr_ptr_t	chanio)
{
	int32 size = 0;
	kcpindex_t	i1;

	if ( ! IS_CHAN (chan) )
		return (0);

/* add up size of the input tables */
	for ( i1=0; i1<FUT_NICHAN; i1++ ) {
		if ( chanio->icode[i1] == FUTIO_UNIQUE )
			size += fut_size_itbl (chan->itbl[i1]);
	}

/* add up size of the output table */
	if ( chanio->ocode == FUTIO_UNIQUE ) {
		size += fut_size_otbl (chan->otbl);
	}

/* add up size of the grid table */
	if ( chanio->gcode == FUTIO_UNIQUE ) {
		size += fut_size_gtbl (chan->gtbl);
	}

	return size;
}


/*
 * fut_size_itbl returns the size in bytes of an input table
 */

static int32
	fut_size_itbl (	fut_itbl_ptr_t	itbl)
{
	if ( ! IS_ITBL (itbl) )
		return (0);

	return ((sizeof (int32)*4) + (sizeof (int32) * (FUT_INPTBL_ENT+1)));
}


/*
 * fut_size_otbl returns the size in bytes of an output table
 */

static int32
	fut_size_otbl (	fut_otbl_ptr_t	otbl)	
{
	if ( ! IS_OTBL (otbl) )
		return (0);

	return ((sizeof (int32)*3) + (sizeof (int16)*FUT_OUTTBL_ENT));
}


/*
 * fut_size_gtbl returns the size in bytes of a grid table
 */

static int32
	fut_size_gtbl (	fut_gtbl_ptr_t	gtbl)
{

/* make sure gtbl has grid table array allocated */
	if ( ! IS_GTBL (gtbl) || gtbl->tbl == 0 )
		return (0);

	return (((int32)sizeof (int32)*5) + ((int32)sizeof (int16)*FUT_NCHAN) + gtbl->tbl_size);
}

PTErr_t
	TpSetImplicitAttr (	threadGlobals_p threadGlobalsP, 
						PTRefNum_t PTRefNum)
{
char attribute[256];
PTErr_t errnum = KCP_SUCCESS;
fut_hdr_ptr_t futHdr;
chan_hdr_ptr_t chan;
int32 i1, j, version;
int32 attributeTag, numOutVar = 0, numInVar[FUT_NCHAN];
KcmHandle PTHdr;
PTType_t format;

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}
/* get the private header info */
	PTHdr = getPTHdr (threadGlobalsP, PTRefNum);

/* get buffer pointer */
	futHdr = (fut_hdr_ptr_t)lockBuffer (PTHdr);
	if (futHdr == NULL) {
		errnum = KCP_MEM_LOCK_ERR;
		diagWindow ("TpSetImplicitAttr: lockBuffer failed", errnum);
		return errnum;
	}

	format = (PTType_t) futHdr->magic;
	version = futHdr->version;

	/* get the number of active output channels */
	/* and the number of active inputs for each channel */

	for (i1 = 0; i1 < FUT_NCHAN; i1++) {
		numInVar[i1] = 0;	/* zero out the number of inputs array */
	}

	switch (format) {
	case PTTYPE_FUTF:
		for (i1 = 0, chan = futHdr->chan; i1 < FUT_NCHAN; i1++, chan++) {
			if ((chan->gcode & FUTIO_CODE) != FUTIO_NULL) {
				numOutVar++;
				for (j=0; j<FUT_NCHAN; j++) {
					if ((chan->icode[j] & FUTIO_CODE) != FUTIO_NULL) {
						(numInVar[i1])++;
					}
				}
			}
		}

		break;

	case PTTYPE_MFT1:
	case PTTYPE_MFT2:
		numOutVar = futHdr->icode[1];		/* copy from header */ 
		for (i1 = 0; i1 < numOutVar; i1++) {
			numInVar[i1] = futHdr->icode[0];
		}

		break;

	default:
		numOutVar = 0;		/* make it obvious */ 
		break;

	}

/* set the technology type attribute */
	errnum = PTSetAttribute (PTRefNum, KCM_TECH_TYPE, KCM_FUT_S);
	if (errnum != KCP_SUCCESS) {
		goto GetOut;
	}

/* set the technology version attribute */
	KpItoa (version, attribute);
	errnum = PTSetAttribute (PTRefNum, KCM_TECH_VERSION, attribute);
	if (errnum != KCP_SUCCESS) {
		goto GetOut;
	}

/* set the attribute for number of inputs for each channnel  */
	for (i1 = 0, attributeTag = KCM_NUM_IN_VAR_1; i1<FUT_NCHAN; i1++) {
		if (numInVar[i1] != 0) {
			KpItoa (numInVar[i1], attribute);
			errnum = PTSetAttribute (PTRefNum, attributeTag, attribute);
			if (errnum != KCP_SUCCESS) {
				goto GetOut;
			}
			attributeTag++;
		}
	}

/* set the number of output channnels attribute */
	KpItoa (numOutVar, attribute);
	errnum = PTSetAttribute (PTRefNum, KCM_NUM_OUT_VAR, attribute);
	if (errnum != KCP_SUCCESS) {
		goto GetOut;
	}

GetOut:
	if ( ! unlockBuffer (PTHdr)) {
		errnum = KCP_MEM_UNLOCK_ERR;
		diagWindow ("TpSetImplicitAttr: unlockBuffer failed", errnum);
		return errnum;
	}

	return errnum;
}	/* TpSetImplicitAttr */



/* Generate attributes which must be in each PT */
PTErr_t
	TpGenerateAttr (threadGlobals_p threadGlobalsP, 
					PTRefNum_t PTRefNumR)
{											 
#if defined (KPMAC)
	#define KCP_STATIC
#else
	#define KCP_STATIC static
#endif

/* attribute structure */
	typedef struct attr_s {
		int32 tag;
		char* string;
	} attr_t;
	int32 attrSize;
	char attrBuf[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	char *copyrightPtr;

/* constant attribute strings */
	char strProdVer[] = "00.00.00";

#define ATTR_LIST_END 0
                     
/* attributes which are the same for all PTs */
	KCP_STATIC attr_t constAttr[] = {
		{KCM_TECH_TYPE, KCM_FUT_S},
		{KCM_RAW, KCM_UNKNOWN_S},
		{ATTR_LIST_END, NULL} };

	PTErr_t errnum = KCP_SUCCESS;
	int i1;

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}

	attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
	errnum = PTGetAttribute (PTRefNumR, KCM_PRODUCT_VERSION,
										&attrSize, attrBuf);

/* if product version isn't there, set it  */
	if (errnum == KCP_INVAL_PTA_TAG) {
		errnum = PTSetAttribute (PTRefNumR, KCM_PRODUCT_VERSION, strProdVer);
		if (errnum != KCP_SUCCESS) {
			return (errnum);
		}
	}
	
	/* If the copyright string isn't there, then generate it	*/
	
	attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
	errnum = PTGetAttribute (PTRefNumR, KCM_COPYRIGHT, &attrSize, attrBuf);
	if (errnum == KCP_INVAL_PTA_TAG) {
	
		if (!fut_make_copyright (attrBuf)) {
			return (KCP_COPYRIGHT_SIZE);
		}
		errnum = stripCopyright (attrBuf, &copyrightPtr);
		if (errnum != KCP_SUCCESS) {
			return (errnum);
		}	
		errnum = PTSetAttribute (PTRefNumR, KCM_COPYRIGHT, copyrightPtr);
		if (errnum != KCP_SUCCESS) {
			return (errnum);
		}
	}
	

	for (i1 = 0; constAttr[i1].tag != ATTR_LIST_END; i1++) {

	/* write to destination PT */
		errnum = PTSetAttribute (PTRefNumR, constAttr[i1].tag,
											constAttr[i1].string);
		if (errnum != KCP_SUCCESS) {
			return (errnum);
		}
	}

	return (errnum);
}	/* TpGenerateAttr */



/* The copyright string generated by fut_make_copyright has a "tag=" prefix.
	This function strips off that prefix by returning a pointer to the first
	character of the base string in the location pointed to by basePtr.			*/ 
static PTErr_t
	stripCopyright (	char_p		stringPtr,
						char_p FAR*	basePtr)
{
int32	length;

	length = (int32)strlen (stringPtr);
	if (length > FUT_COPYRIGHT_MAX_LEN) {
		return (KCP_COPYRIGHT_SIZE);
	}
	             
	while (length--) {
	
		switch (*stringPtr) {
		
			case '=':
				*basePtr = ++stringPtr;
				return (KCP_SUCCESS);
				
			case 0:
				*basePtr = 0;
				return (KCP_INV_COPYRIGHT);
				
			default:
				stringPtr++;
				break;
				
		} /* switch stringPtr */
		
	} /* while length */
	
/* we should never get here but if we do generate an error */

	return (KCP_INV_COPYRIGHT);
	
} /* stripCopyright */

