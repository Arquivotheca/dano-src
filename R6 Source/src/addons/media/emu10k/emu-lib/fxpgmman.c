/* @doc INTERNAL */
/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
*
* @module fxpgmman.c - FX8010 Program Manager API |
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
#include "fxprmapi.h"
#include "fxpgmman.h"
#include "fxresman.h"
#include "fxcbkman.h"
#include "fxparman.h"
#include "fxpatman.h"

#define SWAP16(p)  (USHORT)( (USHORT)(*((BYTE *)p))<<8 |   \
							 (USHORT)(*((BYTE *)(p+1))) ) 
#define SWAP32(p)  (ULONG)( (ULONG)(SWAP16(p))<<16     |   \
							(ULONG)(SWAP16(p+2) ) )
#define NOSWAP16(p)  (USHORT)(*((USHORT *)p))
#define NOSWAP32(p)  (ULONG)(*((ULONG *)p))

#if defined(__BYTE_COHERENT)
	#define RIFX16(t,p) (t?SWAP16(p):NOSWAP16(p))
	#define RIFX32(t,p) (t?SWAP32(p):NOSWAP32(p))
#else
	#define RIFX16(t,p) (t?NOSWAP16(p):SWAP16(p))
	#define RIFX32(t,p) (t?NOSWAP32(p):SWAP32(p))
#endif

#define CH2CK(str)   (CKID)*((CKID *)str)

#define RIFX_CHUNK		CH2CK("RIFX")
#define RIFF_CHUNK		CH2CK("RIFF")
#define PTXT_FORM		CH2CK("PTXT")
#define RESOURCE_CHUNK	CH2CK("rsrc")
#define CODE_CHUNK		CH2CK("code")
#define GPRI_CHUNK		CH2CK("gpri")
#define TRAM_CHUNK		CH2CK("tram")
#define GPRS_CHUNK		CH2CK("gprs")
#define PATC_CHUNK		CH2CK("patc")
#define SCAL_CHUNK		CH2CK("scal")
#define ISCL_CHUNK		CH2CK("iscl")
#define OSCL_CHUNK		CH2CK("oscl")
#define WSCL_CHUNK		CH2CK("wscl")
#define VECT_CHUNK		CH2CK("intr")
#define INP_CHUNK		CH2CK("inp ")
#define OUTP_CHUNK		CH2CK("outp")
#define LIST_CHUNK		CH2CK("LIST")
#define XTRA_CHUNK		CH2CK("xtra")
#define CKID_CHUNK		CH2CK("ckid")
#define TABL_CHUNK		CH2CK("tabl")

#define NEXTCHUNK( ck, p, s )  { p = (BYTE *)ck + sizeof(CKID); \
							     s = RIFX32(type,p);					\
								 p += sizeof(CKSIZE); }
#define NEXTCHUNKL( ck, p, s ) { p = (BYTE *)ck + sizeof(CKID); \
							     s = RIFX32(type,p)-4;				\
								 p += sizeof(CKSIZE) + sizeof(CKID); }

#define INPSCALRATE			FXRAMP_2 /* may need tweaking */
#define OUTSCALRATE			FXRAMP_2 /* may need tweaking */
#define WETSCALRATE			FXRAMP_2 /* may need tweaking */

typedef unsigned short GPRMODE;
#define GPRMODEMASK	0x03ff

/* @struct INSTRUCTION |
 * This structure specifies a single DSP instruction.
 * 
 * @field BYTE		| opcode	| Specifies the 4-bit opcode.
 * @field OPERAND	| opA		| Specifies the accumulator operand.
 * @field OPERAND	| opX		| Specifies the index X operand.
 * @field OPERAND	| opY		| Specifies the index Y operand.
 * @field OPERAND	| opRes		| Specifies the result operand.
 */
typedef struct {
	BYTE    opcode;
	OPERAND opA;
	OPERAND opX;
	OPERAND opY;
	OPERAND opRes;
} INSTRUCTION;

static INSTRUCTION NOP_INSTR = { FXINSTR_SKIP, 
								 ZERO_CONSTANT, 
								 ZERO_CONSTANT, 
								 ZERO_CONSTANT, 
								 ZERO_CONSTANT };

/* @struct FXPROGRAMINFO |
 * This structure contains high-level information about an FX program.
 *
 * @field FXID			| rsrcID		| Specifies the resource FXID.
 * @field FXID			| patchID		| Specifies the patch FXID.
 * @field ADDR			| instrAddr		| Specifies first physical instruction address.
 * @field ADDR			| nInstructions	| Specifies number of instruction steps.
 * @field ULONG			| ulChipHandle	| Specifies the handle of the chip we are running on.
 * @field OPERAND		| InputScaler	| Specifies the input scaler GPR.
 * @field OPERAND		| OutputScaler	| Specifies the output scaler GPR.
 * @field OPERAND		| WetDryScaler	| Specifies the wet/dry mix scaler GPR.
 * @field INSTRUCTION	| instrFirst	| Contains a copy of the first fixed-up instruction.
 * @field ADDR			| addrIntVector	| Contains the physical address of the program's interrupt vector.
 * @field ULONG			| ulInMask		| Contains port input mask.
 * @field ULONG			| ulOutMask		| Contains port output mask.
 * @field BOOL			| isRunning		| Specifies if program is started.
 * @field FXPROGRAMINFO *| pChain		| Chain pointer.
 */
typedef struct _fxPgmInfo {
	FXID		rsrcID;
	FXID		chipID;
	FXID		patchID;
	OPERAND		InputScaler;
	OPERAND		OutputScaler;
	OPERAND		WetDryScaler;
	INSTRUCTION instrFirst;
	ADDR		addrIntVector;
	BOOL		isPort;
	union {
		ULONG		ulChipHandle;	/* only valid in programs */
		ULONG		ulInMask;		/* only valid in ports */
	} u;
	ADDR		instrAddr;
	ADDR		nInstructions;
	ULONG		ulOutMask;			/* only valid in ports */
	ADDR		xtraGPR;
	ADDR		addrVirtTRAMBuf;
	ADDR		nBuffers;
	BOOL		isRunning;
	struct _fxPgmInfo *pChain;
} FXPROGRAMINFO;

static FXPROGRAMINFO *pFreePgmInfo=NULL;
static FXPROGRAMINFO *pActivePrograms=NULL;

#if !FX_DYNAMIC && !FXPSEUDO_DYNAMIC
	static FXPROGRAMINFO PgmInfo[FXMAX_CHIPS*FXMAX_PROGRAMS];
#endif

#if VALIDATE_PGMID
	static FXPROGRAMINFO *fxpid[FXMAX_CHIPS*FXMAX_PROGRAMS+1];

	static FXID fxnewid(FXPROGRAMINFO *p) { 
		int fxpindex;
		for(fxpindex=1;fxpid[fxpindex];fxpindex++);
		fxpid[fxpindex] = p;
		return ((FXID)((USHORT)fxpindex+0x4000));
	}
	static FXID getpid_(FXPROGRAMINFO *p) {
		int fxpindex;
		for(fxpindex=1;fxpid[fxpindex]!=p;fxpindex++);
		return ((FXID)((USHORT)fxpindex+0x4000));
	}

	#define ID2PGM(id)	((((USHORT)id)&XMASK) > (FXMAX_CHIPS*FXMAX_PROGRAMS+1) ? \
						NULL : fxpid[(((USHORT)id)&XMASK)])
	#define PGM2ID(p)	fxnewid(p)
	#define GETPID(p)	getpid_(p)
	#define INVPGM(id)	fxpid[(((USHORT)id)&XMASK)] = NULL;

#else

	#define ID2PGM(id)  (FXPROGRAMINFO *)(id)
	#define PGM2ID(p)   (FXID)p
	#define GETPID(p)	(FXID)p

	#define INVPGM(id)   ;    /* to be filled in later */
#endif

typedef DWORD FOURCC; /* Four-character code */

typedef FOURCC CKID; /* Four-character-code chunk identifier */
typedef DWORD CKSIZE; /* 32-bit unsigned size value */

static void getInstruction( BYTE **, INSTRUCTION *, int );
static void fixupInstruction( FXID, INSTRUCTION *, INSTRUCTION * );
static void writeInstruction( ULONG, ADDR, INSTRUCTION * );
static CK * rifxStart( CK *, CKSIZE *, int * );
static CK * rifxFindChunk( CK *, int, CKID, CKSIZE * );
static CK * rifxFindList( CK *, int, CKID, CKSIZE * );
static BOOL fxPgmIs( FXID, CK *, PGMRSRCQUERY * );

extern ULONG fx8210Mutex;

/*****************************************************************
* 
* Function:		fxPgmInitialize
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxPgmInitialize( )
{
#if !FX_DYNAMIC
#if FXPSEUDO_DYNAMIC
	pFreePgmInfo = NULL;
#else
	int i;

	/* Init PgmInfo array */
	for( i=0; i<FXMAX_CHIPS*FXMAX_PROGRAMS-1; i++ ) {
		PgmInfo[i].pChain = &(PgmInfo[i+1]);
	}
	PgmInfo[i].pChain = NULL;
	pFreePgmInfo = &(PgmInfo[0]);
#endif
#endif

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxPgmInitChip
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxPgmInitChip( FXID chipID, ULONG ulChipHandle )
{
	ADDR i;
	RSRCQUERY rsrcQ;

	OS_WAITMUTEX(fx8210Mutex);

#if !FX_DYNAMIC && FXPSEUDO_DYNAMIC
	{
	FXPROGRAMINFO	pgmblock[];
	extern int fxnextChipNo;
	extern int fxAllocedChips;

	if( fxnextChipNo > fxAllocedChips ) {
		pgmblock = (FXPROGRAMINFO *) 
			OS_MALLOC( sizeof(FXPROGRAMINFO) * FXMAX_PROGRAMS );
		if( !pgmblock ) {
			OS_RELEASEMUTEX(fx8210Mutex);
			return FXERROR_OUT_OF_MEMORY;
		}
		for( i=0; i<FXMAX_PROGRAMS-1; i++ ) {
			pgmblock[i].pChain = &(pgmblock[i+1]);
		}
		pgmblock[i].pChain = pFreePgmList;
		pFreePgmList = &(pgmblock[0]);
	}
#endif

	/* NOP all instructions */
	fxRsrcQueryChip( chipID, &rsrcQ );
	for( i = 0; i < rsrcQ.largestInstr; i++ ) {
		writeInstruction( ulChipHandle, i, &NOP_INSTR );
	}
	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxPgmFreeChip
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxPgmFreeChip( FXID chipID )
{
	FXPROGRAMINFO *p, *l;

	/* Find all programs associated with this chip */
	l = NULL;
	for( p = pActivePrograms; p; ) {
		if( p->chipID == chipID ) {
			fxPgmUnload( GETPID(p) );
			if( !l ) p = pActivePrograms;
			else p = l;
		} else {
			l = p;
			p = p->pChain;
		}
	}

	return FXERROR_NO_ERROR;

}

/*****************************************************************
* 
* Function:		fxPgmIsLoadable
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
BOOL EMUAPIEXPORT
fxPgmIsLoadable( FXPARTID partitionID, CK *ckPgmText )
{
	PGMRSRCQUERY rsrcQuery;

	if( !fxRsrcValidPartitionID((FXID)partitionID) ) return FALSE;

	if( !fxPgmIs( (FXID)partitionID, ckPgmText, &rsrcQuery ) )
		return FALSE;
	return fxRsrcPgmIsLoadable( (FXID)partitionID, &rsrcQuery );
}

/*****************************************************************
* 
* Function:		fxPgmCanReplace
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
BOOL EMUAPIEXPORT
fxPgmCanReplace( CK *ckPgmText, FXPGMID pgmID[], DWORD pgmIDSize )
{
	PGMRSRCQUERY rsrcQuery;
	FXID		 id;
	int			 i=0;

	if( pgmID[0] == UNALLOCED_ID ) return FALSE;
	if( ID2PGM(pgmID[0]) == NULL ) return FALSE;

	if( !fxPgmIs( fxRsrcWhichPartition( (ID2PGM(pgmID[0]))->rsrcID ), 
			 ckPgmText, &rsrcQuery ) )
		return FALSE;

	id = pgmID[0];
	while( id != UNALLOCED_ID && (DWORD)i < pgmIDSize ) {
		if( ID2PGM(id) == NULL ) return FALSE;
		pgmID[i++] = (FXPGMID)((ID2PGM(id))->rsrcID);
		id = (FXID)pgmID[i];
	}
	pgmID[i] = UNALLOCED_ID;

	return fxRsrcPgmCanReplace( &rsrcQuery, (FXID *)pgmID );
}

/*****************************************************************
* 
* Function:		fxPgmQueryRequiredResources
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmQueryRequiredResources( FXPARTID partitionID, CK *ckPgmText, PGMRSRCQUERY *pRsrcQuery )
{
	if( !fxRsrcValidPartitionID((FXID)partitionID) ) return FXERROR_INVALID_ID;

	if( !fxPgmIs( (FXID)partitionID, ckPgmText, pRsrcQuery ) )
		return FXERROR_INVALID_ID;

	return FXERROR_NO_ERROR;
}


/*****************************************************************
*
* @func static BOOL | fxPgmIs |
*
* This function converts the program text 'rsrc' RIFX chunk into a
* PGMRSRCQUERY structure.
*
* @parm FXID			| partitionID	| Specifies partition program would be loaded into. 
* @parm CK *		| ckPgmText		| Pointer to the program text RIFX buffer.
* @parm PGMRSRCQUERY *| rsrcQuery	| Specifies structure to fill.	
*
* @rdesc This function returns TRUE if operation succeeded, or FALSE 
*        otherwise.
*
******************************************************************
*/
static BOOL
fxPgmIs( FXID partitionID, CK *ckPgmText, PGMRSRCQUERY *rsrcQuery )
{
	RSRCQUERY	 partQuery;
	ADDR		 nOvScratch;
	CK			 *ck;
	BYTE		 *ptr;
	CKSIZE		  ckSize;
	int			  type;

	fxRsrcQueryPartition( partitionID, &partQuery );

	ck = rifxStart( ckPgmText, &ckSize, &type );
	if( !ck ) return FALSE;

	ck = rifxFindChunk( ck, type, RESOURCE_CHUNK, &ckSize );
	if( !ck ) return FALSE;

	ptr = (BYTE *)ck + sizeof(CKID)+sizeof(CKSIZE);

	/* ptr @ nInstructions */
	rsrcQuery->instrSize = (ADDR) RIFX16(type,ptr);
	ptr += sizeof( ADDR ); /* ptr @ nInputs */
	ptr += sizeof( ADDR ); /* ptr @ nOutputs */
	rsrcQuery->nGeneral = (ADDR) RIFX16(type,ptr);
	ptr += sizeof( ADDR ); /* ptr @ nGeneral */
	rsrcQuery->nGeneral += (ADDR) RIFX16(type,ptr);
	ptr += sizeof( ADDR ); /* ptr @ nTRAMBufs; */
	rsrcQuery->nTRAMbuf  = (ADDR) RIFX16(type,ptr);
	ptr += sizeof( ADDR ); /* ptr @ nXTRAMBufs; */
	rsrcQuery->nXTRAMbuf = (ADDR) RIFX16(type,ptr);
	ptr += sizeof( ADDR ); /* ptr @ nScratch; */
	nOvScratch = (ADDR) RIFX16(type,ptr);
	if( nOvScratch > partQuery.nScratch ) {
		rsrcQuery->nGeneral += (nOvScratch - partQuery.nScratch);
	}
	ptr += sizeof( ADDR ); /* ptr @ nTRAM */
	rsrcQuery->tramSize = RIFX32(type,ptr);
	ptr += sizeof( ULONG ); /* ptr @ nXTRAM */
	rsrcQuery->xtramSize = RIFX32(type,ptr);
	ptr += sizeof( ULONG ); /* ptr @ nTable */
	rsrcQuery->tableSize = RIFX32(type,ptr);

	return TRUE;

}

/*****************************************************************
* 
* Function:		fxPgmLoad
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmLoad( FXPARTID partitionID, FXPGMID *pgmID, CK *pgmTxt )
{
	FXPROGRAMINFO *pfxPgmInfo;
	PGMRSRC rsrc;
	PGMRSRCQUERY rsrcQuery;
	BYTE *ptr;
	CK *ck, *cktemp;
	CKSIZE ckSize, ckCount;
	FXSTATUS status = FXERROR_NO_ERROR;
	INSTRUCTION instr;
	ADDR addr, addrAddr, addrData;
	ADDR addrorg;
	ULONG ulBase;
	int type;
    USHORT i;
	int shft;
	int adjust; /* A3 hack */
	ULONG ulRevReg;
	char *pszChipRev;

	struct {
		OPERAND		gpr;
		ULONG		ulValue;
		ULONG		ulCount;
		INSTRFIELD	instrField;
	} tramInit;
	BYTE flags, align;

	OS_WAITMUTEX(fx8210Mutex);

	if( !fxPgmIsLoadable( (FXID)partitionID, pgmTxt ) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_OUT_OF_RESOURCES;
	}

	/* Allocate a new FXPROGRAMINFO struct */
#if FX_DYNAMIC	
	pFreePgmInfo = (FXPROGRAMINFO *) 
				OS_MALLOC( sizeof( FXPROGRAMINFO ) );
#endif

	if( !pFreePgmInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_OUT_OF_MEMORY;
	}
	pfxPgmInfo = pFreePgmInfo;

#if !FX_DYNAMIC
	pFreePgmInfo = pFreePgmInfo->pChain;
#endif

	*pgmID = (FXPGMID)(PGM2ID(pfxPgmInfo));
	pfxPgmInfo->pChain = pActivePrograms;
	pActivePrograms = pfxPgmInfo;

	pfxPgmInfo->isPort = FALSE;
	pfxPgmInfo->nBuffers = 0;
	pfxPgmInfo->isRunning = FALSE;

	/* Clear scalers */
	pfxPgmInfo->InputScaler = 0;
	pfxPgmInfo->OutputScaler = 0;
	pfxPgmInfo->WetDryScaler = 0;

	/* Allocate a new rsrcID struct */
	status = fxRsrcNewPgm( (FXID)partitionID, &(pfxPgmInfo->rsrcID) );

	/* Verify that chipRevisionID matches */
	ck = rifxStart( pgmTxt, &ckSize, &type );
	if( !ck ) status = FXERROR_INVALID_PGMTEXT;
	else {
		ck = rifxFindChunk( ck, type, CKID_CHUNK, &ckSize );
		if( !ck ) status = FXERROR_INVALID_PGMTEXT;
		else {
			ptr = (BYTE *)ck + sizeof(CKID)+sizeof(CKSIZE);
			pszChipRev = fxRsrcGetChipRevisionID(
							fxRsrcWhichChip(pfxPgmInfo->rsrcID),
							&ulRevReg
						 ); 
            if( memcmp( ptr, pszChipRev, strlen(pszChipRev) ) ) 
				status = FXERROR_CHIP_MISMATCH;
		}
	}

	/* Allocate DSP resources */
	if( status == FXERROR_NO_ERROR ) {
		pfxPgmInfo->u.ulChipHandle = fxRsrcGetChipHandle(pfxPgmInfo->rsrcID);
		pfxPgmInfo->chipID = fxRsrcWhichChip(pfxPgmInfo->rsrcID);
		
		/* Convert pgmText resource chunk into PGMRSRC struct */
		ck = rifxStart( pgmTxt, &ckSize, &type );
		if( !ck ) status = FXERROR_INVALID_PGMTEXT;
		else {

			ck = rifxFindChunk( ck, type, RESOURCE_CHUNK, &ckSize );
			if( !ck ) status = FXERROR_INVALID_PGMTEXT;
			else {

				ptr = (BYTE *)ck + sizeof(CKID)+sizeof(CKSIZE);

				/* ptr @ nInstructions */
				rsrc.instrSpace = (ADDR) RIFX16(type,ptr);
				ptr += sizeof( ADDR ); /* ptr @ nInputs */
				rsrc.nInputs = (ADDR) RIFX16(type,ptr);
				ptr += sizeof( ADDR ); /* ptr @ nOutputs */
				rsrc.nOutputs = (ADDR) RIFX16(type,ptr);
				ptr += sizeof( ADDR ); /* ptr @ nGeneral */
				rsrc.nGeneral = (ADDR) RIFX16(type,ptr);
				ptr += sizeof( ADDR ); /* ptr @ nTRAMBufs; */
				rsrc.nTRAMbuf  = (ADDR) RIFX16(type,ptr);
				ptr += sizeof( ADDR ); /* ptr @ nXTRAMBufs; */
				rsrc.nXTRAMbuf = (ADDR) RIFX16(type,ptr);
				ptr += sizeof( ADDR ); /* ptr @ nScratch; */
				rsrc.nScratch = (ADDR) RIFX16(type,ptr);
				ptr += sizeof( ADDR ); /* ptr @ nTRAM */
				rsrc.tramSpace = RIFX32(type,ptr);
				ptr += sizeof( ULONG ); /* ptr @ nXTRAM */
				rsrc.xtramSpace = RIFX32(type,ptr);
				ptr += sizeof( ULONG ); /* ptr @ nTable */
				rsrc.tableSpace = RIFX32(type,ptr);
				status = fxRsrcAllocPgm(pfxPgmInfo->rsrcID,&rsrc);
			}
		}
	}

	if( status == FXERROR_NO_ERROR ) {
		pfxPgmInfo->addrVirtTRAMBuf = rsrc.nOutputs + rsrc.nGeneral;
		pfxPgmInfo->nBuffers = rsrc.nTRAMbuf + rsrc.nXTRAMbuf;
	}

	fxRsrcQueryPgm( pfxPgmInfo->rsrcID, &rsrcQuery );

	/* set-up and spawn table loader */
	if( status == FXERROR_NO_ERROR ) {
		ck = rifxStart( pgmTxt, &ckSize, &type );
		ck = rifxFindChunk( ck, type, TABL_CHUNK, &ckSize );
		if( ck ) {
			ptr = (BYTE *)ck + sizeof(CKID);
			ckSize = RIFX32(type,ptr);
			ptr+=sizeof(CKSIZE);
			if( (ckSize / sizeof(ULONG)) > rsrcQuery.tableSize ) {
				status = FXERROR_INVALID_PGMTEXT;
			} else {
				for( addr = rsrcQuery.tableAddr; ckSize; addr++, ckSize-=sizeof(ULONG) ) {
					OS_WRITETABLE( pfxPgmInfo->u.ulChipHandle, addr, RIFX32(type,ptr) );
					ptr += sizeof(ULONG);
				}
			}
		}
	}
		
	/* Get Spare GPR */
	if( status == FXERROR_NO_ERROR ) {
		ck = rifxStart( pgmTxt, &ckSize, &type );
		ck = rifxFindList( ck, type, GPRI_CHUNK, &ckSize );
		if( ck ) {
			NEXTCHUNKL(ck, ptr, ckSize);
			ck = rifxFindChunk( (CK *)ptr, type, XTRA_CHUNK, &ckSize );
			if( ck ) {
				NEXTCHUNK(ck, ptr, ckCount);
				addr = RIFX16(type,ptr);
				if( addr&(~VIRTMASK) ) 
					addr = fxRsrcMapVirtualToPhysicalGPR( 
						pfxPgmInfo->rsrcID,
						(ADDR)(addr&VIRTMASK) );
				pfxPgmInfo->xtraGPR = addr;
			}
		} else status = FXERROR_NO_XTRAGPR;
	} else status = FXERROR_NO_XTRAGPR;

	/* Fix up first instruction and store it */
	if( status == FXERROR_NO_ERROR ) {
		/* locate instructions in program text */
		ck = rifxStart( pgmTxt, &ckSize, &type );
		ck = rifxFindChunk( ck, type, CODE_CHUNK, &ckSize );
		if( !ck ) status = FXERROR_INVALID_PGMTEXT;
		else {
			ptr = (BYTE *)ck + sizeof(CKID) + sizeof(CKSIZE);
			getInstruction( &ptr, &instr, type );
			fixupInstruction( *pgmID, &instr, &(pfxPgmInfo->instrFirst) );
		}
	}

	pfxPgmInfo->instrAddr = rsrcQuery.instrAddr;
	pfxPgmInfo->nInstructions = rsrcQuery.instrSize;

	/* Zero all general GPRs */
	for( addr = 0; addr < rsrc.nOutputs + rsrc.nGeneral; addr++ ) {
		OS_WRITEGPR( pfxPgmInfo->u.ulChipHandle, 
			fxRsrcMapVirtualToPhysicalGPR( pfxPgmInfo->rsrcID, addr ),
			0 );
	}

	if( status == FXERROR_NO_ERROR ) {

		/* Insert SKIP instruction */
		instr.opcode = FXINSTR_SKIP;
		instr.opA = CONDITION_CODE_REG;
		instr.opY = pfxPgmInfo->xtraGPR;
		instr.opX = UNCONDITIONAL_SKIP;
		instr.opRes = CONDITION_CODE_REG;
		fxRsrcQueryPgm( pfxPgmInfo->rsrcID, &rsrcQuery );
		OS_WRITEGPR( pfxPgmInfo->u.ulChipHandle,
					 pfxPgmInfo->xtraGPR,
					 pfxPgmInfo->nInstructions - 1 );

		writeInstruction( pfxPgmInfo->u.ulChipHandle, rsrcQuery.instrAddr, 
			              &instr );
		
	/* Fix up and install rest of instructions */
		addr = rsrcQuery.instrAddr + 1;
		for( i = 1; i < rsrc.instrSpace; i++, addr++ ) {
			getInstruction( &ptr, &instr, type );
			fixupInstruction( (FXID)*pgmID, &instr, &instr );
			writeInstruction( pfxPgmInfo->u.ulChipHandle, addr, &instr );
		}

	/* Set all TRAM/XTRAM buffer addresses and garbage counts and
	 *  set necessary align bits (clear TRAM engine mode).
	 *  store TRAM engine modes
	 */
		ck = rifxStart( pgmTxt, &ckSize, &type );
		ck = rifxFindList( ck, type, GPRI_CHUNK, &ckSize );
		if( ck ) {
			NEXTCHUNKL(ck, ptr, ckSize);
			ck = rifxFindChunk( (CK *)ptr, type, TRAM_CHUNK, &ckSize );
			if( ck ) {
				NEXTCHUNK(ck, ptr, ckCount);
				OS_STOPZEROCOUNTER(pfxPgmInfo->u.ulChipHandle);
				while( ckCount>0 ) {
					tramInit.gpr = RIFX16(type,ptr);
					ptr += sizeof(GPRMODE); 
					ckCount -= sizeof(GPRMODE);
					tramInit.ulValue = RIFX32(type,ptr);
					ptr += sizeof(ULONG);
					ckCount -= sizeof(ULONG);
					tramInit.ulCount = RIFX32(type,ptr);
					ptr += sizeof(ULONG);
					ckCount -= sizeof(ULONG);
					tramInit.instrField = RIFX16(type,ptr);
					ptr += sizeof(INSTRFIELD);
					ckCount -= sizeof(INSTRFIELD);
					addrData = fxRsrcMapVirtualToPhysicalGPR(
							pfxPgmInfo->rsrcID, 
							(ADDR)(tramInit.gpr & GPRMODEMASK) );
					addrAddr = fxRsrcMapVirtualToPhysicalGPR(
							pfxPgmInfo->rsrcID,
							(ADDR)((tramInit.gpr & GPRMODEMASK)+1));
					flags = (BYTE)((tramInit.gpr & (~GPRMODEMASK))>>8)&0xf0;
					
					ulBase = (addrAddr>=FXTRAMADDRMIN && 
						      addrAddr<=FXTRAMADDRMAX) ?
							  rsrcQuery.tramAddr : rsrcQuery.xtramAddr;
					
					/* Figure out align bit value here */
					if( ((tramInit.instrField) & (ADDR)FIELDMASK) == FXOP_RES ) {
						if( addrAddr >= FXXTRAMADDRMIN &&  
							addrAddr <= FXXTRAMADDRMAX ) {
							align = CALCULATEXTRAMWRITEALIGN(
								fxRsrcMapVirtualToPhysicalInstr(
									pfxPgmInfo->rsrcID, 
									(ADDR)(tramInit.instrField & (~FIELDMASK)) ),
								addrAddr-FXXTRAMADDRMIN );
						} else {
							align = CALCULATETRAMWRITEALIGN(
								fxRsrcMapVirtualToPhysicalInstr(
									pfxPgmInfo->rsrcID, 
									(ADDR)(tramInit.instrField & (~FIELDMASK)) ),
								addrAddr-FXTRAMADDRMIN );
						}
					} else {
						if( addrAddr >= FXXTRAMADDRMIN &&  
							addrAddr <= FXXTRAMADDRMAX ) {
							align = CALCULATEXTRAMREADALIGN(
								fxRsrcMapVirtualToPhysicalInstr(
									pfxPgmInfo->rsrcID, 
									(ADDR)(tramInit.instrField & (~FIELDMASK)) ),
								addrAddr-FXXTRAMADDRMIN );
						} else {
							align = CALCULATETRAMREADALIGN(
								fxRsrcMapVirtualToPhysicalInstr(
									pfxPgmInfo->rsrcID, 
									(ADDR)(tramInit.instrField & (~FIELDMASK)) ),
								addrAddr-FXTRAMADDRMIN );
						}
					}

					adjust = 0;
					if( !memcmp( pszChipRev, "EMU8010", strlen( "EMU8010" ) ) &&
						ulRevReg <= 3 ) {
						/* Now, we must implement the A3 silicon hack for XTRAM */
						/*
						   Let b = buffer number (addrAddr-FXXTRAMADDRMIN)
						   Let f = requested offset (tramInit.ulValue + ulBase)

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
						if( addrAddr >= FXXTRAMADDRMIN &&  
							addrAddr <= FXXTRAMADDRMAX ) {

							int b,f,j,n,a1,a2,ap;

							b = addrAddr - FXXTRAMADDRMIN;
							f = tramInit.ulValue + ulBase;

							if( ((tramInit.instrField) & (ADDR)FIELDMASK) == FXOP_RES )
								j = align;
							else
								j = -18 - align;

							n = -(b/2) - 1 + f + j;

							a1 = -(n%16);
							a2 = 16 - (n%16);
							ap = ((a1*(-1))<a2) ? a1 : a2;
							adjust  = ap - (n%2);
							if( (adjust + f) < 0 ) adjust = 16 - (n%16) - (n%2);
						}
						/* End of A3 hack */
					}

					OS_WRITEGPR( pfxPgmInfo->u.ulChipHandle, addrAddr, 
							tramInit.ulValue + ulBase + adjust /*A3 hack*/ );
					OS_WRITEGPR( pfxPgmInfo->u.ulChipHandle, addrData, 
							tramInit.ulCount );

					flags = fxRsrcMapEnumToPhysicalFlags( 
						pfxPgmInfo->rsrcID, (BYTE)(flags>>4) )<<4;
					flags |= align;
					OS_WRITEFLAGS( pfxPgmInfo->u.ulChipHandle, addrAddr,
						flags );

				}
				OS_STARTZEROCOUNTER(pfxPgmInfo->u.ulChipHandle);

			}
		}

	/* Initialize GPRs */
		ck = rifxStart( pgmTxt, &ckSize, &type );
		ck = rifxFindList( ck, type, GPRI_CHUNK, &ckSize );
		if( ck ) {
			NEXTCHUNKL(ck, ptr, ckSize);
			ck = rifxFindChunk( (CK *)ptr, type, GPRS_CHUNK, &ckSize );
			if( ck ) {
				NEXTCHUNK(ck, ptr, ckCount);
				while( ckCount>0 ) {
					addr = RIFX16(type,ptr);
					ptr += sizeof(OPERAND);
					ckCount -= sizeof(OPERAND);
					addrorg = addr;
					if( addr & (~VIRTMASK) ) {
						addr = fxRsrcMapVirtualToPhysicalGPR(
							pfxPgmInfo->rsrcID,
							(ADDR)(addr & VIRTMASK & (~BUFMASK) ) );
					}
					shft = ( addr > LAST_GENERALGPR ) ? TRAMADJSHIFT:0;
					switch( (ADDR)(addrorg & BUFMASK) ) {
					case TRAMMASK:
						OS_WRITEGPR( pfxPgmInfo->u.ulChipHandle, addr, 
									 RIFX32(type,ptr)+rsrcQuery.tramAddr );
						break;

					case XTRAMMASK:
						OS_WRITEGPR( pfxPgmInfo->u.ulChipHandle, addr, 
									 RIFX32(type,ptr)+rsrcQuery.xtramAddr );
						break;

					case TABLEMASK:
						OS_WRITEGPR( pfxPgmInfo->u.ulChipHandle, addr, 
									 RIFX32(type,ptr)+rsrcQuery.tableAddr );

					default:
						OS_WRITEGPR( pfxPgmInfo->u.ulChipHandle, addr, 
									 ((RIFX32(type,ptr))>>shft) );
						break;
					}
					ptr += sizeof(ULONG);
					ckCount -= sizeof(ULONG);
				}
			}
		}

	/* Store input, output, and wet/dry scalers */
		ck = rifxStart( pgmTxt, &ckSize, &type );
		ck = rifxFindList( ck, type, PATC_CHUNK, &ckSize );
		if( ck ) {
			NEXTCHUNKL(ck, ptr, ckSize);
			ck = rifxFindList( ptr, type, SCAL_CHUNK, &ckSize );
			if( ck ) {
				NEXTCHUNKL(ck, ptr, ckCount);
				ck = ptr;
				ckSize = ckCount;
				cktemp = rifxFindChunk(ck,type,ISCL_CHUNK,&ckSize);
				if( cktemp ) {
					ptr = (BYTE *)cktemp + sizeof(CKID)+
						sizeof(CKSIZE);
					pfxPgmInfo->InputScaler = (OPERAND) RIFX16(type,ptr);
					fxParamWriteGPR( *pgmID, (OPERAND)RIFX16(type,ptr), 0L );
				}
				ckSize = ckCount;
				cktemp = rifxFindChunk(ck,type,OSCL_CHUNK,&ckSize);
				if( cktemp ) {
					ptr = (BYTE *)cktemp + sizeof(CKID)+
						sizeof(CKSIZE);
					pfxPgmInfo->OutputScaler = (OPERAND) RIFX16(type,ptr);
					fxParamWriteGPR( *pgmID, (OPERAND)RIFX16(type,ptr), 0L );
				}
				ckSize = ckCount;
				cktemp = rifxFindChunk(ck,type,WSCL_CHUNK,&ckSize);
				if( cktemp ) {
					ptr = (BYTE *)cktemp + sizeof(CKID)+
						sizeof(CKSIZE);
					pfxPgmInfo->WetDryScaler = (OPERAND) RIFX16(type,ptr);
					fxParamWriteGPR( *pgmID, (OPERAND)RIFX16(type,ptr), 0L );
				}
			}
		}
		
		/* Find interrupt vector */
		ck = rifxStart( pgmTxt, &ckSize, &type );
		ck = rifxFindList( ck, type, GPRI_CHUNK, &ckSize );
		if( ck ) {
			NEXTCHUNKL( ck, ptr, ckSize );
			ck = rifxFindChunk( ptr, type, VECT_CHUNK, &ckSize );
			if( ck ) {
				ptr = (BYTE *)ck + sizeof(CKID) + sizeof(CKSIZE);
				pfxPgmInfo->addrIntVector = RIFX16(type,ptr);
				if( pfxPgmInfo->addrIntVector&(~VIRTMASK) ) 
					pfxPgmInfo->addrIntVector =
						fxPgmMapVirtualToPhysicalGPR( *pgmID,
								(ADDR)(pfxPgmInfo->addrIntVector&VIRTMASK) );
			}
		}
	}
			

	if( status == FXERROR_NO_ERROR ) {
		/* Initialize input and output patch stubs */
		status = fxPatchInitPgm( *pgmID, rsrc.nInputs, rsrc.nOutputs, 
								 &(pfxPgmInfo->patchID) );
	}

	if( status == FXERROR_NO_ERROR ) {
		ck = rifxStart( pgmTxt, &ckSize, &type );
		ck = rifxFindList( ck, type, PATC_CHUNK, &ckSize );
		if( ck ) {
			NEXTCHUNKL( ck, ptr, ckSize );
			ck = rifxFindChunk( ptr, type, INP_CHUNK, &ckSize );
			if( ck ) {
				ptr = (BYTE *)ck + sizeof(CKID);
				ckCount = RIFX32(type,ptr);
				ptr += sizeof(CKSIZE);
				
				for( i=0; 
					 i < rsrc.nInputs && ckCount >= sizeof(INSTRFIELD); 
					 i++, ckCount-=sizeof(INSTRFIELD) ) {
					
					addr = RIFX16(type,ptr);
					ptr += sizeof(INSTRFIELD);
					fxPatchInitInputStub( pfxPgmInfo->patchID,
										  i, (ADDR)0, addr );
					if( (ADDR)(addr&(~FIELDMASK)) == 0 ) {
						/* first instruction, fix up stored version */
						switch( (ADDR)(addr&FIELDMASK) ) {
						case FXOP_A:	
							(pfxPgmInfo->instrFirst).opA = ZERO_CONSTANT;
							break;
						case FXOP_X:
							(pfxPgmInfo->instrFirst).opX = ZERO_CONSTANT;
							break;
						case FXOP_Y:
							(pfxPgmInfo->instrFirst).opY = ZERO_CONSTANT;
							break;
						case FXOP_RES:
							(pfxPgmInfo->instrFirst).opRes = ZERO_CONSTANT;
							break;
						}
					} else {
						fxParamWriteInstructionField( *pgmID, addr, 
													  ZERO_CONSTANT );
					}
				}
				if( i != rsrc.nInputs ) 
					status = FXERROR_RESOURCE_MISMATCH;
			} else if( rsrc.nInputs ) {
				status = FXERROR_RESOURCE_MISMATCH;
			}
		} else if( rsrc.nInputs ) {
			status = FXERROR_RESOURCE_MISMATCH;
		}
	}

	if( status == FXERROR_NO_ERROR ) {
		ck = rifxStart( pgmTxt, &ckSize, &type );
		ck = rifxFindList( ck, type, PATC_CHUNK, &ckSize );
		if( ck ) {
			NEXTCHUNKL( ck, ptr, ckSize );
			ck = rifxFindChunk( ptr, type, OUTP_CHUNK, &ckSize );
			if( ck ) {
				ptr = (BYTE *)ck + sizeof(CKID);
				ckCount = RIFX32(type,ptr);
				ptr += sizeof(CKSIZE);
				
				for( i=0; 
					 i < rsrc.nOutputs && 
					 ckCount >= sizeof(OPERAND)+sizeof(INSTRFIELD); 
					 i++, 
					 ckCount-=(sizeof(OPERAND)+sizeof(INSTRFIELD)) ) {
					
					addr = RIFX16(type,ptr);
					ptr += sizeof(OPERAND);
					if( addr&(~VIRTMASK) ) {
						addr = fxRsrcMapVirtualToPhysicalGPR(
											pfxPgmInfo->rsrcID, 
											(ADDR)(addr&VIRTMASK) );
					}
					fxPatchInitOutputStub( pfxPgmInfo->patchID,
										  i, addr, 
										  (INSTRFIELD) RIFX16(type,ptr) );
					if( (ADDR)(RIFX16(type,ptr)&(~FIELDMASK)) == 0 ) {
						/* first instruction, fix up stored version */
						switch( (ADDR)(RIFX16(type,ptr)&FIELDMASK) ) {
						case FXOP_A:	
							(pfxPgmInfo->instrFirst).opA = addr;
							break;
						case FXOP_X:
							(pfxPgmInfo->instrFirst).opX = addr;
							break;
						case FXOP_Y:
							(pfxPgmInfo->instrFirst).opY = addr;
							break;
						case FXOP_RES:
							(pfxPgmInfo->instrFirst).opRes = addr;
							break;
						}
					} else {
						fxParamWriteInstructionField( *pgmID, (INSTRFIELD)RIFX16(type,ptr), 
													  addr );
					}
					ptr += sizeof(INSTRFIELD);
				}
				if( i != rsrc.nOutputs ) 
					status = FXERROR_RESOURCE_MISMATCH;
			} else if( rsrc.nOutputs ) {
				status = FXERROR_RESOURCE_MISMATCH;
			}
		} else if( rsrc.nOutputs ) {
			status = FXERROR_RESOURCE_MISMATCH;
		}
	}

	if( status != FXERROR_NO_ERROR ) {
		fxPgmUnload( pgmID );
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return status;
}

/*****************************************************************
* 
* Function:		fxPgmStart
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmStart( FXPGMID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM((FXID)pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	/* Insert first instruction */
	writeInstruction( pfxPgmInfo->u.ulChipHandle,
					  pfxPgmInfo->instrAddr, &(pfxPgmInfo->instrFirst) );

	pfxPgmInfo->isRunning = TRUE;

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxPgmStop
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmStop( FXPGMID pgmID )
{
	INSTRUCTION instr;
	FXPROGRAMINFO *pfxPgmInfo;
	ULONG ulUCL, ulUCH;

	pfxPgmInfo = ID2PGM((FXID)pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	pfxPgmInfo->isRunning = FALSE;

	/* Store first instruction again (in case things changed) */
	OS_READINSTRUCTION( pfxPgmInfo->u.ulChipHandle, pfxPgmInfo->instrAddr,
						&ulUCL, &ulUCH );
	(pfxPgmInfo->instrFirst).opcode = (BYTE)((ulUCH>>20)&0xf);
	(pfxPgmInfo->instrFirst).opA	= (ADDR)(ulUCH&0x3ff);
	(pfxPgmInfo->instrFirst).opX	= (ADDR)((ulUCL>>10)&0x3ff);
	(pfxPgmInfo->instrFirst).opY	= (ADDR)(ulUCL&0x3ff);
	(pfxPgmInfo->instrFirst).opRes	= (ADDR)((ulUCH>>10)&0x3ff);

	/* Insert SKIP instruction */
	instr.opcode = FXINSTR_SKIP;
	instr.opY = pfxPgmInfo->xtraGPR;
	instr.opA = CONDITION_CODE_REG;
	instr.opX = UNCONDITIONAL_SKIP;
	instr.opRes = CONDITION_CODE_REG;
	OS_WRITEGPR( pfxPgmInfo->u.ulChipHandle,
				 pfxPgmInfo->xtraGPR,
				 pfxPgmInfo->nInstructions - 1 );
	writeInstruction( pfxPgmInfo->u.ulChipHandle, pfxPgmInfo->instrAddr, 
					  &instr );

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxPgmUnload
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmUnload( FXPGMID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo, *p, *l;
	ADDR addr;

	pfxPgmInfo = ID2PGM((FXID)pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXERROR_INVALID_ID;

	OS_WAITMUTEX(fx8210Mutex);

	/* If any callbacks were associated, disassociate them */
	fxCallbackShutdown( (FXID)pgmID );

	/* If any interpolators are running, stop them */
	fxParamStopAllInterpolators( (FXID)pgmID );

	/* deallocate all input and output patch stubs */
	if( pfxPgmInfo->patchID != UNALLOCED_ID ) {
        fxPatchUnpatchPgm( pgmID );
		fxPatchFreePgm( pfxPgmInfo->patchID );
	}

	/* NOP all instructions from end to start */
	if( pfxPgmInfo->nInstructions ) {
		addr = pfxPgmInfo->instrAddr + pfxPgmInfo->nInstructions;
		do {
			addr--;
			writeInstruction( pfxPgmInfo->u.ulChipHandle, addr, &NOP_INSTR );
		} while (addr > pfxPgmInfo->instrAddr);
	}

	/* Clear all TRAM buffers */
	for( addr = pfxPgmInfo->addrVirtTRAMBuf+1; 
		 addr < pfxPgmInfo->addrVirtTRAMBuf + pfxPgmInfo->nBuffers*2;
		 addr+=2 ) {
		OS_WRITEFLAGS( pfxPgmInfo->u.ulChipHandle,
			fxRsrcMapVirtualToPhysicalGPR(pfxPgmInfo->rsrcID,addr),
			fxRsrcMapEnumToPhysicalFlags(pfxPgmInfo->rsrcID, (BYTE)MODE_OFF)<<4
		);
	};

	/* Deallocate DSP resources */
	if( pfxPgmInfo->rsrcID != UNALLOCED_ID ) {
		fxRsrcReleasePgm(pfxPgmInfo->rsrcID);
		fxRsrcDeletePgm(pfxPgmInfo->rsrcID);
	}

	/* Detach from active list */
	l = NULL;
	for( p=pActivePrograms; p && p != pfxPgmInfo; p=p->pChain ) l=p;
	if( p ) {
		if( l ) l->pChain = p->pChain;
		else pActivePrograms = p->pChain;
	}
	
	/* Deallocate FXPROGRAMINFO */
#if FX_DYNAMIC
	OS_FREE( pfxPgmInfo );
#else
	pfxPgmInfo->pChain = pFreePgmInfo;
	pFreePgmInfo = pfxPgmInfo;
#endif

	/* Invalidate pgmID */
	INVPGM((FXID)pgmID);

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxPgmRampdown
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmRampdown( FXPGMID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM((FXID)pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXERROR_INVALID_ID;

	if( pfxPgmInfo->InputScaler ) {
		fxParamInterpolate(pgmID,pfxPgmInfo->InputScaler,0L
#if VARIABLE_RAMPER
			,INPSCALRATE
#endif
			);
	}
	if( pfxPgmInfo->WetDryScaler ) {
		fxParamInterpolate(pgmID,pfxPgmInfo->WetDryScaler,0L
#if VARIABLE_RAMPER
			,WETSCALRATE
#endif
			);
	}
	if( pfxPgmInfo->OutputScaler ) {
		fxParamInterpolate(pgmID,pfxPgmInfo->OutputScaler,0L
#if VARIABLE_RAMPER
			,OUTSCALRATE
#endif
			);
	}

	return FXERROR_NO_ERROR;
}
	
/*****************************************************************
* 
* Function:		fxPgmRampup
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmRampup( FXPGMID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM((FXID)pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXERROR_INVALID_ID;

	if( pfxPgmInfo->OutputScaler ) {
		fxParamInterpolate(pgmID,pfxPgmInfo->OutputScaler,RAMP_ONE
#if VARIABLE_RAMPER
			,OUTSCALRATE
#endif
			);
	}
	if( pfxPgmInfo->WetDryScaler ) {
		fxParamInterpolate(pgmID,pfxPgmInfo->WetDryScaler,RAMP_ONE
#if VARIABLE_RAMPER
			,WETSCALRATE
#endif
			);
	}
	if( pfxPgmInfo->InputScaler ) {
		fxParamInterpolate(pgmID,pfxPgmInfo->InputScaler,RAMP_ONE
#if VARIABLE_RAMPER
			,INPSCALRATE
#endif
			);
	}

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxPgmInputMix
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmInputMix( FXPGMID pgmID, ULONG ulValue )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM((FXID)pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXERROR_INVALID_ID;

	if( pfxPgmInfo->InputScaler ) {
		fxParamInterpolate(pgmID,pfxPgmInfo->InputScaler, ulValue
#if VARIABLE_RAMPER
			,INPSCALRATE
#endif
			);
	} else return FXERROR_NOMIX_PARAM;

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxPgmOutputMix
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmOutputMix( FXPGMID pgmID, ULONG ulValue )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM((FXID)pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXERROR_INVALID_ID;

	if( pfxPgmInfo->OutputScaler ) {
		fxParamInterpolate(pgmID,pfxPgmInfo->OutputScaler, ulValue
#if VARIABLE_RAMPER
			,OUTSCALRATE
#endif
			);
	} else return FXERROR_NOMIX_PARAM;

	return FXERROR_NO_ERROR;
}
	
/*****************************************************************
* 
* Function:		fxPgmWetMix
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmWetMix( FXPGMID pgmID, ULONG ulValue )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM((FXID)pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXERROR_INVALID_ID;

	if( pfxPgmInfo->WetDryScaler ) {
		fxParamInterpolate(pgmID,pfxPgmInfo->WetDryScaler, ulValue
#if VARIABLE_RAMPER
			,WETSCALRATE
#endif
			);
	} else return FXERROR_NOMIX_PARAM;

	return FXERROR_NO_ERROR;
}

/*****************************************************************
*
* @func static CK * | rifxStart |
*
* This function moves a pointer to after the 'PTXT' header chunk.
*
* @parm CK *	| pck		| Specifies start of program text buffer.
* @parm CKSIZE *| pckSize	| Returns size of PTXT chunk, in bytes.
*	
* @rdesc This function returns a pointer to the data area of a PTXT
* chunk, or NULL if format not valid.
*
******************************************************************
*/
static CK *
rifxStart( CK *pck, CKSIZE *pckSize, int *type )
{
	BYTE *ptr;

	ptr = (BYTE *)pck;

	if( *((ULONG *)ptr) == RIFX_CHUNK ) *type = 1;
	else if( *((ULONG *)ptr) == RIFF_CHUNK ) *type = 0;
	else return NULL;

	ptr+=sizeof(CKID);
	*pckSize = RIFX32(*type,ptr) - sizeof(CKID);
	ptr+=sizeof(CKSIZE);
	if( *((ULONG *)ptr) != PTXT_FORM ) return NULL;
	ptr += sizeof(CKID);
	
	return ((CK *) ptr );
}
	
/*****************************************************************
*
* @func static CK * | rifxFindChunk |
*
* This function moves a pointer to after the specified sub-header chunk.
*
* @parm CK *	| pck		| Specifies pointer to data area of master chunk.
* @parm CKID	| ckID		| Specifies chunk name to search for.
* @parm CKSIZE *| pckSize	| Input: Size of current master chunk data, 
*                             Output: Returns size of <p ckID> chunk, in bytes.
*	
* @rdesc This function returns a pointer to the data area of the <p ckID>
* chunk, or NULL if format not valid.
*
******************************************************************
*/
static CK *
rifxFindChunk( CK *pck, int type, CKID ckID, CKSIZE *ckSize )
{
	BYTE *ptr=pck;
	CKSIZE ckCount=0;
	CKID compID;
	CKSIZE ckSizeTemp = *ckSize;

	while( (CKSIZE)(ptr - (BYTE *)pck) < ckSizeTemp ) {
		compID = *((ULONG *)pck);

		ptr = (BYTE *) pck + sizeof( CKID );
		ckCount = RIFX32(type,ptr);
		ptr += sizeof(CKSIZE);
		ptr += ckCount;
		if( ckCount & 1 ) ptr++;   /* word-align */

		ckSizeTemp -= (ckCount + (ckCount&1) + 
				  sizeof(CKID) + sizeof(CKSIZE));
		if( compID == ckID ) {
			*ckSize = ckSizeTemp;
			return pck;
		}

		pck = (CK *)ptr;
	}

	return NULL;
}

static CK *
rifxFindList( CK *pck, int type, CKID ckID, CKSIZE *ckSize )
{
	BYTE *ptr=pck;
	CKSIZE ckCount=0;
	CKID listID, compID;
	long ckSizeTemp = (long) *ckSize;

	while( (long)(ptr - (BYTE *)pck) < ckSizeTemp ) {
		listID = *((ULONG *)pck);

		ptr = (BYTE *) pck + sizeof( CKID );
		ckCount = RIFX32(type,ptr);
		ptr += sizeof(CKSIZE);
		compID = *((ULONG *)ptr);
		ptr += ckCount;
		if( ckCount & 1 ) ptr++;   /* word-align */

		ckSizeTemp -= (ckCount + (ckCount&1) + 
				  sizeof(CKID) + sizeof(CKSIZE));
		if( listID == LIST_CHUNK && compID == ckID ) {
			*ckSize = ckSizeTemp;
			return pck;
		}

		pck = (CK *)ptr;
	}

	return NULL;
}

/*****************************************************************
* 
* Function:		fxPgmWhichPartition
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXPARTID EMUAPIEXPORT
fxPgmWhichPartition( FXPGMID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM((FXID)pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return UNALLOCED_ID;

	return( (FXPARTID)fxRsrcWhichPartition( pfxPgmInfo->rsrcID ) );
}

/*****************************************************************
* 
* Function:		fxPgmWhichChip
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXCHIPID EMUAPIEXPORT
fxPgmWhichChip( FXPGMID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM((FXID)pgmID);
	if( !pfxPgmInfo ) return UNALLOCED_ID;

	return( (FXCHIPID)pfxPgmInfo->chipID );
}

/*****************************************************************
* 
* Function:		fxPgmGetChipHandle
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
ULONG
fxPgmGetChipHandle( FXPGMID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo ) return 0L;

	return( pfxPgmInfo->u.ulChipHandle );
}

/*****************************************************************
* 
* Function:		fxPgmGetRsrcID
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXID
fxPgmGetRsrcID( FXID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return UNALLOCED_ID;

	return( pfxPgmInfo->rsrcID );
}

/*****************************************************************
* 
* Function:		fxPgmInterpolatorID
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXID
fxPgmGetInterpolatorID( FXID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return UNALLOCED_ID;

	return( fxRsrcGetInterpolatorID(pfxPgmInfo->rsrcID) );
}

/*****************************************************************
* 
* Function:		fxPgmGetPatchID
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXID
fxPgmGetPatchID( FXID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo ) return UNALLOCED_ID;

	return( pfxPgmInfo->patchID );
}

/*****************************************************************
* 
* Function:		fxPgmIsPortID
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
BOOL
fxPgmIsPortID( FXID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo || !(pfxPgmInfo->isPort) ) return FALSE;

	return TRUE;
}

/*****************************************************************
* 
* Function:		fxPgmGetIntVector
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
ADDR
fxPgmGetIntVector( FXID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXADDRERROR;

	return( pfxPgmInfo->addrIntVector );
}

/*****************************************************************
* 
* Function:		fxPgmMapVirtualToPhysicalGPR
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
ADDR
fxPgmMapVirtualToPhysicalGPR( FXID pgmID, ADDR addrVirt )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXADDRERROR;

	return( fxRsrcMapVirtualToPhysicalGPR( pfxPgmInfo->rsrcID, addrVirt ) );
}

/*****************************************************************
* 
* Function:		fxPgmMapPhysicalToVirtualGPR
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
ADDR
fxPgmMapPhysicalToVirtualGPR( FXID pgmID, ADDR addrPhys )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXADDRERROR;

	return( fxRsrcMapPhysicalToVirtualGPR( pfxPgmInfo->rsrcID, addrPhys ) );
}

/*****************************************************************
* 
* Function:		fxPgmMapVirtualToPhysicalInstr
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
ADDR
fxPgmMapVirtualToPhysicalInstr( FXID pgmID, ADDR addrVirt )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXADDRERROR;

	return( fxRsrcMapVirtualToPhysicalInstr( pfxPgmInfo->rsrcID, addrVirt ) );
}

/*****************************************************************
* 
* Function:		fxPgmMapPhysicalToVirtualInstr
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
ADDR
fxPgmMapPhysicalToVirtualInstr( FXID pgmID, ADDR addrPhys )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo || pfxPgmInfo->isPort ) return FXADDRERROR;

	return( fxRsrcMapPhysicalToVirtualInstr( pfxPgmInfo->rsrcID, addrPhys ) );
}


/*****************************************************************
*
* @func static void | getInstruction |
*
* This function retrieves the next instruction from a 'code' chunk.
*
* @parm BYTE **		 | ptr	| Specifies address of pointer in code chunk.
* @parm INSTRUCTION *| instr| Specifies pointer to instruction buffer.
*
******************************************************************
*/
static void
getInstruction( BYTE **ptr, INSTRUCTION *instr, int type )
{
	instr->opcode = **ptr;
	(*ptr)++;
	instr->opA = RIFX16(type,*ptr);
	(*ptr)+=sizeof(OPERAND);
	instr->opX = RIFX16(type,*ptr);
	(*ptr)+=sizeof(OPERAND);
	instr->opY = RIFX16(type,*ptr);
	(*ptr)+=sizeof(OPERAND);
	instr->opRes = RIFX16(type,*ptr);
	(*ptr)+=sizeof(OPERAND);
}

/*****************************************************************
* 
* @func static void | fixupInstruction |
*
* This function fixes up a single instruction.
*
* @parm FXID			 | pgmID	| Specifies program to fix.
* @parm INSTRUCTION *| instrIn	| Specifies "broken down" instruction.
* @parm INSTRUCTION *| instrOut	| Specifies buffer for fixed up instruction.
*
******************************************************************
*/
static void
fixupInstruction( FXID pgmID, INSTRUCTION *instrIn, INSTRUCTION *instrOut )
{
	instrOut->opcode = instrIn->opcode;
	instrOut->opA = (instrIn->opA & ~(VIRTMASK) ) ?
		fxPgmMapVirtualToPhysicalGPR(pgmID, (ADDR)(instrIn->opA & VIRTMASK)):
		instrIn->opA ;
	instrOut->opX = (instrIn->opX & ~(VIRTMASK) ) ?
		fxPgmMapVirtualToPhysicalGPR(pgmID, (ADDR)(instrIn->opX & VIRTMASK)):
		instrIn->opX ;
	instrOut->opY = (instrIn->opY & ~(VIRTMASK) ) ?
		fxPgmMapVirtualToPhysicalGPR(pgmID, (ADDR)(instrIn->opY & VIRTMASK)):
		instrIn->opY ;
	instrOut->opRes = (instrIn->opRes & ~(VIRTMASK) ) ?
		fxPgmMapVirtualToPhysicalGPR(pgmID, (ADDR)(instrIn->opRes & VIRTMASK)):
		instrIn->opRes ;
}

/*****************************************************************
* 
* @func static void | writeInstruction |
*
* This function writes a fixed-up instruction to the DSP.
*
* @parm ULONG		 | ulChipHandle	| Specifies handle of DSP.
* @parm ADDR		 | addr			| Specifies physical instruction address.
* @parm INSTRUCTION *| instr		| Specifies instruction to write.
*
******************************************************************
*/
static void
writeInstruction( ULONG ulChipHandle, ADDR addr, INSTRUCTION *instr )
{
	ULONG ulUCL, ulUCH;

	ulUCL = ((ULONG)((instr->opX)&0x3ff))<<10 | 
			((ULONG)((instr->opY)&0x3ff));
	ulUCH = ((ULONG)((instr->opcode)&0xf))<<20 |
			((ULONG)((instr->opRes)&0x3ff))<<10 | 
			((ULONG)((instr->opA)&0x3ff));

	OS_WRITEINSTRUCTION( ulChipHandle, addr, ulUCL, ulUCH );
}

/*****************************************************************
* 
* @func static int | COUNT1S |
*
* This function counts how many 1s are in a 32-bit field.
*
* @parm ULONG		 | ulMask	| Specifies 32-bit mask.
*
* @rdesc Returns a value between 0 and 31.
*
******************************************************************
*/
static int
COUNT1S( ULONG ulMask )
{
	int i,j;

    for(i=j=0; i<32; i++, ulMask>>=1 ) j += (int)(ulMask&1);

	return j;
}


/*****************************************************************
* 
* Function:		fxPgmCreatePortID
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmCreatePortID( FXCHIPID chipID, ULONG ulInMask, ULONG ulOutMask, FXPGMID *portID )
{
	FXPROGRAMINFO *pfxPgmInfo;
	FXSTATUS status;
	int	i, logicalChannel;

	OS_WAITMUTEX(fx8210Mutex);

	/* See if port assignment map is valid */
	if( !(fxRsrcMaskPorts( (FXID)chipID, ulInMask, ulOutMask ) ) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_PORTS_ALREADY_ASSIGNED;
	}

	/* Allocate a new FXPROGRAMINFO struct */
#if FX_DYNAMIC	
	pFreePgmInfo = (FXPROGRAMINFO *) 
				OS_MALLOC( sizeof( FXPROGRAMINFO ) );
#endif

	if( !pFreePgmInfo ) return FXERROR_OUT_OF_MEMORY;
	pfxPgmInfo = pFreePgmInfo;

#if !FX_DYNAMIC
	pFreePgmInfo = pFreePgmInfo->pChain;
#endif

	*portID = (FXPGMID)PGM2ID(pfxPgmInfo);

	/* Ports don't have any resources */
	pfxPgmInfo->rsrcID = NULL;

	pfxPgmInfo->chipID = (FXID)chipID;

	status = fxPatchInitPgm( (FXID)*portID, (USHORT)COUNT1S(ulOutMask), 
							 (USHORT)COUNT1S(ulInMask), 
							 &(pfxPgmInfo->patchID) );
	if( status != FXERROR_NO_ERROR ) {
		/* Deallocate FXPROGRAMINFO */
#if FX_DYNAMIC
		OS_FREE( pfxPgmInfo );
#else
		pfxPgmInfo->pChain = pFreePgmInfo;
		pFreePgmInfo = pfxPgmInfo;
#endif
		/* Invalidate portID */
		INVPGM((FXID)portID);
		
		return status;
	}

	pfxPgmInfo->u.ulInMask  = ulInMask;
	pfxPgmInfo->ulOutMask = ulOutMask;

	for( logicalChannel = i=0; i<32; i++, ulOutMask>>=1 ) {
		if( ulOutMask&1 ) {
			fxPatchInitInputStub( pfxPgmInfo->patchID, (USHORT)logicalChannel,
								  (ADDR)(OUTPUTPORT0+i), (ADDR)0 );
			logicalChannel++;
		}
	}

	for( logicalChannel = i=0; i<32; i++, ulInMask>>=1 ) {
		if( ulInMask&1 ) {
			fxPatchInitOutputStub( pfxPgmInfo->patchID, (USHORT)logicalChannel,
								  (ADDR)(INPUTPORT0+i), (ADDR)0 );
			logicalChannel++;
		}
	}

	pfxPgmInfo->isPort = TRUE;
	fxRsrcSetPortMask( (FXID)chipID, ulInMask, ulOutMask );

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;								  
}

/*****************************************************************
* 
* Function:		fxPgmFreePortID
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxPgmFreePortID( FXPGMID portID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	OS_WAITMUTEX(fx8210Mutex);

	pfxPgmInfo = ID2PGM((FXID)portID);
	if( !pfxPgmInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	if( !(pfxPgmInfo->isPort) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	if( pfxPgmInfo->patchID != UNALLOCED_ID ) {
		fxPatchUnpatchPgm( pfxPgmInfo->patchID );
		fxPatchFreePgm( pfxPgmInfo->patchID );
	}

	fxRsrcClearPortMask( pfxPgmInfo->chipID, pfxPgmInfo->u.ulInMask, 
						 pfxPgmInfo->ulOutMask );

	/* Deallocate FXPROGRAMINFO */
#if FX_DYNAMIC
	OS_FREE( pfxPgmInfo );
#else
	pfxPgmInfo->pChain = pFreePgmInfo;
	pFreePgmInfo = pfxPgmInfo;
#endif

	/* Invalidate portID */
	INVPGM((FXID)portID);

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxPgmValidPgmID
*
* See FXPGMMAN.H for description.
*
******************************************************************
*/
BOOL
fxPgmValidPgmID( FXID pgmID )
{
	if( ID2PGM(pgmID) == NULL ) return FALSE;
	return TRUE;
}

/*****************************************************************
* 
* Function:		fxPgmIsRunning
*
* See FXMGRAPI.H for description.
*
******************************************************************
*/
BOOL EMUAPIEXPORT
fxPgmIsRunning( FXPGMID pgmID )
{
	FXPROGRAMINFO *pfxPgmInfo;

	if( (pfxPgmInfo=ID2PGM((FXID)pgmID)) == NULL ) return FALSE;

	return( pfxPgmInfo->isRunning );
}

/*****************************************************************
* 
* Function:		fxPgmGetFirstInstr
*
* See FXMGRAPI.H for description.
*
******************************************************************
*/
void
fxPgmGetFirstInstr( FXID pgmID, USHORT *opcode, USHORT *opA, USHORT *opX, USHORT *opY, USHORT *opRes )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	*opcode = (pfxPgmInfo->instrFirst).opcode;
	*opA =	  (pfxPgmInfo->instrFirst).opA;
	*opX =    (pfxPgmInfo->instrFirst).opX;
	*opY =    (pfxPgmInfo->instrFirst).opY;
	*opRes =  (pfxPgmInfo->instrFirst).opRes;
}

/*****************************************************************
* 
* Function:		fxPgmWriteFirstInstr
*
* See FXMGRAPI.H for description.
*
******************************************************************
*/
void
fxPgmWriteFirstInstr( FXID pgmID, USHORT opcode, USHORT opA, USHORT opX, USHORT opY, USHORT opRes )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	(pfxPgmInfo->instrFirst).opcode = (BYTE)opcode;
	(pfxPgmInfo->instrFirst).opA	= opA;
	(pfxPgmInfo->instrFirst).opX    = opX;
	(pfxPgmInfo->instrFirst).opY    = opY;
	(pfxPgmInfo->instrFirst).opRes  = opRes;
}

/*****************************************************************
* 
* Function:		fxPgmQueryProgram
*
* See FXMGRAPI.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT fxPgmQueryProgram( FXPGMID pgmID, PGMRSRCQUERY *pRsrc )
{
	FXPROGRAMINFO *pfxPgmInfo;

	pfxPgmInfo = ID2PGM(pgmID);
	if( !pfxPgmInfo ) return FXERROR_INVALID_ID;

	return fxRsrcQueryPgm( pfxPgmInfo->rsrcID, pRsrc );
}
