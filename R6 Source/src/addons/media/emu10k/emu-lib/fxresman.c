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
* Architecture Outline and Notes" Rev 2.3.
*
*******************************************************************
*/
#include <string.h>
#include "fxconfig.h"
#include "fxresman.h"
#include "fxparman.h"

/* Maximum size of GPR space */
#define MAX_GPRS			0x400

/* Maximum independently virtualized GPRs */
#define MAX_RELOC_GPRS		0x340

/* CIRC should be set to TRUE if TRAM and XTRAM spaces can
 * wrap past the head pointer.  This determines whether two
 * free blocks at the beginning and end of the address space
 * can be coalesced into a single block.
 */
#define CIRC				FALSE

#define FLAGMAPSIZE			16

#define INTERPOLATORID		(FXID)1

/* GPRs fall into 3 different categories:  general, TRAM buffer
 * GPRs, or XTRAM buffer GPRs
 */
typedef enum { GPR_GENERAL, GPR_TRAM, GPR_XTRAM } GPRTYPE;

typedef short INDEX;

/* @struct PGMINFOSTRUCT |
 * 
 * This structure contains information about the
 * reserved resources for a program.
 *
 * @field FXID	| partitionID	| Contains FXID of partition program belongs to.
 * @field FXID	| chipID		| Contains FXID of chip program resides on.
 * @field ADDR	| gprBaseReloc	| Contains base index in <p gprReloc[]>.
 * @field ADDR	| gprSpace		| Contains virtual GPR space size.
 * @field INDEX	| instrLoc		| Contains index into <t memMap[]> of instruction block.
 * @field INDEX	| tramLoc		| Contains index into <t memMap[]> of TRAM block.
 * @field INDEX	| xtramLoc		| Contains index into <t memMap[]> of XTRAM block.
 * @field INDEX	| tableLoc		| Contains index into <t memMap[]> of table RAM block.
 * @field ADDR	| nGeneral		| Contains number of general GPRs.
 * @field ADDR	| nTRAMbuf		| Contains number of TRAM buffers.
 * @field ADDR	| nXTRAMbuf		| Contains number of XTRAM buffers.
 * @field ADDR	| addrSpare		| Contains the address of a spare GPR for internal use.
 * @field struct PgmInfoStructTag * |
 *				  pChain		| Chain pointer.
 */
typedef struct PgmInfoStructTag {
	FXID				partitionID;	
	FXID				chipID;			
	ADDR				gprBaseReloc;	
	ADDR				gprSpace;		
	INDEX				instrLoc;		
	INDEX				tramLoc;		
	INDEX				xtramLoc;		
	INDEX				tableLoc;		
	ADDR				nGeneral;		
	ADDR				nTRAMbuf;		
	ADDR				nXTRAMbuf;		
	struct PgmInfoStructTag *pChain;	
} PGMINFOSTRUCT;

/* @struct PARTITIONINFOSTRUCT |
 *
 * This structure contains information about the
 * reserved resources for a partition
 *
 * @field FXID			 | chipID	| Contains FXID of chip partition resides on.
 * @field INDEX			 | instrLoc	| Contains index into <t memMap[]> of instruction block.
 * @field INDEX			 | tramLoc	| Contains index into <t memMap[]> of TRAM block.
 * @field INDEX			 | xtramLoc	| Contains index into <t memMap[]> of XTRAM block.
 * @field INDEX			 | tableLoc	| Contains index into <t memMap[]> of table RAM block.
 * @field PGMINFOSTRUCT	*| pPgmList	| Contains pointer to head of program list.
 * @field struct PartitionInfoStructTag * |
 *						   pChain	| Chain pointer.
 *
 * @xref <t PGMINFOSTRUCT>
 */
typedef struct PartitionInfoStructTag {
	FXID				chipID;			
	INDEX				instrLoc;		
	INDEX				tramLoc;		
	INDEX				xtramLoc;		
	INDEX				tableLoc;		
	PGMINFOSTRUCT	   *pPgmList;		
	struct PartitionInfoStructTag *pChain; 
} PARTITIONINFOSTRUCT;

/* @struct CHIPINFOSTRUCT |
 *
 * This structure contains information about the
 * distribution of resources and free resources on a chip.
 *
 * @field FXID	| gprAlloc[ MAX_GPRS ]			| GPR owner array.
 * @field ADDR	| gprReloc[ MAX_RELOC_GPRS ]	| Program virtual address space relocation array.
 * @field ADDR	| gprRelocTop	| Contains index of next virtual block.
 * @field ADDR	| gprSpace		| Contains total available GPRs.
 * @field ADDR	| addrGeneral	| Contains first address of general GPR space.
 * @field ADDR	| addrTRAMdata	| Contains first address of TRAM data GPRs.
 * @field ADDR	| addrTRAMaddr	| Contains first address of TRAM address GPRs.
 * @field ADDR	| addrXTRAMdata	| Contains first address of XTRAM data GPRs.
 * @field ADDR	| addrXTRAMaddr	| Contains first address of XTRAM address GPRs.
 * @field ADDR	| sizeGeneral	| Contains size of general GPR space.
 * @field ADDR	| sizeTRAMdata	| Contains size of TRAM buffer space.
 * @field ADDR	| sizeXTRAMdata	| Contains size of XTRAM buffer space.
 * @field INDEX	| instrLoc		| Contains index into <t memMap[]> of instruction block.
 * @field INDEX	| tramLoc		| Contains index into <t memMap[]> of TRAM block.
 * @field INDEX	| xtramLoc		| Contains index into <t memMap[]> of XTRAM block.
 * @field INDEX	| tableLoc		| Contains index into <t memMap[]> of table RAM block.
 * @field INDEX | interpLoc		| Contains index into <t memMap[]> of interpolators.
 * @field ADDR	| nScratch		| Contains number of scratch registers.
 * @field ADDR	| nInterpolators| Contains number of interpolators.
 * @field PARTITIONINFOSTRUCT * |pPartitionList	| Partitition list.
 * @field BYTE	| flagMap[FLAGMAPSIZE]			| Contains enumeration to physical TRAM flag conversion.
 * @field ULONG	| ulChipHandle	| Contains opaque hardware handle.
 * @field FXID	| interpolatorID| Contains interpolator FXID.
 * @field ULONG	| ulInMask		| Contains mask of input ports allocated.
 * @field ULONG	| ulOutMask		| Contains mask of output ports allocated.
 */
typedef struct ChipInfoStructTag {
	FXID				gprAlloc[ MAX_GPRS ]; 
	ADDR				gprReloc[ MAX_RELOC_GPRS ]; 
	ADDR				gprRelocTop;	
	ADDR				gprSpace;		
	ADDR				addrGeneral;	
	ADDR				addrTRAMdata;	
	ADDR				addrTRAMaddr;	
	ADDR				addrXTRAMdata;	
	ADDR				addrXTRAMaddr;	
	ADDR				sizeGeneral;	
	ADDR				sizeTRAMdata;	
	ADDR				sizeXTRAMdata;	
	INDEX				instrLoc;		
	INDEX				tramLoc;		
	INDEX				xtramLoc;		
	INDEX				tableLoc;		
	INDEX				interpLoc;		
	ADDR				nScratch;		
	PARTITIONINFOSTRUCT *pPartitionList;
	BYTE				flagMap[FLAGMAPSIZE];
	ULONG				ulChipHandle;
	FXID				interpolatorID;
	ULONG				ulInMask;
	ULONG				ulOutMask;
	char				*chipRevisionID;
	ULONG				ulRevReg;
	BOOL				inuse;
} CHIPINFOSTRUCT;

/* @struct MAPSTRUCT |
 * These structuress are contained in the <t memMap[]> array and contain
 * information about the various contiguous resources.
 *
 * @field ULONG	| ulSize	| Contains size of block.
 * @field ULONG	| ulAddr	| Contains start address of block.
 * @field FXID	| ownerID	| Contains FXID of owner of block.
 * @field INDEX	| prevSeg	| Index to previous contiguous block.
 * @field INDEX	| nextSeg	| Index to next contiguous block.
 */
typedef struct {
	ULONG				ulSize;		
	ULONG				ulAddr;		
	FXID				ownerID;	
	INDEX				prevSeg;	
	INDEX				nextSeg;	
} MAPSTRUCT;

/* @struct ChipRev[] |
 * This array contains available resources for the various
 * revisions of the FX8010 chip.
 *
 * @field char *	| chipRevisionID		| Contains the chip revision information.
 * @field ADDR		| addrGeneral			| Contains start address of general GPRs in the GPR space.
 * @field ADDR		| sizeGeneral			| Contains number of general GPRs.
 * @field ADDR		| addrTRAMdata			| Contains start address of TRAM data GPRs in the GPR space.
 * @field ADDR		| sizeTRAMdata			| Contains number of TRAM buffers.
 * @field ADDR		| addrTRAMaddr			| Contains start address of TRAM address GPRs in the GPR space.
 * @field ADDR		| addrXTRAMdata			| Contains start address of XTRAM data GPRs in the GPR space.
 * @field ADDR		| sizeXTRAMdata			| Contains number of XTRAM buffers.
 * @field ADDR		| addrXTRAMaddr			| Contains start address of XTRAM address GPRs in the GPR space.
 * @field ADDR		| instrSpace			| Contains number if instruction steps.
 * @field ULONG		| TRAMaddrSpace			| Contains size of internal TRAM.
 * @field BYTE		| flagMap[FLAGMAPSIZE]	| Contains a enumeration to physical TRAM engine flag mapping.
 */
static struct {
	char				*chipRevisionID;
	ADDR				addrGeneral;
	ADDR				sizeGeneral;
	ADDR				addrTRAMdata;
	ADDR				sizeTRAMdata;
	ADDR				addrTRAMaddr;
	ADDR				addrXTRAMdata;
	ADDR				sizeXTRAMdata;
	ADDR				addrXTRAMaddr;
	ADDR				instrSpace;
	ULONG				TRAMaddrSpace;
	BYTE				flagMap[FLAGMAPSIZE];
} ChipRevs[] = { 
	{
		"EMU8010_A",
		0x100,	0x100,			/* General GPR space */
		0x200,	0x080,			/* TRAM data buffer GPR space */
		0x300,					/* TRAM address buffer GPR start */
		0x280,	0x020,			/* XTRAM data buffer GPR space */
		0x380,					/* XTRAM address buffer GPR start */
		0x200,					/* Instruction space size */
		0x2000L,				/* Internal TRAM space size */
		{ 0x9, 0x2, 0xb, 0x0, 0x0, 0x1, 0, 0,
		  0, 0, 0, 0, 0, 0, 0, 0 }
	},{
		"RCHIP_A",
		0x060,	0x1A0,			/* General GPR space */
		0x200,	0x100,			/* TRAM data buffer GPR space */
		0x300,					/* TRAM address buffer GPR start */
		0x000,	0x000,			/* XTRAM data buffer GPR space */
		0x000,					/* XTRAM address buffer GPR start */
		0x400,					/* Instruction space size */
		0x2000L,				/* Internal TRAM space size */
		{ 0, 0, 0, 0, 0, 0, 0, 0,
		  0, 0, 0, 0, 0, 0, 0, 0 }
	},{
		NULL,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0L, 
		{ 0, 0, 0, 0, 0, 0, 0, 0, 
		  0, 0, 0, 0, 0, 0, 0, 0 }
	}
};

/* Currently (5/6/97) IDs refer to the location of the respective
 * info structure.  If this were to change, these macros need to
 * be redefined in order to convert from and FXID <--> info struct
 * pointer.
 */
#if VALIDATE_PARTITIONID
	static PARTITIONINFOSTRUCT *fxpad[FXMAX_CHIPS*FXMAX_PARTITIONS+1];

	static FXID fxnewpad(PARTITIONINFOSTRUCT *p) { 
		int fapindex;
		for(fapindex=1;fxpad[fapindex];fapindex++);
		fxpad[fapindex] = p;
		return ((FXID)((USHORT)fapindex+0x8000));
	}
	static FXID getpaid_(PARTITIONINFOSTRUCT *p) {
		int fapindex;
		for(fapindex=1;fxpad[fapindex]!=p;fapindex++);
		return ((FXID)((USHORT)fapindex+0x8000));
	}
	#define ID2PAI(id)	((((USHORT)id)&XMASK) > (FXMAX_CHIPS*FXMAX_PARTITIONS+1) ? \
						NULL : fxpad[(((USHORT)id)&XMASK)])
	#define PAI2ID(p)	fxnewpad(p)
	#define GETPAID(p)	getpaid_(p)
	#define INVPAD(id)	fxpad[((USHORT)id)&XMASK] = NULL;

#else
	#define ID2PAI(id) (PARTITIONINFOSTRUCT *)(id)
	#define PAI2ID(pInfo) (FXID)(pInfo)
	#define GETPAID(pInfo)	(FXID)(pInfo)
	#define INVPAD(id)	;
#endif

#if VALIDATE_CHIPID
	static CHIPINFOSTRUCT *fxcid[FXMAX_CHIPS+1];
	static int fcindex;

	static FXID fxnewpc(CHIPINFOSTRUCT *p) { 
		for(fcindex=1;fxcid[fcindex];fcindex++);
		fxcid[fcindex] = p;
		return ((FXID)((USHORT)fcindex+0x2000));
	}
	static FXID fxgetcid_(CHIPINFOSTRUCT *p) {
		int fapindex;
		for(fapindex=1;fxcid[fapindex]!=p;fapindex++);
		return ((FXID)((USHORT)fapindex+0x2000));
	}
	#define ID2CI(id)	((((USHORT)id)&XMASK) >(FXMAX_CHIPS+1)? \
								NULL : fxcid[((USHORT)id)&XMASK])
	#define CI2ID(p)	fxnewpc(p)
	#define GETCID(p)	fxgetcid_(p)
	#define INVCI(id)	fxcid[((USHORT)id)&XMASK] = NULL;

#else
	#define ID2CI(id) (CHIPINFOSTRUCT *)(id)
	#define CI2ID(pInfo) (FXID)(pInfo)
	#define GETCID(pInfo) (FXID)(pInfo)
	#define INVCI(id)	;
#endif

#define ID2PRI(id) (PGMINFOSTRUCT *)(id)
#define PRI2ID(pInfo) (FXID)(pInfo)

static INDEX				freeInodes;
static INDEX				nextInode;
static MAPSTRUCT			memMap[MAX_INODES];
#define MAPNIL				MAX_INODES

int							fxnextChipNo = 0;
static CHIPINFOSTRUCT		ChipInfo[FXMAX_CHIPS];

#if !FX_DYNAMIC && !FXPSEUDO_DYNAMIC
static PARTITIONINFOSTRUCT	PartitionInfo[FXMAX_CHIPS*FXMAX_PARTITIONS];
static PGMINFOSTRUCT		PgmInfo[FXMAX_CHIPS*FXMAX_PROGRAMS];
#endif

static PARTITIONINFOSTRUCT	*pFreePartitionList=NULL;
static PGMINFOSTRUCT		*pFreePgmList=NULL;

/* Static prototypes */

static INDEX    mapCreate( FXID, ULONG, BOOL );
static FXSTATUS allocGPRs(FXID,FXID,FXID,GPRTYPE,int,BOOL,ADDR,ADDR);
static FXSTATUS allocMap( INDEX *, ULONG, FXID, FXID, INDEX * );
static FXSTATUS deallocGPRs( FXID, FXID, FXID );
static FXSTATUS deallocMap( INDEX *, INDEX, FXID );
static INDEX    findLargestMap( INDEX, FXID, ULONG, int * );
static ADDR		countGPRs( FXID, GPRTYPE, FXID );

extern ULONG fx8210Mutex;

/*****************************************************************
* 
* Function:		fxRsrcInitialize()
*
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxRsrcInitialize()
{
	INDEX i;

	/* Init MAPSTRUCT array */
	for( i=0; i<MAX_INODES; i++ ) {
		memMap[i].ownerID = UNALLOCED_ID;
	}
	freeInodes = MAX_INODES;
	nextInode = 0;

#if !FX_DYNAMIC
#if FXPSEUDO_DYNAMIC
	pFreePartitionList = NULL;
	pFreePgmList = NULL;
#else
	/* Init PartitionInfo array */
	for( i=0; i<FXMAX_CHIPS*FXMAX_PARTITIONS-1; i++ ) {
		PartitionInfo[i].pChain = &(PartitionInfo[i+1]);
	}
	PartitionInfo[i].pChain = NULL;
	pFreePartitionList = &(PartitionInfo[0]);

	/* Init PgmInfo array */
	for( i=0; i<FXMAX_CHIPS*FXMAX_PROGRAMS-1; i++ ) {
		PgmInfo[i].pChain = &(PgmInfo[i+1]);
	}
	PgmInfo[i].pChain = NULL;
	pFreePgmList = &(PgmInfo[0]);
#endif
#endif

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcInitChip()
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxRsrcInitChip( FXID *chipID, char *chipRevisionID, ULONG ulRevReg, ULONG ulChipHandle, 
			    ULONG ulXTRAMSize )
{
    USHORT i,j;
	ADDR a;
	CHIPINFOSTRUCT *pChipInfo;
	BYTE bZeroFlag;

	OS_WAITMUTEX(fx8210Mutex);

	*chipID = (FXID)0;
	
	/* Locate chip in revision structure */
	i = 0;
	while( ChipRevs[i].chipRevisionID ) {
		if( !strcmp( ChipRevs[i].chipRevisionID, chipRevisionID ) )
			break;
		i++;
	}
	if( !(ChipRevs[i].chipRevisionID) ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	/* Is there room for another chip? */
	if( fxnextChipNo > FXMAX_CHIPS ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_OUT_OF_MEMORY;
	}

#if !FX_DYNAMIC && FXPSEUDO_DYNAMIC
	{
	PARTITIONINFOSTRUCT newblock[];
	PGMINFOSTRUCT		pgmblock[];
	
	newblock = (PARTITIONINFOSTRUCT *) 
		OS_MALLOC( sizeof( PARTITIONINFOSTRUCT ) * FXMAX_PARTITIONS );
	if( !newblock ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_OUT_OF_MEMORY;
	}
	for( i=0; i<FXMAX_PARTITIONS-1; i++ ) {
		newblock[i].pChain = &(newblock[i+1]);
	}
	newblock[i].pChain = pFreePartitionList;
	pFreePartitionList = &(newblock[0]);
	
	pgmblock = (PGMINFOSTRUCT *) 
		OS_MALLOC( sizeof(PGMINFOSTRUCT) * FXMAX_PROGRAMS );
	if( !pgmblock ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_OUT_OF_MEMORY;
	}
	for( i=0; i<FXMAX_PROGRAMS-1; i++ ) {
		pgmblock[i].pChain = &(pgmblock[i+1]);
	}
	pgmblock[i].pChain = pFreePgmList;
	pFreePgmList = &(pgmblock[0]);
#endif
	
	for( j=0; j<FXMAX_CHIPS; j++ ) {
		if( !(ChipInfo[j].inuse) ) break;
	}
	pChipInfo = &(ChipInfo[j]);

	/* Set all GPRs as unallocated */
	for( a = 0x000; a < MAX_GPRS; a++ ) {
		pChipInfo->gprAlloc[a] = UNALLOCED_ID;
	}

	pChipInfo->ulChipHandle = ulChipHandle;
	pChipInfo->interpolatorID = UNALLOCED_ID;

	/* Clear virtual GPR spaces */
	pChipInfo->gprRelocTop = 0x000;
	pChipInfo->gprSpace    = 0x000;

	/* Set GPR information */
	pChipInfo->sizeGeneral  = ChipRevs[i].sizeGeneral;
	pChipInfo->addrGeneral  = ChipRevs[i].addrGeneral;
	pChipInfo->gprSpace	   += pChipInfo->sizeGeneral;

	pChipInfo->sizeTRAMdata  = ChipRevs[i].sizeTRAMdata;
	pChipInfo->addrTRAMdata  = ChipRevs[i].addrTRAMdata;
	pChipInfo->addrTRAMaddr  = ChipRevs[i].addrTRAMaddr;
	pChipInfo->gprSpace	    += ((pChipInfo->sizeTRAMdata)<<1);

	pChipInfo->sizeXTRAMdata  = ChipRevs[i].sizeXTRAMdata;
	pChipInfo->addrXTRAMdata  = ChipRevs[i].addrXTRAMdata;
	pChipInfo->addrXTRAMaddr  = ChipRevs[i].addrXTRAMaddr;
	pChipInfo->gprSpace	     += ((pChipInfo->sizeXTRAMdata)<<1);

	pChipInfo->chipRevisionID = ChipRevs[i].chipRevisionID;
	pChipInfo->ulRevReg       = ulRevReg;

	/* Make sure there are enough inodes for instruction space,
	 * TRAM space, XTRAM space, and table RAM space.
	 */
	if( freeInodes < 4 ) {
 	  	 OS_RELEASEMUTEX(fx8210Mutex);
		 return FXERROR_OUT_OF_MEMORY;
	}

	/* Set new chip FXID */
	*chipID = CI2ID( pChipInfo );
	fxnextChipNo++;

	/* Create instruction, TRAM, and XTRAM blocks */
	pChipInfo->instrLoc = mapCreate( *chipID, 
									 ChipRevs[i].instrSpace, FALSE );

	pChipInfo->tramLoc = mapCreate( *chipID,
									 ChipRevs[i].TRAMaddrSpace, CIRC );

	pChipInfo->xtramLoc = mapCreate( *chipID, ulXTRAMSize, CIRC );

	/* Map space will be set in fxRsrcAllocChip() */
	pChipInfo->tableLoc = MAPNIL;

	/* Interpolator space will be set in fxRsrcAllocChip() */
	pChipInfo->interpLoc = MAPNIL;
	
	/* No partitions */
	pChipInfo->pPartitionList = NULL;

	/* Scratch registers will be set in fxRsrcAllocChip() */
	pChipInfo->nScratch = 0;

	/* Copy flagMap */
	for(j=0; j<FLAGMAPSIZE; j++ ) 
		pChipInfo->flagMap[j] = ChipRevs[i].flagMap[j];

	/* Get the value of the "off" mode */
	bZeroFlag = ((ID2CI(*chipID))->flagMap[MODE_OFF])<<4;

	/* Clear all X/TRAM address buffers and flags */
	for( j=ChipRevs[i].addrTRAMaddr; 
		 j<ChipRevs[i].addrTRAMaddr + ChipRevs[i].sizeTRAMdata;
		 j++ ) {
		OS_WRITEGPR( ulChipHandle, j, 0 );
		OS_WRITEFLAGS( ulChipHandle, j, bZeroFlag );
	}

	for( j=ChipRevs[i].addrXTRAMaddr; 
		 j<ChipRevs[i].addrXTRAMaddr + ChipRevs[i].sizeXTRAMdata;
		 j++ ) {
		OS_WRITEGPR( ulChipHandle, j, 0 );
		OS_WRITEFLAGS( ulChipHandle, j, bZeroFlag );
	}

	pChipInfo->inuse = TRUE;

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcGetChipList()
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcGetChipList( FXCHIPID chipID[], DWORD sizeArray, DWORD *numChips )
{
	DWORD i;

	OS_WAITMUTEX(fx8210Mutex);

	*numChips = (DWORD) fxnextChipNo;

	for( i = 0; i < *numChips && i < sizeArray; i++ ) {
		chipID[i] = (FXCHIPID) GETCID(&(ChipInfo[i]));	
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcFreeChip()
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxRsrcFreeChip( FXID chipID )
{
	CHIPINFOSTRUCT *pChipInfo;

	pChipInfo = ID2CI(chipID);
	pChipInfo->inuse = FALSE;

	fxnextChipNo--;

	INVCI(chipID);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcAllocChip( chipID, ulTableRAM, nScratch, nInterpolators )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcAllocChip( FXCHIPID chipID, ULONG ulTableSize, ADDR nScratch, ADDR nInterpolators )
{
	CHIPINFOSTRUCT *pChipInfo;
	PARTITIONRSRC   prsrc = { 0, 0, 0, 0, 0L, 0L, 0L };
	FXSTATUS		status;
    USHORT          i, j;

	OS_WAITMUTEX(fx8210Mutex);

	pChipInfo = ID2CI( (FXID)chipID );
	if( !pChipInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	/* If the chip is partitioned or table space has already been
	 * allocated, fail this call.
	 */
	if( pChipInfo->pPartitionList || pChipInfo->tableLoc != MAPNIL) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_CHIP_IN_USE;
	}

	/* If the chip has an interpolator FXID, fail this call. */
	if( pChipInfo->interpolatorID != UNALLOCED_ID ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_CHIP_IN_USE;
	}

	/* If there isn't any TRAM, there can't be any table RAM, can
	 * there?
	 */
	if( pChipInfo->tramLoc == MAPNIL )  {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_OUT_OF_TABLE_RAM;
	}

	/* If more scratch registers and interpolators are being requested than there
	 * are general GPRs, return an error.
	 */
#if VARIABLE_RAMPER
	if( nScratch + nInterpolators*2 > pChipInfo->sizeGeneral ) 
#else
	if( nScratch + nInterpolators > pChipInfo->sizeGeneral )
#endif
	{
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_OUT_OF_GPR_SPACE;
	}

	/* If more interpolators are being requested than there are
	 * instructions, return an error.
	 */
	if( (ULONG) nInterpolators > memMap[pChipInfo->instrLoc].ulSize ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_OUT_OF_INSTRUCTION_SPACE;
	}

	/* If table space is being requested ... */
	if( ulTableSize > 0 ) {
		
		/* If table size is greater than TRAM space, error */
		if( ulTableSize > memMap[pChipInfo->tramLoc].ulSize ) {
			OS_RELEASEMUTEX(fx8210Mutex);
			return FXERROR_OUT_OF_TABLE_RAM;
		}

		/* If table size is exactly equal to TRAM space, just give
		 * entire block to table RAM
		 */
		if( ulTableSize == memMap[pChipInfo->tramLoc].ulSize ) {
			pChipInfo->tableLoc = pChipInfo->tramLoc;
			pChipInfo->tramLoc = MAPNIL;
		} else {
			/* Get an inode */
			if( !freeInodes ) {
				OS_RELEASEMUTEX(fx8210Mutex);
				return FXERROR_OUT_OF_MEMORY;
			}
			
			/* Decrease TRAM size by table size */
			memMap[pChipInfo->tableLoc].ulSize -= ulTableSize;

			/* Create a table RAM block */
			pChipInfo->tableLoc = 
				mapCreate( chipID, ulTableSize, FALSE );

			/* Set the address to the end of TRAM address space */
			memMap[pChipInfo->tableLoc].ulAddr = 
				memMap[pChipInfo->tramLoc].ulSize;
		}
	}

	/* If interpolators are being requested ... */
	if( nInterpolators > 0 ) {

		/* Grab the first consecutive nInterpolators*2 general
		 * GPRs and stuff the chipID in the gprAlloc[] array.
		 */
#if VARIABLE_RAMPER
		for( j=0, i = pChipInfo->addrGeneral; j < nInterpolators*2;
			++j, ++i ) {
#else
		for( j=0, i = pChipInfo->addrGeneral; j < nInterpolators;
			++j, ++i ) {
#endif
			pChipInfo->gprAlloc[i] = chipID;
		}

		/* Grab nInterpolators consecutive instructions */
		status = allocMap( &(pChipInfo->instrLoc), 
					  nInterpolators, 
					  INTERPOLATORID, 
					  (FXID)chipID, 
					  &(pChipInfo->interpLoc) );
		if( status != FXERROR_NO_ERROR ) {
			OS_RELEASEMUTEX(fx8210Mutex);
			return status;
		}
		/* Note: Only possible error is not enough inodes, in which
		 * case there is no point in deallocating for cleanup, 'cause
		 * we have bigger fish to fry.
		 */

		status = fxParamAllocInterpolators( &(pChipInfo->interpolatorID),
			nInterpolators,	pChipInfo->addrGeneral,
			(ADDR) memMap[pChipInfo->interpLoc].ulAddr );
		if( status != FXERROR_NO_ERROR ) {
			OS_RELEASEMUTEX(fx8210Mutex);
			return status;
		}
		/* same story, except error is out of memory */

	}
		
	/* If scratch registers are being requested ... */
	if( nScratch > 0 ) {

		/* Set up a virtual address space for scratch registers,
		 * set their owner as the chip.
		 */
		allocGPRs( (FXID)chipID, (FXID)chipID, UNALLOCED_ID, GPR_GENERAL, 
			nScratch, TRUE, pChipInfo->gprRelocTop, 0x000 );

		/* Adjust Reloc table pointer */
		pChipInfo->gprRelocTop += nScratch;

		pChipInfo->nScratch = nScratch;
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;

}

/*****************************************************************
* 
* @func static INDEX | mapCreate |
*	
* Creates a new contiguous space block in <t memMap[]>.
*
* @parm	FXID	| chipID	| Specifies FXID of chip.
* @parm ULONG	| ulSize	| Specifies size of contiguous space.
* @parm BOOL	| fCirc		| Specifies whether to link circularly.
*
* @comm This is a static function
*
* This function assumes that a free inode exists.
*
* @rdesc Returns index of created block.
*
******************************************************************
*/
static INDEX
mapCreate( FXID chipID, ULONG ulSize, BOOL fCirc )
{
	/* Find next free inode */
	if( nextInode >= MAX_INODES ) nextInode = 0;

	while( memMap[nextInode].ownerID != UNALLOCED_ID ) {
		if( ++nextInode >= MAX_INODES ) nextInode = 0;
	}

	/* Set information */
	memMap[nextInode].ownerID = chipID;
	memMap[nextInode].ulSize  = ulSize;
	memMap[nextInode].ulAddr    = 0L;

	/* If the list is to be circular, wrap pointers, otherwise
	 * set pointers to MAPNIL
	 */
	if( fCirc ) {
		memMap[nextInode].prevSeg = nextInode;
		memMap[nextInode].nextSeg = nextInode;
	} else {
		memMap[nextInode].prevSeg = MAPNIL;
		memMap[nextInode].nextSeg = MAPNIL;
	}

	freeInodes--;
	return nextInode++;
}

/*****************************************************************
* 
* Function:		fxRsrcNewPartition( chipID, partitionID )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcNewPartition( FXCHIPID chipID, FXPARTID *partitionID ) 
{
	PARTITIONINFOSTRUCT *pPartitionInfo;
	CHIPINFOSTRUCT      *pChipInfo;

	OS_WAITMUTEX(fx8210Mutex);

	pChipInfo = ID2CI( (FXID)chipID );
	if( !pChipInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

#if FX_DYNAMIC	
	pFreePartitionList = (PARTITIONINFOSTRUCT *) 
						 OS_MALLOC( sizeof( PARTITIONINFOSTRUCT ) );
#endif

	if( !pFreePartitionList ) return FXERROR_OUT_OF_MEMORY;
	pPartitionInfo = pFreePartitionList;

#if !FX_DYNAMIC
	pFreePartitionList = pFreePartitionList->pChain;
#endif

	/* Init all fields */
	pPartitionInfo->instrLoc = MAPNIL;
	pPartitionInfo->tramLoc = MAPNIL;
	pPartitionInfo->xtramLoc = MAPNIL;
	pPartitionInfo->tableLoc = MAPNIL;
	pPartitionInfo->chipID = chipID;
	pPartitionInfo->pPgmList = NULL;
	pPartitionInfo->pChain = pChipInfo->pPartitionList;
	pChipInfo->pPartitionList = pPartitionInfo;

	*partitionID = (FXPARTID)PAI2ID( pPartitionInfo );

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcNewPgm( partitionID, rsrcID )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxRsrcNewPgm( FXID partitionID, FXID *rsrcID ) 
{
	PARTITIONINFOSTRUCT *pPartitionInfo;
	PGMINFOSTRUCT       *pPgmInfo;

	pPartitionInfo = ID2PAI( partitionID );
	if( !pPartitionInfo ) return FXERROR_INVALID_ID;

#if FX_DYNAMIC	
	pFreePgmList = (PGMINFOSTRUCT *) 
					OS_MALLOC( sizeof( PGMINFOSTRUCT ) );
#endif

	if( !pFreePgmList ) return FXERROR_OUT_OF_MEMORY;
	pPgmInfo = pFreePgmList;

#if !FX_DYNAMIC
	pFreePgmList = pFreePgmList->pChain;
#endif

	/* init all fields */
	pPgmInfo->instrLoc = MAPNIL;
	pPgmInfo->tramLoc = MAPNIL;
	pPgmInfo->xtramLoc = MAPNIL;
	pPgmInfo->tableLoc = MAPNIL;
	pPgmInfo->chipID = pPartitionInfo->chipID;
	pPgmInfo->partitionID = partitionID;
	pPgmInfo->pChain = pPartitionInfo->pPgmList;
	pPartitionInfo->pPgmList = pPgmInfo;

	*rsrcID = PRI2ID( pPgmInfo );

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcAllocPartition( partitionID, pPartitionRsrc )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcAllocPartition( FXPARTID partitionID, PARTITIONRSRC *pPartitionRsrc )
{
	FXSTATUS status;
	PARTITIONINFOSTRUCT *pPartitionInfo;
	CHIPINFOSTRUCT *pChipInfo;

	OS_WAITMUTEX(fx8210Mutex);

	pPartitionInfo = ID2PAI((FXID)partitionID);
	if( !pPartitionInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	pChipInfo = ID2CI(pPartitionInfo->chipID);

	/* Allocate non-contiguous resources */

	/* Allocate GPRs for partition */
	status = allocGPRs( (FXID)partitionID, 
						pPartitionInfo->chipID,
						UNALLOCED_ID,
						GPR_GENERAL,
						pPartitionRsrc->nGeneralGPRs,
						FALSE, 0, 0 );
	if( status == FXERROR_OUT_OF_GPR_SPACE ) 
		status =  FXERROR_OUT_OF_GENERAL_GPRS;

	if( status == FXERROR_NO_ERROR ) {

		status = allocGPRs( (FXID)partitionID, 
						pPartitionInfo->chipID,
						UNALLOCED_ID,
						GPR_TRAM,
						pPartitionRsrc->nTRAMGPRs,
						FALSE, 0, 0 );
		if( status == FXERROR_OUT_OF_GPR_SPACE ) 
			status =  FXERROR_OUT_OF_TRAM_GPRS;
	}

	if( status == FXERROR_NO_ERROR ) {

		status = allocGPRs( (FXID)partitionID, 
						pPartitionInfo->chipID,
						UNALLOCED_ID,
						GPR_XTRAM,
						pPartitionRsrc->nXTRAMGPRs,
						FALSE, 0, 0 );
		if( status == FXERROR_OUT_OF_GPR_SPACE ) 
			status =  FXERROR_OUT_OF_XTRAM_GPRS;
	}

	/* Allocate Contiguous resources */

	/* Allocate instruction space */
	if( status == FXERROR_NO_ERROR ) {
		status = allocMap( &(pChipInfo->instrLoc), 
					   (ULONG)pPartitionRsrc->instrSpace, 
					   (FXID)partitionID, 
					   pPartitionInfo->chipID, 
					   &(pPartitionInfo->instrLoc) );

		if( status == FXERROR_NO_CONTIGUOUS_SPACE )
			status =  FXERROR_OUT_OF_INSTR_SPACE;
	}

	/* Allocate TRAM space */
	if( status == FXERROR_NO_ERROR ) {
		status = allocMap( &(pChipInfo->tramLoc), 
					   pPartitionRsrc->tramSpace, 
					   (FXID)partitionID, 
					   pPartitionInfo->chipID, 
					   &(pPartitionInfo->tramLoc) );
		if( status == FXERROR_NO_CONTIGUOUS_SPACE )
			status =  FXERROR_OUT_OF_TRAM_SPACE;
	}

	/* Allocate XTRAM space */
	if( status == FXERROR_NO_ERROR ) {
		status = allocMap( &(pChipInfo->xtramLoc), 
					   pPartitionRsrc->xtramSpace, 
					   (FXID)partitionID, 
					   pPartitionInfo->chipID, 
					   &(pPartitionInfo->xtramLoc) );
		if( status == FXERROR_NO_CONTIGUOUS_SPACE )
			status =  FXERROR_OUT_OF_XTRAM_SPACE;
	}

	/* Allocate Table RAM space */
	if( status == FXERROR_NO_ERROR ) {
		status = allocMap( &(pChipInfo->tableLoc), 
					   pPartitionRsrc->tableSpace, 
					   (FXID)partitionID, 
					   pPartitionInfo->chipID, 
					   &(pPartitionInfo->tableLoc) );
		if( status == FXERROR_NO_CONTIGUOUS_SPACE )
			status =  FXERROR_OUT_OF_TABLE_SPACE;
	}

	if( status != FXERROR_NO_ERROR ) {
		fxRsrcReleasePartition( partitionID );
	}

	OS_RELEASEMUTEX(fx8210Mutex);

	return status;
}

/*****************************************************************
* 
* Function:		fxRsrcGetPartitionList( )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcGetPartitionList( FXCHIPID chipID, FXPARTID partitionID[], DWORD sizeArray, DWORD *numParts )
{
	CHIPINFOSTRUCT *pChipInfo;
	PARTITIONINFOSTRUCT *pPartitionInfo;
	DWORD i=0;
	
	OS_WAITMUTEX(fx8210Mutex);

	pChipInfo = ID2CI( (FXID)chipID );
	if( !pChipInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	pPartitionInfo = pChipInfo->pPartitionList;
	while( pPartitionInfo && i < sizeArray ) {
		partitionID[i++] = (FXPARTID)GETPAID(pPartitionInfo);
		pPartitionInfo = pPartitionInfo->pChain;
	}
	*numParts = i;
	
	OS_RELEASEMUTEX(fx8210Mutex);
	
	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcReleasePartition( partitionID )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcReleasePartition( FXPARTID partitionID )
{
	PARTITIONINFOSTRUCT *pPartitionInfo;
	CHIPINFOSTRUCT *pChipInfo;

	OS_WAITMUTEX(fx8210Mutex);

	pPartitionInfo = ID2PAI((FXID)partitionID);
	if( !pPartitionInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	pChipInfo = ID2CI(pPartitionInfo->chipID);

	/* If there are programs still existing in this partition,
	 * return an error.  User must dealloc all programs first!
	 */
	if( pPartitionInfo->pPgmList != NULL ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_PARTITION_NOT_EMPTY;
	}

	/* Blow away any allocated resources! */
	deallocGPRs( (FXID)partitionID, pPartitionInfo->chipID, UNALLOCED_ID );
	deallocMap( &(pChipInfo->instrLoc),
				pPartitionInfo->instrLoc, 
				pPartitionInfo->chipID );
	deallocMap( &(pChipInfo->tramLoc),
				pPartitionInfo->tramLoc, 
				pPartitionInfo->chipID );
	deallocMap( &(pChipInfo->xtramLoc),
				pPartitionInfo->xtramLoc, 
				pPartitionInfo->chipID );
	deallocMap( &(pChipInfo->tableLoc),
				pPartitionInfo->tableLoc, 
				pPartitionInfo->chipID );

	pPartitionInfo->instrLoc = MAPNIL;
	pPartitionInfo->tramLoc = MAPNIL;
	pPartitionInfo->xtramLoc = MAPNIL;
	pPartitionInfo->tableLoc = MAPNIL;

	OS_RELEASEMUTEX(fx8210Mutex);
	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcDeletePartition( partitionID )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcDeletePartition( FXPARTID partitionID )
{
	PARTITIONINFOSTRUCT *pPartitionInfo, *pPartition;
	CHIPINFOSTRUCT *pChipInfo;
	
	OS_WAITMUTEX(fx8210Mutex);

	pPartitionInfo = ID2PAI((FXID)partitionID);
	if( !pPartitionInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	pChipInfo = ID2CI(pPartitionInfo->chipID);

	/* Search for partition in chip's list */
	for( pPartition = pChipInfo->pPartitionList;
		 pPartition && pPartition != pPartitionInfo && 
			 pPartition->pChain != pPartitionInfo;
		 pPartition = pPartition->pChain );

	if( !pPartition ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		 return FXERROR_INVALID_ID;
	}

	/* patch chain */
	if( pPartition == pPartitionInfo ) {
		pChipInfo->pPartitionList = pPartitionInfo->pChain;
	} else {
		pPartition->pChain = pPartitionInfo->pChain;
	}

	/* Free memory according to allocation model */
#if FX_DYNAMIC
	OS_FREE( pPartitionInfo );
#else
	pPartitionInfo->pChain = pFreePartitionList;
	pFreePartitionList     = pPartitionInfo;
#endif

	/* Invalidate ID */
	INVPAD((FXID)partitionID);

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcAllocPgm( rsrcID, pPgmRsrc )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxRsrcAllocPgm( FXID rsrcID, PGMRSRC *pPgmRsrc )
{
	PGMINFOSTRUCT  *pPgmInfo;
	CHIPINFOSTRUCT *pChipInfo;
	PARTITIONINFOSTRUCT *pPartitionInfo;
	ADDR			addr;
	int 			nOvScratch;
	FXSTATUS		status;

	pPgmInfo =  ID2PRI(rsrcID);
	pPartitionInfo = ID2PAI(pPgmInfo->partitionID );
	pChipInfo = ID2CI(pPgmInfo->chipID);

	/* allocate GPRs */
	nOvScratch = pPgmRsrc->nScratch - pChipInfo->nScratch;
	if( nOvScratch < 0 ) nOvScratch = 0;

	pPgmInfo->gprSpace = pPgmRsrc->nOutputs  + pPgmRsrc->nGeneral + 
						 (pPgmRsrc->nTRAMbuf)*2 +  
						 (pPgmRsrc->nXTRAMbuf)*2 +
						 nOvScratch;
	pPgmInfo->nGeneral = pPgmRsrc->nOutputs + pPgmRsrc->nGeneral + nOvScratch;
	pPgmInfo->nTRAMbuf = pPgmRsrc->nTRAMbuf;
	pPgmInfo->nXTRAMbuf = pPgmRsrc->nXTRAMbuf;
	if( pPgmInfo->gprSpace > 
		pChipInfo->gprSpace - pChipInfo->gprRelocTop ) {
		return FXERROR_OUT_OF_GPR_SPACE;
	}

	addr = 0x000;
	status = allocGPRs( rsrcID,
						pPgmInfo->chipID,
						pPgmInfo->partitionID,
						GPR_GENERAL,
						pPgmRsrc->nOutputs + pPgmRsrc->nGeneral,
						TRUE,
						pChipInfo->gprRelocTop,
						addr );
	if( status == FXERROR_OUT_OF_GPR_SPACE ) 
		status =  FXERROR_OUT_OF_GENERAL_GPRS;
	
	if( status == FXERROR_NO_ERROR ) {
		addr += pPgmRsrc->nOutputs + pPgmRsrc->nGeneral;
		status = allocGPRs( rsrcID,
						pPgmInfo->chipID,
						pPgmInfo->partitionID,
						GPR_TRAM,
						pPgmRsrc->nTRAMbuf,
						TRUE,
						pChipInfo->gprRelocTop,
						addr );
		if( status == FXERROR_OUT_OF_GPR_SPACE ) 
			status =  FXERROR_OUT_OF_TRAM_GPRS;
	}

	if( status == FXERROR_NO_ERROR ) {
		addr += (pPgmRsrc->nTRAMbuf)<<1;
		status = allocGPRs( rsrcID,
						pPgmInfo->chipID,
						pPgmInfo->partitionID,
						GPR_XTRAM,
						pPgmRsrc->nXTRAMbuf,
						TRUE,
						pChipInfo->gprRelocTop,
						addr );
		if( status == FXERROR_OUT_OF_GPR_SPACE ) 
			status =  FXERROR_OUT_OF_XTRAM_GPRS;
	}

	if( status == FXERROR_NO_ERROR ) {
		addr += (pPgmRsrc->nXTRAMbuf)<<1;
		status = allocGPRs( rsrcID,
						pPgmInfo->chipID,
						pPgmInfo->partitionID,
						GPR_GENERAL,
						nOvScratch,
						TRUE,
						pChipInfo->gprRelocTop,
						addr );
		if( status == FXERROR_OUT_OF_GPR_SPACE ) 
			status =  FXERROR_OUT_OF_GENERAL_GPRS;
	}

	pPgmInfo->gprBaseReloc = pChipInfo->gprRelocTop;

	if( status == FXERROR_NO_ERROR ) {
		pChipInfo->gprRelocTop += pPgmInfo->gprSpace;
	}

	/* Allocate contiguous resources */

	/* Allocate instruction memory */
	if( status == FXERROR_NO_ERROR ) {
		status = allocMap( &(pPartitionInfo->instrLoc), 
					   (ULONG)pPgmRsrc->instrSpace, 
					   rsrcID, 
					   pPgmInfo->partitionID, 
					   &(pPgmInfo->instrLoc) );
		if( status == FXERROR_NO_CONTIGUOUS_SPACE )
			status =  FXERROR_OUT_OF_INSTR_SPACE;
	}

	/* Allocate TRAM space */
	if( status == FXERROR_NO_ERROR ) {
		status = allocMap( &(pPartitionInfo->tramLoc), 
					   pPgmRsrc->tramSpace, 
					   rsrcID, 
					   pPgmInfo->partitionID, 
					   &(pPgmInfo->tramLoc) );
		if( status == FXERROR_NO_CONTIGUOUS_SPACE )
			status =  FXERROR_OUT_OF_TRAM_SPACE;
	}

	/* Allocate XTRAM space */
	if( status == FXERROR_NO_ERROR ) {
		status = allocMap( &(pPartitionInfo->xtramLoc), 
					   pPgmRsrc->xtramSpace, 
					   rsrcID, 
					   pPgmInfo->partitionID, 
					   &(pPgmInfo->xtramLoc) );
		if( status == FXERROR_NO_CONTIGUOUS_SPACE )
			status =  FXERROR_OUT_OF_XTRAM_SPACE;
	}

	/* Allocate Table RAM space */
	if( status == FXERROR_NO_ERROR ) {
		status = allocMap( &(pPartitionInfo->tableLoc), 
					   pPgmRsrc->tableSpace, 
					   rsrcID, 
					   pPgmInfo->partitionID, 
					   &(pPgmInfo->tableLoc) );
		if( status == FXERROR_NO_CONTIGUOUS_SPACE )
			status =  FXERROR_OUT_OF_TABLE_SPACE;
	}

	if( status != FXERROR_NO_ERROR ) {
		fxRsrcReleasePgm( rsrcID );
	}

	return status;
}

/*****************************************************************
* 
* Function:		fxRsrcReleasePgm( rsrcID );
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxRsrcReleasePgm( FXID rsrcID )
{
	PGMINFOSTRUCT *pPgmInfo, *pPgm;
	PARTITIONINFOSTRUCT *pPartitionInfo, *pPartition;
	CHIPINFOSTRUCT *pChipInfo;
	ADDR src, dest;

	pPgmInfo = ID2PRI(rsrcID);
	pPartitionInfo = ID2PAI(pPgmInfo->partitionID);
	pChipInfo = ID2CI(pPgmInfo->chipID);

	/* Release GPRs */
	
	dest = pPgmInfo->gprBaseReloc;
	src = dest + pPgmInfo->gprSpace;
	
	/* CRITICAL SECTION CODE!!! START
	 * Mutex - Parameter update code (per chip)
	 */
/*	REQUEST_PARAM_MUTEX(pPgmInfo->chipID); */

	/* Shift all virtual spaces down to fill in hole */
	while( src < pChipInfo->gprRelocTop ) {
		pChipInfo->gprReloc[dest++] = pChipInfo->gprReloc[src++];
	}
	pChipInfo->gprRelocTop -= pPgmInfo->gprSpace;

	/* Reset any affected pointers */
	pPartition = pChipInfo->pPartitionList;
	while( pPartition ) {
		pPgm = pPartition->pPgmList;
		while( pPgm ) {
			if( pPgm->gprBaseReloc > pPgmInfo->gprBaseReloc ) {
				pPgm->gprBaseReloc -= pPgmInfo->gprSpace;
			}
			pPgm = pPgm->pChain;
		}
		pPartition = pPartition->pChain;
	}
/*	RELEASE_PARAM_MUTEX(pPgmInfo->chipID); */
	/* CRITICAL SECTION CODE END */

	/* Deallocate all reserved resources */
	deallocGPRs( rsrcID, pPgmInfo->chipID, pPgmInfo->partitionID );
	deallocMap(  &(pPartitionInfo->instrLoc),
				 pPgmInfo->instrLoc, 
				 pPgmInfo->partitionID );
	deallocMap(  &(pPartitionInfo->tramLoc),
				 pPgmInfo->tramLoc, 
				 pPgmInfo->partitionID );
	deallocMap(  &(pPartitionInfo->xtramLoc),
				 pPgmInfo->xtramLoc, 
				 pPgmInfo->partitionID );
	deallocMap(  &(pPartitionInfo->tableLoc),
				 pPgmInfo->tableLoc, 
				 pPgmInfo->partitionID );

	pPgmInfo->instrLoc = MAPNIL;
	pPgmInfo->tramLoc = MAPNIL;
	pPgmInfo->xtramLoc = MAPNIL;
	pPgmInfo->tableLoc = MAPNIL;

	return FXERROR_NO_ERROR;
}
	
/*****************************************************************
* 
* Function:		fxRsrcDeletePgm( rsrcID )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxRsrcDeletePgm( FXID rsrcID )
{
	PGMINFOSTRUCT *pPgmInfo, *pPgm;
	PARTITIONINFOSTRUCT *pPartitionInfo;

	pPgmInfo = ID2PRI(rsrcID);
	pPartitionInfo = ID2PAI(pPgmInfo->partitionID);
	
	/* Find program in partition's list */
	for( pPgm = pPartitionInfo->pPgmList;
		 pPgm && pPgm != pPgmInfo && pPgm->pChain != pPgmInfo;
		 pPgm = pPgm->pChain );

	if( !pPgm ) return FXERROR_INVALID_ID;

	/* patch chain */
	if( pPgm == pPgmInfo ) {
		pPartitionInfo->pPgmList = pPgmInfo->pChain;
	} else {
		pPgm->pChain = pPgmInfo->pChain;
	}

	/* free memory according to allocation model */
#if FX_DYNAMIC
	OS_FREE( pPgmInfo );
#else
	pPgmInfo->pChain = pFreePgmList;
	pFreePgmList     = pPgmInfo;
#endif

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* @func static FXSTATUS | allocGPRs |
*	
* Allocates GPRs from one owner to another and sets
* virtual address space, if requested.
*
* @parm FXID		| ownerID	| Specifies FXID of requesting entity.
* @parm FXID		| chipID	| Specifies which chip to allocate from.
* @parm FXID		| poolID	| Specifies FXID from which pool to allocate.
* @parm GPRTYPE	| typeGPR	| Specifies type of GPR to allocate.
* @parm int		| nGPRs		| Specifies number of GPRs to allocate.
* @parm BOOL	| fSet		| Specifies whether to set <p gprReloc[]> array.
* @parm ADDR	| addrBase	| Specifies base address of virtual array.
* @parm ADDR	| addrVirt	| Specifies starting virtual address.
*
* @comm This is a static function.
*
* @rdesc This function returns one of the following:
* 
* @flag FXERROR_NO_ERROR	| If successful.
* @flag FXERROR_OUT_OF_GPR_SPACE | If not enough of the requested GPR type are available.
*
******************************************************************
*/
static FXSTATUS
allocGPRs( FXID ownerID, FXID chipID, FXID poolID,
		   GPRTYPE typeGPR, int nGPRs,
		   BOOL fSet, ADDR addrBase, ADDR addrVirt )
{
	ADDR addr, i;
	ADDR max;
	ADDR dist;
	CHIPINFOSTRUCT *pChipInfo;

	/* Get chip info struct */
	pChipInfo = ID2CI(chipID);

	/* Set initial address and max address of GPR type */
	switch( typeGPR ) {
		case GPR_GENERAL:	addr = pChipInfo->addrGeneral;
							max =  addr + pChipInfo->sizeGeneral;
							break;
		case GPR_TRAM:		addr = pChipInfo->addrTRAMdata;
							max =  addr + pChipInfo->sizeTRAMdata;
							dist = pChipInfo->addrTRAMaddr - 
									pChipInfo->addrTRAMdata;
							break;
		case GPR_XTRAM:		addr = pChipInfo->addrXTRAMdata;
							max =  addr + pChipInfo->sizeXTRAMdata;
							dist = pChipInfo->addrXTRAMaddr -
									pChipInfo->addrXTRAMdata;
							break;
	}

	/* Set index into gprReloc[] array to virtual base address +
	 * virtual start address
	 */
	addrVirt += addrBase;
	
	/* Loop through all GPRs of the specified type and allocate
	 * them in order to the requesting entity.
	 */
	for( i = addr; nGPRs && i < max; ++i ) {
		
		if( pChipInfo->gprAlloc[i] == poolID ) {
			if( fSet ) {
				pChipInfo->gprReloc[addrVirt++] = i;
				if( typeGPR != GPR_GENERAL ) {
					pChipInfo->gprReloc[addrVirt++] = i + dist;
				}
			}
			pChipInfo->gprAlloc[i] = ownerID;
			if( typeGPR != GPR_GENERAL ) {
				pChipInfo->gprAlloc[i+dist] = ownerID;
			}
			--nGPRs;
		}
	}

	if( nGPRs ) return FXERROR_OUT_OF_GPR_SPACE;

	return FXERROR_NO_ERROR;
}


/*****************************************************************
* 
* @func static FXSTATUS | deallocGPRs |
*	
* Deallocates GPRs from one owner to another.
*
* @parm	FXID	| ownerID	| Specifies FXID of requesting entity .
* @parm	FXID	| chipID	| Specifies which chip to deallocate from.
* @parm FXID	| poolID	| Specifies FXID to which pool to release.
*
* @comm This is a static function.
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*
******************************************************************
*/
static FXSTATUS
deallocGPRs( FXID ownerID, FXID chipID, FXID poolID )
{
	CHIPINFOSTRUCT *pChipInfo;
	ADDR addr;

	pChipInfo = ID2CI(chipID);

	for( addr = 0; addr < MAX_GPRS; ++addr ) {
		if( pChipInfo->gprAlloc[addr] == ownerID )
			pChipInfo->gprAlloc[addr] = poolID;
	}

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* @func static FXSTATUS | allocMap |
*	
* Allocate a contiguous block from the beginning of
* a block in the specified chain according to the
* "first-fit" algorithm.
*
* @parm INDEX *	| mapLoc	| Specifies pointer to chain starting location (output).
* @parm ULONG	| ulSize	| Specifies size of contiguous space needed.
* @parm FXID		| ownerID	| Specifies FXID of requesting entity.
* @parm FXID		| poolID	| Specifies FXID from which pool to allocate.
* @parm INDEX * | mapIndex	| Specifies pointer to return value of new block.
*
* @comm This is a static function.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_NO_CONTIGUOUS SPACE	| If not enough contiguous space.
* @flag	FXERROR_NO_FREE_INODES		| No free i-nodes available.
*
******************************************************************
*/
static FXSTATUS
allocMap( INDEX *mapLoc, ULONG ulSize, FXID ownerID, FXID poolID, INDEX *mapIndex )
{
	INDEX i;

	*mapIndex = MAPNIL;

	/* If no space required, exit function */
	if( !ulSize ) return FXERROR_NO_ERROR;

	/* Search for first block that is large enough to hold
	 * requested space
	 */
	i = *mapLoc;
	while( i != MAPNIL && !(memMap[i].ownerID == poolID  &&
			memMap[i].ulSize >= ulSize ) ) {
		i = memMap[i].nextSeg;
		if( i == *mapLoc ) {
			i = MAPNIL;
			break;
		}
	}

	/* If no block large enough, return error */
	if( i == MAPNIL ) return FXERROR_NO_CONTIGUOUS_SPACE;

	/* If block exact size, reallocate block to ownerID */
	if( memMap[i].ulSize == ulSize ) {
		memMap[i].ownerID = ownerID;
		*mapIndex = i;
		return FXERROR_NO_ERROR;
	}

	/* Find new i-node */
	if( !freeInodes ) return FXERROR_NO_FREE_INODES;

	if( nextInode >= MAX_INODES ) nextInode = 0;

	while( memMap[nextInode].ownerID != UNALLOCED_ID ) {
		if( ++nextInode >= MAX_INODES ) nextInode = 0;
	}

	/* Take first ulSize units for block, and create new block with rest.  Fix chain */
	memMap[nextInode].ownerID = memMap[i].ownerID;
	memMap[i].ownerID         = ownerID;
	memMap[nextInode].ulAddr  = memMap[i].ulAddr + ulSize;
	memMap[nextInode].ulSize  = memMap[i].ulSize - ulSize;
	memMap[i].ulSize		  = ulSize;
	memMap[nextInode].nextSeg = memMap[i].nextSeg;
	memMap[nextInode].prevSeg = i;
	memMap[i].nextSeg		  = nextInode;
	if( memMap[nextInode].nextSeg != MAPNIL ) {
		memMap[memMap[nextInode].nextSeg].prevSeg = nextInode;
	}
	--freeInodes;
	
	*mapIndex = i;

	if( ++nextInode >= MAX_INODES ) nextInode = 0;

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* @func static FXSTATUS | deallocMap |
*	
* Coalesces a contiguous free block into surrounding
* free blocks.
*
* @parm INDEX *	| mapLoc	| Pointer to chain starting index.
* @parm INDEX	| mapIndex	| Specifies index into <t memMap[]> of block to free.
* @parm FXID		| poolID	| Specifies FXID to which pool to deallocate.
*
* @comm This is a static function.
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*
******************************************************************
*/
static FXSTATUS
deallocMap( INDEX *mapLoc, INDEX mapIndex, FXID poolID )
{
	INDEX i;

	if( mapIndex == MAPNIL ) return FXERROR_NO_ERROR;

	memMap[mapIndex].ownerID = poolID;

	if( memMap[mapIndex].nextSeg != MAPNIL &&
		memMap[mapIndex].nextSeg != mapIndex &&
		memMap[memMap[mapIndex].nextSeg].ownerID == poolID ) {

		/* consolidate! */
		i = memMap[mapIndex].nextSeg;
		memMap[mapIndex].nextSeg = memMap[i].nextSeg;
		memMap[mapIndex].ulSize += memMap[i].ulSize;
		if( memMap[i].nextSeg != MAPNIL ) {
			memMap[memMap[i].nextSeg].prevSeg = mapIndex;
		}
		memMap[i].ownerID = UNALLOCED_ID;

		/* This next line only has an effect if the list is
		 * circular.
		 */
		if( i == *mapLoc ) *mapLoc = memMap[i].nextSeg;
		
		++freeInodes;
	}

	if( memMap[mapIndex].prevSeg != MAPNIL &&
		memMap[mapIndex].prevSeg != mapIndex &&
		memMap[memMap[mapIndex].prevSeg].ownerID == poolID ) {

		/* consolidate! */
		i = memMap[mapIndex].prevSeg;
		memMap[i].nextSeg = memMap[mapIndex].nextSeg;
		memMap[i].ulSize += memMap[mapIndex].ulSize;
		if( memMap[mapIndex].nextSeg != MAPNIL ) {
			memMap[memMap[mapIndex].nextSeg].prevSeg = i;
		}
		memMap[mapIndex].ownerID = UNALLOCED_ID;

		/* This next line only has an effect if the list is
		 * circular
		 */
		if( mapIndex == *mapLoc ) *mapLoc = memMap[mapIndex].nextSeg;
		
		++freeInodes;
	}

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcDestroy( chipID )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcDestroy( FXCHIPID cID )
{
	CHIPINFOSTRUCT *pChipInfo;
	PARTITIONINFOSTRUCT *pPartitionInfo;
	PGMINFOSTRUCT *pPgmInfo;
	FXID chipID = (FXID)cID;

	pChipInfo = ID2CI( chipID );
	if( !pChipInfo ) return FXERROR_INVALID_ID;

	/* Walk through all partitions and programs and deallocate their
	 * structures.
	 */
	while( pChipInfo->pPartitionList ) {
		pPartitionInfo = pChipInfo->pPartitionList;
		while( pPartitionInfo->pPgmList ) {
			pPgmInfo = pPartitionInfo->pPgmList;
			deallocGPRs( PRI2ID(pPgmInfo), chipID, UNALLOCED_ID );
			deallocMap(  &(pPartitionInfo->instrLoc),
						pPgmInfo->instrLoc, 
						pPgmInfo->partitionID );
			deallocMap(  &(pPartitionInfo->tramLoc),
						pPgmInfo->tramLoc, 
						pPgmInfo->partitionID );
			deallocMap(  &(pPartitionInfo->xtramLoc),
						pPgmInfo->xtramLoc, 
						pPgmInfo->partitionID );
			deallocMap(  &(pPartitionInfo->tableLoc),
						pPgmInfo->tableLoc, 
						pPgmInfo->partitionID );
			fxRsrcDeletePgm( PRI2ID(pPgmInfo) );
		}
		fxRsrcReleasePartition( GETPAID(pPartitionInfo) );
		fxRsrcDeletePartition( GETPAID(pPartitionInfo) );
	}

	/* Deallocate table RAM */
	if( pChipInfo->tableLoc != MAPNIL ) {
		memMap[pChipInfo->tramLoc].ulSize += 
			memMap[pChipInfo->tableLoc].ulSize;
		memMap[pChipInfo->tableLoc].ownerID = UNALLOCED_ID;
		pChipInfo->tableLoc = MAPNIL;
		freeInodes++;
	}

	/* Deallocate scratch registers */
	pChipInfo->gprRelocTop = 0x000;
	pChipInfo->nScratch = 0;
	deallocGPRs( chipID, chipID, UNALLOCED_ID );

	/* Deallocate interpolators */
	if( pChipInfo->interpolatorID != UNALLOCED_ID ) {
		fxParamFreeInterpolators( pChipInfo->ulChipHandle,
								  pChipInfo->interpolatorID );
		deallocMap( &(pChipInfo->instrLoc),
					pChipInfo->interpLoc,
					chipID );
		pChipInfo->interpLoc = MAPNIL;
		pChipInfo->interpolatorID = UNALLOCED_ID;
	}

	return FXERROR_NO_ERROR;
}

#define MAXM 0xFFFFFFFFL

/*****************************************************************
* 
* Function:		fxRsrcQueryChip( chipID, pRsrcQuery )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcQueryChip( FXCHIPID chipID, RSRCQUERY *pRsrcQry )
{
	CHIPINFOSTRUCT *pChipInfo;
	INDEX i;
	int dummy;

	OS_WAITMUTEX(fx8210Mutex);

	pChipInfo = ID2CI((FXID)chipID);
	if( !pChipInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	i = findLargestMap( pChipInfo->instrLoc, (FXID)chipID, MAXM, &dummy );
	if( i == MAPNIL ) pRsrcQry->largestInstr = 0;
	else pRsrcQry->largestInstr = (ADDR) memMap[i].ulSize;

	i = findLargestMap( pChipInfo->tramLoc, (FXID)chipID, MAXM, &dummy );
	if( i == MAPNIL ) pRsrcQry->largestTRAM = 0;
	else pRsrcQry->largestTRAM = memMap[i].ulSize;

	i = findLargestMap( pChipInfo->xtramLoc, (FXID)chipID, MAXM, &dummy );
	if( i == MAPNIL ) pRsrcQry->largestXTRAM = 0;
	else pRsrcQry->largestXTRAM = memMap[i].ulSize;

	i = findLargestMap( pChipInfo->tableLoc, (FXID)chipID, MAXM, &dummy );
	if( i == MAPNIL ) pRsrcQry->largestTable = 0;
	else pRsrcQry->largestTable = memMap[i].ulSize;

	pRsrcQry->nAvailGeneral = 
		countGPRs( (FXID)chipID, GPR_GENERAL, UNALLOCED_ID );
	pRsrcQry->nAvailTRAMBuf = 
		countGPRs( (FXID)chipID, GPR_TRAM, UNALLOCED_ID );
	pRsrcQry->nAvailXTRAMBuf = 
		countGPRs( (FXID)chipID, GPR_XTRAM, UNALLOCED_ID );

	pRsrcQry->nScratch = pChipInfo->nScratch;

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcQueryPartition( partitionID, pRsrcQuery )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcQueryPartition( FXPARTID partitionID, RSRCQUERY *pRsrcQry )
{
	PARTITIONINFOSTRUCT *pPartitionInfo;
	INDEX i;
	int dummy;

	OS_WAITMUTEX(fx8210Mutex);

	pPartitionInfo = ID2PAI((FXID)partitionID);
	if( !pPartitionInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	i = findLargestMap( pPartitionInfo->instrLoc, (FXID)partitionID, MAXM, &dummy );
	if( i == MAPNIL ) pRsrcQry->largestInstr = 0;
	else pRsrcQry->largestInstr = (ADDR) memMap[i].ulSize;

	i = findLargestMap( pPartitionInfo->tramLoc, (FXID)partitionID, MAXM, &dummy );
	if( i == MAPNIL ) pRsrcQry->largestTRAM = 0;
	else pRsrcQry->largestTRAM = memMap[i].ulSize;

	i = findLargestMap( pPartitionInfo->xtramLoc, (FXID)partitionID, MAXM, &dummy );
	if( i == MAPNIL ) pRsrcQry->largestXTRAM = 0;
	else pRsrcQry->largestXTRAM = memMap[i].ulSize;

	i = findLargestMap( pPartitionInfo->tableLoc, (FXID)partitionID, MAXM, &dummy );
	if( i == MAPNIL ) pRsrcQry->largestTable = 0;
	else pRsrcQry->largestTable = memMap[i].ulSize;

	pRsrcQry->nAvailGeneral = 
		countGPRs(pPartitionInfo->chipID, GPR_GENERAL, (FXID)partitionID);
	pRsrcQry->nAvailTRAMBuf = 
		countGPRs( pPartitionInfo->chipID, GPR_TRAM, (FXID)partitionID );
	pRsrcQry->nAvailXTRAMBuf = 
		countGPRs( pPartitionInfo->chipID, GPR_XTRAM, (FXID)partitionID );

	pRsrcQry->nScratch = (ID2CI(pPartitionInfo->chipID))->nScratch;

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcQueryPgm( rsrcID, pPgmRsrcQuery )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS
fxRsrcQueryPgm( FXID rsrcID, PGMRSRCQUERY *pPgmQry )
{
	PGMINFOSTRUCT *pPgmInfo;
	
	pPgmInfo = ID2PRI(rsrcID);

	if( pPgmInfo->instrLoc == MAPNIL ) {
		pPgmQry->instrAddr = 0x000;
		pPgmQry->instrSize = 0;
	} else {
		pPgmQry->instrAddr = (ADDR)memMap[pPgmInfo->instrLoc].ulAddr;
		pPgmQry->instrSize = (ADDR)memMap[pPgmInfo->instrLoc].ulSize;
	}

	if( pPgmInfo->tramLoc == MAPNIL ) {
		pPgmQry->tramAddr = 0x00000000L;
		pPgmQry->tramSize = 0L;
	} else {
		pPgmQry->tramAddr = memMap[pPgmInfo->tramLoc].ulAddr;
		pPgmQry->tramSize = memMap[pPgmInfo->tramLoc].ulSize;
	}

	if( pPgmInfo->xtramLoc == MAPNIL ) {
		pPgmQry->xtramAddr = 0x00000000L;
		pPgmQry->xtramSize = 0L;
	} else {
		pPgmQry->xtramAddr = memMap[pPgmInfo->xtramLoc].ulAddr;
		pPgmQry->xtramSize = memMap[pPgmInfo->xtramLoc].ulSize;
	}

	if( pPgmInfo->tableLoc == MAPNIL ) {
		pPgmQry->tableAddr = 0x00000000L;
		pPgmQry->tableSize = 0L;
	} else {
		pPgmQry->tableAddr = memMap[pPgmInfo->tableLoc].ulAddr;
		pPgmQry->tableSize = memMap[pPgmInfo->tableLoc].ulSize;
	}

	pPgmQry->nGeneral = pPgmInfo->nGeneral;
	pPgmQry->nTRAMbuf = pPgmInfo->nTRAMbuf;
	pPgmQry->nXTRAMbuf = pPgmInfo->nXTRAMbuf;

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcQueryBlockChip( chipID, typeContig, pBlockQueryStruct )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcQueryBlockChip( FXCHIPID chipID, CONTIGTYPE typeContig, BLOCKQUERY *pBlockQueryStruct )
{
	CHIPINFOSTRUCT *pChipInfo;
	INDEX loc, i;

	OS_WAITMUTEX(fx8210Mutex);

	pChipInfo = ID2CI((FXID)chipID);
	if( !pChipInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	loc = ( typeContig == FXCONTIG_INSTR ) ? pChipInfo->instrLoc :
		  ( typeContig == FXCONTIG_TRAM  ) ? pChipInfo->tramLoc  :
		  ( typeContig == FXCONTIG_XTRAM ) ? pChipInfo->xtramLoc :
		  ( typeContig == FXCONTIG_TABLE ) ? pChipInfo->tableLoc :
											 MAPNIL+1;

	if( loc == MAPNIL+1 ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_CONTIGTYPE;
	}

	if( pBlockQueryStruct->ulSize == 0L ) {
		pBlockQueryStruct->ulSize == 0xFFFFFFFFL;
	}

	i = findLargestMap( loc, (FXID)chipID, pBlockQueryStruct->ulSize, 
						&(pBlockQueryStruct->numSize) );
	pBlockQueryStruct->ulSize = (i == MAPNIL) ? 0L : memMap[i].ulSize;

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcQueryBlockPartition( partitionID, typeContig, 
*										   pBlockQueryStruct )
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXSTATUS EMUAPIEXPORT
fxRsrcQueryBlockPartition( FXPARTID partitionID, CONTIGTYPE typeContig, BLOCKQUERY *pBlockQueryStruct )
{
	PARTITIONINFOSTRUCT *pPartitionInfo;
	INDEX loc, i;

	OS_WAITMUTEX(fx8210Mutex);

	pPartitionInfo = ID2PAI((FXID)partitionID);
	if( !pPartitionInfo ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_ID;
	}

	loc = ( typeContig == FXCONTIG_INSTR ) ? pPartitionInfo->instrLoc :
		  ( typeContig == FXCONTIG_TRAM  ) ? pPartitionInfo->tramLoc  :
		  ( typeContig == FXCONTIG_XTRAM ) ? pPartitionInfo->xtramLoc :
		  ( typeContig == FXCONTIG_TABLE ) ? pPartitionInfo->tableLoc :
											 MAPNIL+1;

	if( loc == MAPNIL+1 ) {
		OS_RELEASEMUTEX(fx8210Mutex);
		return FXERROR_INVALID_CONTIGTYPE;
	}

	if( pBlockQueryStruct->ulSize == 0L ) {
		pBlockQueryStruct->ulSize == 0xFFFFFFFFL;
	}

	i = findLargestMap( loc, (FXID)partitionID, pBlockQueryStruct->ulSize, 
						&(pBlockQueryStruct->numSize) );
	pBlockQueryStruct->ulSize = (i == MAPNIL) ? 0L : memMap[i].ulSize;

	OS_RELEASEMUTEX(fx8210Mutex);

	return FXERROR_NO_ERROR;
}


/*****************************************************************
* 
* @func static INDEX | findLargestMap |
*	
* Finds largest block less than ulRank in size with a certain 
* ownerID in a chain.  Counts number of blocks this size.
*
* @parm INDEX	| loc		| Specifies index into <t memMap[]> of start of chain.
* @parm FXID		| ownerID	| Specifies owner to search.
* @parm ULONG	| ulRank	| Specifies next smaller rank to find.
* @parm int *	| numSize	| Specifies output location of how many found.
*
* @comm This is a static function.
*
* @rdesc This function returns the index of the largest block.
*
******************************************************************
*/
static INDEX
findLargestMap( INDEX loc, FXID ownerID, ULONG ulRank, int *numSize )
{
	INDEX firstloc = loc;
	INDEX largeloc = MAPNIL;

	*numSize = 0;

	while( loc != MAPNIL ) {

		if( memMap[loc].ownerID == ownerID ) {
			if( memMap[loc].ulSize < ulRank ) {
				if( largeloc == MAPNIL ||
				    memMap[loc].ulSize > memMap[largeloc].ulSize ) {
					*numSize = 1;
					largeloc = loc;
				} else if( memMap[loc].ulSize == memMap[largeloc].ulSize ) {
					(*numSize)++;
				}
			}
		}
		loc = memMap[loc].nextSeg;
		if( loc == firstloc ) break;
	}

	return largeloc;
}


/*****************************************************************
* 
* @func static ADDR | countGPRs |	
*
* Counts GPRs with a particular ownerID.
*
* @parm FXID		| chipID	| Specifies chip to search.
* @parm	GPRTYPE	| gprType	| Specifies type of GPR to count.
* @parm FXID		| ownerID	| Specifies owner to search for.
*
* @comm This is a static function
*
* @rdesc This function returns the number of GPRs that fit criteria.
*
******************************************************************
*/
static ADDR
countGPRs( FXID chipID, GPRTYPE gprType, FXID ownerID )
{
	CHIPINFOSTRUCT *pChipInfo;
	ADDR addr, max, count;


	pChipInfo = ID2CI(chipID);

	switch( gprType ) {
		case GPR_GENERAL:	addr = pChipInfo->addrGeneral;
							max =  addr + pChipInfo->sizeGeneral;
							break;
		case GPR_TRAM:		addr = pChipInfo->addrTRAMdata;
							max =  addr + pChipInfo->sizeTRAMdata;
							break;
		case GPR_XTRAM:		addr = pChipInfo->addrXTRAMdata;
							max =  addr + pChipInfo->sizeXTRAMdata;
							break;
	}

	for( count=0; addr < max; addr++ ) {
		if( pChipInfo->gprAlloc[addr] == ownerID ) count++;
	}

	return count;
}

/*****************************************************************
* 
* Function:		fxRsrcMapVirtualtoPhysicalGPR( rsrcID, addrVirt )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
ADDR
fxRsrcMapVirtualToPhysicalGPR( FXID rsrcID, ADDR addrVirt )
{
	PGMINFOSTRUCT  *pPgmInfo;
	CHIPINFOSTRUCT *pChipInfo;

	pPgmInfo = ID2PRI( rsrcID );
	pChipInfo = ID2CI( pPgmInfo->chipID );

	if( addrVirt >= pPgmInfo->gprSpace ) {
		if( addrVirt >= pPgmInfo->gprSpace + pChipInfo->nScratch ) return FXADDRERROR;
		return( pChipInfo->gprReloc[addrVirt - pPgmInfo->gprSpace] );
	}

	return( pChipInfo->gprReloc[pPgmInfo->gprBaseReloc + addrVirt] );
}

/*****************************************************************
* 
* Function:		fxRsrcMapPhysicalToVirtualGPR( rsrcID, addrPhys )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
ADDR
fxRsrcMapPhysicalToVirtualGPR( FXID rsrcID, ADDR addrPhys )
{
	PGMINFOSTRUCT  *pPgmInfo;
	CHIPINFOSTRUCT *pChipInfo;
	ADDR i;

	pPgmInfo = ID2PRI( rsrcID );
	pChipInfo = ID2CI( pPgmInfo->chipID );

	if( pChipInfo->gprAlloc[addrPhys] == rsrcID ) {
		for( i=0; i<pPgmInfo->gprSpace; i++ ) {
			if( pChipInfo->gprReloc[pPgmInfo->gprBaseReloc + i] == addrPhys )
				return i;
		}
		for( i=pPgmInfo->gprSpace; i<pPgmInfo->gprSpace+pChipInfo->nScratch; i++ ) {
			if( pChipInfo->gprReloc[i-pPgmInfo->gprSpace] == addrPhys )
				return i;
		}
	}

	return FXADDRERROR;
}

/*****************************************************************
* 
* Function:		fxRsrcMapVirtualtoPhysicalInstr( rsrcID, addrVirt )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
ADDR
fxRsrcMapVirtualToPhysicalInstr( FXID rsrcID, ADDR addrVirt )
{
	PGMINFOSTRUCT  *pPgmInfo;
	CHIPINFOSTRUCT *pChipInfo;

	pPgmInfo = ID2PRI( rsrcID );
	pChipInfo = ID2CI( pPgmInfo->chipID );

	if( (ULONG)addrVirt >= memMap[pPgmInfo->instrLoc].ulSize )
		return FXADDRERROR;

	return( (ADDR)(memMap[pPgmInfo->instrLoc].ulAddr + addrVirt) );
	
}

/*****************************************************************
* 
* Function:		fxRsrcMapPhysicalToVirtualInstr( rsrcID, addrPhys )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
ADDR
fxRsrcMapPhysicalToVirtualInstr( FXID rsrcID, ADDR addrPhys )
{
	PGMINFOSTRUCT  *pPgmInfo;
	CHIPINFOSTRUCT *pChipInfo;

	pPgmInfo = ID2PRI( rsrcID );
	pChipInfo = ID2CI( pPgmInfo->chipID );

	if( (ULONG)addrPhys <  memMap[pPgmInfo->instrLoc].ulAddr ||
	    (ULONG)addrPhys >= memMap[pPgmInfo->instrLoc].ulAddr + 
					memMap[pPgmInfo->instrLoc].ulSize )
		return FXADDRERROR;

	return( (ADDR)((ULONG)addrPhys - memMap[pPgmInfo->instrLoc].ulAddr) );
	
}


/*****************************************************************
* 
* Function:		fxRsrcPgmIsLoadable( partitionID, pPgmRsrcQuery )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
BOOL
fxRsrcPgmIsLoadable( FXID partitionID, PGMRSRCQUERY *pPgmRsrcQuery )
{
	RSRCQUERY partQuery;
	
	fxRsrcQueryPartition( partitionID, &partQuery );

	if( pPgmRsrcQuery->nGeneral > partQuery.nAvailGeneral ) return FALSE;
	if( pPgmRsrcQuery->nTRAMbuf > partQuery.nAvailTRAMBuf ) return FALSE;
	if( pPgmRsrcQuery->nXTRAMbuf > partQuery.nAvailXTRAMBuf ) return FALSE;
	if( pPgmRsrcQuery->instrSize > partQuery.largestInstr ) return FALSE;
	if( pPgmRsrcQuery->tramSize > partQuery.largestTRAM ) return FALSE;
	if( pPgmRsrcQuery->xtramSize > partQuery.largestXTRAM ) return FALSE;
	if( pPgmRsrcQuery->tableSize > partQuery.largestTable ) return FALSE;

	return TRUE;
}

/*****************************************************************
* 
* Function:		fxRsrcPgmCanReplace( pPgmRsrcQuery, pid[] )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
BOOL
fxRsrcPgmCanReplace( PGMRSRCQUERY *pPgmRsrcQuery, FXID pid[] )
{
	RSRCQUERY	 partQuery;
	PGMRSRCQUERY pgmQuery;
	FXID			 partitionID;
	BOOL		 enoughInstr, enoughTRAM, enoughXTRAM, enoughTable;
	BOOL		 enoughGPRs;
	PARTITIONINFOSTRUCT *pPartitionInfo;
	INDEX		 index;
	ULONG		 ulSize;
	int			 i;

	if( pid[0] == UNALLOCED_ID ) return FALSE;
	partitionID = (ID2PRI(pid[0]))->partitionID;
	pPartitionInfo = ID2PAI(partitionID);

	fxRsrcQueryPartition( partitionID, &partQuery );
	enoughGPRs = (pPgmRsrcQuery->nGeneral <= partQuery.nAvailGeneral) &&
				 (pPgmRsrcQuery->nTRAMbuf <= partQuery.nAvailTRAMBuf) &&
				 (pPgmRsrcQuery->nXTRAMbuf <= partQuery.nAvailXTRAMBuf);
	enoughInstr = (pPgmRsrcQuery->instrSize <= partQuery.largestInstr);
	enoughTRAM  = (pPgmRsrcQuery->tramSize <= partQuery.largestTRAM);
	enoughXTRAM = (pPgmRsrcQuery->xtramSize <= partQuery.largestXTRAM);
	enoughTable = (pPgmRsrcQuery->tableSize <= partQuery.largestTable);
	
	i = 0;
	while( pid[i] != UNALLOCED_ID && !enoughGPRs ) {

		if( (ID2PRI(pid))->partitionID != partitionID ) return FALSE;

		fxRsrcQueryPgm( pid[i], &pgmQuery );
		partQuery.nAvailGeneral += pgmQuery.nGeneral;
		partQuery.nAvailTRAMBuf += pgmQuery.nTRAMbuf;
		partQuery.nAvailXTRAMBuf += pgmQuery.nXTRAMbuf;
		enoughGPRs = (pPgmRsrcQuery->nGeneral <= partQuery.nAvailGeneral) &&
				 (pPgmRsrcQuery->nTRAMbuf <= partQuery.nAvailTRAMBuf) &&
				 (pPgmRsrcQuery->nXTRAMbuf <= partQuery.nAvailXTRAMBuf);
		i++;
	}

	if( !enoughGPRs ) return FALSE;

	/* Find any free or to-be-freed chunk that will fit the required
	 * number of instructions
	 */
	index = pPartitionInfo->instrLoc;
	while( !enoughInstr && index != MAPNIL ) {

		ulSize = 0L;
		while( !enoughInstr ) {
			
			if( index == MAPNIL ) return FALSE;

			if( memMap[index].ownerID != partitionID ) {

				i = 0;
				while( pid[i] != UNALLOCED_ID && memMap[index].ownerID != pid[i] ) {
					i++;
				}
				if( pid[i] == UNALLOCED_ID ) {
					index = memMap[index].nextSeg;
					break;
				}
			}

			ulSize += memMap[index].ulSize;
			enoughInstr = ((ULONG)pPgmRsrcQuery->instrSize <= ulSize);
			index = memMap[index].nextSeg;
		}

	}

	if( !enoughInstr ) return FALSE;
		
	/* Find any free or to-be-freed chunk that will fit the required
	 * number of TRAM locations
	 */
	index = pPartitionInfo->tramLoc;
	while( !enoughTRAM && index != MAPNIL ) {

		ulSize = 0L;
		while( !enoughTRAM ) {
			
			if( index == MAPNIL ) return FALSE;

			if( memMap[index].ownerID != partitionID ) {

				i = 0;
				while( pid[i] != UNALLOCED_ID && memMap[index].ownerID != pid[i] ) {
					i++;
				}
				if( pid[i] == UNALLOCED_ID ) {
					index = memMap[index].nextSeg;
					break;
				}
			}

			ulSize += memMap[index].ulSize;
			enoughTRAM = (pPgmRsrcQuery->tramSize <= ulSize);
			index = memMap[index].nextSeg;
		}

	}

	if( !enoughXTRAM ) return FALSE;

	/* Find any free or to-be-freed chunk that will fit the required
	 * number of XTRAM locations
	 */
	index = pPartitionInfo->xtramLoc;
	while( !enoughXTRAM && index != MAPNIL ) {

		ulSize = 0L;
		while( !enoughXTRAM ) {
			
			if( index == MAPNIL ) return FALSE;

			if( memMap[index].ownerID != partitionID ) {

				i = 0;
				while( pid[i] != UNALLOCED_ID && memMap[index].ownerID != pid[i] ) {
					i++;
				}
				if( pid[i] == UNALLOCED_ID ) {
					index = memMap[index].nextSeg;
					break;
				}
			}

			ulSize += memMap[index].ulSize;
			enoughXTRAM = (pPgmRsrcQuery->xtramSize <= ulSize);
			index = memMap[index].nextSeg;
		}

	}

	if( !enoughXTRAM ) return FALSE;

	/* Find any free or to-be-freed chunk that will fit the required
	 * number of Table RAM locations
	 */
	index = pPartitionInfo->tableLoc;
	while( !enoughTable && index != MAPNIL ) {

		ulSize = 0L;
		while( !enoughTable ) {
			
			if( index == MAPNIL ) return FALSE;

			if( memMap[index].ownerID != partitionID ) {

				i = 0;
				while( pid[i] != UNALLOCED_ID && memMap[index].ownerID != pid[i] ) {
					i++;
				}
				if( pid[i] == UNALLOCED_ID ) {
					index = memMap[index].nextSeg;
					break;
				}
			}

			ulSize += memMap[index].ulSize;
			enoughTable = (pPgmRsrcQuery->tableSize <= ulSize);
			index = memMap[index].nextSeg;
		}

	}

	if( !enoughTable ) return FALSE;

	return TRUE;
}

/*****************************************************************
* 
* Function:		fxRsrcWhichPartition( rsrcID )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXID
fxRsrcWhichPartition( FXID rsrcID )
{
	return( (ID2PRI(rsrcID))->partitionID );
}

/*****************************************************************
* 
* Function:		fxRsrcGetInterpolatorID( rsrcID )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXID
fxRsrcGetInterpolatorID( FXID rsrcID )
{
	return( (ID2CI((ID2PRI(rsrcID))->chipID))->interpolatorID );
}

/*****************************************************************
* 
* Function:		fxRsrcWhichChip( rsrcID )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
FXID
fxRsrcWhichChip( FXID rsrcID )
{
	return( (ID2PRI(rsrcID))->chipID );
}

/*****************************************************************
* 
* Function:		fxRsrcMapPhysicalToEnumFlags( rsrcID, bEnum )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
BYTE
fxRsrcMapEnumToPhysicalFlags( FXID rsrcID, BYTE bEnum )
{
	return( (ID2CI((ID2PRI(rsrcID))->chipID))->flagMap[bEnum] );
}

/*****************************************************************
* 
* Function:		fxRsrcMapEnumToPhysicalFlags( rsrcID, bPhys )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
BYTE
fxRsrcMapPhysicalToEnumFlags( FXID rsrcID, BYTE bPhys )
{
	CHIPINFOSTRUCT *pChipInfo;
	int i;

	pChipInfo = ID2CI( (ID2PRI(rsrcID))->chipID );

	for( i=0; i<FLAGMAPSIZE; i++ )
		if( pChipInfo->flagMap[i] == bPhys ) break;

	return (BYTE)(i&0xff);
}

/*****************************************************************
* 
* Function:		fxRsrcIsTRAMBuf( rsrcID, tramAddr, offs )
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
BOOL
fxRsrcIsTRAMBuf( FXID rsrcID, OPERAND *tramAddr, ULONG *offs )
{
	PGMINFOSTRUCT  *pPgmInfo;
	CHIPINFOSTRUCT *pChipInfo;
	
	pPgmInfo = ID2PRI(rsrcID);
	pChipInfo = ID2CI( pPgmInfo->chipID );

	if( (*tramAddr) & (~VIRTMASK) )
		*tramAddr = pChipInfo->gprReloc[pPgmInfo->gprBaseReloc + (*tramAddr)&VIRTMASK];

	if( *tramAddr >= pChipInfo->addrTRAMaddr && 
		*tramAddr <  pChipInfo->addrTRAMaddr + pChipInfo->sizeTRAMdata ) {
		*offs = memMap[pPgmInfo->tramLoc].ulAddr;
		return TRUE;
	}

	if( *tramAddr >= pChipInfo->addrTRAMdata &&
		*tramAddr <  pChipInfo->addrTRAMdata + pChipInfo->sizeTRAMdata ) {
		*tramAddr = pChipInfo->addrTRAMaddr +(*tramAddr-pChipInfo->addrTRAMdata);
		*offs = memMap[pPgmInfo->tramLoc].ulAddr;
		return TRUE;
	}

	if( *tramAddr >= pChipInfo->addrXTRAMaddr &&
		*tramAddr <  pChipInfo->addrXTRAMaddr + pChipInfo->sizeXTRAMdata ) {
		*offs = memMap[pPgmInfo->xtramLoc].ulAddr;
		return TRUE;
	}

	if( *tramAddr >= pChipInfo->addrXTRAMdata &&
		*tramAddr <  pChipInfo->addrXTRAMdata + pChipInfo->sizeXTRAMdata ) {
		*tramAddr = pChipInfo->addrXTRAMaddr+(*tramAddr-pChipInfo->addrXTRAMdata);
		*offs = memMap[pPgmInfo->xtramLoc].ulAddr;
		return TRUE;
	}

	return FALSE;
}

/*****************************************************************
* 
* Function:		fxRsrcSetPortMask
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
void
fxRsrcSetPortMask( FXID chipID, ULONG ulInMask, ULONG ulOutMask )
{
	(ID2CI(chipID))->ulInMask |= ulInMask;
	(ID2CI(chipID))->ulOutMask |= ulOutMask;
}

/*****************************************************************
* 
* Function:		fxRsrcClearPortMask
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
void
fxRsrcClearPortMask( FXID chipID, ULONG ulInMask, ULONG ulOutMask )
{
	(ID2CI(chipID))->ulInMask &= (~ulInMask);
	(ID2CI(chipID))->ulOutMask &= (~ulOutMask);
}

/*****************************************************************
* 
* Function:		fxRsrcMaskPorts
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
BOOL
fxRsrcMaskPorts( FXID chipID, ULONG ulInMask, ULONG ulOutMask )
{
	return( ((((ID2CI(chipID))->ulInMask) & ulInMask) | 
		     (((ID2CI(chipID))->ulOutMask) & ulOutMask)) == 0 );
}

/*****************************************************************
* 
* Function:		fxRsrcGetChipHandle
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
ULONG
fxRsrcGetChipHandle( FXID rsrcID )
{
	return ((ID2CI((ID2PRI(rsrcID))->chipID))->ulChipHandle);
}

/*****************************************************************
* 
* Function:		fxRsrcGetChipRevisionID
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
char *
fxRsrcGetChipRevisionID( FXID chipID, ULONG *pulRevReg )
{
	*pulRevReg = (ID2CI(chipID))->ulRevReg;
	return( (ID2CI(chipID))->chipRevisionID );
}

/*****************************************************************
* 
* Function:		fxRsrcValidPartitionID
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
BOOL
fxRsrcValidPartitionID( FXID partitionID )
{
	if( ID2PAI(partitionID) == NULL ) return FALSE;
	return TRUE;
}

/*****************************************************************
* 
* Function:		fxRsrcValidChipID
*	
* See FXRESMAN.H for description.
*
******************************************************************
*/
BOOL
fxRsrcValidChipID( FXID chipID )
{
	if( ID2CI(chipID) == NULL ) return FALSE;
	return TRUE;
}
