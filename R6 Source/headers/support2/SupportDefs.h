/******************************************************************************
/
/	File:			SupportDefs.h
/
/	Description:	Common type definitions.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef _SUPPORT2_DEFS_H
#define _SUPPORT2_DEFS_H

// This header also defines everything in support/SupportDefs.h.
#define _SUPPORT_DEFS_H

/*-------------------------------------------------------------*/

#include <be_prim.h>

/*-------------------------------------------------------------*/

#ifdef __cplusplus

#define LIBBE2 1

namespace B {
namespace Support2 {

class BAtom;
template <class TYPE> class atom_ptr;
template <class TYPE> class atom_ref;
template <class TYPE> class BVector;
template <class TYPE> class BOrderedVector;
template <class KEY, class VALUE> class BKeyedVector;
template <class KEY, class VALUE> class BHashTable;

template <class CHILD_TYPE> class IBinderVector;
template <class CHILD_TYPE> class LBinderVector;
template <class CHILD_TYPE> class BBinderVector;

typedef float coord;

class BAtom;
class IBinder;
class IInterface;

class BValue;
class BValueMap;
class BBitfield;
class BBlockCache;
class BBinder;
class BLooper;
class BLocker;
class BDataIO;
class BStreamIO;
class BHandler;
class BMessage;
class BString;

extern const char *B_EMPTY_STRING;

} } // namespace B::Support2

#endif

#include <support2/Errors.h>

/*-------------------------------------------------------------*/
/*----- Locking status result ---------------------------------*/

struct lock_status_t {
	void (*unlock_func)(void* data);	// unlock function, NULL if lock failed
	union {
		status_t error;					// error if "unlock_func" is NULL
		void* data;						// locked object if "unlock_func" is non-NULL
	} value;
	
#ifdef __cplusplus
	inline lock_status_t(void (*f)(void*), void* d)	{ unlock_func = f; value.data = d; }
	inline lock_status_t(status_t e)			{ unlock_func = NULL; value.error = e; }
	
	inline bool is_locked() const				{ return (unlock_func != NULL); }
	inline status_t status() const				{ return is_locked() ? (status_t) B_OK : value.error; }
	inline void unlock() const					{ if (unlock_func) unlock_func(value.data); }
	
	inline operator status_t() const			{ return status(); }
#endif
};

/*-------------------------------------------------------------*/
/*----- Standard PrintToStream() flags ------------------------*/

enum {
	B_PRINT_STREAM_HEADER		= 0x00000001
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _SUPPORT2_DEFS_H */
