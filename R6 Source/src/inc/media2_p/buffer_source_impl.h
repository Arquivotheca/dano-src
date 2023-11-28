#ifndef _MEDIA2_BUFFERSOURCE_IMPL_PRIVATE_
#define _MEDIA2_BUFFERSOURCE_IMPL_PRIVATE_

#include <support2/Locker.h>
#include "buffer_source.h"

namespace B {
namespace Private {

using namespace Support2;

// the BBufferGroup implementation of a buffer source

struct media_buffer_group : public media_buffer_source
{
	BLocker			lock;
	sem_id			waiting_sem;
	int32			waiting_count;
	sem_id			reclaim_sem;

	status_t		Acquire(int32 id);
	status_t		Release(int32 id, bool * out_released);
};

// the ringbuffer implementation of a buffer source
// ASSUMPTIONS:
// - producer maintains a head ID, which is initially equal to tail_id
//   (ID >= 0 is a buffer index.)  we may support a synchronous head/tail
//   reclaim/reset at some point.
// - media_buffer_source::count remains constant.
// - buffers are acquired and released in proper ring order
//
// head & tail are initialized upon buffer-source connection
// (the head index can be stored independantly by the
// producer.) thus system-call penalties are:
// acquire: acquire_sem_etc(available_sem)
// release: tail_lock.Lock()/Unlock(), release_sem_etc(available_sem)

struct media_buffer_ring : public media_buffer_source
{
	sem_id			available_sem;
	BLocker			tail_lock;
	size_t			tail_id;

	status_t		Acquire(int32 id);
	status_t		Release(int32 id, bool * out_released);
};

} } // B::Private
#endif //_MEDIA2_BUFFERSOURCE_IMPL_PRIVATE_
