#include <Autolock.h>
#include <Debug.h>
#include <List.h>
#include <Locker.h>
#include <Looper.h>
#include "Timer.h"
#include "parameters.h"

using namespace Wagner;

namespace Wagner {

status_t Timer::Start(const BMessage* data, bigtime_t interval, int32 count)
{
	BMessage msg;
	if (!data) data = &msg;
	
	Stop();
	
	fInterval = interval;
	fCount = count;
	fHandler = 0;

	PostDelayedMessage(*data,fInterval);

	return B_OK;
}

status_t 
Timer::HandleMessage(BMessage *data)
{
	TimerEvent(data);
	if (fCount > 0) {
		PostDelayedMessage(*data,fInterval);
		fCount--;
	}
	
	return B_OK;
}

status_t Timer::Start(TimerHandler handler, const BMessage* data,
			   bigtime_t interval, int32 count)
{
	status_t res = Start(data, interval, count);
	fHandler = handler;
	return res;
}

void Timer::Stop()
{
	ClearMessages();
}

void Timer::TimerEvent(const BMessage* data)
{
	if( fHandler ) (*fHandler)(data);
}

}
