
#if !defined(_GAME_SOUND_DEVICE_H)
#define _GAME_SOUND_DEVICE_H


#include "GameSoundDefs.h"
#include <Locker.h>


//	This stuff (or something like it) will appear in post-Genki releases with
//	BGameSoundDevice and BGameSoundRoster. Move it into GameSoundDefs at that time.

//	flags for gs_info information
enum gs_info_flags
{
	B_GS_SOFT_BUFFER = 0x1,
	B_GS_HARD_BUFFER = 0x2,
	B_GS_IS_PLAYING = 0x4,
	B_GS_IS_LOOPING = 0x8
};
//	information about a currently allocated sound (which may or may not be playing)
struct gs_info
{
public:
		size_t		current_frame;		//	last known-to-be-played frame
		size_t		next_frame;	//	first known-to-not-being-played frame
		size_t		frame_count;	//	total number of frames
		uint32		flags;
		void		(*hook)(void * cookie, gs_id sound, size_t frame);
		void *		cookie;
		gs_audio_format
					format;
};


enum gs_capability
{
	B_GS_NO_CAPABILITY,
	B_GS_SOFTWARE_CHANNEL_COUNT = 17,	//	int32 count
	B_GS_SOFTWARE_CHANNELS_USED,		//	int32 count
	B_GS_HARDWARE_CHANNEL_COUNT,		//	int32 count
	B_GS_HARDWARE_CHANNELS_USED,		//	int32 count
	B_GS_FORMAT_LIST = 33,				//	gs_audio_format
	B_GS_VALIDATE_FORMAT,				//	gs_audio_format
	B_GS_ATTRIBUTE_INFO,				//	gs_attribute_info
	B_GS_FIRST_PRIVATE_CAPABILITY = 90000,
	B_GS_FIRST_USER_CAPABILITY = 100000
};

enum gs_flags
{
	B_GS_REQUIRE_SOFT_BUFFER = 0x1,
	B_GS_REQUIRE_HARD_BUFFER = 0x2,
	B_GS_INHIBIT_AUTOCONVERT = 0x4,		//	Else sound data will be auto-converted to
										//	(possibly mono) version of main format.
	B_GS_LOOPING_BUFFER = 0x8,			//	Will typically be soft.
	B_GS_UNCHANGED_BUFFER = 0x10000,	//	for UnlockSound
	B_GS_GRANULAR_CALLBACK = 0x20000,	//	for SetSoundCallback
	B_GS_CALLBACK_AT_END = 0x40000		//	for SetSoundCallback
};



class BGameSoundDevice
{
public:

virtual				~BGameSoundDevice();

//	attributes and stuff
virtual	status_t	InitCheck() const;
virtual	status_t	GetDeviceName(
							char * ioDevice,
							size_t inDeviceSize,
							char * ioVendor,
							size_t inVendorSize,
							int32 * outVersion,
							int32 * outAPIVersion);
virtual	status_t	SetMainFormat(
							const gs_audio_format * inFormat);
virtual	status_t	GetMainFormat(
							gs_audio_format * outFormat);
virtual	status_t	SetGranularity(
							size_t inFrameCount);
virtual	ssize_t		Granularity();
virtual	status_t	GetCapabilityList(
							GSCapability * outCapabilities,
							size_t * ioCapabilityCount);
virtual	status_t	GetCapability(
							GSCapability inCapability,
							void * ioBuffer,
							size_t * ioBufferSize);
virtual	status_t	SetCapability(
							GSCapability inCapability,
							void * ioBuffer,
							size_t * ioBufferSize);
		ssize_t		CountChannels(
							const gs_audio_format * inWithFormat = NULL,
							uint32 inWithFlags = 0);
		status_t	VerifyFormat(
							gs_audio_format * ioFormat,
							uint32 inWithFlags = 0);

//	actually doing stuff
virtual	status_t	Start() = 0;		//	will return when sound can start playing
virtual	bool		IsRunning() = 0;
virtual	status_t	Stop() = 0;			//	will stop, flush, and return when done

virtual	status_t	SetMainCallback(		//	callback will be called from MixMore()
							void (*hook)(void * inCookie, void * inBuffer, size_t inByteCount, BGameSoundDevice * me),
							void * cookie,
							uint32 inFlags = 0);

//	convenient individual-sound wrappers for the batch-sound API below
		status_t	LoadSound(
							const void * inData,
							size_t inFrameCount,
							const gs_audio_format * inFormat,
							gs_id * outSound,
							uint32 inFlags = 0);
		status_t	LoadSound(
							const entry_ref * inFile,
							gs_id * outSound,
							uint32 inFlags = 0);
		status_t	AllocateSound(
							size_t inFrameCount,
							gs_id * outSound,
							const gs_audio_format * inFormat = NULL,
							uint32 inFlags = 0);
		status_t	CloneSound(
							gs_id inSound,
							gs_id * outSound,
							uint32 inFlags = 0);
		status_t	FreeSound(
							gs_id inSound,
							uint32 inFlags = 0);
		status_t	LockSound(
							gs_id inSound,			//	use B_MAIN_SOUND for main buffer
							size_t inRequestFirstFrame, 
							size_t inRequestFrameCount, 
							void ** outBufferPtr1,
							size_t * outFrameCount1,
							void ** outBufferPtr2,
							size_t * outFrameCount2,
							uint32 inFlags = 0);
		status_t	UnlockSound(
							gs_id inSound,
							uint32 inFlags = 0);
		status_t	StartSound(
							gs_id inSound,
							uint32 inFlags = 0);
		status_t	StopSound(
							gs_id inSound,
							uint32 inFlags = 0);
		status_t	SetSoundGain(
							gs_id inSound,
							float inVolume,
							float ramp = 0.1);
		status_t	SetSoundPan(
							gs_id inSound,
							float inPan,
							float ramp = 0.1);
		status_t	GetSoundInfo(
							gs_id inSound,
							gs_info * outInfo,
							size_t inInfoSize = sizeof(gs_info));
		status_t	GetSoundInfos(
							const gs_id * inSounds,
							gs_info * outInfo,
							size_t inSoundCount, 
							size_t inInfoSize = sizeof(gs_info));
		status_t	GetSoundAttributes(
							gs_id inSound, 
							gs_attribute * ioAttributes,
							size_t inAttributeCount = 1);
		status_t	SetSoundAttributes(
							gs_id inSound,
							gs_attribute * ioAttributes,
							size_t inAttributeCount = 1);
		status_t	SetSoundCallback(
							gs_id inSound,
							void (*callback)(void * cookie, gs_id sound, size_t atFrame, BGameSoundDevice * me),
							void * cookie,
							uint32 inFlags = 0);

//	actual implementation of functionality allows for batching of operations
		status_t	LoadSounds(
							const void ** inDatas,
							const size_t * inFrameCounts,
							const gs_audio_format * inFormats,
							gs_id * outSounds,
							size_t inSoundCount,
							const uint32 * inFlags = 0);
		status_t	LoadSounds(
							const entry_ref ** inFiles,
							gs_id * outSounds,
							size_t inSoundCount, 
							const uint32 * inFlags = 0);
virtual	status_t	AllocateSounds(
							const size_t * inFrameCounts,
							gs_id * outSounds,
							const gs_audio_format * inFormats,
							const uint32 * inFlags);
virtual	status_t	CloneSounds(
							const gs_id * inSounds,
							gs_id * outSounds,
							size_t inSoundCount,
							const uint32 * inFlags = 0);
virtual	status_t	FreeSounds(
							const gs_id * inSounds,
							size_t inSoundCount,
							const uint32 * inFlags = 0);
virtual	status_t	LockSounds(
							const gs_id * inSounds,
							const size_t * inRequestFirstFrames,
							const size_t * inRequestFrameCounts,
							void ** outBufferPtrs1,
							size_t * outFrameCounts1,
							void ** outBufferPtrs2,
							size_t * outFrameCounts2,
							size_t inSoundCount,
							const uint32 * inFlags = 0);
virtual	status_t	UnlockSounds(
							const gs_id * inSounds,
							size_t inSoundCount,
							const uint32 * inFlags = 0);
virtual	status_t	StartSounds(
							const gs_id * inSounds,
							size_t inSoundCount,
							const uint32 * inFlags = 0);
virtual	bool		IsSoundPlaying(
							gs_id inSound);
virtual	status_t	StopSounds(
							const gs_id * inSounds,
							size_t inSoundCount, 
							const uint32 * inFlags = 0);
virtual	status_t	GetAllSoundAttributes(
							gs_id inSound,
							gs_attribute * outAttributes,
							size_t * ioAttrCount);
virtual	status_t	GetSoundAttributes(
							const gs_id * inSounds, 
							gs_attribute ** ioAttributes,
							size_t inSoundCount,
							size_t inAttributeCount = 1);
virtual	status_t	SetSoundAttributes(
							const gs_id * inSound,
							gs_attribute ** ioAttributes,
							size_t inSoundCount,
							size_t inAttributeCount = 1);
virtual	status_t	SetSoundCallbacks(
							const gs_id * inSound,
							void (*callback)(void * cookie, gs_id sound, size_t atFrame, BGameSoundDevice * me),
							void ** cookies,
							size_t inSoundCount,
							const uint32 * inFlags = 0);

#if _ADVANCED
//	for simple user configuration of the device
virtual	BView *		MakeSettingsView();
virtual	status_t	ApplySettings(
							BView * inFromView);
virtual	status_t	SaveSettings(
							BMessage * ioSettings);
virtual	status_t	RestoreSettings(
							const BMessage * inSettings);
#endif

#if _ADVANCED
//	this is for CD sound tracks -- should we support WAV file soundtracks? MOD?
virtual	status_t	StartCDPlayer(
							int32 inTrack,
							bool inLoop = true,
							int32 inPlayer = -1);
virtual	status_t	StopCDPlayer(
							int32 inPlayer = -1);
virtual	status_t	GetCDPlayerInfo(
							bool * outDetected,
							char * outVolumeName,
							size_t inVolumeNameSize,
							float * outMinGain,
							float * outMaxGain,
							int32 inPlayer = -1);
virtual	ssize_t		CountCDPlayers();
		status_t	SetCDPlayerGain(
							float inGain,
							int32 inPlayer = -1);
		status_t	GetCDPlayerGain(
							float * outGain,
							int32 inPlayer = -1);
#endif

protected:

		enum		//	subclasses should allocate sound handles from HW_HANDLE and up
		{
			MIN_HANDLE = 1024,
			HW_HANDLE = 16384
		};

		bool		Lock();
		void		Unlock();

		void		SetInitError(
							status_t initError);

					BGameSoundDevice(
							const char * inDevice,
							const char * inVendor);

		status_t	MixMore(			//	useful for subclasses
							void * outData,
							size_t inByteCount,
							bigtime_t inPlayTime,
							const gs_audio_format * format);
virtual	status_t	Perform(
							int32 selector,
							void * data,
							size_t * ioSize);

private:

	friend class BGameSoundRoster;

		//	implementation cruft goes here

		BLocker			_fLock;

		status_t		_fInitError;
		char *			_fDeviceStr;
		char *			_fVendorStr;
		void *			_fAddonInfo;

		struct attribute
		{
			float		value;
			float		target;
			float		speed;
		};
		struct sound_data
		{
			void *		data;
			size_t		bytecount;
			int32		refcount;
			area_id		area;
		};
		struct soft_sound
		{
			uint32		format;
			int32		channels;
			bool		swapendian;
			bool		playing;
			bool		changing;
			bool		looping;
			float		l_mult;
			float		r_mult;
			attribute	samplerate;
			attribute	gain;
			attribute	pan;
			sound_data *data;
		};

		sound_data *	_fSoundData;
		int32			_fSoundDataCount;
		soft_sound *	_fSoftSound;
		int32			_fSoftSoundCount;

		status_t		alloc_structs();
		void			free_structs();
		sound_data *	find_free_sound_data();
		soft_sound *	find_free_soft_sound();
		void			deallocate_soft_sound(soft_sound * sound);
		void			deallocate_sound_data(sound_data * data);
		soft_sound *	handle_to_sound(gs_id handle);
		gs_id
						sound_to_handle(soft_sound * sound);

virtual	status_t	_Reserved_GameSoundDevice_1(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_2(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_3(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_4(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_5(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_6(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_7(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_8(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_9(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_10(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_11(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_12(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_13(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_14(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_15(void *, ...);
virtual	status_t	_Reserved_GameSoundDevice_16(void *, ...);

		uint32		_reserved_GameSoundDevice_[32];

};


#if B_BUILDING_GS_ADDON
extern "C" status_t get_device_info(int32 inIndex, char * outDevice, int32 inDeviceSize, char * outVendor, int32 inVendorSize);
extern "C" BGameSoundDevice * make_device(const char * inDevice);
#endif


#endif	//	_GAME_SOUND_DEVICE_H

