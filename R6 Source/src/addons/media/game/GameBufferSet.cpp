
#include "GameBufferSet.h"
#include "gamedefs.h"

#include <RealtimeAlloc.h>

#include <Debug.h>
#include <cstdlib>
#include <cstring>

using namespace BPrivate;


GameBufferSet::GameBufferSet(int fd, uint16 flags) :
	_fd(fd),
	_flags(flags),
	_cloned(false),
	_count(0),
	_buffers(0)
{
}


GameBufferSet::~GameBufferSet()
{
	ClearBuffers();
}

void *
GameBufferSet::operator new(size_t s)
{
	return rtm_alloc(0, s);
}

void 
GameBufferSet::operator delete(void *p, size_t s)
{
	rtm_free(p);
}


status_t 
GameBufferSet::Allocate(uint32 count, int16 stream, size_t byte_size, bool clone)
{
	if (count <= 0) return B_BAD_VALUE;
	if (_count > 0) return B_NOT_ALLOWED;

	status_t err = B_OK;
	_count = count;
	_cloned = clone;
	_buffers = (game_buffer*)rtm_alloc(0, sizeof(game_buffer)*count);
	memset(_buffers, 0, sizeof(game_buffer)*count);
	
	G<game_open_stream_buffers> gosb;
	if (_flags & GAME_BUFFER_LOOPING)
	{
		gosb.in_request_count = 1;
		gosb.buffers = (game_open_stream_buffer*)alloca(
			sizeof(game_open_stream_buffer));
		memset(gosb.buffers, 0, sizeof(game_open_stream_buffer));
		gosb.buffers[0].stream = stream;
		gosb.buffers[0].flags = _flags;
		gosb.buffers[0].byte_size = byte_size * _count;
		fprintf(stderr, "buf.alloc: flags 0x%hx, byte_size 0x%hx\n",
			gosb.buffers[0].flags,
			gosb.buffers[0].byte_size);
	}
	else
	{
		gosb.in_request_count = _count;
		gosb.buffers = (game_open_stream_buffer*)alloca(
			sizeof(game_open_stream_buffer) * _count);
		memset(gosb.buffers, 0, sizeof(game_open_stream_buffer) * _count);
		for (uint32 n = 0; n < count; n++)
		{
			gosb.buffers[n].stream = stream;
			gosb.buffers[n].flags = _flags;
			gosb.buffers[n].byte_size = byte_size;
		}
	}
			
	err = ioctl(_fd, GAME_OPEN_STREAM_BUFFERS, &gosb);
	if (err < B_OK)
	{
		PRINT((
			"GameBufferSet::Allocate():\n\t"
			"GAME_OPEN_STREAM_BUFFERS:\n\t"
			"%s\n",
			strerror(err)));
		return err;
	}
	
	uint32 chunkOffset = 0;
	for (uint32 n = 0; n < _count; n++)
	{
		const game_open_stream_buffer& b = ((_flags & GAME_BUFFER_LOOPING) ? gosb.buffers[0] : gosb.buffers[n]);
		if (b.buffer == GAME_NO_ID)
		{
			PRINT((
				"GameBufferSet::Allocate():\n\t"
				"GAME_OPEN_STREAM_BUFFER(0x%hx, 0x%hx, 0x%x):\n\t"
				"no buffer ID!\n",
				stream, _flags, byte_size, strerror(err)));
			ClearBuffers();
			return B_ERROR;
		}
		
		_buffers[n].id = b.buffer;
		_buffers[n].area = b.area;
		_buffers[n].offset = b.offset + chunkOffset;
		if (_flags & GAME_BUFFER_LOOPING) chunkOffset += byte_size;
		_buffers[n].size = byte_size;
		_buffers[n].local_area = -1;
		_buffers[n].data = 0;
		
		if (_cloned) CloneBufferAt(n);
	}
	
	return B_OK;
}

bool 
GameBufferSet::HasClones() const
{
	return _cloned;
}

status_t 
GameBufferSet::MakeClones()
{
	if (_cloned || !_count) return B_NOT_ALLOWED;
	for (uint32 n = 0; n < _count; n++) CloneBufferAt(n);
	_cloned = true;
	return B_OK;
}

void 
GameBufferSet::CloneBufferAt(uint32 n)
{
	ASSERT(n < _count);
	
	// clone the area only if it hasn't already been cloned
	for (uint32 prev = 0; prev < n; prev++)
	{
		if (_buffers[prev].area == _buffers[n].area)
		{
			_buffers[n].local_area = _buffers[prev].local_area;
			_buffers[n].data = (int8*)_buffers[prev].data +
				_buffers[n].offset - _buffers[prev].offset;
			break;
		}
	}
	if (_buffers[n].local_area < 0)
	{
		void* base;
		area_id cloned = clone_area(
			"mixbuffer clone",
			&base,
			B_ANY_ADDRESS,
			B_READ_AREA | B_WRITE_AREA,
			_buffers[n].area);
		if (cloned < 0)
		{
			PRINT((
				"GameBufferSet::CloneBufferAt(%ld):\n\t"
				"clone_area(): %s\n",
				n, strerror(cloned)));
			return;
		}
		_buffers[n].local_area = cloned;
		_buffers[n].data = (int8*)base + _buffers[n].offset;
	}
}



status_t 
GameBufferSet::Free()
{
	if (!_count) return B_NOT_ALLOWED;
	ClearBuffers();
	return B_OK;
}

uint32 
GameBufferSet::CountBuffers() const
{
	return _count;
}

const game_buffer *
GameBufferSet::BufferAt(uint32 index) const
{
#if DEBUG
	return (index >= _count) ? 0 : &_buffers[index];
#else
	return &_buffers[index];
#endif
}

void 
GameBufferSet::ClearBuffers()
{
	status_t err;
	if (!_buffers) return;
	
	uint32 bufferCount = ((_flags & GAME_BUFFER_LOOPING) ? 1 : _count);
	for (uint32 n = 0; n < bufferCount; n++)
	{
		if(_buffers[n].id == GAME_NO_ID)
			continue; // no buffer alloc'd

		// ask driver to clean up buffer
		H<game_close_stream_buffer> gcsb;
		gcsb.buffer = _buffers[n].id;
		G<game_close_stream_buffers> gcsbs;
		gcsbs.in_request_count = 1;
		gcsbs.buffers = &gcsb;
		err = ioctl(_fd, GAME_CLOSE_STREAM_BUFFERS, &gcsbs);
		if (err < 0)
		{
			PRINT((
				"GameBufferSet::ClearBuffers():\n\t"
				"ioctl(GAME_CLOSE_STREAM_BUFFERS): %s\n",
				strerror(err)));
		}
		
		if (_cloned)
		{
			// clean up cloned area
			area_id clone = _buffers[n].local_area;
			if (clone)
			{
				for (uint32 forward = n; forward < _count; forward++)
				{
					if (_buffers[forward].local_area == clone)
						_buffers[forward].local_area = 0;
				}
				delete_area(clone);
			}
		}
		else
		{
			ASSERT(!_buffers[n].local_area);
			ASSERT(!_buffers[n].data);
		}
	}
	
	_count = 0;
	rtm_free(_buffers);
	_buffers = 0;
	_cloned = false;
}
