/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*					
*******************************************************************************
* @doc INTERNAL
* @module sefriend.h | 
*  This file contains SE8210 sound engine private declarations which it
*  shares with the sample memory manager and potentially other components of
*  the Hardware Resource Manager codebase.  However, this stuff is not for
*  use by any upper-level code.
*
* @iex
* Revision History:
* Version	Person  	Date      	Reason
* -------	--------	----------	--------------------------------------
*  0.003	JK      	Oct 21, 97  Changed priority to a word and 
*									added private routines for data structs.	
*  0.002	JK			Sep 24, 97	Changed to use the 8210 name scheme.
*  0.001	JK			Sep 01, 97	Initial version
* @end
*******************************************************************************
*/

#ifndef __SEFRIEND_H
#define __SEFRIEND_H

/******************************************************************************
 * Included files
 ******************************************************************************/

#include "datatype.h"
#include "hal8210.h"
#include "sm8210.h"
#include "itm8210.h"

/******************************************************************************
 * Preprocessor definitions and macros 
 ******************************************************************************/

#define SE_NUM_VOICES 64


#ifndef NO_VOLUME_PRIORITIZE
# define PRIORITY_SHIFT 24
#else
# define PRIORITY_SHIFT 8
#endif


#define VF_INTERLEAVED	0x2	/* Indicates that voice is reading interleaved
							 * samples.  */
#define VF_CONFIGURED	0x4 /* Flag indicating that voice has been configured */
#define VF_PHASE_SLAVE	0x5 /* Flag indicating that voice is slave (odd
							 * numbered channel) in a phase-locked pair */
#define VF_PHASE_MASTER 0x6 /* Flag indicating that voice is the master
							 * (even numbered channel) in a phase-locked
							 * pair */
#define VF_SYNCABLE     0x8 /* Flag indicating voice is syncable */

/* Notification flags */
#define VF_NOTIFY_VOICE_STOLEN 0x40
#define VF_NOTIFY_ZERO_VOLUME  0x80

#define VF_PHASE_LOCKED (VF_PHASE_SLAVE|VF_PHASE_MASTER)

/*
 * @comm In order to allow us to do argument validation, we create routines
 *  for synthesizing, testing, and dereferencing SEID and SEVOICE handles.
 *  We provide a couple of different versions of these macros.  Those within
 *  the HRM_PARANOID conditional are designed to make it possible to do
 *  extensive checking of the handles for validity.  The non-paranoid versions
 *  are designed to be fast. 
 * 
 * @func stState * | FIND_SESTATE | Given a SEID, this macro returns 
 *  a pointer to the corresponding stSEState structure.  If none can be found,
 *  NULL is returned.
 *	@parm SEID | _seid | The SEID of the state to be found.
 *
 * @func SEID | MAKE_SEID | Given the index of an stSEState structure
 *  in the seStates array, this macro synthesizes an SEID and returns it.
 *  @parm DWORD | _index | The index of the stSEState whose ID is to be made.
 *
 * @func SEID | MAKE_SEID_FROM_PTR | Given an stSEState pointer,
 *  this macro synthesizes an SEID and returns it.
 * @parm stSEState * | _p | A pointer to the state whose ID is to be returned.
 *
 * @func DWORD | GET_STINDEX | Returns the index in the seStates array of the
 *  state structure with the given SEID.
 * @parm SEID | _seid | The ID of the state structure to be returned.
 *
 * @func BOOL | IS_VALID_SEID | Returns a boolean indicating whether or
 *  not the given SEID is actually valid.
 *  @parm SEID | _seid | The SEID to be tested.
 *
 * @func stSEState * | FIND_SESTATE | Returns a pointer to the stSEState
 *  structure associated with the given SEID.
 *  @parm SEID | _seid | The SEID of the state to be found.
 *
 * @func SEVOICE | MAKE_SEVOICE | Given a 
 */

#if defined(HRM_DYNAMIC)
#  define VOICE_PTR 
#else
#  define VOICE_PTR &
#endif


#define MAKE_SEID(_index)		    (SEID) ((0x5E1DL << 16) | (_index))
#define MAKE_SEID_FROM_PTR(_ptr)    MAKE_SEID(_ptr - seStates)
#define GET_STINDEX(_seid)			(_seid & 0xFFFF)

#define IS_VALID_SEID(_seid) \
	((((_seid >> 16) & 0xffff) == 0x5E1D) && \
	 (seStates[GET_STINDEX(_seid)].dwInUse == 0xBEEFFACEL) && \
	 (GET_STINDEX(_seid) < MAX_CHIPS))

#define FIND_SESTATE(_seid)	\
	(IS_VALID_SEID(_seid) ?	&(seStates[GET_STINDEX(_seid)]) : NULL)

#define MAKE_SEVOICE(_sidx, _vidx,_gen)	(((DWORD)_gen & 0xFFFFL) << 16) | (_sidx << 8) | (_vidx)
#define MAKE_SEVOICE_FROM_PTR(_p)	    MAKE_SEVOICE((_p)->state - seStates, (_p) - (_p)->state->voices, (_p)->wGeneration)
#define GET_VINDEX(_sev)				(_sev & 0xFF)
#define GET_GENERATION(_sev)			(_sev >> 16)
#define GET_SEINDEX(_sev)           ((_sev >> 8) & 0xFF)

#define IS_VALID_SEVOICE(_sev) \
	((GET_SEINDEX(_sev) < MAX_CHIPS) && \
	 (seStates[GET_SEINDEX(_sev)].dwInUse == 0xBEEFFACEL) && \
	 (GET_VINDEX(_sev) < SE_NUM_VOICES))

#define FIND_VOICE(_sev) \
	( IS_VALID_SEVOICE(_sev) ? \
		& seStates[GET_SEINDEX(_sev)].voices[_sev & 0xFF] : NULL )

#define CHECK_SESTATE(_sestate, _seid) \
	(_sestate) = FIND_SESTATE(_seid); \
	if ((_sestate) == NULL) RETURN_ERROR(SERR_BAD_HANDLE); 

#define CHECK_VOICE(_voice, _sev) \
	(_voice) = FIND_VOICE(_sev); \
	if ((_voice) == NULL) RETURN_ERROR(SERR_BAD_HANDLE); \
	if ((_voice)->wGeneration != GET_GENERATION(_sev)) \
		RETURN_ERROR(SERR_BAD_HANDLE);

/*
 * @enum enVoiceState | The voice's state provides a general indication
 *  of status; whether the voice is allocated, playing, etc.
 */
typedef enum enVoiceStateTag
{
	VS_FREE,				/* @emem Voice is currently free			*/
	VS_STOPPED,				/* @emem The voice is allocated but isn't playing */
	VS_STARTED,				/* @emem The voice is currently in either attacking,
							 *  decaying, or sustaining					*/
	VS_RELEASED,			/* @emem The voice is in the release stage.		*/
	VS_CHANGING				/* @emem The voice is in the middle of a change */
} enVoiceState;

 
struct stSEStateTag;	    /* Forward declaration, needed to resolve circular
							 * dependency.  */

/* 
 * @struct stVoice | Contains all of the information that the sound engine
 *  needs to keep for a voice.   Among other things, this structure contains
 *  some virtualized parameters which have no direct representation in the
 *  hardware.
 */
typedef struct stVoiceTag
{
    DWORD  halid;           /* @field The HAL for the SE associated with this 
                             *  voice.  This is used for register reads and
                             *  writes. */
    struct stSEStateTag *state;     /* @field A pointer to the sound engine state 
                             *  structure. */
    struct stVoiceTag *nextVoice;   /* @field A pointer for linking a list of
                             *  voices together.  The sample manager uses this
                             *  for maintaining a list of referencing voices. */
	struct stVoiceTag *prevVoice;   /* @file A pointer for linking a list of
							 *  voices together.  The sample manager uses this
							 *  for maintaining a doubly linked list of voices */
    SMHANDLE    smh;        /* @field The sound manager sample memory handle */
    DWORD       dwCallerID; /* @field The ID of the owner of this voice */
    DWORD       dwParamValues[sepLastVoiceParam];
                            /* @field  This field stores temporary values for 
                             *  various sound params. Every time a param write           
							 * occurs we update this structure.  In this way, 
                             *  if a parameter is overwritten during a 
                             *  programming step, we still have a copy of the 
                             *  user-specified value.  */
    enVoiceState vstate;    /* @field The current state of the voice */ 

    SENOTIFYCALLBACK notifyCallback;  /* @field The pointer to the callback 
                             *  function which gets invoked if the voice gets 
                             *  stolen. */
    DWORD       dwCallbackData;     /* @field Data which gets passed to the callback
                             *  function when it is invoked. */
    DWORD       dwPriority;  /* @field The current voice priority.  This is a
                             *  value between 0 and 255, with 255 being the
                             *  highest priority and 0 being the lowest */
    DWORD       dwTempPriority;  /* This is here as a temporary storage
                             *  place for the priority computed in VoiceAllocate.
                             *  We put it in here to avoid excessive stack use.
                             */

    DWORD       dwVolume;   /* @field Volume as of last read */
    WORD        wFlags;     /* @field Various flags indicating the voice's 
                             *  allocation mode */
    WORD        wGeneration;/* @field Maintains a generation count for the
                             *  voice.  Every time a voice is allocated, its
                             *  generation number gets incremented.  This is
                             *  used to ensure that a voice which was stolen 
                             *  can't be reused by the client from which it was
                             *  stolen.  */
    BYTE        byIndex;    /* @field The voice channel index */
    BYTE        byNumRefs;  /* @field Number of times this voice references the
                             *  its Sample Memory Handle. */

} stVoice;

#ifdef SE_RAMP_SENDS
/* @struct stVoiceRampData | Data used to ramp voice sends */
typedef struct stVoiceRampDataTag
{
    stVoice *pVoice;
    BYTE byCurValue[4];
    BYTE byNextIncrement;
    DWORD dwNextUpdate;
    DWORD dwMaxLookAhead;
    ITMHANDLE itmRampISRHandle;
    struct stVoiceRampDataTag *next;
} stVoiceRampData;
#endif

/* @struct stSEState | Contains all state pertaining to a particular 
 * sound engine instance.  There is one stSEState structure for each
 * sound engine in the system.
 */
typedef struct stSEStateTag
{
    DWORD     dwInUse;		/* @field Indicates that this is being used */
	DWORD     id;			/* @field The HAL ID for this sound engine */
    WORD	  wFreeVoices;	/* @field The current number of free voices */
    ITMID     itmid;        /* @field The interrupt pending manager ID */
    ITMHANDLE itmISRHandle; /* @field The handle for the interval timer callback */
    ITMHANDLE itmHandle;    /* @field The handle for the low-pri callback */
    DWORD     dwHWRevision; /* @field The ASIC revision */

#ifdef SE_RAMP_SENDS
    stVoiceRampData *voiceRampList;
    ITMHANDLE itmRampHandle;
#endif

	/* @comm We can compile the driver to use dynamic memory simply by
	 *  defining HRM_DYNAMIC.
	 */
#if CFG_DYNAMIC
    stVoice *voices;	      /* @field Array of pointers to voices */
	DWORD   dwNumPages;       /* @field Number of pages for voice table */
	OSMEMHANDLE voicesMemHdl; /* @field Mem handle for array of voice table */
#else
	stVoice voices[SE_NUM_VOICES];
#endif
} stSEState;


EMUAPIEXPORT stSEState *smpGetSEState();
EMUAPIEXPORT DWORD      smpGetSECount();

#endif  /* __SE8210P_H */

