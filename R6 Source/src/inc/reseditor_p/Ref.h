/*****************************************************************************
 *
 *	The BRef class is a simple template that looks and acts like a pointer
 *  to its template type.  In addition, it calls the functions Acquire()
 *  and Release() on objects as they are assigned and removed from it,
 *  so that reference counts are automatically tracked.
 *  The BCRef class is the same thing, but is the equivalent of a
 *  const pointer.
 *
 *  Any object that is being reference counted must define these two
 *  functions:
 *
 *    status_t Acquire() const;
 *    status_t Release() const;
 *
 *  which do what you would expect.
 *
 *****************************************************************************/

#ifndef _REF_H
#define _REF_H

#include <SupportDefs.h>

namespace BPrivate {

class BRefable
{
public:
	BRefable();
	virtual ~BRefable();
	
	int32 Release() const;
	int32 Acquire() const;

	// For evil vile reference counting hacks.
	int32 ReleaseNoFree() const;

	// For more evil uses.
	int32 RefCount() const;
	
protected:
	virtual void DeleteObject();

private:
	int32 fRefCount;
};

template<class T> class BRef
{
public:
	BRef()						{ object = NULL; }
	BRef(T* obj)				{ object = obj; if(object) object->Acquire(); }
	BRef(const BRef<T>& o)		{ object = o.object; if(object) object->Acquire(); }
	~BRef()						{ if(object) object->Release(); object = NULL; }

	BRef<T>& operator = (const BRef<T>& o)
							{ if(o.object) o.object->Acquire();
							  if(object) object->Release();
							  object = o.object;
							  return *this; }
	BRef<T>& operator = (T* obj)
		{ if(obj) obj->Acquire(); if(object) object->Release();
		  object = obj; return *this; }

	operator T* () const	{ return object; }
	
	T* operator -> () const	{ return object; }
	T&	operator * () const	{ return *object; }

	T* Adopt()				{ T* obj = object; object = NULL; return obj; }
	
	// Deal with relational comparisons of two references
#define	RELOP(OP)													\
	friend bool operator OP(const BRef<T>& a, const BRef<T>& b)	\
		{ return a.object OP b.object; }

	RELOP(<) RELOP(>) RELOP(<=) RELOP(>=)
#undef	RELOP
	
private:
	T* object;
};

// For const pointers

template<class T> class BCRef
{
public:
	BCRef()						{ object = NULL; }
	BCRef(const T* obj)			{ object = obj; if(object) object->Acquire(); }
	BCRef(const BCRef<T>& o)	{ object =o.object; if(object) object->Acquire(); }
	~BCRef()					{ if(object) object->Release(); object = NULL;}

	BCRef<T>& operator = (const BCRef<T>& o)
							{ if(o.object) o.object->Acquire();
							  if(object) object->Release();
							  object = o.object;
							  return *this; }
	BCRef<T>& operator = (const T* obj)
		{ if(obj) obj->Acquire(this); if(object) object->Release();
		  object = obj; return *this; }

	operator const T* () const		{ return object; }
	
	const T* operator -> () const	{ return object; }
	const T&	operator * () const	{ return *object; }

	const T* Adopt()				{ const T* obj = object; object = NULL; return obj; }
	
	// Deal with relational comparisons of two references
#define	RELOP(OP)													\
	friend bool operator OP(const BCRef<T>& a, const BCRef<T>& b)	\
		{ return a.object OP b.object; }

	RELOP(<) RELOP(>) RELOP(<=) RELOP(>=)
#undef	RELOP
	
private:
	const T* object;
};

}	// namespace BPrivate

using namespace BPrivate;

#endif
