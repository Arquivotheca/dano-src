
#include <stdio.h>

#include "StreamingGameSound.h"
#include "PrivGameSound.h"



BStreamingGameSound::BStreamingGameSound(size_t inBufferFrameCount, const gs_audio_format *format, size_t inBufferCount, BGameSoundDevice *device) :
	BGameSound(device),
	_m_lock("StreamingGameSound")
{
	_m_streamHook = NULL;
	_m_streamCookie = NULL;
	if (format == 0) {
		fprintf(stderr, "BStreamingGameSound: format is NULL\n");
		SetInitError(B_BAD_VALUE);
		return;
	}
	PrivGameSound * pgs = PrivGameSound::MakePlayer(*format);
	if (pgs == 0) {
		fprintf(stderr, "BStreamingGameSound: cannot create sound player\n");
		SetInitError(B_ERROR);
		return;
	}
	status_t err;
	if ((err = pgs->InitCheck()) != B_OK) {
		fprintf(stderr, "BStreamingGameSound: error opening sound player %lx\n", err);
		SetInitError(err);
		return;
	}
	gs_id gsh;
	size_t size = inBufferFrameCount*inBufferCount*frame_size_for(*format);
	err = pgs->MakeSound(*format, NULL, size, true, &gsh);	//	looping!
	if (err < B_OK) {
		fprintf(stderr, "BSimpleGameSound: cannot make sound handle\n");
		SetInitError(err);
		return;
	}
	err = pgs->SetCallback(gsh, stream_callback, this);
	if (err < B_OK) {
		fprintf(stderr, "BSimpleGameSound: cannot set stream callback\n");
		SetInitError(err);
		return;
	}
	(void) Init(gsh);
}

BStreamingGameSound::~BStreamingGameSound()
{
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (pgs != NULL) {
		pgs->StopSound(ID());
	}
}

BGameSound *
BStreamingGameSound::Clone() const
{
	fprintf(stderr, "BStreamingGameSound cannot be cloned.\n");
	return NULL;
}

status_t 
BStreamingGameSound::SetStreamHook(void( *hook)( void *inCookie, void *inBuffer, size_t inByteCount, BStreamingGameSound *me), void *cookie)
{
	_m_lock.Lock();
	_m_streamHook = hook;
	_m_streamCookie = cookie;
	_m_lock.Unlock();
	return B_OK;
}

void 
BStreamingGameSound::FillBuffer(void *inBuffer, size_t inByteCount)
{
	_m_lock.Lock();
	if (_m_streamHook) (*_m_streamHook)(_m_streamCookie, inBuffer, inByteCount, this);
	_m_lock.Unlock();
}


BStreamingGameSound::BStreamingGameSound(BGameSoundDevice * device) :
	BGameSound(device)
{
	_m_streamHook = NULL;
	_m_streamCookie = NULL;
}

status_t 
BStreamingGameSound::SetParameters(size_t inBufferFrameCount, const gs_audio_format *format, size_t inBufferCount)
{
	PrivGameSound * pgs = PrivGameSound::MakePlayer(*format);
	if (pgs == NULL) {
		return B_ERROR;
	}
	size_t size = frame_size_for(*format)*inBufferFrameCount*inBufferCount;
	gs_id gsh;
	status_t err = pgs->MakeSound(*format, NULL, size, true, &gsh);	//	streaming sounds always loop their buffer!
	if (err < B_OK) {
		return err;
	}
	err = pgs->SetCallback(gsh, stream_callback, this);
	if (err < B_OK) {
		return err;
	}
	return Init(gsh);
}

bool
BStreamingGameSound::Lock()
{
	return _m_lock.Lock();
}


void
BStreamingGameSound::Unlock()
{
	_m_lock.Unlock();
}


void
BStreamingGameSound::stream_callback(
	void * cookie,
	gs_id handle,
	void * buffer, 
	int32 offset,
	int32 size,
	size_t buf_size)
{
	int32 end = offset+size;
	if ((size_t)offset+size > buf_size) {
		end = buf_size;
	}
//	fprintf(stderr, "stream_callback(%d, %d, %d)\n", offset, size, buf_size);
	reinterpret_cast<BStreamingGameSound *>(cookie)->FillBuffer((char *)buffer+offset, end-offset);
	if (end < offset+size) {
		reinterpret_cast<BStreamingGameSound *>(cookie)->FillBuffer((char *)buffer, offset+size-end);
	}
}

status_t BStreamingGameSound::Perform(int32 selector, void * data)
{
	return BGameSound::Perform(selector, data);
}

status_t 
BStreamingGameSound::SetAttributes(gs_attribute *inAttributes, size_t inAttributeCount)
{
	if (!inAttributes) return B_BAD_VALUE;
	for (int ix=0; ix<inAttributeCount; ix++) {
		if (inAttributes[ix].attribute == B_GS_LOOPING) {
			inAttributes[ix].attribute = B_GS_NO_ATTRIBUTE;
		}
	}
	return BGameSound::SetAttributes(inAttributes, inAttributeCount);
}


status_t BStreamingGameSound::_Reserved_BStreamingGameSound_0(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_1(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_2(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_3(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_4(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_5(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_6(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_7(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_8(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_9(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_10(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_11(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_12(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_13(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_14(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_15(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_16(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_17(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_18(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_19(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_20(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_21(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_22(int32 arg, ...) { return B_ERROR; }
status_t BStreamingGameSound::_Reserved_BStreamingGameSound_23(int32 arg, ...) { return B_ERROR; }

/* that completes our FBC padding for BStreamingGameSound (BGameSound, vcount=24, dcount=24) */

