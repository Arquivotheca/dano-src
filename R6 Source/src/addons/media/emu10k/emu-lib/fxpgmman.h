/* @doc */
/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
* 
* @module fxpgmman.h - Program Manager API |
*
* This file contains definitions, types, macros, and
* the API that pertain to user code in support of the
* FX8010 program manager.
*
*******************************************************************
*/
#ifndef _FXPGMMAN_H
#define _FXPGMMAN_H

#include "fxmgrapi.h"
#include "fxresman.h"

/*****************************************************************
*
* @doc OSAPI
* @func FXSTATUS | fxPgmInitialize |
*	
* This function is called by the operating system once at system
* startup.  It initializes program manager internal structures.
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*				
******************************************************************
*/
FXSTATUS fxPgmInitialize();

/*****************************************************************
*
* @doc OSAPI
* @func FXSTATUS | fxPgmInitChip |
*	
* This function is called by the operating system once for each
* chip that is discovered.  It initializes chip-specific internal
* data structures.
*
* @parm FXID  | chipID		 | Specifies chip FXID.
* @parm ULONG | ulChipHandle | Specifies chip handle.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_OUT_OF_MEMORY	| If not enough memory to complete operation.
*				
******************************************************************
*/
FXSTATUS fxPgmInitChip( FXID, ULONG );

/*****************************************************************
*
* @doc OSAPI
* @func FXSTATUS | fxPgmFreeChip |
*
* Frees chip structures.	
*
* @parm FXID	  | chipID		 | Specifies chip FXID.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
*				
******************************************************************
*/
FXSTATUS fxPgmFreeChip( FXID );

/*****************************************************************
*
* @doc SUPERUSER
* @func ULONG | fxPgmGetChipHandle |
*	
* This function returns the hardware handle of the chip a program
* is running on.
*
* @parm FXID	|	pgmID	| Specifies program FXID.
*
* @rdesc Returns opaque hardware chip handle.
*				
******************************************************************
*/
ULONG	 fxPgmGetChipHandle( FXID );

/*****************************************************************
*
* @doc SUPERUSER
* @func FXID | fxPgmGetRsrcID |
*
* This function returns the resource FXID of a program.
*
* @parm FXID	|	pgmID	| Specifies program FXID.
*
* @rdesc Returns resource FXID.
*				
******************************************************************
*/
FXID		 fxPgmGetRsrcID( FXID );

/*****************************************************************
*
* @doc SUPERUSER
* @func FXID | fxPgmGetInterpolatorID |
*	
* This function returns the interpolator FXID of a program's chip.
*
* @parm FXID	|	pgmID	| Specifies program FXID.
*
* @rdesc Returns interpolator FXID.
*				
******************************************************************
*/
FXID		 fxPgmGetInterpolatorID( FXID );


/*****************************************************************
*
* @doc SUPERUSER
* @func ADDR | fxPgmMapVirtualToPhysicalGPR |
*	
* This function converts a virtual GPR address to a physical chip
* address.
*
* @parm FXID		| pgmID		| Specifies program FXID.
* @parm ADDR	| addrVirt	| Specifies 10-bit virtual address.
*
* @rdesc Returns physical GPR address.
*				
******************************************************************
*/
ADDR	 fxPgmMapVirtualToPhysicalGPR( FXID, ADDR );

/*****************************************************************
*
* @doc SUPERUSER
* @func ADDR | fxPgmMapPhysicalToVirtualGPR |
*	
* This function converts a physical GPR address to a virtual program
* address.
*
* @parm FXID		| pgmID		| Specifies program FXID.
* @parm ADDR	| addrPhys	| Specifies 10-bit physical address.
*
* @rdesc Returns virtual GPR address.
*				
******************************************************************
*/
ADDR	 fxPgmMapPhysicalToVirtualGPR( FXID, ADDR );

/*****************************************************************
*
* @doc SUPERUSER
* @func ADDR | fxPgmMapVirtualToPhysicalInstr |
*	
* This function converts a virtual instruction address to a physical chip
* address.
*
* @parm FXID		| pgmID		| Specifies program FXID.
* @parm ADDR	| addrVirt	| Specifies 10-bit virtual address.
*
* @rdesc Returns physical instruction address.
*				
******************************************************************
*/
ADDR	 fxPgmMapVirtualToPhysicalInstr( FXID, ADDR );

/*****************************************************************
*
* @doc SUPERUSER
* @func ADDR | fxPgmMapPhysicalToVirtualInstr |
*	
* This function converts a physical instruction address to a virtual program
* address.
*
* @parm FXID		| pgmID		| Specifies program FXID.
* @parm ADDR	| addrPhys	| Specifies 10-bit physical address.
*
* @rdesc Returns virtual instruction address.
*				
******************************************************************
*/
ADDR	 fxPgmMapPhysicalToVirtualInstr( FXID, ADDR );

/*****************************************************************
*
* @doc SUPERUSER
* @func ADDR | fxPgmGetIntVector |
*	
* This function returns the interrupt vector of a program.
*
* @parm FXID	|	pgmID	| Specifies program FXID.
*
* @rdesc Returns interrupt vector GPR.
*				
******************************************************************
*/
ADDR fxPgmGetIntVector( FXID );

/*****************************************************************
*
* @doc SUPERUSER
* @func FXID | fxPgmGetPatchID |
*	
* This function returns the patch FXID of a program.
*
* @parm FXID	|	pgmID	| Specifies program FXID.
*
* @rdesc Returns patch FXID.
*				
******************************************************************
*/
FXID fxPgmGetPatchID( FXID );

/*****************************************************************
*
* @doc SUPERUSER
* @func BOOL | fxPgmIsPortID |
*	
* This function determines if a program is a port abstraction.
*
* @parm FXID	|	pgmID	| Specifies program FXID.
*
* @rdesc Returns TRUE if it is a port abstration, FALSE otherwise.
*				
******************************************************************
*/
BOOL fxPgmIsPortID( FXID );

/*****************************************************************
*
* @doc SUPERUSER
* @func BOOL | fxPgmValidPgmID |
*	
* This function determines if a program ID is valid.
*
* @parm FXID	|	pgmID	| Specifies program FXID.
*
* @rdesc Returns TRUE if it is valid, FALSE otherwise.
*				
******************************************************************
*/
BOOL fxPgmValidPgmID( FXID );

/*****************************************************************
*
* @doc SUPERUSER
* @func void | fxPgmGetFirstInstr |
*	
* This function gets the saved first instruction from a stopped
* program.
*
* @parm FXID	|	pgmID	| Specifies program FXID.
* @parm USHORT *|	opcode	| Specifies ptr to opcode.
* @parm USHORT *|	opA		| Specifies ptr to A operand.
* @parm USHORT *|	opX		| Specifies ptr to X operand.
* @parm UHSORT *|	opY		| Specifies ptr to Y operand.
* @parm USHORT *|	opRes	| Specifies ptr to Result operand.
*
******************************************************************
*/
void fxPgmGetFirstInstr( FXID, USHORT *, USHORT *, USHORT *, USHORT *, USHORT * );

/*****************************************************************
*
* @doc SUPERUSER
* @func void | fxPgmWriteFirstInstr |
*	
* This function writes the saved first instruction of a stopped
* program.
*
* @parm FXID	|	pgmID	| Specifies program FXID.
* @parm USHORT  |	opcode	| Specifies opcode.
* @parm USHORT  |	opA		| Specifies A operand.
* @parm USHORT  |	opX		| Specifies X operand.
* @parm UHSORT  |	opY		| Specifies Y operand.
* @parm USHORT  |	opRes	| Specifies Result operand.
*
******************************************************************
*/
void fxPgmWriteFirstInstr( FXID, USHORT, USHORT, USHORT, USHORT, USHORT );

#endif

