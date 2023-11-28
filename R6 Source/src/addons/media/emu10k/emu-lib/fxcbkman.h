/* @doc INTERNAL */
/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
*
* @module fxcbkman.h - Callback Manager API |
* 
* This file contains definitions, types, macros, and
* the API that pertain to user code in support of the
* FX8010 callback manager.
*
*******************************************************************
*/

#ifndef _FXCBKMAN_H
#define _FXCBKMAN_H

#include "fxprmapi.h"

/*****************************************************************
* 
* @doc OSAPI
* @func void | fxCallbackInitialize |
*	
* This function is called once by the system at startup.
*
* @comm This is not a user-level function.  It should only be
* called by the system at initialization time.
*				
******************************************************************
*/
void fxCallbackInitialize();

/*****************************************************************
* 
* @doc OSAPI
* @func FXSTATUS | fxCallbackInitChip |
*	
* This function is called once for each chip by the 
* system at startup
*
* @parm ULONG	| ulChipHandle | Specifies opaque hardware handle for chip.
*
* @comm	This is not a user-level function.  It should only be
* called by the system at initialization time.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_OUT_OF_MEMORY	| If not enough host memory to complete operation.
*				
******************************************************************
*/
FXSTATUS fxCallbackInitChip( ULONG );

/*****************************************************************
* 
* @doc OSAPI
* @func FXSTATUS | fxCallbackFreeChip |
*	
* This function is called to clean up a chip.
*
* @parm ULONG	| ulChipHandle | Specifies opaque hardware handle for chip.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
*				
******************************************************************
*/
FXSTATUS fxCallbackFreeChip( ULONG );

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXSTATUS | fxCallbackShutdown |
*	
* This function will trigger the instantiation of any
* registered callback function for the given program,
* and then unregister all callback functions
* associated with that program.
*
* @parm FXID |  pgmID | Specifies the FXID of the program to shut down.
*
* @comm Users should not explicitly call this function as
* it gets executed during normal program shutdown.
*
* Return Codes: FXERROR_NO_ERROR
*				
******************************************************************
*/
FXSTATUS fxCallbackShutdown( FXID );

#endif

