#ifndef G_PRIVATE
#define G_PRIVATE
/*****************************************************************************/
/*
** "GenPriv.h"
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
**
** Overview
**	Private structures
**
** Modification History
**
**	11/7/95		Major changes, revised just about everything.
**	11/11/95	Added microSyncCount for live link
**	11/16/95	Removed microSyncCount
**				Moved static variables into MusicVar structure
**				Created an external function 'PV_GetExternalTimeSync()' for the external midi source
**   12/95 		upgraded mixing bus to 32 bit; improved scaleback resolution; added reverb unit; first pass at volume ADSR
**	12/6/95 	removed reference to USE_AMP_LOOKUP
**				moved REVERB_TYPE to GENSND.H
**	12/7/95		Added channelReverb to MusicVars structure
**				Added REVERB_CONTROLER_THRESHOLD
**	1/18/96		Spruced up for C++ extra error checking
**				Changed InstUsedList to pUsedList and allocate it when needed
**	2/5/96		Removed unused variables. Working towards multiple songs
**				Moved lots of variables from the MusicVars structure into
**				Moved the MAX_TRACKS define to GenSnd.h
**	2/12/96		Added PV_CleanExternalQueue
**				Moved SongMicroseconds to GenSnd.h
**	2/13/96		Added multi song support
**	3/5/96		Eliminated the global songVolume
**	3/28/96		Added PV_SetSampleIntoCache & PV_GetInstrument
**	4/10/96		Reworked the sample cache system to not clone the sample data
**	5/2/96		Changed int to BOOL_FLAG
**	5/18/96		Added error condition to PV_MusicIRQ
**	6/30/96		Changed font and re tabbed
**	7/3/96		Added packing pragmas
**				Removed usage of Machine.h. Now merged into X_API.h
**	7/14/96		Fixed structure alignment issue for PowerPC
**	7/23/96		Changed PV_GetExternalTimeSync to unsigned long
**	7/24/96		Changed Midi Queue system to use a head/tail
**	7/25/96		Moved Mac audio variables to GenMacTools.c
**				Changed PV_GetExternalTimeSync to GM_GetSyncTimeStampQuantizedAhead
**	8/12/96		Changed PV_ResetControlers to support semi-complete reset
**	9/25/96		Added GM_Song pointer in NoteRecord structure
**	9/27/96		Added more parameters to ServeMIDINote & StopMIDINote
**	10/18/96	Made CacheSampleInfo smaller
**	10/23/96	Removed reference to BYTE and changed them all to UBYTE or SBYTE
**	12/19/96	Added Sparc pragmas
**	12/30/96	Changed copyrights
**	1/23/97		Added support for stereoFilter
**				In NoteRecord changed PitchBend to NotePitchBend
**				Added in NoteRecord NoteFadeRate
**	1/30/97		Changed SYMPHONY_SIZE to MAX_VOICES
*/
/*****************************************************************************/

#ifndef __X_API__
	#include "X_API.h"
#endif

#ifndef G_SOUND
	#include "GenSnd.h"
#endif

#define DEBUG_STR(x)

#if CODE_TYPE == MACINTOSH
	#if 0
		#undef DEBUG_STR
		#define DEBUG_STR(x)		DebugStr(x)
	#endif
	
	#define STAND_ALONE		0
#endif

#define USE_DIRECT_MIXDOWN			1
#define VOLUME_PRECISION_SCALAR		6L		// used to be 8, so we must scale down output by 2
#define OUTPUT_SCALAR				9L		// 9 for volume minus 4 for increased volume_range resolution, plus 2 for increased volume precision scalar
#define VOLUME_RANGE				4096				// original range was 256, therefore:
#define UPSCALAR					16L		// multiplier (NOT a shift count!) for increasing amplitude resolution


#define MAX_CHUNK_SIZE				512		// max samples to build per slice at 44k
#define MAX_SAMPLES					768		// max number of samples that can be loaded
#define DEFAULT_PITCH_RANGE			2		// number of semi-tones that change with pitch bend wheel
#define PERCUSSION_CHANNEL			9		// which channel (zero based) is the default percussion channel
#define SOUND_EFFECT_CHANNEL		16		// channel used for sound effects. One beyond the normal

#define MAX_SONGS					4

#define REVERB_CONTROLER_THRESHOLD	13	// past this value via controlers, reverb is enabled

#define STEP_BIT_RANGE				12L
#define STEP_OVERFLOW_FLAG			(1<<(STEP_BIT_RANGE-1))		
#define STEP_FULL_RANGE				((1<<STEP_BIT_RANGE)-1)

#define MIN_LOOP_SIZE				20		// min number of loop samples that can be processed

#if (MAX_CHUNK_SIZE%4) != 0
	#error "Bad Chunk Size, Divisible by 4 only!" 
#endif

// a macro to handle broken loops and partial buffers in the inner loop code
#define THE_CHECK(TYPE) \
	if (cur_wave >= end_wave)\
	{\
		if (looping)\
		{\
			cur_wave -= wave_adjust;	/* back off pointer for previous sample*/ \
			if (this_voice->doubleBufferProc)\
			{\
				/* we hit the end of the loop call double buffer to notify swap*/ \
				if (PV_DoubleBufferCallbackAndSwap(this_voice->doubleBufferProc, this_voice)) \
				{\
					/* recalculate our internal pointers */\
					end_wave = (FIXED_VALUE)(this_voice->NoteLoopEnd - this_voice->NotePtr) << STEP_BIT_RANGE;\
					wave_adjust =  (FIXED_VALUE)(this_voice->NoteLoopEnd - this_voice->NoteLoopPtr) << STEP_BIT_RANGE;\
					source = (TYPE) this_voice->NotePtr;\
				}\
			}\
		}\
		else\
		{\
			this_voice->NoteDur = -1;\
			PV_DoCallBack(this_voice);\
			goto FINISH;\
		}\
	}

#define ALLOW_16_BIT			1		// 1 - allow 16 bit if available, 0 - force 8 bit
#define ALLOW_STEREO			1		// 1 - allow stereo if available, 0 - force mono
#define ALLOW_DEBUG_STEREO		0		// 1 - allow keyboard debugging of stereo code
#define USE_SEQUENCER			1

typedef unsigned char			OUTSAMPLE8;
typedef short					OUTSAMPLE16;		// 16 bit output sample


#define SUS_NORMAL				0		// normal release at note off
#define SUS_ON_NOTE_ON			1		// note on, with pedal
#define SUS_ON_NOTE_OFF			2		// note off, with pedal

#define MAXRESONANCE			127		// mask and buffer size for resonant filter.  Higher means wider frequency range.

#if __MWERKS__
	#if (CPU_TYPE == kRISC) || (CODE_TYPE == BEBOX)
		#pragma options align=power
	#elif ((CPU_TYPE == k80X86) || (CPU_TYPE == kSPARC))
		#pragma pack (8)
	#endif
#endif //  __MWERKS__

// This structure is created and maintained for each sample that is to mixed into the final output
struct NoteRecord
{
	INT16					NoteDur;				// duration of note to play. -1 is dead
	INT16					NoteDecay;				// after duration == zero then this is ticks of decay
	GM_Instrument			*inst_struct;			// read-only copy of instrument information
	GM_Song					*pSong;					// read-only copy of song information
													// used to backtrace where note came from
	UBYTE FAR				*NotePtr;				// pointer to start of sample
	UBYTE FAR				*NotePtrEnd;			// pointer to end of sample
	FIXED_VALUE				NoteWave;				// current position within sample (NotePtr:NotePtrEnd)
	FIXED_VALUE				NotePitch;				// playback pitch in 16.16 fixed. 1.0 will play recorded speed
	FIXED_VALUE				noteSamplePitchAdjust;	// adjustment to pitch based on difference from 22KHz in recorded rate
	UBYTE FAR				*NoteLoopPtr;			// pointer to start of loop point within NotePtr & NotePtrEnd
	UBYTE FAR				*NoteLoopEnd;			// pointer to end of loop point within NotePtr & NotePtrEnd
	INT32					NoteRefNum;				// user reference number
	INT32					NoteRefNum2;			// user reference number

// Double buffer variables. If using double buffering, then doubleBufferPtr1 will be non-zero. These variables
// will be swapped with NotePtr, NotePtrEnd, NoteLoopPtr, NoteLoopPtrEnd
	UBYTE FAR *					doubleBufferPtr1;
	UBYTE FAR *					doubleBufferPtr2;
	GM_DoubleBufferCallbackPtr	doubleBufferProc;

// Call back procs
	GM_LoopDoneCallbackPtr		NoteLoopProc;		// normal loop continue proc
	GM_SoundDoneCallbackPtr		NoteEndCallback;	// sample done callback proc

	INT16					NoteNextSize;			// number of samples per slice. Use 0 to recalculate
	SBYTE					NoteMIDIPitch;			// midi note pitch to start note
	SBYTE					noteOffsetStart;		// at the start of the midi note, what was the offset
	INT16					ProcessedPitch;			// actual pitch to play (proccessed)
	INT16					NoteProgram;			// midi program number
	SBYTE					NoteChannel;			// channel note is playing on
	SBYTE					NoteTrack;				// track note is playing on
	INT32					NoteVolume;				// note volume (scaled)
	INT16					NoteVolumeEnvelope;		// scalar from volume ADSR and LFO's.  0 min, VOLUME_RANGE max.
	INT16					NoteVolumeEnvelopeBeforeLFO;	// as described.
	INT16					NoteMIDIVolume;			// note volume (unscaled)
	INT16					NotePitchBend;			// 8.8 Fixed amount of bend
	INT16					ModWheelValue;			// 0-127
	INT16					LastModWheelValue;		// has it changed?  This is how we know.
	INT16					LastPitchBend;			// last bend
	INT16					stereoPosition;			// -63 (left) 0 (Middle) 63 (Right)
	UBYTE					NoteLoopCount;
	UBYTE					bitSize;				// 8 or 16 bit data
	UBYTE					channels;				// mono or stereo data
	UBYTE					sustainMode;			// sustain mode, for pedal controls
	UBYTE					avoidReverb;			// don't mix into reverb unit
	UBYTE					sampleAndHold;			// flag whether to sample & hold, or sample & release

// sound effects variables. Not used for normal envelope or instruments
	UBYTE					soundEndAtFade;
	FIXED_VALUE				soundFadeRate;			// when non-zero fading is enabled
	FIXED_VALUE				soundFixedVolume;		// inital volume level that will be changed by soundFadeRate
	INT16					soundFadeMaxVolume;		// max volume
	INT16					soundFadeMinVolume;		// min volume

	INT32					stereoPanBend;

	ADSRRecord				volumeADSRRecord;
	INT32					volumeLFOValue;
	INT16					LFORecordCount;
	LFORecord				LFORecords[MAX_LFOS+1];	// allocate for maximum allowed
	INT32					lastAmplitudeL;
	INT32					lastAmplitudeR;			// used to interpolate between points in volume ADSR

	INT16					z[MAXRESONANCE+1];
	INT32					zIndex, Z1value, previous_zFrequency;
	INT32					LPF_lowpassAmount, LPF_frequency, LPF_resonance;
	INT32					LPF_base_lowpassAmount, LPF_base_frequency, LPF_base_resonance;
//	INT32					s1Left, s2Left, s3Left, s4Left, s5Left, s6Left; // for INTERP3 mode only
};
typedef struct NoteRecord NoteRecord;

// Structure used for caching samples for instruments
struct CacheSampleInfo
{
	unsigned long	cacheBlockID;	// block ID. for debugging
	unsigned long	rate;			// sample rate
	unsigned long	waveSize;		// size in bytes
	unsigned long	waveFrames;		// number of frames
	unsigned long	loopStart;		// loop start frame
	unsigned long	loopEnd;		// loop end frame
	char			bitSize;		// sample bit size; 8 or 16
	char			channels;		// mono or stereo; 1 or 2
	short int		baseKey;		// base sample key
	short int		theID;			// sample ID
	void			*pSampleData;	// pointer to sample data. This may be an offset into the pMasterPtr
	void			*pMasterPtr;	// master pointer that contains the snd format information
};
typedef struct CacheSampleInfo CacheSampleInfo;

struct InstrumentRemap
{
	INT16	from;
	INT16	to;
};
typedef struct InstrumentRemap InstrumentRemap;

#define MAX_QUEUE_EVENTS				256

#if USE_SMALL_MEMORY_REVERB
	#define REVERB_BUFFER_SIZE			4096		// * sizeof(long)
	#define REVERB_BUFFER_MASK 			4095
#else
	#define REVERB_BUFFER_SIZE			16384
	#define REVERB_BUFFER_MASK 			32767
#endif

// This structure is to allow for queuing midi events into the playback other than those that are
// pulled from the midi file stream
struct Q_MIDIEvent
{
	GM_Song			*pSong;			// pSong the event was placed from
	UINT32			timeStamp;		// timestamp of event
	UBYTE			midiChannel;	// which channel
	UBYTE			command;		// which command
	UBYTE			byte1;			// note, controller
	UBYTE			byte2;			// velocity, lsb/msb
};
typedef struct Q_MIDIEvent Q_MIDIEvent;

typedef void			(*InnerLoop)(NoteRecord *r);
typedef void			(*InnerLoop2)(NoteRecord *r, BOOL_FLAG looping);

struct MusicVars
{
	TerpMode			interpolationMode;				// output interpolation mode
	Quality				outputQuality;					// output sample rate
	INT16				sampleExpansion;				// output expansion factor
	BOOL_FLAG	/*0*/	generate16output;				// if TRUE, then build 16 bit output
	BOOL_FLAG	/*1*/	generateStereoOutput;			// if TRUE, then output stereo data
	BOOL_FLAG	/*2*/	insideAudioInterrupt;
	BOOL_FLAG	/*3*/	systemPaused;					// all sound paused and disengaged from hardware
	BOOL_FLAG 	/*4*/	reserved_0;
	BOOL_FLAG	/*5*/	sequencerPaused;				// MIDI sequencer paused
	BOOL_FLAG	/*6*/	cacheSamples;					// if TRUE, then samples will be cached
	BOOL_FLAG	/*7*/	cacheInstruments;				// if TRUE, then instruments will be cached
	BOOL_FLAG	/*0*/	stereoFilter;					// if TRUE, then filter stereo output

	CacheSampleInfo 	*sampleCaches[MAX_SAMPLES];		// cache of samples loaded

	INT16				anoymousCacheID;

	INT16				MaxNotes;
	INT16				mixLevel;
	INT16				MaxEffects;
	INT16 				MasterVolume;

	INT32				scaleBackAmount;

	INT16				maxChunkSize;
	LOOPCOUNT			One_Slice, One_Loop, Two_Loop, Four_Loop, Sixteen_Loop;

	GM_Instrument		*InstrumentData[MAX_INSTRUMENTS*MAX_BANKS];
														// Pointer to loaded instruments
	INT16				remapArray[MAX_INSTRUMENTS*MAX_BANKS];

	void				*pUsedPatchList;				// This is NULL most of the time, only
														// GM_LoadSongInstruments sets it
														// instruments by notes
														// This should be about 4096 bytes
														// during preprocess of the midi file, this will be
														// the instruments that need to be loaded
														// total divided by 8 bits. each bit represents an instrument

	NoteRecord			NoteEntry[MAX_VOICES];
	INT32				songBufferLeftMono[MAX_CHUNK_SIZE + 48];	// keep cache lines from colliding
	INT32				songBufferRightStereo[MAX_CHUNK_SIZE + 2];

// MOD interpreter file playing. NULL is no file
	void				*pModPlaying;

// MIDI Interpreter variables
	GM_Song				*theSongPlaying;				// current song playing
														// If non-null then  a song loaded into memory and ready to play

	GM_Song				*pSongsToPlay[MAX_SONGS];		// number of songs to play at once
	INT16				effectsVolume;					// volume multiplier of all effects

	GM_ControlerCallbackPtr	channelCallbackProc[MAX_CONTROLLERS];		// current controller callback functions

// normal inner loop procs
	InnerLoop2			partialBufferProc;
	InnerLoop			fullBufferProc;
	InnerLoop2			partialBufferProc16;
	InnerLoop			fullBufferProc16;

// procs for resonant low-pass filtering
	InnerLoop2			filterPartialBufferProc;
	InnerLoop			filterFullBufferProc;
	InnerLoop2			filterPartialBufferProc16;
	InnerLoop			filterFullBufferProc16;

// external midi control variables
	Q_MIDIEvent			theExternalMidiQueue[MAX_QUEUE_EVENTS];
// pointers for circular event buffer
	Q_MIDIEvent			*pHead;							// pointer to events to read from queue
	Q_MIDIEvent			*pTail;							// pointer to events to write to queue
														// always points to the next one to use
	BOOL_FLAG			processExternalMidiQueue;		// flag to lock processing of queue

	UINT32				syncCount;						// in microseconds. Current tick of audio output
	INT16				syncBufferCount;
// put these at the end
	INT32				reverbPtr;
	INT32				LPfilterL, LPfilterR;
	INT32				LPfilterLz, LPfilterRz;
	INT32				s1Left, s2Left, s3Left, s4Left, s5Left, s6Left, s7Left, s8Left;
	INT32				s1Right, s2Right, s3Right, s4Right, s5Right, s6Right, s7Right, s8Right;
	INT32				reverbUnitType;
	INT32				*reverbBuffer;

};
typedef struct MusicVars MusicVars;

#if __MWERKS__
	#if (CPU_TYPE == kRISC) || (CODE_TYPE == BEBOX)
		#pragma options align=reset
	#elif ((CPU_TYPE == k80X86) || (CPU_TYPE == kSPARC))
		#pragma pack ()
	#endif
#endif //__MWERKS__

extern MusicVars *MusicGlobals;

#ifdef __cplusplus
	extern "C" {
#endif


/* Interal function declarations
*/
/* Sound Specific
*/
BOOL_FLAG InitSoundManager(void);
void FinisSoundManager(void);

void PV_ProcessReverbMono(void);
void PV_ProcessReverbStereo(void);
void PV_PostFilterStereo(void);
void PV_Generate8outputStereo(OUTSAMPLE8 FAR * dest8);
void PV_Generate8outputMono(OUTSAMPLE8 FAR * dest8);
void PV_Generate16outputStereo(OUTSAMPLE16 FAR * dest16);
void PV_Generate16outputMono(OUTSAMPLE16 FAR * dest16);

long PV_DoubleBufferCallbackAndSwap(GM_DoubleBufferCallbackPtr doubleBufferCallback, 
										NoteRecord *this_voice);
void PV_CalculateStereoVolume(NoteRecord *this_voice, INT32 *pLeft, INT32 *pRight);

void PV_ServeEffectsFades(void);

void PV_ServeStereoInterp3FullBuffer (NoteRecord *this_voice);
void PV_ServeStereoInterp3PartialBuffer (NoteRecord *this_voice, BOOL_FLAG looping);

void PV_ServeInterp2FilterFullBuffer (NoteRecord *this_voice);
void PV_ServeStereoInterp2FilterFullBuffer (NoteRecord *this_voice);
void PV_ServeInterp2FilterFullBuffer16 (NoteRecord *this_voice);
void PV_ServeStereoInterp2FilterFullBuffer16 (NoteRecord *this_voice);

void PV_ServeInterp2FilterPartialBuffer (NoteRecord *this_voice, BOOL_FLAG looping);
void PV_ServeStereoInterp2FilterPartialBuffer (NoteRecord *this_voice, BOOL_FLAG looping);
void PV_ServeInterp2FilterPartialBuffer16 (NoteRecord *this_voice, BOOL_FLAG looping);
void PV_ServeStereoInterp2FilterPartialBuffer16 (NoteRecord *this_voice, BOOL_FLAG looping);

void PV_ServeInterp2FullBuffer (NoteRecord *this_voice);
void PV_ServeStereoInterp2FullBuffer (NoteRecord *this_voice);
void PV_ServeInterp2FullBuffer16 (NoteRecord *this_voice);
void PV_ServeStereoInterp2FullBuffer16 (NoteRecord *this_voice);

void PV_ServeInterp2PartialBuffer (NoteRecord *this_voice, BOOL_FLAG looping);
void PV_ServeStereoInterp2PartialBuffer (NoteRecord *this_voice, BOOL_FLAG looping);
void PV_ServeInterp2PartialBuffer16 (NoteRecord *this_voice, BOOL_FLAG looping);
void PV_ServeStereoInterp2PartialBuffer16 (NoteRecord *this_voice, BOOL_FLAG looping);

void PV_ServeInterp1FullBuffer (NoteRecord *this_voice);
void PV_ServeInterp1PartialBuffer (NoteRecord *this_voice, BOOL_FLAG looping);
void PV_ServeStereoInterp1FullBuffer (NoteRecord *this_voice);
void PV_ServeStereoInterp1PartialBuffer (NoteRecord *this_voice, BOOL_FLAG looping);

void PV_ServeDropSampleFullBuffer (NoteRecord *this_voice);
void PV_ServeDropSamplePartialBuffer (NoteRecord *this_voice, BOOL_FLAG looping);
void PV_ServeStereoAmpFullBuffer (NoteRecord *this_voice);
void PV_ServeStereoAmpPartialBuffer (NoteRecord *this_voice, BOOL_FLAG looping);


void ServeMIDINote(GM_Song *pSong, INT16 the_instrument, 
						INT16 the_channel, INT16 the_track, INT16 notePitch, INT32 Volume);
void StopMIDINote(GM_Song *pSong, INT16 the_instrument, 
						INT16 the_channel, INT16 the_track, INT16 notePitch);

INT16 SetChannelPitchBend(INT16 the_channel, UBYTE bendRange, UBYTE bendMSB, UBYTE bendLSB);
void SetChannelVolume(INT16 the_channel, INT16 newVolume);
INT16 SetChannelStereoPosition(INT16 the_channel, UINT16 newPosition);
void SetChannelModWheel(INT16 the_channel, UINT16 value);
void PV_ChangeSustainedNotes(INT16 the_channel, INT16 data);

void PV_CleanExternalQueue(void);

void PV_ProcessSequencerEvents(void);
void ProcessSampleFrame(void FAR *destSampleData);
OPErr PV_MusicIRQ(GM_Song *pSong);

void PV_ConfigureInstruments(GM_Song *theSong);
OPErr PV_ConfigureMusic(GM_Song *theSong);
void PV_ResetControlers(GM_Song *pSong, INT16 channel2Reset, BOOL_FLAG completeReset);

// GenPatch.c
BOOL_FLAG PV_AnyStereoInstrumentsLoaded(void);
GM_Instrument * PV_GetInstrument(short theID, void *theExternalX, long patchSize);
void PV_SetSampleIntoCache(long theID, XPTR pSndFormatData);

INT32 PV_ScaleVolumeFromChannelAndSong(GM_Song *pSong, INT16 channel, INT32 volume);
void PV_DoCallBack(NoteRecord *this_one);
void PV_CleanNoteEntry(NoteRecord * the_entry);
void PV_CalcScaleBack(void);
FIXED_VALUE PV_GetWavePitch(FIXED_VALUE notePitch);

long VX_GetMouse(void);
BOOL_FLAG IsOptionOn(void);

// GenModFiles.c
void PV_WriteModOutput(Quality q, XBOOL stereo);

// GenAudioStreams.c
void PV_ServeStreamFades(void);

#ifdef __cplusplus
	}
#endif

#endif 	/* G_PRIVATE	*/ 
