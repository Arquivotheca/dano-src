/* @doc INTERNAL */
/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
*
* @module fxpatman.c - FX8010 Patch Manager API |
*
* This code is based on the software architecture
* description in internal document "FX8010 Software 
* Architecture Outline and Notes" Rev ???.
*
*******************************************************************
*/

#include <stdio.h>
#include "fxconfig.h"
#include "fxparman.h"
#include "fxpatman.h"
#include "fxpgmman.h"

typedef struct _patchstruct;

typedef struct _patchstub {
	ADDR		logicalChannel;
	struct _patchstruct *pPatchStruct;
	ADDR		gprAddr;
	INSTRFIELD	instrFirst;
	struct _patchstub *pConnectList;
	struct _patchstub *pPrevConnect;
	struct _patchstub *pNextConnect;
	struct _patchstub *pChain;
} PATCHSTUB;

typedef struct _patchstruct {
	FXID			pgmID;
	ADDR		nInputs;
	ADDR		nOutputs;
	PATCHSTUB	*pInputStubs;
	union {
		PATCHSTUB	*pOutputStubs;
		struct _patchstruct *pChain;
	} u;
} PATCHSTRUCT;

#if !FX_DYNAMIC && !FXPSEUDO_DYNAMIC
static PATCHSTRUCT	 PatchStructs[FXMAX_CHIPS*(FXMAX_PROGRAMS+1)];
static PATCHSTUB	 PatchStubs[FXMAX_CHIPS*(FXMAX_PROGRAMS*FXMAX_STUBS+32)];
#endif

static PATCHSTRUCT	*pFreePatchStructs=NULL;
static PATCHSTUB	*pFreePatchStubs=NULL;

#define CHAN_OWNER(pC) ((FXPGMID)(((pC)->pPatchStruct)->pgmID))
#define PORT(pgm)	   (fxPgmIsPortID(pgm))

#define PS2ID(p)	(FXID)(p)
#define ID2PS(id)	(PATCHSTRUCT *)(id)

static PATCHSTUB * findChannel( PATCHSTUB *, USHORT );

extern ULONG fx8210Mutex;

/*****************************************************************
*
* Function: fxPatchInitialize
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS 
fxPatchInitialize()
{
#if !FX_DYNAMIC
#if FXPSEUDO_DYNAMIC
	pFreePatchStructs = NULL;
	pFreePatchStubs = NULL;
#else
    USHORT i;

	/* Init PatchStructs array */
	for( i=0; i<FXMAX_CHIPS*(FXMAX_PROGRAMS+1)-1; i++ ) {
		PatchStructs[i].u.pChain = &(PatchStructs[i+1]);
	}
	PatchStructs[i].u.pChain = NULL;
	pFreePatchStructs = &(PatchStructs[0]);

	/* Init PatchStubs array */
	for( i=0; i<FXMAX_CHIPS*(FXMAX_PROGRAMS*FXMAX_STUBS+32)-1; i++ ) {
		PatchStubs[i].pChain = &(PatchStubs[i+1]);
	}
	PatchStubs[i].pChain = NULL;
	pFreePatchStubs = &(PatchStubs[0]);
#endif
#endif

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchInitChip
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS 
fxPatchInitChip( )
{
#if !FX_DYNAMIC && FXPSEUDO_DYNAMIC
	{
	PATCHSTRUCT structblock[];
	PATCHSTUB	stubblock[];
	extern int fxnextChipNo;
	extern int fxAllocedChips;

	OS_WAITMUTEX(fx8210Mutex);

	if( fxnextChipNo > fxAllocedChips ) {
		structblock = (PATCHSTRUCT *) 
			OS_MALLOC( sizeof( PATCHSTRUCT ) * (FXMAX_PROGRAMS + 1) );
		if( !structblock ) {
			OS_RELEASEMUTEX(fx8210Mutex);
			return FXERROR_OUT_OF_MEMORY;
		}
		for( i=0; i<FXMAX_PGMS; i++ ) {
			structblock[i].u.pChain = &(structblock[i+1]);
		}
		structblock[i].u.pChain = pFreePatchStructs;
		pFreePatchStructs = &(structblock[0]);
		
		stubblock = (PATCHSTUB *) 
			OS_MALLOC( sizeof(PATCHSTUB) * (FXMAX_PROGRAMS*FXMAX_STUBS+32) );
		if( !stubblock ) {
			OS_RELEASEMUTEX(fx8210Mutex);
			return FXERROR_OUT_OF_MEMORY;
		}
		for( i=0; i<FXMAX_PROGRAMS*FXMAX_STUBS+31; i++ ) {
			stubblock[i].pChain = &(stubblock[i+1]);
		}
		stubblock[i].pChain = pFreePatchStubs;
		pFreePatchStubs = &(stubblock[0]);
	}
	OS_RELEASEMUTEX(fx8210Mutex);
#endif

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchInitPgm
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS 
fxPatchInitPgm( FXID pgmID, USHORT nInputs, USHORT nOutputs, FXID *patchID )
{
    USHORT j;
	PATCHSTRUCT *pPatchStruct;
	PATCHSTUB   *pPatchStub, *pChain, *pTemp;

#if FX_DYNAMIC
	pFreePatchStructs = OS_MALLOC( sizeof(PATCHSTRUCT) );
#endif

	if( !pFreePatchStructs ) return FXERROR_OUT_OF_MEMORY;
	pPatchStruct = pFreePatchStructs;

#if !FX_DYNAMIC
	pFreePatchStructs = pFreePatchStructs->u.pChain;
#endif

	pPatchStruct->pgmID    = pgmID;
	pPatchStruct->nInputs  = nInputs;
	pPatchStruct->nOutputs = nOutputs;
	pPatchStruct->u.pChain   = NULL;

	/* Allocate input stubs */
	pChain = NULL; pPatchStub = NULL;
	for( j=nInputs; j>0; j-- ) {
#if FX_DYNAMIC
		pFreePatchStubs = OS_MALLOC( sizeof( PATCHSTUB ) );
#endif
		if( !pFreePatchStubs ) {
			for( ; pChain; pChain = pTemp ) {
				pTemp = pChain->pChain;
#if FX_DYNAMIC
				OS_FREE( pChain );
#else
				pChain->pChain = pFreePatchStubs;
				pFreePatchStubs = pChain;
#endif
			}
#if FX_DYNAMIC
			OS_FREE( pPatchStruct );
#else
			pPatchStruct->u.pChain = pFreePatchStructs;
			pFreePatchStructs = pPatchStruct;
#endif
			return FXERROR_OUT_OF_MEMORY;
		}

		pPatchStub = pFreePatchStubs;

#if !FX_DYNAMIC
		pFreePatchStubs = pFreePatchStubs->pChain;
#endif
		pPatchStub->logicalChannel = j-1;
		pPatchStub->pPatchStruct = pPatchStruct;
		pPatchStub->instrFirst = 0; /* to be filled in later */
		pPatchStub->pConnectList = NULL;
		pPatchStub->pNextConnect = NULL;
		pPatchStub->pPrevConnect = NULL;
		pPatchStub->pChain = pChain;
		pChain = pPatchStub;
	}
	pPatchStruct->pInputStubs = pPatchStub;

	/* Allocate output stubs */
	pChain = NULL; pPatchStub = NULL;
	for( j=nOutputs; j>0; j-- ) {
#if FX_DYNAMIC
		pFreePatchStubs = OS_MALLOC( sizeof( PATCHSTUB ) );
#endif
		if( !pFreePatchStubs ) {
			for( ; pChain; pChain = pTemp ) {
				pTemp = pChain->pChain;
#if FX_DYNAMIC
				OS_FREE( pChain );
#else
				pChain->pChain = pFreePatchStubs;
				pFreePatchStubs = pChain;
#endif
			}
			for( pChain = pPatchStruct->pInputStubs; pChain; pChain=pTemp){
				pTemp = pChain->pChain;
#if FX_DYNAMIC
				OS_FREE( pChain );
#else
				pChain->pChain = pFreePatchStubs;
				pFreePatchStubs = pChain;
#endif
			}

#if FX_DYNAMIC
			OS_FREE( pPatchStruct );
#else
			pPatchStruct->u.pChain = pFreePatchStructs;
			pFreePatchStructs = pPatchStruct;
#endif
			return FXERROR_OUT_OF_MEMORY;
		}

		pPatchStub = pFreePatchStubs;

#if !FX_DYNAMIC
		pFreePatchStubs = pFreePatchStubs->pChain;
#endif
		pPatchStub->logicalChannel = j-1;
		pPatchStub->pPatchStruct = pPatchStruct;
		pPatchStub->gprAddr = 0;	/* to be filled in later */
		pPatchStub->pConnectList = NULL;
		pPatchStub->pNextConnect = NULL;
		pPatchStub->pPrevConnect = NULL;
		pPatchStub->pChain = pChain;
		pChain = pPatchStub;
	}
	pPatchStruct->u.pOutputStubs = pPatchStub;

	*patchID = PS2ID(pPatchStruct);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchInitOutputStub
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS
fxPatchInitOutputStub( FXID patchID, USHORT logicalChannel, ADDR gprAddr, INSTRFIELD instrFirst )
{
	PATCHSTRUCT *patch;
	PATCHSTUB *stub;

	patch = ID2PS(patchID);
	stub = findChannel( patch->u.pOutputStubs, logicalChannel );

	stub->gprAddr = gprAddr;
	stub->instrFirst = instrFirst;

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchInitInputStub
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS
fxPatchInitInputStub( FXID patchID, USHORT logicalChannel, ADDR gprAddr, INSTRFIELD instrFirst )
{
	PATCHSTRUCT *patch;
	PATCHSTUB *stub;

	patch = ID2PS(patchID);
	stub = findChannel( patch->pInputStubs, logicalChannel );

	stub->instrFirst = instrFirst;
	stub->gprAddr = gprAddr;

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchFreePgm
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS 
fxPatchFreePgm( FXID patchID )
{
	PATCHSTRUCT *pPatchStruct;
	PATCHSTUB *pChain, *pTemp;

	pPatchStruct = ID2PS(patchID);

	for( pChain = pPatchStruct->u.pOutputStubs; pChain; pChain = pTemp ) {
		pTemp = pChain->pChain;
#if FX_DYNAMIC
		OS_FREE( pChain );
#else
		pChain->pChain = pFreePatchStubs;
		pFreePatchStubs = pChain;
#endif
	}
	for( pChain = pPatchStruct->pInputStubs; pChain; pChain=pTemp){
		pTemp = pChain->pChain;
#if FX_DYNAMIC
		OS_FREE( pChain );
#else
		pChain->pChain = pFreePatchStubs;
		pFreePatchStubs = pChain;
#endif
	}

#if FX_DYNAMIC
	OS_FREE( pPatchStruct );
#else
	pPatchStruct->u.pChain = pFreePatchStructs;
	pFreePatchStructs = pPatchStruct;
#endif
	
	return FXERROR_NO_ERROR;

}


/*****************************************************************
* 
* Function: fxPatchChannel
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchChannel( FXPGMID pgmA, USHORT nOutChan, FXPGMID pgmB, USHORT nInpChan )
{
	PATCHSTRUCT *patchA, *patchB;
	PATCHSTUB	*pA, *pB;

	OS_WAITMUTEX(fx8210Mutex);

	patchA = ID2PS( fxPgmGetPatchID((FXID)pgmA) );
	patchB = ID2PS( fxPgmGetPatchID((FXID)pgmB) );
	if( !patchA || !patchB ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	if( fxPgmWhichChip((FXID)pgmA) != fxPgmWhichChip((FXID)pgmB) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_PROGRAMS_ON_DIFFERENT_CHIPS;
	}

	if( nOutChan >= patchA->nOutputs || nInpChan >= patchB->nInputs ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_CHANNEL;
	}

	if( PORT((FXID)pgmA) && PORT((FXID)pgmB) ) {
		OS_RELEASEMUTEX(fx8210utex);
		return FXERROR_PHYSICAL_PORTS;
	}

	/* If pgmB is a port, disconnect the program currently feeding it */
	if( PORT((FXID)pgmB) ) {
		fxPatchGroundInput( pgmB, nInpChan );
	}

	/* Remove from list of channel currently feeding pgmB's 
	 * input
	 */
	pB = findChannel( patchB->pInputStubs, nInpChan );

	if( pB->pPrevConnect ) {
		(pB->pPrevConnect)->pNextConnect = pB->pNextConnect;
	} else if( pB->pConnectList ) {
		(pB->pConnectList)->pConnectList = pB->pNextConnect;
	}
	if( pB->pNextConnect ) {
		(pB->pNextConnect)->pPrevConnect = pB->pPrevConnect;
	}
	
	/* Patch pgmA's output channel to pgmB's input */
	pA = findChannel( patchA->u.pOutputStubs, nOutChan );
	pB->pPrevConnect = NULL;
    if( pA->pConnectList ) pA->pConnectList->pPrevConnect = pB;
	pB->pNextConnect = pA->pConnectList;
	pB->pConnectList = pA;
	pA->pConnectList = pB;

	/* Write pgmA's output GPR into pgmB's instruction */
	if( !PORT(pgmB) ) {
		fxParamWriteInstructionField( pgmB, pB->instrFirst, pA->gprAddr );
		if( !PORT((FXID)pgmA) )
			fxParamWriteInstructionField( pgmA, pA->instrFirst, pA->gprAddr );
	} else {
		if( !PORT((FXID)pgmA) ) {
			fxParamWriteInstructionField( pgmA, pA->instrFirst, pB->gprAddr );
		}
	}
	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchGroundOutput
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchGroundOutput( FXPGMID pgmID, USHORT nOutChan )
{
	PATCHSTRUCT *patch;
	PATCHSTUB *pOut, *pTemp, *pTemp2;

	OS_WAITMUTEX(fx8210Mutex);

	patch = ID2PS( fxPgmGetPatchID((FXID)pgmID) );
	if( !patch ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}
	if( nOutChan >= patch->nOutputs ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_CHANNEL;
	}

	/* Unpatch and ground all input channels connected to this output 
	 * channel
	 */
	pOut = findChannel( patch->u.pOutputStubs, nOutChan );
	for( pTemp = pOut->pConnectList; pTemp; pTemp=pTemp2 ) {

		pTemp2 = pTemp->pNextConnect;
		if( !PORT(CHAN_OWNER(pTemp)) ) {
			fxParamWriteInstructionField( CHAN_OWNER(pTemp), 
										  pTemp->instrFirst,
										  ZERO_CONSTANT );
		} else {
			fxParamWriteInstructionField( pgmID, pOut->instrFirst,
										  pOut->gprAddr );
		}
		pTemp->pPrevConnect = NULL;
		pTemp->pNextConnect = NULL;
		pTemp->pConnectList = NULL;
	}
	pOut->pConnectList = NULL;

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchGroundInput
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchGroundInput( FXPGMID pgmID, USHORT nInpChan )
{
	PATCHSTRUCT *patch;
	PATCHSTUB *pInp;

	OS_WAITMUTEX(fx8210Mutex);

	patch = ID2PS( fxPgmGetPatchID((FXID)pgmID) );
	if( !patch ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}
	if( nInpChan >= patch->nInputs ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_CHANNEL;
	}

	/* Remove from list of channel currently feeding pgmB's 
	 * input
	 */
	pInp = findChannel( patch->pInputStubs, nInpChan );
	if( pInp->pConnectList ) {

		if( PORT((FXID)pgmID) ) {
			fxParamWriteInstructionField( 
				CHAN_OWNER(pInp->pConnectList),
				(pInp->pConnectList)->instrFirst,
				(pInp->pConnectList)->gprAddr );
		} else {
			fxParamWriteInstructionField( pgmID, pInp->instrFirst, 
										  ZERO_CONSTANT );
		}

		if( pInp->pPrevConnect ) {
			(pInp->pPrevConnect)->pNextConnect = pInp->pNextConnect;
		} else {
			(pInp->pConnectList)->pConnectList = pInp->pNextConnect;
		}

		if( pInp->pNextConnect ) {
			(pInp->pNextConnect)->pPrevConnect = pInp->pPrevConnect;
		}
	
		pInp->pConnectList = NULL;
        pInp->pPrevConnect = NULL;
        pInp->pNextConnect = NULL;
	}
	OS_RELEASEMUTEX(fx8210Mutex);
	
	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchUnpatchPgm
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchUnpatchPgm( FXPGMID pgmID )
{
	ADDR i;
	PATCHSTRUCT *patch;

	OS_WAITMUTEX(fx8210Mutex);

	patch = ID2PS( fxPgmGetPatchID((FXID)pgmID) );
	if( !patch ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	for( i=0; i<patch->nInputs; i++ ) {
		fxPatchGroundInput( pgmID, i );
	}
	for( i=0; i<patch->nOutputs; i++ ) {
		fxPatchGroundOutput( pgmID, i );
	}
	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchInsertBefore
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchInsertBefore( FXPGMID pgmA, FXPGMID pgmB )
{
	PATCHSTRUCT *patchA, *patchB;
	PATCHSTUB	*pA, *pX;
    USHORT i,j;

	OS_WAITMUTEX(fx8210Mutex);

	patchA = ID2PS( fxPgmGetPatchID((FXID)pgmA) );
	patchB = ID2PS( fxPgmGetPatchID((FXID)pgmB) );
	if( !patchA || !patchB ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	if( fxPgmWhichChip((FXID)pgmA) != fxPgmWhichChip((FXID)pgmB) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_PROGRAMS_ON_DIFFERENT_CHIPS;
	}

	/* Completely unpatch program B */
	fxPatchUnpatchPgm( pgmB );

	/* Algorithm:
	 * For all i < I(A)
	 *    X(n) = A(i)->pConnectList
	 *    if i < O(B) then
	 *        fxPatchChannel(B(i), A(i))
	 *    endif
	 *    For all j < I(B) such that (j mod I(A)) == i
	 *        fxPatchChannel(X(n), B(j))
	 *    endfor
	 * endfor
	 */
	for( i=0; i< patchA->nInputs; i++ ) {

		pA = findChannel( patchA->pInputStubs, i );
		pX = pA->pConnectList;

		if( i < patchB->nOutputs ) {
			fxPatchChannel( pgmB, i, pgmA, i );
		}

		if( !pX ) continue;

		for( j=i; j < patchB->nInputs; j += patchA->nInputs ) {
			fxPatchChannel( CHAN_OWNER(pX), pX->logicalChannel,
							pgmB, j );
		}
	}
	OS_RELEASEMUTEX(fx8210Mutex);
							
	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchInsertAfter
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchInsertAfter( FXPGMID pgmA, FXPGMID pgmB )
{
	PATCHSTRUCT *patchA, *patchB;
	PATCHSTUB	*pA;
    USHORT i,j;

	OS_WAITMUTEX(fx8210Mutex);

	patchA = ID2PS( fxPgmGetPatchID((FXID)pgmA) );
	patchB = ID2PS( fxPgmGetPatchID((FXID)pgmB) );
	if( !patchA || !patchB ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	if( fxPgmWhichChip((FXID)pgmA) != fxPgmWhichChip((FXID)pgmB) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_PROGRAMS_ON_DIFFERENT_CHIPS;
	}

	/* Completely unpatch program B */
	fxPatchUnpatchPgm( pgmB );

	/* Algorithm:
	 * For all i < O(A)
	 *    if i < O(B)
	 *       For all X(n) connected to A(i)
	 *          fxPatchChannel( B(i), X(n) )
	 *       endfor
	 *    endif
	 *    For all j < I(B) such that (j mod O(A)) == i
	 *       fxPatchChannel(A(i), B(j))
	 *    endfor
	 * endfor
	 */
	for( i = 0; i < patchA->nInputs; i++ ) {

		if( i < patchB->nOutputs ) {
			pA = findChannel( patchA->u.pOutputStubs, i );
			if( pA->pConnectList ) {

				while( pA->pConnectList ) {
					fxPatchChannel( pgmB, i,
									CHAN_OWNER(pA->pConnectList),
									(pA->pConnectList)->logicalChannel );
				}
			}
		}
		for( j = i; j < patchB->nInputs; j += patchA->nOutputs ) {
			fxPatchChannel( pgmA, i, pgmB, j );
		}
	}
	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchAddAfter
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchAddAfter( FXPGMID pgmA, FXPGMID pgmB )
{
	PATCHSTRUCT *patchA, *patchB;
    USHORT i,j;

	OS_WAITMUTEX(fx8210Mutex);

	patchA = ID2PS( fxPgmGetPatchID((FXID)pgmA) );
	patchB = ID2PS( fxPgmGetPatchID((FXID)pgmB) );
	if( !patchA || !patchB ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	if( fxPgmWhichChip((FXID)pgmA) != fxPgmWhichChip((FXID)pgmB) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_PROGRAMS_ON_DIFFERENT_CHIPS;
	}

	/* Unpatch all inputs of program B */
	for( i=0; i<patchB->nInputs; i++ ) {
		fxPatchGroundInput( pgmB, i );
	}

	/* Algorithm:
     * For all i < O(A)
	 *    if i < I(B)
	 *       For all j < I(B) such that (j mod O(A)) == i
	 *		    fxPatchChannel( A(i), B(j) )
	 *       endfor
	 *    endif
	 * endfor
	 */
	for( i=0; i<patchA->nOutputs; i++ ) {
		if( i<patchB->nInputs ) {
			for( j = i; j < patchB->nInputs; j += patchA->nOutputs ) {
				fxPatchChannel( pgmA, i, pgmB, j );
			}
		}
	}
	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchBypass
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchBypass( FXPGMID pgmID )
{
	PATCHSTRUCT *patch;
	PATCHSTUB *pIn, *pOut, *pX;
    USHORT i,j;

	OS_WAITMUTEX(fx8210Mutex);

	patch = ID2PS( fxPgmGetPatchID((FXID)pgmID) );
	if( !patch ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	/* Algorithm:
	 * For all i < I(A)
	 *    For all j < O(A) such that (j mod I(A)) == i
	 *       For all X(n) connected to A(j)
	 *           fxPatchChannel(chan connected to A(i),X(n))
	 *       endfor
	 *    endfor
	 *    fxGroundInput(A(i))
	 * endfor
	 */
	for( i = 0; i < patch->nInputs; i++ ) {

		pIn = findChannel( patch->pInputStubs, i );
		pX = pIn->pConnectList;
		if( !pX ) {
			if( i < patch->nOutputs ) {
				fxPatchGroundOutput( pgmID, i );
			}
			continue;
		}

		for( j = i; j < patch->nOutputs; j += patch->nInputs ) {

			pOut = findChannel( patch->u.pOutputStubs, j );
			while( pOut->pConnectList ) {
				fxPatchChannel( CHAN_OWNER(pX), pX->logicalChannel,
					            CHAN_OWNER(pOut->pConnectList),
								(pOut->pConnectList)->logicalChannel );
			}
		}
		fxPatchGroundInput( pgmID, i );
	}
	
	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function: fxPatchReplace
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchReplace( FXPGMID pgmA, FXPGMID pgmB )
{
	PATCHSTRUCT *patchA, *patchB;
	PATCHSTUB	*pA, *pX;
    USHORT i,j;

	OS_WAITMUTEX(fx8210Mutex);

	patchA = ID2PS( fxPgmGetPatchID((FXID)pgmA) );
	patchB = ID2PS( fxPgmGetPatchID((FXID)pgmB) );
	if( !patchA || !patchB ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	if( fxPgmWhichChip((FXID)pgmA) != fxPgmWhichChip((FXID)pgmB) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_PROGRAMS_ON_DIFFERENT_CHIPS;
	}

	/* Completely unpatch program B */
	fxPatchUnpatchPgm( pgmB );

	/* Algorithm:
	 * For all i < I(A)
	 *    X(n) = A(i)->pConnectList
	 *    For all j < I(B) such that (j mod I(A)) == i
	 *       fxPatchChannel(X(n),B(j))
	 *    endfor
	 *    fxPatchGroundInput(A(i))
	 * endfor
	 * For all i < O(A)
	 *    For all X(n) connected to A(i)
	 *        fxPatchChannel(B(i mod O(B)),X(n))
	 *    endfor
	 * endfor
	 */
	for( i=0; i<patchA->nInputs; i++ ) {

		pA = findChannel( patchA->pInputStubs, i );
		pX = pA->pConnectList;
		if( !pX ) continue;

		for( j=i; j<patchB->nInputs; j += patchA->nInputs ) {
			fxPatchChannel( CHAN_OWNER(pX), pX->logicalChannel,
							pgmB, j );
		}
		fxPatchGroundInput( pgmA, i );
	}
	for( i=0; i<patchA->nOutputs; i++ ) {

		pA = findChannel( patchA->u.pOutputStubs, i );
		while( pA->pConnectList ) {
			fxPatchChannel( pgmB, (USHORT)(i % patchB->nOutputs),
							CHAN_OWNER(pA->pConnectList),
							(pA->pConnectList)->logicalChannel );
		}
	}
	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;

}


/*****************************************************************
* 
* Function: fxPatchQueryOutput
*
* See definition in FXPATMAN.H.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchQueryOutput( FXPGMID pgmID, USHORT nOutChan, USHORT *nChannels, 
				    FXPGMID pgmIn[], DWORD pgmInSize, USHORT nInpChan[], DWORD nInpChanSize )
{
	PATCHSTRUCT *patch;
	PATCHSTUB *pOut, *pIn;
    USHORT j = 0;

	OS_WAITMUTEX(fx8210Mutex);

	patch = ID2PS( fxPgmGetPatchID((FXID)pgmID) );
	if( !patch ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	if( nOutChan > patch->nOutputs ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	pOut = findChannel( patch->u.pOutputStubs, nOutChan );
	pIn = pOut->pConnectList;
	while( pIn ) {
		pgmIn[j]    = CHAN_OWNER(pIn);
		nInpChan[j] = pIn->logicalChannel;
		j++;
	}

	pgmIn[j] = UNALLOCED_ID;
	nInpChan[j] = 0;
	*nChannels = j;

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;

}

/*****************************************************************
* 
* Function: fxPatchQueryInput
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchQueryInput( FXPGMID pgmID, USHORT nInpChan, FXPGMID *pgmOut, USHORT *nOutChan )
{
	PATCHSTRUCT *patch;
	PATCHSTUB *pIn;

	OS_WAITMUTEX(fx8210Mutex);

	patch = ID2PS( fxPgmGetPatchID((FXID)pgmID) );
	if( !patch ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	if( nInpChan > patch->nInputs ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	pIn = findChannel( patch->pInputStubs, nInpChan );
	if( pIn->pConnectList ) {
		*pgmOut   = CHAN_OWNER(pIn->pConnectList);
		*nOutChan = (pIn->pConnectList)->logicalChannel;
	} else {
		*pgmOut   = UNALLOCED_ID;
		*nOutChan = 0;
	}
	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;

}

/*****************************************************************
* 
* Function: fxPatchGetNumIO
*
* See definition in FXPATMAN.H.
*				
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPatchGetNumIO( FXPGMID pgmID, USHORT *nInpChan, USHORT *nOutChan )
{
	PATCHSTRUCT *patch;

	OS_WAITMUTEX(fx8210Mutex);

	patch = ID2PS( fxPgmGetPatchID((FXID)pgmID) );
	if( !patch ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	*nInpChan = patch->nInputs;
	*nOutChan = patch->nOutputs;

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* @func static PATCHSTUB * | findChannel |
*
* This function finds the logical channel patch stub in the given
* list.
*
* @parm PATCHSTUB *	| pPatchList	| Specifies list to search.
* @parm USHORT         | nChan         | Specifies logical channel to search for.
*
* @rdesc This function returns a pointer to the <t PATCHSTUB> structure
* for the specified channel, or NULL if DNE.
*				
******************************************************************
*/
static PATCHSTUB *
findChannel( PATCHSTUB *pPatchList, USHORT nChan )
{

	for( ; pPatchList && pPatchList->logicalChannel != nChan; 
	       pPatchList = pPatchList->pChain );
	return pPatchList;
}
