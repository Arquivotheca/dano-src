/******************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
******************************************************************************/

/******************************************************************************
*
* @doc INTERNAL
* @module ip8210.h | 
* This file contains the public datatypes and functions used by the EMU 8010
* interrupt pending manager.
*
* @iex 
* Revision History:
*
* Person              Date          Reason
* ------------------  ------------  --------------------------------------
* Michael Preston     Oct 14, 1997  Moved chip discovery functions to
*                                   ip8210d.h.
* Michael Preston     Sep 30, 1997  Initial development.
*
******************************************************************************/

#ifndef __IP8210_H
#define __IP8210_H

/****************
* Include files
****************/
#include "datatype.h"
#include "se8210.h"
#include "emuerrs.h"

/******************************************************************************
* @contents1 Contents |
*
* @subindex Enumerations
* @subindex Typedefs & Structures
* @subindex Public Functions
******************************************************************************/

/******************************************************************************
* @contents2 Enumerations |
* @index enum |
******************************************************************************/

/******************************************************************************
* @enum enIPType |
* This enumerated type contains all of the allowable interrupt types.  The
* enumerated values are used in the type value in the <t stIPInfo> structure
* which is passed to the <f ipRegisterCallback> and <f ipGetIPHandleInfo>
* functions.
******************************************************************************/
typedef enum {
   IP_SRTLOCKED, /* @emem Sample rate tracker locked */
   IP_FX8010,    /* @emem Effects engine interrupt */
   IP_HOSTMODEM, /* @emem Host modem interrupt */
   IP_PCIERROR,  /* @emem PCI error */
   IP_VOLINC,    /* @emem Volume increment button pushed */
   IP_VOLDEC,    /* @emem Volume decrement button pushed */
   IP_MUTE,      /* @emem Mute button pushed */
   IP_MICBUFFER, /* @emem Microphone recording buffer interrupt */
   IP_ADCBUFFER, /* @emem ADC recording buffer interrupt */
   IP_FXBUFFER,  /* @emem Effects engine output recording buffer interrupt */
   IP_GPSPDIF,   /* @emem General purpose S/PDIF interrupt */
   IP_CDSPDIF,   /* @emem CD S/PDIF interrupt */
   IP_INTTIMER,  /* @emem Interval timer interrupt */
   IP_UARTTX,    /* @emem MIDI transmit buffer empty */
   IP_UARTRX,    /* @emem MIDI receive buffer non-empty */
   IP_ONLOOP,    /* @emem Voice interrupt on loop */

/* End of interrupt types */
   lastInterruptType
} enIPType;

/******************************************************************************
* @enum enIPError |
* Error codes.
******************************************************************************/
typedef enum {
   IP_SUCCESS = SUCCESS, /* @emem Success */
   IP_MEM_ALLOC_FAILED,  /* @emem Memory allocation failed */
   IP_INVALID_PARAM,     /* @emem Invalid parameter passed into function */
   IP_SYSTEM_ERROR,      /* @emem System error */
   IP_ALREADY_SET        /* @emem Interrupt already set - maximum number of
                          * callback functions registered for this interrupt
                          * type */
} enIPError;

/******************************************************************************
* @contents2 Typedefs & Structures |
* @index type,struct |
******************************************************************************/

/******************************************************************************
* @type IPID | An opaque handle used to reference a particular E8010 interrupt
* pending manager.
******************************************************************************/
typedef DWORD IPID;

/******************************************************************************
* @type IPHANDLE | An opaque handle used to reference a particular E8010
* interrupt callback.  The <t IPHANDLE> data type encapsulates all data that is
* relevant to a particular callback, including the IP manager with which it is
* associated and the type of interrupt.
******************************************************************************/
typedef DWORD IPHANDLE;

/******************************************************************************
* @struct stIPMIDIData | This structure is used to return MIDI data received
* through the MIDI UART.  This data is returned from a callback associated with
* the <e enIPType.IP_UARTRX> interrupt.
******************************************************************************/
typedef struct {
   BYTE MIDIData[32];  /* @field Array of MIDI data */
   DWORD numMIDIBytes; /* @field Number of MIDI bytes returned */
} stIPMIDIData;

/******************************************************************************
* @struct unIPReply | This union is used to store the data returned from an
* interrupt callback function.  A pointer to a structure of this type is passed
* from the IP8210 to the callback function, and is then freed by the IP8210.
* If the callback is of type <e enIPType.IP_UARTRX>, MIDI data is returned.  If
* the callback is of type <e enIPType.IP_ONLOOP>, the <t SEVOICE> handle of the
* voice which has hit a loop point is returned.  If the callback is of type
* <e enIPType.IP_MICBUFFER>, <e enIPType.IP_ADCBUFFER>, or
* <e enIPType.IP_FXBUFFER>, a boolean flag is returned which indicated whether
* the buffer is half full or completely full.
******************************************************************************/
typedef union {
   stIPMIDIData MIDIData; /* @field MIDI data returned from the
                           * <e enIPType.IP_UARTRX> interrupt */
   SEVOICE voiceHandle;   /* @field Voice handle associated with
                           * <e enIPType.IP_ONLOOP> interrupt */
   BOOL bIsHalfBuffer;    /* @field TRUE if half buffer interrupt,
                           * FALSE if full buffer interrupt       */
} unIPReply;

/******************************************************************************
* @type ipCallback | This is the type definition for interrupt callback
* functions.  A pointer to a function of this type is passed into the
* <f ipRegisterCallback> function and returned from the <f ipGetIPHandleInfo>
* function as part of the <t stIPInfo> structure.
*
* @iex
* Example:
*    Function definition:
*       void MyCallback(IPHANDLE handle, enIPType type,
*                       DWORD userParam, unIPReply *pReply);
*    Function pointer usage:
*       stIPInfo info;
*       info.fHandler = MyCallback;
*
* @parm  IPHANDLE | handle | Callback handle assigned by <f ipRegisterCallback>.
* @parm  enIPType | type | Type of interrupt being handled.
* @parm  DWORD | userParam | User parameter originally passed in to
* <f ipRegisterCallback>.
* @parm  unIPReply * | pReply | Pointer to reply structure that contains
* extra data associated with the callback.
*
* @rdesc Returns TRUE if the interrupt should continue to be registered, and
*  FALSE if the interrupt should be deregistered after the handler returns.
*
******************************************************************************/
typedef BOOL (*ipCallback)(IPHANDLE handle, enIPType type, DWORD userParam,
                           unIPReply *pReply);

/******************************************************************************
* @struct stIPInfo | This structure is passed into the <f ipRegisterCallback>
* and <f ipGetIPHandleInfo> functions.  For the <f ipRegisterCallback>
* function, data is passed into the function.  For the <f ipGetIPHandleInfo>,
* data is filled in by the function.
******************************************************************************/
typedef struct {
   enIPType type;            /* @field Interrupt type */
   DWORD interruptParameter; /* @field Interrupt specific parameter -
                              * <nl><tab><t SEVOICE> handle for
                              * <e enIPType.IP_ONLOOP>
                              * <nl><tab>number of sample periods for
                              * <e enIPType.IP_INTTIMER> */
   DWORD userParameter;      /* @field User parameter to be returned on
                              * callback */
   ipCallback fHandler;      /* @field Pointer to callback function */
   ipCallback fISRHandler;   /* @field Pointer to the high-priority ISR func */
} stIPInfo;

/******************************************************************************
* @struct stIPAttrib | This structure is passed into the
* <f ipGetModuleAttributes> function.  This is currently not used.
******************************************************************************/
typedef struct
{
	BYTE blah;
} stIPAttrib;

BEGINEMUCTYPE

/******************************************************************************
* @contents2 Public Functions |
* @index func |
******************************************************************************/

/******************************************************************************
*
* @func  Gets the interrupt pending manager IDs associated with all known
* instances of hardware.
*
* @parm  DWORD | count | Size of ipIDs array.  Only at most this many IDs will
* be returned.
* @parm  IPID * | ipIDs | Array of interrupt pending manager IDs to be returned.
*
* @rdesc  Returns the actual number of IDs returned.  If the ipIDs array is
* invalid, returns 0.
*
******************************************************************************/
DWORD ipGetHardwareInstances(DWORD count, IPID *ipIDs);

/******************************************************************************
*
* @func  Gets the name of the interrupt pending manager for a particular
* instance of hardware.  This function is currently not implemented.
*
* @parm  IPID | ipID | Interrupt pending manager ID.
* @parm  DWORD | count | Size of szName array.
* @parm  CHAR * | szName | Array to be filled with the module name.
*
* @rdesc Returns SUCCESS because the function not yet implemented.
*
******************************************************************************/
EMUSTAT ipGetModuleName(IPID ipID, DWORD count, CHAR *szName);

/******************************************************************************
*
* @func  Get the attributes of the interrupt pending manager for a particular
* instance of hardware.  This function is currently not implemented.
*
* @parm  IPID | ipID | Interrupt pending manager ID.
* @parm  stIPAttrib * | attrib | Attributes structure to be filled.
*
* @rdesc Returns SUCCESS because the function not yet implemented.
*
******************************************************************************/
EMUSTAT ipGetModuleAttributes(IPID ipID, stIPAttrib *attrib);

/******************************************************************************
*
* @func  Registers a callback for a particular interrupt type.  If the
* interrupt for this type is currently disabled, it is enabled.  All interrupts
* can have an implementation defined maximum of 256 callbacks for each
* interrupt type, except for the interval timer (<e enIPType.IP_INTTIMER>)
* which can have only one.
*
* @parm  IPID | ipID | Interrupt pending manager ID.
* @parm  stIPInfo * | info | Pointer to structure which hold info on what type
* of interrupt to set, along with associated parameters, and a pointer to the
* function to be called back when the interrupt is triggered.
* @parm  IPHANDLE * | handle | Pointer to a handle which the function assigns
* if the function succesfully registers the callback.  This handle is used to
* later unregister the function using <f ipUnregisterCallback>.
*
* @rdesc  Returns SUCCESS if the interrupt is successfully registered.
* Otherwise, returns:
*		@flag IP_INVALID_PARAM    | One of the parameters was invalid.
*		@flag IP_MEM_ALLOC_FAILED | Memory allocation failed.
*     @flag IP_ALREADY_SET      | Interrupt is already set.
*
******************************************************************************/
EMUAPIEXPORT EMUSTAT ipRegisterCallback(IPID ipID, stIPInfo *info, IPHANDLE *handle);

/******************************************************************************
*
* @func  Unregisters a previously registered callback.  If this is the last
* callback for a particular type of interrupt, the interrupt is disabled.
*
* @parm  IPHANDLE | handle | Callback handle assigned by <f ipRegisterCallback>.
*
* @rdesc  Returns SUCCESS is the callback is succesfully unregistered.
* Otherwise, returns:
*		@flag IP_INVALID_PARAM    | One of the parameters was invalid.
*
******************************************************************************/
EMUAPIEXPORT EMUSTAT ipUnregisterCallback(IPHANDLE handle);

/******************************************************************************
*
* @func  Returns the interrupt pending manager ID associated with a particular
* callback handle.
*
* @parm  IPHANDLE | handle | Callback handle assigned by <f ipRegisterCallback>.
*
* @rdesc  If successful, returns the IPID associated with the handle.
* Otherwise, returns a NULL ID.
*
******************************************************************************/
IPID ipGetIPID(IPHANDLE handle);

/******************************************************************************
*
* @func  Gets the data that was passed in when registering an interrupt
* callback.
* @parm  IPHANDLE | handle | Callback handle assigned by <f ipRegisterCallback>.
* @parm  stIPInfo * | info | Pointer to a structure to be filled with the data
* that was used to register the callback.
*
* @rdesc  Returns SUCCESS if the structure was successfully filled.
* Otherwise, returns:
*		@flag IP_INVALID_PARAM    | One of the parameters was invalid.
*
******************************************************************************/
EMUSTAT ipGetIPHandleInfo(IPHANDLE handle, stIPInfo* info);

ENDEMUCTYPE

#endif
