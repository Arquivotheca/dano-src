
#include <support2/Looper.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <support2/Parcel.h>
#include <support2/Debug.h>
#include <support2/TLS.h>
#include <support2/StdIO.h>
#include <support2/Locker.h>
#include <support2/Team.h>
#include <support2/StringIO.h>
#include <support2/ITextStream.h>
#include <support2/Value.h>
#include <support2/Vector.h>
#include <support2/KernelStreams.h>
#include <support2_p/BinderIPC.h>
#include <driver/binder2_driver.h>

#if BINDER_DEBUG_LIB
#include "BinderStreams.h"
#define BINDERISTR BBinderIStr
#define BINDEROSTR BBinderOStr
#else
#define BINDERISTR BKernelIStr
#define BINDEROSTR BKernelOStr
#endif

//#define BINDER_REFCOUNT_MSGS 1
//#define BINDER_BUFFER_MSGS 1
//#define BINDER_DEBUG_MSGS 1

namespace B {
namespace Support2 {

enum {
	kSchedulingResumed	= 0x00000001,
	kSyncNeeded			= 0x00000002,
};

static BTeam* g_defaultTeam = NULL;
static vint32 g_haveDefaultTeam = 0;
static inline atom_ptr<BTeam> default_team()
{
	if ((g_haveDefaultTeam&2) != 2) {
		if (atomic_or(&g_haveDefaultTeam, 1) == 0) {
			thread_info thinfo;
			get_thread_info(find_thread(NULL),&thinfo);
			g_defaultTeam = new BTeam(thinfo.team);
			g_defaultTeam->IncRefs(&g_defaultTeam);
			atomic_or(&g_haveDefaultTeam, 2);
		} else {
			while ((g_haveDefaultTeam&2) == 0) snooze(2000);
		}
	}
	
	atom_ptr<BTeam> team;
	if (g_defaultTeam->AttemptAcquire()) {
		team = g_defaultTeam;
		g_defaultTeam->Release();
	}
	return team;
}

static int32 g_openBuffers = 0;

void BLooper::_DeleteSelf(void *blooper)
{
	BLooper *me = (BLooper*)blooper;
	if (me) delete me;
}

BLooper::BLooper(const atom_ptr<BTeam>& team)
	:	m_team(team),
		m_teamID(team->ID()),
		m_thid(find_thread(NULL)),
		m_priority(10),
		m_binderDesc(open_binder(team->ID())),
		m_in(new BINDERISTR(m_binderDesc),256),
		m_out(new BINDEROSTR(m_binderDesc),256),
		m_flags(0)
{
#if BINDER_DEBUG_MSGS
	BStringIO::ptr msg(new BStringIO);
	msg << "BLooper: open binder descriptor ==> " << m_binderDesc << " (" << strerror(m_binderDesc) << ")"
			<< " in thread " << m_thid << endl;
	msg->PrintAndReset(berr);
#endif
	tls_set(TLS,this);
}

BLooper::~BLooper()
{
#if BINDER_DEBUG_MSGS
	BStringIO::ptr msg(new BStringIO);
	msg << "BLooper: closing descriptor <== " << m_binderDesc << " (" << strerror(m_binderDesc) << ")"
			<< " in thread " << m_thid << endl;
	msg->PrintAndReset(berr);
#endif
	tls_set(TLS,NULL);
	close_binder(m_binderDesc);
	m_binderDesc = -1;
}

BLooper * BLooper::This()
{
	BLooper *me = (BLooper*)tls_get(TLS);
	if (me) return me;

	me = new BLooper;
	on_exit_thread(_DeleteSelf,me);
	return me;
}

BLooper * BLooper::This(const atom_ptr<BTeam>& in_team)
{
	BLooper *me = (BLooper*)tls_get(TLS);
	if (me) return me;

	atom_ptr<BTeam> team(in_team == NULL ? default_team() : in_team);

	me = new BLooper(team);
	on_exit_thread(_DeleteSelf,me);
	return me;
}

int32 
BLooper::Thread()
{
	return This()->m_thid;
}

atom_ptr<BTeam>
BLooper::Team()
{
	return This()->m_team;
}

int32 
BLooper::TeamID()
{
	return This()->m_teamID;
}

bool 
BLooper::_ResumingScheduling()
{
	BLooper* loop = This();
	const bool resuming = !(loop->m_flags&kSchedulingResumed);
	if (resuming) loop->m_flags |= kSchedulingResumed;
	return resuming;
}

void 
BLooper::_ClearSchedulingResumed()
{
	This()->m_flags &= ~kSchedulingResumed;
}

status_t 
BLooper::InitMain(const atom_ptr<BTeam>& team)
{
	static bool initMainCalled=false;
	#if !BINDER_DEBUG_LIB
	if (initMainCalled) {
		debugger(
			"BLooper::InitMain() should only be called during global static initialization.\n"
			"Don't call it directly.");
		return B_DONT_DO_THAT;
	}
	#endif
	initMainCalled = true;
	BLooper* loop = This(team);
	loop->m_out.write32(bcSET_THREAD_ENTRY);
	loop->m_out.write32((int32)_DriverLoop);
	loop->m_out.write32((int32)loop);
	return B_OK;
}

status_t 
BLooper::InitOther(const atom_ptr<BTeam>& team)
{
	#if BINDER_DEBUG_LIB
	This(team);
	return B_OK;
	#else
	return B_UNSUPPORTED;
	#endif
}

#if BINDER_DEBUG_LIB
struct spawn_thread_info
{
	BLooper *parent;
	thread_func func;
	void *arg;
};
#endif

status_t BLooper::SpawnThread(thread_func function_name, 
	const char *thread_name, int32 priority, void *arg)
{
	#if BINDER_DEBUG_LIB
	spawn_thread_info* info = (spawn_thread_info*)malloc(sizeof(spawn_thread_info));
	if (info) {
		info->parent = This();
		info->func = function_name;
		info->arg = arg;
		return spawn_thread(_ThreadEntry, thread_name, priority, info);
	}
	return B_NO_MEMORY;
	#else
	return spawn_thread(function_name, thread_name, priority, arg);
	#endif
}

int32 BLooper::_ThreadEntry(void *arg)
{
	spawn_thread_info info = *static_cast<spawn_thread_info*>(arg);
	free(arg);
	atom_ptr<BTeam> team(info.parent ? info.parent->m_team : atom_ptr<BTeam>());
	This(team);
	return info.func(info.arg);
}

status_t BLooper::SpawnLooper()
{
	return resume_thread(spawn_thread((thread_entry)_DriverLoop,"BLooper",B_NORMAL_PRIORITY,This()));
}

status_t BLooper::_Loop(BLooper *parent)
{
	atom_ptr<BTeam> team(parent ? parent->m_team : atom_ptr<BTeam>());
	if (parent) team = parent->m_team;
	return This(team)->_LoopSelf();
}

status_t BLooper::_DriverLoop(BLooper *parent)
{
	atom_ptr<BTeam> team(parent ? parent->m_team : atom_ptr<BTeam>());
	BLooper* loop = This(team);
	loop->m_out.write32(bcREGISTER_LOOPER);
	return loop->_LoopSelf();
}

void BLooper::_SetThreadPriority(int32 priority)
{
	if (m_priority != priority) {
		//printf("*** Changing priority of thread %ld from %ld to %ld\n",
		//		m_thid, m_priority, priority);
		m_priority = priority;
		set_thread_priority(m_thid, m_priority);
	}
}

void BLooper::SetThreadPriority(int32 priority)
{
	This()->_SetThreadPriority(priority);
}

status_t BLooper::Loop()
{
	BLooper* loop = This();
	loop->m_out.write32(bcENTER_LOOPER);
	status_t result = loop->_LoopSelf();
	loop->m_out.write32(bcEXIT_LOOPER);
	loop->m_out.flush();
	return result;
}

#if BINDER_DEBUG_MSGS
static const char *inString[] = {
	"brOK",
	"brTIMEOUT",
	"brWAKEUP",
	"brTRANSACTION",
	"brREPLY",
	"brACQUIRE_RESULT",
	"brTRANSACTION_COMPLETE",
	"brINCREFS",
	"brACQUIRE",
	"brATTEMPT_ACQUIRE",
	"brRELEASE",
	"brDECREFS",
	"brEVENT_OCCURRED",
	"brFINISHED"
};
#endif

IBinder::ptr
BLooper::GetProxyForHandle(int32 handle)
{
	return Team()->GetProxyForHandle(handle);
}

IBinder::ref
BLooper::GetProxyRefForHandle(int32 handle)
{
	return Team()->GetProxyRefForHandle(handle);
}

status_t 
BLooper::ExpungeHandle(int32 handle)
{
	Team()->ExpungeHandle(handle);
	return B_OK;
}

void
BLooper::_SetNextEventTime(bigtime_t when)
{
	TRACE();
	BLooper* l = This();
	ioctl_binder(l->m_binderDesc, BINDER_SET_WAKEUP_TIME, &when, sizeof(when));
}

status_t 
BLooper::_HandleCommand(int32 cmd)
{
	BBinder *ptr;
	status_t result = B_OK;
	
#if BINDER_DEBUG_MSGS
	BStringIO::ptr msg(new BStringIO);
	msg	<< "Thread " << find_thread(NULL)
		<< " got command: " << cmd << " " << inString[cmd] << endl;
	msg->PrintAndReset(berr);
#endif
	switch (cmd) {
		case brERROR: {
			result = m_in.read32();
			printf("*** Binder driver error during read: %s\n", strerror(result));
		} break;
		case brOK: {
		} break;
		case brACQUIRE: {
			ptr = (BBinder*)m_in.read32();
			#if BINDER_REFCOUNT_MSGS
			printf("calling acquire on %p %s (atom %p)\n",ptr,typeid(*ptr).name(),(BAtom*)ptr);
			#endif
			ptr->Acquire(reinterpret_cast<void*>(m_teamID));
		} break;
		case brRELEASE: {
			ptr = (BBinder*)m_in.read32();
			#if BINDER_REFCOUNT_MSGS
			printf("calling release on %p %s (atom %p)\n",ptr,typeid(*ptr).name(),(BAtom*)ptr);
			int32 r =
			#endif
			ptr->Release(reinterpret_cast<void*>(m_teamID));
			#if BINDER_REFCOUNT_MSGS
			printf("called release %ld\n",r);
			if (r > 1) ptr->Report(bout);
			#endif
		} break;
		case brATTEMPT_ACQUIRE: {
			m_priority = m_in.read32();
			ptr = (BBinder*)m_in.read32();
			#if BINDER_REFCOUNT_MSGS
			printf("calling attempt acquire on %p %s (atom %p)\n",ptr,typeid(*ptr).name(),(BAtom*)ptr);
			#endif
			const bool success = ptr->AttemptAcquire(reinterpret_cast<void*>(m_teamID));
			m_out.write32(bcACQUIRE_RESULT);
			m_out.write32((int32)success);
			#if BINDER_REFCOUNT_MSGS
			printf("attempt acquire result on %p: %ld\n",ptr,(int32)success);
			#endif
		} break;
		case brINCREFS: {
			ptr = (BBinder*)m_in.read32();
			#if BINDER_REFCOUNT_MSGS
			printf("calling increfs on %p %s (atom %p)\n",ptr,typeid(*ptr).name(),(BAtom*)ptr);
			#endif
			ptr->IncRefs(reinterpret_cast<void*>(m_teamID));
		} break;
		case brDECREFS: {
			ptr = (BBinder*)m_in.read32();
			#if BINDER_REFCOUNT_MSGS
			printf("calling decrefs on %p %s (atom %p)\n",ptr,typeid(*ptr).name(),(BAtom*)ptr);
			int32 r =
			#endif
			ptr->DecRefs(reinterpret_cast<void*>(m_teamID));
			#if BINDER_REFCOUNT_MSGS
			printf("called decrefs %ld\n",r);
			if (r > 1) ptr->Report(bout);
			#endif
		} break;
		case brTRANSACTION: {
			binder_transaction_data tr;
			m_in.read(&tr, sizeof(tr));
			if (tr.target.ptr) {
				BParcel buffer(tr.data.ptr.buffer, tr.data_size, _BufferFree, this);
				buffer.SetBinderOffsets(tr.data.ptr.offsets, tr.offsets_size);
				BParcel reply(_BufferReply, this);
				m_priority = tr.priority;
				
#if BINDER_DEBUG_MSGS
				msg << "Received transaction buffer: " << buffer.Data() << endl;
				msg->PrintAndReset(berr);
#endif

#if BINDER_BUFFER_MSGS
				printf("Creating transaction buffer for %p, now have %ld open.\n",
						tr.data.ptr.buffer, atomic_add(&g_openBuffers, 1) + 1);
#endif

				atom_ptr<BBinder> b((BBinder*)tr.target.ptr);
				b->Transact(tr.code, buffer, &reply, 0);
				
				// We must finish transactions in this order -- first sending
				// a reply, and then freeing the original buffer.
				reply.Reply();
				buffer.Free();
			} else {
				debugger("No target!");
			}
		} break;
		case brEVENT_OCCURRED: {
			if (m_team != NULL) m_team->DispatchMessage();
		} break;
		case brFINISHED: {
			result = B_IO_ERROR;
		} break;
		default: {
			result = B_ERROR;
		} break;
	}
	
	return result;
}

status_t 
BLooper::_WaitForCompletion(BParcel *reply, status_t *acquireResult)
{
	int32 cmd;
	int32 err = B_OK;
#if BINDER_DEBUG_MSGS
	BStringIO::ptr msg(new BStringIO);
#endif

	// Remember current thread priority, so any transactions executed
	// during this time don't disrupt it.
	const int32 curPriority = m_priority;
	
	while (1) {
		m_out.flush();
		cmd = m_in.read32();
#if BINDER_DEBUG_MSGS
		msg << "_WaitForCompletion got : " << cmd << " " << inString[cmd] << endl;
		msg->PrintAndReset(berr);
#endif
		if (cmd == brTRANSACTION_COMPLETE) {
			if (!reply && !acquireResult) break;
		} else if (cmd == brACQUIRE_RESULT) {
			int32 result = m_in.read32();
#if BINDER_BUFFER_MSGS
			printf("Result of bcATTEMPT_ACQUIRE: %ld\n", result);
#endif
			if (acquireResult) {
				*acquireResult = result ? B_OK : B_NOT_ALLOWED;
				break;
			}
			debugger("Unexpected brACQUIRE_RESULT!");
		} else if (cmd == brREPLY) {
			binder_transaction_data tr;
			m_in.read(&tr, sizeof(tr));
			if (reply) {
				reply->Reference(	tr.data.ptr.buffer, tr.data_size,
									_BufferFree, this);
#if BINDER_BUFFER_MSGS
				printf("Creating reply buffer for %p, now have %ld open.\n",
						tr.data.ptr.buffer, atomic_add(&g_openBuffers, 1) + 1);
#endif
			} else {
#if BINDER_BUFFER_MSGS
				printf("Immediately freeing buffer for %p\n", tr.data.ptr.buffer);
#endif
				if (tr.data.ptr.buffer == NULL) debugger("Sending NULL bcFREE_BUFFER!");
				m_out.write32(bcFREE_BUFFER);
				m_out.write32((int32)tr.data.ptr.buffer);
			}
			break;
		} else if ((err = _HandleCommand(cmd))) break;
	}
	
	// Restore last thread priority.
	_SetThreadPriority(curPriority);
	return err;
}

status_t 
BLooper::_BufferReply(const BParcel& buffer, void* context)
{
	return reinterpret_cast<BLooper*>(context)->_Reply(0, buffer);
}

void
BLooper::_BufferFree(const void* data, ssize_t /*len*/, void* context)
{
	if (data) {
#if BINDER_DEBUG_MSGS
		BStringIO::ptr msg(new BStringIO);
		msg << "Free transaction buffer: " << data << endl;
		msg->PrintAndReset(berr);
#endif
#if BINDER_BUFFER_MSGS
		printf("Freeing binder buffer for %p, now have %ld open.\n",
				data, atomic_add(&g_openBuffers, -1) - 1);
#endif
		if (data == NULL) debugger("Sending NULL bcFREE_BUFFER!");
		reinterpret_cast<BLooper*>(context)->m_out.write32(bcFREE_BUFFER);
		reinterpret_cast<BLooper*>(context)->m_out.write32((int32)data);
	} else {
		debugger("NULL _BufferFree()!");
	}
}

status_t 
BLooper::_Reply(uint32 flags, const BParcel& data)
{
	status_t err = _WriteTransaction(bcREPLY, flags, -1, 0, data);
	if (err < B_OK) return err;
	
	_WaitForCompletion();
	
	return B_OK;
}

void
BLooper::Sync()
{
	BLooper *me = This();
	if (me->m_flags&kSyncNeeded) {
		// need to rewrite
		me->m_flags &= ~kSyncNeeded;
	}
}

IBinder::ptr 
BLooper::_GetRootObject(thread_id id, team_id team)
{
	m_out.write32(bcRESUME_THREAD);
	m_out.write32(id);
	#if BINDER_DEBUG_LIB
	if (team < B_OK) {
		thread_info thinfo;
		get_thread_info(find_thread(NULL),&thinfo);
		team = thinfo.team+1;
	}
	m_out.write32(team);
	#endif
	
	BParcel data;
	_WaitForCompletion(&data);
	BValue value;
	data.GetValues(1, &value);
	
	return value.AsBinder();
}

IBinder::ptr 
BLooper::GetRootObject(thread_id id, team_id team)
{
	return This()->_GetRootObject(id, team);
}

status_t 
BLooper::SetRootObject(IBinder::ptr rootNode)
{
	BValue value(rootNode.ptr());
	BParcel data;
	data.SetValues(&value, NULL);
	return This()->_Reply(tfRootObject,data);
}

int32 BLooper::_LoopSelf()
{
	int32 cmd,err = B_OK;
	size_t count, i;
	while (err == B_OK) {
		m_out.flush();
		m_flags &= ~kSyncNeeded;
		cmd = m_in.read32();
		err = _HandleCommand(cmd);
		// Check to see if any add-ons got unloaded because of us
		count = m_dyingAddons.CountItems();
		for (i=count; i>0; i--) {
			unload_add_on(m_dyingAddons[i-1]);
			printf("unloaded addon %d\n", (int) m_dyingAddons[i-1]);
		}
		m_dyingAddons.MakeEmpty();
	}
	return err;
}

status_t 
BLooper::IncrefsHandle(int32 handle)
{
	TRACE();
#if BINDER_DEBUG_MSGS
	bout << "Writing increfs for handle " << handle << endl;
#endif
	m_out.write32(bcINCREFS);
	m_out.write32(handle);
	return B_OK;
}

status_t 
BLooper::DecrefsHandle(int32 handle)
{
	TRACE();
#if BINDER_DEBUG_MSGS
	bout << "Writing decrefs for handle " << handle << endl;
#endif
	m_out.write32(bcDECREFS);
	m_out.write32(handle);
	return B_OK;
}

status_t 
BLooper::AcquireHandle(int32 handle)
{
	TRACE();
#if BINDER_DEBUG_MSGS
	bout << "Writing acquire for handle " << handle << endl;
#endif
	m_out.write32(bcACQUIRE);
	m_out.write32(handle);
	m_out.flush();
	return B_OK;
}

status_t 
BLooper::ReleaseHandle(int32 handle)
{
	TRACE();
#if BINDER_DEBUG_MSGS
	bout << "Writing release for handle " << handle << endl;
#endif
	m_out.write32(bcRELEASE);
	m_out.write32(handle);
	return B_OK;
}

status_t 
BLooper::AttemptAcquireHandle(int32 handle)
{
	TRACE();
#if BINDER_DEBUG_MSGS
	bout << "Writing attempt acquire for handle " << handle << endl;
#endif
	m_out.write32(bcATTEMPT_ACQUIRE);
	m_out.write32(m_priority);
	m_out.write32(handle);
	status_t result = B_ERROR;
	_WaitForCompletion(NULL, &result);
	return result;
}

status_t
BLooper::_WriteTransaction(int32 cmd, uint32 binderFlags, int32 handle, uint32 code,
	const BParcel& data)
{
	binder_transaction_data tr;

	tr.target.handle = handle;
	tr.code = code;
	tr.flags = binderFlags&~tfInline;
	tr.priority = m_priority;
	tr.data_size = data.Length();
	tr.data.ptr.buffer = data.Data();
	tr.offsets_size = data.BinderOffsetsLength();
	tr.data.ptr.offsets = data.BinderOffsetsData();
	
	m_out.write32(cmd);
	m_out.write(&tr, sizeof(tr));
	
	return B_OK;
}

status_t
BLooper::Transact(int32 handle, uint32 code, const BParcel& data,
	BParcel* reply, uint32 /*flags*/)
{
	status_t err = _WriteTransaction(bcTRANSACTION, 0, handle, code, data);
	if (err < B_OK) return err;
	
	if (reply) {
		err = _WaitForCompletion(reply);
		m_flags &= ~kSyncNeeded;
	} else {
		BParcel fakeReply;
		err = _WaitForCompletion(&fakeReply);
		m_flags |= kSyncNeeded;
	}
	
	return err;
}

} }	// namespace B::Support2
