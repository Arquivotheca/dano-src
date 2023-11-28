
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <GHandler.h>
#include <GLooper.h>
#include <GDispatcher.h>
#include <DelayedDeleteList.h>
#include "token.h"

#define ghCanSchedule		0x00000001
#define ghScheduled			0x00000002
#define ghNeedSchedule		0x00000004
#define ghDying				0x00000008
#define ghCalledSchedule	0x00000010

dispatcher	GHandler::g_defaultDispatcher;
int32		GHandler::g_defaultDispatcherCreated = 0;

void GHandler::Init(GDispatcher *dispatcher, const char *name)
{
	if (!dispatcher) {
		if (!atomic_or(&g_defaultDispatcherCreated,1))
			g_defaultDispatcher = new GDispatcher();
		while (((GDispatcher*)g_defaultDispatcher) == NULL) snooze(20000);
		dispatcher = g_defaultDispatcher;
	};

	m_dispatcher = dispatcher;
	m_name = strdup(name);
	m_state = ghCanSchedule;
	m_dispatcher->IncRefs();
	m_token = NO_TOKEN;
	m_next = m_prev = NULL;
}

GHandler::GHandler(GDispatcher *dispatcher, const char *name)
{
	Init(dispatcher,name);
}

GHandler::GHandler(const char *name)
{
	Init(NULL,name);
}

void GHandler::Cleanup()
{
	int32 token;
	{
		GehnaphoreAutoLock _auto1(m_lock);
		token = m_token;
		m_token = NO_TOKEN - 1;
	}

	if ((token != NO_TOKEN - 1) && (token != NO_TOKEN))
		m_dispatcher->InvalidateToken(token);

	Unschedule();
	ClearMessages();
};

GHandler::~GHandler()
{
	if (m_name) free(m_name);
	m_dispatcher->DecRefs();
}

bigtime_t GHandler::NextMessage()
{
	GehnaphoreAutoLock _auto1(m_lock);
	return m_msgQueue.OldestMessage();
}

void GHandler::DeferScheduling()
{
	GehnaphoreAutoLock _auto1(m_lock);
	m_state &= ~(ghScheduled|ghCanSchedule);
}

void GHandler::ResumeScheduling()
{
	if (GLooper::ResumingScheduling()) {
		m_lock.Lock();
		bool doSchedule = ((m_state & (ghNeedSchedule|ghCalledSchedule)) == ghNeedSchedule);
		m_state |= (doSchedule ? ghCanSchedule|ghCalledSchedule : ghCanSchedule);
		m_lock.Unlock();
		if (doSchedule) m_dispatcher->ScheduleHandler(this);
	}
}

scheduling GHandler::StartSchedule()
{
	GehnaphoreAutoLock _auto1(m_lock);
	m_state &= ~ghCalledSchedule;
	if ((m_state & (ghCanSchedule|ghNeedSchedule|ghDying)) == (ghCanSchedule|ghNeedSchedule)) {
		m_state &= ~ghNeedSchedule;
		if (!m_msgQueue.PeekMessage()) return CANCEL_SCHEDULE;
		if (m_state & ghScheduled) return DO_RESCHEDULE;
		return DO_SCHEDULE;
	};
	return CANCEL_SCHEDULE;
}

void GHandler::DoneSchedule()
{
	GehnaphoreAutoLock _auto1(m_lock);
	m_state |= ghScheduled;
}

void GHandler::PostMessageAtTime(BMessage *message, bigtime_t absoluteTime)
{
	int32 oldVal;
	bool doSchedule = false;
	bool cleanedup = false;
	
	m_lock.Lock();

	cleanedup = CleanupCalled();
	
	if (!cleanedup) {
		m_msgQueue.QueueMessage(message,absoluteTime);
		if (m_msgQueue.PeekMessage() == message) {
			doSchedule = ((m_state & (ghCanSchedule|ghNeedSchedule|ghCalledSchedule)) == ghCanSchedule);
			m_state |= (doSchedule ? ghNeedSchedule|ghCalledSchedule : ghNeedSchedule);
		}
	}
	
	m_lock.Unlock();

	if (cleanedup) delete message;
	
	if (doSchedule) m_dispatcher->ScheduleHandler(this);
}

void GHandler::PostDelayedMessage(BMessage *message, bigtime_t delay)
{
	PostMessageAtTime(message,system_time() + delay);
}

void GHandler::PostMessage(BMessage *msg)
{
	PostDelayedMessage(msg,0);
};


void 
GHandler::PostMessage(uint32 message_constant)
{
	BMessage msg(message_constant);
	PostMessage(&msg);
}


status_t GHandler::DispatchMessage()
{
	bool more = false;

	m_lock.Lock();
	BMessage *msg = m_msgQueue.DequeueMessage(ANY_WHAT,&more);
	if (more) m_state |= ghNeedSchedule;
	m_lock.Unlock();

	if (!msg) return B_ERROR;

	status_t err = B_ERROR;
//	if(AttemptAcquire()) {
		err = HandleMessage(msg);
//		Release();
//	}

	if (err != B_OK) {
		printf("Error handling message: ");
		msg->PrintToStream();
	};

	delete msg;

	ResumeScheduling();
	GLooper::ClearSchedulingResumed();
	return B_OK;
}

void GHandler::Unschedule()
{
	bool doSchedule = false;

	m_lock.Lock();
	uint32 state = m_state;
	m_state &= ~(ghCanSchedule|ghScheduled);
	m_state |= ghDying;
	if ((state & (ghScheduled|ghCalledSchedule)) == ghScheduled) {
		m_state |= ghCalledSchedule;
		doSchedule = true;
	}
	m_lock.Unlock();

	if (doSchedule) m_dispatcher->ScheduleHandler(this);
}

void GHandler::DumpMessages()
{
	Unschedule();
	while (!DispatchMessage());
};

BMessage * GHandler::DequeueMessage(uint32 what)
{
	bool more,doSchedule=false;
	int32 oldVal;

	m_lock.Lock();

	BMessage *oldFirst = m_msgQueue.PeekMessage();
	BMessage *msg = m_msgQueue.DequeueMessage(what,&more);
	if (m_msgQueue.PeekMessage() != oldFirst) {
		doSchedule = ((m_state & (ghCanSchedule|ghNeedSchedule|ghCalledSchedule)) == ghCanSchedule);
		m_state |= (doSchedule ? ghNeedSchedule|ghCalledSchedule : ghNeedSchedule);
	}

	m_lock.Unlock();

	if (doSchedule) m_dispatcher->ScheduleHandler(this);
	return msg;
};

int32 GHandler::CountMessages(uint32 what)
{
	GehnaphoreAutoLock _auto1(m_lock);
	return m_msgQueue.CountMessages(what);
}

void GHandler::ClearMessages()
{
	bool more, doSchedule = false;
	DelayedDeleteList<BMessage> delete_list;
	
	BMessage *msg;
	m_lock.Lock();
	while ((msg = m_msgQueue.DequeueMessage(ANY_WHAT,&more)) != NULL) 
		delete_list.Delete(msg);
	
	uint32 state = m_state;
	m_state &= ~ghScheduled;
	if ((state & (ghScheduled|ghCalledSchedule)) == ghScheduled) {
		m_state |= ghCalledSchedule;
		doSchedule = true;
	}
	m_lock.Unlock();
	
	if (doSchedule) m_dispatcher->ScheduleHandler(this);
};

status_t GHandler::HandleMessage(BMessage */*message*/)
{
	return B_OK;
}

const char * GHandler::Name()
{
	return m_name;
}

port_id GHandler::Port()
{
	return m_dispatcher->Port();
}

int32 GHandler::Token()
{
	GehnaphoreAutoLock _auto1(m_lock);
	if (m_token == (NO_TOKEN - 1))
		return NO_TOKEN;
	if (m_token == NO_TOKEN) m_token = m_dispatcher->ObtainToken(this);
	return m_token;
}

static bool get_handler_token(int16 /*type*/, void* data)
{
	((GHandler*)data)->IncRefs(gDefaultTokens);
	return true;
}

atomref<GHandler> GHandler::LookupByToken(int32 token)
{
	if (token != NO_TOKEN) {
		GHandler* h = NULL;
		if (gDefaultTokens->GetToken(token, 100,
									 (void**)&h, get_handler_token) >= B_OK) {
			atomref<GHandler> ar = h;
			h->DecRefs();
			return ar;
		}
	}
	
	return NULL;
}
