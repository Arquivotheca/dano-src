/* FindFile.c   Routines for finding file 
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

#ifndef JSETOOLKIT_CORE

#ifdef __JSE_UNIX__
  #include "unixfunc.h"
#endif

#if defined(_MSC_VER) && !defined(__JSE_WINCE__)
   #include <sys\stat.h>
   #define stat   _stat
#endif

#if defined(__sun__)
  #include <unistd.h>
  #include <sys/stat.h>
#endif


#if defined(__WATCOMC__) && defined(JSE_UNICODE) && (0!=JSE_UNICODE)
int _ustat( const _unichar *name, struct stat * st)
{
   const char *a = UnicodeToAscii(name);
   int ret = stat(a,st);
   FreeAsciiString(a);
   return ret;
}
#endif

#if defined(__JSE_NWNLM__)
   /* Netware defines only ACCESS_RD, so let's use that for now. */
   #define F_OK ACCESS_RD
#elif defined(__DJGPP__)
   #include <unistd.h>
#elif defined(__BORLANDC__)
   #include <sys\stat.h>
   #define F_OK  2
   #undef S_ISDIR
   #define S_ISDIR( m )     (((m) & S_IFMT) == S_IFDIR)
#elif defined(_MSC_VER)
   #define F_OK  0x00
   #define S_ISDIR( m ) (((m) & S_IFMT) == S_IFDIR)
   #define access(a,b)  _access(a,b)
#elif defined(__IBMCPP__)
   #define F_OK  0x00
   #define S_ISDIR( m )     (((m) & S_IFDIR) == S_IFDIR)
#endif

   jsebool
fileExists( jsechar const SearchSpec[] )
   /* SearchSpec contains a complete file specification. Return True if the file
      exists and false if it does not. */
{
   #if defined(__JSE_DOS16__) || defined(__JSE_DOS32__) || defined(__JSE_WIN16__)
      /* underlying DOS can report that file with spaces exists, so if spaces in this name then it 
        is not a file */
      if ( NULL != strpbrk(SearchSpec,WhiteSpace) ) {
         return False;
      }
   #endif
   #if defined(__JSE_MAC__)
      struct stat buf;
      if ( stat_jsechar( SearchSpec, &buf ) == 0 )
      {
        if ( !S_ISDIR(buf.st_mode) )
          return True;
      }
   #else
      if (  0 == access_jsechar(SearchSpec,F_OK) ) {
#     if defined(__JSE_WINCE__)
         struct _stat buf;
#     else
         struct stat buf;
#     endif
         memset(&buf,0,sizeof(buf));
         stat_jsechar(SearchSpec,&buf);
         if( !S_ISDIR(buf.st_mode) )
         return True;
      }
   #endif /* __JSE_MAC__ */
   return False;
}


   jsechar *
FindFile(const jsechar * FileSpec,
         const jsechar * SearchPaths,
         uint ExtensionCount,const jsechar * const Extension[])
{
   jsechar *dir, *name, *ext;
   uint dirlen, namelen, extlen;
   jsechar const * const *ExtensionList;
   jsechar const * SingleExtension[1];
   jsechar *AllocatedDir;
   jsechar *FoundFile;
   jsechar SearchSpec[(_MAX_PATH*2)+_MAX_EXT+10/*little extra, just in case*/];
   uint i;
#  ifdef __JSE_UNIX__
   jsechar const *DoubleExtension[2];
#  endif
   
   assert( NULL != FileSpec );

   /* not found if nothing there or if no paths to search */
   if ( !FileSpec[0]  ||  !SearchPaths )
      return NULL;

   /* if any wildcards then don't accept */
   if ( strchr_jsechar(FileSpec,'?') || strchr_jsechar(FileSpec,'*') )
      return NULL;

   /* break name into parts */
   FileNameParts(FileSpec,&dir,&dirlen,&name,&namelen,&ext,&extlen);
   /* don't continue if no name or it is too long */
   if ( !namelen  ||  _MAX_PATH < namelen )
      return NULL;

   ExtensionList = Extension;

   /* if file already has an extension, or if there are no extensions to try, then
      make the existing extension (even if "") be the one to use */
   if ( extlen || !ExtensionCount )
   {
      ExtensionCount = 1;
      ExtensionList = SingleExtension;
      SingleExtension[0] = ext;
#     if defined(__JSE_UNIX__)

         if( SingleExtension[0][0]=='\0' )
         {
            DoubleExtension[0] = UNISTR(".jse");
            DoubleExtension[1] = UNISTR("");
            ExtensionCount = 2;
            ExtensionList = DoubleExtension;
         }
#     endif
   }

   /* commented out as per Brent's instruction. I have made the formerly
    * unix- and mac-only code above the 'regular' code. */
#  if 0
      /* if contains any extension then it must be one from the list */
      if ( extlen )
      {
         if ( ExtensionCount )
         {
            uint i;
            for ( i = 0; i < ExtensionCount; i++ )
            {
               if ( !stricmp(Extension[i],ext) )
                  break;
            }
            if ( i == ExtensionCount )
               return NULL;
         }
         ExtensionCount = 1;
         ExtensionList = SingleExtension;
         SingleExtension[0] = ext;
      } /* endif */
#  endif

   /* If file contains a directory spec, then only search that directory.
      Else search through the directories in SearchPaths. */
   if ( dirlen )
      SearchPaths = AllocatedDir = StrCpyMallocLen(dir,dirlen);
   FoundFile = NULL;
   /* searchspec buffer must be big enough */
   do
   {
      while ( *SearchPaths == PATH_SEPARATOR )
      {
         SearchPaths++;
      } /* endwhile */
      if ( *SearchPaths )
      {
         uint PathLen = strcspn_jsechar(SearchPaths,PATH_SEPARATOR_STR);
         /* build name up to last dot before extensions */
         if ( PathLen <= _MAX_PATH )
         {
            memcpy(SearchSpec,SearchPaths,sizeof(jsechar)*PathLen);
            SearchPaths += PathLen;

#           ifdef __REMOTE_JSE__
               if( isRemoteFilename(SearchSpec) )
               {
                  if( SearchSpec[PathLen-1]!='/' )
                     SearchSpec[PathLen++] = '/';
               }
               else
#           endif
               {
                  /* path, before file-name, must end in dir_separator character */
                  if ( SearchSpec[PathLen-1] != DIR_SEPARATOR )
                  {
#                    if !defined(__JSE_MAC__) && !defined(__JSE_UNIX__) && !defined(__JSE_NWNLM__)
                        /* exception if this is a path ending in :, as in "C:TEST.JSE" */
                        if ( SearchSpec[PathLen-1] != ':' )
#                    endif
                           SearchSpec[PathLen++] = DIR_SEPARATOR;
                  } /* endif */
               }

            memcpy(SearchSpec+PathLen,name,sizeof(jsechar)*namelen);
            PathLen += namelen;

            /* try for each extension */
            for ( i = 0; i < ExtensionCount; i++ )
            {
               strcpy_jsechar(SearchSpec+PathLen,ExtensionList[i]);
#              ifdef __REMOTE_JSE__
                  if( isRemoteFilename(SearchSpec) )
                  {
                     jsechar resultbuf[_MAX_PATH];

                     strlwr(SearchSpec);
                     if( HttpFileSearch(SearchSpec,resultbuf,_MAX_PATH) )
                     {
                        FoundFile = StrCpyMalloc(resultbuf);
                        goto BeGone;
                     }
                     continue;
                  }
#              endif

               if ( fileExists(SearchSpec) )
               {

                  /* yea!!!! this is the file; need only get its full specification */
#                 if defined(__JSE_MAC__)
                     FoundFile = StrCpyMalloc(SearchSpec);
#                 else
                     FoundFile = jseMustMalloc(jsechar,(10+_MAX_PATH)*sizeof(jsechar));
                     MakeFullPath(FoundFile,SearchSpec,_MAX_PATH+1);
#                 endif
                  goto BeGone;

               } /* endif */
            } /* endfor */
         } /* endif */

      } /* endif */
   } while ( *SearchPaths ); /* enddo */

BeGone:
   if ( dirlen )
      jseMustFree(AllocatedDir);

   return FoundFile;
}

#if !defined(__JSE_MAC__) && !defined(__JSE_UNIX__) && !defined(__JSE_NWNLM__)
   jsebool
MakeFullPath(jsechar * ResultBuf,const jsechar *PartialPath,uint ResultBufSize)
{
   assert( NULL != ResultBuf );
   assert( NULL != PartialPath );
#  if defined(__WATCOMC__)
   {
      jsechar *TheirFullpath = _fullpath_jsechar(NULL,PartialPath,ResultBufSize-1);
      if ( NULL == TheirFullpath )
         return False;
      strncpy_jsechar(ResultBuf,TheirFullpath,ResultBufSize-1);
      #undef free
      free(TheirFullpath);
      #define free(P)         use MustFree
      return True;
   }
#  elif defined(__GNUC__) && defined (__JSE_DOS32__)
      _fixpath(PartialPath,ResultBuf);
      return TRUE;
#  else
      return ( NULL != _fullpath_jsechar((jsechar*)ResultBuf,(jsechar*)PartialPath,ResultBufSize-1) );
#  endif
}
#endif

#endif /* !JSETOOLKIT_CORE */
