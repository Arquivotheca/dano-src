/***************************************************************************
//
//	File:			support/TypeFuncs.h
//
//	Description:	Templatized functions for various type operations.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT_TYPEFUNCS_H
#define _SUPPORT_TYPEFUNCS_H

#include <support/SupportDefs.h>

#include <memory.h>
#include <new>

namespace B {
namespace Support {

/*--------------------------------------------------------*/
/*----- Basic operations on types ------------------------*/

template<class TYPE>
static inline void BConstruct(TYPE* base, size_t count = 1)
{
	while (--count != (size_t)-1) {
		new(base) TYPE();
		base++;
	}
}

template<class TYPE>
static inline void BDestroy(TYPE* base, size_t count = 1)
{
	while (--count != (size_t)-1) {
		base->~TYPE();
		base++;
	}
}

template<class TYPE>
static inline void BCopy(TYPE* to, const TYPE* from, size_t count = 1)
{
	while (--count != (size_t)-1) {
		new(to) TYPE(*from);
		to++;
		from++;
	}
}

template<class TYPE>
static inline void BMoveBefore(	TYPE* to, TYPE* from, size_t count = 1)
{
	while (--count != (size_t)-1) {
		new(to) TYPE(*from);
		from->~TYPE();
		to++;
		from++;
	}
}

template<class TYPE>
static inline void BMoveAfter(	TYPE* to, TYPE* from, size_t count = 1)
{
	to+=(count-1);
	from+=(count-1);
	while (--count != (size_t)-1) {
		new(to) TYPE(*from);
		from->~TYPE();
		to--;
		from--;
	}
}

template<class TYPE>
static inline void BAssign(	TYPE* to, const TYPE* from, size_t count = 1)
{
	while (--count != (size_t)-1) {
		*to = *from;
		to++;
		from++;
	}
}

template<class TYPE>
static inline void BSwap(TYPE& v1, TYPE& v2)
{
	TYPE tmp(v1); v1 = v2; v2 = tmp;
}

template<class TYPE>
static inline int32 BCompare(const TYPE& v1, const TYPE& v2)
{
	return (v1 < v2) ? -1 : ( (v2 < v1) ? 1 : 0 );
}

template<class TYPE>
static inline bool BLessThan(const TYPE& v1, const TYPE& v2)
{
	return v1 < v2;
}

/*--------------------------------------------------------*/
/*----- Optimizations for all pointer types --------------*/

template<class TYPE>
static inline void BConstruct(TYPE** base, size_t count = 1)
	{ (void)base; (void)count; }
template<class TYPE>
static inline void BDestroy(TYPE** base, size_t count = 1)
	{ (void)base; (void)count; }
template<class TYPE>
static inline void BCopy(TYPE** to, TYPE* const * from, size_t count = 1)
	{ if (count == 1) *to = *from; else memcpy(to, from, sizeof(TYPE*)*count); }
template<class TYPE>
static inline void BMoveBefore(TYPE** to, TYPE** from, size_t count = 1)
	{ if (count == 1) *to = *from; else memcpy(to, from, sizeof(TYPE*)*count); }
template<class TYPE>
static inline void BMoveAfter(TYPE** to, TYPE** from, size_t count = 1)
	{ if (count == 1) *to = *from; else memmove(to, from, sizeof(TYPE*)*count); }
template<class TYPE>
static inline void BAssign(TYPE** to, TYPE* const * from, size_t count = 1)
	{ if (count == 1) *to = *from; else memcpy(to, from, sizeof(TYPE*)*count); }

/*--------------------------------------------------------*/
/*----- Optimizations for basic data types ---------------*/

// Standard optimizations for types that don't contain internal or
// other pointers to themselves.
#define B_IMPLEMENT_SIMPLE_TYPE_FUNCS(TYPE)												\
static inline void BMoveBefore(TYPE* to, TYPE* from, size_t count = 1)					\
	{ memcpy(to, from, (sizeof(TYPE[2])/2)*count); }									\
static inline void BMoveAfter(TYPE* to, TYPE* from, size_t count = 1)					\
	{ memmove(to, from, (sizeof(TYPE[2])/2)*count); }									\

// Extreme optimizations for types whose constructor and destructor
// don't need to be called.
#define B_IMPLEMENT_BASIC_TYPE_FUNCS(TYPE)												\
static inline void BConstruct(TYPE* base, size_t count)									\
	{ (void)base; (void)count; }														\
static inline void BDestroy(TYPE* base, size_t count)									\
	{ (void)base; (void)count; }														\
static inline void BCopy(TYPE* to, const TYPE* from, size_t count = 1)					\
	{ if (count == 1) *to = *from; else memcpy(to, from, (sizeof(TYPE[2])/2)*count); }	\
static inline void BMoveBefore(TYPE* to, TYPE* from, size_t count = 1)					\
	{ if (count == 1) *to = *from; else memcpy(to, from, (sizeof(TYPE[2])/2)*count); }	\
static inline void BMoveAfter(TYPE* to, TYPE* from, size_t count = 1)					\
	{ if (count == 1) *to = *from; else memmove(to, from, (sizeof(TYPE[2])/2)*count); }	\
static inline void BAssign(TYPE* to, const TYPE* from, size_t count = 1)				\
	{ if (count == 1) *to = *from; else memcpy(to, from, (sizeof(TYPE[2])/2)*count); }	\

B_IMPLEMENT_BASIC_TYPE_FUNCS(bool)
B_IMPLEMENT_BASIC_TYPE_FUNCS(int8)
B_IMPLEMENT_BASIC_TYPE_FUNCS(uint8)
B_IMPLEMENT_BASIC_TYPE_FUNCS(int16)
B_IMPLEMENT_BASIC_TYPE_FUNCS(uint16)
B_IMPLEMENT_BASIC_TYPE_FUNCS(int32)
B_IMPLEMENT_BASIC_TYPE_FUNCS(uint32)
B_IMPLEMENT_BASIC_TYPE_FUNCS(int64)
B_IMPLEMENT_BASIC_TYPE_FUNCS(uint64)
B_IMPLEMENT_BASIC_TYPE_FUNCS(float)
B_IMPLEMENT_BASIC_TYPE_FUNCS(double)

} }	// namespace B::Support

#endif
