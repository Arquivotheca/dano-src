/* @doc INTERNAL */
/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
*
* @module fxcbkman.c - FX8010 Callback Manager API |
*
* @comm This code is based on the software architecture
* description in internal document "FX8010 Software 
* Architecture Outline and Notes" Rev ???.
*
*******************************************************************
*/
#include <stdio.h>
#include "fxconfig.h"
#include "fxpgmman.h"
#include "fxcbkman.h"
#include "fxparman.h"

static void fxCallbackDSPInterruptTrap(ULONG);

#define COUNTMASK 0x0fffffff

typedef struct _pgmlist;

/* @struct FXCALLBACK |
 * This structure holds pertinent information needed to instantiate
 * callbacks on system events to FX applets.
 *
 * @field CALLID	| callID		| Contains opaque callback instance FXID.
 * @field FXEVENT	| ulEvent		| Contains event to trigger on.
 * @field ULONG		| ulFuncHandle	| Contains OS handle to callback function.
 * @field ULONG		| ulParam		| Contains a user-supplied parameter.
 * @field ULONG		| ulResetCounter| Contains count reset value (timer only).
 * @field ULONG		| ulCounter		| Contains a running count (timer only).
 * @field FXCALLBACK *| pNextPgm		| Contains head of PGMLIST chain.
 * @field PGMLIST *	| pPgmList		| Contains PGMLIST parent.
 * @field FXCALLBACK *| pChain		| Contains head of event list chain.
 * @field FXCALLBACK *| pCallChain	| Contains head of callback function list chain.
 * @field FXCALLBACK *| pQ			| Contains head of Event Queue chain.
 */
typedef struct _callbackstruct {
	CALLID     callID;					/* opaque callback instance */
	FXEVENT    ulEvent;					/* Event to trigger on */
	ULONG      ulFuncHandle;			/* OS handle to callback function */
	ULONG      ulParam;					/* User-supplied parameter */
	ULONG	   ulResetCounter;			/* Count reset value (timer only) */
	ULONG      ulCounter;				/* running count (timer only) */
	struct _callbackstruct *pNextPgm;	/* PGMLIST chain */
	struct _pgmlist        *pPgmList;	/* PGMLIST parent */
	struct _callbackstruct *pChain;		/* event list chain */
	struct _callbackstruct *pCallChain;	/* callback function list chain */
	struct _callbackstruct *pQ;			/* Event Queue chain */
} FXCALLBACK;

/* @struct CHIPLIST |
 * This structure holds information needed to dispatch DSP interrupts. 
 *
 * @field ULONG		| ulChipHandle	| Contains OS supplied chip handle.
 * @field FXCALLBACK *| cbkList		| Contains head of event list.
 * @field CHIPLIST *| pChain		| Contains chain pointer.
 */
typedef struct _chiplist {
	BOOL		inuse;
	ULONG		ulChipHandle;			/* OS supplied chip handle */
	FXCALLBACK  *cbkList;				/* head of event list */
	struct _chiplist *pChain;			/* chain pointer */
} CHIPLIST;

/* @struct PGMLIST |
 *
 * This structure holds information needed to efficiently maintain
 * callbacks related to FX programs.
 *
 * @field FXID		| pgmID		| Contains program FXID.
 * @field FXCALLBACK *| cbkList	| Contains head of program callback list.
 * @field PGMLIST * | pChain	| Contains chain pointer.
 */
typedef struct _pgmlist {
	FXID         pgmID;					/* program FXID */
	FXCALLBACK   *cbkList;				/* head of pgm callback list */
	struct _pgmlist *pChain;			/* chain pointer */
} PGMLIST;


#if !FX_DYNAMIC
static FXCALLBACK CallbackArray[FXMAX_CALLBACKS*FXMAX_CHIPS];
static PGMLIST  PgmListArray[FXMAX_PROGRAMS*FXMAX_CHIPS];
#endif

static int chipindex=0;
static CHIPLIST chiplist[FXMAX_CHIPS];
static FXCALLBACK *pTimerEventQ = NULL;
static FXCALLBACK *pDSPEventQ = NULL;
static FXCALLBACK *pShutdownEventQ = NULL;
static FXCALLBACK *pFreeCallbackList = NULL;
static PGMLIST  *pFreePgmList = NULL;
static PGMLIST  *pHeadPgmList = NULL;
static CHIPLIST *pFXevent_dspint = NULL;
static FXCALLBACK *pHeadCallList = NULL;
static FXCALLBACK *pFXevent_timer = NULL;

extern ULONG fx8210Mutex;

/* no mutex necessary, will only be set once */
static int numerator;
static int denominator;




/*****************************************************************
* 
* Function:		fxCallbackInitialize()
*	
* See description in FXCBKMAN.H.
*				
******************************************************************
*/
void
fxCallbackInitialize()
{
	int i;

	OS_TRAPTIMER( fxCallbackSampleTimer, &numerator, &denominator );

#if !FX_DYNAMIC
#if FXPSEUDO_DYNAMIC
	pFreeCallbackList = NULL;
	pFreePgmList = NULL;
#else
	/* Init FXCALLBACK array */
	for( i=0; i<FXMAX_CALLBACKS-1; i++ ) {
		CallbackArray[i].pChain = &(CallbackArray[i+1]);
	}
	CallbackArray[i].pChain = NULL;
	pFreeCallbackList = &(CallbackArray[0]);

	/* Init PGMLIST array */
	for( i=0; i<FXMAX_PROGRAMS-1; i++ ) {
		PgmListArray[i].pChain = &(PgmListArray[i+1]);
	}
	PgmListArray[i].pChain = NULL;
	pFreePgmList = &(PgmListArray[0]);
#endif
#endif
}

/*****************************************************************
* 
* Function:		fxCallbackInitChip( ulChipHandle )
*	
* See description in FXCBKMAN.H.
*				
******************************************************************
*/
FXSTATUS
fxCallbackInitChip( ULONG ulChipHandle )
{
	int i;

	OS_WAITMUTEX(fx8210Mutex);
	for(i=0;i<FXMAX_CHIPS;i++) {
		if( !(chiplist[i].inuse) ) break;
	}

	OS_TRAPDSPINTERRUPT(ulChipHandle, fxCallbackDSPInterruptTrap);

	chiplist[i].pChain = pFXevent_dspint;
	pFXevent_dspint = &(chiplist[i]);

	chiplist[i].inuse = TRUE;
	chiplist[i].ulChipHandle = ulChipHandle;
	chiplist[i].cbkList = NULL;
	chiplist[i].pChain = NULL;

#if !FX_DYNAMIC && FXPSEUDO_DYNAMIC
    {
    FXCALLBACK  *callback;
    PGMLIST     *pgmlist;
	extern int fxnextChipNo;
	extern int fxAllocedChips;

	if( fxnextChipNo > fxAllocedChips ) {
		callback = (FXCALLBACK *)  OS_MALLOC( sizeof(FXCALLBACK) * FXMAX_CALLBACKS );
		if( !callback ) { 
			OS_RELEASEMUTEX(fx8210Mutex);
			return FXERROR_OUT_OF_MEMORY;
		}
		for( i=0; i<FXMAX_CALLBACKS-1; i++ ) {
			callback[i].pChain = &(callback[i+1]);
		}
		callback[i].pChain = pFreeCallbackList;
		pFreeCallbackList = &(callback[0]);
		
		pgmlist = (PGMLIST *) OS_MALLOC( sizeof(PGMLIST) * FXMAX_PROGRAMS );
		if( !pgmlist ) {
			OS_RELEASEMUTEX(fx8210Mutex);
			return FXERROR_OUT_OF_MEMORY;
		}
		for( i=0; i<FXMAX_PROGRAMS-1; i++ ) {
			pgmlist[i].pChain = &(pgmlist[i+1]);
		}
		pgmlist[i].pChain = pFreePgmList;
		pFreePgmList = &(pgmlist[0]);
		}
	}
#endif

	OS_RELEASEMUTEX(fx8210Mutex);
	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxCallbackFreeChip( ulChipHandle )
*	
* See description in FXCBKMAN.H.
*				
******************************************************************
*/
FXSTATUS
fxCallbackFreeChip( ULONG ulChipHandle )
{
	CHIPLIST *p, *l;

	OS_WAITMUTEX(fx8210Mutex);

	/* Remove from chiplist */
	l = NULL;
	for( p=pFXevent_dspint;p && p->ulChipHandle != ulChipHandle; 
		 p=p->pChain ) l=p;
	if( l ) l->pChain = p->pChain;
	else pFXevent_dspint = p->pChain;

	/* Free CHIPLIST struct */
	p->inuse = FALSE;

	OS_RELEASEMUTEX(fx8210Mutex);
	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxCallbackRegisterCallback( pcallid, pgmID, ulEvent,
*                                           fHandler, ulParam );
*	
* See description in FXCBKMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxCallbackRegisterCallback( CALLID *pcallid, FXPGMID pgmID, FXEVENT ulEvent, 
						    void (*fHandler)(CALLID,FXEVENT,ULONG,ULONG), ULONG ulParam )
{
	FXCALLBACK *pNewCallback;
	PGMLIST  *pPgmList;
	ULONG	  ulcHandle;
	CHIPLIST *pDSP;

	OS_WAITMUTEX(fx8210Mutex);

	/* Validate pgmID */
	if( !fxPgmValidPgmID((FXID)pgmID) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	/* Allocate a callback structure */

#if FX_DYNAMIC
    pFreeCallbackList = OS_MALLOC( sizeof(FXCALLBACK) );
#endif

	if( !pFreeCallbackList ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_OUT_OF_MEMORY;
	}
	pNewCallback = pFreeCallbackList;

#if !FX_DYNAMIC
	pFreeCallbackList = pFreeCallbackList->pChain;
#endif

	/* Fill it in */
	pNewCallback->callID = (CALLID)pNewCallback;
	pNewCallback->ulEvent = (ulEvent&(~COUNTMASK)*denominator)/numerator;
	pNewCallback->ulFuncHandle = OS_FUNC2HANDLE(fHandler);
	pNewCallback->ulParam = ulParam;
	pNewCallback->ulResetCounter = (ulEvent&COUNTMASK)?ulEvent&(~COUNTMASK):1;
	pNewCallback->ulCounter = pNewCallback->ulResetCounter;

	/* Attach to head of appropriate pgm list */
	for( pPgmList = pHeadPgmList; 
	     pPgmList && pPgmList->pgmID != (FXID)pgmID;
		 pPgmList = pPgmList->pChain ) ;

	if( !pPgmList ) {
		/* Allocate a pgmlist structure */

#if FX_DYNAMIC
		pFreePgmList = OS_MALLOC( sizeof(PGMLIST) );
#endif

		if( !pFreePgmList ) {
#if FX_DYNAMIC
			OS_FREE(pNewCallback);
#else
			pNewCallback->pChain = pFreeCallbackList;
			pFreeCallbackList = pNewCallback;
#endif
			OS_RELEASEMUTEX(fx8210Mutex);
			return FXERROR_OUT_OF_MEMORY;
		}
		pPgmList = pFreePgmList;

#if !FX_DYNAMIC
		pFreePgmList = pFreePgmList->pChain;
#endif

		pPgmList->pgmID = (FXID)pgmID;
		pPgmList->cbkList = NULL;
		pPgmList->pChain = pHeadPgmList;
		pHeadPgmList = pPgmList;
	}
	pNewCallback->pPgmList = pPgmList;
	pNewCallback->pNextPgm = pPgmList->cbkList;
	pPgmList->cbkList = pNewCallback;

	/* Now attach to the appropriate event list */
	switch( ulEvent&(~COUNTMASK) ) {

	case FXEVENT_DSPINTERRUPT:
		/* Search for the chip to which this program is attached */
		ulcHandle = fxPgmGetChipHandle((FXID)pgmID);

		for( pDSP = pFXevent_dspint; pDSP->ulChipHandle != ulcHandle;
		     pDSP = pDSP->pChain );
		pNewCallback->pChain = pDSP->cbkList;
		pDSP->cbkList = pNewCallback;

		break;

	case FXEVENT_SAMPLETIMER:
		/* Tack onto timer list */

		pNewCallback->pChain = pFXevent_timer;
		pFXevent_timer = pNewCallback;

		break;

	case FXEVENT_SHUTDOWN:
		/* Nothing to do.  Shutdown is searched through pgmlist */
		break;
	}

	/* Finally, attach it to the call list */

	pNewCallback->pCallChain = pHeadCallList;
	pHeadCallList = pNewCallback;

	*pcallid = (CALLID)pNewCallback;

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;	
}

/*****************************************************************
* 
* Function:		fxCallbackUnregisterCallback( callID )
*	
* See description in FXCBKMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxCallbackUnregisterCallback( CALLID callID )
{
	FXCALLBACK *pCbk, *pLast, *pTemp;
	PGMLIST  *pPgm, *pLastPgm;
	CHIPLIST *pChip;
	FXID pgmID;
	ULONG ulcHandle;
	
	OS_WAITMUTEX(fx8210Mutex);
	
	/* Find the struct in the call list */
	pLast = NULL;
	for( pCbk = pHeadCallList; pCbk && pCbk->callID != callID; 
	     pLast = pCbk, pCbk = pCbk->pCallChain );
	if( !pCbk ) {
		OS_RELEASEMUTEX(fx8210Mutex);
 	    return FXERROR_NO_ERROR;   /* not in list, exit */
	}

	/* Pull it out of the call list */
	if( pLast ) pLast->pCallChain = pCbk->pCallChain;
	else pHeadCallList = pCbk->pCallChain;

	/* Now, find it in the pgm list */
	pgmID = (pCbk->pPgmList)->pgmID;
	pLast = NULL;
	for( pCbk = (pCbk->pPgmList)->cbkList; pCbk->callID != callID; 
	     pLast = pCbk, pCbk=pCbk->pNextPgm );
	if( pLast ) pLast->pNextPgm = pCbk->pNextPgm;
	else (pCbk->pPgmList)->cbkList = pCbk->pNextPgm;

	/* If pgm list is now empty, deallocate pgmlist struct */
	if( (pCbk->pPgmList)->cbkList == NULL ) {

		pLastPgm = NULL;
		for( pPgm = pHeadPgmList; pPgm != pCbk->pPgmList;
		     pLastPgm = pPgm, pPgm = pPgm->pChain ) ;

		if( pLastPgm ) pLastPgm = pPgm->pChain;
		else pHeadPgmList = pPgm->pChain;

#if FX_DYNAMIC
		OS_FREE(pPgm);
#else
		pPgm->pChain = pFreePgmList;
		pFreePgmList = pPgm;
#endif
	}

	/* Now, remove it from appropriate event list */
	pTemp = pCbk;
	switch( pTemp->ulEvent ) {

	case FXEVENT_DSPINTERRUPT:
		/* remove from chip list */
		ulcHandle = fxPgmGetChipHandle(pgmID);
		for( pChip = pFXevent_dspint; pChip->ulChipHandle != ulcHandle;
		     pChip = pChip->pChain ) ;
		pLast = NULL;
		for( pCbk = pChip->cbkList; pCbk->callID != callID; 
		     pLast = pCbk, pCbk=pCbk->pChain );
		if( pLast ) pLast->pChain = pCbk->pChain;
		else pChip->cbkList = pCbk->pChain;
		break;

	case FXEVENT_SAMPLETIMER:
		/* remove from timer list */
		pLast = NULL;
		for( pCbk = pFXevent_timer; pCbk->callID != callID; 
		     pLast = pCbk, pCbk=pCbk->pChain );
		if( pLast ) pLast->pChain = pCbk->pChain;
		else pFXevent_timer = pCbk->pChain;
		break;

	case FXEVENT_SHUTDOWN:
		/* do nothing, no specific list for shutdown */
		break;
	}

	/* Finally, remove it from any event queues */
	pLast = NULL;
	for( pCbk = pTimerEventQ; pCbk && pCbk->callID != callID; 
	     pCbk=pCbk->pQ ) ;
	if( pCbk ) {
		if(pLast) pLast->pQ = pCbk->pQ;
		else pTimerEventQ = pCbk->pQ;
	}

	pLast = NULL;
	for( pCbk = pDSPEventQ; pCbk && pCbk->callID != callID; 
	     pCbk=pCbk->pQ ) ;
	if( pCbk ) {
		if(pLast) pLast->pQ = pCbk->pQ;
		else pDSPEventQ = pCbk->pQ;
	}

	pLast = NULL;
	for( pCbk = pShutdownEventQ; pCbk && pCbk->callID != callID; 
	     pCbk=pCbk->pQ ) ;
	if( pCbk ) {
		if(pLast) pLast->pQ = pCbk->pQ;
		else pShutdownEventQ = pCbk->pQ;
	}


	/* deallocate callback struct */
#if FX_DYNAMIC
	OS_FREE(pCbk);
#else
	pTemp->pChain = pFreeCallbackList;
	pFreeCallbackList = pTemp;
#endif

	OS_RELEASEMUTEX(fx8210Mutex);
	
	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* @func static void | fxCallbackUnregisterAllCallbacks |
*	
* This function unregisters all callback functions
* for the given program
*
* @parm FXID | pgmID	| Specifies program identifier.
*
* @comm This is a static function		
*				
******************************************************************
*/
static void
fxCallbackUnregisterAllCallbacks( FXPGMID pgmID )
{
	PGMLIST *pPgm, *pLastPgm;
	CHIPLIST *pChip;
	FXCALLBACK *pCbk, *pLast;
	ULONG	 ulcHandle;

	OS_WAITMUTEX(fx8210Mutex);
	
	/* find pgmlist struct */
	pLastPgm = NULL;
	for( pPgm = pHeadPgmList; 
	     pPgm && pPgm->pgmID != (FXID)pgmID; 
		 pLastPgm = pPgm, pPgm = pPgm->pChain ) ;

	if( !pPgm ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return;				/* no callbacks for pgmID */
	}

	/* unchain all from dsp interrupt event list */
	ulcHandle = fxPgmGetChipHandle((FXID)pgmID);
	for( pChip = pFXevent_dspint; pChip->ulChipHandle != ulcHandle;
	     pChip = pChip->pChain ) ;
	pLast = NULL;
	pCbk = pChip->cbkList;
	while( pCbk ) {
		while( pCbk && pCbk->pPgmList == pPgm ) pCbk = pCbk->pChain;
		if( pLast ) pLast->pChain = pCbk;
		else pChip->cbkList = pCbk;
		pLast = pCbk;
		if( pCbk ) pCbk = pCbk->pChain;
	}

	/* unchain all from timer list */
	pLast = NULL;
	pCbk = pFXevent_timer;
	while( pCbk ) {
		while( pCbk && pCbk->pPgmList == pPgm ) pCbk = pCbk->pChain;
		if( pLast ) pLast->pChain = pCbk;
		else pFXevent_timer = pCbk;
		pLast = pCbk;
		if( pCbk ) pCbk = pCbk->pChain;
	}

	/* unchain all from callback list */
	pLast = NULL;
	pCbk = pHeadCallList;
	while( pCbk ) {
		while( pCbk && pCbk->pPgmList == pPgm ) pCbk = pCbk->pCallChain;
		if( pLast ) pLast->pChain = pCbk;
		else pHeadCallList = pCbk;
		pLast = pCbk;
		if( pCbk ) pCbk = pCbk->pChain;
	}

	/* deallocate all callback structs in pPgm chain */
	for( pCbk=pPgm->cbkList; pCbk; pCbk=pLast ) {

		pLast = pCbk->pNextPgm;
#if FX_DYNAMIC
		OS_FREE(pCbk);
#else
		pCbk->pChain = pFreeCallbackList;
		pFreeCallbackList = pCbk;
#endif
	}

	/* deallocate pgmlist struct */
	if( pLastPgm ) pLastPgm = pPgm->pChain;
	else pHeadPgmList = pPgm->pChain;

#if FX_DYNAMIC
	OS_FREE(pPgm);
#else
	pPgm->pChain = pFreePgmList;
	pFreePgmList = pPgm;
#endif

	OS_RELEASEMUTEX(fx8210Mutex);
}

/*****************************************************************
* 
* @func static void | fxCallbackInterruptTrap |
*	
* This function is called by the OS when an interrupt
* is received from a chip.  It searches for which program
* triggered the interrupt, and then services any
* FXEVENT_DSPINTERRUPT callbacks which are registered 
* to that program.
*
* @parm ULONG | ulChipHandle |	Specifies which chip triggered it.
*
* @comm This is a static function.
*
******************************************************************
*/
static void 
fxCallbackDSPInterruptTrap( ULONG ulChipHandle )
{
	CHIPLIST *pChip;
	FXCALLBACK *pCbk;
	FXID        pgmID;
	ULONG     ulIntVector;
	ADDR      addrIntVector;
	ULONG	  ulFunc, ulParam0, ulParam2, ulParam3;

	OS_WAITMUTEX(fx8210Mutex);

	/* Find chip that interrupted us */
	for( pChip = pFXevent_dspint; pChip->ulChipHandle != ulChipHandle;
	     pChip = pChip->pChain );

	/* Queue all callbacks whose program triggered the interrupt */
	for( pCbk = pChip->cbkList; pCbk; pCbk = pCbk->pChain ) {

		pgmID = (pCbk->pPgmList)->pgmID;
		addrIntVector = fxPgmGetIntVector(pgmID);
		ulIntVector = fxParamReadGPR(pgmID, addrIntVector);
		if( ulIntVector ) {
			fxParamWriteGPR( pgmID, addrIntVector, 0L );
			pCbk->ulCounter = ulIntVector;
			pCbk->pQ = pDSPEventQ;
			pDSPEventQ = pCbk;
		}
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	/* Pull callbacks from beginning of queue until there are no more */
	while( pDSPEventQ ) {

		OS_WAITMUTEX(fx8210Mutex);
		pCbk = pDSPEventQ;
		pDSPEventQ = (pCbk) ? pCbk->pQ : NULL;

		ulFunc = pCbk->ulFuncHandle;
		ulParam0 = pCbk->callID;
		ulParam2 = pCbk->ulParam;
		ulParam3 = pCbk->ulCounter;
		
		OS_RELEASEMUTEX(fx8210Mutex);

		if( !pCbk ) break;

		OS_CALLBACK( ulFunc, ulParam0, (ULONG)FXEVENT_DSPINTERRUPT,
			         ulParam2, ulParam3 );
	}

}

/*****************************************************************
* 
* @func static void | fxCallbackSampleTimer |
*	
* This function is called by the OS on a periodic basis.
* If the requested time has passed, any callbacks 
* registered to this event will be instantiated.
*
* @comm This is a static function.
*				
******************************************************************
*/
static void
fxCallbackSampleTimer()
{
	FXCALLBACK *pCbk;
	ULONG ulFunc, ulParam0, ulParam2, ulParam3;

	OS_WAITMUTEX(fx8210Mutex);

	/* walk through pFXevent_timer list and decrement all ulCounters.
	 * if zero, reset timer and queue callback
	 */
	
	for( pCbk = pFXevent_timer; pCbk; pCbk = pCbk->pChain ) {

		if( !(--(pCbk->ulCounter)) ) {

			pCbk->ulCounter = pCbk->ulResetCounter;
			pCbk->pQ = pTimerEventQ;
			pTimerEventQ = pCbk;
		}
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	/* Pull callbacks from beginning of queue until there are no more */
	while( pTimerEventQ ) {

		OS_WAITMUTEX(fx8210Mutex);
		pCbk = pTimerEventQ;
		if( pCbk ) pTimerEventQ = pCbk->pQ;
		else {
			ulFunc = pCbk->ulFuncHandle;
			ulParam0 = pCbk->callID;
			ulParam2 = pCbk->ulParam;
			ulParam3 = 0L;
		}
		OS_RELEASEMUTEX(fx8210Mutex);

		if( !pCbk ) break;

		OS_CALLBACK( ulFunc, ulParam0, (ULONG)FXEVENT_SAMPLETIMER,
			         ulParam2, ulParam3 );
	}
			
}

/*****************************************************************
* 
* Function:		fxCallbackShutdown( pgmID )
*	
* See description in FXCBKMAN.H.
*				
******************************************************************
*/
FXSTATUS
fxCallbackShutdown( FXID pgmID )
{
	PGMLIST *pPgm;
	FXCALLBACK *pCbk;
	ULONG ulFunc, ulParam0, ulParam2, ulParam3;

	OS_WAITMUTEX(fx8210Mutex);

	/* search through pgmlist for pgmID */
	for( pPgm = pHeadPgmList; 
	     pPgm && pPgm->pgmID != pgmID; 
		 pPgm = pPgm->pChain ) ;

	if( !pPgm ) return FXERROR_NO_ERROR;  /* no callbacks for pgmID */

	/* Queue any shutdown functions for this pgm */
	for( pCbk = pPgm->cbkList; pCbk; pCbk = pCbk->pNextPgm ) {

		if( pCbk->ulEvent == FXEVENT_SHUTDOWN ) {

			pCbk->pQ = pShutdownEventQ;
			pShutdownEventQ = pCbk;
		}
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	/* Pull callbacks from beginning of queue until there are no more */
	while( pShutdownEventQ ) {

		OS_WAITMUTEX(fx8210Mutex);
		pCbk = pShutdownEventQ;
		if( pCbk ) pShutdownEventQ = pCbk->pQ;
		else {
			ulFunc = pCbk->ulFuncHandle;
			ulParam0 = pCbk->callID;
			ulParam2 = pCbk->ulParam;
			ulParam3 = 0L;
		}
		OS_RELEASEMUTEX(fx8210Mutex);

		if( !pCbk ) break;

		OS_CALLBACK( ulFunc, ulParam0, (ULONG)FXEVENT_SHUTDOWN,
			         ulParam2, ulParam3 );
	}

	fxCallbackUnregisterAllCallbacks(pgmID);

	return FXERROR_NO_ERROR;
}
