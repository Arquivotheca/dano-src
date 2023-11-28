/* ++++++++++

   FILE:  PlaySound.cpp
   REVS:  $Revision: 1.14 $
   NAME:  r
   DATE:  Mon Jun 05 18:41:47 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#if !NDEBUG
#define FPRINTF(x) fprintf x
#else
#define FPRINTF(x) (void)0
#endif

#define ERRORS(x) FPRINTF(x)
#define PLAYSOUND(x) //FPRINTF(x)

#include <OS.h>
#include <AudioMsgs.h>
#include <Beeper.h>
//#include <Debug.h>
#include <PlaySound.h>
#include <trinity_p.h>
#include <stdio.h>
#include <string.h>


struct play_sound_struct {
	entry_ref		ref;
	sem_id			sem;
	int32 *			clear_flag;
};
static status_t play_sound_imp(play_sound_struct * pss);

#if !_SUPPORTS_MEDIA_NODES
#include "support_misc.h"
#include "miniproto.h"
#include <MediaFile.h>
#include <MediaTrack.h>

#endif


static bool verify_handle(int32 handle)
{
	sem_info sid;
	if (get_sem_info(handle, &sid)) {
		ERRORS((stderr, "verify_handle(%d) failed\n", handle));
		return false;
	}
	if (strncmp(sid.name, "B_P:", 4)) {
		ERRORS((stderr, "verify_handle(%d) saw wrong name\n", handle));
		return false;
	}
	return true;
}

/* ================
   play_sound() - Play a sound file, slightly longer form.
   ================ */

sound_handle play_sound(const entry_ref *soundRef,
						bool mix,
						bool queue,
						bool background)
{
	play_sound_struct pss;
	pss.ref = *soundRef;
	pss.sem = -1;
	pss.clear_flag = 0;
	sound_handle handle = play_sound_imp(&pss);
	if ((handle >= B_OK) && !background) {
		handle = wait_for_sound(handle);
	}
	PLAYSOUND((stderr, "handle: %d\n", handle));
	return handle;
}

status_t play_sound_imp(play_sound_struct * pss)
{
#if _SUPPORTS_MEDIA_NODES
	BMessenger msgr("application/x-vnd.Be.addon-host");
	if (!msgr.IsValid()) {
		return B_MEDIA_SYSTEM_FAILURE;
	}
	BMessage msg(MEDIA_BEEP);
	msg.AddRef("be:file", &pss.ref);
	BMessage reply;
	msg.AddBool("be:mix", true);		//	ignored
	msg.AddBool("be:queue", false);	//	ignored
	msgr.SendMessage(&msg, &reply, DEFAULT_TIMEOUT, DEFAULT_TIMEOUT);
	status_t err = B_OK;
	int32 handle = 0;
	err = reply.FindInt32("be:play_token", &pss.sem);
	if (err == B_OK) reply.FindInt32("error", &err);
	return (err < 0) ? err : pss.sem;
#else
	BMessenger msgr("application/x-vnd.be.snd_server");
	if (!msgr.IsValid()) {
		return B_BAD_TEAM_ID;
	}
	BMessage msg(maStartFile);
	msg.AddRef("be:file", &pss->ref);
	status_t err;
	BMessage reply;
	if ((err = msgr.SendMessage(&msg, &reply, SND_TIMEOUT, SND_TIMEOUT)) < 0) {
		return err;
	}
	int32 handle = -1;
	if ((err = reply.FindInt32("be:handle", &handle)) < 0) {
		return err;
	}
	return handle;
#endif
}

/* stop_sound() - stop a sound already playing.
 *
 * BBeeper::BeepFn() will notice that the sem cound has increased
 * and wil stop playback if it does.
 */
status_t stop_sound(sound_handle handle)
{
	PLAYSOUND((stderr, "stop handle: %d\n", handle));
	if (!verify_handle(handle)) {
		ERRORS((stderr, "stop_sound(%d): bad handle\n", handle));
		return B_BAD_VALUE;
	}
	status_t result;
	while ((result = release_sem(handle)) != B_BAD_SEM_ID) {
		if (result == B_INTERRUPTED) {
			continue;
		}
		snooze(100000);
	}
	return B_OK;
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

	PLAYSOUND((stderr, "wait handle: %d\n", handle));
	if (!verify_handle(handle)) {
		ERRORS((stderr, "wait_for_sound(%d): bad handle\n", handle));
		return B_BAD_VALUE;
	}
	while ((result = acquire_sem(handle)) != B_BAD_SEM_ID) {
		if (result != B_INTERRUPTED) {
			release_sem(handle);
		}
		snooze(100000);
	}
	return B_NO_ERROR;
}


