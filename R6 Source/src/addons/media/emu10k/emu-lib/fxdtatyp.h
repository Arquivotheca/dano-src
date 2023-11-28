/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
* @doc INTERNAL
* @module fxdtatyp.h |
*
* This file contains general definitions, types, and
* macros that pertain to user code in support of the
* FX8010.
*
*******************************************************************
*/
/* @doc EXTERNAL | APPLET */

#ifndef _FXDTATYP_H
#define _FXDTATYP_H

#include "datatype.h"
#include "aset8210.h"

/* @type ADDR | 10-bit GPR or instruction address.
*/
typedef unsigned short		ADDR;
#define FXADDRERROR ((ADDR)0xffff)

/* @type:(INTERNAL) FXID | Opaque identifier
*/

#ifndef VOID
#define VOID void
#endif

typedef VOID *			FXID;

typedef VOID *			FXCHIPID;
typedef VOID *			FXPARTID;
typedef VOID *			FXPGMID;
#define UNALLOCED_ID	NULL


/* @type OPERAND | 
 * Describes a GPR address: <t OPERAND>[9:0] contains the address,
 * and <t OPERAND>[15] is set if the address is virtual, reset if
 * it is physical.  <t OPERAND>[14:10] is reserved.  Can also be used
 * to define an opcode, in which case only <t OPERAND>[3:0] is used.
 */
typedef unsigned short OPERAND;
#define VIRTMASK  0x7fff

/* @type XOPERAND | 
 * Same as <t OPERAND>, except <t XOPERAND>[14:12] defines a fixup
 * type:  000 = general, 001 = TRAM, 010 = XTRAM, 011 = Table.
 */
typedef unsigned short XOPERAND;
#define BUFMASK	  0x7000
#define TRAMMASK  0x2000
#define XTRAMMASK 0x1000
#define TABLEMASK 0x3000

/* @type INSTRFIELD |
 * Describes an instruction address and field: <t INSTRFIELD>[9:0]
 * contains the address and <t OPERAND>[15:13] contains the
 * <t FIELD> enumeration.
 */
typedef unsigned short INSTRFIELD;

/* @enum FIELDENUM |
 * This enumeration defines the instruction fields
 */
typedef unsigned short FIELDENUM;
#define    FX_OPCODE   0x0000
#define    FXOP_A      0x2000
#define    FXOP_X      0x4000
#define    FXOP_Y      0x6000
#define    FXOP_RES    0x8000


#define FIELDMASK	0xe000

/* @enum TRAMMODE |
 * This enumeration defines the various TRAM buffer modes.
 */
typedef enum { 
	   MODE_READ=0,		/* @emem Read from buffer.				*/
	   MODE_WRITE,		/* @emem Write to buffer.				*/
	   MODE_RSAW,		/* @emem Read, Sum and Write to buffer.	*/
	   MODE_TREAD,		/* @emem Read from table.				*/
	   MODE_OFF,		/* @emem No activity.					*/
	   MODE_READNOZERO, /* @emem Read from buffer, no zero count */
	   MODE_TWRITE		/* @emem Write to table					*/
} TRAMMODE;

/* @enum FIELD |
 * This enumeration defines an instruction field.
 */
typedef enum { 
	   OPCODE,		/* @emem Instruction opcode.	*/
	   FIELD_A,		/* @emem Operand Accumulator.	*/
	   FIELD_X,		/* @emem Operand X.				*/
	   FIELD_Y,		/* @emem Operand Y.				*/
	   FIELD_RES	/* @emem Operand result.		*/
} FIELD;

typedef BYTE	CK;


/* @enum FXSTATUS |  
 * This enumeration defines the various return values
 * of most FX8010 manager functions.  The description of each is given
 * on a per-function basis.
 */
typedef enum {
	FXERROR_NO_ERROR,			/* @emem Indicates success */
	FXERROR_OUT_OF_GPR_SPACE,	/* @emem No GPRs available */
	FXERROR_PARTITION_NOT_EMPTY,/* @emem Partition has loaded programs */
	FXERROR_NO_CONTIGUOUS_SPACE,/* @emem Not enough of a contiguous resource available */
	FXERROR_NO_FREE_INODES,		/* @emem Not enough memory to complete operation */
	FXERROR_OUT_OF_MEMORY,		/* @emem Not enough memory to complete operation */
	FXERROR_INVALID_ID,			/* @emem FXID is not valid or has been invalidated */
	FXERROR_CHIP_IN_USE,		/* @emem Chip cannot b repartitioned unless it has been completely deallocated */
	FXERROR_OUT_OF_TABLE_RAM,	/* @emem No more table RAM available */
	FXERROR_OUT_OF_GENERAL_GPRS,/* @emem No more general GPRs available */
	FXERROR_OUT_OF_TRAM_GPRS,	/* @emem No more TRAM buffers available */
	FXERROR_OUT_OF_XTRAM_GPRS,	/* @emem No more XTRAM buffers available */
	FXERROR_OUT_OF_INSTR_SPACE,	/* @emem No more instructions available */
	FXERROR_OUT_OF_TRAM_SPACE,	/* @emem No more TRAM available */
	FXERROR_OUT_OF_XTRAM_SPACE,	/* @emem No more XTRAM available */
	FXERROR_OUT_OF_TABLE_SPACE,	/* @emem No more table space available */
	FXERROR_OUT_OF_INSTRUCTION_SPACE, /* @emem No more instruction space available */
	FXERROR_INVALID_CONTIGTYPE,	/* @emem <t CONTIGTYPE> value is invalid */
	FXERROR_NOT_TRAM_ADDRESS,	/* @emem GPR address is not a X/TRAM buffer address */
	FXERROR_NO_INTERPOLATORS,	/* @emem No interpolators were allocated on this chip */
	FXERROR_PROGRAMS_ON_DIFFERENT_CHIPS,	/* @emem Cannot patch programs on different chips */
	FXERROR_INVALID_CHANNEL,	/* @emem Specified channel not valid */
	FXERROR_OUT_OF_RESOURCES,	/* @emem Not enough resources available */
	FXERROR_INVALID_PGMTEXT,	/* @emem Program text structure not valid */
	FXERROR_RESOURCE_MISMATCH,	/* @emem Number of inputs/outputs does not match resource chunk */
	FXERROR_PORTS_ALREADY_ASSIGNED, /* @emem Port has already been abstracted. */
	FXERROR_NO_XTRAGPR,			/* @emem Program text missing extra GPR chunk. */
	FXERROR_CHIP_MISMATCH,		/* @emem Program text CKID chip revision does not match actual revision of chip */
	FXERROR_PHYSICAL_PORTS,		/* @emem Two physical ports cannot be patched together */
	FXERROR_NOMIX_PARAM,		/* @emem Mix parameter not available */
} FXSTATUS;

#endif
