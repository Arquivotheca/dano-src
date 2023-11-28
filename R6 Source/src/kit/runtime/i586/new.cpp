//	Leak checking version of new for gcc
//	(c) 1997, Be Incorporated

#define _BUILDING_NEW_CPP_

#include <Debug.h>
#include <new>
// Older versions of gcc define __THROW.  Newer ones use __GCC_THROW.
// Even newer ones don't use either.  During transition to the new tools,
// be prepared to supply our own macros.  Eventually this can go away.
#ifndef __THROW	
#define __THROW(what) throw (what)
#define __nothing
#endif
#include "LeakChecking.h"

using std::new_handler;
using std::bad_alloc;

extern new_handler __new_handler;

// from cygnus/gcc/cpp/new.cc
#ifndef __EMBEDDED_CXX__
const std::nothrow_t std::nothrow = { };
#endif

new_handler __new_handler;

new_handler
set_new_handler (new_handler handler)
{
  new_handler prev_handler = __new_handler;
  __new_handler = handler;
  return prev_handler;
}

void *operator new (size_t size, const std::nothrow_t&) __THROW(__nothing) __attribute__ ((weak));
void *operator new(size_t size) __THROW (std::bad_alloc) __attribute__ ((weak));
void operator delete(void *p) __THROW(__nothing) __attribute__ ((weak));
void operator delete(void *ptr, const std::nothrow_t&) __THROW (__nothing) __attribute__ ((weak));
void *operator new[] (size_t size) __THROW (std::bad_alloc) __attribute__ ((weak));
void *operator new[] (size_t size, const std::nothrow_t& nothrow) __THROW(__nothing) __attribute__ ((weak));
void *operator new(size_t, void *place) __THROW(__nothing) __attribute__ ((weak));
void *operator new[](size_t, void *place) __THROW(__nothing) __attribute__ ((weak));

// modified from cygnus/gcc/cpp/new1.cc

#if !_SUPPORTS_LEAK_CHECKING
# define unchecked_malloc 	malloc
# define unchecked_free 	free
#endif

void *
operator new (size_t size, const std::nothrow_t&) __THROW(__nothing)
{

	for (;;) {
		void *p = unchecked_malloc(size);
		
		if (p) {
#if _SUPPORTS_LEAK_CHECKING
			if (NewLeakChecking())
				record_new(p, size);
#endif
			return p;
		}
		new_handler new_handler_func = __new_handler;
		if (!new_handler_func)
			return 0;

#ifdef __EMBEDDED_CXX__
		new_handler_func();
#else
		try {
			new_handler_func();
		}
		catch (bad_alloc) {
			return 0;
		}
#endif

	}

	ASSERT(!"should not be here");
	return 0;
}

void *
operator new(size_t size) __THROW (std::bad_alloc)
{
	for (;;) {
		void *p = unchecked_malloc(size);
		if (p) {
#if _SUPPORTS_LEAK_CHECKING
			if (NewLeakChecking())
				record_new(p, size);
#endif
			return p;
		}
		new_handler new_handler_func = __new_handler;

		if (new_handler_func) 
			new_handler_func();
		else
#ifndef __EMBEDDED_CXX__
			throw bad_alloc();
#else
			return 0;
#endif
	}

	ASSERT(!"should not be here");
	return 0;
}

void
operator delete(void *p) __THROW(__nothing)
{
#if _SUPPORTS_LEAK_CHECKING
	if (p && NewLeakChecking())
		record_delete(p);
#endif

	unchecked_free(p);
}

void
operator delete(void *ptr, const std::nothrow_t&) __THROW (__nothing)
{
	::operator delete(ptr);
}

// modified from cygnus/gcc/cpp/new2.cc


// note:
// The following vector new calls are copies of simple new / nothrow-new.
// this is so that the stack depth of the new glue is the same for both the
// vector new and simple new calls. We could use an inline common function
// instead, this way we don't have problems with inlining being off at -O0
// and changing the call depth.
void *
operator new[] (size_t size) __THROW (std::bad_alloc)
{
	for (;;) {
		void *p = unchecked_malloc(size);
		if (p) {
#if _SUPPORTS_LEAK_CHECKING
			if (NewLeakChecking())
				record_new(p, size);
#endif
			return p;
		}
		new_handler new_handler_func = __new_handler;

		if (new_handler_func) 
			new_handler_func();
		else
#ifndef __EMBEDDED_CXX__
			throw bad_alloc();
#else
			return 0;
#endif
	}

	ASSERT(!"should not be here");
	return 0;
}

void *
operator new[] (size_t size, const std::nothrow_t& nothrow) __THROW(__nothing)
{
	for (;;) {
		void *p = unchecked_malloc(size);
		
		if (p) {
#if _SUPPORTS_LEAK_CHECKING
			if (NewLeakChecking())
				record_new(p, size);
#endif
			return p;
		}
		new_handler new_handler_func = __new_handler;
		if (!new_handler_func)
			return 0;

#ifdef __EMBEDDED_CXX__
		new_handler_func();
#else
		try {
			new_handler_func();
		}
		catch (bad_alloc) {
			return 0;
		}
#endif

	}

	ASSERT(!"should not be here");
	return 0;
}

void
operator delete[] (void *ptr) __THROW (__nothing)
{
	::operator delete(ptr);
}

void
operator delete[] (void *ptr, const std::nothrow_t&) __THROW (__nothing)
{
	::operator delete(ptr);
}

// make sure the call gets emitted out of line in libroot for
// compatibility with old binaries - this is done by including
// the same two calls, only without the inline directive
//
// keep it inline everywhere else
// These conflict with gcc 3 headers.  FIXME if we still need them.

#if __GNUC__ < 3
void *operator new(size_t, void *place) __THROW(__nothing) {
  return place;
}

void *operator new[](size_t, void *place) __THROW(__nothing) {
  return place;
}
#endif
