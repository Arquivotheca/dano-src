/***************************************************************************
//
//	File:			media2/Buffer.h
//
//	Description:	Represents a shared chunk of memory.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_BUFFER_H_
#define _MEDIA2_BUFFER_H_

#include <support2/Atom.h>
#include <support2/Value.h>

#include <media2/MediaDefs.h>
#include <media2/MediaFormat.h>

#include <OS.h>

namespace B {

namespace Private {
	struct media_buffer_source;
	class BBufferOutlet;
	class BBufferInlet;
	struct flat_buffer;
}

namespace Media2 {

class BBufferGroup;

using namespace Support2;

class BBuffer
{
public:
			enum special_id
			{
				ANY_BUFFER		= -2,
				INVALID_BUFFER	= -1
			};

									BBuffer();
									BBuffer(area_id area, uint32 offset, uint32 size);
									BBuffer(void * ptr, area_id area, uint32 offset, uint32 size);
									BBuffer(const BValue & source);
									BBuffer(const BBuffer & clone);
			BBuffer &				operator=(const BBuffer & clone);
									~BBuffer();

			status_t				SetTo(area_id area, uint32 offset, uint32 size);
			status_t				SetTo(void * ptr, area_id area, uint32 offset, uint32 size);
			status_t				InitCheck() const;

			int32					ID() const;
			area_id					Area() const;
			uint32					AreaOffset() const;
			uint32					Size() const;
			void *					Data() const;
			media_buffer_group_id	Owner() const;

			const BValue &			Info() const;
			BValue &				EditInfo();
			void					SetInfo(const BValue & info);
			
			BValue					AsValue() const;
	inline							operator BValue() const { return AsValue(); }
	
	// sets object to an invalid state
			void					Clear();

			void					SetRange (size_t offset, size_t length);
			void					ClearRange();
			
	// note that these don't operate like the BAtom ref-counting methods.
	// when a buffer's reference count drops to zero, ownership of the memory
	// region is represents transfers back to the source -- the actual BBuffer
	// instance won't be deleted, and it is in fact safe to construct a BBuffer
	// on the stack.
	// AcquireBuffer() and ReleaseBuffer() will only succeed if the buffer has
	// an owner and has been acquired from the group (so calls to AcquireBuffer()
	// after the final ReleaseBuffer() will fail.)
			status_t				AcquireBuffer();
			status_t				ReleaseBuffer();

private:
	friend	class	BBufferGroup;

			enum flag_t {
				RELEASED			= 1
			};

			void					Init();
			void					Init(area_id area, uint32 offset, uint32 size);
			void					Init(void * ptr, area_id area, uint32 offset, uint32 size);
			void					Init(const B::Private::flat_buffer & flattened);
			status_t				AttachLocal(media_buffer_id id, media_buffer_group_id owner);
			status_t				Import(const BValue & value);
			status_t				CloneBuffer();
			status_t				CloneList();

	friend	class B::Private::BBufferOutlet;
	friend	class B::Private::BBufferInlet;

									BBuffer(const B::Private::flat_buffer & flattened);
			status_t				SetTo(const B::Private::flat_buffer & flattened);
			status_t				Flatten(B::Private::flat_buffer * outFlattened);

			status_t				mInitStatus;
			media_buffer_id			mID;

			area_id					mArea;
	mutable	area_id					mLocalArea;
			uint32					mOffset;
			uint32					mSize;
	mutable	void *					mData;

			area_id					mListArea;
			area_id					mLocalListArea;
			B::Private::media_buffer_source * mList;
			int32					mFlags;
			
			BValue					mInfo;			
};

inline uint32 
BBuffer::Size() const
{
	return mSize;
}

inline void *
BBuffer::Data() const
{
	return mData;
}

}; }; // namespace B::Media2
#endif //_MEDIA2_BUFFER_H_
