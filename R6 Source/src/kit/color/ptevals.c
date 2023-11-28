/*
 * @(#)ptevals.c	2.13 97/12/22

	Contains:	PT evaluator table stuff

	Written by:	Drivin' Team

	Copyright:	(c) 1991-1995 by Eastman Kodak Company, all rights reserved.

	Change History (most recent first):
		 <3>	 3/24/94	gbp		remove ptrs.h
		 <2>	12/ 9/93	HTR		Add ptr validation to PTEvaluators
		 <1>	11/13/91	blh		first checked in
 */


#include "kcmptlib.h"
#include "kcptmgrd.h"
#include "kcptmgr.h"

#define NO_ENTRY 0					/* evaluator table entry not used */
#define FIRST_ONE 1


/* prototypes */
static PTErr_t GetIndex ARGS((PTEvalTypes_t evaltype, int32_p index, evalList_p evalList));
static PTErr_t AddOne ARGS((PTEvalTypes_t evaltype, evalList_p evalList));
static PTErr_t GetEvalStatus ARGS((PTEvalTypes_t evaltype, evalList_p evalList));


	/* return number and type of all available evaluators */
PTErr_t
	PTEvaluators	(int32_p nEval, evalList_t theList[])
{
threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */
int32 			i1;
PTErr_t 		errnum = KCP_SUCCESS;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		errnum = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut2;
	}

/* Check for valid ptrs */
	if (Kp_IsBadWritePtr(nEval, sizeof(*nEval))) {
		errnum = KCP_BAD_PTR;
		goto ErrOut1;
	}
	if (Kp_IsBadWritePtr(theList, EVAL_LIST_MAX * sizeof(theList[0]))) {
		errnum = KCP_BAD_PTR;
		goto ErrOut1;
	}

	*nEval = 0;
	for (i1 = 0; i1 < EVAL_LIST_MAX; i1++) {
		if (theList != NULL) {
			theList[i1] = threadGlobalsP->processGlobalsP->iGP->evalList[i1];
		}
		
		if (threadGlobalsP->processGlobalsP->iGP->evalList[i1].evalID != NO_ENTRY) {
			(*nEval)++;
		}
	}	

ErrOut1:
	KCMDunloadGlobals();					/* Unlock this apps Globals */
ErrOut2:
	return (errnum);
}


/* initialize the evaluators list to contain only the software evaluator */
PTErr_t
	initList	(evalList_p evalList)
{
int32 i1;
	
	for (i1 = 0; i1 < EVAL_LIST_MAX; i1++) {
		evalList[i1].evalID = (PTEvalTypes_t) NO_ENTRY;
		evalList[i1].number = NO_ENTRY;
	}
	
	return KCP_SUCCESS;
}


/* see if the eval type is present */
static PTErr_t
	GetEvalStatus	(PTEvalTypes_t evaltype, evalList_p evalList)
{
int32	ind;
PTErr_t		errnum;

	errnum = GetIndex(evaltype, &ind, evalList);
	
	return errnum;		/* KCP_SUCCESS if found, else KCP_INVAL_EVAL */ 
}


/* add a new type of evaluator to the list */
PTErr_t
	AddEvaluator	(PTEvalTypes_t evaltype, evalList_p evalList)
{
int32	i1;
PTErr_t	errnum;

	errnum = AddOne(evaltype, evalList);	/* if the type is there, just increment the number */
	if (errnum != KCP_SUCCESS) {	/* no such evaluator type yet exists */
		for (i1 = 0; i1 < EVAL_LIST_MAX; i1++) {
			if (evalList[i1].evalID == NO_ENTRY) {
				evalList[i1].evalID = evaltype;				/* so create one */
				evalList[i1].number = FIRST_ONE;
				return KCP_SUCCESS;
			}
		}
		return KCP_INVAL_EVAL;		/* table was already full */ 
	}

	return KCP_SUCCESS;
}


/* add another evaluator of a given type */
static PTErr_t
	AddOne	(PTEvalTypes_t evaltype, evalList_p evalList)
{
PTErr_t		errnum;
int32	ind;

	errnum = GetIndex (evaltype, &ind, evalList);
	if (errnum == KCP_SUCCESS) {
		evalList[ind].number++;
		return KCP_SUCCESS;
	}
	else 
		return KCP_INVAL_EVAL;		/* couldn't find that type in table */ 
}


/* get the position in list of an evaluator of a given type */
PTErr_t
	GetIndex	(PTEvalTypes_t evaltype, int32_p index, evalList_p evalList)
{
int32 i1;

	for (i1 = 0; i1 < EVAL_LIST_MAX; i1++) {
		if (evalList[i1].evalID == evaltype) {
			*index = i1;
			return KCP_SUCCESS;
		}
	}

	return KCP_INVAL_EVAL;		/* couldn't find evaluator in table */ 
}


/* return evaluator to use given desired type */
PTErr_t
	GetEval	(PTEvalTypes_t reqEval, PTEvalTypes_t *useEval, evalList_p evalList)
{
PTErr_t	errnum;
	
	*useEval = reqEval;						/* get requested evaluator */

	if (*useEval == KCP_EVAL_DEFAULT) {		/* if default, use best available */
		*useEval = KCP_EVAL_CTE;
	}
	
	errnum = GetEvalStatus (*useEval, evalList);

	if ((errnum != KCP_SUCCESS) && (*useEval == KCP_EVAL_CTE)) {	/* if no NFE, switch to SW */
		*useEval = KCP_EVAL_SW;
	}

	return KCP_SUCCESS;
}

