/* sedyna.c
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

#    if defined(JSE_ECMA_BUFFER)
static jsebool NEAR_CALL
isBufferObject(jseContext jsecontext, jseVariable var )
{
   jseVariable temp;

   if( jseTypeObject != jseGetType(jsecontext,var))
      return False;

   temp = jseGetMember(jsecontext,var,UNISTR("class"));
   if ( temp == NULL )
      return False;
   if ( jseTypeString != jseGetType(jsecontext,temp))
      return False;
   if ( strcmp_jsechar(UNISTR("Buffer"),jseGetString(jsecontext,temp,NULL)) != 0 )
      return False;
   temp = jseGetMember(jsecontext,var,UNISTR("data"));
   if ( temp == NULL )
      return False;
   if ( jseTypeBuffer != jseGetType(jsecontext,temp))
      return False;

   return True;
}
#    endif /* JSE_ECMA_BUFFER */

static slong NEAR_CALL
CallDynamicFunction(jseContext jsecontext, DYNA_SYMBOL symbol, uint first_parameter, ulong flags)
{
   uint parameter_count = jseFuncVarCount(jsecontext) - first_parameter;
   uint i, parameter_offset;
   jsebool Success = True;
   slong result;
#  if defined(JSE_OS2_PMDYNAMICLINK)
      struct PMDynamicLibrary *PMdll = CONTEXT_DYNAMIC_LIBRARY->PMdll;
#  endif

   struct DynamicCall * dynaCall = dynamiccallNew( symbol, parameter_count, flags );

   ulong * BufferLengths = jseMustMalloc(ulong,(1+parameter_count)*sizeof(ulong));
   void * data;

   for( i = parameter_offset = 0; i < parameter_count && Success; i++, parameter_offset++ )
   {
      jseVariable v = jseFuncVar(jsecontext,i+first_parameter);
      if ( NULL == v )
      {
         Success = False;
      }
      else
      {

         jseDataType vType = jseGetType(jsecontext,v);
         if( jseTypeBuffer == vType || jseTypeString == vType )
         {
            data = (void *) (jseTypeBuffer == vType)
               ? jseGetWriteableBuffer(jsecontext,v,&(BufferLengths[i]))
               : jseGetWriteableString(jsecontext,v,&(BufferLengths[i]));
#    if defined(__JSE_OS2_TEXT__)
            if( flags & PMCall )
               data = AllocCEnvi2PM(jsecontext,data,jseGetArrayLength(jsecontext,v,NULL));
#    endif
            dynamiccallAdd(dynaCall,data,DataType32);
         }
         else if ( jseTypeUndefined == vType )
         {
#    if defined(__JSE_OS2_TEXT__)
            if( flags & PMCall )
            {
               data = AllocCEnvi2PM(jsecontext,NULL,sizeof(void *));
            }
            else
#    endif
            {
               data = (void *) jseMustMalloc(void,sizeof(void *));
            }
            dynamiccallAdd(dynaCall,data,DataType32);
         }
         else if ( jseTypeObject == vType )
         {
#    ifdef JSE_ECMA_BUFFER /* Check for a 'Buffer' object */
            if( isBufferObject(jsecontext,v))
            {
               jseVariable dataVar = jseGetMember(jsecontext,v,UNISTR("data"));
               assert( dataVar != NULL );
               data = (void *) jseGetWriteableBuffer(jsecontext,dataVar,&(BufferLengths[i]));
               dynamiccallAdd(dynaCall,data,DataType32);
            }
            else
#    endif
            {
               jseVariable DataVar = jseFuncVar(jsecontext, i + first_parameter + 1 );
               ulong datalen;
               if( NULL == DataVar || !blobDataTypeLen(jsecontext,v,&datalen))
               {
                  Success = False;
                  i--; /* We don't want to try to access this again in cleanup */
               }
               else
               {
                  i++;
#    if defined(__JSE_WIN16__)
                  if ( HUGE_MEMORY <= datalen )
                  {
                     data = (void *) HugeMalloc(datalen);
                  }
                  else
#    elif defined(__JSE_OS2TEXT__) && defined(JSE_OS2_PMDYNAMICLINK)
                  if ( flags & PMCall )
                  {
                     data = pmdynamiclibraryAllocCEnvi2PM(PMdll,jsecontext,NULL,datalen);
                  }
                  else
#    endif
                  {
                     data = jseMustMalloc(void,(uint)datalen);
                  }

                  if( !blobPut(jsecontext,data,datalen,v,DataVar,blobBigEndianMode(jsecontext)))
                  {
                     Success = False;
                     jseMustFree(data);
                  }
                  else
                  {
                     dynamiccallAdd(dynaCall,data,DataType32);
                  }
               }
            }
         }
         else if ( jseTypeNumber == vType || jseTypeBoolean == vType )
         {
            slong num = jseGetLong(jsecontext,v);
#    if defined(__JSE_WIN16__)
            dynamiccallAdd(dynaCall,(void*)num,DataType16);
#    else
            dynamiccallAdd(dynaCall,(void*)num,DataType32);
#    endif
         }
         else if ( jseTypeNull == vType )
         {
            dynamiccallAdd(dynaCall,(void *)0, DataType32 );
         }
         else
         {
            jseLibErrorPrintf(jsecontext,textlibGet(textlibDYNA_BAD_PARAMETER),i+first_parameter);
            Success = False;
         }
      }
   }

   if ( !Success )
   {
      assert( i <= parameter_count );
      parameter_count = i;
   }
   else
   {
      assert( i == parameter_count );
      result = dynamiccallCall(dynaCall,jsecontext);
   }

   /* Now we must clean up our mess */
   for( i = 0; i< parameter_count; i++ )
   {
      jseVariable v;
      jseDataType vType;

      v = jseFuncVar(jsecontext,i+first_parameter);
      assert( v != NULL );
      vType = jseGetType(jsecontext,v);

      if ( jseTypeBuffer == vType || jseTypeString == vType )
      {
         data = dynamiccallGet(dynaCall,DataType32);

         if ( NULL != data )
         {
            if ( jseTypeBuffer == vType )
               jsePutBuffer(jsecontext,v,data,BufferLengths[i]);
            else
               jsePutStringLength(jsecontext,v,(jsechar *)data,BufferLengths[i]);

#    if defined(__JSE_OS2_TEXT__)
            if ( flags & PMCall )
               FreeCEnvi2PM(jsecontext,data);
#    endif
         }
      }
      else if ( jseTypeUndefined == vType )
      {
         data = dynamiccallGet(dynaCall,DataType32);
         if ( NULL != data )
         {
            jseConvert(jsecontext,v,jseTypeNumber);
            jsePutLong(jsecontext,v,*((slong *)(data)));
#    if defined(__JSE_OS2_TEXT__)
            if ( flags & PMCall )
               FreeCEnvi2PM(jsecontext,data);
            else
#    endif
               jseMustFree(data);
         }
      }
      else if ( jseTypeObject == vType )
      {
         data = dynamiccallGet(dynaCall,DataType32);
         if( NULL != data )
         {
#    ifdef JSE_ECMA_BUFFER /* Check for a 'Buffer' object */
            if( isBufferObject(jsecontext,v))
            {
               jsePutBuffer(jsecontext,v,data,BufferLengths[i]);
            }
            else
#    endif
            {
               ulong datalen;
#    ifndef NDEBUG
               jsebool temp =
#    endif
                  blobDataTypeLen(jsecontext,v,&datalen);
               assert(temp);
               blobGet(jsecontext,jseFuncVar(jsecontext,(i+1)+first_parameter),(ubyte _HUGE_ *) data,
                       v,blobBigEndianMode(jsecontext));
#    if defined(__JSE_WIN16__)
               if ( HUGE_MEMORY <= datalen )
               {
                  HugeFree(data);
               }
               else
#    elif defined(__JSE_OS2TEXT__) && defined(JSE_OS2_PMDYNAMICLINK)

               if ( flags & PMCall )
               {
                  pmdynamiclibraryFreeCEnvi2PM(PMdll,jsecontext,data,NULL,0);
               }
#    endif
               {
                  jseMustFree(data);
               }
               i++;
            }
         }
      }
      else if ( jseTypeNumber == vType || jseTypeBoolean == vType)
      {
#    if defined(__JSE_WIN16__)
         dynamiccallGet(dynaCall,DataType16); /* We must do a get here so the call retains the offset internally */
#    else
         dynamiccallGet(dynaCall,DataType32); /* We must do a get here so the call retains the offset internally */
#    endif
      }
      else
      {
         assert( jseTypeNull == vType );
         dynamiccallGet(dynaCall,DataType32);
      }
   }

   jseMustFree(BufferLengths);
   dynamiccallDelete(dynaCall);

   return result;
}

void
DynamicLink(jseContext jsecontext, ulong flags )
{
   ulong ordinal = 0;
   const jsechar *module_name = NULL, *symbol_name = NULL;
   DYNA_SYMBOL symbol;
   uint parameter_count = 0;
   slong result;
#  if defined(__JSE_WIN16__)
      sint returnType;
#  endif

   jseVariable ModuleNameOrFunctionPtr = jseFuncVar( jsecontext, 0 );
   if ( ModuleNameOrFunctionPtr == NULL )
      return;

#    if defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__) || \
        defined(__JSE_OS2PM__) || defined(__JSE_OS2TEXT__)
   if( jseGetType( jsecontext, ModuleNameOrFunctionPtr ) != jseTypeString )
   {
      parameter_count = 1;
      if ( !jseVarNeed(jsecontext,ModuleNameOrFunctionPtr,JSE_VN_NUMBER))
         return;
      symbol = (DYNA_SYMBOL) jseGetLong(jsecontext,ModuleNameOrFunctionPtr);
   }
   else
#    endif
   {
      jseVariable NameOrOrdinal;

      /* This is the name of a library */
      if ( !jseVarNeed(jsecontext,ModuleNameOrFunctionPtr,JSE_VN_STRING))
         return;
      module_name = jseGetString( jsecontext, ModuleNameOrFunctionPtr, NULL );
      parameter_count = 2;

      NameOrOrdinal = jseFuncVar(jsecontext,1);
      if( NameOrOrdinal == NULL )
         return;

      /* The following two don't use ordinals */
#    if defined(__JSE_MAC__) || defined(__JSE_UNIX__)
      if( !jseVarNeed(jsecontext,NameOrOrdinal,JSE_VN_STRING))
         return;
      symbol_name = jseGetString(jsecontext, NameOrOrdinal, NULL );
#    else
      if ( jseTypeString == jseGetType(jsecontext,NameOrOrdinal))
         symbol_name = jseGetString( jsecontext, NameOrOrdinal, NULL );
      else
      {
         if( !jseVarNeed(jsecontext,NameOrOrdinal,JSE_VN_NUMBER))
            return;
         ordinal = jseGetLong(jsecontext,NameOrOrdinal);
         symbol_name = NULL;
      }
#    endif

      {
         jsebool result;
         
         result = dynamiclibraryGetSymbol(CONTEXT_DYNAMIC_LIBRARY,jsecontext,
                                          module_name,symbol_name,
                                          ordinal, &symbol, flags);
         if( !result )
            return;
      }

   }

   /* For OS2, look for a bit size variable */
#  if defined(__JSE_OS2PM__) || defined(__JSE_OS2TEXT__)
   {
      jseVariable BitSizeVar;
      uint bitSize;
      
      JSE_FUNC_VAR_NEED(BitSizeVar,jsecontext,parameter_count++,JSE_VN_NUMBER);
      bitSize = jseGetLong(jsecontext,BitSizeVar);
      if( bitSize != BitSize16 && bitSize != BitSize32 )
      {
         jseLibErrorPrintf(jsecontext,textlibGet(textlibDYNA_INVALID_BIT_SIZE));
         return;
      }
      flags |= bitSize;
   }
#  endif

   /* For win16, look for a return type variable */
#  if defined(__JSE_WIN16__)
   {
      jseVariable ReturnTypeVar;

      ReturnTypeVar = jseFuncVar(jsecontext,parameter_count++);
      if ( jseTypeObject != jseGetType(jsecontext,ReturnTypeVar)
        || !isBlobType(jsecontext,ReturnTypeVar) )
      {
         jseLibErrorPrintf(jsecontext,textlibGet(textlibDYNA_INVALID_DYNA_RETURN_TYPE));
         return;
      }
      returnType = (sint)jseGetLong(jsecontext,
                            jseGetMember(jsecontext,ReturnTypeVar,blobInternalDataMember));
   }
#  endif

   /* For flavors of Windows and OS2, look for a calling convention variable */
#    if defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || \
        defined(__JSE_CON32__) || defined(__JSE_OS2PM__) || defined(__JSE_OS2TEXT__)
   {
      jseVariable CallConventionVar;
      ulong callingConvention;

      JSE_FUNC_VAR_NEED(CallConventionVar,jsecontext,parameter_count++, JSE_VN_NUMBER);
      callingConvention = (ulong) jseGetLong(jsecontext,CallConventionVar);
      if( callingConvention != CDecl && callingConvention != PascalDecl
          #      if !defined(__JSE_WIN16__)
          && callingConvention != StdDecl
          #      endif
        )
      {
         jseLibErrorPrintf(jsecontext,
                           textlibGet(textlibDYNA_INVALID_CALLING_CONVENTION));
         return;
      }
      flags |= callingConvention;
   }
#    endif

   result = CallDynamicFunction( jsecontext, symbol, parameter_count, flags );

#  if defined(__JSE_WIN16__)
      if ( blobSWORD16 == returnType  ||  blobUWORD16 == returnType )
      {
         result &= 0x0FFFFL;
         if ( blobSWORD16 == returnType )
            result = (slong)((sword16)result);
      }
#  endif

   jseReturnLong( jsecontext, result );
}

   static void
InitializeLibrary_DynamicLink( jseContext jsecontext )
{
   static CONST_DATA(jsechar) KludgeName[] = UNISTR("** DynamicLink Kludge variable **");
   /* Because this can be called from more than one library */
   if( jseGetMember(jsecontext,NULL,KludgeName) == NULL )
   {
      jseMember(jsecontext,NULL,KludgeName,jseTypeNull);
      
#  if defined(__JSE_WIN32__) || defined(__JSE_CON32__) || defined(__JSE_WIN16__) \
   || defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
      jsePreDefineLong(jsecontext,UNISTR("CDECL"),CDecl);
      jsePreDefineLong(jsecontext,UNISTR("PASCAL"),PascalDecl);
      jsePreDefineLong(jsecontext,UNISTR("STDCALL"),StdDecl);
#  endif

#  if defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
      jsePreDefineLong(jsecontext,UNISTR("BIT16"),BitSize16);
      jsePreDefineLong(jsecontext,UNISTR("BIT32"),BitSize32);
#  endif
   }
}

   static void JSE_CFUNC FAR_CALL
UnloadLibrary_DynamicLink(jseContext jsecontext,void *param)
{
   struct DynamicLibrary _FAR_ * dl = (struct DynamicLibrary _FAR_ *) param;
#  if defined(JSE_OS2_PMDYNAMICLINK)
      assert( jsecontext == dl->PMdll->pConstructorContext );
#  endif
   dynamiclibraryUnloadModules(dl,jsecontext);
   dynamiclibraryDelete(dl);
}

   jsebool 
LoadLibrary_DynamicLink( jseContext jsecontext )
{
   struct DynamicLibrary * This = jseGetSharedData(jsecontext,DYNAMIC_LIBRARY_NAME);
   
   if( This == NULL )
   {
      This = (void *)dynamiclibraryNew(jsecontext);
      jseSetSharedData(jsecontext,DYNAMIC_LIBRARY_NAME,This,UnloadLibrary_DynamicLink);
      InitializeLibrary_DynamicLink(jsecontext);
   }
   
   return ( This != NULL );
}

#else
   ALLOW_EMPTY_FILE
#endif
