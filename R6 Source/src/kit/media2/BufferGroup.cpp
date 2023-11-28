
#include <support2/Autolock.h>
#include <support2/Debug.h>

#include <media2/BufferGroup.h>
#include "buffer_source_impl.h"
#include "AreaCloneCache.h"

using B::Support2::BAutolock;

#include <support2/StdIO.h>

#define checkpoint \
//berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Media2 {

using namespace Private;

const int32 gBufferGroupListSize = B_PAGE_SIZE * 2;

BBufferGroup::BBufferGroup() :
	mBufferArea(-1)
{
	mInitStatus = InitList();
}

BBufferGroup::BBufferGroup(uint32 size, uint32 count, uint32 placement, uint32 lock) :
	mBufferArea(-1)
{
	mInitStatus = InitList();
	if (mInitStatus < 0) return;

	// +++ fancy cacheline-aliasing-prevention stuff goes here, if we need it
	
	uint32 areaSize = (size * count + B_PAGE_SIZE-1) & ~(B_PAGE_SIZE-1);
	void * base;
	mBufferArea = create_area("BBufferGroup::mBufferArea", &base,
		placement, areaSize, lock, B_READ_AREA | B_WRITE_AREA);
	if (mBufferArea < 0)
	{
		mInitStatus = mBufferArea;
		return;
	}
	for (uint32 n = 0; n < count; n++)
	{
		status_t err = AddBuffer(BBuffer(mBufferArea, n * size, size));
		if (err < B_OK)
		{
			mInitStatus = err;
			return;
		}
	}
}

BBufferGroup::~BBufferGroup()
{
}

status_t
BBufferGroup::Acquired(const void * id)
{
	return BAtom::Acquired(id);
}

status_t
BBufferGroup::Released(const void * id)
{
checkpoint
	WaitForBuffers(); // +++ timeout?
	DestroyList();
	mBuffers.MakeEmpty();
	if (mBufferArea >= 0)
	{
		delete_area(mBufferArea);
		mBufferArea = -1;
	}
	return BAtom::Released(id);
//	return B_ERROR;		// don't let object get deleted, yet.
}

status_t 
BBufferGroup::InitCheck() const
{
	return mInitStatus;
}

media_buffer_group_id 
BBufferGroup::ID() const
{
	return mListArea;
}

status_t 
BBufferGroup::AddBuffer(const BBuffer &buffer)
{
	status_t err = mList->lock.Lock();
	if (err < 0) return err;
	
	int32 id = mList->count++;

	media_buffer_entry & e = mList->EntryAt(id);

	e.size = buffer.Size();
	e.state = media_buffer_entry::FREE;
	
	mBufferLock.Lock();
	mBuffers.AddItem(buffer);
	BBuffer & b = mBuffers.EditItemAt(id);
	b.AttachLocal(id, mListArea);
	mBufferLock.Unlock();

	mList->lock.Unlock();	

	int32 oldVal = atomic_and(&mList->waiting_count, 0);
	if (oldVal > 0) release_sem_etc(mList->waiting_sem, oldVal, B_DO_NOT_RESCHEDULE);
	release_sem_etc(mList->reclaim_sem, 1, B_DO_NOT_RESCHEDULE);

	return B_OK;
}

int32 
BBufferGroup::CountBuffers() const
{
	BAutolock _l(mBufferLock.Lock());
	return mBuffers.CountItems();
}

const BBuffer &
BBufferGroup::BufferAt(int32 index) const
{
	BAutolock _l(mBufferLock.Lock());
	return mBuffers[index];
}

status_t 
BBufferGroup::AcquireBuffer(BBuffer *outBuffer, int32 id, bigtime_t timeout)
{
checkpoint
	status_t err;
	if (mInitStatus < 0) return mInitStatus;
	while (true)
	{
		// ** Look for a valid buffer
		err = mList->lock.Lock();
		if (err < B_OK) return err;

		atomic_add(&mList->waiting_count, 1);
		
		int32 targetIndex = -1;
		int32 count = mList->count;
		
		if (id >= 0)
		{
			if (id >= count)
			{
				DEBUGGER("BBufferGroup::AcquireBuffer(): invalid ID");
				mList->lock.Unlock();
				return B_BAD_VALUE;
			}
			media_buffer_entry & e = mList->EntryAt(id);
			if (e.state == media_buffer_entry::FREE)
			{
				targetIndex = id;
			}
		}
		else
		{
			for (int32 n = 0; n < count; n++)
			{
				media_buffer_entry & e = mList->EntryAt(n);
				if (e.state == media_buffer_entry::FREE)
				{
					targetIndex = n;
					break;
				}
			}
		}
		
		if (targetIndex != -1)
		{
			// ** We've found a buffer: mark it acquired and return it.
			id = targetIndex;
			media_buffer_entry & e = mList->EntryAt(id);
			e.refs = 1;
			e.state = media_buffer_entry::ACQUIRED;
			if (atomic_add(&mList->waiting_count, -1) == 0)
			{
				//	A buffer was recycled since we incremented waitingCount,
				//	which means that we were "notified".  Acquire the sem to keep
				//	things in sync, and reset waiting count.
				while (acquire_sem(mList->waiting_sem) == B_INTERRUPTED) {}

				mList->waiting_count = 0;
			}
			
			mList->lock.Unlock();

			while (acquire_sem(mList->reclaim_sem) == B_INTERRUPTED) {}
			
			if (outBuffer) *outBuffer = mBuffers[targetIndex];
			return B_OK;
		}
		
		mList->lock.Unlock();
		
		// ** Wait for another buffer to be freed up
		
		while ((err = acquire_sem_etc(mList->waiting_sem, 1, B_TIMEOUT, timeout)) == B_INTERRUPTED) {}
		if (err == B_TIMED_OUT || err == B_WOULD_BLOCK)
		{
			err = mList->lock.Lock();
			if (err < 0) return err;
			if (atomic_add(&mList->waiting_count, -1) == 0)
			{
				//	A buffer was released since we incremented waitingCount,
				//	which means that we were "notified", even if we did time out.
				//	Acquire the sem to keep	things in sync, and reset waiting count.
				while (acquire_sem(mList->waiting_sem) == B_INTERRUPTED) {}
				mList->waiting_count = 0;
			}
			mList->lock.Unlock();
			return B_TIMED_OUT;
		}
		else if (err < B_OK) return err;
	}
	return B_ERROR;
}

ssize_t 
BBufferGroup::ListBuffers(BVector<BBuffer> *outBuffers)
{
	BAutolock _l(mBufferLock.Lock());
	outBuffers->AddVector(mBuffers);
	return mBuffers.CountItems();
}

status_t 
BBufferGroup::WaitForBuffers(bigtime_t timeout)
{
checkpoint
	int32 count = mList->count;
	status_t err = acquire_sem_etc(mList->reclaim_sem, count, B_TIMEOUT, timeout);
	if (err < 0) return err;
	return release_sem_etc(mList->reclaim_sem, count, B_DO_NOT_RESCHEDULE);
}

status_t 
BBufferGroup::InitList()
{
	// construct the media_buffer_group
	mListArea = create_area("BBufferGroup::mList", (void**)&mList,
		B_ANY_ADDRESS, gBufferGroupListSize, B_FULL_LOCK,
		B_READ_AREA | B_WRITE_AREA);
	if (mListArea < 0) return mListArea;

	// init media_buffer_source
	mList->capacity =
		((gBufferGroupListSize - sizeof(media_buffer_group)) /
		sizeof(media_buffer_entry)) + 1;
	mList->count = 0;
	mList->entry_size = sizeof(media_buffer_entry);
	mList->entry_offset = sizeof(media_buffer_group);
	mList->source_type = media_buffer_source::BUFFER_GROUP;

	// init media_buffer_group
	new ((void*)&mList->lock) BLocker();
	mList->waiting_sem = -1;
	mList->reclaim_sem = -1;
	mList->waiting_sem = create_sem(0, "media_buffer_group::waiting_sem");
	if (mList->waiting_sem < 0) return mList->waiting_sem;
	mList->waiting_count = 0;
	mList->reclaim_sem = create_sem(0, "media_buffer_group::reclaim_sem");
	if (mList->reclaim_sem < 0) return mList->reclaim_sem;

	return B_OK;
}

void 
BBufferGroup::DestroyList()
{
	mList->lock.~BLocker();
	delete_sem(mList->waiting_sem);
	delete_sem(mList->reclaim_sem);
	mList = 0;
	delete_area(mListArea);
	mListArea = -1;
}

} } // B::Media2
