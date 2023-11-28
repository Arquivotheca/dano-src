/* mempool.h - allocator pools to avoid constant calls to malloc()
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

#if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL) && !defined(_MEMPOOL_H)
#define _MEMPOOL_H

struct Allocator
{
   void **saved_ptrs;
   uint num_saved;
   uint item_size;
   ulong pool_size;
};


jsebool allocatorInit(struct Allocator * handle,size_t size,uint initial);
void allocatorTerm(struct Allocator * handle);

#if !defined(JSE_INLINES) || (0==JSE_INLINES)
   void * NEAR_CALL allocatorAllocItem(struct Allocator *This);
   void NEAR_CALL allocatorFreeItem(struct Allocator *This,void *Item);
#else
#  define allocatorAllocItem(this) (((this)->num_saved)? \
      (this)->saved_ptrs[--((this)->num_saved)]:\
      jseMustMalloc(void,(this)->item_size))
#  define allocatorFreeItem(this,item) \
      { \
         if( (this)->pool_size<=(this)->num_saved ) \
            jseMustFree(item); \
         else \
            (this)->saved_ptrs[((this)->num_saved)++] = item; \
      }
#endif

#endif
