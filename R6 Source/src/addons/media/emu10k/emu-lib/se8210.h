/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*
* @doc INTERNAL
* @module se8210.h | 
* This file contains the public data types and functions used by the EMU 8010
* sound engine. 
*
* @iex 
* Revision History:
* Version	Person	Date		Reason
* -------	------	----		---------------------------------- 
*  0.005	JK		Oct 30, 97	Changed the steal callback into a more
*								generalized notification callback scheme.
*								Added SENOTIFYEVENT as a result.
*  0.004	JK		Sep 24, 97	Changed to use the 8210 name scheme.
*  0.003	JK		Sep 23, 97	Cleaned up some of the documentation 
*								bugs.
*  0.002	JK		Sep 05, 97	Added declarations for the voice 
*								management and sound control 
*								routines.
*  0.001	MG		Jun 02, 97	Initial version
*
******************************************************************************
* @doc EXTERNAL
* @contents1 EMU SE8210 Programmer's Manual |
* This document describes the SE8210 sound engine application programming 
* interface.  The sound engine is the portion of the EMU 8010 chip responsible
* for managing the oscillator, filter, amplifier, envelope generators, and
* LFO's which comprise a single voice.  The E8010 supports 64 voices.
*
* <nl>Summary of the SE8210 Features<nl>
*	<tab>64 independent voices, each with:<nl>
*	<tab><tab>8-point pitch interpolator engine<nl>
*	<tab><tab>Independent access to the sample pool in main memory<nl>
*	<tab><tab>Independent loop start and end addresses<nl>
*	<tab><tab>Interrupt or stop upon hitting an end loop point<nl>
*	<tab><tab>Pitch phase lock a voice with a previous voice<nl>
*	<tab><tab>Ability to interpolate 8-bit and interleaved samples<nl>
*	<tab><tab>Two-pole resonant filter<nl>
*	<tab><tab>Amplifier<nl>
*	<tab><tab>Four send outputs, which can be individually connected to 4 of<nl>
*	<tab><tab><tab>the 16 FX8010 inputs<nl>
*	<tab><tab>Dedicated envelope engine, containing<nl>
*	<tab><tab><tab>A 6-segment DAHDSR envelope for volume modulation<nl>
*	<tab><tab><tab>A 6-segment DAHDSR envelope which can modulate the pitch<nl> 
*	<tab><tab><tab><tab>and filter cutoff<nl>
*	<tab><tab><tab>A low frequency oscillator (LFO) with variable delay and<nl>
*	<tab><tab><tab><tab>frequency modulating pitch<nl>
*	<tab><tab><tab>An LFO with variable delay and frequency modulating<nl>
*	<tab><tab><tab><tab>pitch, filter, and volume using independent sends<nl><nl>
*	<tab>Sample memory pool which may consist of up to 32MB of sample data<nl>
*	
*
*/

#ifndef __SE8210_H
#define __SE8210_H

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "datatype.h"   /* Data types */  
#include "aset8210.h"

/******************************************************************************
 * Defines 
 ******************************************************************************/

/* Error values */
#define SERR_FAIL					1
#define SERR_BAD_HANDLE				2
#define SERR_INIT_FAILED			3
#define SERR_BAD_PARM				4
#define SERR_READ_ONLY_REGISTER		5
#define SERR_IS_PLAYING				6
#define SERR_NOT_MASTER				7
#define SERR_NOT_CONFIGURED			8
#define SERR_BAD_SMHANDLE			9

/* seVoiceAllocate priority values */
#define SEAP_RESERVED				63
#define SEAP_HIGH_PRIORITY			62
#define SEAP_LOW_PRIORITY			0

/* seVoiceAllocate flag definitions */
#define SEAF_NORMAL					0
#define SEAF_PAIRED					1
#define SEAF_NOTIFY_VOICE_STOLEN    2
#define SEAF_NOTIFY_ZERO_VOLUME     4
#define SEAF_SYNCABLE               8

/* #define SEAF_NOTIFY_COWS_COME_HOME  8 */

/* Stereo mode flags */
#define SESTMODE_MONO				0
#define SESTMODE_STEREO				1
#define SESTMODE_INTERLEAVED		2

BEGINEMUCTYPE

/******************************************************************************
 * Enumerations
 * @section Enumerated types
 ******************************************************************************/

/* @enum SENOTIFYEVENT |
 *  SENOTIFYEVENT enumerates all possible events that a client can monitor.
 *  A client starts monitoring for an event by specifying one or more of the
 *  notification flags when a voice is initially allocated (see <f seVoiceAllocate>
 *  for more information about the event notification flags; see <t SENOTIFYCALLBACK>
 *  for a description of the notification callback function).
 */
typedef enum 
{
	seneVoiceStolen,    /* @emem The monitored voice has been stolen and is
						 *  no longer available to the client  */
	seneZeroVolume,     /* @emem The volume of the voice is zero and playback
						 *  has stopped.  */
} SENOTIFYEVENT;


/* @enum SEVOICEPARAM |
 * This enumerated type contains all of the allowable voice parameter values.
 * The enumerated values are used in the dwParamID value in the <t stseParam>
 * structure defined above, as well as in the dwParamID paramter in the 
 * <f seParameterRead> and <f seParameterWrite> functions 
 */
typedef enum VoiceParamTag
{
	sepDisableEnv,		/* @emem Disable the envelope generator */
	sepRawPitch,		/* @emem Linear pitch target */
	sepRawFilter,		/* @emem Linear filter target */
	sepRawVolume,		/* @emem Linear volume target */
	sepDelayModLFO,		/* @emem Mod LFO initial delay, 0x8000 - n gives delay 
						 *  of 725*n ms */
	sepFreqModLFO,		/* @emem Modulation LFO frequency in 0.043 Hz steps */
	sepDelayVibLFO,		/* @emem Vibrato LFO initial delay.  Setting this to 
						 *  0x8000-n produces a delay of 725*n ms */
	sepFreqVibLFO,		/* @emem Vibrato frequency in 0.043 Hz steps */
	sepDelayModEnv,		/* @emem Modulation envelope delay; to 0x8000-n 
						 *  gives delay of 725*n ms */
	sepAttackModEnv,	/* @emem Modulation envelope attack, log encoded: 
						 *   0 = infinite, 1=11.88msec, 0x7f=6ms */
	sepHoldModEnv,		/* @emem Modulation envelope hold time, 127-n produces a hold
						 *   time of n*46ms*/
	sepDecayModEnv,		/* @emem Modulation envelope decay time, log encoded: 
						 *   0=47.55sec, 1=23.78sec, 0x7f = 24ms*/
	sepSustainModEnv,	/* @emem Modulation envelope sustain level, 127=full 
						     level, 0 = off, 0.75 incrs*/
	sepReleaseModEnv,	/* @emem Mod envelope release level, log encoded: 
						 *   0=47.55sec, 1=23.78sec, 0x7f = 24ms */
	
	sepDelayVolEnv,		/* @emem Initial volume envelope delay, set 0x8000-n 
						 *   to produce delay of 725*n usec. */
	sepAttackVolEnv,	/* @emem Volume envelope attack time, log encoded: 
						 *   0 = infinite, 1 = 11.88ms, 0x7f = 6ms.*/
	sepHoldVolEnv,		/* @emem Volume envelope hold time, a value of 127-n 
						 *   produces a hold time of n*46 ms*/
	sepDecayVolEnv,		/* @emem Volume envelope decay time, log encoded: 
						 *   0=47.55sec, 1=23.78sec, 0x7f=24ms*/
	sepSustainVolEnv,	/* @emem Volume envelope sustain level, 0x7f=full level,
						 *   0=off, 0.75dB incrs*/
	sepReleaseVolEnv,	/* @emem Volume envelope release level.*/
	
	/* Oscillator */
	sepStartAddrs,		/* @emem Address of the first sample in the wave table */
	sepStartloopAddrs,	/* @emem Address of the first sample in the loop */
	sepEndloopAddrs,	/* @emem Address of the last sample of the loop */
	sepInitialPitch,	/* @emem The initial pitch of the oscillator */
	sepModLFOToPitch,	/* @emem The depth of the modulation LFO modulation, a 
					     *  two's-complement value with a range of +/- one octave*/
	sepVibLFOToPitch,	/* @emem The depth of the vibrato LFO's modulation, a 
						 *  two's-complement value with a range of +/- one octave modulation*/
	sepModEnvToPitch,	/* @emem The amount of pitch modulation from the 
						 *   modulation envelope, a two's-complement value with 
						 *   a range of +/- one octave */

	sepInitialFilterFc,	/* @emem Initial filter cutoff value in exponential units; 
						 *   the 6 most significant bits semitones, the 2 LSB's 
						 *   are fractional. */
	sepInitialFilterQ,	/* @emem Lowpass filter resonance amount. */
	sepModLFOToFilterFc,/* @emem The amount of modulation from the modulation 
						 *  LFO to the filter cutoff value; a two's-complement
						 *  value with a range of of +/- one octave. */
	sepModEnvToFilterFc,/* @emem The amount of modulation from the modulation
						 *  envelope to the filter cutoff value; a signed 
						 *  two's-complement value with extremes of +/- 
						 *  one octave. */
	
	/* Amplifier */
	sepInitialVolume,	/* @emem The initial attenuation in 0.375 dB steps */
	sepModLFOToVolume,	/* @emem The depth of the tremolo, a two's complement 
						 *  value with extremes of +/- 12 dB. */
	
	/* Effects */
	sepEffectsSendC,	/* @emem Linear level of the channel output sent to FX C */
	sepEffectsSendD,	/* @emem Linear level of the channel output sent to FX D */
	sepEffectsSendA,	/* @emem Linear level of the channel output sent to FX A */
	sepEffectsSendB,	/* @emem Linear level of the channel output sent to FX B */
	
	/* Low level Emu8000 sound engine parameters */
	sepCurrentPitch,	/* @emem The linear current pitch value; 0x4000 is unity pitch shift */
	sepFraction,		/* @emem The fractional portion of the current address */
	sepCurrentVolume,	/* @emem The current volume as a linear value. */
	sepCurrentFilter,	/* @emem The current filter cutoff frequency, encoded 
						 *     linearly. */
	sepFilterDelayMemory1,	/* @emem First filter delay memory register */
	sepFilterDelayMemory2,	/* @emem Second filter delay memory */
	
	sepLastEmu8000VoiceVariable,
	
	/* Emu8010 specific registers */
	
	/* Sound Engine */
	sepForceZeroPitch=sepLastEmu8000VoiceVariable, 
						/* @emem When set, forces the pitch to 0. */
	sepInterpolationROM,/* @emem Selects the passband of the interpolation ROM.*/
	sepEightBitSample,		/*@emem Indicates that sound memory contains 8-bit samples */
	
	/* Effects routing */
	sepRouteEffectsSendA,	/* @emem Selects one of the FX8010's 16 inputs for send A */
	sepRouteEffectsSendB,	/* @emem Selects one of the FX8010's 16 inputs for send B */
	sepRouteEffectsSendC,	/* @emem Selects one of the FX8010's 16 inputs for send C */
	sepRouteEffectsSendD,	/* @emem Selects one of the FX8010's 16 inputs for send D */

	/* Virtual stuff */
	sepSampleMemoryHandle,  /* @emem The sample memory handle for the voice.  This
							 *  handle is acquired from the sample memory manager
							 *  (see the sm8210 Programmer's Manual)  and needs to
							 *  be set before attempting to use any of the voice
							 *  playback control functions.  It cannot be changed 
							 *  while a voice is playing.  */
	sepPriority,			/* @emem The current voice priority.  This priority is
							 *  initially set when the voice is allocated, but it
							 *  can be changed at any time with seParameterWrite.
							 *  The intent is that the entity which owns a voice 
							 *  can dynamically raise or lower its priority as a
							 *  hint to the voice allocator.  */
	sepStereo,				/* @emem Puts this voice into one of the voice paired
							 *  modes.  This parameter may be set to one of 
							 *  three values: <t SESTMODE_MONO>, <t SESTMODE_STEREO>, 
							 *  and <t SESTMODE_INTERLEAVED>.  If the stereo or 
							 *  interleaved modes are used, the sound engine code 
							 *  expects that the caller will set the same value for 
							 *  this parameter in both of the paired voices.  */
	sepOneShot,				/* When set to a non-zero value, the sound will
							 *  play once from the start address (stored in
							 *  selStartAddrs) to the end address (stored in
							 *  in selEndloopAddrs) and then stop.
							 */
	sepLastVoiceParam		/* @emem This parameter shouldn't be used; it is provided
							 *  solely as a mechanism for determining the
							 *  size of the parameter arrays.  */
} SEVOICEPARAM;

/* @enum SEGLOBALPARAM |
 * This enumeration table sets up indexes into the Address
 * Table for the frequently used global variables.  It begins
 * numbering at the last value of the VoiceVariable enumeration.  These
 * values are also used with <f seParameterRead()> and <f seParameterWrite()>.  
 * These parameter can be queried through seParameterRead, but in general they
 * are managed implicitly by the sound engine.
 */
typedef enum GlobalParamTag
{
	/* Sample page table memory */
	sepPageTableBase = sepLastVoiceParam, /* @emem */
	sepPageTableAEntry,			/* @emem */
	sepPageTableAIndex,			/* @emem */
	sepPageTableBEntry,			/* @emem */
	sepPageTableBIndex,			/* @emem */
	
	sepStopOnLoopLow,			/* @emem */
	sepStopOnLoopHigh,			/* @emem */
	sepInterruptOnLoopLow,		/* @emem */
	sepInterruptOnLoopHigh,		/* @emem */
	sepInterruptPendingLow,		/* @emem */
	sepInterruptPendingHigh,	/* @emem */
		
	sepLastGlobalParam
} enseGlobalParam, SEGLOBALPARAM;


/* @enum SEVOICECONFIG | 
 *  Enumerates all of the valid possible configurations which can be passed
 *  to seVoiceConfigure.  Note: This stuff needs to be documented a whole
 *  lot more.  In particular, we need to specify exactly which parameters
 *  each configuration needs from the dwParamValues array and which are
 *  drawn from the configuration template.  For secfgCustom, however,
 *  we state that all parameters are taken from the dwParamValues in the
 *  voice setup.
 */
typedef enum enseConfigTag
{
	secfgMonoOneShot,		/* @emem Configuration for mono sounds which play
							 *	through once and then end.  */
    secfgIntlvOneShot,		/* @emem Configuration for stereo interleaved
							 *  sounds which play through once and then stop. */
	secfgStereoOneShot,		/* @emem Configuration for stereo sounds which */
	secfgCustom,			/* @emem A fully custom configuration.  All of the
							 *  parameters for the voice are provided in the
							 *  dwParamValues array. 
							 */
	secfgLastConfiguration
} enseConfig, SECONFIGVALUE;


/******************************************************************************
 * Structures
 ******************************************************************************/

/* @type SEID | An opaque handle used to reference a particular E8010
 *  sound engine.
 */
typedef DWORD SEID;

/* @type SEVOICE | An opague handle used to reference a voice.  The SEVOICE
 *  data type encapsulates all data that is relevant to a particular voice,
 *  including the sound engine with which it is associated and the current
 *  state of all of its parameters.
 */
typedef DWORD SEVOICE;

/* @cb void | SENOTIFYCALLBACK | A function pointer type for a callback.  This
 *  callback function gets invoked whenever the state of monitored voice changes.
 *  A particular instance of this type and a set of events to be monitored are 
 *  registed when a voice is originally allocated (see the description of 
 *  <f seVoiceAllocate>).  If a monitored event occurs, the registed callback
 *  function is invoked.  
 *
 * @parm DWORD | callbackData | Contains the callback data value specified when
 *  the voice was originally allocated.
 * @parm SEVOICE | voice | The voice whose state has changed.
 * @parm SENOTIFYEVENT | event | The event which has occurred. 
 */
typedef void (*SENOTIFYCALLBACK)(DWORD callbackData, SEVOICE voice, SENOTIFYEVENT event);

typedef struct stSEAttribTag
{
	BYTE blah;
} stE8010SEAttrib, SEATTRIB;


/* @struct SEPARAM | 
 *  The SEPARAM structure is used to pass parameter/value pairs to 
 *  sound engine.   Currently, it is used only by the <f seMultiParameterWrite>
 *  function.
 */
typedef struct stseParamTag
{
	DWORD dwParamID; /* @field The parameter being manipulated.  Should be set to
					  * one of the values enumerated in <t VoiceParam> enum */
	DWORD dwValue;	 /* @field the value the parameter should be set to. */
} stseParam, SEPARAM;


/* @struct SEVOICESETUP | Used by the <f seVoiceSetup> and 
 *  <f seVoiceSetupAndStart> functions, this structure contains all the 
 *  information needed to configure a voice for playback.  
 */
typedef struct stseVoiceSetupTag
{
	SEVOICE		  voice;	/* @field The voice being set up */
	DWORD		  dwParamValues[sepLastVoiceParam];	/* @field An array of all of 
							 *   the voice parameters, indexed by the parameter ID.
							 *   This array is used to provide values for
							 *   parameters which don't have default values
							 *   in the selected configuration.
							 */
	SECONFIGVALUE baseConfig;  /* @field The base configuration used by
						     *  this voice. When a voice is set up,
							 *  the template specified by the baseConfig value
							 *  is used to initialize all of the voice's 
							 *  parameters.  Many templates don't contain
							 *  a complete set of parameters, however, and
							 *  parameters which don't exist in the template
							 *  are retrieved from the dwParamValues array.
							 *  See the template descriptions for more details
							 *  on the default values for parameters.  If a
							 *  caller wants complete control over the
							 *  parameters, he or she can set this field to
							 *  'secfgCustom'.  In this case, the values for
							 *  all parameters are drawn from the dwParamValues
							 *  array.  */
} SEVOICESETUP;


/******************************************************************************
 * Functions 
 ******************************************************************************/


/* XXX This shouldn't be exported, but right now hacking discovery
 * into gd8010 is a pain.  WE DON'T WANT TO EXPORT THIS!!!
 */
EMUSTAT seInitSoundEngineVoices(SEID seid);

/******************
 * Public Methods
 *****************/

/* @func This function fills an array with the IDs of all of
 *  the discovered sound engines in the system and returns a count of the
 *  total number of sound engines.  The caller is allowed to 
 *  pass NULL for either or both of the arguments; in this case,
 *  the function will just return the total number of sound engines
 *  without attempting to dereference the array.
 *
 *  This function only returns sound engines for which the seDiscoverChip
 *  function has been previously called. 
 *
 * @parm DWORD | count | The number of SEID handles in the array.
 *  If 'count' is less than the total number of sound engines, only
 *  the first 'count' IDs will be copied into the array.  If 'count' is 0,
 *  the function will not attempt to fill the array.
 * @parm SEID * | seids | An array of 'count' SEID handles.
 *	If NULL, the routine will not attempt to fill the array with IDs.
 * 
 * @rdesc The total number of sound engines in the system.  If an error
 *  occurs, the function will return 0.
 */
EMUAPIEXPORT DWORD seGetHardwareInstances(DWORD count /* VSIZE */, 
										  SEID *seids /* IO */);


/* @func Copy the name of the specified sound engine into the given
 *  array.  At most 'count' characters are copied.  The system guarantees
 *  that a sound engine name will always be fewer than 63 characters in
 *  length, so a caller can safely allocate the space for the string on the
 *  stack.
 *
 * @parm SEID | seid | The ID of the sound engine whose name is being
 *  retrieved.
 * @parm DWORD | count | The size of the destination array, in characters.
 *  If the 'count' is less than the length of the name string, 'count' - 1 
 *  characters will be copied into the array and a terminating '\0' will be 
 *  placed in the last element of the array.
 * @parm char * | szName | A character array into which the device name will
 *  copied.
 *
 * @rdesc Returns SUCCESS if the retrieval completed successfully.  Otherwise,
 *  one of the following error values is returned:
 *		@flag SERR_BAD_HANDLE | the sound engine corresponding to seid wasn't found.  This
 *		 shouldn't happen unless an invalid seid is passed.
 *      @flag SERR_BAD_PARAM | A NULL or invalid pointer was passed in for 
 *       szName.
 */
EMUAPIEXPORT EMUSTAT seGetModuleName(SEID seid, DWORD count /* VSIZE */, 
									 char *szName /* IO */);


/* @func Fill an attribute data structure with the attributes for the
 *  specified sound engine.
 */
EMUAPIEXPORT EMUSTAT seGetModuleAttributes(SEID id, SEATTRIB *attrib /* IO */);


/* @func Returns the number of voices which are currently free
 *  on the specified SE8010.  This function is convenient for situations
 *  in which the caller would like to grab more voices but is unsure
 *  how many are actually available. 
 *
 * @parm SEID | seid | The sound engine to be queried.
 * @rdesc The number of voices free.
 */
EMUAPIEXPORT DWORD seGetFreeVoiceCount(SEID seid);


/* @func Return the total number of voices provided by the specified
 *  sound engine.
 * @parm SEID | seid | The ID of the sound engine being queried.
 * @rdesc The total number of voice channels provided by the sound engine.
 */
EMUAPIEXPORT DWORD seGetTotalVoiceCount(SEID seid);


/* @func Return the total number of reserved voices.
 * @parm SEID | seid | The ID of the sound engine being queried.
 * @rdesc The total number of voice channels provided by the sound engine.
 */
EMUAPIEXPORT DWORD seGetReservedVoiceCount(SEID seid);


/* @func Allocate one or more voices.  The sound engine's voice 
 *  allocator follows a very simple algorithm.  If a voice is
 *  free, it will be given to the caller.  If no voices are free,
 *  the allocator will examine all of the voices and steal the
 *  voice with the lowest priority less  the given priority.
 *  If there are no voices whose priority is less than the given
 *  priority, no voices will be stolen.
 *  If there are multiple voices with the same low priority,
 *  voices that are currently in a stopped state will be stolen
 *  before voices that are in the processing of releasing.  Similarly,
 *  notes that are releasing will be stolen before notes that are
 *  still playing.
 *
 *  When a voice is stolen, the "stolenCallback" that was specified
 *  when the voice was originally allocated will be invoked.  This
 *  callback is passed two parameters: the first is the 
 *  'stealCallbackData' specified when the voice was allocated, and
 *  the second is the SEVOICE handle corresponding to the stolen voice.
 *  Once a voice is stolen, the caller must discard the voice handle
 *  issued.  The caller should _not_ attempt to call any HRM function
 *  using an SEVOICE handle of a stolen voice.
 *
 * @parm SEID | seid | The ID of the sound engine from which the
 *	voices should be allocated.
 * @parm DWORD | callerID | In order to avoid stealing voices from oneself,
 *  a unique ID can be passed.  The allocator will not steal voices which
 *  were allocated using the same non-zero callerID.  If a value of 0
 *  is passed for the callerID, the allocator will ignore callerID values
 *  in its analysis and rely strictly on priorities.  
 * @parm WORD | priority | The priority with which the voices 
 *  are allocated.  The highest priority is SEAP_HIGH_PRIORITY (0x80);
 *  the lowest priority is SEAP_LOW_PRIORITY (0x0).  Voices allocated
 *  with a priority value of SEAP_RESERVED can never be stolen.
 *
 * @parm DWORD	  | flags | A set of flags controlling how the 
 *  voices are allocated.  The following flags are valid:
 *		@flag SEAF_PAIRED | Always allocate pairs of oscillators.
 *		 When this flag is specified, the allocator will attempt to
 *		 allocate even/odd oscillator pairs rather than any arbitrary
 *		 oscillator.  In this case, the even oscillator will always
 *		 returned at an even index in the array.
 *		@flag SEAF_NOTIFY_VOICE_STOLEN | When set, the registered
 *		 callback function will be called when the voice is stolen.
 *		@flag SEAF_NOTIFY_ZERO_VOLUME | When set, the registered callback
 *		 function will be called when the voice's volume drops to zero.
 * @parm DWORD | numVoices | The number of voices to allocate.
 * @parm SEVOICE * | returnedVoices | A pointer to an array of 
 *	 SEVOICE handles.  The handles of all of the voices allocated
 *   will be returned in this array.  This array must have at least
 *	 'numVoices' entries.
 * @parm SENOTIFYCALLBACK | callback | A pointer to the function
 *   that will get invoked if a monitored event occurs for one of allocated
 *   voices. 
 * @parm DWORD | callbackData | The data value which will get
 *   passed as the first argument of the notification callback function.
 *  
 * @rdesc The number of voices allocated is returned.  There is
 *  no guarantee that all of the voices requested will actually be
 *  allocated, so a caller should check this return value to determine
 *  how many voices were actually returned. 
 */
EMUAPIEXPORT DWORD seVoiceAllocate(SEID seid, DWORD callerID, DWORD priority, 
                                   DWORD flags, DWORD numVoices /* VSIZE */,
                                   SEVOICE *returnedVoices /* IO */, 
                                   SENOTIFYCALLBACK callback, 
                                   DWORD callbackData);


/* @func Deallocate one or more voices.  If a voice wasn't silenced through
 *  a previous call to <f seVoiceStop> or <f seVoiceRelease>, deallocating
 *  it will immediately silence the voice.  It is illegal to deallocate a 
 *  voice which was not allocated by the caller; doing so will lead to 
 *  extremely unpredictable results.
 *
 * @parm SEVOICE * | voices | An array of voices which are to be deallocated.
 *  No voice handle may appear more than once in the array. Each voice handle
 *  in the array is zeroed if it is successfully deallocated.
 * @parm DWORD | count | The number of voices in the array.
 *
 * @rdesc If all of the voices are successfully deallocated, the first 'count'
 *  entries of the array will be zeroed and the value SUCCESS will be returned.
 *  Otherwise, one of the following errors is returned:
 *		@flag SERR_BAD_HANDLE | One of the given SEVOICE's is invalid.
 */
EMUAPIEXPORT EMUSTAT  seVoiceFree(DWORD numVoices /* VSIZE */, SEVOICE *voices);


/* @func Returns the SEVOICE of the partner to this voice.  This
 *  function is used when the caller has allocated a stereo or
 *  interleaved voice and needs to access the other member of the
 *  pair.
 *
 * @parm SEVOICE | voice | The voice handle of one of the partners in
 *  a pair.
 * 
 * @rdesc Returns the SEVOICE handle of the partner, or 0 if the
 *  given voice has no partner or is otherwise invalid.
 */
EMUAPIEXPORT SEVOICE seVoiceGetPartner(SEVOICE voice);


/* @func Given a voice handle, returns the numeric index of the E8010 sound 
 *  engine channel which is actually mapped to that voice.  
 * 
 * @parm SEVOICE | voice | The voice handle whose channel index is desired.
 *
 * @rdesc Returns a channel number between 0 and 63 (inclusive)
 */
EMUAPIEXPORT DWORD seGetVoiceIndex(SEVOICE voice);


/* @func A diagnostic and bringup function which, when given a sound engine ID
 *  and a channel index, returns an SEVOICE for that handle.  This routine is
 *  guaranteed to return the same SEVOICE handle if called multiple times with
 *  the same sound engine ID and channelIndex arguments.  No change is made
 *  to the state of the voice, and no guarantees are made concerning the state
 *  of the voice returned.  The caller must configure the voice as desired.
 * 
 * @parm SEID | seid | The ID of the sound engine from which to get the voice
 * @parm WORD | channelIndex | the index of the sound channel
 * @parm SMHANDLE | smh | The sound manager handle for the voice's sample memory.
 *
 * @rdesc If the arguments are valid, an SEVOICE structure is returned.
 *  Otherwise, INVALID_VOICE is returned.
 */
SEVOICE seGetVoiceHandle(SEID seid, WORD channelIndex);
/* DON'T EXPORT */


/* @func Return the ID of the sound engine associated with the given voice.
 * @parm SEVOICE | voice | The voice whose sound engine ID is to be returned.
 */
EMUAPIEXPORT SEID seGetVoiceSEID(SEVOICE voice);


/* @func Returns the current hardware value of the specified voice parameter.
 * @parm SEVOICE | voice | The voice from which the parameter should be read.
 * @parm DWORD | dwParamID | The parameter whose value is to be returned.
 *
 * @rdesc Returns a DWORD containing the value of the specified parameter.
 *	This parameter is read directly from memory, rather than from the stored
 *  parameter table, and thus may not be identical to a previously programmed
 *  value.
 */
EMUAPIEXPORT DWORD seParameterRead (SEVOICE voice, DWORD dwParamID);


/* @func Sets the specified voice's parameter to the given value.  The caller
 *  is expected to know what values are valid for a particular parameter (see
 *  the <t VoiceParam> enumeration for more information about valid parameter
 *  values); setting a parameter to an illegal value will yield unpredictable
 *  results.
 *
 * @parm SEVOICE | voice | The voice to change.
 * @parm DWORD   | param | The parameter to change.
 * @parm DWORD   | value | The new value for the parameter.
 * 
 * @rdesc If the parameter is updated successfully, SUCCESS will be returned.
 *  Otherwise, one of the following will be returned.
 *	@flag SERR_BAD_HANDLE | The voice passed was invalid.  A voice is
 *	 invalid either because the actual value doesn't refer to a voice handle
 *   or because the voice handle has been stolen or freed.
 *  @flag SERR_BAD_PARM | The parameter passwd was invalid.
 */
EMUAPIEXPORT EMUSTAT  seParameterWrite(SEVOICE voice, DWORD dwParamID, DWORD value);


/* @func Sets multiple parameters for the specified specified voice.  
 *  An arbitrary number of parameter/value pairs may be passed, and
 *  the parameters may occur in any order in the parameter array.
 *  
 * @parm SEVOICE | voice | The voice to change.
 * @parm SEPARAM * | params | An array of stseParam structures.  The array
 *	must have 64 or fewer elements.  If more than 64 parameters are passed,
 *  only the first 64 will actually be used.
 * @parm DWORD | numParams | The number of parameters in the params array.  
 *  Should be less than 65.
 *
 * @rdesc Returns SUCCESS if all of the parameters are correctly written.
 *  Otherwise, one of the following errors is returned:
 *  @flag SERR_BAD_HANDLE | The voice handle wasn't valid.
 *  @flag SERR_BAD_PARM  | One or more of the parameters was invalid, or the
 *   parameter array pointer was bogus.
 */
EMUAPIEXPORT EMUSTAT seMultiParameterWrite(SEVOICE voice, 
                                           DWORD numParams /* VSIZE */,
                                           SEPARAM *params);

							   
/* @func Configures a set of voices specified in the voice setup array.
 *
 * @parm DWORD | numSetupPointers | The number of pointers in the following array.
 * @parm SEVOICESETUP ** | voiceSetups | An array of pointers to voice setup 
 *  structures.
 * @rdesc Returns SUCCESS if the voice i configured without incident.
 *  If an error occurs, one of the following will be returned:
 *		@flag SERR_BAD_HANDLE | One of the voices in the voice setup
 *		 structure was invalid.  A voice handle can be invalid for one of
 *		 two reasons: 1). It doesn't refer to an actual SE8210 voice;
 *		 2). It refers to a voice which the caller no longer owns.  A loss
 *		 of ownership can occur either because the caller explicitly freed the
 *		 voice or because another client stole the voice from the caller.
 */
EMUAPIEXPORT EMUSTAT seVoiceSetup(DWORD numSetups /* VSIZE */, 
								  SEVOICESETUP **voiceSetups);
/* PP setup3.inc setup0.inc */

/* @func Fully configures a set voices given a complete set of voice 
 *  parameters. This function is designed for those occasions when a 
 *  caller needs to set a large number of parameters  and start a set of 
 *  voices as quickly as possible.  This function is slightly faster than
 *  calling seVoiceSetup and seVoiceStart individually, since it can avoid
 *  some of the overhead involved in parameter manipulation.  However,
 *  it does impose some overhead of its own, and it does not leave a 
 *  voice in a permanently configured state.  
 *
 * @parm DWORD | numVoiceSetups | The number of SEVOICESETUP 
 *	structures in the subsequent array.
 * @parm SEVOICESETUP ** | voiceSetups | An array of pointers to SEVOICESETUP
 *  structures.  
 *
 * @rdesc Returns SUCCESS if the set of voices is started correctly.
 *  Otherwise, one of the following error values will be returned:
 *		@flag SERR_BAD_HANDLE | One of the voices in the voice setup
 *		 structure was invalid.  A voice handle can be invalid for one of
 *		 two reasons: 1). It doesn't refer to an actual SE8210 voice;
 *		 2). It refers to a voice which the caller no longer owns.  A loss
 *		 of ownership can occur either because the caller explicitly freed the
 *		 voice or because another client stole the voice from the caller.
 *      @flag SERR_BAD_SMHANDLE | The Sample Memory Handle for one of the
 *       voices is invalid.
 */
EMUAPIEXPORT EMUSTAT seVoiceSetupAndStart(DWORD numSetups /* VSIZE */, 
                                          SEVOICESETUP **voiceSetups);
/* PP setupAndStart3.inc setupAndStart0.inc */


/* @func Starts up the given voice.  A voice must first be configured
 *  before it can be started.  This configuration need only be done
 *  once however.  A voice must first be configured before it is 
 *  started.  This setup can be accomplished through calls to
 *  <f seVoiceConfigure> and <f seParameterWrite> or 
 *  <f seMultiParameterWrite>.  
 *
 * @parm DWORD    | numVoices | The number of voices in the subsequent array.
 * @parm SEVOICE  | voices | An array of SEVOICE handles.  The voices to start.
 *
 * @rdesc Returns SUCCESS if the voices are fired correctly.
 *		@flag SERR_BAD_HANDLE | One of the handles passed in the voices
 *		 array was invalid.  A voice handle can be invalid for one of
 *		 two reasons: 1). It doesn't refer to an actual SE8210 voice;
 *		 2). It refers to a voice which the caller no longer owns.  A loss
 *		 of ownership can occur either because the caller explicitly freed the
 *		 voice or because another client stole the voice from the caller.
 *      @flag SERR_BAD_SMHANDLE | The Sample Memory Handle for one of the
 *       voices is invalid.
 */
EMUAPIEXPORT EMUSTAT seVoiceStart(DWORD numVoices /* VSIZE */, SEVOICE *voices);


/* @func 
 */
EMUAPIEXPORT EMUSTAT seVoicePrepareSync(DWORD numVoices /* VSIZE */, SEVOICE *voices);


EMUAPIEXPORT EMUSTAT seVoiceStartSync(DWORD numVoices /* VSIZE */, SEVOICE *voices);


/* @func Places the envelope engine of a set of voices into release mode.
 *  Once in release mode, the voices will fade at a rate controlled by the
 *  ReleaseVolEnv parameter until the sound output level drops to 0.  At this
 *  point, if the 'doFreeVoices' parameter is TRUE, the <f seVoiceFree> 
 *  function will automatically be called for the voice.
 *
 *  Note that this routine does _not_ block until the release phase is completed;
 *  a caller is responsible for polling the voice's parameters if they need
 *  to wait until the audio level goes to zero.
 *
 * @parm DWORD	 | numVoices | The number of voices in the subsequent array.   
 * @parm SEVOICE | voices | An array of voices to release.
 *
 * @rdesc Returns SUCCESS if all of the voices are programmed successfully.
 *		@flag SERR_BAD_HANDLE | One of the voices passed in the voices
 *		 array was invalid.  A voice handle can be invalid for one of
 *		 two reasons: 1). It doesn't refer to an actual SE8210 voice;
 *		 2). It refers to a voice which the caller no longer owns.  A loss
 *		 of ownership can occur either because the caller explicitly freed the
 *		 voice or because another client stole the voice from the caller.
 */
EMUAPIEXPORT EMUSTAT seVoiceRelease(DWORD numVoices /* VSIZE */, 
									SEVOICE *voices);


/* @func Stops all voices as quickly as possible without audible artifacts.
 *  This is achieved by setting the target volume value to 0 so that the
 *  ramp generators for the voices will drop the sound out. Once the output
 *  level reaches zero, if the 'doFreeVoices' parameter is TRUE, the 
 *  <f seVoiceFree> function will be called automatically to deallocate the
 *  voice.
 *
 *  Although the amount of time needed to silence the voices will be small,
 *  this routine does _not_ block until the output levels are zero.
 *
 * @parm DWORD   | numVoices | The number of voices in the subsequent array.
 * @parm SEVOICE | voices    | An array containing the voices to stop
 *
 * @rdesc Returns SUCCESS if all of the voices are programmed successfully.
 *		@flag SERR_BAD_HANDLE | One of the voices passed in the voices
 *		 array was invalid.  A voice handle can be invalid for one of
 *		 two reasons: 1). It doesn't refer to an actual SE8210 voice;
 *		 2). It refers to a voice which the caller no longer owns.  A loss
 *		 of ownership can occur either because the caller explicitly freed the
 *		 voice or because another client stole the voice from the caller.
 */
EMUAPIEXPORT EMUSTAT seVoiceStop(DWORD numVoices /* VSIZE */, SEVOICE *voices);
ENDEMUCTYPE

#endif
