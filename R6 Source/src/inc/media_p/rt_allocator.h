#ifndef _RT_ALLOCATOR_H
#define _RT_ALLOCATOR_H

#include <OS.h>
#include <RealtimeAlloc.h>
#include <stdio.h>


namespace BPrivate {

template <class Element>
class rt_allocator {
public:

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef Element value_type;
	typedef Element * pointer;
	typedef const Element * const_pointer;
	typedef Element & reference;
	typedef const Element & const_reference;

	template <class OtherType> struct rebind {
		typedef rt_allocator<OtherType> other;
	};

	rt_allocator()
		{
		}

	template <class OtherAllocator>
	rt_allocator(
			const OtherAllocator & /*other*/)
		{
		}

	static Element *allocate(size_t count)
		{
			void * ret = rtm_alloc(NULL, count * sizeof(Element));
			if (ret == NULL) {
				fprintf(stderr, "rt_allocator: alloc(%ld) fails\n", sizeof(Element)*count);
			}
			return reinterpret_cast<Element *>(ret);
		}

	static void deallocate(Element *ptr, size_t /*count*/)
		{
			(void)rtm_free(reinterpret_cast<void *>(ptr));
		}


	static void construct(pointer p, const Element & t)
		{
			new ((void *)p) Element(t);
		}
	static void destroy(pointer p)
		{
			p -> ~Element ();
		}
};

}
using namespace BPrivate;


#endif
