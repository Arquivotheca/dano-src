#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <OS.h>
#include <Locker.h>
#include "Timer.h"
#include "Queue.h"

namespace Wagner {

typedef void (*JobHandler)(void *data, size_t dataSize);

class ThreadPool {
public:
	ThreadPool(int32 minThreads, int32 maxThreads);
	~ThreadPool();
	status_t AddJob(JobHandler handler, void *data, size_t dataSize);
	void Shutdown();

private:
	static int32 WorkerThreadStart(void *pool);
	void WorkerThreadLoop();

	sem_id fJobsAvailable;
	Queue fJobQueue;
	Queue fFreeList;
	int32 fPendingJobCount;
	int32 fThreadCount;
	int32 fActiveThreadCount;
	int32 fMinThreadCount;
	int32 fMaxThreadCount;
	BLocker fLock;
	bool fShutdown;
};

}

#endif
