
#ifndef _NT_EVENT_H
#define _NT_EVENT_H

#include <atomic.h>

class Event {

	private:
		int32		m_countWaiting;
		sem_id		m_sem;

	public:
					Event() { m_countWaiting = 0; m_sem = create_sem(0,"Event sem"); };
					~Event() { if (m_sem != B_BAD_SEM_ID) delete_sem(m_sem); };

		void		Shutdown() { delete_sem(m_sem); m_sem = B_BAD_SEM_ID; };

		status_t	Wait(bigtime_t timeout) {
			atomic_add(&m_countWaiting,1);
			status_t err = acquire_sem_etc(m_sem,1,B_ABSOLUTE_TIMEOUT,timeout);
			if (err) {
				int32 newVal,oldVal = m_countWaiting;
				do {
					newVal = oldVal;
					if (newVal > 0) newVal--;
				} while (!cmpxchg32(&m_countWaiting,&oldVal,newVal));
				if (newVal == oldVal) acquire_sem(m_sem);
			};
			
			return err;
		};

		int32		Fire(int32 count) {
			int32 newVal,oldVal = m_countWaiting;
			do {
				newVal = oldVal - count;
				if (newVal < 0) newVal = 0;
			} while (!cmpxchg32(&m_countWaiting,&oldVal,newVal));
			if (newVal < oldVal) release_sem_etc(m_sem,oldVal-newVal,B_DO_NOT_RESCHEDULE);
			return (oldVal-newVal);
		};
};

#endif
