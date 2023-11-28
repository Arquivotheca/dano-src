
#include <support2/Debug.h>
#include <support2/CallStack.h>
#include <support2/String.h>

#include <media2/Buffer.h>
#include <media2/BufferGroup.h>
#include "buffer_source_impl.h"
#include "AreaCloneCache.h"
#include "BufferOutlet.h"

using B::Support2::BValue;

#include <support2/StdIO.h>

#define checkpoint \
//berr << "thid " << find_thread(0) << " BBuffer(" << this << ") -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

#define callstack \
//checkpoint 
//berr->BumpIndentLevel(1); 
//B::Support2::BCallStack cs; cs.Update(); cs.LongPrint(berr); berr << endl; 
//berr->BumpIndentLevel(-1);

namespace B {
namespace Media2 {

using namespace Private;

// value fields
const BValue gBufferIDKey("id");
const BValue gBufferAreaKey("area");
const BValue gBufferOffsetKey("offset");
const BValue gBufferSizeKey("size");
const BValue gBufferListAreaKey("listarea");
const BValue gBufferInfoKey("info");

BBuffer::BBuffer()
{
	Init();
}

BBuffer::BBuffer(area_id area, uint32 offset, uint32 size)
{
	Init(area, offset, size);
}


BBuffer::BBuffer(void *ptr, area_id area, uint32 offset, uint32 size)
{
	Init(ptr, area, offset, size);
}


BBuffer::BBuffer(const flat_buffer & flattened)
{
	Init(flattened);
}


BBuffer::BBuffer(const BValue &source)
{
	Init();
	mInitStatus = Import(source);
}


BBuffer::BBuffer(const BBuffer &clone)
	: mLocalArea(B_ERROR),
	  mLocalListArea(B_ERROR)
{
	operator=(clone);
}

BBuffer &
BBuffer::operator=(const BBuffer &clone)
{
	if (&clone==this)
		return *this;
	
	Clear();

	mInitStatus = clone.mInitStatus;
	mID = clone.mID;
	mArea = clone.mArea;
	mLocalArea = clone.mLocalArea;
	if (mLocalArea >= 0 && mLocalArea != mArea)
	{
		DEBUG_ONLY(status_t err =)AreaCloneCache::Instance()->Acquire(mArea);
		ASSERT(err == B_OK);
	}
	mOffset = clone.mOffset;
	mSize = clone.mSize;
	mData = clone.mData;
	mListArea = clone.mListArea;
	mLocalListArea = clone.mLocalListArea;
	if (mLocalListArea >= 0 && mLocalListArea != mListArea)
	{
		DEBUG_ONLY(status_t err =)AreaCloneCache::Instance()->Acquire(mListArea);
		ASSERT(err == B_OK);
	}
	mList = clone.mList;
	mFlags = clone.mFlags;
	mInfo = clone.mInfo;
	
	return *this;
}


BBuffer::~BBuffer()
{
	Clear();
}

status_t 
BBuffer::SetTo(area_id area, uint32 offset, uint32 size)
{
	Clear();
	Init(area, offset, size);
	return mInitStatus;
}

status_t 
BBuffer::SetTo(void *ptr, area_id area, uint32 offset, uint32 size)
{
	Clear();
	Init(ptr, area, offset, size);
	return mInitStatus;
}

status_t 
BBuffer::SetTo(const B::Private::flat_buffer &flattened)
{
	Clear();
	Init(flattened);
	return mInitStatus;
}

status_t 
BBuffer::InitCheck() const
{
	return mInitStatus;
}

int32 
BBuffer::ID() const
{
	return mID;
}

area_id 
BBuffer::Area() const
{
	return mArea;
}

uint32 
BBuffer::AreaOffset() const
{
	return mOffset;
}

media_buffer_group_id 
BBuffer::Owner() const
{
	return mListArea;
}

const BValue &
BBuffer::Info() const
{
	return mInfo;
}

BValue &
BBuffer::EditInfo()
{
	return mInfo;
}

void 
BBuffer::SetInfo(const BValue &info)
{
	mInfo = info;
}

BValue 
BBuffer::AsValue() const
{
	BValue value;
	if (mArea >= 0)
	{
		if (mID > INVALID_BUFFER)
		{
			value.Overlay(gBufferIDKey, BValue::Int32(mID));
			value.Overlay(gBufferListAreaKey, BValue::Int32(mListArea));
		}
		value.Overlay(gBufferAreaKey, BValue::Int32(mArea));
		value.Overlay(gBufferOffsetKey, BValue::Int32((int32)mOffset));
		value.Overlay(gBufferSizeKey, BValue::Int32((int32)mSize));
	}
	if (mInfo) value.Overlay(gBufferInfoKey, mInfo);
	return value;
}

status_t 
BBuffer::Flatten(flat_buffer * outFlat)
{
	outFlat->area = mArea;
	outFlat->offset = mOffset;
	outFlat->size = mSize;
	outFlat->id = mID;
	outFlat->owner.list_area = mListArea;
	return B_OK;
}

void 
BBuffer::Clear()
{
	if (mLocalArea >= 0 && mLocalArea != mArea)
	{
		AreaCloneCache::Instance()->Release(mArea);
	}
	if (mLocalListArea >= 0 && mLocalListArea != mListArea)
	{
		AreaCloneCache::Instance()->Release(mListArea);
	}
	Init();
}

status_t 
BBuffer::AcquireBuffer()
{
checkpoint
	if (mID <= INVALID_BUFFER || (mFlags & RELEASED)) return B_NOT_ALLOWED;
	ASSERT(mList);
	switch (mList->source_type)
	{
		case media_buffer_source::BUFFER_GROUP:
			return static_cast<media_buffer_group*>(mList)->Acquire(mID);
		
		case media_buffer_source::BUFFER_RING:
			return static_cast<media_buffer_ring*>(mList)->Acquire(mID);
		
		default:
			return B_ERROR;
	}
}

status_t 
BBuffer::ReleaseBuffer()
{
checkpoint
	if (mID <= INVALID_BUFFER || (mFlags & RELEASED)) return B_NOT_ALLOWED;
	ASSERT(mList);
	bool released;
	status_t err = B_ERROR;
	ASSERT (mID < (int32)mList->count);
	switch (mList->source_type)
	{
		case media_buffer_source::BUFFER_GROUP:	
			err = static_cast<media_buffer_group*>(mList)->Release(mID, &released);
			break;
		case media_buffer_source::BUFFER_RING:	
			err = static_cast<media_buffer_ring*>(mList)->Release(mID, &released);
			break;
	}
	if (err >= B_OK && released)
	{
		Clear();
	}
	
	return err;
}

// ---------------------------------------------------------------- //

void 
BBuffer::Init()
{
	Init(-1, 0, 0);
}

void 
BBuffer::Init(area_id area, uint32 offset, uint32 size)
{
	mID = INVALID_BUFFER;
	mArea = area;
	mLocalArea = -1;
	mOffset = offset;
	mSize = size;
	mData = 0;
	mListArea = -1;
	mLocalListArea = -1;
	mList = 0;
	mFlags = 0;
	mInfo.Undefine();
	mInitStatus = (mArea >= 0) ? CloneBuffer() : B_OK;
}

void 
BBuffer::Init(void * ptr, area_id area, uint32 offset, uint32 size)
{
	mID = INVALID_BUFFER;
	mArea = area;
	mLocalArea = -1;
	mOffset = offset;
	mSize = size;
	mData = ptr;
	mListArea = -1;
	mLocalListArea = -1;
	mList = 0;
	mFlags = 0;
	mInfo.Undefine();
	mInitStatus = B_OK;
}

void 
BBuffer::Init(const B::Private::flat_buffer & flattened)
{
	Init(flattened.area, flattened.offset, flattened.size);
	
	mID = flattened.id;
	mArea = flattened.area;
	mListArea = flattened.owner.list_area;
	
	mInitStatus = B_OK;
	
	status_t err = CloneBuffer();
	if (err < B_OK) mInitStatus = err;

	err = CloneList();
	if (err < B_OK) mInitStatus = err;
}

status_t 
BBuffer::AttachLocal(media_buffer_id id, area_id listArea)
{
	status_t err;
	if (mID > INVALID_BUFFER || (mFlags & RELEASED)) return B_NOT_ALLOWED;
	if (id <= INVALID_BUFFER) return B_BAD_VALUE;
	if (listArea < 0) return B_BAD_VALUE;
	mID = id;
	area_info info;
	err = get_area_info(listArea, &info);
	if (err < 0) return err;
	if (mLocalListArea >= 0 && mLocalListArea != mListArea)
	{
		AreaCloneCache::Instance()->Release(mListArea);
	}
	mListArea = listArea;
	mLocalListArea = listArea;
	mList = static_cast<media_buffer_group*>(info.address);
	if (!mData)
	{
		err = CloneBuffer();
		if (err < B_OK) return err;
	}
	return B_OK;
}

status_t 
BBuffer::Import(const BValue & value)
{
	status_t err;
	err = value[gBufferAreaKey].GetInt32(&mArea);
	if (err < 0) return err;
	err = value[gBufferOffsetKey].GetInt32((int32*)&mOffset);
	if (err < 0) return err;
	err = value[gBufferSizeKey].GetInt32((int32*)&mSize);
	if (err < 0) return err;
	
	value[gBufferIDKey].GetInt32(&mID);
	value[gBufferListAreaKey].GetInt32(&mListArea);
	
	mInfo = value[gBufferInfoKey];
	
	err = CloneBuffer();
	if (err < B_OK) return err;
	
	err = CloneList();
	if (err < B_OK) return err;
	
	return B_OK;
}

status_t 
BBuffer::CloneBuffer()
{
	void * base;
	mLocalArea = AreaCloneCache::Instance()->Clone(mArea, &base);
	if (mLocalArea < 0) return mLocalArea;
	mData = (uint8*)base + mOffset;
	return B_OK;
}

status_t 
BBuffer::CloneList()
{
	void * base;
	mLocalListArea = AreaCloneCache::Instance()->Clone(mListArea, &base);
	if (mLocalListArea < 0) return mLocalListArea;
	mList = static_cast<media_buffer_group*>(base);
	return B_OK;
}

void
BBuffer::SetRange (size_t offset, size_t length)
{
	ASSERT(offset+length<=mSize);
	
	mData=(char *)mData+offset;
	mSize=length;
}

} } // B::Media2
