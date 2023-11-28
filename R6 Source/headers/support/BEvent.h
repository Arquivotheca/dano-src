/******************************************************************************
/
/	File:			BEvent.h
/
/	Description:	Event-style synchronization primitive.
/
/	Copyright 2001, Be Incorporated
/
******************************************************************************/

#ifndef _SUPPORT_EVENT_H
#define _SUPPORT_EVENT_H

#include <kernel/OS.h>

namespace B {
namespace Support {

class BEvent {
public:
				BEvent();
virtual			~BEvent();

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
	int32		fCountWaiting;
	sem_id		fSem;
	int32		_reserved[3];
};

} }	// namespace B::Support

#endif
