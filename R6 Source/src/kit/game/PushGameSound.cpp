
#include <string.h>
#include <stdio.h>

#include <PushGameSound.h>

#include "PrivGameSound.h"


BPushGameSound::BPushGameSound(size_t inBufferFrameCount, const gs_audio_format *format, size_t inBufferCount, BGameSoundDevice *device) :
	BStreamingGameSound(inBufferFrameCount, format, inBufferCount, device)
{
//printf("PushGameSound(%d, %d, %.1f, 0x%x, %d, %d)\n", inBufferFrameCount, inBufferCount, format->frame_rate, format->format, format->channel_count, format->buffer_size);
	_m_benCount = inBufferCount;
	_m_benSem = create_sem(0, "PushGameSound::LockNextPage");
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (pgs == NULL) {
		fprintf(stderr, "PushGameSound: no player?\n");
		if (!InitCheck()) SetInitError(B_ERROR);
		return;
	}
	gs_audio_format fmt;
	status_t err = pgs->GetSoundInfo(ID(), &fmt, (void **)&_m_buffer, &_m_bufSize);
	if (err < B_OK) {
		fprintf(stderr, "PushGameSound: cannot get sound handle info: %x\n", err);
		SetInitError(err);
		return;
	}
	_m_pageSize = inBufferFrameCount*frame_size_for(fmt);
	_m_pageCount = inBufferCount;
	_m_lockedPtr = _m_buffer-_m_pageSize;	//	ooh, sneaky!
//	_m_playedPtr = _m_lockedPtr;
	_m_playedPtr = _m_buffer+fmt.buffer_size*2;
	if (_m_playedPtr >= _m_buffer+_m_bufSize) {
		fprintf(stderr, "PushGameSound alternate init\n");
		_m_playedPtr = _m_buffer+(_m_pageCount-2)*_m_pageSize;
	}
}


BPushGameSound::~BPushGameSound()
{
//printf("~BPushGameSound()\n");
	delete_sem(_m_benSem);
}

BGameSound *
BPushGameSound::Clone() const
{
	gs_audio_format fmt;
	void * buf;
	size_t size;
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (pgs == NULL) {
		fprintf(stderr, "BPushGameSound::Clone(): no player\n");
		return NULL;
	}
	status_t err = pgs->GetSoundInfo(ID(), &fmt, (void **)&buf, &size);
	if (err < B_OK) {
		fprintf(stderr, "BPushGameSound::Clone(): %s\n", strerror(err));
		return NULL;
	}
	return new BPushGameSound(_m_pageSize, &fmt, _m_pageCount, Device());
}

BPushGameSound::lock_status
BPushGameSound::LockNextPage(void **out_pagePtr, size_t *out_pageSize)
{
//printf("LockNextPage()\n");
	if (atomic_add(&_m_benCount, -1) < 1) acquire_sem(_m_benSem);
	if (!Lock()) {
		if (atomic_add(&_m_benCount, 1) < 0) release_sem(_m_benSem);
		return lock_failed;
	}
	char * newPtr = _m_lockedPtr + _m_pageSize;
	if (newPtr >= _m_buffer+_m_bufSize) newPtr -= _m_bufSize;
	if ((newPtr <= _m_playedPtr) && (newPtr + _m_pageSize >= _m_playedPtr)) {
		if (atomic_add(&_m_benCount, 1) < 0) release_sem(_m_benSem);
		Unlock();
		return lock_failed;
	}
	_m_lockedPtr = newPtr;
	if (out_pageSize != 0) *out_pageSize = _m_pageSize;
	if (out_pagePtr != NULL) *out_pagePtr = _m_lockedPtr;
	Unlock();
	return lock_ok;
}

status_t 
BPushGameSound::UnlockPage(void *in_pagePtr)
{
	int32 cnt;
	if ((cnt = atomic_add(&_m_benCount, 1)) < 0) release_sem(_m_benSem);
	if (cnt >= _m_pageCount) {
		fprintf(stderr, "BPushGameSound::UnlockPage called too many times!\n");
	}
	return B_OK;
}

BPushGameSound::lock_status
BPushGameSound::LockForCyclic(
	void ** out_basePtr,
	size_t * out_size)
{
//printf("LockForCyclic()\n");
	*out_basePtr = _m_buffer;
	*out_size = _m_bufSize;
	return lock_ok;
}

status_t
BPushGameSound::UnlockCyclic()
{
//printf("UnlockCyclic()\n");
	return B_OK;
}

size_t
BPushGameSound::CurrentPosition()
{
	ssize_t ret = (_m_playedPtr - _m_buffer)/frame_size_for(Format());
//printf("CurrentPosition() = %d\n", ret);
	return ret > 0 ? ret : 0;
}

status_t 
BPushGameSound::Perform(int32 selector, void *data)
{
	return BStreamingGameSound::Perform(selector, data);
}


status_t 
BPushGameSound::SetParameters(size_t inBufferFrameCount, const gs_audio_format *format, size_t inBufferCount)
{
	return B_UNSUPPORTED;
}

status_t 
BPushGameSound::SetStreamHook(void( *hook)( void *inCookie, void *inBuffer, size_t inByteCount, BStreamingGameSound *me), void *cookie)
{
	return B_UNSUPPORTED;
}

void 
BPushGameSound::FillBuffer(void *inBuffer, size_t inByteCount)
{
//printf("FillBuffer(%d)\n", (((char *)inBuffer)-_m_buffer)/frame_size_for(Format()));
	if (!Lock()) return;
	_m_playedPtr = (char *)inBuffer+inByteCount;
	if (_m_playedPtr >= _m_buffer+_m_bufSize) _m_playedPtr -= _m_bufSize;
	Unlock();
}


status_t 
BPushGameSound::_Reserved_BPushGameSound_0(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_1(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_2(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_3(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_4(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_5(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_6(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_7(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_8(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_9(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_10(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_11(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_12(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_13(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_14(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_15(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_16(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_17(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_18(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_19(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_20(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_21(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_22(int32 arg, ...)
{
	return B_ERROR;
}

status_t 
BPushGameSound::_Reserved_BPushGameSound_23(int32 arg, ...)
{
	return B_ERROR;
}

