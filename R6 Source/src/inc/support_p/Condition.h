#ifndef _CONDITION_H
#define _CONDITION_H

#include <OS.h>

class Condition {
public:
	inline Condition(const char *name, BLocker *lock);
	inline ~Condition();
	status_t Wait();
	void Signal();
private:
	int32 fWaitCount;
	sem_id fWaitSem;
	BLocker *fLock;
};

inline Condition::Condition(const char *name, BLocker *lock)
	:	fWaitCount(0),
		fLock(lock)
{
	fWaitSem = create_sem(0, name);
}

inline Condition::~Condition()
{
	delete_sem(fWaitSem);
}

inline status_t Condition::Wait()
{
	ASSERT(fLock->IsLocked());
	fWaitCount++;
	int32 lockRecursion = 0;
	while (fLock->IsLocked()) {
		lockRecursion++;
		fLock->Unlock();
	}

	acquire_sem(fWaitSem);
	while (lockRecursion-- > 0)
		if (!fLock->Lock())
			return B_ERROR;

	return B_OK;
}

inline void Condition::Signal()
{
	fLock->Lock();
	release_sem_etc(fWaitSem, fWaitCount, B_DO_NOT_RESCHEDULE);
	fWaitCount = 0;
	fLock->Unlock();
}

#endif
