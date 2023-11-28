#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <support2/Locker.h>
#include <support2/Looper.h>
#include <support2/Message.h>
#include <www2/parameters.h>
#include <www2/Timer.h>

using namespace B::Support2;
using namespace B::WWW2;

status_t Timer::Start(const BMessage &data, bigtime_t interval, int32 count)
{
//	BMessage msg;
//	if (!data) data = msg;
	
	Stop();
	
	fInterval = interval;
	fCount = count;
	fHandler = 0;

	PostDelayedMessage(data,fInterval);

	return B_OK;
}

status_t Timer::HandleMessage(const BMessage &data)
{
	TimerEvent(data);
	if (fCount > 0) {
		PostDelayedMessage(data,fInterval);
		fCount--;
	}
	
	return B_OK;
}

status_t Timer::Start(TimerHandler handler, const BMessage &data,
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

void Timer::TimerEvent(const BMessage &data)
{
	if( fHandler ) (*fHandler)(data);
}
