/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
* @doc INTERNAL
* @module fxmgrapi.h |
*
* This file contains general definitions, types, and
* macros that pertain to user code in support of the
* FX8010.
*
*******************************************************************
*/
/******************************************************************
*
* @doc EXTERNAL
* @contents1 FX8010 System, Program, and Patch Management Manual |
*
* This document defines the exported data structures and API needed
* to allocate, load, patch, and control effects programs on the
* FX8010.
*
*******************************************************************
*/
/* @doc EXTERNAL */

#ifndef _FXMGRAPI_H
#define _FXMGRAPI_H

#include "fxdtatyp.h"

/* @struct PARTITIONRSRC | 
 *
 * This structure defines the resources required by a partition.  
 * The resource manager expects all fields to be filled, with a 
 * zero in any field not needed.
 *
 * @field ADDR  | nGeneralGPRs | Specifies number of general GPRs required.
 * @field ADDR  | nTRAMGPRs    | Specifies number of TRAM buffers.
 * @field ADDR  | nXTRAMGPRs   | Specifies number of XTRAM buffers.
 * @field ADDR  | instrSpace   | Specifies size of instruction space.
 * @field ULONG | tramSpace    | Specifies size of TRAM space.
 * @field ULONG | xtramSpace   | Specifies size of XTRAM space.
 * @field ULONG | tableSpace   | Specifies size of table space.
 */
typedef struct {
	ADDR				nGeneralGPRs;	/* # general GPRs required */
	ADDR				nTRAMGPRs;		/* # TRAM buffers */
	ADDR				nXTRAMGPRs;		/* # XTRAM buffers */
	ADDR				instrSpace;		/* size of instruction space */
	ULONG				tramSpace;		/* size of TRAM space */
	ULONG				xtramSpace;		/* size of XTRAM space */
	ULONG				tableSpace;		/* size of table space */
} PARTITIONRSRC;

/* @struct PGMRSRC |
 *
 * This structure defines the resources required by a program.  The
 * resource manager expects all fields to be filled, with a zero in
 * any field not needed.
 *
 * @field ADDR	| nInputs	| Specifies number of program inputs.
 * @field ADDR	| nOutputs  | Specifies number of program outputs.
 * @field ADDR	| nGeneral	| Specifies number of general GPRs.
 * @field ADDR	| nTRAMbuf	| Specifies number of TRAM buffers.
 * @field ADDR	| nXTRAMbuf	| Specifies number of XTRAM buffers.
 * @field ADDR	| nScratch	| Specifies number of scratch registers.
 * @field ADDR	| instrSpace| Specifies number of program instruction steps.
 * @field ULONG	| tramSpace	| Specifies size of TRAM space.
 * @field ULONG	| xtramSpace| Specifies size of XTRAM space.
 * @field ULONG	| tableSpace| Specifies size of table space.
 */
typedef struct {
	ADDR				nInputs;		/* # of inputs */
	ADDR				nOutputs;		/* # of outputs */
	ADDR				nGeneral;		/* # of general GPRs */
	ADDR				nTRAMbuf;		/* # of TRAM buffers */
	ADDR				nXTRAMbuf;		/* # of XTRAM buffers */
	ADDR				nScratch;		/* # of scratch registers */
	ADDR				instrSpace;		/* # of instructions */
	ULONG				tramSpace;		/* size of TRAM space */
	ULONG				xtramSpace;		/* size of XTRAM space */
	ULONG				tableSpace;		/* size of table space */
} PGMRSRC;

/* @struct PGMRSRCQUERY |
 *
 * This structure is filled by the <f fxPgmQueryPgm()> function, and
 * contains information about a loaded program.
 *
 * @field ADDR	| instrAddr	| Contains physical instruction address of start of program.
 * @field ADDR	| instrSize	| Contains number of program instruction steps.
 * @field ULONG	| tramAddr	| Contains offset from head of TRAM space.
 * @field ULONG	| tramSize	| Contains size of TRAM space.
 * @field ULONG	| xtramAddr	| Contains offset from head of XTRAM space.
 * @field ULONG	| xtramSize	| Contains size of XTRAM space.
 * @field ULONG	| tableAddr	| Contains start address of table.
 * @field ULONG	| tableSize	| Contains size of table space.
 * @field ADDR	| nGeneral	| Contains number of allocated general GPRs.
 * @field ADDR	| nTRAMbuf	| Contains number of allocated TRAM buffers.
 * @field ADDR	| nXTRAMbuf	| Contains number of allocated XTRAM buffers.
 */
typedef struct {
	ADDR				instrAddr;	/* Chip physical addr of pgm */
	ADDR				instrSize;	/* Size of program space */
	ULONG				tramAddr;	/* Offset from head of TRAM space */
	ULONG				tramSize;	/* Size of TRAM space */
	ULONG				xtramAddr;	/* Offset from head of XTRAM spc */
	ULONG				xtramSize;	/* Size of XTRAM space */
	ULONG				tableAddr;	/* Start address of table */
	ULONG				tableSize;	/* Size of table space */
	ADDR				nGeneral;	/* Number of allocated ge GPRs */
	ADDR				nTRAMbuf;	/* Number of allocated TRAM bufs */
	ADDR				nXTRAMbuf;	/* Number of allocated XTRAM bufs */
} PGMRSRCQUERY;

/* @struct RSRCQUERY |
 *
 * This structure is filled by the <f fxRsrcQueryChip()> and
 * <f fxRsrcQueryPartition()> functions, and contains information about
 * free resources on a specific partition, or on the unpartitioned
 * portions of a chip.
 *
 * @field ADDR	| largestInstr	| Contains size of largest free contiguous instruction block.
 * @field ULONG	| largestTRAM	| Contains size of largest free contiguous TRAM block.
 * @field ULONG	| largestXTRAM	| Contains size of largest free contiguous XTRAM block.
 * @field ULONG	| largestTable	| Contains size of largest free contiguous table block.
 * @field ADDR	| nAvailGeneral	| Contains number of available general GPRs.
 * @field ADDR	| nAvailTRAMBuf	| Contains number of available TRAM buffers.
 * @field ADDR	| nAvailXTRAMBuf| Contains number of available XTRAM buffers.
 * @field ADDR	| nScratch		| Contains number of prealloced scratch registers.
 */
typedef struct {
	ADDR				largestInstr;	/* largest free instr space */
	ULONG				largestTRAM;	/* largest free TRAM space */
	ULONG				largestXTRAM;	/* largest free XTRAM space */
	ULONG				largestTable;	/* largest free table space */
	ADDR				nAvailGeneral;	/* available general GPRs */
	ADDR				nAvailTRAMBuf;	/* available TRAM buffers */
	ADDR				nAvailXTRAMBuf;	/* available XTRAM buffers */
	ADDR				nScratch;		/* # prealloced scratch regs */
} RSRCQUERY;

/*  @enum CONTIGTYPE |
 *
 * This enumeration defines the four contiguous resource types.
 */
typedef enum {
	FXCONTIG_INSTR,					/* @emem Instruction space type */
	FXCONTIG_TRAM,					/* @emem TRAM space type */
	FXCONTIG_XTRAM,					/* @emem XTRAM space type */
	FXCONTIG_TABLE					/* @emem Table RAM space type */
} CONTIGTYPE;

/* @struct BLOCKQUERY |
 *
 * This structure is the input and output of <f fxRsrcQueryBlockChip()>
 * and <f fxRsrcQueryBlockPartition()>.  By filling in <p ulSize> and
 * calling either function, the size of the next largest free block 
 * (that is, the largest free block less than <p ulSize>) of the 
 * specified contiguous resource type is returned in <p ulSize>.  Also,
 * the number of blocks of size <p ulSize> is returned.  An input size
 * of zero will search for the absolute largest free block.
 *
 * @field ULONG	| ulSize	| Contains/Specifies size of free block (input and output).
 * @field int	| numSize	| Contains number of blocks of <p ulSize> size (output only).
 */
typedef struct {
	ULONG				ulSize;			/* size of free block (I/O) */
	int					numSize;		/* number of blocks (O) */
} BLOCKQUERY;

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcAllocChip |
*	
* Sets up table space, scratch registers, and interpolators on an
* unpartitioned chip.
*
* @parm FXCHIPID| chipID		| Specifies the ID of the upartitioned chip.
* @parm ULONG	| ulTableRAM	| Specifies required amount of table RAM.
* @parm ADDR	| nScratch		| Specifies required number of scratch registers.
* @parm ADDR	| nInterpolators| Specifies required number of interpolators.
*
* @comm	This function is only required if table RAM, interpolators
* and/or preallocated scratch registers are needed.
*
* This function can ONLY be called on a completely
* unpartitioned chip.  That is, it must be the very
* next operation on a chip after <f fxRsrcInitChip()> or
* <f fxRsrcDestroy()>.  Otherwise the operation will fail.
*
* @devnote:(INTERNAL) This function violates the loose-cohesion property
* by making a call to the parameter manager.  Sorry.
*
* @rdesc Returns one of the following:
*
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_CHIP_IN_USE		| If chip has been partitioned.
* @flag FXERROR_OUT_OF_TABLE_RAM| If not enough table RAM is available.
* @flag FXERROR_OUT_OF_GPR_SPACE| If not enough general GPRs for the specified number of scratch registers.
* @flag	FXERROR_OUT_OF_MEMORY	| If not enough memory to complete operation.
* @flag FXERROR_NO_FREE_INODES	| If not enough memory to complete operation.
*
* @xref <f fxRsrcDestroy>
*
******************************************************************
*/
EMUCTYPE EMUAPIEXPORT FXSTATUS fxRsrcAllocChip( FXCHIPID, ULONG, ADDR, ADDR );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcDestroy |
*	
* This function deallocates an unpartitioned chip to a completely fresh
* state.  All programs and partitions must first be deallocated.
*
* @parm FXCHIPID| chipID		| Specifies the ID of the upartitioned chip.
*
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_CHIP_IN_USE		| If chip has been partitioned.
*
******************************************************************
*/
EMUCTYPE EMUAPIEXPORT FXSTATUS fxRsrcDestroy( FXCHIPID chipID );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcNewPartition |
*	
* Allocates host memory for a new partition.
*
* @parm FXCHIPID	| chipID	 | Specifies ID of chip to partition.
* @parm FXPARTID *	| partitionID| Pointer to new partition ID.
*
* @comm:(INTERNAL) If FX_DYNAMIC is set, simply FX_MALLOC the
* struct, otherwise, pull from the free list.
*
* @rdesc Returns one of the following:
*
* @flag	FXERROR_NO_ERROR	  | If successful.
* @flag FXERROR_OUT_OF_MEMORY | If not enough memory to complete operation.
*
* @xref <f fxRsrcDeletePartition>
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxRsrcNewPartition( FXCHIPID, FXPARTID * /* IO */);

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcDeletePartition |
*	
* Deallocates host memory reserved for a partition by
* <f fxRsrcNewPartition()>.
*
* @parm FXPARTID	| partitionID | Specifies ID of partition to delete.
*
* @comm	All references to <p partitionID> after this call will
* be invalid.  If <f fxRsrcReleasePartition()> has not
* been called prior to this call, results will be
* undefined.
*
* @rdesc Returns one of the following:
* @flag FXERROR_NO_ERROR   | If successful.
* @flag FXERROR_INVALID_ID | If <p partitionID> is invalid.
*
* @xref <f fxRsrcNewPartition>
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxRsrcDeletePartition( FXPARTID );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcAllocPartition |
*	
* Reserves chip resources for a partition.
*
* @parm FXPARTID				|  partitionID	|
*	Specifies FXID of partition requesting resources.
* @parm PARTITIONRSRC * | pPartitionRsrc | 
*	Pointer to  partition resource definition structure.
*
* @comm	The partition must have been previously allocated
* in the host with a call to <f fxRsrcNewPartition()>.
*
* @rdesc Returns one of the following:
* 
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_OUT_OF_GENERAL_GPRS	| If not enough available general GPRs.
* @flag FXERROR_OUT_OF_TRAM_GPRS	| If not enough available TRAM buffers.
* @flag FXERROR_OUT_OF_XTRAM_GPRS	| If not enough available XTRAM buffers.
* @flag FXERROR_OUT_OF_INSTR_SPACE	| If not enough available contiguous intruction space.
* @flag FXERROR_OUT_OF_TRAM_SPACE	| If not enough available contiguous TRAM space.
* @flag FXERROR_OUT_OF_XTRAM_SPACE	| If not enough available contiguous XTRAM space.
* @flag FXERROR_OUT_OF_TABLE_SPACE	| If not enough available contiguous table space.
* @flag FXERROR_NO_FREE_INODES		| If not enough memory to complete operation.
*
* @xref <f fxRsrcNewPartition> <f fxRsrcReleasePartition>
*		<f fxRsrcDeletePartition> <t PARTITIONRSRC>
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxRsrcAllocPartition( FXPARTID, PARTITIONRSRC * );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcReleasePartition |
*	
* Releases the resources reserved by a partition back to the chip.
*
* @parm FXPARTID	| partitionID | Specifies ID of partition to release.
*
* @comm	This call does not deallocate host resources, only
* DSP resources.  This allows a partition to be
* reconfigured.
*
* The call will fail if there are any programs,
* running or not, associated with this partition.  All
* programs must be released and deleted before this
* call.
*
* @rdesc Returns one of the following:	
* @flag FXERROR_NO_ERROR | If successful.
* @flag FXERROR_PARTITION_NOT_EMPTY | If partition is not completely deallocated.
*
* @xref <f fxRsrcAllocPartition> <f fxRsrcNewPartition>
*	    <f fxRsrcDeletePartition>
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxRsrcReleasePartition( FXPARTID );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcQueryChip |
*	
* Gathers information about free resources on the chip.
*
* @parm FXCHIPID   | chipID		| Specifies ID of chip to query.
* @parm RSRCQUERY *| pRsrcQuery	| Pointer to query structure.
*
* @comm "Free" resources refer to any resource that has
* not been reserved by a partition.
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*
* @xref <t RSRCQUERY>
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxRsrcQueryChip( FXCHIPID, RSRCQUERY * /* IO */ );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcQueryPartition |
*	
* Gathers information about free resources in a
* partition.
*
* @parm FXPARTID   | partitionID | Specifies ID of partition to query.
* @parm RSRCQUERY *| pRsrcQuery	 | Pointer to query structure.
*
* @comm "Free" resources refer to any resource reserved
* by this partition, but not allocated to a program
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*
* @xref <t RSRCQUERY>
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxRsrcQueryPartition( FXPARTID, RSRCQUERY * /* IO */ );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPgmQueryProgram |
*	
* Gathers information about allocated resources in a
* program.
*
* @parm FXPGMID		  | pgmID		 | Specifies ID of program to query.
* @parm PGMRSRCQUERY *| pRsrcQuery	 | Pointer to query structure.
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*
* @xref <t PGMRSRCQUERY>
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmQueryProgram( FXPGMID, PGMRSRCQUERY * /* IO */ );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcQueryBlockChip |
*	
* Finds next largest block of a contiguous resource type on 
* a chip.  Also counts how many blocks of this size exist.
* <p pBlockQueryStruct-><gt><p ulSize> contains lowest rank to ignore,
* and result also returned in <p pBlockQueryStruct>.  A value
* of zero in <p pBlockQueryStruct-><gt><p ulSize> will find absolute largest.
*
* @parm FXCHIPID	| chipID	| Specifies ID of chip to query.
* @parm CONTIGTYPE	| typeContig| Specifies type of contiguous resource.
* @parm BLOCKQUERY *| pBlockQueryStruct | Pointer to query structure.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_CONTIGTYPE	| If <p typeContig> is invalid.
*
* @xref <t BLOCKQUERY> <t CONTIGTYPE>
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxRsrcQueryBlockChip( FXCHIPID, CONTIGTYPE, BLOCKQUERY * /* IO */);

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcQueryBlockPartition |
*	
* Finds next largest block of a contiguous resource type on a 
* partition.  Also counts how many blocks of this size exist.
* <p pBlockQueryStruct-><gt><p ulSize> contains lowest rank to ignore,
* and result also returned in <p pBlockQueryStruct>.  A value
* of zero in <p pBlockQueryStruct-><gt><p ulSize> will find absolute largest.
*
* @parm FXPARTID	| partitionID | Specifies ID of partition to query.
* @parm CONTIGTYPE	| typeContig  | Specifies type of contiguous resource.
* @parm BLOCKQUERY *| pBlockQueryStruct | Pointer to query structure.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_CONTIGTYPE	| If <p typeContig> is invalid.
*
* @xref <t BLOCKQUERY> <t CONTIGTYPE>
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxRsrcQueryBlockPartition( FXPARTID, CONTIGTYPE, BLOCKQUERY * /* IO */);

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcGetChipList |
*
* This function returns a list of the discovered chip IDs.	
*
* @parm FXCHIPID | chipID[]	| Specifies array to fill.
* @parm DWORD    | sizeArray| Specifies max size of array.
* @parm DWORD *  | numChips | Returns number of chips found.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxRsrcGetChipList( FXCHIPID * /* IO */ /* ARRAY 256 */, 
									DWORD, DWORD * /* IO */ );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcGetPartitionList |
*
* This function returns a list of the partitions of a chip.	
*
* @parm FXCHIPID | chipID	| Specifies chip to search.
* @parm FXPARTID | partitionID[]	| Specifies array to fill.
* @parm DWORD    | sizeArray| Specifies max size of array.
* @parm DWORD *  | numChips | Returns number of partitions found.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_ID			| Invalid <p chipID>.
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxRsrcGetPartitionList( FXCHIPID, 
							        FXPARTID * /* IO */ /* ARRAY 256 */, 
									DWORD, DWORD * /* IO */ );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchChannel |
*
* This function patches logical output (source) channel <p nOutChan>
* of program <p pgmA> to logical input (sink) channel <p nInpChan> of
* program <p pgmB>.
*
* @parm FXPGMID	| pgmA		| Specifies program whose output to patch.
* @parm USHORT | nOutChan  | Specifies logical output (source) channel.
* @parm FXPGMID	| pgmB		| Specifies program whose input to patch.
* @parm USHORT | nInpChan  | Specifies logical input (sink) channel.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmA> or <p pgmB> is invalid.
* @flag FXERROR_PROGRAMS_ON_DIFFERENT_CHIPS |
*								  If <p pgmA> and <p pgmB> are not on
*								  the same chip.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchChannel( FXPGMID, USHORT, FXPGMID, USHORT );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchGroundOutput |
*
* This function grounds logical (source) output <p nOutChan> of program <p pgmID>, 
* and consequently grounds the input (sink) of the program that
* was originally attached to it.
*
* @parm FXPGMID	| pgmID		| Specifies program whose output to ground.
* @parm USHORT | nOutChan  | Specifies logical output (source) channel.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchGroundOutput( FXPGMID, USHORT );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchGroundInput |
*
* This function grounds logical (sink) intput <p nInpChan> of program <p pgmID>, 
* and consequently grounds the output (source) of the program that
* was originally attached to it.
*
* @parm FXPGMID	| pgmID		| Specifies program whose input to ground.
* @parm USHORT | nInpChan  | Specifies logical input (sink) channel.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchGroundInput( FXPGMID, USHORT );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchUnpatchPgm |
*
* This function grounds all inputs and outputs of <p pgmID> and the
* corresponding inputs and outputs of all programs attached to it.
*
* @parm FXPGMID	| pgmID		| Specifies program whose input to ground.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchUnpatchPgm( FXPGMID );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchInsertBefore |
*
* This function will patch the inputs of <p pgmA> to become the
* inputs of <p pgmB>, and then patch the outputs of <p pgmB> to 
* become the new inputs of <p pgmA>.  Input patching of <p pgmB>
* will be performed in logical order with input 0 of <p pgmA> becoming
* input 0 of <p pgmB>, input 1 of <p pgmA> becoming input 1 of
* <p pgmB>, and so on.  If <p pgmA> has more inputs than <p pgmB>,
* the unpatched inputs will be grounded.  If <p pgmB> has more inputs
* than <p pgmA>, the remaining inputs will be patched starting from
* <p pgmA> input 0 (that is, wrapped modulo the number of inputs of
* <p pgmA>).  The inputs of <p pgmA> will now be fed by the outputs
* of <p pgmB> according to the same rules above.
*
* @parm FXPGMID	| pgmA	| Specifies program to patch before.
* @parm FXPGMID	| pgmB	| Specifies program to insert.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmA> or <p pgmB> is invalid.
* @flag FXERROR_PROGRAMS_ON_DIFFERENT_CHIPS |
*								  If <p pgmA> and <p pgmB> are not on
*								  the same chip.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchInsertBefore( FXPGMID, FXPGMID );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchInsertAfter |
*
* This function will patch the outputs of <p pgmA> to become the
* outputs of <p pgmB>, and then patch the inputs of <p pgmB> to 
* become the new outputs of <p pgmA>.  Output patching of <p pgmB>
* will be performed in logical order with output 0 of <p pgmA> becoming
* output 0 of <p pgmB>, output 1 of <p pgmA> becoming output 1 of
* <p pgmB>, and so on.  If <p pgmB> has more outputs than <p pgmA>,
* the unpatched outputs will be grounded.  If <p pgmA> has more outputs
* than <p pgmB>, the remaining outputs will be patched starting from
* <p pgmB> output 0 (that is, wrapped modulo the number of outputs of
* <p pgmB>).  The inputs of <p pgmB> will now be fed by the outputs
* of <p pgmA> according to the same rules above.
*
* @parm FXPGMID	| pgmA	| Specifies program to patch after.
* @parm FXPGMID	| pgmB	| Specifies program to insert.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmA> or <p pgmB> is invalid.
* @flag FXERROR_PROGRAMS_ON_DIFFERENT_CHIPS |
*								  If <p pgmA> and <p pgmB> are not on
*								  the same chip.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchInsertAfter( FXPGMID, FXPGMID );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchAddAfter |
*
* This function patches the outputs of <p pgmA> to the inputs of
* <p pgmB> without removing any patches from <p pgmA>.  Wrapping is
* performed in the same way as <f fxPatchInsertAfter>.
*
* @parm FXPGMID	| pgmA	| Specifies program to patch after.
* @parm FXPGMID	| pgmB	| Specifies program to add.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmA> or <p pgmB> is invalid.
* @flag FXERROR_PROGRAMS_ON_DIFFERENT_CHIPS |
*								  If <p pgmA> and <p pgmB> are not on
*								  the same chip.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchAddAfter( FXPGMID, FXPGMID );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchBypass |
*
* This function performs a "patch-around" <p pgmID>.  All inputs of
* <p pgmID> will be patched to become the input of the next program in
* the chain.  That is, input 0 of <p pgmID> will now patch to output
* 0 of <p pgmID>, input 1 of <p pgmID> will now patch to output 1 of
* <p pgmID>, and so on.  If <p pgmID> has more inputs than outputs,
* then the remaining inputs will be grounded.  If <p pgmID> has more
* outputs than inputs, the remaining outputs will be patched logically
* modulo the number of inputs (wrapped).
*
* @parm FXPGMID	| pgmID	| Specifies program to bypass.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchBypass( FXPGMID );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchReplace |
*
* This function essentially replaces <p pgmA> with <p pgmB> by
* grounding all of <p pgmA>'s inputs and outputs and reconnecting
* them to <p pgmB> in the same logical order.  If <p pgmA> has more
* inputs than <p pgmB>, then the remaining inputs will be grounded.
* If <p pgmB> has more inputs than <p pgmA>, then the inputs of
* will be wrapped modulo the number of inputs of <p pgmA>.  The outputs
* are patched according to the same rules.
*
* @parm FXPGMID	| pgmA	| Specifies program to replace.
* @parm FXPGMID	| pgmB	| Specifies program to insert.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmA> or <p pgmB> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchReplace( FXPGMID, FXPGMID );


/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchQueryOutput |
*
* This function returns the IDs and channel numbers of programs 
* attached to output <p nOutChan> of <p pgmID>.
*
* @parm FXPGMID		| pgmID		| Specifies program FXID.
* @parm USHORT     | nOutChan  | Specifies output channel.
* @parm USHORT *   | nChannels | Returns number of channels connected.
* @parm FXPGMID		| pgmIn[]	| Specifies array for IDs for connected programs.
* @parm DWORD	| pgmInSize	| Specifies max size of <p pgmIn> array (use 256).
* @parm USHORT     | chanIn[]  | Specifies array for connected channel numbers.
* @parm DWORD	| chanInSize| Specifies size of <p chanIn> array (use 256).
*
* @rdesc This function returns one of the following:
* @flag	FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchQueryOutput( FXPGMID, USHORT, USHORT * /* IO */,
									   FXPGMID * /* ARRAY 256 */, DWORD,
                                       USHORT * /* ARRAY 256 */, DWORD );

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchQueryInput |
*
* This function returns the FXID and channel of the program attached to 
* input <p nInpChan> of <p pgmID>.
*
* @parm FXPGMID	 | pgmID	| Specifies program FXID.
* @parm USHORT  | nInpChan | Specifies input channel.
* @parm FXPGMID * | pgmOut	| Returns FXID of connected program (0 if none)
* @parm USHORT *| chanOut  | Returns channel number of <p pgmOut>.
*
* @rdesc This function returns one of the following:
* @flag	FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchQueryInput( FXPGMID, USHORT, FXPGMID * /* IO */, USHORT * /* IO */);

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxPatchGetNumIO |
*
* This function finds the number of logical inputs and outputs of <p pgmID>.
*
* @parm FXPGMID		| pgmID		| Specifies program FXID.
* @parm USHORT *   | nInpChan  | Specifies buffer for number of input channels.
* @parm USHORT *   | nOutChan  | Specifies buffer for number of output channels.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPatchGetNumIO( FXPGMID, USHORT * /* IO */, USHORT * /* IO */);

/*****************************************************************
*
* @doc EXTERNAL
* @func BOOL | fxPgmIsLoadable |
*	
* This function determines whether there are enough resources available in
* a partition to load a program.
*
* @parm FXPARTID	| partitionID	| Specifies partition to load program.
* @parm CK *| pgmTxt		| Specifies pointer to RIFX program text.
*
* @rdesc This function returns TRUE if the program can load in the
* current state of the partition, otherwise FALSE is returned.
*				
******************************************************************
*/
EMUCTYPE BOOL EMUAPIEXPORT fxPgmIsLoadable( FXPARTID, CK * /* RIFX */ );

/*****************************************************************
*
* @doc EXTERNAL
* @func BOOL | fxPgmCanReplace |
*	
* This function determines whether there would be enough resources
* available to load a program if various other programs were 
* unloaded prior to loading.
*
* @parm CK *	| pgmTxt		| Specifies pointer to RIFX program text.
* @parm FXPGMID		| pgmIDList[]	| Specifies array of program IDs that would be unloaded.
* @parm DWORD	| pgmIDSize		| Specifies maximum size of array.
*
* @comm <p pgmIDList[]> is terminated by placing a zero FXID at the end
* of the list.  All program IDs must be in the same partition or this
* function will return FALSE.
*
* @rdesc This function returns TRUE if the program can load with the
* list of existing programs unloaded, or FALSE otherwise.
*				
******************************************************************
*/
EMUCTYPE BOOL EMUAPIEXPORT fxPgmCanReplace( CK * /* RIFX */, FXPGMID * /* ARRAY 256 */, DWORD );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmQueryRequiredResources |
*	
* This function determines the resource requirements of a program.
*
* @parm FXPARTID	  | partitionID	| Specifies partition to try.
* @parm CK *		  | pgmTxt		| Specifies pointer to RIFX program text.
* @parm PGMRSRCQUERY *| pRsrcQuery  | Specifies address of structure to fill. 
*									  Note: instrAddr, tramAddr, xtramAddr, and tableAddr are undefined
*
* @rdesc This function returns FXERROR_INVALID_ID if the program ID, partition ID, or
* program text are invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmQueryRequiredResources( FXPARTID, CK * /* RIFX */, 
														    PGMRSRCQUERY * /* IO */ );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXPARTID | fxPgmWhichPartition |
*
* This function determines which partition a program is loaded in.
*
* @parm FXPGMID	|	pgmID	| Specifies program ID.
*
* @rdesc Returns partition ID or 0, if <p pgmID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXPARTID EMUAPIEXPORT fxPgmWhichPartition( FXPGMID );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXCHIPID | fxPgmWhichChip |
*
* This function determines which chip a program is loaded on.
*
* @parm FXPGMID	|	pgmID	| Specifies program ID.
*
* @rdesc Returns chip ID or 0, if <p pgmID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXCHIPID EMUAPIEXPORT fxPgmWhichChip( FXPGMID );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmLoad |
*	
* This function loads a program, without starting it, into the 
* specified partition.  Fixup and initialization are performed by
* this procedure.  Upon successful load, a new program FXID is
* generated.
*
* @parm FXPARTID| partitionID	| Specifies partition to load program into.
* @parm FXPGMID *| pgmID			| Specifies buffer to store new program FXID.
* @parm CK *	| pgmTxt		| Specifies a pointer to the RIFX program text.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_PGMTEXT		| If <p pgmTxt> contains invalid program text.
* @flag FXERROR_OUT_OF_MEMORY		| If not enough host memory to complete operation.
* @flag FXERROR_OUT_OF_GENERAL_GPRS	| If not enough available general GPRs.
* @flag FXERROR_OUT_OF_TRAM_GPRS	| If not enough available TRAM buffers.
* @flag FXERROR_OUT_OF_XTRAM_GPRS	| If not enough available XTRAM buffers.
* @flag FXERROR_OUT_OF_INSTR_SPACE	| If not enough available contiguous intruction space.
* @flag FXERROR_OUT_OF_TRAM_SPACE	| If not enough available contiguous TRAM space.
* @flag FXERROR_OUT_OF_XTRAM_SPACE	| If not enough available contiguous XTRAM space.
* @flag FXERROR_OUT_OF_TABLE_SPACE	| If not enough available contiguous table space.
* @flag FXERROR_NO_FREE_INODES		| If not enough memory to complete operation.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmLoad( FXPARTID, FXPGMID * /* IO */, CK * /* RIFX */ );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmStart |
*	
* This function starts a stopped, or freshly loaded program.
*
* @parm FXPGMID	| pgmID	| Specifies FXID of program to start.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_ID			| If <p pgmID> is not valid.
* @flag FXERROR_PROGRAM_RUNNING		| If program is not stopped.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmStart( FXPGMID );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmStop |
*	
* This function stops a currently running program.  It does not
* re-patch; i.e. all inputs will be grounded, and all outputs will
* replicate the last sample.  It is a good idea to ramp all outputs
* to silence before calling this function.
*
* @parm FXPGMID	| pgmID	| Specifies FXID of program to stop.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_ID			| If <p pgmID> is not valid.
* @flag FXERROR_PROGRAM_STOPPED		| If program is not running.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmStop( FXPGMID );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmUnload |
*	
* This function unloads a loaded, but stopped program.  All resources
* are deallocated, and all applets are signaled to shut down.  Any
* GPRs being ramped or interpolated will stop.
*
* @parm FXPGMID	| pgmID	| Specifies FXID of program to stop.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_ID			| If <p pgmID> is not valid.
* @flag FXERROR_PROGRAM_RUNNING		| If program is running.
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmUnload( FXPGMID );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmRampdown |
*
* This function will initiate the ramping of the input, output, and/or
* mute/dry scalers as specified in the program text.  This should be
* called prior to <f fxPgmStop()>.  However, before a program is 
* restarted with <f fxPgmStart()>, a call should be made to
* <f fxPgmStopRampdown()>, or the scalers may continue to ramp to
* dry or mute.
*
* @parm FXPGMID	| pgmID	| Specifies FXID of program to ramp.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_ID			| If <p pgmID> is not valid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmRampdown( FXPGMID );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmRampup |
*
* This function will initiate the ramping of the input, output, and/or
* mute/dry scalers as specified in the program text.  This should be
* called immediately after <f fxPgmStart()>.
*
* @parm FXPGMID	| pgmID	| Specifies FXID of program to ramp.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_ID			| If <p pgmID> is not valid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmRampup( FXPGMID );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmInputMix |
*
* This function ramps the input mix coefficient for a program.
*
* @parm FXPGMID	| pgmID	  | Specifies ID of program to ramp.
* @parm ULONG	| ulValue | Specifies the value to which to ramp.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_ID			| If <p pgmID> is not valid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmInputMix( FXPGMID, ULONG );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmOutputMix |
*
* This function ramps the output mix coefficient for a program.
*
* @parm FXPGMID	| pgmID	  | Specifies ID of program to ramp.
* @parm ULONG	| ulValue | Specifies the value to which to ramp.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_ID			| If <p pgmID> is not valid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmOutputMix( FXPGMID, ULONG );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmWetMix |
*
* This function ramps the wet/dry mix coefficient for a program.
*
* @parm FXPGMID	| pgmID	  | Specifies ID of program to ramp.
* @parm ULONG	| ulValue | Specifies the value to which to ramp.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_ID			| If <p pgmID> is not valid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmWetMix( FXPGMID, ULONG );

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmCreatePortID |
*
* This function creates a port FXID to be used in program patching
* which is an abstraction of a subset of physical ports.
*
* @parm FXCHIPID| chipID	| Specifies chip FXID.
* @parm ULONG	| ulInMask	| Specifies input port map (see comment).
* @parm ULONG	| ulOutMask	| Specifies output port map (see comment).
* @parm FXPGMID *| portID	| Specifies return buffer for new port ID.
*
* @comm <p ulInMask> and <p ulOutMask> are 32-bit bitmaps which describe
* which ports are to be allocated: LSB = physical port 0, MSB = pysical port 31.  
* Logically, ports will be enumerated from LSB to MSB, skipping over any zero
* bits in the map.  For example, a bitmap of  ...010010, will map this way:
* logical port 0 = physical port 1, and logical port 1 = physical port 4.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_OUT_OF_MEMORY		| If not enough memory to complete operation.
* @flag FXERROR_INVALID_ID			| If <p chipID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmCreatePortID( FXCHIPID, ULONG, ULONG, FXPGMID * /* IO */);

/*****************************************************************
*
* @doc EXTERNAL
* @func FXSTATUS | fxPgmFreePortID |
*
* This function frees a previously created port ID, and grounds all
* channels connected to it.
*
* @parm FXPGMID	| portID	| Specifies port ID to free.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR			| If successful.
* @flag FXERROR_INVALID_ID			| If <p portID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxPgmFreePortID( FXPGMID );

/*****************************************************************
*
* @doc EXTERNAL
* @func BOOL | fxPgmIsRunning |
*	
* This function determines if a program is started or stopped.
*
* @parm FXPGMID	|	pgmID	| Specifies program ID.
*
* @rdesc Returns TRUE if it is running, FALSE if it is stopped.
*				
******************************************************************
*/
EMUCTYPE BOOL EMUAPIEXPORT fxPgmIsRunning( FXPGMID );

/* Note:  This function is available in debug versions ONLY.
 */
EMUCTYPE void EMUAPIEXPORT fxDisasm(ULONG);

#endif
