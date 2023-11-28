
/*********************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* Filename: hal8210.c
*
* Description: Contains all E-mu 8010 PC specific read/write
*              functions.
*
*
* Visible Routines: WGRead
*                   WSEPtrWrite
*                   LSEPtrRead
*                   LSEPtrWrite
*
*
*******************************************************************************
*
* History:
*
* Version    Person        Date         Reason
* -------    ---------   -----------  ---------------------------------------

*  0.001     Steve Verity  Mar 11, 97   Initial Creation
*  0.002     Mike Guzewicz Jul 24, 97   Moved from G8010RW.C
*										Support multiple 8010s
*										Add halInit, halDiscoverChip
*  0.003	 John Kraft	   Sep 24, 97	Changed things to use the 8210 name scheme.
*
******************************************************************************
*/
#ifndef __HAL8210_C
#define __HAL8210_C
 
/************
* Includes
*************/

#include <string.h>
#include "hal8210.h"
#include "pcio.h"
#include "cfg8210.h"
#include "dbg8210.h"


/***********
 * Defines
 ***********/

#define HAL_MAGIC		0x11A1L
#define MAKE_HALID(_x)		((HAL_MAGIC << 16) | (_x))
#define IS_VALID_HALID(_x)      (((_x) >> 16) == HAL_MAGIC)

#define MAX_TIMEOUT 0x0100


/*********************
 * Private Structures
 ********************/

typedef struct stE8010HALStateTag
{ 
	HRMCHIPCONFIG chipConfig; /* Public configuration structure  */
	BOOL bSoundEngine, bJoystick, bHostModem;
	DWORD dwLastPtrRegData;      /* Last programmed value of PTRREG */
	BYTE  byLastACIndexData;     /* Last programmed value of AC97 Index */
} stHALState, HALSTATE;


/***************************
 * Private Global Variables
 **************************/

BOOL bIsInit = FALSE;
HALSTATE chipStates[MAX_CHIPS];


/******************
 * Private MACROs
 ******************/

//#define GETHALSTATE(a) (stHALState *)(((WORD)(a)>=MAX_CHIPS) ? NULL : &chipStates[a])
//#define GETACTIVEHALSTATE(a) (stHALState *)(((a)>=wDiscoveredChipCount) ? NULL : &chipStates[a])

#ifdef __HAL_NOINTELLIGENCE
#define BYPASSINTELLIGENCE() (0==0)
#else
#define BYPASSINTELLIGENCE() (0==1)
#endif

#define ALL_STATES 0xFFFFFFFF
#define NO_ID 0xFFFFFFFF
#define RESET_BYTE 0xFF
 
/*********************************************************
 * Private Function Prototypes used by Public Functions
 ********************************************************/
 
BOOL        halpDetectChip(HALID id);
stHALState *halpGetActiveHALState(HALID id);
stHALState *halpGetActiveHALStateHWID(DWORD hwID, HALID *id);
stHALState *halpGetNextAvailableHALState(HALID *id);
void        halpFreeHALState(HALID id);


/*********************
 * BeOS driver call
 ********************/
 
 #ifdef __BEOS__
 #include <OS.h>
 #include "emu10k_driver.h"
 #include <unistd.h>
 
 static uint32
 beos_driver_call(stHALState* state, int32 op,
 					uint32 index, uint32 bytes, uint32 data)
 {
 	emu_reg_spec reg;
 	reg.index = index;
 	reg.bytes = bytes;
 	reg.data = data;
 
 	ioctl(state->chipConfig.dwUserHardwareID, op, &reg, sizeof(reg));
 	return reg.data;
 }
 
 #endif


/********************************
 * Public Method Implementations
 *******************************/


EMUAPIEXPORT EMUSTAT halInit(void)
{ 
   halpFreeHALState(ALL_STATES);
   bIsInit=TRUE;
   return SUCCESS;
}

EMUAPIEXPORT EMUSTAT halUnDiscoverChip(HALID id, DWORD flags)
{
	stHALState *state = halpGetActiveHALState(id);
	if (state==NULL) return EMUCHIP_TIMEOUT;
	if (flags & HRMDISC_SOUND_ENGINE)
		state->bSoundEngine=FALSE;
	if (flags & HRMDISC_JOYSTICK)
		state->bJoystick=FALSE;
	if (flags & HRMDISC_HOST_MODEM)
		state->bHostModem=FALSE;
	if ((state->bSoundEngine==FALSE) &&
		(state->bJoystick==FALSE) &&
		(state->bHostModem==FALSE))
		halpFreeHALState(id);
	return SUCCESS;
}

EMUAPIEXPORT DWORD halGetUserHardwareID(HALID id)
{
	stHALState *state = halpGetActiveHALState(id);
	if (state==NULL) return NO_ID;
	return state->chipConfig.dwUserHardwareID;
}

EMUAPIEXPORT BOOL halIsDiscovered(HALID id, HRMFLAGS flags)
{
	BOOL retVal=TRUE;
	stHALState *state = halpGetActiveHALState(id);
	if (state==NULL) return FALSE;
	if (flags&HRMDISC_SOUND_ENGINE)
		retVal &= state->bSoundEngine;
	if (flags&HRMDISC_JOYSTICK)
		retVal &= state->bJoystick;
	if (flags&HRMDISC_HOST_MODEM)
		retVal &= state->bHostModem;
	return retVal;
}


EMUAPIEXPORT BOOL halIsValidID(HALID id)
{
	if (IS_VALID_HALID(id) && ((id & 0xFFFF) < MAX_CHIPS))
		return TRUE;
	else 
		return FALSE;
}


EMUAPIEXPORT EMUSTAT halDiscoverChip(HALID *id, HRMCHIPCONFIG *config)
{
	stHALState *state;

	if (bIsInit==FALSE) halInit();

	state = halpGetActiveHALStateHWID(config->dwUserHardwareID, id);
	if (state==NULL)
	{
		state = halpGetNextAvailableHALState(id);
		if (state == NULL) return EMUCHIP_TIMEOUT;
		state->chipConfig.dwUserHardwareID = config->dwUserHardwareID;
	} 

	if ((state->bSoundEngine == FALSE) && (config->hrmDiscoverFlags & HRMDISC_SOUND_ENGINE))
	{
		if (halpDetectChip(config->dwAudioBaseAddress) == FALSE)
		{
			return EMUCHIP_TIMEOUT;
		}
		state->chipConfig.dwAudioBaseAddress = config->dwAudioBaseAddress; 
		state->dwLastPtrRegData = 0x80000000;
		state->byLastACIndexData = 0xFF; 
		state->bSoundEngine = TRUE;
	}

	if ((state->bJoystick == FALSE) && (config->hrmDiscoverFlags & HRMDISC_JOYSTICK))
	{
		state->chipConfig.dwJoystickBaseAddress = config->dwJoystickBaseAddress;
		state->bJoystick = TRUE;
	}

	if ((state->bHostModem == FALSE) && (config->hrmDiscoverFlags & HRMDISC_HOST_MODEM))
	{
		state->chipConfig.dwHostModemBaseAddress = config->dwHostModemBaseAddress;
		state->bHostModem = TRUE;
	}

	return SUCCESS;
}

/***************************************************************************
*
*  Name: LSEPtrWrite() WSEPtrWrite(),  BSEPtrWrite() 
*
*  Description: Write a DWORD, WORD, or BYTE an 8010 "RGA" register.  
*
*  Inputs: dwRegAddr : 32 bit "virtual" register address
*          dwData or wData, or byData: DWORD, WORD, or BYTE to write        
*
*  Name: LSEPtrRead() WSEPtrRead(),  BSEPtrRead() 
*     
*  Description: Read a DWORD, WORD, or BYTE an 8010 "RGA" register.  
*
*  Inputs: dwRegAddr : 32 bit "virtual" register address
*          dwData or wData, or byData: DWORD, WORD, or BYTE pointer to data
*          to be read.         
*
*
*  Operation: 
*
*    The 8010 "RGA" registers are virtual registers that are accessed through
*    a pointer register. To access these registers, the driver must first
*    write the register address to the pointer register, then read/write the data 
*	 to/from the data register.
*
*    Using these routines, along with the register name constants
*    in G8010rw.h will automatically take care of these pointer/data 
*    housekeeping  details, for example:  
*   
*    DWORD myData;
*  
*    LSEPtrWrite(PTBA, myData)
*   
*    writes a 32 bit value to the page table register.  
*   
*  Channel and Non-channel Registers: 
*   
*    There are two type of RGA registers: channel and non-channel.  In the 
*    case of channel registers, there is one register for each channel.  
*	 The channel is specified in bits 5:0 of the pointer register.  To read or 
*    write a channel register, the caller should "or" the channel number with
*	 the register name constant, for example:
*
*    DWORD myData;
*	 BYTE  channel = 6;
*
*    LSEPtrWrite(CPF | channel, myData);
*
*    writes a 32 bit value to the Current Pitch and Fraction register 
*    for channel 6 
*
*    Channel bits for non-channel registers are ignored
*
* Reading RGA registers:
*	
*    Reading an RGA register works in a similar manner, for example:  
*    
*    DWORD MyData;
*	 BYTE  channel = 6;
*
*    MyData = LSEPtrRead(CPF | channel);
*	  
*	 Will set the value of myData to the current value to the Current 
*    Pitch and Fraction register for channel 6 
*
*
* Implementation details:
*
*    The data register is at location 0 of PCI function 0 (dwAudioBaseAddress),
*    and the the pointer register is at location 4. In the case on "Wintel" 
*	 PC clones, PCI I/O is limited to 65535 locations, and uses the "X86" 
*	 I/O instructions for access, however, the base address is a DWORD to be 
*	 compatible with other 	possible PCI implementations.
*
*    These routines try to be somewhat smart in that they
*    shadow the pointer register and do not re-write it if it has not changed.  
*
* Multi-thread considerations:  
* 
*    The write to the pointer and read/write data register must be an atomic
*    operation, which in the case of WINTEL means disabling interrupts around 
*    these two operations, 
*
* Warnings:  
*
*    Do not use these routines to talk to a non-RGA register, for
*    example, the Wall clock register, which has its own actual I/O location. 
*    Use PCIRegRead() and PCIRegWrite() to talk to these registers.
*  
*    The Word and Byte versions of these routines do not execute any faster 
*    than the DWORD version, since the minimum transaction on the PCI bus is
*    32 bits.   
*
*  
*
* TODO:
*        
* 	 disabling interrups around the prt/data reg write has not been 
*    implimented- yet.
*	  
****************************************************************************
*/

EMUAPIEXPORT DWORD E8010RegRead(HALID id, WORD function, WORD sizeType, WORD funcOffs)
{
	switch (function)
	{
	case EMU8010FUNC0:
		switch(sizeType)
		{
		case EMU8010BYTE:
			return (DWORD)B8010SERegRead(id, funcOffs);
		case EMU8010WORD:
			return (DWORD)W8010SERegRead(id, funcOffs);
		case EMU8010LONG:
			return L8010SERegRead(id, funcOffs);
		default:
			return 0L;
		}
	case EMU8010FUNC1:
		switch (sizeType)
		{
                case EMU8010BYTE:
                        return B8010JoyRegRead(id, funcOffs);
                case EMU8010LONG:
		case EMU8010WORD:
		default:
			return 0; 
		} 
	default:
		return 0;
	}
}

EMUAPIEXPORT void E8010RegWrite(HALID id, WORD function, WORD sizeType, WORD funcOffs, DWORD val)
{

	switch (function)
	{
	case EMU8010FUNC0:
		switch(sizeType)
		{
		case EMU8010BYTE:
			B8010SERegWrite(id, funcOffs, (BYTE)val);
			return;
		case EMU8010WORD:
			W8010SERegWrite(id, funcOffs, (WORD)val);
			return;
		case EMU8010LONG:
			L8010SERegWrite(id, funcOffs, val);
		default:
			return;
		}
	case EMU8010FUNC1:
		switch (sizeType)
		{
                case EMU8010BYTE:
                        B8010JoyRegWrite(id, funcOffs, (BYTE)val);
			return;
		default:
			return; 
		} 
	default:
		return;
	}
}

EMUAPIEXPORT void  B8010SERegWrite(HALID id, WORD funcOffs, BYTE byData)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return;

#ifdef __BEOS__
	beos_driver_call(state, EMU_WRITE_SE_REG, funcOffs, 1, byData);
#else
	if (funcOffs==PTRREG) state->dwLastPtrRegData=(DWORD)byData;
	EmuOutB(state->chipConfig.dwAudioBaseAddress+funcOffs, byData);
#endif
}

EMUAPIEXPORT BYTE  B8010SERegRead(HALID id, WORD funcOffs)
{ 
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return 0;

#ifdef __BEOS__
	return beos_driver_call(state, EMU_READ_SE_REG, funcOffs, 1, 0);
#else
	return EmuInB(state->chipConfig.dwAudioBaseAddress+funcOffs);
#endif
}

EMUAPIEXPORT void  W8010SERegWrite(HALID id, WORD funcOffs, WORD data)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return;

#ifdef __BEOS__
	beos_driver_call(state, EMU_WRITE_SE_REG, funcOffs, 2, data);
#else
	if (funcOffs==PTRREG) state->dwLastPtrRegData=(DWORD)data;
	EmuOut(state->chipConfig.dwAudioBaseAddress+funcOffs, data);
#endif
}

EMUAPIEXPORT WORD  W8010SERegRead(HALID id, WORD funcOffs)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return 0;

#ifdef __BEOS__
	return beos_driver_call(state, EMU_READ_SE_REG, funcOffs, 2, 0);
#else
	return EmuIn(state->chipConfig.dwAudioBaseAddress+funcOffs);
#endif
}

EMUAPIEXPORT void  L8010SERegWrite(HALID id, WORD funcOffs, DWORD data)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return;

#ifdef __BEOS__
	beos_driver_call(state, EMU_WRITE_SE_REG, funcOffs, 4, data);
#else
	if (funcOffs==PTRREG) state->dwLastPtrRegData=data;
	EmuOutD(state->chipConfig.dwAudioBaseAddress+funcOffs, data);
#endif
}

EMUAPIEXPORT DWORD L8010SERegRead(HALID id, WORD funcOffs)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return 0;

#ifdef __BEOS__
	return beos_driver_call(state, EMU_READ_SE_REG, funcOffs, 4, 0);
#else
	return EmuInD(state->chipConfig.dwAudioBaseAddress+funcOffs);
#endif
}

EMUAPIEXPORT void B8010JoyRegWrite(HALID id, WORD funcOffs, BYTE data)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return;

#ifdef __BEOS__
	beos_driver_call(state, EMU_WRITE_JOY_REG, funcOffs, 1, data);
#else
    EmuOutB(state->chipConfig.dwJoystickBaseAddress+funcOffs, data);
#endif
}

EMUAPIEXPORT BYTE B8010JoyRegRead(HALID id, WORD funcOffs)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return 0;

#ifdef __BEOS__
	return beos_driver_call(state, EMU_READ_JOY_REG, funcOffs, 1, 0);
#else
    return EmuInB(state->chipConfig.dwJoystickBaseAddress+funcOffs);
#endif
}

EMUAPIEXPORT BYTE B8010JoyRead(HALID id)
{
	stHALState *state;
	BYTE bRetVal;
	if ((state = halpGetActiveHALState(id)) == NULL) return 0;

#ifdef __BEOS__
	bRetVal = beos_driver_call(state, EMU_READ_JOYSTICK, 0, 1, 0);
#else
	IDISABLE();
		EmuOutB(state->chipConfig.dwJoystickBaseAddress, 0);
		bRetVal = EmuInB(state->chipConfig.dwJoystickBaseAddress);
	IENABLE();
#endif

	return bRetVal;
}

EMUAPIEXPORT void Index8010Write(HALID id, WORD funcNum, WORD indexReg, WORD dataSize, DWORD addr, DWORD data)
{ 

	switch(funcNum)
	{
		case EMU8010FUNC0:
			switch (indexReg)
			{
				case PTRREG:
					switch(dataSize)
					{
						case EMU8010WORD:
							WSEPtrWrite(id, addr, (WORD)data);
							return;
						case EMU8010LONG:
							LSEPtrWrite(id, addr, (DWORD)data);
							return;
						default: // which data size
							return;
					}
					break;

				case AC97A:
					if (dataSize!=EMU8010WORD) return;
					AC97Write(id, (BYTE)addr, (WORD)data);
					return;
					break;
				default: // which index register
					return;
			}
			break;
		default: // which function
			return;
	}
	return;
}

EMUAPIEXPORT DWORD Index8010Read(HALID id, WORD funcNum, WORD indexReg, WORD dataSize, DWORD addr)
{ 

	WORD  wData;
	DWORD dwData;

	switch(funcNum)
	{
		case EMU8010FUNC0:
			switch(indexReg)
			{
				case PTRREG:
					switch(dataSize)
					{
						case EMU8010WORD:
							WGRead(id, addr, (&wData));
							return (DWORD)wData;
							break;
						case EMU8010LONG:
							LGRead(id, addr, &dwData);
							return dwData;
							break;
						default: // which data size
							return 0;
					}
					break;
				case AC97A:
					if (dataSize != EMU8010WORD) return 0L;
					return (DWORD)AC97Read(id, (BYTE)addr);
					break;
				default: // which index register
					return 0L;
					break;
			}
			break;
		default:  // which function
			break;
	}
	return 0L;
}

EMUAPIEXPORT void  LSEPtrWrite(HALID id, DWORD dwRegAddr, DWORD dwData) 
{
  stHALState *state;
  if ((state = halpGetActiveHALState(id)) == NULL) return;

#ifdef __BEOS__
  beos_driver_call(state, EMU_WRITE_SE_PTR, dwRegAddr, 4, dwData);
#else
  IDISABLE();
	  if (BYPASSINTELLIGENCE() || (dwRegAddr != state->dwLastPtrRegData))
	  {
		EmuOutD(state->chipConfig.dwAudioBaseAddress + PTRREG, dwRegAddr);  /*  pointer reg  */
		state->dwLastPtrRegData = dwRegAddr;
	  }

	  EmuOutD(state->chipConfig.dwAudioBaseAddress + DATAREG, dwData);              /*  data reg */ 
  IENABLE();
#endif
}


EMUAPIEXPORT void  WSEPtrWrite(HALID id, DWORD dwRegAddr, WORD uiData)
{
  stHALState *state;
  if ((state = halpGetActiveHALState(id)) == NULL);

#ifdef __BEOS__
  beos_driver_call(state, EMU_WRITE_SE_PTR, dwRegAddr, 2, uiData);
#else
  IDISABLE();
	  if (BYPASSINTELLIGENCE() || (dwRegAddr != state->dwLastPtrRegData))
	  {
		EmuOutD(state->chipConfig.dwAudioBaseAddress + PTRREG, dwRegAddr);  
		state->dwLastPtrRegData = dwRegAddr;
	  }

	  EmuOut(state->chipConfig.dwAudioBaseAddress + DATAREG, uiData);               
  IENABLE();
#endif
}

EMUAPIEXPORT void  BSEPtrWrite(HALID id, DWORD dwRegAddr, BYTE byData)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return;

#ifdef __BEOS__
	beos_driver_call(state, EMU_WRITE_SE_PTR, dwRegAddr, 1, byData);
#else
	IDISABLE();
		if (BYPASSINTELLIGENCE() || (dwRegAddr != state->dwLastPtrRegData)) {
			EmuOutD(state->chipConfig.dwAudioBaseAddress + PTRREG, dwRegAddr); 
			state->dwLastPtrRegData = dwRegAddr;
		}

		EmuOutB(state->chipConfig.dwAudioBaseAddress + DATAREG, byData);  
	IENABLE();
#endif
}


EMUAPIEXPORT WORD  WSEPtrRead(HALID id, DWORD dwRegAddr)
{
	stHALState *state;
	WORD wRetVal;

	if ((state = halpGetActiveHALState(id)) == NULL) return EMUCHIP_TIMEOUT;

#ifdef __BEOS__
	wRetVal = beos_driver_call(state, EMU_READ_SE_PTR, dwRegAddr, 2, 0);
#else
	IDISABLE();
		if (BYPASSINTELLIGENCE() || (dwRegAddr != state->dwLastPtrRegData))
		{
			EmuOutD( state->chipConfig.dwAudioBaseAddress + PTRREG, dwRegAddr);   /*  pointer reg  */
			state->dwLastPtrRegData = dwRegAddr;
		}

		wRetVal = EmuIn(state->chipConfig.dwAudioBaseAddress + DATAREG); 
	IENABLE();
#endif

	return wRetVal;
}

EMUAPIEXPORT DWORD  LSEPtrRead(HALID id, DWORD dwRegAddr)
{
	stHALState *state;
	DWORD dwRetVal;

	if ((state = halpGetActiveHALState(id)) == NULL) return EMUCHIP_TIMEOUT;

#ifdef __BEOS__
	dwRetVal = beos_driver_call(state, EMU_READ_SE_PTR, dwRegAddr, 4, 0);
#else
	IDISABLE();
		if (BYPASSINTELLIGENCE() || (dwRegAddr != state->dwLastPtrRegData))
		{
			EmuOutD( state->chipConfig.dwAudioBaseAddress + PTRREG, dwRegAddr );  /*  pointer reg  */
			state->dwLastPtrRegData = dwRegAddr;
		}

		dwRetVal = EmuInD(state->chipConfig.dwAudioBaseAddress + DATAREG);  
	IENABLE();
#endif

	return dwRetVal;
}


EMUAPIEXPORT BYTE BSEPtrRead(HALID id, DWORD dwRegAddr)
{
	stHALState *state;
	BYTE bRetVal;

	if ((state = halpGetActiveHALState(id)) == NULL) return EMUCHIP_TIMEOUT;

#ifdef __BEOS__
	bRetVal = beos_driver_call(state, EMU_READ_SE_PTR, dwRegAddr, 1, 0);
#else
	IDISABLE();
		if (BYPASSINTELLIGENCE() || (dwRegAddr != state->dwLastPtrRegData))
		{
			EmuOutD( state->chipConfig.dwAudioBaseAddress + PTRREG, dwRegAddr );  /*  pointer reg  */
			state->dwLastPtrRegData = dwRegAddr;
		}

		bRetVal = EmuInB(state->chipConfig.dwAudioBaseAddress + DATAREG);  
	IENABLE();
#endif

	return bRetVal;
}

EMUAPIEXPORT EMUSTAT
halWaitWallClockCounts(HALID hid, DWORD wait)
{
	DWORD count, last, curr;
	DWORD  timeout;
	stHALState *state;
	if ((state = halpGetActiveHALState(hid)) == NULL) return EMUCHIP_TIMEOUT;

	last = curr = L8010SERegRead(hid, WC) >> 6;

	for (count=0; count<wait; count++)
	{
		timeout=0;
		do {
			curr = L8010SERegRead(hid, WC) >> 6;

			timeout++;
			if (timeout > MAX_TIMEOUT)
				return 1;
		} while (curr == last);
		
		last = curr;
	}
	return 0;
}

EMUAPIEXPORT void AC97Write(HALID id, BYTE addr, WORD data)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return;

#ifdef __BEOS__
	beos_driver_call(state, EMU_WRITE_AC97, addr, 2, data);
#else
	IDISABLE();
		if (BYPASSINTELLIGENCE() || (state->byLastACIndexData != addr))
		{
			EmuOutB(state->chipConfig.dwAudioBaseAddress+AC97A, addr);
			state->byLastACIndexData = addr;
		}
		EmuOutW(state->chipConfig.dwAudioBaseAddress+AC97D, data);
	IENABLE();
#endif
}

EMUAPIEXPORT WORD AC97Read(HALID id, BYTE addr)
{
	stHALState *state;
	WORD wRetVal;

	if ((state = halpGetActiveHALState(id)) == NULL) return 0;

#ifdef __BEOS__
	wRetVal = beos_driver_call(state, EMU_READ_AC97, addr, 2, 0);
#else
	IDISABLE();
		if (BYPASSINTELLIGENCE() || (state->byLastACIndexData != addr))
		{
			EmuOutB(state->chipConfig.dwAudioBaseAddress+AC97A, addr);
			state->byLastACIndexData=addr;
		}
		wRetVal = EmuInW(state->chipConfig.dwAudioBaseAddress+AC97D);
	IENABLE();
#endif

	return wRetVal;
}


EMUAPIEXPORT DWORD L8010ModemRegRead (HALID id, WORD func3Offset)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return 0;

#ifdef __BEOS__
	return beos_driver_call(state, EMU_READ_MODEM_REG, func3Offset, 4, 0);
#else
	return EmuInD(state->chipConfig.dwHostModemBaseAddress + func3Offset);
#endif
}


EMUAPIEXPORT void  L8010ModemRegWrite(HALID id, WORD func3Offset, DWORD data)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return;

#ifdef __BEOS__
	beos_driver_call(state, EMU_WRITE_MODEM_REG, func3Offset, 4, data);
#else
	(void) EmuOutD(state->chipConfig.dwHostModemBaseAddress + func3Offset, 
	               data);
#endif
}


EMUAPIEXPORT BYTE B8010ModemRegRead(HALID id, WORD func3Offset)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return 0;

#ifdef __BEOS__
	return beos_driver_call(state, EMU_READ_MODEM_REG, func3Offset, 1, 0);
#else
	return EmuInB(state->chipConfig.dwHostModemBaseAddress + func3Offset);
#endif
}


EMUAPIEXPORT void B8010ModemRegWrite(HALID id, WORD func3Offset, BYTE data)
{
	stHALState *state;
	if ((state = halpGetActiveHALState(id)) == NULL) return;

#ifdef __BEOS__
	beos_driver_call(state, EMU_WRITE_MODEM_REG, func3Offset, 1, data);
#else
	EmuOutB(state->chipConfig.dwHostModemBaseAddress + func3Offset, 
	        data);
#endif
}


/************************************
 * Private function implementations 
 ***********************************/

void halpFreeHALState(HALID id)
{
	if (id==ALL_STATES)
	{
	  memset(chipStates, RESET_BYTE, sizeof(chipStates)); 
	}
    else 
    {
        DWORD dwIndex = id & 0xFFFF;

        if (dwIndex < MAX_CHIPS)
            memset(&chipStates[dwIndex], RESET_BYTE, sizeof(HALSTATE)); 
    }
}

stHALState *halpGetActiveHALState(HALID id)
{
	/* Verify and then strip off the magic number */
	if (!IS_VALID_HALID(id)) return NULL;
	id &= 0xFFFF;

	if (id >= MAX_CHIPS) return NULL; 
	return &chipStates[id];
}

stHALState *halpGetActiveHALStateHWID(DWORD dwHardwareID, HALID *id)
{

	WORD count;
	*id = 0x0;
	for (count=0; count<MAX_CHIPS; count++)
	{
		if (chipStates[count].chipConfig.dwUserHardwareID == dwHardwareID)
		{
			*id = MAKE_HALID(count);
			return &chipStates[count];
		}
	}
	return NULL;
}


stHALState *halpGetNextAvailableHALState(HALID *id)
{
	WORD count;
	for (count=0; count<MAX_CHIPS; count++)
	{
		if (chipStates[count].chipConfig.dwUserHardwareID == 0xFFFFFFFF)
		{
			memset(&chipStates[count], 0, sizeof(stHALState)); 
			*id = MAKE_HALID(count);
			return &chipStates[count];
		}
	}
    
    /* Paranoia: We should never run out of chipStates entries */
    ASSERT(0);
    return NULL;
}


BOOL halpDetectChip(HALID id)
{
	/* Awaiting the magic chip detection algorithm, but until then... */
	return TRUE;
}

#endif



