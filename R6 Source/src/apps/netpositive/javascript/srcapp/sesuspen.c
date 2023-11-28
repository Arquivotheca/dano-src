/* sesuspen.c   Default implementation of Suspend Application Service
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

static jsebool
Suspend(jseContext jsecontext, ulong MilliSeconds)
{
   UNUSED_PARAMETER(jsecontext);

#  if defined(__JSE_DOS16__) || defined(__JSE_DOS32__) || defined(__JSE_WIN16__)
   {
      clock_t end_time;
#    if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
      end_time = clock() + (clock_t)((jsenumber)MilliSeconds * (jsenumber)CLOCKS_PER_SEC / (jsenumber)1000);
#    else
      end_time = clock() + MilliSeconds * CLOCKS_PER_SEC / 1000;
#    endif
      while( clock() <= end_time ) ;
   }
#  elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
      DosSleep(MilliSeconds);
#  elif defined(__JSE_CON32__) || defined(__JSE_WIN32__)
      Sleep(MilliSeconds);
#  elif defined(__JSE_NWNLM__)
      delay(MilliSeconds);
#  elif defined(__JSE_UNIX__)
   {
      /* a pretty portable way to delay under Unix. */
      struct timeval tv;
      tv.tv_sec = MilliSeconds/1000;
      tv.tv_usec = (MilliSeconds%1000)*1000;
      select(0,NULL,NULL,NULL,&tv);
   }
#  elif defined(__JSE_MAC__)
   {
      unsigned long ignored;
      unsigned long ticks = (unsigned long)(((float) MilliSeconds / 1000) * 60);
      Delay( ticks, &ignored );
   }
#  else
#     error Need suspend for this OS
      assert( JSE_DEBUG_FEEDBACK(False));
#  endif
   return True;
}

void 
AddStandardService_Suspend(jseContext jsecontext)
{
   assert( SUSPEND_CONTEXT == NULL );
   
   jseSetSharedData(jsecontext,SUSPEND_NAME,(void _FAR_ *)Suspend,NULL);
}
