/*****************************************************************************

     File: Clipboard.cpp

     $Revision: 1.46 $

	 Written by:		Peter Potrebic

     Copyright (c) 1994-96 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef _CLIPBOARD_H
#include <Clipboard.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _APP_DEFS_H
#include <AppDefs.h>
#endif
#ifndef _ROSTER_PRIVATE_H
#include <roster_private.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _AUTO_LOCK_H
#include <Autolock.h>
#endif

#include <Roster.h>

BClipboard	*be_clipboard;

/*---------------------------------------------------------------*/

BClipboard::BClipboard(const char *name, bool transient)
	: fLock(name), fClipHandler(), fDataSource()
{
	fCount = 0;
	if (name)
		fName = strdup(name);
	else
		fName = NULL;

	fData = new BMessage();
	BMessenger	tmp(ROSTER_MIME_SIG, -1, NULL);

	/*
	 Need to determine if this clipboard is being created by the roster
	 server itself?
	*/
	if (_find_cur_team_id_() != tmp.Team()) {
		// register a new clipboard with the roster
		BMessage	msg(CMD_NEW_CLIP);
		BMessage	reply;

		msg.AddString("name", name);
		msg.AddBool("transient", transient);

		tmp.SendMessage(&msg, &reply);
//+		SERIAL_PRINT(("asking for new clipboard (%s), error=%x\n",
//+			name, reply.FindInt32("error")));
			
		reply.FindMessenger("messenger", &fClipHandler);

		// If there is some bizzare error on the server side there's
		// nothing we can do. The clipboard will still work within an
		// app, it just won't work between applications.
		// In practive such an error shouldn't occur.
	}
	// else:
	// want an invalid messenger in this case. The roster should
	// never use a clipboard object
}

/*---------------------------------------------------------------*/

BClipboard::~BClipboard()
{
	if (fName)
		free(fName);
	if (fData)
		delete fData;
}

/*---------------------------------------------------------------*/

const char *BClipboard::Name() const
{
	return fName;
}

/*---------------------------------------------------------------*/

BMessage *BClipboard::Data() const
{
	if (!IsLocked()) {
		debugger("clipboard must be locked before accessing message\n");
		return NULL;
	}
	return fData;
}

/*---------------------------------------------------------------*/

status_t BClipboard::Revert()
{
	if (!AssertLocked())
		return B_ERROR;

	return DownloadFromSystem(true);
}

/*---------------------------------------------------------------*/

bool BClipboard::Lock()
{
	bool result = fLock.Lock();

	if (result)
		// each time the clipboard is locked import from global clipboard
		DownloadFromSystem();
	
	return result;
}

/*---------------------------------------------------------------*/

void BClipboard::Unlock()
{
	fLock.Unlock();
}

/*---------------------------------------------------------------*/

bool BClipboard::IsLocked() const
{
	return fLock.IsLocked();
}

/*---------------------------------------------------------------*/

bool BClipboard::AssertLocked() const
{
	if (!IsLocked()) {
		debugger("clipboard must be locked before proceeding\n");
		return false;
	} else
		return true;
}

/*---------------------------------------------------------------*/

uint32 BClipboard::LocalCount() const
{
	return fSystemCount;
}

/*---------------------------------------------------------------*/

uint32 BClipboard::SystemCount() const
{
	// can't use normal lock because we don't want to suck down the new
	// data!
	uint32 result = 0;
	
	BClipboard *THIS = const_cast<BClipboard*>(this);

	if (!THIS->fLock.Lock())
		return 0;

	// send a message to the roster asking it for the latest count
	BMessage	msg(CMD_GET_CLIP_COUNT);
	BMessage	reply;

	msg.AddString("name", fName);

	if (fClipHandler.SendMessage(&msg, &reply) == B_NO_ERROR) {
		int32	cur_count;
		if (reply.FindInt32("count", &cur_count) == B_OK) {
			result = (uint32) cur_count;
		}
	}
	THIS->fLock.Unlock();
	return result;
}

/*---------------------------------------------------------------*/

status_t BClipboard::StopWatching(BMessenger target)
{
	return be_roster->_StopWatching(BRoster::USE_GIVEN, &fClipHandler,
		CMD_CLIP_MONITOR, target);
}

/*---------------------------------------------------------------*/

status_t BClipboard::StartWatching(BMessenger target)
{
	return be_roster->_StartWatching(BRoster::USE_GIVEN, &fClipHandler,
		CMD_CLIP_MONITOR, target, 0xFFFFFFFF);
}

/*---------------------------------------------------------------*/

status_t BClipboard::Clear()
{
	if (!AssertLocked())
		return B_ERROR;
	
	fData->MakeEmpty();

	return B_NO_ERROR;
}

/*---------------------------------------------------------------*/

status_t BClipboard::DownloadFromSystem(bool force)
{
	if (!AssertLocked())
		return B_ERROR;

	status_t err = B_NO_ERROR;
	
	// send a message to the roster asking it for the latest clipboard
	// data for this clipboard.
	BMessage	msg(CMD_GET_CLIP_DATA);
	BMessage	reply;

	msg.AddString("name", fName);

	// tell the server what 'count' of the clipboard we have. It still might
	// be current. If it is then the server won't send us any data.
	msg.AddInt32("count", force ? (int32)-1 : fSystemCount);

	if ((err = fClipHandler.SendMessage(&msg, &reply)) == B_NO_ERROR) {
		if (reply.what != ROSTER_REPLY)
			err = B_ERROR;
		else if (reply.HasData("data", B_RAW_TYPE)) {
			int32 size;
			const char *data;
			reply.FindData("data", B_RAW_TYPE, (const void **)&data, &size);

			// the data is a flattened message
			err = fData->Unflatten(data);

			// now get the messenger that represents the 'owner' of the data
			// that is, the app that made this data
			err = reply.FindMessenger("owner", &fDataSource);

			// now update the system_count
			reply.FindInt32("count", (long*) &fSystemCount);
		}
		// otherwise the 'count' was still current to the clipboard
		// hasn't changed.
	}

	return err;
}

/*---------------------------------------------------------------*/

status_t BClipboard::UploadToSystem(const BMessenger& dataSource)
{
	if (!AssertLocked())
		return B_ERROR;
	
	// send a message to the roster giving it the latest clipboard data
	ASSERT(fClipHandler.IsValid());
	ASSERT(be_app);

	BMessage	msg(CMD_SET_CLIP_DATA);
	BMessage	reply;

	char	sbuf[2048];
	char	*dbuf = NULL;
	char	*buf;
	size_t	size = fData->FlattenedSize();

	// only allocate buffer in heap if it doesn't fit in stack buffer!
	if (size < sizeof(sbuf)) {
		buf = sbuf;
	} else {
		dbuf = (char *) malloc(size);
		buf = dbuf;
	}
	fData->Flatten(buf, size);

	fDataSource = dataSource;
	msg.AddString("name", fName);
	msg.AddData("data", B_RAW_TYPE, buf, size);
	msg.AddMessenger("owner", fDataSource);
	msg.AddString("clipboard", fName);

	if (dbuf)
		free(dbuf);

	status_t err = fClipHandler.SendMessage(&msg, &reply);

	if (err)
		return err;
	if (reply.what != ROSTER_REPLY)
		return B_ERROR;

//+	PRINT_OBJECT(reply);

	// get the system counter. Useful because it can save time when it
	// comes to Downloading the system clip. If count hasn't changed
	// then we don't have to send any data.
	reply.FindInt32("count", (long*) &fSystemCount);

	return B_NO_ERROR;
}

/*---------------------------------------------------------------*/

status_t BClipboard::Commit()
{
	return Commit(be_app_messenger);
}

/*---------------------------------------------------------------*/

status_t BClipboard::Commit(BMessenger dataSource)
{
	// The clipboard changed so propagate change to system clipboard

	if (!AssertLocked())
		return B_ERROR;
	
	fCount++;
//+	status_t err = 
		UploadToSystem(dataSource);
//+	PRINT(("Upload error = %x\n", err));

	return B_NO_ERROR;
}

/*---------------------------------------------------------------*/

BMessenger BClipboard::DataSource() const
{
	BAutolock auto_lock((BLocker*) &fLock);
	return fDataSource;
}

/*-------------------------------------------------------------*/

BClipboard::BClipboard(const BClipboard &) {}
BClipboard &BClipboard::operator=(const BClipboard &) { return *this; }

/* ---------------------------------------------------------------- */

void BClipboard::_ReservedClipboard1() {}
void BClipboard::_ReservedClipboard2() {}
void BClipboard::_ReservedClipboard3() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
