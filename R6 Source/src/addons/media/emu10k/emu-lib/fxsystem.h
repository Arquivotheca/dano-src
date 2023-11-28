/* @doc OSAPI */
/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
*
* @module fxsystem.h - System Level API |
*
* This file contains definitions, types, macros, and
* the API that pertain to startup code in support of the
* FX8010.
*
*******************************************************************
*/

#include "fxdtatyp.h"

/*****************************************************************
* 
* @func void | fxSystemInitialize |
*	
* This function is called to initialize internal FX
* manager structures.
*
* @comm This function should only be called ONCE.
*				
******************************************************************
*/
EMUCTYPE void fxSystemInitialize( void );

/*****************************************************************
* 
* @func void | fxSystemDiscoverChip |
*	
* This function is to be called at the time of
* discovery for each chip.
*
* @parm char *	| zChipRev		| Specifies string identifier of chip revision.
* @parm ULONG	| ulRevReg		| Specifies incremental chip revision number.
* @parm ULONG	| ulBaseAddr	| Specifies physical address of fixed XTRAM.
* @parm ULONG	| ulXTRAM		| Specifies size of fixed XTRAM.
* @parm ULONG	| ulChipHandle	| Specifies handle to be used to identify this physical chip.
* @parm	FXID   *	| pChipID		| Specifies output: Chip FXID.
*
* @comm This function must be called ONLY once for each chip.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS fxSystemDiscoverChip( char *, ULONG, ULONG, ULONG, ULONG, FXID * );

/*****************************************************************
* 
* @func void | fxSystemDiscoverChip |
*	
* This function ungracefully stops everything on the specified chip
* and frees all of its resources.
*
* @parm	FXID | pChipID		| Specifies chip FXID.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS fxSystemUndiscoverChip( FXID, ULONG );
