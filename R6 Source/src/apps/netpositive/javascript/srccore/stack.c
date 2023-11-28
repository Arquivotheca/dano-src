/* stack.c   Local and temporary placement of variables.
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

#include "srccore.h"

void jsecallstackDelete(struct jseCallStack *This,struct Call *call)
{
   uint i = This->Count;
   while ( i-- )
   {
      VAR_THOROUGH_REMOVE_USER(This->var[i],call);
   }
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      This->cookie = 0;
#  endif
   jseMustFree(This->var);
   jseMustFree(This);
}
