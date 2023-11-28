/*****************************************************************************

	 File: MessageRunner.cpp

	 Written By: Peter Potrebic

     Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <Debug.h>
#include <string.h>
#include <Application.h>

#include <roster_private.h>

#ifndef _MESSAGE_RUNNER_H
#include <MessageRunner.h>
#endif

/*------------------------------------------------------------*/

BMessageRunner::BMessageRunner(BMessenger target, const BMessage *msg,
	bigtime_t interval, int32 count)
{
	InitData(target, msg, interval, count, be_app);
}

/*------------------------------------------------------------*/

BMessageRunner::BMessageRunner(BMessenger target, const BMessage *msg,
	bigtime_t interval, int32 count, BMessenger reply_to)
{
	InitData(target, msg, interval, count, reply_to);
}

/*------------------------------------------------------------*/

BMessageRunner::~BMessageRunner()
{
	if (fToken >= B_OK) {
		BMessage	rm(CMD_MSG_SCHEDULER);
		BMessage	reply;
		
		rm.AddInt32("code", CODE_TASK_REMOVE);
		rm.AddInt32("token", fToken);

		_send_to_roster_(&rm, &reply, false);
	}
}

/*------------------------------------------------------------*/

void BMessageRunner::InitData(BMessenger target, const BMessage *msg,
	bigtime_t interval, int32 count, BMessenger reply_to)
{
	BMessage	rm(CMD_MSG_SCHEDULER);
	BMessage	reply;
	
	rm.AddInt32("code", CODE_TASK_ADD);
	rm.AddMessenger("target", target);
	rm.AddMessenger("reply_to", reply_to);
	rm.AddInt64("interval", interval);
	rm.AddInt32("count", count);
	rm.AddMessage("message", msg);

	_send_to_roster_(&rm, &reply, false);

	if (reply.FindInt32("token", &fToken) != B_OK) {
		fToken = B_ERROR;
	}
}

/*------------------------------------------------------------*/

status_t BMessageRunner::InitCheck() const
{
	return fToken < B_OK ? fToken : B_OK;
}

/*------------------------------------------------------------*/

status_t BMessageRunner::GetInfo(bigtime_t *interval, int32 *count) const
{
	BMessage	rm(CMD_MSG_SCHEDULER);
	BMessage	reply;
	
	rm.AddInt32("code", CODE_TASK_INFO);
	rm.AddInt32("token", fToken);

	_send_to_roster_(&rm, &reply, false);

	status_t	err;
	status_t	e;
	if ((err = reply.FindInt32("error", &e)) == B_OK) {
		if (e) {
			err = e;
		} else {
			if ((e = reply.FindInt32("count", count)) != B_OK)
				err = e;
			if ((e = reply.FindInt64("interval", interval)) != B_OK)
				err = e;
		}
	}
	return err;
}

/*------------------------------------------------------------*/

status_t BMessageRunner::SetInterval(bigtime_t interval)
{
	return SetParams(true, interval, false, 0);
}

/*------------------------------------------------------------*/

status_t BMessageRunner::SetCount(int32 count)
{
	return SetParams(false, 0, true, count);
}

/*------------------------------------------------------------*/

status_t BMessageRunner::SetParams(bool reset_i, bigtime_t interval,
	bool reset_c, int32 count)
{
	BMessage	rm(CMD_MSG_SCHEDULER);
	BMessage	reply;
	
	rm.AddInt32("code", CODE_TASK_ADJUST);
	rm.AddInt32("token", fToken);
	if (reset_i)
		rm.AddInt64("interval", interval);
	if (reset_c)
		rm.AddInt32("count", count);

	_send_to_roster_(&rm, &reply, false);

	status_t	err;
	status_t	e;
	if ((e = reply.FindInt32("error", &err)) != B_OK) {
		err = e;
	}
	return err;
}


/*------------------------------------------------------------*/

BMessageRunner::BMessageRunner(const BMessageRunner &) {}
BMessageRunner &BMessageRunner::operator=(const BMessageRunner &)
	{ return *this; }

/*------------------------------------------------------------*/

void BMessageRunner::_ReservedMessageRunner1() {}
void BMessageRunner::_ReservedMessageRunner2() {}
void BMessageRunner::_ReservedMessageRunner3() {}
void BMessageRunner::_ReservedMessageRunner4() {}
void BMessageRunner::_ReservedMessageRunner5() {}
void BMessageRunner::_ReservedMessageRunner6() {}

