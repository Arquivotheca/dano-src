/* jsemem.c    Random utilities used by ScriptEase.
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

#include "jseopt.h"
#include "seuni.h"
#ifdef __JSE_UNIX__
#include "unixfunc.h"
#endif

#if !defined(__JSE_MAC__) && !defined(__JSE_WINCE__)
  #include <assert.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#include <new.h>
#endif

#if defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)
   #include <windows.h>
#endif
#if !defined(__CGI__) && !defined(__JSE_WIN16__) && !defined(__JSE_WIN32__)
   #include <stdio.h>
#endif
#include "dbgprntf.h"
#include "jsemem.h"

#if defined __JSE_MAC__

static struct MacTextBox * 
SetupTextBox()
{
   struct MacTextBox * textbox = NewMacTextBox();
   assert( MacTextBoxIsValid( textbox ) );
   MacTextBoxSet(textbox, "0003: Insufficient Memory to continue operation.");
   return textbox;
}
   
   #include <sound.h>
   struct MacTextBox * error_box = NULL;
#endif

/* TOOLKIT USER: ASSUMED MEMORY SUCCESS: non-large memory allocations are
 * assumed to success, so they go through these "must" malloc and new
 * functions.  For different error reporting on failed mallocs (which would
 * only fail in extreme script cases on modern operating systems) replace
 * these functions with your own memory failure error reporting.  If you
 * already have set_new_handler then remove this one we added. */

void jseInsufficientMemory(void)
{
#if !defined(__CGI__) && defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   static CONST_DATA(jsechar) NoMemMessage[] =
         UNISTR("0003: Insufficient Memory to continue operation.");
#  if (defined(__JSE_WIN16__) || defined(__JSE_WIN32__)) && !defined(__JSE_WINCE__)
      MessageBox((HWND)0,NoMemMessage,NULL,MB_TASKMODAL|MB_ICONHAND|MB_OK);
#  else
#     if defined(__JSE_MAC__)
         assert( error_box != NULL );
         SysBeep(10);
         MacTextBoxShow( error_box, NULL );
#     elif defined(__JSE_PSX__)
         printf(UNISTR("\a\a%s\n"),NoMemMessage);
#     elif defined(__JSE_WINCE__)
         MessageBox((HWND)0,NoMemMessage,NULL,MB_APPLMODAL|MB_ICONHAND|MB_OK);
#     else
         fprintf(stderr,UNISTR("\a\a%s\n"),NoMemMessage);
#     endif
#  endif
#endif
   assert(False);               /* to ease debugging */
#  if defined(_WINDLL)
#     error for building in a _WINDLL you must provide an alternative\
   to exit() for fatal abort
#  else
      exit(EXIT_FAILURE);
#  endif
}


#if !defined(JSE_MEM_DEBUG) || (0==JSE_MEM_DEBUG)

void *jseUtilMustMalloc(uint size)
{
   void *ret;
   #ifdef __JSE_MAC__
      /* This is a very bad place to initalize this, but there is no function
       * outside of the core that is guaranteed to be called upon
       * initialization.  This is the best alternative I could think of.
       */
      if ( error_box == NULL )
      {
         error_box = (struct MacTextBox *) 1;
         /* Otherwise we go into an infinite loop */
         error_box = SetupTextBox();
      }
   #endif
   assert( 0 < size );
   if ( NULL == (ret = malloc(size)) )
      jseInsufficientMemory();
   return(ret);
}

void *jseUtilMustReMalloc(void *PrevMalloc,uint size)
{
   void *ret;
   assert( 0 < size );

/* You should not remalloc NULL */

   if ( NULL == (ret = (PrevMalloc)?realloc(PrevMalloc,size):malloc(size)) )
      jseInsufficientMemory();
   return(ret);
}

#else /* !defined(JSE_MEM_DEBUG) || (0==JSE_MEM_DEBUG) */

/* anal versions of allocation routines */

   /* ok variable; in thread lock and only for debugging */
static VAR_DATA(ulong) jseDbgAllocationCount = 0;
static VAR_DATA(jsebool) jseDbgInMemDebug = False;
static VAR_DATA(jsebool) jseDbgMemVerbosity = False;
static VAR_DATA(ulong) jseDbgAllocSequenceCounter = 0;
 /* used when debugging to catch a special value */
static VAR_DATA(ulong) DebugWatch = (ulong)(-1);
static VAR_DATA(uint) SizeWatch = (uint)(-1);
static VAR_DATA(ulong) TotalMemoryAllocated = 0;
static VAR_DATA(ulong) MaxTotalMemoryAllocated = 0;


static void NEAR_CALL DebugWatcher(ulong sequence, uint size)
{
   if ( sequence == DebugWatch )
      DebugPrintf(UNISTR("Hit the DebugWatch value %lu."),DebugWatch);
   if(size == SizeWatch)
      DebugPrintf(UNISTR("Hit the SizeWatch value %u."),SizeWatch);

}

static jsechar * NEAR_CALL jsemem_a_to_u(const char *asc_string)
{
   /* return this string convert to unicode in-place */
   static VAR_DATA(jsechar) uni_string[800];
   size_t i;
   for ( i = 0; i < (sizeof(uni_string)/sizeof(jsechar)) - 4; i++ )
   {
      if ( 0 == (uni_string[i] = (jsechar)(asc_string[i])) )
         break;
   }
   uni_string[i] = 0;
   return uni_string;
}

struct AnalMalloc {
   struct AnalMalloc *Prev;
   uint size; /* this does not include the four bytes at the beginning
                 and end */
   ulong AllocSequenceCounter;
   ulong line;           /*__LINE__*/
   const char * file; /*__FILE__*/
   ubyte Head[4];
   ubyte data[1];
};
static VAR_DATA(struct AnalMalloc *) RecentMalloc = NULL;
   /* ok variable; in thread lock and only for debugging */
#if defined(__JSE_WIN32__) || defined(__JSE_CON32__)
   static VAR_DATA(CRITICAL_SECTION) CriticalMallocSectionSemaphore;
      /* ok because needed for lock */
   #define ExclusiveMallocThreadStart() \
       EnterCriticalSection(&CriticalMallocSectionSemaphore);
   #define ExclusiveMallocThreadStop()  \
       LeaveCriticalSection(&CriticalMallocSectionSemaphore);
#else
   #define ExclusiveMallocThreadStart();  /* */
   #define ExclusiveMallocThreadStop();   /* */
#endif

   void
jseInitializeMallocDebugging()
{
#if defined(__JSE_WIN32__) || defined(__JSE_CON32__)
   InitializeCriticalSection(&CriticalMallocSectionSemaphore);
#endif
   TotalMemoryAllocated = 0;
   MaxTotalMemoryAllocated = 0;
}

   void
jseTerminateMallocDebugging()
{
#if defined(__JSE_MAC__)
  if ( error_box != NULL )
     DeleteMacTextBox(error_box);
  error_box = NULL;
#endif

   if ( 0 != jseMemReport(False) ) {
      jseMemDisplay();
      DebugPrintf(UNISTR("9002: Leaving, but there are %ld allocations."),
                  jseMemReport(False));
      assert(False);
      exit(EXIT_FAILURE);
   }
   /*printf_jsechar(UNISTR("\nMaximum memory allocated = %lu\n"),MaxTotalMemoryAllocated);*/
   assert( 0 == TotalMemoryAllocated );
#if defined(__JSE_WIN32__) || defined(__JSE_CON32__)
   DeleteCriticalSection(&CriticalMallocSectionSemaphore);
#endif
}

static CONST_DATA(ubyte) AnalMallocHead[] = {'H','e','a','d'};
   /* size of 4 is hardcoded below */
static CONST_DATA(ubyte) AnalMallocFoot[] = {'F','o','o','t'};
   /* size of 4 is hardcoded below */

   void
jseMemDisplay()
{
   ExclusiveMallocThreadStart();
   {
      struct AnalMalloc *AM;
      for ( AM = RecentMalloc; NULL != AM; AM = AM->Prev ) {
         DebugPrintf(
UNISTR("Memory Block: Sequence = %lu, size = %u, ptr = %08lX line=%lu of file=%s\n"),
   AM->AllocSequenceCounter,AM->size,AM->data,AM->line,jsemem_a_to_u(AM->file));
      }
   }
   ExclusiveMallocThreadStop();
}


   void *
jseUtilMalloc(uint size, ulong line, const char* file)
{
   struct AnalMalloc *AM;
   assert( 0 < size );
#  if !defined(JSE_ENFORCE_JSEMEMCHECK) || (0==JSE_ENFORCE_MEMCHECK)
#     undef malloc
#  endif
   AM = (struct AnalMalloc *)
        malloc(sizeof(*AM) - sizeof(AM->data) + size + 4);
#  if !defined(JSE_ENFORCE_JSEMEMCHECK) || (0==JSE_ENFORCE_MEMCHECK)
#     define malloc(S)       use jseMustMalloc
#  endif
   if ( NULL == AM ) {
      return NULL;
   } /* endif */
   AM->line = line;
   AM->file = file;

   memcpy(AM->Head,AnalMallocHead,4);
   memcpy(&(AM->data[size]),AnalMallocFoot,4);
   AM->size = size;
   ExclusiveMallocThreadStart();
   {
      AM->Prev = RecentMalloc;
      RecentMalloc = AM;
      jseDbgAllocationCount++;
      AM->AllocSequenceCounter = jseDbgAllocSequenceCounter++;
      DebugWatcher(AM->AllocSequenceCounter, size);
      TotalMemoryAllocated += size;
      if ( MaxTotalMemoryAllocated < TotalMemoryAllocated )
         MaxTotalMemoryAllocated = MaxTotalMemoryAllocated;
   }
   ExclusiveMallocThreadStop();

   if ( jseDbgMemVerbosity ) {
      DebugPrintf(UNISTR("Allocation #%lu allocated %u bytes at %08lX\n"),
                  AM->AllocSequenceCounter,size,AM->data);
   } /* endif */

   memset(AM->data,0xEE,size);   /* fill with non-null characters */
   return(AM->data);
}

   void *
jseUtilMustMalloc(uint size, ulong line, const char* file)
{
   void *ptr;
   #ifdef __JSE_MAC__
      if ( error_box == NULL )
      {
         error_box = (struct MacTextBox *) 1;
            /* Otherwise we go into an infinite loop */
         error_box = SetupTextBox();
      }
   #endif
   assert( 0 < size );
   ptr = jseUtilMalloc(size, line, file);
   if ( NULL == ptr  &&  !jseDbgInMemDebug ) {
      jseInsufficientMemory();
   }
   return ptr;
}

   static struct AnalMalloc **
FindAnalMalloc(void *ptr)
{
   struct AnalMalloc **AMptr;
   sint ptrOffset = (sint)(((struct AnalMalloc *)0)->data);
   struct AnalMalloc *AM = (struct AnalMalloc *)(((ubyte *)ptr) - ptrOffset);
   assert( NULL != ptr );
   assert( 1 == sizeof(ubyte) );
   assert( NULL != AM );
   for ( AMptr = &RecentMalloc; *AMptr != AM; AMptr = &((*AMptr)->Prev) ) {
      if ( NULL == *AMptr )
         break;
   } /* endfor */
   if ( NULL == *AMptr ) {
      DebugPrintf(
 UNISTR("9003: Tried to access unalloced memory %08lX; sequence = %lu, size = %u."),
 ptr,AM->AllocSequenceCounter,AM->size);
      assert(False);
      exit(EXIT_FAILURE);
   }
   if ( 0 != memcmp(AnalMallocHead,AM->Head,4) ) {
      DebugPrintf(UNISTR("9004: Beginning of pointer has been overwritten."));
      assert(False);
      exit(EXIT_FAILURE);
   }
   if ( 0 != memcmp(AnalMallocFoot,&(AM->data[AM->size]),4) ) {
         DebugPrintf(
 UNISTR("Memory Block: Sequence = %lu, size = %u, ptr = %08lX line=%lu of file=%s\n"),
 AM->AllocSequenceCounter,AM->size,AM->data,AM->line,jsemem_a_to_u(AM->file));
      DebugPrintf(
  UNISTR("9005: Tail of pointer has been overwritten: %08lX,")
  UNISTR("sequence %lu, size = %u, ptr = %08lX line=%lu of file=%s."),
  ptr,AM->AllocSequenceCounter,AM->size,AM->data,AM->line,jsemem_a_to_u(AM->file));
      assert(False);
      exit(EXIT_FAILURE);
   }
   return(AMptr);
}

   void
jseUtilMustFree(void *ptr)
{
   struct AnalMalloc **AMptr;
   struct AnalMalloc *AM;

   assert( NULL != ptr );
   ExclusiveMallocThreadStart();
   AMptr = FindAnalMalloc(ptr);
   AM = *AMptr;
   TotalMemoryAllocated -= AM->size;
   if ( jseDbgMemVerbosity ) {
      DebugPrintf(UNISTR("Freeing sequence %lu allocated memory at %08lX\n"),
                  AM->AllocSequenceCounter,ptr);
   } /* endif */
   *AMptr = AM->Prev;
   assert( 0 < jseDbgAllocationCount );
   jseDbgAllocationCount--;
   /* Fill in the data area with GARBAGE value */
#  define  BAD_DATA    0xBD
   memset(AM->data,BAD_DATA,AM->size);
#  if !defined(JSE_ENFORCE_JSEMEMCHECK) || (0==JSE_ENFORCE_MEMCHECK)
#     undef free
#  endif
      free(AM);
#  if !defined(JSE_ENFORCE_JSEMEMCHECK) || (0==JSE_ENFORCE_MEMCHECK)
#     define free(P)         use jseMustFree
#  endif
   ExclusiveMallocThreadStop();
}

   void *
jseUtilReMalloc(void *PrevMalloc,uint size, ulong line, const char* file)
{
   struct AnalMalloc **AMptr;
   struct AnalMalloc *AM;

   assert( 0 < size );
   if ( NULL == PrevMalloc ) {
      return(jseUtilMalloc(size,line,file));
   } /* endif */
   ExclusiveMallocThreadStart();
   AMptr = FindAnalMalloc(PrevMalloc);
   TotalMemoryAllocated -= (*AMptr)->size;
#  if !defined(JSE_ENFORCE_JSEMEMCHECK) || (0==JSE_ENFORCE_MEMCHECK)
#     undef realloc
#  endif
   AM = (struct AnalMalloc *)realloc(*AMptr,sizeof(*AM) -
                                     sizeof(AM->data) + size + 4);
#  if !defined(JSE_ENFORCE_JSEMEMCHECK) || (0==JSE_ENFORCE_MEMCHECK)
#     define realloc(P,S)    use jseMustReMalloc
#  endif
   if ( NULL == AM ) {
      ExclusiveMallocThreadStop();
      return NULL;
   }
   AM->line = line;
   AM->file = file;

   if ( AM->size < size ) {
      /* growing; fill in non-null bytes to look for in case of error */
      memset( &(AM->data[AM->size]), 0xDD, size - AM->size );
   }
   (*AMptr = AM)->size = size;
   memcpy(&(AM->data[size]),AnalMallocFoot,4);
   if ( jseDbgMemVerbosity ) {
      DebugPrintf(UNISTR("ReAllocated %lu to sequence %lu memory at %08lX\n"),
                  AM->AllocSequenceCounter,jseDbgAllocSequenceCounter,
                  AM->data);
   } /* endif */
   TotalMemoryAllocated += size;
   if ( MaxTotalMemoryAllocated < TotalMemoryAllocated )
      MaxTotalMemoryAllocated = TotalMemoryAllocated;

   AM->AllocSequenceCounter = jseDbgAllocSequenceCounter++;
   DebugWatcher(AM->AllocSequenceCounter, size);
   ExclusiveMallocThreadStop();
   return(AM->data);
}

   void *
jseUtilMustReMalloc(void *PrevMalloc,uint size, ulong line, const char* file)
{
   void *ptr = jseUtilReMalloc(PrevMalloc,size, line, file);
   if ( NULL == ptr  &&  !jseDbgInMemDebug ) {
      jseInsufficientMemory();
   }
   return ptr;
}

   ulong
jseMemReport(jsebool verbose) /* Do some checks on memory allocation */
{
   jsebool SaveVerbosity = jseDbgMemVerbosity;
   struct AnalMalloc * AM;
   ulong Count;

   jseDbgInMemDebug = True;
   jseDbgMemVerbosity = False;
   if ( verbose )
      DebugPrintf(UNISTR("There are currently %lu blocks allocated."),
                  jseDbgAllocationCount);
   /* loop through current stack and see that allocations all look OK */
   for ( AM = RecentMalloc, Count = 0; NULL != AM; AM = AM->Prev, Count++ )
   {
      if ( 0 != memcmp(AnalMallocHead,AM->Head,4) ) {
         DebugPrintf(UNISTR("9004: Beginning of pointer has been overwritten."));
         assert( False );
         exit(EXIT_FAILURE);
      }
      if ( 0 != memcmp(AnalMallocFoot,&(AM->data[AM->size]),4) ) {
         DebugPrintf(UNISTR("9005: Tail of pointer has been overwritten"));
            /* Why no numeric arguments? -JMC */
         assert( False );
         exit(EXIT_FAILURE);
      }
   } /* endfor */
   if ( Count != jseDbgAllocationCount ) {
      DebugPrintf(UNISTR("9006: There are %lu blocks allocated, but should be %lu"),
                  Count,jseDbgAllocationCount);
      assert( False );
      exit(EXIT_FAILURE);
   }
   if ( verbose ) {
      /* See how many times we can allocate a 100-byte chunk. */
      struct AllocLoop {
         struct AllocLoop *Prev;
      } *Recent = NULL;
      #define DEBUG_MALLOC_SIZE  100
      for ( Count = 0; ; Count++ ) {
         struct AllocLoop *New =
            jseMustMalloc(struct AllocLoop,DEBUG_MALLOC_SIZE -
                          sizeof(struct AllocLoop));
         if ( NULL == New ) {
            break;
         } else {
            New->Prev = Recent;
            Recent = New;
         } /* endif */
      } /* endif */
      DebugPrintf(UNISTR("Could allocate %u bytes %lu times."),
                  DEBUG_MALLOC_SIZE,Count);
      /* free up all the memory just allocated */
      while ( NULL != Recent ) {
         struct AllocLoop *Prev = Recent->Prev;
         jseMustFree(Recent);
         Recent = Prev;
      } /* endwhile */
   } /* endif */
   jseDbgMemVerbosity = SaveVerbosity;
   jseDbgInMemDebug = False;
   return(jseDbgAllocationCount);
}

   void
jseMemVerbose(jsebool SetVerbose)
{
   jseDbgMemVerbosity = SetVerbose;
}

   jsebool
jseMemValid(void *ptr,uint offset)
{
   struct AnalMalloc *AM = *(FindAnalMalloc(ptr));
   ExclusiveMallocThreadStart();
   AM = *(FindAnalMalloc(ptr));
   ExclusiveMallocThreadStop();
   return( offset < AM->size );
}

#endif /* #ifdef #else JSE_MEM_DEBUG */

