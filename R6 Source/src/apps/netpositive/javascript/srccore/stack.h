/* stack.h     Local and temporary placement of cards.
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

#ifndef _STACK_H
#define _STACK_H
#if defined(__cpluplus)
   extern "C" {
#endif

struct jseCallStack
{
   struct Var **var; /* will really re-allocate enough to hold all members */
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      ubyte cookie;
#  endif
   uint Count;
};

struct jseCallStack * NEAR_CALL jsecallstackNew(void);
void jsecallstackDelete(struct jseCallStack *stk,struct Call *mycall);
void jsecallstackPush(struct jseCallStack *stk,
                      struct Var *v,jsebool deleteme);
#define jsecallstackDepth(STK)       ((STK)->Count)
#define jsecallstackPeek(STK,DEPTH)  ((STK)->var[(STK)->Count-DEPTH-1])
#define jsecallstackPop(STK)         ((STK)->var[--((STK)->Count)])

#if defined(__cpluplus)
   }
#endif
#endif
