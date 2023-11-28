/*****************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*					
******************************************************************************
* @doc INTERNAL
* @module se8210.c | 
*  This module contains the implementation of the SE8210 sound engine API.
*  The API is fully documented in the EXTERNAL section of se8210.h.  Please
*  see that file for more details.
*
* @iex
* Revision History:
* Version	Person      Date		Reason
* -------   ---------	----------	--------------------------------------
*  0.006
*  0.005		JK		Oct 23, 97  Added register initialization for the Ancillary
*									registers in the sound engine.
*  0.004		JK		Sep 30, 97	Moved some definitions out to se8210p.h;
*									tweaked the start routines to deal with
*									some problems with the QKBCA register;				
*  0.003		JK		Sep 24, 97	Fixed the setup routines to handle arrays of
*									  of SEVOICESETUP pointers.  Also added support
*									  for interleaving, and changed things over to
*									  use the 8210 naming scheme.
*  0.002		JK		Sep 09, 97	Put in SEVOICE support and wrote functions
*										for voice startup and shutdown.
*  0.001		MG		Jun 02, 97	Initial version
* @end
*******************************************************************************
*/

#ifndef __SE8210_C
#define __SE8210_C


/************
* Includes
************/

#include <stdio.h>		/* For sprintf() */
#include <string.h>		/* For string manipulation functions */
#include <assert.h>		/* Useful debugging fun */

#include "dbg8210.h"
#include "hal8210.h"	/* EMU8010 Register specific stuff */ 
#include "os8210.h"		/* For osHeapAlloc() */
#include "sm8210.h"
#include "se8210.h"		/* Basic sound engine public definitions */
#include "se8210p.h"	/* Sound engine private definitions */
#include "se8210d.h"    /* Sound engine discovery routines */
#include "hrm8210.h"
#include "emuerrs.h"	/* Defines SUCCESS */


/*****************************************************************************
 * @doc INTERNAL
 * Defines
 ****************************************************************************/


/* The following bits are stored in the byRegMode field of the voice.  They
 * Indicate when the hardware register associated with a parameter should
 * be written or read from by the seParameter routines.  In seParameterWrite
 * the values control whether we actually read the register or just return
 * the value stored in the parameter array.  In seParameterWrite, we always
 * update the value in the parameter array, but the value controls whether we
 * also write to the backing register.  Pure virtual parameters are software
 * parameters and have no backing register, so we always read and write only
 * from the parameter array.
 */

#define PURE_VIRTUAL			0x0
#define NEVER_READ				0x0
#define NEVER_WRITE				0x0
#define READ_WHEN_STOPPED		0x1
#define READ_WHEN_PLAYING		0x2
#define READ_WHEN_RELEASING		0x4
#define READ_WHEN_SOUNDING		(READ_WHEN_PLAYING|READ_WHEN_RELEASING)
#define WRITE_WHEN_STOPPED		0x10
#define WRITE_WHEN_PLAYING		0x20
#define WRITE_WHEN_RELEASING	0x40
#define WRITE_WHEN_SOUNDING		(WRITE_WHEN_PLAYING|WRITE_WHEN_RELEASING)
#define READ_ALWAYS				0xF
#define WRITE_ALWAYS			0xF0
#define READWRITE_WHEN_SOUNDING	(READ_WHEN_SOUNDING|WRITE_WHEN_SOUNDING)
#define READWRITE_ALWAYS		(READ_ALWAYS|WRITE_ALWAYS)


#define MAX_TIMEOUT 0x1000
#define ENV_ENGINE_ON(a)  ((a) | 0x80)
#define ENV_ENGINE_OFF(a) ((a) & 0xFFFF7FFF)
#define ENV_ENGINE_RELEASE(a) ((a) | 0x8080)

/*  used in the envelope engine's IP register into the representation
 *  used by the sound engine's CP register.
 */
#define IP_TO_CP(ip) \
   ((ip == 0) ? 0 : ((((DWORD) 0x00001000 | (ip & (DWORD)0x00000FFF)) << \
					 (((ip >> 12) & (DWORD)0x000F) + 4)) & (DWORD)0xFFFF0000))

/* @func DWORD | IV_TO_CV | Converts from the amplitude representation
 *  used in the envelope engine's InitialVolume register.
 */
#define IV_TO_CV(iv) \
	(((DWORD)(0x10 | (~iv & 0x0F)) << (((~iv >> 4) & 0x0F) + 12)) & 0xFFFF0000L)

#define USE_TIMER

#define RAMP_MAX_TIME      100 // 10 ms
#define RAMP_MAX_LOOKAHEAD 480 // 10 ms at 48khz
#define RAMP_MIN_RANGE      32
#define RAMP_MIN_RANGE_INC   1
#define RAMP_ZERO_CROSS_INC  8

#define FIRST_SYNCABLE_VOICE 0
#define LAST_SYNCABLE_VOICE 31

/******************************************************************************
 * Structures
 ******************************************************************************/

/* 
 * @struct stParamData | This structure is used by the Parameter read
 *  and write routines to determine what precisely needs to be done when
 *  a parameter changes.  For the vast majority of parameters, the parameter
 *  routines simply go out and manipulate the appropriate fields of the
 *  hardware registers.  Some parameters, however, are virtualized and
 *  are actually emulated via some aspect of the software.  For these
 *  virtual registers, special code must be invoked when they are read
 *  or written.
 */

typedef struct stParamDataTag
{
	DWORD dwAddress;		/* @field The address of the hardware register
							 *  associated with this parameter.			*/
	DWORD dwMask;			/* @field A mask applied to the hardware
							 *  register to extract the appropriate param bits */
	DWORD dwValidInfoMask;  /* @field For read, masks off all but valid info */
	BYTE  byShift;          /* @field Number of bits to shift data		*/
	BYTE  byNumofWords;     /* @field Register size in 16-bit words		*/
	BYTE  byReadOnly;       /* @field Flag indicating read only stuff   */
	BYTE  byRegMode;		/* @field A bit field describing under what
								circumstances the register is read or written.
								See the comment for the PURE_VIRTUAL define. */

} stParamData;


/*****************************************************************************
 * Forward declarations of private functions
 *****************************************************************************/

static EMUSTAT _prepareVoice(stVoice *voice, DWORD *dwParams, 
                             DWORD *dwPitch, WORD *wVolEnv);
static void    _completeSyncSetup(stVoice *voice);
static SEVOICE _findVoice(SEVOICE voice);
static void    _stopStereoVoice(stVoice *voice);
static void    _stopVoice(stVoice *voice);
static void    _updateStopOnLoop(stVoice *voice, BOOL bSetStopOnLoop);
#ifdef USE_TIMER
static BOOL    _timerISRCallback(ITMSCHEDINFO*);
static BOOL    _timerCallback(ITMSCHEDINFO*);
#endif
#ifdef SE_RAMP_SENDS
static void    _rampVoiceSend(stVoice*, DWORD, DWORD);
static BOOL    _rampISRCallback(ITMSCHEDINFO*);
static BOOL    _rampCallback(ITMSCHEDINFO*);
static void    _GetNextRampUpdate(stVoiceRampData*, DWORD*);
static DWORD   _ComputeRampPitch(stVoice*);
#endif

/*****************************************************************************
 * Private static data declarations 
 ****************************************************************************/

/* @globalv The total number of active se8210's configured.
 */
WORD seCount = 0;

/* @globalv A complete record of the chip state for each of the discovered
 *  sound engines.		*/
stSEState seStates[MAX_CHIPS];

/* @globalv A table containing information on all of the supported params. */
static const stParamData AddressTable[sepLastGlobalParam] = 
{
/* Envelope engine bypass */
VEDS, 0x00000080L,0x00000001L, 7,1,0,
	READWRITE_WHEN_SOUNDING,			/* Disable envelope engine -> off bit  0 */
PTAB, 0xFFFF0000L,0x0000FFFFL,16,2,0,
	READ_ALWAYS|WRITE_WHEN_SOUNDING,	/* Raw pitch -> pitch target 1 */
VTFT, 0x0000FFFFL,0x0000FFFFL,0,2,0,
	READWRITE_WHEN_SOUNDING,			/* Raw filter cutoff -> filter target 2 */
VTFT, 0xFFFF0000L,0x0000FFFFL,16,2,0,
	READWRITE_WHEN_SOUNDING,			/* Raw volume -> volume target 3 */
 
/* Main Lfo 1 */
MLV,0x0000FFFFL,0x0000FFFFL,0,1,0,
	READWRITE_ALWAYS,					/* Lfo1 delay -> lfo1 value (env off) 4 */
TMFQ,0x000000FFL,0x000000FFL,0,1,0,
	READWRITE_ALWAYS,					/* Lfo1 frequency -> lfo1 frequency 5 */

  /* Aux Lfo 2 */
VLV, 0x0000FFFFL,0x0000FFFFL,1,0,0,
	READWRITE_ALWAYS,					/* Lfo2 delay -> lfo2 value (env off) 6 */
VVFQ,0x000000FFL,0x000000FFL,0,1,0,
	READWRITE_ALWAYS,					/* Lfo2 frequency -> lfo2 frequency 7 */

  /* Aux Env 1 */
MEV, 0x0000FFFFL,0x0000FFFFL,0,1,0,
	READWRITE_ALWAYS,					/* Env1 delay -> env1 value (env off) 8 */
MEHA, 0x0000007FL,0x0000007FL,0,1,0,
	READWRITE_ALWAYS,					/* Env1 attack -> env1 attack 9 */
MEHA, 0x00007F00L,0x0000007FL,8,1,0,
	READWRITE_ALWAYS,					/* Env1 hold -> env1 hold 10 */
MEDS, 0x0000007FL,0x0000007FL,0,1,0,
	READ_WHEN_PLAYING|WRITE_WHEN_PLAYING,	/* Env1 decay -> env1 decay 11*/
MEDS, 0x00007F00L,0x0000007FL,8,1,0,
	READ_WHEN_PLAYING|WRITE_WHEN_PLAYING,	/* Env1 sustain -> env1 sustain 12*/
MEDS, 0x0000007FL,0x0000007FL,0,1,0,
	READ_WHEN_RELEASING|WRITE_WHEN_RELEASING,	/* Env1 release -> env1 decay 13*/

  /* Main Env 2 */
VEV, 0x0000FFFFL,0x0000FFFFL,0,1,0,
	READWRITE_ALWAYS,					/* Env2 delay -> env2 value (env off) 14*/
VEHA,0x0000007FL,0x0000007FL,0,1,0,
	READWRITE_ALWAYS,					/* Env2 attack -> env2 attack 15 */
VEHA,0x00007F00L,0x0000007FL,8,1,0,
	READWRITE_ALWAYS,					/* Env2 hold -> env2 hold 16*/
VEDS,0x0000007FL,0x0000007FL,0,1,0,
	READ_WHEN_PLAYING|WRITE_WHEN_PLAYING,/* Env2 decay -> env2 decay 17*/
VEDS,0x00007F00L,0x0000007FL,8,1,0,
	READ_WHEN_PLAYING|WRITE_WHEN_PLAYING,/* Env2 sustain -> env2 sustain 18*/
VEDS,0x0000007FL,0x0000007FL,0,1,0,
	READ_WHEN_RELEASING|WRITE_WHEN_RELEASING,	/* Env2 release -> env2 decay 19 */

  /* Oscillator */
QKBCA, 0x00FFFFFFL,0x00FFFFFFL,0,2,0,
	READ_WHEN_SOUNDING|NEVER_WRITE,		/* start address -> current address 20 */
SCSA,  0x00FFFFFFL,0x00FFFFFFL,0,2,0,
	READWRITE_WHEN_SOUNDING,			/* loop start address -> loop start addr 21*/
SDL,   0x00FFFFFFL,0x00FFFFFFL,0,2,0,
	READWRITE_WHEN_SOUNDING,			/* loop end address -> loop end address 22*/
IP,    0x0000FFFFL,0x0000FFFFL,0,1,0,
	READWRITE_ALWAYS,					/* initial pitch -> initial pitch 23*/
VFM,   0x0000FF00L,0x000000FFL,8,1,0,	/* Lfo1ToPitch -> frequency modulation 24*/
	READWRITE_ALWAYS,
VVFQ,  0x0000FF00L,0x000000FFL,8,1,0,
	READWRITE_ALWAYS,					/* Lfo2ToPitch -> freq mod 2 25*/
PEFE,  0x0000FF00L,0x000000FFL,8,1,0,
	READWRITE_ALWAYS,					/* Env1ToPitch -> pitch envelope 26*/

  /* Filter */
IFA,   0x0000FF00L,0x000000FFL,8,1,0,
	READWRITE_ALWAYS,					/* initial filter cutoff -> init Fc 27 */
QKBCA, 0xF0000000L,0x0000000FL,28,2,0,
	READWRITE_WHEN_SOUNDING,					/* initial filter Q -> resonance coeff 28*/
VFM,   0x000000FFL,0x000000FFL, 0,1,0,
	READWRITE_ALWAYS,					/* Lfo1ToFilterFc -> filter modulation 29*/
PEFE,  0x000000FFL,0x000000FFL, 0,1,0,
	READWRITE_ALWAYS,					/* Env1ToFilterFc -> filter envelope 30*/

  /* Amplifier */
IFA, 0x000000FFL,0x000000FFL,0,1,0,
	READWRITE_ALWAYS,					/* inital volume -> attenuation 31*/
TMFQ,0x0000FF00L,0x000000FFL,8,1,0,
	READWRITE_ALWAYS,					/* Lfo1ToVolume -> tremelo 32*/

  /* Effects */
SCSA, 0xFF000000L,0x000000FFL,24,2,0,
	READWRITE_ALWAYS,					/* Effects Send C -> chorus send 33*/
SDL,  0xFF000000L,0x000000FFL,24,2,0,
	READWRITE_ALWAYS,					/* Effects Send D -> reverb send 34*/
PTAB, 0x0000FF00L,0x000000FFL,8,2,0,
	READWRITE_ALWAYS,					/* Effects Send A -> dry left 35*/
PTAB, 0x000000FFL, 0x000000FFL,0,2,0,
	READWRITE_ALWAYS,					/* Effects Send B -> dry right 36*/

  /* Low level Emu8000 sound engine parameters */
CPF, 0xFFFF0000L,0x0000FFFFL,16,2,0,
	READWRITE_WHEN_SOUNDING,			/* current pitch 37*/
CPF, 0x00003FFFL,0x00003FFFL, 0,2,0,
	READWRITE_WHEN_SOUNDING,			/* fractional pitch 38*/
CVCF,0xFFFF0000L,0x0000FFFFL,16,2,0,
	READWRITE_WHEN_SOUNDING,			/* current volume 39*/
CVCF,0x0000FFFFL,0x0000FFFFL, 0,2,0,
	READWRITE_WHEN_SOUNDING,			/* current filter 40*/
Z1,0xFFFFFFFFL,0xFFFFFFFFL, 0,2,0,
	READWRITE_ALWAYS,					/* filter delay memory 1 41*/
Z2, 0xFFFFFFFFL,0xFFFFFFFFL, 0,2,1,
	READWRITE_ALWAYS,					/* filter delay memory 2 42*/

  /* NOTE: The following are NOT EMU8000 COMPATIBLE! 43*/
CPF,  0x00004000L,0x00000001L,14,2,0,
	READWRITE_WHEN_SOUNDING,			/* force 0 pitch bit 44*/
QKBCA,0x0E000000L,0x00000007L,25,2,0,
	READWRITE_WHEN_SOUNDING,			/* interpolation ROM 45*/
QKBCA,0x01000000L,0x00000001L,24,2,0,
	READWRITE_WHEN_SOUNDING,			/* eight bit sample voice 46*/
FXRT, 0x000F0000L,0x0000000FL,16,2,0,
	READWRITE_ALWAYS,					/* effects send A route 47*/
FXRT, 0x00F00000L,0x0000000FL,20,2,0,
	READWRITE_ALWAYS,					/* effects send B route 48*/
FXRT, 0x0F000000L,0x0000000FL,24,2,0,
	READWRITE_ALWAYS,					/* effects send C route 49*/
FXRT, 0xF0000000L,0x0000000FL,28,2,0,
	READWRITE_ALWAYS,					/* effects send D route 50*/

  /* Virtualized Parameters.  These parameters have no direct mapping
   * in the sound engine hardware, but they are still voice-specific
   */
0,	  0xFFFFFFFFL,0xFFFFFFFFL, 0,2,0,PURE_VIRTUAL, /* Sample memory handle 51*/
0,	  0xFFFFFFFFL,0xFFFFFFFFL, 0,2,0,PURE_VIRTUAL, /* Voice allocation priority 52*/
0,    0x00000001L,0x00000001L, 0,1,0,PURE_VIRTUAL, /* Stereo/Interleave bit 53*/
0,	  0x00000001L,0x00000001L, 0,1,0,PURE_VIRTUAL, /* One shot control flag 54*/

  /* Global registers */
PTBA, 0xFFFFF000L,0x000FFFFFL,12,2,0,
	READWRITE_ALWAYS,					/* page table base */
MAPA, 0xFFFFE000L,0x0007FFFFL,13,2,0,
	READWRITE_ALWAYS,					/* page table a entry */
MAPA, 0x00001FFFL,0x00001FFFL, 0,2,0,
	READWRITE_ALWAYS,					/* page table a index */
MAPB, 0xFFFFE000L,0x0007FFFFL,13,2,0,
	READWRITE_ALWAYS,					/* page table b entry */
MAPB, 0x00001FFFL,0x00001FFFL, 0,2,0,
	READWRITE_ALWAYS,					/* page table b index */ 

SOLL, 0xFFFFFFFFL,0xFFFFFFFFL, 0,2,0,
	READWRITE_ALWAYS,					/* stop on loop, low */
SOLH, 0xFFFFFFFFL,0xFFFFFFFFL, 0,2,0,
	READWRITE_ALWAYS,					/* stop on loop, high */

CLIEL,0xFFFFFFFFL,0xFFFFFFFFL, 0,2,0,
	READWRITE_ALWAYS,					/* interrupt on loop, low  */
CLIEH,0xFFFFFFFFL,0xFFFFFFFFL, 0,2,0,
	READWRITE_ALWAYS,					/* interrupt on loop, high */

CLIPL,0xFFFFFFFFL,0xFFFFFFFFL, 0,2,1,
	READWRITE_ALWAYS,					/* interrupt pending, low  */
CLIPH,0xFFFFFFFFL,0xFFFFFFFFL, 0,2,1,
	READWRITE_ALWAYS					/* interrupt pending, hight */ 
};


/*****************************************************************************
 * @func Just zeroes the seStates array.
 */
EMUAPIEXPORT EMUSTAT
seInit(void) 
{
	seCount = 0;
	memset(seStates, 0, sizeof(stSEState)*MAX_CHIPS);

	return SUCCESS;
}


/*****************************************************************************
 * @func Discover a chip by initializing per-chip data structures
 *  and then initializing the actual sound hardware.
 */
EMUAPIEXPORT EMUSTAT
seDiscoverChip(DWORD halid, SECHIPCONFIG *conf, SEID *id)
{
	WORD		 wIndex;    /* Index variable for loop */
	WORD		 i;			/* Random voice iteration variable */
	stSEState   *pstate;
	stVoice     *currVoice;     /* Voice currently being initialized */

	/* Find a free entry in the states array */
	for (wIndex = 0; wIndex < MAX_CHIPS; wIndex++) 
		if (seStates[wIndex].dwInUse == 0)
			break;
	if (wIndex == MAX_CHIPS)
		RETURN_ERROR(SMERR_INIT_FAILED);

	/* Initialize the state */
	pstate = &seStates[wIndex];
	pstate->dwInUse = 0xBEEFFACEL;
	pstate->id = halid;
    pstate->wFreeVoices = SE_NUM_VOICES;
    pstate->itmid = conf->itmid;
    pstate->dwHWRevision = conf->dwHWRevision;

#ifdef SE_RAMP_SENDS
    pstate->voiceRampList = NULL;
#endif

#ifdef CFG_DYNAMIC
	pstate->dwNumPages = (SE_NUM_VOICES * sizeof(stVoice) + SM_PAGE_SIZE - 1) /
							SM_PAGE_SIZE;
	pstate->voices = (stVoice*) osAllocPages(pstate->dwNumPages, &pstate->voicesMemHdl);
	if (pstate->voices == NULL)
		RETURN_ERROR(SMERR_NO_HOST_MEMORY);
	osLockVirtualRange(pstate->voices, pstate->dwNumPages, 0, NULL);
#endif

	for (i = 0; i < SE_NUM_VOICES; i++) {
		currVoice = &(seStates[wIndex].voices[i]);

		currVoice->state   = &seStates[wIndex];
		currVoice->byIndex = (BYTE) ((i + 8) % SE_NUM_VOICES);
/*		currVoice->byIndex = (BYTE)i; */
		currVoice->halid   = halid;
		currVoice->vstate  = VS_FREE;
		currVoice->smh     = 0;
		currVoice->wFlags  = 0;
		currVoice->wGeneration = 0x1; /* We start at 0x1 so that we don't issue
									   * NULL SEVOICE structures.  */
		currVoice->dwPriority = 0;
		currVoice->byNumRefs  = 0;
	}

	*id = MAKE_SEID(wIndex);
	seInitSoundEngineVoices(*id);

	seCount++;
	return SUCCESS;
}


/*******************************************************************************
 * @func Deallocate any data associated with this chip and mark it as
 *  invalid.
 */
EMUAPIEXPORT EMUSTAT
seUndiscoverChip(SEID seid)
{
	DWORD dwIndex; 
	stSEState *state;
	WORD wVoiceIndex;

	dwIndex = GET_STINDEX(seid);
	if (dwIndex >= MAX_CHIPS)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	state = &seStates[dwIndex];

   /* Unregister the interval timer callback if necessary */
#ifdef USE_TIMER
    if (state->itmHandle != (ITMHANDLE)NULL) {
        itmCancelCallback(state->itmHandle);
        state->itmHandle = 0;
    }

    if (state->itmISRHandle != (ITMHANDLE) NULL) {
        itmCancelCallback(state->itmISRHandle);
        state->itmISRHandle = 0;
    }
#endif

	for (wVoiceIndex = 0;  wVoiceIndex < SE_NUM_VOICES; wVoiceIndex++)
		_stopVoice(&state->voices[wVoiceIndex]);

	halWaitWallClockCounts(state->id, 32);

#ifdef SE_RAMP_SENDS
    while (state->voiceRampList != NULL)
    {
        stVoiceRampData *tmp = state->voiceRampList;
        state->voiceRampList = state->voiceRampList->next;
        itmCancelCallback(tmp->itmRampISRHandle);
        osLockedHeapFree(tmp, sizeof(stVoiceRampData));
    }
    if (state->itmRampHandle != (ITMHANDLE)NULL)
        itmCancelCallback(state->itmRampHandle);
#endif

#if CFG_DYNAMIC
	osUnlockVirtualRange(state->voices, state->dwNumPages);
	osFreePages(state->voicesMemHdl);
#endif

	state->dwInUse = 0;

	seCount--;
	return SUCCESS;
}


/******************************************************************************
 * @func Initializes the sound engine to a reasonable state.
 */
EMUSTAT
seInitSoundEngineVoices(SEID id)
{
	WORD wVoice, wCount;
	HALID hid;
	stSEState *state; 

	CHECK_SESTATE(state, id);
	hid   = state->id;

	/* Turn off all envelopes and stop any attempts to fetch from the
	 * cache.  */
	for (wVoice=0; wVoice < SE_NUM_VOICES; wVoice++) {
		WGWrite(hid, wVoice|VEDS, 0x0);
		LGWrite(hid, wVoice|IP, 0x0);
		LGWrite(hid, wVoice|PTAB, 0x0);
		LGWrite(hid, wVoice|CPF, 0x0); 

		/* This non-intuitive write makes it appear as if the cache 
		 * is totally full; this prevents any further PCI reads. */
		LGWrite(hid, wVoice|CCR, 0x0);

		halWaitWallClockCounts(hid, 2);
		LGWrite(hid, wVoice|QKBCA, 0);
	}

	/* Initialize Map and Cache Data Registers */
	for (wVoice=0; wVoice < SE_NUM_VOICES; wVoice++) {
		
		for(wCount=0; wCount<16; wCount++)
			LGWrite(hid, wVoice|(CD0+(((DWORD)wCount)<<16)), 0);

		LGWrite(hid, wVoice|MAPA, 0xFFFFFFFFL); 
		LGWrite(hid, wVoice|MAPB, 0xFFFFFFFFL);
		LGWrite(hid, wVoice|FXRT, 0x32100000L);
	}
 
    /* Initialize Envelope Engine parameters */
    for (wVoice=0; wVoice< SE_NUM_VOICES ; wVoice++) {
	   WGWrite(hid, (wVoice | MEHA),0);
	   WGWrite(hid, (wVoice | MEDS),0);
	   WGWrite(hid, (wVoice | IP),0);
	   WGWrite(hid, (wVoice | IFA),0);
	   WGWrite(hid, (wVoice | PEFE),0);
	   WGWrite(hid, (wVoice | VFM),0);
	   WGWrite(hid, (wVoice | TMFQ),0);  
	   WGWrite(hid, (wVoice | VVFQ),0);  
	   WGWrite(hid, (wVoice | TMPE),0);
	   WGWrite(hid, (wVoice | VLV),0);
	   WGWrite(hid, (wVoice | MLV),0);
	   WGWrite(hid, (wVoice | VEHA),0);
	   WGWrite(hid, (wVoice | VEV),0);
	   WGWrite(hid, (wVoice | MEV),0);
    }
  
    /* Initialize all Sound Engine parameters  */
    for (wVoice=0; wVoice < SE_NUM_VOICES; wVoice++)
    {
	   LGWrite(hid, (wVoice | SCSA),0);
	   LGWrite(hid, (wVoice | SDL), 0xffffff);
	   LGWrite(hid, (wVoice | VTFT),0);
	   LGWrite(hid, (wVoice | CVCF),0);
    }

	/* Initial cache control registers */
	for (wVoice=0; wVoice < SE_NUM_VOICES; wVoice++)
	{	
		LGWrite(hid, (wVoice | CCR), 0); 
		if (halWaitWallClockCounts(hid, 1) != SUCCESS) return 1;
		LGWrite(hid, (wVoice | CLP), 0);
	}

	/* Initialize the SPDIF Out Channel status registers.  The value specified here
	 * is based on the typical values provided in the specification, namely:
	 * Clock Accuracy of 1000ppm, Sample Rate of 48KHz, unspecified source number,
	 * Generation status = 1, Category code = 0x05 (Music synthesizer), Mode = 0,
	 * Emph = 0, Copy Permitted, AN = 0 (indicating that we're transmitting digital
	 * audio, and the Professional Use bit is 0.
	 */
	LGWrite(hid, SCS0, 0x02108504);
	LGWrite(hid, SCS1, 0x02108504);
	LGWrite(hid, SCS2, 0x02108504);

	return SUCCESS;
}


/*****************************************************************************
 * @func Retrieves an array of all of the currently discovered sound engines.
 */
DWORD 
seGetHardwareInstances(DWORD numVoices, SEID *seids)
{
	WORD i;
    WORD wFound = 0;

	for (i = 0; i < MAX_CHIPS && wFound < numVoices; i++) {
		if (seStates[i].dwInUse) 
			seids[wFound++] = MAKE_SEID(i);
	}

    return seCount;
}


/******************************************************************************
 * @func Synthesize a name string for the specified sound engine.
 */

EMUAPIEXPORT EMUSTAT
seGetModuleName(SEID seid, DWORD count, char *szName)
{
	char  name[64];		/* Name buffer.  We guarantee in the software 
	                         * spec that the name will never exceed 64 
	                         * chars in length */
	
	DWORD id = GET_STINDEX(seid);

	if (!IS_VALID_SEID(seid)) 
		RETURN_ERROR(SERR_BAD_HANDLE);

	sprintf(name, "EMU 8210 Sound Engine %d", id);
	strncpy(szName, name, (size_t) count);

	return SUCCESS;
}


/******************************************************************************
 * @func Return attribute information.  Right now, this doesn't do much.
 */
EMUAPIEXPORT EMUSTAT
seGetModuleAttributes(SEID seid, SEATTRIB *attr)
{
	RETURN_ERROR(1);
}


/******************************************************************************
 * @func Return the number of free voices.
 */
DWORD
seGetFreeVoiceCount(SEID seid)
{
	stSEState *state = FIND_SESTATE(seid);
	if (state == NULL) return 0;

	return state->wFreeVoices;
}


/******************************************************************************
 * @func Return the total number of voices supported by the chip.
 */
DWORD
seGetTotalVoiceCount(SEID seid)
{
	return SE_NUM_VOICES;
}


/******************************************************************************
 * @func Return the total number of reserved voices.
 */
DWORD
seGetReservedVoiceCount(SEID seid)
{
    DWORD i;
    DWORD dwReservedCount = 0;

    stSEState *state = FIND_SESTATE(seid);
    if (state == NULL) return 0;

    for (i = 0; i < SE_NUM_VOICES; i++) {
        if (state->voices[i].dwPriority >= (SEAP_RESERVED << PRIORITY_SHIFT))
            dwReservedCount++;
    }

    return dwReservedCount;
}


/******************************************************************************
 * @func seVoiceAllocate | Allocate voices.  The algorithm works in two phases.
 *  First, it looks to see if there are enough unallocated voices to fulfill
 *  the request.  If not, it computes the priorities of all of the currently
 *  allocated voices and then steals the voices with the lowest priorities.
 *
 * @parm SEID | seid | The ID of the sound engine to allocate from.
 * @parm DWORD    | callerID | A unique value identifying the caller which is
 *	used to avoid allowing a caller to steal voices from himself.  If callerID
 *  is zero then it is ignored.
 * @parm DWORD | priority | The priority of the the allocation.  This value
 *  is used for two things: it sets the initial priority of the voice, and it
 *  controls which voices we can steal.  
 */

#define ALLOC_VOICE(voice) \
	(voice)->vstate = VS_STOPPED; \
	(voice)->nextVoice = NULL;	\
	(voice)->prevVoice = NULL;	\
	(voice)->wFlags = 0x0; \
	if (flags & SEAF_NOTIFY_VOICE_STOLEN) \
		(voice)->wFlags |= VF_NOTIFY_VOICE_STOLEN; \
	if (flags & SEAF_NOTIFY_ZERO_VOLUME) \
		(voice)->wFlags |= VF_NOTIFY_ZERO_VOLUME; \
    if (flags & SEAF_SYNCABLE) \
        (voice)->wFlags |= VF_SYNCABLE; \
	(voice)->dwPriority = priority; \
    (voice)->dwVolume = 0x3FFFFF;  \
	(voice)->dwCallerID = callerID; \
	(voice)->notifyCallback = callback; \
	(voice)->dwCallbackData = callbackData; \
	state->wFreeVoices--;

DWORD
seVoiceAllocate(SEID seid, DWORD callerID, DWORD priority, DWORD flags,
				DWORD numVoices, SEVOICE *returnedVoices,
				SENOTIFYCALLBACK callback, DWORD callbackData)
{
	stSEState *state;
	stVoice   *pVoice, *pVoice1;	/* Voice pointers */
	SEVOICE	   sevoice;				    /* Duh */
	WORD	   wChan;               /* Iterator through the voices */
	BYTE	   byNumAllocated = 0;	/* Number of voices allocated thusfar */
	BYTE	   byVoiceIndex = 0;    /* Index of voice in return array */
	BYTE	   byCount, byCompkey;	/* Indicies into the priorities array used
		                             * during the sort */
	BYTE	   byNumIters;			    /* The number of iterations of the sort to do */
	BYTE	   byChannels[SE_NUM_VOICES];
	BYTE	   byIncr;

#ifdef NO_VOLUME_PRIORITIZE
        WORD       wPriority[SE_NUM_VOICES];
#endif

	CHECK_SESTATE(state, seid);
	ASSERT(state->wFreeVoices <= SE_NUM_VOICES);

	/* We must get an even number of voices if the PAIRED flag is set */
	if ((flags & SEAF_PAIRED) && (numVoices & 0x1))
		RETURN_ERROR(0);

	/* Correct the priority so that it matches the internal representation. */
	priority = (priority << PRIORITY_SHIFT) + 0x3F;

	/* First we try and satisfy our allocation needs using voices which are
	 * currently free
	 */
	byIncr = ((flags & SEAF_PAIRED) ? 2 : 1);
	if (state->wFreeVoices != 0) {

		for (wChan = 0; 
		     (wChan < SE_NUM_VOICES) && (byNumAllocated < numVoices); 
		     wChan += byIncr) {

			pVoice = &state->voices[wChan];

            /* We can only synchronize certain voices */
            if ((flags & SEAF_SYNCABLE) && 
                ((pVoice->byIndex < FIRST_SYNCABLE_VOICE) ||
                 (pVoice->byIndex > LAST_SYNCABLE_VOICE)))
                continue;

			if (flags & SEAF_PAIRED) {
                
				pVoice1 = & state->voices[wChan+1];

				if ((pVoice->vstate == VS_FREE) && 
				    (pVoice1->vstate == VS_FREE)) {

					/* We can allocate this pair */
                    returnedVoices[byNumAllocated] = MAKE_SEVOICE_FROM_PTR(pVoice);
					ALLOC_VOICE(pVoice);
                    DPRINTF(("Allocated index %d, first in stereo pair", pVoice->byIndex));
					byNumAllocated++;
					
					returnedVoices[byNumAllocated] = MAKE_SEVOICE_FROM_PTR(pVoice1);
					ALLOC_VOICE(pVoice1);
					byNumAllocated++;
				}
			} else {
				/* We're just allocating a mono voice.  See if the current
				 * voice is free.
				 */
				if (pVoice->vstate == VS_FREE) {
					returnedVoices[byNumAllocated] = MAKE_SEVOICE_FROM_PTR(pVoice);
                    ALLOC_VOICE(pVoice);
                    DPRINTF(("Allocated index %d, mono voice", pVoice->byIndex));
					byNumAllocated++;
				}
			}
		}
	}

	/* Check to see if we're done */
	if (byNumAllocated == numVoices)
	   return byNumAllocated;

	/* Now we need to start trying to satisfy the allocation by stealing
	 * voices.
	 */

	/* Initialize the priority array by just copying the priorities into it. */

#ifndef NO_VOLUME_PRIORITIZE
# define PRIORITY(_x) state->voices[_x].dwTempPriority
#else
# define PRIORITY(_x) wPriority[_x]
#endif

	for (byCount = 0; byCount < SE_NUM_VOICES; byCount++) {
		pVoice = & state->voices[byCount];

		/* Decrement the decaying priority value.  The idea here is that
		 * voices which have been recently allocated have 0x3F (63) written
		 * into the least significant bits of the priority.  Every time we
		 * do a priority recalculation we decrement these least significant
		 * bits.  This is done to provide an element of "round-robin"
		 * scheduling to the allocation."  */
		if (pVoice->dwPriority & 0x3F)
			pVoice->dwPriority--;

		PRIORITY(byCount)    = pVoice->dwPriority;
		byChannels[byCount]  = byCount;

		/* Bias the priorities in favor of voices which are playing 
		 * or releasing.  */
		if (pVoice->vstate == VS_RELEASED) {
#ifndef NO_VOLUME_PRIORITIZE
			/* For voices which are releasing, we replace the least significant
             * 22 bits with the volume calculated in the release code */
			PRIORITY(byCount) = (pVoice->dwPriority & 0xFF000000) | 0x400000 | pVoice->dwVolume;
#else
			PRIORITY(byCount) = pVoice->dwPriority | 0x40;
#endif
		}

		if (pVoice->vstate == VS_STARTED)
#ifndef NO_VOLUME_PRIORITIZE
			PRIORITY(byCount) |= 0x800000;
#else
			PRIORITY(byCount) |= 0x80;
#endif

		/* If we're trying to allocate channel pairs, the priority of the
		 * pair is equal to the highest of either of the two composite 
		 * channels. Also, if the voice is marked as being part of a stereo
		 * pair, both voices in the pair should get the same priority. */
		if (((flags & SEAF_PAIRED) || pVoice->dwParamValues[sepStereo]) && 
			(byCount & 0x1)) {
			if (PRIORITY(byCount - 1) > PRIORITY(byCount)) 
				PRIORITY(byCount) = PRIORITY(byCount - 1);
			else
				PRIORITY(byCount - 1) = PRIORITY(byCount);
		}
	}

	/* We don't want to steal a voice that we've just allocated, so make
	 * those very, very high priority.	 */
	for (byCount = 0; byCount < byNumAllocated; byCount++) {
		BYTE vindex = (BYTE) GET_VINDEX(returnedVoices[byCount]);
		PRIORITY(vindex) = SEAP_RESERVED << PRIORITY_SHIFT;
	}
	
	/* Sort the array into ascending order using a simple insertion sort; this
	 * sort is nice because we only need to run the inner loop enough times to
	 * cover the number of requested voices.  We keep the channels array
	 * parallel. Note, however, that since we have constraints on which voices
     * can be allocated for synchronization, we actually need to sort all 
     * 64 voices when allocating stuff for syncing.  Bummer.  */
    if (flags & SEAF_SYNCABLE)
        byNumIters = SE_NUM_VOICES;
    else
        byNumIters = (BYTE) numVoices - byNumAllocated;

	for (byCount = 0; byCount < byNumIters; byCount++) {
		DWORD dwCurrPri   = PRIORITY(byCount);
		BYTE byCurrChan = byChannels[byCount];

		for (byCompkey = byCount+1; byCompkey < SE_NUM_VOICES; byCompkey++) {
			if (PRIORITY(byCompkey) < dwCurrPri) {
				/* Swap the priority/channel pairs */
				PRIORITY(byCount)       = PRIORITY(byCompkey);
				byChannels[byCount]     = byChannels[byCompkey];
				PRIORITY(byCompkey)     = dwCurrPri;
				byChannels[byCompkey]   = byCurrChan;
				dwCurrPri                = PRIORITY(byCount);
				byCurrChan              = byChannels[byCount];
			}
		}
	}

	/* Now that we have an array of sorted channels, we do the allocation.
	 * This loop works for both paired an unpaired oscillators, since we 
	 * know that both even and odd channels must have the same priority.
	 */
	for (byCount = 0; (byCount < byNumIters) && (byNumAllocated < numVoices);
  		 byCount++) {

		pVoice  = &state->voices[byChannels[byCount]];
		sevoice = MAKE_SEVOICE_FROM_PTR(pVoice);

		if (priority >= pVoice->dwPriority && 
			(pVoice->dwPriority < (SEAP_RESERVED << PRIORITY_SHIFT)) &&
			(!callerID || (callerID != pVoice->dwCallerID))) {

            /* We can only allocate synchronizable voices from a certain
             * range.  */
            if ((flags & SEAF_SYNCABLE) && 
                ((pVoice->byIndex < FIRST_SYNCABLE_VOICE) || 
                 (pVoice->byIndex > LAST_SYNCABLE_VOICE)))
                continue;

			/* Tell the previous owner that the voice has been stolen */
			if ((pVoice->notifyCallback != NULL) &&
				(pVoice->wFlags & VF_NOTIFY_VOICE_STOLEN)) {
				pVoice->notifyCallback(pVoice->dwCallbackData, sevoice,
					seneVoiceStolen);
			}

			/* If the current voice is part of a voice pair and it is the
			 * master voice, also free the slave voice. */
			if (pVoice->dwParamValues[sepStereo] && !(pVoice->byIndex & 0x1)) {

				stVoice *pSlaveVoice = &state->voices[byChannels[byCount]+1];
				SEVOICE  sevoiceSlave = MAKE_SEVOICE_FROM_PTR(pSlaveVoice);

				if ((pSlaveVoice->notifyCallback != NULL) && 
					(pSlaveVoice->wFlags & VF_NOTIFY_VOICE_STOLEN)) {
					pSlaveVoice->notifyCallback(pSlaveVoice->dwCallbackData,
						sevoiceSlave, seneVoiceStolen);
				}

				seVoiceFree(1, &sevoiceSlave);
			}

			/* Free up the voice and clean up any data structures it might
			 * have been using */
			if (pVoice->vstate != VS_FREE)
				seVoiceFree(1, &sevoice);

			/* Reallocate the voice */
			ALLOC_VOICE(pVoice);
            DPRINTF(("Reallocated voice %d", pVoice->byIndex));
			returnedVoices[byNumAllocated++] = MAKE_SEVOICE_FROM_PTR(pVoice);

		} else {
			/* We need to insure that we stay on an even boundary */
			if (flags & SEAF_PAIRED)
				byCount++;
		}
	}		

	return byNumAllocated;
}


/******************************************************************************
 * @func Free voices.  Nothing particularly tricky here.
 *
 * @parm DWORD     | numVoices | The number of voices to free.
 * @parm SEVOICE * | voices    | The voices to free.
 */

EMUAPIEXPORT EMUSTAT
seVoiceFree(DWORD numVoices, SEVOICE *voices)
{
	BYTE	byCount;
	stVoice *voice;

	ASSERT(voices);
	ASSERT(numVoices <= SE_NUM_VOICES);

	for (byCount = 0; byCount < numVoices; byCount++) {
		voice = FIND_VOICE(voices[byCount]);

		if (voice == NULL || 
			(voice->wGeneration != GET_GENERATION(voices[byCount]))) 
			continue;

		/* We shouldn't be double freeing, although doing so should be
		 * innocuous.  */
		ASSERT(voice->vstate != VS_FREE);

		/* Check the voice's state to figure out whether we need to shut it
		 * down first. 	 */
		if (voice->vstate == VS_STARTED || voice->vstate == VS_RELEASED) {
			voice->byNumRefs = 1;
			seVoiceStop(1, &voices[byCount]);
		}
		
		voice->nextVoice = NULL;
		voice->prevVoice = NULL;
		voice->byNumRefs = 0;
		voice->vstate    = VS_FREE;
		voice->dwPriority = 0;
		voice->smh       = 0;
		voice->wGeneration++;
		voice->state->wFreeVoices++;
		ASSERT(voice->state->wFreeVoices <= SE_NUM_VOICES);

		/* Check for overflow; we never want the generation number to go to 
		 * 0, since this could lead to SEVOICE values of 0x0 */
		if (voice->wGeneration == 0)
			voice->wGeneration = 0x1;

		voices[byCount] = 0;

        /* Check to see if we should turn off the callback */
        if (voice->state->wFreeVoices == SE_NUM_VOICES) {
            if (voice->state->itmISRHandle) {
                itmCancelCallback(voice->state->itmISRHandle);
                voice->state->itmISRHandle = 0;
            }

            if (voice->state->itmHandle) {
                itmCancelCallback(voice->state->itmHandle);
                voice->state->itmHandle = 0;
            }
        }
	}

	return SUCCESS;
}


/******************************************************************************
 * @func Get the channel index given a voice structure.
 * @parm SEVOICE | voice | The voice whose index should be returned.
 */

EMUAPIEXPORT DWORD
seGetVoiceIndex(SEVOICE sevoice)
{
    stVoice *pvoice = FIND_VOICE(sevoice);
    if (!pvoice) 
        return 0;
    else
        return pvoice->byIndex;
}


/******************************************************************************
 * @func Return the ID of the sound engine associated with the voice.
 * @parm SEVOICE | sevoice | Voice whose SEID is to be returned.
 */

EMUAPIEXPORT SEID 
seGetVoiceSEID(SEVOICE sevoice)
{
	stVoice *voice;
    voice = FIND_VOICE(sevoice);
	if (voice == NULL) RETURN_ERROR(0);

	return MAKE_SEID_FROM_PTR(voice->state);
}


/******************************************************************************
 * @func Return a voice handle given an SEID and a channel index
 * @parm SEID | seid | The sound engine from which to get the handle
 * @parm WORD | channelIndex | The channel index to get.
 */

SEVOICE
seGetVoiceHandle(SEID seid, WORD channelIndex)
{
	stSEState *state;
    DWORD      i;

    CHECK_SESTATE(state, seid);
	ASSERT(channelIndex < SE_NUM_VOICES);

    for (i = 0; i < SE_NUM_VOICES; i++) {
        if (state->voices[i].byIndex == channelIndex)
            return MAKE_SEVOICE(GET_STINDEX(seid), i, 
                                state->voices[channelIndex].wGeneration);
    }

    return 0;
}


/******************************************************************************
 * @func Reads a specific user variable from the EMU8010.  First it reads
 *  the register, masks it, and returns the data such the the first LSB
 *  of the data is always placed in bit 0 of the returned DWORD (regardless
 *  of where the data actually was in the register).  This allows us to
 *  abstract away a little bit from the hardware's register implementation.
 *
 * @parm  SEVOICE | readVoice | The voice to read from
 * @parm  DWORD   | dwParamID | The parameter to read
 */

EMUAPIEXPORT DWORD
seParameterRead(SEVOICE sevoice, DWORD dwParamID)
{
	stVoice *pVoice;        /* The voice structure of the voice being read */
	HALID hid;              /* Dereferenced HAL ID for actually doing reads */
	DWORD dwData;           /* The data read from the hardware register */
    WORD uiTemp;            /* Temporary holder for word-sized reads */
    stParamData *pAddInfo;  /* Dereferenced parameter data */

    CHECK_VOICE(pVoice, sevoice);
	
	hid = pVoice->halid;

	ASSERT(dwParamID < sepLastVoiceParam);
	pAddInfo = (stParamData *)&AddressTable[dwParamID];
	
	if (((pVoice->vstate == VS_STOPPED) && !(pAddInfo->byRegMode & READ_WHEN_STOPPED)) ||
		((pVoice->vstate == VS_STARTED) && !(pAddInfo->byRegMode & READ_WHEN_PLAYING)) ||
		((pVoice->vstate == VS_RELEASED) && !(pAddInfo->byRegMode & READ_WHEN_RELEASING)))
	{
		/* For virtual parameters, all we need to do is return the value
		 * in the table. */
		dwData = pVoice->dwParamValues[dwParamID];
	} 
	else 
	{
		/* Read-Modify-Return based on how many words are associated with the
	     * given address (always either one or two)
		 * Right shifts put indeterminate info in front of word. Use 
		 * ValidInfoMask to assure only known valid data gets through.
		 */
	
		/* Single word addresses */
		if (pAddInfo->byNumofWords == 1) {
			/* Read */
			WGRead(hid, (DWORD) pVoice->byIndex | pAddInfo->dwAddress, &uiTemp);
		
			/* Modify-Return */
			dwData = (DWORD)(((uiTemp & pAddInfo->dwMask) >> pAddInfo->byShift)
								& pAddInfo->dwValidInfoMask);
		} else {
			/* Read */
			LGRead(hid, (DWORD) pVoice->byIndex | pAddInfo->dwAddress, &dwData);
		
			/* Modify-Return */
			dwData = (((dwData & pAddInfo->dwMask) >> pAddInfo->byShift)
								& pAddInfo->dwValidInfoMask);
		}

		/* For the StartAddrs, StartloopAddrs, and EndloopAddrs registers
		 * we need to convert an absolute offset from the beginning of
		 * sample memory into a relative offset from the beginning of 
		 * of the sample.
		 */
		if ((dwParamID >= sepStartAddrs) && (dwParamID <= sepEndloopAddrs)) {
			DWORD dwBase;

			smMemoryGetBaseAndLength(pVoice->smh, &dwBase, NULL);

			/* The Base is always stored in bytes, but the addresses are in
			* sample frames.  So convert things appropriately.  */
			if (pVoice->dwParamValues[sepEightBitSample] == 0)
				dwBase /= 2;
			if (pVoice->dwParamValues[sepStereo] == SESTMODE_INTERLEAVED)
				dwBase /= 2;
	
			dwData -= dwBase;
		}
	}

	return dwData;
}


/******************************************************************************
 * @func Writes data into the appropriate device registers.
 * 
 *  @parm SEVOICE | theVoice | The voice to write to.
 *  @parm DWORD   | dwParamID | The parameter to change
 *  @parm DWORD   | dwData   | The value to be written
 */

EMUAPIEXPORT EMUSTAT
seParameterWrite(SEVOICE sevoice, DWORD dwParamID, DWORD dwData)
{
	stVoice *pVoice;        /* Pointer to the voice being changed */
	HALID    hid;           /* HAL we need to write to */
	BYTE     err = SUCCESS; /* Return value */
	WORD     uiTemp;        /* Temporary var for manipulating word-sized values */
	DWORD    dwTemp;        /* Temporary data manipulation variable */
	WORD     wVoice;        /* Voice index */
	stParamData *pAddInfo;  /* Pointer to info for parameter being changed */
#ifdef SE_RAMP_SENDS
    DWORD dwCurData;
#endif

	CHECK_VOICE(pVoice, sevoice);

	hid    = pVoice->halid;
	wVoice = pVoice->byIndex;

	ASSERT(dwParamID < sepLastVoiceParam);
	pAddInfo = (stParamData *)&AddressTable[dwParamID];

	/* Should return error if attempt to write to Read-Only */
	if (pAddInfo->byReadOnly != 0)
		RETURN_ERROR(SERR_READ_ONLY_REGISTER);

#ifdef SE_RAMP_SENDS
    dwCurData = pVoice->dwParamValues[dwParamID];
#endif
	/* Stash the parameter in the array */
	pVoice->dwParamValues[dwParamID] = dwData;

	/* Check to see whether we need to update a virtual parameter.  
	 * which has additional side effects when written to.
	 */
	if (pAddInfo->byRegMode == PURE_VIRTUAL) 
	{

		switch (dwParamID) {
		case sepSampleMemoryHandle:
			pVoice->smh = dwData;
			break;

		case sepPriority:
			/* We need to preserve the decay value in the voice */
			pVoice->dwPriority = (WORD)(dwData << PRIORITY_SHIFT) | 
										(pVoice->dwPriority & 0x3F);
			break;
	
		case sepOneShot:
			_updateStopOnLoop(pVoice, dwData);
			break;
		}

	    return SUCCESS;
	} 
	
	/* Check to see if we're at a point where we can't safely write to
	 * the backing physical register.
	 */
	if (((pVoice->vstate == VS_STOPPED) && !(pAddInfo->byRegMode & WRITE_WHEN_STOPPED)) ||
		((pVoice->vstate == VS_STARTED) && !(pAddInfo->byRegMode & WRITE_WHEN_PLAYING)) ||
		((pVoice->vstate == VS_RELEASED) && !(pAddInfo->byRegMode & WRITE_WHEN_RELEASING)))
	{
		return SUCCESS;
	} 

	/* If we get to this point, we want to go ahead and actually write to
	 * the physical register.
	 */

#ifdef SE_RAMP_SENDS
    /* If we're setting the send amounts, ramp them if the voice is playing */
    if (((dwParamID >= sepEffectsSendC) && (dwParamID <= sepEffectsSendB)) &&
        ((pVoice->vstate == VS_STARTED) || (pVoice->vstate == VS_RELEASED)))
    {
        _rampVoiceSend(pVoice, dwParamID, dwCurData);
        return SUCCESS;
    }
#endif

	/* The Start Address, Start Loop Address and End Loop Address all
	 * need to be converted from relative into absolute addresses by
	 * looking up the appropriate sample memory handle and adding in
	 * the base frame address.
	 */
	if ((dwParamID >= sepStartAddrs) && (dwParamID <= sepEndloopAddrs))	{
		DWORD dwBase;
		smMemoryGetBaseAndLength(pVoice->smh, &dwBase, NULL);

	   /* The Base is always stored in bytes, but the addresses are in
	    * sample frames.  So convert things appropriately.  */
		if (pVoice->dwParamValues[sepEightBitSample] == 0)
			dwBase /= 2;
		if (pVoice->dwParamValues[sepStereo] == SESTMODE_INTERLEAVED)
			dwBase /= 2;
	
		dwData += dwBase;
	}

	/* Read-Modify-Write based on how many words are associated with the
     * given address (always either one or two) */
        
	/* Single word addresses */
	if (pAddInfo->byNumofWords == 1) {
		/* Read-Modify */
		WGRead(hid, (DWORD)(wVoice)|pAddInfo->dwAddress, &uiTemp);
		uiTemp &= (~(WORD)pAddInfo->dwMask);

		uiTemp |= ((WORD)(dwData << pAddInfo->byShift) & ((WORD)pAddInfo->dwMask));

		/* Write */
		WGWrite(hid, (DWORD)(wVoice)|pAddInfo->dwAddress, uiTemp);
	} else {
		/* Double word addresses */

		/* Read-Modify */
		LGRead(hid, (DWORD)(wVoice)|pAddInfo->dwAddress, &dwTemp);
		dwTemp &=  (~pAddInfo->dwMask);
		dwTemp |=  ((dwData << pAddInfo->byShift) & (pAddInfo->dwMask));

		/* Write */
		LGWrite(hid, (DWORD)(wVoice)|pAddInfo->dwAddress, dwTemp);
	}
    
	return SUCCESS;
}


/******************************************************************************
 * @func Update multiple parameters at once.  Theoretically, we could
 *  try and do something exceptionally clever here to minimize the number
 *  of updates needed to the registers, but unless we sort the array
 *  we're going to get hit with an O(N^2) probe overhead.  Instead we're
 *  going to take the easy way out and just call seParameterWrite multiple
 *  times.  One possible improvement would be to keep a temporary in-memory
 *  copy of the registers and then write out the modified registers in a
 *  second pass.  I'm not sure that it is worth the effort, though.
 *
 * @parm SEVOICE     | sevoice   | The voice to update.
 * @parm stseParam * | params    | The parameters to write.
 * @parm DWORD       | numParams | The number of params in the array.
 */

EMUAPIEXPORT EMUSTAT
seMultiParameterWrite(SEVOICE sevoice, DWORD numParams, SEPARAM *params)
{
	WORD    i;        /* Index register */
	EMUSTAT status;   /* Return status from seParameterWrite */

	for (i = 0; i < numParams; i++) {
		status = seParameterWrite(sevoice, params[i].dwParamID, 
		                          params[i].dwValue);
		if (status != SUCCESS) 
			return status;
	}

	return SUCCESS;
}

		 
/******************************************************************************
 * @func EMUSTAT | seVoiceSetup
 *  Brings all of the parameters in a voice to known settings.  We do
 *  this by simply maintaining arrays of default parameter values and calling
 *  seParameterWrite over and over.  While this isn't the most efficient
 *  mechanism for voice configuration (it would be faster to simply maintain
 *  separate tables of underlying hardware parameter values and software
 *  parameter settings), this is probably the easiest and least buggy way
 *  to do it.
 *
 * @parm SEVOICESETUP * | voiceSetups | An array of SEVOICESETUP structures
 *	containing all of the data needed to configure a set of voices.
 * @parm DWORD | numSetups | The number of voice setups in the array.
 */

typedef struct stConfigTableTag 
{
	WORD			wNumParams;
    WORD			wNumOverrides;
	const SEPARAM *	params;
    const SEVOICEPARAM   * overrides;
} stConfigTable;

const SEPARAM TriggerConfig[] = {
		{ sepDelayModLFO,    0x0},		{ sepInitialFilterFc,    0xFF },
		{ sepFreqModLFO,     0x0},		{ sepInitialFilterQ,     0x0 },
		{ sepDelayModEnv,    0x8000},	{ sepModLFOToFilterFc,   0x0 },
		{ sepAttackModEnv,   0x8000},	{ sepModEnvToFilterFc,   0x0 },
		{ sepHoldModEnv,     0x0 },		{ sepInitialVolume,      0x0 },
		{ sepDecayModEnv,    0x7f },	{ sepModLFOToVolume,     0x0 },
		{ sepSustainModEnv,	 0x7f },	{ sepEffectsSendC,       0xff },
		{ sepReleaseModEnv,	 0x7f },	{ sepEffectsSendD,       0xff },
		{ sepDelayVolEnv,	 0x8000 },	{ sepEffectsSendA,       0xff },
		{ sepAttackVolEnv,	 0x7f },	{ sepEffectsSendB,       0xff },
		{ sepHoldVolEnv,	 0x7f },	{ sepForceZeroPitch,     0x00 },
		{ sepDecayVolEnv,	 0x7f },	{ sepInterpolationROM,   0x00 },
		{ sepSustainVolEnv,  0x7f },	{ sepEightBitSample,     0x00 },
		{ sepReleaseVolEnv,	 0x7f },	{ sepRouteEffectsSendA,  0x00 },
		{ sepStartAddrs,     0x0  },	{ sepRouteEffectsSendB,	 0x01 },
		{ sepStartloopAddrs, 0x0  },	{ sepRouteEffectsSendC,	 0x02 },
		{ sepEndloopAddrs,   0x0  },	{ sepRouteEffectsSendD,  0x03 },
		{ sepInitialPitch,   0xE000 },	{ sepModLFOToPitch,      0x00 },
		{ sepVibLFOToPitch,	 0x00 },	{ sepModEnvToPitch,      0x00 },
		{ sepDisableEnv,     0x00 },	{ sepOneShot,			 0x01 },
		{ sepStereo,		 0x00 },

        /* We handle the stereo and interleave configurations simply by
         * adding a couple more entries which override the preceeding
         * values.  This isn't super-efficient in terms of run-time, but
		 * it lets us avoid making two more copies of virtually identical
		 * tables. */
		{ sepStereo,		SESTMODE_INTERLEAVED },	
		{ sepStereo,		SESTMODE_STEREO }
};

const SEVOICEPARAM TriggerOverrides[] = {
	sepStartAddrs, sepStartloopAddrs, sepEndloopAddrs, sepSampleMemoryHandle
};

const stConfigTable ConfigurationTable[] = {
	{ 41, 4, TriggerConfig,	TriggerOverrides },   /* secfgMonoOneShot */
	{ 42, 4, TriggerConfig,	TriggerOverrides },	  /* secfgIntlvOneShot */
	{ 43, 4, TriggerConfig,	TriggerOverrides },	  /* secfgStereoOneShot */
};

EMUAPIEXPORT EMUSTAT
seVoiceSetup(DWORD numSetups, SEVOICESETUP **setups)
{
    stVoice *	pVoice;				/* The voice currently being set up */
	BYTE		byCount;			/* The current setup being programmed */
    DWORD		dwParam;
	BYTE		byIndex;

	for (byCount = 0; byCount < numSetups; byCount++) {
		SEVOICESETUP *setup = setups[byCount];

		CHECK_VOICE(pVoice, setup->voice);
		
		if (setup->baseConfig >= secfgLastConfiguration)
			RETURN_ERROR(SERR_BAD_PARM);

		pVoice->wFlags |= VF_CONFIGURED;

		/* Keep the handling of custom configurations fast */
		if (setup->baseConfig == secfgCustom) {
			for (dwParam = 0; dwParam < sepLastVoiceParam; dwParam++) {
				if (0 == AddressTable[dwParam].byReadOnly)
					seParameterWrite(setup->voice, dwParam, 
									 setup->dwParamValues[dwParam]);
			}
		} 
		else {
			/* We need to do a little more work for this case */
			const stConfigTable *config = &ConfigurationTable[setup->baseConfig];

			seMultiParameterWrite(setup->voice, config->wNumParams, 
				                  (SEPARAM*) config->params);

			/* Write in the overriden values */
			for (byIndex = 0; byIndex < config->wNumOverrides; byIndex++) {
				seParameterWrite(setup->voice, config->overrides[byIndex],
								 setup->dwParamValues[config->overrides[byIndex]]);
			}
		}
	}

	return SUCCESS;
}


/*****************************************************************************
 * @func Start up a bunch of voices synchronously
 */
EMUAPIEXPORT EMUSTAT
seVoiceStartSync(DWORD dwNumVoices, SEVOICE *sevoices)
{
    DWORD dwStartMask;
    DWORD dwOneShotMask;
    DWORD dwValue;
    DWORD i;
    stVoice *voice;


    dwStartMask = 0;
    dwOneShotMask = 0;

    for (i = 0; i < dwNumVoices; i++) {
        CHECK_VOICE(voice, sevoices[i]);

        if (voice->byIndex > LAST_SYNCABLE_VOICE || 
            voice->byIndex < FIRST_SYNCABLE_VOICE)
            RETURN_ERROR(SERR_BAD_HANDLE);

        dwStartMask |= (1 << voice->byIndex);

        if (voice->dwParamValues[sepOneShot])
            dwOneShotMask |= (1 << voice->byIndex);
    }

    dwValue = LSEPtrRead(voice->halid, SOLL);
    dwValue &= ~ dwStartMask;
    LSEPtrWrite(voice->halid, SOLL, dwValue);

    if (dwOneShotMask) {
        halWaitWallClockCounts(voice->halid, 2);
        dwValue |= dwOneShotMask;
        LSEPtrWrite(voice->halid, SOLL, dwValue);
    }

    return SUCCESS;
}
 
   
/*****************************************************************************
 * @func Set up one or more voices and then fire the voices as close to
 *  simultaneously as possible.
 *
 * @parm DWORD | numVoiceSetups | The number of stseVoiceSetup
 *	structures in the array.
 * @parm SEVOICESETUP * | voiceSetups | An array of SEVOICE handles.
 * 
 */

EMUAPIEXPORT EMUSTAT
seVoiceSetupAndStart(DWORD numVoiceSetups, SEVOICESETUP **voiceSetups)
{
	DWORD           dwCurrVS;	/* The current voice setup */
	stVoice *       voice;		/* The current voice pointer */
	SEVOICESETUP *  setup;		/* The current setup */
	HALID           halid;		/* The HAL ID */
	DWORD *         dwParams;	/* A pointer to the parameter table in use */
	DWORD			dwParamTable[sepLastVoiceParam];
	DWORD           dwChanIdx;	/* The voice channel index */
	EMUSTAT         status;		/* Return status from subroutine calls */
	fourByteUnion   fbu;		/* Four byte union for slamming data into
								 * registers */
	DWORD           dwCurrentPitch[SE_NUM_VOICES];
	WORD            wCurrentVolume[SE_NUM_VOICES];
    stVoice *		voiceArray[SE_NUM_VOICES];
//    DWORD ptabData;

	ASSERT(numVoiceSetups <= SE_NUM_VOICES);
	ASSERT(voiceSetups);

	/* Check for obvious stuff */
	if (numVoiceSetups == 0)
		return SUCCESS;

	for (dwCurrVS = 0; dwCurrVS < numVoiceSetups; dwCurrVS++) {
		setup = voiceSetups[dwCurrVS];
		CHECK_VOICE(voice, setup->voice);
		voiceArray[dwCurrVS] = voice;

		/* Release the voice handle if we're retriggering */
		if (voice->vstate == VS_STARTED || voice->vstate == VS_RELEASED)
			seVoiceStop(1, &(setup->voice));

		halid		= voice->halid;
		dwChanIdx	= voice->byIndex;

		/* The rest of this code assumes that the dwParams pointer points to
		 * a paramID indexed array of data values.  We can just use the table
		 * passed in for custom configurations.  */
		if (voiceSetups[dwCurrVS]->baseConfig == secfgCustom) {
			dwParams   = setup->dwParamValues;
		} else {
			BYTE                   byIndex;
			const stConfigTable *  config;

			/* We've been given a template-based configuration, so we build
			 * the parameter table  */
			ASSERT(setup->baseConfig < secfgLastConfiguration);
			config = &ConfigurationTable[setup->baseConfig];
			for (byIndex = 0; byIndex < config->wNumParams; byIndex++) {
				dwParamTable[config->params[byIndex].dwParamID] =
					config->params[byIndex].dwValue;
			}

			/* Stick in the overriden parameters */
			for (byIndex = 0; byIndex < config->wNumOverrides; byIndex++) {
				DWORD dwParamID = config->overrides[byIndex];
				dwParamTable[dwParamID] = setup->dwParamValues[dwParamID];
			}

			dwParams = dwParamTable;
		}

		/* Store all of the parameters */
		memcpy(voice->dwParamValues, dwParams, sepLastVoiceParam*sizeof(DWORD));
		voice->dwPriority = (dwParams[sepPriority] << PRIORITY_SHIFT) | 0x3F;
		_updateStopOnLoop(voice, dwParams[sepOneShot]);

		/* Completely shut down the channel */
		WGWrite(halid, (VEDS | dwChanIdx), (WORD)ENV_ENGINE_OFF(0));
		LGWrite(halid, (VTFT | dwChanIdx), 0x0000FFFF);
	
		/* We need to actually shut down the master's oscillator */
      /* Potential fix to remove clicks */
//      LGRead(halid, (PTAB|dwChanIdx), &ptabData);
//      ptabData &= 0x0000FFFF;
//		LGWrite(halid, (PTAB|dwChanIdx), ptabData);
		LGWrite(halid, (PTAB|dwChanIdx), 0x0);
		LGWrite(halid, (CPF |dwChanIdx), 0x0);
	
		/* Now we get down to the nitty-gritty of actually initializing
		 * the voice parameters.  First, if the envelope engine hasn't
		 * been disabled, we initialize the envelope engine parameters.
		 */

		if (dwParams[sepDisableEnv] == 0) {

			/* Write the volume attack and hold times */
			fbu.byVals.by0 = (BYTE) (dwParams[sepAttackVolEnv] & 0x7F);
			fbu.byVals.by1 = (BYTE) (dwParams[sepHoldVolEnv] & 0x7f);
			WGWrite(halid, (VEHA | dwChanIdx), fbu.wVals.w0);

			/* Write the modulation LFO delay */
			WGWrite(halid, (MLV | dwChanIdx), (WORD) dwParams[sepDelayModLFO]);

			/* Write the mod envelope attack and hold */
			fbu.byVals.by0 = (BYTE) dwParams[sepAttackModEnv] & 0x7F;
			fbu.byVals.by1 = (BYTE) dwParams[sepHoldModEnv] & 0x7F;
			WGWrite(halid, (MEHA | dwChanIdx), fbu.wVals.w0);

			/* Write the mod envelope decay and sustain */
			fbu.byVals.by0 = (BYTE) dwParams[sepDecayModEnv] & 0x7F;
			fbu.byVals.by1 = (BYTE) dwParams[sepSustainModEnv] & 0x7f;
			WGWrite(halid, (MEDS | dwChanIdx), fbu.wVals.w0);

			/* Write the vibrato LFO delay */
			WGWrite(halid, (VLV | dwChanIdx), (WORD) dwParams[sepDelayVibLFO]);

			/* Write the initial pitch */
			WGWrite(halid, (IP | dwChanIdx), (WORD) dwParams[sepInitialPitch]);

			/* Write the initial volume and Fc */
			fbu.byVals.by0 = (BYTE) dwParams[sepInitialVolume];
			fbu.byVals.by1 = (BYTE) dwParams[sepInitialFilterFc];
			WGWrite(halid, (IFA | dwChanIdx), fbu.wVals.w0);

			/* Write the mod envelope to Fc and pitch amounts */
			fbu.byVals.by0 = (BYTE) dwParams[sepModEnvToFilterFc];
			fbu.byVals.by1 = (BYTE) dwParams[sepModEnvToPitch];
			WGWrite(halid, (PEFE | dwChanIdx), fbu.wVals.w0);

			/* Write the mod LFO amounts for Fc and pitch */
			fbu.byVals.by0 = (BYTE) dwParams[sepModLFOToFilterFc];
			fbu.byVals.by1 = (BYTE) dwParams[sepModLFOToPitch];
			WGWrite(halid, (VFM | dwChanIdx), fbu.wVals.w0);

			/* Write the mod LFO frequency & amplifier mod amount */
			fbu.byVals.by0 = (BYTE) dwParams[sepFreqModLFO];
			fbu.byVals.by1 = (BYTE) dwParams[sepModLFOToVolume];
			WGWrite(halid, (TMFQ | dwChanIdx), fbu.wVals.w0);

			/* Write the vib LFO frequency and & pitch mod amount */
			fbu.byVals.by0 = (BYTE) dwParams[sepFreqVibLFO];
			fbu.byVals.by1 = (BYTE) dwParams[sepVibLFOToPitch];
			WGWrite(halid, (VVFQ | dwChanIdx), fbu.wVals.w0);
		}
		
		/* Write the FX routings */
		fbu.dwVal = dwParams[sepRouteEffectsSendD] << 28 | 
					dwParams[sepRouteEffectsSendC] << 24 | 
					dwParams[sepRouteEffectsSendB] << 20 |
					dwParams[sepRouteEffectsSendA] << 16;
		LGWrite(halid, (FXRT | dwChanIdx), fbu.dwVal);

		status = _prepareVoice(voice, dwParams, &dwCurrentPitch[dwCurrVS],
		                       &wCurrentVolume[dwCurrVS]);
		if (status != SUCCESS)
			RETURN_ERROR(status);

		voice->vstate = VS_STARTED;

	} /* End of for (dwCurrVS ...) */


	/* Finally, we trigger all of the voices */
	for (dwCurrVS = 0; dwCurrVS < numVoiceSetups; dwCurrVS++) {
        setup     = voiceSetups[dwCurrVS];
		halid     = voiceArray[dwCurrVS]->halid;
		dwChanIdx = voiceArray[dwCurrVS]->byIndex;

		if (setup->dwParamValues[sepStereo] == SESTMODE_INTERLEAVED) {
			if (dwChanIdx & 0x1) {
				/* Slave channel */
				LGWrite(halid, (PTAB | dwChanIdx),
						dwCurrentPitch[dwCurrVS] & 0x0000FFFF);
				LGWrite(halid, (CPF | dwChanIdx), 0x0000);
			} else {
				/* This is the master (even) channel, and we're in interleave mode, (set the S bit) */
				LGWrite(halid, (PTAB | dwChanIdx), 
					    dwCurrentPitch[dwCurrVS]);
				LGWrite(halid, (CPF  | dwChanIdx),
					    (dwCurrentPitch[dwCurrVS] & 0xFFFF0000L) | 0x8000);
			}
		} else if (setup->dwParamValues[sepStereo] == SESTMODE_STEREO) {
			if (dwChanIdx & 0x1) {
				/* Slave channel; phase lock it (set the S bit) */
				LGWrite(halid, (PTAB | dwChanIdx),
						dwCurrentPitch[dwCurrVS] & 0x0000FFFF);
				//LGWrite(halid, (CPF | dwChanIdx), 0x8000);
			} else {
				/* Master channel; */
				LGWrite(halid, (PTAB|dwChanIdx), dwCurrentPitch[dwCurrVS]);
				LGWrite(halid, (CPF | (dwChanIdx+1)), 0x8000);
				LGWrite(halid, (CPF |dwChanIdx), dwCurrentPitch[dwCurrVS]&0xFFFF0000);
			}
		} else {
			/* This channel is in mono mode, so just write the registers */
			LGWrite(halid, (PTAB | dwChanIdx), 
					dwCurrentPitch[dwCurrVS]);
			LGWrite(halid, (CPF  | dwChanIdx),
				    (dwCurrentPitch[dwCurrVS] & 0xFFFF0000L));
		}
	}

	/* Activate the envelope engines */
	for (dwCurrVS = 0; dwCurrVS < numVoiceSetups; dwCurrVS++) {  
		halid     = voiceArray[dwCurrVS]->halid;
		dwChanIdx = voiceArray[dwCurrVS]->byIndex;

		if (voiceSetups[dwCurrVS]->dwParamValues[sepDisableEnv] == FALSE)
			WGWrite(halid, (VEDS | dwChanIdx), (WORD)(ENV_ENGINE_ON(wCurrentVolume[dwCurrVS])));

        if (voiceArray[dwCurrVS]->wFlags & VF_SYNCABLE)
            _completeSyncSetup(voiceArray[dwCurrVS]);
	}

#ifdef USE_TIMER
   /* Register the interval timer callback if necessary */
    for (dwCurrVS = 0; dwCurrVS < numVoiceSetups; dwCurrVS++)
    {
        SEID seid;
        stSEState *pState;

        seid = MAKE_SEID(GET_SEINDEX(voiceSetups[dwCurrVS]->voice));
        pState = FIND_SESTATE(seid);
        if ((pState->dwHWRevision <= 3) && (pState->itmISRHandle == 0))
        {
            ITMSCHEDINFO schedinfo;
            schedinfo.dwNumerator = 960;
            schedinfo.dwDenominator = 48000;
            schedinfo.dwFlags       = ITMSF_RUN_AT_ISR_TIME;
            schedinfo.fCallback     = _timerISRCallback;
            schedinfo.dwUser1       = seid;
            itmScheduleCallback(pState->itmid, &schedinfo, &pState->itmISRHandle);
        }
    }
#endif

	return SUCCESS;
}


/*****************************************************************************
 * @func Start a single voice.  This function is tuned for situations where
 *  the caller needs to start the same voice over and over again.  We assume
 *  that the voice has been fully configured, so we only update parameters
 *  which need to be rewritten every time the oscillator is fired.
 */

EMUAPIEXPORT EMUSTAT
seVoiceStart(DWORD numVoices, SEVOICE *sevoices)
{
	stVoice *   voice;			/* A handle to the voice */
	HALID		halid;			/* The underlying HAL ID for I/O routines */
	DWORD *     dwParams;		/* The dereferenced voice parameter array */
	WORD        count;			/* Iterator variable for voices */
	DWORD       dwChanIdx;      /* Channel index of voice being changed */
	EMUSTAT     status;         /* Return status of prepareVoice call */
    stVoice *   voiceArray[SE_NUM_VOICES];
    DWORD       dwCurrentPitch[SE_NUM_VOICES]; 
								/* An array of current pitches for channels */
	WORD        wVolEnv[SE_NUM_VOICES];		
								/* An array of volumes for the channels */
//	DWORD ptabData;

	ASSERT(numVoices <= SE_NUM_VOICES);
	ASSERT(sevoices);

    for (count = 0; count < numVoices; count++) {
      
		CHECK_VOICE(voice, sevoices[count]);
		voiceArray[count] = voice;
        DPRINTF(("For voice channel %d", voice->byIndex));
		
		/* Check to see whether this voice is configured */
		if (!(voice->wFlags & VF_CONFIGURED))
			RETURN_ERROR(SERR_NOT_CONFIGURED);

		halid     = voice->halid;
		dwChanIdx = voice->byIndex;
		dwParams  = voice->dwParamValues;

		/* If we were previously in the playing or released stages,
		 * we need to release our sample handle. */
		if (voice->vstate == VS_RELEASED || voice->vstate == VS_STARTED)
			seVoiceStop(1, &sevoices[count]);

		/* Completely shut down the channel */
		WGWrite(halid, (VEDS | dwChanIdx), (WORD)ENV_ENGINE_OFF(0));
		LGWrite(halid, (VTFT | dwChanIdx), 0x0000FFFF);

		/* Crank the pitch down to zero */
		LGWrite(halid, (IP|dwChanIdx), 0x0);
      /* Potential fix to remove clicks */
//      LGRead(halid, (PTAB|dwChanIdx), &ptabData);
//      ptabData &= 0x0000FFFF;
//		LGWrite(halid, (PTAB|dwChanIdx), ptabData);
		LGWrite(halid, (PTAB|dwChanIdx), 0x0);
		LGWrite(halid, (CPF |dwChanIdx), 0x0);

      if (dwParams[sepDisableEnv] == 0) {
	      fourByteUnion       fbu;

      	/* Write the volume attack and hold times */
			fbu.byVals.by0 = (BYTE) (dwParams[sepAttackVolEnv] & 0x7F);
			fbu.byVals.by1 = (BYTE) (dwParams[sepHoldVolEnv] & 0x7f);
			WGWrite(halid, (VEHA | dwChanIdx), fbu.wVals.w0);

			/* Write the modulation LFO delay */
			WGWrite(halid, (MLV | dwChanIdx), (WORD) dwParams[sepDelayModLFO]);

			/* Write the mod envelope attack and hold */
			fbu.byVals.by0 = (BYTE) dwParams[sepAttackModEnv] & 0x7F;
			fbu.byVals.by1 = (BYTE) dwParams[sepHoldModEnv] & 0x7F;
			WGWrite(halid, (MEHA | dwChanIdx), fbu.wVals.w0);

			/* Write the mod envelope decay and sustain */
			fbu.byVals.by0 = (BYTE) dwParams[sepDecayModEnv] & 0x7F;
			fbu.byVals.by1 = (BYTE) dwParams[sepSustainModEnv] & 0x7f;
			WGWrite(halid, (MEDS | dwChanIdx), fbu.wVals.w0);
      }

		/* Set up the voice for playback */
		status = _prepareVoice(voice, dwParams, &dwCurrentPitch[count], &wVolEnv[count]);
		if (status != SUCCESS)
			RETURN_ERROR(status);
	}

	/* Start up the oscillators */	
	for (count = 0; count < numVoices; count++) {
		voice     = voiceArray[count];
		halid     = voiceArray[count]->halid;
		dwChanIdx = voiceArray[count]->byIndex;

        /* Check to see whether the oscillator is slaved to something else */
        if (voice->dwParamValues[sepStereo] == SESTMODE_INTERLEAVED) {
            if (dwChanIdx & 0x1) {
                /* Slave (odd) channel; phase lock it */
                LGWrite(halid, (PTAB | dwChanIdx), dwCurrentPitch[count] & 0x0000FFFFL);
                LGWrite(halid, (CPF  | dwChanIdx), 0x000);
            } else {
                /* Master (even) channel, and we're in interleave mode */
                LGWrite(halid, (IP   | dwChanIdx), voice->dwParamValues[sepInitialPitch]);
                LGWrite(halid, (PTAB | dwChanIdx), dwCurrentPitch[count]);
                LGWrite(halid, (CPF  | dwChanIdx),
                        (dwCurrentPitch[count] & 0xFFFF0000L) | 0x8000L);
            }
        } else if (voice->dwParamValues[sepStereo] == SESTMODE_STEREO) {
            if (dwChanIdx & 0x1) {
                /* Slave channel; phase lock it (set the S bit) */
                LGWrite(halid, (PTAB | dwChanIdx),
                        dwCurrentPitch[count] & 0x0000FFFF);
                //LGWrite(halid, (CPF | dwChanIdx), 0x8000);
            } else {
                /* Master channel */
                LGWrite(halid, (IP  |dwChanIdx), voice->dwParamValues[sepInitialPitch]);
                LGWrite(halid, (PTAB|dwChanIdx), dwCurrentPitch[count]);
				LGWrite(halid, (CPF | (dwChanIdx+1)), 0x8000);
                LGWrite(halid, (CPF |dwChanIdx), dwCurrentPitch[count]&0xFFFF0000L);
            }
        } else {
            /* This channel is in mono mode, so just write the registers */
            LGWrite(halid, (IP   | dwChanIdx), voice->dwParamValues[sepInitialPitch]);
            LGWrite(halid, (PTAB | dwChanIdx), dwCurrentPitch[count]);
            LGWrite(halid, (CPF  | dwChanIdx), (dwCurrentPitch[count] & 0xFFFF0000L));
        }
    }


	/* Activate the envelope engines */
	for (count = 0; count < numVoices; count++) {
		halid     = voiceArray[count]->halid;
		dwChanIdx = voiceArray[count]->byIndex;

		if (dwParams[sepDisableEnv] == FALSE) {
			DPRINTF(("Voice %d: Setting VEDS to %d", dwChanIdx, 
			         wVolEnv[count]));
			WGWrite(halid, (VEDS | dwChanIdx), (WORD) (ENV_ENGINE_ON(wVolEnv[count])));
		}

        /* Complete the synchronization steps if necessary */
        if (voiceArray[count]->wFlags & VF_SYNCABLE)
            _completeSyncSetup(voiceArray[count]);
	}

#ifdef USE_TIMER
   /* Register the interval timer callback if necessary */
    for (count = 0; count < numVoices; count++)
    {
        SEID seid;
        stSEState *pState;

        seid = MAKE_SEID(GET_SEINDEX(sevoices[count]));
        pState = FIND_SESTATE(seid);
        if ((pState->dwHWRevision <= 3) &&
            (pState->itmISRHandle == (ITMHANDLE)NULL))
        {
            ITMSCHEDINFO schedinfo;
            schedinfo.dwNumerator = 960;
            schedinfo.dwDenominator = 48000;
            schedinfo.dwFlags       = ITMSF_RUN_AT_ISR_TIME;
            schedinfo.fCallback     = _timerISRCallback;
            schedinfo.dwUser1       = seid;
            itmScheduleCallback(pState->itmid, &schedinfo, &pState->itmISRHandle);
        }
    }
#endif

	return SUCCESS;
}


/*************************************************************************
 * @func Prepares a voice for playback by initializing all of the registers
 *  whose values need to be updated at the start of playback.  Some of the
 *  registers, like the loop start and end addresses, could perhaps be 
 *  omitted, but we update them in order to deal with the possibility
 *  that the sound gets relocated by sample memory compaction.
 */
static EMUSTAT
_prepareVoice(stVoice *voice, DWORD *dwParams, DWORD *pdwCurrentPitch,
			  WORD *puiEnvVol)
{
	HALID               halid = voice->halid;
	DWORD               dwChanIdx = voice->byIndex;
	fourByteUnion       fbu;
	fourByteUnion       fbInitPitch;
    fourByteUnion       fbQKBCA;
	twoByteUnion        tbInitFilt;
	WORD                uiInitVolume;
	DWORD               dwBase;
	DWORD               dwCount;
   DWORD cdData;
   DWORD ccrData;
   DWORD dwSampleStart;
   DWORD dwStartLoop;
   DWORD dwEndLoop;
   BYTE  bySamplesInCache;
   BYTE  byNumPreloadSamples;
   DWORD dwTmpData[32];
   BYTE i;
   BYTE byFrameSize;
	ASSERT(voice && dwParams && pdwCurrentPitch && puiEnvVol);

	if (dwParams[sepDisableEnv] == 0) {

	   /*
	    * case of a zero attack time.  This is conventionally indicated
	    * by an attack of 0x7f and a decay value of 0x8000, but we
	    * actually need to skip over the envelope's attack phase, since
  	    * the attack slope has a minimum duration of 6ms.
		*/
		if (dwParams[sepAttackModEnv] < 0x7F || 
			dwParams[sepDelayModEnv] != 0x8000) {
			
			/* Setup up for non-zero attack time */
			WGWrite(halid, (MEV | dwChanIdx), (WORD) dwParams[sepDelayModEnv]);
			
			tbInitFilt.byVals.by1 = (BYTE) dwParams[sepInitialFilterFc];
			fbInitPitch.dwVal = IP_TO_CP(dwParams[sepInitialPitch]);
		} else {
				
			/* Handle the case where the attack time is zero.  We do this
			 * by effectively setting an infinite envelope delay and
			 * computing the appropriate target volumes "by hand."
			 */
			
			/* Set the initial mod env delay time to infinite */
			//WGWrite(halid, (MEV | dwChanIdx), 0x8000);
			WGWrite(halid, (MEV | dwChanIdx), 0xBFFF);
			
			/* Calculate initial pitch; since the pitch is only a 16-bit
			 * value, we check for underflow and overflow */
			fbu.dwVal = dwParams[sepInitialPitch] + (dwParams[sepModEnvToPitch] << 4);
			if (fbu.dwVal & 0x80000000L)
				/* Pitch went negative; pull it up to 0 */
				fbu.dwVal = 0x0;
			else if (fbu.dwVal & 0x00010000L)
				/* Pitch overflowed; drop it to the maximum value */
				fbu.dwVal = 0x0000FFFFL;
			
			fbInitPitch.dwVal = IP_TO_CP(fbu.dwVal);
			
			/* Calculate the initial filter value; the filter value is an
			 * 8-bit value, so we check for underflow and overflow */
			tbInitFilt.wVal = (WORD) (dwParams[sepInitialFilterFc] + 
					dwParams[sepModEnvToFilterFc]);
			if (tbInitFilt.wVal & 0x8000)
				tbInitFilt.wVal = 0;
			else if (tbInitFilt.wVal & 0x100)
				tbInitFilt.wVal = 0xFF;
			
			tbInitFilt.byVals.by1 = tbInitFilt.byVals.by0;
			tbInitFilt.byVals.by0 = 0xFF;
		}
		
		/* Now we check for a zero attack on the volume envelope. */
		if (dwParams[sepAttackVolEnv] < 0x7F || 
			dwParams[sepDelayVolEnv] != 0x8000) {
			
			WGWrite(halid, (VEV | dwChanIdx), (WORD) dwParams[sepDelayVolEnv]);
			uiInitVolume = 0;
		} else {
			
			/* Skip past the attack phase of the envelope and stop at the */
			//WGWrite(halid, (VEV | dwChanIdx), 0x8000);
			WGWrite(halid, (VEV | dwChanIdx), 0xBFFF);
			
			fbu.dwVal    = IV_TO_CV(dwParams[sepInitialVolume]);
			uiInitVolume = fbu.wVals.w1;
		}
	} /* End of if (dwParams[sepDisableEnv] == 0) */

	
	/* Check to see whether volume has in fact decayed all the 
	 * way.  This seems a little frightening, since we've just
	 * reprogrammed a whole bunch of crap, but.. 
	 */
	dwCount = 50;
	do {
		LGRead(halid, (CVCF | dwChanIdx), &fbu.dwVal);
		if (fbu.wVals.w1 > 4) {
			halWaitWallClockCounts(halid, 1);
			dwCount--;
		}
	} while (fbu.wVals.w1 > 0 && dwCount);

	if (dwCount == 0) {
		DWORD dwVTFT = L8010SERegRead(halid, (WORD) (CVCF | dwChanIdx));
		DWORD dwCVCF = L8010SERegRead(halid, (WORD) (VTFT | dwChanIdx));
		DPRINTF(("WARNING, CV never went to 0; CVCF = 0x%lx, VTFT = 0x%lx\n",
				dwVTFT, dwCVCF));
	}

	/* Stop the output and DMA engine by turning off the volume and 
 	 * setting the pitch to 0
	 */
	LGWrite(halid, (CVCF | dwChanIdx), 0x0000FFFFL);
	
	/* Program the loop start and end addresses; to do this we need
	 * to lock the memory handle used by this voice so that it can't
	 * move around when we attempt to manipulate it.
	 */
	voice->vstate = VS_STARTED;
	if (smMemoryAddReference(voice->smh, MAKE_SEVOICE_FROM_PTR(voice)) != SUCCESS)
		RETURN_ERROR(SERR_BAD_SMHANDLE);

	smMemoryGetBaseAndLength(voice->smh, &dwBase, NULL);

	/* The Base is always stored in bytes, but the addresses are in
	 * sample frames.  So convert things appropriately.  */
   byFrameSize = ((dwParams[sepEightBitSample] == 0) ? 2 : 1)*
                 ((dwParams[sepStereo] == SESTMODE_INTERLEAVED) ? 2 : 1);
   dwBase /= byFrameSize;

   if ((dwParams[sepStereo] != SESTMODE_INTERLEAVED) ||
       !(dwChanIdx & 0x1))
   {
	   LGWrite(halid, (CCR   | dwChanIdx), 0);

      /* Get the loop addresses awhile so we can properly configure the start
         address and cache */
	   dwStartLoop = dwParams[sepStartloopAddrs] + dwBase;
	   dwEndLoop = dwParams[sepEndloopAddrs] + dwBase;

      dwSampleStart = dwParams[sepStartAddrs] + dwBase;

      /* For synced voices, we need to get the addresses set
       * up for initial voice synchronization  */
      if (voice->wFlags & VF_SYNCABLE) {
          dwStartLoop   = dwSampleStart;
          dwSampleStart = dwEndLoop;
      }

      bySamplesInCache =
         32*((dwParams[sepEightBitSample] == 0) ? 1 : 2) - 4;

      if (dwSampleStart + bySamplesInCache <= dwEndLoop)
      {
         /* If the audio start point + # of samples in cache is before the
            end loop, let the chip fetch the data into the cache */
         dwSampleStart += bySamplesInCache;
         /* Hack for A3 workaround */
         if ((voice->state->dwHWRevision <= 3) && (dwSampleStart&0xf))
         {
             byNumPreloadSamples = bySamplesInCache - (BYTE)(dwSampleStart&0xf);
             dwSampleStart &= 0xfffff0;
         }
         else
             byNumPreloadSamples = bySamplesInCache;
         if (byNumPreloadSamples >= ((bySamplesInCache+4)/2))
             ccrData = ((DWORD)byNumPreloadSamples << 25) |
                       ((DWORD)bySamplesInCache << 16);
         else
         {
             BYTE *pBuf = (BYTE *)dwTmpData;

             ccrData = ((DWORD)bySamplesInCache << 16);
             memset(pBuf, 0, 32*sizeof(DWORD));
             smMemoryRead(dwParams[sepSampleMemoryHandle],
                          pBuf+(bySamplesInCache-byNumPreloadSamples)*byFrameSize,
                          dwParams[sepStartAddrs]*byFrameSize,
                          byNumPreloadSamples*byFrameSize);
         }
      }
      else if (!(voice->wFlags & VF_SYNCABLE))
      {
         /* If the end loop is within the cache, we must do things differently */
         if (dwStartLoop < dwEndLoop)
         {
            /* If the start loop is before the end loop, this is a MIDI loop,
               so we must (potentially) unwrap the loop */
            BYTE byInitBytes = (BYTE)(dwEndLoop-dwSampleStart)*byFrameSize;
            BYTE byLoopBytes = (BYTE)(dwEndLoop-dwStartLoop)*byFrameSize;
            BYTE *pBuf = (BYTE *)dwTmpData;

            ccrData = ((DWORD)bySamplesInCache << 16);
            smMemoryRead(dwParams[sepSampleMemoryHandle], pBuf,
                         dwParams[sepStartAddrs]*byFrameSize, byInitBytes);
            for (i = byInitBytes; i < 64; i++)
               pBuf[i] = pBuf[i-byLoopBytes];
            dwSampleStart += bySamplesInCache;
            while (dwSampleStart >= dwEndLoop)
               dwSampleStart -= (dwEndLoop - dwStartLoop);
         }
         else
         {
            /* If the start loop is after the end loop, this is SAOutput double
               buffering, so we only want to let the chip fetch data up to the
               loop point.  Zeros will play before the real data. */
            byNumPreloadSamples = (BYTE)(dwEndLoop - dwSampleStart);
            ccrData = ((DWORD)byNumPreloadSamples << 25) |
                      ((DWORD)bySamplesInCache << 16);
            dwSampleStart = dwEndLoop;
         }
      }
   }

	/* Okay, now we can actually write all of the useful bits
	 * into the register.
	 */

   /* First, we must set the end loop to something greater than the start
      address just in case it would cause a loop to occur */
   if ((dwParams[sepStereo] != SESTMODE_INTERLEAVED) ||
       !(dwChanIdx & 0x1))
   {
      fbu.dwVal = 0xffffff;
	   fbu.byVals.by3 = (BYTE) dwParams[sepEffectsSendD];
	   LGWrite(halid, (SDL | dwChanIdx), fbu.dwVal);

      /* Now set the QKBCA register; we set the Q to zero initially since we
       * want to be able to zero the Z1 and Z2 registers.  We'll set Q to the
       * correct value later. */
      fbQKBCA.dwVal = dwSampleStart;
	   fbQKBCA.byVals.by3 |= (BYTE) 
		   (((dwParams[sepInterpolationROM] & 0x7) << 1) | 
		   dwParams[sepEightBitSample]);
	   DPRINTF(("    QKBCA = 0x%lx", fbQKBCA.dwVal));
         LGWrite(halid, (QKBCA | dwChanIdx), fbQKBCA.dwVal);
      /* If interleaved at this point, we can assume that this is the even
         channel */
      if (dwParams[sepStereo] == SESTMODE_INTERLEAVED)
	      LGWrite(halid, (QKBCA | (dwChanIdx+1)), fbQKBCA.dwVal);
   }
	
	if (dwParams[sepDisableEnv] != 0x0) {
		/* Envelope engine is disabled */
		uiInitVolume         = (WORD) dwParams[sepRawVolume];
		tbInitFilt.wVal      = (WORD) dwParams[sepRawFilter];
		fbInitPitch.wVals.w1 = (WORD) dwParams[sepRawPitch];
		*puiEnvVol = 0;
	} else { 
		fbu.byVals.by1 = (BYTE) dwParams[sepSustainVolEnv] & 0x7F;
		fbu.byVals.by0 = (BYTE) dwParams[sepDecayVolEnv] & 0x7F;
		*puiEnvVol = fbu.wVals.w0;
	}
	
   /* Now preload or prefill the cache */
   if ((dwParams[sepStereo] != SESTMODE_INTERLEAVED) ||
        !(dwChanIdx & 0x1))
   {
      if (dwParams[sepStereo] == SESTMODE_INTERLEAVED)
      {
         /* Set the CRA of the odd channel to phase lock them */
         LGWrite(halid, (CCR | (dwChanIdx+1)), ccrData&0x00ff0000);
         /* Then, set the stereo bit */
		   LGWrite(halid, (CPF |dwChanIdx), 0x8000);
      }

      if ((ccrData&0xff000000) != 0)
      {
         /* Fill in zeros at the beginning of the cache if necessary*/
         for (cdData = CD0; (byNumPreloadSamples < bySamplesInCache) &&
               (cdData <= CDF); byNumPreloadSamples += 4/byFrameSize, cdData += 0x10000)
            LGWrite(halid, (cdData | dwChanIdx),
                    (dwParams[sepEightBitSample] == 0) ? 0x0 : 0x80808080);
         for (cdData = CD0; (byNumPreloadSamples < bySamplesInCache) &&
               (cdData <= CDF); byNumPreloadSamples += 4/byFrameSize, cdData += 0x10000)
            LGWrite(halid, (cdData | (dwChanIdx+1)),
                    (dwParams[sepEightBitSample] == 0) ? 0x0 : 0x80808080);

         /* The chip will fetch the data into the cache. */
         LGWrite(halid, (CCR | dwChanIdx), ccrData);
      }
      else
      {
         /* We must fill the cache data ourselves */
         for (cdData = CD0, i = 0; cdData <= CDF; cdData += 0x10000, i++)
            LGWrite(halid, (cdData | dwChanIdx), dwTmpData[i]);
         if (dwParams[sepStereo] == SESTMODE_INTERLEAVED)
         {
            for (cdData = CD0, i = 0x10; cdData <= CDF; cdData += 0x10000, i++)
               LGWrite(halid, (cdData | dwChanIdx+1), dwTmpData[i]);
         }
         LGWrite(halid, (CCR | dwChanIdx), ccrData);
      }

      /* Fill in zeros at the end of the cache */
      for (cdData = CDF-(0x10000*(byFrameSize-1));  cdData <= CDF;
           cdData += 0x10000)
              LGWrite(halid, (cdData | (dwChanIdx +
                     ((dwParams[sepStereo] == SESTMODE_INTERLEAVED) ? 1 : 0))),
                 (dwParams[sepEightBitSample] == 0) ? 0x0 : 0x80808080);
   }

    /* Clear the filter memory lines */
    LGWrite(halid, (Z1 | dwChanIdx), 0);
	LGWrite(halid, (Z2 | dwChanIdx), 0);

    /* I think what the following does is give the filter coefficients
     * time to go to DC based on what's actually in the cache.  I'd freely
     * admit that there is a definite element of Voodoo to the choice of 5
     * though.
     */
    halWaitWallClockCounts(halid, 5);

    /* In order to avoid pops caused by the filter state changing, we
     * need to zero Z1 and Z2.  To do this we need to set the filter
     * Q to zero (which is done earlier when we initially set QKBCA).
     * Unfortunately, Dave Rossum says that we shouldn't zero these
     * until the cache state has been set up, so we do the following:  */
    if (dwParams[sepInitialFilterQ]) {
        fbQKBCA.byVals.by3 |= (dwParams[sepInitialFilterQ] << 4);
        LGWrite(halid, QKBCA | dwChanIdx, fbQKBCA.dwVal);
        halWaitWallClockCounts(halid, 1);
    }

    fbu.wVals.w1 = uiInitVolume;
	fbu.wVals.w0 = tbInitFilt.wVal;
	LGWrite(halid, (VTFT | dwChanIdx), fbu.dwVal);
	LGWrite(halid, (CVCF | dwChanIdx), fbu.dwVal);

	fbu.dwVal      = fbInitPitch.dwVal;
	fbu.byVals.by1 = (BYTE) dwParams[sepEffectsSendA];
	fbu.byVals.by0 = (BYTE) dwParams[sepEffectsSendB];
	*pdwCurrentPitch = fbu.dwVal;


    /* Now set the start loop and (most especially) the end loop so the
      cache filled properly */
   if ((dwParams[sepStereo] != SESTMODE_INTERLEAVED) ||
        !(dwChanIdx & 0x1))
   {
	   /* Program the start loop address and send C amount */
	   fbu.dwVal = dwStartLoop;
	   fbu.byVals.by3 = (BYTE) dwParams[sepEffectsSendC];
	   DPRINTF(("    startloop 0x%lx  EffectsSendC 0x%lx   SCSA = 0x%lx",
		       dwParams[sepStartloopAddrs] + dwBase, dwParams[sepEffectsSendC],
	           fbu.dwVal));
	   LGWrite(halid, (SCSA | dwChanIdx), fbu.dwVal);
	
	   /* Program the end loop address and send D amount */
	   fbu.dwVal = dwEndLoop;
	   fbu.byVals.by3 = (BYTE) dwParams[sepEffectsSendD];
	   DPRINTF(("    SDL = 0x%lx", fbu.dwVal));
	   LGWrite(halid, (SDL | dwChanIdx), fbu.dwVal);
   }

    /* When in syncable mode, we need to set the stop on loop
     * bit to insure that the voice won't go anywhere.
     */
    if (voice->wFlags & VF_SYNCABLE)
        _updateStopOnLoop(voice, TRUE);

	return SUCCESS;
}


static void
_completeSyncSetup(stVoice *pVoice)
{
    /* Give the voice a couple of wall clock counts to
     * wrap back to the starting address */
    halWaitWallClockCounts(pVoice->halid, 2);

    /* Set the start loop address back to the real value */
    seParameterWrite(MAKE_SEVOICE_FROM_PTR(pVoice), sepStartloopAddrs,
                     pVoice->dwParamValues[sepStartloopAddrs]);
}



/****************************************************************************
 * @func Kick a voice into the release stage
 */
EMUAPIEXPORT EMUSTAT
seVoiceRelease(DWORD numVoices, SEVOICE *sevoices)
{
	stVoice *	voice;
	HALID		halid;
	DWORD		dwChanIdx;
	DWORD		count;
	DWORD       dwSave;

	ASSERT(sevoices != NULL);
	ASSERT(numVoices <= SE_NUM_VOICES);

	for (count = 0; count < numVoices; count++) {
		CHECK_VOICE(voice, sevoices[count]);

		halid     = voice->halid;
		dwChanIdx = voice->byIndex;

		/* Sometimes we're asked to release voices which aren't playing.
		 * This is a bad thing, and we don't want to do it. */
		if (voice->vstate != VS_STARTED)
			continue;

		ASSERT(voice->byNumRefs > 0);

		if (voice->dwParamValues[sepDisableEnv]) {
			return seVoiceStop(1, &sevoices[count]);
		}

		/* Kick the envelopes into the release phase */ 
		dwSave=(voice->dwParamValues[sepReleaseVolEnv]&0x7F);
		LSEPtrWrite(halid, (VEDS | dwChanIdx), ENV_ENGINE_RELEASE(dwSave));

		dwSave=(voice->dwParamValues[sepReleaseModEnv]&0x7F);
		LSEPtrWrite(halid, (MEDS | dwChanIdx), ENV_ENGINE_RELEASE(dwSave));

		voice->vstate = VS_RELEASED;
	}

#ifdef USE_TIMER
   /* Register the interval timer callback if necessary */
    for (count = 0; count < numVoices; count++)
    {
        SEID seid;
        stSEState *pState;

        seid = MAKE_SEID(GET_SEINDEX(sevoices[count]));
        pState = FIND_SESTATE(seid);
        if ((pState->itmHandle == (ITMHANDLE)NULL))
        {
            ITMSCHEDINFO schedinfo;
            schedinfo.dwNumerator = 960;
            schedinfo.dwDenominator = 48000;
            schedinfo.dwFlags       = 0;
            schedinfo.fCallback     = _timerCallback;
            schedinfo.dwUser1       = seid;
            itmScheduleCallback(pState->itmid, &schedinfo, &pState->itmHandle);
        }
    }
#endif

	return SUCCESS;
}


/****************************************************************************
 * @func Shuts down a voice immediately
 */

EMUAPIEXPORT EMUSTAT
seVoiceStop(DWORD numVoices, SEVOICE *sevoices)
{
	stVoice *	voice;
	WORD		count;

	ASSERT(sevoices != NULL);
	ASSERT(numVoices <= SE_NUM_VOICES);

	for (count = 0; count < numVoices; count++) {
		CHECK_VOICE(voice, sevoices[count]);

		if (voice->vstate == VS_STOPPED)
			continue;

		/* Check to see if the voice is interleaved.  If so, use the
		 * special paired stop call to shut it down. 
		 */
		//if (voice->dwParamValues[sepStereo] == SESTMODE_INTERLEAVED)
		if (voice->dwParamValues[sepStereo] != SESTMODE_MONO)
		{
			_stopStereoVoice(voice);
		}
		else 
		{
			_stopVoice(voice);
                 
			/* Remove the sample reference */
			if (voice->vstate == VS_STARTED || voice->vstate == VS_RELEASED)
				smMemoryRemoveReference(voice->smh, sevoices[count]);

			/* This needs to be done after smMemoryRemoveReference, otherwise
			 * an sm assertion will trigger.  */
			voice->vstate = VS_STOPPED;
		}
	}

	return SUCCESS;
}


/* This routine looks at a voice and determines whether it is part of
 * an interleaved pair.  If so, it figures out who the master is and
 * goes through an interleaved voice shut down sequence.
 */
static void
_stopStereoVoice(stVoice *voice)
{
    stVoice *pMasterVoice;
	stVoice *pSlaveVoice;
	DWORD    dwMasterIdx;
	DWORD    ptabData;
#ifdef SE_RAMP_SENDS
    stVoiceRampData *tmp, dummy;
    BYTE byRampDel;
#endif

	dwMasterIdx  = (voice - voice->state->voices) & (~ 0x1);
	pMasterVoice = &voice->state->voices[dwMasterIdx];
	pSlaveVoice  = &voice->state->voices[dwMasterIdx + 1];

#ifdef SE_RAMP_SENDS
    /* Check for ramping sends */
    dummy.next = voice->state->voiceRampList;
    tmp = &dummy;
    byRampDel = 0;
    while ((tmp->next != NULL) && (byRampDel < 2))
    {
        if ((tmp->next->pVoice->byIndex == pMasterVoice->byIndex) ||
            (tmp->next->pVoice->byIndex == pSlaveVoice->byIndex))
        {
            if (tmp->next->itmRampISRHandle != (ITMHANDLE)NULL)
            {
                stVoiceRampData *tmpdel;

                tmpdel = tmp->next;
                tmp->next = tmp->next->next;
                itmCancelCallback(tmpdel->itmRampISRHandle);
                osLockedHeapFree(tmpdel, sizeof(stVoiceRampData));
            }
            byRampDel++;
        }
        else
            tmp = tmp->next;
    }
    voice->state->voiceRampList = dummy.next;
#endif

	WGWrite(voice->halid, (VEDS | pMasterVoice->byIndex), ENV_ENGINE_OFF(0x7F));
	WGWrite(voice->halid, (VEDS | pSlaveVoice->byIndex),  ENV_ENGINE_OFF(0x7F));
	halWaitWallClockCounts(voice->halid, 1);
	
	LGWrite(voice->halid, (VTFT | pMasterVoice->byIndex), 0x0000FFFF);
	LGWrite(voice->halid, (VTFT | pSlaveVoice->byIndex),  0x0000FFFF);

	/* Potential fix to remove clicks.  The idea is not to zero out the
	 * effects sends unnecessarily.  */
	LGRead(voice->halid, (PTAB | pMasterVoice->byIndex), &ptabData);
	ptabData &= 0x0000FFFF;
	LGWrite(voice->halid, (PTAB | pMasterVoice->byIndex), ptabData);
	LGRead(voice->halid, (PTAB | pSlaveVoice->byIndex), &ptabData);
	ptabData &= 0x0000FFFF;
	LGWrite(voice->halid, (PTAB | pSlaveVoice->byIndex),  ptabData);

#if 0
	/* Fix for separating the interleaved voices.  If we don't clear the
	CCR of the slave voice before decoupling them, it may do a fetch to
	invalid memory.  We probably don't want to leave this for loop in
	permanently, though.  */
	for (count = 0, cpfData = 0;
	(count < 10000) && (cpfData != lastCpfData); count++)
	{
		lastCpfData = cpfData;
		halWaitWallClockCounts(voice->halid, 1);
		LGRead(voice->halid, (CPF | pMasterVoice->byIndex), &cpfData);
	}
#endif
	
	/* NOTE: The following ordering is fairly important.  When we write a 0
	 * to the master's CPF register, we end up decoupling the oscillators.
	 * The slave oscillator might be in strange state, though, and so we
	 * need to make sure that it won't try and fetch from memory once it
	 * has been decoupled, since its state hasn't been updated it a coherent
	 * manner while it has been coupled.  The best way to do this is to mark 
	 * its cache as filled and make sure that we turn it off before we turn off
	 * the master oscillator.
	 */
	LGWrite(voice->halid, (CCR | pSlaveVoice->byIndex), 0x0);
	LGWrite(voice->halid, (CPF | pSlaveVoice->byIndex), 0x0);
	LGWrite(voice->halid, (CPF | pMasterVoice->byIndex), 0x0);

	ASSERT((pMasterVoice->dwParamValues[sepStereo] == SESTMODE_STEREO) ||
           (pMasterVoice->smh == pSlaveVoice->smh));

	smMemoryRemoveReference(pMasterVoice->smh, MAKE_SEVOICE_FROM_PTR(pMasterVoice));
	smMemoryRemoveReference(pSlaveVoice->smh, MAKE_SEVOICE_FROM_PTR(pSlaveVoice));

	/* These need to come after the smMemoryRemoveReferences, because an 
	 * assertion in smpCheckConsistency will bomb if it finds stopped voices
	 * on a region's reference list.  */
	pMasterVoice->vstate = VS_STOPPED;
	pSlaveVoice->vstate  = VS_STOPPED;
}


static void
_stopVoice(stVoice *voice)
{
   DWORD ptabData;
#ifdef SE_RAMP_SENDS
    stVoiceRampData *tmp, dummy;
    BOOL bRampDel;
#endif

#ifdef SE_RAMP_SENDS
    /* Check for ramping sends */
    dummy.next = voice->state->voiceRampList;
    tmp = &dummy;
    bRampDel = FALSE;
    while ((tmp->next != NULL) && !bRampDel)
    {
        if (tmp->next->pVoice->byIndex == voice->byIndex)
        {
            if (tmp->next->itmRampISRHandle != (ITMHANDLE)NULL)
            {
                stVoiceRampData *tmpdel;

                tmpdel = tmp->next;
                tmp->next = tmp->next->next;
                itmCancelCallback(tmpdel->itmRampISRHandle);
                osLockedHeapFree(tmpdel, sizeof(stVoiceRampData));
            }
            bRampDel = TRUE;
        }
        else
            tmp = tmp->next;
    }
    voice->state->voiceRampList = dummy.next;
#endif

	WGWrite(voice->halid, (VEDS | voice->byIndex), ENV_ENGINE_OFF(0x7F));
	halWaitWallClockCounts(voice->halid, 1);

	LGWrite(voice->halid, (VTFT | voice->byIndex), 0x0000FFFF);

	/* Potential fix to remove clicks by not zeroing out the FX sends. 
	 * Not clear how necessary this is, but DanO suggested it. */
	LGRead(voice->halid, (PTAB | voice->byIndex), &ptabData);
	ptabData &= 0x0000FFFF;
	LGWrite(voice->halid, (PTAB | voice->byIndex), ptabData);
	LGWrite(voice->halid, (CPF  | voice->byIndex), 0x0);  
}


/* @func Set up the Stop on Loop bit for this voice based on the 
 *  value specified in the sepOneShot parameter 
 */
void
_updateStopOnLoop(stVoice *pvoice, BOOL bSetStopOnLoop)
{
	DWORD dwChanIdx  = pvoice->byIndex;
	DWORD dwRegister = ((dwChanIdx < (SE_NUM_VOICES/2)) ? SOLL : SOLH);
	DWORD dwValue = LSEPtrRead(pvoice->halid, (dwRegister | dwChanIdx));

	if (bSetStopOnLoop) 
		dwValue |= 1 << (dwChanIdx & (SE_NUM_VOICES/2 - 1));
	else
		dwValue &= ~(1 << (dwChanIdx & (SE_NUM_VOICES/2 - 1)));

	LGWrite(pvoice->halid, (dwRegister | dwChanIdx), dwValue);
}


#ifdef USE_TIMER
BOOL _timerISRCallback(ITMSCHEDINFO *psi)
{
	stSEState *pState;

	if ((pState = FIND_SESTATE(psi->dwUser1)) == NULL)
	{
		ASSERT(0);
		return FALSE;
	}

	if ((pState->dwHWRevision <= 3) &&
        (pState->wFreeVoices < SE_NUM_VOICES))
	{
		WORD wChan;

		for (wChan = 0; (wChan < SE_NUM_VOICES); wChan++)
		{
			stVoice *pVoice = &pState->voices[wChan];
			if (((pVoice->vstate == VS_STARTED) ||
                 (pVoice->vstate == VS_RELEASED)) &&
                ((pVoice->dwParamValues[sepStereo] != SESTMODE_INTERLEAVED) ||
                 !(wChan&1)))
            {
                DWORD dwCurIndex, dwEndloopIndex, dwNextIndex, dwBase;
                BYTE byFrameSize;

                smMemoryGetBaseAndLength(pVoice->smh, &dwBase, NULL);
                byFrameSize =
                    ((pVoice->dwParamValues[sepEightBitSample] == 0) ? 2 : 1)*
                    ((pVoice->dwParamValues[sepStereo] == SESTMODE_INTERLEAVED) ? 2 : 1);

                dwCurIndex = ((LSEPtrRead(pVoice->halid, pVoice->byIndex|QKBCA)&0xffffff)*
                              byFrameSize) >> 12;
                dwEndloopIndex = ((pVoice->dwParamValues[sepEndloopAddrs]&0xffffff)*
                                  byFrameSize + dwBase) >> 12;
                if (dwCurIndex == dwEndloopIndex)
                {
                    dwNextIndex = ((pVoice->dwParamValues[sepStartloopAddrs]&0xffffff)*
                                   byFrameSize + dwBase) >> 12;
                }
                else
                    dwNextIndex = dwCurIndex + 1;

                smUpdateMapRegisters(pVoice->smh, pVoice->byIndex, dwCurIndex,
                                     dwNextIndex);
            }
        }
    }
    /* Reschedule the callback */
    return TRUE;
}

BOOL _timerCallback(ITMSCHEDINFO *psi)
{
	stSEState *pState;
	WORD wNumReleasingVoices = 0;
	
	if ((pState = FIND_SESTATE(psi->dwUser1)) == NULL)
	{
		ASSERT(0);
		return FALSE;
    }
	
	if (pState->wFreeVoices < SE_NUM_VOICES)
	{
		WORD wChan;
		
		for (wChan = 0; (wChan < SE_NUM_VOICES); wChan++)
		{
			stVoice *pVoice = &pState->voices[wChan];

			if (pVoice->vstate == VS_RELEASED)
			{
				WORD wVolEnv;
				
				WGRead(pState->id, ((DWORD) pVoice->byIndex | VEV), &wVolEnv);

                /* Compute something akin to the actual volume level */
                pVoice->dwVolume = (wVolEnv * pVoice->dwParamValues[sepSustainVolEnv]) >> 3;
                ASSERT(pVoice->dwVolume <= 0x3FFFFF);

				if (wVolEnv == 0)
				{
					SEVOICE varr[2];
					
					varr[0] = MAKE_SEVOICE_FROM_PTR(pVoice);
					if (pVoice->dwParamValues[sepStereo] == SESTMODE_STEREO)
					{
						if (!(pVoice->byIndex&1))
						{
                            stVoice *pVoice2 = &pState->voices[wChan+1];
							WGRead(pState->id, ((DWORD)(pVoice->byIndex+1) | VEV), &wVolEnv);

                            pVoice2->dwVolume = (wVolEnv * pVoice2->dwParamValues[sepSustainVolEnv]) >> 3;
                            ASSERT(pVoice2->dwVolume <= 0x3FFFFF);

							if (wVolEnv == 0)
							{
								stVoice *pVoice2 = &pState->voices[wChan+1];

								varr[1] = MAKE_SEVOICE_FROM_PTR(pVoice2);
								seVoiceStop(2, varr);

								if (pVoice->notifyCallback)
									pVoice->notifyCallback(pVoice->dwCallbackData, 
										varr[0], seneZeroVolume);
								if (pVoice2->notifyCallback)
									pVoice2->notifyCallback(pVoice2->dwCallbackData,
										varr[1], seneZeroVolume);
							}
						}
					}
					else {
						seVoiceStop(1, varr);
						if (pVoice->notifyCallback)
							pVoice->notifyCallback(pVoice->dwCallbackData, 
								varr[0], seneZeroVolume);
                    }
				}
				else
					wNumReleasingVoices++;
			}
		}
	}
	
    if (wNumReleasingVoices) 
        return TRUE;
    else
    {
        pState->itmHandle = (ITMHANDLE)NULL;
        return FALSE;
    }
}
#endif

#ifdef SE_RAMP_SENDS
void _rampVoiceSend(stVoice *pVoice, DWORD dwParamID, DWORD dwCurData)
{
    stVoiceRampData *tmp;
    ITMSCHEDINFO schedinfo;
    DWORD i;

    /* First, check to see if we need to do anything */
    if (dwCurData == pVoice->dwParamValues[dwParamID])
        return;

    /* Next, check to see if we're already ramping this voice */
    for (tmp = pVoice->state->voiceRampList; tmp != NULL; tmp = tmp->next)
        if ((tmp->pVoice->byIndex == pVoice->byIndex) &&
            (tmp->itmRampISRHandle != (ITMHANDLE)NULL))
        {
            /* If we found it, just return */
            IDISABLE();
            if (tmp->byCurValue[dwParamID-sepEffectsSendC] != pVoice->dwParamValues[dwParamID])
            {
                DWORD dwTmpMaxLookAhead;

                if (tmp->byCurValue[dwParamID-sepEffectsSendC] > pVoice->dwParamValues[dwParamID])
                    dwTmpMaxLookAhead = _ComputeRampPitch(pVoice)/(RAMP_MAX_TIME*
                        ((tmp->byCurValue[dwParamID-sepEffectsSendC]-
                        pVoice->dwParamValues[dwParamID]+RAMP_ZERO_CROSS_INC-1)/
                            RAMP_ZERO_CROSS_INC));
                else
                    dwTmpMaxLookAhead = _ComputeRampPitch(pVoice)/(RAMP_MAX_TIME*
                        ((pVoice->dwParamValues[dwParamID]-
                        tmp->byCurValue[dwParamID-sepEffectsSendC]+RAMP_ZERO_CROSS_INC-1)/
                            RAMP_ZERO_CROSS_INC));
                if (dwTmpMaxLookAhead < RAMP_MIN_RANGE)
                    tmp->dwMaxLookAhead = RAMP_MIN_RANGE;
                else if (dwTmpMaxLookAhead < tmp->dwMaxLookAhead)
                    tmp->dwMaxLookAhead = dwTmpMaxLookAhead;
            }
            IENABLE();
            return;
        }

    /* Otherwise, add a new structure to the ramp list */
    tmp = osLockedHeapAlloc(sizeof(stVoiceRampData));
    tmp->pVoice = pVoice;
    for (i = sepEffectsSendC; i <= sepEffectsSendB; i++)
    {
        if (i == dwParamID)
            tmp->byCurValue[i-sepEffectsSendC] = (BYTE)dwCurData;
        else
            tmp->byCurValue[i-sepEffectsSendC] = (BYTE)pVoice->dwParamValues[i];
    }
    tmp->next = pVoice->state->voiceRampList;
    pVoice->state->voiceRampList = tmp;

    /* Now, schedule the ISR callback to do the ramping */
    schedinfo.dwDenominator = _ComputeRampPitch(pVoice);
    schedinfo.dwFlags       = ITMSF_RUN_AT_ISR_TIME;
    schedinfo.fCallback     = _rampISRCallback;
    schedinfo.dwUser1       = (DWORD)tmp;
    if (dwCurData > pVoice->dwParamValues[dwParamID])
        tmp->dwMaxLookAhead = schedinfo.dwDenominator/(RAMP_MAX_TIME*
            ((dwCurData-pVoice->dwParamValues[dwParamID]+RAMP_ZERO_CROSS_INC-1)/
                RAMP_ZERO_CROSS_INC));
    else
        tmp->dwMaxLookAhead = schedinfo.dwDenominator/(RAMP_MAX_TIME*
            ((pVoice->dwParamValues[dwParamID]-dwCurData+RAMP_ZERO_CROSS_INC-1)/
                RAMP_ZERO_CROSS_INC));
    if (tmp->dwMaxLookAhead > RAMP_MAX_LOOKAHEAD)
        tmp->dwMaxLookAhead = RAMP_MAX_LOOKAHEAD;
    else if (tmp->dwMaxLookAhead < RAMP_MIN_RANGE)
        tmp->dwMaxLookAhead = RAMP_MIN_RANGE;
    _GetNextRampUpdate(tmp, &schedinfo.dwNumerator);
    //schedinfo.dwNumerator = 5;
    //tmp->dwNextUpdate = 0;
    //tmp->byNextIncrement = 0;

    itmScheduleCallback(pVoice->state->itmid, &schedinfo, &tmp->itmRampISRHandle);

    /* Check to see if the cleanup callback needs to be scheduled */
    if (pVoice->state->itmRampHandle == (ITMHANDLE)NULL)
    {
        schedinfo.dwNumerator   = 480;
        schedinfo.dwDenominator = 48000;
        schedinfo.dwFlags       = 0;
        schedinfo.fCallback     = _rampCallback;
        schedinfo.dwUser1       = (DWORD)pVoice->state;
        itmScheduleCallback(pVoice->state->itmid, &schedinfo,
                            &pVoice->state->itmRampHandle);
    }
}

BOOL _rampISRCallback(ITMSCHEDINFO *psi)
{
    stVoiceRampData *tmp;
	stParamData *pAddInfo;
    DWORD dwTemp;
    LONG lTmpValue;
    BYTE byFinalValue, byNewValue[4];
    DWORD dwCurPos;
    DWORD i;
    BOOL bFoundOne = FALSE;
    stVoice *pVoice;
    stVoice *pMasterVoice;

    tmp = (stVoiceRampData *)psi->dwUser1;
    pVoice = tmp->pVoice;

    if ((pVoice->dwParamValues[sepStereo] == SESTMODE_INTERLEAVED) &&
        (pVoice->byIndex&1))
        pMasterVoice = &pVoice->state->voices[pVoice-pVoice->state->voices-1];
    else
        pMasterVoice = pVoice;

    dwCurPos = seParameterRead(MAKE_SEVOICE_FROM_PTR(pMasterVoice), sepStartAddrs);

    /* Check for late interrupt */
    if ((tmp->byNextIncrement == RAMP_ZERO_CROSS_INC) &&
        (dwCurPos > tmp->dwNextUpdate))
    {
        tmp->byNextIncrement = RAMP_MIN_RANGE_INC;
    }

    /* Compute the new send amounts */
    for (i = sepEffectsSendC; i <= sepEffectsSendB; i++)
    {
        byFinalValue = (BYTE)pVoice->dwParamValues[i];
        if (tmp->byCurValue[i-sepEffectsSendC] < byFinalValue)
        {
            lTmpValue = (LONG)tmp->byCurValue[i-sepEffectsSendC] + tmp->byNextIncrement;
            if (lTmpValue > byFinalValue)
                lTmpValue = byFinalValue;
            byNewValue[i-sepEffectsSendC] = (BYTE)lTmpValue;
        }
        else if (tmp->byCurValue[i-sepEffectsSendC] > byFinalValue)
        {
            lTmpValue = (LONG)tmp->byCurValue[i-sepEffectsSendC] - tmp->byNextIncrement;
            if (lTmpValue < byFinalValue)
                lTmpValue = byFinalValue;
            byNewValue[i-sepEffectsSendC] = (BYTE)lTmpValue;
        }
        else
            byNewValue[i-sepEffectsSendC] = tmp->byCurValue[i-sepEffectsSendC];
    }

    /* Wait until the correct update sample */
    if (tmp->byNextIncrement == RAMP_ZERO_CROSS_INC)
    {
        //ASSERT(tmp->dwNextUpdate < pMasterVoice->dwParamValues[sepEndloopAddrs]);

        while (dwCurPos < tmp->dwNextUpdate)
            dwCurPos = seParameterRead(MAKE_SEVOICE_FROM_PTR(pMasterVoice), sepStartAddrs);
    }
    else if (tmp->dwNextUpdate == pMasterVoice->dwParamValues[sepEndloopAddrs])
    {
        if (pMasterVoice->dwParamValues[sepStartloopAddrs] <
            pMasterVoice->dwParamValues[sepEndloopAddrs])
            while (dwCurPos >= pMasterVoice->dwParamValues[sepStartloopAddrs]+4)
                dwCurPos = seParameterRead(MAKE_SEVOICE_FROM_PTR(pMasterVoice), sepStartAddrs);
        else
            while (dwCurPos < pMasterVoice->dwParamValues[sepEndloopAddrs])
                dwCurPos = seParameterRead(MAKE_SEVOICE_FROM_PTR(pMasterVoice), sepStartAddrs);
    }

    /* Write the new send amounts to the chip */
    for (i = sepEffectsSendC; i <= sepEffectsSendB; i++)
    {
        if (tmp->byCurValue[i-sepEffectsSendC] != byNewValue[i-sepEffectsSendC])
        {
	        pAddInfo = (stParamData *)&AddressTable[i];
	        LGRead(pVoice->halid, (DWORD)(pVoice->byIndex)|pAddInfo->dwAddress, &dwTemp);
	        dwTemp &=  (~pAddInfo->dwMask);
	        dwTemp |=  ((byNewValue[i-sepEffectsSendC] << pAddInfo->byShift) & (pAddInfo->dwMask));
	        LGWrite(pVoice->halid, (DWORD)(pVoice->byIndex)|pAddInfo->dwAddress, dwTemp);
            tmp->byCurValue[i-sepEffectsSendC] = byNewValue[i-sepEffectsSendC];
        }
    }

    /* Look for the next zero crossing */
    for (i = sepEffectsSendC; i <= sepEffectsSendB; i++)
    {
        if (tmp->byCurValue[i-sepEffectsSendC] != pVoice->dwParamValues[i])
        {
            bFoundOne = TRUE;
            break;
        }
    }
    if (bFoundOne)
    {
        _GetNextRampUpdate(tmp, &psi->dwNumerator);
        return TRUE;
    }

    tmp->itmRampISRHandle = (ITMHANDLE)NULL;
    return FALSE;
}

BOOL _rampCallback(ITMSCHEDINFO *psi)
{
    stVoiceRampData *tmp, dummy;
    stSEState *pState;

    pState = (stSEState *)psi->dwUser1;
    dummy.next = pState->voiceRampList;
    tmp = &dummy;
    while (tmp->next != NULL)
    {
        DWORD i;
        BOOL bFoundOne=FALSE;

        if (tmp->next->itmRampISRHandle != (ITMHANDLE)NULL)
            for (i = sepEffectsSendC; i <= sepEffectsSendB; i++)
            {
                if (tmp->next->byCurValue[i-sepEffectsSendC] !=
                    tmp->next->pVoice->dwParamValues[i])
                {
                    bFoundOne = TRUE;
                    break;
                }
            }
        if (!bFoundOne)
        {
            stVoiceRampData *tmpdel;

            tmpdel = tmp->next;
            tmp->next = tmp->next->next;
            osLockedHeapFree(tmpdel, sizeof(stVoiceRampData));
        }
        else
            tmp = tmp->next;
    }
    pState->voiceRampList = dummy.next;

    if (pState->voiceRampList == NULL)
    {
        pState->itmRampHandle = (ITMHANDLE)NULL;
        return FALSE;
    }
    return TRUE;
}

void _GetNextRampUpdate(stVoiceRampData *pRampData, DWORD *pNumSamples)
{
    BOOL bFoundZeroCross = FALSE;
    DWORD dwCurPos;
    static BYTE buf[RAMP_MAX_LOOKAHEAD*4]; /* Static so the system doesn't
                                              allocate off the stack */
    BYTE byFrameSize;
    SHORT shTmpVal, shLastVal;
    DWORD dwBufSamples, dwNumSamples, dwOffset=0;
    stVoice *pVoice = pRampData->pVoice;
    stVoice *pMasterVoice;

    if ((pVoice->dwParamValues[sepStereo] == SESTMODE_INTERLEAVED) &&
        (pVoice->byIndex&1))
        pMasterVoice = &pVoice->state->voices[pVoice-pVoice->state->voices-1];
    else
        pMasterVoice = pVoice;

    /* Read sample data */
    dwCurPos = seParameterRead(MAKE_SEVOICE_FROM_PTR(pMasterVoice), sepStartAddrs);
    if (dwCurPos + 6 > pMasterVoice->dwParamValues[sepEndloopAddrs])
    {
        if (dwCurPos < pMasterVoice->dwParamValues[sepEndloopAddrs])
        {
            dwOffset = pMasterVoice->dwParamValues[sepEndloopAddrs] - dwCurPos;
            dwCurPos = pMasterVoice->dwParamValues[sepStartloopAddrs];
        }
        dwBufSamples = RAMP_MIN_RANGE;
    }
    else if (dwCurPos + pRampData->dwMaxLookAhead > pMasterVoice->dwParamValues[sepEndloopAddrs])
        dwBufSamples = pMasterVoice->dwParamValues[sepEndloopAddrs] - dwCurPos;
    else
        dwBufSamples = pRampData->dwMaxLookAhead;

    byFrameSize = ((pMasterVoice->dwParamValues[sepEightBitSample] == 0) ? 2 : 1)*
                   ((pMasterVoice->dwParamValues[sepStereo] == SESTMODE_INTERLEAVED) ? 2 : 1);
    smMemoryRead(pMasterVoice->smh, buf, dwCurPos*byFrameSize, dwBufSamples*byFrameSize);

    /* Look for a zero crossing */
    if (pMasterVoice->dwParamValues[sepEightBitSample] == 0)
        shTmpVal = *(SHORT *)buf;
    else
        shTmpVal = (SHORT)*buf-0x80;
    for (dwNumSamples = 1; dwNumSamples < dwBufSamples; dwNumSamples++)
    {
        shLastVal = shTmpVal;
        if (pMasterVoice->dwParamValues[sepEightBitSample] == 0)
            shTmpVal = *(SHORT *)(buf+dwNumSamples*byFrameSize);
        else
            shTmpVal = (SHORT)*(buf+dwNumSamples*byFrameSize)-0x80;
        if (((shLastVal > 0) && (shTmpVal <= 0)) ||
            ((shLastVal < 0) && (shTmpVal >= 0)))
        {
            bFoundZeroCross = TRUE;
            break;
        }
    }

    /* Compute the next interrupt point and increment */
    if (bFoundZeroCross)
    {
        *pNumSamples = dwNumSamples + dwOffset;
        pRampData->byNextIncrement = RAMP_ZERO_CROSS_INC;
        pRampData->dwNextUpdate = dwCurPos + dwNumSamples;
        if (pMasterVoice->dwParamValues[sepEightBitSample] == 0)
        {
            *pNumSamples += 29;
            pRampData->dwNextUpdate += 29;
        }
        else
        {
            *pNumSamples += 61;
            pRampData->dwNextUpdate += 61;
        }
        if (pRampData->dwNextUpdate >= pMasterVoice->dwParamValues[sepEndloopAddrs])
            pRampData->dwNextUpdate += pMasterVoice->dwParamValues[sepStartloopAddrs] -
                                       pMasterVoice->dwParamValues[sepEndloopAddrs];
    }
    else
    {
        *pNumSamples = dwBufSamples + dwOffset;
        pRampData->byNextIncrement = RAMP_MIN_RANGE_INC;
        pRampData->dwNextUpdate = dwCurPos + dwBufSamples;
    }

    /* Make sure we get interrupted before the sample */
    *pNumSamples -= 2;

    ASSERT(*pNumSamples < 0x7fffffff);

}

DWORD _ComputeRampPitch(stVoice *pVoice)
{
    if (pVoice->dwParamValues[sepDisableEnv])
        return pVoice->dwParamValues[sepRawPitch]*48000/0x4000;
    else
        return (IP_TO_CP(pVoice->dwParamValues[sepInitialPitch])>>16)*48000/0x4000;
}

#endif

/**************************************************************************
 * Private routines 
 **************************************************************************/

EMUAPIEXPORT stSEState *
stpGetSEState()
{
	return &seStates[0];
}


EMUAPIEXPORT DWORD
stpGetSECount()
{
	return seCount;
}

#endif

