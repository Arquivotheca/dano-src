/***************************************************************************
//
//	File:			media2/BufferGroup.h
//
//	Description:	Manages a set of BBuffer instances.  Implements
//					IBufferSource to provide a buffer-acquisition service:
//					the buffers can be acquired in any order, and are
//					returned to the BBufferGroup when released.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_BUFFERGROUP_H_
#define _MEDIA2_BUFFERGROUP_H_

#include <support2/Binder.h>
#include <support2/Locker.h>

#include <media2/MediaDefs.h>
#include <media2/Buffer.h>
#include <media2/IBufferSource.h>

#include <OS.h>

namespace B {

namespace Private {
	struct media_buffer_group;
}

namespace Media2 {

using namespace Support2;

class b_shared_buffer_list;

class BBufferGroup : public LBufferSource
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BBufferGroup);

									BBufferGroup();
									BBufferGroup(
										uint32 size,
										uint32 count,
										uint32 placement = B_ANY_ADDRESS,
										uint32 lock = B_FULL_LOCK);

	virtual	status_t				Acquired(const void* id);
	virtual	status_t				Released(const void* id);

			status_t				InitCheck() const;
			media_buffer_group_id	ID() const;

			status_t				AddBuffer(const BBuffer & buffer);
			int32					CountBuffers() const;
			const BBuffer &			BufferAt(int32 index) const;

			// block until all buffers have been recycled
			status_t				WaitForBuffers(bigtime_t timeout = B_INFINITE_TIMEOUT);

			// ** BBufferSource

	virtual	status_t				AcquireBuffer(
										BBuffer * outBuffer,
										int32 id = BBuffer::ANY_BUFFER,
										bigtime_t timeout = B_INFINITE_TIMEOUT);

	virtual ssize_t					ListBuffers(
										BVector<BBuffer> * outBuffers);

protected:
	virtual							~BBufferGroup();

private:
			status_t				InitList();
			void					DestroyList();

			status_t				mInitStatus;
			area_id					mListArea;
			B::Private::media_buffer_group * mList;

			area_id					mBufferArea;
	mutable	BLocker					mBufferLock;
			BVector<BBuffer>		mBuffers;
};

}; }; // namespace B::Media2
#endif //_MEDIA2_BUFFERGROUP_H_
