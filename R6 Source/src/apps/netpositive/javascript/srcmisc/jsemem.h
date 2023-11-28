/* jsemem.h
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

#ifndef _MEMDEBUG_H
#define _MEMDEBUG_H

#include "jsetypes.h"
#include "jselib.h"

#ifdef __cplusplus
extern "C" {
#endif
   
   /* common stuff for production version */
void jseInsufficientMemory(void);

#if defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG)

   void * jseUtilMalloc(uint size, ulong line, const char* file);
   /* If cannot malloc then return NULL. */
   void * jseUtilMustMalloc(uint size, ulong line, const char* file);
   /* If cannot malloc then Fatal() and so
    * will not return. */
   void * jseUtilReMalloc(void *PrevMalloc,uint size, ulong line, const char* file);
      /* return NULL if failed; old memory is not freed */
   void * jseUtilMustReMalloc(void *PrevMalloc,uint size, ulong line,
                              const char* file); /* fatal abort if failure */

#  define jseMalloc(TYPE,SIZE) (TYPE *)jseUtilMalloc(SIZE,__LINE__,__FILE__)
#  define jseReMalloc(TYPE,PTR,SIZE) \
      (TYPE *)jseUtilReMalloc(PTR,SIZE,__LINE__,__FILE__)
#  define jseMustMalloc(TYPE,SIZE) \
      (TYPE *)jseUtilMustMalloc(SIZE,__LINE__,__FILE__)
#  define jseMustReMalloc(TYPE,PTR,SIZE) \
      (TYPE *)jseUtilMustReMalloc(PTR,SIZE,__LINE__,__FILE__)

   void jseUtilMustFree(void *ptr);
#  define jseMustFree(PTR)  jseUtilMustFree(PTR)

#  if !defined(JSE_ENFORCE_JSEMEMCHECK) || (0==JSE_ENFORCE_MEMCHECK)
      /* make sure no one tries to use these.  The preceding #if statement
       * will allow ISDK users to selectively not enforce use of our
       * own memory checking, and yet still allow memory-allocation calls
       * on their own objects.
       */
#     define free(P)         use jseMustFree
#     define malloc(S)       use jseMustMalloc or jseMalloc
#     define realloc(P,S)    use jseMustReMalloc or jseReMalloc
#     define calloc(N,S)     use jseMustMalloc or jseMalloc
#  endif

   void    jseInitializeMallocDebugging();
   void    jseTerminateMallocDebugging();
   ulong   jseMemReport(jsebool Verboseness);
      /* if verboseness then write it all to DebugPrintf() */
   void    jseMemVerbose(jsebool SetVerbose);
   jsebool jseMemValid(void *ptr,uint offset);
   void    jseMemDisplay();

#else

   void * jseUtilMustMalloc(uint size);
      /* If cannot malloc then Fatal() and so will not return. */
   void * jseUtilMustReMalloc(void *PrevMalloc,uint size);
   /* fatal abort if failure */

#  define jseMalloc(TYPE,SIZE)             (TYPE *)jseUtilMalloc(SIZE)
#  define jseReMalloc(TYPE,PTR,SIZE)       (TYPE *)jseUtilReMalloc(PTR,SIZE)
#  define jseMustMalloc(TYPE,SIZE)         (TYPE *)jseUtilMustMalloc(SIZE)
#  define jseMustReMalloc(TYPE,PTR,SIZE)   \
      (TYPE *)jseUtilMustReMalloc(PTR,SIZE)

#  define jseMustFree(P)                   free(P)
#  define jseUtilMalloc(SIZE)              malloc(SIZE)
#  define jseUtilReMalloc(PTR,SIZE)        realloc(PTR,SIZE)
   
#endif

#ifdef __cplusplus
}
#endif

#endif
