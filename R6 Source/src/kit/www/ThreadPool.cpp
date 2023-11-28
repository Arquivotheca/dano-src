#include <string.h>

#include <Autolock.h>
#include <Debug.h>

#include "ThreadPool.h"
#include "parameters.h"
#include "Queue.h"

using namespace Wagner;

const char *kWorkerThreadName = "Idle Worker";
const int kMaxJobData = 64;

struct Job : public QueueEntry {
	JobHandler function;
	int dataSize;
	char data[kMaxJobData];
};

ThreadPool::ThreadPool(int32 minThreads, int32 maxThreads)
	:	fThreadCount(0),
		fActiveThreadCount(0),
		fMinThreadCount(minThreads),
		fMaxThreadCount(maxThreads),
		fLock("Thread Pool Lock"),
		fShutdown(false)
{
	fJobsAvailable = create_sem(0, "Jobs Available"); 	
	for (int i = 0; i < minThreads; i++) {
		fThreadCount++;
		resume_thread(spawn_thread(WorkerThreadStart, kWorkerThreadName, B_NORMAL_PRIORITY, this));
	}
}

ThreadPool::~ThreadPool()
{
	Shutdown();
}

status_t ThreadPool::AddJob(JobHandler function, void *data, size_t dataSize)
{
	fLock.Lock();
	Job *job = (Job*) fFreeList.Dequeue();
	if (!job)
		job = new Job;
		
	job->function = function;
	job->dataSize = dataSize;
	if (dataSize > kMaxJobData)
		debugger("ThreadPool: too much data\n");
		
	memcpy(job->data, data, dataSize);
	fJobQueue.Enqueue(job);
	release_sem_etc(fJobsAvailable, 1, B_DO_NOT_RESCHEDULE);
	if (fThreadCount < fMaxThreadCount && fActiveThreadCount == fThreadCount) {
		// All the current threads are busy, and we haven't hit our
		// limit on threads.  Spawn a new one.
		fThreadCount++;
		fLock.Unlock();
		resume_thread(spawn_thread(WorkerThreadStart, kWorkerThreadName, B_NORMAL_PRIORITY, this));
	} else
		fLock.Unlock();

	return B_OK;
}

int32 ThreadPool::WorkerThreadStart(void *pool)
{
	reinterpret_cast<ThreadPool*>(pool)->WorkerThreadLoop();
	return 0;
}

void ThreadPool::WorkerThreadLoop()
{
	bool checkForDeath = false;
	while (true) {
		ssize_t retval;
		do {
			retval = acquire_sem_etc(fJobsAvailable, 1, checkForDeath ? B_TIMEOUT : 0,
				kIdlePoolThreadDeath);
		} while (retval == B_INTERRUPTED);

		if (retval == B_TIMED_OUT) {
			// I've just waited too long for a job to come up.  
			// Quit to reduce the number of idle thread waiting around.
			BAutolock _lock(&fLock);
			fThreadCount--;
			return;
		} else if (retval == B_BAD_SEM_ID) {
			fThreadCount--;
			return;	// The thread pool has gone away.
		}

		fLock.Lock();
		fActiveThreadCount++;
		Job *job = (Job*) fJobQueue.Dequeue();
		fLock.Unlock();
		
		(*job->function)(job->data, job->dataSize);
		rename_thread(find_thread(NULL), kWorkerThreadName);

		fLock.Lock();
		fFreeList.Enqueue(job);
		if (fActiveThreadCount-- > fMinThreadCount)
			checkForDeath = true;	// I am expendable

		fLock.Unlock();
	}
}

void ThreadPool::Shutdown()
{
	if (!fShutdown) {
		fShutdown = true;	
		delete_sem(fJobsAvailable);
	
		// Wait for worker threads to exit
		while (fThreadCount > 0)
			snooze(50000);
	}
}
