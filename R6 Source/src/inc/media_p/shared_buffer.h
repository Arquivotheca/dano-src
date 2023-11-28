
#if !defined(shared_buffer_h)
#define shared_buffer_h

#include <OS.h>
#include <MediaDefs.h>
#include <Debug.h>

#define SB_FLAG_WAITING 0x1

struct _shared_buffer_desc {
	media_buffer_id			id;
	int32					size;
	int32					owner;
};

struct _shared_buffer_list {

//	int32					ownerNode;
	int32					ownerPort;
	int32					flags;

	int32					benaphoreCount;
	sem_id					benaphoreSem;
	int32					bufferCount;
	int32					bufferSlots;
	sem_id					waitingSem;
	int32					waitingCount;
	sem_id					reclaimSem;
	_shared_buffer_desc		buffers[1];

	inline status_t Lock(bigtime_t timeout=0) {
		status_t err = B_OK;
		if (atomic_add(&benaphoreCount,1) >= 1)
			while ((err=acquire_sem_etc(benaphoreSem,1,timeout?B_TIMEOUT:0,timeout)) == B_INTERRUPTED);
		return err;
	};

	inline status_t Unlock() {
		if (atomic_add(&benaphoreCount,-1) > 1)
			return release_sem_etc(benaphoreSem,1,B_DO_NOT_RESCHEDULE);
		return B_OK;
	};
#if 0
	inline status_t ReleaseBuffer(media_buffer_id id, bigtime_t timeout=0) {
		for (int32 i=0;i<bufferCount;i++) {
			if (buffers[i].id == id) {
				if (buffers[i].owner != 0) {
					buffers[i].owner = 0;
					int32 oldVal = atomic_and(&waitingCount,0);
					if (oldVal > 0) release_sem_etc(waitingSem,oldVal,B_DO_NOT_RESCHEDULE);
					release_sem_etc(reclaimSem,1,B_DO_NOT_RESCHEDULE);
					return B_OK;
				} else {
					DEBUGGER("Buffer being recycled, but it is not in use!");
					return B_ERROR;
				};
			};
		};
		return B_ERROR;
	};
#else
	inline status_t ReleaseBuffer(int32 i, int32 id) {
		if (i < 0) {
			for (i=0; i<bufferCount; i++) {
				if (buffers[i].id == id)
					goto done;
			}
			return B_BAD_INDEX;
		}
	done:
		assert(buffers[i].id == id);
		if ((buffers[i].id == id) && (buffers[i].owner != 0)) {
			buffers[i].owner = 0;
			int32 oldVal = atomic_and(&waitingCount,0);
			if (oldVal > 0) release_sem_etc(waitingSem,oldVal,B_DO_NOT_RESCHEDULE);
			release_sem_etc(reclaimSem,1,B_DO_NOT_RESCHEDULE);
			return B_OK;
		}
		else {
			DEBUGGER("Buffer being recycled, but it is not in use!");
			return B_ERROR;
		}
		return B_ERROR;
	};
#endif
	void RecycleBuffersWithOwner(port_id owner) {
		//	While this might seem like one big race; we only release
		//	buffers with this port id as the owner, and this function is
		//	called from a function which cleans up because the node with
		//	that port is gone, so buffers with that owner will not get
		//	changed. All other buffers, even if they're currently active,
		//	will not have that port as the owner.
		for (int ix=0; ix<bufferCount; ix++) {
			if ((buffers[ix].owner == owner) || ((owner < 0) && (buffers[ix].owner < 0))) {
				ReleaseBuffer(ix, buffers[ix].id);
			}
		}
	}
};

#endif	//	shared_buffer_h
