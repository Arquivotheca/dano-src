
/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* @doc INTERNAL
* @module hal8210.h |
* Description: EMU8010 Hardware Abstraction Layer 
*              These are the lowest level register I/O routines before the
*              actual hardware commands.
* 
* @iex
* History:
*
* Version    Person        Date         Reason
* -------    ---------   -----------  --------------------------------------- 
*  0.003        MG       Oct  9, 97   Change xGRead/Write
*                                     Add better chip discovery/undiscovery
*                                     Auto-ducked
*  0.002		JK		 Sep 24, 97	  Changed to use the 8210 name scheme.
*  0.001        MG       Jun 02, 97   Initial version
*
******************************************************************************
* @doc EXTERNAL
* @contents1 EMU HAL8210 Programmer's Manual |
* All modules contained within the Hardware Resource Manager of the EMU8010 talk 
* to the Hardware Abstraction Layer, which performs the register I/O routines, as 
* well as maintains a list of multiple EMU8010 based products living on the same 
* system. General purpose functions below are designed for debug only, whereas 
* the "helper" functions are designed for real-time manipulation, as those are 
* speed optimized. All 'offsets' and 'addresses' have mnemonics which did not make
* it into the programmer's manual document, but may be seen in the HAL8210.H header 
* file.
*
* <nl>Summary of the HAL8210 Features<nl>
*	<tab>Support for multiple instances of EMU8010 hardware<nl>
*	<tab>Full register access<nl>
*	<tab>Helper routines for optimized register access<nl>
******************************************************************************
*/

#ifndef __HAL8210_H
#define __HAL8210_H



/************
* Includes
************/

#include "datatype.h" /* Data types */  
#include "emuerrs.h" 
#include "cfg8210.h"

#if defined(__8010__)
# include "e10k1reg.h"
#elif defined(__E10K2__)
# include "e10k2reg.h"
@elif
# error "hal8210.h needs to have a chip type defined: __8010__ or __E10K2__"
#endif


BEGINEMUCTYPE

typedef DWORD HALID;
 
/************
* Defines
************/

/* This needs to be moved somewhere for FX8010 integration */
#define MAX_CHIPS 2

/************************ HAL8010 Initialization ***********************
* The following are the initialization and chip discovery routines
* for the EMU8010 HAL.
*
*/

/* @func Initialize the software state variables of the Hardware Abstraction layer module. 
 * halInit should be called at program start time.   
 *
 * @rdesc Returns of SUCCESS is normal, other is error
 */
EMUAPIEXPORT EMUSTAT halInit();

/* @func Initialize the software state variables of the HAL for a single EMU8010 
 * PCI function or chip which has been discovered on the system. halDiscoverChip
 * should be called for each chip found, and as each PCI function on each chip is found.  
 *
 * @parm HALID * | pStuffedWithID | 'ID' value returned should be used with any calls to 
 * future functions to indicate the chip in use.
 * @parm HRMCHIPCONFIG * | pConfig | Configuration structure, see 'cfg8210.h'
 *
 * @rdesc Return of SUCCESS is normal, other is error.
 */
EMUAPIEXPORT EMUSTAT halDiscoverChip(HALID *stuffedWithID /* IO */ , HRMCHIPCONFIG *config);
 
/* @func Resets software state variables of the HAL for a single EMU8010 PCI function or chip
 * which was previously discovered on the system.    
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm HRMFLAGS | unDiscoverFlags | Indicates which PCI functions have been un-discovered. 
 * See configuration structure in'cfg8210.h'
 *
 * @rdesc Return of SUCCESS is normal, other is error.
 */
EMUAPIEXPORT EMUSTAT halUnDiscoverChip(HALID id, HRMFLAGS unDiscoverFlags);

/* @func For a given HAL8210 ID, return the unique user supplied Hardware ID  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip 
 *
 * @rdesc Return of 0xFFFFFFFF means the HALID was invalid.
 */
EMUAPIEXPORT DWORD halGetUserHardwareID(HALID id);

/* @func For a given HAL8210 ID, return whether one or more PCI functions have been
 * discovered.
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm HRMFLAGS | unDiscoverFlags | Indicates which PCI function(s) are in question. 
 * See configuration structure in'cfg8210.h'
 *
 * @rdesc Returns TRUE if all PCI functions corresponding to the user set flags were 
 * discovered, FALSE otherwise.
 */
EMUAPIEXPORT BOOL    halIsDiscovered(HALID id, HRMFLAGS isDiscoveredFlags);

/*********************************************************************
 * @func Determines whether the given halid is valid.
 *
 * @parm HALID | halid | The HAL ID to validate.
 * @rdesc Returns TRUE if the HALID is valid; FALSE otherwise.
 */
EMUAPIEXPORT BOOL   halIsValidID(HALID halid);

/************************ EMU8010 Function Map ***********************
* The following are the offsets for all functions supported by the
* EMU8010.
*
*/

#define EMU8010FUNC0	0
#define EMU8010FUNC1	1 
#define EMU8010FUNC2    2

#define EMU8010BYTE     0
#define EMU8010WORD     1
#define EMU8010LONG     2

/* 
   These are the most general purpose functions to use in calling an 
   EMU8010 Register. There are better ones in the "helper" section
   below. These are provided for debug purposes.
*/
 
/* @func The most general purpose function to use in reading an 
 * EMU8010 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | function | 0=(synthesizer) 1=(joystick) 2=(modem)
 * @parm WORD  | sizeType | 0=BYTE, 1=WORD, or 2=DWORD. 
 * @parm WORD  | funcOffs | offset value. 
 *
 * @rdesc Returns the value, padded by 0's where appropriate given 'sizeType'
 */
EMUAPIEXPORT DWORD E8010RegRead (HALID id, WORD function, WORD sizeType, WORD funcOffs);

/* @func The most general purpose function to use in writing to an 
 * EMU8010 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | function | 0=(synthesizer) 1=(joystick) 2=(modem)
 * @parm WORD  | sizeType | 0=BYTE, 1=WORD, or 2=DWORD. 
 * @parm WORD  | funcOffs | offset value. 
 * @parm DWORD | data | data value to be written
 *
 * @rdesc None
 */
EMUAPIEXPORT void  E8010RegWrite(HALID id, WORD function, WORD sizeType, WORD funcOffs, DWORD data);

/* @func The most general purpose function to use in reading an 8 bit value from an 
 * EMU8010 Function 0 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | function0Offset |  offset value.  
 *
 * @rdesc Returns the value
 */
EMUAPIEXPORT BYTE  B8010SERegRead (HALID id, WORD func0Offset); 

/* @func The most general purpose function to use in writing an 8 bit value to an 
 * EMU8010 Function 0 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | function0Offset |  offset value. 
 * @parm BYTE | data | data value to be written
 *
 * @rdesc none
 */
EMUAPIEXPORT void  B8010SERegWrite(HALID id, WORD func0Offset, BYTE data);

/* @func The most general purpose function to use in reading a 16 bit value from an 
 * EMU8010 Function 0 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | function0Offset |  offset value. 
 *
 * @rdesc Returns the value
 */
EMUAPIEXPORT WORD  W8010SERegRead (HALID id, WORD func0Offset); 

/* @func The most general purpose function to use in writing a 16 bit value to an 
 * EMU8010 Function 0 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | function0Offset |  offset value. 
 * @parm WORD | data | data value to be written
 *
 * @rdesc none
 */
EMUAPIEXPORT void  W8010SERegWrite(HALID id, WORD func0Offset, WORD data);

/* @func The most general purpose function to use in reading a 32 bit value from an 
 * EMU8010 Function 0 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | function0Offset |  offset value. 
 *
 * @rdesc Returns the value
 */
EMUAPIEXPORT DWORD L8010SERegRead (HALID id, WORD func0Offset); 

/* @func The most general purpose function to use in writing a 32 bit value to an 
 * EMU8010 Function 0 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | function0Offset |  offset value. 
 * @parm DWORD | data | data value to be written
 *
 * @rdesc none
 */
EMUAPIEXPORT void  L8010SERegWrite(HALID id, WORD func0Offset, DWORD data);


/* @func The most general purpose function to use in reading an 8 bit value from an 
 * EMU8010 Function 1 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | function1Offset |  offset value.  
 *
 * @rdesc Returns the value
 */
EMUAPIEXPORT BYTE B8010JoyRegRead (HALID id, WORD func1Offset); 

/* @func The most general purpose function to use in writing an 8 bit value to an 
 * EMU8010 Function 1 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | function1Offset |  offset value. 
 * @parm BYTE | data | data value to be written
 *
 * @rdesc none
 */
EMUAPIEXPORT void B8010JoyRegWrite(HALID id, WORD func1Offset, BYTE data);

/* Not sure what these are */
#define AC3D	0x0000U
#define AC3CT	0x0004U
#define AC3HD	0x0005U
#define AC3IC	0x0006U
#define AC3CL	0x0007U

#define HMD	0x0000U
#define RXIP    0x0004U
#define TXFS	0x0005U
#define HMIE	0x0006U
#define HMIO	0x0007U


/**************** Helper Functions **********************************
*
* Helper functions are designed to permit programming to certain portions
* of the EMU8010 which have well defined behavior, and which would otherwise
* force the client to write repetitive and difficult to read code. Examples
* would be pointer/data register type functions.
*
* These functions may also be designed to be handled at optimal speed.
*/  

/**************** General (Function Independent) Helpers ***********/

/* The Index8010 series of functions handle any index/data style register in
   the EMU8010, including AC97, PTRREG */

/* @func The most general purpose function to use in writing to any EMU8010 Index/Data
 * style register  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | funcNum | One of 0, 1, or 2
 * @parm WORD  | indexReg |  offset value of the index register
 * @parm WORD  | dataSize | One of 0(BYTE) 1(WORD) or 2(DWORD)
 * @parm DWORD | addr | data with which the index register should be programmed
 * @parm DWORD | data | data with which the corresponding data register should be programmed 
 *
 * @rdesc None
 */
EMUAPIEXPORT void  Index8010Write(HALID id, WORD funcNum, WORD indexReg, WORD dataSize, DWORD addr, DWORD data);

/* @func The most general purpose function to use in reading any EMU8010 Index/Data
 * style register  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm WORD  | funcNum | One of 0, 1, or 2
 * @parm WORD  | indexReg |  offset value of the index register
 * @parm WORD  | dataSize | One of 0(BYTE) 1(WORD) or 2(DWORD)
 * @parm DWORD | addr | data with which the index register should be programmed 
 *
 * @rdesc data from the corresponding data register
 */
EMUAPIEXPORT DWORD Index8010Read (HALID id, WORD funcNum, WORD indexReg, WORD dataSize, DWORD addr);


/**************** Function 0 Helpers ********************************/


/* Macros to map Fx Engine internal addresses to PCI's RGA addresses. FX 
*  addresses computed this way are suitable for input to the 8010 pointer
*  register via LGWrite(), etc.
*/
#define FX_GPR( gprAddr )        (0x01000000+(gprAddr  <<16))
#define FX_UCODEH( ucodeAddr )   (0x04010000+(ucodeAddr<<17))
#define FX_UCODEL( ucodeAddr )   (0x04000000+(ucodeAddr<<17))

/* The 'G' series of calls automatically handle the PTRREG/DATAREG handling.
   They may also support optimized I/O, IE never writing the same value
   to the PTRREG in two consecutive calls. 'G' and error status return style
   is legacy from the G0.5 or EMU8000 software. These macros allow that legacy
   to continue within current code, so long as the error status is not used. User
   should begin using the xSEPtrXXX functions
*/

#define BGWrite(a, b, c) BSEPtrWrite(a, b, c)
#define WGWrite(a, b, c) WSEPtrWrite(a, b, c)
#define LGWrite(a, b, c) LSEPtrWrite(a, b, c)
#define BGRead(a, b, c) *(c) = BSEPtrRead(a, b)
#define WGRead(a, b, c) *(c) = WSEPtrRead(a, b)
#define LGRead(a, b, c) *(c) = LSEPtrRead(a, b)

/* @func Write a 32 bit value to an EMU8010 Function 0 "Pointer/Data" Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm DWORD | dwAddr |  Value which should be plugged into PTREG
 * @parm DWORD | dwData | Value to be written to DATAREG
 *
 * @rdesc None
 */
EMUAPIEXPORT void  LSEPtrWrite(HALID id, DWORD dwAddr, DWORD dwData);

/* @func Write a 16 bit value to an EMU8010 Function 0 "Pointer/Data" Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm DWORD | dwAddr |  Value which should be plugged into PTREG
 * @parm DWORD | wData | Value to be written to DATAREG
 *
 * @rdesc None
 */
EMUAPIEXPORT void  WSEPtrWrite(HALID id, DWORD dwAddr, WORD wData);

/* @func Write an 8 bit value to an EMU8010 Function 0 "Pointer/Data" Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm DWORD | dwAddr |  Value which should be plugged into PTREG
 * @parm DWORD | byData | Value to be written to DATAREG
 *
 * @rdesc None
 */
EMUAPIEXPORT void  BSEPtrWrite(HALID id, DWORD dwAddr, BYTE byData);

/* @func Read a 32 bit value from an EMU8010 Function 0 "Pointer/Data" Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm DWORD | dwAddr |  Value which should be plugged into PTREG 
 *
 * @rdesc Returns the value in DATAREG
 */
EMUAPIEXPORT DWORD LSEPtrRead (HALID id, DWORD  dwAddr);

/* @func Read a 16 bit value from an EMU8010 Function 0 "Pointer/Data" Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm DWORD | dwAddr |  Value which should be plugged into PTREG 
 *
 * @rdesc Returns the value in DATAREG
 */
EMUAPIEXPORT WORD  WSEPtrRead (HALID id, DWORD  dwAddr);

/* @func Read an 8 bit value from an EMU8010 Function 0 "Pointer/Data" Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm DWORD | dwAddr |  Value which should be plugged into PTREG 
 *
 * @rdesc Returns the value in DATAREG
 */
EMUAPIEXPORT BYTE  BSEPtrRead (HALID id, DWORD  dwAddr);

/* @func Wait for a number of wall clock counts to pass, then return.  
 *  The cycle time of the wall clock is defined as one sample tick,
 *  or roughly 20 microseconds.  Thus, 48000 wall clock ticks == 1 sec.
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm DWORD | count |  Number of wall clocks to wait
 *
 * @rdesc Return of non SUCCESS means the wall clock register could not be read,
 * so proper duration may not have passed.
 */
EMUAPIEXPORT EMUSTAT halWaitWallClockCounts(HALID id, DWORD count);


/**************** AC97 Helpers   ********************
*
* These constants are register offsets that are programmed into
* the Function 0 pointer register AC97ADDR so that the 8010's internal 
* register space can be accessed through the data register AC97DATA.
* These are for controlling the AC97 CODEC.  
*/

#define AC97RST    0x00
#define AC97MVOL   0x02
#define AC97HVOL   0x04
#define AC97MVLM   0x06
#define AC97MTON   0x08
#define AC97BEEP   0x0A
#define AC97PHON   0x0C
#define AC97MIC    0x0E
#define AC97LINE   0x10
#define AC97CD     0x12
#define AC97VID    0x14	
#define AC97AUX    0x16 
#define AC97PCM    0x18   
#define AC97RSEL   0x1A
#define AC97RGAIN  0x1C
#define AC97RGMIC  0x1E
#define AC97GEN    0x20
#define AC973D     0x22
#define AC97MODEM  0x24
#define AC97POWR   0x26
#define AC97RESVD  0x28
#define AC97VEN1   0x5A
#define AC97VEN2   0x7A 
#define AC97VEID1  0x7C  
#define AC97VEID2  0x7E

/* @func Write an 8 bit value to an EMU8010 Function 0 AC97 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm BYTE | byAddr |  Value which should be plugged into AC97ADDR
 * @parm WORD | byData | Value to be written to AC97DATA
 *
 * @rdesc None
 */
EMUAPIEXPORT void AC97Write(HALID id, BYTE addr, WORD data);

/* @func Read an 8 bit value from an EMU8010 Function 0 AC97 Register.  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip
 * @parm BYTE | byAddr |  Value which should be plugged into AC97ADDR 
 *
 * @rdesc Returns the value in AC97DATA
 */
EMUAPIEXPORT WORD AC97Read (HALID id, BYTE addr);


/**************** Function 1 Helpers ********************************/

/* This function automatically writes to the AJOYWR register and then
   reads data from the AJOYRD register.
*/

/* @func Poll the EMU8010 Function 1 joystick poll register, return the data value  
 *
 * @parm HALID | halID | 'ID' value returned by halDiscoverChip  
 *
 * @rdesc Returns the polled value
 */
EMUAPIEXPORT BYTE B8010JoyRead(HALID id);

ENDEMUCTYPE

#endif
