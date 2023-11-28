//	realtime_allocator.h
//	Copyright 1998, Be Incorporated
//
//	This allocator was written for the HP/SGI STL version 3.11 to
//	provide memory management for containers within a pre-allocated
//	locked area of memory. We do this so that we don't have to use
//	malloc() or free(), which use a shared lock, or enter VM, which
//	would cause unpredictable delays.
//
//	Thus, this allocator should be used in real-time threads and
//	other cases where predictability is an important factor, and where
//	you can determine an upper bound on the number of allocations that
//	will be required.
//
//	The allocator has been tested with <set> and <map>.
//
//	Note that this allocator is no thread-safe on a per-allocator object,
//	but since each container has an allocator, and each container is not
//	thread-safe on a per-container object, this is not a real problem.
//
//	Typical usage:
//
//	// use a typedef to reduce the typing strain imposed
//	typedef map<int, BString, less<int>, realtime_allocator<BString, realtime_blocks_256>> map_type;
//	map_type m;
//	m[2] = "hello, world!";
//	map_type::iterator i(m.begin());
//	while (i != m.end()) { cout << (*i).second.String(); i++; };
//

#error THIS FILE IS OBSOLETE
#error USE rtm_allocator.h instead

#if !defined(REALTIME_ALLOCATOR_H)
#define REALTIME_ALLOCATOR_H

#if !defined(__GNUC__) || !defined(__INTEL__)
#error This allocator only works with HP/SGI STL 3.11 using EGCS on Intel
#endif

#include <sys/types.h>
#include <typeinfo>
#include <OS.h>
#include <stdexcept>
#include <stdio.h>	//	for sprintf

//	how many times to run the test
#define NITER 10


class realtime_blocks_64 {
public:
	enum { NUM_BLOCKS = 64 };
};
class realtime_blocks_256 {
public:
	enum { NUM_BLOCKS = 256 };
};
class realtime_blocks_1024 {
public:
	enum { NUM_BLOCKS = 1024 };
};

template<class Type, class BlockCount = realtime_blocks_256>
class realtime_allocator {
public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	template <class T> class types {
	public:
		typedef T value_type;
		typedef T * pointer;
		typedef const T * const_pointer;
		typedef T & reference;
		typedef const T & const_reference;
	};
	template <class OtherType> struct rebind {
		typedef realtime_allocator<OtherType> other;
	};

	enum { NUM_BLOCKS = BlockCount::NUM_BLOCKS };

	inline size_t round_size(size_t size) {
		return (size+31)&~31;
	}
	realtime_allocator() {
		m_area = NULL;
	}
	~realtime_allocator() {
		delete m_area;
	}
	template<class Other>
	realtime_allocator(const realtime_allocator<Other> & other) {
		m_area = NULL;
		//	We only allow copying of unused allocators, as that is the
		//	behaviour exhibited under testing <map> and <set>. Anything
		//	more would bring re-entrancy issues in the picture.
		//	assert(other.m_area == NULL);
	}

//	This flavor was specified by the standard
//	Type * allocate(size_t size, const void * hint) {
//		return malloc(size*sizeof(Type));
//	}
	Type * allocate(size_t size) {
		if (!m_area) {
			init_object();
		}
		size_t siz = size*sizeof(Type);
		assert(siz <= m_size);
		if (m_free == NULL) {
			throw bad_alloc();
		}
		Type * ret = reinterpret_cast<Type *>(m_free);
		m_free = *((char **)m_free);
		return ret;
	}
//	This flavor was specified by the standard
//	void deallocate(void * p) {
//		free(p);
//	}
	void deallocate(void * p, size_t size) {
		assert(size*sizeof(Type) <= m_size);
		assert((p >= m_start) && (p < (m_start+(m_size*m_blocks))) && (((((char *)p)-m_start) % m_size) == 0));
		*(char **)p = m_free;
		m_free = (char *)p;
	}
	inline size_type max_size() const {
		return m_blocks;
	}

private:

	realtime_allocator<Type> & operator=(const realtime_allocator<Type> & other);	// unimplemented

	class area_ref;

	area_ref * m_area;
	size_t m_size;
	char * m_start;
	char * m_free;
	int m_blocks;

	void init_object() {
		m_size = round_size(sizeof(Type));
		size_t s = NUM_BLOCKS*m_size;
		m_blocks = NUM_BLOCKS;
		if (s > 65536) {
			m_blocks = NUM_BLOCKS/4;
		}
		m_start = NULL;
		size_t pad_size = (m_size*m_blocks+B_PAGE_SIZE-1)&~(B_PAGE_SIZE-1);
		char name[32];
		sprintf(name, "RTA:%.27s", typeid(Type).name());
		area_id area = create_area(name, (void **)&m_start,
			B_ANY_ADDRESS, pad_size, B_FULL_LOCK, B_READ_AREA|B_WRITE_AREA);
		if (area < 0) {
			throw runtime_error("Could not allocate area in allocator::init_object()");
		}
		m_free = m_start;
		// create linked free list
		char ** ptr = (char **)m_free;
		char * next = m_free+m_size;
		for (int ix=0; ix<m_blocks-1; ix++) {
			*ptr = next;
			next += m_size;
			ptr += m_size/sizeof(char *);
		}
		*ptr = NULL;
		m_area = new area_ref(area);
	}

	//	Class area_ref is used to actually reference the area we allocate
	//	out of, and reference count it.
	class area_ref {
	public:
		area_ref(area_id area, int32 ref_count = 1) {
			m_area = area;
			m_ref_count = ref_count;
		}
		~area_ref() {
			if (atomic_add(&m_ref_count, -1) == 1) {
				delete_area(m_area);
			}
		}
	private:
	
		area_ref(const area_ref & other);
		const area_ref & operator=(const area_ref & other);

		area_id m_area;
		int32 m_ref_count;
	};

};

//	Specialize for void because there are no void references.
//	I wanted template<class Type> class realtime_allocator<Type>::types<void>
//	but that generated an internal compiler error in EGCS on 1998-10-28.
class realtime_allocator<void>::types<void> {
public:
	typedef void * pointer;
	typedef const void * const_pointer;
	typedef void value_type;
};



//	Use operator new(size, allocator) and operator delete(size, allocator)
//	for malloc()-style allocation using the allocator. Note that the allocator
//	is blocking; no block can be bigger than the (rounded-up) size of the
//	Type argument.
template<class Type, class BlockSize>
inline void * operator new(size_t size, realtime_allocator<Type, BlockSize> & a)
{
	return a.allocate(size);
}

template<class Type, class BlockSize>
inline void operator delete(void * ptr, realtime_allocator<Type, BlockSize> & a)
{
	if (!ptr) return;
	a.deallocate(ptr);
}


#endif // REALTIME_ALLOCATOR_H

