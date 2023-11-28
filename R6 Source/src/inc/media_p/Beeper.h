/* ++++++++++

   FILE:  Beeper.h
   REVS:  $Revision: 1.14 $
   NAME:  r
   DATE:  Mon Jun 05 20:14:28 PDT 1995

   Copyright (c) 1995-1997 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _BEEPER_H
#define _BEEPER_H

#include <Subscriber.h>
#include <AudioStream.h>
#include <ClassInfo.h>
#include <OS.h>
#include <SoundFile.h>
#include <StorageDefs.h>



/****************************************************************
 * Interface 
 */

class BBeeper {

public:

	BBeeper();
 	~BBeeper();

    /* all-in-one function to play the sound.  Returns a negative
	 * value for some error or the fSignal semaphore id on success.
	 */
    status_t	Run(const entry_ref *soundRef, bool mix, bool queue, 
					bool background, int32 repeatCount);

	/* ================
	   Private member functions.
	   ================ */
private:

	static bool			_BeepFn(void *, char *, uint32, void *);
	bool				BeepFn(char *data, uint32 size, void *header);
	int32				ReadFrames(char **destination, int32 FramesDesired);
	status_t				Loader();
  	static status_t			_back_beep(void *arg);
	friend status_t	_loader_(void *arg);
	/* ================
	   Private member slots
	   ================ */

	BDACStream			*fDACStream;
	BSubscriber			*fSubscriber;
	BSoundFile			*fSoundFile;	/* source of sound file bits */
 	sem_id				fSignal; 		/* 0=running, 1=dying, nxm=dead */
 
	bool				fMix;			/* true=>share with others */
	bool				fQueue;			/* true=>wait for access */
	bool				fBackground;	/* true=>play in separate thread */
	int32				fRepeatCount;	/* # of times to repeat */
	
	int32				fFrameSize;		/* cached from sound file */

	char				*fBuffer;		/* holding buffer for samples */
	int32				fBufSize;		/* size of fBuffer in frames */
	int32				fBufStart;		/* first inclusive index in fBuffer */
	int32				fBufHas;		/* frames in buffer */
	thread_id			fLoader;		/* buffer loading thread */
	sem_id				fBufNEmpty;		/* buffer n empty */
	sem_id				fBufNFull;		/* buffer n full */
	BLocker				fBufLock;		/* buffer lock */
	
	bool				fRunning;		/* true if currently running */
};

#endif			// #ifdef _BEEPER_H
