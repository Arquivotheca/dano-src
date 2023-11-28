
#include <Debug.h>

#include "minilocker.h"



MiniLocker::MiniLocker(const char *name)
{
	owner = 0;
	sem_cnt = 0;
	lock_nest = 0;
	sem = create_sem(0, name);
}


MiniLocker::~MiniLocker()
{
	delete_sem(sem);
}

bool 
MiniLocker::Lock()
{
	thread_id thr = find_thread(NULL);
	if (owner == thr) {	//	this is thread safe, because we already hold the lock if this is true
		debugger("recursive lock!");
		lock_nest++;
		return true;
	}
	if (atomic_add(&sem_cnt, 1) > 0) {
		if (acquire_sem(sem) < 0)
			return false;
	}
	owner = thr;
	lock_nest = 1;
	return true;
}

void 
MiniLocker::Unlock()
{
	thread_id thr = find_thread(NULL);
	ASSERT(thr == owner);
	ASSERT(lock_nest > 0);
	if (--lock_nest == 0) {
		owner = 0;
		if (atomic_add(&sem_cnt, -1) > 1) {
			release_sem(sem);	//	let the next guy in
		}
	}
}

