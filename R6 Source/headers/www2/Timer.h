#ifndef _TIMER_H
#define _TIMER_H

#include <support2/Handler.h>

namespace B {
namespace WWW2 {

using namespace B::Support2;

class B::Support2::BMessage;

typedef void (*TimerHandler)(const BMessage &data);

class TimerLooper;

class Timer : public BHandler
{
	public:
		status_t Start(const BMessage &data, bigtime_t interval, int32 count = -1);
		status_t Start(TimerHandler handler, const BMessage &data, bigtime_t interval, int32 count = -1);
		
		void Stop();

		virtual void TimerEvent(const BMessage &data);
		virtual status_t HandleMessage(const BMessage &data);

		static void Shutdown();

	private:
		TimerHandler fHandler;
		int32		 fCount;
		bigtime_t	 fInterval;
};

} } // namespace B::WWW2

#endif
