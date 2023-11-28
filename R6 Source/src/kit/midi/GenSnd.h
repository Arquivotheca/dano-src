/*****************************************************************************/
/*
** "GenSnd.h"
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
**
**	© Copyright 1993-1996 Headspace, Inc, All Rights Reserved.
**	Written by Jim Nitchals and Steve Hales
**
**	Headspace products contain certain trade secrets and confidential and
**	proprietary information of Headspace.  Use, reproduction, disclosure
**	and distribution by any means are prohibited, except pursuant to
**	a written license from Headspace. Use of copyright notice is
**	precautionary and does not imply publication or disclosure.
**
**	Restricted Rights Legend:
**	Use, duplication, or disclosure by the Government is subject to
**	restrictions as set forth in subparagraph (c)(1)(ii) of The
**	Rights in Technical Data and Computer Software clause in DFARS
**	252.227-7013 or subparagraphs (c)(1) and (2) of the Commercial
**	Computer Software--Restricted Rights at 48 CFR 52.227-19, as
**	applicable.
**
**	Confidential-- Internal use only
**
** Overview
**	The purpose of this layer of code is to remove Macintosh Specific code.
**	No file, or memory access. All functions are passed pointers to data
**	that needs to be passed into the mixer, and MIDI sequencer
**
** Modification History
**
**	4/6/93		Created
**	4/12/93		First draft ready
**	4/14/93		Added Waveform structure
**	7/7/95		Added Instrument API
**	11/7/95		Major changes, revised just about everything.
**	11/11/95	Added external queued midi links
**	11/20/95	Removed the BF_ flags, now you must walk through the union structure
**				Remove bit fields. BIT FIELDS DON'T WORK WITH MPW!!!!
**	12/5/95		Added avoidReverb into instrument field; removed drop sample case
**	12/6/95		Move REVERB_TYPE from GENPRIV.H
**				Added GM_SetReverbType; removed extern references
**				Added ReverbType to GM_Song structure
**				Removed defaultPlaybackRate & defaultInterpolationMode from the GM_Song
**				structure
**	12/7/95		Moved DEFAULT_REVERB_TYPE from GenSnd.c
**				Added GM_GetReverbType
**	1/4/96		Added GM_ChangeSampleReverb for sound effects
**	1/7/96		Changed GM_BeginDoubleBuffer to use a 32 bit value for volume
**				Added GM_SetEffectsVolume & GM_GetEffectsVolume
**	1/13/96		Added extendedFormat bit to internal instrument format
**	1/18/96		Spruced up for C++ extra error checking
**	1/19/96		Changed GM_BeginSample to support bitsize and channels
**	1/29/96		Changed WaveformInfo to support FIXED_VALUE for sample rate
**				Added useSampleRate factor for playback of instruments & sampleAndHold bits
**	2/4/96		Added songMidiTickLength to GM_Song
**	2/5/96		Moved lots of variables from the MusicVars structure into
**				the GM_Song structure.
**				Changed GM_EndSong & GM_SongTicks & GM_IsSongDone & 
**				GM_SetMasterSongTempo to pass in a GM_Song pointer
**				Added GM_GetSongTickLength
**	2/12/96		Added GM_SetSongTickPosition
**				Added songMicrosecondLength to GM_Song structure
**	2/14/96		Added GM_StartLiveSong
**	2/18/96		Added panPlacement to the GM_Instrument structure
**	2/29/96		Added trackMuted to GM_Song structure
**	3/5/96		Added MAX_SONG_VOLUME
**	4/15/96		Added support to interpret SONG resource via GM_MergeExternalSong
**	4/20/96		Added defines for max lfos, and max curves, MAX_LFOS & MAX_CURVES
**	4/21/96		Added GM_GetRealtimeAudioInformation
**	6/7/96		Increased MAX_TRACKS to 65 to include tempo track and 64 tracks
**	6/9/96		Added GM_GetUserReference & GM_SetUserReference
**	6/30/96		Changed font and retabbed
**	7/2/96		Added packing pragmas
**				Removed usage of Machine.h. Now merged into X_API.h
**	7/3/96		Added support for one level of loop save and playback
**				Required for Headspace support
**	7/5/96		Added GM_KillAllNotes
**	7/6/96		Added GM_GetSyncTimeStamp
**	7/14/96		Fixed structure alignment issue for PowerPC
**	7/23/96		Changed QGM_ functions to use an unsigned long for time stamp
**	7/25/96		Added GM_GetSyncTimeStampQuantizedAhead
**	8/15/96		Added constant for LOW_PASS_AMOUNT & LPF_DEPTH
**	8/19/96		Added GM_SetAudioTask
**	9/10/96		Added GM_NoteOn & GM_NoteOff & GM_ProgramChange & GM_PitchBend & 
**				GM_Controller & GM_AllNotesOff for direct control to bypass the queue
**	9/17/96		Added GM_LoadSongInstrument & GM_UnloadSongInstrument
**				Added Q_GET_TICK
**	9/18/96		Changed GM_SongCallbackProcPtr to pass a GM_Song structure 
**				rather than an ID
**	9/19/96		Added GM_GetSongTempo
**	9/20/96		Added GM_SetSongTempo & GM_ResumeSong & GM_PauseSong
**	9/23/96		Added GM_MuteTrack & GM_UnmuteTrack & GM_MuteChannel & GM_UnmuteChannel
**	9/24/96		Added GM_SetSongTempInBeatsPerMinute & GM_GetSongTempoInBeatsPerMinute
**				Added GM_SoloTrack & GM_SoloChannel
**				Added GM_GetSongPitchOffset & GM_SetSongPitchOffset
**	9/25/96		Added GM_GetChannelMuteStatus & GM_GetTrackMuteStatus
**				Added controller 90 to change the global reverb type
**				Added GM_EndSongNotes
**	10/8/96		Removed pascal from GM_SongCallbackProcPtr
**	10/11/96	Added GM_BeginSampleFromInfo
**	10/13/96	Changed QGM_AllNotesOff to work with the queue and post an event
**				Added GM_ReadIntoMemoryWaveFile & GM_ReadIntoMemoryAIFFFile
**	10/18/96	Made WaveformInfo smaller
**	10/23/96	Removed reference to BYTE and changed them all to UBYTE or SBYTE
**				Added defines for MOD_WHEEL_CONTROL = 'MODW & SAMPLE_NUMBER = 'SAMP'
**				Added more defines for instrument types
**	10/28/96	Modified QGM_NoteOn & QGM_NoteOff & QGM_ProgramChange & 
**				QGM_PitchBend & QGM_Controller & QGM_AllNotesOff to accept a GM_Song *
**	10/31/96	Changed trackMuted and channelMuted to be bit flag based
**				Added soloMuted bit array for solo control
**	10/31/96	Added GM_IsInstrumentLoaded
**	11/3/96		Added midiNote to GM_AudioInfo structure
**	11/5/96		Changed WaveformInfo to GM_Waveform
**				Added GM_ReadFileInformation and GM_FreeWaveform
**	11/6/96		Added GM_UnsoloTrack, GM_GetTrackSoloStatus
**	11/7/96		Added GM_SongTimeCallbackProcPtr & GM_SetSongTimeCallback
**	11/8/96		Added GM_GetSampleVolume & GM_GetSamplePitch & GM_GetSampleStereoPosition
**	11/9/96		Added GM_KillSongNotes
**	11/11/96	Added more error codes and removed extra ones
**	11/14/96	Added pSong reference in GM_GetRealtimeAudioInformation
**	11/19/96	Changed MAX_CHANNELS to 17, 16 for Midi, 1 for sfx
**				Added a GM_Song structure to GM_ConvertPatchBank and removed bank
**	11/21/96	Removed GM_ConvertPatchBank
**	11/26/96	Changed MAX_BANKS to 6
**				Added GM_GetControllerValue
**	12/2/96		Added MOD file code API
**	12/9/96		Added GM_LoadInstrumentFromExternal
**	12/15/96	Added controls for DEFAULT_VELOCITY_CURVE
**	12/19/96	Added Sparc pragmas
**	1/2/97		Moved USE_MOD_API and USE_STREAM_API into X_API.h
**	1/12/97		Changed maxNormalizedVoices to mixLevel
**	1/16/97		Changed LFORecord to LFORecords
**	1/22/97		Added GM_SetSampleDoneCallback
**	1/23/97		Added M_STEREO_FILTER
**				Added GM_SetAudioOutput
**	1/24/97		Added GM_SetSongFadeRate
**				Added GM_SetModLoop & GM_GetModTempoBPM & GM_SetModTempoBPM
**				Changed disposeMidiDataWhenDone to disposeSongDataWhenDone
**				Added GM_SetAudioStreamFadeRate
**	1/28/97		Added more parmeters to GM_SetSongFadeRate to support async
**				ending of song
**	1/28/97		Eliminated terminateDecay flag. Not used anymore
**	1/30/97		Changed SYMPHONY_SIZE to MAX_VOICES
**	2/1/97		Added support for pitch offset control on a per channel basis
**				Added GM_DoesChannelAllowPitchOffset & GM_AllowChannelPitchOffset
**	2/2/97		Tightened up GM_Song and GM_Instrument a bit.		
*/
/*****************************************************************************/

#ifndef G_SOUND
#define G_SOUND

#include <BeBuild.h>

#ifndef __X_API__
	#include "X_API.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

/* System defines */

/* Used in InitGeneralSound */

// Quality types
enum
{
	Q_11K = 0,
	Q_22K,
	Q_44K
};
typedef long Quality;

// Modifier types
#define M_NONE				0L
#define M_USE_16			(1<<0L)
#define M_USE_STEREO		(1<<1L)
#define M_DISABLE_REVERB	(1<<2L)
#define M_STEREO_FILTER		(1<<3L)
typedef long AudioModifiers;

// Interpolation types
enum
{
	E_AMP_SCALED_DROP_SAMPLE = 0,
	E_2_POINT_INTERPOLATION,
	E_LINEAR_INTERPOLATION
};
typedef long TerpMode;

enum 
{
	REVERB_TYPE_1 = 1,		// None
	REVERB_TYPE_2,			// Igor's Closet
	REVERB_TYPE_3,			// Igor's Garage
	REVERB_TYPE_4,			// Igor's Acoustic Lab
	REVERB_TYPE_5,			// Igor's Cavern
	REVERB_TYPE_6,			// Igor's Dungeon
	REVERB_TYPE_7			// WebTV Reverb
};
typedef char ReverbMode;
#define DEFAULT_REVERB_TYPE	REVERB_TYPE_4
#define MAX_REVERB_TYPES	6

enum 
{
	SCAN_NORMAL = 0,		// normal Midi scan
	SCAN_SAVE_PATCHES,		// save patches during Midi scan. Fast
	SCAN_DETERMINE_LENGTH	// calculate tick length. Slow
};
typedef char ScanMode;

enum
{
	VELOCITY_CURVE_1 = 0,	// default S curve
	VELOCITY_CURVE_2,		// more peaky S curve
	VELOCITY_CURVE_3,		// inward curve centered around 95 (used for WebTV)
	VELOCITY_CURVE_4,		// two time exponential
	VELOCITY_CURVE_5		// two times linear
};
#define DEFAULT_VELOCITY_CURVE	VELOCITY_CURVE_1

#define MAX_VOICES			32		// max voices at once
#define MAX_INSTRUMENTS		128		// MIDI number of programs per patch bank
#define MAX_BANKS			6		// three GM banks; three user banks
#define MAX_TRACKS			65		// max MIDI file tracks to process (64 + tempo track)
#define MAX_CHANNELS		17		// max MIDI channels + one extra for sound effects
#define MAX_CONTROLLERS		128		// max MIDI controllers
#define MAX_SONG_VOLUME		127
#define MAX_NOTE_VOLUME		127		// max note volume
#define MAX_CURVES			4		// max curve entries in instruments
#define MAX_LFOS			6		// max LFO's, make sure to add one extra for MOD wheel support
#define MAX_MASTER_VOLUME	256		// max volume level for master volume level

/* Common errors returned from the system */
typedef enum
{
	NO_ERR = 0,
	PARAM_ERR,
	MEMORY_ERR,
	BAD_SAMPLE,
	BAD_INSTRUMENT,
	BAD_MIDI_DATA,
	ALREADY_PAUSED,
	ALREADY_RESUMED,
	NO_SONG_PLAYING,
	TOO_MANY_SONGS_PLAYING,
	BAD_FILE
#if USE_STREAM_API
	,
// stream API errors
	NO_FREE_VOICES = 1000,
	STREAM_STOP_PLAY,
	BAD_FILE_TYPE,
	GENERAL_BAD,
	NOT_REENTERANT
#endif
} OPErr;

typedef BOOL_FLAG	(*GM_LoopDoneCallbackPtr)(INT32 refnum);
typedef void		(*GM_DoubleBufferCallbackPtr)(INT32 refnum, G_PTR pWhichBufferFinished, INT32 *pBufferSize);
typedef void		(*GM_SoundDoneCallbackPtr)(INT32 refnum);
typedef void		(*GM_ControlerCallbackPtr)(short int channel, short int controler, short int value);
typedef	void		(*GM_SongCallbackProcPtr)(struct GM_Song *pSong);
typedef	void		(*GM_SongTimeCallbackProcPtr)(struct GM_Song *pSong, UINT32 currentMicroseconds, UINT32 currentMidiClock);
typedef void		(*GM_AudioTaskCallbackPtr)(long ticks);
typedef void		(*GM_AudioOutputCallbackPtr)(void *samples, long sampleSize, long channels, unsigned long lengthInFrames);


#if __MWERKS__
	#if (CPU_TYPE == kRISC) || (CODE_TYPE == BEBOX)
		#pragma options align=power
	#elif ((CPU_TYPE == k80X86) || (CPU_TYPE == kSPARC))
		#pragma pack (8)
	#endif
#endif // __MWERKS__

struct KeymapSplit
{
	UBYTE					lowMidi;
	UBYTE					highMidi;
	INT16					smodParameter1;
	INT16					smodParameter2;
	struct GM_Instrument	*pSplitInstrument;
};
typedef struct KeymapSplit KeymapSplit;

// Flags for embedded midi objects
#define EM_FLUSH_ID				'FLUS'		// immediate command
#define EM_CACHE_ID				'CACH'		// immediate command
#define EM_DATA_ID				'DATA'		// block resources

// Flags for modules
#define ADSR_ENVELOPE			'ADSR'
#define LOW_PASS_FILTER			'LPGF'
#define LOW_PASS_AMOUNT			'LPAM'
#define DEFAULT_MOD				'DMOD'
#define EXPONENTIAL_CURVE	 	'CURV'


// Flags for ADSR module
#define ADSR_LINEAR_RAMP 		'LINE'
#define ADSR_SUSTAIN 			'SUST'
#define ADSR_TERMINATE			'LAST'
#define ADSR_GOTO				'GOTO'
#define ADSR_GOTO_CONDITIONAL	'GOST'
#define ADSR_RELEASE			'RELS'
#define ADSR_STAGES				8

struct ADSRRecord
{
	INT32			currentTime;
	INT32			currentPosition;
	INT32			currentLevel;
	INT32			mode;
	INT32			previousTarget;
	INT32			sustainingDecayLevel;
	INT32			ADSRLevel[ADSR_STAGES];
	INT32			ADSRTime[ADSR_STAGES];
	INT32			ADSRFlags[ADSR_STAGES];
};
typedef struct ADSRRecord ADSRRecord;

// kinds of LFO modules
#define VOLUME_LFO				'VOLU'
#define PITCH_LFO				'PITC'
#define STEREO_PAN_LFO			'SPAN'
#define STEREO_PAN_NAME2		'PAN '
#define LPF_FREQUENCY			'LPFR'
#define LPF_DEPTH				'LPRE'
#define LOW_PASS_AMOUNT			'LPAM'

// kinds of LFO wave shapes
#define SINE_WAVE 				'SINE'
#define TRIANGLE_WAVE 			'TRIA'
#define SQUARE_WAVE 			'SQUA'
#define SQUARE_WAVE2 			'SQU2'
#define SAWTOOTH_WAVE 			'SAWT'
#define SAWTOOTH_WAVE2 			'SAW2'

// Additional elements used by Curve functions
#define LOWPASS_AMOUNT			'LPAM'
#define VOLUME_LFO_FREQUENCY 	'VOLF'
#define PITCH_LFO_FREQUENCY 	'PITF'
#define NOTE_VOLUME				'NVOL'
#define VOLUME_ATTACK_TIME 		'ATIM'
#define VOLUME_ATTACK_LEVEL 	'ALEV'
#define SUSTAIN_RELEASE_TIME 	'SUST'
#define SUSTAIN_LEVEL			'SLEV'
#define RELEASE_TIME			'RELS'
#define WAVEFORM_OFFSET			'WAVE'
#define SAMPLE_NUMBER			'SAMP'
#define MOD_WHEEL_CONTROL		'MODW'

// Controls for registered parameter status
enum
{
	USE_NO_RP = 0,		// no registered parameters selected
	USE_NRPN,			// use non registered parameters
	USE_RPN				// use registered parameters
};

// Controls for channelBankMode
enum
{
	USE_GM_DEFAULT = 0,			// this is default behavior
								// normal bank for channels 1-9 and 11-16
								// percussion for channel 10
	USE_NON_GM_PERC_BANK,		// will force the use of the percussion
								// bank reguardless of channel and allow program
								// changes to reflect the percussion instrments and
								// allow you to change them with normal notes
	USE_GM_PERC_BANK,			// will force the use of the percussion
								// bank reguardless of channel and act just like a GM
								// percussion channel. ie. midi note represents the instrument
								// to play
	USE_NORM_BANK				// will force the use of the normal
								// bank reguardless of channel
	
};

struct LFORecord
{
	INT32	period;
	INT32	level;
	INT32	mode;
	INT32	currentTime, LFOcurrentTime;
	INT32	currentWaveValue;
	INT32	where_to_feed;
	INT32	DC_feed;
	INT32	waveShape;
	ADSRRecord a;
};
typedef struct LFORecord LFORecord;

struct CurveEntry
{
	INT16	from_Value;
	INT16	to_Scalar;
};
typedef struct CurveEntry CurveEntry;
struct CurveRecord
{
	INT32		tieFrom;
	INT32		tieTo;
	INT16		curveCount;
	CurveEntry	curveData[MAX_CURVES];	// set larger if need be.
};
typedef struct CurveRecord CurveRecord;

struct KeymapSplitInfo
{
	UINT16 		defaultInstrumentID;
	UINT16		KeymapSplitCount;
	KeymapSplit	keySplits[1];
};
typedef struct KeymapSplitInfo KeymapSplitInfo;

struct GM_Waveform
{
	INT32		waveformID;			// extra specific data, or position for files
	UBYTE		bitSize;			// number of bits per sample
	UBYTE		channels;			// number of channels (1 or 2)
	UBYTE		baseMidiPitch;		// base Midi pitch of recorded sample ie. 60 is middle 'C'
	UBYTE		noteDecayPref;		// Note Decay in 1/60th of release
	INT32		waveSize;			// total waveform size in bytes
	INT32		waveFrames;			// number of frames
	INT32		startLoop;			// start loop point offset
	INT32		endLoop;			// end loop point offset
	FIXED_VALUE	sampledRate;		// FIXED_VALUE 16.16 value for recording
	SBYTE 		*theWaveform;		// array data that morphs into what ever you need
};
typedef struct GM_Waveform GM_Waveform;


// Internal Instrument structure
struct GM_Instrument
{
	INT16				masterRootKey;
	INT16				smodResourceID;
	INT16				smodParameter1;
	INT16				smodParameter2;
	INT16				panPlacement;			// inital stereo pan placement of this instrument
	BOOL_FLAG	/*0*/	enableInterpolate;		// enable interpolation of instrument
	BOOL_FLAG	/*1*/	disableSndLooping;		// Disable waveform looping
	BOOL_FLAG	/*2*/	neverInterpolate;		// seems redundant: Never interpolate
	BOOL_FLAG	/*3*/	playAtSampledFreq;		// Play instrument at sampledRate only
	BOOL_FLAG	/*4*/	doKeymapSplit;			// If TRUE, then this instrument is a keysplit defination
	BOOL_FLAG	/*5*/	notPolyphonic;			// if FALSE, then instrument is a mono instrument
	BOOL_FLAG	/*6*/	enablePitchRandomness;	// if TRUE, then enable pitch variance feature
	BOOL_FLAG	/*7*/	avoidReverb;			// if TRUE, this instrument is not mixed into reverb unit
	BOOL_FLAG	/*0*/	enableSoundModifier;
	BOOL_FLAG	/*1*/	extendedFormat;			// extended format instrument
	BOOL_FLAG	/*2*/	sampleAndHold;
	BOOL_FLAG	/*3*/	useSampleRate;			// factor in sample rate into pitch calculation
	BOOL_FLAG	/*4*/	reserved_1;
	BOOL_FLAG	/*5*/	reserved_2;
	BOOL_FLAG	/*6*/	reserved_3;
	UBYTE				usageReferenceCount;	// number of references this instrument is associated to
	ADSRRecord			volumeADSRRecord;
	INT32				LPF_frequency;
	INT32				LPF_resonance;
	INT32				LPF_lowpassAmount;
	UBYTE				LFORecordCount;
	UBYTE				curveRecordCount;
	LFORecord			LFORecords[MAX_LFOS];
	CurveRecord			curve[MAX_CURVES];
	union
	{
		KeymapSplitInfo	k;
		GM_Waveform		w;
	} u;
};
typedef struct GM_Instrument GM_Instrument;

// Internal Song structure
struct GM_Song
{
	INT16				songID;
	INT16				maxSongVoices;
	INT16				mixLevel;
	INT16				maxEffectVoices;
	FIXED_VALUE			MasterTempo;			// master midi tempo (fixed point)
	UINT16				songTempo;				// tempo (16667 = 1.0)

	INT16				songPitchShift;			// master pitch shift
	UINT16				allowPitchShift[(MAX_CHANNELS / 16) + 1];		// allow pitch shift

	long				userReference;			// not used by anything in the synth

	GM_SongCallbackProcPtr		songEndCallbackPtr;
	long						songEndCallbackRefernce1;
	long						songEndCallbackRefernce2;

	GM_SongTimeCallbackProcPtr	songTimeCallbackPtr;
	long						songTimeCallbackRefernce;

	ReverbMode			defaultReverbType;

	UBYTE				velocityCurveType;			// which curve to use. (Range is 0 to 2)

	ScanMode	/* 0 */	AnalyzeMode;				// analyze mode (Byte)
	BOOL_FLAG	/* 1 */	ignoreBadPatches;			// allow bad patches. Don't fail because it can't load

	BOOL_FLAG	/* 0 */	allowProgramChanges;
	BOOL_FLAG	/* 1 */	loopSong;					// loop song when done
	BOOL_FLAG	/* 2 */	disableClickRemoval;
	BOOL_FLAG	/* 3 */	enablePitchRandomness;
	BOOL_FLAG	/* 4 */	disposeSongDataWhenDone;	// if TRUE, then free midi data
	BOOL_FLAG	/* 5 */	SomeTrackIsAlive;			// song still alive
	BOOL_FLAG	/* 6 */	songFinished;				// TRUE at start of song, FALSE and end

	INT16				songVolume;

	FIXED_VALUE			songFadeRate;				// when non-zero fading is enabled
	FIXED_VALUE			songFixedVolume;			// inital volume level that will be changed by songFadeRate
	INT16				songFadeMaxVolume;			// max volume
	INT16				songFadeMinVolume;			// min volume
	BOOL_FLAG			songEndAtFade;				// when true, stop song at end of fade

	INT16				defaultPercusionProgram;	// default percussion program for percussion channel. 
													// -1 means GM style bank select, -2 means allow program changes on percussion

	INT16				songLoopCount;				// current loop counter. Starts at 0
	INT16				songMaxLoopCount;			// when songLoopCount reaches songMaxLoopCount it will be set to 0

	UINT32				songMidiTickLength;			// song midi tick length. -1 not calculated yet.
	UINT32				songMicrosecondLength;		// song microsecond length. -1 not calculated yet.

	void				*midiData;					// pointer to midi data for this song

	//	instrument array. These are the instruments that are used by just this song
	GM_Instrument		*instrumentData[MAX_INSTRUMENTS*MAX_BANKS];

	INT16				instrumentRemap[(MAX_INSTRUMENTS*MAX_BANKS)];

	SBYTE				firstChannelBank[MAX_CHANNELS];		// set during preprocess. this is the program
	INT16				firstChannelProgram[MAX_CHANNELS];	// to be set at the start of a song

// channel based controler values
	SBYTE				channelWhichParameter[MAX_CHANNELS];			// 0 for none, 1 for RPN, 2 for NRPN
	SBYTE				channelRegisteredParameterLSB[MAX_CHANNELS];	// Registered Parameter least signifcant byte
	SBYTE				channelRegisteredParameterMSB[MAX_CHANNELS];	// Registered Parameter most signifcant byte
	SBYTE				channelNonRegisteredParameterLSB[MAX_CHANNELS];	// Non-Registered Parameter least signifcant byte
	SBYTE				channelNonRegisteredParameterMSB[MAX_CHANNELS];	// Non-Registered Parameter most signifcant byte
	UBYTE				channelBankMode[MAX_CHANNELS];					// channel bank mode
	UBYTE				channelSustain[MAX_CHANNELS];					// sustain pedal on/off
	UBYTE				channelVolume[MAX_CHANNELS];					// current channel volume
	UBYTE				channelExpression[MAX_CHANNELS];				// current channel expression
	UBYTE				channelPitchBendRange[MAX_CHANNELS];			// current bend range in half steps
	UBYTE				channelReverb[MAX_CHANNELS];					// current channel reverb
	UBYTE				channelModWheel[MAX_CHANNELS];					// Mod wheel (primarily affects pitch bend)
	INT16				channelBend[MAX_CHANNELS];						// MUST BE AN INT16!! current amount to bend new notes
	INT16				channelProgram[MAX_CHANNELS];					// current channel program
	SBYTE				channelBank[MAX_CHANNELS];						// current bank
	INT16				channelStereoPosition[MAX_CHANNELS];			// current channel stereo position

// mute controls for tracks, channels, and solos
// NOTE: Do not access these directly. Use XSetBit & XClearBit & XTestBit
	UINT32				trackMuted[(MAX_TRACKS / 32) + 1];			// track mute control bits 
	UINT32				soloTrackMuted[(MAX_TRACKS / 32) + 1];		// solo track mute control bits 
	UINT16				channelMuted[(MAX_CHANNELS / 16) + 1];		// current channel muted status
	UINT16				soloChannelMuted[(MAX_CHANNELS / 16) + 1];	// current channel muted status

// internal timing variables for sequencer
	INT32				MicroJif;
	INT32				UnscaledMIDITempo;
	INT32				MIDITempo;
	INT32				MIDIDivision;
	INT32				UnscaledMIDIDivision;
	UINT32				CurrentMidiClock;

	UINT32				songMicrosecondIncrement;
	UINT32				songMicroseconds;

	BOOL_FLAG			songPaused;

// storage for loop playback
	BOOL_FLAG			loopbackSaved;
	UBYTE				*pTrackPositionSave[MAX_TRACKS];
	INT32				trackTicksSave[MAX_TRACKS];
	INT32				currentMidiClockSave;
	INT32				songMicrosecondsSave;
	SBYTE				loopbackCount;

// internal position variables for sequencer. Set after inital preprocess
	UBYTE				trackon[MAX_TRACKS];			// track playing? 'F' is free, 'R' is playing
	UINT32				tracklen[MAX_TRACKS];			// length of track in bytes
	UBYTE				*ptrack[MAX_TRACKS];			// current position in track
	UBYTE				*trackstart[MAX_TRACKS];		// start of track
	INT32				trackticks[MAX_TRACKS];			// current position of track in ticks
	UBYTE				runningStatus[MAX_TRACKS];		// midi running status
};
typedef struct GM_Song GM_Song;

#if __MWERKS__
	#if (CPU_TYPE == kRISC) || (CODE_TYPE == BEBOX)
		#pragma options align=reset
	#elif ((CPU_TYPE == k80X86) || (CPU_TYPE == kSPARC))
		#pragma pack ()
	#endif
#endif // __MWERKS__

// Functions


/**************************************************/
/*
** FUNCTION InitGeneralSound(Quality theQuality, 
**								TerpMode theTerp, INT16 maxVoices,
**								INT16 normVoices, INT16 maxEffects,
**								INT16 maxChunkSize)
**
** Overvue --
**	This will setup the sound system, allocate memory, and such,
**  so that any calls to play effects or songs can happen right
**  away.
**
** Private notes:
**  This code will preinitialize the MIDI sequencer, allocate
**  amplitude scaling buffer & init it, and init the
**  General Magic sound system.
**
**	INPUT	--	theQuality
**					Q_11K	Sound ouput is FIXED_VALUE at 11127
**					Q_22K	Sound output is FIXED_VALUE at 22254
**			--	theTerp
**					Interpolation type
**			--	use16bit
**					If true, then hardware will be setup for 16 bit output
**			--	maxVoices
**					maximum voices
**			--	normVoices
**					number of voices normally. ie a gain
**			--	maxEffects
**					number of voices to be used as effects
**
**	OUTPUT	--	
**
** NOTE:	
**	Only call this once.
*/
/**************************************************/
OPErr GM_InitGeneralSound(Quality theQuality, TerpMode theTerp, AudioModifiers theMods,
				INT16 maxVoices, INT16 normVoices, INT16 maxEffects);

/**************************************************/
/*
** FUNCTION FinisGeneralSound;
**
** Overvue --
**	This will release any memory allocated by InitGeneralSound, clean up.
**
**	INPUT	--	
**	OUTPUT	--	
**
** NOTE:	
**	Only call this once.
*/
/**************************************************/
void GM_FinisGeneralSound(void);

OPErr GM_ChangeSystemVoices(INT16 maxVoices, INT16 normVoices, INT16 maxEffects);

OPErr GM_ChangeAudioModes(Quality theQuality, TerpMode theTerp, AudioModifiers theMods);

/**************************************************/
/*
** FUNCTION PauseGeneralSound;
**
** Overvue --
**	This is used to pause the system, and release hardware for other tasks to
**	play around. Call ResumeGeneralSound when ready to resume working with
**	the system.
**
**	INPUT	--	
**	OUTPUT	--	OPErr	Errors in pausing system.
**
** NOTE:	
*/
/**************************************************/
OPErr GM_PauseGeneralSound(void);

// Pause all songs
void GM_PauseSequencer(void);
// resume all songs
void GM_ResumeSequencer(void);

// Pause just this song
void GM_PauseSong(GM_Song *pSong);
// Resume just this song
void GM_ResumeSong(GM_Song *pSong);

char GM_GetControllerValue(GM_Song *pSong, INT16 channel, INT16 controller);

/**************************************************/
/*
** FUNCTION ResumeGeneralSound;
**
** Overvue --
**	This is used to resume the system, and take over the sound hardware. Call this
**	after calling PauseGeneralSound.
**
**	INPUT	--	
**	OUTPUT	--	OPErr	Errors in resuming. Continue to call until errors have
**						stops, or alert user that something is still holding
**						the sound hardware.
**
** NOTE:	
*/
/**************************************************/
OPErr GM_ResumeGeneralSound(void);

/**************************************************/
/*
** FUNCTION BeginSong(GM_Song *theSong, GM_SongCallbackProcPtr theCallbackProc);
**
** Overvue --
**	This will start a song, given the data structure that contains the midi data,
**	instrument data, and specifics about playback of this song.
**
**	INPUT	--	theSong,			contains pointers to the song data, midi data,
**								all instruments used in this song.
**				theCallbackProc,	when the song is done, even in the looped
**								case, this procedure will be called.
**	OUTPUT	--	
**
** NOTE:	
*/
/**************************************************/
OPErr GM_BeginSong(GM_Song *theSong, GM_SongCallbackProcPtr theCallbackProc);

GM_Song * GM_LoadSong(short int songID, void *theExternalSong, void *theExternalMidiData, long midiSize, 
					short int *pInstrumentArray, short int loadInstruments, OPErr *pErr);
GM_Song * GM_CreateLiveSong(short int songID);
OPErr GM_StartLiveSong(GM_Song *pSong, BOOL_FLAG loadPatches);

// return note offset in semi tones	(12 is down an octave, -12 is up an octave)
long GM_GetSongPitchOffset(GM_Song *pSong);
// set note offset in semi tones	(12 is down an octave, -12 is up an octave)
void GM_SetSongPitchOffset(GM_Song *pSong, long offset);
// If allowPitch is FALSE, then "GM_SetSongPitchOffset" will have no effect on passed 
// channel (0 to 15)
void GM_AllowChannelPitchOffset(GM_Song *pSong, unsigned short int channel, BOOL_FLAG allowPitch);
// Return if the passed channel will allow pitch offset
BOOL_FLAG GM_DoesChannelAllowPitchOffset(GM_Song *pSong, unsigned short int channel);

/**************************************************/
/*
** FUNCTION EndSong;
**
** Overvue --
**	This will end the current song playing.
**  All song resources not be will be disposed or released.
**
**	INPUT	--	
**	OUTPUT	--	
**
** NOTE:	
*/
/*************************************************/
void GM_EndSong(GM_Song *pSong);

void GM_FreeSong(GM_Song *theSong);

void GM_MergeExternalSong(void *theExternalSong, long theSongID, GM_Song *theSong);

/**************************************************/
/*
** FUNCTION SongTicks;
**
** Overvue --
**	This will return in 1/60th of a second, the count since the start of the song
**	currently playing.
**
**	INPUT	--	
**	OUTPUT	--	INT32,		returns ticks since BeginSong.
**						returns 0 if no song is playing.
**
** NOTE:	
*/
/**************************************************/
INT32 GM_SongTicks(GM_Song *pSong);

INT32 GM_SongMicroseconds(GM_Song *pSong);

// Return the length in MIDI ticks of the song passed
//	pSong	GM_Song structure. Data will be cloned for this function.
//	pErr		OPErr error type
INT32 GM_GetSongTickLength(GM_Song *pSong, OPErr *pErr);

OPErr GM_SetSongTickPosition(GM_Song *pSong, INT32 songTickPosition);

// Get current audio time stamp based upon the audio built interrupt
UINT32 GM_GetSyncTimeStamp(void);

// Get current audio time stamp based upon the audio built interrupt, but ahead in time 
// and quantized for the particular OS
UINT32 GM_GetSyncTimeStampQuantizedAhead(void);

// Return the used patch array of instruments used in the song passed.
//	theExternalSong	standard SONG resource structure
//	theExternalMidiData	if not NULL, then will use this midi data rather than what is found in external SONG resource
//	midiSize			size of midi data if theExternalMidiData is not NULL
//	pInstrumentArray	array, if not NULL will be filled with the instruments that need to be loaded.
//	pErr				pointer to an OPErr
INT32 GM_GetUsedPatchlist(void *theExternalSong, void *theExternalMidiData, long midiSize, 
					short int *pInstrumentArray, OPErr *pErr);

/**************************************************/
/*
** FUNCTION IsSongDone;
**
** Overvue --
**	This will return a BOOL_FLAG if a song is done playing or not.
**
**	INPUT	--	
**	OUTPUT	--	BOOL_FLAG,	returns TRUE if song is done playing,
**							or FALSE if not playing.
**
** NOTE:	
*/
/**************************************************/
BOOL_FLAG GM_IsSongDone(GM_Song *pSong);

// Mute and unmute tracks (0 to 64)
void GM_MuteTrack(GM_Song *pSong, short int track);
void GM_UnmuteTrack(GM_Song *pSong, short int track);
// Get mute status of all tracks. pStatus should be an array of 65 bytes
void GM_GetTrackMuteStatus(GM_Song *pSong, char *pStatus);

void GM_SoloTrack(GM_Song *pSong, short int track);
void GM_UnsoloTrack(GM_Song *pSong, short int track);
// will write only MAX_TRACKS bytes for MAX_TRACKS Midi tracks
void GM_GetTrackSoloStatus(GM_Song *pSong, char *pStatus);

// Mute and unmute channels (0 to 15)
void GM_MuteChannel(GM_Song *pSong, short int channel);
void GM_UnmuteChannel(GM_Song *pSong, short int channel);
// Get mute status of all channels. pStatus should be an array of 16 bytes
void GM_GetChannelMuteStatus(GM_Song *pSong, char *pStatus);

void GM_SoloChannel(GM_Song *pSong, short int channel);
void GM_UnsoloChannel(GM_Song *pSong, short int channel);
void GM_GetChannelSoloStatus(GM_Song *pSong, char *pStatus);


/**************************************************/
/*
** FUNCTION BeginSample(G_PTR theData, INT32 theSize, INT32 theRate, 
**						INT32 playTime,
**						INT32 theStartLoop, INT32 theEndLoop, 
**						BOOL_FLAG (*theLoopContinueProc)(void), 
**						void *theCallbackProc);
**
** Overvue --
**	This will play a sampled sound effect mixed into the current song being played.
**
**	INPUT	--	theData,		8 bit unsigned data
**				theSize,		length of sampled sound
**				theRate,		FIXED_VALUE value for sample playback, in 16.16 format
**				theStartLoop,	the starting sample of the loop point
**				theEndLoop,	the ending sample of the loop point
**				theLoopContinueProc,
**							if not NULL, then the sample will continualy play
**							the sample until this function returns FALSE, then
**							proceed to play the rest of the sample
**				theCallBackProc,
**							when the sample is finished being played, this
**							procedure will be called
**
**	OUTPUT	--	ref,		a sound reference number that is unique to 
**							every sample that is currently being played
**
** NOTE:	
*/
/**************************************************/
INT32 GM_BeginSample(G_PTR theData, INT32 theSize, INT32 theRate, 
						INT32 theStartLoop, INT32 theEndLoop, 
						INT32 sampleVolume, INT32 stereoPosition,
						INT32 refData, INT16 bitSize, INT16 channels, 
						GM_LoopDoneCallbackPtr theLoopContinueProc,
						GM_SoundDoneCallbackPtr theCallbackProc);

INT32 GM_BeginDoubleBuffer(G_PTR pBuffer1, G_PTR pBuffer2, INT32 size, INT32 rate,
							INT16 bitSize, INT16 channels,
							INT32 sampleVolume, INT16 stereoPosition,
							INT32 refData,
							GM_DoubleBufferCallbackPtr bufferCallback);

INT32 GM_BeginSampleFromInfo(GM_Waveform *pSample, INT32 refData,
								INT32 sampleVolume, INT32 stereoPosition,
								GM_LoopDoneCallbackPtr theLoopContinueProc,
								GM_SoundDoneCallbackPtr theCallbackProc);

void GM_SetSampleDoneCallback(INT32 reference, GM_SoundDoneCallbackPtr theCallbackProc, INT32 userRef);

/**************************************************/
/*
** FUNCTION EndSample;
**
** Overvue --
**	This will end the sample by the reference number that is passed.
**
**	INPUT	--	theRef,	a reference number that was returned
**						by BeginSound
**	OUTPUT	--	
**
** NOTE:	
*/
/**************************************************/
void GM_EndSample(INT32 theRef);

/**************************************************/
/*
** FUNCTION IsSoundDone;
**
** Overvue --
**	This will return status of a sound that is being played.
**
**	INPUT	--	theRef,		a reference number that was returned
**							by BeginSound
**	OUTPUT	--	BOOL_FLAG	TRUE if sound is done playing
**							FALSE if sound is still playing
**
** NOTE:	
*/
/***********************************************/
BOOL_FLAG GM_IsSoundDone(INT32 theRef);

/*************************************************/
/*
** FUNCTION SetMasterVolume;
**
** Overvue --
**	This will set the master output volume of the mixer.
**
**	INPUT	--	theVolume,	0-256 which is the master volume.
**	OUTPUT	--	
**
** NOTE:	
**	This is different that the hardware volume. This will scale the output by
**	theVolume factor.
**	There is somewhat of a CPU hit, while calulating the new scale buffer.
*/
/*************************************************/
void GM_SetMasterVolume(INT32 theVolume);

INT32 GM_GetMasterVolume(void);

/**************************************************/
/*
** FUNCTION ChangeSamplePitch(INT16 theRef, INT32 theNewRate);
**
** Overvue --
**	This will change the current sample referenced by theRef, to the new sample
**	rate of theNewRate.
**
**	INPUT	--		theRef,		a reference number returned from BeginSound
**					theNewRate,	FIXED_VALUE value for sample playback, in 16.16 format
**	OUTPUT	--	
**
** NOTE:	
*/
/**************************************************/
void GM_ChangeSamplePitch(INT32 theRef, FIXED_VALUE theNewRate);
FIXED_VALUE GM_GetSamplePitch(INT32 theRef);

void GM_ChangeSampleVolume(INT32 theRef, INT16 newVolume);
INT16 GM_GetSampleVolume(INT32 theRef);
void GM_SetSampleFadeRate(INT32 theRef, FIXED_VALUE fadeRate,
							INT16 minVolume, INT16 maxVolume, BOOL_FLAG endSample);



void GM_ChangeSampleStereoPosition(INT32 theRef, INT16 newStereoPosition);
INT16 GM_GetSampleStereoPosition(INT32 theRef);

void GM_ChangeSampleReverb(INT32 theRef, INT16 enable);

/**************************************************/
/*
** FUNCTION GM_EndAllSoundEffects;
**
** Overvue --
**	This will end all sound effects from the system.  It
**	does not shut down sound hardware or deallocate
** 	memory used by the music system.
**
**	INPUT	--	
**	OUTPUT	--	
**
** NOTE:	
*/
/**************************************************/
void GM_EndAllSoundEffects(void);
void GM_EndAllNotes(void);
void GM_KillAllNotes(void);

void GM_EndSongNotes(GM_Song *pSong);
void GM_KillSongNotes(GM_Song *pSong);

/**************************************************/
/*
** FUNCTION SetMasterSongTempo(INT32 newTempo);
**
** Overvue --
**	This will set the master tempo for the currently playing song.
**
**	INPUT	--	newTempo is in microseconds per MIDI quater-note.
**				Another way of looking at it is, 24ths of a microsecond per
**				MIDI clock.
**	OUTPUT	--	NO_SONG_PLAYING is returned if there is no song playing
**
** NOTE:	
*/
/**************************************************/
OPErr GM_SetMasterSongTempo(GM_Song *pSong, INT32 newTempo);

// Sets tempo in microsecond per quarter note
void GM_SetSongTempo(GM_Song *pSong, long newTempo);
// returns tempo in microsecond per quarter note
long GM_GetSongTempo(GM_Song *pSong);

// sets tempo in beats per minute
void GM_SetSongTempInBeatsPerMinute(GM_Song *pSong, long newTempoBPM);
// returns tempo in beats per minute
long GM_GetSongTempoInBeatsPerMinute(GM_Song *pSong);

// Instrument API

OPErr	GM_LoadInstrument(INT32 instrument);
OPErr	GM_LoadInstrumentFromExternal(INT32 instrument, void *theX, INT32 patchSize);
OPErr	GM_UnloadInstrument(INT32 instrument);
OPErr	GM_RemapInstrument(INT32 from, INT32 to);

// Pass TRUE to start caching instruments, FALSE to flush the cache and stop
void	GM_FlushInstrumentCache(BOOL_FLAG startStopCache);

// returns TRUE if instrument is loaded, FALSE if otherwise
XBOOL	GM_IsInstrumentLoaded(INT32 instrument);

/**************************************************/
/*
** FUNCTION GM_LoadSongInstruments(GM_Song *theSong)
**
** Overvue --
**	This will load the instruments required for this song to play
**
**	INPUT	--	theSong,	a pointer to the GM_Song data containing the MIDI Data
**			--	pArray,	an array that will be filled with the instruments that are
**						loaded, if not NULL.
**			--	loadInstruments, if TRUE will load instruments and samples
**
**	OUTPUT	--	OPErr,		an error will be returned if the song
**							cannot be loaded
**
** NOTE:	
*/
/**************************************************/
OPErr	GM_LoadSongInstruments(GM_Song *theSong, short int *pArray, short int loadInstruments);

OPErr	GM_UnloadSongInstruments(GM_Song *theSong);

OPErr	GM_LoadSongInstrument(GM_Song *pSong, INT32 instrument);
OPErr	GM_UnloadSongInstrument(GM_Song *pSong, INT32 instrument);

void	GM_SetSongLoopFlag(GM_Song *theSong, BOOL_FLAG loopSong);

void	GM_SetSongVolume(GM_Song *theSong, INT16 newVolume);
INT16	GM_GetSongVolume(GM_Song *theSong);

void	GM_SetSongFadeRate(GM_Song *pSong, FIXED_VALUE fadeRate,
							INT16 minVolume, INT16 maxVolume, BOOL_FLAG endSong);


// range is 0 to MAX_MASTER_VOLUME (256)
void	GM_SetEffectsVolume(INT16 newVolume);
INT16	GM_GetEffectsVolume(void);

BOOL_FLAG GM_IsInstrumentRangeUsed(INT16 thePatch, INT16 theLowKey, INT16 theHighKey);
BOOL_FLAG GM_IsInstrumentUsed(INT16 thePatch, INT16 theKey);
void GM_SetUsedInstrument(INT16 thePatch, INT16 theKey, BOOL_FLAG used);
void GM_SetInstrumentUsedRange(INT16 thePatch, SBYTE *pUsedArray);
void GM_GetInstrumentUsedRange(INT16 thePatch, SBYTE *pUsedArray);

void GM_SetSongCallback(GM_Song *theSong, GM_SongCallbackProcPtr songEndCallbackPtr);
void GM_SetSongTimeCallback(GM_Song *theSong, GM_SongTimeCallbackProcPtr songTimeCallbackPtr);

void GM_SetControllerCallback(short int controller, GM_ControlerCallbackPtr controllerCallback);

// Display
INT16 GM_GetAudioSampleFrame(INT16 *pLeft, INT16 *pRight, INT32 max_samples);

struct GM_AudioInfo
{
	INT16	maxNotesAllocated;
	INT16	maxEffectsAllocated;
	INT16	mixLevelAllocated;
	INT16	voicesActive;				// number of voices active
	INT16	patch[MAX_VOICES];			// current patches
	INT16	volume[MAX_VOICES];			// current volumes
	INT16	scaledVolume[MAX_VOICES];	// current scaled volumes
	INT16	channel[MAX_VOICES];		// current channel
	INT16	midiNote[MAX_VOICES];		// current midi note
	INT16	voice[MAX_VOICES];			// voice index
	GM_Song	*pSong[MAX_VOICES];			// song associated with voice
};
typedef struct GM_AudioInfo GM_AudioInfo;

void GM_GetRealtimeAudioInformation(GM_AudioInfo *pInfo);

// Set the global reverb type
void GM_SetReverbType(ReverbMode theReverbMode);

// Get the global reverb type
ReverbMode GM_GetReverbType(void);

// External MIDI links
#define Q_GET_TICK	0L		// if you pass this constant for timeStamp it will get the current
							// tick
void QGM_NoteOn(GM_Song *pSong, UINT32 timeStamp, INT16 channel, INT16 note, INT16 velocity);
void QGM_NoteOff(GM_Song *pSong, UINT32 timeStamp, INT16 channel, INT16 note, INT16 velocity);
void QGM_ProgramChange(GM_Song *pSong, UINT32 timeStamp, INT16 channel, INT16 program);
void QGM_PitchBend(GM_Song *pSong, UINT32 timeStamp, INT16 channel, UBYTE valueMSB, UBYTE valueLSB);
void QGM_Controller(GM_Song *pSong, UINT32 timeStamp, INT16 channel, INT16 controller, INT16 value);
void QGM_AllNotesOff(GM_Song *pSong, UINT32 timeStamp);
void QGM_LockExternalMidiQueue(void);
void QGM_UnlockExternalMidiQueue(void);

// External MIDI Links using a GM_Song. Will be unstable unless called via GM_AudioTaskCallbackPtr
void GM_NoteOn(GM_Song *pSong, INT16 channel, INT16 note, INT16 velocity);
void GM_NoteOff(GM_Song *pSong, INT16 channel, INT16 note, INT16 velocity);
void GM_ProgramChange(GM_Song *pSong, INT16 channel, INT16 program);
void GM_PitchBend(GM_Song *pSong, INT16 channel, UBYTE valueMSB, UBYTE valueLSB);
void GM_Controller(GM_Song *pSong, INT16 channel, INT16 controller, INT16 value);
void GM_AllNotesOff(GM_Song *pSong);

long GM_GetUserReference(void);
void GM_SetUserReference(long reference);

void GM_SetAudioTask(GM_AudioTaskCallbackPtr theTask);
void GM_SetAudioOutput(GM_AudioOutputCallbackPtr theHook);

#if USE_STREAM_API

enum
{
	FILE_AIFF_TYPE = 1,
	FILE_WAVE_TYPE,
	FILE_AU_TYPE
};
typedef long AudioFileType;


// This will read into memory the entire file and return a GM_Waveform structure.
// To dispose of a GM_Waveform structure, call GM_FreeWaveform
GM_Waveform * GM_ReadFileIntoMemory(XFILENAME *file, AudioFileType fileType);

// This will read from a block of memory an entire file and return a GM_Waveform structure.
// Assumes that the block of memory is formatted as a fileType
// To dispose of a GM_Waveform structure, call GM_FreeWaveform
GM_Waveform * GM_ReadFileIntoMemoryFromMemory(void *pFileBlock, unsigned long fileBlockSize, 
																	AudioFileType fileType);

// This will read into memory just the information about the file and return a 
// GM_Waveform structure.
// To dispose of a GM_Waveform structure, call GM_FreeWaveform
GM_Waveform * GM_ReadFileInformation(XFILENAME *file, AudioFileType fileType);

void GM_FreeWaveform(GM_Waveform *pWaveform);

// Audio Sample Data Format. (ASDF)
// Support for 8, 16 bit data, mono and stereo. Can be extended for multi channel beyond 2 channels, but
// not required at the moment.
//
//	DATA BLOCK
//		8 bit mono data
//			ZZZZZZZ...
//				Where Z is signed 8 bit data
//
//		16 bit mono data
//			WWWWW...
//				Where W is signed 16 bit data
//
//		8 bit stereo data
//			ZXZXZXZX...
//				Where Z is signed 8 bit data for left channel, and X is signed 8 bit data for right channel.
//
//		16 bit stereo data
//			WQWQWQ...
//				Where W is signed 16 bit data for left channel, and Q is signed 16 bit data for right channel.
//



enum
{
	STREAM_CREATE				=	1,
	STREAM_DESTROY,
	STREAM_GET_DATA
};

// The GM_StreamObjectProc callback is called to allocate buffer memory, get the next block of data to stream and
// mix into the final audio output, and finally dispose of the memory block. All messages will happen at 
// non-interrupt time.
//
// INPUT:
// Message
//	STREAM_CREATE
//		Use this message to create a block a data with a length of pAS->dataLength. Keep in mind that dataLength
//		is always total number of samples,  not bytes allocated. Allocate the block of data into the Audio Sample 
//		Data Format based upon pAS->dataBitSize and pAS->channelSize. Store the pointer into pAS->pData.
//
//	STREAM_DESTROY
//		Use this message to dispose of the memory allocated. pAS->pData will contain the pointer allocated.
//		pAS->dataLength will be the sample size not the buffer size. ie. for 8 bit data use pAS->dataLength, 
//		for 16 bit mono data double pAS->dataLength.
//
//	STREAM_GET_DATA
//		This message is called whenever the streaming object needs a new block of data. Right after STREAM_CREATE
//		is called, STREAM_GET_DATA will be called twice. Fill pAS->pData with the new data to be streamed.
//		Set pAS->dataLength to the amount of data put into pAS->pData.
//
// OUTPUT:
// returns
//	NO_ERR
//		Everythings ok
//
//	STREAM_STOP_PLAY
//		Everything is fine, but stop playing stream
//
//	MEMORY_ERR
//		Couldn't allocate memory for buffers.
//
//	PARAM_ERR
//		General purpose error. Something wrong with parameters passed.
//
//

struct GM_StreamData
{
	long		streamReference;	// 	IN for all messages
	long		userReference;		//	IN for all messages. userReference is passed in at AudioStreamStart
	void		*pData;				// 	OUT for STREAM_CREATE, IN for STREAM_DESTROY and STREAM_GET_DATA
	long		dataLength;			//	OUT for STREAM_CREATE, IN for STREAM_DESTROY. IN and OUT for STREAM_GET_DATA
	long		sampleRate;			//	IN for all messages. Fixed 16.16 value
	char		dataBitSize;		//	IN for STREAM_CREATE. Not used elsewhere
	char		channelSize;		//	IN for STREAM_CREATE. Not used elsewhere
};
typedef struct GM_StreamData	GM_StreamData;


// CALLBACK FUNCTIONS TYPES

// AudioStream object callback
typedef OPErr (*GM_StreamObjectProc)(short int message, GM_StreamData *pAS);



// Multi source user config based streaming
// This will start a streaming audio object.
//
// INPUT:
//	userReference	This is a reference value that will be returned and should be passed along to all AudioStream
//					functions.
//
//	pProc			is a GM_StreamObjectProc proc pointer. At startup of the streaming the proc will be called
//					with STREAM_CREATE, then followed by two STREAM_GET_DATA calls to get two buffers of data,
//					and finally STREAM_DESTROY when finished.
//
// OUTPUT:
//	long			This is an audio stream reference number. Will be 0 if error
long		GM_AudioStreamStart(	long userReference, 			// user reference
									GM_StreamObjectProc pProc, 		// control callback
									long bufferSize, 				// buffer size 
									long sampleRate,				// Fixed 16.16
									char dataBitSize,				// 8 or 16 bit data
									char channelSize);				// 1 or 2 channels of date

// start streaming a file
// OUTPUT:
//	long			This is an audio stream reference number. Will be 0 if error
long		GM_AudioStreamFileStart(	XFILENAME *file,			// file name
										AudioFileType fileType, 	// type of file
										long bufferSize,			// temp buffer to read file
										BOOL_FLAG loopFile);		// TRUE will loop file

// This will stop a streaming audio object and free any memory.
//
// INPUT:
//	reference	This is the reference number returned from AudioStreamStart.
//
OPErr		GM_AudioStreamStop(long reference);

// This will return the last AudioStream Error
OPErr		GM_AudioStreamError(void);

// This will stop all streams that are current playing and free any memory.
void		GM_AudioStreamStopAll(void);

// This is the streaming audio service routine. Call this as much as possible, but not during an
// interrupt. This is a very quick routine. A good place to call this is in your main event loop.
void		GM_AudioStreamService(void);

// Returns TRUE or FALSE if a given AudioStream is still active
BOOL_FLAG	GM_IsAudioStreamPlaying(long reference);

// Returns TRUE if a given AudioStream is valid
BOOL_FLAG	GM_IsAudioStreamValid(long reference);

// Set the volume level of a audio stream
void		GM_AudioStreamSetVolume(long reference, short int newVolume);

// Get the volume level of a audio stream
short int	GM_AudioStreamGetVolume(long reference);

// start a stream fading
void GM_SetAudioStreamFadeRate(long reference, FIXED_VALUE fadeRate, 
							INT16 minVolume, INT16 maxVolume, BOOL_FLAG endStream);

// Set the sample rate of a audio stream
void		GM_AudioStreamSetRate(long reference, unsigned long newRate);

// Get the sample rate of a audio stream
unsigned long	GM_AudioStreamGetRate(long reference);

// Set the stereo position of a audio stream
void		GM_AudioStreamSetStereoPosition(long reference, short int stereoPosition);

// Get the stereo position of a audio stream
short int	GM_AudioStreamGetStereoPosition(long reference);

// Enable/Disable reverb on this particular audio stream
void		GM_AudioStreamReverb(long reference, BOOL_FLAG useReverb);

void		GM_AudioStreamResume(void);
void		GM_AudioStreamPause(void);

#endif	// USE_STREAM_API

#if USE_MOD_API

typedef void		(*GM_ModDoneCallbackPtr)(struct GM_ModData *pMod);

struct GM_ModData
{
	void					*modControl;
	INT16					modVolume;

	FIXED_VALUE				modFadeRate;		// when non-zero fading is enabled
	FIXED_VALUE				modFixedVolume;		// inital volume level that will be changed by modFadeRate
	INT16					modFadeMaxVolume;	// max volume
	INT16					modFadeMinVolume;	// min volume
	BOOL_FLAG				modEndAtFade;

	long					reference;
	long					reference2;
	GM_ModDoneCallbackPtr	callback;
};
typedef struct GM_ModData GM_ModData;


GM_ModData	*GM_LoadModFile(void *pModFile, long fileSize);
void		GM_FreeModFile(GM_ModData *modReference);
void		GM_BeginModFile(GM_ModData *modReference, GM_ModDoneCallbackPtr callback, long reference);
void		GM_StopModFile(GM_ModData *modReference);

// volue max is MAX_SONG_VOLUME. You can overdrive
void		GM_SetModVolume(GM_ModData *modReference, INT16 volume);
short int	GM_GetModVolume(GM_ModData *modReference);
// start volume fade. For every 1.0 of fadeRate, volume will change by 11 ms
void		GM_SetModFadeRate(GM_ModData *modReference, FIXED_VALUE fadeRate, 
							INT16 minVolume, INT16 maxVolume, BOOL_FLAG endSong);

BOOL_FLAG	GM_IsModPlaying(GM_ModData *modReference);
void		GM_ResumeMod(GM_ModData *modReference);
void		GM_PauseMod(GM_ModData *modReference);
void		GM_SetModTempoFactor(GM_ModData *modReference, unsigned long fixedFactor);
void		GM_SetModLoop(GM_ModData *modReference, BOOL_FLAG loop);
void		GM_SetModTempoBPM(GM_ModData *modReference, unsigned long newTempoBPM);
unsigned long	GM_GetModTempoBPM(GM_ModData *modReference);
void		GM_GetModSongName(GM_ModData *mod, char *cName);


#endif	// USE_MOD_API

#ifdef __cplusplus
	}
#endif

#endif /* G_SOUND */



