/*****************************************************************************
* Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*****************************************************************************
*/
/*****************************************************************************
* @doc INTERNAL
* @module cfg8210.h |
* Configuration for HAL8210 and HRM8210 API
*
* @iex
* Revision history:
* Version    Person        Date         Reason
* -------    ---------   -----------  ------------------------------
*  0.003     Mike Guzewicz Oct 9, 97   Auto-ducked
*  0.002     Mike Guzewicz Oct 7, 97   Add configuration structure
*  0.001     Steve Verity  Oct 1, 97   Initial Creation
******************************************************************************
* @doc EXTERNAL
* @contents1 EMU CFG8210 Programmer's Manual |
* This is the configuration data structure for HRM8210 and HAL8210
*/

#ifndef __CFG8210_H__
#define __CFG8210_H__

#include "datatype.h"
#include "os8210.h"
#include "aset8210.h"



/****************
 * Compilation flags
 ****************/

#define CFG_DYNAMIC 1

/****************
 * Definitions
 ****************/

#define HRMDISC_SOUND_ENGINE 1
#define HRMDISC_JOYSTICK     2
#define HRMDISC_HOST_MODEM   4

 /****************
 * Structures
 ***************/

/* @type HRMFLAGS | An bitfield used to indicate individual PCI functions. 
 * Used in discovery, undiscovery, and queries.
 */
typedef DWORD HRMFLAGS;

/* @struct HRMCONTIGMEMORY | 
 *  The HRMCONTIGMEMORY structure is used to configure the portions of the 
 *  hardware resource manager and hardware abstraction layer configurations 
 *  which require the client to allocte contiguous memory. See OS8210.H
 *  for definitions of the datatypes.
 */
typedef struct stHRMContigMemoryTag
{   
	OSVIRTADDR osVirtAddr;	/* @field Indicates the logical or virtual or software 
							 * address of a contiguous buffer */
	OSPHYSADDR osPhysAddr;	/* @field Indicates the actual or physical or hardware 
							 * address of a contiguous buffer */
	DWORD      dwSize;		/* @field Indicates the size of the buffer in bytes */
} HRMCONTIGMEMORY;

/* @struct HRMCHIPCONFIG | 
 *  The HRMCHIPCONFIG structure is used to configure the hardware resource manager
 *  and hardware abstraction layer software for particular functions on particular 
 *  instances of hardware.
 */
typedef struct stHRMChipConfigTag
{
   
   DWORD dwUserHardwareID;		/* @field A user assigned 32 bit value which is unique
								 * per instance of EMU8010 hardware found in the system. 
								 * Since some hardware identification and configuration 
								 * systems (such as Microsoft Plug and Play) will
								 * tell you ONLY about a single FUNCTION on a single EMU8010
								 * at any given time, multiple calls to hrmDiscoverChip are
								 * required. This ID, assigned by the user, is used to 
								 * indicate the hardware correlation between different calls
								 * to hrmDiscoverChip. This field may be assigned to any 
								 * 32 bit number EXCEPT 0xFFFFFFFF */
   HRMFLAGS hrmDiscoverFlags;	/* @field A bit field for indicating individual or
								 * combinations of PCI functions on a single EMU8010.
								 * Values include HRMDISC_SOUND_ENGINE, HRMDISC_JOYSTICK,
								 * HRMDISC_HOST_MODEM. Bit-wise OR these values for combos.*/

   /* The following fields are used when HRMDISC_SOUND_ENGINE
      bit is set in the dwDiscoverFlags call */
   DWORD dwAudioBaseAddress;	/* @field Applicable when HRMDISC_SOUND_ENGINE is set.
								 * Indicates the hardware base address of function 0 */
   DWORD dwHardwareInterruptID; /* @field Applicable when HRMDISC_SOUND_ENGINE is set.
								 * Indicates the hardware interrupt number of function 0 */
   HRMCONTIGMEMORY hbMicRecord;	/* @field Applicable when HRMDISC_SOUND_ENGINE is set.
   								 * Indicates the microphone record buffer */
   HRMCONTIGMEMORY hbSRCRecord;	/* @field Applicable when HRMDISC_SOUND_ENGINE is set.
   								 * Indicates the SRC record buffer */
   HRMCONTIGMEMORY hbFXRecord;	/* @field Applicable when HRMDISC_SOUND_ENGINE is set.
   								 * Indicates the FX record buffer */
   HRMCONTIGMEMORY fxXTRAM;		/* @field Applicable when HRMDISC_SOUND_ENGINE is set.
   								 * Indicates the effects engine external tank memory */
   HRMCONTIGMEMORY smPageTable;	/* @field Applicable when HRMDISC_SOUND_ENGINE is set.
   								 * Indicates the page table */ 
   BOOL bInitSoundEngine;		/* @field Applicable when HRMDISC_SOUND_ENGINE is set.
								 * Indicates whether or not a full hardware initializtion  
								 * should or should NOT occur upon discovery.*/

   /* The following fields are used when HRMDISC_JOYSTICK
      bit is set in the dwDiscoverFlags call */
   DWORD dwJoystickBaseAddress;	/* @field Applicable when HRMDISC_JOYSTICK is set.
								 * Indicates the hardware base address of function 1 */

   /* The following fields are used when HRMDISC_HOST_MODEM
      bit is set in the dwDiscoverFlags call */
   DWORD dwHostModemBaseAddress;/* @field Applicable when HRMDISC_HOST_MODEM is set.
								 * Indicates the hardware base address of function 2 */

   DWORD dwMixerID;				/* @field An unique 32 bit value indicating which mixer is
								 * attached to the EMU8010 */
   HRMFLAGS hrmMixerFlags;		/* @field A bit field indicating which PCI function(s)
								 * must be discovered before allowing the mixer to
								 * be discovered.
								 */

   DWORD totalSystemPhysRam;     /* the total amount of physical RAM in this system  
                                  * in bytes.
                                  */
   DWORD dwHWRevision;           /* The hardware version read from the ASIC */
} HRMCHIPCONFIG;


#endif /* __CFG8210_H__ */
