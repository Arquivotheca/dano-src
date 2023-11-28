/* Code.h   Determine next code card from the input string.
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

#if !defined(_CODE_H) && (0!=JSE_COMPILER)
#define _CODE_H
#if defined(__cplusplus)
   extern "C" {
#endif

union CodeData
{
   struct Var *varPtr; /* only for Type() == Variable                    */
   uint lineNumber;    /* only for Type() == SourceFileLineNumber        */
   VarName name;       /* Type is not determined                         */
};


struct Code
{
   struct Code * PreviousCodeCard;
   struct Code * NextCodeCard;
   struct Code * OtherHalfOfPair;
      /* some cards come in pairs, and this points to
       * the other half ({},(),and ?:) and Begin- EndCode
       */
   union CodeData Data;
   codeval pType;
};


extern CONST_DATA(jsechar) IllegalVariableChars[];

struct Source;
struct FunctionParms;

void NEAR_CALL NewSourceFileName(struct Code *code,struct Call *call,
                                 struct Source *source);
void NEAR_CALL NewSourceLineNumber(struct Code *code,struct Call *call);

jsebool CompileFromText(struct Call *call,jsechar ** SourceText,
                        jsebool SourceIsFileName);
   /* Read the command string and code source and compile into linked cards.
    * If SourceFileName is NULL, then will get the source from the command-line
    * input to cenvi.  Return allocated pointer to filespec if
    * MaybeSourceFileName was the name of a CEnvi-type source file, meaning that
    * the rest of the command line is args to main(), else return NULL meaning
    * that the command line was the source code, and no args go to main().
    * If *FreeSourceFileName then the filename must be freed.
    * Return True for success, else will have printed error and there's an
    * error. If SourceIsFileName then *SourceText may be freed and allocated
    * with the complete path of the source file.
    */

struct Code * NEAR_CALL codeNew(struct Code *PrevCodeCard);
   /* PrevCodeCard may be NULL */
void NEAR_CALL codeDelete(struct Code *code,struct Call *call);

void NEAR_CALL codeFreeAllMemory(struct Code *code,struct Call *call);
/* free all memory for this code, going backwards and forward */

struct Code * NEAR_CALL codePrev(struct Code *code);
struct Code * NEAR_CALL codeNext(struct Code *code);

jsechar * NEAR_CALL CompileStringToken(struct Call *call,struct Code **CodePtr,
                    jsechar *src,
#                   if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                       jsebool CFunction,
#                   endif
                    jsebool *success);
jsebool NEAR_CALL CompileColon(struct Call *call,struct Code **CodePtr,
                               codeval *Type,
                               struct FunctionParms * const FuncParms);
struct Code * NEAR_CALL CompileRightBracket(
   struct Code *code,struct Call *call,struct FunctionParms * FuncParms,
   jsebool *success,codeval *Type,
   struct FunctionParms *const InitFuncParms);
   /* return NULL if bracket means end of this function */
codeval NEAR_CALL CompileRightParentheses(
   struct Code *code,struct Call *call,
   struct FunctionParms * const FuncParms,jsebool *success);
void NEAR_CALL CompileRightArrayToken(struct Code *code,struct Call *call,
                                      struct FunctionParms * const FuncParms,
                                      jsebool *success);
codeval NEAR_CALL GetVariableNameOrKeyword(struct Code *code,
                                           struct Call *call,
                                           jsechar **RetSource);
void NEAR_CALL codeUnlink(struct Code *code,struct Source *source,
                          struct Call *call);
   /* unlink this card from previous cards, so previous ends
    * and this is a new beginning
    */

void NEAR_CALL codeSetVar(struct Code *code,struct Var *var);
void NEAR_CALL codeSetLine(struct Code *code,uint line);


/* code inline stuff as macros */

#define codeSetType(this,type) ((this)->pType = (type))
#define codeGetVar(this) ((this)->Data.varPtr)
#define codeSetNameText(this,call,nam) \
        ((this)->Data.name = EnterIntoStringTable((call),(nam),strlen_jsechar(nam)))
#define codeSetName(this,nam) ((this)->Data.name = (nam))

#define codeGetName(this) ((this)->Data.name)

#define codeGetLine(this) ((this)->Data.lineNumber)

#define codePair(this) ((this)->OtherHalfOfPair)


#define codeTruePrev(this) ((this)->PreviousCodeCard)
#define codeTrueNext(this) ((this)->NextCodeCard)
#define codeGetType(this) ((this)->pType)


/* function parameters stuff */


struct FunctionParms
{
   jsebool *SuccessFlag; /* Set FALSE (and printf error if not 
                          * already FALSE) if error
                          */
#  if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
      jsebool CFunction;
#  endif
   VarName pName;
   struct Code *pFirstCode;
   sint UnmatchedBeginBlock, UnmatchedBeginFunctionCall,
        UnmatchedBeginEvaluationGroup, UnmatchedBeginArrayIndex,
        UnmatchedConditionalTrue;
};

struct FunctionParms * NEAR_CALL functionparmsNew(
                 struct Call *call,struct Code *iFirstCode,
                 struct Code * * LastCodeBeforeFunction,
                 jsebool InitializationFunction,
#                if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                    jsebool CFunction,
#                endif
                 struct Source *source,jsebool *iSuccessFlag);
void NEAR_CALL functionparmsDelete(struct FunctionParms *,struct Call *call);

#define parmsGetName(this) ((this)->pName)
#define parmsFirstCode(this) ((this)->pFirstCode)

#if (defined(JSE_DEFINE) && (0!=JSE_DEFINE)) \
 || (defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)) \
 || (defined(JSE_LINK) && (0!=JSE_LINK))
   jsebool PreprocessorDirective(struct Source **source,struct Call *call);
#endif

struct KeyWords_
{
   const jsechar *Word;
   /* I changed this to an int because the enum being used was 1 byte
    * unsigned, so -1 was converted to '255' which when converted back
    * was not -1!
    */
   codeval Type;
};

#if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
#  define TYPE_IS_DECLAREFUNC(type) \
     (DeclareFunction==type || DeclareCFunction==type)
#else
#  define TYPE_IS_DECLAREFUNC(type) \
     (DeclareFunction==type)
#endif

extern CONST_DATA(struct KeyWords_) KeyWords[];

jsebool NEAR_CALL MoreMustFollow(codeval code);

#if defined(__cplusplus)
   }
#endif
#endif
