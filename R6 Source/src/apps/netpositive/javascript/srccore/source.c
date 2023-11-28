/* source.c     Keep track of the position of source code
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
#if (0!=JSE_COMPILER)

#define  MAX_SOURCE_FILE_LINE_LEN   1500
#if defined(__DJGPP__)
#  define _MAX_PATH   255
#elif defined(__JSE_390__)
#  define _MAX_PATH   80
#elif defined(__JSE_EPOC32__)
#  define _MAX_PATH FILENAME_MAX
#endif

#if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
/* define a function to set the filename.  This happens in this tricky way
 * because for the toolkit user the name field has been defined as a const,
 * so that they don't change it.  But we want to be able to change it
 * ourselves.
 */
   static void NEAR_CALL
sourceSetFilename(struct Source *This,const jsechar * const value)
{
   memcpy((void *)&(This->sourceDesc.name),(void *)&value,
          sizeof(This->sourceDesc.name));
}
#endif

/* ----------------------------------------------------------------------
 * Maintain a list of included files so we don't include them again
 * ---------------------------------------------------------------------- */

#if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
   static void NEAR_CALL
sourceFreeAllIncludedFileMemory(struct Source *This)
{
   while ( NULL != This->RecentIncludedFile ) {
      struct IncludedFile *Prev = This->RecentIncludedFile->Previous;
      jseMustFree(This->RecentIncludedFile);
      This->RecentIncludedFile = Prev;
   }
}
#endif

#if defined(JSE_CONDITIONAL_COMPILE) && (0!=JSE_CONDITIONAL_COMPILE)
   static void NEAR_CALL
sourceRemoveConditionalCompilation(struct Source *This)
{
   struct ConditionalCompilation_ * OldCC = This->ConditionalCompilation;
   assert( NULL != This->ConditionalCompilation );
   This->ConditionalCompilation = This->ConditionalCompilation->prev;
   jseMustFree(OldCC);
}
#endif

void sourceDelete(struct Source *This,struct Call *call)
{
#  if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
   if ( NULL != This->AllocMemory )
   {
#  endif
      /* this source was directly from memory */
      jseMustFree(This->AllocMemory);
#  if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
   }
   else
   {
      /* this source was from a file */
      if ( This->FileIsOpen )
      {
         /* call 'fclose' callback */
#        if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) && \
            (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
            DispatchToClient(call->Global->ExternalDataSegment,
                             (ClientFunction)(call->Global->ExternalLinkParms.
                                              GetSourceFunc),
                             (void *)&call,
                             (void *)&(This->sourceDesc),jseClose);
#        else
            (*(call->Global->ExternalLinkParms.GetSourceFunc))
               (call,&(This->sourceDesc),jseClose);
#        endif
#        if !defined(NDEBUG) && (0<JSE_API_ASSERTLEVEL) && defined(_DBGPRNTF_H)
            if ( !jseApiOK )
            {
               DebugPrintf(UNISTR("Error calling source close function"));
               DebugPrintf(UNISTR("Error message: %s"),jseGetLastApiError());
            }
#        endif
         assert( jseApiOK );
      }
   }
#  endif   
#  if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
      if ( NULL == This->prev ) {
         sourceFreeAllIncludedFileMemory(This);
      } else {
         This->prev->RecentIncludedFile = This->RecentIncludedFile;
      }
#  elif defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
      if ( NULL != This->sourceDesc.name )
         jseMustFree((jsechar *)(This->sourceDesc.name));
#  endif      
#  if defined(JSE_CONDITIONAL_COMPILE) && (0!=JSE_CONDITIONAL_COMPILE)
      while ( NULL != This->ConditionalCompilation ) {
         sourceRemoveConditionalCompilation(This);
      }
#  endif
   jseMustFree(This);
}

#if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
static jsechar * NEAR_CALL sourceFindFileName(struct Source *This,jsechar *FileSpec)
{
   struct IncludedFile *incf;

   assert( NULL != FileSpec  &&  0 != *FileSpec );
   for ( incf = This->RecentIncludedFile; NULL != incf; incf = incf->Previous )
   {
#     if defined(__JSE_UNIX__)
      /* Unix filenames are case-sensitive */
      if ( !strcmp_jsechar(FileSpec,incf->RootFileName) )
#     else
      if ( !stricmp_jsechar(FileSpec,incf->RootFileName) )
#     endif
      {
         return(incf->RootFileName);
      } /* endif */
   } /* endfor */
   return( NULL );
}

   static jsechar * NEAR_CALL
sourceAddFileName(struct Source *This,jsechar *RootFileSpec)
{
   struct IncludedFile *incf =
      jseMustMalloc(struct IncludedFile,sizeof(*incf)+(sizeof(jsechar)*strlen_jsechar(RootFileSpec)));

   assert( NULL == sourceFindFileName(This,RootFileSpec) );
   strcpy_jsechar(incf->RootFileName,RootFileSpec);
   incf->Previous = This->RecentIncludedFile;
   This->RecentIncludedFile = incf;
   return(incf->RootFileName);
}
#endif

static struct Source * NEAR_CALL
sourceNew(struct Source *iprev)
{
   struct Source *This = jseMustMalloc(struct Source,sizeof(struct Source));
   memset(This,0,sizeof(*This));
#  if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
      assert( !This->RecentIncludedFile );
#  endif   
#  if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
      assert( !This->sourceDesc.lineNumber );
      assert( !This->sourceDesc.name );
#  endif      
   assert( !This->AllocMemory );
#  if defined(JSE_CONDITIONAL_COMPILE) && (0!=JSE_CONDITIONAL_COMPILE)   
      assert( NULL == This->ConditionalCompilation );
#  endif      
#  if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
   if ( NULL != (This->prev = iprev) ) {
      This->RecentIncludedFile = iprev->RecentIncludedFile;
   }
#  endif   
   return This;
}

/* ----------------------------------------------------------------------
 * SOURCE DIRECTLY FROM A STRING
 * ---------------------------------------------------------------------- */

   struct Source *
sourceNewFromText(struct Source *PrevSB,const jsechar * const SourceText)
{
   struct Source *This = sourceNew(PrevSB);
   jsechar *s;

   assert( NULL != SourceText );
   This->MemoryPtr =
      (This->AllocMemory = jseMustMalloc(jsechar,sizeof(jsechar)*(1+strlen_jsechar(SourceText)+1))) + 1;
   strcpy_jsechar(This->MemoryPtr,SourceText);
   This->MemoryEnd = This->MemoryPtr + strlen_jsechar(This->MemoryPtr);
   /* turn all newlines into NULL so work one line at a time */
   for ( s = This->MemoryPtr; NULL != (s=strchr_jsechar(s,'\n')); s++ )
   {
      *s = '\0';
   }
#  if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
      if ( NULL != PrevSB )
      {
         This->sourceDesc.lineNumber = PrevSB->sourceDesc.lineNumber;
         sourceSetFilename(This,PrevSB->sourceDesc.name);
      }
#  endif      
   /* reading in plain always force initial NULL to force call to NextLine() */
   assert( This->AllocMemory == (This->MemoryPtr - 1) );
   *(This->MemoryPtr = This->AllocMemory) = 0;

   return This;
}

/* ----------------------------------------------------------------------
 * INITIAL SOURCE FROM SOURCE 'FILE'
 * ---------------------------------------------------------------------- */

#if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
struct Source *sourceNewFromFile(struct Call *call,struct Source *PrevSB,
                                 jsechar * FileSpec,jsebool *Success)
{
   struct Source * This;
   jsechar *FileName;

   /* We used to get the source from a file that we managed.  To remove
    * dependencies on an I/O library we now use a callback to manage any
    * files or file like script sources.  This also opens to the possibility
    * of reading script source line-by-line fron non-file sources i.e 
    * network, database, serialport, etc...
    */
   This = sourceNew(PrevSB);
   assert( NULL == This->AllocMemory );
      /* AllocMemory is NULL for interp from memory  */
   This->MemoryPtr = UNISTR("");
      /* initialize in case someone tries to read from this */
   This->FileIsOpen = False;
      /* in case of error, show that no file was opened */

   FileName = jseMustMalloc(jsechar, _MAX_PATH*sizeof(jsechar));

   assert( NULL != FileSpec );

   /* call 'fullpath' callback */
   if ( NULL == call->Global->ExternalLinkParms.FileFindFunc )
   {
      /* no callback provided, so just copy the file name directly */
      strncpy_jsechar(FileName,FileSpec,_MAX_PATH-1);
      *Success = True;
   }
   else
   {
#     if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) && \
      (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
         *Success = (jsebool)DispatchToClient(call->Global->
                                              ExternalDataSegment,
                              (ClientFunction)(call->Global->
                                               ExternalLinkParms.FileFindFunc),
                              (void *)&call,
                              (void *)FileSpec,
                              (void *)FileName,
                              (void *)(_MAX_PATH-1),
                              (void *)False);
#     else
         *Success = ( (*(call->Global->ExternalLinkParms.FileFindFunc))
                      (call,FileSpec,FileName,_MAX_PATH-1,False) );
#     endif
#     if !defined(NDEBUG) && (0<JSE_API_ASSERTLEVEL) && defined(_DBGPRNTF_H)
         if ( !jseApiOK )
         {
            DebugPrintf(UNISTR("Error calling FileFind function on filespec \"%s\""),
                        FileSpec);
            DebugPrintf(UNISTR("Error message: %s"),jseGetLastApiError());
         }
#     endif
      assert( jseApiOK );
   }

   if ( *Success != False )
   {
#     if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
         /* if this file was already used, then don't use it again */
         jsechar * SavedFileName;
         if ( NULL != (SavedFileName=sourceFindFileName(This,FileName)) )
         {
            sourceSetFilename(This,SavedFileName);
         }
         else
#     endif      
         {
            sourceSetFilename(This,FileName);
            /* call 'fopen' callback */
            if ( NULL == call->Global->ExternalLinkParms.GetSourceFunc )
            {
               *Success = False;
            }
            else
            {
#              if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) && \
                  (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
                  *Success = DispatchToClient(call->Global->
                                              ExternalDataSegment,
                     (ClientFunction)(call->Global->
                                      ExternalLinkParms.GetSourceFunc),
                     (void *)&call,(void *)&This->sourceDesc,jseNewOpen);
#              else
                  *Success = ( (*(call->Global->ExternalLinkParms.
                                  GetSourceFunc))
                                 (call,&This->sourceDesc,jseNewOpen) );
#              endif
#              if !defined(NDEBUG) && (0<JSE_API_ASSERTLEVEL) && \
                  defined(_DBGPRNTF_H)
                  if ( !jseApiOK )
                  {
                     DebugPrintf(
                        UNISTR("Error calling source open function on file \"%s\""),
                                 This->sourceDesc.name);
                     DebugPrintf(UNISTR("Error message: %s"),jseGetLastApiError());
                  }
#              endif
               assert( jseApiOK );

               if ( *Success ) {
                  /* yea! File is open. */
                  This->FileIsOpen = True;
#                 if defined(JSE_GETFILENAMELIST) && (0!=JSE_GETFILENAMELIST)
                     callAddFileName(call,FileName);
#                 endif
#                 if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
                     sourceSetFilename(This,sourceAddFileName(This,FileName));
#                 else
                     sourceSetFilename(This,StrCpyMalloc(FileName));
#                 endif                  
               }
            }
         }
   }

   jseMustFree(FileName);

   if ( !(*Success) )
   {
     callError(call,textcoreUNABLE_TO_OPEN_SOURCE_FILE,FileSpec);
   }
   else
   {
      /* for top level, read in first line now */
      if ( NULL == PrevSB  &&  This->FileIsOpen  &&
           !sourceNextLine(This,call,Success) )
      {
         This->MemoryPtr = UNISTR("");
      }
   }

   return This;
}
#endif

/* ----------------------------------------------------------------------
 * INCLUDING A FILE
 * ---------------------------------------------------------------------- */

#if defined(JSE_CONDITIONAL_COMPILE) && (0!=JSE_CONDITIONAL_COMPILE)

enum ConditionalCompilationDirective {
   ccd_if_flag  = 0x01,  /* this flag set on if, ifdef, and endif */
   ccd_if       = 0x03,
   ccd_ifdef    = 0x05,
   ccd_ifndef   = 0x09,
   ccd_endif    = 0x10,
   ccd_else     = 0x20,
   ccd_elif     = 0x40
};
struct ConditionalComps_ {
   const jsechar *Name;
   uint len;
   enum ConditionalCompilationDirective ccd;
};

   static jsebool NEAR_CALL
sourceEvaluateConditionalCompilation(
                           struct Source *This,struct Call *call,
                           const jsechar * SourceStatement,
                           jsebool WantTruth,jsebool AutomaticDefineFunc)
{
   jsechar * command_buf = jseMustMalloc(jsechar,(strlen_jsechar(SourceStatement) + 50)*sizeof(jsechar));
   jseVariable ReturnVar;
   jsebool InterpSuccess;
   jsebool RestoreOptReqVarKeyword;

   assert( !This->ConditionalCompilation->ConditionHasBeenMet );
   /* create wrapper around statement to interpret */
   jse_sprintf( command_buf, UNISTR("%s %s"), AutomaticDefineFunc ?
                  UNISTR("\"undefined\" != typeof") : UNISTR(""), SourceStatement );
   /* call to interpret the statement */

   /* this is quite likely an #ifdef type of statement, which can
    * causes errors if VAR keyword is required.  So turn off that
    * options during the life of this call.
    */
   RestoreOptReqVarKeyword = ( 0 != ( call->Global->ExternalLinkParms.options
                                    & jseOptReqVarKeyword ) );
   if ( RestoreOptReqVarKeyword )
      call->Global->ExternalLinkParms.options &= ~jseOptReqVarKeyword;
   InterpSuccess = jseInterpret(call,NULL,command_buf,NULL,
                                jseNewFunctions|jseNewAtExit,
                                0,NULL,&ReturnVar);
   if ( RestoreOptReqVarKeyword )
      call->Global->ExternalLinkParms.options |= jseOptReqVarKeyword;

   jseMustFree(command_buf);
   assert( !callQuitWanted(call) );
   if ( InterpSuccess ) {
      jsebool ReturnCode = jseEvaluateBoolean(call,ReturnVar);
      assert( NULL != ReturnVar );
      jseDestroyVariable(call,ReturnVar);
      This->ConditionalCompilation->ConditionHasBeenMet =
      This->ConditionalCompilation->ConditionTrue =
         WantTruth ? ReturnCode : !ReturnCode ;
   } else {
      assert( NULL == ReturnVar );
      /* set flag for higher levels to be sure an error occured */
      callErrorBase(call);
   } /* endif */
   return InterpSuccess;
}

   static jsebool NEAR_CALL
sourceConditionalCompilationFilter(
     struct Source *This,struct Call *call,jsebool *IgnoreThisLine)
     /* If error then return False (error printed through call->errorxx),
      * else True. If filter then set *IgnoreThisLine = True (else ignore it)
      */
{
   static CONST_DATA(struct ConditionalComps_) ConditionalComps[] = {
      { UNISTR("if"),      2,     ccd_if     },
      { UNISTR("ifdef"),   5,     ccd_ifdef  },
      { UNISTR("ifndef"),  6,     ccd_ifndef },
      { UNISTR("endif"),   5,     ccd_endif  },
      { UNISTR("else"),    4,     ccd_else  },
      { UNISTR("elif"),    4,     ccd_elif  },
      { NULL }
   };
   jsechar *PotentialConditionalComp = (This->MemoryPtr) + 1;
   uint PotentialLen = strcspn_jsechar(PotentialConditionalComp,WhiteSpace);
   const struct ConditionalComps_ *cc;
   enum ConditionalCompilationDirective ccd;
   const jsechar *SourceToEvaluate;

   assert( '#' == *(This->MemoryPtr) );
   for ( cc = ConditionalComps; ; cc++ ) {
      if ( NULL == cc->Name ) {
         return True;
      } /* endif */
      assert( cc->len == strlen_jsechar(cc->Name) );
      if ( PotentialLen == cc->len &&
           !strnicmp_jsechar((void*)cc->Name,PotentialConditionalComp,(size_t)PotentialLen) )
         break;
   }

   /* this is a conditional compilation directive.  Handle now */
   ccd = cc->ccd;
   SourceToEvaluate = (This->MemoryPtr) + PotentialLen + 2;
   if ( ccd & ccd_if_flag ) {
      /* add new conditional structure to nested/linked list */
      struct ConditionalCompilation_ *NewCC =
         jseMustMalloc(struct ConditionalCompilation_,
                       sizeof(struct ConditionalCompilation_));
      NewCC->prev = This->ConditionalCompilation;
      This->ConditionalCompilation = NewCC;
      NewCC->ConditionTrue = False;
      if ( NULL != NewCC->prev  &&  !NewCC->prev->ConditionTrue )
      {
         /* we're within a conditional compilation statement that is not true,
          * so this one will not be true either. Just set ConditionHasBeenMet
          * to avoid future inquiries.
          */
         NewCC->ConditionHasBeenMet = True;
      } else {
         NewCC->ConditionHasBeenMet = False;
         /* determine if */
         if ( ccd_if == ccd ) {
            if ( !sourceEvaluateConditionalCompilation(This, call,
                     SourceToEvaluate, True, False ) )
               return False;
         } else {
            assert( ccd_ifdef == ccd  ||  ccd_ifndef == ccd );
            if ( !sourceEvaluateConditionalCompilation(This,call,
                      SourceToEvaluate, ccd_ifdef == ccd, True ) )
               return False;
         } /* endif */
      }
   } else {
      /* not an #ifxxxx, this must be within a conditional compilation */
      if ( NULL == This->ConditionalCompilation ) {
         callError(call,textcoreMUST_APPEAR_WITHIN_CONDITIONAL_COMPILATION,
                   cc->Name);
         return False;
      } /* endif */
      if ( ccd_endif == ccd ) {
         sourceRemoveConditionalCompilation(This);
      } else if ( !This->ConditionalCompilation->ConditionHasBeenMet ) {
         if ( ccd_else == ccd ) {
            /* no condition previously met, so take this one */
            This->ConditionalCompilation->ConditionTrue = True;
            This->ConditionalCompilation->ConditionHasBeenMet = True;
         } else {
            assert( ccd_elif == ccd );
            if ( !sourceEvaluateConditionalCompilation(This,call,
                     SourceToEvaluate, True, False ) )
               return False;
         } /* endif */
      } else {
         /* new directive when have already taken correct one.
            turn off in case it is on */
         This->ConditionalCompilation->ConditionTrue = False;
      }
   }
   *IgnoreThisLine = True; /* will force next line to be read */
   return True;
}

#endif



jsebool sourceNextLine(struct Source *This,struct Call *call,jsebool *success)
     /* read in each line to make one huge file, always at least one
      * newline-whitespace between lines, and at beginning and end of the
      * whole thing.
      */
{
#  if defined(JSE_CONDITIONAL_COMPILE) && (0!=JSE_CONDITIONAL_COMPILE)
      jsebool IgnoreThisLine;
#  endif

   do {
#     if defined(JSE_CONDITIONAL_COMPILE) && (0!=JSE_CONDITIONAL_COMPILE)
         IgnoreThisLine = False;
#     endif
      call->Global->CompileStatus.CompilingLineNumber =
         ++(SOURCE_LINE_NUMBER(This));
         /* set for display in case of error */

#     if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
      if ( NULL != This->AllocMemory ) /* interp from buffer */
#     endif      
      {
         /* Does this comment simply mean that we're 'interpreting from a
          * buffer'? not a file pointer Source object.  Next line is
          * characters beyond next null returned from previous NextLine call
          */
         This->MemoryPtr += 1 + strlen_jsechar(This->MemoryPtr);
         if ( This->MemoryEnd <= This->MemoryPtr) {
            goto EndOfFile;
         }
         SKIP_WHITESPACE(This->MemoryPtr);
      }
#     if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
      else /* interp from customer supplied 'file' */
      {
         /* call 'fgets' callback */
         if ( !This->FileIsOpen /* if file never had to happen */ 
           || !
#        if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) && \
            (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
            DispatchToClient(call->Global->ExternalDataSegment,
                             (ClientFunction)(call->Global->ExternalLinkParms.
                                              GetSourceFunc),
                             (void *)&call,
                             (void *)&This->sourceDesc,jseGetNext)
#        else
            ( (*(call->Global->ExternalLinkParms.GetSourceFunc))
                           (call,&This->sourceDesc,jseGetNext) )
#        endif
            )
            goto EndOfFile;
#        if !defined(NDEBUG) && (0<JSE_API_ASSERTLEVEL) && defined(_DBGPRNTF_H)
            if ( !jseApiOK )
            {
               DebugPrintf(
                  UNISTR("Error calling source-line callback with jseGetNext"));
               DebugPrintf(UNISTR("Error message: %s"),jseGetLastApiError());
            }
#        endif
         assert( jseApiOK );
         This->MemoryPtr = This->sourceDesc.code;
         assert(NULL != This->MemoryPtr);
         SKIP_WHITESPACE(This->MemoryPtr);
      }
#     endif      

      assert( 0 == *(This->MemoryPtr) || !IS_WHITESPACE(*(This->MemoryPtr)) );
#     if defined(JSE_CONDITIONAL_COMPILE) && (0!=JSE_CONDITIONAL_COMPILE)
         if ( '#' == *(This->MemoryPtr) )
         {
         #ifdef __JSE_UNIX__
           if ( '!' == This->MemoryPtr[1] )
           {
              // for unix skip any lines starting with #!
              IgnoreThisLine = True;
           } else
         #endif
            if( !sourceConditionalCompilationFilter(This,call,&IgnoreThisLine))
            {
               assert( callQuitWanted(call) );
               *success = False;
               return False;
            }
         }
#     endif

   } while( 0 == *(This->MemoryPtr)
#            if defined(JSE_CONDITIONAL_COMPILE) && \
                (0!=JSE_CONDITIONAL_COMPILE)
                || IgnoreThisLine
                || ( This->ConditionalCompilation &&
                     !This->ConditionalCompilation->ConditionTrue )
#            endif
          );
   return True;

EndOfFile:
   assert( jseApiOK );
#  if defined(JSE_CONDITIONAL_COMPILE) && (0!=JSE_CONDITIONAL_COMPILE)
      if ( This->ConditionalCompilation ) {
         callError(call,textcoreENDIF_NOT_FOUND);
         *success = False;
      } /* endif */
#  endif      
   return False;
}


#if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
/* if this file was already used, then don't use it again */
jsebool sourceInclude(struct Source **source,struct Call *call)
{
   jsechar QuoteChar;
   jsechar *src = sourceGetPtr(*source);
   jsebool success = False;

   /* next character had better be a quote character */
   if ( (QuoteChar = '\"') != *src
     && (QuoteChar = '\'') != *src
     && (QuoteChar = '<') != *src ) {
      callError(call,textcoreMISSING_INCLINK_NAME_QUOTE,'<',
                textcoreIncludeDirective);
   }
   else
   {
      jsechar EndQuoteChar = (jsechar) (('<' == QuoteChar) ? '>' : QuoteChar);
      jsechar *End;

      if ( NULL == (End = strchr_jsechar(++src,EndQuoteChar)) ) {
         callError(call,textcoreMISSING_INCLINK_NAME_QUOTE,EndQuoteChar,
                   textcoreIncludeDirective);
      }
      else
      {
         *End = 0;
         sourceSetPtr(*source,End+1);
         *source = sourceNewFromFile(call,*source,src,&success);
         if( success )
         {
            if ( !sourceNextLine(*source,call,&success) )
            {
               /* nothing was read at all */
               *((*source)->MemoryPtr) = 0;
            }
         }
      }
   }
   return success;
}
#endif


#if defined(JSE_GETFILENAMELIST) && (0!=JSE_GETFILENAMELIST)
void NEAR_CALL callAddFileName(struct Call *This,jsechar *name)
{
  if( This->Global->FileNameList )
    This->Global->FileNameList =
       jseMustReMalloc(jsechar *,This->Global->FileNameList,
                       (This->Global->number+1)*sizeof(jsechar *));
  else
    This->Global->FileNameList = jseMustMalloc(jsechar *,sizeof(jsechar *));

  This->Global->FileNameList[This->Global->number] =
     jseMustMalloc(jsechar,(strlen_jsechar(name)+1)*sizeof(jsechar));
  strcpy_jsechar(This->Global->FileNameList[This->Global->number++],name);
}
#endif

#endif
