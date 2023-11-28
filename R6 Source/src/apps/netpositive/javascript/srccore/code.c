/* code.c  Determine next code card from the input string.
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

   void NEAR_CALL
codeSetVar(struct Code *this,struct Var *var)
{
   codeSetType(this,pushValue);
   this->Data.varPtr = var;
}

   void NEAR_CALL
codeSetLine(struct Code *this,uint line)
{
   codeSetType(this,sourceLineNumber);
   this->Data.lineNumber = line;
}


   static jsebool NEAR_CALL
FileLocationCode(codeval code)
{
   assert( isValidCodeval(code) );
   return ( sourceLineNumber==code  ||  sourceFilename==code );
}

   struct Code * NEAR_CALL
codePrev(struct Code *This)
{
   struct Code * prev = This->PreviousCodeCard;
   return ( ( (NULL == prev)  ||  (!FileLocationCode(codeGetType(prev))) )
          ? prev : codePrev(prev) );
}


   struct Code * NEAR_CALL
codeNext(struct Code *this)
{
   struct Code *next = this->NextCodeCard;
   return ( ( (NULL == next)  ||  (!FileLocationCode(codeGetType(next))) )
          ? next : codeNext(next) );
}


   void NEAR_CALL
NewSourceFileName(struct Code *this,struct Call *call,struct Source *source)
{
   codeSetType(this,sourceFilename);
   codeSetNameText( this,call,
                    NULL != (call->Global->CompileStatus.CompilingFileName
                             = SOURCE_FILENAME(source))
                  ? SOURCE_FILENAME(source) : UNISTR("") );
}

   void NEAR_CALL
NewSourceLineNumber(struct Code *this,struct Call *call)
{
   assert( NULL != this->PreviousCodeCard );
   codeSetLine(this,call->Global->CompileStatus.CompilingLineNumber);
   if ( sourceLineNumber == codeGetType(this->PreviousCodeCard) ) {
      /* it's silly to have two line numbers in a row,
       * so delete that earlier one
       */
      codeDelete(this->PreviousCodeCard,call);
   } /* endif */
}


   jsebool
CompileFromText(struct Call *call,jsechar ** SourceText,jsebool SourceIsFileName)
{
   struct Code *code, *FirstCode, *RememberLastInitFunctionCode;
   const jsechar *RememberFileName;
   struct Source *source, *PrevSource;
   jsebool success = True;
#  if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
      jsebool InCFunction = CBehavior(call);
#  endif
   struct FunctionParms *InitFuncParms;
   struct FunctionParms *FuncParms;
   uint CommentDepth;
   jsechar *src;


   call->Global->CompileStatus.NowCompiling++;
   assert( 0 != call->Global->CompileStatus.NowCompiling );

#  if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
   if ( SourceIsFileName )
   {
      source = sourceNewFromFile(call,NULL,*SourceText,&success);
      if ( success )
      {
         jseMustFree(*SourceText);
         *SourceText = StrCpyMalloc(SOURCE_FILENAME(source)
                                   ?(SOURCE_FILENAME(source)):UNISTR("stdin"));
      } /* endif */
   }
   else
#  endif   
   {
      /* source is only from text line */
      source = sourceNewFromText(NULL,*SourceText) ;
   } /* endif */

   if( !success )
   {
      if( source ) sourceDelete(source,call);
      return False;
   }

   /* first code card is always BeginBlock */
   codeSetType(FirstCode = codeNew(NULL),BeginBlock);

   InitFuncParms = functionparmsNew(call,FirstCode,
                      &RememberLastInitFunctionCode,
                      True,
#                     if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                        (jsebool)( jseOptDefaultCBehavior
                                 & call->Global->ExternalLinkParms.options),
#                     endif
                      source,&success);
   FuncParms = InitFuncParms;
   FuncParms->UnmatchedBeginBlock++;

   /* always have next code card preallocated to fill */
   NewSourceFileName(code = codeNew(FirstCode),call,source);
   NewSourceLineNumber(code = codeNew(code),call);
   code = codeNew(code);

   CommentDepth = 0;
   for ( src = sourceGetPtr(source); success; )
   {
      if ( 0 == *src ) {
         EndOfSourceLine:
         if ( !sourceNextLine(source,call,&success) )
         {
            RememberFileName = SOURCE_FILENAME(source);
            PrevSource = sourcePrev(source);
            if ( NULL == PrevSource )
            {
               /* end of this Source, so break from the big loop */
               break;
            } /* endif */
            assert( NULL != source );
            sourceDelete(source,call);
            source = PrevSource;
            if ( RememberFileName != SOURCE_FILENAME(source) )
            {
               /* Comments must not span files. */
               if ( CommentDepth )
               {
                  callError(call,textcoreEND_COMMENT_NOT_FOUND);
                  success = False;
               } /* endif */
               NewSourceFileName(code,call,source);
               code = codeNew(code);
            } /* endif */
         } /* endif */
         NewSourceLineNumber(code,call);
         code = codeNew(code);
         src = sourceGetPtr(source);
      } else {
         if ( 0 != CommentDepth ) {
            if ( '*' == *src ) {
               if ( '/' == src[1] ) {
                  CommentDepth--;
                  src++;
               } /* endif */
            } else if ( '/' == *src ) {
               if ( '*' == src[1] ) {
                  /* CommentDepth++; */
                  /* Rich: comments do not nest */
                  src++;
               }
            }
         } else if ( !IS_WHITESPACE(*src) ) {
            codeval Type = UnknownCardType;
            if ( *src == src[1]  &&  NULL != strchr_jsechar(UNISTR("+-<>&|=/"),*src) ) {
               /* Handle certain cases where two of the same character is
                * different than one.
                */
               switch ( *(src++) ) {
                  case '+':   Type = opPreIncrement;     break;
                  case '-':   Type = opPreDecrement;     break;
                  case '<':   Type = opShiftLeft;        break;
                  case '>':   Type = opSignedShiftRight; break;
                  case '&':   Type = LogicalAND;         break;
                  case '|':   Type = LogicalOR;          break;
                  case '=':   Type = opEqual;            break;
                  case '/':   /* Comment to end of line (or end of src) */
                     goto EndOfSourceLine;
                 #ifndef NDEBUG
                  default:
                     assert( JSE_DEBUG_FEEDBACK(False) );
                 #endif
               } /* endswitch */
               if( Type==opSignedShiftRight && src[1]=='>' )
               {
                  src++;
                  Type = opUnsignedShiftRight;
               }
            } else {
               switch( *src ) {
                  case '!':   Type = opBoolNot;                        break;
                  case '-':
                     /* unary or subtraction */
                  case '+':
                     switch ( codeGetType(codePrev(code)) )
                     {
                        case pushValue: case pushVariable:
                        case EndFunctionCall: case EndEvaluationGroup:
                        case EndArrayIndex:
                        case structureMemberName:
                           Type = ( '+' == *src ) ? opAdd : opSubtract ;
                           break;
                        default:
                           /* unary operator */
                           Type = ( '+' == *src ) ? opPositive : opNegative ;
                           break;
                     }
                     break;
                  case '*':
                     if ( '/' == src[1] ) {
                        callError(call,textcoreNO_BEGIN_COMMENT);
                        success = False;
                     } /* endif */
                     Type = opMultiply;
                     break;
                  case '/':
                     if ( '*' == src[1] ) {
                        /* comment - remove up to "* /" */
                        assert( 0 == CommentDepth );
                        CommentDepth = 1;
                        src += 2;
                        continue;
                     } else {
                        Type = opDivide;
                     }
                     break;
                  case '%':   Type = opModulo;                       break;
                  case '&':   Type = opBitAnd;                       break;
                  case '^':   Type = opBitXor;                       break;
                  case '~':   Type = opBitNot;                       break;
                  case '|':   Type = opBitOr;                        break;
                  case '=':   Type = opAssign;                       break;
                  case '<':   Type = opLess;                         break;
                  case '>':   Type = opGreater;                      break;
                  case '?':   Type = ConditionalTrue;
                              FuncParms->UnmatchedConditionalTrue++;  break;
                  case ':':
                     if ( !CompileColon(call,&code,&Type,FuncParms) )
                     {
                        success = False;
                     }
                     break;
                  case ',':   Type = popEval;                        break;
                  case '{':
                  {
                     struct Code *BeforeFunc;
                     codeval BeforeFuncType;

                     if ( codeGetType(codePrev(code))==EndFunctionCall
                       && FuncParms == InitFuncParms
                       && 1 == FuncParms->UnmatchedBeginBlock )
                     {
                        BeforeFunc = codePair(codePrev(code));
                        if( codeGetType(BeforeFunc)==UnresolvedFunctionCall )
                        {
                           BeforeFunc = codePrev(BeforeFunc);
                           BeforeFuncType = codeGetType(BeforeFunc);
                           if( !MoreMustFollow(BeforeFuncType) &&
                               ( !( jseOptReqFunctionKeyword
                                  & call->Global->ExternalLinkParms.options )
                                 || ( DeclareFunction==BeforeFuncType
                                   || DeclareCFunction==BeforeFuncType) ) )
                           {
                              /* this must be a new function declaration */
                              if ( TYPE_IS_DECLAREFUNC(BeforeFuncType) ) {
                                 /* keyword has been used up; Remove it. */
#                        if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                                 InCFunction
                                 = (DeclareCFunction==BeforeFuncType);
#                        endif
                                 codeDelete(BeforeFunc,call);
                              } /* endif */
                              FuncParms
                              = functionparmsNew(call,code,
                                   &RememberLastInitFunctionCode,
                                   False,
#                       if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                                   ( jseOptDefaultCBehavior
                                   & call->Global->ExternalLinkParms.options )
                                   || DeclareCFunction==BeforeFuncType,
#                       endif
                                   source,&success);
                           }
                        }
                     }
                     Type = BeginBlock;
                     FuncParms->UnmatchedBeginBlock++;
                  }  break;
                  case '}':  /* find code card this matches to, jumping around
                              * matched () or {}
                              */
                     if ( NULL == (code = CompileRightBracket(code,call,
                                   FuncParms,&success,&Type,InitFuncParms)) ) {
                        /* end of that function; back to global
                         * initialization function
                         */
                        FuncParms = InitFuncParms;
                        Type = codeGetType(code = RememberLastInitFunctionCode);
#                       if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                           InCFunction = CBehavior(call);
#                       endif
                     }
                     break;
                  case ';':   Type = StatementEnd;                   break;
                  case '(':
                     if ( pushVariable == (Type = codeGetType(codePrev(code))) )
                     {
                        /* Beginning of a new function */
                        code = codePrev(code);
                        codeDelete(codeNext(code),call);
                        Type = UnresolvedFunctionCall;
                        FuncParms->UnmatchedBeginFunctionCall++;
                     } else if ( structureMemberName == Type
                              || EndArrayIndex == Type
                              || EndFunctionCall == Type ) {
                        /* only reason I know for no_more_to_follow is one
                         * of structure members operators
                         */
                        Type = functionCall;
                        FuncParms->UnmatchedBeginFunctionCall++;
                     } else {
                        Type = EvaluationGroup;
                        FuncParms->UnmatchedBeginEvaluationGroup++;
                     }
                     break;
                  case ')':
                     Type = CompileRightParentheses(code,call,
                                                    FuncParms,&success);
                     break;
                  case '[':
                     Type = structureMember;
                     FuncParms->UnmatchedBeginArrayIndex++;
                     break;
                  case ']':
                     CompileRightArrayToken(code,call,FuncParms,&success);
                     Type = EndArrayIndex;
                     break;
                  case '\'': /* these are the same except that single quote */
                  case '\"': /* can be a non-array and if more than one     */
                  case '`':  /*  then it doesn't end in null. And back-tick */
                             /* means no escape sequences                   */
                     src = CompileStringToken(call,&code,src,
#                       if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                                              InCFunction,
#                       endif
                                              &success);
                     Type = pushValue;
                     break;
#                 if (defined(JSE_DEFINE) && (0!=JSE_DEFINE)) \
                  || (defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)) \
                  || (defined(JSE_LINK) && (0!=JSE_LINK))
                  case '#':
                     sourceSetPtr(source,src);
                     RememberFileName = SOURCE_FILENAME(source);
                     if ( !PreprocessorDirective(&source,call) )
                     {
                        success = False;
                        break;
                     }
                     src = sourceGetPtr(source);
                     if ( RememberFileName != SOURCE_FILENAME(source) )
                     {
                        /* new source, and so make card for a file
                         * name and line number
                         */
                        assert( NULL != SOURCE_FILENAME(source) );
                        NewSourceFileName(code,call,source);
                        NewSourceLineNumber(code = codeNew(code),call);
                        code = codeNew(code);
                     }
                     continue;
#                 endif
                  case '.':
                     if ( ( '0' <= src[+1]  &&  src[+1] <= '9' ) ) {
                        /* this is part of a number */
#                       if defined(JSE_FLOATING_POINT)&&(0!=JSE_FLOATING_POINT)
                        goto srcNumber;
#                       else
                        callError(call,textcoreNO_FLOATING_POINT);
                        success = False;
#                       endif
                     } else {
                        Type = structureMemberName;
                     } /* endif */
                     break;
                  case '0':case '1':case '2':case '3':case '4':
                  case '5':case '6':case '7':case '8':case '9':
#                 if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
                  srcNumber:
#                 endif
                  {
                     VarRead *var;
                     jsebool AssignSuccess;
                     var = constructVarRead(call,VUndefined);
                     if( !AssignFromText(var,call,src,
                                         &AssignSuccess,False,&src) )
                        success = False;
                     assert( VNumber == VAR_TYPE(var) );
                     codeSetVar(code,var);
                     src--; /* will have gone 1 too many */
                     Type = pushValue;
                     break;
                  }
                  default:
                     if( NULL != strchr_jsechar(IllegalVariableChars,*src) )
                     {
                        success = False;
                        callError(call,textcoreBAD_CHAR,*src,*src);
                        break;
                     }

                     Type = GetVariableNameOrKeyword(code,call,&src);
                     if( Type==UnknownCardType )
                       {
                         success = False;
                         break;
                       }

                     if ( pushVariable == Type ) {
#                       if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
                        const jsechar *ReplaceSrc;
                        if ( NULL != (ReplaceSrc =
                            defineFindReplacement(call->session.Definitions,
                               GetStringTableEntry(call,codeGetName(code),NULL))) )
                        {
                           /* source points to new code to replace for old */
                           sourceSetPtr(source,src+1);
                           source = sourceNewFromText(source,ReplaceSrc);
                           src = sourceGetPtr(source);
                           /* delete the code that THOUGHT it was a variable */
                           codeSetType(code,Type);
                              /* so deleting will free Name() */
                           code = codeNew(code);
                           codeDelete(codePrev(code),call);
                           continue;
                        }
                        else
#                       endif
                        {
                           struct Code *PrevCode = codePrev(code);
                           codeval PrevType = codeGetType(PrevCode);
                           if ( NULL == codeGetName(PrevCode)
                             && ( gotoAlways == PrevType
                               || structureMemberName == PrevType ) ) {

                              /* in some cases, what looks like a variable
                               * belongs on previous variable
                               */
                              Type = PrevType;
                              codeDelete(PrevCode,call);

                              if ( structureMemberName == PrevType )
                              {
                                 /* to handle the special case of allowing
                                  * statements such as
                                  * "function foo.goo() { }"
                                  * in our code we will check if the predicing
                                  * code is <function>|<cfunction> name." and
                                  * if se we'll remove the "." token and append
                                  * this token to the <name> code
                                  */
                                 assert( NULL != codePrev(code) );
                                 if ( pushVariable == codeGetType(PrevCode
                                                          = codePrev(code)) ) {
                                    assert( NULL != codePrev(PrevCode) );
                                    if ( DeclareFunction == (PrevType
                                             = codeGetType(codePrev(PrevCode)))
                                      || DeclareCFunction == PrevType )
                                    {
                                       /* combine prev code with this code */
                                       const jsechar * oldname
                                       = GetStringTableEntry(
                                            call,codeGetName(PrevCode),NULL);
                                       const jsechar * newext
                                       = GetStringTableEntry(
                                            call,codeGetName(code),NULL);
                                       jsechar * newname = jseMustMalloc(jsechar,
                                          sizeof(jsechar)*(strlen_jsechar(oldname)+1+strlen_jsechar(newext)+1));
                                       strcat_jsechar(strcat_jsechar(strcpy_jsechar(newname,oldname),
                                                     UNISTR(".")),
                                              newext);
                                       RemoveFromStringTable(call,codeGetName(code));
                                       codeSetNameText(code,call,newname);
                                       jseMustFree(newname);
                                       Type = pushVariable;
                                       codeDelete(PrevCode,call);
                                    }
                                 }
                              }
                           }
                        }
                     }
                     break;
               }
            }
            /* Lots of code cards can be followed by '=' to make them mean
             * something else.  Handle those here.
             */
            if ( '=' == src[1] ) {
               src++;
               switch ( Type ) {
                  case opMultiply:     Type = opAssignMultiply;      break;
                  case opDivide:       Type = opAssignDivide;        break;
                  case opModulo:       Type = opAssignModulo;        break;
                  case opAdd:          Type = opAssignAdd;           break;
                  case opSubtract:     Type = opAssignSubtract;      break;
                  case opShiftLeft:    Type = opAssignShiftLeft;     break;
                  case opSignedShiftRight:
                                       Type = opAssignSignedShiftRight;  break;
                  case opUnsignedShiftRight:
                                       Type = opAssignUnsignedShiftRight;break;
                  case opBitAnd:       Type = opAssignBitAnd;        break;
                  case opBitXor:       Type = opAssignBitXor;        break;
                  case opBitOr:        Type = opAssignBitOr;         break;
                  case opLess:         Type = opLessEqual;           break;
                  case opGreater:      Type = opGreaterEqual;        break;
                  case opBoolNot:      Type = opNotEqual;            break;
                  default: /* not special */ src--;               break;
               }
            }
            codeSetType(code,Type);
            code = codeNew(code);
         }
         src++;
      }
   }

   /* if still within a comment, then error */
   if ( 0 != CommentDepth ) {
      callError(call,textcoreEND_COMMENT_NOT_FOUND);
      success = False;
   }

   /* if still within a function, then error */
   if ( FuncParms != InitFuncParms ) {
      callError(call,textcoreFUNCTION_IS_UNFINISHED,parmsGetName(FuncParms));
      functionparmsDelete(FuncParms,call);
      success = False;
   }

   /* last code card will be an EndBlock for initialization function */
   codeSetType(code,EndBlock);
   InitFuncParms->UnmatchedBeginBlock--;

   /* First and Last code card are matching halfs */
   code->OtherHalfOfPair = FirstCode;
   FirstCode->OtherHalfOfPair = code;

#  ifndef NDEBUG
      if ( !callReasonToQuit(call) )
      {
         assert( NULL == codePrev(FirstCode) \
              && BeginBlock == codeGetType(FirstCode) );
         assert( NULL != codePair(FirstCode) \
              && FirstCode == codePair(codePair(FirstCode)) );
         assert( NULL == codeNext(codePair(FirstCode)) \
              && EndBlock == codeGetType(codePair(FirstCode)) );
      }
#  endif
   functionparmsDelete(InitFuncParms,call);

   /* tell error reporting that no longer compiling */
   call->Global->CompileStatus.CompilingFileName = NULL;

   /* delete all source objects */
   assert( NULL != source );
   do {
      PrevSource = sourcePrev(source);
      sourceDelete(source,call);
   } while ( NULL != (source = PrevSource) );

   assert( 0 != call->Global->CompileStatus.NowCompiling );
   call->Global->CompileStatus.NowCompiling--;

   return success;
}


   struct Code * NEAR_CALL
codeNew(struct Code *PrevCodeCard)
{
   struct Code *this = jseMustMalloc(struct Code,sizeof(struct Code));

   codeSetType(this,UnknownCardType);
   this->OtherHalfOfPair = NULL;
   if ( NULL != PrevCodeCard ) {
      if ( NULL != (this->NextCodeCard = PrevCodeCard->NextCodeCard) ) {
         /* anal check that code cards are properly linked */
         assert( this->NextCodeCard->PreviousCodeCard == PrevCodeCard );
         this->NextCodeCard->PreviousCodeCard = this;
      }
      PrevCodeCard->NextCodeCard = this;
   } else {
      this->NextCodeCard = NULL;
   }
   this->Data.name = NULL;
   this->PreviousCodeCard = PrevCodeCard;

   return this;
}


   void NEAR_CALL
codeDelete(struct Code *this,struct Call *call)
{
   switch ( codeGetType(this) ) {
      case sourceFilename:
      case Label:
      case gotoAlways:
      case UnresolvedFunctionCall:
      case structureMemberName:
      case pushVariable:
      case functionCall:
         if( NULL != codeGetName(this) )
            RemoveFromStringTable(call,codeGetName(this));
         break;
      case pushValue:
      {
         Var *v;
         assert( NULL != codeGetVar(this) );
         /* its a constant, so nothing should be turning it into an lvalue */
         assert( !varIsLvalue(codeGetVar(this)) );
         v = codeGetVar(this);
         VAR_REMOVE_USER(v,call);
      }  break;
      default: /* no other cases have malloc'ed this pointer */
         break;
   }
   if ( NULL != this->PreviousCodeCard )
   {
      assert( this == this->PreviousCodeCard->NextCodeCard );
      this->PreviousCodeCard->NextCodeCard = this->NextCodeCard;
   }
   if ( NULL != this->NextCodeCard )
   {
      assert( this == this->NextCodeCard->PreviousCodeCard );
      this->NextCodeCard->PreviousCodeCard = this->PreviousCodeCard;
   }
   jseMustFree(this);
}


   void NEAR_CALL
codeFreeAllMemory(struct Code *this,struct Call *call)
{
   /* backup to first code in this linked list */
   struct Code *code, *next;
   for ( code = this;
         NULL != code->PreviousCodeCard;
         code = code->PreviousCodeCard )
      ;
   /* free all cards from code to end */
   do {
      next = code->NextCodeCard;
      codeDelete(code,call);
   } while ( NULL != (code = next) );
}

#endif /* #if (0!=JSE_COMPILER) */
