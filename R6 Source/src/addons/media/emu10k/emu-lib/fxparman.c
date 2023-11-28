/* @doc INTERNAL */
/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
*
* @module fxresman.c - FX8010 Resource Manager API |
*
* This code is based on the software architecture
* description in internal document "FX8010 Software 
* Architecture Outline and Notes" Rev ???.
*
*******************************************************************
*/

#include <stdio.h>
#include <string.h>
#include "fxconfig.h"
#include "fxparman.h"
#include "fxpgmman.h"
#include "fxresman.h"

/* @struct FXINTERPOLATOR |
 * This persistent structure holds information about the state of a
 * single interpolator.
 * @field BOOL				| inUse		| States whether ramper is active or not.
 * @field ADDR				| gpr		| Contains physical address of GPR to ramp.
 * @field ULONG				| ulDest	| Contains destination value of <p gpr>.
 * @field ADDR				| instrAddr	| Contains physical address of INTERP instruction.
 * @field ADDR				| addrGPRx	| Contains physical GPR address of time-constant.
 * @field ADDR				| addrGPRy	| Contains physical GPR address of destination variable.
 * @field FXID				| pgmID		| Contains program FXID.
 * @field FXINTERPOLATOR *	| pChain	| Contains chain pointer.
 */
typedef struct _interpolator {
	BOOL	inUse;	
	ADDR	gpr;
	ULONG	ulDest;
	ADDR	instrAddr;
#if VARIABLE_RAMPER
	ADDR	addrGPRx;
#endif
	ADDR	addrGPRy;
	FXID	pgmID;
	struct _interpolator *pChain;
} FXINTERPOLATOR;

#define ID2INT(id) (FXINTERPOLATOR *)(id)
#define INT2ID(p)  (FXID)(p)

#if !FX_DYNAMIC && !FXPSEUDO_DYNAMIC
static FXINTERPOLATOR	interp[FXMAX_CHIPS*FXMAX_INTERPOLATORS];
#endif

static FXINTERPOLATOR *pFreeInterpolators;

static VOID fxParamPackInstruction( USHORT, USHORT, USHORT, USHORT, 
								    USHORT, ULONG *, ULONG * );

extern ULONG fx8210Mutex;

/******************************************************************
*
* Function:	fxParamInitialize
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS 
fxParamInitialize()
{
	int i;

#if !FX_DYNAMIC
#if FXPSEUDO_DYNAMIC
	pFreeInterpolators = NULL;
#else
	/* Init interp array */
	for( i=0; i<FXMAX_CHIPS*FXMAX_INTERPOLATORS; i++ ) {
		interp[i].pChain = &(interp[i+1]);
	}
	interp[i].pChain = NULL;
	pFreeInterpolators = &(interp[0]);

#endif
#endif

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamInitChip
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS
fxParamInitChip()
{
#if !FX_DYNAMIC && FXPSEUDO_DYNAMIC
	int i;
	extern int fxnextChipNo;
	extern int fxAllocedChips;

	{
	FXINTERPOLATOR newblock[];

	OS_WAITMUTEX(fx8210Mutex);
	if( fxnextChipNo > fxAllocedChips ) {
		newblock = (FXINTERPOLATOR *) 
			OS_MALLOC( sizeof( FXINTERPOLATOR ) * FXMAX_INTERPOLATORS );
		if( !newblock ) {
			OS_RELEASEMUTEX(fx8210Mutex);
			return FXERROR_OUT_OF_MEMORY;
		}
		for( i=0; i<FXMAX_INTERPOLATORS-1; i++ ) {
			newblock[i].pChain = &(newblock[i+1]);
		}
		newblock[i].pChain = pFreeInterpolators;
		pFreeInterpolators = &(newblock[0]);
		OS_RELEASEMUTEX(fx8210Mutex);
	}
#endif

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamAllocInterpolators
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS 
fxParamAllocInterpolators( FXID *interpolatorID, int nInterpolators, ADDR gprAddr,
						  ADDR instrAddr )
{
	FXINTERPOLATOR *pPtr, *pInterp, *pTemp;

#if FX_DYNAMIC
	int i;
#endif
	
	if( !nInterpolators ) return FXERROR_NO_ERROR;

	/*Count out n interpolators */
#if !FX_DYNAMIC
	for( pPtr = pFreeInterpolators; pPtr && nInterpolators--; 
		pPtr = pPtr->pChain ) {
		pPtr->inUse     = FALSE;
		pPtr->instrAddr = instrAddr++;
#if VARIABLE_RAMPER
		pPtr->addrGPRx  = gprAddr++;
#endif
		pPtr->addrGPRy  = gprAddr++;
		pTemp = pPtr;
	}

	if( !pPtr ) return FXERROR_OUT_OF_MEMORY;

	pInterp = pFreeInterpolators;
	pFreeInterpolators = pPtr; /*was ->pChain */
	pTemp->pChain = NULL;
#else
	{
    FXINTERPOLATOR *newblock;
	
	newblock = (FXINTERPOLATOR *) 
		OS_MALLOC( sizeof( FXINTERPOLATOR ) * nInterpolators );
	if( !newblock ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return 	FXERROR_OUT_OF_MEMORY;
	}
	for( i=0; i<nInterpolators; i++ ) {
		newblock[i].inUse     = FALSE;
		newblock[i].instrAddr = instrAddr++;
#if VARIABLE_RAMPER
		newblock[i].addrGPRx  = gprAddr++;
#endif
		newblock[i].addrGPRy  = gprAddr++;
		newblock[i].pChain    = &(newblock[i+1]);
	}
	newblock[i-1].pChain = NULL;
	pInterp = &(newblock[0]);
    }
#endif

	*interpolatorID = INT2ID(pInterp);

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamFreeInterpolators
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS
fxParamFreeInterpolators( ULONG ulChipHandle, FXID interpolatorID )
{
	FXINTERPOLATOR *pPtr, *pFirst;
	ULONG ulUCL, ulUCH;

	pPtr = ID2INT(interpolatorID);
	if( !pPtr ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_NO_ERROR;
	}
	pFirst = pPtr;

	/* Insert a NOP instruction */
	fxParamPackInstruction( FXINSTR_SKIP,ZERO_CONSTANT,
		                    ZERO_CONSTANT, ZERO_CONSTANT,
							ZERO_CONSTANT, &ulUCL, &ulUCH );
	while( pPtr ) {

		OS_WRITEINSTRUCTION(ulChipHandle,pPtr->instrAddr,
							ulUCL, ulUCH);
		if( !(pPtr->pChain ) ) break;
		else pPtr = pPtr->pChain;
	}

#if !FX_DYNAMIC
	pPtr->pChain = pFreeInterpolators;
	pFreeInterpolators = pFirst;
#else
	OS_FREE( pFirst );
#endif

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamReadGPR
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
ULONG EMUAPIEXPORT
fxParamReadGPR( FXPGMID pgmID, OPERAND gprAddress )
{
	register ADDR addr;
	ULONG ulChipHandle;
	ULONG ulValue;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return 0L;

	OS_WAITMUTEX(fx8210Mutex);
	
	ulChipHandle = fxPgmGetChipHandle((FXID)pgmID);

	addr = (gprAddress&(~VIRTMASK)) ?
		fxPgmMapVirtualToPhysicalGPR((FXID)pgmID, (ADDR)(gprAddress&VIRTMASK)):
		(ADDR)gprAddress;

	ulValue = OS_READGPR( ulChipHandle, addr );

	if( addr > LAST_GENERALGPR ) ulValue <<= TRAMADJSHIFT;

	OS_RELEASEMUTEX(fx8210Mutex);

	return ulValue;

}

/******************************************************************
*
* Function:	fxParamReadGPRArray
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamReadGPRArray( FXPGMID pgmID, OPERAND baseGPR, ULONG aulArray[], DWORD nGPRs )
{
	register ADDR addr;
	DWORD i;
	ULONG ulChipHandle;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);
	
	ulChipHandle = fxPgmGetChipHandle( (FXID)pgmID );

	for( i=0; i<nGPRs ; i++, baseGPR++ ) {
		addr = (baseGPR&(~VIRTMASK)) ?
			fxPgmMapVirtualToPhysicalGPR((FXID)pgmID, (ADDR)(baseGPR&VIRTMASK)):
			(ADDR)baseGPR;
		aulArray[i] = OS_READGPR( ulChipHandle, addr );

		if( addr > LAST_GENERALGPR ) aulArray[i] <<= TRAMADJSHIFT;
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
		
}

/******************************************************************
*
* Function:	fxParamWriteGPRArray
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamWriteGPRArray( FXPGMID pgmID, OPERAND baseGPR, ULONG aulArray[], DWORD nGPRs )
{
	register ADDR addr;
	DWORD i;
	ULONG ulChipHandle;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	ulChipHandle = fxPgmGetChipHandle( (FXID)pgmID );

	for( i=0; i<nGPRs ; i++, baseGPR++ ) {
		addr = (baseGPR&(~VIRTMASK)) ?
			fxPgmMapVirtualToPhysicalGPR((FXID)pgmID, (ADDR)(baseGPR&VIRTMASK)):
			(ADDR)baseGPR;
		
		if( addr > LAST_GENERALGPR ) aulArray[i] >>= TRAMADJSHIFT;

		OS_WRITEGPR( ulChipHandle, addr, aulArray[i] );
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
		
}

/******************************************************************
*
* Function:	fxParamWriteGPR
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamWriteGPR( FXPGMID pgmID, OPERAND gprAddress, ULONG ulValue )
{
	register ADDR addr;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	addr = (gprAddress&(~VIRTMASK)) ?
		fxPgmMapVirtualToPhysicalGPR((FXID)pgmID, (ADDR)(gprAddress&VIRTMASK)):
		(ADDR)gprAddress;

	if( addr > LAST_GENERALGPR ) ulValue >>= TRAMADJSHIFT;

	OS_WRITEGPR( fxPgmGetChipHandle((FXID)pgmID), addr, ulValue );

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamReadTableArray
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamReadTableArray( FXPGMID pgmID, ADDR tabAddr, ULONG ulSize, ULONG* pData )
{
	PGMRSRCQUERY rsrcQuery;
	ULONG ulChipHandle;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);
	
	ulChipHandle = fxPgmGetChipHandle((FXID)pgmID);

	if( (tabAddr&(~VIRTMASK)) ) {
		fxRsrcQueryPgm( fxPgmGetRsrcID( (FXID)pgmID ), &rsrcQuery );
		tabAddr = (tabAddr&VIRTMASK) - rsrcQuery.tableAddr;
	}

	for( ; ulSize > 0; pData++, ulSize-- ) {
		*pData = OS_READTABLE( ulChipHandle, tabAddr++ );
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamWriteTableArray
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamWriteTableArray( FXPGMID pgmID, ADDR tabAddr, ULONG ulSize, ULONG* pData )
{
	PGMRSRCQUERY rsrcQuery;
	ULONG ulChipHandle;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);
	
	ulChipHandle = fxPgmGetChipHandle((FXID)pgmID);

	if( (tabAddr&(~VIRTMASK)) ) {
		fxRsrcQueryPgm( fxPgmGetRsrcID( (FXID)pgmID ), &rsrcQuery );
		tabAddr = (tabAddr&VIRTMASK) - rsrcQuery.tableAddr;
	}

	for( ; ulSize > 0; pData++, ulSize-- ) {
		OS_WRITETABLE( ulChipHandle, tabAddr++, *pData );
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamReadInstructionField
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
OPERAND EMUAPIEXPORT 
fxParamReadInstructionField( FXPGMID pgmID, INSTRFIELD instrAddr )
{
	ULONG ulUCL, ulUCH;
	OPERAND retop;
	register ADDR phys;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXADDRERROR;

	OS_WAITMUTEX(fx8210Mutex);

	if( (ADDR)(instrAddr&(~FIELDMASK)) == 0  && 
		!(fxPgmIsRunning(pgmID)) ) {
			USHORT opcode, opa, opx, opy, opres;
			
			fxPgmGetFirstInstr( (FXID)pgmID, &opcode, &opa, &opx, &opy, &opres );
			switch( instrAddr&FIELDMASK ) {
				case FX_OPCODE: 	
					 OS_RELEASEMUTEX(fx8210Mutex);
					 return (OPERAND)opcode;
				case FXOP_A:	phys = (ADDR)opa;	break;
				case FXOP_X:	phys = (ADDR)opx;	break;
				case FXOP_Y:	phys = (ADDR)opy;	break;
				case FXOP_RES:	phys = (ADDR)opres; break;
			}
	} else {

		OS_READINSTRUCTION( 
			fxPgmGetChipHandle((FXID)pgmID),
			fxPgmMapVirtualToPhysicalInstr((FXID)pgmID,(ADDR)(instrAddr&(~FIELDMASK))),
			&ulUCL, &ulUCH
		);

		switch( instrAddr&FIELDMASK ) {
		
		case FX_OPCODE:	OS_RELEASEMUTEX(fx8210Mutex); 
						return (OPERAND)((ulUCH>>20)&0xf);

		case FXOP_A:	phys = (ADDR)(ulUCH&0x3ff);
						break;

		case FXOP_X:	phys = (ADDR)((ulUCL>>10)&0x3ff);
						break;

		case FXOP_Y:	phys = (ADDR)(ulUCL&0x3ff);
						break;

		case FXOP_RES:	phys = (ADDR)((ulUCH>>10)&0x3ff);
						break;
		}
	}

	retop = fxPgmMapPhysicalToVirtualGPR((FXID)pgmID, phys);
	if( retop == FXADDRERROR ) retop = phys; else retop |= (~VIRTMASK);

	OS_RELEASEMUTEX(fx8210Mutex);
	return retop;

}

/******************************************************************
*
* Function:	fxParamReadInstruction
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT 
fxParamReadInstruction( FXPGMID pgmID, ADDR instrAddr, FXINSTR *instr )
{
	ULONG ulUCL, ulUCH;
	ADDR  opcode, opa, opx, opy, opres;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	if( instrAddr == 0  && !(fxPgmIsRunning(pgmID)) ) {
			
			fxPgmGetFirstInstr( (FXID)pgmID, &opcode, &opa, &opx, &opy, &opres );
	} else {

		OS_READINSTRUCTION( 
			fxPgmGetChipHandle((FXID)pgmID),
			fxPgmMapVirtualToPhysicalInstr((FXID)pgmID,instrAddr),
			&ulUCL, &ulUCH
		);

		opcode = (USHORT)((ulUCH>>20)&0xf);
		opa = (ADDR)(ulUCH&0x3ff);
		opx = (ADDR)((ulUCL>>10)&0x3ff);
		opy = (ADDR)(ulUCL&0x3ff);
		opres = (ADDR)((ulUCH>>10)&0x3ff);
	}

	instr->opcode= (BYTE) opcode;
	instr->opA   = fxPgmMapPhysicalToVirtualGPR((FXID)pgmID, opa);
	if( instr->opA == FXADDRERROR ) instr->opA = opa; else instr->opA |= (~VIRTMASK);
	instr->opX   = fxPgmMapPhysicalToVirtualGPR((FXID)pgmID, opx);
	if( instr->opX == FXADDRERROR ) instr->opX = opx; else instr->opX |= (~VIRTMASK);
	instr->opY   = fxPgmMapPhysicalToVirtualGPR((FXID)pgmID, opy);
	if( instr->opY == FXADDRERROR ) instr->opY = opy; else instr->opY |= (~VIRTMASK);
	instr->opRes = fxPgmMapPhysicalToVirtualGPR((FXID)pgmID, opres);
	if( instr->opRes == FXADDRERROR ) instr->opRes = opres; else instr->opRes |= (~VIRTMASK);

	OS_RELEASEMUTEX(fx8210Mutex);
	return FXERROR_NO_ERROR;

}


/******************************************************************
*
* Function:	fxParamWriteInstructionField
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamWriteInstructionField( FXPGMID pgmID, INSTRFIELD instrAddr, OPERAND wValue )
{
	ULONG ulChipHandle;
	ADDR  map;
	ULONG ulUCL, ulUCH;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	if( wValue&(~VIRTMASK) ) 
		wValue = fxPgmMapVirtualToPhysicalGPR((FXID)pgmID,(ADDR)(wValue&VIRTMASK));

	if( (ADDR)(instrAddr&(~FIELDMASK)) == 0 && 
		!(fxPgmIsRunning(pgmID)) ) {
		USHORT opcode, opa, opx, opy, opres;
		
		fxPgmGetFirstInstr( (FXID)pgmID, &opcode, &opa, &opx, &opy, &opres );
		switch( instrAddr&FIELDMASK ) {
			case FX_OPCODE: 
				fxPgmWriteFirstInstr((FXID)pgmID,wValue,opa,opx,opy,opres); 
				OS_RELEASEMUTEX(fx8210Mutex);
				return FXERROR_NO_ERROR;
			case FXOP_A:
				fxPgmWriteFirstInstr((FXID)pgmID,opcode,wValue,opx,opy,opres); 
				OS_RELEASEMUTEX(fx8210Mutex);
				return FXERROR_NO_ERROR;
			case FXOP_X:
				fxPgmWriteFirstInstr((FXID)pgmID,opcode,opa,wValue,opy,opres); 
				OS_RELEASEMUTEX(fx8210Mutex);
				return FXERROR_NO_ERROR;
			case FXOP_Y:
				fxPgmWriteFirstInstr((FXID)pgmID,opcode,opa,opx,wValue,opres); 
				OS_RELEASEMUTEX(fx8210Mutex);
				return FXERROR_NO_ERROR;
			case FXOP_RES:
				fxPgmWriteFirstInstr((FXID)pgmID,opcode,opa,opx,opy,wValue); 
				OS_RELEASEMUTEX(fx8210Mutex);
				return FXERROR_NO_ERROR;
		}
	}

	ulChipHandle = fxPgmGetChipHandle((FXID)pgmID);
	map = fxPgmMapVirtualToPhysicalInstr((FXID)pgmID,(ADDR)(instrAddr&(~FIELDMASK)));

	OS_READINSTRUCTION( ulChipHandle, map, &ulUCL, &ulUCH );

	switch( instrAddr&FIELDMASK ) {
	
	case FX_OPCODE:	ulUCH &= 0xff0fffffLU; 
					ulUCH |= ((ULONG)(wValue&0xf)<<20);
					break;

	case FXOP_A:	ulUCH &= 0xfffffc00LU;
					ulUCH |= ((ULONG)(wValue&0x3ff));
					break;

	case FXOP_X:	ulUCL &= 0xfff003ffLU;
					ulUCL |= ((ULONG)(wValue&0x3ff)<<10);
					break;

	case FXOP_Y:	ulUCL &= 0xfffffc00LU;
					ulUCL |= ((ULONG)(wValue&0x3ff));
					break;

	case FXOP_RES:	ulUCH &= 0xfff003ffLU;
					ulUCH |= ((ULONG)(wValue&0x3ff)<<10);
					break;
	}

	OS_WRITEINSTRUCTION( ulChipHandle, map, ulUCL, ulUCH );

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamWriteInstruction
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamWriteInstruction( FXPGMID pgmID, ADDR instrAddr, FXINSTR *instr )
{
	ULONG ulChipHandle;
	ADDR  map;
	ULONG ulUCL, ulUCH;
	ADDR  opcode, opa, opx, opy, opres;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	opcode = (ADDR) instr->opcode;
	opa = (instr->opA&(~VIRTMASK)) ? 
		fxPgmMapVirtualToPhysicalGPR((FXID)pgmID,(ADDR)((instr->opA)&VIRTMASK)) :
		instr->opA;
	opx = (instr->opX&(~VIRTMASK)) ? 
		fxPgmMapVirtualToPhysicalGPR((FXID)pgmID,(ADDR)((instr->opX)&VIRTMASK)) :
		instr->opX;
	opy = (instr->opY&(~VIRTMASK)) ? 
		fxPgmMapVirtualToPhysicalGPR((FXID)pgmID,(ADDR)((instr->opY)&VIRTMASK)) :
		instr->opY;
	opres = (instr->opRes&(~VIRTMASK)) ? 
		fxPgmMapVirtualToPhysicalGPR((FXID)pgmID,(ADDR)((instr->opRes)&VIRTMASK)) :
		instr->opRes;

	if( instrAddr == 0 && !(fxPgmIsRunning(pgmID)) ) {
		fxPgmWriteFirstInstr((FXID)pgmID,opcode,opa,opx,opy,opres); 
	} else {
		ulChipHandle = fxPgmGetChipHandle((FXID)pgmID);
		map = fxPgmMapVirtualToPhysicalInstr((FXID)pgmID,(ADDR)instrAddr);
		
		ulUCH = ulUCL = 0L;
		ulUCH |= ((ULONG)(opcode&0xf)<<20);
		ulUCH |= ((ULONG)(opa&0x3ff));
		ulUCH |= ((ULONG)(opres&0x3ff)<<10);
		ulUCL |= ((ULONG)(opx&0x3ff)<<10);
		ulUCL |= ((ULONG)(opy&0x3ff));
	
		OS_WRITEINSTRUCTION( ulChipHandle, map, ulUCL, ulUCH );
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamReadFlags
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
TRAMMODE EMUAPIEXPORT
fxParamReadFlags( FXPGMID pgmID, OPERAND tramBufAddr )
{
	FXID	rsrcID;
	BYTE bFlags;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return (TRAMMODE) 0;

	OS_WAITMUTEX(fx8210Mutex);

	rsrcID = fxPgmGetRsrcID( (FXID)pgmID );
	if( tramBufAddr&(~VIRTMASK) )
		tramBufAddr = 
			fxRsrcMapVirtualToPhysicalGPR(rsrcID,(ADDR)(tramBufAddr&VIRTMASK));

	bFlags = OS_READFLAGS( fxPgmGetChipHandle((FXID)pgmID), tramBufAddr );

	bFlags = fxRsrcMapPhysicalToEnumFlags( rsrcID, (BYTE)(bFlags>>4) );

	OS_RELEASEMUTEX(fx8210Mutex);

	return (TRAMMODE) bFlags;
	
}

/******************************************************************
*
* Function:	fxParamWriteFlags
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamWriteFlags( FXPGMID pgmID, OPERAND tramBufAddr, TRAMMODE tramMode )
{
	FXID	rsrcID;
	BYTE bFlags, fByte;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	rsrcID = fxPgmGetRsrcID( (FXID)pgmID );
	if( tramBufAddr&(~VIRTMASK) )
		tramBufAddr = 
			fxRsrcMapVirtualToPhysicalGPR(rsrcID,(ADDR)(tramBufAddr&VIRTMASK));

	fByte = OS_READFLAGS( fxPgmGetChipHandle((FXID)pgmID), tramBufAddr );
	bFlags = (fByte&1) | 
			 ((fxRsrcMapEnumToPhysicalFlags(rsrcID,(BYTE)(tramMode)))<<4);

	OS_WRITEFLAGS( fxPgmGetChipHandle((FXID)pgmID), tramBufAddr, bFlags );

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamReadTRAMAddress
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
ULONG EMUAPIEXPORT
fxParamReadTRAMAddress( FXPGMID pgmID, OPERAND tramAddr )
{
	register ULONG ulAddr;
	ULONG offs;
	
	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXADDRERROR;

	OS_WAITMUTEX(fx8210Mutex);

	if( !(fxRsrcIsTRAMBuf( fxPgmGetRsrcID((FXID)pgmID), &tramAddr, &offs) ) )
		return FXADDRERROR;

	ulAddr = OS_READGPR( fxPgmGetChipHandle((FXID)pgmID), tramAddr ) ;

	OS_RELEASEMUTEX(fx8210Mutex);
	
	return ((ADDR)(ulAddr&0xffff - offs));
}

/******************************************************************
*
* Function:	fxParamWriteTRAMAddress
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamWriteTRAMAddress( FXPGMID pgmID, OPERAND tramAddr, ULONG addrNew )
{
	register ULONG ulAddr;
	ULONG offs;
	int adjust; /* A3 hack */
	char *pszChipRev;
	ULONG ulRevReg;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	if( !(fxRsrcIsTRAMBuf( fxPgmGetRsrcID((FXID)pgmID), &tramAddr, &offs) ) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_NOT_TRAM_ADDRESS;
	}

	adjust = 0;
	pszChipRev = fxRsrcGetChipRevisionID( fxPgmWhichChip((FXID)pgmID), &ulRevReg );
	if( !memcmp( pszChipRev, "EMU8010", strlen( "EMU8010" ) ) &&
		ulRevReg <= 3 ) {
		/* Now, we must implement the A3 silicon hack for XTRAM */
		/*
		   Let b = buffer number (tramAddr-FXXTRAMADDRMIN)
		   Let f = requested offset (addrNew + offs)

		   j = | -18, if read, align=0
			   | -19, if read, align=1
			   |   0, if write, align=0
			   |   1, if write, align=1

		   n = -(b div 2) - 1 + f + j

		   a1 = -(n mod 16)
		   a2 = 16 - (n mod 16)

		   ap = { a1, if |a1| < a2
				  a2, otherwise

		   a  = ap - (n mod 2)

		   actual buffer = f+a, where -8 < a <= 8
		*/
		if( tramAddr >= FXXTRAMADDRMIN && tramAddr <= FXXTRAMADDRMAX ) {

			int b,f,j,n,a1,a2,ap;
			BYTE fmode;

			b = tramAddr - FXXTRAMADDRMIN;
			f = addrNew + offs;

			fmode = OS_READFLAGS(fxPgmGetChipHandle((FXID)pgmID),tramAddr);
			if( fmode & 0x20 )
				j = (int)(fmode & 1);
			else
				j = -18 - (int)(fmode&1);

			n = -(b/2) - 1 + f + j;

			a1 = -(n%16);
			a2 = 16 - (n%16);
			ap = ((a1*(-1))<a2) ? a1 : a2;
			adjust  = ap - (n%2);
			if( (adjust + f) < 0 ) adjust = 16 - (n%16) - (n%2);
		}
		/* End of A3 hack */
	}

	ulAddr = addrNew + offs + adjust /* A3 hack*/;

	OS_WRITEGPR( fxPgmGetChipHandle((FXID)pgmID), tramAddr, ulAddr );

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* Function:	fxParamInterpolate
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamInterpolate( FXPGMID pgmID, OPERAND gprAddress, ULONG ulDest
#if VARIABLE_RAMPER
				   , FXRATE fxRate
#endif
)
{
	FXINTERPOLATOR *pHead, *pPtr, *pClosest = NULL;
	ULONG ulDif, ulComp = 0xffffffff, ulChipHandle;
	ULONG ulUCL, ulUCH;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	pHead = ID2INT( fxRsrcGetInterpolatorID( fxPgmGetRsrcID((FXID)pgmID) ) );
	ulChipHandle = fxPgmGetChipHandle( (FXID)pgmID );

	if( !pHead ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_NO_INTERPOLATORS;
	}

	/* translate to physical GPR */
	if( gprAddress & (~VIRTMASK) ) 
		gprAddress = fxPgmMapVirtualToPhysicalGPR((FXID)pgmID, 
										(ADDR)(gprAddress&VIRTMASK));

	/* if GPR already being ramped, use this one */
	for( pPtr = pHead; pPtr; pPtr = pPtr->pChain ) {

		if( pPtr->inUse && pPtr->gpr == gprAddress ) {
			OS_WRITEGPR( ulChipHandle, pPtr->addrGPRy, ulDest );
			pPtr->ulDest = ulDest;
			OS_RELEASEMUTEX(fx8210Mutex);
			return FXERROR_NO_ERROR;
		}
	}

	/* find unused interpolator */
	for( pPtr = pHead; pPtr && pPtr->inUse; pPtr = pPtr->pChain ) {

		ulDif = OS_READGPR( ulChipHandle, pPtr->gpr );
		if( pPtr->gpr > LAST_GENERALGPR ) ulDif <<= TRAMADJSHIFT;
		ulDif = (ulDif > pPtr->ulDest) ? ulDif - pPtr->ulDest :
										 pPtr->ulDest - ulDif;
		if( ulDif < ulComp ) {
			ulComp = ulDif;
			pClosest = pPtr;
		}
	}

	/* all interpolators are in use, use whichever one is closest to
	 * its destination
	 */
	if( !pPtr ) {
		pPtr = pClosest;
		if( !pPtr ) {
			OS_RELEASEMUTEX(fx8210Mutex);
			return FXERROR_NO_INTERPOLATORS;
		}
	} else {
		pClosest = NULL;
	}

	/* translate to physical GPR */
	if( gprAddress & (~VIRTMASK) ) 
		gprAddress = fxPgmMapVirtualToPhysicalGPR((FXID)pgmID, 
										(ADDR)(gprAddress&VIRTMASK));

	/* Generate instruction: INTERP current, rate, dest, current */
	fxParamPackInstruction( FXINSTR_INTERP, gprAddress,
#if VARIABLE_RAMPER
							pPtr->addrGPRx, 
#else
							RAMP_CONSTANT,
#endif
							pPtr->addrGPRy, gprAddress,
							&ulUCL, &ulUCH );

	/* Load new values into X and Y
	 *?????????????????  Q: Should we set instruction to INTERP y,x,y,y
	 *?????????????????     before setting these?
	 */
#if VARIABLE_RAMPER
	OS_WRITEGPR( ulChipHandle, pPtr->addrGPRx, fxRate );
#endif
	OS_WRITEGPR( ulChipHandle, pPtr->addrGPRy, ulDest );
	
	/* write instruction */
	OS_WRITEINSTRUCTION( ulChipHandle, pPtr->instrAddr, 
						 ulUCL, ulUCH );

	/* if we took an interpolator already in use, slam its value */
	if( pClosest ) {
		if( pPtr->gpr <= LAST_GENERALGPR ) {
			OS_WRITEGPR( ulChipHandle, pPtr->gpr, pPtr->ulDest );
		} else {
			OS_WRITEGPR( ulChipHandle, pPtr->gpr, (pPtr->ulDest)>>TRAMADJSHIFT );
		}
	}

	pPtr->pgmID  = (FXID)pgmID;
	pPtr->inUse  = TRUE;
	pPtr->gpr    = gprAddress;
	pPtr->ulDest = ulDest;

	OS_RELEASEMUTEX(fx8210Mutex);
	
	return FXERROR_NO_ERROR;

}

/******************************************************************
*
* Function:	fxParamStopInterpolator
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamStopInterpolator( FXPGMID pgmID, OPERAND gprAddress )
{
	FXINTERPOLATOR *pHead, *pPtr;
	ULONG ulChipHandle;
	ULONG ulUCL, ulUCH;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	pHead = ID2INT( fxRsrcGetInterpolatorID( fxPgmGetRsrcID((FXID)pgmID) ) );
	ulChipHandle = fxPgmGetChipHandle((FXID)pgmID);
	
	/* translate to physical GPR */
	if( gprAddress & (~VIRTMASK) ) 
		gprAddress = fxPgmMapVirtualToPhysicalGPR((FXID)pgmID, 
										(ADDR)(gprAddress&VIRTMASK));

	for( pPtr = pHead; pPtr && !(pPtr->inUse && pPtr->gpr == gprAddress); 
		 pPtr = pPtr->pChain ) ;

	/* If program owns interpolator, stop and free it */
	if( pPtr ) {

		/* Generate instruction: INTERP y, x, y, y
		 * This is equivalent to:
		 *     y = y - x(y - y), or y = y
		 */
		fxParamPackInstruction( FXINSTR_INTERP, pPtr->addrGPRy,
#if VARIABLE_RAMPER
								pPtr->addrGPRx, 
#else
								RAMP_CONSTANT,
#endif
								pPtr->addrGPRy, 
								pPtr->addrGPRy, &ulUCL, &ulUCH );

		OS_WRITEINSTRUCTION( ulChipHandle, pPtr->instrAddr,
							 ulUCL, ulUCH );

		pPtr->inUse = FALSE;
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}


/******************************************************************
*
* Function:	fxParamStopAllInterpolators
*
* See description in FXPARMAN.H.
*
*******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxParamStopAllInterpolators( FXPGMID pgmID )
{
	FXINTERPOLATOR *pHead, *pPtr;
	ULONG ulChipHandle;
	ULONG ulUCL, ulUCH;

	if( !fxPgmValidPgmID((FXID)pgmID) ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	pHead = ID2INT( fxRsrcGetInterpolatorID( fxPgmGetRsrcID((FXID)pgmID) ) );
	ulChipHandle = fxPgmGetChipHandle((FXID)pgmID);
	
	for( pPtr = pHead; pPtr; pPtr = pPtr->pChain ) {

		/* If program owns interpolator, stop and free it */
		if( pPtr->inUse && pPtr->pgmID == pgmID ) {

			/* Generate instruction: INTERP y, x, y, y
			 * This is equivalent to:
			 *     y = y - x(y - y), or y = y
			 */
			fxParamPackInstruction( FXINSTR_INTERP, pPtr->addrGPRy,
#if VARIABLE_RAMPER
									pPtr->addrGPRx, 
#else
									RAMP_CONSTANT,
#endif
									pPtr->addrGPRy, 
									pPtr->addrGPRy, &ulUCL, &ulUCH );

			OS_WRITEINSTRUCTION( ulChipHandle, pPtr->instrAddr,
								 ulUCL, ulUCH );

			pPtr->inUse = FALSE;
		}
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/******************************************************************
*
* @func static VOID | fxParamPackInstruction |
*
* This function takes arguments for the opcode and four operands
* and creates a 44-bit instruction in the UCL/UCH format.
*
* @parm USHORT	| opcode	| Specifies the 4-bit opcode.
* @parm USHORT	| opa		| Specifies 10-bit (physical) A operand.
* @parm USHORT	| opx		| Specifies 10-bit (physical) X operand.
* @parm USHORT	| opy		| Specifies 10-bit (physical) Y operand.
* @parm USHORT	| opRes		| Specifies 10-bit (physical) RES operand.
* @parm ULONG *	| pulUCL	| Specifies low dword to fill.
* @parm ULONG * | pulUCH	| Sepecfied high dword to fill.
*
* @comm This is a static function, but may be upgraded if other managers
* have a similar need.
*
* The high 4-bits of <p packed_instruction[0]> are not used.
* 
*******************************************************************
*/
static VOID
fxParamPackInstruction( USHORT opcode,USHORT opa,USHORT opx,USHORT opy,USHORT opres,
					    ULONG *pulUCL, ULONG *pulUCH )
{
	*pulUCH = ((ULONG)opcode<<20) | ((ULONG)opres<<10) | ((ULONG)opa);
	*pulUCL = ((ULONG)opx<<10) | ((ULONG)opy);
}
