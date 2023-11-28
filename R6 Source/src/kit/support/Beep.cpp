/* ++++++++++

   FILE:  Beep.cpp
   REVS:  $Revision: 1.7 $
   NAME:  r
   DATE:  Mon Jun 05 18:41:47 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <Beep.h>
#include <Roster.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <MediaRoster.h>
#include <Messenger.h>
#include <Message.h>
#if _SUPPORTS_MEDIA_NODES
 #include <trinity_p.h>
#endif
#include <AudioMsgs.h>
#include <string.h>
#include <stdio.h>
#if !_SUPPORTS_MEDIA_NODES
 #include "support_misc.h"
 #include "miniproto.h"
#endif


#define FPRINTF(x) fprintf x
#define ERRORS(x) FPRINTF(x)
#define PLAYSOUND(x) //FPRINTF(x)


/* ================
   beep() - Play a beep, short form.
   ================ */

static bool is_old_api()
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

static status_t legacy_beep()
{
	int fd; /* try legacy beep (if available) */
	if ((fd = open("/dev/audio/pcbeep/1", O_WRONLY)) >= 0) {
		char data = 3;
		int result = write(fd, &data, 1);
		if (result > 0) {
			snooze(50000);	/* short, concise beep */
		}
		data = 0;
		result = write(fd, &data, 1);
		close(fd);
		if (result > B_OK) {
			result = B_OK;
		}
		return result;
	}
	return B_ERROR;
}

#if !_SUPPORTS_MEDIA_NODES
static void strip_spaces(const char * in, char * out)
{
	while (*in != 0) {
		if (*in != ' ') {
			*(out++) = *in;
		}
		in++;
	}
	*out = 0;
}
#endif

status_t system_beep(const char * event_name)
{
#if _SUPPORTS_MEDIA_NODES
	const char * signature = is_old_api() ? AUDIO_SERVER_ID :
		"application/x-vnd.Be.addon-host";
	BMessenger server(signature);
	int32 result = B_OK;
	if (!server.IsValid() && (!event_name || !strcmp(event_name, "Beep"))) {
		return legacy_beep();
	}
	if (is_old_api()) {
	 	BMessage msg(BEEP);
		BMessage reply;
		result = server.SendMessage(&msg, &reply, 200000, 100000);
		if (reply.what == ERROR_RETURN)
			reply.FindInt32("error", &result);
	}
	else {
		BMessage msg(MEDIA_BEEP);
		// add the beep "kind" string
		if (event_name != NULL) {
			msg.AddString("be:media_type", "Sounds");
			msg.AddString("be:media_name", event_name);
		}
		BMessage reply;
		result = server.SendMessage(&msg, &reply, 500000, 100000);
	
		if (result == B_NO_ERROR)
			if (reply.FindInt32("error", &result) || !result)
				reply.FindInt32("be:play_token", &result);
//		if (result < 0) {
//			goto manual_beep;
//		}
	}
	return result;
#else
	if (event_name == NULL) {
		event_name = "BeBeep";
	}
	char * eName = (char *)alloca(strlen(event_name)+1);
	strip_spaces(event_name, eName);
	PLAYSOUND((stderr, "event: '%s'\n", eName));
	bool errOK = (strcmp(eName, "BeBeep") != 0);	// for events other than BeBeep, it's OK not to beep
	BMessenger msgr("application/x-vnd.be.snd_server");
	status_t err;
	BMessage reply;
	int32 handle = -1;
	BMessage msg(maStartEvent);
	if (!msgr.IsValid()) {
		ERRORS((stderr, "snd_server msgr.IsValid() is false\n"));
		err = B_BAD_TEAM_ID;
		goto err0rz;
	}

	msg.AddString("be:event", eName);
	if (errOK)
		msgr.SendMessage(&msg, (BHandler*) 0, 0);
	else {
		if ((err = msgr.SendMessage(&msg, &reply, SND_TIMEOUT, SND_TIMEOUT)) < 0) {
			ERRORS((stderr, "SendMessage(): %s\n", strerror(err)));
			goto err0rz;
		}
		if (!reply.FindInt32("error", &err) && (err < 0)) {
			ERRORS((stderr, "reply error: %s\n", strerror(err)));
			goto err0rz;
		}
		if ((err = reply.FindInt32("be:handle", &handle)) < 0) {
			ERRORS((stderr, "be:handle: %s\n", strerror(err)));
			goto err0rz;
		}
		return handle;
	}

	return B_OK;

err0rz:
	if (!errOK) {
		err = legacy_beep();
	}
	return err;
#endif
}  


status_t
add_system_beep_event(
	const char * name,
	uint32)
{
#if _SUPPORTS_MEDIA_NODES
	status_t err = 0;
	BMessenger msgr("application/x-vnd.Be.media-server", -1, &err);
	if (!msgr.IsValid()) return (err < 0) ? err : B_BAD_TEAM_ID;
	BMessage msg(MEDIA_TYPE_ITEM_OP);
	msg.AddInt32("be:operation", _BMediaRosterP::B_OP_SET_ITEM);
	msg.AddString("be:type", "Sounds");
	msg.AddString("be:item", name);
	BMessage rep;
	err = msgr.SendMessage(&msg, &rep, 6000000LL, 6000000LL);
	if (err < B_OK) return err;
	rep.FindInt32("error", &err);
	return err;
#else
	return B_OK;
#endif
}

status_t beep()
{
	system_beep(NULL);
	return B_OK;
}

