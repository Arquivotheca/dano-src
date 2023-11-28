/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
* @doc INTERNAL
* @module fxpatman.h - Channel Patching API |
*
* This file contains definitions, types, macros, and
* the API that pertain to channel patching in support of the
* FX8010.
*
*******************************************************************
*/

/*****************************************************************
* 
* @doc OSAPI
* @func FXSTATUS | fxPatchInitialize |
*
* This function is called once at startup to initialize internal
* patch manager data structures.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
*				
******************************************************************
*/
FXSTATUS fxPatchInitialize();

/*****************************************************************
* 
* @doc OSAPI
* @func FXSTATUS | fxPatchInitChip |
*
* This function is called upon discovery once for each chip to
* initialize chip-specific patch manager data structures.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_OUT_OF_MEMORY	| If not enough memory to complete operation.
*				
******************************************************************
*/
FXSTATUS fxPatchInitChip();

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXSTATUS | fxPatchInitPgm |
*
* This function is called by the resource manager during the program
* load process.  It initializes the internal patch structures related
* to a program.
*
* @parm FXID	| pgmID		| Specifies program FXID.
* @parm USHORT | nInputs   | Specifies number of inputs.
* @parm USHORT | nOutputs  | Specifies number of outputs.
* @parm FXID *| patchID	| Specifies output buffer for patch FXID.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_OUT_OF_MEMORY	| If not enough memory to complete operation.
*				
******************************************************************
*/
FXSTATUS fxPatchInitPgm( FXID, USHORT, USHORT, FXID * );

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXSTATUS | fxPatchInitOutputStub |
*
* This function is called by the resource manager to initialize
* an output stub structure.
*
* @parm FXID		| patchID			| Specifies the patch FXID of a program.
* @parm USHORT     | logicalChannel    | Specifies the output channel number.
* @parm ADDR	| gprAddr			| Specifies the output GPR.
* @parm ADDR	| instrFirst		| Specifies the instruction address the output is written to.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_OUT_OF_MEMORY	| If not enough memory to complete operation.
*				
******************************************************************
*/
FXSTATUS fxPatchInitOutputStub( FXID, USHORT, ADDR, ADDR );

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXSTATUS | fxPatchInitInputStubInstr |
*
* This function is called by the program manager to initialize
* an input stub structure.
*
* @parm FXID		| patchID			| Specifies the patch FXID of a program.
* @parm USHORT     | logicalChannel    | Specifies the input channel number.
* @parm ADDR	| gprAddr			| Specifies the input GPR (ports only).
* @parm ADDR	| instrFirst		| Specifies the instruction address the input is read from.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_OUT_OF_MEMORY	| If not enough memory to complete operation.
*				
******************************************************************
*/
FXSTATUS fxPatchInitInputStub( FXID, USHORT, ADDR, ADDR );

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXSTATUS | fxPatchFreePgm |
*
* This function is called by the program manager in order to
* deallocate patch structures for a shut down program.
*
* @parm FXID	| pgmID	| Specifies program to free.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is invalid.
*				
******************************************************************
*/
FXSTATUS fxPatchFreePgm( FXID );


