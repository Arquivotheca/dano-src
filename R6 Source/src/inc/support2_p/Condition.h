#ifndef _CONDITION_H
#define _CONDITION_H

#include <OS.h>
#include <support2/Locker.h>

using namespace B::Support2;

class Condition
{
	public:
		inline Condition(const char *name, BNestedLocker *lock);
		inline ~Condition();
		status_t Wait();
		void Signal();
	private:
		int32 fWaitCount;
		sem_id fWaitSem;
		BNestedLocker *fLock;
};

inline Condition::Condition(const char *name, BNestedLocker *lock)
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
	ASSERT(fLock->NestingLevel() != 0);
#warning "Fix me Condition::Wait()"
	fWaitCount++;
	int32 lockRecursion = 0;
	
	while (fLock->NestingLevel()) {
		lockRecursion++;
		fLock->Unlock();
	}

	acquire_sem(fWaitSem);
	
	while (lockRecursion-- > 0) {
		if (!fLock->Lock())
			return B_ERROR;
	}

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
