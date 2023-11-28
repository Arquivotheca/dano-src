/*****************************************************************************

	 File: Invoker.cpp

	 Written By: Peter Potrebic

     Copyright (c) 1996-97 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <Debug.h>
#include <string.h>

#ifndef _INVOKER_H
#include <Invoker.h>
#endif
#include <Message.h>

#define _MAX_TIMEOUT_ 3600000000U

/*------------------------------------------------------------*/

BInvoker::BInvoker()
	: fMessenger()
{
	fMessage = NULL;
	fReplyTo = NULL;
	fTimeout = ULONG_MAX;
	fNotifyKind = 0;
}

/*------------------------------------------------------------*/

BInvoker::BInvoker(BMessage *msg, BMessenger target)
	: fMessenger(target)
{
	fMessage = msg;
	fReplyTo = NULL;
	fTimeout = ULONG_MAX;
	fNotifyKind = 0;
}

/*------------------------------------------------------------*/

BInvoker::BInvoker(BMessage *msg, const BHandler *handler,
	const BLooper *looper)
	: fMessenger(handler, looper)
{
	fMessage = msg;
	fReplyTo = NULL;
	fTimeout = ULONG_MAX;
	fNotifyKind = 0;
}

/*------------------------------------------------------------*/

BInvoker::~BInvoker()
{
	SetMessage(NULL);
}

/*------------------------------------------------------------*/

status_t	BInvoker::SetMessage(BMessage *message)
{
	if (fMessage == message)
		return B_OK;

	if (fMessage)
		delete fMessage;

	fMessage = message;

	return B_OK;
}

/*------------------------------------------------------------*/

BMessage	*BInvoker::Message() const
{
	return fMessage;
}

/*------------------------------------------------------------*/

ulong	BInvoker::Command() const
{
	if (fMessage)
		return fMessage->what;
	else
		return 0;
}

/* ---------------------------------------------------------------- */

status_t BInvoker::SetHandlerForReply(BHandler *reply_to)
{
	fReplyTo = reply_to;
	return B_NO_ERROR;
}

/* ---------------------------------------------------------------- */

BHandler *BInvoker::HandlerForReply() const
{
	return fReplyTo;
}

/* ---------------------------------------------------------------- */

status_t BInvoker::SetTarget(BMessenger target)
{
	fMessenger = target;
	return B_NO_ERROR;
}

/* ---------------------------------------------------------------- */

status_t BInvoker::SetTarget(const BHandler *target, const BLooper *looper)
{
	status_t	err = 0;
	fMessenger = BMessenger(target, looper, &err);
	return err;
}

/* ---------------------------------------------------------------- */

BHandler *BInvoker::Target(BLooper **looper) const
{
	return fMessenger.Target(looper);
}

/* ---------------------------------------------------------------- */

status_t	BInvoker::SetTimeout(bigtime_t timeout)
{
	// the timeout arg is in microseconds.
	if (timeout == B_INFINITE_TIMEOUT) {
		fTimeout = ULONG_MAX;
	} else if (timeout > _MAX_TIMEOUT_) {
		fTimeout = _MAX_TIMEOUT_;
	} else if (timeout < 0) {
		fTimeout = 0;
	} else {
		fTimeout = (uint32) timeout;
	}
	return B_OK;
}

/* ---------------------------------------------------------------- */

bigtime_t	BInvoker::Timeout() const
{
	bigtime_t	bt;
	if (fTimeout == ULONG_MAX)
		bt = B_INFINITE_TIMEOUT;
	else
		bt = (bigtime_t) fTimeout;

	return bt;
}

/* ---------------------------------------------------------------- */

BMessenger	BInvoker::Messenger() const
{
	return fMessenger;
}

/* ---------------------------------------------------------------- */

bool	BInvoker::IsTargetLocal() const
{
	return fMessenger.IsTargetLocal();
}

/* ---------------------------------------------------------------- */

status_t BInvoker::Invoke(BMessage *msg)
{
	if (!msg)
		msg = fMessage;
	if (!msg)
		return B_BAD_VALUE;

	return fMessenger.SendMessage(msg, fReplyTo, Timeout());
}

/* ---------------------------------------------------------------- */

status_t BInvoker::InvokeNotify(BMessage *msg, uint32 kind)
{
	if( fNotifyKind != 0 ) {
		// If we are already in an InvokeNotify() context, then bail on
		// this one.  This is to keep backwards compatibility with
		// ObjectSynth -- otherwise its text controls get into an endless
		// recursion.  However, it seems like a good idea, anyway...
		return B_WOULD_BLOCK;
	}
	BeginInvokeNotify(kind);
	status_t err = Invoke(msg);
	EndInvokeNotify();
	return err;
}

/* ---------------------------------------------------------------- */

uint32 BInvoker::InvokeKind(bool* notify)
{
	if( notify ) *notify = (fNotifyKind != 0) ? true : false;
	return (fNotifyKind != 0) ? fNotifyKind : B_CONTROL_INVOKED;
}

/* ---------------------------------------------------------------- */

void BInvoker::BeginInvokeNotify(uint32 kind)
{
	fNotifyKind = kind;
}

/* ---------------------------------------------------------------- */

void BInvoker::EndInvokeNotify()
{
	fNotifyKind = 0;
}

/*-------------------------------------------------------------*/

BInvoker::BInvoker(const BInvoker &) {}
BInvoker &BInvoker::operator=(const BInvoker &) { return *this; }

/*-------------------------------------------------------------*/

void BInvoker::_ReservedInvoker1() {}
void BInvoker::_ReservedInvoker2() {}
void BInvoker::_ReservedInvoker3() {}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
