/* ++++++++++

   FILE:  PlaySound.cpp
   REVS:  $Revision: 1.14 $
   NAME:  r
   DATE:  Mon Jun 05 18:41:47 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#define DO_NOTHING(x...)

#if !NDEBUG
#define FPRINTF fprintf
#else
#define FPRINTF DO_NOTHING
#endif

#define ERRORS FPRINTF
#define PLAYSOUND DO_NOTHING //FPRINTF

#include <OS.h>
#include <AudioMsgs.h>
#include <Beeper.h>
//#include <Debug.h>
#include <PlaySound.h>
#include <trinity_p.h>
#include <stdio.h>
#include <string.h>


static bool verify_handle(int32 handle)
{
	sem_info sid;
	if (get_sem_info(handle, &sid)) {
		ERRORS(stderr, "verify_handle(%ld) failed\n", handle);
		return false;
	}
	if (strncmp(sid.name, "B_P:", 4)) {
		ERRORS(stderr, "verify_handle(%ld) saw wrong name\n", handle);
		return false;
	}
	return true;
}

static bool is_old_api()	//	cribbed from support/Beep.cpp
{
	static bool inited = false;
	static bool new_api = false;
	if (!inited) {
		const char * val = getenv("USE_OLD_AUDIO");
		if (!val || (strcasecmp(val, "true") && strcasecmp(val, "yes"))) {
			new_api = true;
		}
		else {
			new_api = false;
		}
		inited = true;
	}
	return !new_api;
}


/* ================
   play_sound() - Play a sound file, slightly longer form.
   ================ */

sound_handle play_sound(const entry_ref *soundRef,
						bool mix,
						bool queue,
						bool background)
{
	if (is_old_api()) {
		BBeeper *beeper = new BBeeper();
		sound_handle result = beeper->Run(soundRef, mix, queue, background, 1);
		if (result < 0 || !background)
			delete beeper;
		return result;
	}
	return play_sound(soundRef, background, 1.0);
}

sound_handle 
play_sound(const entry_ref *soundRef, bool background, float gain)
{
	BMessenger msgr("application/x-vnd.Be.addon-host");
	if (!msgr.IsValid()) {
		return B_MEDIA_SYSTEM_FAILURE;
	}
	BMessage msg(MEDIA_BEEP);
	msg.AddRef("be:file", soundRef);
	BMessage reply;
	msg.AddFloat("be:_volume", gain);
	msgr.SendMessage(&msg, &reply, DEFAULT_TIMEOUT, DEFAULT_TIMEOUT);
	status_t err = B_OK;
	int32 handle = 0;
	err = reply.FindInt32("be:play_token", &handle);
	reply.FindInt32("error", &err);
	if ((err >= B_OK) && !background) {
		err = wait_for_sound(handle);
	}
	PLAYSOUND(stderr, "handle: %d err: %d\n", handle, err);
	return err >= 0 ? handle : err;
}


/* stop_sound() - stop a sound already playing.
 *
 * BBeeper::BeepFn() will notice that the sem cound has increased
 * and wil stop playback if it does.
 */
status_t stop_sound(sound_handle handle)
{
	PLAYSOUND(stderr, "stop handle: %d\n", handle);
	if (!is_old_api() && !verify_handle(handle)) {
		return B_BAD_VALUE;
	}
	return release_sem(handle);
}

/* wait_for_sound() - wait until a sound has completed.
 *
 * BBeeper::_CompletionFn() will delete the semaphore, so any
 * attempts to block on it will give an error return.  But we
 * need to handle the case that someone else called stop_sound()
 * and bumped the sem count.  In this (rare) case, we snooze and 
 * try again.
 */
status_t wait_for_sound(sound_handle handle)
{
	status_t result;

	PLAYSOUND(stderr, "wait handle: %d\n", handle);
	if (!is_old_api() && !verify_handle(handle)) {
		return B_BAD_VALUE;
	}
	while ((result = acquire_sem(handle)) != B_BAD_SEM_ID) {
		if (result == B_INTERRUPTED) {
			continue;
		}
		release_sem(handle);
		snooze(100000);
	}
	return B_NO_ERROR;
}
