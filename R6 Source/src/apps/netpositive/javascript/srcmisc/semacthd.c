/* semacthd.c - Macintosh Thread functions
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

#if defined(__JSE_MAC__)

long MacThreadYieldFrequency = MAC_THREAD_YIELD_FREQ;
long MacThreadNumberOfTasks = 0;
jsebool MacThreadPoolInitialized = False;
short MacThreadExclusiveThreadCount = 0;

struct MacThread *
NewMacThread( ThreadEntryProcPtr entryFunc, void * entryParam )
{
   struct MacThread * This = jseMustMalloc(struct MacThread,sizeof(struct MacThread));
    This->threadEntryFunc = entryFunc;
   This->threadEntryParam = entryParam;
   This->threadID = kNoThreadID;
  
   #if MAC_THREAD_POOL_SIZE > 0
      if ( !MacThreadPoolInitialized )
      {
        if ( CreateThreadPool( kCooperativeThread, MAC_THREAD_POOL_SIZE,
                               MAC_THREAD_STACK_SIZE ) == noErr )
           MacThreadPoolInitialized = True;
      }
   #endif
   return This;
}

void
DeleteMacThread( struct MacThread * This )
{
   ThreadID id = This->threadID;

   if( id != kNoThreadID )
      --MacThreadNumberOfTasks;
   MacThreadExclusiveThreadCount = 0; /* Re-enable multithreading */
   
   jseMustFree( This );     /* Free memory occupied by structure */
   MacThreadDispose( id );  /* We must dispose of the thread afterwards because
                             * we might dispose of the current thread
                             */
}

void
MacThreadDispose( ThreadID id )
{
   if ( id != kNoThreadID )
   {
      #if MAC_THREAD_POOL_SIZE > 0
        if ( MacThreadPoolInitialized )
           DisposeThread( id, NULL, True );
        else
           DisposeThread( id, NULL, False );
      #else
         DisposeThread( id, NULL, False );
      #endif
   }
}

jsebool
MacThreadRun( struct MacThread * This )
{
  OSErr  myErr = noErr;
  
  #if MAC_THREAD_MAX > 0
    if( MacThreadNumberOfTasks >= MAC_THREAD_MAX )
      return False;
  #endif
  
  #if MAC_THREAD_POOL_SIZE > 0
    if ( MacThreadPoolInitialized )
    {
      /* First we attempt to get a thread from the pool */
      myErr = NewThread( kCooperativeThread, This->threadEntryFunc, (void *) This, 
                         MAC_THREAD_STACK_SIZE, kUsePremadeThread, (void **) NULL, 
                         &This->threadID );
    }
    /* There is no pool or all the threads were used up */
    if ( !MacThreadPoolInitialized || myErr == threadTooManyReqsErr )
  #endif
  
  {
    myErr = NewThread( kCooperativeThread, This->threadEntryFunc, (void *) This,
                       MAC_THREAD_STACK_SIZE, kCreateIfNeeded, (void **) NULL,
                       &This->threadID );
  }
  
  if ( myErr != noErr )
    return False;
    
  /* Now we have successfully created the thread */
  MacThreadNumberOfTasks++;
  
  return True;
}

  static ulong lastCount = 0;

void
MacThreadOccasionalYield(  )
{
  if ( MacThreadExclusiveThreadCount )
    return;
      
  if( TickCount() -lastCount > MacThreadYieldFrequency )
  {
    YieldToAnyThread();
    lastCount = TickCount();
  }  
}

void
MacThreadRunTasks( ulong ticksToRun )
{
  if( MacThreadExclusiveThreadCount )
    return;
  
  if( MacThreadNumberOfTasks > 0 )
  {
    ulong current = TickCount();
    ulong start = current;
    ulong end = start += ticksToRun;

    do
    {
      YieldToAnyThread();
      current = TickCount();
    } while( (current < end) && (MacThreadNumberOfTasks > 0) );
  }

  return;
}

#else
   ALLOW_EMPTY_FILE
#endif
