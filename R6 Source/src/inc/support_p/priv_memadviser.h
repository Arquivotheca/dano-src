/******************************************************************************
/
/	File:			priv_memadviser.h
/
/	Description:	Common type definitions.
/
/	Copyright 2001, Be Incorporated
/
******************************************************************************/

#ifndef __PRIVATE_MEMORY_ADVISER
#define __PRIVATE_MEMORY_ADVISER

#include <support/SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif


//
// These are to be called from memory adviser helpers, to
// ensure proper lock ordering
//
bool madv_lock(void);
void madv_unlock(void);


#ifdef __cplusplus
}
#endif

#endif
