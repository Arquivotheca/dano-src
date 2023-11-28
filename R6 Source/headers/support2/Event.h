
#ifndef _SUPPORT2_EVENT_H
#define _SUPPORT2_EVENT_H

#include <support2/SupportDefs.h>
#include <kernel/OS.h>

namespace B {
namespace Support2 {

class BEvent {
public:
						BEvent();
						BEvent(const char* name);
	virtual				~BEvent();
	
			status_t	InitCheck() const;
		
			void		Shutdown();
	
			// Pass B_CAN_INTERRUPT to allow the Wait() to be interrupted;
			// otherwise, it will wait until success or an error occurs.
			status_t	Wait(uint32 flags=0, bigtime_t timeout=B_INFINITE_TIMEOUT);
		
			// Allow all waiting threads to run.
			int32		FireAll();
	
			// Allow the first 'count' waiting threads to run.
			int32		Fire(int32 count);
	
private:
	virtual	status_t	ReservedEvent1();
	virtual	status_t	ReservedEvent2();
	virtual	status_t	ReservedEvent3();
	virtual	status_t	ReservedEvent4();
	virtual	status_t	ReservedEvent5();
	virtual	status_t	ReservedEvent6();
	
			int32		m_countWaiting;
			sem_id		m_sem;
			int32		_reserved[3];
};

} } // namespace B::Support2

#endif /* _SUPPORT2_EVENT_H */
