/* dllrun.c    Functions for linking to jse DLL when the run-time
 *             link method is chosen.
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

#if defined(__JSE_DLLRUN__)


#if defined(FIGURE_DLLRUN_ENUMS)
   #define DLL_TABLE_ENTRY(DLLENTRY)   DLLENTRY,
   enum jseDllTableEnums {
      MakeSureStartAtZero = -1,

#else

#include <jseTypes.h>
#include <jseLib.h>
#if defined(__JSE_WIN16__) || defined(__JSE_DOS16__) \
 || defined(__JSE_WIN32__) || defined(__JSE_CON32__)
   #include <windows.h>
#endif
#include <stdarg.h>
#include <assert.h>

#define FIGURE_DLLRUN_ENUMS
#include "DllRun.c"
#undef FIGURE_DLLRUN_ENUMS

   #undef DLL_TABLE_ENTRY
   #define DLL_TABLE_ENTRY(DLLENTRY)   { 0, #DLLENTRY },

   typedef void (PFUNC FAR_CALL *jseDllProc)();

   struct jseDll_ {
      jseDllProc Proc;
      char *Name;
   };
   struct jseDll_ _NEAR_ jseDll[] = {
#endif
      DLL_TABLE_ENTRY(JSECREATEVARIABLE)
      DLL_TABLE_ENTRY(JSECREATESIBLINGVARIABLE)
      DLL_TABLE_ENTRY(JSECREATELONGVARIABLE)
      DLL_TABLE_ENTRY(JSEDESTROYVARIABLE)
      DLL_TABLE_ENTRY(JSEGETARRAYLENGTH)
      DLL_TABLE_ENTRY(JSESETARRAYLENGTH)
      DLL_TABLE_ENTRY(JSEGETTYPE)
      DLL_TABLE_ENTRY(JSECONVERT)
      DLL_TABLE_ENTRY(JSEASSIGN)
      DLL_TABLE_ENTRY(JSEGETBYTE)
      DLL_TABLE_ENTRY(JSEPUTBYTE)
      DLL_TABLE_ENTRY(JSEGETLONG)
      DLL_TABLE_ENTRY(JSEPUTLONG)
      DLL_TABLE_ENTRY(JSEGETDATA)
      DLL_TABLE_ENTRY(JSEGETDATA0)
      DLL_TABLE_ENTRY(JSEPUTDATA)
      DLL_TABLE_ENTRY(JSEEVALUATEBOOLEAN)
      DLL_TABLE_ENTRY(JSECOMPARE)
      DLL_TABLE_ENTRY(JSEMEMBEREX)
      DLL_TABLE_ENTRY(JSEINDEXMEMBEREX)
      DLL_TABLE_ENTRY(JSEGETNEXTMEMBER)
      DLL_TABLE_ENTRY(JSEDELETEMEMBER)
      DLL_TABLE_ENTRY(JSEGETMEMBEREX)
      DLL_TABLE_ENTRY(JSEGETINDEXMEMBEREX)
      DLL_TABLE_ENTRY(JSEGLOBALOBJECT)
      DLL_TABLE_ENTRY(JSEGETCURRENTTHISVARIABLE)
      DLL_TABLE_ENTRY(JSECREATESTACK)
      DLL_TABLE_ENTRY(JSEDESTROYSTACK)
      DLL_TABLE_ENTRY(JSEPUSH)
      DLL_TABLE_ENTRY(JSEFUNCVARCOUNT)
      DLL_TABLE_ENTRY(JSEFUNCVAR)
      DLL_TABLE_ENTRY(JSEFUNCVARNEED)
      DLL_TABLE_ENTRY(JSEVARNEED)
      DLL_TABLE_ENTRY(JSERETURNVAR)
      DLL_TABLE_ENTRY(JSERETURNLONG)
      DLL_TABLE_ENTRY(JSELIBRARYDATA)
      DLL_TABLE_ENTRY(JSEGETFUNCTION)
      DLL_TABLE_ENTRY(JSECALLFUNCTION)
      DLL_TABLE_ENTRY(JSEPREVIOUSCONTEXT)
      DLL_TABLE_ENTRY(JSECURRENTCONTEXT)
      DLL_TABLE_ENTRY(JSEISLIBRARYFUNCTION)
      DLL_TABLE_ENTRY(JSEADDFUNCTION)
      DLL_TABLE_ENTRY(JSEPREDEFINELONG)
      DLL_TABLE_ENTRY(JSECALLATEXIT)
      DLL_TABLE_ENTRY(JSELIBSETERRORFLAG)
      DLL_TABLE_ENTRY(JSELIBERRORPRINTF)
      DLL_TABLE_ENTRY(JSELIBSETEXITFLAG)
      DLL_TABLE_ENTRY(JSEQUITFLAGGED)
      DLL_TABLE_ENTRY(JSEINTERPRET)
      DLL_TABLE_ENTRY(JSEPREDEFINESTRING)
      DLL_TABLE_ENTRY(JSEADDLIBRARY)
      DLL_TABLE_ENTRY(JSELOCATESOURCE)
      DLL_TABLE_ENTRY(JSECURRENTFUNCTIONNAME)
      DLL_TABLE_ENTRY(JSESETERRNO)
      DLL_TABLE_ENTRY(JSEGETFILENAMELIST)
      DLL_TABLE_ENTRY(JSEINITIALIZEEXTERNALLINK)
      DLL_TABLE_ENTRY(JSETERMINATEEXTERNALLINK)
      DLL_TABLE_ENTRY(JSEGETLINKDATA)
      DLL_TABLE_ENTRY(JSEGETEXTERNALLINKPARAMETERS)
      DLL_TABLE_ENTRY(JSEAPPEXTERNALLINKREQUEST)
      DLL_TABLE_ENTRY(JSEADDDYNAMOBJECTFUNCTION)
      DLL_TABLE_ENTRY(JSETELLSECURITY)
      DLL_TABLE_ENTRY(JSECREATECODETOKENBUFFER)
      DLL_TABLE_ENTRY(JSEPUTNUMBER)
      DLL_TABLE_ENTRY(JSERETURNNUMBER)
      DLL_TABLE_ENTRY(JSEGETFLOATINDIRECT)
      DLL_TABLE_ENTRY(JSEPREDEFINEFLOAT)
};




#if !defined(FIGURE_DLLRUN_ENUMS)

static HINSTANCE dllhandle = 0;
static int dllLinkCount = 0;

   static void
UnlinkjseDll(void)
{
   assert( 0 != dllLinkCount );
   assert( dllhandle );
   if ( 0 == --dllLinkCount ) {
      FreeLibrary(dllhandle);
      dllhandle = 0;
   } /* endif */
}

   static jsebool
LinkjseDll(void)
{
   int i, FunctionCount;
   struct jseDll_ *Dll;

   if ( 0 == dllLinkCount++ ) {
      dllhandle = LoadLibrary(DLL_NAME);
      if ( dllhandle < (HINSTANCE)32 ) {
         dllhandle = 0;
         dllLinkCount--;
      } else {
         // get addresses of all our functions in jseDll table
         FunctionCount = sizeof(jseDll) / sizeof(jseDll[0]);
         for ( i = 0, Dll= jseDll; i < FunctionCount; i++, Dll++ ) {
            if ( NULL == (Dll->Proc = (jseDllProc)GetProcAddress(dllhandle,Dll->Name)) ) {
               UnlinkjseDll();
               break;
            }
         } /* endfor */
      } /* endif */
   } /* endif */
   return( 0 != dllhandle );
}

   void JSEPFUNC
jseAddLibrary(jseContext jsecontext,struct jseFunctionDescription CPP_CONST _FAR_ *FunctionList,
              jseLibraryInitFunction LibInit,jseLibraryTermFunction LibTerm)
{
   ((void (PFUNC FAR_CALL *)(jseContext,struct jseFunctionDescription CPP_CONST _FAR_ *,
                                jseLibraryInitFunction,jseLibraryTermFunction))
      (jseDll[JSEADDLIBRARY].Proc))(jsecontext,FunctionList,LibInit,LibTerm);
}

   jsebool JSEPFUNC
jseAssign(jseContext jsecontext,jseVariable variable,jseVariable SrcVar)
{
   return ((jsebool (PFUNC FAR_CALL *)(jseContext,jseVariable,jseVariable))
      (jseDll[JSEASSIGN].Proc))(jsecontext,variable,SrcVar);
}

   void JSEPFUNC
jseCallAtExit(jseContext jsecontext,jseAtExitFunc ExitFunc,void _FAR_ *Param)
{
   ((void (PFUNC FAR_CALL *)(jseContext,jseAtExitFunc,void _FAR_ *))
      (jseDll[JSECALLATEXIT].Proc))(jsecontext,ExitFunc,Param);
}

   jsebool JSEPFUNC
jseCallFunction(jseContext jsecontext,jseFunction jsefunction,jseStack jsestack,jseVariable _FAR_ *ReturnVar)
{
   return ((jsebool (PFUNC FAR_CALL *)(jseContext,jseFunction,jseStack,jseVariable _FAR_ *))
      (jseDll[JSECALLFUNCTION].Proc))(jsecontext,jsefunction,jsestack,ReturnVar);
}

   slong JSEPFUNC
jseCompare(jseVariable variable1,jseVariable variable2)
{
   return ((slong (PFUNC FAR_CALL *)(jseVariable,jseVariable))
      (jseDll[JSECOMPARE].Proc))(variable1,variable2);
}

   jsebool JSEPFUNC
jseConvert(jseContext jsecontext,jseVariable variable,jseDataType dType,uint dimension)
{                                    
   return ((jsebool (PFUNC FAR_CALL *)(jseContext,jseVariable,jseDataType,uint))
      (jseDll[JSECONVERT].Proc))(jsecontext,variable,dType,dimension);
}


   jseVariable JSEPFUNC
jseCreateLongVariable(slong l)
{
   return ((jseVariable (PFUNC FAR_CALL *)(slong))
      (jseDll[JSECREATELONGVARIABLE].Proc))(l);
}

   jseVariable JSEPFUNC
jseCreateSiblingVariable(jseVariable OlderSiblingVar,slong ElementOffsetFromOlderSibling)
{
   return ((jseVariable (PFUNC FAR_CALL *)(jseVariable,slong))
      (jseDll[JSECREATESIBLINGVARIABLE].Proc))(OlderSiblingVar,ElementOffsetFromOlderSibling);
}

z   jseStack JSEPFUNC
jseCreateStack(void)
{
   return ((jseStack (PFUNC FAR_CALL *)(void))
      (jseDll[JSECREATESTACK].Proc))();
}

   jseVariable JSEPFUNC
jseCreateVariable(jseDataType VType,uint InitialDimension)                
{
   return ((jseStack (PFUNC FAR_CALL *)(jseDataType,uint))
      (jseDll[JSECREATEVARIABLE].Proc))(VType,InitialDimension);
}

   jseContext JSEPFUNC
jseCurrentjseContext(jseContext AncestorjseContext)
{
   return ((jseContext (PFUNC FAR_CALL *)(jseContext))
      (jseDll[JSECURRENTCONTEXT].Proc))(AncestorjseContext);
}

   void JSEPFUNC
jseDestroyStack(jseStack jsestack)
{
   ((void (PFUNC FAR_CALL *)(jseStack))
      (jseDll[JSEDESTROYSTACK].Proc))(jsestack);
}

   void JSEPFUNC
jseDestroyVariable(jseVariable variable)
{
   ((void (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSEDESTROYVARIABLE].Proc))(variable);
}

   jsebool JSEPFUNC
jseEvaluateBoolean(jseContext jsecontext,jseVariable variable,jsebool _FAR_ *BooleanResult)
{                                     
   return ((jsebool (PFUNC FAR_CALL *)(jseContext,jseVariable,jsebool _FAR_ *))
      (jseDll[JSEEVALUATEBOOLEAN].Proc))(jsecontext,variable,BooleanResult);
}

   jseVariable JSEPFUNC
jseFindGlobalVariable(jseContext jsecontext,CPP_CONST char _FAR_ *name)
{
   return ((jseVariable (PFUNC FAR_CALL *)(jseContext,CPP_CONST char _FAR_ *))
      (jseDll[JSEFINDGLOBALVARIABLE].Proc))(jsecontext,name);
}

   jsebool JSEPFUNC
jseFindNames(jseContext jsecontext,jseVariable variable,CPP_CONST char _FAR_ *Buffer,uint SizeOfBuffer)
{
   return ((jsebool (PFUNC FAR_CALL *)(jseContext,jseVariable,CPP_CONST char _FAR_ *,uint))
      (jseDll[JSEFINDNAMES].Proc))(jsecontext,variable,Buffer,SizeOfBuffer);
}

   jseVariable JSEPFUNC
jseFuncVar(jseContext jsecontext,uint ParameterOffset)
{
   return ((jseVariable (PFUNC FAR_CALL *)(jseContext,uint))
      (jseDll[JSEFUNCVAR].Proc))(jsecontext,ParameterOffset);
}

   uint JSEPFUNC
jseFuncVarCount(jseContext jsecontext)
{
   return ((uint (PFUNC FAR_CALL *)(jseContext))
      (jseDll[JSEFUNCVARCOUNT].Proc))(jsecontext);
}

   jseVariable JSEPFUNC
jseFuncVarNeed(jseContext jsecontext,uint ParameterOffset,jseVarNeeded need)
{
   return ((jseVariable (PFUNC FAR_CALL *)(jseContext,uint,jseVarNeeded))
      (jseDll[JSEFUNCVARNEED].Proc))(jsecontext,ParameterOffset,need);
}

   ulong JSEPFUNC
jseGetArrayLength(jseVariable variable,slong _FAR_ *MinIndex)
{
   return ((ulong (PFUNC FAR_CALL *)(jseVariable,slong _FAR_ *))
      (jseDll[JSEGETARRAYLENGTH].Proc))(variable,MinIndex);
}

   uchar JSEPFUNC
jseGetByte(jseVariable variable)
{
   return ((uchar (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSEGETBYTE].Proc))(variable);
}

   void _HUGE_ * JSEPFUNC
jseGetData(jseVariable variable,slong Index)
{
   return ((void _HUGE_ * (PFUNC FAR_CALL *)(jseVariable,slong))
      (jseDll[JSEGETDATA].Proc))(variable,Index);
}

   void _HUGE_ * JSEPFUNC
jseGetData0(jseVariable variable)
{
   return ((void _HUGE_ * (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSEGETDATA0].Proc))(variable);
}

   void _FAR_ * JSEPFUNC
jseGetDatumPtr(jseVariable variable)
{
   return ((void _FAR_ * (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSEGETDATUMPTR].Proc))(variable);
}

   jseFunction JSEPFUNC
jseGetFunction(jseContext jsecontext,CPP_CONST char _FAR_ *FunctionName,jsebool ErrorIfNotFound)
{
   return ((jseFunction (PFUNC FAR_CALL *)(jseContext,CPP_CONST char _FAR_ *,jsebool))
      (jseDll[JSEGETFUNCTION].Proc))(jsecontext,FunctionName,ErrorIfNotFound);
}

   slong JSEPFUNC
jseGetLong(jseVariable variable)
{
   return ((slong (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSEGETLONG].Proc))(variable);
}

   slong JSEPFUNC
jseGetLongFromArray(jseVariable variable,slong Index)
{
   return ((slong (PFUNC FAR_CALL *)(jseVariable,slong))
      (jseDll[JSEGETLONGFROMARRAY].Proc))(variable,Index);
}

   jseVariable JSEPFUNC
jseGetNextMember(jseVariable structure_var,jseVariable PrevjseVariable,CPP_CONST char _FAR_ * _FAR_ * Name)
{
   return ((jseVariable (PFUNC FAR_CALL *)(jseVariable,jseVariable,CPP_CONST char _FAR_ * _FAR_ *))
      (jseDll[JSEGETNEXTMEMBER].Proc))(structure_var,PrevjseVariable,Name);
}

   jseDataType JSEPFUNC
jseGetType(jseVariable variable)
{
   return ((jseDataType (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSEGETTYPE].Proc))(variable);
}

   jseContext JSEPFUNC
jseInitializeExternalLink(void _FAR_ *LinkData,
            jseErrorMessageFunc PrintErrorFunc,jseMayIContinueFunc MayIContinue,
            uword16 _FAR_ *jseDllVersionID)
{
   if ( !LinkjseDll() )
      return NULL;
   return ((jseContext (PFUNC FAR_CALL *)(void _FAR_ *,jseErrorMessageFunc,jseMayIContinueFunc,uword16 _FAR_ *))
      (jseDll[JSEINITIALIZEEXTERNALLINK].Proc))(LinkData,PrintErrorFunc,MayIContinue,jseDllVersionID);
}

   jsebool JSEPFUNC
jseInterpret(jseContext jsecontext,CPP_CONST char _FAR_ * SourceText,
             jsebool RunAsChild/*else run in same context*/,
             int HowToInterpret,jseContext LocalVariableContext, // NULL if not inherit local variables
             int _FAR_ *ReturnCode)
{
   return ((jsebool (PFUNC FAR_CALL *)(jseContext,CPP_CONST char _FAR_ *,jsebool,
                              int,jseContext,int _FAR_ *))
   (jseDll[JSEINTERPRET].Proc))(jsecontext,SourceText,RunAsChild,HowToInterpret,LocalVariableContext,ReturnCode);
}

   void _FAR_ * JSEPFUNC
jseLibraryData(jseContext jsecontext)
{
   return ((void _FAR_ * (PFUNC FAR_CALL *)(jseContext))
      (jseDll[JSELIBRARYDATA].Proc))(jsecontext);
}

   void JSEPFUNC
jseLibSetErrorFlag(jseContext jsecontext)
{
   ((void (PFUNC FAR_CALL *)(jseContext))
      (jseDll[JSELIBSETERRORFLAG].Proc))(jsecontext);
}

   void JSEPFUNC
jseLibSetExitFlag(jseContext jsecontext,sint ExitCode)
{
   ((void (PFUNC FAR_CALL *)(jseContext,sint))
      (jseDll[JSELIBSETEXITFLAG].Proc))(jsecontext,ExitCode);
}

   char _FAR_ * JSEPFUNC
jseLocateSource(jseContext jsecontext,uint _FAR_ *LineNumber)
{
   return ((char _FAR_ * JSEPFUNC (PFUNC FAR_CALL *)(jseContext,uint _FAR_ *))
      (jseDll[JSELOCATESOURCE].Proc))(jsecontext,LineNumber);
}

   void JSEPFUNC
jseMakeNullPtr(jseVariable variable)
{
   ((void (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSEMAKENULLPTR].Proc))(variable);
}

   void JSEPFUNC
jseMakeString(jseVariable variable)
{
   ((void (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSEMAKESTRING].Proc))(variable);
}

   jseVariable JSEPFUNC
jseMemberEx(jseVariable structure_var,CPP_CONST char _FAR_ *Name,jsebool CreateIfNotThere,
          jseDataType DType,uint InitialDimension,uword16 flags)
{
   return ((jseVariable (PFUNC FAR_CALL *)(jseVariable,CPP_CONST char _FAR_ *,jsebool,jseDataType,uint))
      (jseDll[JSEMEMBEREX].Proc))(structure_var,Name,CreateIfNotThere,DType,InitialDimension,flags);
}

   CPP_CONST char _FAR_ * JSEPFUNC
jseNextLocalFunctionName(jseContext jsecontext,void _FAR_ * _FAR_ * EnumHandle)
{
   return ((CPP_CONST char _FAR_ * (PFUNC FAR_CALL *)(jseContext,void _FAR_ * _FAR_ *))
      (jseDll[JSENEXTLOCALFUNCTIONNAME].Proc))(jsecontext,EnumHandle);
}

   jsebool JSEPFUNC
jseNullPtr(jseVariable variable)
{
   return ((jsebool (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSENULLPTR].Proc))(variable);
}

   jsebool JSEPFUNC
jseNumeric(jseVariable variable)
{
   return ((jsebool (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSENUMERIC].Proc))(variable);
}

   void JSEPFUNC
jsePreDefineLong(jseContext jsecontext,CPP_CONST char _FAR_ *FindString,slong ReplaceL)
{
   ((void (PFUNC FAR_CALL *)(jseContext,CPP_CONST char _FAR_ *,slong))
      (jseDll[JSEPREDEFINELONG].Proc))(jsecontext,FindString,ReplaceL);
}

   void JSEPFUNC
jsePreDefineString(jseContext jsecontext,CPP_CONST char _FAR_ *FindString,CPP_CONST char _FAR_ *ReplaceString)
{
   ((void (PFUNC FAR_CALL *)(jseContext,CPP_CONST char _FAR_ *,CPP_CONST char _FAR_ *))
      (jseDll[JSEPREDEFINESTRING].Proc))(jsecontext,FindString,ReplaceString);
}

   jseContext JSEPFUNC
jsePreviousContext(jseContext jsecontext)
{
   return ((jseContext (PFUNC FAR_CALL *)(jseContext))
      (jseDll[JSEPREVIOUSCONTEXT].Proc))(jsecontext);
}

   void JSEPFUNC
jsePush(jseStack jsestack,jseVariable variable,jsebool DeletejseVariableWhenFinished)
{
   ((void (PFUNC FAR_CALL *)(jseStack,jseVariable,jsebool))
      (jseDll[JSEPUSH].Proc))(jsestack,variable,DeletejseVariableWhenFinished);
}

   void JSEPFUNC
jsePutByte(jseVariable variable,uchar c)
{
   ((void (PFUNC FAR_CALL *)(jseVariable,uchar))
      (jseDll[JSEPUTBYTE].Proc))(variable,c);
}

   void JSEPFUNC
jsePutData(jseVariable variable,CPP_CONST void _HUGE_ *data,slong Index,ulong Count)
{
   ((void (PFUNC FAR_CALL *)(jseVariable,CPP_CONST void _HUGE_ *,slong,ulong))
      (jseDll[JSEPUTDATA].Proc))(variable,data,Index,Count);
}

   void JSEPFUNC
jsePutLong(jseVariable variable,slong l)
{
   ((void (PFUNC FAR_CALL *)(jseVariable,slong))
      (jseDll[JSEPUTLONG].Proc))(variable,l);
}

   uint JSEPFUNC
jseQuitFlagged(jseContext jsecontext)
{                                 
   return ((uint (PFUNC FAR_CALL *)(jseContext))
      (jseDll[JSEQUITFLAGGED].Proc))(jsecontext);
}

   void JSEPFUNC
jseReturnLong(jseContext jsecontext,slong l)
{
   ((void (PFUNC FAR_CALL *)(jseContext,slong))
      (jseDll[JSERETURNLONG].Proc))(jsecontext,l);
}

   void JSEPFUNC
jseReturnVar(jseContext jsecontext,jseVariable variable,jseReturnAction RetAction)
{
   ((void (PFUNC FAR_CALL *)(jseContext,jseVariable,jseReturnAction))
      (jseDll[JSERETURNVAR].Proc))(jsecontext,variable,RetAction);

}

   void JSEPFUNC
jseSetArrayLength(jseVariable variable,slong MinIndex,ulong Length)
{
   ((void (PFUNC FAR_CALL *)(jseVariable,slong,ulong))
      (jseDll[JSESETARRAYLENGTH].Proc))(variable,MinIndex,Length);
}

   jsebool JSEPFUNC
jseString(jseVariable variable)
{
   return ((jsebool (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSESTRING].Proc))(variable);
}

   void JSEPFUNC
jseTerminateExternalLink(jseContext jsecontext)
{
   ((void (PFUNC FAR_CALL *)(jseContext))
      (jseDll[JSETERMINATEEXTERNALLINK].Proc))(jsecontext);
   UnlinkjseDll();
}

   void JSEPFUNC
jseUnDefine(jseVariable variable)
{
   ((void (PFUNC FAR_CALL *)(jseVariable))
      (jseDll[JSEUNDEFINE].Proc))(variable);
}

   jsebool JSEPFUNC
jseVarNeed(jseContext jsecontext,jseVariable variable,jseVarNeeded need)
{
   return ((jsebool (PFUNC FAR_CALL *)(jseContext,jseVariable,jseVarNeeded))
      (jseDll[JSEVARNEED].Proc))(jsecontext,variable,need);
}

   static void
jseDllRunCallF(enum jseDllTableEnums CallFuncEnum,va_list valist,jseContext jsecontext,CPP_CONST char _FAR_ * FormatS)
{
   ulong vars[14];
   int i;
   for ( i = 0; i < 14; i++ ) {
      vars[i] = va_arg(valist,ulong);
   } /* endfor */
   ((void (CFUNC FAR_CALL *)(jseContext,CPP_CONST char _FAR_ *,...))
      (jseDll[CallFuncEnum].Proc))(jsecontext,FormatS,
            vars[0],vars[1],vars[2],vars[3],vars[4],vars[5],vars[6],
            vars[7],vars[8],vars[9],vars[10],vars[11],vars[12],vars[13]);
}

   void JSECFUNC
jseLibErrorPrintf(jseContext ExitjseContext,CPP_CONST char _FAR_ * FormatS,...)
{
   va_list  valist;
   va_start(valist,FormatS);
   jseDllRunCallF(_JSELIBERRORPRINTF,valist,ExitjseContext,FormatS);
   va_end(valist); 
}

   JSECALLSEQ(jseVariable)
jseGetCurrentThisVariable(jseContext jsecontext) {
  return ((jsebool (PFUNC FAR_CALL *)(jseContext))
     (jseDll[JSEGETCURRENTTHISVARIABLE].Proc))(jsecontext);
}

#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   jsefloat JSEPFUNC
jseGetFloat(jseVariable variable)
{
   jsefloat f;
   ((void (PFUNC FAR_CALL *)(jseVariable,jsefloat _FAR_ *))
      (jseDll[JSEGETFLOATINDIRECT].Proc))(variable,&f);
   return f;
}

   void JSEPFUNC
jsePreDefineFloat(jseContext jsecontext,CPP_CONST char _FAR_ *FindString,jsefloat ReplaceF)
{
   ((void (PFUNC FAR_CALL *)(jseContext,CPP_CONST char _FAR_ *,jsefloat))
      (jseDll[JSEPREDEFINEFLOAT].Proc))(jsecontext,FindString,ReplaceF);
}

   void JSEPFUNC
jsePutFloat(jseVariable variable,jsefloat f)
{
   ((void (PFUNC FAR_CALL *)(jseVariable,jsefloat))
      (jseDll[JSEPUTFLOAT].Proc))(variable,f);
}

   void JSEPFUNC
jseReturnFloat(jseContext jsecontext,jsefloat f)
{
   ((void (PFUNC FAR_CALL *)(jseContext,jsefloat))
      (jseDll[JSERETURNFLOAT].Proc))(jsecontext,f);
}
#endif /* JSE_FLOATING_POINT */


#endif

#else /* __JSE_DLLRUN__ */
  ALLOW_EMPTY_FILE
#endif
