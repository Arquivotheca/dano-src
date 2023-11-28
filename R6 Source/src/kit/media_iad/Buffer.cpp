/*	BBuffer.cpp	*/


#include "trinity_p.h"
#include "Buffer.h"
#include "tr_debug.h"
#include "shared_buffer.h"

#include <string.h>


#if NDEBUG
#define FPRINTF (void)
#else
#define FPRINTF fprintf
#endif

#define ERRORS FPRINTF


//void
//BBuffer::PrintToStream()
//{
//	fprintf(stderr, "BBuffer ID %d data %x size %d used %d offset %d list %x owner %d\n",
//			ID(), _mData, _mSize, _mHeader.size_used, _mOffset, _mList, (_mList ? _mList->ownerPort : 0));
//}


BBuffer::BBuffer(
	area_id area,
	size_t offset,
	size_t size,
	int32 flags)
{
	memset(&_mHeader, 0, sizeof(_mHeader));
	_mOrigArea = area;
	_mArea = ((_BMediaRosterP *)BMediaRoster::Roster())->NewAreaUser(area);
	_mListArea = _mOrigListArea = B_BAD_VALUE;
	_mList = NULL;
	_m_listOffset = -1;
	if (_mArea < 0)
		dlog("NewAreaUser(%d) returns %x", area, _mArea);
	area_info info;
	if (get_area_info(area, &info) == B_OK)
	{
		_mData = ((char *)info.address)+offset;
	}
	else
	{
		_mData = NULL;
	}
	_mOffset = offset;
	_mSize = size;
	_mBufferID = -1;
	_mFlags = flags;
	_mRefCount = 1;
	if (_mData != NULL)
	{
		if (((_BMediaRosterP *)BMediaRoster::Roster())->RegisterBuffer(this) != B_OK)
		{
			_mData = NULL;
			dlog("RegisterBuffer failed");
		}
//		printf("BBuffer created, id=%d, data=%08x, offset=%08x\n",ID(),Data(),_mOffset);
	}
}


BBuffer::BBuffer(
	media_header * small_data)
{
	assert(this == reinterpret_cast<BBuffer *>(small_data));
	_mArea = -1;
	_mList = NULL;
	_m_listOffset = -1;
	_mData = _mHeader.user_data;
	_mOffset = 0;
	_mSize = sizeof(_mHeader.size_used);
	_mBufferID = -1;
	_mFlags = B_SMALL_BUFFER;
	_mRefCount = 1;
	_mOrigListArea = B_BAD_VALUE;
	_mListArea = B_BAD_VALUE;
}


BBuffer::~BBuffer()
{
//	ASSERT(_mRefCount == 0);
	if (_mFlags & B_SMALL_BUFFER) {
		return;	//	do nothing; placement new
	}
//	((_BMediaRosterP *)BMediaRoster::Roster())->UnregisterBuffer(this);
	((_BMediaRosterP *)BMediaRoster::Roster())->RemoveAreaUser(_mOrigArea);
	SetOwnerArea(B_BAD_VALUE);
}


void *
BBuffer::Data()	/* returns NULL if buffer not correctly initialized */
{
	return ((char *)_mData)+_mHeader.data_offset;
}


size_t
BBuffer::Size()
{
	return _mSize-_mHeader.data_offset;
}


size_t
BBuffer::SizeAvailable()
{
	return _mSize-_mHeader.data_offset;
}


size_t
BBuffer::SizeUsed()
{
	return _mHeader.size_used;
}


void
BBuffer::SetSizeUsed(
	size_t size_used)
{
	_mHeader.size_used = size_used;
}

void
BBuffer::SetOwnerArea(area_id owner)
{
//fprintf(stderr, "BBuffer::SetOwnerArea(%d) this = 0x%x\n", owner, this);
	assert(!(_mFlags & B_SMALL_BUFFER));

	area_info ai;
	if (owner == _mOrigListArea) return;

	if (_mListArea != B_BAD_VALUE)
		((_BMediaRosterP *)BMediaRoster::Roster())->RemoveAreaUser(_mOrigListArea);

	if (owner < 0) goto clear;
	_mOrigListArea = owner;
	_mListArea = ((_BMediaRosterP *)BMediaRoster::Roster())->NewAreaUser(_mOrigListArea);
	if (_mListArea < 0) goto clear;

	get_area_info(_mListArea,&ai);
	_mList = (_shared_buffer_list*)ai.address;
	for (int ix=0; ix<_mList->bufferCount; ix++) {
		if (_mList->buffers[ix].id == _mBufferID) {
			_m_listOffset = ix;
			break;
		}
	}
	assert(_m_listOffset != -1);

	return;

	clear:
	_mOrigListArea = B_BAD_VALUE;
	_mListArea = B_BAD_VALUE;
	_mList = NULL;
	_m_listOffset = -1;
};

void
BBuffer::SetHeader(media_header *header)
{
	SetOwnerArea(header->owner);
	_mHeader = *header;
};

void
BBuffer::Recycle()
{
	if (_mFlags & B_SMALL_BUFFER)
		return;

	if (!_mList) {
		dlog("BBuffer::Recycle() %08x can't recycle a buffer with no owner!",this);
		return;
	}

	_mList->ReleaseBuffer(_m_listOffset, _mBufferID);
}


buffer_clone_info
BBuffer::CloneInfo() const
{
	buffer_clone_info ret;
	ret.buffer = _mBufferID;
	ret.area = _mArea;
	ret.offset = _mOffset;
	ret.size = _mSize;
	ret.flags = _mFlags;
	return ret;
}


media_buffer_id
BBuffer::ID()
{
	return _mBufferID;
}


media_type
BBuffer::Type()
{
	return _mHeader.type;
}


media_header * 
BBuffer::Header()
{
	return &_mHeader;
}


media_audio_header *
BBuffer::AudioHeader()
{
	return &_mHeader.u.raw_audio;
}


media_video_header *
BBuffer::VideoHeader()
{
	return &_mHeader.u.raw_video;
}


BBuffer::BBuffer(
	const buffer_clone_info & info)
{
	memset(&_mHeader, 0, sizeof(_mHeader));
	_mOrigListArea = B_BAD_VALUE;
	_mListArea = B_BAD_VALUE;
	_mList = NULL;
	_m_listOffset = -1;
	_mArea = ((_BMediaRosterP *)BMediaRoster::Roster())->NewAreaUser(info.area);
	if (_mArea < 0) {
		ERRORS(stderr, "NewAreaUser(%d) returns %x\n", info.area, _mArea);
	}
	_mOrigArea = info.area;
	area_info ainfo;
	status_t err;
	if ((err = get_area_info(_mArea, &ainfo)) == B_OK) {
		_mData = ((char *)ainfo.address)+info.offset;
	}
	else {
		_mData = NULL;
		ERRORS(stderr, "get_area_info(%d) returns %x\n", _mArea, err);
	}
	_mOffset = info.offset;
	_mSize = info.size;
	_mBufferID = info.buffer;
	_mFlags = info.flags;
	_mRefCount = 1;
	if ((_mData != NULL) && (_mBufferID < 1))
	{
		if (B_OK != ((_BMediaRosterP *)BMediaRoster::Roster())->RegisterBuffer(this))
		{
			_mData = NULL;
			ERRORS(stderr, "RegisterBuffer failed\n");
		}
	}
}


uint32
BBuffer::Flags()
{
	return _mFlags;
}


void
BBuffer::SetGroupOwnerPort(
	port_id owner)
{
	if (_mList && (_mList->ownerPort == -1)) _mList->ownerPort = owner;
}

void
BBuffer::SetCurrentOwner(
	port_id owner)
{
	if (_mList) {
		assert(_m_listOffset >= 0);
		assert(_mList->buffers[_m_listOffset].id == _mBufferID);
		_mList->buffers[_m_listOffset].owner = owner;
	}
}


//BBuffer *
//BBuffer::BufferFor(
//	const media_header * header)
//{
//	BBuffer * ret = ((_BMediaRosterP *)BMediaRoster::_sDefault)->FindBuffer(header->buffer);
//	if (ret != NULL) {
//		memcpy(ret->Header(), header, sizeof(media_header));
//	}
//	return ret;
//}


BSmallBuffer::BSmallBuffer() :
	BBuffer((media_header *)this)
{
}

size_t
BSmallBuffer::SmallBufferSizeLimit()
{
#if __MWERKS__
	media_header metrowerks_rots;
	return sizeof(metrowerks_rots.user_data);
#else
	return sizeof(media_header::user_data);
#endif
}
