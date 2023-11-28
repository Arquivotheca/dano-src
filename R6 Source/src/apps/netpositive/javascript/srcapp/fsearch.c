/* fsearch.c    File Searcher.  Find source files. 
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
#include "fsearch.h"

#ifndef __CENVI__

#if defined(__JSE_DOS16__) && defined(_MSC_VER)
   #define  strlen   _fstrlen
   #define  strcmp   _fstrcmp
   #define  strncpy  _fstrncpy
   #define  strcpy   _fstrcpy
#endif

#if defined(__JSE_WIN32__) || defined(__JSE_CON32__) || defined(__JSE_WIN16__) || defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
#  define EXTLIB_EXTENSION_COUNT 1
   static CONST_DATA(jsechar *) ExtlibExtensions[EXTLIB_EXTENSION_COUNT]
   = { UNISTR(".dll") };
#elif defined(__JSE_UNIX__)
#  define EXTLIB_EXTENSION_COUNT 2
   static CONST_DATA(jsechar *) ExtlibExtensions[EXTLIB_EXTENSION_COUNT]
   = { UNISTR(""), UNISTR(".so") };
#elif defined(__JSE_MAC__)
#  define EXTLIB_EXTENSION_COUNT 2
   static CONST_DATA(jsechar *) ExtlibExtensions[EXTLIB_EXTENSION_COUNT]
   = { UNISTR(""), UNISTR(".cfm") };
#elif defined(__JSE_NWNLM__)
#  define EXTLIB_EXTENSION_COUNT 2
   static CONST_DATA(jsechar *) ExtlibExtensions[EXTLIB_EXTENSION_COUNT]
   = { UNISTR(""), UNISTR(".nlm") };
#elif defined(__JSE_DOS16__) || defined(__JSE_DOS32__)
   /* these systems do not support #link */
#else
#  error #link Stuff not defined.
#endif


   jsebool JSE_CFUNC FAR_CALL
ToolkitAppFileSearch(jseContext jsecontext,const jsechar _FAR_ * FileSpec,
                     jsechar _FAR_ * FilePathResults,uint FilePathLen,jsebool FindLink)
{
   jsebool retval;
   jseVariable SrchPath;
   jsechar * SearchPathVar = NULL;
   jsechar *FileName = NULL;
   uint extensionCount;
   const jsechar ** Extensions;

   UNUSED_PARAMETER( FilePathLen );

   SrchPath = jseGetMember(jsecontext, NULL, "LibSearchPath");
   if ( NULL != SrchPath ) {
      SearchPathVar = (jsechar*)jseGetString(jsecontext, SrchPath, NULL);
   }
   
#  if defined(JSE_LINK) && (0!=JSE_LINK)
   if( FindLink )
   {
      extensionCount = EXTLIB_EXTENSION_COUNT;
      Extensions = ExtlibExtensions;
   }
   else
#  endif
   {
      extensionCount = 0;
      Extensions = NULL;
   }


   if ( strcmp_jsechar(FileSpec,"-")
       && NULL == (FileName = FindFile(FileSpec,CURRENT_DIRECTORY,extensionCount,Extensions))
       && NULL == (FileName = FindFile(FileSpec,SearchPathVar,extensionCount,Extensions))
      #if !defined(__JSE_MAC__)
       && NULL == (FileName = FindFile(FileSpec,getenv("JSEPATH"),extensionCount,Extensions))
       && NULL == (FileName = FindFile(FileSpec,getenv("PATH"),extensionCount,Extensions))
      #endif
   ) {
      /* file not found */
      retval = False;
   }else {
      retval = True;
      /* file was found */
      strncpy(FilePathResults,FileName,FilePathLen-1);
      jseMustFree(FileName);
   }/* endif */

   return retval;
}

#endif
