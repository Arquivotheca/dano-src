
#include <TLS.h>
#include <GDispatcher.h>
#include <GLooper.h>

#include <stdio.h>
#include <malloc.h>
#include <message_util.h>

static Gehnaphore GLooper_TLS_Lock;
static int32 GLooper_TLS = -1;
extern int32 send_msg_proc_TLS;
typedef status_t (*send_msg_proc)(port_id, void *, int32, uint32, int64, bool);

status_t GLooper::Register()
{
	if (!(atomic_or(&m_flags,1) & 1)) {
		m_dispatcher->RegisterGLooper(this);
	}
	
	return B_OK;
}

status_t GLooper::Unregister()
{
	if (atomic_and(&m_flags,~1) & 1) {
		m_dispatcher->UnregisterGLooper(this);
	}

	return B_OK;
}

GLooper *
GLooper::This()
{
	return (GLooper*)tls_get(GLooper_TLS);
}

bool 
GLooper::ResumingScheduling()
{
	GLooper *t = This();
	if (t) return !(atomic_or(&t->m_flags,2) & 2);
	else return false;
}

void 
GLooper::ClearSchedulingResumed()
{
	GLooper *t = This();
	if (t) atomic_and(&t->m_flags,~2);
}

status_t 
GLooper::RegisterThis()
{
	GLooper *t = This();
	if (t) t->Register();
	else return B_ERROR;
	return B_OK;
}

status_t 
GLooper::UnregisterThis()
{
	GLooper *t = This();
	if (t) t->Unregister();
	else return B_ERROR;
	return B_OK;
}

GLooper::GLooper(GDispatcher *dispatcher, const char *name)
{
	m_flags = 0;
	if (!name) name = "GLooper";
	m_dispatcher = dispatcher;
	Register();
	m_thid = spawn_thread(LaunchThread,"GLooper",B_NORMAL_PRIORITY,this);
	resume_thread(m_thid);
}

GLooper::~GLooper()
{
	Unregister();
}

status_t GLooper::send_msg_proc(port_id port, void *buf, int32 size, uint32 flags, int64 timeout, bool synchronousReply)
{
	if (synchronousReply) {
		GLooper *looper = This();
		if (looper && looper->m_dispatcher && (looper->m_dispatcher->Port()==port))
			looper->m_dispatcher->MaybeSpawn();
	}
	status_t err;
	do {
		err= write_port_etc(port, STD_SEND_MSG_CODE, buf, size, flags, timeout);
	} while (err == B_INTERRUPTED);
	return err;
}

int32 GLooper::Thread()
{
	if (GLooper_TLS == -1) {
		GLooper_TLS_Lock.Lock();
		if (GLooper_TLS == -1) GLooper_TLS = tls_allocate();
		GLooper_TLS_Lock.Unlock();
	}
	
	tls_set(send_msg_proc_TLS, (void*)(&send_msg_proc));
	tls_set(GLooper_TLS,(GLooper*)this);

	int32 err;
	while ((err = m_dispatcher->DispatchMessage()) == B_OK);
	delete this;
	return err;
}

int32 GLooper::LaunchThread(void *instance)
{
	return ((GLooper*)instance)->Thread();
}
