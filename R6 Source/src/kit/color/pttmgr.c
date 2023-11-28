/*
 * @(#)pttmgr.c	2.24 97/12/22

	Contains:	PT table management

	Written by:	Drivin' Team

	Copyright:	(c) 1991-1995 Eastman Kodak Company, all rights reserved.

	Change History (most recent first):

		 <2>	 1/11/94	sek		Cleaned up warnings
 */


#if defined (KPMAC)
	#include <Packages.h>
	#include <Memory.h>
#endif

#include "kcms_sys.h"
#include "kcmptlib.h"
#include "kcptmgrd.h"
#include "kcptmgr.h"
#include "kcpfchn.h"


#define PTT_UNUSED	0

/* globals */
static int32 curRefNum = PTT_UNUSED + 1;	/* current reference number counter */

/* prototypes */
static void freePTTable ARGS((threadGlobals_p threadGlobalsP, PTTable_p* ptt));
static PTErr_t newPTTable ARGS((threadGlobals_p threadGlobalsP, PTTable_p** PTTable));
static PTTable_p* getPTTable ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum));
static PTTable_p GetTableP ARGS((PTTable_p* ptt_h));
static void SetRootHandle ARGS((threadGlobals_p threadGlobalsP, PTTable_p*));
static PTTable_p* GetRootHandle ARGS((threadGlobals_p threadGlobalsP));


/***********************************************************************/

/* free a PT table */
static void
	freePTTable (threadGlobals_p threadGlobalsP, PTTable_p * ptt_h)
{
PTTable_p ptt_p, next_p, prev_p;
int32_p			attrTagCountP;
	
	ptt_p = GetTableP(ptt_h);
	
/* free space allocated to this PTs' attributes */
	if (ptt_p->attrBase != NULL) {
		attrTagCountP = lockBuffer (ptt_p->attrBase);
		if (*attrTagCountP != 0) {
			diagWindow (" freeing pt table before freeing it's tags.", 0);
			freeAttributes(ptt_p->attrBase);
		}
		freeBuffer (ptt_p->attrBase);
	}

/* unlink block from list */

	next_p = GetTableP((PTTable_p*)ptt_p->next);
	prev_p = GetTableP((PTTable_p*)ptt_p->prev);
	
	if (next_p == ptt_p)	{	/* last one? */
		if (prev_p != ptt_p) {	/* just to be sure */
			diagWindow (" freePTTable: prev_p != ptt_p", 0);
		}
		SetRootHandle (threadGlobalsP, NULL);				/* no more PT table entries */
	}
	else {
		if (ptt_h == GetRootHandle(threadGlobalsP)) {		/* if this is the root table */
			SetRootHandle (threadGlobalsP, ptt_p->next);	/* must define new root table */
		}
		next_p->prev = ptt_p->prev;
		prev_p->next = ptt_p->next;
	}

	unlockBuffer ((KcmHandle)ptt_p->next);
	unlockBuffer ((KcmHandle)ptt_p->prev);
	unlockBuffer ((KcmHandle)ptt_h);

/* free space allocated to this table */
	freeBuffer ((KcmHandle)ptt_h);
}


/***********************************************************************/
/* clear out a PT table entry */

static PTErr_t
	clearPTTable (PTTable_p PTTable)
{
KpInt32_t	i;

	PTTable->refNum = PTT_UNUSED;				/* PT is unused */
	PTTable->hdr = NULL;
	PTTable->attrBase = NULL;
	PTTable->data = NULL;
	PTTable->checkInFlag = NOT_CHECKED_IN;		/* show this pt not checked in */
	PTTable->serialPTflag = NOT_SERIAL_PT;		/* show this pt not a serial PT */
	PTTable->inUseCount = 0;					/* set in use count */
	PTTable->serialCount = 0;					/* no serial PTs yet */

	for (i = 0; i < MAX_PT_CHAIN_SIZE; i++) {
		PTTable->serialDef[i] = PTT_UNUSED;
	}

	return (KCP_SUCCESS);
}


/***********************************************************************/
/* allocate memory for the PT table and initialize to all unused */
PTErr_t
	initPTTable (threadGlobals_p threadGlobalsP)
{
	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	SetRootHandle (threadGlobalsP, NULL);	/* no PT table entries */

	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	return (KCP_SUCCESS);
}



/***********************************************************************/
/* make a new PT table entry */

static PTErr_t
	newPTTable (threadGlobals_p threadGlobalsP, PTTable_p ** PTTable)
{
PTTable_p	*newH, new, *firstH, first, last;
PTErr_t PTErr;

	newH = (PTTable_p*)allocSysBufferHandle (sizeof (PTTable_t));	/* allocate new PT and get a handle to it */
	if (newH == NULL) {
		return (KCP_NO_CHECKIN_MEM);
	}

	new = (PTTable_p)lockBuffer ((KcmHandle)newH);	/* get the table pointer */
		
	PTErr = clearPTTable (new);						/* clear the entry */
	if (PTErr != KCP_SUCCESS) {
		freeBuffer ((KcmHandle)newH);			/* free space allocated to new table */
		return (PTErr);
	}

	firstH = GetRootHandle(threadGlobalsP);			/* get the first entry in the table */
	if (firstH == NULL) {							/* empty table? */
		new->next = new->prev = newH;				/* links to itself */
	}
	else {
		first = GetTableP(firstH);
		last = GetTableP(first->prev);				/* get the last entry in the table */
		
		new->next = firstH;							/* link new PT into the list */
		new->prev = first->prev;
		first->prev = newH;
		last->next = newH;

		unlockBuffer((KcmHandle)new->next);
		unlockBuffer((KcmHandle)GetRootHandle(threadGlobalsP));
	}

	unlockBuffer((KcmHandle)newH);

	SetRootHandle(threadGlobalsP, newH);			/* define new root */
	*PTTable = newH;								/* return table entry handle */

	return (KCP_SUCCESS);
}


/***********************************************************************/
/* get the PT table handle for a given PT reference number */
static PTTable_p*
	getPTTable (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum)
{
PTTable_p* ptt_h, * next, ptt_p;
PTRefNum_t thisRefNum;

	if (PTRefNum == PTT_UNUSED) {
		return (NULL);
	}

	ptt_h = GetRootHandle(threadGlobalsP);
	if (ptt_h != NULL) {	
		do {
			ptt_p = GetTableP(ptt_h);				/* get this table */
			next = (PTTable_p*)ptt_p->next;	/* and its info */
			thisRefNum = ptt_p->refNum;
			unlockBuffer((KcmHandle)ptt_h);

			if (thisRefNum == PTRefNum) {
				return (ptt_h);								/* found!! return handle of PTRefNum PT */
			}
			ptt_h = next;										/* not, move to next one */
			
		} while (ptt_h != GetRootHandle(threadGlobalsP));
	}

	return (NULL);		/* not found */
}


/***********************************************************************/
/* delete the given PT table */
void
	deletePTTable (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum)
{
PTTable_p* ptt;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	ptt = getPTTable (threadGlobalsP, PTRefNum);		/* get table handle */

	if (ptt != NULL) {
		freePTTable (threadGlobalsP, ptt);			/* it's not used */
	}

	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
}


/**************************************************************************/
/* register a PT by setting its implicit attributes and entering its info */
/* into the PT list */
PTErr_t
	registerPT (threadGlobals_p threadGlobalsP,
					KcmHandle hdr, 
					KcmHandle attrBase,
					PTRefNum_t * PTRefNum)
{
PTTable_p* PTTable;
PTTable_p  Table_p;
PTErr_t		errnum;
PTRefNum_t	theRN;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	theRN = PTT_UNUSED;

	errnum = newPTTable (threadGlobalsP, &PTTable);		/* get a new table */

	if (errnum == KCP_SUCCESS) {
		Table_p = GetTableP(PTTable);

		theRN = curRefNum++;					/* get reference number, increment for next time */
		Table_p->refNum = theRN;				/* save reference number */

		Table_p->hdr = hdr;						/* save header address */
		Table_p->attrBase = attrBase;			/* save attribute address */
		Table_p->data = NULL;					/* show there's no PT data */
		Table_p->checkInFlag = IS_CHECKED_IN;	/* show this pt is checked in */
		unlockBuffer((KcmHandle)PTTable);

		if (hdr != NULL) {
			errnum = TpSetImplicitAttr(threadGlobalsP, theRN);	/* set attributes using header info */
			if (errnum != KCP_SUCCESS) {
				theRN = PTT_UNUSED;
			}
		}

	}

	*PTRefNum = theRN;							/* return number to caller */
	
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	return (errnum);

}

/***********************************************************************/
/* get a PT table entry and load it with given serial PT info */
PTErr_t addSerialPT (threadGlobals_p threadGlobalsP,
						PTRefNum_t  CurrentPTRefNum,
						PTRefNum_t  InputPTRefNum,
						PTRefNum_p	ResultPTRefNum)
{
PTTable_p* CurrentPTTable, *InputPTTable, *ResultPTTable = NULL;
PTTable_p  CurrentPTTableP = NULL, InputPTTableP = NULL, ResultPTTableP = NULL;
PTErr_t		errnum;
KpInt32_t	newSerialCount, inputSerialCount, i1;
	
	CurrentPTTable = getPTTable (threadGlobalsP, CurrentPTRefNum);
	if (CurrentPTTable == NULL) {
		return (KCP_NOT_CHECKED_IN);
	}

	InputPTTable = getPTTable (threadGlobalsP, InputPTRefNum);
	if (InputPTTable == NULL) {
		return (KCP_NOT_CHECKED_IN);
	}

	errnum = registerPT(threadGlobalsP, NULL, NULL, ResultPTRefNum);
	if (errnum == KCP_SUCCESS) {
		makeSerial (threadGlobalsP, *ResultPTRefNum);
		ResultPTTable = getPTTable (threadGlobalsP, *ResultPTRefNum);
	}
	
	if (ResultPTTable == NULL) {
		return (KCP_NOT_CHECKED_IN);
	}
	
	CurrentPTTableP = GetTableP(CurrentPTTable);	
	ResultPTTableP = GetTableP(ResultPTTable);
		
	newSerialCount = 0;

	/* move in the 1st PT */
	if (CurrentPTTableP->serialCount == 0) {
		if (CurrentPTTableP->data != NULL) {
			ResultPTTableP->serialDef[0] = CurrentPTRefNum;
			makeActive (threadGlobalsP, CurrentPTRefNum, CurrentPTTableP->data);
			newSerialCount = 1;
		}
	} else {
		newSerialCount = CurrentPTTableP->serialCount;
		if (newSerialCount >= MAX_PT_CHAIN_SIZE) {
			errnum = KCP_EXCESS_PTCHAIN;
			goto ErrOut;
		}
		for (i1 = 0; i1 < newSerialCount; i1++) {
			ResultPTTableP = GetTableP(ResultPTTable);
			CurrentPTTableP = GetTableP(CurrentPTTable);	
			ResultPTTableP->serialDef[i1] = CurrentPTTableP->serialDef[i1];
			makeActive (threadGlobalsP, ResultPTTableP->serialDef[i1], NULL);
		}
	}
	
	CurrentPTTableP = GetTableP(CurrentPTTable);	
	InputPTTableP = GetTableP(InputPTTable);	
	ResultPTTableP = GetTableP(ResultPTTable);
		
	/* now add in the 2nd PT */
	if (InputPTTableP->serialCount == 0) {
		if (InputPTTableP->data != NULL) {
			if (newSerialCount >= MAX_PT_CHAIN_SIZE) {
				errnum = KCP_EXCESS_PTCHAIN;
				goto ErrOut;
			}
			ResultPTTableP->serialDef[newSerialCount] = InputPTRefNum;
			makeActive (threadGlobalsP, InputPTRefNum, InputPTTableP->data);
			newSerialCount++;
		}
	}
	else {
		if ((newSerialCount+CurrentPTTableP->serialCount) > MAX_PT_CHAIN_SIZE) {
			errnum = KCP_EXCESS_PTCHAIN;
			goto ErrOut;
		}
		inputSerialCount = InputPTTableP->serialCount;
		for (i1 = 0; i1 < inputSerialCount; i1++) {
			ResultPTTableP = GetTableP(ResultPTTable);
			CurrentPTTableP = GetTableP(CurrentPTTable);	
			ResultPTTableP->serialDef[i1] = CurrentPTTableP->serialDef[newSerialCount];
			makeActive (threadGlobalsP, ResultPTTableP->serialDef[newSerialCount], NULL);
			newSerialCount++;
		}
	}

	ResultPTTableP = GetTableP(ResultPTTable);
	ResultPTTableP->serialCount = newSerialCount;

ErrOut:
	if (CurrentPTTableP != NULL) 
		unlockBuffer((KcmHandle)CurrentPTTable);
	if (InputPTTableP != NULL) 
		unlockBuffer((KcmHandle)InputPTTable);
	if (ResultPTTableP != NULL) 
		unlockBuffer((KcmHandle)ResultPTTable);
	
	return (errnum);
}


/***********************************************************************/
/* get a PT table entry and load it with given serial PT info */
PTErr_t
	freeSerialData (threadGlobals_p threadGlobalsP,
						PTRefNum_t  PTRefNum)
{
PTTable_p*	PTTable;
PTTable_p	PTTableP;
PTErr_t		errnum = KCP_SUCCESS;
KpInt32_t	i, theCount;
PTRefNum_t	theRefNum;	

	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
		errnum = KCP_NOT_CHECKED_IN;
	}
	else {
		PTTableP = GetTableP(PTTable);	
		theCount = PTTableP->serialCount;
		PTTableP->serialCount = 0;
		PTTableP->serialPTflag = NOT_SERIAL_PT;		/* show this pt not a serial PT */
		
		for (i = 0; i < theCount; i++) {
			PTTableP = GetTableP(PTTable);
			theRefNum = PTTableP->serialDef[i];
			PTTableP->serialDef[i] = PTT_UNUSED;
			errnum = makeInActive(threadGlobalsP, theRefNum);	/* deactivate the PT */
		}

		unlockBuffer((KcmHandle)PTTable);
	}

	return (errnum);
}

#if !defined (PTTest)
/***********************************************************************/
/* free the PT table entries of an application */
PTErr_t
	freeApplPT (threadGlobals_p threadGlobalsP)
{
PTTable_p * ptt_h, ptt_p;
PTRefNum_t thisRefNum;
PTErr_t	errnum = KCP_SUCCESS;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	while ((ptt_h = GetRootHandle(threadGlobalsP)) != NULL) {	
		ptt_p = GetTableP(ptt_h);							/* get this table */
		thisRefNum = ptt_p->refNum;							/* get it's refNum */
		unlockBuffer((KcmHandle)ptt_h);

		errnum = makeInActive(threadGlobalsP, thisRefNum);	/* deactivate the PT */
		errnum = PTCheckOut (thisRefNum);					/* release this one */
		if ((errnum != KCP_SUCCESS) && (errnum != KCP_NOT_CHECKED_IN)) {
			diagWindow (" freeApplPT: checkOut failed.", errnum);
		}
	}

	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	return (errnum);
}

#endif


/***********************************************************************/
/* make the given PT active */
void makeActive (threadGlobals_p threadGlobalsP,
					PTRefNum_t PTRefNum,
					KcmHandle PTData)
{
	PTTable_p*	PTTable;
	PTTable_p	PTTableP;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
/*		diagWindow (" makeActive: not checked in", KCP_NOT_CHECKED_IN);*/
	}
	else {
		PTTableP = GetTableP(PTTable);	
		
		if (PTTableP->data == NULL) {
			PTTableP->data = PTData;		/* define PT address if not already */
			PTTableP->inUseCount = 1;		/* used once */
		}
		else {
			PTTableP->inUseCount++;			/* count the PT */
		}
		
		unlockBuffer((KcmHandle)PTTable);
	}
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
}


/***********************************************************************/
/* make the given PT active */
void
	makeSerial (threadGlobals_p	threadGlobalsP,
				PTRefNum_t		PTRefNum)
{
	PTTable_p*	PTTable;
	PTTable_p	PTTableP;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable != NULL) {
		PTTableP = GetTableP(PTTable);	

		PTTableP->serialPTflag = IS_SERIAL_PT;	/* show this is a serial PT */

		unlockBuffer((KcmHandle)PTTable);
	}
	
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
}


/***********************************************************************/
/* make the given PT inactive */
PTErr_t makeInActive (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum)
{
PTTable_p*	PTTable;
PTTable_p	PTTableP;
KcmHandle	PTData;
PTErr_t		errnum = KCP_SUCCESS;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
		errnum = KCP_NOT_CHECKED_IN;
	}
	else {
		PTTableP = GetTableP(PTTable);	
		errnum = freeSerialData (threadGlobalsP,  PTRefNum);	/* free any serial data */
				
		PTTableP = GetTableP(PTTable);			/* handle may have been moved */
		if (PTTableP->data != NULL) {
			PTTableP->inUseCount--;
			if (PTTableP->inUseCount == 0) {

				PTData = PTTableP->data;
				PTTableP->data = NULL;
				errnum = TpFreeData(threadGlobalsP, PTData);

				PTTableP = GetTableP(PTTable);			/* handle may have been moved */
				if (PTTableP->checkInFlag == NOT_CHECKED_IN) {
		 			deletePTTable (threadGlobalsP, PTRefNum);
				}
				else {
					unlockBuffer((KcmHandle)PTTable);
				}
			}
			else {
				unlockBuffer((KcmHandle)PTTable);
			}
		}
	}
	
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	return (errnum);
}


/***********************************************************************/
/* make the given PT inactive */
PTErr_t makeCheckedOut (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum)
{
	PTTable_p*	PTTable;
	PTTable_p	PTTableP;
	KcmHandle	PTHdr, PTAttrBase;
	KpUInt32_t	PTinUseCount;
	PTErr_t		errnum = KCP_SUCCESS, errnum1 = KCP_SUCCESS;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
		errnum = KCP_NOT_CHECKED_IN;
	}
	else {
		PTTableP = GetTableP(PTTable);	
		if (PTTableP->checkInFlag == NOT_CHECKED_IN) {
			errnum = KCP_NOT_CHECKED_IN;
			unlockBuffer((KcmHandle)PTTable);
		}
		else {
			PTHdr = PTTableP->hdr;
			PTTableP->hdr = NULL;
			PTAttrBase = PTTableP->attrBase;
			PTinUseCount = PTTableP->inUseCount;
			PTTableP->checkInFlag = NOT_CHECKED_IN;
			
			errnum = TpFreeHdr(threadGlobalsP, PTHdr);		/* free the header */
			errnum1 = freeAttributes(PTAttrBase);			/* free the attributes */

			if (PTinUseCount == 0) {
	 			deletePTTable (threadGlobalsP, PTRefNum);
			}
			else {
				unlockBuffer((KcmHandle)PTTable);
			}
		}
	}
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	if (errnum == KCP_SUCCESS) {
		errnum = errnum1;
	}
	return (errnum);
}


/***********************************************************************/
/* get the status for a given PT reference number */
PTErr_t kcpGetStatus (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum)
{
	PTTable_p*	PTTable;
	PTTable_p	PTTableP;
	PTErr_t 	status;
	
	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
/*		diagWindow (" kcpGetStatus: not checked in", KCP_NOT_CHECKED_IN);*/
		status = KCP_NOT_CHECKED_IN;
	}
	else {
		PTTableP = GetTableP(PTTable);	
		if (PTTableP->serialPTflag == IS_SERIAL_PT) {
			status = KCP_SERIAL_PT;			/* must be a serial pt */
		} else 
		if (PTTableP->checkInFlag == NOT_CHECKED_IN) {
			status = KCP_PT_INVISIBLE;		/* must be a serial component */
		} else 
		if (PTTableP->inUseCount == 0) {
			status = KCP_PT_INACTIVE;		/* not active */
		} else {
			status = KCP_PT_ACTIVE;			/* active */
		}
		unlockBuffer((KcmHandle)PTTable);
	}
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	return (status);
}


/***********************************************************************/
/* get the PT header address for a given PT reference number */
KcmHandle getPTHdr (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum)
{
	PTTable_p* PTTable;
	PTTable_p  PTTableP;
	KcmHandle hdrHandle;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
/*		diagWindow (" getPTHdr: not checked in", KCP_NOT_CHECKED_IN);*/
		hdrHandle = (KcmHandle) 0;
	}
	else {
		PTTableP = GetTableP(PTTable);
		hdrHandle = PTTableP->hdr;
		unlockBuffer((KcmHandle)PTTable);
	}
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	return (hdrHandle);
}

/***********************************************************************/
/* get the PT header address for a given PT reference number */
PTErr_t setPTHdr (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum, KcmHandle hdrHandle)
{
	PTTable_p*	PTTable;
	PTTable_p	PTTableP;
	PTErr_t		PTErr = KCP_BAD_PTR;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
/*		diagWindow (" getPTHdr: not checked in", KCP_NOT_CHECKED_IN);*/
		hdrHandle = (KcmHandle) 0;
		PTErr = KCP_NOT_CHECKED_IN;
	}
	else {
		PTTableP = GetTableP(PTTable);
		PTTableP->hdr = hdrHandle;
		unlockBuffer((KcmHandle)PTTable);
	}
	if (hdrHandle != NULL) {
		/* set attributes using header info */
		PTErr = TpSetImplicitAttr(threadGlobalsP, PTRefNum);
	}

	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	return (PTErr);
}


/***********************************************************************/
/* get the attribute base for a given PT reference number */
KcmHandle getPTAttr (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum)
{
	PTTable_p* PTTable;
	PTTable_p  PTTableP;
	KcmHandle attrHandle;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
/*		diagWindow (" getPTAttr: not checked in", KCP_NOT_CHECKED_IN);*/
		attrHandle = (KcmHandle) 0;
	}
	else {
		PTTableP = GetTableP(PTTable);
		attrHandle = PTTableP->attrBase;
		unlockBuffer((KcmHandle)PTTable);
	}
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	return (attrHandle);
}


/***********************************************************************/
/* set the attribute base for a given PT reference number */
void setPTAttr (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum, KcmHandle attrBase)
{
	PTTable_p* PTTable;
	PTTable_p  PTTableP;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
/*		diagWindow (" setPTAttr: not checked in", KCP_NOT_CHECKED_IN);*/
	}
	else {
		PTTableP = GetTableP(PTTable);
		PTTableP->attrBase = attrBase;
		unlockBuffer((KcmHandle)PTTable);
	}
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

}


/***********************************************************************/
/* get the PT data location for a given PT reference number */
KcmHandle getPTData (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum)
{
	PTTable_p* PTTable;
	PTTable_p  PTTableP;
	KcmHandle dataHandle;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
/*		diagWindow (" getPTData: not checked in", KCP_NOT_CHECKED_IN);*/
		dataHandle = (KcmHandle) 0;
	}
	else {
		PTTableP = GetTableP(PTTable);
		dataHandle = PTTableP->data;
		unlockBuffer((KcmHandle)PTTable);
	}
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	return (dataHandle);
}


/***********************************************************************/
/* get the PT data location for a given PT reference number */
PTErr_t resolvePTData (threadGlobals_p threadGlobalsP,
							PTRefNum_t PTRefNum,
							KpInt32_p SerialCount,
							PTRefNum_p SerialRefNum)
{
	PTTable_p*	PTTable;
	PTTable_p	PTTableP;
	KpUInt32_t	i;
	PTErr_t		PTErr;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
/*		diagWindow (" getPTData: not checked in", KCP_NOT_CHECKED_IN);*/
		PTErr = KCP_NOT_CHECKED_IN;
	}
	else {
		PTTableP = GetTableP(PTTable);
		if ((PTTableP->serialPTflag == IS_SERIAL_PT) && (PTTableP->serialCount > 0)) {
			*SerialCount = PTTableP->serialCount;
			for (i = 0; i < PTTableP->serialCount; i++) {
				SerialRefNum[i] = PTTableP->serialDef[i];
			}
		} else {
			*SerialCount = 1;
			SerialRefNum[0] = PTRefNum;
		}
		PTErr = KCP_SUCCESS;					/* must be a serial pt */
		unlockBuffer((KcmHandle)PTTable);
	}
	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	return (PTErr);
}


/* get the pointer to a PTTable */
static PTTable_p GetTableP (PTTable_p * ptt_h)
{
PTTable_p	p;
KcmHandle	aFutH;

	if (ptt_h == NULL) {
		return (NULL);
	}

	p = lockBuffer ((KcmHandle)ptt_h);

	aFutH = p->data;

	return (p);
}


/* get the starting handle of the linked PTTable list */
static PTTable_p*
	GetRootHandle (threadGlobals_p threadGlobalsP)
{
	return (threadGlobalsP->processGlobalsP->PTTableRootH);
}


/* set the starting handle of the linked PTTable list */
static void
	SetRootHandle (threadGlobals_p threadGlobalsP, PTTable_p* ptt_h)
{
	threadGlobalsP->processGlobalsP->PTTableRootH = ptt_h;
}


#if !defined (PTTest)
/***********************************************************************/
/* get the size in bytes of the header and attribute area */
int32 getCheckInSize (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum)
{
	int32 size;
	PTTable_p* PTTable;
	PTTable_p  PTTableP;

	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTTable = getPTTable (threadGlobalsP, PTRefNum);

	if (PTTable == NULL) {
/*		diagWindow (" getCheckInSize: not checked in", KCP_NOT_CHECKED_IN);*/
		size = 0;
	}
	else {
		PTTableP = GetTableP(PTTable);

		size = ((int32)sizeof (PT_t) + getAttrSize (PTTableP->attrBase));
		unlockBuffer((KcmHandle)PTTable);
	}

	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	return (size);
}
#endif


#if defined (PTTest)
/**********************************/
/* Test program for PT allocation */
/**********************************/
	#include <stdio.h>

void TryPrintingPTErr (char * func, PTErr_t err)
{
	printf ("%s: %d\n", func, err);
	if (err != KCP_SUCCESS)
		exit (1);
}

int main (argc, argv)
int argc;
char *argv [];
{
	PTRefNum_t	PTRefNum [5];

	TryPrintingPTErr ("initPTTable", initPTTable ());

	TryPrintingPTErr ("registerPT",
			registerPT ((KcmHandle) 1, (KcmHandle) 2, &PTRefNum [0]));
	TryPrintingPTErr ("registerPT",
			registerPT ((KcmHandle) 2, (KcmHandle) 2, &PTRefNum [1]));
	TryPrintingPTErr ("registerPT",
			registerPT ((KcmHandle) 3, (KcmHandle) 2, &PTRefNum [2]));
	TryPrintingPTErr ("registerPT",
			registerPT ((KcmHandle) 4, (KcmHandle) 2, &PTRefNum [3]));

	deletePTTable (PTRefNum [1]);
	deletePTTable (PTRefNum [3]);
	deletePTTable (PTRefNum [0]);
	deletePTTable (PTRefNum [2]);

    return (0);
}
#endif

