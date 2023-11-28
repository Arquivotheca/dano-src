/*	BBufferGroup.cpp	*/

#include <trinity_p.h>
#include <BufferGroup.h>
#include <Buffer.h>
#include <tr_debug.h>
//	#if defined(__GNUC__) && defined(__INTEL__)
//	#define USE_REALTIME_ALLOCATOR 1
//	#include <realtime_allocator.h>
//	#else
//	#define USE_REALTIME_ALLOCATOR 0
//	#endif

#include <locked_allocator.h>
#include <rt_map.h>
#include <SupportDefs.h>
#include <Locker.h>
#include <buffer_id_cache.h>

#include "shared_buffer.h"

#define MIN_BUFFER_SIZE 64
#define CACHELINE_FIX 64

#define DO_NOTHING(x...)

#if !NDEBUG
#define FPRINTF fprintf
#else
#define FPRINTF DO_NOTHING
#endif

#define ERRORS FPRINTF
#define AVAIL DO_NOTHING //FPRINTF

#define BUFFERGROUP_CAN_RECLAIM 0x00000001

void *
_buffer_id_cache::operator new(
	size_t size)
{
	void * r = rtm_alloc(NULL, size);
	if (!r) throw std::bad_alloc();
	return r;
}


void
_buffer_id_cache::operator delete(
	void * ptr)
{
	if (ptr != NULL) rtm_free(ptr);
}


status_t
BBufferGroup::IBufferGroup()
{
	_m_local_err = 0;
	_mBufferListArea = create_area("buffer group list",
		(void**)&_mBufferList,B_ANY_ADDRESS,4096*2,
		B_LAZY_LOCK,B_READ_AREA|B_WRITE_AREA);
	_mBufferList->ownerPort = -1;
	_mBufferList->flags = 0;
//	_mBufferList->ownerNode = -1;
	_mBufferList->benaphoreCount = 0;
	_mBufferList->benaphoreSem = create_sem(0,"buffer group lock");
	_mBufferList->bufferCount = 0;
	_mBufferList->bufferSlots = ((8192-sizeof(_shared_buffer_list)) / sizeof(_shared_buffer_desc)) + 1;
	_mBufferList->waitingCount = 0;
	_mBufferList->waitingSem = create_sem(0,"buffer waiting sem");
	_mBufferList->reclaimSem = create_sem(0,"buffer reclaim sem");
	_mBufferCount = 0;
	_mFlags = 0;

//	((_BMediaRosterP *)BMediaRoster::Roster())->RegisterBufferGroup(this);

	_mBufferCache = new _buffer_id_cache(static_cast<_BMediaRosterP*>(BMediaRoster::Roster()));

	((_BMediaRosterP *)BMediaRoster::Roster())->RegisterDedicatedArea(_mBufferListArea);
	((_BMediaRosterP *)BMediaRoster::Roster())->NewAreaUser(_mBufferListArea);
	((_BMediaRosterP *)BMediaRoster::Roster())->AddBufferGroupToBeRegistered(_mBufferListArea);

	return B_OK;
}

status_t
BBufferGroup::Lock()
{
	return _mBufferList->Lock();
}

status_t
BBufferGroup::Unlock()
{
	return _mBufferList->Unlock();
}

status_t
BBufferGroup::AddToList(BBuffer *buffer)
{
	status_t error = Lock();
	if (error != B_OK)
		return error;
		
	buffer->_mHeader.size_used = buffer->_mSize;
	buffer->_mHeader.data_offset = 0;
	_mBufferList->buffers[_mBufferList->bufferCount].id = buffer->ID();
	_mBufferList->buffers[_mBufferList->bufferCount].size = buffer->_mSize;
	_mBufferList->buffers[_mBufferList->bufferCount].owner = 0;
	_mBufferList->bufferCount++;
	_mBufferCount++;
	Unlock();

	buffer->SetOwnerArea(_mBufferListArea);

	int32 oldVal = atomic_and(&_mBufferList->waitingCount,0);
	if (oldVal > 0) release_sem_etc(_mBufferList->waitingSem,oldVal,B_DO_NOT_RESCHEDULE);
	release_sem_etc(_mBufferList->reclaimSem,1,B_DO_NOT_RESCHEDULE);

	return B_OK;
}

BBufferGroup::BBufferGroup(
	size_t size,
	int32 count,
	uint32 placement,
	uint32 lock)
{
	_m_init_error = IBufferGroup();
	/*	construct actual BBuffers */
	void * addr = NULL;
	size_t rounded_size;
	/* We round each buffer to an even multiple of cache lines, plus we try */
	/* to prevent cache line aliasing by spacing power-of-two buffer sizes */
	/* an extra little bit apart (currently, 64 == 2 cache lines). */
	if (size < MIN_BUFFER_SIZE) {
		rounded_size = MIN_BUFFER_SIZE;
	}
	else {
		rounded_size = size + CACHELINE_FIX;
		rounded_size -= rounded_size % CACHELINE_FIX;
	}
	size_t padded_size = count * rounded_size;
	padded_size = (padded_size+B_PAGE_SIZE-1)&~(B_PAGE_SIZE-1);
#if DEBUG
	if (padded_size > 2000000) {
		char str[100];
		sprintf(str, "BBufferGroup::BBufferGroup(): padded_size is %ld (big!)\n", padded_size);
		debugger(str);
	}
#endif
	area_id the_area = create_area("media buffers", &addr, placement, 
		padded_size, lock, B_READ_AREA | B_WRITE_AREA);
	if (the_area < B_OK)
	{
		_m_init_error = the_area;
		dlog("BufferGroup size 0x%x returns %x", padded_size, _m_init_error);
		return;
	}
	((_BMediaRosterP *)BMediaRoster::Roster())->RegisterDedicatedArea(the_area);
	/*	add them to our list */
	for (int ix=0; ix<count; ix++) {
		/* we add buffers with rounded_size size, because that's how big they are */
		BBuffer * buf = new BBuffer(the_area, ix*rounded_size, rounded_size);
		if (buf->Data()) {
//			((_BMediaRosterP *)BMediaRoster::Roster())->BindBuffer(buf, this);
			AddToList(buf);
		}
		else {
			delete buf;
			_m_init_error = B_ERROR;
		}
	}
	dlog("BufferGroup returns %x", _m_init_error);
}


BBufferGroup::BBufferGroup()
{
	_m_init_error = IBufferGroup();
}


BBufferGroup::BBufferGroup(
	int32 count,
	const media_buffer_id * buffers)
{
	_m_init_error = IBufferGroup();
	if (_m_init_error == B_OK) {
		for (int ix=0; ix<count; ix++) {
			BBuffer * buf = _mBufferCache->FindBuffer(buffers[ix]);
//			BBuffer * buf = ((_BMediaRosterP *)BMediaRoster::Roster())->FindBuffer(buffers[ix]);
			if (buf != NULL) {
//				((_BMediaRosterP *)BMediaRoster::Roster())->BindBuffer(buf, this);
				ASSERT(buffers[ix] == buf->ID());
				AddToList(buf);
			}
			else {
				ERRORS(stderr, "Cannot add buffer %ld in new BBufferGroup\n", buffers[ix]);
			}
		}
		dlog("Added buffers: BBufferGroup(%d, %x)", count, buffers);
	}
	else {
		dlog("ERROR: BBufferGroup %x", _m_init_error);
	}
}


BBufferGroup::~BBufferGroup()
{
	/* wait for all buffers */
	if (!(_mFlags & BUFFERGROUP_CAN_RECLAIM)) WaitForBuffers();
	((_BMediaRosterP *)BMediaRoster::CurrentRoster())->AddBufferGroupToBeUnregistered(_mBufferListArea);

	/* we always orphan the buffers (they may have other parents, too) */
	((_BMediaRosterP *)BMediaRoster::Roster())->OrphanReclaimableBuffers(this);
//	((_BMediaRosterP *)BMediaRoster::Roster())->UnregisterBufferGroup(this);

	delete_sem(_mBufferList->benaphoreSem);
	delete_sem(_mBufferList->waitingSem);
static team_id myself;
static thread_info ti;
	if (myself == 0) {
		get_thread_info(find_thread(NULL), &ti);
		myself = ti.team;
	}
	set_sem_owner(_mBufferList->reclaimSem, myself);	//	so we can delete it
	delete_sem(_mBufferList->reclaimSem);
	((_BMediaRosterP *)BMediaRoster::Roster())->RemoveAreaUser(_mBufferListArea);
	delete _mBufferCache;

	_mBufferCache = (_buffer_id_cache *)0x75757575;
	_mBufferList = (_shared_buffer_list *)0x75757575;
	_mBufferListArea = (area_id)0x75757575;
}


status_t
BBufferGroup::InitCheck()
{
	return _m_init_error;
}

/*	This function is used by users who create an empty group and		*/
/*	add their own custom buffers (BBitmaps, BDirectWindows, ...)		*/
/*	In that case, the buffer ID should be set to <= 0 (which is done by	*/
/*	the buffer_clone_info constructor, actually).						*/
status_t
BBufferGroup::AddBuffer(
	const buffer_clone_info & info,
	BBuffer ** out_buffer)
{
	assert(info.buffer <= 0);

	BBuffer * b = new BBuffer(info);
	if (!b->Data()) {
		delete b;
		return B_NO_MEMORY;
	}
	
//	((_BMediaRosterP *)BMediaRoster::Roster())->BindBuffer(buf, this);
	if (out_buffer) *out_buffer = b;
	AddToList(b);
	return B_OK;
}

status_t
BBufferGroup::_RequestBuffer(
	size_t size,
	media_buffer_id wantID,
	BBuffer **buffer,
	bigtime_t timeout)	/* 0 means no timeout */
{
	status_t r;
	int32 i,count,best,bestSize,id=-1;

	while (1) {
		r = Lock();
		if (r != B_OK)
			return r;

		if (_mFlags & BUFFERGROUP_CAN_RECLAIM) {
			Unlock();
			return B_MEDIA_BUFFERS_NOT_RECLAIMED;
		}

		atomic_add(&_mBufferList->waitingCount,1);
	
		best = -1;
		bestSize = 0x7FFFFFFF;
		count = _mBufferList->bufferCount;
		for (i=0;i<count;i++) {
			if ((_mBufferList->buffers[i].owner == 0) &&
				((size_t)_mBufferList->buffers[i].size >= size) &&
				(_mBufferList->buffers[i].size < bestSize) &&
				((wantID == -1) || (_mBufferList->buffers[i].id == wantID))) {
				best = i;
				bestSize = _mBufferList->buffers[i].size;
			}
		}

		if (best != -1) {
			id = _mBufferList->buffers[best].id;
			_mBufferList->buffers[best].owner = _mBufferList->ownerPort;
			if (atomic_add(&_mBufferList->waitingCount,-1) == 0) {
				/*	A buffer was recycled since we incremented waitingCount,
					which means that we were "notified".  Acquire the sem to keep
					things in sync, and reset waiting count. */
				while ((r=acquire_sem(_mBufferList->waitingSem)) == B_INTERRUPTED);
				_mBufferList->waitingCount = 0;
			}
			
			Unlock();
			
			while ((r=acquire_sem(_mBufferList->reclaimSem)) == B_INTERRUPTED);
			if (!(*buffer)) *buffer = _mBufferCache->FindBuffer(id);
			(*buffer)->_mHeader.owner = _mBufferListArea;
			(*buffer)->_mHeader.time_source = 0;
			return B_OK;
		}
		
		Unlock();

		while ((r=acquire_sem_etc(_mBufferList->waitingSem,1,B_TIMEOUT,timeout)) == B_INTERRUPTED);
		if ((r == B_TIMED_OUT) || (r == B_WOULD_BLOCK)) {
			r = Lock();
			if (r != B_OK)
				return r;
				
			if (atomic_add(&_mBufferList->waitingCount,-1) == 0) {
				/*	A buffer was released since we incremented waitingCount,
					which means that we were "notified", even if we did time out.
					Acquire the sem to keep	things in sync, and reset waiting count. */
				while ((r=acquire_sem(_mBufferList->waitingSem)) == B_INTERRUPTED);
				_mBufferList->waitingCount = 0;
			}
			Unlock();
			return B_TIMED_OUT;
		} else if (r < B_OK)
			return r;		// Sem might have gone away.
							// This can occur if the buffer group is deleted
							// with threads waiting on it.
	}
	return B_ERROR;
}

/* This function should not be called while an outstanding */
/* ReclaimAllBuffers() request is active. */
/* To make sure that doesn't happen, after returning from */
/* SetBufferGroup(), a buffer producer should not know about */
/* the old group anymore. */
BBuffer *
BBufferGroup::RequestBuffer(
	size_t size,
	bigtime_t timeout)	/* 0 means no timeout */
{
	BBuffer *buffer=NULL;
	_m_local_err = _RequestBuffer(size,-1,&buffer,timeout);
	return buffer;
}

/* This function should not be called while an outstanding */
/* ReclaimAllBuffers() request is active. */
/* To make sure that doesn't happen, after returning from */
/* SetBufferGroup(), a buffer producer should not know about */
/* the old group anymore. */
/* Use buffer affinity for F1 and F2 raw_video buffers, for instance */
status_t
BBufferGroup::RequestBuffer(
	BBuffer * buffer,
	bigtime_t timeout)	/* 0 means no timeout */
{
	if (!buffer) return B_BAD_VALUE;
	return (_m_local_err = _RequestBuffer(0,buffer->ID(),&buffer,timeout));
}

status_t
BBufferGroup::RequestError()
{
	return _m_local_err;
}

status_t
BBufferGroup::AddBuffersTo(
	BMessage * message,
	const char * name,
	bool needLock)
{
	status_t err,error=B_OK;

	if (needLock) {
		err = Lock();
		if (err != B_OK)
			return err;
	}
	
	for (int32 i=0;i<_mBufferCount;i++) {
		if ((err = message->AddInt32(name, _mBufferList->buffers[i].id)) != B_OK) error = err;
	}
	if (needLock) Unlock();
	return error;
}

status_t
BBufferGroup::ReclaimAllBuffers()
{
	status_t err = Lock();
	if (err != B_OK)
		return err;

	if (!(_mFlags & BUFFERGROUP_CAN_RECLAIM)) {
		Unlock();
		return B_MEDIA_CANNOT_RECLAIM_BUFFERS;
	}
	
	err = ((_BMediaRosterP *)BMediaRoster::Roster())->ReclaimOutputBuffers(this);
	if (err == B_OK) {
		atomic_and(&_mBufferList->flags, ~SB_FLAG_WAITING);
		_mFlags &= ~BUFFERGROUP_CAN_RECLAIM;
	}

	Unlock();

//fixme	why is this releasing the semaphore a second time???
//	if (err == B_OK && (err = WaitForBuffers()) == B_OK)
//		release_sem_etc(_mBufferList->reclaimSem, _mBufferCount, B_DO_NOT_RESCHEDULE);
	if (err == B_OK)
		err = WaitForBuffers();

#if !NDEBUG
int32 count = 0;
get_sem_count(_mBufferList->reclaimSem, &count);
assert(count == _mBufferList->bufferCount);
#endif
	for (int ix=0; ix<_mBufferCount; ix++) {
		BBuffer * b = _mBufferCache->FindBuffer(_mBufferList->buffers[ix].id);
		if (b != 0) {
			b->SetOwnerArea(_mBufferListArea);
		}
	}

	return err;
}


status_t
BBufferGroup::WaitForBuffers()
{
	status_t err = acquire_sem_etc(_mBufferList->reclaimSem, _mBufferCount, 0, B_INFINITE_TIMEOUT);
	if (err != B_OK)
		return err;
	_mBufferList->ownerPort = -1;	//	unknown who the owner port is now, but it doesn't matter
assert(_mBufferCount == _mBufferList->bufferCount);
	return release_sem_etc(_mBufferList->reclaimSem, _mBufferCount, B_DO_NOT_RESCHEDULE);
}


//	CanReclaim() means that we're not currently in possession of the buffers,
//	but we can be again by calling ReclaimAllBuffers() after first telling
//	whomever we gave the buffers to to hand them back.
bool
BBufferGroup::CanReclaim()
{
	return (_mFlags & BUFFERGROUP_CAN_RECLAIM);
}

void
BBufferGroup::WillReclaim()
{
	status_t error = Lock();
	if (error != B_OK)
		return;
		
	atomic_or(&_mBufferList->flags, SB_FLAG_WAITING);
	_mFlags |= BUFFERGROUP_CAN_RECLAIM;
	Unlock();
	WaitForBuffers();
}

status_t
BBufferGroup::CountBuffers(
	int32 * out_count)
{
	if (out_count == NULL) return B_BAD_VALUE;
	*out_count = _mBufferCount;
	return B_OK;
}

status_t
BBufferGroup::GetBufferList(
	int32 data_count,
	BBuffer ** out_bufs)
{
	int32 count = data_count;
	if ((count < 1) || (out_bufs == NULL)) return B_BAD_VALUE;

	status_t error = Lock();
	if (error != B_OK)
		return error;
	
	if (count > _mBufferCount) count = _mBufferCount;
	for (int32 i=0;i<count;i++) {
		out_bufs[i] = _mBufferCache->FindBuffer(_mBufferList->buffers[i].id);
	}
	Unlock();

	return B_OK;
}

void
BBufferGroup::SetOwnerPort(
	port_id port)
{
	if (_mBufferList->ownerPort == port) return;
	_mBufferList->ownerPort = port;
}

/*	Below are other ideas for implementations of _RequestBuffer/ReleaseBuffer
	I think the one I chose has a lot going for it. */

/*
	uint32 bits = ;
	bits = (bits & 0x55555555) + ((bits>> 1) & 0x55555555);
	bits = (bits & 0x33333333) + ((bits>> 2) & 0x33333333);
//	bits = (bits & 0x07070707) + ((bits>> 4) & 0x07070707);
	bits = bits + (bits>> 4);
	bits = (bits & 0x000F000F) + ((bits>> 8) & 0x000F000F);
	bits = (bits & 0x0000001F) + (bits>>16);
*/
#if 0
	inline status_t ReleaseBuffer(media_buffer_id id) {
		int32 oldVal;
		status_t err;
		for (int32 i=0;i<bufferCount;i++) {
			if (buffers[i].id == id) {
				oldVal = atomic_and(&buffers[i].state,0);
				if (oldVal > 1) release_sem_etc(waitingSem,oldVal-1,B_DO_NOT_RESCHEDULE);
				else if (oldVal == 0) printf("ack! buffer recycled when not in use?\n");
				else if (oldVal == -1) printf("ack! buffer recycled during RequestBuffer (bug)?\n");
				release_sem_etc(reclaimSem,1,B_DO_NOT_RESCHEDULE);
				return B_OK;
			};
		};
		return B_ERROR;
	};
#endif
#if 0
	inline status_t ReleaseBuffer(media_buffer_id id, bigtime_t timeout=0) {
		status_t err;
		if ((err=Lock(timeout)) == B_OK) {
			for (int32 i=0;i<bufferCount;i++) {
				if (buffers[i].id == id) {
					buffers[i].state = 0;
					if (waitingCount) {
						release_sem_etc(waitingSem,waitingCount,B_DO_NOT_RESCHEDULE);
						waitingCount = 0;
					};
					release_sem_etc(reclaimSem,1,B_DO_NOT_RESCHEDULE);
					Unlock();
					return B_OK;
				};
			};
			err = B_ERROR;
			Unlock();
		};
		return err;
	};
#endif

#if 0
status_t
BBufferGroup::_RequestBuffer(
	size_t size,
	media_buffer_id wantID,
	BBuffer **buffer,
	bigtime_t timeout)	/* 0 means no timeout */
{
	status_t error;
	int32 oldVal,i,count,best,bestSize,id=-1;

	Lock();

	if (_mFlags & BUFFERGROUP_CAN_RECLAIM) {
		Unlock();
		return B_MEDIA_BUFFERS_NOT_RECLAIMED;
	};

	best = -1;
	bestSize = 0x7FFFFFFF;
	count = _mBufferList->bufferCount;
	for (i=0;i<count;i++) {
		if ((_mBufferList->buffers[i].size > size) &&
			(_mBufferList->buffers[i].size < bestSize) &&
			((wantID == -1) || (_mBufferList->buffers[i].id == wantID))) {
			if (!atomic_add(&_mBufferList->buffers[i].state,1)) {
				best = i;
				bestSize = _mBufferList->buffers[i].size;
				id = _mBufferList->buffers[i].id;
			};
		};
	};

	/*	We've traversed the whole list.  Here is the state we have set up: any buffer in
		the queue that we could possibly use we have tried to claim.  Those that were free
		are currently held by us.  Those that were not, we have registered a notification
		request	on, for anyone who happens to recycle them. */	
		
	while (1) {
		if ((best != -1) || (error == B_TIMED_OUT)) {
			/*	Either we have claimed a buffer we're happy with or 
				Go back through all of the buffers we either claimed or registered a
				notification on which aren't our successful buffer (which may be all of them)
				and clear them. */
			for (i=0;i<count;i++) {
				if ((_mBufferList->buffers[i].size > size) &&
					(_mBufferList->buffers[i].size < bestSize) &&
					((wantID == -1) || (_mBufferList->buffers[i].id == wantID)) && (best != i)) {
					if (atomic_add(_mBufferList->buffers[i].state,-1) == 0) {
						/*	Someone has recycled this buffer since we looked at it last.  That
							means they released a semaphore as asked by our notification request.
							We have to acquire the sem, to keep things in sync, and zero it out
							again.  Nobody is looking at this state right now because the buffer
							isn't in use and the other RequestBuffers are locked out, so this
							simple assignment is legal. */
						acquire_sem(_mBufferList->waitingSem);
						mBufferList->buffers[i].state = 0;
					};
				};
			};

			Unlock();

			if (error == B_TIMED_OUT) return B_TIMED_OUT;
			
			/*	Return happily */
			while ((r=acquire_sem(_mBufferList->reclaimSem)) == B_INTERRUPTED);
			if (!(*buffer)) *buffer = _mBufferCache->FindBuffer(id);
			(*buffer)->_mHeader.owner = _mBufferListArea;
			return B_OK;
		};

		Unlock();

		/*	If (best == -1), we did not find any buffers we liked that were available.
			We have registered notifications on all buffers that we could use, so let's
			wait for one of them to become available. */

//		printf("_RequestBuffer having to wait (%d,%d,%d)\n",_mBufferCount,size,wantID);
		while ((error=acquire_sem_etc(_mBufferList->waitingSem,1,timeout?B_TIMEOUT:0,timeout)) == B_INTERRUPTED);

		Lock();

		if (error != B_TIMED_OUT) {
			/*	We've gotten a notification.  It wasn't neccessarily meant for us; it could
				have been for anyone who was waiting for a buffer.  But let's go check. */
			best = -1;
			bestSize = 0x7FFFFFFF;
			count = _mBufferList->bufferCount;
			for (i=0;i<count;i++) {
				if ((_mBufferList->buffers[i].size > size) &&
					(_mBufferList->buffers[i].size < bestSize) &&
					((wantID == -1) || (_mBufferList->buffers[i].id == wantID))) {
					if (!_mBufferList->buffers[i].state) {
						best = i;
						bestSize = _mBufferList->buffers[i].size;
						id = _mBufferList->buffers[i].id;
					};
				};
			};
			
			if (best != -1) _mBufferList->buffers[best].state = 1;
		};
		
		/*	At this point, we will be in one of three states:
		
				1) We have claimed a usable buffer.  Yay!  (best != -1)
				2) We have gotten tired of waiting.  Ok.   (error == B_TIMED_OUT)
				3) Either the notification wasn't for us, or someone got to the buffer
				   before we did, because we couldn't find it. (best == -1)
		*/
	};

	return B_ERROR;
};
#endif
#if 0
status_t
BBufferGroup::_RequestBuffer(
	size_t size,
	media_buffer_id wantID,
	BBuffer **buffer,
	bigtime_t timeout)	/* 0 means no timeout */
{
	status_t r;
	int32 i,count,best,bestSize,id=-1;

	while (id == -1) {
		if ((r=Lock()) == B_TIMED_OUT) break;

		if (_mFlags & BUFFERGROUP_CAN_RECLAIM) {
			Unlock();
//			printf("_RequestBuffer return B_MEDIA_BUFFERS_NOT_RECLAIMED\n");
			return B_MEDIA_BUFFERS_NOT_RECLAIMED;
		};
	
		best = -1;
		bestSize = 0x7FFFFFFF;
		count = _mBufferList->bufferCount;
		for (i=0;i<count;i++) {
			if ((_mBufferList->buffers[i].state == 0) &&
				(_mBufferList->buffers[i].size > size) &&
				(_mBufferList->buffers[i].size < bestSize) &&
				((wantID == -1) || (_mBufferList->buffers[i].id == wantID))) {
				best = i;
				bestSize = _mBufferList->buffers[i].size;
			};
		};
	
		if (best != -1) {
			id = _mBufferList->buffers[best].id;
			_mBufferList->buffers[best].state = 1;
		} else _mBufferList->waitingCount++;
	
		Unlock();

		if (id == -1) {
//			printf("_RequestBuffer having to wait (%d,%d,%d)\n",_mBufferCount,size,wantID);
			while ((r=acquire_sem_etc(_mBufferList->waitingSem,1,timeout?B_TIMEOUT:0,timeout)) == B_INTERRUPTED);
			if (r == B_TIMED_OUT) {
				Lock();
				if (i=_mBufferList->waitingCount)
					_mBufferList->waitingCount--;
				Unlock();
				if (!i) acquire_sem(_mBufferList->waitingSem);
				break;
			};
		};
	};

	if (id != -1) {
		while ((r=acquire_sem(_mBufferList->reclaimSem)) == B_INTERRUPTED);
		if (!(*buffer)) *buffer = _mBufferCache->FindBuffer(id);
		(*buffer)->_mHeader.owner = _mBufferListArea;
//		printf("_RequestBuffer return B_OK with id=%d, data=%08x\n",id,(*buffer)->Data());
		return B_OK;
	};

//	printf("_RequestBuffer return '%s'\n",strerror(r));
	return r;
};
#endif
