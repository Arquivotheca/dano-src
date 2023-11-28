//========================================================================
//	MBufferPlayer.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>
#include <Debug.h>

#include "MBufferPlayer.h"
#include "IDEConstants.h"

MBufferPlayer::MBufferPlayer(
	const void *data,
	int32 bufSize,
	int32 sampleSize,
	int32 sampleFormat,
	int32 byteOrder,
	int32 numChannels,
	int32 sampleRate) :
	BSubscriber("MBufferPlayer"),
	fSampleSize(sampleSize),
	fSampleFormat(sampleFormat),
	fByteOrder(byteOrder),
	fNumChannels(numChannels),
	fSampleRate(sampleRate),
	fData(data),
	fLength(bufSize),
	fPlayPos(0),
	fDone(true)
{
	fBlockSem = create_sem(1, "MBufferPlayer done");
}


MBufferPlayer::~MBufferPlayer()
{
	if (!fDone)
		StopThisMinute();
	delete_sem(fBlockSem);
}


status_t
MBufferPlayer::Play(
	bool wait,
	bool (*callback)(void *userData, int32 position),
	void *userData)
{
	status_t err = B_NO_ERROR;
	fPlayPos = 0;
	fUserCallback = callback;
	fUserData = userData;
	if (fDone) {
		fSubscriberID = B_SHARED_SUBSCRIBER_ID;

		err = Subscribe(&fOutPutStream);
		if (err)
			return err;
		err = fOutPutStream.SetSamplingRate(fSampleRate);
		if (err) {
			Unsubscribe();
			return err;
		}
		err = EnterStream(NULL, false, this, StreamFunc, CompleteFunc, true);
		if (err) {
			Unsubscribe();
			return err;
		}
		fDone = false;
		acquire_sem(fBlockSem);
	}
	if (wait)
		WaitUntilDone();
	return err;
}


bool
MBufferPlayer::Done()
{
	return fDone;
}


void
MBufferPlayer::WaitUntilDone()
{
	acquire_sem(fBlockSem);
	release_sem(fBlockSem);
}


void
MBufferPlayer::StopThisMinute()
{
	if (!fDone)
		ExitStream(false);
}


bool
MBufferPlayer::StreamFunc(
	void *data,
	char *buf,
	size_t size,
	void */*header*/)
{
	MBufferPlayer *play = (MBufferPlayer *)data;
	int32 leftSamples = (play->fLength - play->fPlayPos)/
		(play->fSampleSize*play->fNumChannels);
	if (leftSamples < 1) {
		return false;	//	Bye-bye!
	}
	int32 copySamples = size/(2*play->fNumChannels);
	if (copySamples > leftSamples) {
		copySamples = leftSamples;
	}
	Mix((char *)buf, copySamples, play->fNumChannels,
		((const char *)play->fData)+play->fPlayPos, play->fSampleSize, 
		play->fSampleFormat);
	play->fPlayPos += copySamples*play->fSampleSize*play->fNumChannels;

	if (play->fUserCallback) {
		return (*play->fUserCallback)(play->fUserData, play->fPlayPos);
	} else {
		return true;
	}
}


status_t
MBufferPlayer::CompleteFunc(
	void *data,
	status_t error)
{
	MBufferPlayer *play = (MBufferPlayer *)data;

	play->ExitStream(false);
	play->fDone = true;
	release_sem(play->fBlockSem);
	if (play->fUserCallback) {
		(*play->fUserCallback)(play->fUserData, -1L);
		play->fUserCallback = NULL;
		play->fUserData = NULL;
	}
	return error;
}


void
MBufferPlayer::Mix(
	char * out,		//	out is always 16 bit linear
	int32 numSamples,
	int32 numChannels,
	const char * in,
	int32 sampleSize,
	int32 sampleFormat)
{
	int32 toMix = numSamples * numChannels;
	short *dst = (short *)out;

	if ((sampleSize == 4) && (sampleFormat == B_FLOAT_SAMPLES)) {
		float *ptr = (float *)in;
		float fval = *ptr;
		int sval = *dst;
		while (toMix-- > 1) {
			register float wrt = sval + (fval * 32767.0);
			register int cvt = (wrt > 32767.0) ? 32767 : ((wrt < -32768.0) ? -32768 : (int) wrt);
			*dst = cvt;
			fval = *++ptr;
			sval = *++dst;
		}
		register float wrt = sval + (fval * 32767.0);
		register int cvt = (wrt > 32767.0) ? 32767 : ((wrt < -32768.0) ? -32768 : (int) wrt);
		*dst = cvt;
	} else if ((sampleSize == 2) && (sampleFormat == B_LINEAR_SAMPLES)) {
		short *ptr = (short *)in;
		int fval = *ptr;
		int sval = *dst;
		while (toMix-- > 1) {
			*dst = sval + fval;
			fval = *++ptr;
			sval = *++dst;
		}
		*dst = sval + fval;
	} else if ((sampleSize == 1) && (sampleFormat == B_LINEAR_SAMPLES)) {
		uchar *ptr = (uchar *)in;
		int fval = *ptr;
		int sval = *dst;
		while (toMix-- > 1) {
			*dst = sval + ((fval)<<8);
			fval = *++ptr;
			sval = *++dst;
		}
		*dst = sval + ((fval)<<8);
	} else {
		//	we don't know it - punt!
		ASSERT(!"Sorry, don't know the sample format");
	}
}

