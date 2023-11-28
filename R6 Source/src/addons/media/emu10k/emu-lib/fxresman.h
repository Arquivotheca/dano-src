/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
* @doc INTERNAL
* @module fxresman.h - Resource Manager API |
*
* This file contains definitions, types, macros, and
* the API that pertain to user code in support of the
* FX8010 resource manager.
*
*******************************************************************
*/
/* @doc EXTERNAL */

#ifndef _FXRESMAN_H
#define _FXRESMAN_H

#include "fxmgrapi.h"
#include "fxprmapi.h"

/* Structures, Types, and Enumerations |
 *
 * This section describes the public structures, types, macros, and 
 * enumerations used by the Resource Manager.
 */

/*****************************************************************
* 
* Resource Manager API
*
******************************************************************
*/

/*****************************************************************
* 
* Startup Initialization Functions |
*
* This section describes the API used by the Operating System to
* initialize the Resource Manager.
*
******************************************************************
*/
/*****************************************************************
*
* @doc OSAPI 
* @func FXSTATUS | fxRsrcInitialize |
*	
* This function initializes all of the data structures
* used by the resource manager.
*
* @comm This function MUST be called before any other
* function is called.  It should be called during
* a system startup sequence and need not be called
* again.
*
* @comm:(INTERNAL) This function inits the <t memMap[]> array by setting
* all ownerIDs to UNALLOCED_ID, and marking all
* inodes as free.  If dynamic allocation is not being
* done, all structs are chained into their respective
* free lists.
* 
* @rdesc Always returns FXERROR_NO_ERROR.
*				
******************************************************************
*/
FXSTATUS fxRsrcInitialize();

/*****************************************************************
* 
* @doc OSAPI
* @func FXSTATUS | fxRsrcInitChip |
*	
* This function instantiates the management of resources for a chip.
*
* @parm FXID *	| chipID		| Pointer to FXID to assign.
* @parm char *	| chipRevisionID| Chip identifier (see comment).
* @parm ULONG   | ulRevReg		| Chip revision.
* @parm ULONG	| ulChipHandle	| Specifies opaque hardware handle.
* @parm ULONG	| ulXTRAM		| Specifies total size of XTRAM space.
*
* @comm	This function is called once per chip.  The "chip
* identifier" is (currently) either "EMU8010_A" or
* "RCHIP_A".
*
* This function MUST be called before any other
* operation is performed on a chip. It should be 
* called during a system startup sequence and need not
* be called again.
*
* @rdesc Returns one of the following: 
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p chipRevisionID> is invalid.
* @flag FXERROR_OUT_OF_MEMORY	| If insufficient memory is available.
*
******************************************************************
*/
FXSTATUS fxRsrcInitChip( FXID *, char *, ULONG, ULONG, ULONG );

/*****************************************************************
* 
* @doc OSAPI
* @func FXSTATUS | fxRsrcFreeChip |
*	
* This function frees the CHIPINFOSTRUCT.
*
* @parm FXID 	| chipID		| FXID to clear.
*
* @rdesc Returns one of the following: 
* @flag FXERROR_NO_ERROR		| If successful.
*
******************************************************************
*/
FXSTATUS fxRsrcFreeChip( FXID );

/*****************************************************************
*
* Dynamic Initialization Functions
*
******************************************************************
*/

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXSTATUS | fxRsrcDestroy |
*	
* Deallocates all chip resources in preparation for
* complete reinitialization.
*
* @parm FXID	| chipID | Specifies the chip FXID of the chip to reinitialize.
*
* @comm This function ONLY deallocates resources, it does
* not remove patches or parameter management for the
* chip.  These functions are separate and must be
* called before <f fxRsrcDestroy()>.
*
* @devnote:(INTERNAL) This function violates the loose-cohesion property
* by making a call to the parameter manager.  Sorry.
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*
******************************************************************
*/
FXSTATUS fxRsrcDestroy( FXID );

/*****************************************************************
*
*  Host Resource Initialization Functions
*
******************************************************************
*/

/*****************************************************************
* 
* @doc SUPERUSER
* @func	FXSTATUS | fxRsrcNewPgm |
*	
* Allocates host memory for a new program.
*
* @parm FXID	| partitionID | Specifies partition to place program in.
* @parm FXID *| rsrcID	  | Pointer to new program resource FXID.
*
* @comm:(INTERNAL) If FX_DYNAMIC is set, simply FX_MALLOC the
* struct, otherwise, pull from the free list.
*
* @rdesc Returns one of the following:
* @flag FXERROR_NO_ERROR	  | If successful.
* @flag	FXERROR_OUT_OF_MEMORY | If not enough memory to complete operation.
*
* @xref <f fxRsrcDeletePgm>
*
******************************************************************
*/
FXSTATUS fxRsrcNewPgm( FXID, FXID * );

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXSTATUS | fxRsrcDeletePgm |
*	
* Deallocates host memory reserved for a program by
* <f fxRsrcNewPgm()>.
*
* @parm FXID	| rsrcID | Specfies resource FXID of program to delete.
*
* @comm All references to <p rsrcID> after this call will be
* invalid.  If <f fxRsrcReleasePgm()> has not been called
* prior to this call, results will be undefined.
*
* @rdesc Returns one of the following:
*
* @flag FXERROR_NO_ERROR	| If successful.
* @flag FXERROR_INVALID_ID	| If <p rsrcID> is invalid.
*
******************************************************************
*/
FXSTATUS fxRsrcDeletePgm( FXID );

/*****************************************************************
*
*  DSP Resource Allocation Functions
*
******************************************************************
*/

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXSTATUS | fxRsrcAllocPgm |
*	
* Reserves partition resources for a program.
*
* @parm FXID		 | rsrcID  | Specifies resource FXID of progam requesting partition resources.
* @parm PGMRSRC *| pPgmRsrc| Pointer to resource definition structure.
*
* @comm	The program must have been previously allocated in
* the host with a call to <f fxRsrcNewPgm()>.
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
* @xref <f fxRsrcNewPgm> <f fxRsrcReleasePgm> <f fxRsrcDeletePgm>
*       <t PGMRSRC>
******************************************************************
*/
FXSTATUS fxRsrcAllocPgm( FXID, PGMRSRC * );

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXSTATUS | fxRsrcReleasePgm |
*	
* Releases the resources reserved by a program back
* to the partition.
*
* @parm FXID	| pgmID | Specifies resource FXID of program to release.
*
* @comm This call does not deallocate host resources, only
* DSP resources.  This allows a new program to be
* loaded into an existing partition chain.
*
* @rdesc This funciton always returns FXERROR_NO_ERROR.
*
* @xref <f fxRsrcAllocPgm> <f fxRsrcNewPgm> <f fxRsrcDeletePgm>
*
******************************************************************
*/
FXSTATUS fxRsrcReleasePgm( FXID );

/*****************************************************************
*
*  Resource Queries 
*
******************************************************************
*/

/*****************************************************************
* 
* @doc EXTERNAL
* @func FXSTATUS | fxRsrcQueryPgm |
*	
* Gathers information about physical locations and
* GPR usage for a program.
*
* @parm FXID			   | rsrcID			| Specifies resource FXID of program to query.
* @parm PGMRSRCQUERY * | pPgmRsrcQuery	| Pointer to query structure.	
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*
* @xref <t PGMRSRCQUERY>
*
******************************************************************
*/
FXSTATUS fxRsrcQueryPgm( FXID, PGMRSRCQUERY * );

/*****************************************************************
* 
* @doc SUPERUSER
* @func ADDR | fxRsrcMapVirtualtoPhysicalGPR |
*	
* Translates a program's virtual GPR address to an
* FX8010 physical address.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
* @parm ADDR	| addrVirt	| Virtual GPR address.
*
* @rdesc This function returns the corresponding physical address or
* ADDRERROR if not a valid virtual address.
*
******************************************************************
*/
ADDR fxRsrcMapVirtualToPhysicalGPR( FXID, ADDR );

/*****************************************************************
* 
* @doc SUPERUSER
* @func ADDR | fxRsrcMapPhysicalToVirtualGPR |
*	
* Translates a program's physical GPR address to a
* virtual address.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
* @parm ADDR	| addrPhys	| Physical GPR address.
*
* @rdesc This function returns the corresponding virtual address or
* ADDRERROR if not in the virtual address space of <p rsrcID>.
*
******************************************************************
*/
ADDR fxRsrcMapPhysicalToVirtualGPR( FXID, ADDR );

/*****************************************************************
* 
* @doc SUPERUSER
* @func ADDR | fxRsrcMapVirtualtoPhysicalInstr  |
*	
* Translates a program's virtual instruction address to an
* FX8010 physical address.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
* @parm ADDR	| addrVirt	| Virtual instruction address.
*
* @rdesc This function returns the corresponding physical address or
* ADDRERROR if not a virtual address.
*
******************************************************************
*/
ADDR fxRsrcMapVirtualToPhysicalInstr( FXID, ADDR );

/*****************************************************************
* 
* @doc SUPERUSER
* @func ADDR | fxRsrcMapPysicalToVirtualInstr |
*	
* Translates a physical instruction address to a program's
* virtual address.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
* @parm ADDR	| addrPhys	| Physical instruction address.
*
* @rdesc This function returns the corresponding virtual address or
* ADDRERROR if not in the virtual address space of <p rsrcID>.
*
******************************************************************
*/
ADDR fxRsrcMapPhysicalToVirtualInstr( FXID, ADDR );

/*****************************************************************
* 
* @doc SUPERUSER
* @func BOOL | fxRsrcPgmIsLoadable |
*	
* Determines if a program is loadable into a specific partition.
*
* @parm	FXID				| partitionID	| Specifies FXID of partition.
* @parm PGMRSRCQUERY *	| pPgmRsrcQuery	| Pointer to program resource query structure.
*
* @rdesc This function returns:
* @flag TRUE  | If program can load into existing partition.
* @flag FALSE | If program cannot load without removing running programs.
*
* @xref <t PGMRSRCQUERY>
*
******************************************************************
*/
BOOL fxRsrcPgmIsLoadable( FXID, PGMRSRCQUERY * );

/*****************************************************************
* 
* @doc SUPERUSER
* @func BOOL | fxRsrcPgmCanReplace |
*	
* Determines if a program can be loaded into a partition if the specified
* resources are released.
*
* @parm	PGMRSRCQUERY *	| pPgmRsrcQuery	| Pointer to program resource query structure.
* @parm FXID				| pid[]			| NULL terminated array of resource IDs.
*
* @rdesc This function returns:
* @flag TRUE	| If program can load if the listed programs are removed.
* @flag FALSE	| If program cannot load or if all programs are not in the same partition.
*
* @xref <t PGMRSRCQUERY>
*
******************************************************************
*/
BOOL fxRsrcPgmCanReplace( PGMRSRCQUERY *, FXID * );

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXID | fxRsrcWhichPartition |
*	
* Returns which partition a program is in.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
*
* @rdesc This function returns the partition FXID the program resides on.
*
******************************************************************
*/
FXID fxRsrcWhichPartition( FXID );

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXID | fxRsrcGetPortID |
*	
* Returns which port a program can connect to.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
*
* @rdesc This function returns the port FXID of the chip
*
******************************************************************
*/
FXID fxRsrcGetPortID( FXID );

/*****************************************************************
* 
* @doc SUPERUSER
* @func ULONG | fxRsrcGetChipHandle |
*	
* Returns the opaque handle of the chip a program resides on.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
*
* @rdesc This function returns the handle of the chip the program resides on.
*
******************************************************************
*/
ULONG fxRsrcGetChipHandle( FXID );

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXID | fxRsrcWhichChip |
*	
* Returns which chip a program is running on.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
*
* @rdesc This function returns the chip FXID the 
* program resides on.
*
******************************************************************
*/
FXID fxRsrcWhichChip( FXID );

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXID | fxRsrcGetInterpolatorID |
*	
* Returns the interpolator FXID the program should use.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
*
* @rdesc This function returns the interpolator FXID of the chip the
* program resides on.
*
******************************************************************
*/
FXID fxRsrcGetInterpolatorID( FXID );

/*****************************************************************
* 
* @doc SUPERUSER
* @func ADDR | fxRsrcMapEnumToPysicalFlags |
*	
* Translates the enumerated TRAM engine flags to physical flags.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
* @parm BYTE	| bEnum		| TRAM engine mode enumeration.
*
* @rdesc This function returns the physical flag values.
*
******************************************************************
*/
BYTE fxRsrcMapEnumToPhysicalFlags( FXID, BYTE );

/*****************************************************************
* 
* @doc SUPERUSER
* @func ADDR | fxRsrcMapPysicalToEnumFlags |
*	
* Translates the physical TRAM engine flags to an enumeration.
*
* @parm	FXID		| rsrcID	| Specifies resource FXID of program.
* @parm BYTE	| bPhys		| TRAM engine mode flag values.
*
* @rdesc This function returns the flag enumeration.
*
******************************************************************
*/
BYTE fxRsrcMapPhysicalToEnumFlags( FXID, BYTE );

/*****************************************************************
* 
* @doc SUPERUSER
* @func BOOL | fxRsrcIsTRAMBuf |
*	
* Determines whether an address is a valid TRAM or XTRAM buffer.
*
* @parm	FXID		 | rsrcID	| Specifies resource FXID of program.
* @parm OPERAND	*| tramAddr	| As input, specifies the address to test. As
*	output, contains the physical address of address GPR.
* @parm ULONG *	 | offs		| Outputs the physical offset of the program's
*	TRAM or XTRAM buffer.
*
* @comm <p tramAddr> can be either the data or address GPR for a TRAM
* or XTRAM buffer.  The return value will be modified to be the physical
* address of the address GPR in any case.  <p tramAddr> can be either
* virtual or physical.
*
* @rdesc This function returns:
* @flag TRUE	| If the specified GPR is an X/TRAM data or address GPR.
* @flag FALSE	| If the specified GPR is not a valid X/TRAM buffer GPR.
*
******************************************************************
*/
BOOL fxRsrcIsTRAMBuf( FXID, OPERAND *, ULONG * );

/*****************************************************************
* 
* @doc SUPERUSER
* @func BOOL | fxRsrcMaskPorts |
*	
* Determines if all requested ports are free.
*
* @parm FXID		| chipID	| Specifies chip FXID.
* @parm ULONG	| ulInMask	| Specifies input port mask.
* @parm ULONG	| ulOutMask	| Specifies output port mask.
*
* @rdesc This function returns TRUE if all requested ports are
* available, or FALSE if any requested port is allocated.
*
******************************************************************
*/
BOOL fxRsrcMaskPorts( FXID, ULONG, ULONG );

/*****************************************************************
* 
* @doc SUPERUSER
* @func void | fxRsrcSetPortMask |
*	
* Marks ports as allocated.
*
* @parm FXID		| chipID	| Specifies chip FXID.
* @parm ULONG	| ulInMask	| Specifies input port mask.
* @parm ULONG	| ulOutMask	| Specifies output port mask.
*
******************************************************************
*/
void fxRsrcSetPortMask( FXID, ULONG, ULONG );

/*****************************************************************
* 
* @doc SUPERUSER
* @func void | fxRsrcClearPortMask |
*	
* Marks ports as unallocated.
*
* @parm FXID		| chipID	| Specifies chip FXID.
* @parm ULONG	| ulInMask	| Specifies input port mask.
* @parm ULONG	| ulOutMask	| Specifies output port mask.
*
******************************************************************
*/
void fxRsrcClearPortMask( FXID, ULONG, ULONG );

/*****************************************************************
* 
* @doc SUPERUSER
* @func char * | fxRsrcGetChipRevisionID |
*	
* Get string identifier of chip.
*
* @parm FXID	| chipID	| Specifies chip FXID.
* @parm ULONG *	| pulRevReg	| Return value of chip revision number.
*
* @rdesc Returns pointer to string identifier.
*
******************************************************************
*/
char * fxRsrcGetChipRevisionID( FXID, ULONG * );

/*****************************************************************
* 
* @doc SUPERUSER
* @func BOOL | fxRsrcValidPartitionID |
*	
* Validate partition ID.
*
* @parm FXID	| partitionID	| Specifies partition FXID.
*
* @rdesc Returns TRUE if valid partition, FALSE otherwise.
*
******************************************************************
*/
BOOL fxRsrcValidPartitionID( FXID );

/*****************************************************************
* 
* @doc SUPERUSER
* @func BOOL | fxRsrcValidChipID |
*	
* Validate chip ID.
*
* @parm FXID	| chipID	| Specifies chip FXID.
*
* @rdesc Returns TRUE if valid chip ID, FALSE otherwise.
*
******************************************************************
*/
BOOL fxRsrcValidChipID( FXID );

#endif

