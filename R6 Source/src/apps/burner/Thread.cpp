//
// BThread implementation.  See Thread.h for details.
//
//
// by Nathan Schrenk (nschrenk@be.com)

#include "Thread.h"
#include <OS.h>

BThread::BThread(BRunnable *runner, const char *name, int32 priority)
	: fRunner(runner), fExited(false)
{
	char buf[B_OS_NAME_LENGTH]; 	// make sure that the name is not too long
	strncpy(buf, name, B_OS_NAME_LENGTH - 1);
	buf[B_OS_NAME_LENGTH - 1] = '\0';
	fId = spawn_thread(BThread::Run, buf, priority, (void *)this);
	// add this BThread to the static thread map
	if (sLock.Lock()) {
		sThreadMap[fId] = this;
		sLock.Unlock();
	}
}

BThread::~BThread()
{
	// remove this BThread from the static thread map
	if (sLock.Lock()) {
		sThreadMap.erase(fId);
		sLock.Unlock();
	}
}

BLocker	BThread::sLock;

map<thread_id, BThread *> BThread::sThreadMap;


status_t BThread::InitCheck()
{
	status_t r = (fId != B_NO_MORE_THREADS && fId != B_NO_MEMORY) ? B_OK : fId;
	return r;
}

status_t BThread::Kill()
{
	status_t ret = kill_thread(fId);
	fExited = true;	
	return ret;
}

status_t BThread::Suspend()
{
	return suspend_thread(fId);
}

status_t BThread::Resume()
{
	return resume_thread(fId);
}

status_t BThread::Wait()
{
	return wait_for_thread(fId, &fExitValue);
}

//bigtime_t BThread::EstimatedMaxSchedulingLatency()
//{
//	return estimate_max_scheduling_latency(fId);
//}

thread_id BThread::ThreadId()
{
	return fId;
}

int32 BThread::Priority()
{
	thread_info info;
	status_t c = get_thread_info(fId, &info);
	if (c == B_OK) {
		return info.priority;
	} else {
		return -1;
	}
}

status_t BThread::SetPriority(int32 priority)
{
	return set_thread_priority(fId, priority);
}

const char *BThread::Name()
{
	thread_info info;
	status_t c = get_thread_info(fId, &info);
	if (c == B_OK) {
		return info.name;
	} else {
		return NULL;
	}
}

status_t BThread::SetName(const char *name)
{
	char buf[B_OS_NAME_LENGTH]; 	// make sure that the name is not too long
	strncpy(buf, name, B_OS_NAME_LENGTH - 1);
	buf[B_OS_NAME_LENGTH - 1] = '\0';
	return rename_thread(fId, buf);
}


BThread *BThread::CurrentThread()
{
	BThread *ret = NULL;
	thread_id id = find_thread(NULL);
	if (sLock.Lock()) {
		ret = sThreadMap[id];
		sLock.Unlock();
	}
	return ret;
}


int32 BThread::Run(void *castToBThread)
{
	BThread *thread = (BThread *)castToBThread;
	thread->fExitValue = thread->fRunner->Run();
	thread->fExited = true;
	return thread->fExitValue;
}
