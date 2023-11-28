/*
 * @(#)ptiomem.c	2.45 97/12/22

	Contains:	functions to read and write binary PT "memory" files.

	Written by:	Drivin' Team

	Copyright (c) 1992-1995 Eastman Kodak Company, all rights reserved.

	The external database is responsible for loading and storing a block of memory
	which contains all of the PT information.  It needs to know only the location
	and size of this block.
	
	11 Jan 94	sek		Cleaned up warnings
	 8 Dec 93	HTR		Mod PTActivate to not makeActive() if CRC fails; add
						 checks for valid ptrs in API function arguments
	18 Nov 93	gbp		add PTGetSizeF and PTGetPTF
*/

#include <string.h>

#include "fut.h"
#include "fut_io.h"
#include "fut_util.h"
#include "kcms_sys.h"
#include "kcmptlib.h"
#include "kcptmgrd.h"
#include "attrib.h"
#include "kcptmgr.h"

/* prototypes */
int32	CollisionCheck ARGS((void));
static int PTMemTest ARGS((void));
static PTErr_t gridDimValid(threadGlobals_p	threadGlobalsP,
							PTType_t format, PTRefNum_t	PTRefNum,
							PTRefNum_p resizePTRefNum);

/* PTCheckIn reads the private data header and attributes of a PT in an
 * external memory block, then enters it into the checked in PT list.
 */

PTErr_t
	PTCheckIn (	PTRefNum_p	PTRefNumP,
				PTAddr_t	PTAddr)
{
	threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */
	PTErr_t		errnum;
	KpFd_t		fd;
	KcmHandle	PTHdr = NULL, PTAttr = NULL;
	fut_hdr_t	FAR *futp;
	PTType_t	format;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		errnum = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut5;
	}

/* Check for valid PTRefNumP */
	if (Kp_IsBadWritePtr(PTRefNumP, sizeof(*PTRefNumP))) {
		errnum = KCP_BAD_PTR;
		goto ErrOut4;
	}

/* initialize memory file manager to read the PT */
	if (Kp_open (NULL, "m", &fd, NULL, (KpGenericPtr_t)PTAddr, PTCHECKINSIZE) != KCMS_IO_SUCCESS) {
		errnum = KCP_SYSERR_1;
		goto ErrOut4;
	}

/* read the header info */
	errnum = TpReadHdr(threadGlobalsP, &fd, &PTHdr, &format);
	if (errnum != KCP_SUCCESS) {
		goto ErrOut1;
	}

	switch (format) {
	case PTTYPE_FUTF:
	case PTTYPE_FTUF:						/* discard the attribute info */
		futp = lockBuffer (PTHdr);	/* get the attributes */
		errnum = readAttributes(threadGlobalsP, &fd, futp->idstr_len, &PTAttr);
		unlockBuffer (PTHdr);
		if (errnum != KCP_SUCCESS) {
			goto ErrOut2;
		}

		if (!PTMemTest()) {		/* not enough memory to continue */
			errnum = KCP_NO_CHECKIN_MEM;
			goto ErrOut3;
		}

		break;

	default:
		break;
	}
	   	
/* enter PT into list */
	errnum = registerPT(threadGlobalsP, PTHdr, PTAttr, PTRefNumP);
	if (errnum != KCP_SUCCESS) {
		goto ErrOut3;
	}

	switch (format) {
	case PTTYPE_MFT1:
	case PTTYPE_MFT2:						/* discard the attribute info */

		errnum = TpGenerateAttr(threadGlobalsP, *PTRefNumP); /* generate constant attributes */
		break;

	default:
		break;
	}
	   	
	if (errnum != KCP_SUCCESS) {
ErrOut3:
		(void) freeAttributes(PTAttr);	/* free the attributes */
ErrOut2:
		(void) TpFreeHdr(threadGlobalsP, PTHdr);		/* free the header */
	}

ErrOut1:
	(void) Kp_close (&fd);

ErrOut4:
	KCMDunloadGlobals();					/* Unlock this apps Globals */

ErrOut5:
	return (errnum);
}


/* PTActivate reads the PT data from an external memory block and
 * sets up the technology specific PT memory structures.  Before loading
 * the PT, check to make sure that it matches the "checked in" info.
 */
PTErr_t
	PTActivate(	PTRefNum_t	PTRefNum,
				int32		mBlkSize,
				PTAddr_t	PTAddr)
{
PTErr_t	errnum;

	errnum = doActivate( PTRefNum, mBlkSize, PTAddr, 1L);

	return (errnum);
}

PTErr_t
	doActivate(	PTRefNum_t	PTRefNum,
				int32		mBlkSize,
				PTAddr_t	PTAddr,
				int32		crcMode)
{
	threadGlobals_p	threadGlobalsP;			/* a pointer to the process global */
	PTErr_t		errnum;
	KpFd_t		fd;
	KcmHandle	PTHdr, PTData, PTHdr2;
	fut_hdr_t	FAR *futp;
	char 		strCRCmade[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	char 		strCRCfound[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	int32		attrSize, crcAttrSize, crc32;
	PTType_t	format;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		errnum = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut4;
	}

	errnum = kcpGetStatus(threadGlobalsP, PTRefNum);
	if (errnum != KCP_PT_INACTIVE) {
		goto ErrOut1;
	}

	/* initialize memory file manager */
	if (Kp_open (NULL, "m", &fd, NULL, (KpGenericPtr_t)PTAddr, mBlkSize) != KCMS_IO_SUCCESS) {
		errnum = KCP_SYSERR_1;
		goto ErrOut1;
	}

/* read in the encoded header, verify that its the same as the original,
 * then discard */
	errnum = TpReadHdr (threadGlobalsP, &fd, &PTHdr2, &format);
	if (errnum != KCP_SUCCESS ) {
		goto ErrOut2;
	}

/* get and save size of attributes */
    futp = lockBuffer (PTHdr2);
	attrSize = futp->idstr_len;
	unlockBuffer (PTHdr2);

	PTHdr = getPTHdr(threadGlobalsP, PTRefNum);	/* get the original PT header */

/* make sure the PT header and checkin info match */
	errnum = TpCompareHdr(threadGlobalsP, PTHdr, PTHdr2);

	(void) TpFreeHdr(threadGlobalsP, PTHdr2);	/* free the header */

	if (errnum != KCP_SUCCESS) {	/* then check for an error in hdrVerify */
		goto ErrOut2;
	}

	switch (format) {
	case PTTYPE_FUTF:						/* discard the attribute info */
	case PTTYPE_FTUF:
		if (Kp_skip (&fd, attrSize) != KCMS_IO_SUCCESS){ /* may have been setAttribute after checkin */
			errnum = KCP_PTERR_3;
			goto ErrOut2;
		}

		break;

	default:
		break;
	}
	
	errnum = TpReadData (threadGlobalsP, &fd, format, PTRefNum, PTHdr, &PTData);		/* get the PT data */
	if (errnum == KCP_SUCCESS) {
		if (!PTMemTest()) {			/* enough memory to continue operations? */
			errnum = KCP_NO_ACTIVATE_MEM;
			goto ErrOut3;
		}

		if (crcMode != 0) {
			errnum = TpCalCrc (threadGlobalsP, PTHdr, PTData, &crc32);	/* calculate the CRC */
			if (errnum == KCP_SUCCESS) {
				KpItoa(crc32, strCRCmade);
				crcAttrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
				errnum = PTGetAttribute(PTRefNum, KCM_CRC, &crcAttrSize, strCRCfound);

				if (errnum == KCP_INVAL_PTA_TAG) {		/* if not present, just set it */
					PTSetAttribute(PTRefNum, KCM_CRC, strCRCmade);
					errnum = KCP_SUCCESS;
				}
				else {
					if ((errnum == KCP_SUCCESS)			/* if present, must match */
						&& (strcmp (strCRCmade, strCRCfound) != 0)) {
						errnum = KCP_INCON_PT;
						goto ErrOut3;
					}
				}
			}
		}
	}

	if (errnum == KCP_SUCCESS) {	/* Everything's OK, now activate */
		makeActive(threadGlobalsP, PTRefNum, PTData);
	}

ErrOut2:
	(void) Kp_close (&fd);
ErrOut1:
	KCMDunloadGlobals();					/* Unlock this apps Globals */
ErrOut4:
	return (errnum);


ErrOut3:
	(void) TpFreeData(threadGlobalsP, PTData);	/* Release the PT memory */
	goto ErrOut2;
}


/* register a new PT using externally defined header and data */
PTErr_t PTNewPT (PTAddr_t * PTHdr, PTAddr_t * PTData, PTRefNum_p PTRefNumP)
{
	threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */
	PTErr_t errnum;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		errnum = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut2;
	}

/* Check for valid PTHdr, PTData */
	if (Kp_IsBadReadPtr(PTHdr, sizeof(*PTHdr))) {
		errnum = KCP_BAD_PTR;
		goto ErrOut1;
	}

	if (Kp_IsBadReadPtr(PTData, sizeof(*PTData))) {
		errnum = KCP_BAD_PTR;
		goto ErrOut1;
	}

	if (Kp_IsBadWritePtr(PTRefNumP, sizeof(*PTRefNumP))) {
		errnum = KCP_BAD_PTR;
		goto ErrOut1;
	}

	errnum = registerPT(threadGlobalsP, PTHdr, NULL, PTRefNumP);	/* enter PT into list */

	if ((errnum == KCP_SUCCESS) && (PTData != NULL)) {
		makeActive(threadGlobalsP, *PTRefNumP, PTData);		/* activate the new PT */

		errnum = TpGenerateAttr(threadGlobalsP, *PTRefNumP); /* generate constant attributes */
		if (errnum != KCP_SUCCESS) {
			(void) PTCheckOut(*PTRefNumP);	/* clean up if error */
		}
	}

ErrOut1:
	KCMDunloadGlobals();					/* Unlock this apps Globals */
ErrOut2:
	return (errnum);
}

/* deactivate a checked in PT */
PTErr_t PTDeActivate (PTRefNum_t PTRefNum)
{
	threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */
	PTErr_t errnum;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		errnum = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut1;
	}

	errnum = kcpGetStatus(threadGlobalsP, PTRefNum);		/* must currently be active */

	if ((errnum == KCP_PT_ACTIVE) || (errnum == KCP_SERIAL_PT)) {
		errnum = makeInActive (threadGlobalsP, PTRefNum);	/* deactivate the PT */
	}

	KCMDunloadGlobals();							/* Unlock this apps Globals */
ErrOut1:
	return errnum;
}


/* check out a checked in PT. */
PTErr_t PTCheckOut (PTRefNum_t PTRefNum)
{
	threadGlobals_p	threadGlobalsP;					/* a pointer to the process global */
	PTErr_t 	errnum;

	threadGlobalsP = KCMDloadGlobals();				/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		errnum = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut;
	}

	errnum = PTDeActivate(PTRefNum);				/* free the PT data */
	if ((errnum == KCP_SUCCESS) || (errnum == KCP_PT_INACTIVE)) {
		errnum = makeCheckedOut (threadGlobalsP, PTRefNum);		/* release PT table */
	}

	KCMDunloadGlobals();							/* Unlock this apps Globals */

ErrOut:
	return (errnum);
}


/* PTGetPTInfo returns the header, attribute, and data of a PT. */
PTErr_t PTGetPTInfo(PTRefNum_t PTRefNum, PTAddr_t ** PTHdr, PTAddr_t ** PTAttr,
			PTAddr_t ** PTData)
{
	threadGlobals_p	threadGlobalsP;			/* a pointer to the process global */
	PTErr_t errnum;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		errnum = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut2;
	}

	errnum = kcpGetStatus(threadGlobalsP, PTRefNum);

	if ((errnum == KCP_PT_ACTIVE) || (errnum == KCP_PT_INACTIVE) || (errnum == KCP_SERIAL_PT)) {

		/* Check for valid ptrs */
		if ((PTHdr != NULL) && (Kp_IsBadWritePtr (PTHdr, sizeof(*PTHdr)))) {
			errnum = KCP_BAD_PTR;
			goto ErrOut1;
		}
		if ((PTAttr != NULL) && (Kp_IsBadWritePtr (PTAttr, sizeof(*PTAttr)))) {
			errnum = KCP_BAD_PTR;
			goto ErrOut1;
		}
		if ((PTData != NULL) && (Kp_IsBadWritePtr (PTData, sizeof(*PTData)))) {
			errnum = KCP_BAD_PTR;
			goto ErrOut1;
		}

		/* return the requested information */
		if (PTHdr != NULL) {
			*PTHdr = getPTHdr(threadGlobalsP, PTRefNum);
		}
		if (PTAttr != NULL) {
			*PTAttr = getPTAttr(threadGlobalsP, PTRefNum);
		}
		if ( ((errnum == KCP_PT_ACTIVE) || (errnum == KCP_SERIAL_PT)) && (PTData != NULL)) {
			*PTData = getPTData(threadGlobalsP, PTRefNum);
		}
	}

ErrOut1:
	KCMDunloadGlobals();					/* Unlock this apps Globals */
ErrOut2:
	return (errnum);
}


/* PTGetSize calculates the size of a PT in FuT format.  */
PTErr_t
	PTGetSize (	PTRefNum_t	PTRefNum,
				int32_p		mBlkSize)
{
	PTErr_t		errnum;

	errnum = PTGetSizeF (PTRefNum, PTTYPE_FUTF, mBlkSize);

	return (errnum);
}


/* PTGetSizeF calculates the size of a PT in any format.  */
PTErr_t
	PTGetSizeF (PTRefNum_t	PTRefNum,
				PTType_t	format,
				int32_p		mBlkSize)
{

	threadGlobals_p	threadGlobalsP;			/* a pointer to the process global */
	PTErr_t		errnum;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		errnum = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut1;
	}

	errnum = doGetSizeF (threadGlobalsP, PTRefNum, format, mBlkSize, 1L);

	KCMDunloadGlobals();					/* Unlock this apps Globals */
ErrOut1:
	return (errnum);
}


PTErr_t
	doGetSizeF (threadGlobals_p threadGlobalsP,
				PTRefNum_t	PTRefNum,
				PTType_t	format,
				int32_p		mBlkSize,
				int32		crcMode)
{
	PTErr_t		errnum;
	int32		extSize, intSize;
	KcmHandle	PTHdr, PTAttr, PTData;
	char 		strCRCmade[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	int32		crc32;
	PTRefNum_t	auxPTRefNum;

	errnum = kcpGetStatus(threadGlobalsP, PTRefNum);

	if ((errnum == KCP_PT_ACTIVE) || (errnum == KCP_PT_INACTIVE) || (errnum == KCP_SERIAL_PT)) {

		if (Kp_IsBadWritePtr(mBlkSize, sizeof(*mBlkSize)))
			return (KCP_BAD_PTR);

		switch (format) {
			case PTTYPE_FUTF:
			case PTTYPE_FTUF:
				extSize = sizeof(PT_t);		/* size of external header */
				break;

			case PTTYPE_MFT1:
			case PTTYPE_MFT2:
				extSize = (2 * sizeof (int32)) + (4 * sizeof (u_int8))
					+ (MF_MATRIX_DIM * MF_MATRIX_DIM * sizeof (int32));
				break;

			default:
				return (KCP_INVAL_PTTYPE);
		}
	   	
		if ((errnum == KCP_PT_ACTIVE) || (errnum == KCP_SERIAL_PT)) {

		/* when active, add size of external PT data block */
			PTHdr = getPTHdr(threadGlobalsP, PTRefNum);
			PTData = getPTData(threadGlobalsP, PTRefNum);
			intSize = TpGetDataSize (threadGlobalsP, PTHdr, PTData, format);
			if (intSize == 0) {

				/* 	TpGetDataSize will return 0 if the grid table 
					dimensions are not valid for the format specified.
					PTGetPTF will attempt to resize the grid table of
					the PT.  Check if that resizing is possible				*/
				errnum = gridDimValid(threadGlobalsP, format, PTRefNum, NULL);
  				if (errnum != KCP_SUCCESS) {
					return errnum;
				}

				/*	The PT will be resized, by precomposing it with the
					appropriate auxilliary PT.  The size of the aux PT will
					determine the size of the new PT.  Find the aux PT to
					be used.												*/
				errnum = getResizeAuxPT(threadGlobalsP, PTRefNum, &auxPTRefNum);
				if (errnum != KCP_SUCCESS) {
					return errnum;
				}

				/*	Determine the size of that auxPT						*/
				PTHdr = getPTHdr(threadGlobalsP, auxPTRefNum);
				PTData = getPTData(threadGlobalsP, auxPTRefNum);
				intSize = TpGetDataSize (threadGlobalsP, PTHdr, PTData, format);
				PTCheckOut (auxPTRefNum);
				if (intSize == 0) {
					return KCP_INCON_PT;
				}
			}
			extSize += intSize;	/* add size of data */

			switch (format) {
			case PTTYPE_FUTF:
				if (crcMode != 0) {
					errnum = TpCalCrc (threadGlobalsP, PTHdr, PTData, &crc32);
					if (errnum == KCP_SUCCESS) {
						KpItoa(crc32, strCRCmade);
						PTSetAttribute(PTRefNum, KCM_CRC, strCRCmade);
					}
				}
				break;

			default:
				break;
			}
		}
	   	
/* add size of attributes. Must be done after CRC calculation */
		switch (format) {
		case PTTYPE_FUTF:
			PTAttr = getPTAttr(threadGlobalsP, PTRefNum);
			extSize += getAttrSize(PTAttr);		/* plus size of attributes */

			break;

		default:
			break;
		}
	   	
		*mBlkSize = extSize;	/* return external size of PT */
		errnum = KCP_SUCCESS;
	}

	return (errnum);
}


/* PTGetPT writes a PT to external memory in FuT format.  */
PTErr_t
	PTGetPT (	PTRefNum_t	PTRefNum,
				int32		mBlkSize,
				PTAddr_t	PTAddr)
{
PTErr_t		errnum;

	errnum = PTGetPTF (PTRefNum, PTTYPE_FUTF, mBlkSize, PTAddr);

	return (errnum);
}


/* PTGetPTF writes a PT to external memory in any format.  */
PTErr_t
	PTGetPTF (	PTRefNum_t	PTRefNum,
				PTType_t	format,
				int32		mBlkSize,
				PTAddr_t	PTAddr)
{
	threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */
	PTErr_t		errnum, PTstatus;
	KcmHandle	PTHdr, PTAttr, PTData;
	KpFd_t		fd;
	int32		attrSize, resultSize, nBytes;
	PTRefNum_t	resizePTRefNum, thePTRefNum;
	char_p		memData;


	threadGlobalsP = KCMDloadGlobals();
	if (threadGlobalsP == NULL) {
		errnum = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut2;
	}

	errnum = kcpGetStatus(threadGlobalsP, PTRefNum);

	if ((errnum == KCP_PT_ACTIVE) || (errnum == KCP_PT_INACTIVE) || (errnum == KCP_SERIAL_PT)) {
		PTstatus = errnum;

		/* 	verify the dimensions of the grid table are valid for the
			specified format.  If they are not the right size attempt to
			create a grid table of the appropriate size						*/
		errnum = gridDimValid (threadGlobalsP, format, PTRefNum, 
							   &resizePTRefNum);
		if (errnum != KCP_SUCCESS) {
			diagWindow ("PTGetPTF: validate grid table dimensions failed",
						errnum);
			goto ErrOut1;
		}

		if (resizePTRefNum != 0) {
			thePTRefNum = resizePTRefNum;
		
			/*	determine the size of the resized PT.  It may be smaller 
				than than the buffer size, but it can't be larger			*/
			errnum = PTGetSizeF (thePTRefNum, format, &resultSize);
			if (errnum != KCP_SUCCESS) {
				goto ErrOut1;
			}
			if (resultSize > mBlkSize) {
				errnum = KCP_PT_BLOCK_TOO_SMALL;
				goto ErrOut1;
			}
		}
		else {
			thePTRefNum = PTRefNum;
			resultSize = mBlkSize;
		}

		PTAttr = getPTAttr (threadGlobalsP, thePTRefNum);
		PTHdr = getPTHdr (threadGlobalsP, thePTRefNum);
		PTData = getPTData (threadGlobalsP, thePTRefNum);

		/* initialize memory file manager to write the PT */
		if (Kp_open (NULL, "m", &fd, NULL, (KpGenericPtr_t)PTAddr, mBlkSize) != KCMS_IO_SUCCESS) {
			errnum = KCP_SYSERR_1;
			goto ErrOut1;
		}

		attrSize = getAttrSize (PTAttr);					/* get size of attributes */
		errnum = TpWriteHdr (threadGlobalsP, &fd, format, PTHdr, attrSize);	/* write the header info */

		if (errnum != KCP_SUCCESS) {
			Kp_close (&fd);
			goto ErrOut1;
		} 	

		switch (format) {
		case PTTYPE_FUTF:
			errnum = writeAttributes (threadGlobalsP, &fd, PTAttr);	/* write the attributes */
			if (errnum != KCP_SUCCESS) {
				break;
			}

		default:
			break;
		}
	
		/* if PT active, write data to external format */
		if (((PTstatus == KCP_PT_ACTIVE) || (PTstatus == KCP_SERIAL_PT)) && (errnum == KCP_SUCCESS)) {
			errnum = TpWriteData (threadGlobalsP, &fd, format, PTHdr, PTData);
		}
		(void) Kp_close (&fd);

		/*	if the result PT size is smaller than the memory block size
			fill the end of the memory block with zeros						*/
		nBytes = mBlkSize - resultSize;
		if (nBytes > 0) {
			memData = (char_p)PTAddr + resultSize;
			while (nBytes--) {
				*memData++ = 0;
			}
		}			

	}

ErrOut1:
	if (resizePTRefNum != 0) {
		PTCheckOut (resizePTRefNum);
	}
	KCMDunloadGlobals();
ErrOut2:
	return (errnum);
}



/* PTMemTest checks whether there is adequate room between the top of the
 * heap and the bottom of the stack to allow procedure calls to be made
 * without collision
 */
static int PTMemTest(void)
{
int32 memSize;

	memSize = CollisionCheck();
	if (memSize < MINPTMEMSIZE)
		return (0);
	else
		return (1);
}


/*
 * gridDimValid determines whether the grid table dimensions are valid
 * for the format specified.  If the dimensions are not valid and 
 * resizePTRefNumP is not NULL then the function attempts to create a
 * PT with the correct size grid tables.  If it is successful the
 * PTRefNum of the resized PT is returned in the location pointed to by
 * resizePTRefNumP.  If resizing is not required the value returned in 
 * the location pointed to by resizePTRefNumP is 0.
 *
 * NOTE:  	If this function creates a resized PT, that PT is checked in.  
 *			it is the responsibility of the calling function to check out
 *			that PT.
 */
static PTErr_t
	gridDimValid(	threadGlobals_p	threadGlobalsP,
					PTType_t		format,
					PTRefNum_t		PTRefNum,
					PTRefNum_p		resizePTRefNumP)
{

KcmHandle		PTData;
KpInt32_t		inputChans, outputChans, LUTDimensions;
PTRefNum_t		auxPTRefNum;
fut_ptr_t 		fut;
PTErr_t			retVal, error = KCP_SUCCESS;

	/*	Assume no resizing */
	if (NULL != resizePTRefNumP) {
		*resizePTRefNumP = 0;
	}

	/*	Convert the PTRefNum to a fut */
	PTData = getPTData (threadGlobalsP, PTRefNum);
	fut = fut_lock_fut (PTData);
	if (fut == FUT_NULL) {
		diagWindow ("gridDimValid: fut_lock_fut 1 failed", KCP_PTERR_2);
		return KCP_PTERR_2;
	}

	if ( ! IS_FUT (fut) ) {	/* check for valid fut */
		diagWindow ("lockPT: IS_FUT failed", KCP_NOT_FUT);
		retVal = KCP_NOT_FUT;
		goto GetOut;
	}

	switch (format ) {

	case PTTYPE_FUTF:
		/* 	may want to check if any of the grid dimensions exceed the max 
			grid dimension, but for now accept any size						*/
		break;

	case PTTYPE_MFT1:
	case PTTYPE_MFT2:
		/*	The grid dimensions must all be the same.  If they are not then
			attempt to build a grid table where all the dimensions are the
			same.															*/
		retVal = (PTErr_t) fut_mfutInfo (fut, &LUTDimensions, &inputChans, 
									   &outputChans);
		if (1 != retVal) {
			if (-2 == retVal) {
				
				/*	if resizePTRefNumP is NULL then return success.			*/
				if (NULL == resizePTRefNumP) {
					break;
				}
				
				/*	Find the appropriate aux PT to precompose with the
					original PT to get the correct size grid table.			*/
				retVal = getResizeAuxPT (threadGlobalsP, PTRefNum, 
									 	 &auxPTRefNum);
				if (KCP_SUCCESS != retVal) {
					diagWindow ("gridDimValid: Error getting Resize PT", retVal);
					goto GetOut;
				}

			/*	Do the pre composition	*/
/* unlock this fut until we start counting locks ... */
				fut_unlock_fut (fut);
				retVal = PTCombine (PT_COMBINE_STD, auxPTRefNum, PTRefNum,
									resizePTRefNumP);
				if (KCP_SUCCESS != retVal) {
					diagWindow ("gridDimValid: PTCombine failed", retVal);
					error = retVal;
					*resizePTRefNumP = 0;
				}
/* ... then lock it again!!! */
				fut = fut_lock_fut (PTData);
				if (fut == FUT_NULL) {
					diagWindow ("gridDimValid: fut_lock_fut 2 failed", KCP_PTERR_2);
				}

				/*	Free the aux PT */
				retVal = PTCheckOut(auxPTRefNum);
				if (retVal != KCP_SUCCESS) {
					diagWindow(" gridDimValid: PTCheckOut aux PT failed", retVal);
					*resizePTRefNumP = 0;
					goto GetOut;
				}
				if (KCP_SUCCESS != error) {
					retVal = error;
					goto GetOut;
				}
 			} /* if (retVal == -2) */
			else {
				retVal = KCP_INVAL_GRID_DIM;
				goto GetOut;
			}
		} /* if (retVal != 1) */
		break;

	default:
		retVal = KCP_INVAL_PTTYPE;
	}

	retVal = KCP_SUCCESS;

GetOut:
	fut_unlock_fut (fut);
	return retVal;

} /* gridDimValid */ 



