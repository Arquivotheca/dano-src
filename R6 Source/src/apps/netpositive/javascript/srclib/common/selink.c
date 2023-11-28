/* selink.c  Support for link libraries
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
#include "selink.h"

#if defined(JSETOOLKIT_LINK)

#  if !defined(__JSE_WIN16__) && !defined(__JSE_WIN32__) && !defined(__JSE_CON32__) && \
      !defined(__JSE_OS2TEXT__) && !defined(__JSE_OS2PM__) && !defined(__JSE_UNIX__) && \
      !defined(__JSE_MAC__)  && !defined(__JSE_NWNLM__)
#    error Extensions are not yet supported on this platform
#  endif

static CONST_DATA(jsechar) LinkFuncNotSupported[] = UNISTR("6010: Application does not support function \"%s\"/");
#  define FUNCTION_SUPPORTED(name,returnOnError);                          \
      if(!FunctionIsSupported(jsecontext,name))                            \
      {                                                                    \
         jseLibErrorPrintf(jsecontext,LinkFuncNotSupported,#name); \
         returnOnError;                                                    \
      }

/********************************************************************/
/*                jse Extension Wrapper functions                   */
/* Cast the jseContext passed to the jse Extension to a Call class  */
/********************************************************************/

#  if defined(__JSE_WIN16__)
#ifdef _cplusplus
extern "C"
#endif
uword32 JSE_CFUNC _FAR_ DispatchToHost(uword16 DataSegment, void * FuncHost, uint WordCnt,...);
#    define STACKREG_SIZE(FOO)  ((sizeof(FOO)+1)/2)
#    define PUSH_1_PARM(A)               STACKREG_SIZE(A),A
#    define PUSH_2_PARM(A,B)             STACKREG_SIZE(A)+STACKREG_SIZE(B),A,B
#    define PUSH_3_PARM(A,B,C)           STACKREG_SIZE(A)+STACKREG_SIZE(B)+STACKREG_SIZE(C),A,B,C
#    define PUSH_4_PARM(A,B,C,D)         STACKREG_SIZE(A)+STACKREG_SIZE(B)+STACKREG_SIZE(C)+STACKREG_SIZE(D),A,B,C,D
#    define PUSH_5_PARM(A,B,C,D,E)       STACKREG_SIZE(A)+STACKREG_SIZE(B)+STACKREG_SIZE(C)+STACKREG_SIZE(D)+STACKREG_SIZE(E),A,B,C,D,E
#    define PUSH_6_PARM(A,B,C,D,E,F)     STACKREG_SIZE(A)+STACKREG_SIZE(B)+STACKREG_SIZE(C)+STACKREG_SIZE(D)+STACKREG_SIZE(E)+STACKREG_SIZE(F),A,B,C,D,E,F
#    define PUSH_7_PARM(A,B,C,D,E,F,G)   STACKREG_SIZE(A)+STACKREG_SIZE(B)+STACKREG_SIZE(C)+STACKREG_SIZE(D)+STACKREG_SIZE(E)+STACKREG_SIZE(F)+STACKREG_SIZE(G),A,B,C,D,E,F,G
#    define PUSH_8_PARM(A,B,C,D,E,F,G,H) STACKREG_SIZE(A)+STACKREG_SIZE(B)+STACKREG_SIZE(C)+STACKREG_SIZE(D)+STACKREG_SIZE(E)+STACKREG_SIZE(F)+STACKREG_SIZE(G)+STACKREG_SIZE(H),A,B,C,D,E,F,G,H
#    define PUSH_9_PARM(A,B,C,D,E,F,G,H,I) STACKREG_SIZE(A)+STACKREG_SIZE(B)+STACKREG_SIZE(C)+STACKREG_SIZE(D)+STACKREG_SIZE(E)+STACKREG_SIZE(F)+STACKREG_SIZE(G)+STACKREG_SIZE(H),A,B,C,D,E,F,G,H,I
#  endif

#  define jseFuncs(CC) (**((struct jseFuncTable_t ** _FAR_ *)CC))

/* ---------------------------------------------------------------------- */

JSEEXTNSN_EXPORT(long) jseExtensionVer(jseContext jsecontext)
{
   UNUSED_PARAMETER(jsecontext);
   return JSEEXTERNALVER;
}

JSEEXTNSN_EXPORT(jsebool) jseLoadExtension(jseContext jsecontext)
{
   if( jsecontext==NULL ) return False;
   if( !jseCheckCEnviVersion(jsecontext))
   {
      jseLibErrorPrintf(jsecontext, UNISTR("This is an outdated SE host"));
      return False;
   }

   return ExtensionLoadFunc(jsecontext);
}

/* ---------------------------------------------------------------------- */

   jsebool JSE_CFUNC
jseCheckCEnviVersion(jseContext jsecontext)
{

   /* Compare External Lib size & version with the Loading interpreter's size & version.
      External Lib version can be greater than or equal to the loading interpreter.
      External Lib funct table size can be less than or equal to the loading interpreter.
      External Lib version is held in (& controlled by the toolkit version.
   */
   if(JSEEXTERNALVER > jseFuncs(jsecontext)->Version)
   {
      return False;
   }
   if(sizeof(struct jseFuncTable_t) < jseFuncs(jsecontext)->TableSize)
   {
      return False;
   }
   return True;
}

   jseVariable JSE_CFUNC FAR_CALL
jseCreateVariable(jseContext jsecontext,jseDataType VType)
{
   assert(FunctionIsSupported(jsecontext,jseCreateVariable));
#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCreateVariable,
           PUSH_2_PARM(jsecontext,VType));
#  else
   return (*jseFuncs(jsecontext)->jseCreateVariable)(jsecontext, VType);
#  endif
}

   jseVariable JSE_CFUNC FAR_CALL
jseCreateFunctionTextVariable(jseContext jsecontext,jseVariable FuncVar)
{
   assert(FunctionIsSupported(jsecontext,jseCreateFunctionTextVariable));
   FUNCTION_SUPPORTED(jseCreateFunctionTextVariable,return NULL);

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCreateFunctionTextVariable,
           PUSH_2_PARM(jsecontext,FuncVar));
#  else
   return (*jseFuncs(jsecontext)->jseCreateFunctionTextVariable)(jsecontext, FuncVar);
#  endif
}

   jseVariable JSE_CFUNC FAR_CALL
jseCreateSiblingVariable(jseContext jsecontext,jseVariable OlderSiblingVar,JSE_POINTER_SINDEX ElementOffsetFromOlderSibling)
{
   assert(FunctionIsSupported(jsecontext,jseCreateSiblingVariable));
#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCreateSiblingVariable,
           PUSH_3_PARM(jsecontext,OlderSiblingVar,ElementOffsetFromOlderSibling));
#  else
   return (*jseFuncs(jsecontext)->jseCreateSiblingVariable)(jsecontext, OlderSiblingVar, ElementOffsetFromOlderSibling);
#  endif
}

   jseVariable JSE_CFUNC FAR_CALL
jseCreateConvertedVariable(jseContext jsecontext,jseVariable VariableToConvert,
                           jseConversionTarget TargetType)
{
   assert(FunctionIsSupported(jsecontext,jseCreateConvertedVariable));
#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCreateConvertedVariable,
           PUSH_3_PARM(jsecontext,VariableToConvert,TargetType));
#  else
   return (*jseFuncs(jsecontext)->jseCreateConvertedVariable)(jsecontext,VariableToConvert,TargetType);
#  endif
}

   jseVariable JSE_CFUNC FAR_CALL
jseCreateLongVariable(jseContext jsecontext,slong l)
{ /* shortcut */
   assert(FunctionIsSupported(jsecontext,jseCreateLongVariable));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCreateLongVariable,
           PUSH_2_PARM(jsecontext,l));
#  else
   return (*jseFuncs(jsecontext)->jseCreateLongVariable)(jsecontext, l);
#  endif
}

   void JSE_CFUNC FAR_CALL
jseDestroyVariable(jseContext jsecontext,jseVariable variable)
{
   assert(FunctionIsSupported(jsecontext,jseDestroyVariable));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseDestroyVariable,
                  PUSH_2_PARM(jsecontext,variable));
#  else
   (*jseFuncs(jsecontext)->jseDestroyVariable)(jsecontext, variable);
#  endif
}


/* return MaxIndex, set MinIndex if not NULL */
   JSE_POINTER_UINDEX JSE_CFUNC FAR_CALL
jseGetArrayLength(jseContext jsecontext,jseVariable variable,JSE_POINTER_SINDEX *MinIndex)
{
   assert(FunctionIsSupported(jsecontext,jseGetArrayLength));

#  if defined(__JSE_WIN16__)
   return (JSE_POINTER_UINDEX)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetArrayLength,
           PUSH_3_PARM(jsecontext,variable,MinIndex));
#  else
   return (*jseFuncs(jsecontext)->jseGetArrayLength)(jsecontext, variable, MinIndex);
#  endif
}

/* grow or shrink to this size */
   void JSE_CFUNC FAR_CALL
jseSetArrayLength(jseContext jsecontext,jseVariable variable,JSE_POINTER_SINDEX MinIndex,JSE_POINTER_UINDEX Length)
{
   assert(FunctionIsSupported(jsecontext,jseSetArrayLength));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseSetArrayLength,
                  PUSH_4_PARM(jsecontext,variable,MinIndex,Length));
#  else
   (*jseFuncs(jsecontext)->jseSetArrayLength)(jsecontext, variable, MinIndex, Length);
#  endif
}

   void JSE_CFUNC FAR_CALL
jseSetAttributes(jseContext jsecontext,jseVariable var,jseVarAttributes attr)
{
   assert(FunctionIsSupported(jsecontext,jseSetAttributes));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseSetAttributes,
           PUSH_3_PARM(jsecontext,var,attr));
#  else
   (*jseFuncs(jsecontext)->jseSetAttributes)(jsecontext, var, attr);
#  endif
}

   jseVarAttributes JSE_CFUNC FAR_CALL
jseGetAttributes(jseContext jsecontext,jseVariable var)
{
   assert(FunctionIsSupported(jsecontext,jseGetAttributes));

#  if defined(__JSE_WIN16__)
   return (jseVarAttributes)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetAttributes,
           PUSH_2_PARM(jsecontext,var));
#  else
   return (*jseFuncs(jsecontext)->jseGetAttributes)(jsecontext, var);
#  endif
}


/********************************
 *** JSE VARIABLE DATA ACCESS ***
 ********************************/

   jseDataType JSE_CFUNC FAR_CALL
jseGetType(jseContext jsecontext,jseVariable variable)
{
   assert(FunctionIsSupported(jsecontext,jseGetType));

#  if defined(__JSE_WIN16__)
   return (jseDataType)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetType,
                                      PUSH_2_PARM(jsecontext,variable));
#  else
   return (*jseFuncs(jsecontext)->jseGetType)(jsecontext, variable);
#  endif
}



/* False and print error and LibError is set; else True */
   void JSE_CFUNC FAR_CALL
jseConvert(jseContext jsecontext,jseVariable variable,jseDataType dType)
{
   assert(FunctionIsSupported(jsecontext,jseConvert));

#  if defined(__JSE_WIN16__)
   (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseConvert,
                                  PUSH_3_PARM(jsecontext,variable,dType));
#  else
   (*jseFuncs(jsecontext)->jseConvert)(jsecontext, variable, dType);
#  endif
}

/* False and print error; else True */
   jsebool JSE_CFUNC FAR_CALL
jseAssign(jseContext jsecontext,jseVariable variable,jseVariable SrcVar)
{
   assert(FunctionIsSupported(jsecontext,jseAssign));

#  if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseAssign,
                                  PUSH_3_PARM(jsecontext,variable,SrcVar));
#  else
   return (*jseFuncs(jsecontext)->jseAssign)(jsecontext, variable, SrcVar);
#  endif
}

   ubyte JSE_CFUNC FAR_CALL
jseGetByte(jseContext jsecontext,jseVariable variable)
{
   assert(FunctionIsSupported(jsecontext,jseGetByte));

#  if defined(__JSE_WIN16__)
   return (ubyte)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetByte,
                                PUSH_2_PARM(jsecontext,variable));
#  else
   return (*jseFuncs(jsecontext)->jseGetByte)(jsecontext, variable);
#  endif
}

   void JSE_CFUNC FAR_CALL
jsePutByte(jseContext jsecontext,jseVariable variable,ubyte c)
{

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePutByte,
                  PUSH_3_PARM(jsecontext,variable,c));
#  else
   (*jseFuncs(jsecontext)->jsePutByte)(jsecontext, variable, c);
#  endif
}


   slong JSE_CFUNC FAR_CALL
jseGetLong(jseContext jsecontext,jseVariable variable)
{
   assert(FunctionIsSupported(jsecontext,jseGetLong));

#  if defined(__JSE_WIN16__)
   return DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetLong,
                         PUSH_2_PARM(jsecontext,variable));
#  else
   return (*jseFuncs(jsecontext)->jseGetLong)(jsecontext, variable);
#  endif
}

   void JSE_CFUNC FAR_CALL
jsePutLong(jseContext jsecontext,jseVariable variable,slong l)
{
   assert(FunctionIsSupported(jsecontext,jsePutLong));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePutLong,
                  PUSH_3_PARM(jsecontext,variable,l));
#  else
   (*jseFuncs(jsecontext)->jsePutLong)(jsecontext, variable, l);
#  endif
}

/* shortcut to jseGetString */
   const jsechar _HUGE_ * JSE_CFUNC FAR_CALL
jseGetString(jseContext jsecontext,jseVariable variable,JSE_POINTER_UINDEX *filled)
{
   assert(FunctionIsSupported(jsecontext,jseGetString));

#  if defined(__JSE_WIN16__)
   return (const jsechar _HUGE_ *)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetString,
                                         PUSH_3_PARM(jsecontext,variable,filled));
#  else
   return (*jseFuncs(jsecontext)->jseGetString)(jsecontext, variable,filled);
#  endif
}

/* shortcut to jseGetBuffer */
   const void _HUGE_ * JSE_CFUNC FAR_CALL
jseGetBuffer(jseContext jsecontext,jseVariable variable,JSE_POINTER_UINDEX *filled)
{
   assert(FunctionIsSupported(jsecontext,jseGetBuffer));

#  if defined(__JSE_WIN16__)
   return (const void _HUGE_ *)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetBuffer,
                                         PUSH_3_PARM(jsecontext,variable,filled));
#  else
   return (*jseFuncs(jsecontext)->jseGetBuffer)(jsecontext, variable,filled);
#  endif
}

/* shortcut to jseGetWriteableString */
   jsechar _HUGE_ * JSE_CFUNC FAR_CALL
jseGetWriteableString(jseContext jsecontext,jseVariable variable,JSE_POINTER_UINDEX *filled)
{
   assert(FunctionIsSupported(jsecontext,jseGetWriteableString));

#  if defined(__JSE_WIN16__)
   return (jsechar _HUGE_ *)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetWriteableString,
                                         PUSH_3_PARM(jsecontext,variable,filled));
#  else
   return (*jseFuncs(jsecontext)->jseGetWriteableString)(jsecontext, variable,filled);
#  endif
}

/* shortcut to jseGetWriteableBuffer */
   void _HUGE_ * JSE_CFUNC FAR_CALL
jseGetWriteableBuffer(jseContext jsecontext,jseVariable variable,JSE_POINTER_UINDEX *filled)
{
   assert(FunctionIsSupported(jsecontext,jseGetWriteableBuffer));

#  if defined(__JSE_WIN16__)
   return (void _HUGE_ *)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetWriteableBuffer,
                                         PUSH_3_PARM(jsecontext,variable,filled));
#  else
   return (*jseFuncs(jsecontext)->jseGetWriteableBuffer)(jsecontext, variable,filled);
#  endif
}

/* shortcut to jsePutString */
   void JSE_CFUNC FAR_CALL
jsePutStringLength(jseContext jsecontext,jseVariable variable,const jsechar _HUGE_ *data,JSE_POINTER_UINDEX size)
{
   assert(FunctionIsSupported(jsecontext,jsePutStringLength));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePutStringLength,
                                         PUSH_4_PARM(jsecontext,variable,data,size));
#  else
   (*jseFuncs(jsecontext)->jsePutStringLength)(jsecontext, variable,data,size);
#  endif
}

/* shortcut to jsePutString */
   void JSE_CFUNC FAR_CALL
jsePutString(jseContext jsecontext,jseVariable variable,const jsechar _HUGE_ *data)
{
   assert(FunctionIsSupported(jsecontext,jsePutString));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePutString,
                                         PUSH_3_PARM(jsecontext,variable,data));
#  else
   (*jseFuncs(jsecontext)->jsePutString)(jsecontext, variable,data);
#  endif
}

/* shortcut to jsePutBuffer */
   void JSE_CFUNC FAR_CALL
jsePutBuffer(jseContext jsecontext,jseVariable variable,const void _HUGE_ *data,JSE_POINTER_UINDEX size)
{
   assert(FunctionIsSupported(jsecontext,jsePutBuffer));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePutBuffer,
                                         PUSH_4_PARM(jsecontext,variable,data,size));
#  else
   (*jseFuncs(jsecontext)->jsePutBuffer)(jsecontext, variable,data,size);
#  endif
}

/* shortcut to jseCopyString */
   JSE_POINTER_UINDEX JSE_CFUNC FAR_CALL
jseCopyString(jseContext jsecontext,jseVariable variable,jsechar _HUGE_ *buffer,JSE_POINTER_UINDEX start,JSE_POINTER_UINDEX length)
{
   assert(FunctionIsSupported(jsecontext,jseCopyString));

#  if defined(__JSE_WIN16__)
   return (JSE_POINTER_UINDEX)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCopyString,
                                         PUSH_5_PARM( jsecontext, variable, buffer, start, length));
#  else
   return (*jseFuncs(jsecontext)->jseCopyString)(jsecontext, variable, buffer, start, length);
#  endif
}

/* shortcut to jseCopyBuffer */
   JSE_POINTER_UINDEX JSE_CFUNC FAR_CALL
jseCopyBuffer(jseContext jsecontext,jseVariable variable,void _HUGE_ *buffer,JSE_POINTER_UINDEX start,JSE_POINTER_UINDEX length)
{
   assert(FunctionIsSupported(jsecontext,jseCopyBuffer));

#  if defined(__JSE_WIN16__)
   return (JSE_POINTER_UINDEX)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCopyBuffer,
                                         PUSH_5_PARM( jsecontext, variable, buffer, start, length));
#  else
   return (*jseFuncs(jsecontext)->jseCopyBuffer)(jsecontext, variable, buffer, start, length);
#  endif
}

/* Set BooleanResult True or False on this Var; if error then error will have */
/* printed and return False, else return True */
   jsebool JSE_CFUNC FAR_CALL
jseEvaluateBoolean(jseContext jsecontext,jseVariable variable)
{
   assert(FunctionIsSupported(jsecontext,jseEvaluateBoolean));

#  if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseEvaluateBoolean,
                                   PUSH_2_PARM(jsecontext,variable));
#  else
   return (*jseFuncs(jsecontext)->jseEvaluateBoolean)(jsecontext, variable);
#  endif
}


   jsebool JSE_CFUNC FAR_CALL
jseCompare(jseContext jsecontext,jseVariable variable1,jseVariable variable2,slong *CompareResults)
{
   assert(FunctionIsSupported(jsecontext,jseCompare));

#  if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCompare,
                          PUSH_4_PARM(jsecontext,variable1,variable2,CompareResults));
#  else
   return (*jseFuncs(jsecontext)->jseCompare)(jsecontext, variable1, variable2, CompareResults);
#  endif
}


/******************************************************
 *** STRUCTURE VARIABLES AND THEIR MEMBER VARIABLES ***
 ******************************************************/

   jseVariable JSE_CFUNC FAR_CALL
jseMemberEx(jseContext jsecontext,jseVariable structure_var,const jsechar *Name,
            jseDataType DType,uword16 flags)
{
   assert(FunctionIsSupported(jsecontext,jseMemberEx));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseMemberEx,
               PUSH_5_PARM(jsecontext,structure_var,Name,DType,flags));
#  else
   return (*jseFuncs(jsecontext)->jseMemberEx)(jsecontext, structure_var, Name, DType, flags );
#  endif
}

   jseVariable JSE_CFUNC FAR_CALL
jseIndexMemberEx(jseContext jsecontext,jseVariable object_variable,JSE_POINTER_SINDEX index,
                 jseDataType DType,uword16 flags)
{
   assert(FunctionIsSupported(jsecontext,jseIndexMemberEx));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseIndexMemberEx,
           PUSH_5_PARM(jsecontext,object_variable,index,DType,flags));
#  else
   return (*jseFuncs(jsecontext)->jseIndexMemberEx)(jsecontext, object_variable, index, DType, flags );
#  endif
}

/* return next structure member after PrevVariable.  If PrevVariable is NULL */
/* then return first member.  Name will point to the variable name; DO NOT ALTER */
/* NAME DATA */
   jseVariable JSE_CFUNC FAR_CALL
jseGetNextMember(jseContext jsecontext,jseVariable structure_var,jseVariable PrevMemberVariable,const jsechar * * Name)
{
   assert(FunctionIsSupported(jsecontext,jseGetNextMember));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetNextMember,
                                       PUSH_4_PARM(jsecontext,structure_var,PrevMemberVariable,Name));
#  else
   return (*jseFuncs(jsecontext)->jseGetNextMember)(jsecontext, structure_var,PrevMemberVariable, Name);
#  endif
}

/* remove the member of this structure variable with the given name.  If name is NULL then delete */
/* all members.  It is not an error to pass in a Name that does not exist, in which case this */
/* function just returns without doing anything. */
   void JSE_CFUNC FAR_CALL
jseDeleteMember(jseContext jsecontext,jseVariable structure_var,const jsechar *Name)
{
   assert(FunctionIsSupported(jsecontext,jseDeleteMember));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseDeleteMember,
                  PUSH_3_PARM(jsecontext,structure_var,Name));
#  else
   (*jseFuncs(jsecontext)->jseDeleteMember)(jsecontext, structure_var, Name);
#  endif
}


/***********************************************
 *** GLOBAL VARIABLES = VARIABLES WITH NAMES ***
 ***********************************************/

   jseVariable JSE_CFUNC FAR_CALL
jseGlobalObject(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseGlobalObject));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGlobalObject,
                                      PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseGlobalObject)(jsecontext);
#  endif
}

   jseVariable JSE_CFUNC FAR_CALL
jseActivationObject(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseActivationObject));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseActivationObject,
                                      PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseActivationObject)(jsecontext);
#  endif
}

   void JSE_CFUNC FAR_CALL
jseSetGlobalObject(jseContext jsecontext,jseVariable newObject)
{
   if( FunctionIsSupported(jsecontext,jseSetGlobalObject) )
   {
#  if defined(__JSE_WIN16__)
      DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseSetGlobalObject,
                                       PUSH_2_PARM(jsecontext,newObject));
#  else
      (*jseFuncs(jsecontext)->jseSetGlobalObject)(jsecontext,newObject);
#  endif
   
   }
}

/* return a jseVariable for the current "this" var */
   jseVariable JSE_CFUNC FAR_CALL
jseGetCurrentThisVariable(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseGetCurrentThisVariable));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetCurrentThisVariable,
                                       PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseGetCurrentThisVariable)(jsecontext);
#  endif
}



/*********************
 *** THE JSE STACK ***
 *********************/
   jseStack JSE_CFUNC FAR_CALL
jseCreateStack(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseCreateStack));

#  if defined(__JSE_WIN16__)
   return (jseStack)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCreateStack,
                                    PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseCreateStack)(jsecontext);
#  endif
}

   void JSE_CFUNC FAR_CALL
jseDestroyStack(jseContext jsecontext,jseStack jsestack)
{
   assert(FunctionIsSupported(jsecontext,jseDestroyStack));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseDestroyStack,
                  PUSH_2_PARM(jsecontext,jsestack));
#  else
   (*jseFuncs(jsecontext)->jseDestroyStack)(jsecontext, jsestack);
#  endif
}

   void JSE_CFUNC FAR_CALL
jsePush(jseContext jsecontext,jseStack jsestack,jseVariable variable,jsebool DeleteVariableWhenFinished)
{
   assert(FunctionIsSupported(jsecontext,jsePush));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePush,
                  PUSH_4_PARM(jsecontext,jsestack,variable,DeleteVariableWhenFinished));
#  else
   (*jseFuncs(jsecontext)->jsePush)(jsecontext, jsestack, variable, DeleteVariableWhenFinished);
#  endif
}


   uint JSE_CFUNC FAR_CALL
jseFuncVarCount(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseFuncVarCount));

#  if defined(__JSE_WIN16__)
   return (uint)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseFuncVarCount,
                                PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseFuncVarCount)(jsecontext);
#  endif
}

/* return NULL for error; message already printed and error flag already set */
/* will only return NULL if ParameterOffset is invalid, and so if you */
/* know offset is valid (e.g., by FunctionList min and max) then you */
/* don't need to check for NULL. */
   jseVariable JSE_CFUNC FAR_CALL
jseFuncVar(jseContext jsecontext,uint ParameterOffset)
{
   assert(FunctionIsSupported(jsecontext,jseFuncVar));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseFuncVar,
                                       PUSH_2_PARM(jsecontext,ParameterOffset));
#  else
   return (*jseFuncs(jsecontext)->jseFuncVar)(jsecontext, ParameterOffset);
#  endif
}

/* return NULL for error; message already printed and error flag already set */
   jseVariable JSE_CFUNC FAR_CALL
jseFuncVarNeed(jseContext jsecontext,uint ParameterOffset,jseVarNeeded need)
{
   assert(FunctionIsSupported(jsecontext,jseFuncVarNeed));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseFuncVarNeed,
                                       PUSH_3_PARM(jsecontext,ParameterOffset,need));
#  else
   return (*jseFuncs(jsecontext)->jseFuncVarNeed)(jsecontext, ParameterOffset, need);
#  endif
}

/* similar to FuncVarNeed but already have variable */
   jsebool JSE_CFUNC FAR_CALL
jseVarNeed(jseContext jsecontext,jseVariable variable,jseVarNeeded need)
{
   assert(FunctionIsSupported(jsecontext,jseVarNeed));

#  if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseVarNeed,
                                   PUSH_3_PARM(jsecontext,variable,need));
#  else
   return (*jseFuncs(jsecontext)->jseVarNeed)(jsecontext, variable, need);
#  endif
}


/*****************************************************************
 *** METHODS TO RETURN VARIABLE FROM EXTERNAL LIBRARY FUNCTION ***
 *****************************************************************/

   void JSE_CFUNC FAR_CALL
jseReturnVar(jseContext jsecontext,jseVariable variable,jseReturnAction RetAction)
{
   assert(FunctionIsSupported(jsecontext,jseReturnVar));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseReturnVar,
                  PUSH_3_PARM(jsecontext,variable,RetAction));
#  else
   (*jseFuncs(jsecontext)->jseReturnVar)(jsecontext, variable, RetAction);
#  endif
}

   void JSE_CFUNC FAR_CALL
jseReturnLong(jseContext jsecontext,slong l)
{
   assert(FunctionIsSupported(jsecontext,jseReturnLong));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseReturnLong,
                  PUSH_2_PARM(jsecontext,l));
#  else
   (*jseFuncs(jsecontext)->jseReturnLong)(jsecontext, l);
#  endif
}


/*******************************************
 *** DEFINING EXTERNAL LIBRARY FUNCTIONS ***
 *******************************************/

/* if LibInit or LibTerm are NULL then not called. */
   void JSE_CFUNC FAR_CALL
jseAddLibrary(jseContext jsecontext,
              const jsechar * object_var_name/*NULL for global object*/,
              const struct jseFunctionDescription *FunctionList,
              void _FAR_ *InitLibData,jseLibraryInitFunction LibInit,jseLibraryTermFunction LibTerm)
{
   assert(FunctionIsSupported(jsecontext,jseAddLibrary));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseAddLibrary,
                  PUSH_6_PARM(jsecontext,object_var_name,FunctionList,InitLibData,LibInit,LibTerm));
#  else
   (*jseFuncs(jsecontext)->jseAddLibrary)(jsecontext, object_var_name, FunctionList, InitLibData,LibInit,LibTerm);
#  endif
}

/* following call to LibraryInitFunction this will always */
/* return the value returned by LibraryInitFunction. */
   void _FAR_ * JSE_CFUNC FAR_CALL
jseLibraryData(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseLibraryData));

#  if defined(__JSE_WIN16__)
   return (void _FAR_ * )DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseLibraryData,
                                         PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseLibraryData)(jsecontext);
#  endif
}

/**********************************************************
 *** WORKING WITH OTHER INTERNAL AND EXTERNAL FUNCTIONS ***
 **********************************************************/

/* Return NULL if not found; If ErrorIfNotFound then also call Oops() to print error; */
   jseVariable JSE_CFUNC FAR_CALL
jseGetFunction(jseContext jsecontext,jseVariable jseObject,
               const jsechar *FunctionName,
               jsebool ErrorIfNotFound)
{
   assert(FunctionIsSupported(jsecontext,jseGetFunction));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetFunction,
                                       PUSH_4_PARM(jsecontext, jseObject, FunctionName,ErrorIfNotFound));
#  else
   return (*jseFuncs(jsecontext)->jseGetFunction)(jsecontext, jseObject, FunctionName, ErrorIfNotFound);
#  endif
}

   jsebool JSE_CFUNC FAR_CALL
jseIsFunction(jseContext jsecontext,jseVariable jseObject)
{
   assert(FunctionIsSupported(jsecontext,jseIsFunction));

#  if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseIsFunction,
                                       PUSH_2_PARM(jsecontext, jseObject));
#  else
   return (*jseFuncs(jsecontext)->jseIsFunction)(jsecontext, jseObject);
#  endif
}

/* ReturnVar NULL if no return variable, else the variable returned. */
/* Return True if shall continue program, else FALSE if quit or exit set */
/* Context may be not be NULL. This call may may set ERROR or ExitCode accordingly. */
   jsebool JSE_CFUNC FAR_CALL
jseCallFunction(jseContext jsecontext,jseVariable jseVar,
                jseStack jsestack,jseVariable *ReturnVar,
                jseVariable ThisVar)
{
   assert(FunctionIsSupported(jsecontext,jseCallFunction));

#  if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCallFunction,
                                   PUSH_5_PARM(jsecontext,jseVar,jsestack,ReturnVar,ThisVar));
#  else
   return (*jseFuncs(jsecontext)->jseCallFunction)(jsecontext,jseVar,jsestack,ReturnVar,ThisVar);
#  endif
}

   jseContext JSE_CFUNC FAR_CALL
jsePreviousContext(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jsePreviousContext));

#  if defined(__JSE_WIN16__)
   return (jseContext)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePreviousContext,
                                      PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jsePreviousContext)(jsecontext);
#  endif
}
/* Return the current context for the the current thread of execution. If you're */
/* being called by the jse interpreter then this will be the same value passed */
/* to your function in the context parameter and this call is not needed */
/* (and should be avoided because of its extra overhead) but if you're in */
/* a callback function then you can use this.  If you know the ancestor */
/* jsecontext, such as from InitializejseLink, then this will return the context */
/* currently in use (descendat of AncestorContext).  This is intended only */
/* for use by callback or interrupt-like functions. */
   jseContext JSE_CFUNC FAR_CALL
jseCurrentContext(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseCurrentContext));

#  if defined(__JSE_WIN16__)
   return (jseContext)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCurrentContext,
                                      PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseCurrentContext)(jsecontext);
#  endif
}

   jseVariable JSE_CFUNC FAR_CALL
jseCreateWrapperFunction(jseContext jsecontext,
      const jsechar *FunctionName,                 /* used for error reporting (real name or use as hint) */
      void (JSE_CFUNC FAR_CALL *FuncPtr)(jseContext jsecontext),
      sword8 MinVariableCount, sword8 MaxVariableCount, /*-1 for no max*/
      jseVarAttributes VarAttributes,                             /* bitwise-OR jseVarAttributes */
      jseFuncAttributes FuncAttributes,                            /* bitwise-OR */
      void _FAR_ *fData)                                /* fData is available to the function through jseLibraryData() */
{
   assert(FunctionIsSupported(jsecontext,jseCreateWrapperFunction));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCreateWrapperFunction,
                  PUSH_8_PARM(jsecontext,FunctionName,FuncPtr,
                               MinVariableCount,MaxVariableCount,
                               VarAttributes,FuncAttributes,
                               fData));
#  else
   return (*jseFuncs(jsecontext)->jseCreateWrapperFunction)(jsecontext,FunctionName,FuncPtr,
                               MinVariableCount,MaxVariableCount,
                               VarAttributes,FuncAttributes,
                               fData);
#  endif

}

   jseVariable JSE_CFUNC FAR_CALL
jseMemberWrapperFunction(jseContext jsecontext,jseVariable structure_var,
      const jsechar *FunctionName,                 /* used for error reporting (real name or use as hint) */
      void (JSE_CFUNC FAR_CALL *FuncPtr)(jseContext jsecontext),
      sword8 MinVariableCount, sword8 MaxVariableCount, /*-1 for no max*/
      jseVarAttributes VarAttributes,                             /* bitwise-OR jseVarAttributes */
      jseFuncAttributes FuncAttributes,                            /* bitwise-OR */
      void _FAR_ *fData)                                /* fData is available to the function through jseLibraryData() */
{
   assert(FunctionIsSupported(jsecontext,jseMemberWrapperFunction));

#  if defined(__JSE_WIN16__)
   return (jseVariable)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseMemberWrapperFunction,
                  PUSH_9_PARM(jsecontext,structure_var,FunctionName,FuncPtr,
                               MinVariableCount,MaxVariableCount,
                               VarAttributes,FuncAttributes,
                               fData));
#  else
   return (*jseFuncs(jsecontext)->jseMemberWrapperFunction)(jsecontext,structure_var,FunctionName,FuncPtr,
                               MinVariableCount,MaxVariableCount,
                               VarAttributes,FuncAttributes,
                               fData);
#  endif

}

/************************************************
 *** #DEFINE STATEMENTS DURING INITIALIZATION ***
 ************************************************/

#  if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
   void JSE_CFUNC FAR_CALL
jsePreDefineLong(jseContext jsecontext,const jsechar *FindString,slong ReplaceL)
{
   assert(FunctionIsSupported(jsecontext,jsePreDefineLong));
   FUNCTION_SUPPORTED(jsePreDefineLong,return);

#    if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePreDefineLong,
                  PUSH_3_PARM(jsecontext,FindString,ReplaceL));
#    else
   (*jseFuncs(jsecontext)->jsePreDefineLong)(jsecontext,FindString,ReplaceL);
#    endif
}

   void JSE_CFUNC FAR_CALL
jsePreDefineString(jseContext jsecontext,const jsechar *FindString,const jsechar *ReplaceString)
{
   assert(FunctionIsSupported(jsecontext,jsePreDefineString));
   FUNCTION_SUPPORTED(jsePreDefineString,return);

#    if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePreDefineString,
                  PUSH_3_PARM(jsecontext,FindString,ReplaceString));
#    else
   (*jseFuncs(jsecontext)->jsePreDefineString)(jsecontext,FindString,ReplaceString);
#    endif
}
#  endif

/************************************
 ******** REMOTE-JSE (RPjse) ********
 ************************************/

#  if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
/* returns buffer or NULL if error; buffer must be freed by the caller. */
/* if return non-NULL then *BufferLen is set to length of data in the buffer */
   void * JSE_CFUNC FAR_CALL
jseCreateCodeTokenBuffer(jseContext jsecontext,
       const jsechar *Source,jsebool SourceIsFileName/*else is source string*/,
       uint *BufferLen)
{
   assert(FunctionIsSupported(jsecontext,jseCreateCodeTokenBuffer));
   FUNCTION_SUPPORTED(jseCreateCodeTokenBuffer,return NULL);

#    if defined(__JSE_WIN16__)
   return (void *)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCreateCodeTokenBuffer,
                        PUSH_4_PARM(jsecontext,Source,SourceIsFileName,BufferLen));
#    else
   return (*jseFuncs(jsecontext)->jseCreateCodeTokenBuffer)(jsecontext,Source,SourceIsFileName,(unsigned int *)BufferLen);
#    endif
}
#  endif

/*********************
 *** MISCELLANEOUS ***
 *********************/

/* call this function at exit time with the Param parameter */
   jsebool JSE_CFUNC FAR_CALL
jseCallAtExit(jseContext jsecontext,jseAtExitFunc ExitFunc,void _FAR_ *Param)
{
   assert(FunctionIsSupported(jsecontext,jseCallAtExit));

#  if defined(__JSE_WIN16__)
   return DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCallAtExit,
                  PUSH_3_PARM(jsecontext,ExitFunc,Param));
#  else
   return (*jseFuncs(jsecontext)->jseCallAtExit)(jsecontext,ExitFunc,Param);
#  endif
}

/* set flag that there has been an error */
   void JSE_CFUNC FAR_CALL
jseLibSetErrorFlag(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseLibSetErrorFlag));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseLibSetErrorFlag,
                  PUSH_1_PARM(jsecontext));
#  else
   (*jseFuncs(jsecontext)->jseLibSetErrorFlag)(jsecontext);
#  endif
}

/* print error; set error flag in context */
   void JSE_CFUNC FAR_CALL
jseLibErrorPrintf(jseContext ExitContext,const jsechar * FormatS,...)
{
   jseContext jsecontext = ExitContext;
   ulong   arg[20];
   uint    argcnt = 0;
   va_list arglist;
   va_start(arglist,FormatS);
   while (argcnt < 20)
   {
      arg[argcnt++] = va_arg(arglist, ulong);
   }
   assert(argcnt == 20);

   assert(FunctionIsSupported(jsecontext,jseLibErrorPrintf));


#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseLibErrorPrintf, 40+4, jsecontext, FormatS,
           arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9],
           arg[10],arg[11],arg[12],arg[13],arg[14],arg[15],arg[16],arg[17],arg[18],arg[19]);
#  else
   (*jseFuncs(jsecontext)->jseLibErrorPrintf)( jsecontext, FormatS,
           arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9],
           arg[10],arg[11],arg[12],arg[13],arg[14],arg[15],arg[16],arg[17],arg[18],arg[19]);
#  endif
   va_end(arglist);
}

/* Sets exit flag for this jsecontext, and saves exit code */
   void JSE_CFUNC FAR_CALL
jseLibSetExitFlag(jseContext jsecontext,jseVariable ExitVar)
{
   assert(FunctionIsSupported(jsecontext,jseLibSetExitFlag));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseLibSetExitFlag,
                  PUSH_2_PARM(jsecontext,ExitVar));
#  else
   (*jseFuncs(jsecontext)->jseLibSetExitFlag)( jsecontext, ExitVar);
#  endif
}

/* return 0 if a call has been made on this context to Exit or for */
/* one of the above error functions.  Return one of the following */
/* non-0 (non-False) defines if should exit. */
   uint JSE_CFUNC FAR_CALL
jseQuitFlagged(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseQuitFlagged));

#  if defined(__JSE_WIN16__)
   return (uint)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseQuitFlagged,
                                PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseQuitFlagged)(jsecontext);
#  endif
}

   jsebool JSE_CFUNC FAR_CALL
jseInterpret(jseContext jsecontext,
             const jsechar * SourceFile,         /* NULL if pure text */
             const jsechar * SourceText,         /* text or options if SourceFile */
             const void * PreTokenizedSource, /*NULL or data is already precompiled as in jseCreateCodeTokenBuffer */
             jseNewContextSettings NewContextSettings,
             int fHowToInterpret,                     /* flags, may be JSE_INTERPRET_NO_INHERIT */
             jseContext LocalVariableContext,         /* NULL if not inherit local variables */
             jseVariable *ReturnVar)
{

#  if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseInterpret,
                                  PUSH_8_PARM(jsecontext,SourceFile,SourceText,PreTokenizedSource,NewContextSettings,fHowToInterpret,LocalVariableContext,ReturnVar));
#  else
   return (*jseFuncs(jsecontext)->jseInterpret)(jsecontext,SourceFile,SourceText,PreTokenizedSource,NewContextSettings,fHowToInterpret,LocalVariableContext,ReturnVar);
#  endif
}

   const jsechar * JSE_CFUNC FAR_CALL
jseLocateSource(jseContext jsecontext,uint *LineNumber)
{
   assert(FunctionIsSupported(jsecontext,jseLocateSource));

#  if defined(__JSE_WIN16__)
   return (jsechar *)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseLocateSource,
                                 PUSH_2_PARM(jsecontext,LineNumber));
#  else
   return (*jseFuncs(jsecontext)->jseLocateSource)(jsecontext, LineNumber);
#  endif
}

   const jsechar * JSE_CFUNC FAR_CALL
jseCurrentFunctionName(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseCurrentFunctionName));

#  if defined(__JSE_WIN16__)
   return (jsechar *)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseCurrentFunctionName,
                                 PUSH_1_PARM(jsecontext));
#  else
   return (jsechar *)(*jseFuncs(jsecontext)->jseCurrentFunctionName)(jsecontext);
#  endif
}

#  if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   void JSE_CFUNC FAR_CALL
jseReturnNumber(jseContext jsecontext,jsenumber f)
{
   assert(FunctionIsSupported(jsecontext,jseReturnNumber));

#    if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseReturnNumber,
                  PUSH_2_PARM(jsecontext,f));
#    else
   (*jseFuncs(jsecontext)->jseReturnNumber)(jsecontext, f);
#    endif
}
#    if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
   void JSE_CFUNC FAR_CALL
jsePreDefineNumber(jseContext jsecontext,const jsechar *FindString,jsenumber ReplaceF)
{
   assert(FunctionIsSupported(jsecontext,jsePreDefineNumber));
   FUNCTION_SUPPORTED(jsePreDefineNumber,return);

#      if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePreDefineNumber,
                  PUSH_3_PARM(jsecontext,FindString,ReplaceF));
#      else
   (*jseFuncs(jsecontext)->jsePreDefineNumber)(jsecontext,FindString,ReplaceF);
#      endif
}
#    endif
   jsenumber
jseGetNumber(jseContext jsecontext,jseVariable variable)
{
   jsenumber it;
   assert(FunctionIsSupported(jsecontext,jseGetFloatIndirect));
   FUNCTION_SUPPORTED(jseGetFloatIndirect,return 0);

#    if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetFloatIndirect,
                  PUSH_3_PARM(jsecontext,variable,&it));
#    else
   (*jseFuncs(jsecontext)->jseGetFloatIndirect)(jsecontext, variable, &it);
#    endif
   return it;
}
   void JSE_CFUNC FAR_CALL
jsePutNumber(jseContext jsecontext,jseVariable variable,jsenumber f)
{
   assert(FunctionIsSupported(jsecontext,jsePutNumber));

#    if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePutNumber,
                  PUSH_3_PARM(jsecontext,variable,f));
#    else
   (*jseFuncs(jsecontext)->jsePutNumber)(jsecontext, variable, f);
#    endif
}
#  endif

   jsechar * * JSE_CFUNC FAR_CALL
jseGetFileNameList(jseContext jsecontext,int *number)
{
   assert(FunctionIsSupported(jsecontext,jseGetFileNameList));
   FUNCTION_SUPPORTED(jseGetFileNameList,return NULL);

#  if defined(__JSE_WIN16__)
   return (jsechar * *)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetFileNameList,
                  PUSH_2_PARM(jsecontext,number));
#  else
   return (*jseFuncs(jsecontext)->jseGetFileNameList)(jsecontext,number);
#  endif
}

   jsebool JSE_CFUNC FAR_CALL
jseBreakpointTest(jseContext jsecontext,const jsechar *FileName,uint LineNumber)
{
   assert(FunctionIsSupported(jsecontext,jseGetFileNameList));
   FUNCTION_SUPPORTED(jseBreakpointTest,return False);

#  if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseBreakpointTest,
                  PUSH_3_PARM(jsecontext,FileName,LineNumber));
#  else
   return (*jseFuncs(jsecontext)->jseBreakpointTest)(jsecontext,FileName,LineNumber);
#  endif
}

#  if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
   jsebool JSE_CFUNC FAR_CALL
jseTellSecurity(jseContext jsecontext,jseVariable InfoVar)
{
   assert(FunctionIsSupported(jsecontext,jseTellSecurity));
   FUNCTION_SUPPORTED(jseTellSecurity,return False);

#    if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseTellSecurity,
                                     PUSH_2_PARM(jsecontext,InfoVar));
#    else
   return (*jseFuncs(jsecontext)->jseTellSecurity)(jsecontext,InfoVar);
#    endif
}
#  endif

   jseContext JSE_CFUNC FAR_CALL
jseInitializeExternalLink(jseContext jsecontext,void _FAR_ *LinkData,struct jseExternalLinkParameters * LinkParms,
                          const jsechar * GlobalObjectName,const char *AccessKey)
{
   assert(FunctionIsSupported(jsecontext,jseInitializeExternalLink));

#  if defined(__JSE_WIN16__)
   return (jseContext)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseInitializeExternalLink,
                  PUSH_4_PARM(LinkData,LinkParms,GlobalObjectName,AccessKey));
#  else
   return (*jseFuncs(jsecontext)->jseInitializeExternalLink)(LinkData,LinkParms,GlobalObjectName,AccessKey);
#  endif
}

   void JSE_CFUNC FAR_CALL
jseTerminateExternalLink(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseTerminateExternalLink));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseTerminateExternalLink,
                  PUSH_1_PARM(jsecontext));
#  else
   (*jseFuncs(jsecontext)->jseTerminateExternalLink)(jsecontext);
#  endif
}

   void _FAR_ * JSE_CFUNC FAR_CALL
jseGetLinkData(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseGetLinkData));

#  if defined(__JSE_WIN16__)
   return (void _FAR_ *)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetLinkData,
                  PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseGetLinkData)(jsecontext);
#  endif
}

   struct jseExternalLinkParameters _FAR_ * JSE_CFUNC FAR_CALL
jseGetExternalLinkParameters(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseGetExternalLinkParameters));

#  if defined(__JSE_WIN16__)
   return (struct jseExternalLinkParameters _FAR_ *)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetExternalLinkParameters,
                                PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseGetExternalLinkParameters)(jsecontext);
#  endif
}

   jseContext JSE_CFUNC FAR_CALL
jseAppExternalLinkRequest(jseContext jsecontext,jsebool Initialize)
{
   assert(FunctionIsSupported(jsecontext,jseAppExternalLinkRequest));

#  if defined(__JSE_WIN16__)
   return (jseContext)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseAppExternalLinkRequest,
                                PUSH_2_PARM(jsecontext,Initialize));
#  else
   return (*jseFuncs(jsecontext)->jseAppExternalLinkRequest)(jsecontext,Initialize);
#  endif
}

   jseVariable JSE_CFUNC FAR_CALL
jseFindVariable(jseContext jsecontext,
                const jsechar * name, ulong flags)
{
   assert(FunctionIsSupported(jsecontext,jseFindVariable));

#  if defined(__JSE_WIN16__)
   return (jseContext)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseFindVariable,
                                PUSH_3_PARM(jsecontext,name,flags));
#  else
   return (*jseFuncs(jsecontext)->jseFindVariable)(jsecontext,name,flags);
#  endif
}

   jsebool JSE_CFUNC FAR_CALL
jseGetVariableName(jseContext jsecontext,
   jseVariable variableToFind, jsechar * const buffer, uint bufferSize)
{
   assert(FunctionIsSupported(jsecontext,jseGetVariableName));

#  if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetVariableName,
                                PUSH_4_PARM(jsecontext,variableToFind,buffer,bufferSize));
#  else
   return (*jseFuncs(jsecontext)->jseGetVariableName)(jsecontext,variableToFind,buffer,bufferSize);
#  endif
}

   jseContext JSE_CFUNC FAR_CALL
jseInterpInit(jseContext jsecontext,
              const jsechar * SourceFile,
              const jsechar * SourceText,
              const void * PreTokenizedSource,
              jseNewContextSettings NewContextSettings,
              int howToInterpret,
              jseContext localVariableContext)
{
   assert(FunctionIsSupported(jsecontext,jseInterpInit));

#  if defined(__JSE_WIN16__)
   return (jseContext)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseInterpInit,
                                PUSH_7_PARM(jsecontext,SourceFile,SourceText,PreTokenizedSource,NewContextSettings,
                                            howToInterpret,localVariableContext));
#  else
   return (*jseFuncs(jsecontext)->jseInterpInit)(jsecontext,SourceFile,SourceText,PreTokenizedSource,NewContextSettings,
                                             howToInterpret,localVariableContext);
#  endif
}

   jseVariable JSE_CFUNC FAR_CALL
jseInterpTerm(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseInterpTerm));

#  if defined(__JSE_WIN16__)
   return (jseContext)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseInterpTerm,
                                PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseInterpTerm)(jsecontext);
#  endif
}

   jseContext JSE_CFUNC FAR_CALL
jseInterpExec(jseContext jsecontext)
{
   assert(FunctionIsSupported(jsecontext,jseInterpExec));

#  if defined(__JSE_WIN16__)
   return (jseContext)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseInterpExec,
                                PUSH_1_PARM(jsecontext));
#  else
   return (*jseFuncs(jsecontext)->jseInterpExec)(jsecontext);
#  endif
}

   jsebool JSE_CFUNC FAR_CALL
jseGetBoolean(jseContext jsecontext, jseVariable var)
{
   assert(FunctionIsSupported(jsecontext,jseGetBoolean));

#  if defined(__JSE_WIN16__)
   return (jsebool)DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jseGetBoolean,
                                PUSH_2_PARM(jsecontext,var));
#  else
   return (*jseFuncs(jsecontext)->jseGetBoolean)(jsecontext,var);
#  endif
   
}

   void JSE_CFUNC FAR_CALL
jsePutBoolean(jseContext jsecontext, jseVariable var, jsebool val)
{
   assert(FunctionIsSupported(jsecontext,jsePutBoolean));

#  if defined(__JSE_WIN16__)
   DispatchToHost(jseFuncs(jsecontext)->DS, jseFuncs(jsecontext)->jsePutBoolean,
                                PUSH_3_PARM(jsecontext,var,val));
#  else
   (*jseFuncs(jsecontext)->jsePutBoolean)(jsecontext,var,val);
#  endif
   
}

/**********************************************************/
/*                MS-Windows 16Bit                        */
/**********************************************************/
#  if defined(__JSE_WIN16__)
#    include <windows.h>
#ifdef _cplusplus
extern "C" {
#endif
   int FAR PASCAL LibMain(HINSTANCE hInstance,WORD ,WORD ,LPSTR );
   int __export FAR PASCAL WEP(int );
#ifdef _cplusplus
}
#endif

   int FAR PASCAL
LibMain(HINSTANCE unused1,WORD unused2,WORD unused3,LPSTR unused4)
{
   return TRUE; /*success */
}

   int __export FAR PASCAL
WEP(int unused)
{

   return TRUE; /* success */
}



/**********************************************************/
/*                          OS/2                          */
/**********************************************************/
#  elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)

#    if defined (__WATCOMC__)
#ifdef __cplusplus 
extern "C" {
#endif    
   int __dll_initialize(void);
   int __dll_terminate(void);
#ifdef __cplusplus 
}
#endif

   int
__dll_initialize(void)
{
   return TRUE;
}

   int
__dll_terminate(void)
{
   return TRUE;
}

#    elif defined(__BORLANDC__)
extern "C" {
   ULONG cdecl _dllmain(ULONG termflag,HMODULE);
}
   ULONG cdecl
_dllmain(ULONG termflag,HMODULE)
{
   return TRUE;
}
#    elif defined(__IBMCPP__)
#      error dll entry still needs coded for VisualAgeC++
#    endif



/**********************************************************/
/*                MS-Windows 32Bit                        */
/**********************************************************/
#  elif defined(__JSE_WIN32__)
   BOOL APIENTRY
LibMain(HINSTANCE hinstDLL_unused, DWORD fdwReason, LPVOID _unused)
{

   switch (fdwReason)
   {
      case DLL_PROCESS_ATTACH:
      /* The DLL is being mapped into the process's address space. */
      break;
      case DLL_THREAD_ATTACH:
      /* A thread is being created. */
      break;

      case DLL_THREAD_DETACH:
      /* A thread is exiting cleanly. */
      break;
      case DLL_PROCESS_DETACH:
      /* The DLL is being unmapped from the process's address */
      /* space. */
      break;
   }
   return TRUE;
}

#  elif defined(__JSE_NWNLM__)
/**********************************************************/
/*                   Netware NLM                          */
/**********************************************************/
#    include <process.h>

void main()
{
   /* we just need to stay loaded. If we get unloaded, so be it. */
   while( 1 ) delay(1000);
}
#  endif

#else
   ALLOW_EMPTY_FILE
#endif
