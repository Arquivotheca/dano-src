/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
* @doc INTERNAL
* @module fxprmapi.h |
*
* This file contains general definitions, types, and
* macros that pertain to user code in support of the
* FX8010.
*
*******************************************************************
*/
/******************************************************************
*
* @doc APPLET
* @contents1 FX8010 Applet Programmer's Manual |
*
* This document defines the exported API needed to make parameter
* modifications and to allow reaction to FX8010 system events for
* effects applets.
*
*******************************************************************
*/

#ifndef _FXPRMAPI_H
#define _FXPRMAPI_H

#include "fxdtatyp.h"

#define VARIABLE_RAMPER 0

/*
 * @doc APPLET
 * @enum FXRATE |
 * This enumeration defines the time-constants for the ramping functions.
 * The times given show approximately how long it will take for the GPR to
 * reach 95% and 99.9% of its destination value.
 */
typedef unsigned long FXRATE;
#define    FXRAMP_1    0x40000000
#define    FXRAMP_2    0x20000000
#define    FXRAMP_3    0x10000000
#define    FXRAMP_5    0x04000000
#define    FXRAMP_6    0x02000000
#define    FXRAMP_7    0x01000000
#define    FXRAMP_8    0x00800000
#define    FXRAMP_16   0x00008000

/*
 * @doc APPLET
 * @enum FXEVENT | This enumeration defines the callback event types.
 */
typedef unsigned long FXEVENT;
#define    FXEVENT_DSPINTERRUPT 0x10000000L
#define    FXEVENT_SAMPLETIMER  0x20000000L
#define    FXEVENT_SHUTDOWN     0x30000000L

typedef ULONG CALLID;

/*
 * @doc APPLET
 * @struct FXINSTR |
 * This structure contains a complete instruction.  It is used by
 * <f fxParamReadInstruction> and <f fxParamWriteInstruction>.
 *
 * @field BYTE		| opcode	| Contains the opcode of the instruction.
 * @field OPERAND	| opRes		| Contains the result operand.
 * @field OPERAND	| opA		| Contains the accumulator operand.
 * @field OPERAND	| opX		| Contains the X operand.
 * @field OPERAND	| opY		| Contains the Y operand.
 */
typedef struct {
	BYTE	opcode;
	OPERAND	opRes;
	OPERAND	opA;
	OPERAND	opX;
	OPERAND	opY;
} FXINSTR;

/*****************************************************************
* 
* @doc APPLET
* @func ULONG | fxParamReadGPR |
*	
* This function finds the value of the GPR specified
* by <p gprAddr>.
*
* @parm FXPGMID		| pgmID		| Specifies program ID.
* @parm	OPERAND	| gprAddr	| Specifies GPR to read.
*
* @comm This function does not return an error if <p gprAddr> or
* <p pgmID> are
* invalid.  It is up to the applet programmer to make sure that the
* parameters are valid.  If an invalid address is given, this function
* will return zero. 
*
* @rdesc This function returns the value of the GPR specified
* by <p gprAddr>.
*
* @xref <t OPERAND>
*				
******************************************************************
*/
EMUCTYPE ULONG EMUAPIEXPORT fxParamReadGPR( FXPGMID, OPERAND );

/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxParamReadGPRArray |
*	
* This function reads <p nGPRs>, starting from address <p baseGPR>,
* and fills the values into the array <p aulArray>.
*
* @parm FXPGMID	| pgmID		| Specifies program ID.
* @parm	OPERAND	| baseGPR	| Specifies base GPR to read.
* @parm ULONG   | aulArray[]| Specifies array to fill.
* @parm DWORD	| nGPRs		| Number of GPRs to read.
*
* @rdesc This function returns FXERROR_NO_ERROR on success, or
* FXERROR_INVALID_ID if <p pgmID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamReadGPRArray( FXPGMID, OPERAND, 
								ULONG * /* ARRAY 256 */, DWORD );

/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxParamWriteGPRArray |
*	
* This function writes <p nGPRs>, starting from address <p baseGPR>,
* and fills the values from the array <p aulArray>.
*
* @parm FXPGMID	| pgmID		| Specifies program ID.
* @parm	OPERAND	| baseGPR	| Specifies base GPR to write.
* @parm ULONG   | aulArray[]| Specifies array of values.
* @parm DWORD	| nGPRs		| Number of GPRs to write.
*
* @rdesc This function returns FXERROR_NO_ERROR on success, or
* FXERROR_INVALID_ID if <p pgmID> is invalid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamWriteGPRArray( FXPGMID, OPERAND, 
								ULONG * /* ARRAY 256 */, DWORD );

/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxParamWriteGPR |
*	
* This function writes the specified value to the specified GPR.
*
* @parm FXPGMID		| pgmID		| Specifies program ID.
* @parm	OPERAND	| gprAddr	| Specifies GPR to write.
* @parm ULONG	| ulValue	| Specifies value to write.
*
* @rdesc This function will return one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
* @flag FXERROR_INVALID_GPR		| If <p gprAddr> is not valid.
*
* @xref <t OPERAND>
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamWriteGPR( FXPGMID, OPERAND, ULONG );

/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxParamReadTableArray |
*	
* This function reads an array of table data.
*
* @parm FXPGMID		| pgmID			| Specifies program ID.
* @parm ADDR		| addrTable		| Specifies physical or virtual table address.
* @parm ULONG		| ulSize		| Specifies number of values to read.
* @parm ULONG *		| pData			| Specifies pointer to storage array.
*
* @rdesc This function will return one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamReadTableArray( FXPGMID, ADDR, 
													  ULONG /* VSIZE */, ULONG * /* IO */);
/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxParamWriteTableArray |
*	
* This function writes an array of table data.
*
* @parm FXPGMID		| pgmID			| Specifies program ID.
* @parm ADDR		| addrTable		| Specifies physical or virtual table address.
* @parm ULONG		| ulSize		| Specifies number of values to write.
* @parm ULONG *		| pData			| Specifies pointer to array of values to write.
*
* @rdesc This function will return one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamWriteTableArray( FXPGMID, ADDR, 
													  ULONG /* VSIZE */, ULONG * );

/*****************************************************************
* 
* @doc APPLET
* @func OPERAND | fxParamReadInstructionField |
*	
* This function reads and fixes down the specified
* instruction address and field.
*
* @parm FXPGMID			| pgmID			| Specifies program ID.
* @parm INSTRFIELD	| wInstrField	| Specifies intruction address and operand field.
*
* @rdesc This function returns a fixed-down value of the specified
* instruction field, or <t ADDRERROR> if either parameter is invalid.
*				
******************************************************************
*/
EMUCTYPE OPERAND EMUAPIEXPORT fxParamReadInstructionField( FXPGMID, INSTRFIELD );

/*****************************************************************
* 
* @doc APPLET
* @func OPERAND | fxParamReadInstruction |
*	
* This function reads and fixes down the specified instruction address.
*
* @parm FXPGMID	| pgmID			| Specifies program ID.
* @parm ADDR	| wInstrField	| Specifies virtual intruction address.
* @parm FXINSTR*| pInstr		| Specifies pointer to structure to fill.
*
* @rdesc This function returns FXERROR_INVALID_ID if <p pgmID> is invalid,
* or FXERROR_NO_ERROR otherwise.  If a field can be fixed down, it will
* contain the virtual address (high bit set).  If not, the physical address
* will be given (high bit clear).
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamReadInstruction( FXPGMID, ADDR, FXINSTR * /* IO */ );

/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxParamWriteInstructionField |
*	
* This function fixes up and writes the specified
* value to the specified instruction address and field.
*
* @parm FXPGMID		| pgmID			| Specifies program ID.
* @parm INSTRFIELD	| wInstrField	| Specifies instruction address and operand field.
* @parm OPERAND		| wValue		| Specifies value to write.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
* @flag FXERROR_INVALID_INSTR	| If <p wInstrField> is not valid.
*
* @xref <t INSTRFIELD> <t OPERAND>
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamWriteInstructionField( FXPGMID, INSTRFIELD, OPERAND );

/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxParamWriteInstruction |
*	
* This function fixes up and writes the specified instruction
* specified instruction address.
*
* @parm FXPGMID		| pgmID			| Specifies program ID.
* @parm ADDR		| wInstrField	| Specifies virtual instruction address.
* @parm FXINSTR *	| pInstr		| Specifies pointer to struct to write.
*
* @comm For this function, each of the operand fields in <p pInstr> is
* assumed to be of type <t OPERAND>, meaning that they can contain virtual
* or physical values.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
*
* @xref <t INSTRFIELD> <t OPERAND>
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamWriteInstruction( FXPGMID, ADDR, FXINSTR * );

/*****************************************************************
* 
* @doc APPLET
* @func FLAGBYTE | fxParamReadFlags |
*	
* This function returns an enumeration of the
* addressing mode and alignment value of the
* given X/TRAM buffer.
*
* @parm FXPGMID		| pgmID			| Specifies program ID.
* @parm OPERAND	| tramBufAddr	| Specifies X/TRAM buffer.
*
* @comm <p tramBufAddr> can be either the data or address GPR for
* an X/TRAM buffer.  This function will set the appropriate flags.
*
* @rdesc This function returns the enumerated flag values or
* <t FLAGERROR> if <p pgmID> is not valid or <p tramBufAddr> does not
* point to a valid X/TRAM data or address buffer GPR.
*
* @xref <t OPERAND> <t FLAGBYTE>
*				
******************************************************************
*/
EMUCTYPE TRAMMODE EMUAPIEXPORT fxParamReadFlags( FXPGMID, OPERAND );

/*****************************************************************
*
* @doc APPLET 
* @func FXSTATUS | fxParamWriteFlags |
*	
* This function sets the mode and alignment value
* of the specified X/TRAM buffer.
*
* @parm FXPGMID		| pgmID			| Specifies program ID.
* @parm OPERAND	| tramBufAddr	| Specifies X/TRAM buffer GPR.
* @parm FLAGBYTE| fByte			| Specifies value to write.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
* @flag FXERROR_INVALID_GPR		| If <p tramBufAddr> is not valid.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamWriteFlags( FXPGMID, OPERAND, TRAMMODE );

/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxParamInterpolate |
*	
* This function sets an interpolator to ramp the
* given GPR to <p ulValue> at rate <p fxRate>.
*
* @parm FXPGMID		| pgmID		| Specifies program ID.
* @parm OPERAND	| gprAddr	| Specifies GPR to ramp.
* @parm ULONG	| ulDest	| Specifies destination GPR value.
* @parm FXRATE	| fxRate	| Specifies time constant.
*
* @comm The interpolator will continue to ramp the GPR until one of
* the following conditions is met: (1) the program is shut down; 
* (2) a call is made to <f fxPgmStopInterpolator()>; (3) a call is made
* to <f fxPgmStopAllInterpolators()>; or, (4) it is reallocated to
* another program or GPR.  So, when a value reaches its
* destination, it cannot be assumed that the interpolator has gone
* away.  If the value of the GPR is modified externally and the
* interpolator is still allocated, it will continue to ramp its value.
* It can also not be assumed that the value will always reach
* <p ulDest> exactly.  Furthermore, <p fxRate> is approximate, and
* there is no guarantee that the interpolator will not get reallocated
* before the GPR reaches its destination value.  In this case, the
* GPR will be forced to <p ulDest> before reallocation.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
* @flag FXERROR_INVALID_GPR		| If <p gprAddr> is not valid.
*
* @xref <t OPERAND> <t FXRATE> <f fxPgmStopInterpolator>
*       <f fxPgmStopAllInterpolators>
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamInterpolate( FXPGMID, OPERAND, ULONG 
#if VARIABLE_RAMPER												  
												  ,FXRATE
#endif
												  );

/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxParamStopInterpolator |
*	
* This function stops the ramping of the specified GPR.
*
* @parm FXPGMID		| pgmID			| Specifies program ID.
* @parm OPERAND	| gprAddr		| Specifies ramping GPR.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
*
* @xref <t OPERAND> <f fxParamInterpolate> <f fxParamStopAllInterpolators>
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamStopInterpolator( FXPGMID, OPERAND );

/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxParamStopAllInterpolators |
*	
* This function stops ramping all GPRs associated with the
* specified program.
*
* @parm FXPGMID		| pgmID			| Specifies program ID.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is not valid.
*
* @xref <f fxParamInterpolate> <f fxParamStopAllInterpolators>
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamStopAllInterpolators( FXPGMID );

/*****************************************************************
*
* @doc APPLET
* @func ULONG | fxParamReadTRAMAddress |
*	
* This function reads the fixed-down address of a TRAM address
* buffer.
*
* @parm FXPGMID		| pgmID			| Specifies program ID.
* @parm OPERAND	| tramBufAddr	| Specifies the X/TRAM buffer.
*
* @comm <p tramBufAddr> can be either the data or address GPR for
* an X/TRAM buffer.  This function will write the proper GPR.
*
* @rdesc This function returns the virtual TRAM address of the
* specified buffer, or <t ULONGERROR> if <p pgmID> is invalid or
* <p tramBufAddr> does not refer to a valid X/TRAM buffer.
*
* @xref <t OPERAND>
*
******************************************************************
*/
EMUCTYPE ULONG EMUAPIEXPORT fxParamReadTRAMAddress( FXPGMID, OPERAND );

/*****************************************************************
*
* @doc APPLET
* @func FXSTATUS | fxParamWriteTRAMAddress |
*	
* This function writes the fixed-up address into a TRAM address
* buffer.
*
* @parm FXPGMID		| pgmID			| Specifies program ID.
* @parm OPERAND	| tramBufAddr	| Specifies the X/TRAM buffer.
* @parm ULONG	| ulAddrNew		| Specifies the new address to write.
*
* @comm <p tramBufAddr> can be either the data or address GPR for
* an X/TRAM buffer.  This function will write the proper GPR.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_INVALID_ID		| If <p pgmID> is invalid.
* @flag FXERROR_INVALID_GPR		| If <p tramBufAddr> is not a valid X/TRAM buffer.
* @flag FXERROR_OUT_OF_BOUNDS	| If <p ulAddrNew> is larger than the allocated delay-line memory.
*
* @xref <t OPERAND>
*
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxParamWriteTRAMAddress( FXPGMID, OPERAND, ULONG );

/*****************************************************************
*
* @doc APPLET 
* @func FXSTATUS | fxCallbackRegisterCallback |
*	
* This function registers a callback to be executed
* on receipt of a system event.  
*
* @comm The callback will be
* executed with the following parameter list:
*               
* <f callback_func>( <p callID>, <p ulEvent>, <p ulParam>, <p ulSystemParam> )
*
* <p callID> will be the callback ID of the registered
* function, <p ulEvent> will be the event which caused the
* callback to be instantiated, <p ulParam> will be the 
* 32-bit general-purpose paramter that was passed to 
* <f fxCallbackRegisterCallback()>, and <p ulSystemParam> is
* a 32-bit parameter passed from the system.
* <p ulSystemParam> will be the value passed from the DSP
* program if the event was <t FXEVENT_DSPINTERRUPT>, and
* zero otherwise.
*
* @parm CALLID *	| pcallid	| Specifies a pointer to an opaque 
*								  identifier to be returned by this call.
* @parm	FXPGMID			| pgmID		| Specifies ID of DSP program.
* @parm FXEVENT		| ulEvent   | Specifies <t FXEVENT> type ORed with a count
*                                 value (for <t FXEVENT_SAMPLETIMER> 
*                                 only).
* @parm (void *)(CALLID,FXEVENT,ULONG,ULONG) | fHandler |
*                                 Specifies callback function address.
* @parm ULONG		| ulParam	| Specifies a user-definable general-purpose parameter.
*
* @comm:(SUPERUSER) It is assumed that when this function is called
* the current context will still be that of the calling
* process.  This is because the callback address is
* translated to an OS handle during the function call.
*
* @rdesc This function returns one of the following:
* @flag FXERROR_NO_ERROR		| If successful.
* @flag FXERROR_OUT_OF_MEMORY	| If not enough host memory to complete operation.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxCallbackRegisterCallback( CALLID * /* IO */, FXPGMID, FXEVENT, 
											   void (*)(CALLID,FXEVENT,ULONG,ULONG), 
											   ULONG );

/*****************************************************************
* 
* @doc APPLET
* @func FXSTATUS | fxCallbackUnregisterCallback |
*	
* This function will remove a callback function from
* the system event chain.
*
* @parm CALLID | callID	| Specifes ID returned by registration.
*
* @rdesc This function always returns FXERROR_NO_ERROR.
*				
******************************************************************
*/
EMUCTYPE FXSTATUS EMUAPIEXPORT fxCallbackUnregisterCallback( CALLID );

#endif
