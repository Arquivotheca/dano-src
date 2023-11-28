/* sedyncal.c
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

#if defined(JSE_SELIB_DYNAMICLINK) || defined(JSE_OS2_PMDYNAMICLINK)

#  if defined (__JSE_WIN16__)
#    ifdef __cplusplus
extern "C" {
#    endif
   ulong JSE_CFUNC FAR_CALL
      WindowsDLLCall(void _FAR_ * function,jsebool C_Call/*else pascal*/,
                     uint ParameterCount,uword16 _FAR_ *Parameters);
#    ifdef __cplusplus
}
#    endif
#  elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)
#    ifdef __cplusplus
extern "C" {
#    endif
   slong JSE_CFUNC
         NTDLL32CALL(FARPROC function,jsebool PushLeftFirst/*else push right first*/,
                     jsebool PopParameters/*else callee pops parameters*/,
                     uint ParameterCount,sword32 *Parameters);
#    ifdef __cplusplus
}
#    endif
#  elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
#    include "os2\seos2cal.h"
#  endif

struct DynamicCall *
dynamiccallNew( DYNA_SYMBOL _symbol, uint max_parameters, ulong _flags )
{
   struct DynamicCall * This;
   This = jseMustMalloc(struct DynamicCall,sizeof(struct DynamicCall));

   This->symbol = _symbol;
   This->flags = _flags;

   This->add_offset = 0;
   This->get_offset = 0;

#  ifndef NDEBUG
   This->max_param = max_parameters;
#  else
   UNUSED_PARAMETER(max_parameters);
#  endif

#  if defined(__JSE_UNIX__)
   assert(max_parameters <= DYNA_MAX_PARAM-DYNA_MIN_PARAM);
   This->stack = jseMustMalloc(ulong,(DYNA_MAX_PARAM-DYNA_MIN_PARAM)*sizeof(ulong));
#  elif defined(__JSE_MAC__)
   assert(max_parameters <= DYNA_MAX_PARAM-DYNA_MIN_PARAM);
   This->stack = jseMustMalloc(uint,(DYNA_MAX_PARAM-DYNA_MIN_PARAM)*sizeof(uint));
#  elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)
   {
      uint safe_alloc_size = 1 + (sizeof(jsenumber) / sizeof(*(This->stack))) * max_parameters * sizeof(*(This->stack));
      This->stack = jseMustMalloc(sword32,safe_alloc_size);
      memset(This->stack,0,safe_alloc_size);
   }
#  elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
   {
      uint safe_alloc_size = 1 + (sizeof(jsenumber) / sizeof(*(This->stack))) * max_parameters * sizeof(*(This->stack));
      This->stack = jseMustMalloc(uword16,safe_alloc_size);
      memset(This->stack,0,safe_alloc_size);
   }
#  elif defined(__JSE_WIN16__)
   {
      uint safe_alloc_size = 1 + 2 * max_parameters * sizeof(*(This->stack));
      This->stack = jseMustMalloc(uword16,safe_alloc_size);
   }
#  else
#    error Dont know how to initialize dynamic stack on this OS
#  endif
   return This;
}

void
dynamiccallDelete(struct DynamicCall *This)
{
#  if defined(__JSE_MAC__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__) || \
        defined(__JSE_WIN16__) || defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) || \
        defined(__JSE_UNIX__)
   jseMustFree(This->stack);
#  else
#    error Dont know how to free dynamic stack on this OS
#  endif
   jseMustFree(This);
}

void
dynamiccallAdd(struct DynamicCall *This,void * data, ulong data_flags )
{
#  if defined(__JSE_UNIX__)
      This->stack[This->add_offset++] = (ulong) data;
#  elif defined(__JSE_MAC__)
      UNUSED_PARAMETER(data_flags);
      This->stack[This->add_offset++] = (uint) data;
#  elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)
      This->stack[This->add_offset++] = (sword32) data;
#  elif defined(__JSE_WIN16__)
      if (data_flags & DataType16 )
      {
         This->stack[This->add_offset++] = (uword16) data;
      }
      else
      {
         uint segment_index = (data_flags & CDecl) ? 1 : 0;
         This->stack[This->add_offset + segment_index] = (uword16) (((uword32) data) >> 16 );
         This->stack[This->add_offset + (segment_index ^ 1)] = (uword16) data;
         This->add_offset += 2;
      }
#  elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
      if ( data_flags & BitSize16 )
      {
         if ( data_flags & DataType16 )
         {
            This->stack[This->add_offset++] = (uword16) data;
         }
         else
         {
            PVOID16 data16 = (PVOID16) data;
            uint segment_index = (data_flags & CDecl) ? 1 : 0;
            This->stack[This->add_offset + segment_index] = HIUSHORT(data16);;
            This->stack[This->add_offset + (segment_index ^ 1)] = LOUSHORT(data16);
            This->add_offset += 2;
         }
      }
      else
      {
         assert( data_flags & BitSize32 );
         ((sword32 *)(This->stack))[This->add_offset++] = (sword32) data;
      }
#  else
#    error Dont know how to add parameter to dynamic stack on this OS
#  endif
}

void *
dynamiccallGet(struct DynamicCall *This,ulong data_flags )
{
#  if defined(__JSE_UNIX__) || defined(__JSE_MAC__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)
      UNUSED_PARAMETER(data_flags);
      return (void *) This->stack[This->get_offset++];
#  elif defined(__JSE_WIN16__)
      if ( data_flags & DataType16 )
         return (void *) This->stack[This->get_offset++];
      else
      {
         uint index = ( data_flags & CDecl ) ? 1 : 0;
         uword16 segment = This->stack[This->get_offset + index];
         uword16 offset = This->stack[This->get_offset + (index ^ 1)];
         This->get_offset += 2;
         return (void *) SegOffPtr(segment,offset);
      }
#  elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
      if ( data_flags & DataType16 )
         return (void *) This->stack[This->get_offset++];
      else
      {
         uint index = ( data_flags & CDecl ) ? 1 : 0;
         USHORT sel = This->stack[This->get_offset + index];
         USHORT off = This->stack[This->get_offset + (index ^ 1)];
         This->get_offset += 2;
         return (void *) MAKE16P(sel,off);
      }
#  else
#    error Dont know how to get parameter from dynamic stack
#  endif
}


slong
   dynamiccallCall(struct DynamicCall *This,jseContext jsecontext )
{
#  if defined(__JSE_UNIX__)
      typedef ulong (JSE_CFUNC FAR_CALL *dynamic_function)
                     (ulong,ulong,ulong,ulong,ulong,ulong,ulong,ulong,ulong,ulong,
                     ulong,ulong,ulong,ulong,ulong,ulong,ulong,ulong,ulong,ulong);

      UNUSED_PARAMETER(jsecontext);
      assert( DYNA_MAX_PARAM - DYNA_MIN_PARAM == 20 );
      return (slong) ((dynamic_function)This->symbol)
         (This->stack[0],This->stack[1],This->stack[2],This->stack[3],This->stack[4],This->stack[5],This->stack[6],
          This->stack[7],This->stack[8],This->stack[9],This->stack[10],This->stack[11],This->stack[12],This->stack[13],
          This->stack[14],This->stack[15],This->stack[16],This->stack[17],This->stack[18],This->stack[19]);

#  elif defined(__JSE_MAC__)
      typedef long (*dynamic_function)(uint,uint,uint,uint,uint,uint,uint,uint,uint,uint,uint,uint);
      dynamic_function func = (dynamic_function) This->symbol;
   
      UNUSED_PARAMETER(jsecontext);
      assert( DYNA_MAX_PARAM - DYNA_MIN_PARAM == 12 ); /* If this fails then the code below must be changed */
   
      return (*func)( This->stack[0], This->stack[1], This->stack[2], This->stack[3], This->stack[4], This->stack[5],
                       This->stack[6], This->stack[7], This->stack[8], This->stack[9], This->stack[10], This->stack[11] );

#  elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)

      UNUSED_PARAMETER(jsecontext);
      return NTDLL32CALL(This->symbol,(This->flags & PascalDecl) ? 1 : 0, (This->flags & CDecl) ? 1 : 0, This->add_offset, This->stack );

#  elif defined(__JSE_WIN16__)

      UNUSED_PARAMETER(jsecontext);
      return WindowsDLLCall(This->symbol,(This->flags & CDecl) ? 1 : 0, This->add_offset, This->stack );

#  elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)

#     if defined(__JSE_OS2TEXT__) && defined(JSE_OS2_PMDYNAMICLINK)
         struct PMDynamicLibrary *PMdll = CONTEXT_DYNAMIC_LIBRARY->PMdll;
#     endif
      slong result;
      if ( This->flags & BitSize16 )
      {
#        if defined(__JSE_OS2TEXT__) && defined(JSE_OS2_PMDYNAMICLINK)
         if( This->flags & PMCall )
         {
            result = pmdynamiclibraryOS2DLL16PMCall(PMdll,jsecontext,This->symbol,(This->flags & PascalDecl) ? 1 : 0,(This->flags & CDecl) ? 1 : 0,
                                           (uword16)This->add_offset,This->stack);
         }
         else
#     endif
         {
            result = OS2DLL16Call(This->symbol,(This->flags & PascalDecl) ? 1 : 0,(This->flags & CDecl) ? 1 : 0,(uword16)This->add_offset,
                                  This->stack);
         }
      }
      else
      {
#        if defined(__JSE_OS2TEXT__) && defined(JSE_OS2_PMDYNAMICLINK)
            if ( This->flags & PMCall )
            {
               result = pmdynamiclibraryOS2DLL32PMCall(PMdll,jsecontext,This->symbol,(This->flags & PascalDecl) ? 1 : 0,(This->flags & CDecl) ? 1 : 0,This->add_offset,
                                              (sword32 *) This->stack);
            }
            else
#        endif
            {
               result = OS2DLL32Call(This->symbol,(This->flags & PascalDecl) ? 1 : 0,(This->flags & CDecl) ? 1 : 0,This->add_offset,
                                     (sword32 *) This->stack);
            }
      }
      return result;

#  else

#    error Dont know how to do dynamic call on this OS

#  endif
}

#else
   ALLOW_EMPTY_FILE
#endif
