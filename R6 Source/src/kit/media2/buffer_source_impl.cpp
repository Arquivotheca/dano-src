#include "buffer_source_impl.h"
#include <support2/Autolock.h>

namespace B {
namespace Private {

using namespace Support2;

status_t 
media_buffer_group::Acquire(int32 id)
{
	if (id < 0) return B_BAD_INDEX;
	media_buffer_entry & e = EntryAt(id);
	if (e.state != media_buffer_entry::ACQUIRED) return B_NOT_ALLOWED;
	if (atomic_add(&e.refs, 1) <= 0)
	{
		atomic_add(&e.refs, -1);
		return B_NOT_ALLOWED;
	}
	return B_OK;
}

status_t 
media_buffer_group::Release(int32 id, bool *released)
{
	media_buffer_entry & e = EntryAt(id);
	if (e.state != media_buffer_entry::ACQUIRED)
	{
		DEBUGGER("media_buffer_group: Buffer being recycled, but it is not in use!");
		return B_ERROR;
	}
	if (atomic_add(&e.refs, -1) == 1)
	{
		e.state = media_buffer_entry::FREE;
		int32 oldVal = atomic_and(&waiting_count, 0);
		if (oldVal > 0) release_sem_etc(waiting_sem, oldVal, B_DO_NOT_RESCHEDULE);
		release_sem_etc(reclaim_sem, 1, B_DO_NOT_RESCHEDULE);
		*released = true;
	}
	else
	{
		*released = false;
	}
	return B_OK;
}

status_t 
media_buffer_ring::Acquire(int32 id)
{
	if (id < 0 || id >= (int32)count) return B_BAD_INDEX;
	media_buffer_entry & e = EntryAt(id);
	if (e.state != media_buffer_entry::ACQUIRED) return B_NOT_ALLOWED;
	if (atomic_add(&e.refs, 1) <= 0)
	{
		atomic_add(&e.refs, -1);
		return B_NOT_ALLOWED;
	}
	return B_OK;
}

status_t 
media_buffer_ring::Release(int32 id, bool *out_released)
{
	if (id < 0 || id >= (int32)count) return B_BAD_INDEX;

	BAutolock _l(tail_lock.Lock());
	
	media_buffer_entry & e = EntryAt(id);

	if (atomic_add(&e.refs, -1) == 1)
	{
		if (e.state != media_buffer_entry::ACQUIRED)
		{
			DEBUGGER("media_buffer_ring: Buffer being recycled, but it is not in use!");
			return B_ERROR;		
		}
		if (id != (int32)tail_id)
		{
			DEBUGGER("media_buffer_ring: Buffer released out of order!");
			return B_ERROR;		
		}
		e.state = media_buffer_entry::FREE;
		release_sem_etc(available_sem, 1, B_DO_NOT_RESCHEDULE);
		if (++tail_id > count) tail_id = 0;
		*out_released = true;
	}
	else
	{
		*out_released = false;
	}

	return B_OK;
}

} } // B::Private
