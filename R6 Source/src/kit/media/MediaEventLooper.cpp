
#include <MediaEventLooper.h>
#include <TimeSource.h>
#include <OS.h>
#include <stdio.h>
#include <scheduler.h>
#include <Debug.h>
#include <assert.h>
#include <string.h>

#define CALL(x...)	//printf
#define EVENT(x...)	//printf
#define ERROR(x...)	//printf
#define	LOOP(x...)	//printf
#define OFFLINE(x...)	//printf
#define RUNMODE(x...)	//printf

BMediaEventLooper::BMediaEventLooper(uint32 apiVersion) :
	BMediaNode("error"),
	_mControlThread(-1),
	_mCurrentPriority(B_NORMAL_PRIORITY),
	_mSetPriority(B_NORMAL_PRIORITY),
	_mRunState(B_UNREGISTERED),
	_mEventLatency(0),
	_mSchedulingLatency(0),
	_mBufferDuration(0),
	_mOfflineTime(0),
	_mAPIVersion(apiVersion)
{
	_mEventQueue.SetCleanupHook(_CleanUpEntry, this);
	_mRealTimeQueue.SetCleanupHook(_CleanUpEntry, this);
}

BMediaEventLooper::~BMediaEventLooper()
{
	if ((_mRunState != B_TERMINATED) && (_mRunState != B_UNREGISTERED)) {
		fprintf(stderr, "You MUST call BMediaEventLooper::Quit() in your destructor!\n");
#if !NDEBUG
		debugger("You MUST call BMediaEventLooper::Quit() in your destructor!\n");
#endif
	}
}

void 
BMediaEventLooper::NodeRegistered()
{
	Run();
}

void 
BMediaEventLooper::Start(bigtime_t performance_time)
{
	CALL("BMediaEventLooper::Start @ %Ld\n", performance_time);
	media_timed_event event(performance_time, BTimedEventQueue::B_START);
	_mEventQueue.AddEvent(event);

	if (RunMode() == B_OFFLINE)
		SetOfflineTime(performance_time);
}

void 
BMediaEventLooper::Stop(bigtime_t performance_time, bool immediate)
{
	CALL("BMediaEventLooper::Stop @ %Ld %s\n", performance_time, immediate ? "immediate": "");
	if (immediate)
	{
		media_timed_event stopit(0, BTimedEventQueue::B_STOP);
		DispatchEvent(&stopit, 0, false);
	}
	else {
		media_timed_event event(performance_time, BTimedEventQueue::B_STOP);
		_mEventQueue.AddEvent(event);
	}
}

void 
BMediaEventLooper::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	CALL("BMediaEventLooper::Seek to %Ld @ %Ld\n", media_time, performance_time);
	media_timed_event event(performance_time, BTimedEventQueue::B_SEEK);
	event.bigdata = media_time;
	_mEventQueue.AddEvent(event);
}

void 
BMediaEventLooper::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	CALL("BMediaEventLooper::TimeWarp to %Ld @ %Ld\n", at_real_time, to_performance_time);
	media_timed_event event(at_real_time, BTimedEventQueue::B_WARP);
	event.bigdata = to_performance_time;
	_mRealTimeQueue.AddEvent(event);
}

status_t 
BMediaEventLooper::AddTimer(bigtime_t at_performance_time, int32 cookie)
{
	media_timed_event event(at_performance_time, BTimedEventQueue::B_TIMER);
	event.data = cookie;
	event.cleanup = BTimedEventQueue::B_EXPIRE_TIMER;
	_mEventQueue.AddEvent(event);
	return B_OK;
}

BTimedEventQueue *
BMediaEventLooper::EventQueue()
{
	return &_mEventQueue;
}

BTimedEventQueue *
BMediaEventLooper::RealTimeQueue()
{
	return &_mRealTimeQueue;
}

int32 
BMediaEventLooper::Priority() const
{
	return _mCurrentPriority;
}

int32 
BMediaEventLooper::RunState() const
{
	return _mRunState;
}

bigtime_t 
BMediaEventLooper::EventLatency() const
{
	return _mEventLatency;
}

bigtime_t 
BMediaEventLooper::BufferDuration() const
{
	return _mBufferDuration;
}

bigtime_t 
BMediaEventLooper::SchedulingLatency() const
{
	return _mSchedulingLatency;
}

void 
BMediaEventLooper::SetRunMode(run_mode mode)
{
	CALL("SetRunMode: %ld for thread: %ld\n", mode, _mControlThread);

	if (mode == B_OFFLINE)
	{
		if (_mControlThread > 0)
		{
			status_t err = B_OK;
			RUNMODE("calling set thread priority\n");
			err = set_thread_priority(_mControlThread, B_LOW_PRIORITY);
			RUNMODE("from non-offline to offline: set_thread_priority error: %s (%d)\n", strerror(err), err);
			if (err >= B_OK)
			{
				_mCurrentPriority = B_LOW_PRIORITY;
				_mSchedulingLatency = estimate_max_scheduling_latency(_mControlThread);
			}
		} else _mCurrentPriority = B_LOW_PRIORITY;
	
	} else {
		if (_mControlThread > 0)
		{
			status_t err = B_OK;
			RUNMODE("calling set thread priority\n");
			err = set_thread_priority(_mControlThread, _mSetPriority);
			RUNMODE("from non-offline to offline: set_thread_priority error: %s (%d)\n", strerror(err), err);
			if (err >= B_OK)
			{
				_mCurrentPriority = _mSetPriority;
				_mSchedulingLatency = estimate_max_scheduling_latency(_mControlThread);
			}
		} else _mCurrentPriority = _mSetPriority;
	
	}

}

status_t 
BMediaEventLooper::SetPriority(int32 priority)
{
	// clamp to a valid value
	if (priority < 5)
		priority = 5;
	else if (priority > 120)
		priority = 120;
	
	if (_mControlThread > 0)
	{
		status_t err = set_thread_priority(_mControlThread, priority);
		if (err < B_OK)
			return B_ERROR;
			
		_mCurrentPriority = priority;
		_mSchedulingLatency = estimate_max_scheduling_latency(_mControlThread);
	}
	else _mCurrentPriority = priority;
	
	_mSetPriority = priority;
	return B_OK;
}

void 
BMediaEventLooper::SetEventLatency(bigtime_t latency)
{
	// clamp to a valid value
	if (latency < 0)
		latency = 0;
	
	_mEventLatency = latency;
}

void 
BMediaEventLooper::SetBufferDuration(bigtime_t duration)
{
	// clamp to a valid value
	if (duration < 0)
		duration = 0;
	
	_mBufferDuration = duration;
}

void 
BMediaEventLooper::SetRunState(run_state state)
{
	_mRunState = state;
}

void 
BMediaEventLooper::Run()
{
	CALL("BMediaEventLooper::Run()\n");
	if (_mControlThread == -1 && RunState() == B_UNREGISTERED) {
		CALL("BMediaEventLooper::Run() - spawning thread\n");
		_mRunState = B_STOPPED;
		char threadName[32];
		sprintf(threadName, "%.20s control", Name());
		_mControlThread = spawn_thread(_ControlThreadStart, threadName, _mSetPriority, this);
		_mSchedulingLatency = estimate_max_scheduling_latency(_mControlThread);
		resume_thread(_mControlThread);
	}
	else {
#if DEBUG
	debugger("Node registered twice! There is much badness.");
#else
	printf("Node registered twice! There is much badness.\n");
#endif
	}
}

void 
BMediaEventLooper::Quit()
{
	if (_mRunState != B_TERMINATED) {
		_mRunState = B_QUITTING;
		int32 ctrl = _mControlThread;
		status_t err = close_port(ControlPort());
		if (ctrl != find_thread(NULL)) {
			wait_for_thread(ctrl, &err);
		}
		else {
			fprintf(stderr, "BMediaEventLooper::Quit() called from service thread.\n");
		}
	}
}


bigtime_t 
BMediaEventLooper::OfflineTime()
{
	ASSERT_WITH_MESSAGE(RunMode() == B_OFFLINE, "OfflineTime called when not in B_OFFLINE mode\n"
		"If you do not support B_OFFLINE mode you must override SetRunMode() to forbid those changes\n");
	return _mOfflineTime;
}

void 
BMediaEventLooper::SetOfflineTime(bigtime_t offTime)
{
	_mOfflineTime = offTime;
}

int32 
BMediaEventLooper::_ControlThreadStart(void *arg)
{
	BMediaEventLooper *looper = reinterpret_cast<BMediaEventLooper *>(arg);
	if (looper)
		looper->ControlLoop();
	return 0;
}

void
BMediaEventLooper::ControlLoop()
{
	bool lastWasReal = false;
	while(true)
	{
		if (RunMode() != B_OFFLINE)
		{
			bigtime_t waitUntil = B_INFINITE_TIMEOUT;
			bool useRealTimeEvent = true;
			
			bigtime_t rEventTime = B_INFINITE_TIMEOUT;
			const media_timed_event *rEvent = NULL;
			
			bigtime_t pEventTime = B_INFINITE_TIMEOUT;
			const media_timed_event *pEvent = NULL;
			bool bufferEvent = false;
			
			if (_mEventQueue.HasEvents())
			{
				pEvent = _mEventQueue.FirstEvent();
				
				if (pEvent) {
					pEventTime = TimeSource()->RealTimeFor(pEvent->event_time, _mEventLatency + _mSchedulingLatency);
					if (pEvent->type == BTimedEventQueue::B_HANDLE_BUFFER)
						bufferEvent = true;
				}
			}	
			
			if (_mRealTimeQueue.HasEvents())
			{
				rEvent = RealTimeQueue()->FirstEvent();
				
				if (rEvent)
					rEventTime = rEvent->event_time - _mSchedulingLatency;
			}
				
			if ((pEventTime < rEventTime) || ((pEventTime == rEventTime) && lastWasReal)) {
				waitUntil = pEventTime;
				if (bufferEvent)
				{ // A buffer event should be processed at least _mBufferDuration before the event
					waitUntil -= _mBufferDuration;
					if (_mBufferDuration > _mEventLatency)
						waitUntil += _mEventLatency;
				}
				useRealTimeEvent = false;
				lastWasReal = false;
			} else {
				waitUntil = rEventTime;
				useRealTimeEvent = true;
				lastWasReal = true;
			}
			
			
			status_t err = B_TIMED_OUT;
			err = WaitForMessage(waitUntil);
			
			if ((err == B_TIMED_OUT) || (err == B_WOULD_BLOCK))
			{
				const media_timed_event *nextEvent = NULL;
				bigtime_t lateness = BTimeSource::RealTime();
				
				if (useRealTimeEvent)
				{
					nextEvent = rEvent;
					if (nextEvent)
						lateness -= rEventTime;
				}
				else
				{
					nextEvent = pEvent;
					if (nextEvent)
						lateness -= pEventTime;
				}
				if (nextEvent) {
					DispatchEvent(nextEvent, lateness, useRealTimeEvent);
					if (useRealTimeEvent)
						_mRealTimeQueue.RemoveEvent(nextEvent);
					else
						_mEventQueue.RemoveEvent(nextEvent);
				}
			}
			else if (err < B_OK)
			{
				ERROR("WaitForMessage error: %s (%d) ONLINE\n", strerror(err), err);
				_mRunState = B_IN_DISTRESS;
				ReportError(B_NODE_IN_DISTRESS);
				break;
			}
	
		} else {
			// we're in offline mode!
			OFFLINE("We're in offline mode\n");
			bigtime_t offTime = OfflineTime();
			OFFLINE("MediaEventLooper::ControlLoop: offTime: %Ld\n", offTime);
			
			// handle all events <= offTime
			const media_timed_event *pEvent = _mEventQueue.FirstEvent();
			while (pEvent && pEvent->event_time <= offTime)
			{
				OFFLINE("MediaEventLooper::ControlLoop: dispatch offline event\n");
				DispatchEvent(pEvent, 0, false);
				_mEventQueue.RemoveEvent(pEvent);
				pEvent = _mEventQueue.FirstEvent();
			}
			
			bigtime_t waitUntil = B_INFINITE_TIMEOUT;
			const media_timed_event *rEvent = _mRealTimeQueue.FirstEvent();
			if (rEvent)
				waitUntil = rEvent->event_time - _mSchedulingLatency;
	
			status_t err = B_TIMED_OUT;
			
			OFFLINE("MediaEventLooper::ControlLoop: Offline waitUntil: %Ld\n", waitUntil);
			
			err = WaitForMessage(waitUntil);
			
			if ((err == B_TIMED_OUT) || (err == B_WOULD_BLOCK))
			{
				if (rEvent) {
					DispatchEvent(rEvent, BTimeSource::RealTime() - rEvent->event_time, true);
					_mRealTimeQueue.RemoveEvent(rEvent);
				}
			}
			else if (err < B_OK)
			{
				ERROR("WaitForMessage error: %s (%d) OFFLINE\n", strerror(err), err);
				_mRunState = B_IN_DISTRESS;
				ReportError(B_NODE_IN_DISTRESS);
				break;
			}
		}
	}
	CALL("BMediaEventLooper::ControlLoop - exiting\n");
	_mControlThread = -1;
	_mRunState = B_TERMINATED;
}


thread_id 
BMediaEventLooper::ControlThread()
{
	return _mControlThread;
}


void 
BMediaEventLooper::DispatchEvent(const media_timed_event *event, bigtime_t lateness, bool realTimeEvent)
{
	EVENT("BMediaEventLooper::DispatchEvent: %Ld 0x%lx %Ld %d\n", event->event_time, event->type, lateness, realTimeEvent);

	bigtime_t eventTime = event->event_time;
	int32 eventType = event->type;
	
	if (eventType == BTimedEventQueue::B_TIMER)
	{
		TimerExpired(event->event_time, event->data);
		return;
	}
	
	HandleEvent(event, lateness, realTimeEvent);
	
	// post-handling cleanup and state tracking	
	switch(eventType) {
		case BTimedEventQueue::B_START:
			_mRunState = B_STARTED;
			break;
		
		case BTimedEventQueue::B_STOP: 
			_mRunState = B_STOPPED;
			if (realTimeEvent)
				NodeStopped(TimeSource()->PerformanceTimeFor(eventTime));
			else NodeStopped(eventTime);
			break;
	
		case BTimedEventQueue::B_SEEK:
			/* nothing */
			break;
		
		case BTimedEventQueue::B_WARP:
			/* nothing */
			break;
		
		default:
			break;
	}
	
}

void 
BMediaEventLooper::_CleanUpEntry(const media_timed_event *event, void *context)
{
	BMediaEventLooper *looper = reinterpret_cast<BMediaEventLooper *>(context);
	if (looper)
		looper->_DispatchCleanUp(event);
	return;
}

void 
BMediaEventLooper::_DispatchCleanUp(const media_timed_event *event)
{
	if (event->type == BTimedEventQueue::B_TIMER && event->cleanup == BTimedEventQueue::B_EXPIRE_TIMER)
		TimerExpired(event->event_time, event->data, B_MEDIA_BAD_NODE);
	else CleanUpEvent(event);
}


void 
BMediaEventLooper::CleanUpEvent(const media_timed_event */*event*/)
{
	/* do nothing */
}

status_t 
BMediaEventLooper::DeleteHook(BMediaNode *node)
{
	Quit();
	return BMediaNode::DeleteHook(node);
}

status_t BMediaEventLooper::_Reserved_BMediaEventLooper_0(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_1(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_2(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_3(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_4(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_5(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_6(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_7(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_8(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_9(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_10(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_11(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_12(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_13(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_14(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_15(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_16(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_17(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_18(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_19(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_20(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_21(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_22(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_23(int32 /*arg*/, ...) { return B_ERROR; }
