/* codeprt2.c   Extension of Code.cpp because it grew too big.
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

/* the dot character is placed here intentionally so that it can be ignored
 * if parsing string following a function or cfunction keyword
 */
CONST_DATA(jsechar) IllegalVariableChars[] = UNISTR(".&+-<>|=!*/%^~?:,{};()[]\'\"`#");

   jsebool NEAR_CALL
MoreMustFollow(codeval code)
{
   assert( isValidCodeval(code) );
   return ( ((MORE_TO_FOLLOW <= code) && (code < NO_MORE_TO_FOLLOW))
         || (getParamValue==code) );
}

   jsechar * NEAR_CALL
CompileStringToken(struct Call *call,struct Code **CodePtr,
                   jsechar *src,
#                  if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                      jsebool CFunction,
#                  endif
                   jsebool *success)
{
   struct Code *code = *CodePtr;
   jsechar EndChrs[3];
   jsechar *End;


   /* whatever else goes wrong, calling code expects a variable to be created */
   VarRead *var = constructVarRead(call,VUndefined);
   codeSetVar(code,var);

   /* find the end of this string (but skip beyond escape sequences) */
   EndChrs[0] = *src;
   EndChrs[1] = (jsechar) (('`'==*src) ? '\0' : '\\');
   EndChrs[2] = '\0';
   End = src;

   assert( src  &&  *src  &&  strchr_jsechar(UNISTR("`\"\'"),*src) );
   assert( *success );

   do {
      End++;
      End += strcspn_jsechar(End,EndChrs);
   } while ( '\\' == *End  &&  '\0' != *(++End) );
   if ( *src != *End ) {
      /* make error in reasonable-sized buffer */
      jsechar ErrorBuf[80];
      strncpy_jsechar(ErrorBuf,src,(sizeof(ErrorBuf)-1)/sizeof(jsechar));
      callError(call,textcoreNO_TERMINATING_QUOTE,ErrorBuf[0],ErrorBuf);
      *success = False;
   } else {
      /* assign the string to the variable */
#     if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
         jsebool NullString = ( (src + 1) == End );
#     endif         
      jsechar TempSaveBeyondEnd = *(++End);
      jsebool AssignSuccess;

      assert( 0 != End[-1] );
      *End = '\0';
      assert( *success );
      *success = AssignFromText(var,call,src,&AssignSuccess,False,&src);
      *End = TempSaveBeyondEnd;
      if ( *success ) {
         /* check if variable must concatenate with previous variables */
         assert( var == codeGetVar(code) );
         src--; /* will have gone 1 too many */
         assert( VString == VAR_TYPE(var) );

#        if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
         /* For C-Like extensions it is possible to have a single
          * byte if '?' type input
          */
         if ( CFunction
           && '\'' == EndChrs[0]
           && 1 == varGetArrayLength(var,call,NULL)
           && !NullString ) {
            /* this is not really a string.  Use the first byte for a number */
            jsechar NumChar = *((jsechar *)(varGetData(var,0)));
            varConvert(var,call,VNumber);
            varPutLong(var,NumChar);
         }
         else
#        endif
         {
            if ( pushValue == codeGetType(codePrev(code)) )
            {
               /* If two variables in a row are defined as string types then
                * concatenate them add this data to end of previous data
                */
               Var *cgVar = codeGetVar(codePrev(code));
               VarRead *PreviousVar = GET_READABLE_VAR(cgVar,call);
               if( VAR_TYPE(PreviousVar)==VString )
               {
                  VarRead *NewVar;
                  cgVar = codeGetVar(code);
                  NewVar = GET_READABLE_VAR(cgVar,call);
                  ConcatenateStrings(call,PreviousVar,PreviousVar,NewVar);
                  /* delete this new code and variable and replace with old */
                  code = codePrev(code);
                  codeDelete(codeNext(code),call);
                  VAR_REMOVE_USER(PreviousVar,call);
                  VAR_REMOVE_USER(NewVar,call);
               }
            }
         }

      }

   }

   *CodePtr = code;
   return src;
}


   jsebool NEAR_CALL
CompileColon(struct Call *call,struct Code **CodePtr,codeval *Type,
             struct FunctionParms * const FuncParms)
{
   jsebool success = True;
   struct Code *code = *CodePtr;

   if ( pushVariable == codeGetType(codePrev(code))
     && !MoreMustFollow(codeGetType(codePrev(codePrev(code)))) ) {
      /* Label */
      code = codePrev(code);
      codeDelete(codeNext(code),call);
      *Type = Label;
   } else if ( SwitchDefault == codeGetType(codePrev(code)) )
   {
      /* ignore colon after Default */
      *Type = CaseColon;
   } else if ( SwitchCase == codeGetType(codePrev(code)) )
   {
      callError(call,textcoreCASE_STATEMENT_WITHOUT_VALUE,
                parmsGetName(FuncParms));
      success = False;
   } else {
      /* Must look backwards and figure if this is a CASE statement
       * colon, or a ConditionalFalse.
       */
      struct Code *c;
      for ( c = codePrev(code);
            ConditionalTrue != codeGetType(c)  &&  SwitchCase != codeGetType(c);
            c = codePrev(c) ) {
         assert( NULL != c );
         if ( NULL != codePair(c) )
            c = codePair(c);
         if ( parmsFirstCode(FuncParms) == c
           || StatementEnd == codeGetType(c) ) {
            callError(call,textcoreNO_MATCHING_CONDITIONAL_OR_CASE,
                      parmsGetName(FuncParms));
            success = False;
            break;
         }
      }
      if ( c == codePrev(code) ) {
         callError(call,textcoreNO_MATCHING_CONDITIONAL_OR_CASE,
                   parmsGetName(FuncParms));
         success = False;
      } else {
         if ( ConditionalTrue == codeGetType(c) ) {
            (c->OtherHalfOfPair = code)->OtherHalfOfPair = c;
            *Type = ConditionalFalse;
            FuncParms->UnmatchedConditionalTrue--;
         } else {
            *Type = CaseColon;
         }
      }
   }
   *CodePtr = code;
   return success;
}


   struct Code * NEAR_CALL
CompileRightBracket(struct Code *code,struct Call *call,
                    struct FunctionParms * FuncParms,
                    jsebool *success,codeval *Type,
                    struct FunctionParms *const InitFuncParms)
{
   struct Code *c;

   for ( c = codePrev(code); BeginBlock != codeGetType(c); c = codePrev(c) ) {
      assert( NULL != c );
      if ( NULL != codePair(c) )
         c = codePair(c);
      if ( parmsFirstCode(FuncParms) == c ) {
         NoStartBraceFound:
         callError(call,textcoreMISMATCHED_END_BRACE,parmsGetName(FuncParms));
         *success = False;
         assert( NULL != code );
         return code;
      }
   }
   /* this is an error if did find begin block on first
    * function, which has an auto begin block
    */
   if ( FuncParms == InitFuncParms  &&  parmsFirstCode(FuncParms) == c )
   {
      goto NoStartBraceFound;
   }
   FuncParms->UnmatchedBeginBlock--;
   *Type = EndBlock;
   c->OtherHalfOfPair = code;
   code->OtherHalfOfPair = c;
   if ( parmsFirstCode(FuncParms) == c ) {
      /* restore context to what it was before code cards */
      codeSetType(code,EndBlock);
      /* want to know if source file name has changed, so find  */
      functionparmsDelete(FuncParms,call);
      code = NULL;
   } else {
      assert( NULL != code );
   }
   return code;
}


   codeval NEAR_CALL
CompileRightParentheses(struct Code *this,struct Call *call,
                        struct FunctionParms * const FuncParms,
                        jsebool *success)
{
   codeval Type;
   struct Code *c;
   /* match to previous unmatched BeginFunctionCall or BeginEvaluationGroup */
   for ( c = codePrev(this);
         EvaluationGroup != (Type=codeGetType(c))
         &&  UnresolvedFunctionCall != Type
         &&  functionCall != Type;
         c = codePrev(c) ) {
      assert( NULL != c );
      if ( NULL != codePair(c) )
         c = codePair(c);
      if ( parmsFirstCode(FuncParms) == c ) {
         callError(call,textcoreMISMATCHED_END_PAREN,parmsGetName(FuncParms));
         *success = False;
         return UnknownCardType;
      }
   }
   assert( EvaluationGroup == Type || UnresolvedFunctionCall == Type \
        || functionCall == Type );
   if ( EvaluationGroup == Type ) {
      Type = EndEvaluationGroup;
      FuncParms->UnmatchedBeginEvaluationGroup--;
   } else {
      struct Code *cc;

      Type = EndFunctionCall;
      FuncParms->UnmatchedBeginFunctionCall--;
      /* alter CommaOperator to SeparateFunctionCallArgs */
      for ( cc = codePrev(this); cc != c; cc = codePrev(cc) )
      {
         assert( NULL != cc );
         if ( popEval == codeGetType(cc) )
         {
            codeSetType(cc,getParamValue);
         } 
         else if ( NULL != codePair(cc) ) 
         {
            cc = codePair(cc);
         }
         else if ( opBitAnd == codeGetType(cc) &&
            ( c == codePrev(cc) || popEval == codeGetType(codePrev(cc)) ) ) 
         {
            codeSetType(cc,passByReference);
         }
      }
   }
   c->OtherHalfOfPair = this;
   this->OtherHalfOfPair = c;
   return Type;
}


   void NEAR_CALL
CompileRightArrayToken(struct Code *this,struct Call *call,
                       struct FunctionParms * const FuncParms,jsebool *success)
{
   /* Match to the begin array.
    * Must match outside of ExpressionGroups and inside CurrentFunction.
    */
   struct Code *c;
   for ( c = codePrev(this); structureMember != codeGetType(c);
         c = codePrev(c) )
   {
      assert( NULL != c );
      if ( NULL != codePair(c) )
         c = codePair(c);
      if ( parmsFirstCode(FuncParms) == c ) {
         callError(call,textcoreMISMATCHED_END_BRACKET,parmsGetName(FuncParms));
         *success = False;
         return;
      }
   }
   c->OtherHalfOfPair = this;
   this->OtherHalfOfPair = c;
   FuncParms->UnmatchedBeginArrayIndex--;
}


#ifndef NDEBUG
   static void NEAR_CALL
AnalCodeLinkTest(struct Code *ThisCode)
{
   struct Code *code;
   assert( NULL != ThisCode );
   /* look backwards for valid links */
   for ( code = ThisCode; NULL != code; code = code->PreviousCodeCard )
   {
      if ( NULL != code->PreviousCodeCard )
      {
         assert( code->PreviousCodeCard->NextCodeCard == code );
      }
      if ( NULL != code->NextCodeCard )
      {
         assert( code->NextCodeCard->PreviousCodeCard == code );
      }
   }
   /* look forwards for valid links */
   for ( code = ThisCode; NULL != code; code = code->NextCodeCard )
   {
      if ( NULL != code->NextCodeCard )
      {
         assert( code->NextCodeCard->PreviousCodeCard == code );
      }
      if ( NULL != code->PreviousCodeCard )
      {
         assert( code->PreviousCodeCard->NextCodeCard == code );
      }
   }
}
#endif

#define PHONY_NULL_TYPE  ((codeval)(-1))
#define PHONY_TRUE_TYPE  ((codeval)(-2))
#define PHONY_FALSE_TYPE ((codeval)(-3))

CONST_DATA(struct KeyWords_) KeyWords[] = {
   {textcorevtype_null,PHONY_NULL_TYPE},
   {textcorevtype_bool_true,PHONY_TRUE_TYPE},
   {textcorevtype_bool_false,PHONY_FALSE_TYPE},
   {textcoreVariableKeyword,declareVar},
   {textcoreNewKeyword,functionNew},
   {textcoreIfKeyword,statementIf},
   {textcoreElseKeyword,statementElse},
   {textcoreSwitchKeyword,statementSwitch},
   {textcoreCaseKeyword,SwitchCase},
   {textcoreDefaultKeyword,SwitchDefault},
   {textcoreWhileKeyword,statementWhile},
   {textcoreDoKeyword,statementDo},
   {textcoreForKeyword,statementFor},
   {textcoreInKeyword,statementIn},
   {textcoreDeleteKeyword,opDelete},
   {textcoreTypeofKeyword, opTypeof},
   {textcoreVoidKeyword,opVoid},
   {textcoreWithKeyword, withVar},
   {textcoreBreakKeyword,statementBreak},
   {textcoreContinueKeyword,statementContinue},
   {textcoreGotoKeyword,gotoAlways},
   {textcoreReturnKeyword,returnVar},
   {textcoreFunctionKeyword,DeclareFunction},
#  if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
      {textcoreCFunctionKeyword,DeclareCFunction},
#  endif
   {textcoreCatchKeyword, UnknownCardType},
   {textcoreClassKeyword, UnknownCardType},
   {textcoreConstKeyword, UnknownCardType},
   {textcoreDebuggerKeyword, UnknownCardType},
   {textcoreEnumKeyword, UnknownCardType},
   {textcoreExportKeyword, UnknownCardType},
   {textcoreExtendsKeyword, UnknownCardType},
   {textcoreFinallyKeyword, UnknownCardType},
   {textcoreImportKeyword, UnknownCardType},
   {textcoreSuperKeyword, UnknownCardType},
   {textcoreThrowKeyword, UnknownCardType},
   {textcoreTryKeyword, UnknownCardType},
   {NULL}
};

   codeval NEAR_CALL
GetVariableNameOrKeyword(struct Code *this,struct Call *call,jsechar **RetSource)
{
   jsechar *Start, *End;
   codeval Type;
   sint VarNameLen;
   struct KeyWords_ const *Key;

   assert( 0 != **RetSource );
   assert( !IS_WHITESPACE(**RetSource) );
   assert( NULL == strchr_jsechar(IllegalVariableChars,**RetSource) );
   for ( End = Start = *RetSource;
         0 != *End  &&  !IS_WHITESPACE(*End)
         &&  NULL == strchr_jsechar(IllegalVariableChars,*End);
         End++ )
      ;
   assert( End != Start );
   VarNameLen = (sint)(End - Start);
   assert( 0 < VarNameLen );

   for ( Key = KeyWords; NULL != Key->Word; Key++ )
   {
      if ( 0 == memcmp(Start,(void*)Key->Word,sizeof(jsechar)*VarNameLen)
        && 0 == Key->Word[VarNameLen] )
         break;
   }

   if ( NULL != Key->Word )
   {
      if( (Type = Key->Type) == UnknownCardType )
      {
         jsechar buffer[100];
         if( VarNameLen > 99 ) VarNameLen = 99;
         strncpy_jsechar(buffer,Start,(size_t)VarNameLen);
         buffer[VarNameLen] = 0;
         callError(call,textcoreRESERVED_KEYWORD,buffer);
         *RetSource = End - 1;
         codeSetNameText(this,call,buffer);
         return pushVariable;
      }
      if ( Type < 0 )
      {
         /* not really a variable type, but a keyword */
         VarRead *v = constructVarRead( call,
            (VarType)( PHONY_NULL_TYPE == Type? VNull : VBoolean) );

         assert( PHONY_NULL_TYPE==Type \
              || PHONY_TRUE_TYPE==Type \
              || PHONY_FALSE_TYPE==Type );
         if ( PHONY_TRUE_TYPE == Type )
         {
            varPutBoolean(v,True);
         }
         assert( (PHONY_TRUE_TYPE==Type && varGetLong(v)) \
              || (PHONY_TRUE_TYPE!=Type && !varGetLong(v)));
         codeSetVar(this,v);
         Type = pushValue;
         assert( pushValue == codeGetType(this) );
      }
#     if !defined(JSE_RETURN_AUTOMATIC_SEMICOLON) || (0!=JSE_RETURN_AUTOMATIC_SEMICOLON)
      /* ecmascript has unusual rule about autamitcally inserting a semicolon on
       * return statements if followed by newline or '}'.  That rule goes here.
       */
      else if ( returnVar == Type )
      {
         /* automatic semicolon insertion if end-of-line or '}' */
         jsechar *Next = End;
         SKIP_WHITESPACE(Next);
         if ( 0 == *Next  ||  '}' == *Next ) {
            /* automatically sneak in a semicolon */
            *(--End) = ';';
         }
      }
#     endif
   }
   else
   {
      /* allocate memory and save this variable name */
      jsechar c = Start[VarNameLen];

      Type = pushVariable;
      Start[VarNameLen] = '\0';
      codeSetNameText(this,call,Start);
      Start[VarNameLen] = c;
   }

   *RetSource = End - 1;
   return Type;
}


#ifndef NDEBUG
struct MostRecentPairStart_ {
   struct MostRecentPairStart_ *Prev;
   struct Code *code;
};
/* verify that all pairs have a matching pair, and that no pairs extend past
 * their outer pairs, i.e., ({()})  and not ({(}))
 */
   static void NEAR_CALL
AnalPairingCheck(struct Call *call,struct Code *Start,struct Code *End,
                 VarName FunctionName)
{
   struct Code *BeyondEnd = codeNext(End);
   struct MostRecentPairStart_ *MostRecentPairStart = NULL;
   struct Code *c;
   struct MostRecentPairStart_ *MRPS;

   assert( NULL !=  End );
   for ( c = Start; c != BeyondEnd; c = codeNext(c) )
   {
      assert( NULL != c );
      if ( NULL != codePair(c) )
      {
         if ( NULL != MostRecentPairStart 
           && codePair(MostRecentPairStart->code) == c )
         {
            /* this matches most recent start, and so remove it */
            MRPS = MostRecentPairStart;
            MostRecentPairStart = MostRecentPairStart->Prev;
            jseMustFree(MRPS);
         } else {
            MRPS = jseMustMalloc(struct MostRecentPairStart_,
                                 sizeof(struct MostRecentPairStart_));
            MRPS->Prev = MostRecentPairStart;
            MRPS->code = c;
            MostRecentPairStart = MRPS;
         }
      }
   }
   if ( NULL != MostRecentPairStart )
   {
      callFatalID(call,textcoreUNMATCHED_CODE_CARD_PAIRS,
                  GetStringTableEntry(call,FunctionName,NULL));
   }
}
#endif


   void NEAR_CALL
codeUnlink(struct Code *this,struct Source *source,struct Call *call)
{
   if ( NULL != this->PreviousCodeCard )
   {
      this->PreviousCodeCard->NextCodeCard = NULL;
   }
   if ( NULL == SOURCE_FILENAME(source) )
   {
      this->PreviousCodeCard = NULL;
   }
   else
   {
      /* create two cards before this one to keep source information */
      struct Code *code;
      NewSourceFileName(code = codeNew(NULL),call,source);
      NewSourceLineNumber(code = codeNew(code),call);
      code->NextCodeCard = this;
      this->PreviousCodeCard = code;
   }
}

   struct FunctionParms * NEAR_CALL
functionparmsNew(struct Call *call,struct Code *iFirstCode,
                 struct Code * * LastCodeBeforeFunction,
                 jsebool InitializationFunction,
#                if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                    jsebool CFunction,
#                endif
                 struct Source *source,jsebool *iSuccessFlag)
{
   struct FunctionParms *this
      = jseMustMalloc(struct FunctionParms,sizeof(struct FunctionParms));
   struct Code *BeginFunctionCallCode;

#  if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
      this->CFunction = CFunction;
#  endif
   this->pFirstCode = iFirstCode;
   this->SuccessFlag = iSuccessFlag;

#  ifndef NDEBUG
      AnalCodeLinkTest(parmsFirstCode(this));
#  endif
   if ( InitializationFunction )
   {
      assert( NULL == codePrev(iFirstCode) );
      BeginFunctionCallCode = iFirstCode;
      /* this is the global initialization function */
      this->pName = GLOBAL_STRING(call,init_function_entry);
   } else {
      struct Code *EndFunctionCallCode;
      struct Code *c;
      sint ParameterCount;


      EndFunctionCallCode = codePrev(iFirstCode);
      assert( EndFunctionCall == codeGetType(EndFunctionCallCode) );
      BeginFunctionCallCode = codePair(EndFunctionCallCode);
      assert( NULL != BeginFunctionCallCode \
           && EndFunctionCallCode == codePair(BeginFunctionCallCode) );
      assert( UnresolvedFunctionCall == codeGetType(BeginFunctionCallCode)  ||
              functionCall == codeGetType(BeginFunctionCallCode) );
      *LastCodeBeforeFunction = BeginFunctionCallCode->PreviousCodeCard;
      assert( NULL != *LastCodeBeforeFunction );
      this->pName = codeGetName(BeginFunctionCallCode);

      /* for convenience, now is a good time to check that the parameters
       * passed in to this function are the valid way that parameters come
       * in (variable names separated only by commas; no environment) and,
       * for convenience later, to remove those darned comma operator code
       * cards.
       */
      for ( c = codeNext(BeginFunctionCallCode), ParameterCount = 1;
            pushVariable == codeGetType(c) || 
            passByReference == codeGetType(c);
            ParameterCount++ )
      {
         if( passByReference == codeGetType(c) &&
            pushVariable != codeGetType(c = codeNext(c)) )
           break;
        /* either comma specifies another code card, or we're out of here */
         if ( getParamValue != codeGetType(c = codeNext(c)) )
            break;
         /* delete the comma code card for convenience later on */
         c = codeNext(c);
         codeDelete(codePrev(c),call);
      }
      if ( c != EndFunctionCallCode )
      {
         callError(call,textcoreINVALID_PARAMETER_DECLARATION,
                   ParameterCount,parmsGetName(this));
         *(this->SuccessFlag) = False;
      }
   }

   /* these code cards for the first function must be unlinked so they are
   * separate lists, and line number must be saved
    */
   codeUnlink(BeginFunctionCallCode,source,call);

   this->UnmatchedBeginBlock =
      this->UnmatchedBeginFunctionCall =
      this->UnmatchedBeginEvaluationGroup =
      this->UnmatchedBeginArrayIndex =
      this->UnmatchedConditionalTrue = 0;

   return this;
}


   void NEAR_CALL
functionparmsDelete(struct FunctionParms *this,struct Call *call)
{
   enum textcoreID Description = (enum textcoreID) 0;
   jsechar Start = '{', End = '}';


#  ifndef NDEBUG
      AnalCodeLinkTest(parmsFirstCode(this));
#  endif
   if ( 0 != this->UnmatchedBeginBlock )
      Description = textcoreBLOCK_TOKEN_MISSING, Start = '{', End = '}';
   else if ( 0 != this->UnmatchedBeginFunctionCall )
      Description = textcoreFUNCTION_CALL_TOKEN_MISSING, Start = '(', End = ')';
   else if ( 0 != this->UnmatchedBeginEvaluationGroup )
      Description = textcoreGROUPING_TOKEN_MISSING, Start = '(', End = ')';
   else if ( 0 != this->UnmatchedBeginArrayIndex )
      Description = textcoreARRAY_TOKEN_MISSING, Start = '[', End = ']';
   else if ( 0 != this->UnmatchedConditionalTrue )
      Description = textcoreCONDITIONAL_TOKEN_MISSING, Start = '?', End = ':';
   if ( 0 != Description )
   {
      jsechar DescriptionBuf[200];

      strcpy_jsechar(DescriptionBuf,textcoreGet(Description));
      assert( NULL != call );
      callError(call,textcoreTOKEN_MISSING,DescriptionBuf,Start,
                parmsGetName(this),End);
      *(this->SuccessFlag) = False;
   }

#  ifndef NDEBUG
   if ( !callReasonToQuit(call) )
   {
      /* don't check for error if we already have a reason to quit */   
      AnalPairingCheck(call,parmsFirstCode(this),
                       codePair(parmsFirstCode(this)),parmsGetName(this));
   }
#  endif

   if ( *(this->SuccessFlag) )
   {
      /* This call links itself into a pre-created list,
       * so the value is saved
       */
      struct LocalFunction *lfunc = localNew(call,parmsGetName(this),
         parmsFirstCode(this)
#        if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
            ,this->CFunction
#        endif                                       
      );
      if ( !localResolveVariableNames(lfunc,call) )
      {
         /* function failed; so delete the variable that is this function */
         VarRead * funcVar = GetDotNamedVar(call,call->session.GlobalVariable,
                                            GetStringTableEntry(call,parmsGetName(this),NULL),True);
// seb 99.2.3 - This fix is from Nombas.
		 if (this->pName != GLOBAL_STRING(call,init_function_entry) &&
			 this->pName != GLOBAL_STRING(call,main_entry))
            VAR_REMOVE_USER(funcVar,call);
         *(this->SuccessFlag) = False;
      }
   }
   else
   {
      codeFreeAllMemory(parmsFirstCode(this),call);
   }

   jseMustFree(this);
}

#endif /* #if (0!=JSE_COMPILER) */
