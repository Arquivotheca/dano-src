
#include <support2/Handler.h>

#include <support2/Autolock.h>
#include <support2/Looper.h>
#include <support2/Message.h>
#include <support2/StdIO.h>
#include <support2/Team.h>
#include <support2_p/BinderKeys.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <new>

namespace B {
namespace Support2 {

using namespace B::Private;

/**************************************************************************************/

status_t IHandler::PostMessageAtTime(	const BMessage &message,
										bigtime_t absoluteTime,
										IHandler::arg replyTo,
										uint32 /*flags*/)
{
	BMessage* msg = new(std::nothrow) BMessage(message);
	if (msg) {
		msg->SetWhen(absoluteTime);
		msg->SetReplyTo(replyTo);
		return EnqueueMessage(msg);
	}
	return B_NO_MEMORY;
}

status_t IHandler::PostDelayedMessage(	const BMessage &message,
										bigtime_t delay,
										IHandler::arg replyTo,
										uint32 flags)
{
	return PostMessageAtTime(message, system_time() + (delay >= 0 ? delay : 0), replyTo, flags);
}

status_t IHandler::PostMessage(	const BMessage &message,
								IHandler::arg replyTo,
								uint32 flags)
{
	return PostMessageAtTime(message, system_time(), replyTo, flags);
}

IHandler::~IHandler()
{
}

/**************************************************************************************/

class RHandler : public RInterface<IHandler>
{
public:

	RHandler(IBinder::arg o) : RInterface<IHandler>(o) {}

	virtual void RedirectMessagesTo(IHandler::arg new_dest) {
		Remote()->Put(BValue(g_keyRedirectMessagesTo, new_dest->AsBinder()));
	}

	virtual	status_t EnqueueMessage(BMessage* message)
	{
		const status_t result = Remote()->Put(BValue(g_keyPost, message->AsValue()));
		delete message;
		return result;
	}
};

/**************************************************************************************/

const BValue IHandler::descriptor(BValue::TypeInfo(typeid(IHandler)));
B_IMPLEMENT_META_INTERFACE(Handler)

/**************************************************************************************/

enum {
	ghCanSchedule 		= 0x00000001,
	ghScheduled			= 0x00000002,
	ghNeedSchedule		= 0x00000004,
	ghDying				= 0x00000008,
	ghCalledSchedule	= 0x00000010
};

/**************************************************************************************/

status_t 
LHandler::Told(value &val)
{
	BValue v;
	if ((v = val[g_keyPost]).IsDefined()) {
		BMessage* msg = new(std::nothrow) BMessage(v);
		if (msg) return EnqueueMessage(msg);
		return B_NO_MEMORY;
	}
	if ((v = val[g_keyRedirectMessagesTo]).IsDefined()) {
		IHandler::ptr new_dest = IHandler::AsInterface(v);
		RedirectMessagesTo(new_dest);
	}
	return B_OK;
}

status_t 
LHandler::Asked(const value &, value &)
{
	Push(BValue(IHandler::descriptor, BValue::Binder(this)));
	#warning Implement LHandler::Asked()
	return B_UNSUPPORTED;
}

/**************************************************************************************/

BHandler::BHandler()
	:	m_team(BLooper::Team()), m_lock("Some BHandler"),
		m_state(ghCanSchedule), m_next(NULL), m_prev(NULL)
{
}

BHandler::~BHandler()
{
}

/**************************************************************************************/

status_t BHandler::Acquired(const void* id)
{
	return BBinder::Acquired(id);
};

status_t BHandler::Released(const void* id)
{
	Unschedule();
	ClearMessages();
	return BBinder::Released(id);
};

status_t 
BHandler::EnqueueMessage(BMessage *msg)
{
	bool doSchedule = false;

	m_lock.Lock();
	if (m_redirectedTo == NULL) {
		m_msgQueue.EnqueueMessage(msg);
		if (m_msgQueue.Head() == msg) {
				doSchedule =
					((m_state & (ghCanSchedule|ghNeedSchedule|ghCalledSchedule)) == ghCanSchedule);
				m_state |= (doSchedule ? ghNeedSchedule|ghCalledSchedule : ghNeedSchedule);
		}
	}
	else {
		m_redirectedTo->EnqueueMessage(msg);
	}
	m_lock.Unlock();

	if (doSchedule) m_team->ScheduleHandler(this);

	return B_OK;
}

void
BHandler::RedirectMessagesTo(IHandler::arg new_dest)
{
	if (new_dest.ptr() != this) {
		Unschedule();
		BAutolock _autolock(m_lock.Lock());
	
		m_redirectedTo = new_dest;
		
		for (BMessage *move_me = m_msgQueue.RemoveTail();
		     move_me;
		     move_me = m_msgQueue.RemoveTail())
		{
			new_dest->EnqueueMessage(move_me);
		}
	}
}

status_t 
BHandler::HandleMessage(const BMessage &)
{
	return B_ERROR;
}

/**************************************************************************************/

bigtime_t BHandler::NextMessageTime()
{
	BAutolock _auto1(m_lock.Lock());
	return m_msgQueue.OldestMessage();
}

status_t BHandler::DispatchMessage()
{
	bool more = false;

	m_lock.Lock();
	BMessage *msg = m_msgQueue.DequeueMessage(B_ANY_WHAT,&more);
	if (more) m_state |= ghNeedSchedule;
	m_lock.Unlock();

	if (!msg) return B_ERROR;

	status_t err = HandleMessage(*msg);

	if (err != B_OK) {
		berr << "Error handling: " << *msg << endl;
	};

	delete msg;

	ResumeScheduling();
	BTeam::ClearSchedulingResumed();
	return B_OK;
}

/**************************************************************************************/

void BHandler::DeferScheduling()
{
	BAutolock _auto1(m_lock.Lock());
	m_state &= ~(ghScheduled|ghCanSchedule);
}

void BHandler::ResumeScheduling()
{
	if (BTeam::ResumingScheduling()) {
		m_lock.Lock();
		const bool doSchedule =
			((m_state & (ghNeedSchedule|ghCalledSchedule)) == ghNeedSchedule);
		m_state |= (doSchedule ? ghCanSchedule|ghCalledSchedule : ghCanSchedule);
		m_lock.Unlock();
		if (doSchedule) m_team->ScheduleHandler(this);
	}
}

/**************************************************************************************/

BHandler::scheduling BHandler::StartSchedule()
{
	BAutolock _auto1(m_lock.Lock());
	m_state &= ~ghCalledSchedule;
	if ((m_state & (ghCanSchedule|ghNeedSchedule|ghDying))
				== (ghCanSchedule|ghNeedSchedule)) {
		m_state &= ~ghNeedSchedule;
		if (m_msgQueue.IsEmpty()) return CANCEL_SCHEDULE;
		if (m_state & ghScheduled) return DO_RESCHEDULE;
		return DO_SCHEDULE;
	};
	return CANCEL_SCHEDULE;
}

void BHandler::DoneSchedule()
{
	BAutolock _auto1(m_lock.Lock());
	m_state |= ghScheduled;
}

/**************************************************************************************/

BMessage * BHandler::DequeueMessage(uint32 what)
{
	bool more,doSchedule=false;

	m_lock.Lock();

	const BMessage *oldFirst = m_msgQueue.Head();
	BMessage *msg = m_msgQueue.DequeueMessage(what,&more);
	if (m_msgQueue.Head() != oldFirst) {
		doSchedule = ((m_state & (ghCanSchedule|ghNeedSchedule|ghCalledSchedule)) == ghCanSchedule);
		m_state |= (doSchedule ? ghNeedSchedule|ghCalledSchedule : ghNeedSchedule);
	}

	m_lock.Unlock();

	if (doSchedule) m_team->ScheduleHandler(this);
	return msg;
};

int32 BHandler::CountMessages(uint32 what)
{
	BAutolock _auto1(m_lock.Lock());
	return m_msgQueue.CountMessages(what);
}

void BHandler::DumpMessages()
{
	Unschedule();
	while (!DispatchMessage());
};

void BHandler::ClearMessages()
{
	BMessageList tmp;
	m_lock.Lock();
	tmp.Adopt(m_msgQueue);
	uint32 state = m_state;
	m_state &= ~ghScheduled;
	m_lock.Unlock();

	tmp.MakeEmpty();
	
	if (state & ghScheduled) m_team->UnscheduleHandler(this);
};

void BHandler::Unschedule()
{
	m_lock.Lock();
	uint32 state = m_state;
	m_state &= ~(ghCanSchedule|ghScheduled);
	m_state |= ghDying;
	m_lock.Unlock();

	if (state & ghScheduled) m_team->UnscheduleHandler(this);
}

} }	// namespace B::Support2
