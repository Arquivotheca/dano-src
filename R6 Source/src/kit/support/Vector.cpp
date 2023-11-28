#include <support/Vector.h>

#include <kernel/OS.h>
#include <support/Debug.h>

#include <stdlib.h>

namespace B {
namespace Support {

inline const uint8* BUntypedVector::data() const
{
	return (m_avail <= m_localSpace) ? m_data.local : m_data.heap;
}

inline uint8* BUntypedVector::edit_data()
{
	return (m_avail <= m_localSpace) ? m_data.local : m_data.heap;
}

BUntypedVector::BUntypedVector(size_t element_size)
	:	m_elementSize(element_size),
		m_localSpace(sizeof(m_data)/m_elementSize),
		m_avail(sizeof(m_data)/m_elementSize),
		m_size(0)
{
}

BUntypedVector::BUntypedVector(const BUntypedVector& o)
	:	m_elementSize(o.m_elementSize),
		m_localSpace(sizeof(m_data)/m_elementSize),
		m_avail(sizeof(m_data)/m_elementSize),
		m_size(0)
{
	if (o.m_size > 0) {
		uint8* dest = grow(o.m_size);
		const uint8* src = o.data();
		if (dest && src) {
			o.PerformCopy(dest, src, o.m_size);
			m_size = o.m_size;
		}
	}
}

BUntypedVector::~BUntypedVector()
{
	if (m_size != 0) debugger("BUntypedVector: subclass must call MakeEmpty() in destructor");
}

BUntypedVector&	BUntypedVector::operator=(const BUntypedVector& o)
{
	if (m_elementSize == o.m_elementSize) {
		if (o.m_size > 0) {
			if (m_size > 0) {
				uint8* cur = edit_data();
				PerformDestroy(cur, m_size);
				m_size = 0;
			}
			uint8* dest = grow(o.m_size);
			const uint8* src = o.data();
			if (dest && src) {
				PerformCopy(dest, src, o.m_size);
				m_size = o.m_size;
			}
		} else {
			MakeEmpty();
		}
	
	} else {
		debugger("BUntypedVector element sizes do not match");
	}
	
	return *this;
}

size_t BUntypedVector::ElementSize() const
{
	return m_elementSize;
}

size_t BUntypedVector::CountElements() const
{
	return m_size;
}

size_t BUntypedVector::CountAvail() const
{
	return m_avail;
}

const void* BUntypedVector::ElementAt(size_t index) const
{
	const uint8* d = data();
	if (index < m_size && d != NULL) return d+(index*m_elementSize);
	return NULL;
}

ssize_t BUntypedVector::AddElement(const void* newElement)
{
	uint8* d = grow(1);
	if (d) {
		if (newElement)
			PerformCopy(d+(m_size*m_elementSize), newElement, 1);
		else
			PerformConstruct(d+(m_size*m_elementSize), 1);
		return m_size++;
	}
	return B_NO_MEMORY;
}

ssize_t BUntypedVector::InsertElement(const void* newElement, size_t index)
{
	if (index > m_size) index = m_size;
	uint8* d = grow(1, 3, index);
	if (d) {
		m_size++;
		if (newElement)
			PerformCopy(d+(index*m_elementSize), newElement, 1);
		else
			PerformConstruct(d+(index*m_elementSize), 1);
		return index;
	}
	return B_NO_MEMORY;
}

void* BUntypedVector::EditElementAt(size_t index)
{
	uint8* d = edit_data();
	if (index < m_size && d != NULL) return d+(index*m_elementSize);
	return NULL;
}

ssize_t BUntypedVector::AddUntypedVector(const BUntypedVector& o)
{
	return InsertUntypedVector(o, m_size);
}

ssize_t BUntypedVector::InsertUntypedVector(const BUntypedVector& o, size_t index)
{
	if (m_elementSize == o.m_elementSize) {
		const uint8* src = o.data();
		if (o.m_size > 0 && src != NULL) {
			if (index > m_size) index = m_size;
			uint8* d = grow(o.m_size, 3, index);
			if (d) {
				PerformCopy(d+(index*m_elementSize), src, o.m_size);
				return m_size+=o.m_size;
			}
			return B_NO_MEMORY;
		}
		return m_size;
	}
	
	debugger("BUntypedVector element sizes do not match");
	return B_BAD_TYPE;
}

void BUntypedVector::RemoveElements(size_t index, size_t count)
{
	if (count > 0) {
		if (index >= m_size) index = m_size-1;
		if ((index+count) > m_size) count = m_size-index;
		if (count > 0) {
			PerformDestroy(edit_data()+(index*m_elementSize), count);
			shrink(count, 4, index);
			m_size -= count;
		}
	}
}

void BUntypedVector::MakeEmpty()
{
	uint8* d = edit_data();
	if (d) {
		if (m_size > 0)
			PerformDestroy(d, m_size);
		if (d != m_data.local)
			free(d);
	}
	m_avail = m_localSpace;
	m_size = 0;
}

void BUntypedVector::Reserve(size_t total_space)
{
	if (total_space < m_size) total_space = m_size;
	PRINT(("Reserve: requested %ld, have %ld, using %ld, grow=%ld\n",
			total_space, m_avail, m_size, total_space-m_size));
	grow(total_space-m_size, 2);
}

void BUntypedVector::ReserveExtra(size_t extra_space)
{
	grow(extra_space, 2);
}

void BUntypedVector::Swap(BUntypedVector& o)
{
	uint8 buffer[sizeof(BUntypedVector)];
	memcpy(buffer, this, sizeof(BUntypedVector));
	memcpy(this, &o, sizeof(BUntypedVector));
	memcpy(&o, buffer, sizeof(BUntypedVector));
}

uint8* BUntypedVector::grow(size_t amount, size_t factor, size_t pos)
{
	const size_t total_needed = m_size+amount;
	
	if (total_needed > m_avail) {
		// Need to grow the available space...
		if (total_needed > m_localSpace) {
			// Figure out how much space to reserve, and allocate new heap
			// space for it.
			const size_t can_use = ((total_needed+1)*factor)/2;
			uint8* alloc = static_cast<uint8*>(malloc(can_use*m_elementSize));
			if (alloc == NULL) return NULL;
			if (m_size > 0) {
				// If there are existing elements, move them into the new space.
				uint8* src = edit_data();
				if (src) {
					if (pos >= m_size) {
						PRINT(("Grow heap: copying %ld entries\n", m_size));
						PerformMoveBefore(alloc, src, m_size);
					} else {
						PRINT(("Grow heap: copying %ld entries (%ld at %ld, %ld from %ld to %ld)\n",
									m_size,
									pos, 0L,
									(m_size-pos), pos, pos+amount));
						if (pos > 0)
							PerformMoveBefore(alloc, src, pos);
						PerformMoveBefore(	alloc+((pos+amount)*m_elementSize),
											src+(pos*m_elementSize),
											(m_size-pos));
					}
				}
			}
			// Free old memory if it is on the heap.
			if (m_avail > m_localSpace) free(m_data.heap);
			m_data.heap = alloc;
			m_avail = can_use;
			return alloc;
		} else {
			debugger("BUntypedVector::grow -- total_needed < m_localSpace, but total_needed > m_avail!");
		}
	}
	
	uint8* d = edit_data();
	if (pos < m_size) {
		// If no memory changes have occurred, but we are growing inside
		// the vector, then just move those elements in-place.
		if (d) {
			PRINT(("Grow: moving %ld entries (from %ld to %ld)\n",
						m_size-pos, pos, pos+amount));
			PerformMoveAfter(	d+((pos+amount)*m_elementSize),
								d+(pos*m_elementSize),
								(m_size-pos));
		}
	}
	
	return d;
}

uint8* BUntypedVector::shrink(size_t amount, size_t factor, size_t pos)
{
	if (amount > m_size) amount = m_size;
	const size_t total_needed = m_size - amount;
	
	if (total_needed <= m_localSpace) {
		// Needed size now fits in local data area...
		if (m_avail > m_localSpace) {
			// We are currently using heap storage; copy into local data.
			uint8* src = edit_data();
			if (total_needed > 0) {
				if (pos >= total_needed) {
					PRINT(("Shrink to local: copying %ld entries (was %ld)\n",
								total_needed, m_size));
					PerformMoveBefore(m_data.local, src, total_needed);
				} else {
					PRINT(("Shrink to local: copying %ld entries (%ld at %ld, %ld from %ld to %ld)\n",
								total_needed,
								pos, 0L,
								(total_needed-pos), pos+amount, pos));
					if (pos > 0)
						PerformMoveBefore(m_data.local, src, pos);
					PerformMoveBefore(	m_data.local+(pos*m_elementSize),
										src+((pos+amount)*m_elementSize),
										(total_needed-pos));
				}
			}
			free(src);
			m_avail = m_localSpace;
			return m_data.local;
		}
	} else if ((total_needed*factor) < m_avail) {
		// Needed size still must to be on heap, but we can reduce our
		// current heap usage...
		factor /= 2;
		if (factor < 1) factor = 1;
		const size_t will_use = total_needed*factor;
		if (will_use > m_localSpace) {
			uint8* alloc = static_cast<uint8*>(malloc(will_use*m_elementSize));
			// We can fail gracefully if this allocation doesn't succeed.
			if (alloc != NULL) {
				uint8* src = edit_data();
				if (src) {
					if (pos >= total_needed) {
						PRINT(("Shrink heap: copying %ld entries (was %ld)\n",
									total_needed, m_size));
						PerformMoveBefore(alloc, src, total_needed);
					} else {
						PRINT(("Shrink heap: copying %ld entries (%ld at %ld, %ld from %ld to %ld)\n",
									total_needed,
									pos, 0L,
									(total_needed-pos), pos+amount, pos));
						if (pos > 0)
							PerformMoveBefore(alloc, src, pos);
						PerformMoveBefore(	alloc+(pos*m_elementSize),
											src+((pos+amount)*m_elementSize),
											(total_needed-pos));
					}
				}
				free(m_data.heap);
				m_data.heap = alloc;
				m_avail = will_use;
				return alloc;
			}
		} else {
			debugger("BUntypedVector::shrink -- total_needed < m_avail*4, but m_avail < m_localSpace!");
		}
	}
	
	uint8* d = edit_data();
	if (pos < total_needed) {
		// If no memory changes have occurred, but we are shrinking inside
		// the vector, then just move those elements in-place.
		if (d) {
			PRINT(("Shrink: moving %ld entries (from %ld to %ld)\n",
						total_needed-pos, pos+amount, pos));
			PerformMoveBefore(	d+(pos*m_elementSize),
								d+((pos+amount)*m_elementSize),
								(total_needed-pos));
		}
	}
	
	return d;
}

void BMoveBefore(BUntypedVector* to, BUntypedVector* from, size_t count)
{
	memcpy(to, from, sizeof(BUntypedVector)*count);
}

void BMoveAfter(BUntypedVector* to, BUntypedVector* from, size_t count)
{
	memmove(to, from, sizeof(BUntypedVector)*count);
}

status_t BUntypedVector::_ReservedUntypedVector1() { return B_UNSUPPORTED; }
status_t BUntypedVector::_ReservedUntypedVector2() { return B_UNSUPPORTED; }
status_t BUntypedVector::_ReservedUntypedVector3() { return B_UNSUPPORTED; }
status_t BUntypedVector::_ReservedUntypedVector4() { return B_UNSUPPORTED; }
status_t BUntypedVector::_ReservedUntypedVector5() { return B_UNSUPPORTED; }
status_t BUntypedVector::_ReservedUntypedVector6() { return B_UNSUPPORTED; }
status_t BUntypedVector::_ReservedUntypedVector7() { return B_UNSUPPORTED; }
status_t BUntypedVector::_ReservedUntypedVector8() { return B_UNSUPPORTED; }
status_t BUntypedVector::_ReservedUntypedVector9() { return B_UNSUPPORTED; }
status_t BUntypedVector::_ReservedUntypedVector10() { return B_UNSUPPORTED; }

} }	// namespace B::Support
