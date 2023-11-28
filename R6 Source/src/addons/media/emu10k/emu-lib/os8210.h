
/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*
* @doc INTERNAL
* @module os8210.h | 
*  This header file contains the structure definitions and function prototypes
*  for the Operating System Abstraction Layer.
*
* @iex 
* Revision History:
* Version	Person	Date		Reason
* -------	------	----		---------------------------------- 
*  0.001	JK		Sep 30, 97	Created.
******************************************************************************
* @doc EXTERNAL
* @contents1 EMU OS8210 Programmer's Manual |
*  This document describes the Operating System Abstraction Layer API. 
*  The OSAL provides a set of functions which insulate the core driver
*  from the specifics of a particular operating system implementation.
*  It includes functions for allocating and freeing memory, for allocating
*  and freeing physical pages, for translating virtual addresses to lists
*  of physical page frames (and vice versa), and other such operations which
*  fall into the domain of the host operating system.
*  <nl><nl>
*  Glossary of terms:
*  @flag Sample Address | A virtual address which is used by the EMU 8010
*   to address a particular sample.  This address is translated by the EMU8010
*   into a PCI address using that chip's page table.
*  @flag Host Physical Address | The host physical address is the value
*   which is actually issued by the CPU or PCI bridge when it attempts to
*   read or write the physical memory.
*  @flag Host Virtual Address | The Host Virtual Address is the address
*   that the driver uses when it reads from or writes to memory.  This
*   address is translated into a Host Physical Address by the CPU's 
*   Memory Management Unit.  Host virtual addresses are also known as
*   Linear Addresses.
*  @flag Locked memory | The driver assumes that all of the functions in
*   this file produce locked physical pages.  When a page is locked, it
*   cannot be swapped or relocated until it is explicitly freed.  The driver
*   also assumes that the virtual address corresponding to a particular
*   set of locked and mapped pages remains valid.
*  @flag Page Frame Number | The driver assumes that pages are 4K in size.
*   The page frame number is the most signficant 20 bits of 32 bit physical
*   address.  
*/

#ifndef __OS8210_H
#define __OS8210_H

#include "datatype.h"
#include "aset8210.h"


/*****************************************************************************
 *  Definitions and macros
 *****************************************************************************/

/* Error return codes */
#define OSERR_NO_MEMORY			1
#define OSERR_BAD_ADDRESS		2
#define OSERR_BAD_MAPPING		3

/* osAllocPhysPages flags */
#define OSALLOC_CONTIGUOUS		1

/*****************************************************************************
 *  Enumerated type declarations 
 *****************************************************************************/

/*****************************************************************************
 *  Structure and typedef declarations
 *****************************************************************************/

/* @type OSVADDR | This type is used to store host virtual addresses. 
 */
typedef void * OSVIRTADDR;

/* @type OSPHYSADDR | This type is used to store Host physical addresses.
 */
typedef DWORD OSPHYSADDR;

/* @type OSSAMPADDR | This type is used to store Sample memory addresses.
 */
typedef DWORD OSSAMPADDR;


/* @type OSMEMHANDLE | This is an opaque handle which is used to
 *  reference a particular region of allocated host memory.
 */
typedef DWORD OSMEMHANDLE;


/* @type OSMUTEXHDL | A Mutex handle for the mutex operations.
 */
typedef DWORD OSMUTEXHDL;

/* @type OSTIMEOUTHANDL | A handle for a previously scheduled timeout.
 */
typedef DWORD OSTIMEOUTHANDLE;

typedef void (*OSCALLBACK)(DWORD dwData);

/*****************************************************************************
 * Heinous Macros
 *****************************************************************************/
/*
 * @func void | IDISABLE |
 * This macro disables interrupts, and must be followed up with
 * an <f IENABLE>.
 */
#define IDISABLE() osDisableInterrupts()

/*
 * @func void | IENABLE |
 * This macro reenables interrupts after a call to <f IDISABLE>
 */

#define IENABLE() osEnableInterrupts()

#define OS_X86MUTEX OSMUTEXHDL

#define OS_CREATEX86MUTEX(m)  osCreateMutex(&(m))
#define OS_WAITONX86MUTEX(m)  osWaitMutex(m)
#define OS_RELEASEX86MUTEX(m) osReleaseMutex(m)
#define OS_DELETEX86MUTEX(m)  osDeleteMutex(m)

/*****************************************************************************
 * Functions
 *****************************************************************************/

BEGINEMUCTYPE

/* @func Allocates a specified number of committed 4K pages and provides a 
 *  virtual mapping for them.  These pages are NOT locked.
 *
 * @parm DWORD | dwNumPages | The number of 4K physical pages to allocate.
 * @parm OSMEMHANDLE * | retMemHdl | A pointer to the OSMEMHANDLE that will 
 *  receive  the newly allocated handle.
 *
 * @rdesc Returns the virtual address at which the pages are mapped.  If
 *  the routine fails, NULL is returned.
 */
EMUAPIEXPORT OSVIRTADDR osAllocPages(DWORD dwNumPages,  OSMEMHANDLE *memhdl /* IO */);


/* @func  Allocates the specified number of physical pages such that the
 *  pages are contiguous.  This routine may only be called early in the
 *  driver initialization sequence, and even then there is no absolute
 *  guarantee that a sufficient number of contiguous pages will be found.
 *  The returned pages are locked.
 *  
 * @parm DWORD | dwNumPages | The number of 4K physical pages returned.
 * @parm OSPHYSADDR | retPhysicalAddr | A pointer to the OSPHYSADDR that receives
 *  the physical address of the first page in the contiguous range.
 */
EMUAPIEXPORT OSVIRTADDR osAllocContiguousPages(DWORD dwNumPages, OSPHYSADDR *retPhysicalAddr /* IO */,
                       OSMEMHANDLE *retMemHdl /* IO */);

/* @func Frees previously allocated pages and releases the virtual address 
 *  range that mapped the pages.
 *
 * @parm DWORD | dwNumPages | The number of pages in the array.
 * @parm OSPHYSADDRS * | physAddrs | A pointer to an array containing the
 *  addresses of the pages to be freed.
 * @parm OSVIRTADDR | virtAddr | The virtual address that was mapping the 
 *  pages.
 *
 * @rdesc Returns SUCCESS if no error occurs during the free process.  
 *  Otherwise, one of the following will be returned:
 *  @flag OSERR_BAD_ADDRESS | <t physAddrs> was an invalid pointer.
 *  @flag OSERR_BAD_MAPPING | <t virtAddr> doesn't actually map the given physical
 *   pages.
 */
EMUAPIEXPORT EMUSTAT osFreePages(OSMEMHANDLE memhdl);


/* @func Allocates the specified number of bytes from the system's heap and
 *  returns a host virtual address.  This function is intended as an 
 *  OS-independent version of the standard malloc routine.  The memory returned
 *  is pageable.
 *	
 * @parm DWORD | dwNumBytes | The number of bytes to allocate.
 * @parm DWORD | dwFlags    | Not used.
 *
 * @rdesc  Returns the virtual address of the first allocated byte.
 */
EMUAPIEXPORT OSVIRTADDR osHeapAlloc(DWORD dwNumBytes, DWORD dwFlags);


/* @func Allocates the specified number of bytes from the system's locked
 *  heap and returns a host virtual address.
 */
EMUAPIEXPORT OSVIRTADDR osLockedHeapAlloc(DWORD dwNumBytes);

/* @func Frees the memory associated with the given virtual address. This
 *  memory must have been originally allocated with the osHeapAlloc function.
 * 
 * @parm OSVIRTADDR | virtAddr   | The address of the memory to free.
 */
EMUAPIEXPORT void osHeapFree(OSVIRTADDR virtAddr);

/* @func Deallocate the locked heap memory. */
EMUAPIEXPORT void osLockedHeapFree(OSVIRTADDR virtAddr, DWORD dwNumBytes);

/* @func Translates a host virtual address range into a set of host physical
 *  page addresses and locks the pages.
 *
 * @parm OSVIRTADDR | virtAddr | The host virtual address which points to the
 *  beginning of the virtual range.
 * @parm DWORD | virtSize | The number of pages in the virtual address range
 *  to translate.
 * @parm DWORD | dwNumRetPages | The number of physical addresses to return to
 *  the caller
 *
 * @rdesc Returns SUCCESS if the function completes without error. Otherwise
 *  a value > 0 will be returned.
 */
EMUAPIEXPORT EMUSTAT osLockVirtualRange(OSVIRTADDR virtAddr, DWORD dwNumPages,  
										DWORD dwNumRetPages /* VSIZE */,
										OSPHYSADDR *retPhysAddrs /* IO */);

/* @func  Releases the lock held on the virtual range and makes the
 *  pages swappable/pageable.  
 *
 * @parm OSVIRTADDR | virtAddr | The host virtual address which points to the
 *  beginning of the virtual range.
 * @parm DWORD | dwLength | The number of pages in the virtual range.
 *
 * @rdesc Return SUCCESS if the function completes without error.  Otherwise,
 *  a value > 0 will be returned.
 */
EMUAPIEXPORT EMUSTAT osUnlockVirtualRange(OSVIRTADDR virtAddr, DWORD dwNumPages);


/* @func Creates a unique Mutex semaphore and writes its handle into <p pdwHandle>.
 *
 * @parm DWORD * | pdwHandle | A pointer to a DWORD that will receive the mutex
 *  handle.
 * @rdesc Returns SUCCESS if the value is successfully allocated, and FAILURE 
 *  otherwise.
 */
EMUAPIEXPORT EMUSTAT osCreateMutex(OSMUTEXHDL *pmutexhdl);


/* @func Attempts an instantaneous request to obtain the specified mutex
 *  semaphore.
 *
 * @parm DWORD | dwHandle | Specifies mutex semaphore.
 *
 * @rdesc If it is available, the OS gives the semaphore to the
 * process and returns TRUE.  If it is unavailable, FALSE is returned.
 * This macro does not wait for it to be available.
 */
EMUAPIEXPORT EMUSTAT osRequestMutex(OSMUTEXHDL mutexhdl);

/* @func Releases a semaphore that was granted to a process.
 *
 * @parm ULONG | ulHandle | Specifies mutex semaphore.
 */
EMUAPIEXPORT void osReleaseMutex(OSMUTEXHDL mutexhdl);

/* @func  Waits INDEFINITELY for a semaphore to be released
 * by another process.  Once the semaphore is available, it will be
 * given to the calling process and execution will continue.
 *
 * @parm | ulHandle | Specifies mutex semaphore.
 */
EMUAPIEXPORT void osWaitMutex(OSMUTEXHDL mutexhdl);


/* @func Deallocates the mutex semaphore.
 *
 * @parm ULONG | ulHandle | Specifies mutex semaphore.
 */
EMUAPIEXPORT void osDeleteMutex(OSMUTEXHDL mutexhdl);


/* @func This macro is the same as <f OS_REQUESTMUTEX>, except that the
 * semaphore is not given to the calling process, if it is available.
 * This is simply to test the status of a semaphore.
 *
 * @parm DWORD | ulHandle | Specifies mutex semaphore.
 *
 * @rdesc If it is available, this macro returns TRUE.  If it is 
 * unavailable, FALSE is returned.
 */
EMUAPIEXPORT BOOL osPollMutex(DWORD dwHandle);

/* @func This function is invoked by the OS_CALLBACK in fxconfig.h
 *  This function schedules a callback to a ring3 application
 *  via a ring3 helper DLL     
 */
EMUAPIEXPORT void OSFxCallBack( ULONG ulHandle, ULONG callID, 
							    ULONG ulEvent, ULONG ulParam, 
				                ULONG ulSystemParam );   


/* @func Schedule a function to execute in the future.
 *  This routine schedules functions with a relatively low
 *  priority, so it shouldn't be used for things which must
 *  execute at precise intervals.  On the other hand, it
 *  runs at callback time, so there are no restrictions on
 *  what the callback function can do.
 */
EMUAPIEXPORT EMUSTAT osScheduleTimeout(DWORD dwMillisecs, 
                                       OSCALLBACK fCallback,
                                       DWORD dwUser,
                                       OSTIMEOUTHANDLE *pRetHandle);

/* @func Stop a previously scheduled timeout.  
 */
EMUAPIEXPORT EMUSTAT osUnscheduleTimeout(OSTIMEOUTHANDLE);


/* @func Get the current system tick.  The system tick is the current
 *  number of milliseconds since the system started.  While there
 *  is no hard requirement that the value of the tick update every
 *  millisecond, this is preferable.
 */
EMUAPIEXPORT DWORD osGetCurrentTick();


/* @func Reschedule the timeout to trigger again.  Useful
 *  for periodic interrupt applications.
 */
EMUAPIEXPORT EMUSTAT osRescheduleTimeout(OSTIMEOUTHANDLE, DWORD dwMillisecs);


/* @func Request that the OS call the specified routine as soon
 *  as possible in a "normal" non-ISR kernel environment (this is
 *  similar in concept to NT's deferred procedure call).  The
 *  intent of this routine is that it can be called at ISR-time to
 *  schedule lower-priority processing in a less restrictive
 *  environment.  OS's with no limitations about when routines can 
 *  be called may turn around and invoke this routine immediately.
 */
EMUAPIEXPORT EMUSTAT osScheduleCallback(OSCALLBACK callback, DWORD dwParameter);

/* @func Disable interrupts.  This function supports nested calls,
 *  so it keeps an internal count of the number of times it gets
 *  invoked so that an equivalent number of osEnableInterrupts 
 *  will be needed before the interrupt mask is changed.
 */
EMUAPIEXPORT void osDisableInterrupts();


/* @func Enable interrupts.  This function supports nested calls */
EMUAPIEXPORT void osEnableInterrupts();

  ENDEMUCTYPE

#endif /* __OS8210_H */
