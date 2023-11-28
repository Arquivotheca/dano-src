/*****************************************************************************/
/*
** "BAE.h"
**
**	Generalized Audio Synthesis package presented in an oop fashion
**
**	\xA9 Copyright 1996-1999 Beatnik, Inc, All Rights Reserved.
**	Written by Steve Hales
**
**	Beatnik products contain certain trade secrets and confidential and
**	proprietary information of Beatnik.  Use, reproduction, disclosure
**	and distribution by any means are prohibited, except pursuant to
**	a written license from Beatnik. Use of copyright notice is
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
** Modification History:
**
**	7/8/96		Created
**	11/18/96	Added LoadResourceSample
**	11/19/96	Changed char to BAE_BOOL in cases of boolean use
**	11/21/96	Changed LoadResourceSample to pass in size of data
**				Seperated BAEMidi into two object types. BAEMidiSong and
**				BAEMidiSynth, to make it clear as to what you are controlling
**	11/25/96	Modifed class to sub class the BAEMidiSynth class.
**	11/26/96	Added Fade
**	12/1/96		Changed GetCurrentProgramBank to GetProgramBank
**				Changed GetCurrentControlValue to GetControlValue
**				Added TRUE/FALSE
**	12/2/96		Added Mod code
**	12/3/96		Fixed bug with BAESound::Start
**	12/10/96	Added BAEOutputMixer::ChangeAudioFileToMemory
**	12/30/96	Changed copyright
**	1/2/97		Added GetSongNameFromAudioFile & GetSampleNameFromAudioFile &
**				GetInstrumentNameFromAudioFile
**	1/7/97		Added BAEMidiSong::LoadFromMemory
**	1/10/97		Added some const
**	1/16/97		Added GetInfoSize
**	1/22/97		Added BAEOutputMixer::GetTick
**				Fixed callbacks and added SetDoneCallback in BAESound and BAEModSong
**	1/24/97		Fixed a bug with BAEOutputMixer::Close that could make my counters
**				get out of sync
**				Added BAEModSong::SetLoopFlag & BAEModSong::SetTempoInBeatsPerMinute &
**				BAEModSong::GetTempoInBeatsPerMinute
**				Added LONG_TO_FIXED & FIXED_TO_LONG
**				Rebuilt fade system. Completely intergrated with low level mixer
**	1/28/97		Added more comments on how to use the ChangeAudioFileToMemory method
**				Added a BAERmfSong class. Its pretty much the same as a BAEMidiSong
**				object, except it knows how to read an RMF file
**	2/1/97		Added BAEMidiSynth::AllowChannelPitchOffset & 
**				BAEMidiSynth::DoesChannelAllowPitchOffset
**	2/5/97		Added BAESound::LoadMemorySample
**				Changed MOD handling not to keep MOD loaded into memory once parsed
**				Added BAEModSong::LoadFromMemory
**	2/19/97		Added support for platform specific data when creating an BAEOutputMixer
**	2/26/97		Removed extra copyright date field
**	2/28/97		Added NoteOnWithLoad & IsInstrumentLoaded & TranslateBankProgramToInstrument & 
**				TranslateCurrentProgramToInstrument
**	3/3/97		Removed NoteOnWithLoad & TranslateCurrentProgramToInstrument
**				Added the typedef for special instrument
**	3/10/97		Removed extra reverbmode get, and typed the SetReverb correctly
**	3/12/97		Changed the default for MixLevel to 8 in the BAEMixer Open method
**				Added a BAE_REVERB_NO_CHANGE reverb mode. This means don't change the
**				current mixer state
**	3/18/97		Added GetLoopFlag in BAEMidiSong & BAEModSong
**	3/20/97		Added LoadFromID in the BAEMidiSong method
**				Added GetMicrosecondLength & SetMicrosecondPosition & GetMicrosecondPosition
**				in the BAEMidiSong class
**	4/18/97		Removed extra linkage and unused variables
**	4/20/97		Added BAEMidiSong::SetMetaEventCallback
**	5/1/97		Changed BAESound::Start to accecpt an frame offset when starting the sample
**				Added BAESound::StartCustomSample
**	5/5/97		Fixed a problem with BAESound::StartCustomSample and BAESound::Start
**				in which the stereoPosition was scaled wrong. Now the value is -63 to 63.
**	5/7/97		Added to BAESound::StartCustomSample & BAESound::Start new error
**				messages for a voice that is still playing, a way to stop the sound
**				and an error when the volume level is zero
**	5/12/97		Fixed memory leak when failing with BAEMidiSong::LoadFromFile & 
**				BAEMidiSong::LoadFromMemory
**	5/13/97		Added BAEOutputMixer::GetVersionFromAudioFile
**	5/21/97		Added BAESound::GetPlaybackPosition
**				Changed method names for BAESoundStream. To stream a normal file,
**				you'll now call SetupFileStream then Start. To do a custom stream
**				call SetupCustomStream, then start.
**				Added BAESoundStream::GetInfo to return information about the
**				file once it has been setup.
**				Added BAESound::GetInfo
**	5/23/97		Added BAESound::GetSamplePointer & BAESound::GetSampleLoopPoints &
**				BAESound::SetSampleLoopPoints
**	6/25/97		Changed an unsigned to an unsigned long in BAESound::SetSampleLoopPoints
**	6/27/97		Changed BAEMidiSynth::Open to set the song to use the current
**				mixer settings
**				Changed BAEOutputMixer::GetMixLevel & BAEOutputMixer::GetSoundVoices & 
**				BAEOutputMixer::GetMidiVoice & BAEOutputMixer::GetModifiers update to
**				the real values
**	7/9/97		Added BAEMidiSynth::ParseMidiData to parse midi data and disburse it
**				to the various functions
**	7/15/97		Added BAENoise base class to all BAEAudio objects
**				Added BAEOutputMixer::StartOutputToFile & BAEOutputMixer::StopOutputToFile & 
**				BAEOutputMixer::ServiceAudioOutputToFile
**	8/15/97		Moved the method BAEOutputMixer::SetControlCallback to BAEMidiSong::SetControlCallback
**	8/20/97		Changed BAESoundStream::SetupCustomStream & BAESoundStream::SetupFileStream
**				to use BAE_MIN_STREAM_BUFFER_SIZE to control minimumn size of buffer
**	9/11/97		Added BAEMidiSong::IsPaused && BAEModSong::IsDone
**	9/18/97		Respelled GetAudioLatancy to GetAudioLatency
**	9/30/97		Added BAEMidiSong::GetEmbeddedMidiVoices & BAEMidiSong::GetEmbeddedMixLevel & 
**				BAEMidiSong::GetEmbeddedSoundVoices
**	10/2/97		Added BAE_MAX_SONGS
**	10/3/97		Added BAEMidiSong::SetEmbeddedMidiVoices & BAEMidiSong::SetEmbeddedMixLevel & 
**				BAEMidiSong::SetEmbeddedSoundVoices
**				Added BAEMidiSong::GetEmbeddedReverbType & BAEMidiSong::SetEmbeddedReverbType
**				Added BAEOutputMixer::GetQuality & BAEOutputMixer::GetTerpMode
**	10/12/97	Added BAERmfSong::LoadFromBank
**	10/16/97	Changed BAEMidiSong::Start to allow for an optional reconfigure of
**				the mixer when starting a song
**				Modified BAEMidiSong::LoadFromID & BAEMidiSong::LoadFromBank & BAEMidiSong::LoadFromFile &
**				BAEMidiSong::LoadFromMemory & BAERmfSong::LoadFromFile & BAERmfSong::LoadFromMemory &
**				BAERmfSong::LoadFromBank to allow for optional reporting of failure to load instruments
**				Removed BAEMidiSynth::FlushInstrumentCache. Not required or used.
**				Renamed GetPatches to GetInstruments and changed the array passed to
**				be of type BAE_INSTRUMENT
**	11/6/97		Added BAERmfSong::LoadFromID
**	11/11/97	Added GetMaxDeviceCount & SetCurrentDevice & GetCurrentDevice & GetDeviceName
**	11/24/97	Added BAESoundStream::Flush
**	12/18/97	Cleaned up some warnings and added some rounding devices
**	1/22/98		Added BAERmfSong::IsCompressed BAERmfSong::IsEncrypted
**	1/27/98		Added a parameter to BAEMidiSong::SetTimeCallback and changed the way the callbacks
**				are handled
**	2/6/98		Added virtuals to all sub classed destructor methods
**	2/11/98		Changed BAESound::SetSampleLoopPoints to accept 0 as a valid start
**				loop point
**				Added BAE_8K, BAE_48K, BAE_11K_TERP_22K, BAE_22K_TERP_44K, BAE_24K
**	2/18/98		Added BAESound::StartDoubleBuffer
**	2/19/98		Added BAEOutputMixer::GetURLFromAudioFile & BAEOutputMixer::GetNameFromAudioFile
**	2/24/98		Fixed a problem with the way the SONG resource and memory based files handle
**				retriving the size of memory blocks inside of a memory file
**	3/5/98		Changed TranslateBankProgramToInstrument to a static object
**	3/9/98		Modified open to allow an optional not connect to audio hardware. If you call
**				Open without connecting to hardware, you'll need to call BAEOutputMixer::ReengageAudio
**				Added new method BAEOutputMixer::IsAudioEngaged
**	3/12/98		Added BAEMidiSong::SetEmbeddedVolume & BAEMidiSong::GetEmbeddedVolume
**	3/16/98		Added new verb types
**	3/23/98		MOE: Added error codes: BAE_BAD_SAMPLE_RATE, BAE_TOO_MANY_SAMPLES,
**				BAE_UNSUPPORTED_FORMAT, BAE_FILE_IO_ERROR
**	3/23/98		MOE: Added enums: BAEEncryptionType{}, BAECompressionType{}
**	4/30/98		Added SUB_GENRE_INFO & GENRE_INFO
**	5/7/98		Added BAE_REVERB_TYPE_COUNT
**
**	6/5/98		Jim Nitchals RIP	1/15/62 - 6/5/98
**				I'm going to miss your irreverent humor. Your coding style and desire
**				to make things as fast as possible. Your collaboration behind this entire
**				codebase. Your absolute belief in creating the best possible relationships 
**				from honesty and integrity. Your ability to enjoy conversation. Your business 
**				savvy in understanding the big picture. Your gentleness. Your willingness 
**				to understand someone else's way of thinking. Your debates on the latest 
**				political issues. Your generosity. Your great mimicking of cartoon voices. 
**				Your friendship.
**
**	6/18/98		Added SetCacheStatusForAudioFile & GetCacheStatusForAudioFile
**	7/27/98		Added LockQueue & UnlockQueue
**	7/28/98		Added BAEMidiSynth::SetStereoPosition & BAEMidiSynth::GetStereoPosition
**	7/30/98		Added reverb state and fixed all the BAESound::Start/BAESoundStream::Start functions 
**				to start the verb up when the object gets started.
**	8/6/98		Changed reference to mSoundVoiceReference in the BAESound class.
**				Changed mReference to mSoundStreamVoiceReference in the BAESoundStream class
**	8/10/98		Added macros FLOAT_TO_UNSIGNED_FIXED & UNSIGNED_FIXED_TO_FLOAT & LONG_TO_UNSIGNED_FIXED &
**				UNSIGNED_FIXED_TO_LONG & UNSIGNED_FIXED_TO_SHORT
**	8/11/98		Renamed pName to cName and implemented GetName in the BAENoise base class
**				Changed default behavior of BAEMidiSong::LoadFromFile & BAEMidiSong::LoadFromBank & 
**				BAEMidiSong::LoadFromID & BAEMidiSong::LoadFromMemory to ignore bad instruments
**	8/13/98		Added BAEVoiceType to BAEAudioInfo structure
**				Added BAEOutputMixer::GetCPULoadInMicroseconds & BAEOutputMixer::GetCPULoadInPercent
**	9/2/98		Added BAE_MPEG_TYPE to BAEFileType
**	9/8/98		Added BAE_SAMPLE_TO_LARGE
**	9/12/98		Added BAESound::GetSamplePointerFromMixer
**	10/26/98	Added error code to BAEOutputMixer::ServiceAudioOutputToFile
**	10/30/98	Implemented a default BAESound done callback, that marks the sample finished when playing out
**				normally. Related to the 10/17/98 bug.
**	11/24/98	Added BAESound::SetReverbAmount & BAESound::GetReverbAmount.
**	12/3/98		Added BAESoundStream::SetReverbAmount & BAESoundStream::GetReverbAmount
**				Added BAENoiseGroup class, and modified BAENoise for linked list issues
**	12/9/98		Added BAEMidiSynth::GetPitchBend
**	12/17/98	Added BAEOutputMixer::SetHardwareBalance & BAEOutputMixer::GetHardwareBalance
**	12/18/98	Added to BAEMidiSong::Start auto level
**	1/14/99		Added BAEMidiSynth::CreateInstrumentAsData && BAEMidiSynth::LoadInstrumentFromData
**	2/18/99		Renamed pSongVariables to m_pSongVariables, queueMidi to m_queueMidi, 
**				reference to m_reference
**				Added GetMetaCallback & GetMetaCallbackReference and support variables
**	3/1/99		Added new types to BAECompressionType
**	3/16/99		MOE:  Added BAE_ABORTED
**	3/24/99		Added TEMPO_DESCRIPTION_INFO & ORIGINAL_SOURCE_INFO
**	4/8/99		Added BAEOutputMixer::GetCPUType
**	5/17/99		Added BAEMidiSong::Preroll
**	5/19/99		Added BAE_FILE_NOT_FOUND error code
**	5/26/99		MOE:  Changed BAE_COMPRESSION_NORMAL to BAE_COMPRESSION_LOSSLESS
**	5/28/99		MOE:  made BAEMidiSynth::TranslateBankProgramToInstrument() static again
**	6/3/99		Added BAEOutputMixer::IsStereoSupported(void) & BAEOutputMixer::Is8BitSupported(void) &
**				BAEOutputMixer::Is16BitSupported(void). 
**	6/4/99		Added BAE_DEFAULT_PROGRAM & BAE_DEFAULT_BANK
**	6/8/99		Added a NoiseType to the BAENoise class. It gets set automaticly when creating
**				various decended class objects. Added GetType to BAENoise retrive the type of
**				object for later use when polymorphing objects.
**	6/9/99		Added BAEOutputMixer::GetMixerVersion. Added BAE_VERSION_MAJOR & BAE_VERSION_MINOR & BAE_VERSION_SUB_MINOR
**	6/10/99		Placed in BAENoise::~BAENoise the ability to remove from the event queue
**				if there. Finished BAEOutputMixer::RemoveEvent
**	6/15/99		Changed the behavior of BAEMidiSong::Stop to not remove the song from the mixer. This
**				is done to allow us to continue to send events for processing.
**	6/17/99		Added virtual to BAERmfSong & BAEMidiSong
**	6/29/99		Added a new version of BAEOutputMixer::GetNameFromAudioFile that takes a BAEPathName
**	7/9/99		Added BAEOutputMixer::GetTaskCallback, BAEOutputMixer::GetTaskReference
**	7/13/99		Renamed HAE to BAE. Renamed BAEAudioMixer to BAEOutputMixer. Renamed BAEMidiDirect
**				to BAEMidiSynth. Renamed BAEMidiFile to BAEMidiSong. Renamed BAERMFFile to BAERmfSong.
**				Renamed BAErr to BAEResult. Renamed BAEMidiExternal to BAEMidiInput. Renamed BAEMod
**				to BAEModSong. Renamed BAEGroup to BAENoiseGroup. Renamed BAEReverbMode to BAEReverbType.
**				Renamed BAEAudioNoise to BAENoise.
**	7/14/99		Added BAEOutputMixer::GetChannelSoloStatus & BAEOutputMixer::GetChannelMuteStatus
**				Added BAEOutputMixer::DisengageAudio & BAEOutputMixer::ReengageAudio
**	7/28/99		Added BAEOutputMixer::GetVersionFromAudioFile
**	8/3/99		Added BAESound::SaveFile
**	8/12/99		Added BAE_RESOURCE_NOT_FOUND
**	8/28/99		Added BAE_RAW_PCM to BAEFileType. Changed BAEOutputMixer::StartOutputToFile to accept
**				a BAEFileType. Supports BAE_RAW_PCM only.
**	8/28/99		Added BAEOutputMixer::SetAudioLatency
**	9/7/99		Added mSongFinished to BAEMidiSong. In BAENoiseGroup & BAEMidiSong changed member name to
**				mUserReference rather than the same name as the variable passed into the constructor.
*/
/*****************************************************************************/

/*
	Description of use:
	
	Create one BAEOutputMixer and only one. Call open to setup the mixing engine. When
	done call close or the destructor method. When you call Open, make sure you allocate
	voices for Midi and Sound effects. If you allocate 0 sound effects no effects will 
	play, and the same for midi. Pass in the path of the sound bank you want to work as 
	your default. The BAEPathName path name is a FSSpec for Macintosh, and everyone 
	else is a 'C' string with a complete path name. If you want to change instrument
	libraries, call ChangeAudioFile. Once a song is loaded, the file is no longer
	needed, and you can changed them at will.
	
	If you have your sample library, in ROM for example, and you want to set up the engine
	to access that data but not duplicate it, then call the Open method from the BAEOutputMixer
	with NULL for the sound bank. Then call the method ChangeAudioFileToMemory and point
	it to the block of memory that represents the file. The data will not be copied
	unless its compressed. Do not release the memory block until you are finished.

	To play a midi file create a BAEMidiSong and call LoadFromFile or LoadFromBank
	then call Start.

	You can have as many of BAEMidiSong objects as memory will hold, but you can only
	call Start on 4 of them. Start will return an error if you are trying to play to
	many. Once the midi file is playing you can call any method from the BAEMidiSynth
	class except for Open. This allows you to modify the song in realtime. If you call open
	then the BAEMidiSong object will basicly become a BAEMidiSynth object.
	
	To drive the synthesizer directly. ie NoteOn, NoteOff, etc; create a BAEMidiSynth
	and call Open. Then LoadInstrument for each instrument you want, unless you passed 
	TRUE to Open. You can then call NoteOn, NoteOff, etc. To post events in the future, 
	call GetTick then add the amount of time you want to post. Remember all times are 
	in microseconds.
	
	To play sound effects create a BAESound. You can have as many of these as you like.
	If you dispose of the BAESound before the sound is done playing, the sound will be
	stopped. First you create an BAESound for every sound object you want to play, then
	you load the object with the type of sample data required, ie. from a resource; from
	a file, from a memory pointer, then you can play it. It will always copy the data
	from the source data. When you destroy the object the sample data will go as well.
	
	To stream sound, create a BAESoundStream. You can have up the total number of sound
	effects you allocated when you created the BAEOutputMixer. Call the SetupFileStream method
	with your choice of file types, file, and buffer size, then call the Start method to start
	the stream. You must call the  BAEOutputMixer::ServiceStreams method as much as possible. 
	If you don't you'll get hicups in the sound. Once the stream is running, you can control 
	various features from the BAESoundStream class. If you want to stream custom data you'll 
	use the SetupCustomStream method with a callback that prompts you when to allocate, 
	destroy memory and fill memory.

	Remember to dispose of all of your BAESound, BAESoundStream, BAEMidiSynth and 
	BAEMidiSong objects before disposing of the BAEOutputMixer.

*/

#ifndef BAE_AUDIO
#define BAE_AUDIO

// types
enum BAETerpMode
{
	BAE_DROP_SAMPLE = 0,
	BAE_2_POINT_INTERPOLATION,
	BAE_LINEAR_INTERPOLATION
};

// Quality types
enum BAEQuality
{
	BAE_8K = 0,								// output at 8 kHz
	BAE_11K,								// 11 kHz
	BAE_11K_TERP_22K,						// 11 kHz interpolated to 22 kHz
	BAE_22K,								// 22 kHz
	BAE_22K_TERP_44K,						// 22 kHz interpolated to 44 kHz
	BAE_24K,								// 24 kHz
	BAE_44K,								// 44 kHz
	BAE_48K									// 48 kHz
};

// Modifier types
#define BAE_NONE				0L
#define BAE_USE_16				(1<<0L)		// use 16 bit output
#define BAE_USE_STEREO			(1<<1L)		// use stereo output
#define BAE_DISABLE_REVERB		(1<<2L)		// disable reverb
#define BAE_STEREO_FILTER		(1<<3L)		// if stereo is enabled, use a stereo filter
typedef long BAEAudioModifiers;

enum BAEReverbType 
{
	BAE_REVERB_NO_CHANGE = 0,				// don't change the mixer settings
	BAE_REVERB_NONE = 1,
	BAE_REVERB_TYPE_1 = 1,					// None
	BAE_REVERB_TYPE_2,						// Igor's Closet
	BAE_REVERB_TYPE_3,						// Igor's Garage
	BAE_REVERB_TYPE_4,						// Igor\xD5s Acoustic Lab
	BAE_REVERB_TYPE_5,						// Igor's Cavern
	BAE_REVERB_TYPE_6,						// Igor's Dungeon
	BAE_REVERB_TYPE_7,						// Small reflections Reverb used for WebTV
	BAE_REVERB_TYPE_8,						// Early reflections (variable verb)
	BAE_REVERB_TYPE_9,						// Basement (variable verb)
	BAE_REVERB_TYPE_10,						// Banquet hall (variable verb)
	BAE_REVERB_TYPE_11,						// Catacombs (variable verb)
	BAE_REVERB_TYPE_COUNT
};

// used by the BAEExporter code
enum BAEEncryptionType
{
	BAE_ENCRYPTION_NONE,
	BAE_ENCRYPTION_NORMAL,
	BAE_ENCRYPTION_TYPE_COUNT
};
enum BAECompressionType
{
	BAE_COMPRESSION_NONE,
	BAE_COMPRESSION_LOSSLESS,
	BAE_COMPRESSION_IMA,
	BAE_COMPRESSION_MPEG_32,
	BAE_COMPRESSION_MPEG_40,
	BAE_COMPRESSION_MPEG_48,
	BAE_COMPRESSION_MPEG_56,
	BAE_COMPRESSION_MPEG_64,
	BAE_COMPRESSION_MPEG_80,
	BAE_COMPRESSION_MPEG_96,
	BAE_COMPRESSION_MPEG_112,
	BAE_COMPRESSION_MPEG_128,
	BAE_COMPRESSION_MPEG_160,
	BAE_COMPRESSION_MPEG_192,
	BAE_COMPRESSION_MPEG_224,
	BAE_COMPRESSION_MPEG_256,
	BAE_COMPRESSION_MPEG_320,
	BAE_COMPRESSION_TYPE_COUNT
};

typedef void *		BAEPathName;			// this a pointer to 
											// a 'C' string
											// ie. "C:\FOLDER\FILE" for WinOS
											// and is a FSSpec for MacOS

/* Common errors returned from the system */
enum BAEResult
{
	BAE_NO_ERROR = 0,
	BAE_PARAM_ERR = 10000,
	BAE_MEMORY_ERR,
	BAE_BAD_INSTRUMENT,
	BAE_BAD_MIDI_DATA,
	BAE_ALREADY_PAUSED,
	BAE_ALREADY_RESUMED,
	BAE_DEVICE_UNAVAILABLE,
	BAE_NO_SONG_PLAYING,
	BAE_STILL_PLAYING,
	BAE_TOO_MANY_SONGS_PLAYING,
	BAE_NO_VOLUME,
	BAE_GENERAL_ERR,
	BAE_NOT_SETUP,
	BAE_NO_FREE_VOICES,
	BAE_STREAM_STOP_PLAY,
	BAE_BAD_FILE_TYPE,
	BAE_GENERAL_BAD,
	BAE_BAD_FILE,
	BAE_NOT_REENTERANT,
	BAE_BAD_SAMPLE,
	BAE_BUFFER_TO_SMALL,
	BAE_BAD_BANK,
	BAE_BAD_SAMPLE_RATE,
	BAE_TOO_MANY_SAMPLES,
	BAE_UNSUPPORTED_FORMAT,
	BAE_FILE_IO_ERROR,
	BAE_SAMPLE_TO_LARGE,
	BAE_UNSUPPORTED_HARDWARE,
	BAE_ABORTED,
	BAE_FILE_NOT_FOUND,
	BAE_RESOURCE_NOT_FOUND,
	
	BAE_ERROR_COUNT
};

enum BAEInfoType
{
	TITLE_INFO = 0,				// Title
	PERFORMED_BY_INFO,			// Performed by
	COMPOSER_INFO,				// Composer(s)
	COPYRIGHT_INFO,				// Copyright Date
	PUBLISHER_CONTACT_INFO,		// Publisher Contact Info
	USE_OF_LICENSE_INFO,		// Use of License
	LICENSED_TO_URL_INFO,		// Licensed to what URL
	LICENSE_TERM_INFO,			// License term
	EXPIRATION_DATE_INFO,		// Expiration Date
	COMPOSER_NOTES_INFO,		// Composer Notes
	INDEX_NUMBER_INFO,			// Index Number
	GENRE_INFO,					// Genre
	SUB_GENRE_INFO,				// Sub-genre
	TEMPO_DESCRIPTION_INFO,		// Tempo description
	ORIGINAL_SOURCE_INFO,		// Original source

	INFO_TYPE_COUNT				// always count of type InfoType
};
#define BAEInfoType	BAEInfoType

// These are embedded text events inside of midi files
enum BAEMetaType
{
	GENERIC_TEXT_TYPE	=	1,	// generic text
	COPYRIGHT_TYPE		=	2,	// copyright text
	TRACK_NAME_TYPE		=	3,	// track name of sequence text
	LYRIC_TYPE			=	5,	// lyric text
	MARKER_TYPE	 		=	6,	// marker text (BAE supports LOOPSTART, LOOPEND, LOOPSTART= commands)
	CUE_POINT_TYPE		=	7	// cue point text
};

enum BAEFileType
{
	BAE_INVALID_TYPE = 0,
	BAE_AIFF_TYPE = 1,
	BAE_WAVE_TYPE,
	BAE_MPEG_TYPE,
	BAE_AU_TYPE,

	// meta types
	BAE_GROOVOID,
	BAE_RMF,
	BAE_RAW_PCM
};

// what type of CPU are we running on. 
enum BAECPUType
{
	BAE_CPU_UNKNOWN	= 0,
	BAE_CPU_POWERPC,
	BAE_CPU_SPARC,
	BAE_CPU_JAVA,
	BAE_CPU_MIPS,
	BAE_CPU_INTEL_PENTIUM,
	BAE_CPU_INTEL_PENTIUM3,
	BAE_CPU_CRAY_XMP3,
	
	BAE_CPU_COUNT
};

// All volume levels are a 16.16 fixed value. 1.0 is 0x10000. Use can use this macro
// to convert a floating point number to a fixed value, and visa versa

typedef long								BAE_FIXED;				// fixed point value can be signed
typedef unsigned long						BAE_UNSIGNED_FIXED;		// fixed point value unsigned 

#define	BAE_FIXED_1							0x10000L
#define FLOAT_TO_FIXED(x)					((BAE_FIXED)((double)(x) * 65536.0))	// the extra long is for signed values
#define FIXED_TO_FLOAT(x)					((double)(x) / 65536.0)
#define LONG_TO_FIXED(x)					((BAE_FIXED)(x) * BAE_FIXED_1)
#define FIXED_TO_LONG(x)					((x) / BAE_FIXED_1)
#define	FIXED_TO_SHORT(x)					((short)((x) / BAE_FIXED_1))

#define FLOAT_TO_UNSIGNED_FIXED(x)			((BAE_UNSIGNED_FIXED)((double)(x) * 65536.0))	// the extra long is for signed values
#define UNSIGNED_FIXED_TO_FLOAT(x)			((double)(x) / 65536.0)
#define LONG_TO_UNSIGNED_FIXED(x)			((BAE_UNSIGNED_FIXED)(x) * BAE_FIXED_1)
#define UNSIGNED_FIXED_TO_LONG(x)			((x) / BAE_FIXED_1)
#define	UNSIGNED_FIXED_TO_SHORT(x)			((unsigned short)((x) / BAE_FIXED_1))

#define RATIO_TO_FIXED(a,b)					(LONG_TO_FIXED(a) / (b))
#define FIXED_TO_LONG_ROUNDED(x)			FIXED_TO_LONG((x) + BAE_FIXED_1 / 2)
#define FIXED_TO_SHORT_ROUNDED(x)			FIXED_TO_SHORT((x) + BAE_FIXED_1 / 2)

#define UNSIGNED_RATIO_TO_FIXED(a,b)		(LONG_TO_UNSIGNED_FIXED(a) / (b))
#define UNSIGNED_FIXED_TO_LONG_ROUNDED(x)	UNSIGNED_FIXED_TO_LONG((x) + BAE_FIXED_1 / 2)
#define UNSIGNED_FIXED_TO_SHORT_ROUNDED(x)	UNSIGNED_FIXED_TO_SHORT((x) + BAE_FIXED_1 / 2)


typedef char			BAE_BOOL;
typedef unsigned long	BAE_INSTRUMENT;			// reference to an instrument
typedef long			BAE_EVENT_REFERENCE;	// reference to an idle time event

#ifndef TRUE
	#define TRUE	1
#endif
#ifndef FALSE
	#define	FALSE	0
#endif

#undef NULL
#define	NULL	0L

// Callback for midi controllers
typedef void		(*BAEControlerCallbackPtr)(void * pReference, short int channel, short int track, short int controler, 
												short int value);
// Callback for task called at 11 ms intervals
typedef void		(*BAETaskCallbackPtr)(void *pReference);

// Callback from midi sequencer
typedef void		(*BAETimeCallbackPtr)(void *pReference, unsigned long currentMicroseconds, unsigned long currentMidiClock);

// Callback from midi sequencer when meta events are encountered
typedef void		(*BAEMetaEventCallbackPtr)(void * pReference, BAEMetaType type, void *pMetaText, long metaTextLength);

// Callback when object is finished
typedef void		(*BAEDoneCallbackPtr)(void * pReference);

// Callback for BAESound double buffer playback
typedef void		(*BAEDoubleBufferCallbackPtr)(void * pReference, void *pWhichBufferFinished, unsigned long *pNewBufferSize);


// Callback when object needs to continue a loop
typedef BAE_BOOL	(*BAELoopDoneCallbackPtr)(void * pReference);

// The void will become either a short int, or a unsigned char depending upon how the mixer is
// setup
typedef void		(*BAEOutputCallbackPtr)(void *samples, long sampleSize, long channels, unsigned long lengthInFrames);

// Callback for sample frame calls
typedef void		(*BAESampleFrameCallbackPtr)(void * pReference, unsigned long sampleFrame);

// General defines
enum 
{
	BAE_MAX_VOICES				=	64,		// total number of voices. This is shared amongst
											// all BAESound's, BAESoundStream's, and BAEMidiSong's
	BAE_MAX_INSTRUMENTS 		=	768,
	BAE_MAX_SONGS				=	16,
	BAE_MAX_MIDI_VOLUME			=	127,
	BAE_MAX_MIDI_TRACKS			=	65,		// 64 midi tracks, plus 1 tempo track

	BAE_DEFAULT_PROGRAM			=	0,
	BAE_DEFAULT_BANK			=	0,
	
	BAE_MIN_STREAM_BUFFER_SIZE	=	30000,

	BAE_FULL_LEFT_PAN			=	(-63),
	BAE_CENTER_PAN				=	0,
	BAE_FULL_RIGHT_PAN			=	(63)
};

// BAE mixer version number. If you call BAEOutputMixer::GetMixerVersion, the values returned should match
// these constants. If they don't then your header files and libraries don't match.
enum
{
	BAE_VERSION_MAJOR		=	1,
	BAE_VERSION_MINOR		=	5,
	BAE_VERSION_SUB_MINOR	=	1
};

enum BAEVoiceType
{
	BAE_UNKNOWN = 0,			// Voice is an undefined type
	BAE_MIDI_PCM_VOICE,			// Voice is a PCM voice used by MIDI
	BAE_SOUND_PCM_VOICE			// Voice is a PCM sound effect used by BAESound/BAESoundStream
};


struct BAEAudioInfo
{
	short int		voicesActive;						// number of voices active
	short int		voice[BAE_MAX_VOICES];				// voice index
	BAEVoiceType	voiceType[BAE_MAX_VOICES];			// voice type
	short int		instrument[BAE_MAX_VOICES];			// current instruments
	short int		midiVolume[BAE_MAX_VOICES];			// current volumes
	short int		scaledVolume[BAE_MAX_VOICES];		// current scaled volumes
	short int		channel[BAE_MAX_VOICES];			// current channel
	short int		midiNote[BAE_MAX_VOICES];			// current midi note
	long			userReference[BAE_MAX_VOICES];		// userReference associated with voice
};
typedef struct BAEAudioInfo BAEAudioInfo;



struct BAESampleInfo
{
	unsigned short		bitSize;			// number of bits per sample
	unsigned short		channels;			// number of channels (1 or 2)
	unsigned short		baseMidiPitch;		// base Midi pitch of recorded sample ie. 60 is middle 'C'
	unsigned long		waveSize;			// total waveform size in bytes
	unsigned long		waveFrames;			// number of frames
	unsigned long		startLoop;			// start loop point offset
	unsigned long		endLoop;			// end loop point offset
	BAE_UNSIGNED_FIXED	sampledRate;		// fixed 16.16 value for recording
};
typedef struct BAESampleInfo BAESampleInfo;


// NOTE:
//	This event modal is designed to be used during interrupts. Which means no allocation of memory can happen
//	to create and setup these events.
//	They are stored first come first served order and processed that way.

enum
{
	BAE_MAX_EVENTS		=	64	// max events at once
};

// BAE mixer class. Can only be one mixer object per sound card.
class BAEOutputMixer 
{
public:
friend class BAERmfSong;
friend class BAEMidiSong;
friend class BAEMidiSynth;
friend class BAESound;
friend class BAESoundStream;
friend class BAENoise;

		BAEOutputMixer();
virtual	~BAEOutputMixer();

		// number of devices. ie different versions of the BAE connection. DirectSound and waveOut
		// return number of devices. ie 1 is one device, 2 is two devices.
		// NOTE: This function can be called before Open is called
		long 			GetMaxDeviceCount(void);

		// set current device. should be a number from 0 to BAEOutputMixer::GetMaxDeviceCount()
		// deviceParameter is a pointer to device specific info. It will
		// be NULL if there's nothing interesting. For Windows Device 1 (DirectSound) it
		// is the window handle that DirectSound will attach to. Pass NULL, if you don't know
		// what is correct.
		void 			SetCurrentDevice(long deviceID, void *deviceParameter = NULL);

		// get current device. deviceParameter is a pointer to device specific info. It will
		// be NULL if there's nothing interesting. For Windows Device 1 (DirectSound) it
		// is the window handle that DirectSound will attach to.
		long 			GetCurrentDevice(void *deviceParameter = NULL);

		// get device name
		// NOTE:	This function can be called before Open()
		//			Format of string is a zero terminated comma delinated C string.
		//			"platform,method,misc"
		//	example	"MacOS,Sound Manager 3.0,SndPlayDoubleBuffer"
		//			"WinOS,DirectSound,multi threaded"
		//			"WinOS,waveOut,multi threaded"
		//			"WinOS,VxD,low level hardware"
		//			"WinOS,plugin,Director"
		void			GetDeviceName(long deviceID, char *cName, unsigned long cNameLength);
		
		// Will return TRUE if the BAEMixer is already open
		BAE_BOOL			IsOpen(void) const;

		// Return status about current device. Mixer not need to be "opened" to determine
		// this information.
		BAE_BOOL			Is16BitSupported(void) const;
		BAE_BOOL			Is8BitSupported(void) const;
		BAE_BOOL			IsStereoSupported(void) const;

		// Verify that the file passed in, is a valid audio bank file for BAE
		BAEResult				ValidateAudioFile(BAEPathName pAudioPathName);

		// open the mixer. Only one of these.
		// Note: it is valid to pass NULL to pAudioPathName. This means open the mixer
		// for sound effects and other types of mixing, but Midi and built in songs will
		// not work. If you want to associate a patch file to an open mixer, call 
		// ChangeAudioFile or ChangeAudioFileToMemory.
		BAEResult				Open(BAEPathName pAudioPathName = NULL,
									BAEQuality q = BAE_22K,
									BAETerpMode t = BAE_LINEAR_INTERPOLATION,
									BAEReverbType r = BAE_REVERB_TYPE_4,
									BAEAudioModifiers am = (BAE_USE_16 | BAE_USE_STEREO),
									short int maxMidiVoices = 56,
									short int maxSoundVoices = 4,
									short int mixLevel = 8,
									BAE_BOOL engageAudio = TRUE
								);
		// close the mixer
		void				Close(void);

		// reconfigure, by changing sample rates, interpolation modes, bit depth, etc based
		// upon performance of host computer
		void				PerformanceConfigure(void);

		// return a value that measures CPU performance
		unsigned long		MeasureCPUPerformance(void);

		// return CPU type this mixer is running on
		BAECPUType			GetCPUType(void);

		// return version of the BAE software.
		void				GetMixerVersion(short int *pVersionMajor, short int *pVersionMinor, short int *pVersionSubMinor);

		// Flush read in or created cache for current AudioFile. TRUE allows cache, FALSE does not.
		void				SetCacheStatusForAudioFile(BAE_BOOL enableCache);
		BAE_BOOL			GetCacheStatusForAudioFile(void);

		// Flush read in or created cache for current AudioFile. TRUE allows cache, FALSE does not.
		void				SetCacheStatusAudioFileCache(BAE_BOOL enableCache);

		// change audio file
		BAEResult				ChangeAudioFile(BAEPathName pAudioPathName);

		// change audio file to work from memory. Assumes file was loaded into memory
		BAEResult				ChangeAudioFileToMemory(void * pAudioFile, unsigned long fileSize);

		// Get default bank URL from current bank
		BAEResult				GetURLFromAudioFile(char *pURL, unsigned long urlLength);
		// Get default bank name from current bank
		BAEResult 				GetNameFromAudioFile(char *cName, unsigned long cLength);

		// Get default bank name from specific bank
		BAEResult				GetNameFromAudioFile(BAEPathName pAudioPathName, char *cName, unsigned long cLength);

		// get audio file version numbers from specific bank
		BAEResult				GetVersionFromAudioFile(BAEPathName pAudioPathName, short int *pVersionMajor, short int *pVersionMinor, short int *pVersionSubMinor);

		// get audio file version numbers
		void					GetVersionFromAudioFile(short int *pVersionMajor, short int *pVersionMinor, short int *pVersionSubMinor);

		// Get names of songs that are included in the audio file. Call successively until
		// the name is zero
		void					GetSongNameFromAudioFile(char *cSongName, long *pID = NULL, BAEFileType *pSongType = NULL);

		// Get names of samples that are included in the audio file. Call successively until
		// the name is zero
		void					GetSampleNameFromAudioFile(char *cSampleName, long *pID);

		// Get names of instruments that are included in the audio file. Call successively until
		// the name is zero
		void					GetInstrumentNameFromAudioFile(char *cInstrumentName, long *pID);

		// Get names of instruments that are included in the audio file, referenced by ID only. Will return
		// and error if instrument not found.
		BAEResult				GetInstrumentNameFromAudioFileFromID(char *cInstrumentName, long theID);

		// change audio modes
		BAEResult				ChangeAudioModes(BAEQuality q, BAETerpMode t, BAEAudioModifiers am);

		// change voice allocation
		BAEResult				ChangeSystemVoices(	short int maxMidiVoices,
										short int maxSoundVoices,
										short int mixLevel);
		// get current system tick in microseconds
 		unsigned long		GetTick(void);

		// set current system audio lantency. This is platform specific. May not work on all
		// platforms. Will return BAE_NOT_SETUP if not supported. Make sure you've set the current
		// device with SetCurrentDevice prior to calling this.
		BAEResult			SetAudioLatency(unsigned long requestedLatency);

		// get current system audio lantency. This is platform specific. Result is in
		// microseconds
		unsigned long		GetAudioLatency(void);

		// change reverb mode
		void				SetReverbType(BAEReverbType r);
		BAEReverbType		GetReverbType(void);

		// get and set the master mix volume. A volume level of 1.0
		// is normal, and volume level of 4.0 will overdrive 4 times
		void				SetMasterVolume(BAE_UNSIGNED_FIXED theVolume);
		BAE_UNSIGNED_FIXED	GetMasterVolume(void) const;

		// get and set the hardware volume. A volume level of 1.0 is
		// considered hardware full volume. Overdriving is up to the particular
		// hardware platform
		void				SetHardwareVolume(BAE_UNSIGNED_FIXED theVolume);
		BAE_UNSIGNED_FIXED	GetHardwareVolume(void) const;

		// get and set the hardware balance. Use -1.0 for full left, and 1.0 for full
		// right; use 0 for center
		void				SetHardwareBalance(BAE_FIXED theVolume);
		BAE_FIXED			GetHardwareBalance(void) const;

		// get and set the master sound effects volume. A volume level of 1.0
		// is normal, and volume level of 4.0 will overdrive 4 times
		void				SetMasterSoundEffectsVolume(BAE_UNSIGNED_FIXED theVolume);
		BAE_UNSIGNED_FIXED	GetMasterSoundEffectsVolume(void) const;

		// display feedback information
		// This will return the number of samples stored into the pLeft and pRight
		// arrays. Usually 1024. This returns the current data points being sent
		// to the hardware.
		short int			GetAudioSampleFrame(short int *pLeft, short int *pRight) const;

		// Get realtime information about the current synthisizer state
		void				GetRealtimeStatus(BAEAudioInfo *pStatus) const;

		// Fill an array of 16 BAE_BOOL types with all songs playing particular status
		void				GetChannelMuteStatus(BAE_BOOL *pChannels);
		void				GetChannelSoloStatus(BAE_BOOL *pChannels);

		// is the mixer connected to the audio hardware
		BAE_BOOL			IsAudioEngaged(void);
		// disengage from audio hardware
		BAEResult			DisengageAudio(void);
		// reengage to audio hardware
		BAEResult			ReengageAudio(void);

		// Set a interrupt level task callback
		void				SetTaskCallback(BAETaskCallbackPtr pCallback, void *reference);
		// get task callback
		BAETaskCallbackPtr	GetTaskCallback(void)	{return pTask;}
		void				*GetTaskReference(void)	{return mTaskReference;}

		// start the task callback
		void				StartTaskCallback(void);
		// Stop the task callback
		void				StopTaskCallback(void);

		// Set a output callback. This will call your output code. This is used to modify the
		// sample output before its sent to the hardware. Be very careful here. Don't use to
		// much time or the audio will skip
		void				SetOutputCallback(BAEOutputCallbackPtr pCallback);
		// start the output callback
		void				StartOutputCallback(void);
		// Stop the output callback
		void				StopOutputCallback(void);

		// start saving audio output to a file
		BAEResult 			StartOutputToFile(BAEPathName pAudioOutputFile, BAEFileType outputType = BAE_RAW_PCM);

		// Stop saving audio output to a file
		void				StopOutputToFile(void);

		// once started saving to a file, call this to continue saving to file
		BAEResult			ServiceAudioOutputToFile(void);

		// Call this during idle time to service audio streams, and other
		// idle time processes
		void				ServiceIdle(void);

		// get in realtime CPU load in microseconds used to create 11 ms worth
		// of sample data.
		unsigned long		GetCPULoadInMicroseconds(void);
		unsigned long		GetCPULoadInPercent(void);

		BAEAudioModifiers	GetModifiers(void);
		BAETerpMode			GetTerpMode(void);
		BAEQuality			GetQuality(void);

		short int			GetMidiVoices(void);
		short int			GetSoundVoices(void);
		short int			GetMixLevel(void);

		// events to be called during a ServiceIdle call. All event functions are staticly defined because
		// we need the ability to call them without allocating memory. Then the actaul event will be processed
		// during the ServiceIdle call in the main thread which can safely allocate memory, or can calls
		// code outside of BAE that allocates memory.
		BAE_EVENT_REFERENCE	AddEventLoadInstrument(BAEMidiSynth *object, BAE_INSTRUMENT instrument);

		// note on that checks to see if an instrument needs to be loaded. 
		BAE_EVENT_REFERENCE	AddEventNoteOnWithLoad(BAEMidiSynth *object, unsigned char channel, 
																		unsigned char note, 
																		unsigned char velocity,
																		unsigned long time = 0);

		BAE_EVENT_REFERENCE	AddEventMetaEvent(BAEMidiSong *object, BAEMetaEventCallbackPtr callback, BAEMetaType type,
													void *pReference, void *pMetaText, long metaTextLength);
		BAE_EVENT_REFERENCE	AddEventObjectDone(BAENoise *object, BAEDoneCallbackPtr callback, void *pReference);
		BAE_EVENT_REFERENCE	AddEventControlerEvent(BAEMidiSong *object, BAEControlerCallbackPtr callback, void *pReference,
													short int channel, short int track, short int controler, short int value);
		void				RemoveEvent(BAE_EVENT_REFERENCE reference);

private:
		enum BAEEventType
		{
			BAE_DEAD_EVENT		= 0,
			BAE_ALLOCATING,
			BAE_LOAD_INSTRUMENT,
			BAE_NOTE_ON_WITH_LOAD,
			BAE_MIDI_META_EVENT,		// BAEMidiSong & BAERmfSong meta event
			BAE_MIDI_CONTROLER_EVENT,
			BAE_MIDI_DONE,				// BAEMidiSong callback on stop
			BAE_RMF_DONE,				// BAERmfSong callback on stop
			BAE_SOUND_DONE,				// BAESound callback on stop
			BAE_SOUND_STREAM_DONE		// BAESoundStream callback on stop
		};

		// events are in the format:
		struct BAEEvent_Generic
		{
			BAEEventType			event;
			BAENoise			*object;
		};
		typedef struct BAEEvent_Generic BAEEvent_Generic;

		// events are in the format:
		//	event, object, callback proc, pReference
		struct BAEEvent_LoadInstrument
		{
			BAEEventType			event;		// BAE_LOAD_INSTRUMENT
			BAEMidiSynth			*object;	//  object can be a BAEMidiSong or a BAERmfSong.

			BAE_INSTRUMENT			instrument;
		};
		typedef struct BAEEvent_LoadInstrument BAEEvent_LoadInstrument;

		// events are in the format:
		//	event, object, callback proc, pReference
		struct BAEEvent_NoteOnWithLoad
		{
			BAEEventType			event;		// BAE_NOTE_ON_WITH_LOAD
			BAEMidiSynth			*object;	//  object can be a BAEMidiSong or a BAERmfSong.

			unsigned char			channel;
			unsigned char			note;
			unsigned char			velocity;
			unsigned long			time;
		};
		typedef struct BAEEvent_NoteOnWithLoad BAEEvent_NoteOnWithLoad;

		// events are in the format:
		//	event, object, callback proc, type, pReference, pMetaText, metaTextLength
		struct BAEEvent_MetaEvent
		{
			BAEEventType			event;		// BAE_MIDI_META_EVENT
			BAEMidiSong				*object;

			// callback. Make sure all memory pointers will not go away prior to event taking place.
			BAEMetaEventCallbackPtr	callback;
			BAEMetaType			type;
			void					*pReference;
			void					*pMetaText;
			long					metaTextLength;
		};
		typedef struct BAEEvent_MetaEvent BAEEvent_MetaEvent;

		// events are in the format:
		//	event, object, callback proc, pReference
		struct BAEEvent_ObjectDone
		{
			BAEEventType			event;		// BAE_MIDI_DONE, BAE_RMF_DONE, BAE_SOUND_DONE, BAE_SOUND_STREAM_DONE
			BAENoise			*object;	//  object will be morphed into correct class prior to calling

			// callback. Make sure all memory pointers will not go away prior to event taking place.
			BAEDoneCallbackPtr		callback;
			void					*pReference;
		};
		typedef struct BAEEvent_ObjectDone BAEEvent_ObjectDone;

		// events are in the format:
		//	event, object, callback proc, type, pReference, pMetaText, metaTextLength
		struct BAEEvent_ControlerEvent
		{
			BAEEventType			event;		// BAE_MIDI_CONTROLER_EVENT
			BAEMidiSong				*object;

			// callback. Make sure all memory pointers will not go away prior to event taking place.
			BAEControlerCallbackPtr	callback;
			void					*pReference;
			short int				channel;
			short int				track;
			short int				controler;
			short int				value;
		};
		typedef struct BAEEvent_ControlerEvent BAEEvent_ControlerEvent;

		BAEEvent_Generic * GetNextStorableIdleEvent(void);
		BAEEvent_Generic * GetNextReadOnlyIdleEvent(void);

		void				*m_pEvents[BAE_MAX_EVENTS];
		short int			mHeadEvent, mTailEvent;

		BAEQuality			iQuality;
		BAETerpMode			iTerpMode;

		BAEReverbType		iReverbMode;
		BAEAudioModifiers	iModifiers;

		short int			iMidiVoices;
		short int			iSoundVoices;
		short int			iMixLevel;

		short int			songNameCount;
		BAEFileType			songNameType;

		short int			sampleNameCount;
		short int			instrumentNameCount;

		// hooks
		BAETaskCallbackPtr		pTask;
		void					*mTaskReference;
		BAEOutputCallbackPtr	pOutput;

		// banks
		void					*mOpenAudioFiles;
		unsigned short int		mOpenAudioFileCount;
		BAE_BOOL				mCacheStatus;

		// link to all objects
		void				*pTop;

		BAE_BOOL			mIsAudioEngaged;

		BAE_BOOL			mWritingToFile;
		BAEFileType			mWriteToFileType;
		void				*mWritingToFileReference;
		void				*mWritingDataBlock;
};

// base class
class BAENoise
{
friend class BAERmfSong;
friend class BAEMidiSong;
friend class BAEMidiSynth;
friend class BAESound;
friend class BAESoundStream;
friend class BAEOutputMixer;
friend class BAENoiseGroup;
public:
		enum NoiseType
		{
			SOUND_NOISE	= 0,
			SOUND_STREAM_NOISE,
			RMF_NOISE,
			MIDI_NOISE,
			MIDI_FILE_NOISE,
			MOD_NOISE,
			GROUP_NOISE
		};

		BAENoise(BAEOutputMixer *pBAEOutputMixer, char const *cName, NoiseType type);
virtual	~BAENoise();
		BAEOutputMixer	*GetMixer(void);
		const char		*GetName(void)	const	{return mName;}
		const NoiseType	GetType(void) const		{return mType;}

		// virtual methods that must be defined on all classes
virtual void				SetVolume(BAE_UNSIGNED_FIXED newVolume) = 0;
virtual	BAE_UNSIGNED_FIXED	GetVolume(void) = 0;
virtual void				SetStereoPosition(short int stereoPosition) = 0;
virtual	short int			GetStereoPosition(void) = 0;

private:
		void			SetType(NoiseType newType);

		NoiseType		mType;					// type of object;
		char			mName[64];				// name of object
		BAEOutputMixer	*mAudioMixer;
		BAENoise	*pNext;					// link of objects starting from a mixer
		BAENoise	*pGroupNext;			// link of objects starting from a group

		BAE_EVENT_REFERENCE	mEventReference;	// will be non-zero if this object is in an event queue
};

// Sound effects
class BAESound : public BAENoise
{
friend class BAENoiseGroup;
public:
					BAESound(BAEOutputMixer *pBAEOutputMixer, 
								char const *cName = 0L, void * userReference = 0);
	virtual			~BAESound();

	void					*GetReference(void)				const	{return mUserReference;}
	BAEDoneCallbackPtr		GetDoneCallback(void)			const	{return mDoneCallback;}
	BAELoopDoneCallbackPtr	GetLoopDoneCallback(void)			const	{return mLoopDoneCallback;}
	void					*GetDoneCallbackReference(void)	const	{return mCallbackReference;}
	
	BAEResult			AddSampleFrameCallback(unsigned long frame, BAESampleFrameCallbackPtr pCallback, void * pReference);
	BAEResult			RemoveSampleFrameCallback(unsigned long frame, BAESampleFrameCallbackPtr pCallback);
	
	// load a sample from a raw pointer. This will make a copy of the passed data,
	// so you can dispose of your origial pointer once this has been called.
	BAEResult			LoadCustomSample(void * sampleData,				// pointer to audio data
									unsigned long frames, 			// number of frames of audio
									unsigned short int bitSize, 	// bits per sample 8 or 16
									unsigned short int channels, 	// mono or stereo 1 or 2
									BAE_UNSIGNED_FIXED rate, 		// 16.16 fixed sample rate
									unsigned long loopStart, 		// loop start in frames
									unsigned long loopEnd);			// loop end in frames

	// load a sample from a resource 'snd' memory block.The memory will be deallocated when this
	// object is destroyed. Call start once loaded to start the playback.
	BAEResult			LoadResourceSample(void *pResource, unsigned long resourceSize);

	// load a sample playing from a formatted file. The memory will be deallocated when this
	// object is destroyed. Call start once loaded to start the playback.
	BAEResult			LoadFileSample(BAEPathName pWaveFilePath, BAEFileType fileType);

	// save a loaded file
	BAEResult			SaveFile(BAEPathName pFile, BAEFileType fileType);

	// load a sample playing from a formatted block of memory. The memory will be deallocated 
	// when this object is destroyed. Call start once loaded to start the playback.
	BAEResult			LoadMemorySample(void *pMemoryFile, unsigned long memoryFileSize, BAEFileType fileType);

	// load a sample playing from the current audio file. The memory will be deallocated when this
	// object is destroyed.. Call start once loaded to start the playback.
	BAEResult			LoadBankSample(char *cName);

	// currently paused
	BAE_BOOL		IsPaused(void);

	// pause BAESound from playback
	void			Pause(void);

	// resume BAESound where paused
	void			Resume(void);

	// fade from source volume, to dest volume in time miliseconds. Always async
	void			FadeFromToInTime(BAE_FIXED sourceVolume, BAE_FIXED destVolume, BAE_FIXED timeInMiliseconds);

	// fade sample from current volume to silence
	void			Fade(BAE_BOOL doAsync = FALSE);

	// fade from current volume to specified volume
	void			FadeTo(BAE_FIXED destVolume, BAE_BOOL doAsync = FALSE);

	// This will setup a BAESound once data has been loaded with one of the Load... calls. Call __Start to start playing
	// NOTE: This is note final. It will change
	BAEResult			__Setup(BAE_UNSIGNED_FIXED sampleVolume = BAE_FIXED_1, 		// sample volume	(1.0)
							short int stereoPosition = BAE_CENTER_PAN,			// stereo placement -63 to 63
							void * refData = NULL, 								// callback reference
							BAELoopDoneCallbackPtr pLoopContinueProc = NULL,
							BAEDoneCallbackPtr pDoneProc = NULL,
							unsigned long startOffsetFrame = 0L,				// starting offset in frames
							BAE_BOOL stopIfPlaying = TRUE);						// TRUE will restart sound otherwise return and error

	// This will, given all the information about a sample, will play sample memory without
	// copying the data. Be carefull and do not dispose of the memory associated with this sample
	// while its playing. Call __Start to start sound
	// NOTE: This is note final. It will change
	BAEResult			__SetupCustom(void * sampleData,							// pointer to audio data
									unsigned long frames, 						// number of frames of audio
									unsigned short int bitSize, 				// bits per sample 8 or 16
									unsigned short int channels, 				// mono or stereo 1 or 2
									BAE_UNSIGNED_FIXED rate, 					// 16.16 fixed sample rate
									unsigned long loopStart = 0L, 				// loop start in frames
									unsigned long loopEnd = 0L,					// loop end in frames
									BAE_UNSIGNED_FIXED sampleVolume = BAE_FIXED_1, 		// sample volume	(1.0)
									short int stereoPosition = BAE_CENTER_PAN,	// stereo placement -63 to 63
									void *refData = NULL, 						// callback reference
									BAELoopDoneCallbackPtr pLoopContinueProc = NULL,
									BAEDoneCallbackPtr pDoneProc = NULL,
									BAE_BOOL stopIfPlaying = TRUE);

	// NOTE: This is note final. It will change
	BAEResult			__Start(void);

	// This will, given all the information about a sample, will play sample memory without
	// copying the data. Be carefull and do not dispose of the memory associated with this sample
	// while its playing.
	BAEResult			StartCustomSample(void * sampleData,						// pointer to audio data
									unsigned long frames, 						// number of frames of audio
									unsigned short int bitSize, 				// bits per sample 8 or 16
									unsigned short int channels, 				// mono or stereo 1 or 2
									BAE_UNSIGNED_FIXED rate, 					// 16.16 fixed sample rate
									unsigned long loopStart = 0L, 				// loop start in frames
									unsigned long loopEnd = 0L,					// loop end in frames
									BAE_UNSIGNED_FIXED sampleVolume = BAE_FIXED_1, 		// sample volume	(1.0)
									short int stereoPosition = BAE_CENTER_PAN,	// stereo placement -63 to 63
									void *refData = NULL, 						// callback reference
									BAELoopDoneCallbackPtr pLoopContinueProc = NULL,
									BAEDoneCallbackPtr pDoneProc = NULL,
									BAE_BOOL stopIfPlaying = TRUE);

	// This will start the first buffer of audio playing, and when its finished, it will call your callback
	// and start the second buffer of audio playing, when the second buffer is finished, it will call your
	// callback again, and start the first buffer of audio playing. If you return 0 in the callback's
	// pNewBufferSize the sample will then stop.
	//
	// This is a very low level call. Since the BAEOutputMixer looks 4 samples ahead for terping, make sure that
	// your buffers have copied an extra 4 samples of the previous data, otherwise you'll get clicks. Since
	// this is being called during the mixing stage, any delays will cause the mixer to skip or even shutdown.
	// Be very CPU sensitive here.
	//
	// The best way to use this function is to block move data into the opposite pointer that is playing.
	// .ie: fill buffer1 with something, start playing, fill buffer2 with something, accecpt callback, 
	// fill buffer1 and so on. Making sure that when you fill the buffers that you copy the previous 4 samples
	// into the next buffer before your new data.
	BAEResult			StartDoubleBuffer(void *buffer1,								// pointer to first buffer
									void *buffer2,									// pointer to second buffer
									unsigned long frames, 							// number of frames of audio
									unsigned short int bitSize, 					// bits per sample 8 or 16
									unsigned short int channels, 					// mono or stereo 1 or 2
									BAE_UNSIGNED_FIXED rate, 						// 16.16 fixed sample rate
									BAE_UNSIGNED_FIXED sampleVolume = BAE_FIXED_1, 	// sample volume	(1.0)
									short int stereoPosition = BAE_CENTER_PAN,		// stereo placement -63 to 63
									void * refData = NULL, 							// callback reference
									BAEDoubleBufferCallbackPtr pDoubleBufferCallback = NULL,
									BAE_BOOL stopIfPlaying = TRUE);

	// This will start a BAESound once data has been loaded
	BAEResult			Start(BAE_UNSIGNED_FIXED sampleVolume = BAE_FIXED_1, 		// sample volume	(1.0)
							short int stereoPosition = BAE_CENTER_PAN,			// stereo placement -63 to 63
							void * refData = NULL, 								// callback reference
							BAELoopDoneCallbackPtr pLoopContinueProc = NULL,
							BAEDoneCallbackPtr pDoneProc = NULL,
							unsigned long startOffsetFrame = 0L,				// starting offset in frames
							BAE_BOOL stopIfPlaying = TRUE);						// TRUE will restart sound otherwise return and error
			
	// This will stop a BAESound
	void			Stop(BAE_BOOL startFade = FALSE);

	// get information about the sample
	BAEResult			GetInfo(BAESampleInfo *pInfo);

	// Set a call back when song is done
	void			SetDoneCallback(BAEDoneCallbackPtr pDoneProc, void * pReference);

	// Get the position of a audio stream in samples
	unsigned long	GetPlaybackPosition(void);

	// Get the mixer pointer for this sample as its being mixed. This pointer is 
	// the actual pointer used by the mixer. This will change if this object is
	// an double buffer playback.
	void *			GetSamplePointerFromMixer(void);

	// Returns TRUE or FALSE if a given BAESound is still active
	BAE_BOOL		IsPlaying(void);

	// Returns the opposite of IsPlaying
	BAE_BOOL		IsDone(void);

	// Set the volume level of a BAESound
virtual void		SetVolume(BAE_UNSIGNED_FIXED newVolume);

	// Get the volume level of a BAESound
virtual	BAE_UNSIGNED_FIXED	GetVolume(void);

	// Set the sample rate of a BAESound
	void			SetRate(BAE_UNSIGNED_FIXED newRate);

	// Get the sample rate of a BAESound
	BAE_UNSIGNED_FIXED	GetRate(void);

	// Set the stereo position of a BAESound (-63 left to 63 right, 0 is middle)
virtual void		SetStereoPosition(short int stereoPosition);

	// Set the stereo position of a BAESound (-63 left to 63 right, 0 is middle)
virtual	short int	GetStereoPosition(void);

	// Enable/Disable reverb on this particular BAESound
	void			SetReverb(BAE_BOOL useReverb);
	BAE_BOOL 		GetReverb(void);
	// get/set reverb mix amount of this particular BAESound
	short int		GetReverbAmount(void);
	void 			SetReverbAmount(short int reverbAmount);

	// get/set low pass filter amount of this particular BAESound
	// amount range is -255 to 255
	short int		GetLowPassAmountFilter(void);
	void 			SetLowPassAmountFilter(short int lowpassamount);
	// get/set resonance filter amount of this particular BAESound
	// resonance range is 0 to 256
	short int		GetResonanceAmountFilter(void);
	void 			SetResonanceAmountFilter(short int resonanceAmount);
	// get/set frequency filter amount of this particular BAESound
	// Range is 512 to 32512
	short int		GetFrequencyAmountFilter(void);
	void 			SetFrequencyAmountFilter(short int frequencyAmount);

	// Get the sample point offset by a sample frame count. This is a pointer to
	// the actual PCM audio data stored. This is used by the mixer when playing back.
	void *			GetSamplePointer(unsigned long sampleFrame = 0L);

	// set the loop points in sample frames. Pass in start==end to remove the loop point.
	BAEResult			SetSampleLoopPoints(unsigned long start = 0L, unsigned long end = 0L);

	// Get the current loop points in sample frames
	BAEResult			GetSampleLoopPoints(unsigned long *pStart, unsigned long *pEnd);


//private:
	// This function is called when a sample finishes naturally or is stopped. Note: If you
	// override this function and don't call the parent, auto voice allocation will fail.
	void					DefaultSampleDoneCallback(void);

private:
	BAEResult			PreStart(BAE_UNSIGNED_FIXED sampleVolume,
							short int stereoPosition,
							void * refData,
							BAELoopDoneCallbackPtr pLoopContinueProc,
							BAEDoneCallbackPtr pDoneProc,
							BAE_BOOL stopIfPlaying);

	BAE_BOOL				mReverbState;
	short int				mReverbAmount;
	short int				mLowPassAmount;
	short int				mResonanceAmount;
	short int				mFrequencyAmount;
	void					*mUserReference;
	unsigned long			pauseVariable;
	void					*pFileVariables;
	void					*pSoundVariables;
	void					*pSampleFrameVariable;
	BAEDoneCallbackPtr		mDoneCallback;
	BAELoopDoneCallbackPtr	mLoopDoneCallback;
	void					*mCallbackReference;
	void					*mSoundVoiceReference;
	BAE_UNSIGNED_FIXED		mSoundVolume;
	short int				mStereoPosition;

};

// Audio Sample Data Format. (ASDF)
// Support for 8, 16 bit data, mono and stereo. Can be extended for multi channel beyond 2 channels, but
// not required at the moment.
//
//	DATA BLOCK
//		8 bit mono data
//			ZZZZZZZ\xC9
//				Where Z is signed 8 bit data
//
//		16 bit mono data
//			WWWWW\xC9
//				Where W is signed 16 bit data
//
//		8 bit stereo data
//			ZXZXZXZX\xC9
//				Where Z is signed 8 bit data for left channel, and X is signed 8 bit data for right channel.
//
//		16 bit stereo data
//			WQWQWQ\xC9
//				Where W is signed 16 bit data for left channel, and Q is signed 16 bit data for right channel.
//



typedef enum
{
	BAE_STREAM_NULL				=	0,
	BAE_STREAM_CREATE,
	BAE_STREAM_DESTROY,
	BAE_STREAM_GET_DATA,
	BAE_STREAM_GET_SPECIFIC_DATA,
	BAE_STREAM_HAVE_DATA				//	Used for BAECaptureStream
} BAEStreamMessage;

// The BAEStreamObjectProc callback is called to allocate buffer memory, get the next block of data to stream and
// mix into the final audio output, and finally dispose of the memory block. All messages will happen at 
// non-interrupt time. All messages will be called with the structure BAEStreamData.
//
// INPUT:
// Message
//	BAE_STREAM_CREATE
//		Use this message to create a block a data with a length of dataLength. Keep in mind that dataLength
//		is always total number of samples,  not bytes allocated. Allocate the block of data into the Audio Sample 
//		Data Format based upon dataBitSize and channelSize. Store the pointer into pData.
//
//	BAE_STREAM_DESTROY
//		Use this message to dispose of the memory allocated. pData will contain the pointer allocated.
//		dataLength will be the sample size not the buffer size. ie. for 8 bit data use dataLength, 
//		for 16 bit mono data double dataLength.
//
//	BAE_STREAM_GET_DATA
//		This message is called whenever the streaming object needs a new block of data. Right after BAE_STREAM_CREATE
//		is called, BAE_STREAM_GET_DATA will be called twice. Fill pData with the new data to be streamed.
//		Set dataLength to the amount of data put into pData.
//
//	BAE_STREAM_GET_SPECIFIC_DATA
//		This message is optional. It will be called when a specific sample frame and length needs to be captured.
//		The method GetSampleData will call this message. If you do not want to implement this message
//		return an error of BAE_NOT_SETUP. Fill (pData) with the new sample data betweem sample frames (startSample)
//		and (endSample). Set (dataLength) to the amount of data put into (pData).
//		Note: this message will should not be called to retrive sample data for streaming. Its only used to capture
//		a range of data inside of a stream for display or other processes.
//
//	BAE_STREAM_HAVE_DATA
//		This message is used with an BAECapture object when audio is being captured into the buffer and the
//		buffer is full.
//
// OUTPUT:
// returns
//	BAE_NO_ERR
//		Everythings ok
//
//	BAE_STREAM_STOP_PLAY
//		Everything is fine, but stop playing stream
//
//	BAE_MEMORY_ERR
//		Couldn't allocate memory for buffers.
//
//	BAE_PARAM_ERR
//		General purpose error. Something wrong with parameters passed.
//
//	BAE_NOT_SETUP
//		If message BAE_STREAM_GET_SPECIFIC_DATA is called and it is not implemented you should return this error.
//

struct BAEStreamData
{
	long				userReference;		// IN for all messages. userReference is passed in at AudioStreamStart
	void				*pData;				// OUT for BAE_STREAM_CREATE, IN for BAE_STREAM_DESTROY and BAE_STREAM_GET_DATA
	unsigned long		dataLength;			// OUT for BAE_STREAM_CREATE, IN for BAE_STREAM_DESTROY. IN and OUT for BAE_STREAM_GET_DATA
	BAE_UNSIGNED_FIXED	sampleRate;			// IN for all messages. Fixed 16.16 value
	char				dataBitSize;		// IN for BAE_STREAM_CREATE. Not used elsewhere
	char				channelSize;		// IN for BAE_STREAM_CREATE. Not used elsewhere
	unsigned long		startSample;		// IN for BAE_STREAM_GET_SPECIFIC_DATA only.
	unsigned long		endSample;			// IN for BAE_STREAM_GET_SPECIFIC_DATA only
};
typedef struct BAEStreamData	BAEStreamData;

typedef BAEResult (*BAEStreamObjectProc)(BAEStreamMessage message, BAEStreamData *pAS);

// Audio Stream
class BAESoundStream : public BAENoise
{
friend class BAENoiseGroup;
public:
					BAESoundStream(BAEOutputMixer *pBAEOutputMixer,
									char const *cName = 0L, void * userReference = 0);
virtual				~BAESoundStream();

	void			*GetReference(void)	const	{return mUserReference;}
	BAEStreamObjectProc GetCallbackProc(void) const {return mCallbackProc;}

	// setup a streaming file. Does not require a callback control
	BAEResult			SetupFileStream(	BAEPathName pWaveFilePath, 
										BAEFileType fileType,
										unsigned long bufferSize,			// temp buffer to read file
										BAE_BOOL loopFile);				// TRUE will loop file

	// Multi source user config based streaming
	// This will start a streaming audio object.
	// INPUT:
	//	pProc			is a BAEStreamObjectProc proc pointer. At startup of the streaming the proc will be called
	//					with BAE_STREAM_CREATE, then followed by two BAE_STREAM_GET_DATA calls to get two buffers 
	//					of data and finally BAE_STREAM_DESTROY when finished.
	//	bufferSize		total size of buffer to work with. This will not allocate memory, instead it will call
	//					your control callback with a BAE_STREAM_CREATE with a size
	BAEResult			SetupCustomStream(	BAEStreamObjectProc pProc, 	// control callback
										unsigned long bufferSize, 		// buffer size 
										BAE_UNSIGNED_FIXED sampleRate,	// Fixed 16.16
										char dataBitSize,				// 8 or 16 bit data
										char channelSize);				// 1 or 2 channels of date

	// This will return the last BAESoundStream error
	BAEResult			LastError(void);

	// This will preroll a stream (get everything ready for syncronized start)
	BAEResult 			Preroll(void);

	// This will start a stream once data has been loaded
	BAEResult			Start(void);

	// get information about the current stream
	BAEResult			GetInfo(BAESampleInfo *pInfo);

	// This will stop a streaming audio object and free any memory.
	void			Stop(BAE_BOOL startFade = FALSE);

	// This will stop and flush the current stream and force a read of data. This
	// will cause gaps in the audio.
	void			Flush(void);

	// Get the position of a audio stream in samples
	unsigned long	GetPlaybackPosition(void);

	// currently paused
	BAE_BOOL		IsPaused(void);

	// Pause BAESoundStream from playback
	void			Pause(void);

	// Resume BAESoundStream from where paused.
	void			Resume(void);

	// Returns TRUE or FALSE if a given BAESoundStream is still active
	BAE_BOOL		IsPlaying(void);

	// Returns the opposite of IsPlaying
	BAE_BOOL		IsDone(void);

	// Returns TRUE if a given BAESoundStream is valid
	BAE_BOOL		IsValid(void);

	// Set the volume level of a audio stream
virtual void			SetVolume(BAE_UNSIGNED_FIXED newVolume);

	// Get the volume level of a audio stream
virtual	BAE_UNSIGNED_FIXED		GetVolume(void);

	// fade from source volume, to dest volume in time miliseconds. Always async
	void			FadeFromToInTime(BAE_FIXED sourceVolume, BAE_FIXED destVolume, BAE_FIXED timeInMiliseconds);

	// fade stream from current volume to silence
	void			Fade(BAE_BOOL doAsync = FALSE);

	// fade stream from current volume to specified volume
	void			FadeTo(BAE_FIXED destVolume, BAE_BOOL doAsync = FALSE);

	// Set the sample rate of a audio stream
	void			SetRate(BAE_UNSIGNED_FIXED newRate);

	// Get the sample rate of a audio stream
	BAE_UNSIGNED_FIXED		GetRate(void);

	// Set the stereo position of a audio stream (-63 left to 63 right, 0 is middle)
virtual void		SetStereoPosition(short int stereoPosition);

	// Get the stereo position of a audio stream (-63 left to 63 right, 0 is middle)
virtual short int	GetStereoPosition(void);

	// Enable/Disable reverb on this particular audio stream
	void			SetReverb(BAE_BOOL useReverb);
	BAE_BOOL 		GetReverb(void);
	// get reverb mix amount of this particular BAESoundStream
	short int		GetReverbAmount(void);
	void 			SetReverbAmount(short int reverbAmount);

	// get/set low pass filter amount of this particular BAESoundStream
	// amount range is -255 to 255
	short int		GetLowPassAmountFilter(void);
	void 			SetLowPassAmountFilter(short int lowpassamount);
	// get/set resonance filter amount of this particular BAESoundStream
	// resonance range is 0 to 256
	short int		GetResonanceAmountFilter(void);
	void 			SetResonanceAmountFilter(short int resonanceAmount);
	// get/set frequency filter amount of this particular BAESoundStream
	// Range is 512 to 32512
	short int		GetFrequencyAmountFilter(void);
	void 			SetFrequencyAmountFilter(short int frequencyAmount);

private:
	BAE_BOOL				mPrerolled;
	BAE_BOOL				mReverbState;
	short int				mReverbAmount;
	short int				mLowPassAmount;
	short int				mResonanceAmount;
	short int				mFrequencyAmount;
	BAE_UNSIGNED_FIXED		mVolumeState;
	short int				mPanState;
	BAESampleInfo			mStreamSampleInfo;
	unsigned long			mPauseVariable;
	unsigned long			mSoundStreamVoiceReference;
	void					*mUserReference;
	BAEStreamObjectProc		mCallbackProc;
};



// Midi Device class for direct control
class BAEMidiSynth : public BAENoise
{
friend class BAEMidiSong;
friend class BAERmfSong;
friend class BAEOutputMixer;
public:
				BAEMidiSynth(BAEOutputMixer *pBAEOutputMixer, 
								char const* cName = 0L, void *userReference = 0);
virtual			~BAEMidiSynth();

	BAEResult			Open(BAE_BOOL loadInstruments);
	void			Close(void);

	BAE_BOOL		IsLoaded(void);

	void			*GetReference(void)	const	{return mReference;}

	void			*GetSongVariables(void) const	{return m_pSongVariables;}

	// set song volume. You can overdrive by passing values larger than 1.0
virtual void		SetVolume(BAE_UNSIGNED_FIXED volume);
	// get the song volume
virtual	BAE_UNSIGNED_FIXED		GetVolume(void);

	// Set the master stereo position of a BAEMidiSynth (-63 left to 63 right, 0 is middle)
virtual void		SetStereoPosition(short int stereoPosition);

	// Set the master stereo position of a BAEMidiSynth (-63 left to 63 right, 0 is middle)
virtual short int	GetStereoPosition(void);

	// return note offset in semi tones	(-12 is down an octave, 12 is up an octave)
	long			GetPitchOffset(void);
	// set note offset in semi tones	(-12 is down an octave, 12 is up an octave)
	void			SetPitchOffset(long offset);

	// If allowPitch is FALSE, then "SetPitchOffset" will have no effect on passed 
	// channel (0 to 15)
	void			AllowChannelPitchOffset(unsigned short int channel, BAE_BOOL allowPitch);
	// Return if the passed channel will allow pitch offset
	BAE_BOOL		DoesChannelAllowPitchOffset(unsigned short int channel);

// Mute and unmute channels (0 to 15)
	void			MuteChannel(unsigned short int channel);
	void			UnmuteChannel(unsigned short int channel);
	void			GetChannelMuteStatus(BAE_BOOL *pChannels);

	void			SoloChannel(unsigned short int channel);
	void			UnSoloChannel(unsigned short int channel);
	void			GetChannelSoloStatus(BAE_BOOL *pChannels);


// Use these functions to drive the synth engine directly
	BAEResult			LoadInstrument(BAE_INSTRUMENT instrument);
	BAEResult			UnloadInstrument(BAE_INSTRUMENT instrument);
	BAE_BOOL		IsInstrumentLoaded(BAE_INSTRUMENT instrument);

// load an instrument with custom patch data.
	BAEResult			LoadInstrumentFromData(BAE_INSTRUMENT instrument, void *data, unsigned long dataSize);
// create a data block that is the instrument. Data block then can be passed into LoadInstrumentFromData
	BAEResult			CreateInstrumentAsData(BAE_INSTRUMENT instrument, void **pData, unsigned long *pDataSize);

	BAE_BOOL		GetCacheSample(void);
	void			SetCacheSample(BAE_BOOL cacheSamples);

static
	BAE_INSTRUMENT	TranslateBankProgramToInstrument(unsigned short bank, 
													unsigned short program, 
													unsigned short channel,
													unsigned short note = 0);
	BAEResult			RemapInstrument(BAE_INSTRUMENT from, BAE_INSTRUMENT to);

	// get current midi tick in microseconds
 	unsigned long	GetTick(void);

	// set queue control of midi commmands. Use TRUE to queue commands, FALSE to
	// send directly to engine. Default is TRUE
	void			SetQueue(BAE_BOOL useQueue);

	// Get current queue control status. TRUE is queuing enabled, FALSE is no queue
	BAE_BOOL		GetQueue(void)	const	{return mQueueMidi;}

	// Lock/Unlock queue processing. Storing of queue'd events still happens.
	// you need to match Locks with unlocks. You can nest and they are accumulitive
	void			LockQueue(void);
	void			UnlockQueue(void);

	// Get the current Midi controler value. Note this is not time based
	char			GetControlValue(unsigned char channel, 
											unsigned char controller);

	// Get the current Midi program and bank values
	void			GetProgramBank(unsigned char channel,
										unsigned char *pProgram,
										unsigned char *pBank);
	
	void			GetPitchBend(unsigned channel, unsigned char *pLSB, unsigned char *pMSB);

	// given a midi stream, parse it out to the various midi functions
	// for example:
	// 0x92			0x50		0x7F		0x00
	// comandByte	data1Byte	data2Byte	data3Byte
	// Note 80 on with a velocity of 127 on channel 2
	void			ParseMidiData(unsigned char commandByte, unsigned char data1Byte, 
									unsigned char data2Byte, unsigned char data3Byte,
									unsigned long time = 0);

	// if you pass 0 for time the current time will be passed
	// The channel variable is 0 to 15. Channel 9 is percussion for example.
	// The programNumber variable is a number from 0-127
	// If queuing is disabled, by calling SetQueue(FALSE), time has no meaning.
	void			NoteOff(unsigned char channel, 
							unsigned char note, 
							unsigned char velocity,
							unsigned long time = 0);

	// note on that checks to see if an instrument needs to be loaded. DO NOT call this
	// during an interrupt, as it might load memory. This only works when queuing is enabled
	void 			NoteOnWithLoad(unsigned char channel, 
							unsigned char note, 
							unsigned char velocity,
							unsigned long time = 0);

	void			NoteOn(unsigned char channel, 
						   unsigned char note, 
						   unsigned char velocity,
						   unsigned long time = 0);

	void			KeyPressure(unsigned char channel, 
								unsigned char note, 
								unsigned char pressure,
								unsigned long time = 0);

	void			ControlChange(unsigned char channel, 
								  unsigned char controlNumber,
								  unsigned char controlValue, 
								  unsigned long time = 0);

	void			ProgramBankChange(unsigned char channel,
								  unsigned char programNumber,
								  unsigned char bankNumber,
								  unsigned long time = 0);

	void			ProgramChange(unsigned char channel, 
								  unsigned char programNumber,
								  unsigned long time = 0);

	void			ChannelPressure(unsigned char channel, 
									unsigned char pressure, 
									unsigned long time = 0);

	void			PitchBend(unsigned char channel, 
							  unsigned char lsb, 
							  unsigned char msb,
							  unsigned long time = 0);

	void			AllNotesOff(unsigned long time = 0);


private:
	void			*m_pSongVariables;
	void			*m_pPerformanceVariables;
	unsigned long	mPerformanceVariablesLength;

	void			*mReference;
	BAE_BOOL		mQueueMidi;
};

// Midi Device class for Standard Midi files
class BAEMidiSong : public BAEMidiSynth
{
public:
			BAEMidiSong(BAEOutputMixer *pBAEOutputMixer, 
				char const* cName = 0L, void *userReference = 0);
virtual		~BAEMidiSong();

	// get embedded voice configuration from file
	short int		GetEmbeddedMidiVoices(void);
	short int		GetEmbeddedMixLevel(void);
	short int		GetEmbeddedSoundVoices(void);
	// set embedded voice configuration that is in file
	// NOTE: Does not change current mixer settings only when Start is called
	void			SetEmbeddedMidiVoices(short int midiVoices);
	void			SetEmbeddedMixLevel(short int mixLevel);
	void			SetEmbeddedSoundVoices(short int soundVoices);

	// get/set embedded reverb type.
	// NOTE: Does not change current mixer settings only when Start is called
	BAEReverbType	GetEmbeddedReverbType(void);
	void			SetEmbeddedReverbType(BAEReverbType verb);

	// get/set embedded volume type.
	// NOTE: Does not change current settings only when Start is called
	void			SetEmbeddedVolume(BAE_UNSIGNED_FIXED volume);
	BAE_UNSIGNED_FIXED		GetEmbeddedVolume(void);

	// get info about this song file. Will return a 'C' string
	BAEResult			GetInfo(BAEInfoType infoType, char *cInfo);

	// get size of info about this song file. Will an unsigned long
	unsigned long	GetInfoSize(BAEInfoType infoType);

	// load a midi file into memory from a file
virtual BAEResult		LoadFromFile(const BAEPathName pMidiFilePath, BAE_BOOL ignoreBadInstruments = TRUE);

	// load a midi file into memory from the current audio file	by name
virtual BAEResult		LoadFromBank(const char *cName, BAE_BOOL ignoreBadInstruments = TRUE);

	// load a midi file into memory from the current audio file	by ID
virtual BAEResult		LoadFromID(unsigned long id, BAE_BOOL ignoreBadInstruments = TRUE);

	// Given a pointer to a file, load it into a BAEMidiSong object. 
	//
	// If duplicateObject is TRUE, then the pointer passed in will be duplicated. 
	// You can free the memory pointer passed after success.
	// If FALSE the user pointer will be used, but
	// not copied. Don't delete the object until after you have deleted this object.
virtual BAEResult		LoadFromMemory(void const* pMidiData, unsigned long midiSize, 
							BAE_BOOL duplicateObject = TRUE, BAE_BOOL ignoreBadInstruments = TRUE);

	// free song from memory
	void			Unload(void);

	// This will preroll a song, load instruments, etc. After calling this method, you can
	// issue MIDI commands directly without starting the song.
	BAEResult 			Preroll(BAE_BOOL useEmbeddedMixerSettings = TRUE, BAE_BOOL autoLevel = FALSE);

	// start song. If useEmbeddedMixerSettings is TRUE then the mixer will be reconfigured
	// to the embedded song settings. If false, then song will attempt to start with the
	// current mixer configuration.
	BAEResult			Start(BAE_BOOL useEmbeddedMixerSettings = TRUE, BAE_BOOL autoLevel = FALSE);

	// end song. If startFade is TRUE, song will fade out and then stop. This is asyncronous
	void			Stop(BAE_BOOL startFade = FALSE);

	// fade from source volume, to dest volume in time miliseconds. Always async
	void			FadeFromToInTime(BAE_FIXED sourceVolume, BAE_FIXED destVolume, BAE_FIXED timeInMiliseconds);

	// fade song from current volume to silence. If doAsync is FALSE this function will
	// return only when the fade is finished. This will not stop the song.
	void			Fade(BAE_BOOL doAsync = FALSE);

	// fade song from current volume to specified volume
	void			FadeTo(BAE_FIXED destVolume, BAE_BOOL doAsync = FALSE);

	// pause and resume song playback
	void			Pause(void);
	void			Resume(void);
	BAE_BOOL		IsPaused(void);

	// Set song loop playback control
	void			SetLoopMax(short int maxLoops);
	short int 		GetLoopMax(void);

	// pass TRUE to entire loop song, FALSE to not loop
	void			SetLoopFlag(BAE_BOOL loop);

	BAE_BOOL		GetLoopFlag(void);

	// get ticks in midi ticks of length of song
	unsigned long	GetTickLength(void);
	// set the current playback position of song in midi ticks
	BAEResult			SetTickPosition(unsigned long ticks);
	// get the current playback position of a song in midi ticks
	unsigned long	GetTickPosition();

	// get ticks in microseconds of length of song
	unsigned long	GetMicrosecondLength(void);
	// set the current playback position of song in microseconds
	BAEResult			SetMicrosecondPosition(unsigned long ticks);
	// get the current playback position of a song in microseconds
	unsigned long	GetMicrosecondPosition();

	// return the instruments required to play this song. Make sure pArray768 is
	// an array of BAE_INSTRUMENT with 768 elements.
	BAEResult			GetInstruments(BAE_INSTRUMENT *pArray768, short int *pReturnedCount);

	// Set a call back during song playback. Pass NULL to clear callback
	void					SetTimeCallback(BAETimeCallbackPtr pSongCallback, void *pReference);

	BAETimeCallbackPtr		GetTimeCallback(void)	{return m_pSongTimeCallback;}
	void					*GetTimeCallbackReference(void)	{return m_pTimeCallbackReference;}

	// Set a call back during song playback for meta events. Pass NULL to clear callback.
	void					SetMetaEventCallback(BAEMetaEventCallbackPtr pSongCallback, void * pReference);
	BAEMetaEventCallbackPtr	GetMetaCallback(void)			{return m_pSongMetaCallback;}
	void					*GetMetaCallbackReference(void)	{return mSongMetaReference;}

	// Set a call back on midi controler events
	void					SetControlCallback(short int controller, 		// controller to connect
									BAEControlerCallbackPtr pControllerCallback,
									void * pReference);
	BAEControlerCallbackPtr	GetControlCallback(void)			{return m_pControllerCallbackProc;}
	void					*GetControlCallbackReference(void)	{return m_pControllerCallbackReference;}

	// Set a call back when song is done. Pass NULL to clear event
	void					SetDoneCallback(BAEDoneCallbackPtr pSongCallback, void * pReference);
	BAEDoneCallbackPtr		GetDoneCallback(void)			{return m_pSongCallback;}
	void					*GetDoneCallbackReference(void)	{return mSongCallbackReference;}

	// poll to see if song is done
	BAE_BOOL		IsDone(void);

	// poll to see if a song is playing
	BAE_BOOL		IsPlaying(void);

	// set song master tempo. (1.0 uses songs encoded tempo, 2.0 will play
	// song twice as fast, and 0.5 will play song half as fast
	void			SetMasterTempo(BAE_UNSIGNED_FIXED tempoFactor);
	// get song master tempo
	BAE_UNSIGNED_FIXED		GetMasterTempo(void);

	// Sets tempo in microsecond per quarter note
	void			SetTempo(unsigned long newTempo);
	// returns tempo in microsecond per quarter note
	unsigned long	GetTempo(void);

	// sets tempo in beats per minute
	void			SetTempoInBeatsPerMinute(unsigned long newTempoBPM);
	// returns tempo in beats per minute
	unsigned long	GetTempoInBeatsPerMinute(void);

	// Mute and unmute tracks (0 to 64)
	void			MuteTrack(unsigned short int track);
	void			UnmuteTrack(unsigned short int track);
	void			GetTrackMuteStatus(BAE_BOOL *pTracks);

	void			SoloTrack(unsigned short int track);
	void			UnSoloTrack(unsigned short int track);
	void			GetSoloTrackStatus(BAE_BOOL *pTracks);

private:
	BAE_BOOL				mSongFinished;
	void					*m_pControllerCallbackReference;
	BAEControlerCallbackPtr	m_pControllerCallbackProc;

	void					*m_pTimeCallbackReference;
	BAETimeCallbackPtr		m_pSongTimeCallback;

	void					*mSongCallbackReference;
	BAEDoneCallbackPtr		m_pSongCallback;

	void					*mSongMetaReference;
	BAEMetaEventCallbackPtr	m_pSongMetaCallback;
};


// Midi Device class for RMF files
class BAERmfSong : public BAEMidiSong
{
public:
			BAERmfSong(BAEOutputMixer *pBAEOutputMixer, 
				char const* cName = 0L, void *userReference = 0);
virtual		~BAERmfSong();

	// is this RMF file encrypted?
	BAE_BOOL		IsEncrypted() const;
	// is this RMF file compressed?
	BAE_BOOL		IsCompressed() const;

	// load a RMF into memory from the current audio file by name. This assumes
	// that you have imported the RMF file into the bank
virtual BAEResult		LoadFromBank(char const* cName, BAE_BOOL ignoreBadInstruments = TRUE);

	// load a RMF file into memory from the current audio file	by ID
virtual BAEResult		LoadFromID(unsigned long id, BAE_BOOL ignoreBadInstruments = TRUE);

	// load a RMF file into memory from a file.
virtual BAEResult		LoadFromFile(const BAEPathName pRMFFilePath, BAE_BOOL ignoreBadInstruments = TRUE);

	// Given a pointer to a file, load it into a BAERmfSong object. 
	//
	// If duplicateObject is TRUE, then the pointer passed in will be duplicated. 
	// You can free the memory pointer passed after success.
	// If FALSE the user pointer will be used, but
	// not copied. Don't delete the object until after you have deleted this object.
virtual BAEResult		LoadFromMemory(void const* pRMFData, unsigned long rmfSize, 
								BAE_BOOL duplicateObject = TRUE,
								BAE_BOOL ignoreBadInstruments = TRUE);

private:
	void	*m_pRMFDataBlock;
};

// Class designed to associate BAE objects together and start/stop/change
// them in sync. Create BAESound/BAESoundStream objects, associate them
// then perform functions. When deleting the BAENoiseGroup make sure you delete
// the objects you've added. Deleting an BAENoiseGroup will not delete the associated
// object
class BAENoiseGroup : public BAENoise
{
public:
					BAENoiseGroup(BAEOutputMixer *pBAEOutputMixer, 
						char const* cName = 0L, void *userReference = 0);
					~BAENoiseGroup();

	void			*GetReference(void)				const	{return mUserReference;}

	// Associate an BAE object to this group
	BAEResult			AddSound(BAESound *pSound);
	BAEResult			AddStream(BAESoundStream *pStream);

	// Disassociate an BAE object from this group
	BAEResult			RemoveSound(BAESound *pSound);
	BAEResult			RemoveStream(BAESoundStream *pStream);

	BAEResult			Start(void);
	void			Stop(BAE_BOOL startFade = FALSE);

	// Set the stereo position an entire group (-63 left to 63 right, 0 is middle)
virtual void		SetStereoPosition(short int stereoPosition);
virtual	short int	GetStereoPosition(void) {return 0L;}

	// Set the volume level of an entire group
virtual void		SetVolume(BAE_UNSIGNED_FIXED newVolume);
virtual	BAE_UNSIGNED_FIXED	GetVolume(void) {return 0L;}

	// Enable/Disable reverb of an entire group
	void			SetReverb(BAE_BOOL useReverb);
	// set reverb mix amount of an entire group
	void 			SetReverbAmount(short int reverbAmount);

private:
	void			*mUserReference;

	void			*linkedPlaybackReference;

	BAESound		*m_topSound;
	BAESoundStream	*m_topStream;
};

// Mod file playback. CAN ONLY PLAY ONE MOD FILE AT A TIME
// Can play MOD file types: MOD, STM, S3M, ULT
class BAEModSong : public BAENoise
{
public:
					BAEModSong(BAEOutputMixer *pBAEOutputMixer, 
								char const *cName = 0L, void * userReference = 0);
virtual				~BAEModSong();

	void			*GetReference(void)	const	{return userReference;}

// load a sample playing from a file. The memory will be deallocated when this
// object is destroyed. Call start once loaded to start the playback.
	BAEResult			LoadFromFile(const BAEPathName pModFilePath);

// Load memory mapped MOD pointer into BAEModSong object. This will parse the MOD file and get
// it ready for playing. You can dispose of the data passed once this method returns
	BAEResult			LoadFromMemory(void const* pModData, unsigned long modSize);

	// get info about this song file. Will return a 'C' string.
	// MOD only supports TITLE_INFO and COMPOSER_NOTES_INFO
	BAEResult			GetInfo(BAEInfoType infoType, char *cInfo);

	// get size of info about this song file. Will an unsigned long
	unsigned long	GetInfoSize(BAEInfoType infoType);

	// currently paused
	BAE_BOOL		IsPaused(void);

	// pause BAEModSong from playback
	void			Pause(void);

	// resume BAEModSong where paused
	void			Resume(void);

	// fade from source volume, to dest volume in time miliseconds. Always async
	void			FadeFromToInTime(BAE_FIXED sourceVolume, BAE_FIXED destVolume, BAE_FIXED timeInMiliseconds);

	// fade sample from current volume to silence
	void			Fade(BAE_BOOL doAsync = FALSE);

	// fade from current volume to specified volume
	void			FadeTo(BAE_FIXED destVolume, BAE_BOOL doAsync = FALSE);

	// This will start a BAEModSong once data has been loaded
	BAEResult			Start(BAEDoneCallbackPtr pDoneProc = NULL);

	// Set a call back when song is done
	void			SetDoneCallback(BAEDoneCallbackPtr pDoneProc, void * pReference);

	// This will stop a BAEModSong
	void			Stop(BAE_BOOL startFade = FALSE);

	// Returns TRUE or FALSE if a given BAEModSong is still active
	BAE_BOOL		IsPlaying(void);
	// returns the opposite of IsPlaying
	BAE_BOOL		IsDone(void);
	// set song master tempo. (1.0 uses songs encoded tempo, 2.0 will play
	// song twice as fast, and 0.5 will play song half as fast
	void			SetMasterTempo(BAE_UNSIGNED_FIXED tempoFactor);

// sets tempo in beats per minute
	void			SetTempoInBeatsPerMinute(unsigned long newTempoBPM);
// returns tempo in beats per minute
	unsigned long	GetTempoInBeatsPerMinute(void);

	// pass TRUE to entire loop song, FALSE to not loop
	void			SetLoopFlag(BAE_BOOL loop);

	BAE_BOOL		GetLoopFlag(void);

	// Set the volume level of a BAEModSong
virtual void		SetVolume(BAE_UNSIGNED_FIXED newVolume);

	// Get the volume level of a BAEModSong
virtual	BAE_UNSIGNED_FIXED	GetVolume(void);

private:
	void			*userReference;
	void			*pSoundVariables;
	long			pauseVariable;
};

// backward support
#define BAEAudioOutput	BAEOutputMixer
#define GetAudioOutput	GetMixer
#define BAEInfoTypes	BAEInfoType
#define BAEMetaTypes	BAEMetaType
#define BAEErr			BAEResult

#endif	// BAE_AUDIO
