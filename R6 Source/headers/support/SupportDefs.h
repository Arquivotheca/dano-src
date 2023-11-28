/******************************************************************************
/
/	File:			SupportDefs.h
/
/	Description:	Common type definitions.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef _SUPPORT_DEFS_H
#define _SUPPORT_DEFS_H

#include <BeBuild.h>
#include <sys/types.h>
#include <be_prim.h>

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

/*-----------------------------------------------*/
/*----- Empty string ("") -----------------------*/ 
#ifdef __cplusplus
extern const char *B_EMPTY_STRING;
#endif

/*-----------------------------------------------------------*/
/*-------- Obsolete or discouraged API ----------------------*/

/* use 'true' and 'false' */
#ifndef FALSE
#define FALSE		0
#endif
#ifndef TRUE
#define TRUE		1
#endif


/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _SUPPORT_DEFS_H */
