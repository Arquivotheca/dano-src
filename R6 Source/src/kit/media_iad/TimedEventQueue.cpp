
#include <TimedEventQueue.h>

#include <Buffer.h>

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <new>
#include <algorithm>

#define CALL		(void)
#define INFO		(void)
#define LOCKINFO	(void)
#define LOOP		(void)
#define PRINTF 		(void)
#define	ERROR		(void)

media_timed_event::media_timed_event()
{
	memset((void *)this, 0, sizeof(media_timed_event));
}

media_timed_event::media_timed_event(bigtime_t inTime, int32 inType)
{
	memset((void *)this, 0, sizeof(media_timed_event));
	event_time = inTime;
	type = inType;
}


media_timed_event::media_timed_event(bigtime_t inTime, int32 inType, void *inPointer, uint32 inCleanup)
{
	memset((void *)this, 0, sizeof(media_timed_event));
	event_time = inTime;
	type = inType;
	pointer = inPointer;
	cleanup = inCleanup;
}

media_timed_event::media_timed_event(	bigtime_t inTime, int32 inType,
							void *inPointer, uint32 inCleanup,
							int32 inData, int64 inBigdata,
							char *inUserData, size_t dataSize)
{
	memset((void *)this, 0, sizeof(media_timed_event));
	event_time = inTime;
	type = inType;
	pointer = inPointer;
	cleanup = inCleanup;
	data = inData;
	bigdata = inBigdata;

	if (inUserData)
	{
		size_t copySize = 64;
		if (dataSize < 64)
			copySize = dataSize;
		if (copySize > 0)
			memcpy(user_data, inUserData, copySize);
	}
}


media_timed_event::media_timed_event(const media_timed_event &event)
{
	memcpy((void *)this, (void *)&event, sizeof(media_timed_event));
}

void 
media_timed_event::operator=(const media_timed_event &event)
{
	memcpy((void *)this, (void *)&event, sizeof(media_timed_event));
}


media_timed_event::~media_timed_event()
{
	memset((void *)this, 0, sizeof(media_timed_event));
}


#include "rt_allocator.h"
#include <list>

typedef list<media_timed_event, rt_allocator<media_timed_event> > _te_list_type;

class _event_queue_imp {
	public:
		void * operator new(size_t s)	{return rtm_alloc(NULL, s);}
		void operator delete(void *p, size_t s)	{rtm_free(p);}
		
		_te_list_type					fEvents;
		mutable int32 					fLock;
		sem_id							fSem;
		BTimedEventQueue::cleanup_hook	fCleanup;
		void *							fCleanupContext;
		
		inline							_event_queue_imp();
		inline							~_event_queue_imp();
		inline bool						lock() const;
		inline void						unlock() const;
		
		status_t 						add_event(const media_timed_event &event);
		status_t						remove_event(const media_timed_event *event);
		status_t 						remove_first_event(media_timed_event *event);

		bool							has_events() const;
		int32							event_count() const;

		const media_timed_event * 			first_event() const;
		bigtime_t						first_event_time() const;
		const media_timed_event * 			last_event() const;
		bigtime_t						last_event_time() const;
		
		const media_timed_event *				find_first_match(
											bigtime_t eventTime,
											BTimedEventQueue::time_direction direction,
											bool inclusive = true,
											int32 eventType = BTimedEventQueue::B_ANY_EVENT);

		status_t						do_for_each(
											BTimedEventQueue::for_each_hook hook,
											void *context,
											bigtime_t eventTime,
											BTimedEventQueue::time_direction direction,
											bool inclusive = true,
											int32 eventType = BTimedEventQueue::B_ANY_EVENT);

		status_t						set_cleanup_hook(
											BTimedEventQueue::cleanup_hook hook,
											void *context);
static 	BTimedEventQueue::queue_action	flush_entry(media_timed_event *event, void *context);
		BTimedEventQueue::queue_action	flush(media_timed_event *event);
		status_t 						flush_events(
											bigtime_t eventTime,
											BTimedEventQueue::time_direction direction,
											bool inclusive = true,
											int32 eventType = BTimedEventQueue::B_ANY_EVENT);

};

struct _lock {
	public:
		_lock(const _event_queue_imp * q) : qu(q) { ok = qu->lock(); }
		~_lock() { if (ok) qu->unlock(); }
		bool operator!() { return !ok; }
	private:
		const _event_queue_imp * qu;
		bool ok;
};

template<BTimedEventQueue::time_direction m_direction, bool m_inclusive = false>
class _time_comp_t {
public:
	bigtime_t m_time;
	inline _time_comp_t(
			bigtime_t time) :
		m_time(time)
		{
		}
	inline bool operator()(
			media_timed_event & event)
		{
			switch (m_direction)
			{
			case BTimedEventQueue::B_ALWAYS:
				return true;
			case BTimedEventQueue::B_BEFORE_TIME:
				return (event.event_time < m_time) || (m_inclusive && (event.event_time == m_time));
			case BTimedEventQueue::B_AT_TIME:
				return event.event_time == m_time;
			case BTimedEventQueue::B_AFTER_TIME:
				return (event.event_time > m_time) || (m_inclusive && (event.event_time == m_time));
			default:
				assert("shouldn't be here" == NULL);
			}
			return false;
		}
};

class _time_comp {
public:
	bigtime_t m_time;
	BTimedEventQueue::time_direction m_direction;
	bool m_inclusive;
	inline _time_comp(
			bigtime_t time,
			BTimedEventQueue::time_direction direction,
			bool inclusive) :
		m_time(time),
		m_direction(direction),
		m_inclusive(inclusive)
		{
		}
	inline bool operator()(
			media_timed_event & event)
		{
			switch (m_direction)
			{
			case BTimedEventQueue::B_ALWAYS:
				return true;
			case BTimedEventQueue::B_BEFORE_TIME:
				return (event.event_time < m_time) || (m_inclusive && (event.event_time == m_time));
			case BTimedEventQueue::B_AT_TIME:
				return event.event_time == m_time;
			case BTimedEventQueue::B_AFTER_TIME:
				return (event.event_time > m_time) || (m_inclusive && (event.event_time == m_time));
			default:
				assert("shouldn't be here" == NULL);
			}
			return false;
		}
};

bool operator==(const media_timed_event & a, const media_timed_event & b)
{
	if(	(a.event_time == b.event_time) && (a.type == b.type) &&
		(a.pointer == b.pointer) && (a.cleanup == b.cleanup) &&
		(a.data == b.data) && (a.bigdata == b.bigdata))
		{
			uint64 *a_udata = (uint64 *)a.user_data;
			uint64 *b_udata = (uint64 *)b.user_data;

			for (int ix = 0; ix < 8; ix++)
			{
				if (a_udata[ix] != b_udata[ix])
					return false;
			}			
			return true;
		}
	else return false;
}

bool operator!=(const media_timed_event & a, const media_timed_event & b)
{
	if(	(a.event_time == b.event_time) && (a.type == b.type) &&
		(a.pointer == b.pointer) && (a.cleanup == b.cleanup) &&
		(a.data == b.data) && (a.bigdata == b.bigdata))
		{
			uint64 *a_udata = (uint64 *)a.user_data;
			uint64 *b_udata = (uint64 *)b.user_data;

			for (int ix = 0; ix < 8; ix++)
			{
				if (a_udata[ix] == b_udata[ix])
					return false;
			}			
			return true;
		}
	else return true;
}

bool operator<(const media_timed_event & a, const media_timed_event & b)
{
	if (a.event_time < b.event_time) return true;
	if (a.event_time > b.event_time) return false;
	if (a.type < b.type) return true;
	if (a.type > b.type) return false;
	if (a.data < b.data) return true;
	if (a.data > b.data) return false;
	if (a.bigdata < b.bigdata) return true;
	if (a.bigdata > b.bigdata) return false;

	uint64 *a_udata = (uint64 *)a.user_data;
	uint64 *b_udata = (uint64 *)b.user_data;
	for (int ix = 0; ix < 8; ix++)
	{
		if (a_udata[ix] < b_udata[ix]) return true;
		if (a_udata[ix] > b_udata[ix]) return false;
	}
	
	if (a.pointer < b.pointer) return true;
	if (a.pointer > b.pointer) return false;
	if (a.cleanup < b.cleanup) return true;
	if (a.cleanup > b.cleanup) return false;
	return false;
}

bool operator>(const media_timed_event & a, const media_timed_event &b)
{
	if (a.event_time > b.event_time) return true;
	if (a.event_time < b.event_time) return false;
	if (a.type > b.type) return true;
	if (a.type < b.type) return false;
	if (a.data > b.data) return true;
	if (a.data < b.data) return false;
	if (a.bigdata > b.bigdata) return true;
	if (a.bigdata < b.bigdata) return false;

	uint64 *a_udata = (uint64 *)a.user_data;
	uint64 *b_udata = (uint64 *)b.user_data;
	for (int ix = 0; ix < 8; ix++)
	{
		if (a_udata[ix] > b_udata[ix]) return true;
		if (a_udata[ix] < b_udata[ix]) return false;
	}
	
	if (a.pointer > b.pointer) return true;
	if (a.pointer < b.pointer) return false;
	if (a.cleanup > b.cleanup) return true;
	if (a.cleanup < b.cleanup) return false;
	return false;
}

_event_queue_imp::_event_queue_imp() :
	fLock(1), fSem(-1), fCleanup(NULL), fCleanupContext(NULL)
{
	fSem = create_sem(0, "_event_queue_imp");
}

_event_queue_imp::~_event_queue_imp()
{
	delete_sem(fSem);
	fCleanup = NULL;
	fCleanupContext = NULL;
}

bool 
_event_queue_imp::lock() const
{
	LOCKINFO("_event_queue_imp.lock()\n");
	if (atomic_add(&fLock, -1) < 1)
		if (acquire_sem(fSem) < B_OK)
			return false;
	return true;
}

void 
_event_queue_imp::unlock() const
{
	LOCKINFO("_event_queue_imp.unlock()\n");
	if (atomic_add(&fLock, 1) < 0)
		release_sem(fSem);
}

status_t 
_event_queue_imp::add_event(const media_timed_event &event)
{
	_lock l(this);
	if (!l) return B_ERROR;
	try {
		_te_list_type::iterator iter(find_if(fEvents.begin(), fEvents.end(), _time_comp(event.event_time, BTimedEventQueue::B_AFTER_TIME, false)));
		fEvents.insert(iter, event);
	}
	catch(...)
	{
		return B_ERROR;
	}
	return B_OK;
}

status_t
_event_queue_imp::remove_event(const media_timed_event *event)
{
	_lock l(this);
	if (!l) return B_ERROR;
	try {
		// Make a copy. DON'T pass in the actual object in the list
		fEvents.remove(media_timed_event(*event));
		return B_OK;
	}
	catch(...)
	{
		return B_ERROR;
	}
}

status_t 
_event_queue_imp::remove_first_event(media_timed_event *event)
{
	_lock l(this);
	if (!l) {
		ERROR("_event_queue_imp::remove_first_event: could not lock queue\n");
		return B_ERROR;
	}
	try {
		status_t err = B_OK;
		_te_list_type::iterator i(fEvents.begin());
		if (i != fEvents.end())
		{
			if (event)
				*event = *i;
			fEvents.erase(i);
			err =  B_OK;
		}
		else {
			ERROR("_event_queue_imp::remove_first_event: could not find event size: %ld\n", fEvents.size());
			err = B_ERROR;
		}
		return err;
	}
	catch(...)
	{
		return B_ERROR;
	}
}

bool 
_event_queue_imp::has_events() const
{
	return !fEvents.empty();
}

int32 
_event_queue_imp::event_count() const
{
	return fEvents.size();
}

const media_timed_event *
_event_queue_imp::first_event() const
{
	_lock l(this);
	if (!l) return NULL;
	try {
		const media_timed_event *event = NULL;
		_te_list_type::const_iterator i(fEvents.begin());
		if (i != fEvents.end())
			event =  &(*i);
		else event =  NULL;
		return event;
	}
	catch(...)
	{
		return NULL;
	}
}

bigtime_t
_event_queue_imp::first_event_time() const
{
	_lock l(this);
	if (!l) return B_INFINITE_TIMEOUT;
	try {
		_te_list_type::const_iterator i(fEvents.begin());
		if (i != fEvents.end())
			return (*i).event_time;
		else return B_INFINITE_TIMEOUT;
	}
	catch(...)
	{
		return B_INFINITE_TIMEOUT;
	}
}

const media_timed_event *
_event_queue_imp::last_event() const
{
	_lock l(this);
	if (!l) return NULL;
	try {
		const media_timed_event *event = NULL;
		_te_list_type::const_reverse_iterator i(fEvents.rbegin());
		if (i != fEvents.rend())
			event =  &(*i);
		else event =  NULL;
		return event;
	}
	catch(...)
	{
		return NULL;
	}
}

bigtime_t
_event_queue_imp::last_event_time() const
{
	_lock l(this);
	if (!l) return B_INFINITE_TIMEOUT;
	try {
		bigtime_t ret = B_INFINITE_TIMEOUT;
		_te_list_type::const_reverse_iterator i(fEvents.rbegin());
		if (i != fEvents.rend())
			ret = (*i).event_time;
		else ret =  B_INFINITE_TIMEOUT;
		return ret;
	}
	catch(...)
	{
		return B_INFINITE_TIMEOUT;
	}
}

#if 1
const media_timed_event *
_event_queue_imp::find_first_match(bigtime_t eventTime, BTimedEventQueue:: time_direction direction,
		bool inclusive, int32 eventType)
{
	CALL("_event_queue_imp::find_first_match\n");
	_lock l(this);
	if (!l) return NULL;
	try {
		_te_list_type::iterator current;
		_te_list_type::iterator end;
		switch(direction) {
			case BTimedEventQueue::B_BEFORE_TIME: {
					INFO("B_BEFORE\n");
					current = fEvents.begin();
					end = find_if(current, fEvents.end(), _time_comp(eventTime, BTimedEventQueue::B_AFTER_TIME, !inclusive));
				}
				break;
			
			case BTimedEventQueue::B_AFTER_TIME: {
					INFO("B_AFTER\n");
					current = find_if(fEvents.begin(), fEvents.end(), _time_comp(eventTime, BTimedEventQueue::B_AFTER_TIME, inclusive));
					end = fEvents.end();
				}
				break;

			case BTimedEventQueue::B_AT_TIME: {
					INFO("B_AT\n");
					current = find_if(fEvents.begin(), fEvents.end(), _time_comp_t<BTimedEventQueue::B_AT_TIME, true>(eventTime));
					end = find_if(current, fEvents.end(), _time_comp_t<BTimedEventQueue::B_AFTER_TIME, false>(eventTime));
				}
				break;
			case BTimedEventQueue::B_ALWAYS : {
					INFO("B_ALWAYS\n");
					current = fEvents.begin();
					end = fEvents.end();
				}
				break;
			default:
				assert("shouldn't be here" == NULL);
				return NULL;
		}

		while (current != end)
		{
			if ((eventType == BTimedEventQueue::B_ANY_EVENT) ||
					(eventType == (*current).type))
			{
				return &(*current);
			}
			else current++;
		}
		return NULL;
	}
	catch(...) {
		return NULL;
	}
}
#else

const media_timed_event *
_event_queue_imp::find_first_match(bigtime_t eventTime, BTimedEventQueue:: time_direction direction,
		bool inclusive, int32 eventType) const
{
	CALL("_event_queue_imp::find_first_match\n");
	_lock l(this);
	if (!l) return NULL;
	try {
		_te_list_type::const_iterator current;
		_te_list_type::const_iterator end;
		switch(direction) {
			case BTimedEventQueue::B_BEFORE_TIME: {
					INFO("B_BEFORE\n");
					current = fEvents.begin();
					_te_list_type::const_iterator toPlace = fEvents.end();
					end = find_if(current, toPlace, _time_comp(eventTime, BTimedEventQueue::B_AFTER_TIME, !inclusive));
				}
				break;
			
			case BTimedEventQueue::B_AFTER_TIME: {
					INFO("B_AFTER\n");
					_te_list_type::const_iterator fromPlace = fEvents.begin();
					_te_list_type::const_iterator toPlace = fEvents.end();
					current = find_if(fromPlace, toPlace, _time_comp(eventTime, BTimedEventQueue::B_AFTER_TIME, inclusive));
					end = fEvents.end();
				}
				break;

			case BTimedEventQueue::B_AT_TIME: {
					INFO("B_AT\n");
					_te_list_type::const_iterator fromPlace = fEvents.begin();
					_te_list_type::const_iterator toPlace = fEvents.end();
					current = find_if(fromPlace, toPlace, _time_comp_t<BTimedEventQueue::B_AT_TIME, true>(eventTime));
					end = find_if(current, toPlace, _time_comp_t<BTimedEventQueue::B_AFTER_TIME, false>(eventTime));
				}
				break;
			case BTimedEventQueue::B_ALWAYS : {
					INFO("B_ALWAYS\n");
					current = fEvents.begin();
					end = fEvents.end();
				}
				break;
			default:
				assert("shouldn't be here" == NULL);
				return NULL;
		}

		while (current != end)
		{
			if ((eventType == BTimedEventQueue::B_ANY_EVENT) ||
					(eventType == (*current).type))
			{
				return &(*current);
			}
			else current++;
		}
		return NULL;
	}
	catch(...) {
		return NULL;
	}
}
#endif

status_t 
_event_queue_imp::do_for_each(BTimedEventQueue:: for_each_hook hook, void *context, bigtime_t eventTime,
	BTimedEventQueue:: time_direction direction, bool inclusive, int32 eventType)
{
	CALL("_event_queue_imp::do_for_each\n");
	status_t err = B_ERROR;
	_lock l(this);
	if (!l) return B_ERROR;
	try {
		_te_list_type::iterator current;
		_te_list_type::iterator end;
		switch(direction) {
			case BTimedEventQueue::B_BEFORE_TIME: {
					INFO("B_BEFORE\n");
					current = fEvents.begin();
					end = find_if(current, fEvents.end(), _time_comp(eventTime, BTimedEventQueue::B_AFTER_TIME, !inclusive));
				}
				break;
			
			case BTimedEventQueue::B_AFTER_TIME: {
					INFO("B_AFTER\n");
					current = find_if(fEvents.begin(), fEvents.end(), _time_comp(eventTime, BTimedEventQueue::B_AFTER_TIME, inclusive));
					end = fEvents.end();
				}
				break;

			case BTimedEventQueue::B_AT_TIME: {
					INFO("B_AT\n");
					current = find_if(fEvents.begin(), fEvents.end(), _time_comp_t<BTimedEventQueue::B_AT_TIME, true>(eventTime));
					end = find_if(current, fEvents.end(), _time_comp_t<BTimedEventQueue::B_AFTER_TIME, false>(eventTime));
				}
				break;
			case BTimedEventQueue::B_ALWAYS : {
					INFO("B_ALWAYS\n");
					current = fEvents.begin();
					end = fEvents.end();
				}
				break;
			default:
				assert("shouldn't be here" == NULL);
				return B_BAD_VALUE;
		}

		_te_list_type::iterator old(current);
		bool done = false;
		bool resort = false;

		while (current != end && !done)
		{
			if ((eventType == BTimedEventQueue::B_ANY_EVENT) ||
					(eventType == (*current).type))
			{
				old = current;
				bool removed = false;
				BTimedEventQueue::queue_action action = hook(&(*current), context);
				switch(action)
				{
					case BTimedEventQueue::B_DONE:
						done = true;
						break;
					
					case BTimedEventQueue::B_NO_ACTION:
						break;
						
					case BTimedEventQueue::B_REMOVE_EVENT:
						removed = true;
						current++;
						// Make a copy. DON'T pass in the actual object in the list
						fEvents.remove(media_timed_event(*old));
						break;
					case BTimedEventQueue::B_RESORT_QUEUE:
						resort = true;
						break;
				}
				if (!removed) current++;
			}
			else current++;
		}
		// cleanup everything
		if (resort)
			fEvents.sort();
		return B_OK;
	}
	catch(...) {
		return B_ERROR;
	}
}

status_t 
_event_queue_imp::set_cleanup_hook(BTimedEventQueue:: cleanup_hook hook, void *context)
{
	fCleanup = hook;
	fCleanupContext = context;
	return B_OK;
}

BTimedEventQueue::queue_action 
_event_queue_imp::flush_entry(media_timed_event *event, void *context)
{
	if (!event || !context)
		return BTimedEventQueue::B_NO_ACTION;
		
	_event_queue_imp *imp = reinterpret_cast<_event_queue_imp *>(context);
	if (!imp)
		return BTimedEventQueue::B_NO_ACTION;
		
	return imp->flush(event);
}

BTimedEventQueue::queue_action 
_event_queue_imp::flush(media_timed_event *event)
{
	if (event)
	{
		bool passOn = false;
		switch(event->cleanup) {
			case BTimedEventQueue::B_RECYCLE_BUFFER: 
				{	BBuffer *buffer = (BBuffer *) event->pointer;
					buffer->Recycle();
					passOn = false;
				}
				break;
				
			case BTimedEventQueue::B_NO_CLEANUP:
				passOn = false;
				break;
				
			default:
				passOn = true;
				break;	
		}
	
		if (passOn && fCleanup)
			fCleanup(event, fCleanupContext);
	}
	return BTimedEventQueue::B_REMOVE_EVENT;
}

#if 0
status_t 
_event_queue_imp::flush_events(bigtime_t eventTime, BTimedEventQueue::time_direction direction,
				bool inclusive, int32 eventType)
{
	CALL("_event_queue_imp::flush_events\n");
	status_t err = B_ERROR;
	_lock l(this);
	if (!l) return B_ERROR;
	try {
		_te_list_type::iterator current;
		_te_list_type::iterator end;
		switch(direction) {
			case BTimedEventQueue::B_BEFORE_TIME: {
					INFO("B_BEFORE\n");
					current = fEvents.begin();
					end = find_if(current, fEvents.end(), _time_comp(eventTime, BTimedEventQueue::B_AFTER_TIME, !inclusive));
				}
				break;
			
			case BTimedEventQueue::B_AFTER_TIME: {
					INFO("B_AFTER\n");
					current = find_if(fEvents.begin(), fEvents.end(), _time_comp(eventTime, BTimedEventQueue::B_AFTER_TIME, inclusive));
					end = fEvents.end();
				}
				break;

			case BTimedEventQueue::B_AT_TIME: {
					INFO("B_AT\n");
					current = find_if(fEvents.begin(), fEvents.end(), _time_comp_t<BTimedEventQueue::B_AT_TIME, true>(eventTime));
					end = find_if(current, fEvents.end(), _time_comp_t<BTimedEventQueue::B_AFTER_TIME, false>(eventTime));
				}
				break;
			case BTimedEventQueue::B_ALWAYS : {
					INFO("B_ALWAYS\n");
					current = fEvents.begin();
					end = fEvents.end();
				}
				break;
			default:
				assert("shouldn't be here" == NULL);
				return B_BAD_VALUE;
		}

		while (current != end)
		{
			if ((eventType == BTimedEventQueue::B_ANY_EVENT) ||
					(eventType == (*current).type))
			{
				cleanup_event(&(*current));
				atomic_add(&fCount, -1);
				_te_list_type::iterator old(current);
				current++;
				fEvents.erase(old);
			}
			else current++;
		}

		err = B_OK;
	}
	catch(...) {
		return B_ERROR;
	}
	return err;
}
#endif


/* BTimedEventQueue */

void * BTimedEventQueue::operator new(size_t s)				{return rtm_alloc(NULL, s);}

void BTimedEventQueue::operator delete(void *p, size_t s)	{rtm_free(p);}


BTimedEventQueue::BTimedEventQueue() :
	fImp(NULL)
{
	fImp = new _event_queue_imp;
}

BTimedEventQueue::~BTimedEventQueue()
{
	if (fImp)
	{
		FlushEvents(0, B_ALWAYS);
		delete fImp; fImp = NULL;
	}
}

status_t 
BTimedEventQueue::AddEvent(const media_timed_event &event)
{
	if (event.type == B_NO_EVENT || event.type == B_ANY_EVENT || event.event_time == B_INFINITE_TIMEOUT)
		return B_BAD_VALUE;
		
	if (fImp)
		return fImp->add_event(event);
	else return B_NO_INIT;
}

status_t 
BTimedEventQueue::RemoveEvent(const media_timed_event *event)
{
	if (fImp)
		return fImp->remove_event(event);
	else return B_NO_INIT;
}

status_t 
BTimedEventQueue::RemoveFirstEvent(media_timed_event *out_event)
{
	if (fImp)
		return fImp->remove_first_event(out_event);
	else return B_NO_INIT;
}

bool 
BTimedEventQueue::HasEvents() const
{
	if (fImp)
	{
		return fImp->has_events();
	}
	else return false;
}

int32 
BTimedEventQueue::EventCount() const
{
	if (fImp)
		return fImp->event_count();
	else return 0;
}

const media_timed_event *
BTimedEventQueue::FirstEvent() const
{
	if (fImp)
		return fImp->first_event();
	else return NULL;
}

bigtime_t
BTimedEventQueue::FirstEventTime() const
{
	if (fImp)
		return fImp->first_event_time();
	else return B_INFINITE_TIMEOUT;
}

const media_timed_event *
BTimedEventQueue::LastEvent() const
{
	if (fImp)
		return fImp->last_event();
	else return NULL;
}

bigtime_t 
BTimedEventQueue::LastEventTime() const
{
	if (fImp)
		return fImp->last_event_time();
	else return B_INFINITE_TIMEOUT;
}

const media_timed_event *
BTimedEventQueue::FindFirstMatch(bigtime_t eventTime, time_direction direction, bool inclusive, int32 eventType)
{
	if (fImp)
		return fImp->find_first_match(eventTime, direction, inclusive, eventType);
	else return NULL;
}

status_t 
BTimedEventQueue::DoForEach(for_each_hook hook, void *context,
					bigtime_t eventTime, time_direction direction, bool inclusive, int32 eventType)
{
	if(fImp)
		return fImp->do_for_each(hook, context, eventTime, direction, inclusive, eventType);
	else return B_NO_INIT;
}

void 
BTimedEventQueue::SetCleanupHook(cleanup_hook hook, void *context)
{
	if (fImp)
		fImp->set_cleanup_hook(hook, context);
}

status_t 
BTimedEventQueue::FlushEvents(bigtime_t eventTime, time_direction direction, bool inclusive, int32 eventType)
{
	if (fImp) {
		return fImp->do_for_each(fImp->flush_entry, fImp, eventTime, direction, inclusive, eventType);
	} else return B_NO_INIT;
}

status_t BTimedEventQueue::_Reserved_BTimedEventQueue_0(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_1(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_2(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_3(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_4(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_5(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_6(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_7(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_8(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_9(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_10(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_11(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_12(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_13(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_14(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_15(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_16(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_17(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_18(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_19(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_20(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_21(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_22(void *, ...) {return B_ERROR;}
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_23(void *, ...) {return B_ERROR;}

