#ifndef _TIMER_H
#define _TIMER_H

#include <GHandler.h>

namespace Wagner {

typedef void (*TimerHandler)(const BMessage* data);

class TimerLooper;

class Timer : public GHandler
{
public:
	status_t Start(const BMessage* data, bigtime_t interval, int32 count = -1);
	status_t Start(TimerHandler handler, const BMessage* data,
				   bigtime_t interval, int32 count = -1);
	void Stop();
	
	virtual void TimerEvent(const BMessage* data);
	virtual status_t HandleMessage(BMessage* data);

	static void Shutdown();
	
private:
	TimerHandler fHandler;
	int32		 fCount;
	bigtime_t	 fInterval;
};

}

#endif
