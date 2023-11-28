/* filelist.c  RedirectionInfo  Application Service
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

static uint jseFindStdFileIndex(jseContext jsecontext,FILE * fp)
{
   struct RedirectionInfo * info = RedirectionInfoFromContext(jsecontext);
   uint idx;
   for ( idx = 0; fp != info->std_file[idx].fp; idx++ )
   {
      assert( idx < std_idx_count );
   } /* endfor */
   return idx;
}

static void
jseNewStdFile(jseContext jsecontext, FILE *fp, const jsechar *name,ulong ref)
{
   struct RedirectionInfo * info = RedirectionInfoFromContext(jsecontext);
   uint idx = jseFindStdFileIndex(jsecontext,fp);
   struct std_file_ * prev = jseMustMalloc(struct std_file_,sizeof(struct std_file_));
   
   *prev = info->std_file[idx];
   prev->ReOpenOffset = (-1L == ftell(fp)) ? 0 : ftell(fp);
   
   info->std_file[idx].Previous = prev;
   info->std_file[idx].filename = StrCpyMalloc( name );
   info->std_file[idx].ref = ref;
}

static void
jseCloseStdFile(jseContext jsecontext, FILE *fp)
{
   struct RedirectionInfo * info = RedirectionInfoFromContext(jsecontext);
   uint idx = jseFindStdFileIndex(jsecontext,fp);

   if ( NULL != info->std_file[idx].filename )
   {
      struct std_file_ *prev_std_info;
      jsechar *PreviousFileName;
#    ifdef __JSE_UNIX__
      static CONST_DATA(jsechar) DefaultConsoleName[] = "/dev/tty";
#    elif defined(__JSE_MAC__)
      static CONST_DATA(jsechar *) DefaultConsoleName = NULL;
#    else
      static CONST_DATA(jsechar) DefaultConsoleName[] = UNISTR("CON");
#    endif
      const jsechar *FileName;
      const jsechar *Mode;

      jseMustFree(info->std_file[idx].filename);

      prev_std_info = info->std_file[idx].Previous;
      assert( NULL != prev_std_info );

      /* this file has been opened or reopened, so return to its previous state */
      PreviousFileName = prev_std_info->filename;

      /* this file has been reopened since the previous FileSystem, so return to that previous state */
      if ( NULL == PreviousFileName )
      {
         FileName = DefaultConsoleName;
         Mode = info->std_file[idx].OpenMode;
      }
      else
      {
         FileName = PreviousFileName;
         Mode = info->std_file[idx].ReOpenMode;
      }
      if ( FileName != NULL &&
           NULL == freopen_jsechar(FileName,Mode,fp))
      {
         /* failed to reopen, so last resort is to reopen console */
#    if defined(__JSE_WIN32__)
         /* con often fails with NT, so open NUL and call it CON */
         freopen_jsechar(UNISTR("NUL"),info->std_file[idx].OpenMode,fp) ;
#    else
         FileName = DefaultConsoleName;
         freopen_jsechar(FileName,info->std_file[idx].OpenMode,fp) ;
#    endif
      }
      else
      {
         if ( NULL != PreviousFileName  &&  idx == stdin_idx )
         {
            /* reopening a read file; position to previous location */
            fseek(fp,prev_std_info->ReOpenOffset,SEEK_SET);
         }
      } /* endif */

      /* Make sure under CON32 that the standard handles are correct otherwise
       * strange things can happen to the screen. Only do this when stdio is
       * directed to the console, though. 
       */
#    ifdef __JSE_CON32__
      if( NULL == PreviousFileName )
      {
         SetStdHandle(info->std_file[idx].standard_value,info->std_file[idx].saved);
      }
#    endif

      /* restore std_info to the previous std_info */
      info->std_file[idx] = *prev_std_info;
      jseMustFree(prev_std_info);

   } /* endif */
  
}

   static void JSE_CFUNC FAR_CALL
RemoveStandardService_FileRedirection(jseContext jsecontext,struct RedirectionInfo * info)
{
   assert( info != NULL );
   UNUSED_PARAMETER(jsecontext);
   jseMustFree(info);
}

   void
AddStandardService_FileRedirection(jseContext jsecontext)
{
   struct RedirectionInfo * info;
   
   assert(RedirectionInfoFromContext(jsecontext)==NULL);

   info = jseMustMalloc(struct RedirectionInfo,sizeof(struct RedirectionInfo));
   memset(info,0,sizeof(struct RedirectionInfo));
#  ifndef NDEBUG
   {
      uint i;
      for ( i = 0; i < std_idx_count; i++ )
      {
         assert( NULL == info->std_file[i].filename );
         assert( NULL == info->std_file[i].Previous );
      }
   }
#  endif
   info->std_file[stdin_idx].fp = stdin;
   info->std_file[stdin_idx].OpenMode = UNISTR("rt");
   info->std_file[stdin_idx].ReOpenMode = UNISTR("rt");
   info->std_file[stdin_idx].VariableName = UNISTR("stdin");
   info->std_file[stdout_idx].fp = stdout;
   info->std_file[stdout_idx].VariableName = UNISTR("stdout");
   info->std_file[stderr_idx].fp = stderr;
   info->std_file[stderr_idx].VariableName = UNISTR("stderr");
#  if defined(__JSE_WIN32__) || defined(__JSE_CON32__)
      info->std_file[stdout_idx].OpenMode = UNISTR("wt");
      info->std_file[stdout_idx].ReOpenMode = UNISTR("at");
      info->std_file[stderr_idx].OpenMode = UNISTR("wt");
      info->std_file[stderr_idx].ReOpenMode = UNISTR("at");
#  else
      info->std_file[stdout_idx].OpenMode = UNISTR("wt+");
      info->std_file[stdout_idx].ReOpenMode = UNISTR("at+");
      info->std_file[stderr_idx].OpenMode = UNISTR("wt+");
      info->std_file[stderr_idx].ReOpenMode = UNISTR("at+");
#  endif
#  if defined(__JSE_CON32__)
      info->std_file[stdin_idx].standard_value = STD_INPUT_HANDLE;
      info->std_file[stdout_idx].standard_value = STD_OUTPUT_HANDLE;
      info->std_file[stderr_idx].standard_value = STD_ERROR_HANDLE;

      {
         uint m;
         for ( m = 0; m < std_idx_count; m++ )
         {
            info->std_file[m].saved = (HANDLE) -1;
            DuplicateHandle(GetCurrentProcess(),
                            GetStdHandle(info->std_file[m].standard_value),
                            GetCurrentProcess(),
                            &(info->std_file[m].saved),
                            0L,TRUE,DUPLICATE_SAME_ACCESS);
         }
      }
#  endif

   info->Open = jseNewStdFile;
   info->Close = jseCloseStdFile;
   info->Find = jseFindStdFileIndex;

   jseSetSharedData(jsecontext,REDIRECTION_NAME,
                     info,(void *)RemoveStandardService_FileRedirection);
}
