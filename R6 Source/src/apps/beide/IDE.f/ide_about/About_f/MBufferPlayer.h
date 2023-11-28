//========================================================================
//	MBufferPlayer.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Initialize it with sound data to be played
//	Call Play() to enter audio stream (shared), 
//	play sound and exit stream.
//	Call Play(false) to just get the ball rolling,
//	and call Done() to test for done-ness.
//	Or call WaitUntilDone() which is more efficient
//	than polling Done(), but blocks.

#ifndef _MBUFFERPLAYER_H
#define _MBUFFERPLAYER_H

#include <AudioStream.h>
#include <OS.h>
#include <Subscriber.h>

class MBufferPlayer : public BSubscriber
{
		BDACStream				fOutPutStream;
		subscriber_id			fSubscriberID;
		const void *			fData;
		int32					fLength;
		int32					fPlayPos;

		int32					fSampleFormat;
		int32					fSampleSize;
		int32					fByteOrder;
		int32					fNumChannels;
		int32					fSampleRate;
		sem_id					fBlockSem;
		void *					fUserData;

		bool					(*fUserCallback)(
									void *userData,
									int32 position);
		bool					fDone;


static	bool					StreamFunc(
									void *data,
									char *buffer,
									size_t count,
									void *header);
static	status_t				CompleteFunc(
									void *data,
									status_t error);

static	void					Mix(
									char * out,		//	out is always 16 bit linear
									int32 numSamples,
									int32 numChannels,
									const char * in,
									int32 sampleSize,
									int32 sampleFormat);

public:
				//	MBufferPlayer just "borrows" the pointer;
				//	it's up to you to ensure that it's valid 
				//	as long as MBufferPlayer is playing, and 
				//	to dispose of it once it's done.

								MBufferPlayer(
									const void *buffer,
									int32 bufSize,
									int32 sampleSize = 2,
									int32 sampleFormat = B_LINEAR_SAMPLES,
									int32 byteOrder = B_BIG_ENDIAN,
									int32 numChannels = 1,
									int32 sampleRate = 44100);
								~MBufferPlayer();

		status_t				Play(
									bool waitForDone = true,
									bool (*callback)(
										void *userData,
										int32 position) = NULL,
									void *userData = NULL);
		bool					Done();
		void					WaitUntilDone();
		void					StopThisMinute();
};

#endif
