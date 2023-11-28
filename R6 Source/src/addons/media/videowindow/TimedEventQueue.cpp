
#include "TimedEventQueue.h"

#include <Locker.h>
#include <Buffer.h>

#include <stdio.h>

/* defines an event */
struct timed_event {
	timed_event();
	timed_event(int32 what, void *pointer, uint32 cleanup, int64 data);
	int32		what;			// the type of event
	void *		pointer;		// additional data needed for the event (BBuffer * for B_HANDLE_BUFFER)
	uint32		cleanup;		// a flag describing how the pointer should be cleaned up
	int64		data;			// for time information or additional info
};


timed_event::timed_event() :
	what(0), pointer(NULL), cleanup(0), data(0)
{
}

timed_event::timed_event(int32 what, void *pointer, uint32 cleanup, int64 data) :
	what(what), pointer(pointer), cleanup(cleanup), data(data)
{
}

#include "rt_allocator.h"
#include "rt_map.h"

typedef rt_multimap<bigtime_t, timed_event, 64, SortedArray<bigtime_t, pair<bigtime_t, timed_event>,
	_select1st<pair<bigtime_t, timed_event>, const bigtime_t>,
	_sorted_array_64 > > event_map;


class _event_queue_imp {
	public:
		event_map	fEvents;
		BLocker		fLock;

		_event_queue_imp() {
			fLock = true;
		}

		~_event_queue_imp() {
		
		}
	
		bool lock() {
			return fLock.Lock();
		}
		
		void unlock() {
			fLock.Unlock();
		}
};




BTimedEventQueue::BTimedEventQueue() :
	fQueue(new _event_queue_imp),
	fEventCount(0)
{

}


BTimedEventQueue::~BTimedEventQueue()
{
	// flush all items in the queue
	FlushEvents(B_INFINITE_TIMEOUT, B_BEFORE_TIME);
	delete fQueue; fQueue = NULL;	
}

status_t 
BTimedEventQueue::PushEvent(bigtime_t time, int32 what, void *pointer, uint32 pointer_cleanup, int64 data)
{
	if (what == B_NO_EVENT || what == B_ANY_EVENT) return B_BAD_VALUE;
	if (!fQueue->lock()) return B_ERROR;
	try {
		fQueue->fEvents.insert(event_map::value_type(time, timed_event(what, pointer, pointer_cleanup, data)));
		atomic_add(&fEventCount, 1);
		fQueue->unlock();
	}
	catch(...) {
		fQueue->unlock();
		return B_ERROR;
	}
	return B_OK;
}

status_t 
BTimedEventQueue::PopEvent(bigtime_t *perf, int32 *what, void **pointer, uint32 *flags, int64 *data)
{
	if (!fQueue->lock()) return B_ERROR;
	try {
		event_map::iterator i(fQueue->fEvents.begin());
		if (i != fQueue->fEvents.end()) {
			if (perf) *perf = (*i).first;
			if (what) *what = (*i).second.what;
			if (pointer) *pointer = (*i).second.pointer;
			if (flags) *flags = (*i).second.cleanup;
			if (data) *data = (*i).second.data;
			atomic_add(&fEventCount, -1);
			/* user will clean up as appropriate */
			fQueue->fEvents.erase(i);
		}
		else return B_ERROR;
		fQueue->unlock();
	}
	catch (...) {
		fQueue->unlock();
		return B_ERROR;
	}
	return B_OK;
	
}

bigtime_t 
BTimedEventQueue::NextEvent(int32 *what)
{
	if (!fQueue->lock()) return B_ERROR;
	try {
		event_map::iterator i(fQueue->fEvents.begin());
		if (i != fQueue->fEvents.end()) {
			if (what) *what = (*i).second.what;
			return (*i).first;
		} else {
			if (what) *what = B_NO_EVENT;
			return B_INFINITE_TIMEOUT;
		}
	}
	catch (...) {
		fQueue->unlock();
		return B_ERROR;
	}
}


status_t 
BTimedEventQueue::FlushEvents(bigtime_t time, time_direction direction, bool inclusive, int32 type)
{
	status_t err = B_ERROR;
	if (!fQueue->lock()) return err;
	try {
		event_map::iterator current;
		event_map::iterator end;
		
		switch(direction) {
			case B_BEFORE_TIME: {
				current = fQueue->fEvents.begin();
				if (inclusive)
					end = fQueue->fEvents.upper_bound(time);
				else
					end = fQueue->fEvents.lower_bound(time);
				break;
			}
			
			case B_AFTER_TIME: {
				end = fQueue->fEvents.end();
				if (inclusive)
					current = fQueue->fEvents.lower_bound(time);
				else
					current = fQueue->fEvents.upper_bound(time);
				break;
			}
			case B_AT_TIME: {
				current = fQueue->fEvents.lower_bound(time);
				end = fQueue->fEvents.upper_bound(time);
				break;
			}
			default:
				return B_BAD_VALUE;
		}

		
		while (current != end) {
			if (type != B_ANY_EVENT && type != B_NO_EVENT && type != (*current).second.what) {
				current++;
				continue;
			}
			atomic_add(&fEventCount, -1);
			/* clean up the item */
//			printf("CleanUpEvent: FlushEvents\n");
			CleanUpEvent((*current).second.what, (*current).second.pointer, (*current).second.cleanup);
			/* remove the item from the event queue */
			fQueue->fEvents.erase(current++);
		}
		err = B_OK;
		fQueue->unlock();
	}
	catch(...) {
		fQueue->unlock();
		return B_ERROR;
	}
	return err;
}

bool 
BTimedEventQueue::HasEvents()
{
	return fEventCount > 0;
}


void 
BTimedEventQueue::CleanUpEvent(const int32 what, void *pointer, uint32 pointer_cleanup)
{
	switch(pointer_cleanup) {
		case B_RECYCLE: {
			if (what == B_HANDLE_BUFFER) {
				BBuffer *buffer = (BBuffer *) pointer;
				buffer->Recycle();
			}
			break;
		}
		
		default:
			return;
	}
}


status_t 
BTimedEventQueue::_Reserved_TimedEventQueue_0(void *)
{
	return B_ERROR;
}

status_t 
BTimedEventQueue::_Reserved_TimedEventQueue_1(void *)
{
	return B_ERROR;
}

status_t 
BTimedEventQueue::_Reserved_TimedEventQueue_2(void *)
{
	return B_ERROR;
}

status_t 
BTimedEventQueue::_Reserved_TimedEventQueue_3(void *)
{
	return B_ERROR;
}

status_t 
BTimedEventQueue::_Reserved_TimedEventQueue_4(void *)
{
	return B_ERROR;
}

status_t 
BTimedEventQueue::_Reserved_TimedEventQueue_5(void *)
{
	return B_ERROR;
}

status_t 
BTimedEventQueue::_Reserved_TimedEventQueue_6(void *)
{
	return B_ERROR;
}

status_t 
BTimedEventQueue::_Reserved_TimedEventQueue_7(void *)
{
	return B_ERROR;
}

