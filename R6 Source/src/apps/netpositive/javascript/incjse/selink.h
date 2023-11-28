/* extnsn.h - Code for external link libraries */

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

#if !defined(_EXTNSN_H)
#define _EXTNSN_H

#if defined(JSE_LINK) && (0!=JSE_LINK)

#if !defined(__JSE_WIN16__)  && !defined(__JSE_WIN32__) && !defined(__JSE_CON32__) && \
    !defined(__JSE_OS2TEXT__) && !defined(__JSE_OS2PM__) && !defined(__JSE_UNIX__) && \
    !defined(__JSE_MAC__) && !defined(__JSE_NWNLM__)
  #error Extensions are not yet supported on this platform
#endif

#if defined(__JSE_WIN32__) || defined(__JSE_CON32__) || defined(__JSE_WIN16__)
   #include <windows.h>
#endif

#define JSEEXTERNALVER  420

#if !defined(__JSE_UNIX__) && !defined(__JSE_MAC__)
  #if defined(__BORLANDC__)
   #pragma option -a1
  #else
   #pragma pack( 1 )
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__JSE_WIN16__)
  #define JSEEXTNSN_API                    _FAR_ _export _pascal
  #define JSEEXTNSN_EXPORT(rettype)        rettype _FAR_ _export _pascal
#elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)
 #if !defined(_MSC_VER)
  #define JSEEXTNSN_API                    _export CDECL
  #define JSEEXTNSN_EXPORT(rettype)        rettype _export _cdecl
 #else
  #define JSEEXTNSN_API                    _declspec(dllexport) CDECL
  #define JSEEXTNSN_EXPORT(rettype)        _declspec(dllexport) rettype _cdecl
 #endif
#elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
  #if defined(__IBMCPP__)
    #define JSEEXTNSN_API                    _Export
    #define JSEEXTNSN_EXPORT(rettype)        rettype _Export
  #else
    #define JSEEXTNSN_API                    _export _cdecl
    #define JSEEXTNSN_EXPORT(rettype)        rettype _export _cdecl
  #endif
#elif defined(__JSE_UNIX__)
  #define JSEEXTNSN_API
  #define JSEEXTNSN_EXPORT(type)           type
#elif defined(__JSE_NWNLM__)
  #define JSEEXTNSN_API
  #define JSEEXTNSN_EXPORT(type)           type
#elif defined(__JSE_MAC__)
  #define JSEEXTNSN_API
  #define JSEEXTNSN_EXPORT(type)           type
#else
  #error define the extension qualifiers
#endif

jsebool JSE_CFUNC jseCheckCEnviVersion(jseContext jsecontext);

struct jseFuncTable_t {
/* order is important here.  Any new functions must be added to the the
   end of this list.  If a function's interface changes copy the function
   to the end with a new name (provides backward compatability).
*/
#if defined(__JSE_WIN16__)
  uword16 DS;
#endif
  uword32 TableSize;
  uword32 Version;
  jseVariable (JSE_CFUNC _FAR_* jseCreateVariable)(jseContext jsecontext,jseDataType VType);
  jseVariable (JSE_CFUNC _FAR_* jseCreateSiblingVariable)(jseContext jsecontext,jseVariable OlderSiblingVar,JSE_POINTER_SINDEX ElementOffsetFromOlderSibling);
/*  jseVariable (JSE_CFUNC _FAR_* jseCreateCloneVariable)(jseContext,jseVariable VariableToClone); */
  jseVariable (JSE_CFUNC _FAR_* jseCreateConvertedVariable)(jseContext jsecontext,jseVariable VariableToConvert,jseConversionTarget TargetType);
  jseVariable (JSE_CFUNC _FAR_* jseCreateLongVariable)(jseContext jsecontext,slong l);
  jseVariable (JSE_CFUNC _FAR_* jseCreateFunctionTextVariable)(jseContext jsecontext,jseVariable FuncVar);
  void (JSE_CFUNC _FAR_* jseDestroyVariable)(jseContext jsecontext,jseVariable variable);
  JSE_POINTER_UINDEX (JSE_CFUNC _FAR_* jseGetArrayLength)(jseContext jsecontext,jseVariable variable,JSE_POINTER_SINDEX *MinIndex);
  void (JSE_CFUNC _FAR_* jseSetArrayLength)(jseContext jsecontext,jseVariable variable,JSE_POINTER_SINDEX MinIndex,JSE_POINTER_UINDEX Length);
  void (JSE_CFUNC _FAR_* jseSetAttributes)(jseContext jsecontext,jseVariable var,jseVarAttributes attr);
  jseVarAttributes (JSE_CFUNC _FAR_ * jseGetAttributes)(jseContext jsecontext,jseVariable var);
  jseDataType (JSE_CFUNC _FAR_* jseGetType)(jseContext jsecontext,jseVariable variable);
  void (JSE_CFUNC _FAR_* jseConvert)(jseContext jsecontext,jseVariable variable,jseDataType dType);
  jsebool (JSE_CFUNC _FAR_* jseAssign)(jseContext jsecontext,jseVariable variable,jseVariable SrcVar);
  ubyte (JSE_CFUNC _FAR_* jseGetByte)(jseContext jsecontext,jseVariable variable);
  void (JSE_CFUNC _FAR_* jsePutByte)(jseContext jsecontext,jseVariable variable,ubyte c);
  slong (JSE_CFUNC _FAR_* jseGetLong)(jseContext jsecontext,jseVariable variable);
  void (JSE_CFUNC _FAR_* jsePutLong)(jseContext jsecontext,jseVariable variable,slong l);

  const jsechar _HUGE_ * (JSE_CFUNC _FAR_* jseGetString)(jseContext jsecontext,jseVariable variable,JSE_POINTER_UINDEX *filled);
  const void _HUGE_ * (JSE_CFUNC _FAR_* jseGetBuffer)(jseContext jsecontext,jseVariable variable,JSE_POINTER_UINDEX *filled);
  jsechar _HUGE_ * (JSE_CFUNC _FAR_* jseGetWriteableString)(jseContext jsecontext,jseVariable variable,JSE_POINTER_UINDEX *filled);
  void _HUGE_ * (JSE_CFUNC _FAR_* jseGetWriteableBuffer)(jseContext jsecontext,jseVariable variable,JSE_POINTER_UINDEX *filled);
  void (JSE_CFUNC _FAR_* jsePutString)(jseContext jsecontext,jseVariable variable,const jsechar _HUGE_ *data);
  void (JSE_CFUNC _FAR_* jsePutStringLength)(jseContext jsecontext,jseVariable variable,const jsechar _HUGE_ *data,JSE_POINTER_UINDEX size);
  void (JSE_CFUNC _FAR_* jsePutBuffer)(jseContext jsecontext,jseVariable variable,const void _HUGE_ *data,JSE_POINTER_UINDEX size);
  JSE_POINTER_UINDEX (JSE_CFUNC _FAR_* jseCopyString)(jseContext jsecontext,jseVariable variable,jsechar _HUGE_ *buffer,JSE_POINTER_UINDEX start,JSE_POINTER_UINDEX length);
  JSE_POINTER_UINDEX (JSE_CFUNC _FAR_* jseCopyBuffer)(jseContext jsecontext,jseVariable variable,void _HUGE_ *buffer,JSE_POINTER_UINDEX start,JSE_POINTER_UINDEX length);

  jsebool (JSE_CFUNC _FAR_* jseEvaluateBoolean)(jseContext jsecontext,jseVariable variable);
  jsebool (JSE_CFUNC _FAR_* jseCompare)(jseContext jsecontext,jseVariable variable1,jseVariable variable2,slong *CompareResult);

  jseVariable (JSE_CFUNC _FAR_* jseMemberEx)(jseContext jsecontext,jseVariable structure_var,const jsechar *Name,jseDataType DType,uword16 flags);
  jseVariable (JSE_CFUNC _FAR_* jseIndexMemberEx)(jseContext,jseVariable struct_variable,JSE_POINTER_SINDEX index,jseDataType DType,uword16 flags);

  jseVariable (JSE_CFUNC _FAR_* jseGetNextMember)(jseContext jsecontext,jseVariable structure_var,jseVariable PrevMemberVariable,const jsechar * * Name);
  void (JSE_CFUNC _FAR_* jseDeleteMember)(jseContext jsecontext,jseVariable structure_var,const jsechar *Name);
  jseVariable (JSE_CFUNC _FAR_ * jseGlobalObject)(jseContext jsecontext);
  void (JSE_CFUNC _FAR_ * jseSetGlobalObject)(jseContext jsecontext,jseVariable newObject);
  jseVariable (JSE_CFUNC _FAR_ * jseActivationObject)(jseContext jsecontext);
  jseVariable (JSE_CFUNC _FAR_* jseGetCurrentThisVariable)(jseContext jsecontext);
  jseStack (JSE_CFUNC _FAR_* jseCreateStack)(jseContext jsecontext);
  void (JSE_CFUNC _FAR_* jseDestroyStack)(jseContext jsecontext,jseStack jsestack);
  void (JSE_CFUNC _FAR_* jsePush)(jseContext jsecontext,jseStack jsestack,jseVariable variable,jsebool DeleteVariableWhenFinished);
  uint (JSE_CFUNC _FAR_* jseFuncVarCount)(jseContext jsecontext);
  jseVariable (JSE_CFUNC _FAR_* jseFuncVar)(jseContext jsecontext,uint ParameterOffset);
  jseVariable (JSE_CFUNC _FAR_* jseFuncVarNeed)(jseContext jsecontext,uint ParameterOffset,jseVarNeeded need);
  jsebool (JSE_CFUNC _FAR_* jseVarNeed)(jseContext jsecontext,jseVariable variable,jseVarNeeded need);
  void (JSE_CFUNC _FAR_* jseReturnVar)(jseContext jsecontext,jseVariable variable,jseReturnAction RetAction);
  void (JSE_CFUNC _FAR_* jseReturnLong)(jseContext jsecontext,slong l); /* shortcut */
  void _FAR_ * (JSE_CFUNC _FAR_* jseLibraryData)(jseContext jsecontext);
  jseVariable (JSE_CFUNC _FAR_* jseGetFunction)(jseContext jsecontext,jseVariable Object,const jsechar *FunctionName,
                                                jsebool ErrorIfNotFound);
  jsebool (JSE_CFUNC _FAR_ * jseIsFunction)(jseContext jsecontext,jseVariable Object);
  jsebool (JSE_CFUNC _FAR_* jseCallFunction)(jseContext jsecontext,jseVariable jsefuncvar,
                                 jseStack jsestack,jseVariable *ReturnVar,jseVariable ThisVar);
  jseContext (JSE_CFUNC _FAR_* jsePreviousContext)(jseContext jsecontext); /* may be NULL */
  jseContext (JSE_CFUNC _FAR_* jseCurrentContext)(jseContext AncestorContext); /* not NULL */
  jsebool (JSE_CFUNC _FAR_ * jseIsLibraryFunction)(jseContext jsecontext,jseVariable FunctionVariable);
  jseVariable (JSE_CFUNC _FAR_ * jseCreateWrapperFunction)(jseContext jsecontext,
         const jsechar *FunctionName,
         void (JSE_CFUNC FAR_CALL *FuncPtr)(jseContext jsecontext),
         sword8 MinVariableCount, sword8 MaxVariableCount,jseVarAttributes VarAttributes,
         jseFuncAttributes FuncAttributes,void _FAR_ *fData);
  jseVariable (JSE_CFUNC _FAR_ * jseMemberWrapperFunction)(jseContext jsecontext,
         jseVariable objectVar,
         const jsechar *FunctionName,
         void (JSE_CFUNC FAR_CALL *FuncPtr)(jseContext jsecontext),
         sword8 MinVariableCount, sword8 MaxVariableCount,jseVarAttributes VarAttributes,
         jseFuncAttributes FuncAttributes,void _FAR_ *fData);

   void (JSE_CFUNC _FAR_* jsePreDefineLong)(jseContext jsecontext,const jsechar *FindString,slong ReplaceL);
   void (JSE_CFUNC _FAR_* jsePreDefineString)(jseContext jsecontext,const jsechar *FindString,const jsechar *ReplaceString);
     void (JSE_CFUNC _FAR_ * jsePreDefineNumber)(jseContext jsecontext,const jsechar *FindString,jsenumber ReplaceF);
  jsebool (JSE_CFUNC _FAR_* jseCallAtExit)(jseContext jsecontext,jseAtExitFunc ExitFunc,void _FAR_ *Param);
  void (JSE_CFUNC _FAR_* jseLibSetErrorFlag)(jseContext jsecontext); /* set flag that there has been an error */
  void (JSE_CFUNC _FAR_* jseLibErrorPrintf)(jseContext ExitContext,const jsechar * FormatS,...);
  void (JSE_CFUNC _FAR_* jseLibSetExitFlag)(jseContext jsecontext,jseVariable ExitVar);
  uint (JSE_CFUNC _FAR_* jseQuitFlagged)(jseContext jsecontext);
  jsebool (JSE_CFUNC _FAR_* jseInterpret)(jseContext jsecontext,
                                 const jsechar * SourceFile,
                                 const jsechar * SourceText,
                                 const void * PreTokenizedSource,
                                 jseNewContextSettings NewContextSettings,
                                 int fHowToInterpret,
                                 jseContext LocalVariableContext,
                                 jseVariable *ReturnVar);
  void (JSE_CFUNC _FAR_* jseAddLibrary)(jseContext jsecontext,const jsechar * object_var_name,
                                        const struct jseFunctionDescription *FunctionList,
                                        void _FAR_ *InitLibData,
                                        jseLibraryInitFunction LibInit,
                                        jseLibraryTermFunction LibTerm);
  const jsechar * (JSE_CFUNC _FAR_* jseLocateSource)(jseContext jsecontext,uint *LineNumber);
  const jsechar * (JSE_CFUNC _FAR_* jseCurrentFunctionName)(jseContext jsecontext);
  jsechar * * (JSE_CFUNC _FAR_* jseGetFileNameList)(jseContext jsecontext,int *number);
  jseContext (JSE_CFUNC _FAR_ * jseInitializeExternalLink)(void _FAR_ *LinkData,struct jseExternalLinkParameters * LinkParms,
                           const jsechar * GlobalObjectName,const char * AccessKey);
  void (JSE_CFUNC _FAR_ *jseTerminateExternalLink)(jseContext jsecontext);
  void _FAR_ * (JSE_CFUNC _FAR_ * jseGetLinkData)(jseContext jsecontext);
  struct jseExternalLinkParameters * (JSE_CFUNC _FAR_ * jseGetExternalLinkParameters)(jseContext jsecontext);
  jseContext (JSE_CFUNC _FAR_ *jseAppExternalLinkRequest)(jseContext jsecontext,jsebool Initialize);
  jsebool (JSE_CFUNC _FAR_ *jseBreakpointTest)(jseContext jsecontext,const jsechar *FileName,uint LineNumber);
  jsebool (JSE_CFUNC _FAR_ *jseTellSecurity)(jseContext jsecontext,jseVariable InfoVar);
  jseTokenRetBuffer (JSE_CFUNC _FAR_ *jseCreateCodeTokenBuffer)(jseContext jsecontext,
       const jsechar *Source,jsebool SourceIsFileName,
       uint *BufferLen);

void (JSE_CFUNC _FAR_ * jsePutNumber)(jseContext jsecontext,jseVariable variable,jsenumber f);
void (JSE_CFUNC _FAR_ * jseReturnNumber)(jseContext jsecontext,jsenumber f);
  void (JSE_CFUNC _FAR_ * jseGetFloatIndirect)(jseContext jsecontext,jseVariable variable,jsenumber *GetFloat);
  const jsechar * (JSE_CFUNC _FAR_ * jseGetLastApiError)();
  void (JSE_CFUNC _FAR_ * jseClearApiError)();
  jseVariable (JSE_CFUNC _FAR_ *jseFindVariable)(jseContext jsecontext, const jsechar * name, ulong flags);
  jsebool (JSE_CFUNC _FAR_ * jseGetVariableName)(jseContext jsecontext, jseVariable var, jsechar * const buffer, uint bufferSize);
  jseContext (JSE_CFUNC _FAR_ *jseInterpInit)(jseContext jsecontext,
              const jsechar * SourceFile,
              const jsechar * SourceText,
              const void * PreTokenizedSource,
              jseNewContextSettings NewContextSettings,
              int howToInterpret,
              jseContext localVariableContext);
  jseVariable (JSE_CFUNC _FAR_ *jseInterpTerm)(jseContext jsecontext);
  jseContext (JSE_CFUNC _FAR_ *jseInterpExec)(jseContext jsecontext);
  jsebool (JSE_CFUNC _FAR_ *jseGetBoolean)(jseContext jsecontext, jseVariable var);
  void (JSE_CFUNC _FAR_ *jsePutBoolean)(jseContext jsecontext, jseVariable var, jsebool val);
}; /* end of "struct jseFuncTable_t" */

JSEEXTNSN_EXPORT(long) jseExtensionVer(jseContext jsecontext);
JSEEXTNSN_EXPORT(jsebool) jseLoadExtension(jseContext jsecontext);

/* These must be provided by the .dll's author */

extern struct jseFunctionDescription ExtensionFuncTable[];
jsebool JSE_CFUNC FAR_CALL ExtensionLoadFunc(jseContext jsecontext);
void * JSE_CFUNC FAR_CALL ExtensionInitFunc(jseContext jsecontext,void * PrevData);
void JSE_CFUNC FAR_CALL ExtensionTermFunc(jseContext jsecontext,void * InstanceData);

#define FunctionIsSupported(jsecontext,name)  \
    (jseFuncs(jsecontext)->name != NULL)

#ifdef __cplusplus
}
#endif

#if !defined(__JSE_UNIX__) && !defined(__JSE_MAC__)
  #if defined(__BORLANDC__)
   #pragma option -a.
  #else
   #pragma pack( )
  #endif
#endif

#endif /*defined(JSE_LINK) */
#endif /*!defined(_EXTNSN_H) */

