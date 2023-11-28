/* mempool.c    Memory pooling routines
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

#if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)

#if !defined(JSE_INLINES) || (0==JSE_INLINES)

   void * NEAR_CALL
allocatorAllocItem(struct Allocator *This)
{
   return ( This->num_saved )
        ? This->saved_ptrs[--(This->num_saved)]
        : jseMustMalloc(void,This->item_size);
}

   void NEAR_CALL
allocatorFreeItem(struct Allocator *This,void *Item)
{
   if( This->pool_size <= This->num_saved )
      jseMustFree(Item);
   else
      This->saved_ptrs[This->num_saved++] = Item;
}

#endif

#endif
