
/////////////////////////////////////////////////////////////////////
//                           WIN_MEM.H
// This module is a Windows memory manager for handling very
// large segments.  The routines herein must use the 'huge'
// keyword to avoid the problems associated with 'far' pointers.
// Even though far pointers (in Intel-land) have an extent of 32
// bits, the offset of the pointer will wrap once it exceeds
// 0xFFFF.  In other words, the segment address will _not_
// increment as you might expect.  Therefore we must normalize
// the pointers with the 'huge' keyword.  A huge pointer, like a
// far pointer, has an extent of 32 bits, but because it is
// normalized, the segment will have a value from 0 to 15 since
// a segment can start every 16 bytes.  Please note that there
// is only one possible huge address, or more exactly,
// only one Segment:Offset pair.
//
// Person              Date           Reason
// ------------------  ----------     --------------------------
// Michael R. Preston  8/5/96         Added PTRMEMSET() and PTRMEMCPY().
// Michael R. Preston  5/16/96        Added memory tracking
// Robert S. Crawford  5/4/95         Moved all the definitions to DATATYPE.H
// Mike Guzewicz       3/14/95        Add SYS_MSDOS
// Robert S. Crawford  12/6/94        Initial Creation
//                                                                                                                                  //
/////////////////////////////////////////////////////////////////////
#ifndef __WIN_MEM_H
#define __WIN_MEM_H

/////////////////////////////
//       Includes          //
/////////////////////////////

#include <stddef.h>
#include "datatype.h"
//////////////////////////////////////////////////////////////////
// Please notate the desired memory manager by defining it
// for the desired memory routines for your environment as 
// specified by the global SYSTEM compile flag.
//////////////////////////////////////////////////////////////////

// System specific header file declarations go here

////////////////////////////////////////////////////////////////
// The following header files are only applicable under Windows
////////////////////////////////////////////////////////////////
//#if defined(__SYS_WINDOWS)
//#include <windows.h>           // The all-important windows include file

////////////////////////////////////////////////////////////////
// The following is normal memory management using new/delete
////////////////////////////////////////////////////////////////
//#elif defined (__SYS_GENERAL) || defined (__SYS_UNIX)
//#include "datatype.h"

//////////////////////////////////////////////////////////////////////
// The following is normal memory management using farmalloc/farfree
//////////////////////////////////////////////////////////////////////
#if defined(__SYS_MSDOS)
//#include "datatype.h"

#ifdef __BOUND_16BIT
//#include <alloc.h>
void * operator new(size_t size);
void   operator delete(void * p);
#endif

#endif

/////////////////////////////
//        Defines          //
/////////////////////////////

#define DEBUG_MEMORY 0

#define PTRMEMSET(ptr, val, num) \
{DWORD i, tmpnum = num/(sizeof (*ptr)); \
 for (i = 0; i < tmpnum; i++) \
    memset(ptr+i, val, sizeof (*ptr)); }
#define PTRMEMCPY(ptr1, ptr2, num) \
{DWORD i, tmpnum = num/(sizeof (*ptr)); \
 for (i = 0; i < tmpnum; i++) \
    memcpy(ptr1+i, ptr2+i, sizeof (*ptr)); }

/////////////////////////////
//       Declarations      //
/////////////////////////////

///////////////////////////////////////////////////////////////////
// These are the prototypes of what the malloc functions MUST have
///////////////////////////////////////////////////////////////////

// For GREATER THAN 64kByte contiguous memory
// IE BYTEPTR value = (BYTEPTR)Alloc(16*sizeof(CHAR));

VOIDPTR Alloc(DWORD dwSize);
void    Dealloc(VOIDPTR pMem);
VOIDPTR Memset(VOIDPTR pMem, WORD wValue, DWORD dwSize);
VOIDPTR HFMemCopy(VOIDPTR pDest, VOIDPTR pSrc, DWORD dwNBytes);

// For LESS THAN 64 kByte contiguous memory whose pointers do NOT
// have the datatype xxxPTR defined in datatype.h
// IE CHAR *value = (CHAR *)NewAlloc(16*sizeof(CHAR));
// CAUTION should be exercised when using these to insure you really
// have less than 64kB of contiguous memory AND you are working on 16
// bit OSs!

void * NewAlloc(DWORD dwSize);
void   DeleteAlloc(void *pMem);

#if (DEBUG_MEMORY == 1)
void OutputTrackerResult(void);
#endif

#if defined (__TRACK_MEMORY)

struct mem_ptr_list
{
   void *ptr;
   size_t size;
   struct mem_ptr_list *next;
};

void *operator new(size_t size);
void operator delete(void *ptr);

#endif

#endif // __WIN_MEM_H
//////////////////////// End of WIN_MEM.H ///////////////////////////




