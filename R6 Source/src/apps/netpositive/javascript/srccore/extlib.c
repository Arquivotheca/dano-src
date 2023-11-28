/* extlib.c  External Library Link (ScriptEase Plug-ins) code
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

#if defined(__hpux__)
  #include <dl.h>
#elif defined(_AIX)
  #include <aixdlfcn.h>
#elif defined(__JSE_UNIX__) && !defined(__JSE_BEOS__) // seb 98.11.12
  #include <dlfcn.h>
#endif

#ifdef __JSE_NWNLM__
  #include <process.h>
  #include <advanced.h>
#endif

#if defined(JSE_LINK) && (0!=JSE_LINK)  &&  !defined(__DJGPP__)

#if !defined(__JSE_WIN16__) && !defined(__JSE_WIN32__) && !defined(__JSE_CON32__) && \
    !defined(__JSE_OS2TEXT__) && !defined(__JSE_OS2PM__) && !defined(__JSE_UNIX__) && \
    !defined(__JSE_MAC__) && !defined(__JSE_NWNLM__)
  #error Extensions are not yet supported on this platform
#endif

#include "selink.h"

#define ARRAYLENGTH(array)    (sizeof(array)/sizeof(array[0]))

#if defined(__JSE_MAC__)
  #define JSEEXTERNALLIBEXT        UNISTR(".cfm")
#endif


static CONST_DATA(struct jseFuncTable_t) gjseFuncTable = {
  /* When adding functions to this list, don't forget to
     increment the EXTERNALVER #define in seclib\extnsn.h
  */
   #if defined(__JSE_WIN16__)
      0,                   /* uword16 DS; */
   #endif
   sizeof(gjseFuncTable),  /* ulong TableSize; */
   JSEEXTERNALVER          /* uulong Version; */
/*START AUTOMATED .DEF FILE CREATION*/
/* warning, for any of the following lines to be found they must be read with ",jse" at
 * exactly the fourth column
 */
   ,jseCreateVariable
   ,jseCreateSiblingVariable
   ,jseCreateConvertedVariable
   ,jseCreateLongVariable
#if defined(JSE_CREATEFUNCTIONTEXTVARIABLE) && (0!=JSE_CREATEFUNCTIONTEXTVARIABLE)
   ,jseCreateFunctionTextVariable
#else
   ,NULL
#endif
   ,jseDestroyVariable
   ,jseGetArrayLength
   ,jseSetArrayLength
   ,jseSetAttributes
   ,jseGetAttributes
   ,jseGetType
   ,jseConvert
   ,jseAssign
   ,jseGetByte
   ,jsePutByte
   ,jseGetLong
   ,jsePutLong
   ,jseGetString
   ,jseGetBuffer
   ,jseGetWriteableString
   ,jseGetWriteableBuffer
   ,jsePutString
   ,jsePutStringLength
   ,jsePutBuffer
   ,jseCopyString
   ,jseCopyBuffer
   ,jseEvaluateBoolean
   ,jseCompare
   ,jseMemberEx
   ,jseIndexMemberEx
   ,jseGetNextMember
   ,jseDeleteMember
   ,jseGlobalObject
#if 0 != JSE_MULTIPLE_GLOBAL
   ,jseSetGlobalObject
#else
   ,NULL
#endif
   ,jseActivationObject
   ,jseGetCurrentThisVariable
   ,jseCreateStack
   ,jseDestroyStack
   ,jsePush
   ,jseFuncVarCount
   ,jseFuncVar
   ,jseFuncVarNeed
   ,jseVarNeed
   ,jseReturnVar
   ,jseReturnLong
   ,jseLibraryData
   ,jseGetFunction
   ,jseIsFunction
   ,jseCallFunction
   ,jsePreviousContext
   ,jseCurrentContext
   ,jseIsLibraryFunction
   ,jseCreateWrapperFunction
   ,jseMemberWrapperFunction
#if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
   ,jsePreDefineLong
   ,jsePreDefineString
# if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   ,jsePreDefineNumber
# else
   ,NULL
# endif
#else
   ,NULL
   ,NULL
   ,NULL
#  endif
   ,jseCallAtExit
   ,jseLibSetErrorFlag
   ,jseLibErrorPrintf
   ,jseLibSetExitFlag
   ,jseQuitFlagged
   ,jseInterpret
   ,jseAddLibrary
   ,jseLocateSource
   ,jseCurrentFunctionName
#if defined(JSE_GETFILENAMELIST) && (0!=JSE_GETFILENAMELIST)
   ,jseGetFileNameList
#else
   ,NULL
#endif
   ,jseInitializeExternalLink
   ,jseTerminateExternalLink
   ,jseGetLinkData
   ,jseGetExternalLinkParameters
   ,jseAppExternalLinkRequest
#if defined(JSE_BREAKPOINT_TEST) && (0!=JSE_BREAKPOINT_TEST)
   ,jseBreakpointTest
#else
   ,NULL
#endif
#if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
   ,jseTellSecurity
#else
   ,NULL
#endif
#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   ,jseCreateCodeTokenBuffer
#else
   ,NULL
#endif
   ,jsePutNumber
   ,jseReturnNumber
#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   ,jseGetFloatIndirect
#else
   ,NULL
#endif
#if ( 0 < JSE_API_ASSERTLEVEL )
   ,jseGetLastApiError
   ,jseClearApiError
#else
   ,NULL
   ,NULL
#endif
   ,jseFindVariable
   ,jseGetVariableName
   ,jseInterpInit
   ,jseInterpTerm
   ,jseInterpExec
   ,jseGetBoolean
   ,jsePutBoolean
/*END AUTOMATED .DEF FILE CREATION*/
};


struct ExtensionLibrary *extensionNew(struct Call *call,struct ExtensionLibrary *Parent)
{
   jsebool success = False;
   struct ExtensionLibrary *this =
      jseMalloc(struct ExtensionLibrary,sizeof(struct ExtensionLibrary));
   
   if( this != NULL )
   {
      this->RecentLinkedExtensionLib = NULL;
      if ( NULL != (this->MyParent = Parent) ||
         extensionBuildjseFuncTable(this,call) )
           success = True;
   }
   
   if( !success )
   {
      FreeIfNotNull(this);
      this = NULL;
   }

   return this;
}


void extensionDelete(struct ExtensionLibrary *this,struct Call *call)
{
   assert( NULL != call->Global->jseFuncTable );
   extensionFreeAllLibs(this);
   if ( NULL == this->MyParent )
   {
      jseMustFree(call->Global->jseFuncTable);
   }

   jseMustFree(this);
}


   jsebool
extensionLink(struct Source **source,struct Call *call)
{
   return extensionLinkOnto(call->session.ExtensionLib,source,call);
}


   int NEAR_CALL
extensionLinkOnto(struct ExtensionLibrary *this,struct Source **source,
                  struct Call *call)
{
  /* parse jse source string for the extension name
   * ex. #link 'GifExt.dll'
   */
   jsechar *End;
   jsechar QuoteChar, EndQuoteChar;
   jsechar *src = sourceGetPtr(*source);
   jsechar *FullFileName;
   jsebool success;


   /* next character had better be a quote character */
   if ( (QuoteChar = '\"') != *src
     && (QuoteChar = '\'') != *src
     && (QuoteChar = '<') != *src )
   {
      callError(call,textcoreMISSING_INCLINK_NAME_QUOTE,'<',textcoreExtLinkDirective);
      return False;
   }
   EndQuoteChar = (jsechar) (('<' == QuoteChar) ? '>' : QuoteChar);
   if ( NULL == (End = strchr_jsechar(++src,EndQuoteChar)) )
   {
      callError(call,textcoreMISSING_INCLINK_NAME_QUOTE,EndQuoteChar,textcoreExtLinkDirective);
      return False;
   }
   *End = 0;
   sourceSetPtr(*source,End+1);

   /* allocate memory for file name plus a little just-in-case */
   FullFileName = StrCpyMallocLen(src,strlen_jsechar(src)+10);

   success = extensionLibraryStartup(this,call,FullFileName);

   jseMustFree(FullFileName);

   return success;
}


   jsechar * NEAR_CALL
extensionFindLibName(struct ExtensionLibrary *this,const jsechar *FileSpec)
{
   struct ExtensionLibrary *extlib = this;
   struct LinkedExtensionLib *linkf;


   assert( NULL != FileSpec  &&  0 != *FileSpec );
   do {
      assert( NULL != extlib );
      for( linkf = extlib->RecentLinkedExtensionLib; NULL != linkf; linkf = linkf->Previous )
      {
        #if defined(__JSE_UNIX__)
         /* filenames on unix are case-sensitive */
         if ( !strcmp_jsechar(FileSpec,linkf->RootLibraryName) )
        #else
         if ( !stricmp_jsechar(FileSpec,linkf->RootLibraryName) )
        #endif
         {
            return linkf->RootLibraryName;
         }
      }
   } while ( NULL != (extlib = extlib->MyParent) );
   return NULL ;
}


   jsechar * NEAR_CALL
extensionAddLibName(struct ExtensionLibrary *this,
                    const jsechar *RootFileSpec,
                                     ExtensionLibHandle ExtLibHandle)
{
   struct LinkedExtensionLib *linkf =
      jseMustMalloc(struct LinkedExtensionLib,sizeof(*linkf)+sizeof(jsechar)*strlen_jsechar(RootFileSpec));

   assert( NULL == extensionFindLibName(this,RootFileSpec) );
   strcpy_jsechar(linkf->RootLibraryName,RootFileSpec);
   linkf->hExtensionLib = ExtLibHandle;
   linkf->Previous = this->RecentLinkedExtensionLib;
   this->RecentLinkedExtensionLib = linkf;
   return linkf->RootLibraryName;
}


   jsechar * NEAR_CALL
extensionFindLibPath(struct ExtensionLibrary *this,
                     const jsechar *FileSpec,struct Call *call)
{
/* FindFile allocates memory to hold this full file
 *     name/path so must remember to free later
 */
   jsechar *FileName = jseMustMalloc(jsechar, _MAX_PATH*sizeof(jsechar));
   jsebool Success = (*(call->Global->ExternalLinkParms.FileFindFunc))(call,FileSpec,FileName,
                                                                       _MAX_PATH-1,True);
   if( !Success )
   {
      FileName[0] = '\0';
   }
   return FileName;
}

/* ---------------------------------------------------------------------- */

#ifdef __JSE_NWNLM__
static void get_root(jsechar *buffer,const jsechar *filename)
{
  jsechar *tmp, *s;

  while( (tmp = strchr_jsechar(filename,':'))!=NULL ||
         (tmp = strchr_jsechar(filename,'/'))!=NULL ||
         (tmp = strchr_jsechar(filename,'\\'))!=NULL )
    {
      filename = tmp;
    }

  strcpy_jsechar(buffer,filename);
  s = strchr_jsechar(buffer,'.');
  if( s ) s[0] = '\0';
  strlwr_jsechar(buffer);
}
#endif

/* ---------------------------------------------------------------------- */

/* these 2 typedef's should be integrated?
*/
#if (defined(__JSE_WIN32__) || defined(__JSE_CON32__)) \
 && defined(_MSC_VER)
   typedef int JSEEXTNSN_API (*FP) (jseContext);
#else
   typedef int (JSEEXTNSN_API *FP) (jseContext);
#endif
#if defined(__JSE_WIN32__) || defined(__JSE_CON32__)
  #if defined(__BORLANDC__)
    typedef int __import _FAR_ (__cdecl *import_cast)(jseContext);
  #else
    typedef __declspec(dllimport) int _FAR_ (__cdecl *import_cast)(jseContext);
  #endif
#elif defined(__JSE_WIN16__)
  typedef int (_FAR_ JSE_PFUNC *import_cast)(jseContext);
#else
  typedef int (JSE_CFUNC *import_cast)(jseContext);
#endif

/* Core calls this extension function to load the extension
 * and pass it the jseContext
 */
jsebool NEAR_CALL extensionLibraryStartup(struct ExtensionLibrary *this,
                                                 struct Call *call, const jsechar * LibraryName)
{
/* Load and Open a sharable module,
 * Find and Call the extension load function,
 * Add module handle and fullpath to a linked list
 * return TRUE on success, FALSE on failure
 */
  jsebool     retval = True;
  long        ver;
  FP          fp;
  ExtensionLibHandle temphExtensionLib;
  jsechar * FullLibraryPath;

#if defined(__JSE_NWNLM__)
  jsechar before[256],all[256];
  get_root(before,LibraryName);
#endif

#if defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
  jsechar LoadError[256];
  APIRET     rc;
  PFN        ModuleAddr;
#endif


  assert(call != NULL);
  assert(LibraryName != NULL);

  if( extensionFindLibName(this,LibraryName) != NULL)
  {
     /* Extension Library Already loaded extension, don't load again */
     return True;
  }

  /* FindExtensionLibPath(FindFile) allocates memory to hold this
   * full file name/path so must remember to free later
   */
  FullLibraryPath = extensionFindLibPath(this,LibraryName, call);
#if defined(__JSE_UNIX__)
  if( FullLibraryPath==NULL || FullLibraryPath[0]=='\0' )
    {
      jsechar buffer[256];
      jse_sprintf(buffer,"lib%s",LibraryName);
      FullLibraryPath = extensionFindLibPath(this,buffer,call);
    }
#endif

  #if !defined(__JSE_MAC__)
    if(FullLibraryPath == NULL || FullLibraryPath[0]=='\0')
    {
       callError(call,textcoreLINK_LIBRARY_NOT_EXISTS,LibraryName);
       jseMustFree(FullLibraryPath);
       return False;
    }
  #endif

    /* Load the Extension Library (OP SYS specific) */
  #if defined(__JSE_WIN32__) || defined(__JSE_WIN16__) || defined(__JSE_CON32__)
    {
       temphExtensionLib = LoadLibrary(FullLibraryPath);
    }
  #elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
    {
       LoadError[0] = '\0';
       rc = DosLoadModule(LoadError,sizeof(LoadError)/sizeof(jsechar),FullLibraryPath,&temphExtensionLib);
       if (rc != 0) {
          temphExtensionLib = NULL;
       }
    }
  #elif defined(__FreeBSD__)
    temphExtensionLib = dlopen(FullLibraryPath,1);
  #elif defined(__hpux__)
    temphExtensionLib = shl_load(FullLibraryPath,BIND_DEFERRED,0);
// seb 98.11.12
  #elif defined(__JSE_BEOS__)
#warning External_libraries_not_implemented_yet  
  #elif defined(__JSE_UNIX__)
    temphExtensionLib = dlopen(FullLibraryPath,RTLD_LAZY);
  #elif defined(__JSE_MAC__)
    {
       Ptr       mainAddr;
       OSErr     myErr;
       ubyte     errName[255];
       FSSpec    libSpec;
       const char *fullpath2;
       const char *libname2;
       
       fullpath2 = UnicodeToAscii(FullLibraryPath);
       c2pstr( (char *) fullpath2 );

       if ((myErr = FSMakeFSSpec(0, 0, (ubyte *) fullpath2, &libSpec)) == noErr )
       {
           libname2 = UnicodeToAscii(LibraryName);
           c2pstr( (char *) libname2 );

           myErr = GetDiskFragment( &libSpec, 0L, kCFragGoesToEOF, (ubyte *) libname2,
                                    kLoadCFrag, &temphExtensionLib, &mainAddr, errName );

           p2cstr( (ubyte *) libname2 );

           if (myErr != noErr)
           {
              char * tmpLibName;
              
              tmpLibName = jseMustMalloc( char, (strlen_jsechar(libname2) + strlen_jsechar(JSEEXTERNALLIBEXT) + 1) *sizeof(char));
              
              strcpy( tmpLibName, libname2);
              strcat( tmpLibName, JSEEXTERNALLIBEXT);
              
              c2pstr( tmpLibName );
              myErr = GetDiskFragment( &libSpec, 0L, kCFragGoesToEOF, (ubyte *) tmpLibName,
                                       kLoadCFrag, &temphExtensionLib, &mainAddr, errName );
              
              jseMustFree( tmpLibName );
           }

           FreeAsciiString(libname2);
        }

        p2cstr((ubyte *) fullpath2);
        FreeAsciiString(fullpath2);

        if ( myErr != noErr )
        {
           temphExtensionLib = NULL;
        }
    }
  #elif defined(__JSE_NWNLM__)
    jse_sprintf(all,"%sjseExtensionVer",before);
    fp = (FP)ImportSymbol(GetNLMHandle(),all);

    /* If we can't find the symbol, try loading the library and looking again. */
    if( fp==NULL )
    {
       spawnlp(P_NOWAIT,FullLibraryPath,"",NULL);
       jse_sprintf(all,"%sjseExtensionVer",before);
       fp = (FP)ImportSymbol(GetNLMHandle(),all);
    }

    temphExtensionLib = (void *)fp;
  #else
    #error this system not done.
  #endif
    if( temphExtensionLib == NULL )
    {
       callError(call,textcoreLINK_LIBRARY_LOAD_FAILED,FullLibraryPath);
       retval = False;
    }

  /* The Extension Library has been loaded into memory, the handle is
   * stored in hExtensionLib.
   */

  /* First thing to do is check the version number of the module. */

  if( temphExtensionLib )
  {
    #if defined(__JSE_WIN16__)
      fp = (FP)GetProcAddress(temphExtensionLib, "JSEEXTENSIONVER");
    #elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)
      /* Borland style */
      fp = (FP)GetProcAddress(temphExtensionLib, "_jseExtensionVer");
      if(fp == NULL) {
        /* Watcom style */
        fp = (FP)GetProcAddress(temphExtensionLib, "jseExtensionVer_");
        if(fp == NULL) {
          /* MSVC style */
          fp = (FP)GetProcAddress(temphExtensionLib, "jseExtensionVer");
          if(fp == NULL) {
            /* PASCAL (win16) style */
            fp = (FP)GetProcAddress(temphExtensionLib, "JSEEXTENSIONVER");
          }
        }
      }
    #elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
      ModuleAddr  = 0;
      fp = NULL;
      rc = DosQueryProcAddr(temphExtensionLib,0L,"_JSEEXTENSIONVER",&ModuleAddr);
      /* if rc != 0, then there is no jseLoadExtension function in the dll */
      if (rc == 0) {
        fp = (FP)ModuleAddr;
      }
    #elif defined(__hpux__)
      if( shl_findsym(&temphExtensionLib,"jseExtensionVer",TYPE_UNDEFINED,(void *)&fp)==-1 )
        fp = NULL;
// seb 98.11.12
    #elif defined(__JSE_BEOS__)
#warning External_libraries_not_implemented_yet  
    #elif defined(__JSE_UNIX__)
      fp = (FP)dlsym(temphExtensionLib,"jseExtensionVer");
      if( fp==NULL )
        fp = (FP)dlsym(temphExtensionLib,"_jseExtensionVer");
    #elif defined(__JSE_MAC__)
      {
         Ptr procedure;
         CFragSymbolClass symClass;
         char symName[255];
         OSErr myErr;

         strcpy( symName, "jseExtensionVer" );
         c2pstr( symName );

         myErr = FindSymbol( temphExtensionLib, (ubyte *) symName, &procedure, &symClass );

         fp = (FP)procedure;
         
         if ( myErr != noErr ) 
            fp = NULL;
      }
    #elif defined(__JSE_NWNLM__)
      jse_sprintf(all,"%sjseExtensionVer",before);
      fp = (FP)ImportSymbol(GetNLMHandle(),all);
    #else
      #error dont know how to resolve procedure address for this system
    #endif
      if(fp != NULL) {
        ver = (*((import_cast)fp))(NULL);
        if( ver != JSEEXTERNALVER ) {
          callError(call,textcoreLINK_LIBRARY_BAD_VERSION,ver,FullLibraryPath,JSEEXTERNALVER);
          retval = False;
        }
      }else {
         /* fp is null, so there was no jseExtensionVer function in the dll */
        #if defined(__JSE_WIN32__) || defined(__JSE_WIN16__) || defined(__JSE_CON32__)
          FreeLibrary(temphExtensionLib);
        #elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
          DosFreeModule(temphExtensionLib);
        #elif defined(__hpux__)
          shl_unload(temphExtensionLib);
// seb 98.11.12
        #elif defined(__JSE_BEOS__)
#warning External_libraries_not_implemented_yet  
        #elif defined(__JSE_UNIX__)
          dlclose(temphExtensionLib);
        #elif defined(__JSE_MAC__)
          CloseConnection( &temphExtensionLib );
        #elif defined(__JSE_NWNLM__)
          UnimportSymbol(GetNLMHandle(),all);
        #else
           #error Not done for this system
        #endif
        callError(call,textcoreLINK_LIBRARY_FUNC_NOT_EXIST,FullLibraryPath);
        retval = False;
      }
  }

#if defined(__JSE_NWNLM__)
  UnimportSymbol(GetNLMHandle(),all);
#endif

  /* Second call the Ext Lib's initialization function (jseLoadExtension)
   * this will pass a jseContext ptr to the dll and the dll will add its function
   * table to the host's jse Function Library (known as 'call.session.TheLibrary')
   */
  if( retval ) {
    #if defined(__JSE_WIN16__)
      fp = (FP)GetProcAddress(temphExtensionLib, "JSELOADEXTENSION");
    #elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)
      /* Borland style */
      fp = (FP)GetProcAddress(temphExtensionLib, "_jseLoadExtension");
      if(fp == NULL) {
        /* Watcom style */
        fp = (FP)GetProcAddress(temphExtensionLib, "jseLoadExtension_");
        if(fp == NULL) {
          /* MSVC style */
          fp = (FP)GetProcAddress(temphExtensionLib, "jseLoadExtension");
          if(fp == NULL) {
            /* PASCAL (win16) style */
            fp = (FP)GetProcAddress(temphExtensionLib, "JSELOADEXTENSION");
          }
        }
      }
    #elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
      PFN   ModuleAddr  = 0;
      fp = NULL;
      rc = DosQueryProcAddr(temphExtensionLib,0L,"_JSELOADEXTENSION",&ModuleAddr);
      /* if rc != 0, then there is no jseLoadExtension function in the dll */
      if (rc == 0) {
        fp = (FP)ModuleAddr;
      }
    #elif defined(__hpux__)
      if( shl_findsym(&temphExtensionLib,"jseLoadExtension",TYPE_UNDEFINED,(void *)&fp)==-1 )
        fp = NULL;
// seb 98.11.12
    #elif defined(__JSE_BEOS__)
#warning External_libraries_not_implemented_yet  
    #elif defined(__JSE_UNIX__)
      fp = (FP)dlsym(temphExtensionLib,"jseLoadExtension");
      /* FreeBSD uses the _X that was the original way for Unix */
      if( fp==NULL )
        fp = (FP)dlsym(temphExtensionLib,"_jseLoadExtension");
    #elif defined(__JSE_MAC__)
      Ptr procedure;
      CFragSymbolClass symClass;
      char symName[255];
      OSErr myErr;

      strcpy( symName, "jseLoadExtension" );
      c2pstr( symName );

      myErr = FindSymbol( temphExtensionLib, (ubyte *) symName, &procedure, &symClass );

      fp = (FP)procedure;
      
      if ( myErr != noErr ) 
        fp = NULL;
    #elif defined(__JSE_NWNLM__)
      jse_sprintf(all,"%sjseLoadExtension",before);
      fp = (FP)ImportSymbol(GetNLMHandle(),all);
    #else
      #error dont know how to resolve procedure address for this system
    #endif

    if(fp != NULL) {
      retval = (*((import_cast)fp))(call);

      if( retval ) {
         /* The Extension is successfully added to the library and
          * did it's own initialization, so now save the Library info
          */
        extensionAddLibName(this,LibraryName,temphExtensionLib);
      }else {
         /* New protocol - the .dll prints any error messages, We just quit.
          * the extension lib (dll) reported an error to the host
          */
         callSetReasonToQuit(call,FlowError);
      }
    }else {
       /* fp is null, so there was no jseLoadExtension function in the dll */
      #if defined(__JSE_WIN32__) || defined(__JSE_WIN16__) || defined(__JSE_CON32__)
        FreeLibrary(temphExtensionLib);
      #elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
        DosFreeModule(temphExtensionLib);
      #elif defined(__hpux__)
        shl_unload(temphExtensionLib);
// seb 98.11.12
      #elif defined(__JSE_BEOS__)
#warning External_libraries_not_implemented_yet  
      #elif defined(__JSE_UNIX__)
        dlclose(temphExtensionLib);
      #elif defined(__JSE_MAC__)
        CloseConnection( &temphExtensionLib );
      #elif defined(__JSE_NWNLM__)
        UnimportSymbol(GetNLMHandle(),all);
      #endif
      callError(call,textcoreLINK_LIBRARY_FUNC_NOT_EXIST,FullLibraryPath);
      retval = False;
    }
  }

#if defined(__JSE_NWNLM__)
  UnimportSymbol(GetNLMHandle(),all);
#endif

  jseMustFree(FullLibraryPath);
  return retval;
}


   void NEAR_CALL 
extensionFreeAllLibs(struct ExtensionLibrary *this)
{
   while ( NULL != this->RecentLinkedExtensionLib )
   {
      struct LinkedExtensionLib *Prev = this->RecentLinkedExtensionLib->Previous;

      linkedLibraryShutdown(this->RecentLinkedExtensionLib);
      jseMustFree(this->RecentLinkedExtensionLib);
      this->RecentLinkedExtensionLib = Prev;
   }
}


/* Core calls this function to unload the extension */
   void NEAR_CALL
linkedLibraryShutdown(struct LinkedExtensionLib *this)
{
   /* Close/UnLoad the sharable module,
    * return FALSE on success, TRUE on failure
    */

  assert(this->hExtensionLib != NULL);

#  if defined(__JSE_WIN32__) || defined(__JSE_WIN16__) || defined(__JSE_CON32__)
      FreeLibrary(this->hExtensionLib);
#  elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
      DosFreeModule(this->hExtensionLib);
#  elif defined(__hpux__)
      shl_unload(this->hExtensionLib);
// seb 98.11.12
#elif defined(__JSE_BEOS__)
#warning External_libraries_not_implemented_yet  
#  elif defined(__JSE_UNIX__)
      dlclose(this->hExtensionLib);
#  elif defined(__JSE_MAC__)
      CloseConnection( &(this->hExtensionLib) );
#  elif defined(__JSE_NWNLM__)
      /* no known unload-NLM for netware */
#  else
#     error dont know how to free library on this system.
#  endif
}

   jsebool NEAR_CALL
extensionBuildjseFuncTable(struct ExtensionLibrary *this,struct Call *call)
{
   struct jseFuncTable_t *jseFuncTable = jseMalloc(struct jseFuncTable_t,sizeof(*jseFuncTable));
   
   if( jseFuncTable != NULL )
   {
   memcpy(jseFuncTable,&gjseFuncTable,sizeof(*jseFuncTable));

   {
#if defined(__JSE_WIN16__)
   static VAR_DATA(ubyte)   NearByte; /* this byte used just to get the current DS for win16 */
   #if defined(_MSC_VER)
      jseFuncTable->DS = (uint)FP_SEG(NearByte); /* was NearByte, &NearByte changed for MSVC152 */
   #else
      jseFuncTable->DS = (uint)FP_SEG(&NearByte); /* was NearByte, &NearByte changed for MSVC152 */
   #endif
#endif
   }
   call->Global->jseFuncTable = jseFuncTable;
   }
   
   return jseFuncTable != NULL;
}


#if defined(JSE_LINK) && (0!=JSE_LINK) \
 && defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   void
extensionTokenWrite(struct ExtensionLibrary *this,struct Call *call,struct TokenSrc *tSrc)
{
   /* write out the name of each external library. These names should be written
    * backwards to write the extensions in the same order they are read in.
    */

   /* first count how many extensions there are */
   struct LinkedExtensionLib *LinkExtLib;
   uword8 ExtCount = 0;
   for ( LinkExtLib = this->RecentLinkedExtensionLib; NULL != LinkExtLib; LinkExtLib = LinkExtLib->Previous )
   {
      ExtCount++;
   }

   /* save the number of extensions */
   tokenWriteByte(tSrc,ExtCount);

   /* write out the name of each extlib */
   while ( 0 < ExtCount-- )
   {
      uword8 i;
      jsechar *dir, *name, *ext;
      uint dirlen, namelen, extlen;
      VarName vName;

      for ( i = 0, LinkExtLib = this->RecentLinkedExtensionLib;
            i < ExtCount; i++, LinkExtLib = LinkExtLib->Previous )
      {
         assert( NULL != LinkExtLib );
      }
      assert( NULL != LinkExtLib );
      assert( NULL != LinkExtLib->RootLibraryName );
      /* write out only the root of the extension library name */
      FileNameParts(LinkExtLib->RootLibraryName,&dir,&dirlen,&name,&namelen,&ext,&extlen);
      vName = EnterIntoStringTable(call,name,namelen);
      tokenWriteString(call,tSrc,vName);
      RemoveFromStringTable(call,vName);
   }
}
#endif  /* defined(JSE_LINK) && defined(JSE_TOKENSRC) */


#if defined(JSE_LINK) && (0!=JSE_LINK) \
 && defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)
void extensionTokenRead(struct ExtensionLibrary *this,struct Call *call,struct TokenDst *tDst)
{
   /* read the number of extension libraries */
   uword8 ExtCount = tokenReadByte(tDst);

   while ( 0 < ExtCount-- ) {
      const jsechar * LibraryName = tokenReadString(call,tDst);
      if ( !extensionLibraryStartup(this,call,LibraryName) )
      {
         tokenFatalError();
      }
   }
}
#endif  /* defined(JSE_LINK) && defined(JSE_TOKENDST) */

#endif /* defined(JSE_LINK) */
