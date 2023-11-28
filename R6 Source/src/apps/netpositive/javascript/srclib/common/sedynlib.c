/* sedynlib.c
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

#ifdef __JSE_MAC__
#  include <TextUtils.h>
#endif

#if defined(JSE_SELIB_DYNAMICLINK) || defined(JSE_OS2_PMDYNAMICLINK)

#  if defined(__JSE_UNIX__)
static CONST_DATA(jsechar *) module_type = UNISTR("shared object");
#  elif defined(__JSE_MAC__)
static CONST_DATA(jsechar *) module_type = UNISTR("shared library");
#  elif defined(__JSE_WIN32__) || defined(__JSE_CON32__) || \
      defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) || \
      defined(__JSE_WIN16__)
static CONST_DATA(jsechar *) module_type = UNISTR("dll module");
#  elif
#    error Unknown module type for this OS
#  endif

#  if defined(__JSE_UNIX__) || defined(__JSE_MAC__)
static CONST_DATA(jsechar *) sym_name_type = UNISTR("symbol");
static CONST_DATA(jsechar *) sym_ordinal_type = NULL;
#  elif defined(__JSE_WIN32__) || defined(__JSE_CON32__) || \
      defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) || \
      defined(__JSE_WIN16__)
static CONST_DATA(jsechar *) sym_name_type = UNISTR("named procedure");
static CONST_DATA(jsechar *) sym_ordinal_type = UNISTR("ordinal procedure");
#  elif
#    error Unknown symbol type for this OS
#  endif

static jsebool NEAR_CALL
dynamiclibraryOpenModule( struct DynamicLibrary *This, jseContext jsecontext,
                          const jsechar * module_name,
                          DYNA_MODULE *module, slong *err_code, ulong flags )
{

#  if defined(__JSE_UNIX__)
      jsechar * Filename;

#     ifdef __hpux__
         const jsechar * Extensions[1] = { UNISTR(".sl") };
#     else
         const jsechar * Extensions[1] = { UNISTR(".so") };
#     endif

      const jsechar * libpath = GetEnvironment(UNISTR("LD_ELF_LIBRARY_PATH"));
      
      UNUSED_PARAMETER(This);
      UNUSED_PARAMETER(flags);
      UNUSED_PARAMETER(err_code);

      if ( libpath == NULL )
         libpath = GetEnvironment(UNISTR("LD_LIBRARY_PATH"));
      if ( NULL == (Filename = FindFile(module_name,libpath,1,Extensions)) &&
           NULL == (Filename = FindFile(module_name,UNISTR("/usr/lib"),1,Extensions)) &&
           NULL == (Filename = FindFile(module_name,UNISTR("/lib"),1,Extensions)))
         return False;

#     if defined(__FreeBSD__)
         *module = dlopen(Filename,1);
#     elif defined(__hpux__)
         *module = shl_load(Filename,BIND_DEFERRED,0);
#     else
         *module = dlopen(Filename,RTLD_LAZY);
#     endif

      jseMustFree(Filename);
      if( NULL == *module )
         return False;
      else
         return True;

#  elif defined (__JSE_MAC__)

      Ptr mainAddr;
      unsigned char errName[255];
      jsechar fragName[63];
   
      UNUSED_PARAMETER(This);
      UNUSED_PARAMETER( jsecontext );
      UNUSED_PARAMETER( flags );

      if (strlen(module_name) >= 63 )
         strncpy(fragName,module_name,62);
      strcpy(fragName,module_name);
      c2pstr(fragName);
      *err_code = (slong) GetSharedLibrary( (unsigned char *)fragName, kPowerPCCFragArch,
                                           kLoadCFrag, module, &mainAddr, errName );
      if ( 0 != (*err_code))
         return False;
      else
         return True;

#  elif defined(__JSE_WIN16__)

      UNUSED_PARAMETER( jsecontext );
      UNUSED_PARAMETER( flags );

      *module = LoadLibrary(module_name);
      if((WORD)(*module) < (WORD) 32 )
      {
         *err_code = (slong)(*module);
         return False;
      }
      else
         return True;

#  elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)

      UNUSED_PARAMETER( jsecontext );
      UNUSED_PARAMETER( flags );

      *module = LoadLibrary(module_name);
      if ( NULL == *module )
      {
         *err_code = (slong) GetLastError();
         return False;
      }
      else
         return True;

#  elif defined(__JSE_OS2PM__) || defined (__JSE_OS2TEXT__)

#     if defined(__JSE_OS2PM__)
         UNUSED_PARAMETER(jsecontext);
#        ifdef NDEBUG
            UNUSED_PARAMETER(flags);
#        endif
         assert( !(flags & PMCall));
#     endif

#     if defined(__JSE_OS2TEXT__) && defined(JSE_OS2_PMDYNAMICLINK)
         if ( !(flags & PMCall))
         {
            struct PMDynamicLibrary *PMdll = CONTEXT_DYNAMIC_LIBRARY->PMdll;
#           define getPMdll PMdll->SharedData->data.GetDLLModuleHandle
            strcpy(getPMdll.ModuleName,module_name);
            pmdynamiclibraryCommunicateWithCEnvi2PM(PMdll,jsecontext,eGetDLLModuleHandle);
            *module = getPMdll.ModuleHandle;
            *err_code = getPMdll.rc;
         }
         else
#     endif
         {
            jsechar LoadError[100];
            *err_code = (slong) DosLoadModule(LoadError,sizeof(LoadError)/sizeof(LoadError[0]),
                                              (char *)module_name,module);
         }

      if ( NO_ERROR != *err_code )
         return False;
      else
         return True;

#  else

#     error Dont know how to open dynamic module on this OS

#  endif
}

static void NEAR_CALL
dynamiclibraryCloseModule(struct DynamicLibrary *This,jseContext jsecontext, DYNA_MODULE *module, ulong flags)
{
#  if defined(__JSE_UNIX__)
      UNUSED_PARAMETER(jsecontext);
      UNUSED_PARAMETER(flags);
#     ifdef __hpux__
         shl_unload(*module);
#     else
         dlclose(*module);
#     endif
#  elif defined(__JSE_MAC__)
      UNUSED_PARAMETER(jsecontext);
      UNUSED_PARAMETER(flags);
      CloseConnection( module );
#  elif defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)
      UNUSED_PARAMETER(jsecontext);
      UNUSED_PARAMETER(flags);
      FreeLibrary( *module );
#  elif defined(__JSE_OS2PM__) || defined(__JSE_OS2TEXT__)
#     if defined(__JSE_OS2TEXT__) && defined(JSE_OS2_PMDYNAMICLINK)
         if ( flags & PMCall )
         {
            struct PMDynamicLibrary *PMdll = CONTEXT_DYNAMIC_LIBRARY->PMdll;
            PMdll->SharedData->data.FreeDLLModuleHandle.ModuleHandle = *module;
            pmdynamiclibraryCommunicateWithCEnvi2PM(PMdll,jsecontext,eFreeDLLModuleHandle);
         }
         else
#     else
         UNUSED_PARAMETER(jsecontext);
         UNUSED_PARAMETER(flags);
#     endif
         {
            DosFreeModule(*module);
         }
#  else
#     error Dont know how to close dynamic module on this OS
#  endif
   UNUSED_PARAMETER(This);
}

   static jsebool NEAR_CALL
dynamiclibraryFindSymbol(struct DynamicLibrary *This,jseContext jsecontext, DYNA_MODULE *module,
                      const jsechar * sym_name, ulong ordinal,
                      DYNA_SYMBOL *symbol, slong *err_code, ulong flags)
{
#  if defined(__JSE_UNIX__)

      UNUSED_PARAMETER(This);
      UNUSED_PARAMETER(jsecontext);
      UNUSED_PARAMETER(ordinal);
      UNUSED_PARAMETER(err_code);
      UNUSED_PARAMETER(flags);

#     ifdef __hpux__
         *symbol = shl_findsym(*module,sym_name,TYPE_UNDEFINED,0);
#     else
         *symbol = dlsym(*module,sym_name);
#     endif

      if ( *symbol == NULL )
         return False;
      else
         return True;

#  elif defined(__JSE_MAC__)

      jsechar symName[255];
      CFragSymbolClass sym_class;

      UNUSED_PARAMETER(This);
      UNUSED_PARAMETER(jsecontext);
      UNUSED_PARAMETER(ordinal);
      UNUSED_PARAMETER(flags);

      strcpy( symName, sym_name );
      c2pstr( symName );
      *err_code = (slong) FindSymbol(*module, (unsigned char *) symName, symbol, &sym_class);
      if ( noErr != *err_code )
         return False;
      else
         return True;

#  elif defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)

      UNUSED_PARAMETER(jsecontext);
      UNUSED_PARAMETER(flags);
      UNUSED_PARAMETER(err_code);

      *symbol = GetProcAddress(*module, NULL == sym_name ? (LPCSTR) ordinal : sym_name);
      if ( *symbol == NULL )
         return False;
      else
         return True;

#  elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM)

#     if defined(__JSE_OS2PM__)
         UNUSED_PARAMETER(jsecontext);
#        ifdef NDEBUG
            UNUSED_PARAMETER(flags)
#        endif
         assert( !(flags & PMCall));
#     elif defined(JSE_OS2_PMDYNAMICLINK)
         if ( flags & PMCall )
         {
            if ( !pmdynamiclibraryInitializeCEnvi2PM(CONTEXT_DYNAMIC_LIBRARY->PMdll,jsecontext))
            {
               *err_code = ERROR_PROC_NOT_FOUND;
               return False;
            }
         }
#     endif

#     if defined(__JSE_OS2TEXT__) && defined(JSE_OS2_PMDYNAMICLINK)
         if ( flags & PMCall )
         {
            struct PMDynamicLibrary *PMdll = CONTEXT_DYNAMIC_LIBRARY->PMdll;
#           define DQPA PMdll->SharedData->data.DosQueryProcAddr
            DQPA.ModuleHandle = *module;
            DQPA.ordinal = ordinal;
            if ( NULL == sym_name )
               DQPA.ProcName = NULL;
            else
               strcpy(DQPA.ProcName = DQPA.ProcedureNameBuffer,sym_name);
            pmdynamiclibraryCommunicateWithCEnvi2PM(PMdll,jsecontext,eDosQueryProcAddr);
            *err_code = DQPA.rc;
            *symbol = DQPA.pfn;
         }
         else
#     endif
         {
            *err_code = DosQueryProcAddr(*module,ordinal,(char *)sym_name,symbol);
         }
      if ( NO_ERROR == *err_code )
         return True;
      else
         return False;

#  else

#     error Dont know how to get symbol address on this OS

#  endif
}

   jsebool
dynamiclibraryGetModule(struct DynamicLibrary *This,jseContext jsecontext, const jsechar * module_name,
                        DYNA_MODULE *module, ulong flags )
{
   struct open_module * current;

   assert( module_name != NULL );
   current = This->OpenModules;

   for( ; current != NULL; current = current->prev )
   {
      if ( !stricmp_jsechar(current->name, module_name))
         break;
   }

   if ( current != NULL )
      *module = current->module;
   else
   {
      slong err_code = 0;
      if ( !dynamiclibraryOpenModule(This,jsecontext,module_name, module, &err_code, flags))
      {
         jseLibErrorPrintf(jsecontext,textlibGet(textlibDYNA_CANNOT_LOAD_MODULE),
                           module_type, module_name, &err_code );
         return False;
      }
      else
      {
         current = jseMustMalloc( struct open_module, sizeof(struct open_module));
         current->name = StrCpyMalloc(module_name);
         current->module = *module;
         current->flags = flags;
         current->prev = This->OpenModules;
         This->OpenModules = current;
      }
   }

   return True;
}

   void
dynamiclibraryUnloadModules(struct DynamicLibrary *This,jseContext jsecontext)
{
   struct open_module * current;

   while( This->OpenModules != NULL )
   {
      current = This->OpenModules;
      This->OpenModules = current->prev;
      dynamiclibraryCloseModule(This,jsecontext, &(current->module), current->flags);
      jseMustFree(current->name);
      jseMustFree(current);
   }
}

   jsebool
dynamiclibraryGetSymbol(struct DynamicLibrary *This,jseContext jsecontext, const jsechar * module_name,
                        const jsechar * symbol_name, ulong ordinal, DYNA_SYMBOL *sym,
                        ulong flags)
{
   DYNA_MODULE module;
   long err_code;

   assert( module_name != NULL );

   if ( !dynamiclibraryGetModule(This,jsecontext,module_name,&module,flags))
      return False;

   if ( !dynamiclibraryFindSymbol(This,jsecontext,&module,symbol_name,ordinal,sym,&err_code,flags))
   {
      if ( NULL == symbol_name )
      {
         assert( NULL != sym_ordinal_type );
         jseLibErrorPrintf(jsecontext,textlibGet(textlibDYNA_CANNOT_FIND_ORDINAL_SYM),
                           sym_ordinal_type,ordinal,module_type,
                           module_name,&err_code);
      }
      else
         jseLibErrorPrintf(jsecontext,textlibGet(textlibDYNA_CANNOT_FIND_NAMED_SYM),
                           sym_name_type,symbol_name,module_type,
                           module_name,&err_code);
      return False;
   }

   return True;
}


struct DynamicLibrary * dynamiclibraryNew(jseContext InitialContext)
{
   struct DynamicLibrary *This;
   This = jseMustMalloc(struct DynamicLibrary,sizeof(struct DynamicLibrary));
   This->OpenModules = NULL;
#  if defined(JSE_OS2_PMDYNAMICLINK)
      This->PMdll = pmdynamiclibraryNew(InitialContext);
#  else
      UNUSED_PARAMETER(InitialContext);
#  endif
   return This;
}

void dynamiclibraryDelete(struct DynamicLibrary *This)
{
#  if defined(JSE_OS2_PMDYNAMICLINK)
      pmdynamiclibraryDelete(This->PMdll);
#  endif
   jseMustFree(This);
}

#  if defined(JSE_OS2_PMDYNAMICLINK)
struct PMDynamicLibrary * NEAR_CALL
pmdynamiclibraryNew(jseContext InitialContext)
{
   struct PMDynamicLibrary * This = jseMustMalloc(struct PMDynamicLibrary,sizeof(struct PMDynamicLibrary));
   This->pConstructorContext = InitialContext;
#    ifdef __JSE_OS2TEXT__
   This->CEnvi2PMIsRunning = False;
#    endif
   return This;
}

void NEAR_CALL
pmdynamiclibraryDelete(struct PMDynamicLibrary *This)
{
#  if defined(__JSE_OS2TEXT__)
      pmdynamiclibraryUnloadCEnvi2PM(This,This->pConstructorContext);
#  endif
   jseMustFree(This);
}
#  endif /* defined(JSE_OS2_PMDYNAMICLINK) */


#else
   ALLOW_EMPTY_FILE
#endif
