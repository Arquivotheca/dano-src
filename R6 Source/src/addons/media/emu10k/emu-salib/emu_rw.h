
/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1994. All rights Reserved.
*                             
*******************************************************************************
*/

/*****************************************************************************
*
* Filename: emu_rw.h
*
* Description:  Header file for E-mu 8000 Register Read/Write routines.
*               For use with E-mu 8200 Low Level API
*
* Revision: 0.03, 12/13/94
*******************************************************************************
*/

#ifndef __EMU_RW_H
#define __EMU_RW_H

/**************
** Includes
***************/
#include "datatype.h"
#include "emuerrs.h"

/***********************
** Enumeration Tables
***********************/

typedef enum GlobalFuncTag
{
  RightVolPan,            /* use (1.0 - Pan) to calculate right volume */
  RightVolAuxD,           /* use aux data to control right volume      */
  AudioOn,                /* turn main audio on                        */
  AudioOff,               /* main audio off                            */
  SndMemTristate,         /* tristate sound memory                     */
  SndMemUntristate,       /* untristate sound memory                   */
  PowerDown,              /* put chip into sleep mode                  */
  WakeUp                  /* pull chip out of sleep mode               */
} GlobalFunc;

/*
	The following enumeration table sets up the indexes into the
	Address Table for channel variables.  These values are used for
	iAddr in EmuRead() and EmuWrite().  These are all the voice channel
	variables the user can access directly.
*/

typedef enum VoiceVariableTag
{
  /* Envelope Engine Bypass */
  DisableEnv,
  RawPitch,
  RawFilter,
  RawVolume,

  /* Main Lfo 1 */
  DelayLfo1,
  FreqLfo1,

  /* Aux Lfo 2 */
  DelayLfo2,
  FreqLfo2,

  /* Aux Env 1 */
  DelayEnv1,
  AttackEnv1,
  HoldEnv1,
  DecayEnv1,
  SustainEnv1,
  ReleaseEnv1,

  /* Main Env 2 */
  DelayEnv2,
  AttackEnv2,
  HoldEnv2,
  DecayEnv2,
  SustainEnv2,
  ReleaseEnv2,

  /* Oscillator */
  StartAddrs,
  StartloopAddrs,
  EndloopAddrs,
  InitialPitch,
  Lfo1ToPitch,
  Lfo2ToPitch,
  Env1ToPitch,

  /* Filter */
  InitialFilterFc,
  InitialFilterQ,
  Lfo1ToFilterFc,
  Env1ToFilterFc,

  /* Amplifier */
  InitialVolume,
  Lfo1ToVolume,
  
  /* Effects */
  ChorusEffectsSend,
  ReverbEffectsSend,
  PanEffectsSend,
  AuxEffectsSend,

  /* Low level Emu8000 sound engine parameters */
  CurrentPitch,
  Fraction,
  Stereo,
  CurrentVolume,
  CurrentFilter,
  FilterDelayMemory1,
  FilterDelayMemory2,

  LastVoiceVariable
} VoiceVariable;

/*
   The following enumeration table sets up indexes into the Address
   Table for the frequently used global variables.  It begins
   numbering at the last value of the VoiceVariable enumeration.  These
   values are also used with EmuRead() and EmuWrite().  The iChannel
   value in EmuRead() or WmuWrite() is ignored when one of these variables
   is written.
*/

typedef enum GlobalVariableTag
{
  WallClock = LastVoiceVariable,    /* 16 bit counter                      */
  ChanUnderService,                 /* (0 - 0x1F)                          */
  OpComplete,                       /* Operation complete                  */
  RotaryEncoder,                    /* [s.4] signed delta since last read  */
  Potentiometer,                    /* [0.8] linear fraction of full scale */
  SmallPointer,                     /* Pointer reg in small address mode     
                                       [3.5] where 3 msb are extended         
                                       address and 5 lsb are voice pointer */
  LargePointer,                     /* Pointer reg in large address mode      
                                       [0.5] where 5 lsb are voice pointer */  
  LRDMAEmpty,                       /* Left Read DMA empty (boolean)       */
  RRDMAEmpty,                       /* Right Read DMA empty (boolean)      */
  LWDMAFull,                        /* Left Write DMA empty (boolean)      */
  RWDMAFull,                        /* Right Write DMA empty (boolean)     */
  DMALeftReadAddrs,                 /* Left Read DMA position              */
  DMARightReadAddrs,                /* Right Read DMA position             */
  DMALeftWriteAddrs,                /* Left Write DMA position             */
  DMARightWriteAddrs                /* Right Write DMA position            */

} GlobalVariable;

/************************
** Function Prototypes
************************/

EMUCTYPE EMUSTAT   EmuGlobal(BYTE byFunction);
EMUCTYPE EMUSTAT   EmuRead(WORD uiVoice, BYTE byAddress, DWORD *pdwData);
EMUCTYPE EMUSTAT   EmuWrite(WORD uiVoice, BYTE byAddress, DWORD dwData);
EMUCTYPE EMUSTAT   EmuWriteVoice(WORD wEvent, WORD wSubEvent, WORD wSubSubEvent,
                             BYTE byAddress, DWORD dwData);


#endif /* __EMU_RW_H */

/*********************** End of EMU_RW.H *****************************/
