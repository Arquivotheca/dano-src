//******************************************************************************
//
//	File:			Messenger.cpp
//
//	Description:	BMessenger. Remove Application connection
//
//	Written by:		Peter Potrebic & Benoit Schillings
//
//	Copyright 1992-96, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#include <string.h>

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _ROSTER_PRIVATE_H
#include <roster_private.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _MESSAGES_H
#include <messages.h>
#endif
#ifndef _TOKEN_SPACE_H
#include <token.h>
#endif
#ifndef _LIST_H
#include <List.h>
#endif
#ifndef _SESSION_H
#include <session.h>
#endif
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _APPLICATION_H
#include "Application.h"
#endif
#ifndef _MESSENGER_H
#include "Messenger.h"
#endif
#ifndef _ROSTER_H
#include "Roster.h"
#endif

#include <CallStack.h>

/*---------------------------------------------------------------*/

BMessenger::BMessenger(const char *mime_sig, team_id team, status_t *perr)
{
	InitData(mime_sig, team, perr);
}

/*---------------------------------------------------------------*/

void BMessenger::InitData(const char *mime_sig, team_id team, status_t *perr)
{
	app_info	a_info;
	bool		exempt = FALSE;
	status_t	error;

	fPort = -1;
	fHandlerToken = NO_TOKEN;
	fPreferredTarget = FALSE;
	fTeam = -1;

	if ((team == -1) && !mime_sig) {
		if (perr)
			*perr = B_BAD_TYPE;
		return;
	}

	if (mime_sig && strcasecmp(mime_sig, ROSTER_MIME_SIG) == 0) {
		thread_id	thread = find_thread(ROSTER_THREAD_NAME);
		thread_info	tinfo;
		tinfo.team = -1;
		error = get_thread_info(thread, &tinfo);
		if (error == B_NO_ERROR) {
			fTeam = tinfo.team;
			fPort = find_port(ROSTER_PORT_NAME);
			if (fPort < 0)
				error = fPort;
		}
		exempt = TRUE;
#if DEBUG
		if (error) {
			BSer << "BMessenger::init_data():" << endl;
			SERIAL_PRINT(("ERROR - no messenger to roster: port=%ld, team=%ld, thread=%ld, err=0x%lx (%s))\n",
				fPort, fTeam, thread, error, strerror(error)));
			BCallStack stack;
			stack.Update(1);
			stack.LongPrint(BSer);
		}
#endif
	} else if (team == -1) {
		team = be_roster->TeamFor(mime_sig);
		if (team != -1) {
			error = be_roster->GetRunningAppInfo(team, &a_info);
			if (error == B_NO_ERROR) {
				fPort = a_info.port;
				fTeam = a_info.team;
			}
		} else
			error = B_BAD_VALUE;
	} else if (team != -1) {
		if ((error = be_roster->GetRunningAppInfo(team, &a_info)) ==
					B_NO_ERROR) {

			// make sure the specified team matches the sig

			/*
			If mime_sig is NULL then user wants a messenger based on team ONLY.
			This might occur because an app has been launched and registered
			but didn't have an APPI resource to supply a sig and so was
			registered without a sig. Since it was registered, however,
			then it will have a messaging port and is able to start
			buffering messages for eventual handling by it's Run loop.
			*/

			// make sure the specified team matches the sig
			if (mime_sig && (strcasecmp(mime_sig, a_info.signature) != 0))
				error = B_MISMATCHED_VALUES;
			else {
				fPort = a_info.port;
				fTeam = a_info.team;
			}
		} else {
			error = B_BAD_TEAM_ID;
		}
	} else {
		error = B_BAD_VALUE;
	}

	// if we found a registered app then make sure it is not B_ARGV_ONLY so
	// that it will respond if sent a synchronous message
	if (!exempt && (error == B_NO_ERROR) && (a_info.flags & B_ARGV_ONLY)) {
		error = B_BAD_TYPE;
		fPort = -1;
	}
	if (perr)
		*perr = error;
}

/*-------------------------------------------------------------*/

BMessenger::BMessenger()
{
	fPort = -1;
	fTeam = -1;
	fHandlerToken = NO_TOKEN;
	fPreferredTarget = FALSE;
}

/*-------------------------------------------------------------*/

BMessenger::BMessenger(team_id team, port_id port, int32 token, bool pref)
{
	fPort = port;
	fTeam = team;
	fHandlerToken = token;
	fPreferredTarget = pref;
}

/*-------------------------------------------------------------*/

BMessenger::BMessenger(const BHandler *r, const BLooper *looper, status_t *perr)
{
	const BLooper	*l = 0;
	int32			token = 0;
	status_t		error = B_NO_ERROR;

	fPreferredTarget = FALSE;
	fTeam = -1;

	if (!r && !looper) {
		fPort = fTeam = -1;
		fHandlerToken = NO_TOKEN;
		if (perr)
			*perr = B_BAD_VALUE;
		return;
	}

	// ??? Have assume that dereferencing the handler 'r' is OK. Caller
	// must ensure that pointer will remain valid for duration of constructor.
	BLooper *rl = r ? r->Looper() : NULL;
	if ((r && !rl) || ((r && looper) && (rl != looper))) {
		fPort = fTeam = -1;
		fHandlerToken = NO_TOKEN;
		if (perr)
			*perr = B_MISMATCHED_VALUES;
		return;
	}

	// do non critical (safety-wise) things
	if (r) {
		l = rl;
		ASSERT(l);
		token = _get_object_token_(r);
	} else if (looper) {
		l = looper;
		token = _get_object_token_(l);
		fPreferredTarget = TRUE;
	}

	if (l) {
		fHandlerToken = token;
		error = B_NO_ERROR;

		/*
		 Need to use extreme caution when dealing with the looper object.
		 Use BLooper's static sLooperList to validate the the looper is
		 still alive and kicking.
		*/
		if (!BLooper::sLooperListLock.Lock()) {
			goto done;
		}
		if (BLooper::IsLooperValid(l)) {
			fPort = l->fMsgPort;
			fTeam = l->Team();
		} else {
			error = B_BAD_HANDLER;
			fPort = fTeam = -1;
			fHandlerToken = NO_TOKEN;
		}
		BLooper::sLooperListLock.Unlock();
	} else {
done:
		fPort = fTeam = -1;
		fHandlerToken = NO_TOKEN;
		error = B_BAD_HANDLER;
	}

	if (perr)
		*perr = error;
}

/*-------------------------------------------------------------*/

BMessenger::BMessenger(const BMessenger &m)
{
	fPort = m.fPort;
	fTeam = m.fTeam;
	fHandlerToken = m.fHandlerToken;
	fPreferredTarget = m.fPreferredTarget;
}

/*-------------------------------------------------------------*/

bool BMessenger::operator==(const BMessenger &m) const
{
	// only look at important fields. Don't need to worry about fTeam
	return ((fPort == m.fPort) && (fHandlerToken == m.fHandlerToken) &&
		(fPreferredTarget == m.fPreferredTarget));
}

/*-------------------------------------------------------------*/

bool operator<(const BMessenger & a, const BMessenger & b)
{
	if (a.fPort != b.fPort) return a.fPort < b.fPort;
	if (a.fHandlerToken != b.fHandlerToken) return a.fHandlerToken < b.fHandlerToken;
	if (a.fPreferredTarget != b.fPreferredTarget) return b.fPreferredTarget;
	return false;
}

/*-------------------------------------------------------------*/

bool operator!=(const BMessenger & a, const BMessenger & b)
{
	return !(a == b);
}

/*-------------------------------------------------------------*/

BMessenger &BMessenger::operator=(const BMessenger &m)
{
	// dont really need to check for (this == &m). It isn't
	// dangerous

	fPort = m.fPort;
	fTeam = m.fTeam;
	fHandlerToken = m.fHandlerToken;
	fPreferredTarget = m.fPreferredTarget;

	return *this;
}

/*-------------------------------------------------------------*/

BMessenger::~BMessenger()
{
}

/*-------------------------------------------------------------*/

bool BMessenger::IsTargetLocal() const
{
	if (fTeam == -1)
		return FALSE;

	return (fTeam == _find_cur_team_id_());
}

/*-------------------------------------------------------------*/

BHandler *BMessenger::Target(BLooper **plooper) const
{
	if (plooper)
		*plooper = NULL;

	if (fHandlerToken == NO_TOKEN)
		return NULL;

	if (!IsTargetLocal())
		return NULL;

	BHandler	*handler = NULL;
	BLooper		*loop = NULL;

	// get a pointer to the looper by using the fMsgPort

	// don't lock the looper. looper the private looper_list instead.
	// This will avoid deadlocks.
	if (!BLooper::sLooperListLock.Lock())
		return NULL;

	loop = BLooper::LooperForPort(fPort);
	if (loop) {
		gDefaultTokens->GetToken(fHandlerToken, HANDLER_TOKEN_TYPE, (void **) &handler);
		if (plooper)
			// we have the looper, return it even if the
			// handler is no longer valid - this is needed to
			// be able to unlock the messenger target
			*plooper = loop;
		if (handler) {
			// We've got both the handler
			if (fPreferredTarget)
				// In this case the handler must actually be a looper
				handler = NULL;

		} else {
			// The handler was deleted or removed so the messenger is no
			// longer valid.
			handler = NULL;
		}
	}
	BLooper::sLooperListLock.Unlock();

	return handler;
}

/*-------------------------------------------------------------*/

bool	BMessenger::LockTarget() const
{
	status_t err = LockTargetWithTimeout(B_INFINITE_TIMEOUT);
	return (err == B_OK);
}

/*-------------------------------------------------------------*/

status_t	BMessenger::LockTargetWithTimeout(bigtime_t timeout) const
{
	// This will naturally fail if the looper lives in another address
	// space. In that case fPort will not map to a looper in this team
	// so the lock will fail.
	return BLooper::_Lock(NULL, fPort, timeout, 0);
}

/*-------------------------------------------------------------*/

status_t	BMessenger::SendMessageAtTime(const BMessage& msg, bigtime_t when,
	const BMessenger& reply_to, uint32 flags, bigtime_t timeout) const
{
	if (fPort == -1)
		return B_BAD_PORT_ID;

//+	SERIAL_PRINT(("SendMessage(%.4s) (async)\n", (char *) &(a_message->what)));

	// Make sure timestamp is not before current time.  This keeps the caller
	// from causing live locks.
	const bigtime_t sysTime = system_time();
	if (when < sysTime)
		when = sysTime;
	
	// Try to find a direct target object for whoever the message is being
	// sent to.
	BDirectMessageTarget* direct = NULL;
	if (_find_cur_team_id_() == fTeam)
		direct = gDefaultTokens->TokenTarget(fHandlerToken);
	
	status_t error;

	if (msg.WasDelivered()) {
		// we're forwarding a message...

		// pass along to reply info as the reply info for the new msg.
		BMessenger mm(msg.ReturnAddress());

		const bool waiting = msg.IsSourceWaiting();
		
		if (waiting) {
			// forwarding a message that is expecting a synchronous reply
			// should deliver immediately
			when = sysTime;
			// and need to pass along ownership of the tmp_port
			port_info	pinfo;
			status_t e = get_port_info(fPort, &pinfo);
			if (e == B_NO_ERROR) {
				set_port_owner(mm.fPort, pinfo.team);
				
				#if DEBUG
				port_info pi;
				pi.name[0] = 0;
				get_port_info(mm.fPort, &pi);
				BSer << "Transfering ownership of port "
					 << mm.fPort << " (" << pi.name
					 << ") to team " << pinfo.team << endl;
				if (strncmp(pi.name, "tmp_rport", 9) != 0) {
					BCallStack stack;
					stack.Update(1);
					stack.LongPrint(BSer);
					BSer << "Sending message: " << *msg << endl;
					BSer << "(Target flags = "
						 << (void*)((message_target*)msg->fTarget)->flags
						 << ")" << endl;
				}
				#endif
			}
		}

		error = msg.send_asynchronous(direct, when, fPort, fHandlerToken, mm,
										(flags&~(MTF_PREFERRED_TARGET
												|MTF_REPLY_REQUIRED
												|MTF_SOURCE_IS_WAITING))
										| (fPreferredTarget ? MTF_PREFERRED_TARGET : 0)
										| (waiting
											? MTF_SOURCE_IS_WAITING
											: (msg.IsReplyRequested()
												? MTF_REPLY_REQUIRED
												: 0)),
										timeout);

		// This message is no longer responsible for the reply.
		if (error == B_OK) {
			((message_target*)msg.fTarget)->flags |= MTF_REPLY_SENT;
		}

	} else {
		/*
		 This message is being sent for the first time. Establish
		 the 'return' address based on the 'reply_to'
		*/

		// if no return address was given use the 'be_app'
		BMessenger mm(reply_to.IsValid() ? reply_to : be_app_messenger);

		error = msg.send_asynchronous(direct, when, fPort, fHandlerToken, mm,
										(flags&~(MTF_PREFERRED_TARGET))
										| (fPreferredTarget ? MTF_PREFERRED_TARGET : 0),
										timeout);
	}

	if (direct)
		direct->ReleaseTarget();
	
	return error;
}

/*-------------------------------------------------------------*/

status_t	BMessenger::SendMessage(const BMessage& msg, BMessage *reply,
	uint32 flags, bigtime_t send_timeout, bigtime_t reply_timeout) const
{
//+	SERIAL_PRINT(("SendMessage(%.4s), dest_port=%d, dest_team=%d tid=%d\n",
//+		(char *) &(msg->what), fPort, fTeam, find_thread(NULL)));

	if (msg.WasDelivered()) {
		// Doing a synchronous send on a forwarded message is not allowed
		// because it is possible that a reply to the resulting message
		// should go to more than one place.  This restriction maybe could
		// be lifted if the forward messages is not MTF_SOURCE_IS_WAITING
		// or MTF_REPLY_REQUIRED.
		debugger("Can't do a synchronous send on a fowarded message.");
		return B_ERROR;
	}

	if (fPort == -1)
		return B_BAD_PORT_ID;

#if 0
	/*
	 There's a possibility here of a client trying to send a message
	 to himself (i.e. the same thread/looper). If that's true then this
	 will deadlock. There is no super-effient way of checking for this
	 error case. Here's some code that will work. If we had a discipline
	 like scheme in the messaging system we could use this code.
	 Can't use the be_app object here as you can Send messages w/o it.
	 As of 4/16/95 this code would slow down Send/Reply of messaging
	 about 7%. That's too much.
	*/
	port_info	pinfo;
	thread_info	tinfo;
	thread_id	tid = find_thread(NULL);
	get_port_info(fPort, &pinfo);
	get_thread_info(tid, &tinfo);
	if (pinfo.team == tinfo.team) {
//+		SERIAL_PRINT(("SendMessage(%.4s) from team=%d to team=%d\n",
//+			(char *) &(a_message.what), pinfo.team, tinfo.team));
		// sending to the same team...
		// now determine the handler, and from the the destination looper
		BHandler	*handler = NULL;
		BLooper		*looper;
		gDefaultTokens->GetToken(fHandlerToken, HANDLER_TOKEN_TYPE, (void **) &handler);
		if (handler && (looper = handler->Looper())) {
			SERIAL_PRINT(("SendMessage(%.4s) from team=%d to team=%d\n",
				(char *) &(a_message.what), pinfo.team, tinfo.team));
			SERIAL_PRINT(("	SendMessage deadlock: tid=%d to tid=%d\n", looper->Thread(), tid));
			if (looper->Thread() == tid) {
				// deadlock
				return B_ERROR;
			}
		}
	}
#endif

	// Try to find a direct target object for whoever the message is being
	// sent to.
	BDirectMessageTarget* direct = NULL;
	if (_find_cur_team_id_() == fTeam)
		direct = gDefaultTokens->TokenTarget(fHandlerToken);
	
	status_t err = msg.send_synchronous(direct, system_time(), fPort, fTeam, fHandlerToken,
										(flags&~(MTF_PREFERRED_TARGET))
										| (fPreferredTarget ? MTF_PREFERRED_TARGET : 0),
										reply, send_timeout, reply_timeout);
	if (direct)
		direct->ReleaseTarget();
	
	return err;
}

status_t	BMessenger::SendMessage(uint32 command, BMessage* reply) const
{
	return SendMessage(BMessage(command), reply);
}

/*-------------------------------------------------------------*/

bool	BMessenger::IsValid() const
{
	if (fPort < 0)
		return (false);

	return (port_count(fPort) != B_BAD_PORT_ID);
}

/*-------------------------------------------------------------*/

team_id BMessenger::Team() const
{
	return fTeam;
}

/*-------------------------------------------------------------*/

BDataIO& operator<<(BDataIO& io, const BMessenger& messenger)
{
#if SUPPORTS_STREAM_IO
	port_info pi;
	status_t res = get_port_info(messenger.fPort, &pi);
	io << "BMessenger(port=" << messenger.fPort;
	if (res) io << " (" << pi.name << ")";
	io << ", team=" << messenger.fTeam;
	if (messenger.fPreferredTarget) io << ", target=preferred:";
	else io << ", target=direct:";
	if (messenger.fHandlerToken == NO_TOKEN) io << "NONE";
	else io << (void*)(messenger.fHandlerToken);
	io << ")";
#else
	(void)messenger;
#endif
	return io;
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

/* Old BMessenger API */

status_t	BMessenger::SendMessage(uint32 command, BHandler *reply_to) const
{
	return SendMessageAtTime(BMessage(command), 0, BMessenger(reply_to));
}

status_t	BMessenger::SendMessage(BMessage *msg, BMessenger reply_to,
	bigtime_t timeout) const
{
	return SendMessageAtTime(*msg, 0, reply_to, B_TIMEOUT, timeout);
}

status_t	BMessenger::SendMessage(BMessage *msg, BHandler *reply_to,
	bigtime_t timeout) const
{
	return SendMessageAtTime(*msg, 0, BMessenger(reply_to), B_TIMEOUT, timeout);
}

status_t	BMessenger::SendMessage(BMessage *msg, BMessage *reply,
	bigtime_t send_timeout, bigtime_t reply_timeout) const
{
	return SendMessage(*msg, reply, B_TIMEOUT, send_timeout, reply_timeout);
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
