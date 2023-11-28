/* ++++++++++
   
   FILE:  Beeper.cpp
   REVS:  $Revision: 1.33 $
   NAME:  r
   DATE:  Mon Jun 05 20:29:38 PDT 1995
   
   Copyright (c) 1995-1997 by Be Incorporated.  All Rights Reserved.
   
   About the fSignal semaphore:
   
   We use a semaphore as a means to "remotely" block until the sound
   has finished playing and to stop a sound that's in progress.  Here's
   how it works:
   
   The signal semaphore is initialized to a thread count of 0.
   
   The Run() function returns the semaphore id.
   
   In the inner playback loop, the thread count is examined.  If it has
   be increased above 0 or if the semaphore itself has been deleted, we
   stop processing samples.
   
   
   THEREFORE:
   
   To remotely stop the playback, increase the semaphore count.
   
   To block until playback has finished, acquire the semaphore id
   returned by Run().  When the acquire_sem() gives you a bad sem id,
   you'll know that the semaphore has been deleted (e.g. the BBeeper
   has deleted itself).  If you get unblocked but the semaphore exists,
   it's because someone else has requested a stop (see above).  You
   should re-release the semaphore, snooze for a bit, and try again.
   
   +++++ */

#include <R3MediaDefs.h>
#include <Beeper.h>
#include <byteorder.h>
#include <SupportDefs.h>

//#include <stdio.h>
//#define X(N) {printf("<%d> ",(N));fflush(stdout);}

#define BUFFER_SIZE		(12 * 8192) // expressed in FRAMES
#define ALTERNATE_SIZE	(2 * 8192) // expressed in FRAMES


BBeeper::BBeeper()
{
    fDACStream = new BDACStream();
	fSubscriber = new BSubscriber("Beeper");
	fSoundFile = new BSoundFile();
	fBuffer = NULL;
	fRunning = FALSE;
	fLoader = B_BAD_THREAD_ID;
	fSignal = B_BAD_SEM_ID;
	fBufNEmpty = B_BAD_SEM_ID;
	fBufNFull = B_BAD_SEM_ID;
}

BBeeper::~BBeeper()
{
	delete fSubscriber;
	delete fDACStream;
    kill_thread(fLoader);
	delete fSoundFile;
	delete_sem(fSignal);
	delete_sem(fBufNEmpty);
	delete_sem(fBufNFull);
	if (fBuffer)
	  free (fBuffer);
}

status_t BBeeper::Run(const entry_ref *soundRef, bool mix, bool queue, 
					  bool background, int32 repeatCount)
{
	status_t result;
	subscriber_id clique;

	if (fRunning)
	  return fSignal;
	
	fMix = mix;
	fQueue = queue;					// OBSOLETE
	fBackground = background;
	fRepeatCount = repeatCount;

	if ((result = fSoundFile->SetTo(soundRef, O_RDONLY)) < 0)
	  return result;

	if (fMix)
	  clique = B_SHARED_SUBSCRIBER_ID;
	else
	  clique = fSubscriber->ID();
	
	/* get access to the audio stream */
	if ((result = fSubscriber->Subscribe(fDACStream)) < 0)
	  return result;
	
	/* cache a useful value */
	fFrameSize = fSoundFile->FrameSize();
	
	/* Configure the audio stream to match the sound file.  For now,
	 * ignore any error return.
	 *
	 * ### Someday, we should re-write this to change the audio parameters 
	 * ### only when we're the exlcusive subscriber to the stream and 
	 * ### otherwise do format conversion to match what's already playing.
	 *
	 * ### This is no longer possible and requires a fix ###
	fSubscriber->SetDACSampleInfo(fSoundFile->SampleSize(),
									   fSoundFile->CountChannels(),
									   fSoundFile->ByteOrder(),
									   fSoundFile->SampleFormat());
	 */
	fDACStream->SetSamplingRate(fSoundFile->SamplingRate());
	
	/* allocate a "transfer buffer" -- all units in terms of frames */
	if (fBuffer)
	  free (fBuffer);
	fBufSize = BUFFER_SIZE;
	fBuffer = (char *)malloc(fBufSize * fFrameSize);
	if (fBuffer == NULL) {
	  /* if big buffer fails try small buffer */
	  fBufSize = ALTERNATE_SIZE;
	  fBuffer = (char *)malloc(fBufSize * fFrameSize);
	  if (fBuffer == NULL)
		return B_NO_MEMORY;
	}
	fBufStart = fBufHas = 0;
	fRepeatCount -= fRepeatCount;	/* file is already prepared for first repeat */
	
	fRunning = TRUE;
	
	delete_sem(fSignal);
	delete_sem(fBufNEmpty);
	delete_sem(fBufNFull);
	fSignal = create_sem(0, "Death by Beeping");
	fBufNEmpty = create_sem(fBufSize, "BBeeper empty frames");
	fBufNFull = create_sem(0, "BBeeper full frames");

	while (1) {
		fLoader = spawn_thread (_loader_, "BBeeper reader", B_DISPLAY_PRIORITY,
								(void*) this);
		if (fLoader != B_INTERRUPTED)
			break;
	}

	result = resume_thread(fLoader);

	if (result >= 0)
	  if (!fBackground)
		result = fSubscriber->EnterStream(NULL, /* enter stream */
										  FALSE, /* ...after everyone */
										  (void *)this,
										  _BeepFn,
										  NULL,
										  fBackground);
	  else {
		thread_id backThread;
		while (1) {
			backThread = spawn_thread((thread_entry)_back_beep,
									"Background beeper",
									B_REAL_TIME_PRIORITY,
									(void *)this);
			if (backThread != B_INTERRUPTED)
				break;
		}
		result = resume_thread(backThread);
	  }

	/* There's a race here--the thread could exit before
	 * the following is called.  But I doubt it ever will.
	 */
	
	if (result < 0) 
	  return result;
	return fSignal;
}

status_t BBeeper::_back_beep(void *arg)
{
	BBeeper *my_beeper = (BBeeper *)arg;

	my_beeper->fSubscriber->EnterStream(NULL, /* enter stream */
										FALSE, /* ...after everyone */
										(void *)my_beeper,
										my_beeper->_BeepFn,
										NULL,
										FALSE);
	delete my_beeper;
	return B_NO_ERROR;
}


	
/* ================================================================
   Private member functions.
   ================================================================ */

bool BBeeper::_BeepFn(void *userData, char *data, uint32 size, void *header)
{
	return ((BBeeper *)userData)->BeepFn(data, size, header);
}

/* Arrive here when we've received a buffer of data from the 
 * buffer stream.  Mix in samples from the input file, returning
 * FALSE when there are no more samples to process.
 */
bool BBeeper::BeepFn(char *data, uint32 size, void */*header*/)
{
	register int32 sample0;
	register int32 sample1;
	register int32 temp0;
	register int32 temp1;
	register int32 k8000 = 0x8000;
	char* src8;
	register int16* dst16 = (int16*) data;
	int32 stereo = (fSoundFile->CountChannels() == 2 ? 1 : 0);
	int32 sampleSize = fSoundFile->SampleSize();
	int32 signalCount;
	int32 framesGotten;

	/*
	 * Mix in samples from the sound file.
	 */

	for (int32 dstHas = size / 4; dstHas > 0; dstHas -= framesGotten) {
	  status_t err = get_sem_count(fSignal, &signalCount);
	  if (err != B_NO_ERROR		// Our signal semaphore has gone away
		  || signalCount > 0)	// someone has signaled us to quit
		return FALSE;

	  framesGotten = ReadFrames(&src8, dstHas);
	  if (framesGotten == 0)
		return FALSE;

	  char* srcEnd = &src8[framesGotten * fFrameSize];

	  if (sampleSize == 1) {						/* 8 bit */
		register int zero = 0;
		if (fSoundFile->FileFormat() == B_WAVE_FILE)
		  zero = 128;
		while (src8 < srcEnd) {
		  sample0 = dst16[0] + ((int32) (int8) (*src8 - zero) << 8);
		  src8 += stereo;
		  sample1 = dst16[1] + ((int32) (int8) (*src8++ - zero) << 8);
		  temp0 = (sample0 + k8000) & 0xFFFF0000;
		  temp1 = (sample1 + k8000) & 0xFFFF0000;
		  dst16[0] = sample0;
		  if (temp0)
			dst16[0] = (sample0 > 0 ? 0x7FFF : k8000);
		  dst16[1] = sample1;
		  if (temp1)
			dst16[1] = (sample1 > 0 ? 0x7FFF : k8000);
		  dst16 += 2;
		}
	  }
	  else {										/* 16 bit */
		register int16* src16 = (int16*) src8;
		if ((fSoundFile->ByteOrder() == B_LITTLE_ENDIAN)
			^ !!B_HOST_IS_LENDIAN)
		  while ((char*) src16 < srcEnd) {				/* AC */
			sample0 = (int32) *(uint16*) src16;
			sample0 = dst16[0] + (int32) (int16) ((sample0 << 8) | (sample0 >> 8));
			src16 += stereo;
			sample1 = (int32) *(uint16*) src16++;
			sample1 = dst16[0] + (int32) (int16) ((sample1 << 8) | (sample1 >> 8));
			temp0 = (sample0 + k8000) & 0xFFFF0000;
			temp1 = (sample1 + k8000) & 0xFFFF0000;
			dst16[0] = sample0;
			if (temp0)
			  dst16[0] = (sample0 > 0 ? 0x7FFF : k8000);
			dst16[1] = sample1;
			if (temp1)
			  dst16[1] = (sample1 > 0 ? 0x7FFF : k8000);
			dst16 += 2;
		  }
		else
		  while ((char*) src16 < srcEnd) {				/* DC */
			sample0 = dst16[0] + (int32) *src16;
			src16 += stereo;
			sample1 = dst16[1] + (int32) *src16++;
			temp0 = (sample0 + k8000) & 0xFFFF0000;
			temp1 = (sample1 + k8000) & 0xFFFF0000;
			dst16[0] = sample0;
			if (temp0)
			  dst16[0] = (sample0 > 0 ? 0x7FFF : k8000);
			dst16[1] = sample1;
			if (temp1)
			  dst16[1] = (sample1 > 0 ? 0x7FFF : k8000);
			dst16 += 2;
		  }
	  }
	}

	return TRUE;
}

/* Read upto FramesDesired frames from the input file, pointing
 * *destination to the samples.  Returns the number of frames
 * actually available in *destination, which may be less than
 * FramesDesired.  A return of 0 means that no more frames are
 * available.
 */

int32 BBeeper::ReadFrames(char **destination, int32 framesDesired)
{
	int32 framesGotten = 0;
	
	if (fRunning == FALSE)
	  return 0;
		
	while (acquire_sem_etc(fBufNFull, framesDesired, 0, 0) == B_INTERRUPTED)
		;
	fBufLock.Lock();
	if (fBufHas > 0) {
		/* Read from fBuffer and update fBufStart */	
		framesGotten = min_c(fBufHas, framesDesired);
		framesGotten = min_c(framesGotten, fBufSize - fBufStart);
		*destination = &fBuffer[fBufStart*fFrameSize];
		fBufStart += framesGotten;
		if (fBufStart >= fBufSize)
		  fBufStart = 0;
		fBufHas -= framesGotten;
	}
	fBufLock.Unlock();

	if (framesGotten > 0)
	  release_sem_etc(fBufNEmpty, framesGotten, 0);

	if (framesGotten < framesDesired)
	  release_sem_etc(fBufNFull, framesDesired - framesGotten, 0);

	return framesGotten;
}

status_t _loader_ (void* beeper)
{
  return ((BBeeper*) beeper)->Loader();
}

status_t BBeeper::Loader()
{
  bool bof = TRUE;
  bool eof = FALSE;
#if __GNUC__	/* WHAT THE HELL IS THIS? */
  int32 hacko;	/* to fool the compiler into not generating bad code */
#endif
  int32 trigger = min_c (fBufSize / 4, 32768 / fFrameSize);

  while (fRunning && !eof) {

	while (acquire_sem_etc(fBufNEmpty, trigger, 0, 0) == B_INTERRUPTED)
		;
	fBufLock.Lock();
#if __GNUC__
	hacko = fBufStart + fBufHas;
	int32 bufEnd = hacko % fBufSize;
#else
	int32 bufEnd = (fBufStart + fBufHas) % fBufSize;
#endif
	int32 request = min_c(fBufSize - bufEnd, fBufSize - fBufHas);
	if (!bof && fBufSize == BUFFER_SIZE)
	  request = min_c(request, 2 * trigger);
	bof = FALSE;
	fBufLock.Unlock();
	int32 framesRead = fSoundFile->ReadFrames(&fBuffer[bufEnd * fFrameSize],
											 request);
	if (framesRead > 0) {
	  fBufLock.Lock();
	  fBufHas += framesRead;
	  fBufLock.Unlock();
	}
	else if (framesRead < 0
			 || fRepeatCount <= 0
			 || fSoundFile->SeekToFrame(0) != B_NO_ERROR)
    {
	  /* error or eof */
	  framesRead = 0;
	  delete_sem(fBufNFull);
 	  fBufNFull = B_BAD_SEM_ID;
	  eof = TRUE;
	}
	else
	  --fRepeatCount;

	if (framesRead > 0)
	  release_sem_etc(fBufNFull, framesRead, 0);

	if (framesRead < trigger)
	  release_sem_etc(fBufNEmpty, trigger - framesRead, 0);
	else if (framesRead > trigger)
	  while (acquire_sem_etc(fBufNEmpty, framesRead - trigger, 0, 0) == B_INTERRUPTED)
		;
  }
  return B_NO_ERROR;
}	  
