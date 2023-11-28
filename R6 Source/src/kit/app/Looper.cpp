/*****************************************************************************

     $Revision: 1.108 $

     $Author: peter $

	 Written By: Peter Potrebic

     Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <new>
#include <string.h>

#ifndef _DEBUG_H
#include <Debug.h>
#endif
#ifndef _STDLIB_H
#include <stdlib.h>
#endif
#ifndef _LOOPER_H
#include <Looper.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _MESSENGER_H
#include <Messenger.h>
#endif
#ifndef _MESSAGEQUEUE_H
#include <MessageQueue.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _LIST_H
#include <List.h>
#endif
#ifndef _MESSAGE_FILTER_H
#include <MessageFilter.h>
#endif
#ifndef _ALERT_H
#include <Alert.h>
#endif
#include <Autolock.h>
#include <ClassInfo.h>
#include <Handler.h>
#include <PropertyInfo.h>
#include <StreamIO.h>
#include <View.h>
#include <Window.h>

//  ---- private includes ----
#ifndef _MESSAGES_H
#include <messages.h>
#endif
#ifndef _MESSAGE_BODY_H
#include <MessageBody.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _TOKEN_SPACE_H
#include <token.h>
#endif
#ifndef _APP_DEFS_PRIVATE_H
#include <AppDefsPrivate.h>
#endif
#include <DebugLock.h>
using namespace B::Support;
//  ---- end of private includes ----

#include <new>

team_id
_find_cur_team_id_()
{
	return (BLooper::sTeamID);
}


static team_id
init_cur_team_id()
{	
	thread_info threadInfo;
	get_thread_info(find_thread(NULL), &threadInfo);
	return (threadInfo.team);
}

struct _looper_list_nuker_ {
	~_looper_list_nuker_();
};

_looper_list_nuker_::~_looper_list_nuker_()
{
	if (BLooper::sLooperList) free(BLooper::sLooperList);
}
static _looper_list_nuker_ list_nuker;

/*---------------------------------------------------------------*/

namespace BPrivate {

class BLooperTarget : public BDirectMessageTarget
{
public:
	BLooperTarget(BMessageQueue* queue, port_id port)
		: fCount(0), fQueue(queue), fPort(port)
	{
//+		BErr << "Creating BLooperTarget " << this << endl;
	}

	void Shutdown()
	{
		BAutolock _l(fAccess);
		fQueue = NULL;
		fPort = B_BAD_PORT_ID;
	}
	
	virtual bool AcquireTarget()
	{
		atomic_add(&fCount, 1);
		return true;
	}
	
	virtual void ReleaseTarget()
	{
		if (atomic_add(&fCount, -1) == 1) {
			delete this;
		}
	}
	
	virtual status_t EnqueueMessage(BMessage* msg)
	{
		BAutolock _l(fAccess);
		if (fQueue) {
//+			BErr << "Enqueueing message: " << *msg << endl;
			fQueue->AddMessage(msg);
			if (fQueue->IsNextMessage(msg) && port_count(fPort) <= 0) {
				// Wake up looper thread.  This only needs to be done
				// if there aren't already messages waiting in the port.
				// Note that we also write to the port with a 0 timeout,
				// that we won't block in the unlikely event that between
				// the above check and now the port becomes full.
//+				BErr << "Waking up target looper!" << endl;
				write_port(fPort,0,NULL,0);
			}
//+			BErr << "Message enqueued." << endl;
			return B_OK;
		}
		
//+		BErr << "Dropping message: " << *msg << endl;
		delete msg;
		return B_ERROR;
	}

protected:
	virtual ~BLooperTarget()
	{
//+		BErr << "Deleting BLooperTarget " << this << endl;
		Shutdown();
	}

private:
	int32 fCount;
	BLocker fAccess;
	BMessageQueue* fQueue;
	port_id fPort;
};

}	// namespace BPrivate

using namespace BPrivate;

/*---------------------------------------------------------------*/

uint32		BLooper::sLooperID = 0;
uint32		BLooper::sLooperListSize = 0;
uint32		BLooper::sLooperCount = 0;
_loop_data_	*BLooper::sLooperList = NULL;
BLocker		BLooper::sLooperListLock("looper_list_lock");
team_id		BLooper::sTeamID = init_cur_team_id();

#define LL_SIZE 20

char	lock_error[] = "The looper must be locked\n";


/*---------------------------------------------------------------*/

BLooper::BLooper(const char *name, int32 priority, int32 port_capacity)
	: BHandler(name), fHandlers(10)
{
	InitData(name, priority, port_capacity);
}

void BLooper::InitData(const char *name, int32 priority, int32 port_capacity)
{
	InitData();

	if (port_capacity <= 0)
		port_capacity = B_LOOPER_PORT_DEFAULT_CAPACITY;

	if (name) {
		char tmp[1024];
		size_t	len = strlen(name);
		if (len < 950)
			strcpy(tmp, name);
		else
			strcpy(tmp, "x");

		strcat(tmp, "_Sem");
#if SUPPORTS_LOCK_DEBUG
		if (LockDebugLevel())
			fLockSem = reinterpret_cast<sem_id>(
				new(std::nothrow) DebugLock(	"BLooper", this, tmp,
										LOCK_CAN_DELETE_WHILE_HELD));
		else
#endif
			fLockSem = create_sem(0, tmp);
		if (len < 950)
			strcpy(tmp, name);
		else
			strcpy(tmp, "x");
		strcat(tmp, "_Port");
		fMsgPort = create_port(port_capacity, name);
	} else {
#if SUPPORTS_LOCK_DEBUG
		if (LockDebugLevel())
			fLockSem = reinterpret_cast<sem_id>(
				new(std::nothrow) DebugLock(	"BLooper", this, "anonymous looper_lock",
										LOCK_CAN_DELETE_WHILE_HELD));
		else
#endif
			fLockSem = create_sem(0, "anonymous looper_lock");
		fMsgPort = create_port(port_capacity, "anonymous LooperPort");
	}

	fInitPriority = priority;

	AddLooper(this);
	// Remember that the looper is locked at this point

	AddHandler(this);
	SetNextHandler(NULL);
}

/*---------------------------------------------------------------*/

		// S_NAME is first defined by BHandler
#define S_NAME			"_name"
#define S_PRIORITY		"_prio"
#define S_PORT_CAPACITY	"_port_cap"

BLooper::BLooper(BMessage *data)
	: BHandler(data)
{
	long prio;
	long cap;
	const char *str;

	data->FindString(S_NAME, &str);
	if (data->FindInt32(S_PRIORITY, &prio) != B_OK) {
		prio = B_NORMAL_PRIORITY;
	}
	if (data->FindInt32(S_PORT_CAPACITY, &cap) != B_OK)
		cap = B_LOOPER_PORT_DEFAULT_CAPACITY;

	/*
	 In PR2 the Archive() code was potentially storing garbage in the
	 S_PRIORITY field. Try to find this garbage and reset to default.
	*/
	if ((prio <= 0) || (prio > B_REAL_TIME_PRIORITY))
		prio = B_NORMAL_PRIORITY;

	InitData(str, prio, cap);
}

/*---------------------------------------------------------------*/

status_t BLooper::Archive(BMessage *data, bool deep) const
{
	_inherited::Archive(data, deep);

	thread_info	tinfo;
	port_info	pinfo;

	if (get_thread_info(Thread(), &tinfo) == B_OK)
		data->AddInt32(S_PRIORITY, tinfo.priority);
	if (get_port_info(fMsgPort, &pinfo) == B_OK)
		data->AddInt32(S_PORT_CAPACITY,  pinfo.capacity);

	return 0;
}

/*---------------------------------------------------------------*/

BLooper::BLooper(int32 priority, port_id port, const char *name)
	: BHandler(name), fHandlers(10)
{
	InitData();

#if SUPPORTS_LOCK_DEBUG
	if (LockDebugLevel())
		fLockSem = reinterpret_cast<sem_id>(
			new(std::nothrow) DebugLock(	"BApplication", this, "app_looper_lock",
									LOCK_CAN_DELETE_WHILE_HELD));
	else
#endif
		fLockSem = create_sem(0, "app_looper_lock");
	fMsgPort = port;
	fInitPriority = priority;

	// if port create fails then fMsgPort will remain < 0

//+	PRINT(("App looper (name=%s, sem=%d, port=%d\n",
//+		name?name:"<null>", fLockSem, fMsgPort));

	AddLooper(this);
	// Remember that the looper is locked at this point

	AddHandler(this);
	SetNextHandler(NULL);
}

/*---------------------------------------------------------------*/

BArchivable *BLooper::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BLooper"))
		return NULL;
	return new BLooper(data);
}

/*---------------------------------------------------------------*/

void BLooper::InitData()
{
	fQueue = new BMessageQueue();
	fMsgPort = -1;
	fTerminating = false;
	fLastMessage = NULL;
	fTaskID = -1;
	fRunCalled = false;
	fInitPriority = 0;
	fOwner = -1;
	fOwnerCount = 0;
	fAtomicCount = 0;
	fCommonFilters = NULL;
	fPreferred = NULL;
	fCachedPid = -1;
	fCachedStack = 0;
	fTarget = NULL;
	fFlags = 0;
}

/*---------------------------------------------------------------*/

BLooper::~BLooper()
{
	thread_id tid;

	if (fTarget) {
		fTarget->Shutdown();
		fTarget->ReleaseTarget();
		fTarget = NULL;
	}
	
//+	PRINT(("BLooper::~BLooper(this=%x, sem=%d, fTaskID=%d, tid=%d)\n",
//+		this, fLockSem, fTaskID, find_thread(NULL)));
	ASSERT((fTaskID == -1) || (fTaskID == find_thread(NULL)));
	if (fRunCalled && !fTerminating)
		debugger("You can't call delete on a BLooper object once it is running.\n");

	// Delete the queye will also delete any messages inside the queue
	BMessage *msg;
	fQueue->Lock();
	while ((msg = fQueue->NextMessage()) != 0) {
//+		PRINT(("	excess qmsg (%.4s)\n", (char *) &(msg->what)));
		delete msg;
	}
	fQueue->Unlock();
	delete fQueue;
	fQueue = NULL;

	// Don't let anyone send more messages to this looper.
	if (fMsgPort > 0) {
		close_port(fMsgPort);
	}
	
	// Read remaining messages from port, so that they can be
	// cleanly deleted and any expected replies returned.
	status_t err = B_OK;
	do {
		BMessage *msg = ReadMessageFromPort(0);
		if (msg) {
//+			PRINT(("	excess msg (%.4s)\n", (char *) &(msg->what)));
			delete msg;
		}
		// See if there are any more messages.  ReadMessageFromPort()
		// may return NULL if a PUSH or invalid message is in the port.
		while ((err=port_buffer_size_etc(fMsgPort, B_TIMEOUT, 0)) == B_INTERRUPTED)
			;
	} while (err >= B_OK);
//+	PRINT(("(%s, %d), read all\n", Name(), Thread()));

	if (fMsgPort > 0) {
		delete_port(fMsgPort);
	}

//+	PRINT(("(%s, %d), deleted port (%d)\n", Name(), Thread(), fMsgPort));

	if (fLastMessage) {
		delete fLastMessage;
		fLastMessage = 0;
	}
	
	Lock();
	RemoveHandler(this);
	SetCommonFilterList(NULL);

	int			i = 0;
	BHandler	*h;
	while ((h = (BHandler *) fHandlers.ItemAt(i++)) != NULL) {
		RemoveHandler(h);
	}

	Unlock();
	RemoveLooper(this);

#if SUPPORTS_LOCK_DEBUG
	if (LockDebugLevel())
		reinterpret_cast<DebugLock*>(fLockSem)->Delete();
	else
#endif
		delete_sem(fLockSem);

	if (fRunCalled) {
		/*
		 Need this explicit call the the _inherited destructor because
		 we're about to kill the thread so the destructor wouldn't otherwise
		 get called.
		*/
		fHandlers.BList::~BList();
		this->BHandler::~BHandler();

		ASSERT((fTaskID != -1));
		tid = fTaskID;
		operator delete(this);
			// since destructor doesn't return, delete the raw memory
	
		exit_thread(0);				// doesn't return!
	}
}

/*---------------------------------------------------------------*/

void	BLooper::do_quit_requested(BMessage *msg)
{
	bool	wants_reply = false;
	bool	ok;
	
	if (msg->IsSourceWaiting())
		wants_reply = true;
	else
		msg->FindBool("wants_reply", &wants_reply);

	ok =  QuitRequested();

//+	PRINT(("do_quit_requested(%s, %d) ok=%d, reply=%d\n",
//+		Name(), Thread(), ok, wants_reply));

	if (wants_reply) {
		BMessage reply(B_REPLY);
		reply.AddBool("result", ok);
		reply.AddInt32("thread", Thread());
		msg->SendReply(&reply);
	}

	if (ok)
		Quit();
}


/*---------------------------------------------------------------*/

bool	BLooper::QuitRequested()
{
#if DEBUG
	if (!IsLocked())
		TRACE();
#endif
	return true;
}

/*---------------------------------------------------------------*/

void	BLooper::Quit()
{
	if (!IsLocked()) {
		printf("ERROR - you must Lock a looper before calling Quit(), team=%li, looper=%s\n",
			_find_cur_team_id_(), Name() ? Name() : "no-name");
	}

	if (!Lock())
		return;		// looper is already gone.
	
	if (!fRunCalled) {
		// Run() hasn't been called so just delete the object
		fTerminating = true;
		delete this;
	} else if (find_thread(NULL) == fTaskID) {
		/*
		 We're in the right thread (the Looper thread) so proceed with quit
		*/
		fTerminating = true;
		delete this;	// does not return!
	} else {
		/*
		 We're not in the looper task, so we must send it a message
		 and wait for it to quit.

		 By posting a message, all previously posted
		 messages will be handled, before the TERMINATE message arrives.
		*/
		status_t	error;
		thread_id	tid = Thread();
		
		error = PostMessage(_QUIT_);
		UnlockFully();

		/*
		 There's a small, but non-zero chance that the above Post
		 failed. This could happen because the port was full. In this
		 case we must busy wait/Re-post until successful.
		*/
		while (error == B_WOULD_BLOCK) {
			snooze(50000);	// 1/20 of a second
			error = PostMessage(_QUIT_);
		}

		// now wait until the close finishes in the Looper thread
		int32		retval;
		do {
			error = wait_for_thread(tid, &retval);
		} while (error == B_INTERRUPTED);
	}
}

/*---------------------------------------------------------------*/

bool BLooper::IsRunningInCurrentThread()
{
	return ((fFlags & STEAL_CURRENT_THREAD) != 0);
}

thread_id BLooper::RunInCurrentThread()
{
	fFlags |= STEAL_CURRENT_THREAD;
	return Run();
}

thread_id	BLooper::Run()
{
	AssertLocked();

	if (fRunCalled) {
		debugger("can't call BLooper::Run twice!");
		return -1;
	}

	// if we couldn't create the port then Run() should fail as well.
	if (fMsgPort < 0)
		return fMsgPort;

	fRunCalled = true;

	thread_id tid;
	if (IsRunningInCurrentThread())
	{
		tid = find_thread(NULL);
		set_thread_priority(tid, fInitPriority);
		if (Name())
			rename_thread(tid, Name());
		task_looper();
	}
	else
	{	
		tid = spawn_thread(BLooper::_task0_, Name() ? Name() : (char *) "",
			fInitPriority, this);
		fTaskID = tid;
	
//+	PRINT(("looper (name=%s, tid=%d, sem=%d, port=%d\n",
//+		Name()?Name():"<null>", fTaskID, fLockSem, fMsgPort));
	
		Unlock();
		// the looper could be killed at any point now. Don't deref 'this'
		// anymore.
	
		if (tid > B_NO_ERROR)
			resume_thread(tid);
	}
	
	return tid;
}

/*---------------------------------------------------------------*/

status_t BLooper::_task0_(void *arg)
{
	BLooper		*looper;

	looper = (BLooper *) arg;

	// Need to Lock the looper in order to call this virtual function!

	if (looper->Lock()) {

		// task_looper will do the Unlock();
		looper->task_looper();
		// when this returns the looper is dying.

		looper->fTerminating = true;
		// fTerminating will already be true for BLooper::task_looper
		// overrides of task_looper don't have access to fTerminating
		// but it they get here, they are already terminating properly
		// - set fTerminating to avoid a complaint in the destructor
		
		delete looper;
	}

	return B_NO_ERROR;
}

/*-------------------------------------------------------------*/

bool BLooper::IsMessageWaiting() const
{
	ssize_t sz;
	
	/*
	 Returns true if there is a waiting message. Either in the msg_queue
	 or in the port.
	*/
	if (!IsLocked()) {
		debugger("The Looper must be locked before calling IsMessageWaiting");
		return false;
	}

	if (!fQueue->IsEmpty())
		return true;

	while ((sz = port_buffer_size_etc(fMsgPort, B_TIMEOUT, 0)) == B_INTERRUPTED)
		;

	return (sz != 0);
}

/*-------------------------------------------------------------*/

BMessage *BLooper::ReadMessageFromPort(bigtime_t timeout)
{
	ssize_t		size;
	while ((size = port_buffer_size_etc(fMsgPort, B_TIMEOUT, timeout))
				== B_INTERRUPTED)
		;
	// If an error or data is too small for a message, just return.
	if (size < 4) {
		if (size >= B_OK) {
			int32 code;
			int8 buf[4];
			read_port_etc(fMsgPort, &code, &buf, size, B_TIMEOUT, 0);
		}
		return NULL;
	}
	
	long		code;
	BMessage*	msg = new BMessage;
	
	if (!msg)
		return NULL;
	
	status_t	err = msg->ReadPort(fMsgPort, size, &code, B_TIMEOUT, 0);
	if (err < B_OK) {
		delete msg;
		msg = NULL;
	}
	
	return msg;
}

BMessage *
BLooper::MessageFromPort(bigtime_t timeout)
{
	// backward compatible way of making ReadMessageFromPort protected
	// and allow calling it from task_looper overrides
	return ReadMessageFromPort(timeout);
}


/*-------------------------------------------------------------*/

BMessage *BLooper::ConvertToMessage(void */*raw*/, int32 /*code*/)
{
	// No longer used.
	return NULL;
}

/*-------------------------------------------------------------*/

BLooper::loop_state::loop_state()
{
	memset(this, 0, sizeof(this));
	reset();
}

BLooper::loop_state::~loop_state()
{
}

void BLooper::loop_state::reset()
{
	next_loop_time = B_INFINITE_TIMEOUT;
}

/*-------------------------------------------------------------*/

status_t BLooper::ReadyToLoop(loop_state* /*outState*/)
{
	return B_OK;
}

/*-------------------------------------------------------------*/

void	BLooper::task_looper()
{
	BMessage	*msg;
	loop_state	loop;
	
	// The looper is locked before calling this virtual. Let's Unlock here.
	AssertLocked();
	Unlock();

	if (IsLocked())
		debugger("shouldn't be locked anymore");

	while(1) {
loop_again:
		loop.reset();
		Lock();
		ReadyToLoop(&loop);
		Unlock();
		
		bigtime_t nextTime = fQueue->NextTime();
		
		if (nextTime > loop.next_loop_time) {
			nextTime = loop.next_loop_time;
		}
		if (nextTime != B_INFINITE_TIMEOUT) {
			bigtime_t sysTime = system_time();
			if (nextTime > sysTime) nextTime -= sysTime;
			else nextTime = 0;
		}
		msg = ReadMessageFromPort(nextTime);
		if (msg)
			_AddMessagePriv(msg);

		if (fTerminating)
			goto out;

		int32 pcount = port_count(fMsgPort);
		while(pcount > 0) {
			pcount--;
			msg = ReadMessageFromPort(0);
			if (msg)
				_AddMessagePriv(msg);
		}

		while(fQueue->NextTime() <= system_time()) {
			msg = fLastMessage = fQueue->NextMessage();
			ASSERT(fLastMessage->what != 'PUSH');

			BHandler	*handler = NULL;
			
#if _SUPPORTS_EXCEPTION_HANDLING
			try
#endif
				{
				long token;
				bool pref;
				
				Lock();

				pref = _use_preferred_target_(fLastMessage);
				token = _get_message_target_(fLastMessage);

				if (pref) {
					handler = PreferredHandler();
					if (!handler)
						handler = this;
				} else {
					if (token != NO_TOKEN)
						gDefaultTokens->GetToken(token, HANDLER_TOKEN_TYPE,
												 (void **) &handler);
				}

				/*
				 The handler might be null if the handler was deleted in
				 between the time of the posting/sending and now. Or
				 the handler might not belong to Looper anymore.

				 If the handler doesn't exist then the message is dropped.
				*/

				if ((token == NO_TOKEN) || fHandlers.HasItem(handler)) {
					ASSERT(!handler || is_kind_of(handler, BHandler));

					bool skip = false;

					if (fLastMessage->HasSpecifiers()) {
						int32	s;
						fLastMessage->GetCurrentSpecifier(&s, NULL, NULL, NULL);
						if (s >= 0) {
							BHandler *h = handler ? handler : this;
							handler = resolve_specifier(h, fLastMessage);
							if (!handler)
								skip = true;
						}
					}

					if (!skip) {
						BHandler *new_handler =
							top_level_filter(fLastMessage, handler);
						if (new_handler) {
							ASSERT(new_handler->Looper() == this);
							handler = new_handler;
							DispatchMessage(fLastMessage, handler);
						}
					}
				}
				Unlock();
			}

#if _SUPPORTS_EXCEPTION_HANDLING			
			catch (...) {
				// unhandled exception...
				BAlert *alert;
				bool	is_app = is_kind_of(this, BApplication);
				char buf[300] = "";
				app_info ai;
				if (be_app) {
					if (!be_app->GetAppInfo(&ai)) {
						strcpy(buf, ai.ref.name);
						strcat(buf, ": ");
					}
				}
				if (is_app) {
					strcat(buf, "Unexpected application error. Should an attempt be made to continue running?");
				}
				else {
					strcat(buf, "Unexpected thread error. Should an attempt be made to continue running?");
				}
				alert = new BAlert("", buf, 
					"Continue", "Give up", NULL,
					B_WIDTH_FROM_WIDEST, B_STOP_ALERT);
				long r = alert->Go();
				if (r == 1) {
					fTerminating = true;
				} else {
					// unlock the looper
					long c = CountLocks();
					while (c--) {
						Unlock();
					}
				}
			}
#endif

			// fLastMessage might have been set to NULL during DispatchMessage
			if (fLastMessage)
				delete fLastMessage;
			fLastMessage = 0;

			// Check to make sure message queue is unlocked.  An application
			// should never lock the message queue longer than it is handling
			// a message.
			if (fQueue->IsLocked()) {
				debugger("Message queue lock held!");
				fQueue->UnlockFully();
			}
			
			if (fTerminating)
				goto out;

			if (port_count(fMsgPort)) {
				goto loop_again;
			}
		}
	}
out:
	return;
}

/*---------------------------------------------------------------*/

void BLooper::AddMessage(BMessage *msg)
{
	ASSERT(fQueue);

	if (_get_message_target_(msg) == NO_TOKEN) {
		_set_message_target_(msg, _get_object_token_(this),
			_use_preferred_target_(msg));
	}

	fQueue->AddMessage(msg);

	// wake up the looper thread if needed
	if ((fTaskID > 0) && (find_thread(NULL) != fTaskID) &&
		(port_count(fMsgPort) == 0)) {
			long pusher;

			pusher = 'PUSH';
			long err;
			while ((err = write_port(fMsgPort, -1, (char *) &pusher, sizeof(pusher))) == B_INTERRUPTED)
				;
	}
}

/*---------------------------------------------------------------*/

// this version of BLooper::AddMessage assumes that it is being
// called by in looper's own thread, so it bypasses the 
// find_thread(NULL)/'PUSH' stuff

void BLooper::_AddMessagePriv(BMessage *msg)
{
	ASSERT(fQueue);

	if (_get_message_target_(msg) == NO_TOKEN) {
		_set_message_target_(msg, _get_object_token_(this),
			_use_preferred_target_(msg));
	}

	fQueue->AddMessage(msg);
}

/*---------------------------------------------------------------*/


void BLooper::DispatchMessage(BMessage *msg, BHandler *handler)
{
//+	if (msg->what != B_PULSE) {
//+		PRINT(("BLooper::Dispatch(what=%.4s, tid=%d, name=%s, sem=%d)\n",
//+			(char *) &(msg->what), Thread(),
//+			Name() ? Name() : "<null>", fLockSem));
//+	}

	ASSERT(handler);
	ASSERT(IsLocked());

	switch (msg->what) {
		case B_QUIT_REQUESTED: {
			if (handler == this)
				do_quit_requested(msg);
			else
				handler->MessageReceived(msg);
			break;
		}
		case _QUIT_: {
			fTerminating = true;
			delete this;		// doesn't return
			break;
		}
		default: {
			handler->MessageReceived(msg);
			break;
		}
	}
}

/*---------------------------------------------------------------*/

void BLooper::MessageReceived(BMessage *msg)
{
	bool		handled = false;
	BMessage	reply(B_REPLY);
	status_t	err;
	BMessage	spec;
	int32		form;
	const char	*prop;
	int32		cur;
	err = msg->GetCurrentSpecifier(&cur, &spec, &form, &prop);

	if (!err) {
		switch (msg->what) {
			case B_COUNT_PROPERTIES: {
				if (strcmp(prop, "Handler") == 0) {
					reply.AddInt32("result", CountHandlers());
					handled = true;
				}
				break;
			}
			case B_GET_PROPERTY: {
				if (strcmp(prop, "Handlers") == 0) {
					BHandler	*hand;
					int32		i = 0;
					while ((hand = (BHandler *) HandlerAt(i++)) != NULL) {
						reply.AddMessenger("result", BMessenger(hand, this));
					}
					handled = true;
				}
				break;
			}
		}
	}

	if (handled)
		msg->SendReply(&reply);
	else
		_inherited::MessageReceived(msg);
}

/*---------------------------------------------------------------*/

BHandler *BLooper::apply_filters(BList *list, BMessage *msg,
	BHandler *target)
{
//+	if (msg->what == B_KEY_DOWN) {
//+		PRINT(("applying filters on target (%s), list size = %d\n",
//+			target->Name() ? target->Name() : "null", 
//+			list ? list->CountItems() : -1));
//+	}

	if (!list)
		return target;

	BHandler		*new_target = target;
	long			i = 0;
	BMessageFilter	*filter;
	filter_result	result;

	// allow more than 1 filter for a given 'what' field
	while ((filter = (BMessageFilter *) list->ItemAt(i++)) != 0) {
		filter->SetLooper(this);
		message_delivery deliver = filter->MessageDelivery();
		message_source	source = filter->MessageSource();
		bool			dropped = msg->WasDropped();
		bool			remote = msg->IsSourceRemote();
//+		PRINT(("	peeking at (delivery=%d, source=%d, d=%d, r=%d)\n",
//+			deliver, source, msg->WasDropped(), msg->IsSourceRemote()));
		if (
			(filter->FiltersAnyCommand() || (filter->Command() == msg->what))
			&&
			((deliver == B_ANY_DELIVERY) ||
			 ((deliver == B_DROPPED_DELIVERY) && dropped) ||
			 ((deliver == B_PROGRAMMED_DELIVERY) && !dropped))
			&&
			((source == B_ANY_SOURCE) ||
			 ((source == B_REMOTE_SOURCE) && remote) ||
			 ((source == B_LOCAL_SOURCE) && !remote))
		)	{
			filter_hook ff = filter->FilterFunction();
			if (ff)
				result = (*ff) (msg, &new_target, filter);
			else
				result = filter->Filter(msg, &new_target);

			if (result == B_SKIP_MESSAGE)
				return NULL;
		}
	}

//+	if (new_target && (new_target->Looper() != this)) {
//+		PRINT(("Handler does not belong, new=%x,%s,%s, orig=%x,%s,%s\n",
//+			new_target, new_target->Name() ? new_target->Name() : "null",
//+				class_name(new_target),
//+			target, target->Name() ? target->Name() : "null",
//+				class_name(target)));
//+	}

	return new_target;
}

/*---------------------------------------------------------------*/

BHandler *BLooper::handler_only_filter(BMessage *msg, BHandler *target)
{
	BHandler	*prev_target = NULL;

	/*
	 Keep filtering until that target doesn't change.
	*/
	while (target && (target != prev_target)) {
		prev_target = target;
		target = apply_filters(prev_target->FilterList(),
			msg, prev_target);
		if (target && (target->Looper() != this)) {
			debugger("Handler returned by (H) FilterMessage does not belong to the looper");
			return NULL;
		}
	}

	return target;
}

/*---------------------------------------------------------------*/

BHandler *BLooper::top_level_filter(BMessage *msg, BHandler *target)
{
	BHandler	*new_target;

	// First we filter on the Looper's list of filters.
	if ((new_target = apply_filters(CommonFilterList(), msg, target)) != 0) {
		if (new_target->Looper() != this) {
//+			PRINT(("(%.4s) Handler does not belong, new=%x,%s,%s, orig=%x,%s,%s\n",
//+				(char*) &(msg->what), new_target,
//+					new_target->Name() ? new_target->Name() : "null",
//+					class_name(new_target),
//+				target, target->Name() ? target->Name() : "null",
//+					class_name(target)));
			debugger("Handler returned by (L) FilterMessage does not belong to the looper");
			return NULL;
		}

		new_target = handler_only_filter(msg, new_target);
	}

	return new_target;
}

/*---------------------------------------------------------------*/

BMessage *BLooper::CurrentMessage() const
{
	/*
	 This method only makes sense when called from the looper thread.
	 Otherwise the last msg is meaningless. It's also unsafe as at
	 any point the msg might get deleted.
	*/
	if (find_thread(NULL) != Thread())
		return NULL;

	return fLastMessage;
}

/*-------------------------------------------------------------*/

void BLooper::AddHandler(BHandler *rec)
{
	if (!IsLocked()) {
		debugger("Looper must be locked before calling AddHandler.\n");
	}

	if (rec->Looper()) {
		debugger("can't add same handler to 2 loopers\n");
		return;
	}

	fHandlers.AddItem(rec);
	rec->SetLooper(this);
	
	// by default the 'NextHandler' of all handlers is the looper.
	rec->SetNextHandler(this);
	
	// Create the looper target, if needed, and set it for the handler
	// in the token space.  Doing this after actually adding the handler
	// is fine because if anything tries to send a message to it before
	// now then it will just go through the port.
	InstallLooperTarget(rec);
}

/*-------------------------------------------------------------*/

void BLooper::InstallLooperTarget(BHandler* rec)
{
	// Install the looper target, ONLY if we have a message port.  This
	// conditition is to take care of the BApplication subclass, which will
	// create the message port after our BLooper constructor is done.  It
	// is then up to BApplication to call InstallLooperTarget() to get
	// it installed once the port is available.
	if (!fTarget && fMsgPort >= B_OK) {
		fTarget = new BLooperTarget(fQueue, fMsgPort);
		fTarget->AcquireTarget();
	}
	if (fTarget) {
		gDefaultTokens->SetTokenTarget(rec->fToken, fTarget);
	}
}

/*-------------------------------------------------------------*/

bool BLooper::RemoveHandler(BHandler *rec)
{
	if (!IsLocked()) 
		debugger("Looper must be locked before calling RemoveHandler.\n");

	if (rec->Looper() != this)
		return false;

	if (rec == fPreferred)
		fPreferred = NULL;

	// Remove direct target for this handler.
	gDefaultTokens->SetTokenTarget(rec->fToken, NULL);
	
	rec->SetNextHandler(NULL);
	rec->SetLooper(NULL);
	
	return fHandlers.RemoveItem(rec);
}

/*-------------------------------------------------------------*/

int32 BLooper::IndexOf(BHandler *handler) const
{
	if (!IsLocked())
		debugger("Looper must be locked before calling IndexOf.\n");

	return fHandlers.IndexOf(handler);
}

/*-------------------------------------------------------------*/

BHandler *BLooper::HandlerAt(int32 index) const
{
	if (!IsLocked())
		debugger("Looper must be locked before calling HandlerAt.\n");

	return (BHandler *) fHandlers.ItemAt(index);
}

/*-------------------------------------------------------------*/

int32 BLooper::CountHandlers() const
{
	if (!IsLocked())
		debugger("Looper must be locked before calling CountHandlers.\n");

	return fHandlers.CountItems();
}

/*-------------------------------------------------------------*/

BMessage *BLooper::DetachCurrentMessage()
{
	BMessage *m = fLastMessage;
	fLastMessage = 0;
	return m;
}

/*-------------------------------------------------------------*/

BMessageQueue	*BLooper::MessageQueue() const
{
	return(fQueue);
}

/*-------------------------------------------------------------*/

thread_id	BLooper::Team() const
{
	return _find_cur_team_id_();
}

/*-------------------------------------------------------------*/

thread_id	BLooper::Thread() const
{
	return fTaskID;
}

/*---------------------------------------------------------------*/

status_t BLooper::PostMessage(BMessage *msg)
{
	return _PostMessage(msg, this, NULL);
}

/*---------------------------------------------------------------*/

status_t BLooper::PostMessage(uint32 command)
{
	BMessage msg(command);
	return _PostMessage(&msg, this, NULL);
}

/*---------------------------------------------------------------*/

status_t BLooper::PostMessage(BMessage *msg, BHandler *handler, BHandler *reply)
{
	return _PostMessage(msg, handler, reply);
}

/*---------------------------------------------------------------*/

status_t BLooper::PostMessage(uint32 command, BHandler *handler, BHandler *reply)
{
	BMessage msg(command);
	return _PostMessage(&msg, handler, reply);
}

/*---------------------------------------------------------------*/

status_t BLooper::_PostMessage(BMessage *msg, BHandler *rec, BHandler *reply_to)
{
	long	error = B_NO_ERROR;

	// check for NULL, calling Post on a NULL pointer.
	if (this == NULL)
		return B_BAD_VALUE;

	// ??? I don't think that this is safe! The BHandler could disappear!
	if (rec && (rec->Looper() != this)) {
		// If there's a handler, that handler's looper better be 'this'.
		return B_MISMATCHED_VALUES;
	}

	// Don't need to sLooperList locking here because
	// BMessenger does it as well. So use the error state of the
	// messenger as a guide.

	// safe to construct here because BMessenger locks the sLooperList lock
	BMessenger target(rec, this, &error);

	if (error == B_NO_ERROR) {
		error = target.SendMessage(msg, reply_to, 0);
	}

	return error;
}

/*---------------------------------------------------------------*/

void BLooper::SetPreferredHandler(BHandler *h)
{
	if (h && !fHandlers.HasItem(h))
		return;

	BWindow *window = dynamic_cast<BWindow *>(this);
	if (window != NULL) {
		fPreferred = window->CurrentFocus();
		return;
	}

	fPreferred = h;
}

/*---------------------------------------------------------------*/

BHandler *BLooper::PreferredHandler() const
{
	return fPreferred;
}

/*-------------------------------------------------------------*/

bool	BLooper::Lock()
{
	return (_Lock(this, -1, B_INFINITE_TIMEOUT, 0) == B_OK);
}

/*-------------------------------------------------------------*/

lock_status_t	BLooper::LockWithStatus()
{
	return _Lock(this, -1, B_INFINITE_TIMEOUT, 0);
}

/*-------------------------------------------------------------*/

lock_status_t	BLooper::LockWithTimeout(bigtime_t timeout, uint32 debug_flags)
{
	return _Lock(this, -1, timeout, debug_flags);
}

/*-------------------------------------------------------------*/
#if xDEBUG
	#include <Window.h>
#endif

lock_status_t	BLooper::_Lock( BLooper *looper, port_id port,
								bigtime_t timeout, uint32 debug_flags)
{
	uint32		stack;
	thread_id	ourPid;

	if ((looper == NULL) && (port == -1)) {
//+		PRINT(("NULL->Lock() attempted, pid=%d\n", ourPid));
		return lock_status_t(B_BAD_VALUE);
	}

	if (!sLooperListLock.Lock()) {
		printf("Attempt to lock a looper after main() returned. Can't have static Loopers.\n");
		return lock_status_t(B_BAD_VALUE);
	}

	if (port >= 0) {
		// we're looking up the looper by port.
		looper = LooperForPort(port);
		if (!looper) {
			// The looper must have been deleted.
			sLooperListLock.Unlock();
			return lock_status_t(B_BAD_VALUE);
		}
	}

	// first ensure that the Looper is valid
	if (!IsLooperValid(looper)) {
		sLooperListLock.Unlock();
		return lock_status_t(B_BAD_VALUE);
	}

	stack = ((uint32)&stack) & 0xFFFFF000;
	if ((looper->fCachedStack == stack) ||
		((ourPid=find_thread(NULL)) == looper->fOwner)) {
		looper->fCachedStack = stack;
		looper->fOwnerCount++;
		sLooperListLock.Unlock();
		return lock_status_t((void (*)(void*))_UnlockFunc, looper);
	}

	long	sem = looper->fLockSem;
	int32	old;
	
#if SUPPORTS_LOCK_DEBUG
	if (LockDebugLevel()) {
		reinterpret_cast<DebugLock*>(sem)->IncRefs();
		old = 1;
	} else
#endif
	{
		if (sem < 0) {
			sLooperListLock.Unlock();
			return lock_status_t(B_BAD_VALUE);
		}
	
		old = atomic_add(&(looper->fAtomicCount), 1);
	}

	/*
	 At this point we're ready to try an lock the actual looper. But,
	 first we must release the global LooperListLock so that we don't
	 block the entire application. After unlocking we must realize that
	 the Looper we're trying to lock could go away before we acquire
	 the semaphore. So the code must be careful to not access any data
	 from the 'this' pointer unless acquire successfully returns. That's
	 why we put a copy of the 'lock_sem' on the stack.
	*/
	sLooperListLock.Unlock();

	return _LockComplete(looper, old, ourPid, sem, timeout, debug_flags);
}

/*-------------------------------------------------------------*/

lock_status_t	BLooper::_LockComplete(BLooper *looper, int32 old,
	thread_id ourPid, sem_id sem, bigtime_t timeout, uint32 debug_flags)
{
#if !SUPPORTS_LOCK_DEBUG
	(void)debug_flags;
#endif
	lock_status_t	result(B_NO_ERROR);

	if (old >= 1) {
#if SUPPORTS_LOCK_DEBUG
		if (LockDebugLevel()) {
			result.value.error = reinterpret_cast<DebugLock*>(sem)
				->Lock(B_TIMEOUT, timeout, debug_flags);
			reinterpret_cast<DebugLock*>(sem)->DecRefs();
		} else
#endif
		{
			do {
				result.value.error = acquire_sem_etc(sem, 1, B_TIMEOUT, timeout);
			} while (result.value.error == B_INTERRUPTED);
		}
	}

	if (result.value.error == B_NO_ERROR) {
		ASSERT(looper->fOwner == -1);
		looper->fOwner = ourPid;
		looper->fOwnerCount = 1;
		looper->fCachedStack = ((uint32)&result) & 0xFFFFF000;
		result.unlock_func = (void (*)(void*))(_UnlockFunc);
		result.value.data = looper;
#if xDEBUG
		BWindow *w;
		if (w = cast_as(looper, BWindow)) {
			if (w->fInUpdate) {
				SERIAL_PRINT(("Window (%s) update, locked, tid=%d, wtid=%d\n",
					w->Title(), find_thread(NULL), w->Thread()));
				debugger("HERE - locking in update\n");
			}
		}
#endif
	}

	return result;
}

/*----------------------------------------------------------------*/

void	BLooper::_UnlockFunc(BLooper *loop)
{
	loop->AssertLocked();

	loop->fOwnerCount--;

	if (loop->fOwnerCount == 0) {
#if xDEBUG
		BWindow *w;
		if (w = cast_as(loop, BWindow)) {
			if (w->fInUpdate) {
				SERIAL_PRINT(("Window (%s) update, unlocked, tid=%d, wtid=%d\n",
					 w->Title(), find_thread(NULL), w->Thread()));
				debugger("HERE - who did it\n");
			}
		}
#endif
		loop->fOwner = -1;
		loop->fCachedStack = 0;

#if SUPPORTS_LOCK_DEBUG
		if (LockDebugLevel()) {
			reinterpret_cast<DebugLock*>(loop->fLockSem)->Unlock();
		} else
#endif
		{
			long old = atomic_add(&loop->fAtomicCount, -1);
			if (old > 1)
				release_sem(loop->fLockSem);
		}
	
		// don't touch any of my member variables from this point on!!!
		// the looper (this) could be gone by now
	}
}

/*----------------------------------------------------------------*/

void	BLooper::Unlock()
{
	_UnlockFunc(this);
}

/*-------------------------------------------------------------*/

void BLooper::UnlockFully() 
{
	AssertLocked();

	fOwnerCount = 0;
	fOwner = -1;
	fCachedStack = 0;

#if SUPPORTS_LOCK_DEBUG
	if (LockDebugLevel()) {
		reinterpret_cast<DebugLock*>(fLockSem)->Unlock();
	} else
#endif
	{
		long old = atomic_add(&fAtomicCount, -1);
		if (old > 1)
			release_sem(fLockSem);
	}
	
	// don't touch any of my member variables from this point on!!!
	// the looper (this) could be gone by now
}

/*-------------------------------------------------------------*/

void BLooper::AddLooper(BLooper *looper)
{
	ulong i;
	_loop_data_ *item;

	if (!sLooperListLock.Lock()) {
		return;
	}
	
	if (sLooperCount == sLooperListSize) {
		sLooperList = (_loop_data_ *) realloc(sLooperList,
			(sLooperListSize + LL_SIZE) * sizeof(_loop_data_));
		memset(&sLooperList[sLooperListSize], -1, LL_SIZE*sizeof(_loop_data_));
		sLooperListSize += LL_SIZE;
		i = sLooperCount;
//		PRINT(("AddLooper - %d (grew list to %d)\n", i, sLooperListSize));
	} else {
		// find an empty slot
		for (i = 0, item = sLooperList; i < sLooperListSize; i++, item++) {
			if (item->id == -1)
				break;
		}
		ASSERT(i < sLooperListSize);
	}

	item = sLooperList + i;
	item->ptr = looper;
	item->id = sLooperID++;
//	PRINT(("add Looper(%x): index=%d, id=%d, sem=%d\n",
//		looper, i, item->id, looper->fLockSem));
	looper->fLooperID = item->id;

	sLooperCount++;

	/*
	 VERY IMPORTANT and SUBTLE.
	 Looper's are initially created in the locked state. So do the initial
	 Lock here, while the sLooperListLock is still held. This will prevent
	 the race condition where some other thread could see this new looper and
	 tell it to Quit while it was still being initialized. The looper will
	 be unlocked when Run is called (see the task_looper method).
	*/
	
#if DEBUG
	bool r =
#endif
#if SUPPORTS_LOCK_DEBUG
	looper->LockWithTimeout(B_INFINITE_TIMEOUT, LOCK_SKIP_DEADLOCK_CHECK);
#else
	looper->LockWithTimeout(B_INFINITE_TIMEOUT, 0);
#endif

	ASSERT(r);

	sLooperListLock.Unlock();
}

/*-------------------------------------------------------------*/

void BLooper::RemoveLooper(BLooper *looper)
{
	if (!sLooperListLock.Lock()) {
		return;
	}
	
	ulong		i;
	_loop_data_ *item;

//	PRINT(("rm Looper: looper=0x%x", looper));

	// walk through the list looking for the given looper
	for (i = 0, item = sLooperList; i < sLooperListSize; i++, item++) {
		if ((item->ptr == looper) && ((uint32)item->id == looper->fLooperID)) {
//			PRINT((" valid"));
			item->ptr = NULL;
			item->id = -1;
			sLooperCount--;

			// we're using the LooperID as an indicator of any changes in the
			// looper list, both addition and deletions. So just bump the count.
			sLooperID++;
			break;
		}
	}
//	PRINT(("\n"));

	sLooperListLock.Unlock();
}

/*-------------------------------------------------------------*/

BLooper *BLooper::LooperForPort(port_id port)
{
	ulong		i;
	_loop_data_	*item;
	BLooper		*looper = NULL;

	if (!sLooperListLock.Lock()) {
		return NULL;
	}

	for (i = 0, item = sLooperList; i < sLooperListSize; i++, item++) {
		if (item->id != -1) {
			if (item->ptr->fMsgPort == port) {
				looper = (BLooper *) item->ptr;
				break;
			}
		}
	}

	sLooperListLock.Unlock();
	return looper;
}

/*-------------------------------------------------------------*/

BLooper *BLooper::LooperForThread(thread_id tid)
{
	ulong		i;
	_loop_data_	*item;
	BLooper		*looper = NULL;

	if (!sLooperListLock.Lock()) {
		return NULL;
	}

	for (i = 0, item = sLooperList; i < sLooperListSize; i++, item++) {
		if (item->id != -1) {
			if (item->ptr->Thread() == tid) {
				looper = (BLooper *) item->ptr;
				break;
			}
		}
	}

	sLooperListLock.Unlock();
	return looper;
}

/*-------------------------------------------------------------*/

BLooper *BLooper::LooperForName(const char */*name*/)
{
#if 0
	// Not doing this because of the race condition mentioned below!

	ulong		i;
	_loop_data_	*item;
	BLooper		*looper = NULL;

	if (!name)
		return NULL;

	if (!sLooperListLock.Lock()) {
		return NULL;
	}

	for (i = 0, item = sLooperList; i < sLooperListSize; i++, item++) {
		if (item->id != -1) {
			// ??? There's a race condition here. The Name can be changed
			// via BHandler::SetName(). So the pointer returned by Name()
			// can be freed at any time!
			const char *n = item->ptr->Name();
			if (n && (strcmp(n, name) == 0)) {
				looper = (BLooper *) item->ptr;
				break;
			}
		}
	}

	sLooperListLock.Unlock();
	return looper;
#endif
	return NULL;
}

/*-------------------------------------------------------------*/

void BLooper::GetLooperList(BList *list)
{
	if (!sLooperListLock.Lock()) {
		return;
	}

	_loop_data_ *item = sLooperList;
	for (ulong i = 0; i < sLooperListSize; i++, item++) {
		if (item->id != -1) {
			list->AddItem((void *) item->ptr);
		}
	}

	sLooperListLock.Unlock();
}

/*-------------------------------------------------------------*/

bool BLooper::IsLooperValid(const BLooper *looper)
{
	ASSERT(sLooperListLock.IsLocked());
//+	if (!sLooperListLock.IsLocked()) {
//+		PRINT(("sLooper err, lock_tid=%d, tid=%d, count=%d\n",
//+			sLooperListLock.LockingThread(), find_thread(NULL),
//+			sLooperListLock.CountLocks()));
//+		return false;
//+	}
	
	if (!looper)
		return false;

	// walk through the list looking for the given looper.
	_loop_data_ *item = sLooperList;
	for (ulong i = 0; i < sLooperListSize; i++, item++) {
		// if the addr's match then check the id's.
		if ((item->id != -1) && (item->ptr == looper)
			&& ((uint32)item->id == looper->fLooperID)) {
//			PRINT(("item->ptr=%x, item->id=%d, w=%x, w->id=%d\n",
//				item->ptr, item->id, looper, looper->fLooperID));
			return true;
		}
	}

	return false;
}

/*-------------------------------------------------------------*/

bool BLooper::AssertLocked() const
{
	if (!IsLocked()) {
		debugger("looper must be locked before proceeding\n");
		return false;
	} else
		return true;
}

/*---------------------------------------------------------------*/

void	BLooper::SetCommonFilterList(BList *filters)
{
	BLooper *loop = Looper();

	if (loop && !loop->IsLocked()) {
		debugger("Owning Looper must be locked before calling SetCommonFilterList");
		return;
	}

	if (fCommonFilters) {
		long c = fCommonFilters->CountItems();
		for (long i = 0; i < c; i++) {
			BMessageFilter *filter = (BMessageFilter *) fCommonFilters->ItemAtFast(i);
			ASSERT(filter);
			delete filter;
		}


		delete fCommonFilters;
		fCommonFilters = NULL;
	}
	fCommonFilters = filters;
}

/*---------------------------------------------------------------*/

void	BLooper::AddCommonFilter(BMessageFilter *filter)
{
	/*
	 This is just a handy shortcut so that in the common case the
	 developer doesn't have to create the initial BList by hand.
	*/ 
	BLooper *loop = Looper();

	if (loop && !loop->IsLocked()) {
		debugger("Owning Looper must be locked before calling AddCommonFilter");
		return;
	}
	if (filter->Looper() != NULL) {
		debugger("A MessageFilter can only be used once.");
		return;
	}

	if (fCommonFilters == NULL) {
		fCommonFilters = new BList();
	}
	filter->SetLooper(this);

	fCommonFilters->AddItem(filter);
}

/*---------------------------------------------------------------*/

bool	BLooper::RemoveCommonFilter(BMessageFilter *filter)
{
	BLooper *loop = Looper();

	if (loop && !loop->IsLocked()) {
		debugger("Owning Looper must be locked before calling RemoveCommonFilter");
		return false;
	}

	if (!fCommonFilters)
		return false;

	if (fCommonFilters->RemoveItem(filter)) {
		filter->SetLooper(NULL);
		return true;
	}
	return false;
}

/*---------------------------------------------------------------*/

BList	*BLooper::CommonFilterList() const
{
	return fCommonFilters;
}

/*-------------------------------------------------------------*/

void	BLooper::check_lock()
{
	uint32 bar;
	bar = ((uint32)&bar) & 0xFFFFF000;
	if (bar != fCachedStack) {
		if (fOwner != find_thread(NULL)) {
			SERIAL_PRINT(("check_lock2 (%s), owner=%d, this=%d, count=%d\n",
				Name(), LockingThread(), find_thread(NULL), 
				CountLocks()));
			debugger(lock_error);
		} else {
			fCachedStack = bar;
		}
	}
}

/*---------------------------------------------------------------*/

port_id _get_looper_port_(const BLooper *loop)
{
	return loop->fMsgPort;
}

/*---------------------------------------------------------------*/

thread_id BLooper::LockingThread() const
	{ return fOwner; }

/*---------------------------------------------------------------*/

bool BLooper::IsLocked() const
{
	BAutolock autolock(sLooperListLock);

	if (!autolock.IsLocked()) {
		printf("IsLocked() called after main() returned. Can't have static Loopers.\n");
		return false;
	}

	if (!IsLooperValid(this)) {
		return false;
	}
	
	uint32 stack;
	stack = ((uint32)&stack) & 0xFFFFF000;
	if (fCachedStack == stack) return true;
	if (fOwner == find_thread(NULL)) {
		const_cast<BLooper*>(this)->fCachedStack = stack;
		return true;
	}

	return false;
}

/*---------------------------------------------------------------*/

int32 BLooper::CountLocks() const
	{ return fOwnerCount; }

/*---------------------------------------------------------------*/

int32 BLooper::CountLockRequests() const
	{ return fAtomicCount; }

/*---------------------------------------------------------------*/

sem_id BLooper::Sem() const
{
#if SUPPORTS_LOCK_DEBUG
	if (LockDebugLevel())
		return reinterpret_cast<DebugLock*>(fLockSem)->Semaphore();
	else
#endif
		return fLockSem;
}

/*-------------------------------------------------------------*/

BHandler *BLooper::resolve_specifier(BHandler *target, BMessage *msg)
{
	BHandler	*new_target = target;
	status_t	err;
	int32		cur_index;

//+	PRINT(("resolve_specifier(%s, class=%s)\n", Name(), class_name(this)));
	/*
	 Loop on the specifiers. Several levels of specifiers might be handled
	 within the same looper (e.g. Button 4 of View 3 of ...). That's why
	 we loop.
	*/
	do {
		BMessage	specifier;
		const char	*property;
		int32		form;
		err = msg->GetCurrentSpecifier(&cur_index, &specifier, &form,
			&property);
		if (err) {
//+			PRINT(("err=%x (%s), curi=%d\n", err, strerror(err), cur_index));
			BMessage	reply(B_REPLY);
			reply.AddInt32("error", err);
			msg->SendReply(&reply);
			new_target = NULL;
			break;
		}
		ASSERT(property);

		target = new_target;

//+		PRINT(("Calling ResolveSpecifier, cur_index=%d, class=%s, prop=%s, form=%d\n",
//+			cur_index, class_name(target), property, form));
		new_target = target->ResolveSpecifier(msg, cur_index, &specifier,
			form, property);
//+		PRINT(("	returned: class=%s\n",
//+			new_target ? class_name(new_target) : "null"));
		/*
		 If new_target is NULL we will break out of this loop and this
		 looper will do nothing with this message. It is assumed that
		 the message was forwarded to the appropriate entity.
		*/

		/*
		 Race condition here. If new_target is under the control
		 of another looper then it could die at any moment! Even
		 calling Looper() above is dangerous!  For this reason we have to
		 use BLooper::IndexOf(BHandler*), which doesn't need to derefence
		 the handler pointer. If the Handler belongs to this looper, then
		 we're safe, it can't get deleted while the looper is locked.
		*/
		if (new_target && (IndexOf(new_target) < 0)) {
			// returning such a Handler is an error. ResolveSpecifier must
			// return either NULL or a handler that belongs to this looper.
			debugger("Handler returned by ResolveSpecifier does not belong to this looper");
			new_target = NULL;
		}

		msg->GetCurrentSpecifier(&cur_index);

	} while (new_target &&
			(new_target != target) &&
			(cur_index >= 0));

//+	if (new_target)
//+		PRINT(("resolve_spec, final target=%s, class=%s\n",
//+			new_target->Name(), class_name(new_target)));
//+	else
//+		PRINT(("resolve_spec, no target\n"));

	return new_target;
}

/*-------------------------------------------------------------*/

#if _SUPPORTS_FEATURE_SCRIPTING
enum {
	_NO_TAG_ = -1,
	_FOR_ME_ = 0,
	_HANDLER_IDX_
};

static property_info prop_list[] = {
	{"Handler",
		{},
		{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		NULL, _HANDLER_IDX_,
		{},
		{},
		{}
	},
	{"Handlers",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, _FOR_ME_,
		{B_MESSENGER_TYPE},
		{},
		{}
	},
	{"Handler",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		NULL, _FOR_ME_,
		{B_INT32_TYPE},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};
#endif

BHandler *BLooper::ResolveSpecifier(BMessage *_SCRIPTING_ONLY(msg), int32 _SCRIPTING_ONLY(index),
	BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form), const char *_SCRIPTING_ONLY(property))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t	err = B_OK;
	BHandler	*target = this;
	BMessage	error_msg(B_MESSAGE_NOT_UNDERSTOOD);
	int32		match;
	int32		tag = _NO_TAG_;

//+	PRINT(("BLooper::Resolve: msg->what=%.4s, index=%d, form=0x%x, prop=%s\n",
//+		(char*) &(msg->what), index, spec->what, property));

	BPropertyInfo	pi(prop_list);

	match = pi.FindMatch(msg, index, spec, form, property, &tag);
	switch (tag) {
	case _NO_TAG_: {
		ASSERT(match < 0);
		target = _inherited::ResolveSpecifier(msg,index,spec,form,property);
		break;
	}
	case _FOR_ME_: {
		target = this;
		break;
	}
	case _HANDLER_IDX_: {
		int32	index = spec->FindInt32("index");
		if (form == B_REVERSE_INDEX_SPECIFIER)
			index = CountHandlers() - index;
		BHandler *hand = HandlerAt(index);
		if (hand) {
			msg->PopSpecifier();
			target = hand;
		} else {
			err = B_BAD_INDEX;
			error_msg.AddString("message", "handler index out of range");
		}
		break;
	}
	}

	if (err) {
		error_msg.AddInt32("error", err);
		msg->SendReply(&error_msg);
		target = NULL;
	}

	return target;
#else
	return NULL;
#endif
}

/*-------------------------------------------------------------*/

status_t BLooper::GetSupportedSuites(BMessage *_SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", "suite/vnd.Be-looper");
	BPropertyInfo	pi(prop_list);
	data->AddFlat("messages", &pi);
	return _inherited::GetSupportedSuites(data);
#else
	return B_UNSUPPORTED;
#endif
}

/*-------------------------------------------------------------*/

BLooper::BLooper(const BLooper &)
	:	BHandler()
	{}
BLooper &BLooper::operator=(const BLooper &) { return *this; }

/*----------------------------------------------------------------*/

status_t BLooper::Perform(perform_code d, void *arg)
{
	return _inherited::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

inline status_t _call_ready_to_loop_(BLooper* This, BLooper::loop_state* outState)
{
	return This->BLooper::ReadyToLoop(outState);
}

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT status_t
	#if __GNUC__
	_ReservedLooper1__7BLooper
	#elif __MWERKS__
	_ReservedLooper1__7BLooperFv
	#endif
	(BLooper* This, BLooper::loop_state* outState)
	{
		return _call_ready_to_loop_(This, outState);
	}
}
#endif

//void BLooper::_ReservedLooper1() {}
void BLooper::_ReservedLooper2() {}
void BLooper::_ReservedLooper3() {}
void BLooper::_ReservedLooper4() {}
void BLooper::_ReservedLooper5() {}
void BLooper::_ReservedLooper6() {}

// --------- Deprecated BLocker methods 02/2001 (Dano?) ---------

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT status_t
	#if __GNUC__
	LockWithTimeout__7BLooperx
	#elif __MWERKS__
	LockWithTimeout__7BLooperFx
	#endif
	(BLooper* This, bigtime_t timeout)
	{
		return This->LockWithTimeout(timeout);
	}
	
	_EXPORT lock_status_t
	#if __GNUC__
	LockWithTimeout__7BLooperxb
	#elif __MWERKS__
	LockWithTimeout__7BLooperFxb
	#endif
	(BLooper* This, bigtime_t timeout, bool)
	{
		return This->LockWithTimeout(timeout, 0);
	}
	
}
#endif

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
