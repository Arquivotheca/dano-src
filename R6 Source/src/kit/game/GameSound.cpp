
#include <SoundPlayer.h>
#include <MediaDefs.h>
#include <rt_alloc.h>

#include "GameSound.h"
#include "PrivGameSound.h"
#include "trinity_p.h"



//	BGameSound::SetMemoryPoolSize()
//	is implemented in PrivGameSound.cpp


	static bool
	is_heap_address(
		void * ptr)
	{
		bool heap = ((((ulong)ptr)&0xf0000000UL)==0x80000000UL);
#if DEBUG
		if (!heap) {
			fprintf(stderr, "guarded_rtm_free(): 0x%lx is not a heap address\n", (ulong)ptr);
		}
#endif
		return heap;
	}

static void
guarded_rtm_free(
	void * ptr)
{
	rtm_pool * pool = rtm_get_pool(get_game_pool(), ptr);
	if (pool == NULL) {
		if (is_heap_address(ptr)) {
			::operator delete(ptr);
		}
	}
	else {
		rtm_free(ptr);
	}
}

void *
BGameSound::operator new(
	size_t size)
{
	void * r = rtm_alloc(get_game_pool(), size);
#if _SUPPORTS_EXCEPTION_HANDLING
	if (!r) throw std::bad_alloc();
#endif
	return r;
}


void *
BGameSound::operator new(
	size_t size,
	const std::nothrow_t & t) throw()
{
#if _SUPPORTS_EXCEPTION_HANDLING
	try {
#endif
		return rtm_alloc(get_game_pool(), size);
#if _SUPPORTS_EXCEPTION_HANDLING
	}
	catch (...) {
		return NULL;
	}
#endif
}


void
BGameSound::operator delete(
	void * ptr)
{
	return guarded_rtm_free(ptr);
}


#if !__MWERKS__
void
BGameSound::operator delete(
	void * ptr, 
	const std::nothrow_t & t) throw()
{
#if _SUPPORTS_EXCEPTION_HANDLING
	try {
#endif
		guarded_rtm_free(ptr);
#if _SUPPORTS_EXCEPTION_HANDLING
	}
	catch (...) {
		//	nothing
	}
#endif
}
#endif // !__MWERKS__


BGameSound::BGameSound(BGameSoundDevice *device)
{
	_m_handle = B_GS_INVALID_SOUND;
	_m_device = 0;
	_m_format = *(gs_audio_format *)&media_raw_audio_format::wildcard;
	_m_initError = B_OK;
}


BGameSound::~BGameSound()
{
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (pgs == NULL) return;
	(void) pgs->ReleaseSound(_m_handle);
}

status_t
BGameSound::InitCheck() const
{
	return _m_initError;
}

BGameSoundDevice *
BGameSound::Device() const
{
	return _m_device;
}

gs_id 
BGameSound::ID() const
{
	return _m_handle;
}

const gs_audio_format &
BGameSound::Format() const
{
	return _m_format;
}

status_t 
BGameSound::StartPlaying()
{
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (!pgs) return B_ERROR;
	return pgs->StartSound(_m_handle);
}

bool 
BGameSound::IsPlaying()
{
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (!pgs) return false;
	return pgs->IsPlaying(_m_handle);
}

status_t 
BGameSound::StopPlaying()
{
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (!pgs) return B_ERROR;
	return pgs->StopSound(_m_handle);
}

status_t 
BGameSound::SetGain(float gain, bigtime_t duration)
{
	gs_attribute attr;
	attr.attribute = B_GS_GAIN;
	attr.duration = duration;
	attr.value = gain;
	attr.flags = 0;
	return SetAttributes(&attr, 1);
}

status_t 
BGameSound::SetPan(float pan, bigtime_t duration)
{
	gs_attribute attr;
	attr.attribute = B_GS_PAN;
	attr.duration = duration;
	attr.value = pan;
	attr.flags = 0;
	return SetAttributes(&attr, 1);
}

float 
BGameSound::Gain()
{
	gs_attribute attr;
	attr.attribute = B_GS_GAIN;
	status_t err = GetAttributes(&attr, 1);
	return err ? 1.0 : attr.value;
}

float 
BGameSound::Pan()
{
	gs_attribute attr;
	attr.attribute = B_GS_PAN;
	status_t err = GetAttributes(&attr, 1);
	return err ? 0.0 : attr.value;
}

status_t 
BGameSound::SetAttributes(gs_attribute *inAttributes, size_t inAttributeCount)
{
	status_t err = B_OK;
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (!pgs) return B_ERROR;
	for (int ix=0; ix<inAttributeCount; ix++) {
		status_t e = B_OK;
		switch (inAttributes[ix].attribute) {
		case B_GS_MAIN_GAIN: {
				//	ramp not supported for main gain
				float f = inAttributes->value;
				if (f < 0.0) f = 0.0;
				if (f > 1.0) f = 1.0;
				(void)pgs->SetVolume(f);
			}
			break;
		case B_GS_GAIN: {
				e = pgs->SetSoundGain(_m_handle, inAttributes[ix].value, inAttributes[ix].duration/1000000.0);
			}
			break;
		case B_GS_PAN: {
				e = pgs->SetSoundPan(_m_handle, inAttributes[ix].value, inAttributes[ix].duration/1000000.0);
			}
			break;
		case B_GS_SAMPLING_RATE: {
				e = pgs->SetSoundSamplingRate(_m_handle, inAttributes[ix].value, inAttributes[ix].duration/1000000.0);
			}
			break;
		case B_GS_LOOPING: {
				e = pgs->SetSoundLooping(_m_handle, inAttributes[ix].value > 0.0);
			}
			break;
		default:
			e = B_BAD_VALUE;
			break;
		}
		if (!err) err = e;
	}
	return err;
}

status_t 
BGameSound::GetAttributes(gs_attribute *outAttributes, size_t inAttributeCount)
{
	status_t err = B_OK;
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (!pgs) return B_ERROR;
	for (int ix=0; ix<inAttributeCount; ix++) {
		status_t e = B_OK;
		switch (outAttributes[ix].attribute) {
		case B_GS_GAIN: {
				float d;
				e = pgs->GetSoundGain(_m_handle, &outAttributes[ix].value, &d);
				outAttributes[ix].duration = (bigtime_t)(d * (double)1000000.0);
			}
			break;
		case B_GS_PAN: {
				float d;
				e = pgs->GetSoundPan(_m_handle, &outAttributes[ix].value, &d);
				outAttributes[ix].duration = (bigtime_t)(d * (double)1000000.0);
			}
			break;
		case B_GS_SAMPLING_RATE: {
				float d;
				e = pgs->GetSoundSamplingRate(_m_handle, &outAttributes[ix].value, &d);
				outAttributes[ix].duration = bigtime_t(d * (double)1000000.0);
			}
			break;
		case B_GS_LOOPING: {
				bool b = false;
				e = pgs->GetSoundLooping(_m_handle, &b);
				outAttributes[ix].value = (b ? 1.0 : 0.0);
			}
			break;
		default:
			e = B_BAD_VALUE;
			break;
		}
		if (!err) err = e;
	}
	return err;
}

status_t
BGameSound::SetInitError(
	status_t in_initError)
{
	status_t oldErr = _m_initError;
	if ((oldErr >= 0) || (in_initError < 0))
		_m_initError = in_initError;
	return _m_initError;
}

status_t 
BGameSound::Init(gs_id handle)
{
	_m_handle = handle;
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (pgs == NULL) return B_ERROR;
	return pgs->GetSoundInfo(handle, &_m_format, NULL, NULL);
}


BGameSound::BGameSound(const BGameSound &other)
{
	_m_handle = 0;
	_m_device = other._m_device;
	_m_format = other._m_format;
	_m_initError = other._m_initError;
}

BGameSound &
BGameSound::operator=(const BGameSound &other)
{
	_m_handle = B_GS_INVALID_SOUND;
	_m_device = other._m_device;
	_m_format = other._m_format;
	_m_initError = other._m_initError;
	return *this;
}

status_t BGameSound::Perform(int32 selector, void * data)
{
	return B_ERROR;
}

status_t BGameSound::_Reserved_BGameSound_0(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_1(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_2(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_3(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_4(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_5(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_6(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_7(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_8(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_9(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_10(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_11(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_12(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_13(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_14(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_15(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_16(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_17(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_18(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_19(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_20(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_21(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_22(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_23(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_24(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_25(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_26(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_27(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_28(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_29(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_30(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_31(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_32(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_33(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_34(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_35(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_36(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_37(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_38(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_39(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_40(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_41(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_42(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_43(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_44(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_45(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_46(int32 arg, ...) { return B_ERROR; }
status_t BGameSound::_Reserved_BGameSound_47(int32 arg, ...) { return B_ERROR; }

/* that completes our FBC padding for BGameSound (base class, vcount=48, dcount=32) */

