#ifndef _MEDIA2_BUFFERSOURCE_PRIVATE_
#define _MEDIA2_BUFFERSOURCE_PRIVATE_

#include <OS.h>
#include <support2/Debug.h>

namespace B {
namespace Private {

// this is the general shared descriptor for a buffer source, which
// represents a set of data buffers.  each buffer source is located
// in a unique area (its buffer data may reside in the same area, or
// in any number of separate areas) so buffer sources can be uniquely
// referenced by area_id.
//
// buffer IDs are now explicitly defined as buffer entry indices
// (a buffer with ID < 0 is unshared.)

struct media_buffer_entry;

struct media_buffer_source
{
	size_t			capacity;			// max # of buffer entries (constant)
	size_t			count;				// current # of buffer entries
										// (may increase over time, but never decrease)
	size_t			entry_size;			// size of buffer entry (>= sizeof(media_buffer_entry))
	size_t			entry_offset;		// offset of first buffer entry relative
										// to start of media_buffer_source
	enum type_t
	{
		BUFFER_GROUP,
		BUFFER_RING
	};
	type_t			source_type;		// how is this source implemented?
	
	inline	media_buffer_entry & EntryAt(int32 index)
	{
		return *(media_buffer_entry*)
			((int8*)this + (entry_offset + index * entry_size));
	}

	// implementation details may follow, with entries starting
	// at entry_offset
	
};

struct media_buffer_entry
{
	area_id			area;
	size_t			offset;
	size_t			size;

	enum buffer_state
	{
		FREE,
		ACQUIRED
	};
	buffer_state	state;
	int32			refs;

	// implementations may add buffer-specific state info here
};

} } // B::Private
#endif //_MEDIA2_BUFFERSOURCE_PRIVATE_
