
#include <support2/Team.h>
#include <support2/Handler.h>
#include <support2/Looper.h>
#include <support2/StdIO.h>
#include <support2_p/BinderIPC.h>
#include <image.h>

#include <stdio.h>

//#define HANDLE_DEBUG_MSGS 1

namespace B {
namespace Support2 {

using namespace B::Private;

/**************************************************************************************/

#if BINDER_DEBUG_LIB
static int32 nextFakeTeamIDDelta = 1;
struct root_wrapper_info {
	team_id	fake_team_id;
	root_object_func func;
	BValue params;
};

void root_wrapper(root_wrapper_info* info)
{
	atom_ptr<BTeam> team(new BTeam(info->fake_team_id));
	BLooper::InitMain(team);
	BLooper::SetRootObject(info->func(info->params));
	BLooper::Loop();
}
#endif

IBinder::ptr load_object(const char *pathname, const BValue& params)
{
	IBinder::ptr (*startRootFunc)(const BValue &, image_id);
	image_id addon = load_add_on(pathname);
	if (addon < 0) return NULL;

	status_t err = get_image_symbol(addon,"_start_root_",B_SYMBOL_TYPE_TEXT,reinterpret_cast<void**>(&startRootFunc));
	if (err < 0) return NULL;
	return startRootFunc(params, addon);
};

IBinder::ptr load_object_remote(const char *pathname, const BValue& params)
{
	#if BINDER_DEBUG_LIB
		image_id addon = load_add_on(pathname);
		if (addon < 0) return NULL;
	
		root_wrapper_info info;
		info.fake_team_id = this_team()+atomic_add(&nextFakeTeamIDDelta,1);
		info.params = params;
		
		status_t err = get_image_symbol(addon,"root",B_SYMBOL_TYPE_TEXT,reinterpret_cast<void**>(&info.func));
		if (err < 0) return NULL;
		thread_id thid = spawn_thread((thread_entry)root_wrapper,"root wrapper",B_NORMAL_PRIORITY,&info);
		if (thid < 0) return NULL;
		return BLooper::GetRootObject(thid, info.fake_team_id);
	#else
		#warning load_object_remote() needs to propagate parameters
		const char *argv[2];
		argv[0] = "/system/servers/stub";
		argv[1] = pathname;
		thread_id team = load_image(2,argv,environ);
		return BLooper::GetRootObject(team);
	#endif
}

/**************************************************************************************/

BTeam::BTeam(team_id tid)
	:	m_id(tid), m_lock("BTeam"),
		m_pendingHandlers(NULL), m_nextEventTime(B_INFINITE_TIMEOUT)
{
	printf("************* Creating BTeam %p (id %ld) *************\n", this, tid);
}

BTeam::~BTeam()
{
}

int32 BTeam::ID() const
{
	return m_id;
}

status_t BTeam::Acquired(const void*)
{
	return B_OK;
}

status_t BTeam::Released(const void*)
{
	return B_OK;
}

bool BTeam::InsertHandler(BHandler **handlerP, BHandler *handler)
{
	BHandler *p = *handlerP;

	if (!p) {
		*handlerP = handler;
		handler->m_next = handler->m_prev = NULL;
		return true;
	}

	const bigtime_t msgTime = handler->NextMessageTime();
	if (p->NextMessageTime() > msgTime) {
		(*handlerP)->m_prev = handler;
		*handlerP = handler;
		handler->m_next = p;
		handler->m_prev = NULL;
	} else {
		while (p->m_next && (p->m_next->NextMessageTime() < msgTime)) p = p->m_next;
		handler->m_next = p->m_next;
		handler->m_prev = p;
		if (p->m_next) p->m_next->m_prev = handler;
		p->m_next = handler;
	}
	return false;
}

void BTeam::UnscheduleHandler(BHandler *handler, bool lock)
{
	if (lock) m_lock.Lock();

	const bool hadRef =
		((m_pendingHandlers == handler) || handler->m_prev || handler->m_next);

	if (hadRef) {
		if (m_pendingHandlers == handler)	m_pendingHandlers = handler->m_next;
		if (handler->m_next)				handler->m_next->m_prev = handler->m_prev;
		if (handler->m_prev)				handler->m_prev->m_next = handler->m_next;
		handler->m_prev = handler->m_next = NULL;
	}

	if (lock) m_lock.Unlock();

	if (hadRef) handler->DecRefs(this);
}

void BTeam::ScheduleNextEvent()
{
	const bigtime_t nextTime	= m_pendingHandlers
								? m_pendingHandlers->NextMessageTime()
								: B_INFINITE_TIMEOUT;
	
	if (nextTime != m_nextEventTime) {
		m_nextEventTime = nextTime;
		BLooper::_SetNextEventTime(m_nextEventTime);
	}
}

void BTeam::ScheduleHandler(BHandler *handler)
{
	m_lock.Lock();
	const BHandler::scheduling s = handler->StartSchedule();
	if (s != BHandler::CANCEL_SCHEDULE) {
		handler->IncRefs(this);
		UnscheduleHandler(handler,false);
		InsertHandler(&m_pendingHandlers,handler);
		handler->DoneSchedule();
	} else {
		UnscheduleHandler(handler,false);
	}
	
	ScheduleNextEvent();
	m_lock.Unlock();
}

void BTeam::DispatchMessage()
{
	BHandler* handler;
	bool firstTime = true;
	
	do {
		m_lock.Lock();
		if ((handler = m_pendingHandlers) != NULL
				&& handler->NextMessageTime() <= system_time()) {
			m_pendingHandlers = handler->m_next;
			handler->m_next = handler->m_prev = NULL;
			if (m_pendingHandlers) m_pendingHandlers->m_prev = NULL;
			handler->DeferScheduling();
		} else {
			handler = NULL;
		}
		
		// The binder driver clears its event time when processing an event,
		// so we always must inform it if there is another one to schedule.
		if (firstTime) m_nextEventTime = B_INFINITE_TIMEOUT;
		
		// Inform the binder of next event time if: this is the first time
		// through the loop (so that other BLooper threads can be activated
		// to execute in other handlers); or if this is the last time through
		// the loop so we can make sure the binder is up-to-date about when
		// the next event is to occur.
		if (firstTime || !handler) {
			ScheduleNextEvent();
			firstTime = false;
		}
		m_lock.Unlock();
		
		if (handler) {
			if (handler->AttemptAcquire(this)) {
				handler->DispatchMessage();
				handler->Release(this);
			}
			handler->DecRefs(this);
		}
	} while (handler);
}

bool BTeam::ResumingScheduling()
{
	return BLooper::_ResumingScheduling();
}

void BTeam::ClearSchedulingResumed()
{
	BLooper::_ClearSchedulingResumed();
}

IBinder* & BTeam::BinderForHandle(int32 handle)
{
	if (m_handleRefs.CountItems() <= (uint32)handle) {
		m_handleRefs.SetCapacity(handle+1);
		while (m_handleRefs.CountItems() <= (uint32)handle) m_handleRefs.AddItem(NULL);
	}
	return m_handleRefs.EditItemAt(handle);
}

IBinder::ptr
BTeam::GetProxyForHandle(int32 handle)
{
	IBinder::ptr r;
	m_handleRefLock.Lock();
	IBinder* &b = BinderForHandle(handle);
	if (b == NULL) {
		b = new RBinder(handle);
		r = b;
#if HANDLE_DEBUG_MSGS
		bout << "BTeam Creating RBinder " << b << " for handle " << handle << endl;
#endif
	} else {
		// This little bit of nastyness is to allow us to add a primary
		// reference to the remote proxy when this team doesn't have one
		// but another team is sending the handle to us.
		b->ForceAcquire(this);
		r = b;
		b->Release(this);
	}
	m_handleRefLock.Unlock();
	return r;
}

IBinder::ref
BTeam::GetProxyRefForHandle(int32 handle)
{
	IBinder::ref r;
	m_handleRefLock.Lock();
	IBinder* &b = BinderForHandle(handle);
	if (b == NULL) {
		b = new RBinder(handle);
#if HANDLE_DEBUG_MSGS
		bout << "BTeam Creating RBinder " << b << " for handle " << handle << endl;
#endif
	}
	r = b;
	m_handleRefLock.Unlock();
	return r;
}

void
BTeam::ExpungeHandle(int32 handle)
{
	m_handleRefLock.Lock();
	IBinder* &b = BinderForHandle(handle);
#if HANDLE_DEBUG_MSGS
	bout << "BTeam Expunging RBinder " << b << " for handle " << handle << endl;
#endif
	b = NULL;
	m_handleRefLock.Unlock();
}

} } // namespace B::Support2
