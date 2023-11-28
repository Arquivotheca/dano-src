
/* BTimedEventQueue */
/* A queue of various media events */


#ifndef _TIMED_EVENT_QUEUE_H
#define _TIMED_EVENT_QUEUE_H

#include <MediaDefs.h>

struct _event_queue_imp;

class BTimedEventQueue {
	public:

		enum event_type {
			B_NO_EVENT = -1,		// never push this type! it will fail
			B_ANY_EVENT = 0,		// never push this type! it will fail
			B_START,
			B_STOP,
			B_SEEK,
			B_WARP,
			B_HANDLE_BUFFER,
			B_DATA_STATUS,
			/* user defined events above this value */
			B_USER_EVENT = 0x4000
		};

		enum cleanup_flag {
			B_NO_CLEANUP = 0,
			B_RECYCLE,					// recycle buffers
			B_DELETE,
			B_USER_CLEANUP = 0x4000,	//	above here user is responsible for cleanup
		};

		enum time_direction {
			B_ALWAYS = -1,
			B_BEFORE_TIME = 0,
			B_AT_TIME,
			B_AFTER_TIME
		};
		
							BTimedEventQueue();
		virtual				~BTimedEventQueue();
		
		/* push an event on to the queue */
		status_t			PushEvent(bigtime_t time, int32 what, void *pointer, uint32 pointer_cleanup, int64 data);

		/* give them the event */
		status_t			PopEvent(bigtime_t *time, int32 *what, void **pointer, uint32 *pointer_cleanup, int64 *data);

		/* if no events in the queue, return B_INFINITE_TIMEOUT and B_NO_EVENT in what */
		bigtime_t			NextEvent(int32 *what);
		
		/* flush events */
		/* calling with B_INFINITE_TIMEOUT & B_BEFORE will clear all events */
		status_t			FlushEvents(bigtime_t time, time_direction direction, bool inclusive = true, int32 event = B_ANY_EVENT);
		
		/* called on every event - be sure to call BTimedEventQueue::CleanUpEvent() if you */
		/* do not handle the cleanup yourself */
		/* only handle user event types */
		virtual void	 	CleanUpEvent(int32 what, void *pointer, uint32 cleanup);
		
		bool				HasEvents();

	private:
							BTimedEventQueue(					//unimplemented
								const BTimedEventQueue & other);
							BTimedEventQueue &operator =(		//unimplemented
								const BTimedEventQueue & other);

		/* hide actual queue implementation */
		_event_queue_imp *	fQueue;
		int32				fEventCount;

		/* Mmmh, stuffing! */
		virtual	status_t 	_Reserved_TimedEventQueue_0(void *);
		virtual	status_t 	_Reserved_TimedEventQueue_1(void *);
		virtual	status_t 	_Reserved_TimedEventQueue_2(void *);
		virtual	status_t 	_Reserved_TimedEventQueue_3(void *);
		virtual	status_t 	_Reserved_TimedEventQueue_4(void *);
		virtual	status_t 	_Reserved_TimedEventQueue_5(void *);
		virtual	status_t 	_Reserved_TimedEventQueue_6(void *);
		virtual	status_t 	_Reserved_TimedEventQueue_7(void *);

		uint32 				_reserved_timed_event_queue_[8];
};

#endif
