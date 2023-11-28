
#include <OS.h>
#include <Locker.h>
#include <Autolock.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <new>

#include "GameSoundDevice.h"
#include "GameSoundRoster.h"


static const char * _gs_debug_str = getenv("GAME_SOUND_DEBUG");
int _gs_debug = atoi(_gs_debug_str ? _gs_debug_str : "0");

#define SOUND_DATA_COUNT _s_sound_data_count;
#define SOFT_SOUND_COUNT _s_soft_sound_count;

static int get_value(const char * env, int val)
{
	char * v = getenv(env);
	if (v != NULL)
	{
		int r = atoi(v);
		if (r >= 1) return r;
	}
	return val;
}

static int _s_sound_data_count = get_value("GAME_SOUND_SOFT_COUNT", 32);
static int _s_soft_sound_count = get_value("GAME_SOUND_SOFT_COUNT", 32);


BGameSoundDevice::~BGameSoundDevice()
{
//	BAutolock lock(BGameSoundRoster::_s_lock);
//	if (_fAddonInfo) ((_gd_loaded_addon *)_fAddonInfo)->release();
	BGameSoundRoster::Unregister(this);
	free(_fDeviceStr);
	free(_fVendorStr);
	free_structs();
}

status_t 
BGameSoundDevice::InitCheck() const
{
	return _fInitError;
}

status_t 
BGameSoundDevice::GetDeviceName(char *ioDevice, size_t inDeviceSize, char *ioVendor, size_t inVendorSize, int32 *outVersion, int32 *outAPIVersion)
{
	if (_fInitError < 0) return _fInitError;
	if (ioDevice)
	{
		strncpy(ioDevice, _fDeviceStr, inDeviceSize);
		ioDevice[inDeviceSize-1] = 0;
	}
	if (ioVendor)
	{
		strncpy(ioVendor, _fVendorStr, inVendorSize);
		ioVendor[inVendorSize-1] = 0;
	}
	return B_OK;
}

status_t 
BGameSoundDevice::SetMainFormat(const gs_audio_format *inFormat)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::GetMainFormat(gs_audio_format *outFormat)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::SetGranularity(size_t inFrameCount)
{
	//fixme	implementation goes here
}

ssize_t 
BGameSoundDevice::Granularity()
{
	//fixme	implementation goes here
}


status_t
BGameSoundDevice::GetCapabilityList(
	GSCapability * outCapabilities,
	size_t * ioCapabilityCount)
{
	static GSCapability caps[] = {
		B_GS_HARDWARE_CHANNEL_COUNT, 
		B_GS_HARDWARE_CHANNELS_USED, 
		B_GS_SOFTWARE_CHANNEL_COUNT,
		B_GS_SOFTWARE_CHANNELS_USED,
		//fixme	implementation of GS_FORMAT_LIST goes here
		B_GS_NO_CAPABILITY
	};
	size_t cnt = *ioCapabilityCount;
	int ix = 0;
	while ((ix < cnt) && (caps[ix] != B_GS_NO_CAPABILITY))
	{
		outCapabilities[ix] = caps[ix];
		ix++;
	}
	*ioCapabilityCount = ix;
	return B_OK;
}


status_t 
BGameSoundDevice::GetCapability(GSCapability inCapability, void *ioBuffer, size_t *ioBufferSize)
{
	if (_fInitError < 0) return _fInitError;
	BAutolock lock(_fLock);
	switch (inCapability)
	{
	case B_GS_HARDWARE_CHANNEL_COUNT:
		*(int32*)ioBuffer = 0;
		break;
	case B_GS_HARDWARE_CHANNELS_USED:
		*(int32*)ioBuffer = 0;
		break;
	case B_GS_SOFTWARE_CHANNEL_COUNT:
		*(int32*)ioBuffer = _fSoftSoundCount;
		break;
	case B_GS_SOFTWARE_CHANNELS_USED:
		*(int32*)ioBuffer = 0;
		for (int ix=0; ix<_fSoftSoundCount; ix++)
			if (_fSoftSound[ix].data != NULL)
				(*(int32*)ioBuffer)++;
		break;
	case B_GS_FORMAT_LIST:
	case B_GS_VALIDATE_FORMAT:
	case B_GS_ATTRIBUTE_INFO:
		//fixme	implementation goes here
	default:
		return B_ERROR;
	}
	return B_OK;
}

status_t 
BGameSoundDevice::SetCapability(GSCapability inCapability, void *ioBuffer, size_t *ioBufferSize)
{
	//fixme	implementation of GS_FORMAT_LIST and friends goes here
	return B_GS_READ_ONLY_VALUE;
}

ssize_t 
BGameSoundDevice::CountChannels(const gs_audio_format *inWithFormat, uint32 inWithFlags)
{
	int32 hw_count = 0;
	int32 sw_count = 0;
	status_t err = B_OK;
	size_t size;
	BAutolock lock(_fLock);
	if (!(inWithFlags & B_GS_REQUIRE_HARD_BUFFER))
	{
		size = sizeof(sw_count);
		err = GetCapability(B_GS_SOFTWARE_CHANNEL_COUNT, &sw_count, &size);
	}
	if (!err && !(inWithFlags & B_GS_REQUIRE_SOFT_BUFFER))
	{
		size = sizeof(hw_count);
		err = GetCapability(B_GS_HARDWARE_CHANNEL_COUNT, &hw_count, &size);
	}
	if (err < B_OK) return err;
	return sw_count + hw_count;
}

status_t 
BGameSoundDevice::VerifyFormat(gs_audio_format *ioFormat, uint32 inWithFlags)
{
	return SetCapability(B_GS_VALIDATE_FORMAT, ioFormat, sizeof(*ioFormat));
}

status_t 
BGameSoundDevice::Start()
{
	puts("BGameSoundDevice::Start() is pure virtual.");
	return B_ERROR;
}

bool 
BGameSoundDevice::IsRunning()
{
	puts("BGameSoundDevice::IsRunning() is pure virtual.");
	return false;
}

status_t 
BGameSoundDevice::Stop()
{
	puts("BGameSoundDevice::Stop() is pure virtual.");
	return B_ERROR;
}

status_t 
BGameSoundDevice::LoadSound(const void *inData, size_t inFrameCount,
	const gs_audio_format *inFormat, gs_id *outSound, uint32 inFlags)
{
	return LoadSounds(&inData, &inFrameCount, inFormat, outSound, 1, &inFlags);
}

status_t 
BGameSoundDevice::LoadSounds(const void **inDatas, const size_t *inFrameCounts, const gs_audio_format *inFormats, gs_id *outSounds, size_t inSoundCount, const uint32 *inFlags)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::LoadSound(const entry_ref *inFile, gs_id *outSound, uint32 inFlags)
{
	return LoadSounds(&inFile, outSound, 1, &inFlags);
}

status_t 
BGameSoundDevice::LoadSounds(const entry_ref **inFiles, gs_id *outSounds, size_t inSoundCount, const uint32 *inFlags)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::AllocateSound(size_t inFrameCount, gs_id *outSound,
	const gs_audio_format *inFormat, uint32 inFlags)
{
	return AllocateSounds(&inFrameCount, outSound, &inFormat, 1, &inFlags);
}

status_t 
BGameSoundDevice::AllocateSounds(const size_t *inFrameCounts, gs_id *outSounds, const gs_audio_format *inFormats, const uint32 *inFlags)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::CloneSound(gs_id inSound, gs_id *outSound, uint32 inFlags)
{
	return CloneSounds(&inSound, outSound, 1, &inFlags);
}

status_t 
BGameSoundDevice::CloneSounds(const gs_id *inSounds, gs_id *outSounds, size_t inSoundCount, const uint32 *inFlags)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::FreeSound(gs_id inSound, uint32 inFlags)
{
	return FreeSounds(&inSound, 1, &inFlags);
}

status_t 
BGameSoundDevice::FreeSounds(const gs_id *inSounds, size_t inSoundCount, const uint32 *inFlags)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::LockSound(gs_id inSound, size_t inRequestFirstFrame,
	size_t inRequestFrameCount, void **outBufferPtr1, size_t *outFrameCount1,
	void **outBufferPtr2, size_t *outFrameCount2, uint32 inFlags)
{
	return LockSounds(&inSound, &inRequestFirstFrame, &inRequestFrameCount, outBufferPtr1,
		outFrameCount1, outBufferPtr2, outFrameCount2, 1, &inFlags);
}

status_t 
BGameSoundDevice::LockSounds(const gs_id *inSounds, const size_t *inRequestFirstFrames, const size_t *inRequestFrameCounts, void **outBufferPtrs1, size_t *outFrameCounts1, void **outBufferPtrs2, size_t *outFrameCounts2, size_t inSoundCount, const uint32 *inFlags)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::UnlockSound(gs_id inSound, uint32 inFlags)
{
	return UnlockSound(&inSound, 1, &inFlags);
}

status_t 
BGameSoundDevice::UnlockSounds(const gs_id *inSounds, size_t inSoundCount, const uint32 *inFlags)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::StartSound(gs_id inSound, uint32 inFlags)
{
	return StartSounds(&inSound, 1, &inFlags);
}

status_t 
BGameSoundDevice::StartSounds(const gs_id *inSounds, size_t inSoundCount, const uint32 *inFlags)
{
	//fixme	implementation goes here
}

bool 
BGameSoundDevice::IsSoundPlaying(gs_id inSound)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::StopSound(gs_id inSound, uint32 inFlags)
{
	return StopSounds(&inSound, 1, &inFlags);
}

status_t 
BGameSoundDevice::StopSounds(const gs_id *inSounds, size_t inSoundCount, const uint32 *inFlags)
{
	//fixme	implementation goes here
}


status_t 
BGameSoundDevice::SetSoundGain(gs_id inSound, float inVolume, float ramp)
{
	gs_attribute attr, *attrp = &attr;
	attr.m_what = B_GS_GAIN_ATTRIBUTE;
	attr.m_value = inVolume;
	if (ramp > 0.0)
	{
		GetSoundAttributes(&inSound, &attrp, 1);
		attr.m_change = (inVolume - attr.m_value)/ramp;
		if (attr.m_what != B_GS_GAIN_ATTRIBUTE)
		{	//	broken implementation might not support getting volume?
			attr.m_what = B_GS_GAIN_ATTRIBUTE;
			attr.m_value = inVolume;
		}
	}
	else
		attr.m_change = 0.0;
	attr.m_limit = inVolume;
	return SetSoundAttributes(&inSound, &attrp, 1);
}

status_t 
BGameSoundDevice::SetSoundPan(gs_id inSound, float inPan, float ramp)
{
	gs_attribute attr, *attrp = &attr;
	attr.m_what = B_GS_PAN_ATTRIBUTE;
	attr.m_value = inPan;
	if (ramp > 0.0)
	{
		GetSoundAttributes(&inSound, &attrp, 1);
		attr.m_change = (inPan - attr.m_value)/ramp;
		if (attr.m_what != B_GS_PAN_ATTRIBUTE)
		{	//	broken implementation might not support getting volume?
			attr.m_what = B_GS_PAN_ATTRIBUTE;
			attr.m_value = inPan;
		}
	}
	else
		attr.m_change = 0.0;
	attr.m_limit = inPan;
	return SetSoundAttributes(&inSound, &attrp, 1);
}

status_t 
BGameSoundDevice::SetMainCallback(void( *hook)( void *inCookie, void *inBuffer, size_t inByteCount, BGameSoundDevice *me), void *cookie, uint32 inFlags)
{
	if (_fInitError < 0) return _fInitError;
	//fixme	*** implementation goes here
	return B_ERROR;
}

status_t 
BGameSoundDevice::GetSoundInfo(gs_id inSound, gs_info *outInfo, size_t inInfoSize)
{
	return GetSoundInfos(&inSound, outInfo, 1, inInfoSize);
}

status_t 
BGameSoundDevice::GetSoundInfos(const gs_id *inSounds, gs_info *outInfo, size_t inSoundCount, size_t inInfoSize)
{
	//fixme	implementation goes here
}

status_t
BGameSoundDevice::GetAllSoundAttributes(
	gs_id inSound,
	gs_attribute * ioAttributes,
	size_t * ioAttrCount)
{
	if (_fInitError < 0) return _fInitError;
	*ioAttrCount = 0;
	//fixme	*** implementation goes here
	return B_ERROR;
}

status_t 
BGameSoundDevice::GetSoundAttributes(gs_id inSound, gs_attribute *ioAttributes, size_t inAttributeCount)
{
	return GetSoundAttributes(&inSound, &ioAttributes, 1, inAttributeCount);
}

status_t 
BGameSoundDevice::GetSoundAttributes(const gs_id *inSounds, gs_attribute **ioAttributes, size_t inSoundCount, size_t inAttributeCount)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::SetSoundAttributes(gs_id inSound, gs_attribute *ioAttributes, size_t inAttributeCount)
{
	return SetSoundAttributes(&inSound, &ioAttributes, 1, inAttributeCount);
}

status_t 
BGameSoundDevice::SetSoundAttributes(const gs_id *inSound, gs_attribute **ioAttributes, size_t inSoundCount, size_t inAttributeCount)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::SetSoundCallback(gs_id inSound, void( *callback)( void *cookie, gs_id sound, size_t atFrame, BGameSoundDevice *me), void *cookie, uint32 inFlags)
{
	return SetSoundCallback(&inSound, callback, &cookie, 1, &inFlags);
}

status_t 
BGameSoundDevice::SetSoundCallbacks(const gs_id *inSound, void( *callback)( void *cookie, gs_id sound, size_t atFrame, BGameSoundDevice *me), void **cookies, size_t inSoundCount, const uint32 *inFlags)
{
	//fixme	implementation goes here
}


#if _ADVANCED
BView *
BGameSoundDevice::MakeSettingsView()
{
	return NULL;
}

status_t 
BGameSoundDevice::ApplySettings(BView *inFromView)
{
	if (_fInitError < 0) return _fInitError;
	return B_OK;
}

status_t 
BGameSoundDevice::SaveSettings(BMessage *ioSettings)
{
	if (_fInitError < 0) return _fInitError;
	return B_OK;
}

status_t 
BGameSoundDevice::RestoreSettings(const BMessage *inSettings)
{
	if (_fInitError < 0) return _fInitError;
	return B_OK;
}
#endif

#if _ADVANCED
status_t 
BGameSoundDevice::StartCDPlayer(int32 inTrack, bool inLoop)
{
	if (_fInitError < 0) return _fInitError;
	return B_ERROR;
}

status_t 
BGameSoundDevice::StopCDPlayer()
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::GetCDPlayerInfo(bool *outDetected, char *outVolumeName, size_t inVolumeNameSize, float *outMinGain, float *outMaxGain, int32 inIndex)
{
	return B_ERROR;
}

ssize_t
BGameSoundDevice::CountCDPlayers()
{
	return 0;
}

status_t 
BGameSoundDevice::SetCDPlayerGain(float inGain, int32 inIndex)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::GetCDPlayerGain(float *outGain, int32 inIndex)
{
	return B_ERROR;
}
#endif


void
BGameSoundDevice::SetInitError(
	status_t inInitError)
{
	_fInitError = inInitError;
}

BGameSoundDevice::BGameSoundDevice(const char *inDevice, const char * inVendor) :
	_fLock(inDevice)
{
	_fInitError = B_NO_MEMORY;
	_fDeviceStr = strdup(inDevice);
	_fVendorStr = strdup(inVendor);
	if ((_fDeviceStr != NULL) && (_fVendorStr != NULL)) _fInitError = B_OK;
	_fAddonInfo = NULL;
	_fSoundData = NULL;
	_fSoftSound = NULL;
	if (_fInitError == B_OK) _fInitError = alloc_structs();
}

status_t
BGameSoundDevice::MixMore(
	void * outData,
	size_t inByteCount,
	bigtime_t inPlayTime,
	const gs_audio_format * format)
{
	//fixme	implementation goes here
}

status_t 
BGameSoundDevice::Perform(int32 selector, void *data, size_t *ioSize)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_1(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_2(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_3(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_4(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_5(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_6(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_7(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_8(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_9(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_10(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_11(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_12(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_13(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_14(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_15(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundDevice::_Reserved_GameSoundDevice_16(void *, ...)
{
	return B_ERROR;
}


status_t
BGameSoundDevice::alloc_structs()
{
	_fSoundDataCount = SOUND_DATA_COUNT;
	_fSoundData = new (nothrow) sound_data[_fSoundDataCount];
	_fSoftSoundCount = SOFT_SOUND_COUNT;
	_fSoftSound = new (nothrow) soft_sound[_fSoftSoundCount];
	return ((_fSoundData == NULL) || (_fSoftSound == NULL)) ? B_NO_MEMORY : B_OK;
}

void
BGameSoundDevice::free_structs()
{
	for (int ix=0; ix<_fSoundDataCount; ix++)
	{
		if (_fSoundData[ix].area >= 0)
			delete_area(_fSoundData[ix].area);
	}
	delete _fSoundData;
	delete _fSoftSound;
}

BGameSoundDevice::sound_data *
BGameSoundDevice::find_free_sound_data()
{
	for (int ix=_fSoundDataCount-1; ix>=0; ix--)
		if (_fSoundData[ix].data == NULL)
			return &_fSoundData[ix];
	return NULL;
}

BGameSoundDevice::soft_sound *
BGameSoundDevice::find_free_soft_sound()
{
	for (int ix=_fSoftSoundCount-1; ix>=0; ix--)
		if (_fSoftSound[ix].data == NULL)
			return &_fSoftSound[ix];
	return NULL;
}

gs_id
BGameSoundDevice::sound_to_handle(soft_sound * sound)
{
	if (!sound) return -1;
	return MIN_HANDLE + (sound-_fSoftSound);
}

BGameSoundDevice::soft_sound *
BGameSoundDevice::handle_to_sound(gs_id handle)
{
	if (handle < MIN_HANDLE) return NULL;
	if (handle >= MIN_HANDLE + _fSoftSoundCount) return NULL;
	return &_fSoftSound[handle-MIN_HANDLE];
}

void
BGameSoundDevice::deallocate_sound_data(sound_data * data)
{
	if (--(data->refcount) == 0)
	{
		delete_area(data->area);
		data->area = -1;
		data->data = NULL;
	}
}

void
BGameSoundDevice::deallocate_soft_sound(soft_sound * sound)
{
	sound->data = NULL;
}

