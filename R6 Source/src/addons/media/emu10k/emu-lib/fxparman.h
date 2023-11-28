/* @doc */
/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
* 
* @module fxresman.h - Resource Manager API |
*
* This file contains definitions, types, macros, and
* the API that pertain to user code in support of the
* FX8010 resource manager.
*
*******************************************************************
*/

#ifndef _FXPARMAN_H
#define _FXPARMAN_H

#include "fxprmapi.h"

/*****************************************************************
* 
* @doc OSAPI
* @func FXSTATUS | fxParamInitialize |
*	
* This function is called at system startup by the
* OS and initializes all static structures.
*
* @comm This is not a user-level function.  It should only be called 
* by the OS once at startup.
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*				
******************************************************************
*/
FXSTATUS fxParamInitialize();

/*****************************************************************
* 
* @doc OSAPI
* @func FXSTATUS | fxParamInitChip |
*	
* This function is called as each chip is discovered
* by the OS.  It initializes chip structures and
* allocates interpolators.
*
* @comm This is not a user-level function.  It should only be called
* once by the OS for each chip.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If sucessful.
* @flag FXERROR_OUT_OF_MEMORY	| If there is not enough host memory to complete operation.
*				
******************************************************************
*/
FXSTATUS fxParamInitChip();

/*****************************************************************
* 
* @doc SUPERUSER
* @func FXSTATUS | fxParamAllocInterpolators |
*	
* This function allocates <p nInterpolators> rampers for use by a chip.
*
* @parm FXID *	| interpolatorID| Specifies output buffer for opaque interpolator FXID.
* @parm int		| nInterpolators| Specifies number of rampers to allocate.
* @parm ADDR	| gprAddr		| Specifies physical address of first consecutive general GPR.
* @parm ADDR	| instrAddr		| Specifies physical address of first consecutive instruction.
*
* @comm This is not a user-level function.  It is called by the
* resource manager to set up the ramper data structures.  Each ramper
* reserves one instruction and two GPRs, and it is assumed that these
* consecutive addresses have already been reserved for this purpose.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_OUT_OF_MEMORY	| If not enough memory to complete operation.				
*
* @xref <f fxParamFreeInterpolators>
*
******************************************************************
*/
FXSTATUS fxParamAllocInterpolators( FXID *, int, ADDR, ADDR );

/*****************************************************************
*
* @doc SUPERUSER 
* @func FXSTATUS | fxParamFreeInterpolators |
*	
* This function frees the rampers allocated by <f fxParamAllocInterpolators()>.
*
* @parm ULONG	| ulChipHandle	| Specifies OS-defined hardware handle.
* @parm FXID		| interpolatorID| Specifies interpolator FXID.
*
* @comm This is not a user-level function.  It is called by the
* resource manager to free the ramper data structures.  The resource
* manager is then free to use the instructions and GPRs for other
* purposes.
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*
* @xref <f fxParamAllocInterpolators>
*				
******************************************************************
*/
FXSTATUS fxParamFreeInterpolators( ULONG, FXID );

#endif /* _FXPARMAN_H */

