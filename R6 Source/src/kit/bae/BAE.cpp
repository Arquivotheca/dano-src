/*****************************************************************************/
/*
** "BAE.cpp"
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
**	11/1/96		Added BAEStreamSound class
**	11/15/96	Fixed StopMidiSong to end the sequencer
**				Fixed FadeMidiSong to restore the volume after the song ends
**	11/18/96	Added LoadResourceSample
**	11/19/96	Changed char to BAE_BOOL in cases of boolean use
**	11/21/96	Changed LoadResourceSample to pass in size of data
**	11/26/96	Changed ServiceStreams to ServiceIdle
**				Added FadeTo and async for all fades to the BAEMidiSong
**				and BAESoundStream, and BAESound
**	12/1/96		Changed GetCurrentProgramBank to GetProgramBank
**	12/2/96		Added Mod code
**	12/3/96		Fixed bug with BAESound::Start
**	12/10/96	Added BAEOutputMixer::ChangeAudioFileToMemory
**	12/16/96	Fixed bug in destructors that hung in a loop while trying to remove
**				objects from fade links
**	12/16/96	Fixed bug in BAEMidiSong::SetTempoInBeatsPerMinute.Used the wrong API
**	12/30/96	Changed copyright
**	1/2/97		Added GetSongNameFromAudioFile & GetSampleNameFromAudioFile &
**				GetInstrumentNameFromAudioFile
**	1/7/97		Added BAEMidiSong::LoadFromMemory
**	1/10/97		Added some const
**	1/15/97		Changed LoadFromBank and LoadBankSample to support native MacOS resources
**	1/16/97		Added GetInfoSize
**	1/22/97		Added BAEOutputMixer::GetTick
**				Fixed callbacks and added SetDoneCallback in BAESound and BAEModSong
**	1/24/97		Fixed a bug with BAEOutputMixer::Close that could make my counters
**				get out of sync
**				Added BAEModSong::SetLoopFlag & BAEModSong::SetTempoInBeatsPerMinute &
**				BAEModSong::GetTempoInBeatsPerMinute(void)
**				Rebuilt fade system. Completely intergrated with low level mixer
**	1/27/97		Changed Fade to not stop the audio object once it reaches zero
**	1/28/97		Added a BAERmfSong class. Its pretty much the same as a BAEMidiSong
**				object, except it knows how to read an RMF file
**	2/1/97		Added BAEMidiSynth::AllowChannelPitchOffset & 
**				BAEMidiSynth::DoesChannelAllowPitchOffset
**	2/5/97		Added BAESound::LoadMemorySample
**				Changed MOD handling not to keep MOD loaded into memory once parsed
**	2/8/97		Modified BAEMixer to change modifiers for stereo support
**	2/19/97		Added support for platform specific data when creating an BAEOutputMixer
**	2/28/97		Added NoteOnWithLoad & IsInstrumentLoaded & TranslateBankProgramToInstrument & 
**				TranslateCurrentProgramToInstrument
**	3/3/97		Changed NoteOnWithLoad to get the current time after loading instrument
**	3/10/97		Removed extra reverbmode get, and typed the SetReverb correctly
**	3/18/97		Fixed the pause/resume methods. They were reversed
**	3/20/97		Changed ChangeAudioModes to remap modes to C API
**				Added GetMicrosecondLength & SetMicrosecondPosition & GetMicrosecondPosition
**				in the BAEMidiSong class
**				Fixed a bug with BAEOutputMixer::SetReverbType. Forgot to set reverb type
**				for later retreival
**	4/1/97		Fixed a bug with ProgramBankChange. Forgot to include the controller change
**				for the non queued version
**				Changed GetTick & GetAudioLatency to pull the tick count from the new mixer
**				syncCount rather than XMicroseconds
**	4/18/97		Removed extra linkage and unused variables
**	4/20/97		Added BAEMidiSong::SetMetaEventCallback
**	5/1/97		Changed BAESound::Start to accecpt an frame offset when starting the sample
**	5/3/97		Fixed a few potential problems that the MOT compiler found
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
**	6/4/97		Restricted 68k codebase to 11k, mono, drop sample
**	6/25/97		Changed an unsigned to an unsigned long in BAESound::SetSampleLoopPoints
**				Fixed BAESound::SetSampleLoopPoints to actually set the playing samples
**				loop points
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
**	7/21/97		Changed BAEModSong::GetTempoInBeatsPerMinute and BAEMidi::GetTempoInBeatsPerMinute to
**				to unsigned long
**	7/22/97		Changed SYNC_BUFFER_TIME to BUFFER_SLICE_TIME
**	7/28/97		Put USE_STREAM_API around the stream class
**	8/8/97		Modified PV_ProcessSongMetaEventCallbacks to support extra parameter
**	8/18/97		Seperated GM_StartHardwareSoundManager from being called inside of 
**				GM_InitGeneralSound. Now its called in BAEOutputMixer::Open. Likewise
**				GM_StopHardwareSoundManager is no longer being called within GM_FinisGeneralSound,
**				it nows is called at BAEOutputMixer::Close
**	8/20/97		Changed BAESoundStream::SetupCustomStream & BAESoundStream::SetupFileStream
**				to use BAE_MIN_STREAM_BUFFER_SIZE to control minimumn size of buffer
**	8/25/97		Changed parameter order in PV_ProcessSongControllerCallbacks
**	9/9/97		Fixed bug with PV_ProcessSongTimeCallbacks that used the wrong callback !
**	9/11/97		Added BAEMidiSong::IsPaused && BAEModSong::IsDone && BAEMidiSong::IsPlaying &&
**				BAESoundStream::IsDone && BAESound::IsDone
**	9/15/97		Changed BAEMidiSong::Unload to call BAEMidiSynth::Close instead of duplicating
**				code.
**	9/30/97		Changed references to GM_AudioStreamError to pass in the stream reference
**				Changed BAEOutputMixer::GetInstrumentNameFromAudioFileFromID to allow for ID
**				being 0
**				Fixed a bug in which BAEOutputMixer::ChangeAudioFileToMemory && 
**				BAEOutputMixer::ChangeAudioFile would leave a bad patch file open if it
**				failed to open the passed file. Now it reverts back to the previously
**				open patch file if failure occurs.
**	9/30/97		Added BAEMidiSong::GetEmbeddedMidiVoices & BAEMidiSong::GetEmbeddedMixLevel & 
**				BAEMidiSong::GetEmbeddedSoundVoices
**	10/3/97		Added BAEMidiSong::SetEmbeddedMidiVoices & BAEMidiSong::SetEmbeddedMixLevel & 
**				BAEMidiSong::SetEmbeddedSoundVoices
**				Added BAEMidiSong::GetEmbeddedReverbType & BAEMidiSong::SetEmbeddedReverbType
**				Added BAEOutputMixer::GetQuality & BAEOutputMixer::GetTerpMode
**	10/12/97	Added BAERmfSong::LoadFromBank
**	10/15/97	Changed the BAERmfSong class to always copy the RMF data
**	10/16/97	Changed BAEMidiSong::Start to allow for an optional reconfigure of
**				the mixer when starting a song
**				Modified BAEMidiSong::LoadFromID & BAEMidiSong::LoadFromBank & BAEMidiSong::LoadFromFile &
**				BAEMidiSong::LoadFromMemory & BAERmfSong::LoadFromFile & BAERmfSong::LoadFromMemory &
**				BAERmfSong::LoadFromBank to allow for optional reporting of failure to load instruments
**				Removed BAEMidiSynth::FlushInstrumentCache. Not required or used.
**				Renamed GetPatches to GetInstruments and changed the array passed to
**				be of type BAE_INSTRUMENT
**	11/6/97		Added BAERmfSong::LoadFromID
**	11/10/97	Wrapped some conditional code around BAEMidiSong::GetInstruments
**	11/11/97	Cleaned up some garbage code from BAEOutputMixer::BAEOutputMixer
**				Added GetMaxDeviceCount & SetCurrentDevice & GetCurrentDevice & GetDeviceName
**	11/24/97	Added BAESoundStream::Flush
**	12/18/97	Cleaned up some warnings and added some rounding devices
**	1/18/98		Fixed up IsPlaying/IsDone pairs to return BOOLEANS correctly
**	1/20/98		Fixed callback problem with BAEMidiSong::Start
**				Fixed memory allocation problem with BAEMidiSynth::Close
**	1/22/98		Changed name of pRMFDataBlock to m_pRMFDataBlock
**				Added BAERmfSong::IsCompressed BAERmfSong::IsEncrypted
**	1/27/98		Added a parameter to BAEMidiSong::SetTimeCallback and changed the way the callbacks
**				are handled
**	2/10/98		In BAEMidiSynth::Open enabled sample caching, and BAEMidiSynth::Close
**				disables sample caching
**				Fixed BAEMidiSong::LoadFromMemory to return a memory error in case it
**				can't duplicate the memory block
**				Fixed BAERmfSong::LoadFromMemory to return a memory error in case it
**				can't duplicate the memory block, and fixed the function to actaully
**				copy the data correctly.
**	2/11/98		Changed BAESound::SetSampleLoopPoints to accept 0 as a valid start
**				loop point
**				Added BAE_8K, BAE_48K, BAE_11K_TERP_22K, BAE_22K_TERP_44K, BAE_24K
**	2/18/98		Added BAESound::StartDoubleBuffer
**	2/19/98		Added BAEOutputMixer::GetURLFromAudioFile & BAEOutputMixer::GetNameFromAudioFile
**				Added code wrappers around GetURLFromAudioFile & GetNameFromAudioFile
**	2/24/98		Fixed a problem with the way the SONG resource and memory based files handle
**				retriving the size of memory blocks inside of a memory file
**	3/2/98		Fixed BAEOutputMixer::Open to Close down the mixer correctly when failing
**	3/5/98		Fixed BAESound::StartCustomSample & BAESound::Start to return an error if
**				trying to play samples larger than 1MB
**	3/9/98		Modified open to allow an optional not connect to audio hardware. If you call
**				Open without connecting to hardware, you'll need to call BAEOutputMixer::ReengageAudio
**				Added new method BAEOutputMixer::IsAudioEngaged
**	3/11/98		Grr. Fixed a problem with BAEOutputMixer::Open. Didn't connect to audio hardware
**				if you passed NULL as the audiofile.
**	3/12/98		Added BAEMidiSong::SetEmbeddedVolume & BAEMidiSong::GetEmbeddedVolume
**				Changed BAEOutputMixer::ReengageAudio & BAEOutputMixer::DisengageAudio
**				to change the audioSetup flag correctly.
**				Fixed BAEMidiSynth::Open to return an error code if memory fails
**	3/16/98		Added new verb types
**	4/30/98		Added SUB_GENRE_INFO & GENRE_INFO
**	5/5/98		Fixed BAEOutputMixer::GetNameFromAudioFile & BAEOutputMixer::GetURLFromAudioFile
**				to clear the name in case there is no information
**	5/7/98		Fixed BAESound::LoadFileSample & BAESound::LoadMemorySample to handle
**				error codes better
**				Created BAEPrivate.h to allow access to certain functions.
**				Renamed PV_TranslateOPErr to BAE_TranslateOPErr
**	5/26/98		Removed from BAEOutputMixer::Open the auto close and deallocate of the mixer upon
**				error. Deallocation will now happen when BAEOutputMixer object is deleted.
**
**	6/5/98		Jim Nitchals RIP	1/15/62 - 6/5/98
**				I'm going to miss your irreverent humor. Your coding style and desire
**				to make things as fast as possible. Your collaboration behind this entire
**				codebase. Your absolute belief in creating the best possible relationships 
**				from honesty and integrity. Your ability to enjoy conversation. Your business 
**				savvy in understanding the big picture. Your gentleness. Your willingness 
**				to understand someone else's way of thinking. Your debates on the latest 
**				political issues. Your generosity. Your great mimicking of cartoon voices. 
**				Your friendship. - Steve Hales
**
**	6/18/98		Added SetCacheStatusForAudioFile & GetCacheStatusForAudioFile
**	6/20/98		Renamed PV_CustomStreamCallback to PV_CustomOutputStreamCallback
**	7/6/98		Changed BAEMidiSynth::IsInstrumentLoaded to use direct GM_ API
**	7/17/98		Added BAE_UseThisFile
**	7/20/98		Changed ChangeAudioFile & ChangeAudioFileToMemory to handle
**				passing of a NULL to close the current audio patch file association
**				to the mixer
**	7/27/98		Added LockQueue & UnlockQueue
**	7/28/98		Added BAEMidiSynth::SetStereoPosition & BAEMidiSynth::GetStereoPosition
**	7/30/98		Added reverb state and fixed all the BAESound::Start/BAESoundStream::Start functions 
**				to start the verb up when the object gets started.
**	8/6/98		Changed reference to mSoundVoiceReference in the BAESound class. Fixed a few area where
**				I forgot to check for the voice being active
**				Changed mReference to mSoundStreamVoiceReference in the BAESoundStream class
**	8/10/98		Removed some sign/unsigned conflicts
**	8/11/98		Renamed pName to cName and implemented GetName in the BAENoise base class
**	8/13/98		Added voiceType to BAEOutputMixer::GetRealtimeStatus
**				Added BAEOutputMixer::GetCPULoadInMicroseconds & BAEOutputMixer::GetCPULoadInPercent
**	8/14/98		Modified BAEOutputMixer::GetCPULoadInPercent to use embedded Gen API function
**				rather than calculate it
**	9/2/98		Added BAE_TranslateAudioFileType
**	9/10/98		Fixed some problems when USE_HIGHLEVEL_FILE_API is not defined
**	9/12/98		Added BAESound::GetSamplePointerFromMixer
**	10/17/98	Fixed a problem with BAESound::Start & BAESound::StartDoubleBuffer & BAESound::StartCustomSample
**				in which two or more voices that are started will sometimes kill the previous voice allocated
**	10/26/98	Added error code to BAEOutputMixer::ServiceAudioOutputToFile
**	10/30/98	Implemented a default BAESound done callback, that marks the sample finished when playing out
**				normally. Related to the 10/17/98 bug.
**	11/19/98	Added new parameter to BAE_UseThisFile
**	11/20/98	Added support for mixer->Open passing in BAE_REVERB_NO_CHANGE
**	11/24/98	Added BAESound::SetReverbAmount & BAESound::GetReverbAmount.
**	12/3/98		Added BAESoundStream::SetReverbAmount & BAESoundStream::GetReverbAmount
**				Added BAENoiseGroup class, and modified BAENoise for linked list issues
**	12/9/98		Added BAEMidiSynth::GetPitchBend
**	12/17/98	Added BAEOutputMixer::SetHardwareBalance & BAEOutputMixer::GetHardwareBalance
**	12/18/98	Added to BAEMidiSong::Start auto level
**	1/5/99		Changed copyright and fixed a sign conversion warning
**	1/14/99		Added BAEMidiSynth::CreateInstrumentAsData && BAEMidiSynth::LoadInstrumentFromData
**	1/29/99		Changed BAEOutputMixer::GetVersionFromAudioFile to use new XGetVersion API
**	2/12/99		Renamed USE_BAE_FOR_MPEG to USE_MPEG_DECODER
**	2/18/99		Renamed pSongVariables to m_pSongVariables, queueMidi to mQueueMidi, 
**				reference to mReference, m_performanceVariablesLength to mPerformanceVariablesLength
**				Added GetMetaCallback & GetMetaCallbackReference and support variables
**	2/24/99		Changed PV_ProcessSongEndCallbacks to use the new context from a GM_Song to deal
**				with the this pointer.
**				Changed BAEMidiSong::SetMetaEventCallback & PV_ProcessSongMetaEventCallbacks to use
**				new context of GM_Song structure rather than extra references
**	3/1/99		Fixed a bug in BAESoundStream::GetReverbAmount in which the wrong value was being
**				returned
**	3/3/99		Changed to the new way of starting samples. First a setup, then a start
**	3/5/99		Added threadContext to PV_ProcessSongTimeCallbacks & 
**				PV_ProcessSongMetaEventCallbacks & PV_ProcessSongControllerCallbacks & 
**				PV_ProcessSequencerEvents
**				Changed BAEOutputMixer::ServiceAudioOutputToFile to not call behind the wall
**				functions
**	3/6/99		Changed BAENoiseGroup class to handle massive changes inside of GenSample.c
**				and to handle sync starts.
**	3/11/99		Changed BAEOutputMixer::ChangeAudioModes so that is does not do anything
**				if result is the same.
**	3/12/99		Put in support for different loop types
**	3/18/99		Fixed bug in PV_ProcessSongEndCallbacks in which I forgot to change the function
**				parameters along with the threadContext change, so song end callbacks were not
**				happening.
**	3/24/99		Added TEMPO_DESCRIPTION_INFO & ORIGINAL_SOURCE_INFO
**	4/8/99		Added BAEOutputMixer::GetCPUType
**	5/16/99		Changed BAEMidiSong::IsPaused to use interal GM_IsSongPaused API.
**	5/17/99		Added BAEMidiSong::Preroll
**	5/28/99		MOE:  Eliminated data-truncation warning from BAEOutputMixer::SetHardwareBalance()
**	6/3/99		Added BAEOutputMixer::IsStereoSupported(void) & BAEOutputMixer::Is8BitSupported(void) &
**				BAEOutputMixer::Is16BitSupported(void)
**	6/8/99		Changed BAESound::SetSampleLoopPoints to clear the loop points if you set the start
**				and end point the same.
**	6/10/99		Placed in BAENoise::~BAENoise the ability to remove from the event queue
**				if there. Finished BAEOutputMixer::RemoveEvent
**	6/15/99		Changed the behavior of BAEMidiSong::Stop to not remove the song from the mixer. This
**				is done to allow us to continue to send events for processing
**				Added a slice delay to BAEMidiSynth::NoteOnWithLoad and changed it to handle non queued
**				events.
**	6/29/99		Added a new version of BAEOutputMixer::GetNameFromAudioFile that takes a BAEPathName
**	7/9/99		Changed BAEOutputMixer::SetTaskCallback to accept a reference in the callback structure
**				and placed glue code between the C++ layer and the C layer
**	7/13/99		Renamed HAE to BAE. Renamed BAEAudioMixer to BAEOutputMixer. Renamed BAEMidiDirect
**				to BAEMidiSynth. Renamed BAEMidiFile to BAEMidiSong. Renamed BAERMFFile to BAERmfSong.
**				Renamed BAErr to BAEResult. Renamed BAEMidiExternal to BAEMidiInput. Renamed BAEMod
**				to BAEModSong. Renamed BAEGroup to BAENoiseGroup. Renamed BAEReverbMode to BAEReverbType.
**				Renamed BAEAudioNoise to BAENoise.
**	7/14/99		Added BAEOutputMixer::GetChannelSoloStatus & BAEOutputMixer::GetChannelMuteStatus
**				Added BAEOutputMixer::DisengageAudio & BAEOutputMixer::ReengageAudio
**	7/28/99		Added BAEOutputMixer::GetVersionFromAudioFile
**	8/3/99		Added BAESound::SaveFile
**	8/4/99		Removed platform test in BAEMidiSong::GetInfo. Now just checks for Mac.
**	8/24/99		Bounds check BAEMidiSynth::SetPitchOffset
**	8/28/99		Added BAEOutputMixer::SetAudioLatency
**	9/2/99		Added in BAEOutputMixer::Open a check for at least 1MB of free memory for MacOS only
**	9/7/99		Changed BAEMidiSong::IsPaused to check a songfinished flag via BAEMidiSong::Stop. This
**				handles the condition that we need the song still in the mixer, when Stop is called and
**				we need Pause to return FALSE
**	9/7/99		Added mSongFinished to BAEMidiSong. In BAENoiseGroup & BAEMidiSong changed member name to
**				mUserReference rather than the same name as the variable passed into the constructor.
**				In BAEOutputMixer::Open added a test for version matching with the header file
**				the the main codebase.
*/
/*****************************************************************************/


/* THINGS TO DO \xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5


Fix BAERmfSong::LoadFromMemory so that duplicate object actaully does the right thing

*/

#include <stdio.h>

#include "BAE.h"

#include "X_API.h"
#include "GenSnd.h"
#include "GenPriv.h"
#include "X_Formats.h"
#include "BAE_API.h"
#include "BAEPrivate.h"

#define USE_LINKED_OBJECTS			0		// if 1, then all objects will be linked

#define USE_BANK_TEST				0

// variables
#if 0
	#pragma mark ### variables ###
#endif

static char				audioSetup = 0;				// audio will only be setup once
static XFILE			thePatchFile = (XFILE)NULL;	// current audio patch library
static XShortResourceID	midiSongCount = 0;			// everytime a new song is loaded, this is increments
													// this is used as an ID for song callbacks and such

// private functions

#if 0
	#pragma mark ### Support functions ###
#endif

#if X_PLATFORM == X_MACINTOSH

// Menu hook will allow code to run while a menu is being held down
static MenuHookUPP saveHook;
static MenuHookUPP newHook;

static pascal void PV_MacDoMenuHook(void)
{
}

static void PV_MacSetupMenuHook(void)
{
	saveHook = (MenuHookUPP)LMGetMenuHook();
	newHook = NewMenuHookProc(PV_MacDoMenuHook);
	LMSetMenuHook((UniversalProcPtr)newHook);
}

static void PV_MacCleanupMenuHook(void)
{
	LMSetMenuHook((UniversalProcPtr)saveHook);
	DisposeRoutineDescriptor(newHook);
}

// GetNextEvent hook will allow code to run every time GetNextEvent is called outside of
// the clients codebase
static GNEFilterUPP	saveGNEHook;
static GNEFilterUPP	newGNEHook;

static void PV_MacDoGNEHook(EventRecord *theEvent, Boolean *result)
{
	theEvent;
	result;
}

static void PV_MacSetupGNEFilter(void)
{
	saveGNEHook = LMGetGNEFilter();
	newGNEHook = NewGetNextEventFilterProc(PV_MacDoGNEHook);
	LMSetGNEFilter(newGNEHook);

}

static void PV_MacCleanupGNEFilter(void)
{
	LMSetGNEFilter(saveGNEHook);	
}
#endif

// Read a file into memory and return an allocated pointer
static XPTR PV_GetFileAsData(XFILENAME *pFile, long *pSize)
{
	XPTR	data;

	if (XGetFileAsData(pFile, &data, pSize))
	{
		data = NULL;
	}
	return data;
}

static INLINE XFILE PV_GetIndexedFile(XFILE *fileList, unsigned long fileIndex)
{
	if (fileList)
	{
		return fileList[fileIndex];
	}
	return (XFILE)NULL;
}

static INLINE void PV_SetIndexedFile(XFILE *fileList, unsigned long fileIndex, XFILE file)
{
	if (fileList)
	{
		fileList[fileIndex] = file;
	}
}

// given an list of xfiles, a count of xfiles, and a file path; open the file, expand the list
// and store the file index. Returns NULL if file fails to open, or memory allocation failure
static XFILE * PV_OpenToFileList(XFILE * files, unsigned long fileCount, BAEPathName pAudioPathName)
{
	XFILENAME		theFile;
	XFILE			*newFileList;
	unsigned long	size;
	XFILE			file;

	newFileList = NULL;
	// everythings ok, so open the file
	XConvertNativeFileToXFILENAME(pAudioPathName, &theFile);
	file = XFileOpenResource(&theFile, TRUE);
	if (file)
	{
		XFileUseThisResourceFile(file);
		size = sizeof(XFILE) * (fileCount + 1);
		newFileList = (XFILE *)XNewPtr(size);
		if (newFileList)
		{
			if (files)
			{
				XBlockMove(files, newFileList, size - sizeof(XFILE));
				XDisposePtr(files);
			}
	
			PV_SetIndexedFile(newFileList, fileCount, file);
		}
	}
	return newFileList;
}

// given a list of xfiles, a count of xfiles, and a xfile index to delete. shrink the list, close the file and remove
// from list
static XFILE * PV_CloseFromFileList(XFILE * files, unsigned long fileCount, unsigned long thisFileIndex)
{
	unsigned long	size, count, count2;
	XFILE			*newFileList;
	XFILE			xfile, file;

	newFileList = files;	
	if (thisFileIndex < fileCount)
	{
		xfile = PV_GetIndexedFile(files, thisFileIndex);
		if (xfile)
		{
			XFileClose(xfile);
		}
		if (fileCount > 0)
		{	// something in the list
			fileCount--;
			size = sizeof(XFILE) * fileCount;
			newFileList = (XFILE *)XNewPtr(size);
			if (newFileList)
			{
				// copy all except file to remove
				for (count = 0, count2 = 0; count < fileCount; count++)
				{
					file = PV_GetIndexedFile(files, count);
					if (xfile != file)
					{
						PV_SetIndexedFile(newFileList, count2++, file);
					}
				}
			}
		}
		else
		{	// empty, so return empty list
			newFileList = NULL;
		}
	}
	return newFileList;
}

// close all files in file list, and delete memory
static void PV_CloseAllFromFileList(XFILE * files, unsigned long fileCount)
{
	unsigned long	count;
	XFILE			file;

	if (fileCount > 0)
	{	// something in the list
		// copy all except file to remove
		for (count = 0; count < fileCount; count++)
		{
			file = PV_GetIndexedFile(files, count);
			if (file)
			{
				XFileClose(file);
			}
		}
		XDisposePtr((XPTR)files);
	}
}

static const ReverbMode translateInternal[] = {
								REVERB_NO_CHANGE,
								REVERB_TYPE_1,
								REVERB_TYPE_2,
								REVERB_TYPE_3,
								REVERB_TYPE_4,
								REVERB_TYPE_5,
								REVERB_TYPE_6,
								REVERB_TYPE_7,
								REVERB_TYPE_8,
								REVERB_TYPE_9,
								REVERB_TYPE_10,
								REVERB_TYPE_11
											};
static const BAEReverbType translateExternal[] = {
								BAE_REVERB_NO_CHANGE,
								BAE_REVERB_TYPE_1,
								BAE_REVERB_TYPE_2,
								BAE_REVERB_TYPE_3,
								BAE_REVERB_TYPE_4,
								BAE_REVERB_TYPE_5,
								BAE_REVERB_TYPE_6,
								BAE_REVERB_TYPE_7,
								BAE_REVERB_TYPE_8,
								BAE_REVERB_TYPE_9,
								BAE_REVERB_TYPE_10,
								BAE_REVERB_TYPE_11
											};
// translate reverb types from BAEReverbType to ReverbMode
ReverbMode BAE_TranslateFromBAEReverb(BAEReverbType igorVerb)
{
	ReverbMode				r;
	short int				count;

	r = REVERB_TYPE_1;
	for (count = 0; count < MAX_REVERB_TYPES; count++)
	{
		if (igorVerb == translateExternal[count])
		{
			r = translateInternal[count];
			break;
		}
	}
	return r;
}

// translate reverb types to BAEReverbType from ReverbMode
BAEReverbType BAE_TranslateToBAEReverb(ReverbMode r)
{
	BAEReverbType			igorVerb;
	short int				count;

	igorVerb = BAE_REVERB_TYPE_1;
	for (count = 0; count < MAX_REVERB_TYPES; count++)
	{
		if (r == translateInternal[count])
		{
			igorVerb = translateExternal[count];
			break;
		}
	}
	return igorVerb;
}

static const BAEResult translateExternalError[] = {
										BAE_NO_ERROR,
										BAE_BUFFER_TO_SMALL,
										BAE_NOT_SETUP,
										BAE_PARAM_ERR,
										BAE_MEMORY_ERR,
										BAE_BAD_INSTRUMENT,
										BAE_BAD_MIDI_DATA,
										BAE_ALREADY_PAUSED,
										BAE_ALREADY_RESUMED,
										BAE_DEVICE_UNAVAILABLE,
										BAE_STILL_PLAYING,
										BAE_NO_SONG_PLAYING,
										BAE_TOO_MANY_SONGS_PLAYING,
										BAE_NO_VOLUME,
										BAE_NO_FREE_VOICES,
										BAE_STREAM_STOP_PLAY,
										BAE_BAD_FILE_TYPE,
										BAE_GENERAL_BAD,
										BAE_BAD_SAMPLE,
										BAE_BAD_FILE,
										BAE_FILE_NOT_FOUND,
										BAE_NOT_REENTERANT,
										BAE_SAMPLE_TO_LARGE,
										BAE_UNSUPPORTED_HARDWARE,
										BAE_ABORTED
									};


static const OPErr translateInternalError[] = {
										NO_ERR,
										BUFFER_TO_SMALL,
										NOT_SETUP,
										PARAM_ERR,
										MEMORY_ERR,
										BAD_INSTRUMENT,
										BAD_MIDI_DATA,
										ALREADY_PAUSED,
										ALREADY_RESUMED,
										DEVICE_UNAVAILABLE,
										STILL_PLAYING,
										NO_SONG_PLAYING,
										TOO_MANY_SONGS_PLAYING,
										NO_VOLUME,
										NO_FREE_VOICES,
										STREAM_STOP_PLAY,
										BAD_FILE_TYPE,
										GENERAL_BAD,
										BAD_SAMPLE,
										BAD_FILE,
										FILE_NOT_FOUND,
										NOT_REENTERANT,
										SAMPLE_TO_LARGE,
										UNSUPPORTED_HARDWARE,
										ABORTED_PROCESS
									};
										
// Translate from OPErr to BAEResult
BAEResult BAE_TranslateOPErr(OPErr theErr)
{
	BAEResult		igorErr;
	short int	count,  max;

	igorErr = BAE_GENERAL_ERR;
	max = sizeof(translateExternalError) / sizeof(BAEResult);
	for (count = 0; count < max; count++)
	{
		if (translateInternalError[count] == theErr)
		{
			igorErr = translateExternalError[count];
			break;
		}
	}
	return igorErr;
}


// Translate from BAEResult to OPErr
OPErr BAE_TranslateBAErr(BAEResult theErr)
{
	OPErr		igorErr;
	short int	count,  max;

	igorErr = GENERAL_BAD;
	max = sizeof(translateExternalError) / sizeof(BAEResult);
	for (count = 0; count < max; count++)
	{
		if (translateExternalError[count] == theErr)
		{
			igorErr = translateInternalError[count];
			break;
		}
	}
	return igorErr;
}




#if USE_HIGHLEVEL_FILE_API != FALSE
AudioFileType BAE_TranslateBAEFileType(BAEFileType fileType)
{
	AudioFileType	haeFileType;

	haeFileType = FILE_INVALID_TYPE;
	switch (fileType)
	{
		case BAE_AIFF_TYPE:
			haeFileType = FILE_AIFF_TYPE;
			break;
		case BAE_WAVE_TYPE:
			haeFileType = FILE_WAVE_TYPE;
			break;
#if USE_MPEG_DECODER != FALSE
		case BAE_MPEG_TYPE:
			haeFileType = FILE_MPEG_TYPE;
			break;
#endif
		case BAE_AU_TYPE:
			haeFileType = FILE_AU_TYPE;
			break;
	}
	return haeFileType;
}
#endif

#if X_PLATFORM == X_WIN95
/*

For Windows, you may need to pass in the
	window name of the application window. If you do, you can pass in a partial 'C' string
	name. If you pass NULL, it will search for an open window, but Open may fail. For all
	other platforms, you should pass NULL.
	
static char *pWindowName;
static BAE_BOOL CALLBACK PV_EnumWndProc( HWND hWnd, LPARAM lParam )
{
    char szWindowName[256];

    // Get the window name and class name
    //...................................
    GetWindowText( hWnd, szWindowName, 255 );    // This should be 11
	// Scan for the Netscape Window
	if (XStrStr(szWindowName, pWindowName))
	{
		HWND	*pFoundWindow	=	(HWND *)lParam;
		
		if (pFoundWindow)
		{
			// Got it
			*pFoundWindow	=	hWnd;
		}
	}
	// Haven't found it yet.
    return( TRUE);
}
		if (pPlatformData)
		{
			pWindowName = (char *)pPlatformData;
			// Enumerate all top-level windows currently running 
			// and see if it matches our 'C' name passed in.
			EnumWindows((WNDENUMPROC)PV_EnumWndProc, (LPARAM)&w95Init.hwndOwner );
			if (w95Init.hwndOwner)
	        {
				// Make Window the Active Window
				EnableWindow(w95Init.hwndOwner, TRUE);
				SetForegroundWindow(w95Init.hwndOwner);
				SetActiveWindow(w95Init.hwndOwner);
			}
			else
			{
				pPlatformData = NULL;	// didn't find a named window, so look for current
			}
		}
		if (pPlatformData == NULL)
*/

#endif

// Class implemention for BAEAudio

#if 0
	#pragma mark ### BAEOutputMixer class ###
#endif

BAEOutputMixer::BAEOutputMixer()
{
	typedef struct
	{
		union
		{
			BAEEvent_ControlerEvent	u1;
			BAEEvent_ObjectDone		u2;
			BAEEvent_MetaEvent		u3;
			BAEEvent_LoadInstrument	u4;
		} u;
	} _calculate_total_event_size;
	short int		count;

	// ok, now allocate static buffers for all of our events for the largest event size.
	for (count = 0; count < BAE_MAX_EVENTS; count++)
	{
		m_pEvents[count] = XNewPtr(sizeof(_calculate_total_event_size));
	}
	mHeadEvent = 0;
	mTailEvent = 0;

	mWritingToFile = FALSE;
	mWritingToFileReference = NULL;
	iMidiVoices = 0;
	iSoundVoices = 0;
	iMixLevel = 0;
	iQuality = BAE_22K;
	iTerpMode = BAE_LINEAR_INTERPOLATION;
	iReverbMode = BAE_REVERB_TYPE_4;
	iModifiers = BAE_USE_STEREO | BAE_USE_16;
	pTop = NULL;
	pTask = NULL;
	songNameCount = 0;
	songNameType = BAE_GROOVOID;
	sampleNameCount = 0;
	instrumentNameCount = 0;
	mOpenAudioFileCount = 0;
	mOpenAudioFiles = NULL;
	mIsAudioEngaged = FALSE;
	mCacheStatus = TRUE;
}

BAEOutputMixer::~BAEOutputMixer()
{
	Close();
}

BAE_BOOL BAEOutputMixer::IsOpen(void) const
{
	return audioSetup ? (BAE_BOOL)TRUE : (BAE_BOOL)FALSE;
}

BAE_BOOL BAEOutputMixer::Is16BitSupported(void) const
{
	return (BAE_BOOL)XIs16BitSupported();
}

BAE_BOOL BAEOutputMixer::Is8BitSupported(void) const
{
	return (BAE_BOOL)XIs8BitSupported();
}

BAE_BOOL BAEOutputMixer::IsStereoSupported(void) const
{
	return (BAE_BOOL)XIsStereoSupported();
}


// return number of devices available to this mixer hardware
// will return a number from 1 to max number of devices.
// ie. a value of 2 means two devices
long BAEOutputMixer::GetMaxDeviceCount(void)
{
#if USE_DEVICE_ENUM_SUPPORT == TRUE
	return GM_MaxDevices();
#else
	return 0;
#endif
}

// set current device. should be a number from 0 to BAEOutputMixer::GetDeviceCount()
void BAEOutputMixer::SetCurrentDevice(long deviceID, void *deviceParameter)
{
#if USE_DEVICE_ENUM_SUPPORT == TRUE
	if (deviceID < GetMaxDeviceCount())
	{
		if (IsOpen())
		{
			DisengageAudio();		// shutdown from hardware
		}
		GM_SetDeviceID(deviceID, deviceParameter);	// change to new device
		if (IsOpen())
		{
			ReengageAudio();		// connect back to audio with new device
		}
	}
#else
	deviceID = deviceID;
	deviceParameter = deviceParameter;
#endif
}

// get current device.
long BAEOutputMixer::GetCurrentDevice(void *deviceParameter)
{
	return GM_GetDeviceID(deviceParameter);
}

// get device name
// NOTE:	This function needs to function before any other calls may have happened.
//			Format of string is a zero terminated comma delinated C string.
//			"platform,method,misc"
//	example	"MacOS,Sound Manager 3.0,SndPlayDoubleBuffer"
//			"WinOS,DirectSound,multi threaded"
//			"WinOS,waveOut,multi threaded"
//			"WinOS,VxD,low level hardware"
//			"WinOS,plugin,Director"
void BAEOutputMixer::GetDeviceName(long deviceID, char *cName, unsigned long cNameLength)
{
#if USE_DEVICE_ENUM_SUPPORT == TRUE
	GM_GetDeviceName(deviceID, cName, cNameLength);
#else
	deviceID = deviceID;
	cName = cName;
	cNameLength = cNameLength;
#endif
}

static TerpMode PV_GetDefaultTerp(BAETerpMode t)
{
	TerpMode	theTerp;

	switch (t)
	{
		#if USE_DROP_SAMPLE == TRUE
		case BAE_DROP_SAMPLE:
			theTerp = E_AMP_SCALED_DROP_SAMPLE;
			break;
		#endif
		#if USE_TERP1 == TRUE
		case BAE_2_POINT_INTERPOLATION:
			theTerp = E_2_POINT_INTERPOLATION;
			break;
		#endif
		default:
		case BAE_LINEAR_INTERPOLATION:
		#if USE_TERP2 == TRUE
			theTerp = E_LINEAR_INTERPOLATION;
		#endif
		#if USE_U3232_LOOPS == TRUE
			theTerp = E_LINEAR_INTERPOLATION_U3232;
		#endif
		#if USE_FLOAT_LOOPS == TRUE
			theTerp = E_LINEAR_INTERPOLATION_FLOAT;
		#endif
			break;
	}
	return theTerp;
}


BAEResult BAEOutputMixer::Open(BAEPathName pAudioPathName,
						BAEQuality q, BAETerpMode t, 
						BAEReverbType r, BAEAudioModifiers am,
						short int maxSongVoices,
						short int maxSoundVoices,
						short int mixLevel,
						BAE_BOOL engageAudio)
{
	OPErr			theErr;
	Quality			theQuality;
	TerpMode		theTerp;
	AudioModifiers	theMods;
	ReverbMode		theReverb;

	theErr = NO_ERR;
	// if we've never setup the audio engine, do that now
	if (audioSetup == 0)
	{		
#if (X_PLATFORM == X_MACINTOSH) && (CPU_TYPE == k68000)
		// we're running on a MacOS 68k, so we've got to restrict the features in order to get decent playback
		q = BAE_11K;
		am &= ~BAE_USE_STEREO;			// mono only
		am &= ~BAE_STEREO_FILTER;		// don't allow
		am |= BAE_DISABLE_REVERB;		// don't allow
//		am &= ~BAE_USE_16;
		switch (q)
		{
			case BAE_44K:		// no way
			case BAE_48K:
			case BAE_24K:
			case BAE_22K_TERP_44K:
				q = BAE_22K;
				break;
		}
		t = BAE_DROP_SAMPLE;

		switch (t)
		{
			case BAE_LINEAR_INTERPOLATION:
				t = BAE_2_POINT_INTERPOLATION;
				break;
		}
		// reverb doesn't matter because we're only using the shallow memory version anyways
		// and we force a disable
#endif

		switch (q)
		{
			case BAE_8K:
				theQuality = Q_8K;
				break;
			case BAE_11K:
				theQuality = Q_11K;
				break;
			case BAE_11K_TERP_22K:
				theQuality = Q_11K_TERP_22K;
				break;
			case BAE_22K_TERP_44K:
				theQuality = Q_22K_TERP_44K;
				break;
			case BAE_22K:
				theQuality = Q_22K;
				break;
			case BAE_24K:
				theQuality = Q_24K;
				break;
			case BAE_44K:
				theQuality = Q_44K;
				break;
			case BAE_48K:
				theQuality = Q_48K;
				break;
			default:
				theErr = PARAM_ERR;
				break;
		}
		
		switch (t)
		{
			case BAE_DROP_SAMPLE:
			case BAE_2_POINT_INTERPOLATION:
			case BAE_LINEAR_INTERPOLATION:
				theTerp = PV_GetDefaultTerp(t);
				break;
			default:
				theErr = PARAM_ERR;
				break;
		}

		switch (r)
		{
			case BAE_REVERB_NO_CHANGE:
				theReverb = BAE_TranslateFromBAEReverb(GetReverbType());
				break;
			case BAE_REVERB_TYPE_1:
				theReverb = REVERB_TYPE_1;
				break;
			case BAE_REVERB_TYPE_2:
				theReverb = REVERB_TYPE_2;
				break;
			case BAE_REVERB_TYPE_3:
				theReverb = REVERB_TYPE_3;
				break;
			case BAE_REVERB_TYPE_4:
				theReverb = REVERB_TYPE_4;
				break;
			case BAE_REVERB_TYPE_5:
				theReverb = REVERB_TYPE_5;
				break;
			case BAE_REVERB_TYPE_6:
				theReverb = REVERB_TYPE_6;
				break;
			case BAE_REVERB_TYPE_7:
				theReverb = REVERB_TYPE_7;
				break;
			case BAE_REVERB_TYPE_8:
				theReverb = REVERB_TYPE_8;
				break;
			case BAE_REVERB_TYPE_9:
				theReverb = REVERB_TYPE_9;
				break;
			case BAE_REVERB_TYPE_10:
				theReverb = REVERB_TYPE_10;
				break;
			case BAE_REVERB_TYPE_11:
				theReverb = REVERB_TYPE_11;
				break;
			default:
				theErr = PARAM_ERR;
				break;
		}

		theMods = M_NONE;
		if ((am & BAE_USE_16) && XIs16BitSupported())
		{
			theMods |= M_USE_16;
		}
		else
		{
			am &= BAE_USE_16;			// 8 bit
		}

		if ( (am & BAE_USE_STEREO) && XIsStereoSupported())
		{
			theMods |= M_USE_STEREO;
			if (am & BAE_STEREO_FILTER)
			{
				theMods |= M_STEREO_FILTER;
			}
		}
		else
		{
			am &= ~BAE_USE_STEREO;			// mono
		}
		if (am & BAE_DISABLE_REVERB)
		{
			theMods |= M_DISABLE_REVERB;
		}
#if X_PLATFORM == X_MACINTOSH
// make sure we have at least 1MB of free memory
		if (FreeMem() < (1024L * 1024L))
		{
			theErr = MEMORY_ERR;
		}
#endif

		// check to see if the version numbers match the header files and
		// the built codebase
		{
			short int major, minor, subminor;

			GetMixerVersion(&major, &minor, &subminor);
			if ((major != BAE_VERSION_MAJOR) || (minor != BAE_VERSION_MINOR) || 
				(subminor != BAE_VERSION_SUB_MINOR))
			{
				theErr = GENERAL_BAD;
			}
		}

		if (theErr == NO_ERR)
		{
			theErr = GM_InitGeneralSound(NULL, theQuality, theTerp, theMods,
											maxSongVoices,
											mixLevel,
											maxSoundVoices);
			if (theErr == NO_ERR)
			{
				audioSetup++;	// allocated

				iMidiVoices = maxSongVoices;
				iSoundVoices = maxSoundVoices;
				iMixLevel = mixLevel;

				iQuality = q;
				iTerpMode = t;
				iReverbMode = r;
				iModifiers = am;

				GM_SetReverbType(theReverb);

				if (pAudioPathName)
				{
#if USE_BANK_TEST
					void		*newFiles;

					// everythings ok, so open the file
					newFiles = PV_OpenToFileList((XFILE *)mOpenAudioFiles, mOpenAudioFileCount, pAudioPathName);
					if (newFiles)
					{
						mOpenAudioFiles = newFiles;
						mOpenAudioFileCount++;
					}
					else
					{
						theErr = BAD_FILE;
					}
#else
					XFILENAME		theFile;

					// everythings ok, so open the file
					XConvertNativeFileToXFILENAME(pAudioPathName, &theFile);
					thePatchFile = XFileOpenResource(&theFile, TRUE);
					if (thePatchFile)
					{
						XFileUseThisResourceFile(thePatchFile);
					}
					else
					{
						theErr = BAD_FILE;
					}
#endif
				}
				if (theErr == NO_ERR)
				{
					if (engageAudio)
					{
						theErr = GM_ResumeGeneralSound(NULL);
						if (theErr == NO_ERR)
						{
							audioSetup++;
							mIsAudioEngaged = TRUE;
						}
						else
						{
							mIsAudioEngaged = FALSE;
						}
					}
				}
			}
		}
	}
	else
	{
		theErr = NOT_REENTERANT;		// can't be reentrant
	}
	return BAE_TranslateOPErr(theErr);
}

void BAEOutputMixer::Close(void)
{
	short int	count;

	if (audioSetup > 0)
	{
		StopOutputToFile();		// just in case

		GM_SetAudioTask(NULL, NULL);
		mIsAudioEngaged = FALSE;

		if (audioSetup > 1)
		{
			audioSetup--;
			// Close up sound manager BEFORE releasing memory!
			GM_StopHardwareSoundManager(NULL);
		}
		if (audioSetup > 0)
		{
			audioSetup--;
			GM_FinisGeneralSound(NULL);
		}
	}

	if (thePatchFile)
	{
		XFileClose(thePatchFile);
		thePatchFile = 0;
	}
	for (count = 0; count < BAE_MAX_EVENTS; count++)
	{
		XDisposePtr(m_pEvents[count]);
		m_pEvents[count] = NULL;
	}

#if USE_BANK_TEST
	if (mOpenAudioFiles)
	{
		PV_CloseAllFromFileList((XFILE *)mOpenAudioFiles, mOpenAudioFileCount);
//		XDisposePtr(mOpenAudioFiles);
		mOpenAudioFiles = NULL;
		mOpenAudioFileCount = 0;
	}
#endif
}

// Get default bank URL
BAEResult BAEOutputMixer::GetURLFromAudioFile(char *pURL, unsigned long urlLength)
{
#if USE_CREATION_API == TRUE
	BankStatus	bank;
	BAEResult		theErr;

	theErr = BAE_NO_ERROR;
	if (audioSetup && pURL && urlLength)
	{
		pURL[0] = 0;
		XGetBankStatus(&bank);
		if ((unsigned long)XStrLen(bank.bankURL) == 0)
		{
			theErr = BAE_BAD_BANK;
		}
		else
		{
			if ((unsigned long)XStrLen(bank.bankURL) < (urlLength+1))
			{
				XStrCpy(pURL, bank.bankURL);
			}
		}
	}
	else
	{
		theErr = BAE_NOT_SETUP;
	}
	return theErr;
#else
	pURL = pURL;
	urlLength = urlLength;
	return BAE_NOT_SETUP;
#endif
}

// Get default bank name
BAEResult BAEOutputMixer::GetNameFromAudioFile(char *cName, unsigned long cLength)
{
#if USE_CREATION_API == TRUE
	BankStatus	bank;
	BAEResult		theErr;

	theErr = BAE_NO_ERROR;
	if (audioSetup && cName && cLength)
	{
		cName[0] = 0;
		XGetBankStatus(&bank);
		if ((unsigned long)XStrLen(bank.bankName) == 0)
		{
			theErr = BAE_BAD_BANK;
		}
		else
		{
			if ((unsigned long)XStrLen(bank.bankName) < (cLength+1))
			{
				XStrCpy(cName, bank.bankName);
			}
		}
	}
	else
	{
		theErr = BAE_NOT_SETUP;
	}
	return theErr;
#else
	cName = cName;
	cLength = cLength;
	return BAE_NOT_SETUP;
#endif
}

// Get version numbers from bank
void BAEOutputMixer::GetVersionFromAudioFile(short int *pVersionMajor, short int *pVersionMinor, short int *pVersionSubMinor)
{
	XVersion	vers;

	if (pVersionMajor && pVersionMinor && pVersionSubMinor)
	{
		*pVersionMajor = 0;
		*pVersionMinor = 0;
		*pVersionSubMinor = 0;
		if (audioSetup)
		{
			XGetVersionNumber(&vers);
			*pVersionMajor = vers.versionMajor;
			*pVersionMinor = vers.versionMinor;
			*pVersionSubMinor = vers.versionSubMinor;
		}
	}
}

// get audio file version numbers from specific bank
BAEResult BAEOutputMixer::GetVersionFromAudioFile(BAEPathName pAudioPathName, short int *pVersionMajor, short int *pVersionMinor, short int *pVersionSubMinor)
{
	BAEResult	theErr;
	XFILENAME	theFile;
	XFILE		file;
	XVersion	vers;

	theErr = BAE_NO_ERROR;
	if (pVersionMajor && pVersionMinor && pVersionSubMinor)
	{
		*pVersionMajor = 0;
		*pVersionMinor = 0;
		*pVersionSubMinor = 0;
		XConvertNativeFileToXFILENAME(pAudioPathName, &theFile);
		file = XFileOpenResource(&theFile, TRUE);
		if (file)
		{
			XFileUseThisResourceFile(file);
			XGetVersionNumber(&vers);
			*pVersionMajor = vers.versionMajor;
			*pVersionMinor = vers.versionMinor;
			*pVersionSubMinor = vers.versionSubMinor;
			XFileClose(file);
		}
		else
		{
			theErr = BAE_BAD_BANK;
		}
	}
	else
	{
		theErr = BAE_PARAM_ERR;
	}
	return theErr;
}


// Get names of songs that are included in the audio file. Call successively until
// the name is zero
void BAEOutputMixer::GetSongNameFromAudioFile(char *cSongName, 
												long *pID, BAEFileType *pSongType)
{
	long		size;
	XPTR		pData;
	long		id;

	if (audioSetup && cSongName)
	{
		cSongName[0] = 0;
		pData = NULL;
		if (songNameType == BAE_GROOVOID)
		{
			pData = XGetIndexedResource(ID_SONG, &id, songNameCount, cSongName, &size);
			if (pData == NULL)
			{
				songNameType = BAE_RMF;
				songNameCount = 0;
			}
		}
		if (songNameType == BAE_RMF)
		{
			pData = XGetIndexedResource(ID_RMF, &id, songNameCount, cSongName, &size);
			if (pData == NULL)
			{
				songNameType = BAE_GROOVOID;
				songNameCount = 0;
			}
		}
		if (pData)
		{
			XPtoCstr(cSongName);
			XDisposePtr(pData);
			songNameCount++;
			if (pID)
			{
				*pID = id;
			}
			if (pSongType)
			{
				*pSongType = songNameType;
			}
		}
	}
}

// Get names of samples that are included in the audio file. Call successively until
// the name is zero
void BAEOutputMixer::GetSampleNameFromAudioFile(char *cSampleName, long *pID)
{
	long		size;
	XPTR		pData;

	if (cSampleName && pID)
	{
		cSampleName[0] = 0;
		if (audioSetup)
		{
			// look for compressed version first
			pData = XGetIndexedResource(ID_CSND, pID, sampleNameCount, cSampleName, &size);
			if (pData == NULL)
			{
				// look for standard version
				pData = XGetIndexedResource(ID_SND, pID, sampleNameCount, cSampleName, &size);

				if (pData == NULL)
				{
					// look for encrypted version
					pData = XGetIndexedResource(ID_ESND, pID, sampleNameCount, cSampleName, &size);
				}
			}
			if (pData)
			{
				XPtoCstr(cSampleName);
				XDisposePtr(pData);
				sampleNameCount++;
			}
			else
			{
				sampleNameCount = 0;
				cSampleName[0] = 0;
			}
		}
	}
}

// Get names of instruments that are included in the audio file. Call successively until
// the name is zero
void BAEOutputMixer::GetInstrumentNameFromAudioFile(char *cInstrumentName, long *pID)
{
	long		size;
	XPTR		pData;

	if (cInstrumentName && pID)
	{
		cInstrumentName[0] = 0;
		if (audioSetup)
		{
			pData = XGetIndexedResource(ID_INST, pID, instrumentNameCount, cInstrumentName, &size);
			if (pData)
			{
				XPtoCstr(cInstrumentName);
				XDisposePtr(pData);
				instrumentNameCount++;
			}
			else
			{
				instrumentNameCount = 0;
				cInstrumentName[0] = 0;
			}
		}
	}
}

// Get names of instruments that are included in the audio file, referenced by ID only. Will return
// and error if instrument not found.
BAEResult BAEOutputMixer::GetInstrumentNameFromAudioFileFromID(char *cInstrumentName, long theID)
{
	BAEResult		theErr;

	theErr = BAE_NO_ERROR;
	if (cInstrumentName)
	{
		cInstrumentName[0] = 0;
		if (audioSetup)
		{
			XGetResourceName(ID_INST, theID, cInstrumentName);
			if (cInstrumentName[0] == 0)
			{
				theErr = BAE_BAD_INSTRUMENT;
			}
		}
	}
	return theErr;
}

// Get default bank name
BAEResult BAEOutputMixer::GetNameFromAudioFile(BAEPathName pAudioPathName, char *cName, unsigned long cLength)
{
	BAEResult		theErr;
	XFILENAME	theFile;
	XFILE		file;
	BankStatus	bank;

	theErr = BAE_BAD_BANK;
	XConvertNativeFileToXFILENAME(pAudioPathName, &theFile);
	file = XFileOpenResource(&theFile, TRUE);
	if (file)
	{
		XFileUseThisResourceFile(file);
		theErr = BAE_NO_ERROR;
		if (cName && cLength)
		{
			cName[0] = 0;
			XGetBankStatus(&bank);
			if ((unsigned long)XStrLen(bank.bankName) == 0)
			{
				theErr = BAE_BAD_BANK;
			}
			else
			{
				if ((unsigned long)XStrLen(bank.bankName) < (cLength+1))
				{
					XStrCpy(cName, bank.bankName);
				}
			}
		}
		else
		{
			theErr = BAE_NOT_SETUP;
		}
		XFileClose(file);
	}
	return theErr;
}


// Verify that the file passed in, is a valid audio bank file for BAE
BAEResult BAEOutputMixer::ValidateAudioFile(BAEPathName pAudioPathName)
{
	BAEResult		theErr;
	XFILENAME	theFile;
	XFILE		file;
	long		size;
	XVersion	*pData;

	theErr = BAE_BAD_BANK;
	XConvertNativeFileToXFILENAME(pAudioPathName, &theFile);
	file = XFileOpenResource(&theFile, TRUE);
	if (file)
	{
		XFileUseThisResourceFile(file);
		pData = (XVersion *)XGetAndDetachResource(ID_VERS, 0, &size);
		if (pData)
		{
			XDisposePtr(pData);
		}
		theErr = BAE_NO_ERROR;
		XFileClose(file);
	}
	return theErr;
}

// Flush read in or created cache for current AudioFile. TRUE allows cache, FALSE does not.
void BAEOutputMixer::SetCacheStatusForAudioFile(BAE_BOOL enableCache)
{
	mCacheStatus = enableCache;
}

BAE_BOOL BAEOutputMixer::GetCacheStatusForAudioFile(void)
{
	return mCacheStatus;
}

// Change audio file to use the passed in XFILE
BAEResult BAE_UseThisFile(XFILE audioFile, XBOOL closeOldFile)
{
	OPErr			theErr;
	XFILE			oldPatchFile;

	theErr = NO_ERR;
	oldPatchFile = thePatchFile;
	if (audioSetup)
	{
		thePatchFile = audioFile;
		XFileUseThisResourceFile(thePatchFile);

		if (closeOldFile)
		{
			// close old one, if different
			if (oldPatchFile)
			{
				if (oldPatchFile != audioFile)
				{
					XFileClose(oldPatchFile);
				}
			}
		}
	}
	else
	{
		thePatchFile = oldPatchFile;	// restore old file
		theErr = BAD_FILE;
	}
	return BAE_TranslateOPErr(theErr);
}

// change audio file
BAEResult BAEOutputMixer::ChangeAudioFile(BAEPathName pAudioPathName)
{
	OPErr			theErr;

	theErr = NO_ERR;
	if (audioSetup)
	{
#if USE_BANK_TEST
		void			*newFiles;

		PV_CloseAllFromFileList((XFILE *)mOpenAudioFiles, mOpenAudioFileCount);
		mOpenAudioFileCount = 0;
		// everythings ok, so open the file
		newFiles = PV_OpenToFileList((XFILE *)mOpenAudioFiles, mOpenAudioFileCount, pAudioPathName);
		if (newFiles)
		{
			mOpenAudioFiles = newFiles;
			mOpenAudioFileCount++;
		}
		else
		{
			theErr = BAD_FILE;
		}
#else
		XFILENAME		theFile;
		XFILE			oldPatchFile;

		if (pAudioPathName)
		{
			oldPatchFile = thePatchFile;
			XConvertNativeFileToXFILENAME(pAudioPathName, &theFile);
			thePatchFile = XFileOpenResource(&theFile, TRUE);
			if (thePatchFile)
			{
				XFileUseThisResourceFile(thePatchFile);
				// close old one, if different
				if (oldPatchFile)
				{
					if (oldPatchFile != thePatchFile)
					{
						XFileClose(oldPatchFile);
					}
				}
				if (mCacheStatus == FALSE)	// don't allow cache
				{
					XFileFreeResourceCache(thePatchFile);	// free cache
				}
			}
			else
			{
				thePatchFile = oldPatchFile;	// restore old file
				theErr = BAD_FILE;
			}
		}
		else
		{
			if (thePatchFile)
			{
				XFileClose(thePatchFile);
				thePatchFile = 0;
			}
		}
#endif
	}
	else
	{
		theErr = NOT_SETUP;		// mixer not allocated
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAEOutputMixer::ChangeAudioFileToMemory(void * pAudioFile, unsigned long fileSize)
{
	OPErr			theErr;
	XFILE			oldPatchFile;

	theErr = NO_ERR;
	if (audioSetup)
	{
		if (pAudioFile)
		{
			oldPatchFile = thePatchFile;
			thePatchFile = XFileOpenResourceFromMemory(pAudioFile, fileSize, FALSE);
			if (thePatchFile)
			{
				XFileUseThisResourceFile(thePatchFile);
				// close old one, if different
				if (oldPatchFile)
				{
					if (oldPatchFile != thePatchFile)
					{
						XFileClose(oldPatchFile);
					}
				}
			}
			else
			{
				thePatchFile = oldPatchFile;	// restore old file
				theErr = BAD_FILE;
			}
		}
		else
		{
			if (thePatchFile)
			{
				XFileClose(thePatchFile);
				thePatchFile = 0;
			}
		}
	}
	else
	{
		theErr = NOT_SETUP;		// mixer not allocated
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAEOutputMixer::ChangeAudioModes(BAEQuality q, BAETerpMode t, BAEAudioModifiers am)
{
	OPErr			theErr;
	Quality			theQuality;
	TerpMode		theTerp;
	AudioModifiers	theMods;

	if ( (iQuality == q) && (iTerpMode == t) && (iModifiers == am) )
	{
		// don't bother doing anything if everything is the same
		return BAE_NO_ERROR;
	}

	theErr = NO_ERR;
	switch (q)
	{
		case BAE_8K:
			theQuality = Q_8K;
			break;
		case BAE_11K_TERP_22K:
			theQuality = Q_11K_TERP_22K;
			break;
		case BAE_22K_TERP_44K:
			theQuality = Q_22K_TERP_44K;
			break;
		case BAE_11K:
			theQuality = Q_11K;
			break;
		case BAE_22K:
			theQuality = Q_22K;
			break;
		case BAE_24K:
			theQuality = Q_24K;
			break;
		case BAE_44K:
			theQuality = Q_44K;
			break;
		case BAE_48K:
			theQuality = Q_48K;
			break;
		default:
			theErr = PARAM_ERR;
			break;
	}
		
	switch (t)
	{
		case BAE_DROP_SAMPLE:
		case BAE_2_POINT_INTERPOLATION:
		case BAE_LINEAR_INTERPOLATION:
			theTerp = PV_GetDefaultTerp(t);
			break;
		default:
			theErr = PARAM_ERR;
			break;
	}

	theMods = M_NONE;
	if ((am & BAE_USE_16) && XIs16BitSupported())
	{
		theMods |= M_USE_16;
	}
	else
	{
		am &= ~BAE_USE_16;	// 8 bit
	}
	if ( (am & BAE_USE_STEREO) && XIsStereoSupported())
	{
		theMods |= M_USE_STEREO;
		if (am & BAE_STEREO_FILTER)
		{
			theMods |= M_STEREO_FILTER;
		}
	}
	else
	{
		am &= ~BAE_USE_STEREO;	// mono
	}
	if (am & BAE_DISABLE_REVERB)
	{
		theMods |= M_DISABLE_REVERB;
	}
	if (theErr == NO_ERR)
	{
		theErr = GM_ChangeAudioModes(NULL, theQuality, theTerp, theMods);
		if (theErr == NO_ERR)
		{
			iQuality = q;
			iTerpMode = t;
			iModifiers = am;
		}
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAEOutputMixer::ChangeSystemVoices(	short int maxSongVoices,
										short int maxSoundVoices,
										short int mixLevel)
{
	OPErr	theErr;

	theErr = GM_ChangeSystemVoices(maxSongVoices, mixLevel, maxSoundVoices);
	
	if (theErr == NO_ERR)
	{
		iMidiVoices = maxSongVoices;
		iSoundVoices = maxSoundVoices;
		iMixLevel = mixLevel;
	}
	return BAE_TranslateOPErr(theErr);
}

// return time in 60ths of a second
static unsigned long PV_GetTick(void)
{
#if USE_FLOAT != FALSE
	double time;
	
	time = (double)XMicroseconds() / 16666.7;
	return (unsigned long)time;
#else
	return (XMicroseconds() * 10) / 166667;
#endif
}

// Performance testing code
unsigned long BAEOutputMixer::MeasureCPUPerformance(void)
{
	unsigned long	count, loops;
	unsigned long	postTicks;

//	Wait to top of tick count
	postTicks = PV_GetTick();
	while (postTicks == PV_GetTick()) {};

	loops = 60L * 2;	// 2 seconds
//	count as fast as possible for counts per ticks
	count = 0;
	postTicks = PV_GetTick() + loops;
	while (PV_GetTick() <= postTicks)
	{
		count++;
	}
	return count / loops;
}

// reconfigure, by changing sample rates, interpolation modes, bit depth, etc based
// upon performance of host cpu
void BAEOutputMixer::PerformanceConfigure(void)
{
	unsigned long			raw_ticks, engaged_ticks, percentage;
	long					count;
	BAEQuality				q;
	BAETerpMode				t;
	BAEAudioModifiers		am;
	// under 10 is a very fast CPU
	static unsigned long	cpuGauge[] = {10, 15, 20, 25, 30, 35, 40, 45, 50, 55};

	if (audioSetup)
	{
		DisengageAudio();		// disenguage from hardware
		raw_ticks = MeasureCPUPerformance();
		ReengageAudio();		// enguage from hardware
		engaged_ticks = MeasureCPUPerformance();

		percentage = 100L - ((engaged_ticks * 100L) / raw_ticks);

		for (count = 0; count < 10; count++)
		{
			if (percentage < cpuGauge[count])
			{
				break;
			}
		}
		switch (10-count)
		{
			default:
				count = -1;
				break;
			case 10:			// 10 %
				q = BAE_22K;
				t = BAE_LINEAR_INTERPOLATION;
				am = BAE_USE_16 | BAE_USE_STEREO | BAE_STEREO_FILTER;
				break;
			case 9:				// 15 %
				q = BAE_22K;
				t = BAE_LINEAR_INTERPOLATION;
				am = BAE_USE_16 | BAE_USE_STEREO;
				break;			
			case 8:				// 20 %
				q = BAE_22K;
				t = BAE_LINEAR_INTERPOLATION;
				am = BAE_USE_16;
				break;
			case 7:				// 25 %
				q = BAE_22K;
				t = BAE_LINEAR_INTERPOLATION;
				am = BAE_USE_16 | BAE_DISABLE_REVERB;
				break;
			case 6:				// 30 %
				q = BAE_22K;
				t = BAE_LINEAR_INTERPOLATION;
				am = BAE_USE_16 | BAE_DISABLE_REVERB;
				break;
			case 5:				// 35 %
				q = BAE_22K;
				t = BAE_2_POINT_INTERPOLATION;
				am = BAE_USE_16 | BAE_DISABLE_REVERB;
				break;
			case 4:				// 40 %
				q = BAE_22K;
				t = BAE_DROP_SAMPLE;
				am = BAE_USE_16 | BAE_DISABLE_REVERB;
				break;
			case 3:				// 45 %
				q = BAE_22K;
				t = BAE_DROP_SAMPLE;
				am = BAE_DISABLE_REVERB;
				break;
			case 2:				// 50 %
				q = BAE_22K;
				t = BAE_DROP_SAMPLE;
				am = BAE_DISABLE_REVERB;
				break;
			case 1:				// 55 %
				q = BAE_11K;
				t = BAE_DROP_SAMPLE;
				am = BAE_DISABLE_REVERB;
				break;
		}
		if (count != -1)
		{
			ChangeAudioModes(q, t, am);
		}
	}
}

// return version of the BAE software.
void BAEOutputMixer::GetMixerVersion(short int *pVersionMajor, short int *pVersionMinor, short int *pVersionSubMinor)
{
	if (pVersionMajor && pVersionMinor && pVersionSubMinor)
	{
		*pVersionMajor = BAE_VERSION_MAJOR;
		*pVersionMinor = BAE_VERSION_MINOR;
		*pVersionSubMinor = BAE_VERSION_SUB_MINOR;
	}
}

// return CPU type this mixer is running on
BAECPUType BAEOutputMixer::GetCPUType(void)
{
	BAECPUType	type;

	type = BAE_CPU_UNKNOWN;

	#if X_PLATFORM == X_MACINTOSH
		#if GENERATING68K == FALSE
			type = BAE_CPU_POWERPC;
		#endif
	#endif
	#if X_PLATFORM == X_NAVIO
		#if CPU_TYPE == k80X86
			type = BAE_CPU_INTEL_PENTIUM;
		#endif
	#endif
	#if X_PLATFORM == X_WIN95
		#if CPU_TYPE == k80X86
			type = BAE_CPU_INTEL_PENTIUM;
			#if USE_KAT
				if (PV_IntelKatActive())
				{
					type = BAE_CPU_INTEL_PENTIUM3;
				}
			#endif
		#endif
	#endif
	#if X_PLATFORM == X_SOLARIS
		#if CPU_TYPE == kSPARC
			type = BAE_CPU_SPARC;
		#endif
	#endif
	return type;
}

BAEReverbType BAEOutputMixer::GetReverbType(void)
{
	ReverbMode		r;
	BAEReverbType	verb;

	r = GM_GetReverbType();
	verb = BAE_TranslateToBAEReverb(r);
	SetReverbType(verb);
	return iReverbMode;
}


void BAEOutputMixer::SetReverbType(BAEReverbType verb)
{
	ReverbMode	r;

	r = BAE_TranslateFromBAEReverb(verb);
	iReverbMode = BAE_TranslateToBAEReverb(r);

	GM_SetReverbType(r);
}

unsigned long BAEOutputMixer::GetTick(void)
{
	return GM_GetSyncTimeStamp();
}

// set current system audio lantency. This is platform specific. May not work on all
// platforms. Will return BAE_NOT_SETUP if not supported. Make sure you've set the current
// device with SetCurrentDevice prior to calling this.
BAEResult BAEOutputMixer::SetAudioLatency(unsigned long requestedLatency)
{
	#if X_PLATFORM == X_WIN95
	{
		BAEWinOSParameters	parms;
		long				device;

		device = GetCurrentDevice((void *)&parms);	// get current
		parms.synthFramesPerBlock = (requestedLatency / BAE_GetSliceTimeInMicroseconds()) + 1;
		SetCurrentDevice(device, (void *)&parms);	// set modified
		return BAE_NO_ERROR;
	}
	#else
	requestedLatency;
	#endif
	return BAE_NOT_SETUP;
}


unsigned long BAEOutputMixer::GetAudioLatency(void)
{
	return GM_GetSyncTimeStampQuantizedAhead() - GM_GetSyncTimeStamp();
}

void BAEOutputMixer::SetMasterVolume(BAE_UNSIGNED_FIXED theVolume)
{
	GM_SetMasterVolume((INT32)(UNSIGNED_FIXED_TO_LONG_ROUNDED(theVolume * MAX_MASTER_VOLUME)));
}

BAE_UNSIGNED_FIXED BAEOutputMixer::GetMasterVolume(void) const
{
	BAE_UNSIGNED_FIXED	value;

	value = UNSIGNED_RATIO_TO_FIXED(GM_GetMasterVolume(), MAX_MASTER_VOLUME);
	return value;
}

void BAEOutputMixer::SetHardwareVolume(BAE_UNSIGNED_FIXED theVolume)
{
	XSetHardwareVolume(FIXED_TO_SHORT_ROUNDED(theVolume * X_FULL_VOLUME));
}

BAE_UNSIGNED_FIXED BAEOutputMixer::GetHardwareVolume(void) const
{
	BAE_UNSIGNED_FIXED	value;

	value = UNSIGNED_RATIO_TO_FIXED(XGetHardwareVolume(), X_FULL_VOLUME);
	return value;
}

// get and set the hardware balance. Use -1.0 for full left, and 1.0 for full
// right; use 0 for center
void BAEOutputMixer::SetHardwareBalance(BAE_FIXED balance)
{
	BAE_SetHardwareBalance((short)FIXED_TO_LONG(balance * X_FULL_VOLUME));
}

BAE_FIXED BAEOutputMixer::GetHardwareBalance(void) const
{
	BAE_FIXED	balance;

	balance = RATIO_TO_FIXED(BAE_GetHardwareBalance(), X_FULL_VOLUME);
	return balance;
}


void BAEOutputMixer::SetMasterSoundEffectsVolume(BAE_UNSIGNED_FIXED theVolume)
{
	GM_SetEffectsVolume(FIXED_TO_SHORT_ROUNDED(theVolume * MAX_MASTER_VOLUME));
}

BAE_UNSIGNED_FIXED BAEOutputMixer::GetMasterSoundEffectsVolume(void) const
{
	BAE_UNSIGNED_FIXED	value;

	value = UNSIGNED_RATIO_TO_FIXED(GM_GetEffectsVolume(), MAX_MASTER_VOLUME);
	return value;
}

// display feedback information
// This will return the number of samples stored into the pLeft and pRight
// arrays. Usually 1024. This returns the current data points being sent
// to the hardware.
short int BAEOutputMixer::GetAudioSampleFrame(short int *pLeft, short int *pRight) const
{
	return GM_GetAudioSampleFrame(pLeft, pRight);
}

// Fill an array of 16 BAE_BOOL types with all songs playing particular status
void BAEOutputMixer::GetChannelMuteStatus(BAE_BOOL *pChannels)
{
	if (pChannels)
	{
		GM_GetChannelMuteStatus(NULL, pChannels);
	}
}

void BAEOutputMixer::GetChannelSoloStatus(BAE_BOOL *pChannels)
{
	if (pChannels)
	{
		GM_GetChannelSoloStatus(NULL, pChannels);
	}
}

// Get realtime information about the current synthisizer state
void BAEOutputMixer::GetRealtimeStatus(BAEAudioInfo *pStatus) const
{
	GM_AudioInfo	status;
	short int		count;
	BAEVoiceType	voiceType;

	if (pStatus)
	{
		GM_GetRealtimeAudioInformation(&status);
		XSetMemory(pStatus, (long)sizeof(BAEAudioInfo), 0);
		pStatus->voicesActive = status.voicesActive;
		for (count = 0; count < status.voicesActive; count++)
		{
			pStatus->voice[count] = status.voice[count];

			voiceType = BAE_UNKNOWN;
			switch (status.voiceType[count])
			{
				case MIDI_PCM_VOICE:
					voiceType = BAE_MIDI_PCM_VOICE;
					break;
				case SOUND_PCM_VOICE:
					voiceType = BAE_SOUND_PCM_VOICE;
					break;
			}
			pStatus->voiceType[count] = voiceType;
			pStatus->instrument[count] = status.patch[count];
			pStatus->scaledVolume[count] = status.scaledVolume[count];
			pStatus->midiVolume[count] = status.volume[count];
			pStatus->channel[count] = status.channel[count];
			pStatus->midiNote[count] = status.midiNote[count];
			if (status.pSong[count])
			{
				pStatus->userReference[count] = status.pSong[count]->userReference;
			}
		}
	}
}

// is the mixer connected to the audio hardware
BAE_BOOL BAEOutputMixer::IsAudioEngaged(void)
{
	return mIsAudioEngaged;
}

// disengage from audio hardware
BAEResult BAEOutputMixer::DisengageAudio(void)
{
	OPErr	err;
	
	err = GM_PauseGeneralSound(NULL);
	if (err == NO_ERR)
	{
		mIsAudioEngaged = FALSE;
		if (audioSetup > 1)
		{
			audioSetup--;
		}
	}
	return BAE_TranslateOPErr(err);
}

// reengage to audio hardware
BAEResult BAEOutputMixer::ReengageAudio(void)
{
	OPErr	err;
	
	err = GM_ResumeGeneralSound(NULL);
	if (err == NO_ERR)
	{
		mIsAudioEngaged = TRUE;
		if (audioSetup == 1)
		{
			audioSetup++;
		}
	}
	return BAE_TranslateOPErr(err);
}

// Get next event read only from the queue. If there's none left, return NULL
// Pull the next available event from the event queue
BAEOutputMixer::BAEEvent_Generic * BAEOutputMixer::GetNextReadOnlyIdleEvent(void)
{
	BAEEvent_Generic	*pEvent;
	short int			count;
	short int			newTail;

	newTail = mTailEvent;
	for (count = 0; count < BAE_MAX_EVENTS; count++)
	{
		pEvent = (BAEOutputMixer::BAEEvent_Generic *)m_pEvents[newTail];
		newTail++;
		// if we've overflowed then reset back to the begining
		if (newTail > (BAE_MAX_EVENTS-1))
		{
			newTail = 0;
		}

		if ((pEvent->event != BAE_DEAD_EVENT) && (pEvent->event != BAE_ALLOCATING))
		{
			return pEvent;
		}
	}
	return NULL;
}

// Find an empty slot in the queue, timestamp it, and return a pointer
BAEOutputMixer::BAEEvent_Generic * BAEOutputMixer::GetNextStorableIdleEvent(void)
{
	BAEEvent_Generic	*pTail, *pHead, *pStoredEvent;
	short int			newHead;

	newHead = mHeadEvent;
	pTail = (BAEOutputMixer::BAEEvent_Generic *)m_pEvents[mTailEvent];
	while (1)
	{
		pHead = (BAEOutputMixer::BAEEvent_Generic *)m_pEvents[newHead];	// get current write event pointer
		if (pHead->event == BAE_DEAD_EVENT)
		{
			pHead->event = BAE_ALLOCATING;
			pStoredEvent = pHead;
			newHead++;	// increment past this stored event
			// if we've overflowed then reset back to the begining
			if (newHead > (BAE_MAX_EVENTS-1))
			{
				// NOTE: 	This might be a problem. If we've actaully gotten more events than what
				//			is avaiable in the queue, currently the code will wrap and overwrite 
				//			and lose events. We need to deal with this in some way.
				//			perhaps just doing pEvent = NULL will work, but it needs to be studied.
				newHead = 0;
			}
			mHeadEvent = newHead;
			return pStoredEvent;
		}
		else
		{
			newHead++;
			// if we've overflowed then reset back to the begining
			if (newHead > (BAE_MAX_EVENTS-1))
			{
				// NOTE: 	This might be a problem. If we've actaully gotten more events than what
				//			is avaiable in the queue, currently the code will wrap and overwrite 
				//			and lose events. We need to deal with this in some way.
				//			perhaps just doing pEvent = NULL will work, but it needs to be studied.
				newHead = 0;
			}
			if (newHead == mTailEvent)
			{
				// bummer, we've wrapped around the buffer, so just bail at this point
				break;
			}
		}
	}
	return NULL;
}


BAE_EVENT_REFERENCE BAEOutputMixer::AddEventLoadInstrument(BAEMidiSynth *object, BAE_INSTRUMENT instrument)
{
	BAEEvent_LoadInstrument	*pEvent;
	BAE_EVENT_REFERENCE		ref;

	ref = 0;
	pEvent = (BAEEvent_LoadInstrument *)GetNextStorableIdleEvent();
	if (pEvent && object)
	{
		pEvent->object = object;
		pEvent->instrument = instrument;
		pEvent->event = BAE_LOAD_INSTRUMENT;	// activate
		ref = (BAE_EVENT_REFERENCE)pEvent;
	}
	return ref;
}

// note on that checks to see if an instrument needs to be loaded. 
BAE_EVENT_REFERENCE BAEOutputMixer::AddEventNoteOnWithLoad(BAEMidiSynth *object, unsigned char channel, 
																unsigned char note, 
																unsigned char velocity,
																unsigned long time)
{
	BAEEvent_NoteOnWithLoad	*pEvent;
	BAE_EVENT_REFERENCE		ref;

	ref = 0;
	pEvent = (BAEEvent_NoteOnWithLoad *)GetNextStorableIdleEvent();
	if (pEvent && object)
	{
		pEvent->object = object;
		pEvent->channel = channel;
		pEvent->note = note;
		pEvent->velocity = velocity;
		pEvent->time = time;
		pEvent->event = BAE_LOAD_INSTRUMENT;	// activate
		ref = (BAE_EVENT_REFERENCE)pEvent;
	}
	return ref;
}

BAE_EVENT_REFERENCE BAEOutputMixer::AddEventMetaEvent(BAEMidiSong *object, BAEMetaEventCallbackPtr callback, BAEMetaType type,
											void *pReference, void *pMetaText, long metaTextLength)
{
	BAEEvent_MetaEvent		*pEvent;
	BAE_EVENT_REFERENCE		ref;

	ref = 0;
	pEvent = (BAEEvent_MetaEvent *)GetNextStorableIdleEvent();
	if (pEvent && object)
	{
		pEvent->object = object;
		pEvent->callback = callback;
		pEvent->pReference = pReference;
		pEvent->type = type;
		pEvent->pMetaText = pMetaText;
		pEvent->metaTextLength = metaTextLength;
		pEvent->event = BAE_MIDI_META_EVENT;	// activate
		ref = (BAE_EVENT_REFERENCE)pEvent;
	}
	return ref;
}

BAE_EVENT_REFERENCE BAEOutputMixer::AddEventObjectDone(BAENoise *object, BAEDoneCallbackPtr callback, void *pReference)
{
	BAEEvent_ObjectDone		*pEvent;
	BAE_EVENT_REFERENCE		ref;
	BAEEventType			type;

	ref = 0;
	if (object)
	{
		// get our type make sure we can deal with it
		type = BAE_DEAD_EVENT;
		switch (object->GetType())
		{
			case BAENoise::SOUND_NOISE:
				type = BAE_SOUND_DONE;
				break;
			case BAENoise::SOUND_STREAM_NOISE:
				type = BAE_SOUND_STREAM_DONE;
				break;
			case BAENoise::RMF_NOISE:
				type = BAE_RMF_DONE;
				break;
			case BAENoise::MIDI_FILE_NOISE:
				type = BAE_MIDI_DONE;
				break;
		}

		if (type != BAE_DEAD_EVENT)
		{
			pEvent = (BAEEvent_ObjectDone *)GetNextStorableIdleEvent();
			if (pEvent)
			{
				pEvent->object = object;
				pEvent->callback = callback;
				pEvent->pReference = pReference;
				pEvent->event = type;
				ref = (BAE_EVENT_REFERENCE)pEvent;
			}
		}
	}
	return ref;
}

BAE_EVENT_REFERENCE BAEOutputMixer::AddEventControlerEvent(BAEMidiSong *object, BAEControlerCallbackPtr callback, void *pReference,
											short int channel, short int track, short int controler, short int value)
{
	BAEEvent_ControlerEvent	*pEvent;
	BAE_EVENT_REFERENCE		ref;

	ref = 0;
	pEvent = (BAEEvent_ControlerEvent *)GetNextStorableIdleEvent();
	if (pEvent && object)
	{
		pEvent->object = object;
		pEvent->callback = callback;
		pEvent->pReference = pReference;
		pEvent->channel = channel;
		pEvent->track = track;
		pEvent->controler = controler;
		pEvent->value = value;
		pEvent->event = BAE_MIDI_CONTROLER_EVENT;	// activate
		ref = (BAE_EVENT_REFERENCE)pEvent;
	}
	return ref;
}

void BAEOutputMixer::RemoveEvent(BAE_EVENT_REFERENCE reference)
{
	BAEEvent_Generic	*pEvent;

	pEvent = (BAEEvent_Generic *)reference;
	if (pEvent)
	{
		pEvent->event = BAE_DEAD_EVENT;
		if (pEvent->object)
		{
			pEvent->object->mEventReference = BAEOutputMixer::BAE_DEAD_EVENT;
		}
	}
}

// Call this during idle time to service audio streams, fades, and other idle
// time processes
void BAEOutputMixer::ServiceIdle(void)
{
	BAEEvent_Generic		*pEvent0;
	BAEEvent_ControlerEvent	*pEvent1;
	BAEEvent_ObjectDone		*pEvent2;
	BAEEvent_MetaEvent		*pEvent3;
	BAEEvent_LoadInstrument	*pEvent4;
	BAEEvent_NoteOnWithLoad	*pEvent5;
#if USE_LINKED_OBJECTS
	BAENoise	*pBAETop;

	// time to check objects
	pBAETop = (BAENoise *)pTop;
	while (pBAETop)
	{
		// do something interesting
		pBAETop = pBAETop->pNext;
	}
#endif

	// process event. Don't do a loop because the head and tail may change during this process.
	// mHeadEvent only gets modified by this code, and mTailEvent gets modified by the event adder.
	if (mHeadEvent != mTailEvent)
	{
		// dispatch events
		pEvent0 = (BAEEvent_Generic *)m_pEvents[mHeadEvent];
		if ((pEvent0->event != BAE_DEAD_EVENT) && (pEvent0->object))
		{
			switch (pEvent0->event)
			{
				case BAE_NOTE_ON_WITH_LOAD:
					pEvent5 = (BAEEvent_NoteOnWithLoad *)pEvent0;
					pEvent5->object->NoteOnWithLoad(pEvent5->channel, pEvent5->note, 
													pEvent5->velocity, pEvent5->time);
					break;
				case BAE_LOAD_INSTRUMENT:
					pEvent4 = (BAEEvent_LoadInstrument *)pEvent0;
					if (pEvent4->object->IsInstrumentLoaded(pEvent4->instrument) == FALSE)
					{
						pEvent4->object->LoadInstrument(pEvent4->instrument);
					}
					break;
				case BAE_MIDI_META_EVENT:
					pEvent3 = (BAEEvent_MetaEvent *)pEvent0;
					if (pEvent3->callback)
					{
						(*pEvent3->callback)(pEvent3->pReference, pEvent3->type, pEvent3->pMetaText, pEvent3->metaTextLength);
					}
					break;
				case BAE_MIDI_CONTROLER_EVENT:
					pEvent1 = (BAEEvent_ControlerEvent *)pEvent0;
					if (pEvent1->callback)
					{
						(*pEvent1->callback)(pEvent1->pReference, pEvent1->channel, pEvent1->track, pEvent1->controler, 
												pEvent1->value);
					}
					break;
				case BAE_MIDI_DONE:
				case BAE_RMF_DONE:
				case BAE_SOUND_DONE:
				case BAE_SOUND_STREAM_DONE:
					pEvent2 = (BAEEvent_ObjectDone *)pEvent0;
					if (pEvent2->callback)
					{
						(*pEvent2->callback)(pEvent2->pReference);
					}
					break;
			}
			pEvent0->event = BAE_DEAD_EVENT;
		}
		// increment for next event, and wrap around
		mHeadEvent++;
		if (mHeadEvent > (BAE_MAX_EVENTS-1))
		{
			mHeadEvent = 0;
		}
	}

	// process streams
#if USE_STREAM_API
	GM_AudioStreamService(NULL);
#endif
}

// get in realtime CPU load in microseconds used to create 11 ms worth
// of sample data.
unsigned long BAEOutputMixer::GetCPULoadInMicroseconds(void)
{
	return GM_GetMixerUsedTime();
}

// get in realtime CPU load in percent used to create 11 ms worth
// of sample data.
unsigned long BAEOutputMixer::GetCPULoadInPercent(void)
{
	return GM_GetMixerUsedTimeInPercent();
}


BAEAudioModifiers BAEOutputMixer::GetModifiers(void)
{
	return iModifiers;
}

BAETerpMode BAEOutputMixer::GetTerpMode(void)
{
	return iTerpMode;
}

BAEQuality BAEOutputMixer::GetQuality(void)
{
	return iQuality;
}

short int BAEOutputMixer::GetMidiVoices(void)
{
	INT16 song, mix, sound;

	GM_GetSystemVoices(&song, &mix, &sound);
	iMidiVoices = song;
	return iMidiVoices;
}

short int BAEOutputMixer::GetSoundVoices(void)
{
	INT16 song, mix, sound;

	GM_GetSystemVoices(&song, &mix, &sound);
	iSoundVoices = sound;
	return iSoundVoices;
}

short int BAEOutputMixer::GetMixLevel(void)
{
	INT16 song, mix, sound;

	GM_GetSystemVoices(&song, &mix, &sound);
	iMixLevel = mix;
	return iMixLevel;
}

static void PV_TaskCallbackGlue(void *context, void *reference)
{
	BAEOutputMixer		*pMixer;
	BAETaskCallbackPtr	callback;

	context;
	pMixer = (BAEOutputMixer *)reference;
	if (pMixer)
	{
		callback = pMixer->GetTaskCallback();
		if (callback)
		{
			(*callback)(pMixer->GetTaskReference());
		}
	}
}

// Set a interrupt level task callback
void BAEOutputMixer::SetTaskCallback(BAETaskCallbackPtr pCallback, void *reference)
{
	pTask = pCallback;
	mTaskReference = reference;
}

// start the task callback
void BAEOutputMixer::StartTaskCallback(void)
{
	GM_SetAudioTask(PV_TaskCallbackGlue, mTaskReference);
}

// Stop the task callback
void BAEOutputMixer::StopTaskCallback(void)
{
	GM_SetAudioTask(NULL, NULL);
}

// Set a output callback. This will call your output code. This is used to modify the
// sample output before its sent to the hardware. Be very careful here. Don't use to
// much time or the audio will skip
void BAEOutputMixer::SetOutputCallback(BAEOutputCallbackPtr pCallback)
{
	pOutput = pCallback;
}

// start the output callback
void BAEOutputMixer::StartOutputCallback(void)
{
	GM_SetAudioOutput((GM_AudioOutputCallbackPtr)pOutput);
}

// Stop the output callback
void BAEOutputMixer::StopOutputCallback(void)
{
	GM_SetAudioOutput(NULL);
}

// start saving audio output to a file
BAEResult BAEOutputMixer::StartOutputToFile(BAEPathName pAudioOutputFile, BAEFileType outputType)
{
#if USE_CREATION_API == TRUE
	OPErr			theErr;
	XFILENAME		theFile;

	theErr = NO_ERR;
	// close old one first
	if (mWritingToFile)
	{
		StopOutputToFile();
	}
	mWriteToFileType = outputType;
	if (outputType == BAE_RAW_PCM)
	{
		XConvertNativeFileToXFILENAME(pAudioOutputFile, &theFile);
		mWritingToFileReference = (void *)XFileOpenForWrite(&theFile, TRUE);
		if (mWritingToFileReference)
		{
			GM_StopHardwareSoundManager(NULL);		// disengage from hardware
			mWritingToFile = TRUE;
		}
		else
		{
			theErr = BAD_FILE;
		}
	}
	else
	{
		theErr = BAD_FILE_TYPE;
	}
	return BAE_TranslateOPErr(theErr);
#else
	pAudioOutputFile = pAudioOutputFile;
	return BAE_NOT_SETUP;
#endif
}

// Stop saving audio output to a file
void BAEOutputMixer::StopOutputToFile(void)
{
	if (mWritingToFile && mWritingToFileReference)
	{
		XFileClose((XFILE)mWritingToFileReference);
		mWritingToFileReference = NULL;
		mWritingDataBlock = NULL;
		GM_StartHardwareSoundManager(NULL);			// reconnect to hardware
	}
	mWritingToFile = FALSE;
}

BAEResult BAEOutputMixer::ServiceAudioOutputToFile(void)
{
	long						outputSize;
	long						sampleSize, channels;
	char						bufferData[8192];
	BAEResult						theErr;

	theErr = BAE_NO_ERROR;
	if (mWritingToFile && mWritingToFileReference)
	{
		sampleSize = (GetModifiers() & BAE_USE_16) ? 2 : 1;
		channels = (GetModifiers() & ~BAE_USE_STEREO) ? 2 : 1;
		outputSize = GM_GetAudioBufferOutputSize();
		if (outputSize)
		{
			if (outputSize < 8192)
			{
				BAE_BuildMixerSlice(NULL, &bufferData[0], 
											outputSize, 
											(unsigned long)(outputSize / channels / sampleSize));
				switch (mWriteToFileType)
				{
					case BAE_RAW_PCM:
						if (XFileWrite((XFILE)mWritingToFileReference, &bufferData[0], outputSize) == -1)
						{
							theErr = BAE_FILE_IO_ERROR;
						}
						break;
					default:
						theErr = BAE_BAD_FILE_TYPE;
						break;
				}
			}
		}
		else
		{
			theErr = BAE_BUFFER_TO_SMALL;
		}
	}
	else
	{
		theErr = BAE_NOT_SETUP;
	}
	return theErr;
}


// Class implemention for BAENoise

#if 0
	#pragma mark ### BAENoise class ###
#endif

BAENoise::BAENoise(BAEOutputMixer *pBAEOutputMixer, char const *cName, NoiseType type)
{
	mAudioMixer = pBAEOutputMixer;
	mName[0] = 0;
	mType = type;
	mEventReference = BAEOutputMixer::BAE_DEAD_EVENT;
	if (cName)
	{
		char	tempName[2048];
		
		XStrCpy(tempName, (char *)cName);
		if (XStrLen(tempName) > 63)
		{
			tempName[63] = 0;	// anything larger than 64 characters is truncated
		}
		XStrCpy(mName, tempName);
	}

	pNext = NULL;
	pGroupNext = NULL;
#if USE_LINKED_OBJECTS
	{
		BAENoise	*pBAETop, *pBAENext;

		// add to link
		pBAETop = (BAENoise *)pBAEOutputMixer->pTop;
		if (pBAETop)
		{
			pBAENext = NULL;
			while (pBAETop)
			{
				pBAENext = pBAETop;
				pBAETop = (BAENoise *)pBAETop->pNext;
			}
			if (pBAENext)
			{
				pBAENext->pNext = this;
			}
		}
		else
		{
			mAudioMixer->pTop = this;
		}
	}
#endif
}

BAENoise::~BAENoise()
{
#if USE_LINKED_OBJECTS
	BAENoise	*pBAETop;

	// remove link
	pBAETop = (BAENoise *)mAudioMixer->pTop;
	if (pBAETop != this)
	{
		while (pBAETop)
		{
			if (pBAETop->pNext == this)
			{
				pBAETop->pNext = this->pNext;
				break;
			}

			pBAETop = (BAEMidiSynth *)pBAETop->pNext;
		}
	}
	else
	{
		mAudioMixer->pTop = NULL;
	}
#endif

	// if object is being delete, make sure its not in an event queue
	if (mEventReference)
	{
		mAudioMixer->RemoveEvent(mEventReference);
	}
}

void BAENoise::SetType(NoiseType newType)
{
	mType = newType;
}

BAEOutputMixer * BAENoise::GetMixer(void)
{
	return mAudioMixer;
}

	
// Class implemention for BAEMidiSong

#if 0
	#pragma mark ### BAEMidiSong class ###
#endif

BAEMidiSong::BAEMidiSong(BAEOutputMixer *pBAEOutputMixer, 
						   char const* cName, 
						   void *userReference):
									BAEMidiSynth(pBAEOutputMixer, 
													cName, userReference)
{
	m_pTimeCallbackReference = NULL;
	m_pSongTimeCallback = NULL;
	m_pControllerCallbackReference = NULL;
	m_pControllerCallbackProc = NULL;
	mSongCallbackReference = NULL;
	m_pSongCallback = NULL;
	mSongMetaReference = NULL;
	m_pSongMetaCallback = NULL;
	mSongFinished = FALSE;
	SetType(MIDI_FILE_NOISE);		// set our specific type because we can't set it automaticly
	// most of the real variables get setup when the BAEMidiSynth
	// gets created
}

BAEMidiSong::~BAEMidiSong()
{
	Unload();
}

// Given a pointer to a file, load it into a BAEMidiSong object. 
//
// If duplicateObject is TRUE, then the pointer passed in will be duplicated. 
// You can free the memory pointer passed after success.
// If FALSE the user pointer will be used, but
// not copied. Don't delete the object until after you have deleted this object.
BAEResult BAEMidiSong::LoadFromMemory(void const* pMidiData, unsigned long midiSize, 
						BAE_BOOL duplicateObject, BAE_BOOL ignoreBadInstruments)
{
	SongResource		*pXSong;
	OPErr				theErr;
	XShortResourceID	theID;
	GM_Song				*pSong;
	XPTR				newData;
	BAEOutputMixer		*pMixer;

	if (m_pSongVariables)
	{
		Unload();
	}
	theErr = NO_ERR;
	if (duplicateObject)
	{
		newData = XNewPtr(midiSize);
		if (newData)
		{
			XBlockMove((XPTR)pMidiData, newData, midiSize);
			pMidiData = newData;
		}
		else
		{
			theErr = MEMORY_ERR;
		}
	}
	if (pMidiData && midiSize && (theErr == NO_ERR))
	{
		theID = midiSongCount++;	// runtime midi ID
		pMixer = BAENoise::GetMixer();
		pXSong = XNewSongPtr(SONG_TYPE_SMS,
							theID,
							pMixer->GetMidiVoices(),
							pMixer->GetMixLevel(),
							pMixer->GetSoundVoices(),
							BAE_TranslateFromBAEReverb(pMixer->GetReverbType()));
	
		if (pXSong)
		{
			m_pPerformanceVariables = (void *)pXSong;	// preserve for use later
			mPerformanceVariablesLength = (unsigned long)XGetPtrSize((XPTR)pXSong);

			pSong = GM_LoadSong(NULL, this, theID, (void *)pXSong,
										(void *)pMidiData,
										(long)midiSize,
										NULL,		// no callback
										TRUE,		// load instruments
										ignoreBadInstruments,
										&theErr);
			if (pSong)
			{
				// things are cool
				pSong->disposeSongDataWhenDone = duplicateObject;	// dispose of midi data
				GM_SetSongLoopFlag(pSong, FALSE);					// don't loop song

				pSong->userReference = (long)mReference;
				m_pSongVariables = pSong;				// preserve for use later
			}
			else
			{
				if (duplicateObject)
				{
					XDisposePtr((XPTR)pMidiData);
				}
			}
		}
		else
		{
			theErr = MEMORY_ERR;
			if (duplicateObject)
			{
				XDisposePtr((XPTR)pMidiData);
			}
		}
	}
	else
	{
		theErr = BAD_FILE;
	}
	return BAE_TranslateOPErr(theErr);
}

// read a song from a file into memory
BAEResult BAEMidiSong::LoadFromFile(const BAEPathName pMidiFilePath, BAE_BOOL ignoreBadInstruments)
{
	XFILENAME			name;
	XPTR				pMidiData;
	SongResource		*pXSong;
	long				midiSize;
	OPErr				theErr;
	XShortResourceID	theID;
	GM_Song				*pSong;
	BAEOutputMixer		*pMixer;

	if (m_pSongVariables)
	{
		Unload();
	}
	theErr = NO_ERR;
	XConvertNativeFileToXFILENAME(pMidiFilePath, &name);
	pMidiData = PV_GetFileAsData(&name, &midiSize);
if (!pMidiData) {
	fprintf(stderr, "PV_GetFileAsData(%s) failed\n", name.theFile);
}
	if (pMidiData)
	{
		theID = midiSongCount++;	// runtime midi ID
		pMixer = BAENoise::GetMixer();
		pXSong = XNewSongPtr(SONG_TYPE_SMS,
							theID,
							pMixer->GetMidiVoices(),
							pMixer->GetMixLevel(),
							pMixer->GetSoundVoices(),
							BAE_TranslateFromBAEReverb(pMixer->GetReverbType()));
	
		if (pXSong)
		{
			m_pPerformanceVariables = (void *)pXSong;	// preserve for use later
			mPerformanceVariablesLength = (unsigned long)XGetPtrSize((XPTR)pXSong);

			pSong = GM_LoadSong(NULL, this, theID, (void *)pXSong,
										pMidiData,
										midiSize,
										NULL,		// no callback
										TRUE,		// load instruments
										ignoreBadInstruments,
										&theErr);
			if (pSong)
			{
				// things are cool
				pSong->disposeSongDataWhenDone = TRUE;	// dispose of midi data
				GM_SetSongLoopFlag(pSong, FALSE);		// don't loop song

				pSong->userReference = (long)mReference;
				m_pSongVariables = pSong;				// preserve for use later
			}
			else
			{
fprintf(stderr, "GM_LoadSong() failed\n");
				XDisposePtr(pMidiData);
			}
		}
		else
		{
fprintf(stderr, "XNewSongPtr() failed\n");
			XDisposePtr(pMidiData);
			theErr = MEMORY_ERR;
		}
	}
	else
	{
fprintf(stderr, "General Failure\n");
		theErr = BAD_FILE;
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAEMidiSong::LoadFromBank(const char *cName, BAE_BOOL ignoreBadInstruments)
{
	SongResource		*pXSong;
	long				size;
	OPErr				theErr;
	XShortResourceID	theID;
	GM_Song				*pSong;

	if (m_pSongVariables)
	{
		Unload();
	}
	theErr = BAD_FILE;
#if X_PLATFORM != X_MACINTOSH
	// on all platforms except MacOS we need a valid open resource file. BAE's resource manager is designed
	// to fall back into the MacOS resource manager if no valid BAE file is open. So this test is removed
	// MacOS.
	if (thePatchFile)
#endif
	{
		pXSong = (SongResource *)XGetNamedResource(ID_SONG, const_cast<char *>(cName), &size);		// look for song
		if (pXSong)
		{
			m_pPerformanceVariables = (void *)pXSong;	// preserve for use later
			mPerformanceVariablesLength = (unsigned long)size;
			
			theID = midiSongCount++;	// runtime midi ID
			pSong = GM_LoadSong(NULL, this, theID, (void *)pXSong,
										NULL,
										0L,
										NULL,		// no callback
										TRUE,		// load instruments
										ignoreBadInstruments,
										&theErr);
			if (pSong)
			{
				// things are cool
				pSong->disposeSongDataWhenDone = TRUE;	// dispose of midi data
				GM_SetSongLoopFlag(pSong, FALSE);		// don't loop song

				pSong->userReference = (long)mReference;
				m_pSongVariables = pSong;				// preserve for use later
				theErr = NO_ERR;
			}
		}
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAEMidiSong::LoadFromID(unsigned long id, BAE_BOOL ignoreBadInstruments)
{
	SongResource		*pXSong;
	unsigned long		size;
	OPErr				theErr;
	GM_Song				*pSong;

	if (m_pSongVariables)
	{
		Unload();
	}
	theErr = BAD_FILE;
#if X_PLATFORM != X_MACINTOSH
	// on all platforms except MacOS we need a valid open resource file. BAE's resource manager is designed
	// to fall back into the MacOS resource manager if no valid BAE file is open. So this test is removed
	// MacOS.
	if (thePatchFile)
#endif
	{
		pXSong = (SongResource *)XGetAndDetachResource(ID_SONG, (XLongResourceID)id, (long *)&size);		// look for song
		if (pXSong)
		{
			m_pPerformanceVariables = (void *)pXSong;	// preserve for use later
			mPerformanceVariablesLength = size;

			pSong = GM_LoadSong(NULL, this, (XShortResourceID)id, (void *)pXSong,
										NULL,
										0L,
										NULL,		// no callback
										TRUE,		// load instruments
										ignoreBadInstruments,
										&theErr);
			if (pSong)
			{
				// things are cool
				pSong->disposeSongDataWhenDone = TRUE;	// dispose of midi data
				GM_SetSongLoopFlag(pSong, FALSE);		// don't loop song

				pSong->userReference = (long)mReference;
				m_pSongVariables = pSong;				// preserve for use later
				theErr = NO_ERR;
			}
		}
	}
	return BAE_TranslateOPErr(theErr);
}

// Preroll song, allocate on mix buss, but don't start sequencer.
// If useEmbeddedMixerSettings is TRUE then the mixer will be reconfigured
// to the embedded song settings. If FALSE, then song will attempt to start with the
// current mixer configuration.
// if autoLevel is TRUE, then the mixer will be auto magicly reconfigured based upon 
// how many songs are playing, etc
BAEResult BAEMidiSong::Preroll(BAE_BOOL useEmbeddedMixerSettings, BAE_BOOL autoLevel)
{
	OPErr theErr;

	theErr = NOT_SETUP;
	if (m_pSongVariables)
	{
		theErr = GM_PrerollSong((GM_Song *)m_pSongVariables, NULL, 
								(XBOOL)useEmbeddedMixerSettings, (XBOOL)autoLevel);
		SetDoneCallback(m_pSongCallback, mSongCallbackReference);
	}
	return BAE_TranslateOPErr(theErr);
}

// start song.
// If useEmbeddedMixerSettings is TRUE then the mixer will be reconfigured
// to the embedded song settings. If FALSE, then song will attempt to start with the
// current mixer configuration.
// if autoLevel is TRUE, then the mixer will be auto magicly reconfigured based upon 
// how many songs are playing, etc
BAEResult BAEMidiSong::Start(BAE_BOOL useEmbeddedMixerSettings, BAE_BOOL autoLevel)
{
	OPErr theErr;

	theErr = NOT_SETUP;
	if (m_pSongVariables)
	{
		mSongFinished = FALSE;
		theErr = GM_BeginSong((GM_Song *)m_pSongVariables, NULL, 
								(XBOOL)useEmbeddedMixerSettings, (XBOOL)autoLevel);
		SetDoneCallback(m_pSongCallback, mSongCallbackReference);
	}
	return BAE_TranslateOPErr(theErr);
}

void BAEMidiSong::Unload(void)
{
	Stop();
	Close();
}

// end song now
void BAEMidiSong::Stop(BAE_BOOL startFade)
{
	short int	songVolume;

	if (IsPaused())
	{
		Resume();
	}
	if (m_pSongVariables)
	{
		if (startFade)
		{
			songVolume = GM_GetSongVolume((GM_Song *)m_pSongVariables);
			GM_SetSongFadeRate((GM_Song *)m_pSongVariables, FLOAT_TO_FIXED(2.2),
															0, songVolume, TRUE);
		}
		else
		{
			GM_KillSongNotes((GM_Song *)m_pSongVariables);
			// End the song, but keep it active in the mixer. This is done so that we can still send
			// midi events to this object.
			GM_EndSongButKeepActive(NULL, (GM_Song *)m_pSongVariables);
			mSongFinished = TRUE;
		}
	}
}

static double PV_CalculateTimeDeltaForFade(BAE_FIXED sourceVolume, BAE_FIXED destVolume, BAE_FIXED timeInMiliseconds)
{
	double	delta;
	double	source, dest;
	double	time;

	source = FIXED_TO_FLOAT(sourceVolume);
	dest = FIXED_TO_FLOAT(destVolume);
	time = FIXED_TO_FLOAT(timeInMiliseconds) * 1000;

	delta = (dest - source) / (time / BAE_GetSliceTimeInMicroseconds());
	return delta;
}

// fade from source volume, to dest volume in time miliseconds. Always async
void BAEMidiSong::FadeFromToInTime(BAE_FIXED sourceVolume, BAE_FIXED destVolume, BAE_FIXED timeInMiliseconds)
{
	short int	source, dest;
	double		delta;
	INT16		minVolume;
	INT16		maxVolume;

	if (m_pSongVariables)
	{
		delta = PV_CalculateTimeDeltaForFade(sourceVolume, destVolume, timeInMiliseconds);
		delta = delta * -MAX_SONG_VOLUME;

		source = FIXED_TO_SHORT_ROUNDED(sourceVolume * MAX_SONG_VOLUME);
		dest = FIXED_TO_SHORT_ROUNDED(destVolume * MAX_SONG_VOLUME);
		minVolume = XMIN(source, dest);
		maxVolume = XMAX(source, dest);
		GM_SetSongFadeRate((GM_Song *)m_pSongVariables, FLOAT_TO_FIXED(delta), minVolume, maxVolume, FALSE);
	}
}

// fade song
void BAEMidiSong::Fade(BAE_BOOL doAsync)
{
	short int	songVolume;

	if (m_pSongVariables)
	{
		songVolume = GM_GetSongVolume((GM_Song *)m_pSongVariables);
		if (doAsync == FALSE)
		{
			// We're going to fade the song out and don't stop it	
			GM_SetSongFadeRate((GM_Song *)m_pSongVariables, FLOAT_TO_FIXED(2.2),
													0, songVolume, FALSE);
			while (GM_GetSongVolume((GM_Song *)m_pSongVariables) && 
					(GM_IsSongDone((GM_Song *)m_pSongVariables) == FALSE)) 
			{
				GetMixer()->ServiceAudioOutputToFile();
				GetMixer()->ServiceIdle();
				XWaitMicroseocnds(1000);
			}
		}
		else
		{
			GM_SetSongFadeRate((GM_Song *)m_pSongVariables, FLOAT_TO_FIXED(2.2),
															0, songVolume, FALSE);
		}
	}
}


// fade song
void BAEMidiSong::FadeTo(BAE_FIXED destVolume, BAE_BOOL doAsync)
{
	short int	songVolume, saveVolume, newSongVolume, saveNewSongVolume;
	XFIXED		delta;

	if (m_pSongVariables)
	{
		newSongVolume = FIXED_TO_SHORT_ROUNDED(destVolume * MAX_SONG_VOLUME);
		saveNewSongVolume = newSongVolume;
		// We're going to fade the song out before we stop it.		
		songVolume = GM_GetSongVolume((GM_Song *)m_pSongVariables);
		saveVolume = songVolume;

		if (newSongVolume < songVolume)
		{	// fade out
			songVolume = newSongVolume;
			newSongVolume = saveVolume;
			delta = FLOAT_TO_UNSIGNED_FIXED(2.2);
		}
		else
		{	// fade in
			delta = (XFIXED)FLOAT_TO_FIXED(-2.2);
		}
		if (doAsync == FALSE)
		{
			GM_SetSongFadeRate((GM_Song *)m_pSongVariables, delta,
													songVolume, newSongVolume, FALSE);
			while (GM_GetSongVolume((GM_Song *)m_pSongVariables) != saveNewSongVolume)
			{
				GetMixer()->ServiceAudioOutputToFile();
				GetMixer()->ServiceIdle();
				XWaitMicroseocnds(1000);
			}
		}
		else
		{
			GM_SetSongFadeRate((GM_Song *)m_pSongVariables, delta,
													songVolume, newSongVolume, FALSE);
		}
	}
}


// pause and resume song playback
void BAEMidiSong::Pause(void)
{
	if (m_pSongVariables)
	{
		GM_PauseSong((GM_Song *)m_pSongVariables);
	}
}

void BAEMidiSong::Resume(void)
{
	if (m_pSongVariables)
	{
		GM_ResumeSong((GM_Song *)m_pSongVariables);
	}
}

// currently paused
BAE_BOOL BAEMidiSong::IsPaused(void)
{
	BAE_BOOL	done;

	done = TRUE;
	if (m_pSongVariables)
	{
		if (mSongFinished == FALSE)
		{
			done = GM_IsSongPaused((GM_Song *)m_pSongVariables);
		}
		else
		{
			done = FALSE;
		}
	}
	return done;
}



// get ticks in midi ticks of length of song
unsigned long BAEMidiSong::GetTickLength(void)
{
	OPErr			theErr;
	unsigned long	ticks;

	ticks = 0;
	if (m_pSongVariables)
	{
		ticks = GM_GetSongTickLength((GM_Song *)m_pSongVariables, &theErr);
	}
	return ticks;
}

// set the current playback position of song in midi ticks
BAEResult BAEMidiSong::SetTickPosition(unsigned long ticks)
{
	OPErr	theErr;

	theErr = NO_ERR;
	if (m_pSongVariables)
	{
		theErr = GM_SetSongTickPosition((GM_Song *)m_pSongVariables, ticks);
	}
	return BAE_TranslateOPErr(theErr);
}

// get the current playback position of a song in midi ticks
unsigned long BAEMidiSong::GetTickPosition(void)
{
	if (m_pSongVariables)
	{
		return GM_SongTicks((GM_Song *)m_pSongVariables);
	}
	return 0;
}

// get ticks in microseconds of length of song
unsigned long BAEMidiSong::GetMicrosecondLength(void)
{
	OPErr			theErr;
	unsigned long	ticks;

	ticks = 0;
	if (m_pSongVariables)
	{
		ticks = GM_GetSongMicrosecondLength((GM_Song *)m_pSongVariables, &theErr);
	}
	return ticks;
}

// set the current playback position of song in microseconds
BAEResult BAEMidiSong::SetMicrosecondPosition(unsigned long ticks)
{
	OPErr	theErr;

	theErr = NO_ERR;
	if (m_pSongVariables)
	{
		theErr = GM_SetSongMicrosecondPosition((GM_Song *)m_pSongVariables, ticks);
	}
	return BAE_TranslateOPErr(theErr);
}

// get the current playback position of a song in microseconds
unsigned long BAEMidiSong::GetMicrosecondPosition(void)
{
	if (m_pSongVariables)
	{
		return GM_SongMicroseconds((GM_Song *)m_pSongVariables);
	}
	return 0;
}

// return the patches required to play this song
BAEResult BAEMidiSong::GetInstruments(BAE_INSTRUMENT *pArray768, short int *pReturnedCount)
{
#if USE_CREATION_API == TRUE
	OPErr				theErr;
	void				*pMidiData;
	XShortResourceID	instruments[MAX_INSTRUMENTS * MAX_BANKS];
	short int			count;

	theErr = NO_ERR;
	if (pArray768 && pReturnedCount && m_pSongVariables && m_pPerformanceVariables)
	{
		*pReturnedCount = 0;	// total number of patches loaded
		pMidiData = ((GM_Song *)m_pSongVariables)->midiData;
		if (pMidiData)
		{
			*pReturnedCount = (short int)GM_GetUsedPatchlist(m_pPerformanceVariables, 
															pMidiData, 
															XGetPtrSize(pMidiData), 
															instruments, 
															&theErr);
			for (count = 0; count < *pReturnedCount; count++)
			{
				pArray768[count] = (BAE_INSTRUMENT)instruments[count];
			}
		}
	}
	return BAE_TranslateOPErr(theErr);
#else
	pArray768 = pArray768;
	pReturnedCount = pReturnedCount;
	return BAE_NOT_SETUP;
#endif
}

static void PV_ProcessSongEndCallbacks(void *threadContext, GM_Song *pSong)
{
	BAEDoneCallbackPtr	pCallback;
	BAEMidiSong			*pSongObject;

	threadContext;
	if (pSong)
	{
		pSongObject = (BAEMidiSong *)GM_GetSongContext(pSong);
		if (pSongObject)
		{
			pCallback = (BAEDoneCallbackPtr)pSongObject->GetDoneCallback();
			if (pCallback)
			{
				(*pCallback)(pSongObject->GetDoneCallbackReference());
			}
		}
	}
}

// Set a call back when song is done
void BAEMidiSong::SetDoneCallback(BAEDoneCallbackPtr pSongCallback, void * pReference)
{
	// save callbacks
	mSongCallbackReference = pReference;
	m_pSongCallback = pSongCallback;

	if (m_pSongVariables)
	{
		GM_SetSongCallback((GM_Song *)m_pSongVariables, PV_ProcessSongEndCallbacks);
	}
}

static void PV_ProcessSongTimeCallbacks(void *threadContext, GM_Song *pSong, unsigned long currentMicroseconds, unsigned long currentMidiClock)
{
	BAETimeCallbackPtr	pCallback;
	BAEMidiSong			*pSongObject;

	threadContext;
	if (pSong)
	{
		pSongObject = (BAEMidiSong *)GM_GetSongContext(pSong);
		if (pSongObject)
		{
			pCallback = pSongObject->GetTimeCallback();
			if (pCallback)
			{
				(*pCallback)(pSongObject->GetTimeCallbackReference(), currentMicroseconds, currentMidiClock);
			}
		}
	}
}

// Set a call back during song processing
void BAEMidiSong::SetTimeCallback(BAETimeCallbackPtr pSongCallback, void *pReference)
{
	m_pTimeCallbackReference = pReference;
	m_pSongTimeCallback = pSongCallback;

	if (m_pSongVariables)
	{
		GM_SetSongTimeCallback((GM_Song *)m_pSongVariables, 
								PV_ProcessSongTimeCallbacks,
								0L);
	}
}

static void PV_ProcessSongMetaEventCallbacks(void *threadContext, GM_Song *pSong, 
									char type, void *pMetaText, long metaTextLength, INT16 currentTrack)
{
	BAEMetaEventCallbackPtr	pCallback;
	BAEMidiSong				*pSongObject;

	threadContext;
	currentTrack;
	if (pSong)
	{
		pSongObject = (BAEMidiSong *)GM_GetSongContext(pSong);
		if (pSongObject)
		{
			pCallback = pSongObject->GetMetaCallback();
			if (pCallback)
			{
				(*pCallback)(pSongObject->GetMetaCallbackReference(), 
									(BAEMetaType)type, pMetaText, metaTextLength);
			}
		}
	}
}

// Set a call back during song playback for meta events. Pass NULL to clear callback.
void BAEMidiSong::SetMetaEventCallback(BAEMetaEventCallbackPtr pSongCallback, void * pReference)
{
	mSongMetaReference = pReference;
	m_pSongMetaCallback = pSongCallback;

	if (m_pSongVariables)
	{
		GM_SetSongMetaEventCallback((GM_Song *)m_pSongVariables, 
								PV_ProcessSongMetaEventCallbacks, 
								0L);
	}
}

static void PV_ProcessSongControllerCallbacks(void *threadContext, GM_Song *pSong, void * reference, short int channel, short int track, short int controler, 
														short int value)
{
	BAEMidiSong				*pBAEMidi;
	BAEControlerCallbackPtr	callback;
	void					*pReference;

	threadContext;
	pSong;
	pBAEMidi = (BAEMidiSong *)reference;
	if (pBAEMidi)
	{
		callback = pBAEMidi->GetControlCallback();
		pReference = pBAEMidi->GetControlCallbackReference();
		if (callback)
		{
			(*callback)(pReference, channel, track, controler, value);
		}
	}
}
												
void BAEMidiSong::SetControlCallback(short int controller, BAEControlerCallbackPtr pControllerCallback, 
																							void * pReference)
{
	m_pControllerCallbackReference = pReference;
	m_pControllerCallbackProc = pControllerCallback;

	if (m_pSongVariables)
	{
		GM_SetControllerCallback((GM_Song *)m_pSongVariables, this, 
									PV_ProcessSongControllerCallbacks, controller);
	}
}


// poll to see if song is done
BAE_BOOL BAEMidiSong::IsDone(void)
{
	BAE_BOOL	done;

	done = TRUE;
	if (m_pSongVariables)
	{
		done = (BAE_BOOL)GM_IsSongDone((GM_Song *)m_pSongVariables);
	}
	return done;
}

BAE_BOOL BAEMidiSong::IsPlaying(void)
{
	if (IsDone() == FALSE)
	{
		return TRUE;
	}
	return FALSE;
}
	

// pass TRUE to loop song, FALSE to not loop
void BAEMidiSong::SetLoopFlag(BAE_BOOL loop)
{
	if (m_pSongVariables)
	{
		GM_SetSongLoopFlag((GM_Song *)m_pSongVariables, loop);
	}
}

BAE_BOOL BAEMidiSong::GetLoopFlag(void)
{
	if (m_pSongVariables)
	{
		return (BAE_BOOL)GM_GetSongLoopFlag((GM_Song *)m_pSongVariables);
	}
	return FALSE;
}

void BAEMidiSong::SetLoopMax(short int maxLoopCount)
{
	if (m_pSongVariables)
	{
		GM_SetSongLoopMax((GM_Song *)m_pSongVariables, maxLoopCount - 1);
	}
}

short int BAEMidiSong::GetLoopMax(void)
{
	short int	loop;

	loop = 0;
	if (m_pSongVariables)
	{
		loop = GM_GetSongLoopMax((GM_Song *)m_pSongVariables);
	}
	return loop;
}

// set song master tempo
void BAEMidiSong::SetMasterTempo(BAE_UNSIGNED_FIXED newTempo)
{
	if (m_pSongVariables)
	{
		GM_SetMasterSongTempo((GM_Song *)m_pSongVariables, newTempo);
	}
}

// get song master tempo
BAE_UNSIGNED_FIXED BAEMidiSong::GetMasterTempo(void)
{
	BAE_UNSIGNED_FIXED tempo;

	tempo = 0;
	if (m_pSongVariables)
	{
		tempo = GM_GetMasterSongTempo((GM_Song *)m_pSongVariables);
	}
	return tempo;
}


// Sets tempo in microsecond per quarter note
void BAEMidiSong::SetTempo(unsigned long newTempo)
{
	if (m_pSongVariables)
	{
		GM_SetSongTempo((GM_Song *)m_pSongVariables, newTempo);
	}
}

// returns tempo in microsecond per quarter note
unsigned long BAEMidiSong::GetTempo(void)
{
	unsigned long	tempo;

	tempo = 0;
	if (m_pSongVariables)
	{
		tempo = GM_GetSongTempo((GM_Song *)m_pSongVariables);
	}
	return tempo;
}

// sets tempo in beats per minute
void BAEMidiSong::SetTempoInBeatsPerMinute(unsigned long newTempo)
{
	if (m_pSongVariables)
	{
		GM_SetSongTempInBeatsPerMinute((GM_Song *)m_pSongVariables, newTempo);
	}
}

// returns tempo in beats per minute
unsigned long BAEMidiSong::GetTempoInBeatsPerMinute(void)
{
	unsigned long	tempo;

	tempo = 0;
	if (m_pSongVariables)
	{
		tempo = GM_GetSongTempoInBeatsPerMinute((GM_Song *)m_pSongVariables);
	}
	return tempo;
}

// Get embedded data types
short int BAEMidiSong::GetEmbeddedMidiVoices(void)
{
	short int song;

	song = 0;
	if (m_pSongVariables)
	{
		song = ((GM_Song *)m_pSongVariables)->maxSongVoices;
	}
	return song;
}

short int BAEMidiSong::GetEmbeddedSoundVoices(void)
{
	short int sound;

	sound = 0;
	if (m_pSongVariables)
	{
		sound = ((GM_Song *)m_pSongVariables)->maxEffectVoices;
	}
	return sound;
}

short int BAEMidiSong::GetEmbeddedMixLevel(void)
{
	short int mix;

	mix = 0;
	if (m_pSongVariables)
	{
		mix = ((GM_Song *)m_pSongVariables)->mixLevel;
	}
	return mix;
}

BAEReverbType BAEMidiSong::GetEmbeddedReverbType(void)
{
	ReverbMode		r;
	BAEReverbType	verb;

	verb = BAE_REVERB_NO_CHANGE;
	if (m_pSongVariables)
	{
		r = ((GM_Song *)m_pSongVariables)->defaultReverbType;
		verb = BAE_TranslateToBAEReverb(r);
	}
	return verb;
}


void BAEMidiSong::SetEmbeddedReverbType(BAEReverbType verb)
{
	ReverbMode	r;

	if (m_pSongVariables)
	{
		r = BAE_TranslateFromBAEReverb(verb);
		((GM_Song *)m_pSongVariables)->defaultReverbType = r;
	}
}

// get/set embedded volume type.
// NOTE: Does not change current settings only when Start is called
void BAEMidiSong::SetEmbeddedVolume(BAE_UNSIGNED_FIXED volume)
{
	if (m_pPerformanceVariables)
	{
		XSetSongVolume((SongResource *)m_pPerformanceVariables,
							FIXED_TO_SHORT_ROUNDED(volume * MAX_SONG_VOLUME));
	}
}

BAE_UNSIGNED_FIXED BAEMidiSong::GetEmbeddedVolume(void)
{
	BAE_UNSIGNED_FIXED	volume;

	volume = 0;
	if (m_pPerformanceVariables)
	{
		volume = UNSIGNED_RATIO_TO_FIXED(XGetSongVolume((SongResource *)m_pPerformanceVariables), MAX_SONG_VOLUME);
	}
	return volume;
}


// set embedded data types
void BAEMidiSong::SetEmbeddedMidiVoices(short int midiVoices)
{
	if (m_pSongVariables)
	{
		((GM_Song *)m_pSongVariables)->maxSongVoices = midiVoices;
	}
}

void BAEMidiSong::SetEmbeddedSoundVoices(short int soundVoices)
{
	if (m_pSongVariables)
	{
		((GM_Song *)m_pSongVariables)->maxEffectVoices = soundVoices;
	}
}

void BAEMidiSong::SetEmbeddedMixLevel(short int mixLevel)
{
	if (m_pSongVariables)
	{
		((GM_Song *)m_pSongVariables)->mixLevel = mixLevel;
	}
}


void BAEMidiSong::MuteTrack(unsigned short int track)
{
	if (m_pSongVariables)
	{
		GM_MuteTrack((GM_Song *)m_pSongVariables, track);
	}
}

void BAEMidiSong::UnmuteTrack(unsigned short int track)
{
	if (m_pSongVariables)
	{
		GM_UnmuteTrack((GM_Song *)m_pSongVariables, track);
	}
}

void BAEMidiSong::GetTrackMuteStatus(BAE_BOOL *pTracks)
{
	if (m_pSongVariables)
	{
		GM_GetTrackMuteStatus((GM_Song *)m_pSongVariables, pTracks);
	}
}

void BAEMidiSong::SoloTrack(unsigned short int track)
{
	if (m_pSongVariables)
	{
		GM_SoloTrack((GM_Song *)m_pSongVariables, track);
	}
}

void BAEMidiSong::UnSoloTrack(unsigned short int track)
{
	if (m_pSongVariables)
	{
		GM_UnsoloTrack((GM_Song *)m_pSongVariables, track);
	}
}

void BAEMidiSong::GetSoloTrackStatus(BAE_BOOL *pTracks)
{
	if (m_pSongVariables)
	{
		GM_GetTrackSoloStatus((GM_Song *)m_pSongVariables, pTracks);
	}
}

static SongInfo PV_TranslateInfoType(BAEInfoType infoType)
{
	SongInfo	info;

	switch (infoType)
	{
		default:
			info = I_INVALID;
			break;
		case TITLE_INFO:
			info = I_TITLE;
			break;
		case PERFORMED_BY_INFO:
			info = I_PERFORMED_BY;
			break;
		case COMPOSER_INFO:
			info = I_COMPOSER;
			break;
		case COPYRIGHT_INFO:
			info = I_COPYRIGHT;
			break;
		case PUBLISHER_CONTACT_INFO:
			info = I_PUBLISHER_CONTACT;
			break;
		case USE_OF_LICENSE_INFO:
			info = I_USE_OF_LICENSE;
			break;
		case LICENSE_TERM_INFO:
			info = I_LICENSE_TERM;
			break;
		case LICENSED_TO_URL_INFO:
			info = I_LICENSED_TO_URL;
			break;
		case EXPIRATION_DATE_INFO:
			info = I_EXPIRATION_DATE;
			break;
		case COMPOSER_NOTES_INFO:
			info = I_COMPOSER_NOTES;
			break;
		case INDEX_NUMBER_INFO:
			info = I_INDEX_NUMBER;
			break;
		case GENRE_INFO:
			info = I_GENRE;
			break;
		case SUB_GENRE_INFO:
			info = I_SUB_GENRE;
			break;
		case TEMPO_DESCRIPTION_INFO:
			info = I_TEMPO;
			break;
		case ORIGINAL_SOURCE_INFO:
			info = I_ORIGINAL_SOURCE;
			break;
	}
	return info;
}

// get size of info about this song file. Will an unsigned long
unsigned long BAEMidiSong::GetInfoSize(BAEInfoType infoType)
{
#if USE_FULL_RMF_SUPPORT == TRUE
	SongInfo		info;
	unsigned long	size;

	size = 0;
	info = PV_TranslateInfoType(infoType);
	if ((info != I_INVALID) && m_pSongVariables && m_pPerformanceVariables)
	{
		size = XGetSongInformationSize((SongResource *)m_pPerformanceVariables, mPerformanceVariablesLength, 
									info);
	}
	return size;
#else
	infoType;
	return 0;
#endif
}

// get info about this song file. Will return a 'C' string
BAEResult BAEMidiSong::GetInfo(BAEInfoType infoType, char *cInfo)
{
#if USE_FULL_RMF_SUPPORT == TRUE
	SongInfo	info;
	BAEResult		theErr;

	theErr = BAE_NO_ERROR;
	cInfo[0] = 0;
	info = PV_TranslateInfoType(infoType);

	if ((info != I_INVALID) && m_pSongVariables && mPerformanceVariablesLength)
	{
		XGetSongInformation((SongResource *)m_pPerformanceVariables, mPerformanceVariablesLength, 
									info, cInfo);

#if (X_PLATFORM != X_MACINTOSH)
		// data stored in the copyright fields is Mac ASCII, any other platform should translate
		while (*cInfo)
		{
			*cInfo = XTranslateMacToWin(*cInfo);
			cInfo++;
		}
#endif
	}
	else
	{
		theErr = BAE_PARAM_ERR;
	}
	return theErr;
#else
	infoType;
	cInfo[0] = 0;
	return BAE_NOT_SETUP;
#endif
}

// Class implemention for BAERmfSong

#if 0
	#pragma mark ### BAERmfSong class ###
#endif

BAERmfSong::BAERmfSong(BAEOutputMixer *pBAEOutputMixer, 
						   char const* cName, 
						   void * userReference):
									BAEMidiSong(pBAEOutputMixer, 
													cName, userReference)
{
	// most of the real variables get setup when the BAEMidiSynth and BAEMidiSong
	// gets created
	m_pRMFDataBlock = NULL;
	SetType(RMF_NOISE);		// set our specific type because we can't do it automaticly
}

BAERmfSong::~BAERmfSong()
{
	Unload();
	XDisposePtr((XPTR)m_pRMFDataBlock);
	m_pRMFDataBlock = NULL;
}

// is this RMF file encrypted?
BAE_BOOL BAERmfSong::IsEncrypted() const
{
	BAE_BOOL	locked;

	locked = FALSE;
	if (m_pPerformanceVariables)
	{
		locked = (BAE_BOOL)XIsSongLocked((SongResource *)m_pPerformanceVariables);
	}
	return locked;
}

// is this RMF file compressed?
BAE_BOOL BAERmfSong::IsCompressed() const
{
	BAE_BOOL	compressed;

	compressed = FALSE;
	if (m_pPerformanceVariables)
	{
		compressed = (BAE_BOOL)XIsSongCompressed((SongResource *)m_pPerformanceVariables);
	}
	return compressed;
}

BAEResult BAERmfSong::LoadFromFile(const BAEPathName pRMFFilePath, BAE_BOOL ignoreBadInstruments)
{
	XFILE				fileRef;
	XFILENAME			name;
	SongResource		*pXSong;
	GM_Song				*pSong;
	OPErr				theErr;
	XLongResourceID		theID;
	long				size;

	theErr = BAD_FILE;

	if (m_pSongVariables)
	{
		Unload();
	}
	XConvertNativeFileToXFILENAME(pRMFFilePath, &name);
	fileRef = XFileOpenResource(&name, TRUE);
	if (fileRef)
	{
		// look for first song. RMF files only contain one SONG resource
		pXSong = (SongResource *)XGetIndexedResource(ID_SONG, &theID, 0, NULL, &size);
		if (pXSong)
		{
			m_pPerformanceVariables = (void *)pXSong;	// preserve for use later
			mPerformanceVariablesLength = (unsigned long)size;

			pSong = GM_LoadSong(NULL, this, (XShortResourceID)theID, (void *)pXSong,
										NULL,
										0L,
										NULL,		// no callback
										TRUE,		// load instruments
										ignoreBadInstruments,
										&theErr);
			if (pSong)
			{
				// things are cool
				pSong->disposeSongDataWhenDone = TRUE;	// dispose of midi data
				GM_SetSongLoopFlag(pSong, FALSE);		// don't loop song

				pSong->userReference = (long)mReference;
				m_pSongVariables = pSong;				// preserve for use later
				theErr = NO_ERR;
			}
		}
		XFileClose(fileRef);
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAERmfSong::LoadFromMemory(void const* pRMFData, unsigned long rmfSize, 
									BAE_BOOL duplicateObject,
									BAE_BOOL ignoreBadInstruments)
{
	XFILE				fileRef;
	SongResource		*pXSong;
	GM_Song				*pSong;
	OPErr				theErr;
	XLongResourceID		theID;
	long				size;
	void				*newData;

	theErr = NO_ERR;
	if (m_pSongVariables)
	{
		Unload();
	}
	if (pRMFData && rmfSize)
	{
		if (duplicateObject)
		{
			XDisposePtr(m_pRMFDataBlock);
			newData = XNewPtr(rmfSize);
			if (newData)
			{
				XBlockMove((XPTR)pRMFData, newData, rmfSize);
				m_pRMFDataBlock = newData;
				pRMFData = newData;
			}
			else
			{
				theErr = MEMORY_ERR;
			}
		}
		if (theErr == NO_ERR)
		{
			fileRef = XFileOpenResourceFromMemory((XPTR)pRMFData, rmfSize, 
													(duplicateObject) ? FALSE : TRUE);
			if (fileRef)
			{
				// look for first song. RMF files only contain one SONG resource
				pXSong = (SongResource *)XGetIndexedResource(ID_SONG, &theID, 0, NULL, &size);
				if (pXSong)
				{
					m_pPerformanceVariables = (void *)pXSong;	// preserve for use later
					mPerformanceVariablesLength = (unsigned long)size;

					pSong = GM_LoadSong(NULL, this, (XShortResourceID)theID, (void *)pXSong,
												NULL,
												0L,
												NULL,		// no callback
												TRUE,		// load instruments
												ignoreBadInstruments,
												&theErr);
					if (pSong)
					{
						// things are cool
						pSong->disposeSongDataWhenDone = TRUE;	// dispose of midi data
						GM_SetSongLoopFlag(pSong, FALSE);		// don't loop song
	
						pSong->userReference = (long)mReference;
						m_pSongVariables = pSong;				// preserve for use later
					}
				}
				XFileClose(fileRef);
			}
			else
			{
				theErr = BAD_FILE;
			}
		}
		if (theErr)
		{
			XDisposePtr(m_pRMFDataBlock);
			m_pRMFDataBlock = NULL;
		}
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAERmfSong::LoadFromBank(char const* cName, BAE_BOOL ignoreBadInstruments)
{
	XPTR		pRMF;
	long		size;
	BAEResult		theErr;

	theErr = BAE_BAD_FILE;

	if (m_pSongVariables)
	{
		Unload();
	}
	pRMF = XGetNamedResource(ID_RMF, (void *)cName, &size);		// look for embedded RMF song
	if (pRMF)
	{
		theErr = LoadFromMemory(pRMF, size, TRUE, ignoreBadInstruments);
		XDisposePtr(pRMF);
	}
	return theErr;
}

BAEResult BAERmfSong::LoadFromID(unsigned long id, BAE_BOOL ignoreBadInstruments)
{
	XPTR		pRMF;
	long		size;
	BAEResult		theErr;

	theErr = BAE_BAD_FILE;

	if (m_pSongVariables)
	{
		Unload();
	}
	pRMF = XGetAndDetachResource(ID_RMF, (XLongResourceID)id, &size);		// look for embedded RMF song
	if (pRMF)
	{
		theErr = LoadFromMemory(pRMF, size, TRUE, ignoreBadInstruments);
		XDisposePtr(pRMF);
	}
	return theErr;
}

// Class implemention for BAEMidiSynth

#if 0
	#pragma mark ### BAEMidiSynth class ###
#endif

BAEMidiSynth::BAEMidiSynth(BAEOutputMixer *pBAEOutputMixer, char const* cName, 
							   void* userReference) :
									BAENoise(pBAEOutputMixer, cName, BAENoise::MIDI_NOISE)
{
	m_pPerformanceVariables = NULL;
	mPerformanceVariablesLength = 0;
	mQueueMidi = TRUE;
	mReference = userReference;
	m_pSongVariables = NULL;
}

BAEMidiSynth::~BAEMidiSynth()
{
	Close();
}

BAEResult BAEMidiSynth::Open(BAE_BOOL loadInstruments)
{
	GM_Song			*pSong;
	OPErr			theErr;
	BAEOutputMixer	*pMixer;

	theErr = NO_ERR;
	if (m_pSongVariables)
	{
		Close();
	}
	pSong = GM_CreateLiveSong(this, midiSongCount++);
	if (pSong)
	{
		pMixer = GetMixer();
		pSong->maxSongVoices = pMixer->GetMidiVoices();
		pSong->maxEffectVoices = pMixer->GetSoundVoices();
		pSong->mixLevel = pMixer->GetMixLevel();
		GM_SetCacheSamples(pSong, TRUE);

		pSong->userReference = (long)mReference;
		theErr = GM_StartLiveSong(pSong, loadInstruments);
		if (theErr)
		{
			while (GM_FreeSong(NULL, pSong) == STILL_PLAYING)
			{
				XWaitMicroseocnds(BAE_GetSliceTimeInMicroseconds());
			}
			pSong = NULL;
		}
	}
	else
	{
		theErr = MEMORY_ERR;
	}
	m_pSongVariables = (void *)pSong;
	return BAE_TranslateOPErr(theErr);
}

void BAEMidiSynth::Close(void)
{
	if (m_pSongVariables)
	{
		GM_SetCacheSamples((GM_Song *)m_pSongVariables, FALSE);
		GM_KillSongNotes((GM_Song *)m_pSongVariables);
		while (GM_FreeSong(NULL, (GM_Song *)m_pSongVariables) == STILL_PLAYING)
		{
			XWaitMicroseocnds(BAE_GetSliceTimeInMicroseconds());
		}
		m_pSongVariables = NULL;
	}
	if (m_pPerformanceVariables)
	{
		XDisposeSongPtr((SongResource *)m_pPerformanceVariables);
		m_pPerformanceVariables = NULL;
		mPerformanceVariablesLength = 0;
	}
}

BAE_BOOL BAEMidiSynth::IsLoaded(void)
{
	if (m_pSongVariables)
	{
		return TRUE;
	}
	return FALSE;
}

// return controller value
char BAEMidiSynth::GetControlValue(unsigned char channel,  unsigned char controller)
{
	char	value;

	value = 0;
	if (m_pSongVariables)
	{
		value = GM_GetControllerValue((GM_Song *)m_pSongVariables, channel, controller);
	}
	return value;
}

void BAEMidiSynth::SetCacheSample(BAE_BOOL cacheSamples)
{
	if (m_pSongVariables)
	{
		GM_SetCacheSamples((GM_Song *)m_pSongVariables, cacheSamples);
	}
}
		
BAE_BOOL BAEMidiSynth::GetCacheSample(void)
{
	BAE_BOOL	value;

	value = 0;
	if (m_pSongVariables)
	{
		value = GM_GetCacheSamples((GM_Song *)m_pSongVariables);
	}
	return value;
}
		
// Get the current Midi program and bank values
void BAEMidiSynth::GetProgramBank(unsigned char channel, 
									unsigned char *pProgram,
									unsigned char *pBank)
{
	if (m_pSongVariables && pBank && pProgram)
	{
		*pProgram = (unsigned char)((GM_Song *)m_pSongVariables)->channelProgram[channel];
		*pBank = ((GM_Song *)m_pSongVariables)->channelBank[channel];
	}
}

void BAEMidiSynth::MuteChannel(unsigned short int channel)
{
	if (m_pSongVariables)
	{
		GM_MuteChannel((GM_Song *)m_pSongVariables, channel);
	}
}

void BAEMidiSynth::UnmuteChannel(unsigned short int channel)
{
	if (m_pSongVariables)
	{
		GM_UnmuteChannel((GM_Song *)m_pSongVariables, channel);
	}
}

void BAEMidiSynth::GetChannelMuteStatus(BAE_BOOL *pChannels)
{
	if (m_pSongVariables)
	{
		GM_GetChannelMuteStatus((GM_Song *)m_pSongVariables, pChannels);
	}
}

void BAEMidiSynth::SoloChannel(unsigned short int channel)
{
	if (m_pSongVariables)
	{
		GM_SoloChannel((GM_Song *)m_pSongVariables, channel);
	}
}

void BAEMidiSynth::UnSoloChannel(unsigned short int channel)
{
	if (m_pSongVariables)
	{
		GM_UnsoloChannel((GM_Song *)m_pSongVariables, channel);
	}
}

void BAEMidiSynth::GetChannelSoloStatus(BAE_BOOL *pChannels)
{
	if (m_pSongVariables)
	{
		GM_GetChannelSoloStatus((GM_Song *)m_pSongVariables, pChannels);
	}
}


// set song volume. You can overdrive by passing values larger than 1.0
void BAEMidiSynth::SetVolume(BAE_UNSIGNED_FIXED volume)
{
	if (m_pSongVariables)
	{
		GM_SetSongVolume((GM_Song *)m_pSongVariables,
							FIXED_TO_SHORT_ROUNDED(volume * MAX_SONG_VOLUME));
	}
}

// get the song volume
BAE_UNSIGNED_FIXED BAEMidiSynth::GetVolume(void)
{
	BAE_UNSIGNED_FIXED	volume;

	volume = 0;
	if (m_pSongVariables)
	{
		volume = UNSIGNED_RATIO_TO_FIXED(GM_GetSongVolume((GM_Song *)m_pSongVariables),
										MAX_SONG_VOLUME);
	}
	return volume;
}


// Set the master stereo position of a BAEMidiSynth (-63 left to 63 right, 0 is middle)
void BAEMidiSynth::SetStereoPosition(short int stereoPosition)
{
	if (m_pSongVariables)
	{
		GM_SetSongStereoPosition((GM_Song *)m_pSongVariables, stereoPosition);
	}
}

// Set the master stereo position of a BAEMidiSynth (-63 left to 63 right, 0 is middle)
short int BAEMidiSynth::GetStereoPosition(void)
{
	short int stereoPosition;

	stereoPosition = 0;
	if (m_pSongVariables)
	{
		stereoPosition = GM_GetSetStereoPosition((GM_Song *)m_pSongVariables);
	}
	return stereoPosition;
}

// If allowPitch is FALSE, then "SetPitchOffset" will have no effect on passed 
// channel (0 to 15)
void BAEMidiSynth::AllowChannelPitchOffset(unsigned short int channel, BAE_BOOL allowPitch)
{
	if (m_pSongVariables)
	{
		GM_AllowChannelPitchOffset((GM_Song *)m_pSongVariables, channel, allowPitch);
	}
}

// Return if the passed channel will allow pitch offset
BAE_BOOL BAEMidiSynth::DoesChannelAllowPitchOffset(unsigned short int channel)
{
	BAE_BOOL	flag;

	flag = FALSE;
	if (m_pSongVariables)
	{
		flag = GM_DoesChannelAllowPitchOffset((GM_Song *)m_pSongVariables, channel);
	}
	return flag;
}

// set note offset in semi tones	(-12 is down an octave, 12 is up an octave)
void BAEMidiSynth::SetPitchOffset(long offset)
{
	if (m_pSongVariables)
	{
		if ( (offset > -128) && (offset < 128) )
		{
			GM_SetSongPitchOffset((GM_Song *)m_pSongVariables, offset);
		}
	}
}

// return note offset in semi tones	(-12 is down an octave, 12 is up an octave)
long BAEMidiSynth::GetPitchOffset(void)
{
	long	offset;

	offset = 0;
	if (m_pSongVariables)
	{
		offset = GM_GetSongPitchOffset((GM_Song *)m_pSongVariables);
	}
	return offset;
}

BAE_BOOL BAEMidiSynth::IsInstrumentLoaded(BAE_INSTRUMENT instrument)
{
	return (BAE_BOOL)GM_IsSongInstrumentLoaded((GM_Song *)m_pSongVariables, (XLongResourceID)instrument);
}

/*
	void			TranslateInstrumentToBankProgram(BAE_INSTRUMENT instrument, 
														unsigned short *pBank, 
														unsigned short *pProgram);
void BAEMidiSynth::TranslateInstrumentToBankProgram(BAE_INSTRUMENT instrument, 
														unsigned short *pBank, 
														unsigned short *pProgram)
{
	if (pBank && pProgram)
	{
		*pBank = instrument / 128;
		*pProgram = instrument % 128;
	}
}
*/

BAE_INSTRUMENT BAEMidiSynth::TranslateBankProgramToInstrument(unsigned short bank, 
											unsigned short program, unsigned short channel, unsigned short note)
{
	BAE_INSTRUMENT	instrument;

	instrument = program;
	if (channel == PERCUSSION_CHANNEL)
	{
		bank = (bank * 2) + 1;		// odd banks are percussion
	}
	else
	{
		bank = bank * 2 + 0;		// even banks are for instruments
		note = 0;
	}

	if (bank < MAX_BANKS)
	{
		instrument = (bank * 128) + program + note;
	}

	return instrument;
}


BAEResult BAEMidiSynth::LoadInstrument(BAE_INSTRUMENT instrument)
{
	OPErr	theErr;

	theErr = NO_ERR;
	if (m_pSongVariables)
	{
		theErr = GM_LoadSongInstrument((GM_Song *)m_pSongVariables, (XLongResourceID)instrument);
	}
	else
	{
		theErr = NOT_SETUP;
	}
	return BAE_TranslateOPErr(theErr);
}

// load an instrument with custom patch data.
BAEResult BAEMidiSynth::LoadInstrumentFromData(BAE_INSTRUMENT instrument, void *data, unsigned long dataSize)
{
	OPErr	theErr;

	theErr = NO_ERR;
	if (m_pSongVariables)
	{
		theErr = GM_LoadInstrumentFromExternalData((GM_Song *)m_pSongVariables, (XLongResourceID)instrument, 
													(InstrumentResource *)data, dataSize);
	}
	else
	{
		theErr = NOT_SETUP;
	}
	return BAE_TranslateOPErr(theErr);
}

// create a data block that is the instrument. Data block then can be passed into LoadInstrumentFromData
BAEResult BAEMidiSynth::CreateInstrumentAsData(BAE_INSTRUMENT instrument, void **pData, unsigned long *pDataSize)
{
	OPErr				theErr;
	InstrumentResource	*pInstrument;

	theErr = NO_ERR;
	if (m_pSongVariables)
	{
		pInstrument = (InstrumentResource *)XGetAndDetachResource(ID_INST, (XLongResourceID)instrument, (long *)pDataSize);		// look for an instrument
		if (pInstrument)
		{
			*pData = (void *)pInstrument;
		}
		else
		{
			theErr = BAD_INSTRUMENT;
		}
	}
	else
	{
		theErr = NOT_SETUP;
	}
	return BAE_TranslateOPErr(theErr);
}


BAEResult BAEMidiSynth::UnloadInstrument(BAE_INSTRUMENT instrument)
{
	OPErr	theErr;

	theErr = NO_ERR;
	if (m_pSongVariables)
	{
		theErr = GM_UnloadSongInstrument((GM_Song *)m_pSongVariables, (XLongResourceID)instrument);
	}
	else
	{
		theErr = NOT_SETUP;
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAEMidiSynth::RemapInstrument(BAE_INSTRUMENT from, BAE_INSTRUMENT to)
{
	OPErr	theErr;

	theErr = GM_RemapInstrument((GM_Song *)m_pSongVariables, (XLongResourceID)from, (XLongResourceID)to);
	return BAE_TranslateOPErr(theErr);
}


// get current midi tick in microseconds
unsigned long BAEMidiSynth::GetTick(void)
{
	return GM_GetSyncTimeStamp();
}

// set queue control of midi commmands. Use TRUE to queue commands, FALSE to
// send directly to engine. Default is TRUE
void BAEMidiSynth::SetQueue(BAE_BOOL useQueue)
{
	mQueueMidi = useQueue;
}

void BAEMidiSynth::LockQueue(void)
{
	if (mQueueMidi)
	{
		QGM_LockExternalMidiQueue();
	}
}

void BAEMidiSynth::UnlockQueue(void)
{
	if (mQueueMidi)
	{
		QGM_UnlockExternalMidiQueue();
	}
}

// given a midi stream, parse it out to the various midi functions
// for example:
// 0x92			0x50		0x7F		0x00
// comandByte	data1Byte	data2Byte	data3Byte
// Note 80 on with a velocity of 127 on channel 2
void BAEMidiSynth::ParseMidiData(unsigned char commandByte, unsigned char data1Byte, 
									unsigned char data2Byte, unsigned char data3Byte,
									unsigned long time)
{
	unsigned char	channel;

	channel = commandByte & 0x0F;
	data3Byte = data3Byte;
	switch(commandByte & 0xF0)
	{
		case 0x80:	// Note off
			NoteOff(channel, data1Byte, data2Byte, time);
			break;
		case 0x90:	// Note on
			NoteOn(channel, data1Byte, data2Byte, time);
			break;
		case 0xA0:	// key pressure (aftertouch)
			KeyPressure(channel, data1Byte, data2Byte, time);
			break;
		case 0xB0:	// controllers
			ControlChange(channel, data1Byte, data2Byte, time);
			break;
		case 0xC0:	// Program change
			ProgramChange(channel, data1Byte, time);
			break;
		case 0xD0:	// channel pressure (aftertouch)
			ChannelPressure(channel, data1Byte, time);
			break;
		case 0xE0:	// SetPitchBend
			PitchBend(channel, data1Byte, data2Byte, time);
			break;
	}

}

// normal note on
void BAEMidiSynth::NoteOn(unsigned char channel, 
					   unsigned char note, 
					   unsigned char velocity,
					   unsigned long time)
{
	if (time == 0)
	{
		time = GM_GetSyncTimeStamp();
	}

	if (mQueueMidi)
	{
		QGM_NoteOn((GM_Song *)m_pSongVariables, time, channel, note, velocity);
	}
	else
	{
		GM_NoteOn((GM_Song *)m_pSongVariables, channel, note, velocity);
	}
}

// normal note on with load
void BAEMidiSynth::NoteOnWithLoad(unsigned char channel, 
					   unsigned char note, 
					   unsigned char velocity,
					   unsigned long time)
{
	BAE_INSTRUMENT	inst;
	unsigned char	program, bank;
	BAEOutputMixer	*pMixer;

	pMixer = BAENoise::GetMixer();
	// wait around for at least one slice to let events catch up
	XWaitMicroseocnds(pMixer->GetAudioLatency() / 1000);
	XWaitMicroseocnds(pMixer->GetAudioLatency() / 1000);

	// pull the current program, bank from the current state. Should be valid by this time.
	GetProgramBank(channel, &program, &bank);
	inst = TranslateBankProgramToInstrument(bank, program, channel, note);
	if (IsInstrumentLoaded(inst) == FALSE)
	{
		LoadInstrument(inst);
	}

	if (mQueueMidi)
	{
		if (time == 0)
		{
			time = GM_GetSyncTimeStamp();
		}

		QGM_NoteOn((GM_Song *)m_pSongVariables, time, channel, note, velocity);
	}
	else
	{
		GM_NoteOn((GM_Song *)m_pSongVariables, channel, note, velocity);
	}
}

void BAEMidiSynth::NoteOff(unsigned char channel, 
						unsigned char note, 
						unsigned char velocity,
						unsigned long time)
{
	if (time == 0)
	{
		time = GM_GetSyncTimeStamp();
	}

	if (mQueueMidi)
	{
		QGM_NoteOff((GM_Song *)m_pSongVariables, time, channel, note, velocity);
	}
	else
	{
		GM_NoteOff((GM_Song *)m_pSongVariables, channel, note, velocity);
	}
}


void BAEMidiSynth::ControlChange(unsigned char channel, 
							  unsigned char controlNumber,
							  unsigned char controlValue, 
							  unsigned long time)
{
	if (time == 0)
	{
		time = GM_GetSyncTimeStamp();
	}

	if (mQueueMidi)
	{
		QGM_Controller((GM_Song *)m_pSongVariables, time, channel, controlNumber, controlValue);
	}
	else
	{
		GM_Controller((GM_Song *)m_pSongVariables, channel, controlNumber, controlValue);
	}
}


/*
	void			InstrumentChange(unsigned char channel,
								  BAE_INSTRUMENT instrument,
								  unsigned long time = 0);

void BAEMidiSynth::InstrumentChange(unsigned char channel,
								  BAE_INSTRUMENT instrument,
								  unsigned long time)
{
	if (time == 0)
	{
		time = GM_GetSyncTimeStamp();
	}

	if (mQueueMidi)
	{
		QGM_Controller((GM_Song *)m_pSongVariables, time, channel, 0, bankNumber);
		QGM_ProgramChange((GM_Song *)m_pSongVariables, time, channel, programNumber);
	}
	else
	{
		GM_Controller((GM_Song *)m_pSongVariables, channel, 0, bankNumber);
		GM_ProgramChange((GM_Song *)m_pSongVariables, channel, programNumber);
	}
}
*/

void BAEMidiSynth::ProgramBankChange(unsigned char channel, 
							  unsigned char programNumber,
							  unsigned char bankNumber,
							  unsigned long time)
{
	if (time == 0)
	{
		time = GM_GetSyncTimeStamp();
	}

	if (mQueueMidi)
	{
		QGM_Controller((GM_Song *)m_pSongVariables, time, channel, 0, bankNumber);
		QGM_ProgramChange((GM_Song *)m_pSongVariables, time, channel, programNumber);
	}
	else
	{
		GM_Controller((GM_Song *)m_pSongVariables, channel, 0, bankNumber);
		GM_ProgramChange((GM_Song *)m_pSongVariables, channel, programNumber);
	}
}

void BAEMidiSynth::ProgramChange(unsigned char channel, 
							  unsigned char programNumber,
							  unsigned long time)
{
	if (time == 0)
	{
		time = GM_GetSyncTimeStamp();
	}

	if (mQueueMidi)
	{
		QGM_ProgramChange((GM_Song *)m_pSongVariables, time, channel, programNumber);
	}
	else
	{
		GM_ProgramChange((GM_Song *)m_pSongVariables, channel, programNumber);
	}
}

void BAEMidiSynth::ChannelPressure(unsigned char channel, unsigned char pressure, unsigned long time)
{
	time = time;
	channel = channel;
	pressure = pressure;
}

void BAEMidiSynth::KeyPressure(unsigned char channel, unsigned char note, unsigned char pressure, unsigned long time)
{
	time = time;
	pressure = pressure;
	channel = channel;
	note = note;
}

void BAEMidiSynth::GetPitchBend(unsigned channel, unsigned char *pLSB, unsigned char *pMSB)
{
	if (m_pSongVariables && pLSB && pMSB)
	{
		GM_GetPitchBend((GM_Song *)m_pSongVariables, channel, pLSB, pMSB);
	}
}

void BAEMidiSynth::PitchBend(unsigned char channel, 
						  unsigned char lsb, 
						  unsigned char msb,
						  unsigned long time)
{
	if (time == 0)
	{
		time = GM_GetSyncTimeStamp();
	}

	if (mQueueMidi)
	{
		QGM_PitchBend((GM_Song *)m_pSongVariables, time, channel, msb, lsb);
	}
	else
	{
		GM_PitchBend((GM_Song *)m_pSongVariables, channel, msb, lsb);
	}
}


void BAEMidiSynth::AllNotesOff(unsigned long time)
{
	if (time == 0)
	{
		time = GM_GetSyncTimeStamp();
	}

	if (mQueueMidi)
	{
		QGM_AllNotesOff((GM_Song *)m_pSongVariables, time);
	}
	else
	{
		GM_AllNotesOff((GM_Song *)m_pSongVariables);
	}
}


#if 0
	#pragma mark ### BAESoundStream class ###
#endif

#if USE_STREAM_API

BAESoundStream::BAESoundStream(BAEOutputMixer *pBAEAudio,
								 char const *cName, void * userReference) :
									BAENoise(pBAEAudio, cName, BAENoise::SOUND_STREAM_NOISE)
{
	mSoundStreamVoiceReference = 0;
	mUserReference = userReference;
	mPauseVariable = 0;
	mCallbackProc = NULL;
	mReverbState = FALSE;		// off
	mReverbAmount = 0;
	mLowPassAmount = 0;
	mResonanceAmount = 0;
	mFrequencyAmount = 0;
	mVolumeState = BAE_FIXED_1;	// 1.0
	mPanState = BAE_CENTER_PAN;	// center;
	mPrerolled = FALSE;
}


BAESoundStream::~BAESoundStream()
{
	Stop();
}

// streaming file callback. Used to decode typed files.
static OPErr PV_CustomOutputStreamCallback(void *context, GM_StreamMessage message, GM_StreamData *pAS)
{
	BAESoundStream			*pBAEStream;
	BAEStreamObjectProc		callback;
	BAEStreamData			iData;
	OPErr					theErr;
	BAEResult					igorErr;
	BAEStreamMessage		igorMessage;

	context = context;
	theErr = NO_ERR;
	igorErr = BAE_NO_ERROR;
	pBAEStream = (BAESoundStream *)pAS->userReference;
	if (pBAEStream)
	{
		iData.userReference = (long)pBAEStream->GetReference();
		iData.pData = pAS->pData;
		iData.dataLength = pAS->dataLength;
		iData.sampleRate = pAS->sampleRate;
		iData.dataBitSize = pAS->dataBitSize;
		iData.channelSize = pAS->channelSize;
		iData.startSample = pAS->startSample;
		iData.endSample = pAS->endSample;
		callback = (BAEStreamObjectProc)pBAEStream->GetCallbackProc();
		if (callback)
		{
			switch (message)
			{
				default:
					igorMessage = BAE_STREAM_NULL;
					break;
				case STREAM_CREATE:
					igorMessage = BAE_STREAM_CREATE;
					break;
				case STREAM_DESTROY:
					igorMessage = BAE_STREAM_DESTROY;
					break;
				case STREAM_GET_DATA:
					igorMessage = BAE_STREAM_GET_DATA;
					break;
				case STREAM_GET_SPECIFIC_DATA:
					igorMessage = BAE_STREAM_GET_SPECIFIC_DATA;
					break;
			}
			if (igorMessage != BAE_STREAM_NULL)
			{
				igorErr = (*callback)(igorMessage, &iData);
			}
			else
			{
				igorErr = BAE_NOT_SETUP;
			}
			pAS->pData = iData.pData;
			pAS->dataLength = iData.dataLength;
			pAS->sampleRate = iData.sampleRate;
			pAS->dataBitSize = iData.dataBitSize;
			pAS->channelSize = iData.channelSize;
			pAS->startSample = iData.startSample;
			pAS->endSample = iData.endSample;
		}
	}
	switch (igorErr)
	{
		default:
			theErr = GENERAL_BAD;
			break;
		case BAE_NO_ERROR:
			theErr = NO_ERR;
			break;
		case BAE_BUFFER_TO_SMALL:
			theErr = BUFFER_TO_SMALL;
			break;
		case BAE_NOT_SETUP:
			theErr = NOT_SETUP;
			break;
		case BAE_PARAM_ERR:
			theErr = PARAM_ERR;
			break;
		case BAE_MEMORY_ERR:
			theErr = MEMORY_ERR;
			break;
		case BAE_STREAM_STOP_PLAY:
			theErr = STREAM_STOP_PLAY;
			break;
	}
	return theErr;
}

// start streaming a file
BAEResult BAESoundStream::SetupFileStream(BAEPathName pWaveFilePath, 
								BAEFileType fileType,
								unsigned long bufferSize,			// temp buffer to read file
								BAE_BOOL loopFile)		// TRUE will loop file
{
#if USE_HIGHLEVEL_FILE_API
	XFILENAME		theFile;
	GM_Waveform		fileInfo;
	AudioFileType	type;
	BAEResult			theErr;

	theErr = BAE_NO_ERROR;
	XConvertNativeFileToXFILENAME(pWaveFilePath, &theFile);

	type = BAE_TranslateBAEFileType(fileType);
	if (type != FILE_INVALID_TYPE)
	{
		if (bufferSize >= BAE_MIN_STREAM_BUFFER_SIZE)
		{
			mSoundStreamVoiceReference = GM_AudioStreamFileSetup(NULL, &theFile, type, bufferSize, &fileInfo, loopFile);
			mStreamSampleInfo.bitSize = fileInfo.bitSize;
			mStreamSampleInfo.channels = fileInfo.channels;
			mStreamSampleInfo.sampledRate = fileInfo.sampledRate;
			mStreamSampleInfo.baseMidiPitch = fileInfo.baseMidiPitch;
			mStreamSampleInfo.waveSize = fileInfo.waveSize;
			mStreamSampleInfo.waveFrames = fileInfo.waveFrames;
			mStreamSampleInfo.startLoop = 0;
			mStreamSampleInfo.endLoop = 0;
			theErr = BAE_TranslateOPErr(GM_AudioStreamError(mSoundStreamVoiceReference));
		}
		else
		{
			theErr = BAE_BUFFER_TO_SMALL;
		}
	}
	else
	{
		theErr = BAE_BAD_FILE_TYPE;
	}
	return theErr;
#else
	fileType;
	pWaveFilePath;
	bufferSize;
	loopFile;
	return BAE_NOT_SETUP;
#endif
}

BAEResult BAESoundStream::SetupCustomStream(BAEStreamObjectProc pProc, 	// control callback
									unsigned long bufferSize, 			// buffer size 
									BAE_UNSIGNED_FIXED sampleRate,				// Fixed 16.16
									char dataBitSize,					// 8 or 16 bit data
									char channelSize)					// 1 or 2 channels of date
{
	mCallbackProc = pProc;
	if (bufferSize >= BAE_MIN_STREAM_BUFFER_SIZE)
	{
		mSoundStreamVoiceReference = GM_AudioStreamSetup(NULL, (long)this, PV_CustomOutputStreamCallback, bufferSize,
										sampleRate, dataBitSize, channelSize);					

		mStreamSampleInfo.bitSize = dataBitSize;
		mStreamSampleInfo.channels = channelSize;
		mStreamSampleInfo.baseMidiPitch = 60;
		mStreamSampleInfo.waveSize = 0;
		mStreamSampleInfo.waveFrames = 0;
		mStreamSampleInfo.startLoop = 0;
		mStreamSampleInfo.endLoop = 0;
		mStreamSampleInfo.sampledRate = sampleRate;
		return BAE_TranslateOPErr(GM_AudioStreamError(mSoundStreamVoiceReference));
	}
	return BAE_BUFFER_TO_SMALL;
}

// Get the position of a audio stream in samples
unsigned long BAESoundStream::GetPlaybackPosition(void)
{
	return GM_AudioStreamGetFileSamplePosition(mSoundStreamVoiceReference);
}

BAEResult BAESoundStream::GetInfo(BAESampleInfo *pInfo)
{
	if (pInfo)
	{
		*pInfo = mStreamSampleInfo;
	}
	return BAE_NO_ERROR;
}

// This will return the last AudioStream Error
BAEResult BAESoundStream::LastError(void)
{
	return BAE_TranslateOPErr(GM_AudioStreamError(mSoundStreamVoiceReference));
}

BAEResult BAESoundStream::Preroll(void)
{
	OPErr	theErr;

	theErr = NO_ERR;
	if (mPrerolled == FALSE)
	{
		if (mSoundStreamVoiceReference)
		{
			GM_AudioStreamSetVolume(mSoundStreamVoiceReference,
									FIXED_TO_SHORT_ROUNDED(mVolumeState * MAX_NOTE_VOLUME), TRUE);
			GM_AudioStreamSetStereoPosition(mSoundStreamVoiceReference, mPanState);
			theErr = GM_AudioStreamPreroll(mSoundStreamVoiceReference);
			if (theErr == NO_ERR)
			{
				mPrerolled = TRUE;
			}
		}
	}
	else
	{
		theErr = NOT_SETUP;
	}
	return BAE_TranslateOPErr(theErr);
}

// This will start a stream once data has been loaded
BAEResult BAESoundStream::Start(void)
{
	OPErr	theErr;

	theErr = NO_ERR;
	if (mSoundStreamVoiceReference)
	{
		Preroll();
		theErr = GM_AudioStreamStart(mSoundStreamVoiceReference);
		SetReverb(mReverbState);	// set current reverb state
	}
	else
	{
		theErr = NOT_SETUP;
	}
	return BAE_TranslateOPErr(theErr);
}

// This will stop a streaming audio object and free any memory.
void BAESoundStream::Stop(BAE_BOOL startFade)
{
	short int	streamVolume;

	mPrerolled = FALSE;
	if (mSoundStreamVoiceReference)
	{
		mPrerolled = FALSE;
		if (IsPaused())
		{
			Resume();
		}
		if (startFade)
		{
			streamVolume = GM_AudioStreamGetVolume(mSoundStreamVoiceReference);
			GM_SetAudioStreamFadeRate(mSoundStreamVoiceReference, FLOAT_TO_FIXED(2.2), 0, streamVolume, TRUE);
		}
		else
		{
			GM_AudioStreamStop(NULL, mSoundStreamVoiceReference);
//			GM_AudioStreamDrain(NULL, mSoundStreamVoiceReference);	// wait for it to be finished
		}
		mSoundStreamVoiceReference = DEAD_STREAM;
	}
}

// This will stop and flush the current stream and force a read of data. This
// will cause gaps in the audio.
void BAESoundStream::Flush(void)
{
	if (IsPaused())
	{
		Resume();
	}
	GM_AudioStreamFlush(mSoundStreamVoiceReference);
}

// Returns TRUE or FALSE if a given AudioStream is still active
BAE_BOOL BAESoundStream::IsPlaying(void)
{
	if (mSoundStreamVoiceReference)
	{
		return (BAE_BOOL)GM_IsAudioStreamPlaying(mSoundStreamVoiceReference);
	}
	return FALSE;
}

BAE_BOOL BAESoundStream::IsDone(void)
{
	if (IsPlaying() == FALSE)
	{
		return TRUE;
	}
	return FALSE;
}

// Returns TRUE if a given AudioStream is valid
BAE_BOOL BAESoundStream::IsValid(void)
{
	return (BAE_BOOL)GM_IsAudioStreamValid(mSoundStreamVoiceReference);
}

// Set the volume level of a audio stream
void BAESoundStream::SetVolume(BAE_UNSIGNED_FIXED newVolume)
{
	mVolumeState = newVolume;
	GM_AudioStreamSetVolume(mSoundStreamVoiceReference,
							FIXED_TO_SHORT_ROUNDED(newVolume * MAX_NOTE_VOLUME), FALSE);
}

// Get the volume level of a audio stream
BAE_UNSIGNED_FIXED BAESoundStream::GetVolume(void)
{
	if (mSoundStreamVoiceReference)
	{
		return UNSIGNED_RATIO_TO_FIXED(GM_AudioStreamGetVolume(mSoundStreamVoiceReference), MAX_NOTE_VOLUME);
	}
	return mVolumeState;
}

// fade from source volume, to dest volume in time miliseconds. Always async
void BAESoundStream::FadeFromToInTime(BAE_FIXED sourceVolume, BAE_FIXED destVolume, BAE_FIXED timeInMiliseconds)
{
	short int	source, dest;
	double		delta;
	INT16		minVolume;
	INT16		maxVolume;

	if (mSoundStreamVoiceReference)
	{
		delta = PV_CalculateTimeDeltaForFade(sourceVolume, destVolume, timeInMiliseconds);
		delta = delta * -MAX_SONG_VOLUME;

		source = FIXED_TO_SHORT_ROUNDED(sourceVolume * MAX_SONG_VOLUME);
		dest = FIXED_TO_SHORT_ROUNDED(destVolume * MAX_SONG_VOLUME);
		minVolume = XMIN(source, dest);
		maxVolume = XMAX(source, dest);
		GM_SetAudioStreamFadeRate(mSoundStreamVoiceReference, FLOAT_TO_FIXED(delta), minVolume, maxVolume, FALSE);
	}
}

void BAESoundStream::Fade(BAE_BOOL doAsync)
{
	short int	sampleVolume;

	sampleVolume = GM_AudioStreamGetVolume(mSoundStreamVoiceReference);
	if (doAsync == FALSE)
	{
		// We're going to fade the stream, but don't stop

		GM_SetAudioStreamFadeRate(mSoundStreamVoiceReference, FLOAT_TO_FIXED(2.2),
												0, sampleVolume, FALSE);
		while (GM_AudioStreamGetVolume(mSoundStreamVoiceReference) && GM_IsAudioStreamPlaying(mSoundStreamVoiceReference)) 
		{
			GetMixer()->ServiceAudioOutputToFile();
			GetMixer()->ServiceIdle();
			XWaitMicroseocnds(1000);
		}
	}
	else
	{
		GM_SetAudioStreamFadeRate(mSoundStreamVoiceReference, FLOAT_TO_FIXED(2.2), 0, sampleVolume, FALSE);
	}
}


void BAESoundStream::FadeTo(BAE_FIXED destVolume, BAE_BOOL doAsync)
{
	short int	songVolume, saveVolume, newSongVolume, saveNewSongVolume;
	XFIXED		delta;

	if (mSoundStreamVoiceReference)
	{
		newSongVolume = FIXED_TO_SHORT_ROUNDED(destVolume * MAX_SONG_VOLUME);
		saveNewSongVolume = newSongVolume;
		// We're going to fade the song out before we stop it.		
		songVolume = GM_AudioStreamGetVolume(mSoundStreamVoiceReference);
		saveVolume = songVolume;

		if (newSongVolume < songVolume)
		{	// fade out
			songVolume = newSongVolume;
			newSongVolume = saveVolume;
			delta = FLOAT_TO_UNSIGNED_FIXED(2.2);
		}
		else
		{	// fade in
			delta = (XFIXED)FLOAT_TO_FIXED(-2.2);
		}
		if (doAsync == FALSE)
		{
			GM_SetAudioStreamFadeRate(mSoundStreamVoiceReference, delta, songVolume, newSongVolume, FALSE);
			while (GM_AudioStreamGetVolume(mSoundStreamVoiceReference) != saveNewSongVolume)
			{
				GetMixer()->ServiceAudioOutputToFile();
				GetMixer()->ServiceIdle();
				XWaitMicroseocnds(1000);
			}
		}
		else
		{
			GM_SetAudioStreamFadeRate(mSoundStreamVoiceReference, delta, songVolume, newSongVolume, FALSE);
		}
	}
}

// Set the sample rate of a audio stream
void BAESoundStream::SetRate(BAE_UNSIGNED_FIXED newRate)
{
	GM_AudioStreamSetRate(mSoundStreamVoiceReference, newRate);
}

// Get the sample rate of a audio stream
BAE_UNSIGNED_FIXED BAESoundStream::GetRate(void)
{
	return GM_AudioStreamGetRate(mSoundStreamVoiceReference);
}

// Set the stereo position of a audio stream
void BAESoundStream::SetStereoPosition(short int stereoPosition)
{
	mPanState = stereoPosition;
	GM_AudioStreamSetStereoPosition(mSoundStreamVoiceReference, stereoPosition);
}

// Get the stereo position of a audio stream
short int BAESoundStream::GetStereoPosition(void)
{
	if (mSoundStreamVoiceReference)
	{
		mPanState = GM_AudioStreamGetStereoPosition(mSoundStreamVoiceReference);
	}
	return mPanState;
}

// Enable/Disable reverb on this particular audio stream
void BAESoundStream::SetReverb(BAE_BOOL useReverb)
{
	mReverbState = useReverb;
	if (mSoundStreamVoiceReference)
	{
		GM_AudioStreamReverb(mSoundStreamVoiceReference, useReverb);
	}
}

BAE_BOOL BAESoundStream::GetReverb(void)
{
	if (mSoundStreamVoiceReference)
	{
		mReverbState = GM_AudioStreamGetReverb(mSoundStreamVoiceReference);
	}
	return mReverbState;
}

void BAESoundStream::SetReverbAmount(short int reverbAmount)
{
	mReverbAmount = reverbAmount;
	SetReverb((reverbAmount) ? TRUE : FALSE);
	if (mSoundStreamVoiceReference)
	{
		GM_SetStreamReverbAmount(mSoundStreamVoiceReference, reverbAmount);
	}
}

short int BAESoundStream::GetReverbAmount(void)
{
	if (mSoundStreamVoiceReference)
	{
		mReverbAmount = GM_GetStreamReverbAmount(mSoundStreamVoiceReference);
	}
	return mReverbAmount;
}

void BAESoundStream::SetLowPassAmountFilter(short int lowpassamount)
{
	mLowPassAmount = lowpassamount;
	if (mSoundStreamVoiceReference)
	{
		GM_AudioStreamSetLowPassAmountFilter(mSoundStreamVoiceReference, lowpassamount);
	}
}

short int BAESoundStream::GetLowPassAmountFilter(void)
{
	if (mSoundStreamVoiceReference)
	{
		mLowPassAmount = GM_AudioStreamGetLowPassAmountFilter(mSoundStreamVoiceReference);
	}
	return mLowPassAmount;
}

void BAESoundStream::SetResonanceAmountFilter(short int resonance)
{
	mResonanceAmount = resonance;
	if (mSoundStreamVoiceReference)
	{
		GM_AudioStreamSetResonanceFilter(mSoundStreamVoiceReference, resonance);
	}
}

short int BAESoundStream::GetResonanceAmountFilter(void)
{
	if (mSoundStreamVoiceReference)
	{
		mResonanceAmount = GM_AudioStreamGetResonanceFilter(mSoundStreamVoiceReference);
	}
	return mResonanceAmount;
}

void BAESoundStream::SetFrequencyAmountFilter(short int frequency)
{
	mFrequencyAmount = frequency;
	if (mSoundStreamVoiceReference)
	{
		GM_AudioStreamSetFrequencyFilter(mSoundStreamVoiceReference, frequency);
	}
}

short int BAESoundStream::GetFrequencyAmountFilter(void)
{
	if (mSoundStreamVoiceReference)
	{
		mFrequencyAmount = GM_AudioStreamGetFrequencyFilter(mSoundStreamVoiceReference);
	}
	return mFrequencyAmount;
}

// currently paused
BAE_BOOL BAESoundStream::IsPaused(void)
{
	return (mPauseVariable) ? (BAE_BOOL)TRUE : (BAE_BOOL)FALSE;
}

// Pause the stream
void BAESoundStream::Pause(void)
{
	if (mPauseVariable == 0)
	{
		mPauseVariable = (unsigned long)GetRate();
		SetRate(0L);	// pause samples in their tracks
	}
}

// Resume the stream
void BAESoundStream::Resume(void)
{
	if (mPauseVariable)
	{
		SetRate((BAE_UNSIGNED_FIXED)mPauseVariable);
		mPauseVariable = 0;
	}
}
#endif	// USE_STREAM_API

#if 0
	#pragma mark ### BAESound class ###
#endif


BAESound::BAESound(BAEOutputMixer *pBAEAudio, 
					 char const *cName, void * userReference) :
									BAENoise(pBAEAudio, cName, BAENoise::SOUND_NOISE)
{
	mUserReference = userReference;
	mSoundVoiceReference = DEAD_VOICE;
	pauseVariable = 0;
	pFileVariables = NULL;
	pSoundVariables = NULL;
	pSampleFrameVariable = NULL;
	mReverbState = FALSE;	// off
	mReverbAmount = 0;
	mLowPassAmount = 0;
	mResonanceAmount = 0;
	mFrequencyAmount = 0;
	cName = cName;
	mDoneCallback = NULL;
	mLoopDoneCallback = NULL;
	mCallbackReference = NULL;
	mSoundVolume = 0;
	mStereoPosition = BAE_CENTER_PAN;
}


BAESound::~BAESound()
{
	GM_SampleCallbackEntry	*pNext, *pLast;

	Stop();

	// remove callback links
	pNext = (GM_SampleCallbackEntry *)pSampleFrameVariable;
	while (pNext)
	{
		pLast = pNext;
		pNext = pNext->pNext;
		XDisposePtr((XPTR)pLast);
	}

	if (pFileVariables)
	{
		GM_FreeWaveform((GM_Waveform *)pFileVariables);
		pFileVariables = NULL;
	}
	if (pSoundVariables)
	{
		XDisposePtr(pSoundVariables);
		pSoundVariables = NULL;
	}
}

// currently paused
BAE_BOOL BAESound::IsPaused(void)
{
	return (pauseVariable) ? (BAE_BOOL)TRUE : (BAE_BOOL)FALSE;
}

void BAESound::Resume(void)
{
	if (pauseVariable)
	{
		SetRate((BAE_UNSIGNED_FIXED)pauseVariable);
		pauseVariable = 0;
	}
}

void BAESound::Pause(void)
{
	if (pauseVariable == 0)
	{
		pauseVariable = (unsigned long)GetRate();
		SetRate(0L);	// pause samples in their tracks
	}
}

// load a sample playing from a formatted block of memory. The memory will be deallocated 
// when this object is destroyed. Call start once loaded to start the playback.
BAEResult BAESound::LoadMemorySample(void *pMemoryFile, unsigned long memoryFileSize, BAEFileType fileType)
{
#if USE_HIGHLEVEL_FILE_API
	OPErr			theErr;
	AudioFileType	type;

	theErr = NO_ERR;
	pFileVariables = NULL;
	type = BAE_TranslateBAEFileType(fileType);
	if (type != FILE_INVALID_TYPE)
	{
		pFileVariables = GM_ReadFileIntoMemoryFromMemory(pMemoryFile, memoryFileSize, type, &theErr);
		if ( (pFileVariables == NULL) && (theErr == NO_ERR) )
		{
			theErr = BAD_FILE;
		}
	}
	else
	{
		theErr = BAD_FILE_TYPE;
	}
	return BAE_TranslateOPErr(theErr);
#else
	pMemoryFile = pMemoryFile;
	memoryFileSize = memoryFileSize;
	fileType = fileType;
	return BAE_NOT_SETUP;
#endif
}

// Load file into sound object. This will copy the file directly into memory. It
// will get disposed once you destroy this object.
BAEResult BAESound::LoadFileSample(BAEPathName pWaveFilePath, BAEFileType fileType)
{
#if USE_HIGHLEVEL_FILE_API
	XFILENAME		theFile;
	OPErr			theErr;
	AudioFileType	type;

	theErr = NO_ERR;
	XConvertNativeFileToXFILENAME(pWaveFilePath, &theFile);
	pFileVariables = NULL;
	type = BAE_TranslateBAEFileType(fileType);
	if (type != FILE_INVALID_TYPE)
	{
		pFileVariables = GM_ReadFileIntoMemory(&theFile, type, &theErr);
	}
	else
	{
		theErr = BAD_FILE_TYPE;
	}
	if ((pFileVariables == NULL) && (theErr == NO_ERR))
	{
		theErr = BAD_FILE;
	}
	return BAE_TranslateOPErr(theErr);
#else
	fileType;
	pWaveFilePath;
	return BAE_NOT_SETUP;
#endif
}

// save a loaded file
BAEResult BAESound::SaveFile(BAEPathName pFile, BAEFileType fileType)
{
#if USE_HIGHLEVEL_FILE_API
	XFILENAME		theFile;
	OPErr			theErr;
	AudioFileType	type;

	theErr = NO_ERR;
	if (pFileVariables)
	{
		XConvertNativeFileToXFILENAME(pFile, &theFile);
		type = BAE_TranslateBAEFileType(fileType);
		if (type != FILE_INVALID_TYPE)
		{
			theErr = GM_WriteFileFromMemory(&theFile, (GM_Waveform *)pFileVariables, type);
		}
		else
		{
			theErr = BAD_FILE_TYPE;
		}
	}
	else
	{
		theErr = NOT_SETUP;
	}
	return BAE_TranslateOPErr(theErr);
#else
	fileType;
	pFile;
	return BAE_NOT_SETUP;
#endif
}



BAEResult BAESound::AddSampleFrameCallback(unsigned long frame, BAESampleFrameCallbackPtr pCallback, void * pReference)
{
	GM_SampleCallbackEntry	*pEntry;
	GM_SampleCallbackEntry	*pNext;
	OPErr					theErr;

	theErr = MEMORY_ERR;
	pEntry = (GM_SampleCallbackEntry *)XNewPtr((long)sizeof(GM_SampleCallbackEntry));
	if (pEntry)
	{
		pEntry->frameOffset = frame;
		pEntry->pCallback = (GM_SampleFrameCallbackPtr)pCallback;
		pEntry->reference = (long)pReference;

		// add to linked list
		pNext = (GM_SampleCallbackEntry *)pSampleFrameVariable;
		while (pNext)
		{
			if (pNext->pNext == NULL)
			{
				break;
			}
			else
			{
				pNext = pNext->pNext;
			}
		}
		if (pNext == NULL)
		{
			pSampleFrameVariable = (void *)pEntry;
		}
		else
		{
			pNext->pNext = pEntry;
		}
		theErr = NO_ERR;
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAESound::RemoveSampleFrameCallback(unsigned long frame, BAESampleFrameCallbackPtr pCallback)
{
	GM_SampleCallbackEntry	*pEntry;
	GM_SampleCallbackEntry	*pNext, *pLast;
	OPErr					theErr;

	// find link
	pNext = (GM_SampleCallbackEntry *)pSampleFrameVariable;
	pEntry = NULL;
	while (pNext)
	{
		if (pNext->frameOffset == frame)
		{
			if (pNext->pCallback == (GM_SampleFrameCallbackPtr)pCallback)
			{
				pEntry = pNext;	// found
				break;
			}
		}
		pNext = pNext->pNext;
	}
	theErr = PARAM_ERR;
	if (pEntry)
	{
		// remove from linked list
		pLast = pNext = (GM_SampleCallbackEntry *)pSampleFrameVariable;
		while (pNext)
		{
			if (pNext == pEntry)								// found object in list?
			{
				if (pNext == (GM_SampleCallbackEntry *)pSampleFrameVariable)			// is object the top object
				{
					pSampleFrameVariable = (void *)pNext->pNext;		// yes, change to next object
				}
				else
				{
					if (pLast)									// no, change last to point beyond next
					{
						pLast->pNext = pNext->pNext;
					}
				}
				break;
			}
			pLast = pNext;
			pNext = pNext->pNext;
		}
		theErr = NO_ERR;
	}
	return BAE_TranslateOPErr(theErr);
}



// load a custom sample. This will copy sample data into memory.
BAEResult BAESound::LoadCustomSample(void * sampleData,
								unsigned long frames,
								unsigned short int bitSize,
								unsigned short int channels,
								BAE_UNSIGNED_FIXED rate,
								unsigned long loopStart,
								unsigned long loopEnd)
{
	GM_Waveform 	*pWave;
	OPErr			theErr;
	long			size;

	theErr = NO_ERR;
	size = frames * channels * (bitSize / 8);
	pSoundVariables = XNewPtr(size);
	if (pSoundVariables)
	{
		XBlockMove(sampleData, pSoundVariables, size);
		pWave = (GM_Waveform *)XNewPtr((long)sizeof(GM_Waveform));
		if (pWave)
		{
			pWave->waveSize = size;
			pWave->waveFrames = frames;
			pWave->startLoop = loopStart;
			pWave->endLoop = loopEnd;
			pWave->baseMidiPitch = 60;
			pWave->bitSize = (unsigned char)bitSize;
			pWave->channels = (unsigned char)channels;
			pWave->sampledRate = rate;
	
			pWave->theWaveform = (SBYTE *)pSoundVariables;
		}
		else
		{
			XDisposePtr(pSoundVariables);
			pSoundVariables = NULL;
		}
		pFileVariables = pWave;
	}
	else
	{
		theErr = MEMORY_ERR;
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAESound::LoadResourceSample(void *pResource, unsigned long resourceSize)
{
	SampleDataInfo	newSoundInfo;
	GM_Waveform 	*pWave;
	XPTR			theData;
	OPErr			theErr;

	theErr = NO_ERR;
	if (pResource)
	{
		theData = XNewPtr(resourceSize);
		if (theData)
		{
			XBlockMove(pResource, theData, resourceSize);
			pResource = theData;
			theData = XGetSamplePtrFromSnd(pResource, &newSoundInfo);
			if (theData)
			{
				pSoundVariables = pResource;
				pWave = (GM_Waveform *)XNewPtr((long)sizeof(GM_Waveform));
				if (pWave)
				{
					pWave->waveSize = newSoundInfo.size;
					pWave->waveFrames = newSoundInfo.frames;
					pWave->startLoop = newSoundInfo.loopStart;
					pWave->endLoop = newSoundInfo.loopEnd;
					pWave->baseMidiPitch = (unsigned char)newSoundInfo.baseKey;
					pWave->bitSize = (unsigned char)newSoundInfo.bitSize;
					pWave->channels = (unsigned char)newSoundInfo.channels;
					pWave->sampledRate = newSoundInfo.rate;
					pWave->theWaveform = (SBYTE *)theData;

					pFileVariables = pWave;
				}
			}
			else
			{
				XDisposePtr(pResource);
				theErr = MEMORY_ERR;
			}
		}
		else
		{
			theErr = MEMORY_ERR;
		}
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAESound::LoadBankSample(char *cName)
{
	OPErr			theErr;
	XPTR			thePreSound, theData;
	GM_Waveform 	*pWave;
	SampleDataInfo	newSoundInfo;
	long			size;

	theErr = BAD_SAMPLE;

#if X_PLATFORM != X_MACINTOSH
	// on all platforms except MacOS we need a valid open resource file. BAE's resource manager is designed
	// to fall back into the MacOS resource manager if no valid BAE file is open. So this test is removed
	// MacOS.
	if (thePatchFile)
#endif
	{
		theData = XGetSoundResourceByName(cName, &size);
		pSoundVariables = theData;
		if (theData)
		{
			// convert snd resource into a simple pointer of data with information

			thePreSound = XGetSamplePtrFromSnd(theData, &newSoundInfo);

			if (newSoundInfo.pMasterPtr != theData)
			{	// this means that XGetSamplePtrFromSnd created a new sample
				XDisposePtr(theData);
			}
			if (thePreSound)
			{
				pSoundVariables = newSoundInfo.pMasterPtr;
				pWave = (GM_Waveform *)XNewPtr((long)sizeof(GM_Waveform));
				if (pWave)
				{
					pWave->waveSize = newSoundInfo.size;
					pWave->waveFrames = newSoundInfo.frames;
					pWave->startLoop = newSoundInfo.loopStart;
					pWave->endLoop = newSoundInfo.loopEnd;
					pWave->baseMidiPitch = (unsigned char)newSoundInfo.baseKey;
					pWave->bitSize = (unsigned char)newSoundInfo.bitSize;
					pWave->channels = (unsigned char)newSoundInfo.channels;
					pWave->sampledRate = newSoundInfo.rate;
					pWave->theWaveform = (SBYTE *)thePreSound;

					pFileVariables = pWave;
					theErr = NO_ERR;
				}
			}
			else
			{
				theErr = MEMORY_ERR;
			}
		}
		else
		{
			theErr = BAD_SAMPLE;
		}
	}
	return BAE_TranslateOPErr(theErr);
}

// set the loop points in sample frames. Pass in start==end to remove the loop point.
BAEResult BAESound::SetSampleLoopPoints(unsigned long start, unsigned long end)
{
	GM_Waveform 	*pWave;
	BAEResult			theErr;

	theErr = BAE_NO_ERROR;
	if (pFileVariables)
	{
		if (start > end)
		{
			theErr = BAE_PARAM_ERR;
		}
		else
		{
			if (start == end)
			{
				// then we're going to shut the loop point off
				start = 0;
				end = 0;
			}
			else
			{
				if ( (end - start) < MIN_LOOP_SIZE)
				{
					theErr = BAE_BUFFER_TO_SMALL;
				}
			}
		}
		if (theErr == BAE_NO_ERROR)
		{
			pWave = (GM_Waveform *)pFileVariables;
			pWave->startLoop = start;
			pWave->endLoop = end;
			if (mSoundVoiceReference != DEAD_VOICE)
			{
				GM_SetSampleLoopPoints(mSoundVoiceReference, start, end);
			}
		}
	}
	else
	{
		theErr = BAE_NOT_SETUP;
	}
	return theErr;
}

// Get the current loop points in sample frames
BAEResult BAESound::GetSampleLoopPoints(unsigned long *pStart, unsigned long *pEnd)
{
	GM_Waveform 	*pWave;
	BAEResult			theErr;

	theErr = BAE_NO_ERROR;
	if (pFileVariables && pStart && pEnd)
	{
		pWave = (GM_Waveform *)pFileVariables;
		*pStart = pWave->startLoop;
		*pEnd = pWave->endLoop;
	}
	else
	{
		theErr = BAE_NOT_SETUP;
	}
	return theErr;
}

// Get the sample point offset by a sample frame count
void * BAESound::GetSamplePointer(unsigned long sampleFrame)
{
	char			*pSample;
	GM_Waveform 	*pWave;

	pSample = NULL;
	if (pFileVariables)
	{
		pWave = (GM_Waveform *)pFileVariables;
		pSample = (char *)pWave->theWaveform;
		if (pSample)
		{
			pSample += sampleFrame * (pWave->bitSize / 8) * pWave->channels;
		}
	}
	return (void *)pSample;
}

// Get the mixer pointer for this sample as its being mixed. This pointer is 
// the actual pointer used by the mixer. This will change if this object is
// an double buffer playback.
void * BAESound::GetSamplePointerFromMixer(void)
{
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		return GM_GetSamplePlaybackPointer(mSoundVoiceReference);
	}
	return NULL;
}

// Get the position of a audio playback in samples
unsigned long BAESound::GetPlaybackPosition(void)
{
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		return GM_GetSamplePlaybackPosition(mSoundVoiceReference);
	}
	return 0;
}

BAEResult BAESound::GetInfo(BAESampleInfo *pInfo)
{
	GM_Waveform 	*pWave;
	BAEResult			theErr;

	theErr = BAE_NO_ERROR;
	if (pFileVariables)
	{
		pWave = (GM_Waveform *)pFileVariables;
		pInfo->waveSize = pWave->waveSize;
		pInfo->waveFrames = pWave->waveFrames;
		pInfo->startLoop = pWave->startLoop;
		pInfo->endLoop = pWave->endLoop;
		pInfo->baseMidiPitch = pWave->baseMidiPitch;
		pInfo->bitSize = pWave->bitSize;
		pInfo->channels = pWave->channels;
		pInfo->sampledRate = pWave->sampledRate;
	}
	else
	{
		theErr = BAE_NOT_SETUP;
	}
	return theErr;
}

// This is the default loop callback, which tells the mixer to always loop samples
static XBOOL PV_DefaultSampleLoopCallback(void *context)
{
	BAESound				*pSound;
	GM_LoopDoneCallbackPtr	userCallback;

	pSound = (BAESound *)context;
	if (pSound)
	{
		userCallback = (GM_LoopDoneCallbackPtr)pSound->GetLoopDoneCallback();
		if (userCallback)
		{
			return (*userCallback)(pSound->GetDoneCallbackReference());
		}
	}
	return TRUE;	// always loop
}

void BAESound::DefaultSampleDoneCallback(void)
{
	BAEDoneCallbackPtr	userCallback;

	userCallback = GetDoneCallback();
	if (userCallback)
	{
		(*userCallback)(GetDoneCallbackReference());
	}
	mSoundVoiceReference = DEAD_VOICE;	// voice dead
}

// Glue "C" code between Gen API and the BAE C++ API
static void PV_DefaultSampleDoneCallback(void *context)
{
	BAESound				*pSound;

	pSound = (BAESound *)context;
	if (pSound)
	{
		pSound->DefaultSampleDoneCallback();
	}
}

// private function that sets everything up prior to starting a sample
BAEResult BAESound::PreStart(BAE_UNSIGNED_FIXED sampleVolume, 			// sample volume
						short int stereoPosition,					// stereo placement
						void * refData, 							// callback reference
						BAELoopDoneCallbackPtr pLoopContinueProc,
						BAEDoneCallbackPtr pDoneProc,
						BAE_BOOL stopIfPlaying)
{
	OPErr	theErr;
	long	volume;

	theErr = NO_ERR;
	mDoneCallback = pDoneProc;
	mLoopDoneCallback = pLoopContinueProc;
	mCallbackReference = refData;
	mSoundVolume = sampleVolume;
	mStereoPosition = stereoPosition;
	if (pFileVariables)
	{
		if (mSoundVoiceReference != DEAD_VOICE)
		{
			if (IsPlaying())		// sample playing?
			{
				if (stopIfPlaying)	// can we stop it
				{
					Stop();
				}
				else
				{
					theErr = STILL_PLAYING;
				}
			}
		}
		if (theErr == NO_ERR)
		{
			mSoundVoiceReference = DEAD_VOICE;
			volume = UNSIGNED_FIXED_TO_LONG_ROUNDED(sampleVolume * MAX_NOTE_VOLUME);
			if (volume)
			{
#if (USE_FLOAT_LOOPS == 0) && (USE_U3232_LOOPS == 0)
				if (((GM_Waveform *)pFileVariables)->waveFrames > MAX_SAMPLE_FRAMES)
				{
					theErr = SAMPLE_TO_LARGE;
				}
#endif
			}
			else
			{
				theErr = NO_VOLUME;
			}
		}
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAESound::Start(BAE_UNSIGNED_FIXED sampleVolume, 			// sample volume
						short int stereoPosition,					// stereo placement
						void * refData, 							// callback reference
						BAELoopDoneCallbackPtr pLoopContinueProc,
						BAEDoneCallbackPtr pDoneProc,
						unsigned long startOffsetFrame,
						BAE_BOOL stopIfPlaying)
{
	OPErr	theErr;
	long	volume;

	theErr = BAE_TranslateBAErr(
									PreStart(sampleVolume, stereoPosition, refData, 
												pLoopContinueProc, pDoneProc, stopIfPlaying)
								);
	if (theErr == NO_ERR)
	{
		mSoundVoiceReference = DEAD_VOICE;
		volume = UNSIGNED_FIXED_TO_LONG_ROUNDED(sampleVolume * MAX_NOTE_VOLUME);
		if (volume)
		{
			mSoundVoiceReference = GM_SetupSampleFromInfo((GM_Waveform *)pFileVariables, (void *)this, 
												volume,
												mStereoPosition,
												(GM_LoopDoneCallbackPtr)PV_DefaultSampleLoopCallback, 
												PV_DefaultSampleDoneCallback,
												startOffsetFrame);
			if (mSoundVoiceReference == DEAD_VOICE)
			{
				theErr = NO_FREE_VOICES;
			}
			else
			{
				GM_StartSample(mSoundVoiceReference);
				GM_SetSampleOffsetCallbackLinks(mSoundVoiceReference, 
										(GM_SampleCallbackEntry *)pSampleFrameVariable);
				SetReverb(mReverbState);	// set current reverb state
			}
		}
	}
	return BAE_TranslateOPErr(theErr);
}

// This will setup a BAESound once data has been loaded. Call __Start to start playing
BAEResult BAESound::__Setup(BAE_UNSIGNED_FIXED sampleVolume, 		// sample volume	(1.0)
							short int stereoPosition,			// stereo placement -63 to 63
							void * refData, 								// callback reference
							BAELoopDoneCallbackPtr pLoopContinueProc,
							BAEDoneCallbackPtr pDoneProc,
							unsigned long startOffsetFrame,				// starting offset in frames
							BAE_BOOL stopIfPlaying)						// TRUE will restart sound otherwise return and error
{

	OPErr	theErr;
	long	volume;

	theErr = BAE_TranslateBAErr(
									PreStart(sampleVolume, stereoPosition, refData, 
												pLoopContinueProc, pDoneProc, stopIfPlaying)
								);
	if (theErr == NO_ERR)
	{
		mSoundVoiceReference = DEAD_VOICE;
		volume = UNSIGNED_FIXED_TO_LONG_ROUNDED(sampleVolume * MAX_NOTE_VOLUME);
		if (volume)
		{
			mSoundVoiceReference = GM_SetupSampleFromInfo((GM_Waveform *)pFileVariables, (void *)this, 
												volume,
												mStereoPosition,
												(GM_LoopDoneCallbackPtr)PV_DefaultSampleLoopCallback, 
												PV_DefaultSampleDoneCallback,
												startOffsetFrame);
			if (mSoundVoiceReference == DEAD_VOICE)
			{
				theErr = NO_FREE_VOICES;
			}
		}
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAESound::__Start(void)
{
	BAEResult	err;

	err = BAE_NOT_SETUP;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		GM_StartSample(mSoundVoiceReference);
		GM_SetSampleOffsetCallbackLinks(mSoundVoiceReference, 
								(GM_SampleCallbackEntry *)pSampleFrameVariable);
		SetReverb(mReverbState);	// set current reverb state
		err = BAE_NO_ERROR;
	}
	return err;
}

void BAESound::Stop(BAE_BOOL startFade)
{
	short int	sampleVolume;

	if (IsPaused())
	{
		Resume();
	}
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		if (startFade)
		{
			sampleVolume = GM_GetSampleVolume(mSoundVoiceReference);
			GM_SetSampleFadeRate(mSoundVoiceReference, FLOAT_TO_FIXED(2.2),
													0, sampleVolume, TRUE);
		}
		else
		{
			GM_EndSample(mSoundVoiceReference);
			GM_SetSampleOffsetCallbackLinks(mSoundVoiceReference, NULL);
		}
	}
	mSoundVoiceReference = DEAD_VOICE;	// done
}

	// This will, given all the information about a sample, will play sample memory without
	// copying the data. Be carefull and do not dispose of the memory associated with this sample
	// while its playing. Call __Start to start sound
BAEResult BAESound::__SetupCustom(void * sampleData,					// pointer to audio data
									unsigned long frames, 				// number of frames of audio
									unsigned short int bitSize, 		// bits per sample 8 or 16
									unsigned short int channels, 		// mono or stereo 1 or 2
									BAE_UNSIGNED_FIXED rate, 			// 16.16 fixed sample rate
									unsigned long loopStart, 			// loop start in frames
									unsigned long loopEnd,				// loop end in frames
									BAE_UNSIGNED_FIXED sampleVolume, 	// sample volume	(1.0)
									short int stereoPosition,			// stereo placement
									void *refData, 						// callback reference
									BAELoopDoneCallbackPtr pLoopContinueProc,
									BAEDoneCallbackPtr pDoneProc,
									BAE_BOOL stopIfPlaying)
{
	OPErr	theErr;
	long	volume;

	theErr = BAE_TranslateBAErr(
									PreStart(sampleVolume, stereoPosition, refData, 
												pLoopContinueProc, pDoneProc, stopIfPlaying)
								);
	if (theErr == NO_ERR)
	{
		mSoundVoiceReference = DEAD_VOICE;
		volume = UNSIGNED_FIXED_TO_LONG_ROUNDED(sampleVolume * MAX_NOTE_VOLUME);
		if (volume)
		{
			mSoundVoiceReference = GM_SetupSample((XPTR)sampleData, frames, rate, 
										loopStart, loopEnd, 0, 
										volume, 
										stereoPosition,
										(void *)this, 
										bitSize, channels, 
										(GM_LoopDoneCallbackPtr)PV_DefaultSampleLoopCallback, 
										PV_DefaultSampleDoneCallback);
			if (mSoundVoiceReference == DEAD_VOICE)
			{
				theErr = NO_FREE_VOICES;
			}
		}
	}
	return BAE_TranslateOPErr(theErr);
}

BAEResult BAESound::StartCustomSample(void * sampleData,					// pointer to audio data
									unsigned long frames, 				// number of frames of audio
									unsigned short int bitSize, 		// bits per sample 8 or 16
									unsigned short int channels, 		// mono or stereo 1 or 2
									BAE_UNSIGNED_FIXED rate, 			// 16.16 fixed sample rate
									unsigned long loopStart, 			// loop start in frames
									unsigned long loopEnd,				// loop end in frames
									BAE_UNSIGNED_FIXED sampleVolume, 	// sample volume	(1.0)
									short int stereoPosition,			// stereo placement
									void *refData, 						// callback reference
									BAELoopDoneCallbackPtr pLoopContinueProc,
									BAEDoneCallbackPtr pDoneProc,
									BAE_BOOL stopIfPlaying)
{
	OPErr	theErr;
	long	volume;

	theErr = NO_ERR;
	mDoneCallback = pDoneProc;
	mLoopDoneCallback = pLoopContinueProc;
	mCallbackReference = refData;
	mSoundVolume = sampleVolume;
	mStereoPosition = stereoPosition;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		if (IsPlaying())		// sample playing?
		{
			if (stopIfPlaying)	// can we stop it
			{
				Stop();
			}
			else
			{
				theErr = STILL_PLAYING;
			}
		}
	}
	if (theErr == NO_ERR)
	{
		mSoundVoiceReference = DEAD_VOICE;
		volume = UNSIGNED_FIXED_TO_LONG_ROUNDED(sampleVolume * MAX_NOTE_VOLUME);
		if (volume)
		{
#if (USE_FLOAT_LOOPS == 0) && (USE_U3232_LOOPS == 0)
			if (frames < MAX_SAMPLE_FRAMES)
#else
			if (1)
#endif
			{
				mSoundVoiceReference = GM_SetupSample((XPTR)sampleData, frames, rate, 
											loopStart, loopEnd, 0, 
											volume, stereoPosition,
											(void *)this, 
											bitSize, channels, 
											(GM_LoopDoneCallbackPtr)PV_DefaultSampleLoopCallback, 
											PV_DefaultSampleDoneCallback);
				if (mSoundVoiceReference == DEAD_VOICE)
				{
					theErr = NO_FREE_VOICES;
				}
				else
				{
					GM_StartSample(mSoundVoiceReference);
					GM_SetSampleOffsetCallbackLinks(mSoundVoiceReference, 
											(GM_SampleCallbackEntry *)pSampleFrameVariable);
					SetReverb(mReverbState);	// set current reverb state
				}
			}
			else
			{
				theErr = SAMPLE_TO_LARGE;
			}
		}
		else
		{
			theErr = NO_ERR;
		}
	}
	else
	{
		theErr = NO_VOLUME;
	}
	return BAE_TranslateOPErr(theErr);
}


BAEResult BAESound::StartDoubleBuffer(	void *buffer1,						// pointer to audio data 1 & 2
									void *buffer2,
									unsigned long frames, 				// number of frames of audio
									unsigned short int bitSize, 		// bits per sample 8 or 16
									unsigned short int channels, 		// mono or stereo 1 or 2
									BAE_UNSIGNED_FIXED rate, 			// 16.16 fixed sample rate
									BAE_UNSIGNED_FIXED sampleVolume, 	// sample volume	(1.0)
									short int stereoPosition,			// stereo placement
									void *refData, 						// callback reference
									BAEDoubleBufferCallbackPtr pDoubleBufferCallback,
									BAE_BOOL stopIfPlaying)
{
	OPErr	theErr;
	long	volume;

	mDoneCallback = NULL;
	mLoopDoneCallback = NULL;
	mCallbackReference = refData;
	mSoundVolume = sampleVolume;
	mStereoPosition = stereoPosition;
	theErr = NO_ERR;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		if (IsPlaying())		// sample playing?
		{
			if (stopIfPlaying)	// can we stop it
			{
				Stop();
			}
			else
			{
				theErr = STILL_PLAYING;
			}
		}
	}
	if (theErr == NO_ERR)
	{
		mSoundVoiceReference = DEAD_VOICE;
		volume = UNSIGNED_FIXED_TO_LONG_ROUNDED(sampleVolume * MAX_NOTE_VOLUME);
		if (volume)
		{
			mSoundVoiceReference = GM_SetupSampleDoubleBuffer((XPTR)buffer1, (XPTR)buffer2, frames, 
											rate, bitSize, channels,
											volume, stereoPosition,
											(void *)this, 
											(GM_DoubleBufferCallbackPtr)pDoubleBufferCallback,
											PV_DefaultSampleDoneCallback);
			if (mSoundVoiceReference == DEAD_VOICE)
			{
				theErr = NO_FREE_VOICES;
			}
			else
			{
				// ok
				GM_StartSample(mSoundVoiceReference);
				SetReverb(mReverbState);	// set current reverb state
			}
		}
		else
		{
			theErr = NO_ERR;
		}
	}
	else
	{
		theErr = NO_VOLUME;
	}
	return BAE_TranslateOPErr(theErr);
}

// Set a call back when song is done
void BAESound::SetDoneCallback(BAEDoneCallbackPtr pDoneProc, void * pReference)
{
	mDoneCallback = pDoneProc;
	mCallbackReference = pReference;
}


BAE_BOOL BAESound::IsPlaying(void)
{
	if (IsDone() == FALSE)
	{
		return TRUE;
	}
	return FALSE;
}

BAE_BOOL BAESound::IsDone(void)
{
	BAE_BOOL	done;

	done = TRUE;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		done = (BAE_BOOL)GM_IsSoundDone(mSoundVoiceReference);
		if (done)
		{
			mSoundVoiceReference = DEAD_VOICE;
		}
	}
	return done;
}

void BAESound::SetRate(BAE_UNSIGNED_FIXED newRate)
{
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		GM_ChangeSamplePitch(mSoundVoiceReference, newRate);
	}
}

BAE_UNSIGNED_FIXED BAESound::GetRate(void)
{
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		return GM_GetSamplePitch(mSoundVoiceReference);
	}
	return 0L;
}

void BAESound::SetVolume(BAE_UNSIGNED_FIXED volume)
{
	mSoundVolume = volume;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		GM_ChangeSampleVolume(mSoundVoiceReference, FIXED_TO_SHORT_ROUNDED(volume * MAX_NOTE_VOLUME));
	}
}

BAE_UNSIGNED_FIXED BAESound::GetVolume(void)
{
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		mSoundVolume = UNSIGNED_RATIO_TO_FIXED(GM_GetSampleVolume(mSoundVoiceReference), MAX_NOTE_VOLUME);
	}
	return mSoundVolume;
}

// fade from source volume, to dest volume in time miliseconds. Always async
void BAESound::FadeFromToInTime(BAE_FIXED sourceVolume, BAE_FIXED destVolume, BAE_FIXED timeInMiliseconds)
{
	short int	source, dest;
	double		delta;
	INT16		minVolume;
	INT16		maxVolume;

	if (mSoundVoiceReference != DEAD_VOICE)
	{
		delta = PV_CalculateTimeDeltaForFade(sourceVolume, destVolume, timeInMiliseconds);
		delta = delta * -MAX_NOTE_VOLUME;

		source = FIXED_TO_SHORT_ROUNDED(sourceVolume * MAX_NOTE_VOLUME);
		dest = FIXED_TO_SHORT_ROUNDED(destVolume * MAX_NOTE_VOLUME);
		minVolume = XMIN(source, dest);
		maxVolume = XMAX(source, dest);
		GM_SetSampleFadeRate(mSoundVoiceReference, FLOAT_TO_FIXED(delta), minVolume, maxVolume, FALSE);
	}
}

void BAESound::Fade(BAE_BOOL doAsync)
{
	short int	sampleVolume;

	if (mSoundVoiceReference != DEAD_VOICE)
	{
		sampleVolume = GM_GetSampleVolume(mSoundVoiceReference);
		if (doAsync == FALSE)
		{
			// We're going to fade the sample out and don't stop it		

			GM_SetSampleFadeRate(mSoundVoiceReference, FLOAT_TO_FIXED(2.2),
													0, sampleVolume, FALSE);
			while (GM_GetSampleVolume(mSoundVoiceReference) && (GM_IsSoundDone(mSoundVoiceReference) == FALSE)) 
			{
				GetMixer()->ServiceAudioOutputToFile();
				GetMixer()->ServiceIdle();
				XWaitMicroseocnds(1000);
			}
		}
		else
		{
			GM_SetSampleFadeRate(mSoundVoiceReference, FLOAT_TO_FIXED(2.2),
													0, sampleVolume, FALSE);
		}
	}
}


void BAESound::FadeTo(BAE_FIXED destVolume, BAE_BOOL doAsync)
{
	short int	soundVolume, saveVolume, newSoundVolume, saveNewSoundVolume;
	XFIXED		delta;

	if (mSoundVoiceReference != DEAD_VOICE)
	{
		newSoundVolume = FIXED_TO_SHORT_ROUNDED(destVolume * MAX_NOTE_VOLUME);
		saveNewSoundVolume = newSoundVolume;
		// We're going to fade the Sound out and don't stop it		
		soundVolume = GM_GetSampleVolume(mSoundVoiceReference);
		saveVolume = soundVolume;

		if (newSoundVolume < soundVolume)
		{	// fade out
			soundVolume = newSoundVolume;
			newSoundVolume = saveVolume;
			delta = FLOAT_TO_UNSIGNED_FIXED(2.2);
		}
		else
		{	// fade in
			delta = (XFIXED)FLOAT_TO_FIXED(-2.2);
		}
		if (doAsync == FALSE)
		{
			GM_SetSampleFadeRate(mSoundVoiceReference, delta, soundVolume, newSoundVolume, FALSE);
			while (GM_GetSampleVolume(mSoundVoiceReference) != saveNewSoundVolume)
			{
				GetMixer()->ServiceAudioOutputToFile();
				GetMixer()->ServiceIdle();
				XWaitMicroseocnds(1000);
			}
		}
		else
		{
			GM_SetSampleFadeRate(mSoundVoiceReference, delta, soundVolume, newSoundVolume, FALSE);
		}
	}
}

void BAESound::SetStereoPosition(short int stereoPosition)
{
	mStereoPosition = stereoPosition;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		GM_ChangeSampleStereoPosition(mSoundVoiceReference, stereoPosition);
	}
}

short int BAESound::GetStereoPosition(void)
{
	mStereoPosition;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		mStereoPosition = GM_GetSampleStereoPosition(mSoundVoiceReference);
	}
	return mStereoPosition;
}

void BAESound::SetReverb(BAE_BOOL useReverb)
{
	mReverbState = useReverb;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		GM_ChangeSampleReverb(mSoundVoiceReference, useReverb);
	}
}

BAE_BOOL BAESound::GetReverb(void)
{
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		mReverbState = GM_GetSampleReverb(mSoundVoiceReference);
	}
	return mReverbState;
}

void BAESound::SetReverbAmount(short int reverbAmount)
{
	mReverbAmount = reverbAmount;
	SetReverb((reverbAmount) ? TRUE : FALSE);
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		GM_SetSampleReverbAmount(mSoundVoiceReference, reverbAmount);
	}
}

short int BAESound::GetReverbAmount(void)
{
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		mReverbAmount = GM_GetSampleReverbAmount(mSoundVoiceReference);
	}
	return mReverbAmount;
}

void BAESound::SetLowPassAmountFilter(short int lowpassamount)
{
	mLowPassAmount = lowpassamount;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		GM_SetSampleLowPassAmountFilter(mSoundVoiceReference, lowpassamount);
	}
}

short int BAESound::GetLowPassAmountFilter(void)
{
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		mLowPassAmount = GM_GetSampleLowPassAmountFilter(mSoundVoiceReference);
	}
	return mLowPassAmount;
}

void BAESound::SetResonanceAmountFilter(short int resonance)
{
	mResonanceAmount = resonance;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		GM_SetSampleResonanceFilter(mSoundVoiceReference, resonance);
	}
}

short int BAESound::GetResonanceAmountFilter(void)
{
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		mResonanceAmount = GM_GetSampleResonanceFilter(mSoundVoiceReference);
	}
	return mResonanceAmount;
}

void BAESound::SetFrequencyAmountFilter(short int frequency)
{
	mFrequencyAmount = frequency;
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		GM_SetSampleFrequencyFilter(mSoundVoiceReference, frequency);
	}
}

short int BAESound::GetFrequencyAmountFilter(void)
{
	if (mSoundVoiceReference != DEAD_VOICE)
	{
		mFrequencyAmount = GM_GetSampleFrequencyFilter(mSoundVoiceReference);
	}
	return mFrequencyAmount;
}

#if 0
	#pragma mark ### BAEModSong class ###
#endif
#if USE_MOD_API
BAEModSong::BAEModSong(BAEOutputMixer *pBAEAudio, 
					 char const *cName, void * userReference) :
									BAENoise(pBAEAudio, cName, BAENoise::MOD_NOISE)
{
	mUserReference = userReference;
	pauseVariable = 0;
	pSoundVariables = NULL;
}

BAEModSong::~BAEModSong()
{
	Stop();

	if (pSoundVariables)
	{
		GM_FreeModFile((GM_ModData *)pSoundVariables);
		pSoundVariables = NULL;
	}
}

// currently paused
BAE_BOOL BAEModSong::IsPaused(void)
{
	return (pauseVariable) ? (BAE_BOOL)TRUE : (BAE_BOOL)FALSE;
}

void BAEModSong::Pause(void)
{
	if (pauseVariable == 0)
	{
		pauseVariable = 1;
		GM_PauseMod((GM_ModData *)pSoundVariables);
	}
}

void BAEModSong::Resume(void)
{
	if (pauseVariable)
	{
		pauseVariable = 0;
		GM_ResumeMod((GM_ModData *)pSoundVariables);
	}
}

// Load file into sound object. This will copy the file directly into memory. It
// will get disposed once you destroy this object.
BAEResult BAEModSong::LoadFromFile(const BAEPathName pModFilePath)
{
	XFILENAME		theFile;
	OPErr			theErr;
	long			fileSize;
	XPTR			pFileVariables;

	theErr = NO_ERR;
	XConvertNativeFileToXFILENAME(pModFilePath, &theFile);
	pFileVariables = PV_GetFileAsData(&theFile, &fileSize);
	if (pFileVariables)
	{
		pSoundVariables = GM_LoadModFile(pFileVariables, fileSize);
		if (pSoundVariables == NULL)
		{
			theErr = BAD_FILE;
		}
		XDisposePtr(pFileVariables);	// throw away file, we've parsed the mod file
	}
	else
	{
		theErr = MEMORY_ERR;
	}
	return BAE_TranslateOPErr(theErr);
}

// Load memory mapped MOD pointer into BAEModSong object. This will parse the MOD file and get
// it ready for playing. You can dispose of the data passed once this method returns
BAEResult BAEModSong::LoadFromMemory(void const* pModData, unsigned long modSize)
{
	OPErr			theErr;

	theErr = PARAM_ERR;
	if (pModData && modSize)
	{
		pSoundVariables = GM_LoadModFile((void *)pModData, modSize);
		if (pSoundVariables == NULL)
		{
			theErr = BAD_FILE;
		}
		else
		{
			theErr = NO_ERR;
		}
	}
	return BAE_TranslateOPErr(theErr);
}

static void PV_ModDoneCallback(GM_ModData *pMod)
{
	BAEDoneCallbackPtr pDoneProc;

	pDoneProc = (BAEDoneCallbackPtr)pMod->reference2;
	if (pDoneProc)
	{
		(*pDoneProc)((void *)pMod->reference);
	}		
}

BAEResult BAEModSong::Start(BAEDoneCallbackPtr pDoneProc)
{
	OPErr	theErr;

	theErr = BAD_FILE;
	if (pSoundVariables)
	{
		if (IsPlaying())
		{
			Stop();
		}
		((GM_ModData *)pSoundVariables)->reference2 = (long)pDoneProc;
		GM_BeginModFile((GM_ModData *)pSoundVariables, 
						(GM_ModDoneCallbackPtr)PV_ModDoneCallback, (long)GetReference());
		theErr = NO_ERR;
	}
	return BAE_TranslateOPErr(theErr);
}

void BAEModSong::Stop(BAE_BOOL startFade)
{
	short int	songVolume;

	if (IsPaused())
	{
		Resume();
	}
	if (pSoundVariables)
	{
		if (startFade)
		{
			songVolume = GM_GetModVolume((GM_ModData *)pSoundVariables);
			GM_SetModFadeRate((GM_ModData *)pSoundVariables, FLOAT_TO_FIXED(2.2),
													0, songVolume, TRUE);
		}
		else
		{
			GM_StopModFile((GM_ModData *)pSoundVariables);
		}
	}
}


// fade from source volume, to dest volume in time miliseconds. Always async
void BAEModSong::FadeFromToInTime(BAE_FIXED sourceVolume, BAE_FIXED destVolume, BAE_FIXED timeInMiliseconds)
{
	short int	source, dest;
	double		delta;
	INT16		minVolume;
	INT16		maxVolume;

	if (pSoundVariables)
	{
		delta = PV_CalculateTimeDeltaForFade(sourceVolume, destVolume, timeInMiliseconds);
		delta = delta * -MAX_SONG_VOLUME;

		source = FIXED_TO_SHORT_ROUNDED(sourceVolume * MAX_SONG_VOLUME);
		dest = FIXED_TO_SHORT_ROUNDED(destVolume * MAX_SONG_VOLUME);
		minVolume = XMIN(source, dest);
		maxVolume = XMAX(source, dest);
		GM_SetModFadeRate((GM_ModData *)pSoundVariables, FLOAT_TO_FIXED(delta), minVolume, maxVolume, FALSE);
	}
}

void BAEModSong::Fade(BAE_BOOL doAsync)
{
	short int	songVolume;

	if (pSoundVariables)
	{
		songVolume = GM_GetModVolume((GM_ModData *)pSoundVariables);
		if (doAsync == FALSE)
		{
			// We're going to fade the song out and don't stop it
			GM_SetModFadeRate((GM_ModData *)pSoundVariables, FLOAT_TO_FIXED(2.2),
													0, songVolume, FALSE);
			while (	GM_GetModVolume((GM_ModData *)pSoundVariables) && 
					GM_IsModPlaying((GM_ModData *)pSoundVariables)) 
			{
				GetMixer()->ServiceAudioOutputToFile();
				GetMixer()->ServiceIdle();
				XWaitMicroseocnds(1000);
			}
		}
		else
		{
			GM_SetModFadeRate((GM_ModData *)pSoundVariables, FLOAT_TO_FIXED(2.2),
															0, songVolume, FALSE);
		}
	}
}

// FadeTo a volume level
void BAEModSong::FadeTo(BAE_FIXED destVolume, BAE_BOOL doAsync)
{
	short int	songVolume, saveVolume, newSongVolume, saveNewSongVolume;
	XFIXED		delta;

	if (pSoundVariables)
	{
		newSongVolume = FIXED_TO_SHORT_ROUNDED(destVolume * MAX_SONG_VOLUME);
		saveNewSongVolume = newSongVolume;
		// We're going to fade the song out before we stop it.		
		songVolume = GM_GetModVolume((GM_ModData *)pSoundVariables);
		saveVolume = songVolume;

		if (newSongVolume < songVolume)
		{	// fade out
			songVolume = newSongVolume;
			newSongVolume = saveVolume;
			delta = FLOAT_TO_UNSIGNED_FIXED(2.2);
		}
		else
		{	// fade in
			delta = (XFIXED)FLOAT_TO_FIXED(-2.2);
		}
		if (doAsync == FALSE)
		{
			GM_SetModFadeRate((GM_ModData *)pSoundVariables, delta,
								songVolume, newSongVolume, FALSE);
			while (GM_GetModVolume((GM_ModData *)pSoundVariables) != saveNewSongVolume)
			{
				GetMixer()->ServiceAudioOutputToFile();
				GetMixer()->ServiceIdle();
				XWaitMicroseocnds(1000);
			}
		}
		else
		{
			GM_SetModFadeRate((GM_ModData *)pSoundVariables, delta,
								songVolume, newSongVolume, FALSE);
		}
	}
}

// Set a call back when song is done
void BAEModSong::SetDoneCallback(BAEDoneCallbackPtr pDoneProc, void * pReference)
{
	if (pSoundVariables)
	{
		((GM_ModData *)pSoundVariables)->reference = (long)pReference;
		((GM_ModData *)pSoundVariables)->reference2 = (long)pDoneProc;
	}
}


// set song master tempo. (1.0 uses songs encoded tempo, 2.0 will play
// song twice as fast, and 0.5 will play song half as fast
void BAEModSong::SetMasterTempo(BAE_UNSIGNED_FIXED tempoFactor)
{
	if (pSoundVariables)
	{
		GM_SetModTempoFactor((GM_ModData *)pSoundVariables, tempoFactor);
	}
}


BAE_BOOL BAEModSong::IsPlaying(void)
{
	BAE_BOOL	play;

	play = FALSE;
	if (pSoundVariables)
	{
		play = GM_IsModPlaying((GM_ModData *)pSoundVariables);
	}
	return play;
}

BAE_BOOL BAEModSong::IsDone(void)
{
	if (IsPlaying() == FALSE)
	{
		return TRUE;
	}
	return FALSE;
}

void BAEModSong::SetVolume(BAE_UNSIGNED_FIXED volume)
{
	if (pSoundVariables)
	{
		GM_SetModVolume((GM_ModData *)pSoundVariables,
						(short int)(UNSIGNED_FIXED_TO_LONG_ROUNDED(volume * MAX_SONG_VOLUME)));
	}
}

BAE_UNSIGNED_FIXED BAEModSong::GetVolume(void)
{
	BAE_UNSIGNED_FIXED	value;

	value = 0;
	if (pSoundVariables)
	{
		value = UNSIGNED_RATIO_TO_FIXED(GM_GetModVolume((GM_ModData *)pSoundVariables),
									MAX_SONG_VOLUME);
	}
	return value;
}

// sets tempo in beats per minute
void BAEModSong::SetTempoInBeatsPerMinute(unsigned long newTempoBPM)
{
	if (pSoundVariables)
	{
		GM_SetModTempoBPM((GM_ModData *)pSoundVariables, newTempoBPM);
	}
}

// returns tempo in beats per minute
unsigned long BAEModSong::GetTempoInBeatsPerMinute(void)
{
	unsigned long tempo;
	
	tempo = 0;
	if (pSoundVariables)
	{
		tempo = (unsigned long)GM_GetModTempoBPM((GM_ModData *)pSoundVariables);
	}
	return tempo;
}

// pass TRUE to entire loop song, FALSE to not loop
void BAEModSong::SetLoopFlag(BAE_BOOL loop)
{
	if (pSoundVariables)
	{
		GM_SetModLoop((GM_ModData *)pSoundVariables, loop);
	}
}

BAE_BOOL BAEModSong::GetLoopFlag(void)
{
	if (pSoundVariables)
	{
		return (BAE_BOOL)GM_GetModLoop((GM_ModData *)pSoundVariables);
	}
	return FALSE;
}

unsigned long BAEModSong::GetInfoSize(BAEInfoType infoType)
{
	unsigned long	size;

	size = 0;

	if (pSoundVariables)
	{
		switch (infoType)
		{
			case TITLE_INFO:
				size = GM_GetModSongNameLength((GM_ModData *)pSoundVariables);
				break;
			case COMPOSER_NOTES_INFO:
				size = GM_GetModSongCommentsLength((GM_ModData *)pSoundVariables);
				break;
		}
	}
	return size;
}

BAEResult BAEModSong::GetInfo(BAEInfoType infoType, char *cInfo)
{
	if (pSoundVariables && cInfo)
	{
		cInfo[0] = 0;
		switch (infoType)
		{
			case TITLE_INFO:
				GM_GetModSongName((GM_ModData *)pSoundVariables, cInfo);
				break;
			case COMPOSER_NOTES_INFO:
				GM_GetModSongComments((GM_ModData *)pSoundVariables, cInfo);
				break;
		}
	}
	return BAE_NO_ERROR;
}

#endif	// USE_MOD_API


#if 0
	#pragma mark ### BAENoiseGroup class ###
#endif

BAENoiseGroup::BAENoiseGroup(BAEOutputMixer *pBAEAudio, 
					 char const *cName, void * userReference) :
									BAENoise(pBAEAudio, cName, BAENoise::GROUP_NOISE)
{
	mUserReference = userReference;
	m_topStream = NULL;
	m_topSound = NULL;
	linkedPlaybackReference = DEAD_LINKED_VOICE;
}


BAENoiseGroup::~BAENoiseGroup()
{
	BAENoise	*pTop, *pNext;
	BAESound		*pSound;
	BAESoundStream	*pStream;

	Stop();		// stop this group

	// clear links for BAESound objects
	pTop = (BAENoise *)m_topSound;
	m_topSound = NULL;
	while (pTop)
	{
		pSound = (BAESound *)pTop;
		delete pSound;
		pNext = pTop->pGroupNext;
		pTop->pGroupNext = NULL;
		pTop = pNext;
	}

	// clear links for BAESoundStream objects
	pTop = (BAENoise *)m_topStream;
	m_topStream = NULL;
	while (pTop)
	{
		pStream = (BAESoundStream *)pTop;
		delete pStream;
		pNext = pTop->pGroupNext;
		pTop->pGroupNext = NULL;
		pTop = pNext;
	}
}

// Associate an BAE object to this group
BAEResult BAENoiseGroup::AddSound(BAESound *pSound)
{
	BAESound	*pTop, *pNext;

	// add to link
	pTop = (BAESound *)m_topSound;
	if (pTop)
	{
		pNext = NULL;
		while (pTop)
		{
			pNext = pTop;
			pTop = (BAESound *)pTop->pGroupNext;
		}
		if (pNext)
		{
			pNext->pGroupNext = pSound;
		}
	}
	else
	{
		m_topSound = pSound;
	}
	return BAE_NO_ERROR;
}

BAEResult BAENoiseGroup::AddStream(BAESoundStream *pStream)
{
	BAESoundStream			*pTop, *pNext;
	BAEResult					err;

	err = BAE_NO_ERROR;
	// add to link
	pTop = (BAESoundStream *)m_topStream;
	if (pTop)
	{
		if (pStream->mSoundStreamVoiceReference)
		{
			// add link into our local list
			pNext = NULL;
			while (pTop)
			{
				pNext = pTop;
				pTop = (BAESoundStream *)pTop->pGroupNext;
			}
			if (pNext)
			{
				pNext->pGroupNext = pStream;
			}
		}
		else
		{
			err = BAE_NOT_SETUP;
		}
	}
	else
	{
		m_topStream = pStream;
	}

	return err;
}

// Disassociate an BAE object from this group
BAEResult BAENoiseGroup::RemoveSound(BAESound *pSound)
{
	BAESound	*pTop;

	// remove link
	pTop = (BAESound *)m_topSound;
	if (pTop != pSound)
	{
		while (pTop)
		{
			if (pTop->pGroupNext == pSound)
			{
				pTop->pGroupNext = pSound->pGroupNext;
				break;
			}

			pTop = (BAESound *)pTop->pGroupNext;
		}
	}
	else
	{
		m_topSound = NULL;
	}
	return BAE_NO_ERROR;
}

BAEResult BAENoiseGroup::RemoveStream(BAESoundStream *pStream)
{
	BAESoundStream	*pTop;

	// remove link
	pTop = (BAESoundStream *)m_topStream;
	if (pTop != pStream)
	{
		while (pTop)
		{
			if (pTop->pGroupNext == pStream)
			{
				pTop->pGroupNext = pStream->pGroupNext;
				break;
			}

			pTop = (BAESoundStream *)pTop->pGroupNext;
		}
	}
	else
	{
		m_topStream = NULL;
	}
	return BAE_NO_ERROR;
}

BAEResult BAENoiseGroup::Start(void)
{
	BAENoise			*pNext;
	BAESound				*pSound;
#if USE_STREAM_API == TRUE
	BAESoundStream			*pStream;
	LINKED_STREAM_REFERENCE	link, top;
#endif

	// process sounds
	pNext = (BAENoise *)m_topSound;
	while (pNext)
	{
		pSound = (BAESound *)pNext;
		pSound->__Start();
		pNext = pNext->pGroupNext;
	}

#if USE_STREAM_API == TRUE
	// process streams
	pNext = (BAENoise *)m_topStream;
	top = NULL;
	while (pNext)
	{
		pStream = (BAESoundStream *)pNext;
		pStream->Preroll();

		link = GM_NewLinkedStreamList((STREAM_REFERENCE)pStream->mSoundStreamVoiceReference, NULL);
		top = GM_AddLinkedStream(top, link);
		pNext = pNext->pGroupNext;
	}
	if (top)
	{
		GM_StartLinkedStreams(top);
		GM_FreeLinkedStreamList(top);
	}
#endif

	return BAE_NO_ERROR;
}

void BAENoiseGroup::Stop(BAE_BOOL startFade)
{
	BAENoise	*pNext;
	BAESound		*pSound;
#if USE_STREAM_API == TRUE
	BAESoundStream	*pStream;
#endif
	// process sounds
	pNext = (BAENoise *)m_topSound;
	while (pNext)
	{
		pSound = (BAESound *)pNext;
		pSound->Stop(startFade);
		pNext = pNext->pGroupNext;
	}

#if USE_STREAM_API == TRUE
	// process streams
	pNext = (BAENoise *)m_topStream;
	while (pNext)
	{
		pStream = (BAESoundStream *)pNext;
		pStream->Stop(startFade);
		pNext = pNext->pGroupNext;
	}
#endif
}

// Set the stereo position an entire group (-63 left to 63 right, 0 is middle)
void BAENoiseGroup::SetStereoPosition(short int stereoPosition)
{
	BAENoise	*pNext;
#if USE_STREAM_API == TRUE
	BAESoundStream	*pStream;
#endif
	BAESound		*pSound;

	// process sounds
	pNext = (BAENoise *)m_topSound;
	while (pNext)
	{
		pSound = (BAESound *)pNext;
		pSound->SetStereoPosition(stereoPosition);
		pNext = pNext->pGroupNext;
	}

#if USE_STREAM_API == TRUE
	// process streams
	pNext = (BAENoise *)m_topStream;
	while (pNext)
	{
		pStream = (BAESoundStream *)pNext;
		pStream->SetStereoPosition(stereoPosition);
		pNext = pNext->pGroupNext;
	}
#endif
}

// Set the volume level of an entire group
void BAENoiseGroup::SetVolume(BAE_UNSIGNED_FIXED newVolume)
{
	BAENoise	*pNext;
#if USE_STREAM_API == TRUE
	BAESoundStream	*pStream;
#endif
	BAESound		*pSound;

	// process sounds
	pNext = (BAENoise *)m_topSound;
	while (pNext)
	{
		pSound = (BAESound *)pNext;
		pSound->SetVolume(newVolume);
		pNext = pNext->pGroupNext;
	}

#if USE_STREAM_API == TRUE
	// process streams
	pNext = (BAENoise *)m_topStream;
	while (pNext)
	{
		pStream = (BAESoundStream *)pNext;
		pStream->SetVolume(newVolume);
		pNext = pNext->pGroupNext;
	}
#endif
}

// Enable/Disable reverb of an entire group
void BAENoiseGroup::SetReverb(BAE_BOOL useReverb)
{
	BAENoise	*pNext;
#if USE_STREAM_API == TRUE
	BAESoundStream	*pStream;
#endif
	BAESound		*pSound;

	// process sounds
	pNext = (BAENoise *)m_topSound;
	while (pNext)
	{
		pSound = (BAESound *)pNext;
		pSound->SetReverb(useReverb);
		pNext = pNext->pGroupNext;
	}

#if USE_STREAM_API == TRUE
	// process streams
	pNext = (BAENoise *)m_topStream;
	while (pNext)
	{
		pStream = (BAESoundStream *)pNext;
		pStream->SetReverb(useReverb);
		pNext = pNext->pGroupNext;
	}
#endif
}

// set reverb mix amount of an entire group
//MOE: "amount" should be typed UBYTE
void BAENoiseGroup::SetReverbAmount(short int reverbAmount)
{
	BAENoise	*pNext;
#if USE_STREAM_API == TRUE
	BAESoundStream	*pStream;
#endif
	BAESound		*pSound;

	// process sounds
	pNext = (BAENoise *)m_topSound;
	while (pNext)
	{
		pSound = (BAESound *)pNext;
		pSound->SetReverbAmount(reverbAmount);
		pNext = pNext->pGroupNext;
	}

#if USE_STREAM_API == TRUE
	// process streams
	pNext = (BAENoise *)m_topStream;
	while (pNext)
	{
		pStream = (BAESoundStream *)pNext;
		pStream->SetReverbAmount(reverbAmount);
		pNext = pNext->pGroupNext;
	}
#endif
}

// EOF of BAE.cpp



