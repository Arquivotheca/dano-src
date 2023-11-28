// BPrivate::GameBufferSet
// - creates and frees a group of buffers for data exchange with
//   game_audio drivers.

#if !defined(__GAME_BUFFER_SET_H__)
#define __GAME_BUFFER_SET_H__

#include "game_audio.h"
namespace BPrivate {

struct game_buffer
{
	int16			id;
	area_id			area;
	size_t			offset;
	size_t			size;
	// the remaining members are only valid if clones were allocated
	area_id			local_area;
	int8*			data;
};

class GameBufferSet
{
public:
	// flags: currently may be 0 or GAME_BUFFER_LOOPING
	GameBufferSet(int fd, uint16 flags);
	~GameBufferSet();

	void* operator new(size_t s);
	void operator delete(void* p, size_t s);

	status_t Allocate(
		uint32 count,
		int16 stream,
		size_t byte_size,
		bool clone =false);
	bool HasClones() const;
	status_t MakeClones();
	status_t Free();
	
	uint32 CountBuffers() const;
	const game_buffer* BufferAt(uint32 index) const;

private:
	const int		_fd;
	const uint16	_flags;
	bool			_cloned;
	// number of buffers (or chunks, if _looping is set)
	uint32			_count;
	game_buffer*	_buffers;
	
	void ClearBuffers();
	void CloneBufferAt(uint32 index);
};

}; // BPrivate
#endif //__GAME_BUFFER_SET_H__
