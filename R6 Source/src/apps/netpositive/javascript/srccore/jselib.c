/* jselib.c   ISDK interface functions
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

   JSECALLSEQ( void )
jseLibSetExitFlag(jseContext jsecontext,jseVariable variable)
{
   VarRead *v;
   JSE_API_STRING(ThisFuncName,UNISTR("jseLibSetExitFlag"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   if ( NULL == variable )
   {
      /* API uses NULL to automatically return EXIT_SUCCESS */
      v = constructVarRead(jsecontext,VNumber);
      varPutLong(v,EXIT_SUCCESS);
   }
   else
   {
      JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return);
      v = GET_READABLE_VAR(variable,jsecontext);
   }

   callSetExitVar(jsecontext,v);
   VAR_REMOVE_USER(v,jsecontext);
}


/* To do the 'steps' of an interpret, first call jseInterpInit() passing the
 * same parameters as you would to the full interpret. This returns NULL if
 * there was some problem, else it returns a new context. This new context is
 * used to iterate the statements of the code being interpretted.
 */
   JSECALLSEQ( jseContext )
jseInterpInit(jseContext jsecontext,
              const jsechar * SourceFile,
              const jsechar * SourceText,
              const void * PreTokenizedSource,
              jseNewContextSettings NewContextSettings,
              int howToInterpret,
              jseContext localVariableContext
             )
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseInterpInit"),return NULL);
   callSetReasonToQuit(jsecontext,FlowNoReasonToQuit);
   return interpretInit(jsecontext,SourceFile,SourceText,PreTokenizedSource,
                        NewContextSettings,howToInterpret,localVariableContext);
}


/* Once the iteration is complete, you must clean up the interpret by
 * calling this routine. You pass the same original context you used to
 * begin the interpret, not any of the cookie contexts that were given
 * to you during the interpret. The return of this function is the
 * variable that was returned from the interpret. You must destroy the
 * variable when you are done with it. You may call this function
 * prematurely to cancel the interpret but in this case the return
 * will be NULL.
 */
   JSECALLSEQ( jseVariable )
jseInterpTerm(jseContext jsecontext)
{
   jseVariable ret;
   
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseInterpTerm"),return NULL);
   ret = interpretTerm(jsecontext);
   callSetReasonToQuit(jsecontext,FlowNoReasonToQuit);
   return ret;
}


/* Once you have set up for an interpret session using jseInterpInit(),
 * you actually execute the ScriptEase statements using successive calls
 * to this routine. Initially, you pass the context you received as a
 * result of jseInterpInit(). You will be returned a new context that you
 * pass back to execute the next statement. Continue calling this function
 * with the result of the last call as the parameter. When this function
 * returns NULL, the interpret is complete. At this time, you call the
 * jseInterpTerm() function to clean everything up and get the return
 * value.
 *
 * If you call jseInterpret(), your 'MayIContinue' function is called
 * after each statement. If you use the jseInterpXXX() functions directly,
 * your 'MayIContinue' statement is NOT called. Instead, you may execute
 * whatever code you like between successive calls to this function.
 * You may decide to discontinue executing code by calling jseInterpTerm()
 * at any time.
 *
 * Certain functions, due to the design of the interpreter, must be completely
 * processed and so are atomic to this function. This means that if you
 * do a jseCallFunction() or access a dynamic object, the statement will
 * not be executed iteratively by your jseInterpExec() but will be handled
 * behind the scenes all at once. Thus, when you call jseInterpExec() in these
 * cases, many statements can be processed for your one call. In this case,
 * your MayIContinue() function WILL be called during these statements.
 * Thus, you should always have a valid MayIContinue() function.
 */
   JSECALLSEQ( jseContext )
jseInterpExec(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseInterpExec"),return NULL);
   return secodeInterpret(jsecontext);
}


   JSECALLSEQ( jsebool )
jseInterpret(jseContext jsecontext,
             const jsechar * SourceFile,
             const jsechar * SourceText,
             const void * PreTokenizedSource,
             jseNewContextSettings NewContextSettings,
             int howToInterpret,
             jseContext localVariableContext,
             jseVariable *retvar)
{
   jseContext newc;
   VarRead *ret;
   
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseInterpret"),return False);

   callSetReasonToQuit(jsecontext,FlowNoReasonToQuit);
   newc = interpretInit(jsecontext,SourceFile,SourceText,PreTokenizedSource,
                        NewContextSettings,howToInterpret,localVariableContext);
   if( newc==NULL )
   {
      if( retvar!=NULL ) *retvar = NULL;
      return False;
   }

   while( (newc = secodeInterpret(newc))!=NULL )
   {
      if( !MayIContinue(newc) ) break;
   }

   ret = interpretTerm(jsecontext);
   if( retvar!=NULL )
   {
      *retvar = (Var *)ret;
   }
   else
   {
      if( ret!=NULL ) VAR_THOROUGH_REMOVE_USER(ret,jsecontext);
   }

   callSetReasonToQuit(jsecontext,FlowNoReasonToQuit);
   return True;
}


#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   JSECALLSEQ( jseTokenRetBuffer)
     jseCreateCodeTokenBuffer(jseContext jsecontext,
                              const jsechar *source,
                              jsebool sourceIsFileName
                                 /*else is source string*/,
                              uint *bufferLen)
{
#if 0
   struct Call *OldCall = jsecontext;
#endif
   struct Call *newCall;
   void *ret;
   JSE_API_STRING(ThisFuncName,UNISTR("jseCreateCodeTokenBuffer"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
   JSE_API_ASSERT_(source,2,ThisFuncName,return NULL);
   JSE_API_ASSERT_(bufferLen,4,ThisFuncName,return NULL);

   newCall = callNew(jsecontext,NULL,0,jseNewGlobalObject);
   
   ret = CompileIntoTokens(newCall,source,sourceIsFileName,bufferLen);
   
   callDelete(newCall);
   
   #if 0
   /* Create new call to compile on; if applinkfunc then let it create,
      else create from scratch */
   AppLinkFunc = OldCall->Global->ExternalLinkParms.AppLinkFunc;
   if ( NULL != AppLinkFunc )
   {
      struct Call * NewCall = (*AppLinkFunc)(OldCall,True);
      if ( NULL == NewCall ) {
         ret = NULL;
      } else {
         ret = CompileIntoTokens(NewCall,source,sourceIsFileName,bufferLen);
         (*AppLinkFunc)(NewCall,False);
      }
   } else {
      stringLengthType globalNameLen;
      const jsechar * globalName = GetStringTableEntry(OldCall,
                     GLOBAL_STRING(OldCall,userglobal_entry),&globalNameLen);
      /* no app link function, so just create from scratch */
      struct Call *NewCall = callInitial(OldCall->Global->GenericData,
         &(OldCall->Global->ExternalLinkParms),globalName,globalNameLen);
      ret = CompileIntoTokens(NewCall,source,sourceIsFileName,bufferLen);
      callDelete(NewCall);
   }
   #endif
   return ret;
}
#endif

   JSECALLSEQ( void )
jseAddLibrary(jseContext jsecontext, const jsechar * objectVariableName,
              const struct jseFunctionDescription *FunctionList,
              void _FAR_ *InitLibData,
              jseLibraryInitFunction libInitFunction,
              jseLibraryTermFunction libTermFunction)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jseAddLibrary"));
   
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_(FunctionList,3,ThisFuncName,return);
   
   libraryAddFunctions(jsecontext->session.TheLibrary,jsecontext,
                       objectVariableName,FunctionList,
                       libInitFunction,libTermFunction,InitLibData);
}

   JSECALLSEQ( jseVariable )
jseFuncVar(jseContext jsecontext,uint ParameterOffset)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseFuncVar"),return NULL);
   return ( callGetVar(jsecontext,ParameterOffset) );
}

   JSECALLSEQ( jseVariable )
jseFuncVarNeed(jseContext jsecontext,uint parameterOffset,jseVarNeeded need)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseFuncVarNeed"),return NULL);
   return ( callGetVarNeed(jsecontext,NULL,parameterOffset,need) );
}

   JSECALLSEQ( jsebool )
jseVarNeed(jseContext jsecontext,jseVariable variable,jseVarNeeded need)
{
   VarRead *v;
   jsebool ret;
   JSE_API_STRING(ThisFuncName,UNISTR("jseVarNeed"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return False);
   
   v = GET_READABLE_VAR(variable,jsecontext);
   ret = NULL != callGetVarNeed(jsecontext,v,0,need);
   VAR_REMOVE_USER(v,jsecontext);
   return ret;
}


#if ( 0 < JSE_API_ASSERTLEVEL )
static jsebool NEAR_CALL TestValidForArrayLength(jseContext jsecontext,
                                                 VarRead *var)
{
   VarType vType = VAR_TYPE(var);
   UNUSED_PARAMETER(jsecontext);
   if ( VObject != vType
#    if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
     && VBuffer != vType
#    endif
     && VString != vType )
   {
      SetLastApiError(UNISTR("Invalid var type for Get/SetArrayLength"));
      return False;
   }
   return True;
}
#endif

   JSECALLSEQ( JSE_POINTER_UINDEX )
jseGetArrayLength(jseContext jsecontext,jseVariable variable,
                  JSE_POINTER_SINDEX *MinIndex)
{
   VarRead *var;
   JSE_POINTER_UINDEX ret;
   JSE_API_STRING(ThisFuncName,UNISTR("jseGetArrayLength"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return 0);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return 0);
   
   var = GET_READABLE_VAR(variable,jsecontext);
#  if ( 0 < JSE_API_ASSERTLEVEL )
   if ( !TestValidForArrayLength(jsecontext,var) )
   {
      ret = 0;
      if ( NULL != MinIndex )
         *MinIndex = 0;
   }
   else
#  endif
   {
      ret = varGetArrayLength(var,jsecontext,MinIndex);
   }
   VAR_REMOVE_USER(var,jsecontext);
   return ret;
}

   JSECALLSEQ( void )
jseSetArrayLength(jseContext jsecontext,jseVariable variable,
                  JSE_POINTER_SINDEX MinIndex,JSE_POINTER_UINDEX Length)
{
   VarRead *var;
   JSE_API_STRING(ThisFuncName,UNISTR("jseSetArrayLength"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return);
   
   var = GET_READABLE_VAR(variable,jsecontext);
#  if ( 0 < JSE_API_ASSERTLEVEL )
   if ( TestValidForArrayLength(jsecontext,var) )
#  endif
   {
      varSetArrayLength(var,jsecontext,MinIndex,Length);
   }
   VAR_REMOVE_USER(var,jsecontext);
}

   JSECALLSEQ( void )
jseSetAttributes(jseContext jsecontext,jseVariable variable,
                 jseVarAttributes attr)
{
   VarRead *var;
   JSE_API_STRING(ThisFuncName,UNISTR("jseSetAttributes"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return);

   var = GET_READABLE_VAR(variable,jsecontext);
   varSetAttributes(var,(uword8)attr);
   VAR_REMOVE_USER(var,jsecontext);
}

   JSECALLSEQ( jseVarAttributes )
jseGetAttributes(jseContext jsecontext,jseVariable variable)
{
   VarRead *var;
   jseVarAttributes ret;
   JSE_API_STRING(ThisFuncName,UNISTR("jseGetAttributes"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return 0);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return 0);

   var = GET_READABLE_VAR(variable,jsecontext);
   ret = varGetAttributes(var);
   VAR_REMOVE_USER(var,jsecontext);
   return ret;
}

static void NEAR_CALL NumberReturn(jseContext jsecontext,jsenumber number)
{
   VarRead *var = constructVarRead(jsecontext,VNumber);
   varPutNumber(var,number);
   callReturnVar(jsecontext,var,jseRetTempVar);
}
   JSECALLSEQ( void )
jseReturnLong(jseContext jsecontext,slong longValue)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseReturnLong"),return);
   NumberReturn(jsecontext,(jsenumber)longValue);
}
#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   JSECALLSEQ( void )
jseReturnNumber(jseContext jsecontext,jsenumber number)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseReturnNumber"),return);
   NumberReturn(jsecontext,number);
}
#endif

   JSECALLSEQ( void )
jseReturnVar(jseContext jsecontext,jseVariable variable,
             jseReturnAction RetAction)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jseReturnVar"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return);
   
   callReturnVar(jsecontext,variable,RetAction);
}

   JSECALLSEQ( jsebool )
jseCompare(jseContext jsecontext,jseVariable variable1,jseVariable variable2,
           slong *CompareResult)
{
   VarRead *var1,*var2;
   jsebool ret;
   JSE_API_STRING(ThisFuncName,UNISTR("jseCompare"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(variable1,2,jseVariable_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(variable2,3,jseVariable_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_(CompareResult,4,ThisFuncName,return False);

   var1 = GET_READABLE_VAR((Var *)variable1,jsecontext);
   var2 = GET_READABLE_VAR((Var *)variable2,jsecontext);

   if( CompareResult==JSE_COMPLESS )
   {
      ret = (varCompareLess(jsecontext,var1,var2)==1);
   }
   else if( CompareResult==JSE_COMPEQUAL )
   {
      ret = varCompareEquality(jsecontext,var1,var2);
   }
   else
   {
      ret = varCompare(jsecontext,var1,var2,CompareResult);
   }

   VAR_REMOVE_USER(var1,jsecontext);
   VAR_REMOVE_USER(var2,jsecontext);
   return ret;
}

   JSECALLSEQ(jseVariable)  
jseFindVariable(jseContext jsecontext, const jsechar * name, ulong flags)
{
   jseContext LocalVariableContext = jsecontext;
   jseVariable ret;
   VarName varname;

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseFindVariableByName"),
                  return NULL);
                  
   /* caller may not know exactly how far up to find local variables,
    * so find most recent local function
    */
   while ( NULL != LocalVariableContext
       && NULL != callFunc(LocalVariableContext)
       && !functionTestIfLocal(callFunc(LocalVariableContext)) )
   {
      LocalVariableContext = callPrevious(LocalVariableContext);
   }
   
   /* Somehow we didn't find a local variable context, so use jsecontext */
   if( NULL == LocalVariableContext )
      LocalVariableContext = jsecontext;
   
   varname = EnterIntoStringTable(LocalVariableContext,name,strlen_jsechar(name));
   
   ret = FindVariableByName(LocalVariableContext,varname);
   RemoveFromStringTable(LocalVariableContext,varname);
 
   if( !(flags & jseCreateVar) )
      CALL_ADD_TEMP_VAR(jsecontext,ret);
   
   return ret;
}

   JSECALLSEQ( jseVariable )
jseCreateVariable(jseContext jsecontext,jseDataType VDataType)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseCreateVariable"),
                  return NULL);
   if( !isValidVarType(VDataType) )
   {
#     if ( 0 < JSE_API_ASSERTLEVEL )
#           if (0!=JSE_API_ASSERTNAMES)
               SetLastApiError(
      UNISTR("%s: Invalid data type"),UNISTR("jseCreateVariable"));
#        else
               SetLastApiError(UNISTR("Invalid data type"));
#        endif
#     endif
      return constructVarRead(jsecontext,(VarType)VUndefined);   
   }
   return( constructVarRead(jsecontext,(VarType)VDataType) );
}

   JSECALLSEQ( jseVariable )
jseCreateSiblingVariable(jseContext jsecontext,jseVariable olderSiblingVar,
                         JSE_POINTER_SINDEX elementOffsetFromOlderSibling)
{
   VarRead *var;
   jseVariable ret;
   JSE_API_STRING(ThisFuncName,UNISTR("jseCreateSinblingVariable"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
   JSE_API_ASSERT_C(olderSiblingVar,2,jseVariable_cookie,ThisFuncName,
                  return NULL);

   var = GET_READABLE_VAR(olderSiblingVar,jsecontext);
   ret = CONSTRUCT_SIBLING(jsecontext,var,elementOffsetFromOlderSibling,False);
   VAR_REMOVE_USER(var,jsecontext);
   return ret;
}

   JSECALLSEQ(jseVariable)
jseCreateConvertedVariable(jseContext jsecontext,jseVariable variableToConvert,
                           jseConversionTarget targetType)
{
   VarRead *var;
   jseVariable ret;
   JSE_API_STRING(ThisFuncName,UNISTR("jseCreateSinblingVariable"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
   JSE_API_ASSERT_C(variableToConvert,2,jseVariable_cookie,ThisFuncName,
                  return NULL);

   var = GET_READABLE_VAR(variableToConvert,jsecontext);
   ret = convert_var(jsecontext,var,targetType);
   VAR_REMOVE_USER(var,jsecontext);
   return ret;
}

   JSECALLSEQ( jseVariable )
jseCreateLongVariable(jseContext jsecontext,slong value)
{
   VarRead *ret;

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseCreateLongVariable"),
                  return NULL);
   
   ret = constructVarRead(jsecontext,VNumber);
   varPutLong(ret,value);
   return ret;
}

   JSECALLSEQ( void )
jseDestroyVariable(jseContext jsecontext,jseVariable variable)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jseDestroyVariable"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return);

   /* Before JSEGETMEMBERXXX had the create flag there was no way for
    * any API user to have the member field not set.  Now that is not true.
    */ 
        /* we don't want to be deleting references (i.e. member!=NULL)
         * 'cause the only way for the API to get them is via member access
         * which isn't supposed to be deleted.
         */
   /*   assert( varGetTheMember(variable)==NULL );
    *   if( varGetTheMember(variable)!=NULL)
    *   {
    *#     if ( 0 < JSE_API_ASSERTLEVEL )       
    *         SetLastApiError(UNISTR("variable is not yours to destroy."));
    *#     endif
    *      return;
    *   }
    */
   VAR_THOROUGH_REMOVE_USER(variable,jsecontext);
}

   JSECALLSEQ( jseDataType )
jseGetType(jseContext jsecontext,jseVariable variable)
{
   VarType ret;
   JSE_API_STRING(ThisFuncName,UNISTR("jseGetType"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return 0);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return 0);

   /* The idea is that if it is an undefined variable, return its type
    * as undefined rather than give an error. It allows defined() to work.
    * if the person actually tries to access its value, then we give an error
    */
   if ( VAR_HAS_DATA(variable) )
   {
      ret = VAR_TYPE(variable);
   }
   else if ( NULL == variable->reference.parentObject )
   {
      ret = VUndefined;
   }
   else
   {
      VarRead *var = GET_READABLE_VAR(variable,jsecontext);
      ret = VAR_TYPE(var);
      VAR_REMOVE_USER(var,jsecontext);
   }
   return ret;
}

   static jsenumber NEAR_CALL
GenericGetNumber(
#  if (0!=JSE_API_ASSERTNAMES)
      const jsechar * ThisFuncName,
#  endif
   jseContext jsecontext,jseVariable variable,sword8 varType)
{
   VarRead *var;
   jsenumber number;

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return jseNaN);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return jseNaN);

   var = GET_READABLE_VAR(variable,jsecontext);

   if( varType != VAR_TYPE(var) && 
       (jseOptLenientConversion & jsecontext->Global->ExternalLinkParms.options) )
   {
      VarRead * convar = convert_var(jsecontext,var,varType == VNumber ? jseToNumber : jseToBoolean);
      number = varGetNumber(convar);
      VAR_REMOVE_USER(convar,jsecontext);
   }
   else
      number = varGetNumber(var);

   VAR_REMOVE_USER(var,jsecontext);
   return number;
}
#if (0!=JSE_API_ASSERTNAMES)
#  define GENERIC_GET_NUMBER(FNAME,CNTXT,VAR,TYPE) GenericGetNumber(FNAME,CNTXT,VAR,TYPE)
#else
#  define GENERIC_GET_NUMBER(FNAME,CNTXT,VAR,TYPE) GenericGetNumber(CNTXT,VAR,TYPE)
#endif
#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   JSECALLSEQ( void )
jseGetFloatIndirect(jseContext jsecontext,jseVariable variable,
                    jsenumber *GetFloat)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jseGetFloatIndirect"));
   JSE_API_ASSERT_(GetFloat,3,ThisFuncName,return);
   

   *GetFloat = GENERIC_GET_NUMBER(ThisFuncName,jsecontext,variable,VNumber); 
}
#endif
   JSECALLSEQ( slong )
jseGetLong(jseContext jsecontext,jseVariable variable)
{
   return (slong) GENERIC_GET_NUMBER(UNISTR("jseGetLong"),jsecontext,variable,VNumber);
}
   JSECALLSEQ( ubyte )
jseGetByte(jseContext jsecontext,jseVariable variable)
{
   return (ubyte)GENERIC_GET_NUMBER(UNISTR("jseGetByte"),jsecontext,variable,VNumber);
}
   JSECALLSEQ( jsebool )
jseGetBoolean(jseContext jsecontext,jseVariable variable)
{
   return GENERIC_GET_NUMBER(UNISTR("jseGetBoolean"),jsecontext,variable,VBoolean) ? True : False ;
}

   JSECALLSEQ( jsebool )
jseEvaluateBoolean(jseContext jsecontext,jseVariable variable)
{
   VarRead *var;
   jsebool ret;
   JSE_API_STRING(ThisFuncName,UNISTR("jseEvaluateBoolean"));
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return False);
   var = GET_READABLE_VAR(variable,jsecontext);
   ret = ToBoolean(jsecontext,var);
   VAR_REMOVE_USER(var,jsecontext);
   return ret;
}

   static void NEAR_CALL
GenericPutNumber(
#  if (0!=JSE_API_ASSERTNAMES)
      const jsechar * ThisFuncName,
#  endif
   jseContext jsecontext,jseVariable variable,jsenumber number,sword8 varType)
{
   VarWrite *var;

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return);

   var = getWriteableVar(variable,jsecontext);
   assert( VAR_HAS_DATA(var) );

   if( !VAR_HAS_REFERENCE(var) && VAR_TYPE(var) != varType )
   {
      if( (jseOptLenientConversion & jsecontext->Global->ExternalLinkParms.options) )
         varConvert(var,jsecontext,varType);
      else
      {
#     if ( 0 < JSE_API_ASSERTLEVEL )
#        if (0!=JSE_API_ASSERTNAMES)
            SetLastApiError(
               UNISTR("%s: Data type is not correct numeric type"),ThisFuncName);
#        else
            SetLastApiError(UNISTR("Data type is not correct numeric type"));
#        endif
         VAR_REMOVE_USER(var,jsecontext);
         return;
#     endif
      }
   }

   varPutNumberFix(var,jsecontext,number,varType);
   VAR_REMOVE_USER(var,jsecontext);
}
#if (0!=JSE_API_ASSERTNAMES)
#  define GENERIC_PUT_NUMBER(FNAME,CNTXT,VAR,N,VARTYPE) \
      GenericPutNumber(FNAME,CNTXT,VAR,N,VARTYPE)
#else
#  define GENERIC_PUT_NUMBER(FNAME,CNTXT,VAR,N,VARTYPE) GenericPutNumber(CNTXT,VAR,N,VARTYPE)
#endif
   JSECALLSEQ( void )
jsePutNumber(jseContext jsecontext,jseVariable variable,jsenumber number)
{
   GENERIC_PUT_NUMBER(UNISTR("jsePutNumber"),jsecontext,variable,number,VNumber);
}
   JSECALLSEQ( void )
jsePutLong(jseContext jsecontext,jseVariable variable,slong longValue)
{
   GENERIC_PUT_NUMBER(UNISTR("jsePutLong"),jsecontext,variable,(jsenumber)longValue,VNumber);
}
   JSECALLSEQ( void )
jsePutByte(jseContext jsecontext,jseVariable variable,ubyte byteValue)
{
   GENERIC_PUT_NUMBER(UNISTR("jsePutByte"),jsecontext,variable,(jsenumber)byteValue,VNumber);
}
   JSECALLSEQ( void )
jsePutBoolean(jseContext jsecontext,jseVariable variable,jsebool boolValue)
{
   GENERIC_PUT_NUMBER(UNISTR("jsePutBoolean"),jsecontext,variable,boolValue,VBoolean);
}

   static void _HUGE_ * NEAR_CALL
GenericGetDataPtr(
#  if (0!=JSE_API_ASSERTNAMES)
      const jsechar * ThisFuncName,
#  endif
   jseContext jsecontext,jseVariable variable,
   JSE_POINTER_UINDEX *filled,VarType vType,
   jsebool Writeable)
{
   VarRead *var;
   void _HUGE_ *ptr;

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return NULL);

   assert( TYPE_IS_ARRAY(vType) );

   var = GET_READABLE_VAR(variable,jsecontext);
   if ( VAR_TYPE(var) != vType )
   {
      if( !Writeable && (jseOptLenientConversion & 
           jsecontext->Global->ExternalLinkParms.options) )
      {
         VarRead * convar = convert_var(jsecontext,variable,
#           if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
               vType == VBuffer ? jseToBuffer :
#           endif
            jseToString);
         ptr = (void _HUGE_ *) varGetData(convar,0);
         if( filled )
            *filled = varGetArrayLength(convar,jsecontext,NULL);
         CALL_ADD_TEMP_VAR(jsecontext,convar);
      }
      else
      {
         /* should not have called in this way; but don't crash */
         ptr = UNISTR("");
         if ( filled ) *filled = 0;
#     if ( 0 < JSE_API_ASSERTLEVEL )
#           if (0!=JSE_API_ASSERTNAMES)
               SetLastApiError(
      UNISTR("%s: Data type is not correct string or buffer type"),ThisFuncName);
#        else
               SetLastApiError(UNISTR("Data type is not correct string or buffer type"));
#        endif
#     endif
      }
   }
   else if ( Writeable )
   {
      ptr = GetWriteableData(variable,var,jsecontext,filled);
   }
   else
   {
      ptr = (void _HUGE_ *)varGetData(var,0);
      if( filled ) *filled = varGetArrayLength(var,jsecontext,NULL);
      /* put this variable on the tempvar list to be cleaned up when the
         function leaves */
      if( 1 == var->userCount )
      {
        varAddUser(var); /* compensate for VAR_REMOVE_USER at end */
        CALL_ADD_TEMP_VAR(jsecontext,var);
      }
   }
   VAR_REMOVE_USER(var,jsecontext);
   return ptr;
}
#if (0!=JSE_API_ASSERTNAMES)
#  define GENERIC_GET_DATAPTR(FNAME,CNTXT,VAR,FILLED,TYPE,WRITE) \
            GenericGetDataPtr(FNAME,CNTXT,VAR,FILLED,TYPE,WRITE)
#else
#  define GENERIC_GET_DATAPTR(FNAME,CNTXT,VAR,FILLED,TYPE,WRITE) \
            GenericGetDataPtr(CNTXT,VAR,FILLED,TYPE,WRITE)
#endif
   JSECALLSEQ( const jsechar _HUGE_ * )
jseGetString(jseContext jsecontext,jseVariable variable,
             JSE_POINTER_UINDEX *filled)
{
   return (const jsechar _HUGE_ *)
      GENERIC_GET_DATAPTR(UNISTR("jseGetString"),jsecontext,variable,filled,
                          VString,False);
}
#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
   JSECALLSEQ( const void _HUGE_ * )
jseGetBuffer(jseContext jsecontext,jseVariable variable,
             JSE_POINTER_UINDEX *filled)
{
   return (const void _HUGE_ *)
      GENERIC_GET_DATAPTR(UNISTR("jseGetBuffer"),jsecontext,variable,filled,
                          VBuffer,False);
}
#endif
   JSECALLSEQ( jsechar _HUGE_ * )
jseGetWriteableString(jseContext jsecontext,jseVariable variable,
                      JSE_POINTER_UINDEX *filled)
{
   return (jsechar _HUGE_ *)
      GENERIC_GET_DATAPTR(UNISTR("jseGetWriteableString"),jsecontext,variable,
                          filled,VString,True);
}
#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
   JSECALLSEQ( void _HUGE_ * )
jseGetWriteableBuffer(jseContext jsecontext,jseVariable variable,
                      JSE_POINTER_UINDEX *filled)
{
   return (jsechar _HUGE_ *)
      GENERIC_GET_DATAPTR(UNISTR("jseGetWriteableBuffer"),jsecontext,variable,
                          filled,VBuffer,True);
}
#endif

   static void NEAR_CALL
GenericPutDataPtr(
#  if (0!=JSE_API_ASSERTNAMES)
      const jsechar * ThisFuncName,
#  endif
   jseContext jsecontext,jseVariable variable,void _HUGE_ *data,
   VarType vType,JSE_POINTER_UINDEX *size)
{
   VarWrite *var;

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return);
#  if ( 0 < JSE_API_ASSERTLEVEL )
      if ( NULL != size  &&  0 != *size )
      {
         JSE_API_ASSERT_(data,3,ThisFuncName,return);
      }
#  endif
   assert( isValidVarType(vType) );

   var = getWriteableVar(variable,jsecontext);
   /* only if it is not a dynamic put waiting to happen */
   if ( !VAR_HAS_REFERENCE(var) && VAR_TYPE(var) != vType )
   {
      if( jsecontext->Global->ExternalLinkParms.options &
          jseOptLenientConversion )
         varConvert(var,jsecontext,vType);
      /* should not have called in this way; but don't crash */
#     if ( 0 < JSE_API_ASSERTLEVEL )
      else
      {
#        if (0!=JSE_API_ASSERTNAMES)
            SetLastApiError(
                 UNISTR("%s: Data type is not correct string or buffer type"),
                 ThisFuncName);
#        else
            SetLastApiError(UNISTR("Data type is not correct string or buffer type"));
#        endif
         VAR_REMOVE_USER(var,jsecontext);
         return;
      }
#     endif
   }

   if ( size )
   {
      /* this call can handle string or buffer */
      varPutData(var,jsecontext,data,*size,vType);
   }
   else
   {
      /* only way to not pass length is for null-term string length */
      assert( VString == vType );
      varPutString(var,jsecontext,data);
   }

   VAR_REMOVE_USER(var,jsecontext);
}
#if (0!=JSE_API_ASSERTNAMES)
#  define GENERIC_PUT_DATAPTR(FNAME,CNTXT,VAR,DATA,TYPE,LENPTR) \
            GenericPutDataPtr(FNAME,CNTXT,VAR,DATA,TYPE,LENPTR)
#else
#  define GENERIC_PUT_DATAPTR(FNAME,CNTXT,VAR,DATA,TYPE,LENPTR) \
            GenericPutDataPtr(CNTXT,VAR,DATA,TYPE,LENPTR)
#endif
   JSECALLSEQ( void )
jsePutString(jseContext jsecontext,jseVariable variable,
             const jsechar _HUGE_ *data)
{
   GENERIC_PUT_DATAPTR(UNISTR("jsePutString"),jsecontext,variable,
                       (void _HUGE_ *)data,VString,NULL);
}
   JSECALLSEQ( void )
jsePutStringLength(jseContext jsecontext,jseVariable variable,
                   const jsechar _HUGE_ *data,
                   JSE_POINTER_UINDEX size)
{
   GENERIC_PUT_DATAPTR(UNISTR("jsePutStringLen"),jsecontext,variable,
                       (void _HUGE_ *)data,VString,&size);
}
#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
   JSECALLSEQ( void )
jsePutBuffer(jseContext jsecontext,jseVariable variable,
             const void _HUGE_ *data,
             JSE_POINTER_UINDEX size)
{
   GENERIC_PUT_DATAPTR(UNISTR("jsePutStringLen"),jsecontext,variable,
                       (void _HUGE_ *)data,VBuffer,&size);
}
#endif
static JSE_POINTER_UINDEX CopyPtrData(
#  if (0!=JSE_API_ASSERTNAMES)
      const jsechar * ThisFuncName,
#  endif
   jseContext jsecontext,jseVariable variable,void _HUGE_ *buffer,
   JSE_POINTER_UINDEX start,JSE_POINTER_UINDEX length,
   VarType vType)
{
   JSE_POINTER_UINDEX filled;
   const void _HUGE_ *data;
   JSE_POINTER_UINDEX CopyLen;

   assert( isValidVarType(vType) );

   data = GENERIC_GET_DATAPTR(ThisFuncName,jsecontext,variable,&filled,
                              vType,False);
   if ( NULL == data )
      return 0;

   if( (start+length)>filled )
   {
      length = filled-start;
      if( start>filled || length==0 )
         return 0;
   }
   CopyLen = length;
   if ( VString == vType )
   {
      /* jsechars can take up extra room */
      start *= sizeof(jsechar);
      CopyLen *= sizeof(jsechar);
   }
   HugeMemMove(buffer,HugePtrAddition(data,start),CopyLen);

   return length;
}
#if (0!=JSE_API_ASSERTNAMES)
#  define COPY_PTR_DATA(FNAME,CNTXT,VAR,BUF,START,LEN,VTYPE) \
            CopyPtrData(FNAME,CNTXT,VAR,BUF,START,LEN,VTYPE)
#else
#  define COPY_PTR_DATA(FNAME,CNTXT,VAR,BUF,START,LEN,VTYPE) \
            CopyPtrData(CNTXT,VAR,BUF,START,LEN,VTYPE)
#endif
   JSECALLSEQ( JSE_POINTER_UINDEX )
jseCopyString(jseContext jsecontext,jseVariable variable,
              jsechar _HUGE_ *buffer,
              JSE_POINTER_UINDEX start,JSE_POINTER_UINDEX length)
{
   return COPY_PTR_DATA(UNISTR("jseCopyString"),jsecontext,variable,buffer,start,
                        length,VString);
}
#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
   JSECALLSEQ( JSE_POINTER_UINDEX )
jseCopyBuffer(jseContext jsecontext,jseVariable variable,void _HUGE_ *buffer,
              JSE_POINTER_UINDEX start,JSE_POINTER_UINDEX length)
{
   return COPY_PTR_DATA(UNISTR("jseCopyBuffer"),jsecontext,variable,buffer,start,
                        length,VBuffer);
}
#endif

   JSECALLSEQ( void )
jseConvert(jseContext jsecontext,jseVariable variable,jseDataType dType)
{
   VarWrite *var;
   JSE_API_STRING(ThisFuncName,UNISTR("jseConvert"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_C(variable,2,jseVariable_cookie,ThisFuncName,return);

   if( !isValidVarType(dType) )
   {
#     if ( 0 < JSE_API_ASSERTLEVEL )
#           if (0!=JSE_API_ASSERTNAMES)
               SetLastApiError(
      UNISTR("%s: Invalid data type"),UNISTR("jseConvert"));
#        else
               SetLastApiError(UNISTR("Invalid data type"));
#        endif
#     endif
   }
   else
   {
      var = getWriteableVar(variable,jsecontext);
      varConvert(var,jsecontext,(VarType)dType);
      VAR_REMOVE_USER(var,jsecontext);
   }
}

   JSECALLSEQ( jsebool )
jseAssign(jseContext jsecontext,jseVariable destVar,jseVariable srcVar)
{
   VarWrite *var;
   VarRead *src;
   jsebool ret;
   JSE_API_STRING(ThisFuncName,UNISTR("jseAssign"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(destVar,2,jseVariable_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(srcVar,3,jseVariable_cookie,ThisFuncName,return False);

   var = getWriteableVar(destVar,jsecontext);
   src = GET_READABLE_VAR(srcVar,jsecontext);
   ret = varAssign(var,jsecontext,src);
#  if ( 0 != JSE_DYNAMIC_OBJ_INHERIT )
      /* if assigning a _prototype to another object, then inherit
       * the dynamic properties of that other object
       */
      if ( destVar->reference.memberName == GLOBAL_STRING(jsecontext,prototype_entry)
        && NULL != destVar->reference.parentObject
        && VObject == VAR_TYPE(var) )

      {
         assert( VObject == VAR_TYPE(destVar->reference.parentObject) );
         destVar->reference.parentObject->varmem->data.vobject.members.flags
         |= (var->varmem->data.vobject.members.flags & (uword16)HAS_ALL_PROP);
      }
#  endif
   VAR_REMOVE_USER(var,jsecontext);
   VAR_REMOVE_USER(src,jsecontext);
   return ret;
}

static jseVariable NEAR_CALL doMember(
#  if (0!=JSE_API_ASSERTNAMES)
      const jsechar * ThisFuncName,
#  endif
   jseContext jsecontext,jseVariable object_var,VarName vName,
   jseDataType DType,uword16 flags)
{ 
   VarRead *var;
   Var *ret;

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);

   assert( isValidVarType(DType) );

   if ( object_var == NULL )
   {
      object_var = jsecontext->session.GlobalVariable;
   }
   else
   {
      JSE_API_ASSERT_C(object_var,2,jseVariable_cookie,ThisFuncName,return NULL);
   }
   assert( NULL != object_var );
   var = GET_READABLE_VAR(object_var,jsecontext);
#  if ( 0 < JSE_API_ASSERTLEVEL )
      if ( VObject != VAR_TYPE(var) )
      {
         /* force error handling and message reporting */
         ret = callGetVarNeed(jsecontext,var,0,JSE_VN_OBJECT);
         assert( NULL == ret );
         goto DoMemberFini;
      }
#  endif
   assert( VObject == VAR_TYPE(var) );

   if ( NULL == varMemberGet(jsecontext,var,vName,True,
                             0 == ( flags & jseDontSearchPrototype )) )
   {
      if( flags & jseDontCreateMember )
      {
         ret = NULL;
         goto DoMemberFini;
      }
      else
      {
         varCreateMember(var,jsecontext,vName,(VarType)DType);
      }
   }
   
   /* The reason this returns a reference is because we don't know if the
    * variable will be used for reading or for writing (or both). The reference
    * allows prototype stuff and dynamic stuff to all work.
    */
   ret = constructReference(jsecontext,var,vName);
   /* in case the variable must later be created, use this hint for where
    * it will be created
    */
   ret->offset_or_createType = DType;
   if ( ( jseLockRead | jseLockWrite ) & flags )
   {
      /* var is specifically defined for reading or writing, and
       * so create and return a var readable or writable
       */
      Var *_able;
      if ( jseLockRead & flags )
      {
#        if ( 0 < JSE_API_ASSERTLEVEL )
            if ( jseLockWrite & flags )
            {
               SetLastApiError(
       UNISTR("jseLockRead and jseLockWrite flags are mutually exclusive"));
            }
#        endif
            _able = GET_READABLE_VAR(ret,jsecontext);
      }
      else
      {
         _able = getWriteableVar(ret,jsecontext);
      }
      VAR_REMOVE_USER(ret,jsecontext);
      ret = _able;
   }
   /* This routine is used to construct structure members and since
    * it is used to return vars that never get deleted, we must arrange
    * for them to be deleted.
    */
   if ( !(jseCreateVar & flags) )
   {
      CALL_ADD_TEMP_VAR(jsecontext,ret);
   }
DoMemberFini:
   VAR_REMOVE_USER(var,jsecontext);
   return ret;
}
#if (0!=JSE_API_ASSERTNAMES)
#  define DO_MEMBER(FNAME,CNTXT,VAR,NAME,DTYPE,FLAGS) \
            doMember(FNAME,CNTXT,VAR,NAME,DTYPE,FLAGS)
#else
#  define DO_MEMBER(FNAME,CNTXT,VAR,NAME,DTYPE,FLAGS) \
            doMember(CNTXT,VAR,NAME,DTYPE,FLAGS)
#endif
   JSECALLSEQ( jseVariable )
jseMemberEx(jseContext jsecontext,jseVariable objectVar,
            const jsechar *Name,
            jseDataType DType,uword16 flags)
{
   VarName vName;
   jseVariable ret;

   JSE_API_STRING(ThisFuncName,UNISTR("jseMemberEx"));
   JSE_API_ASSERT_(Name,3,ThisFuncName,return NULL);
   if( !isValidVarType(DType) )
   {
#     if ( 0 < JSE_API_ASSERTLEVEL )
#           if (0!=JSE_API_ASSERTNAMES)
               SetLastApiError(
      UNISTR("jseMemberEx: Invalid data type"));
#        else
               SetLastApiError(UNISTR("Invalid data type"));
#        endif
#     endif
      DType = VUndefined;
   }

   vName = EnterIntoStringTable(jsecontext,Name,strlen_jsechar(Name));
   ret = DO_MEMBER(ThisFuncName,jsecontext,objectVar,
                   vName,DType,flags);
   RemoveFromStringTable(jsecontext,vName);
   return ret;
}
   JSECALLSEQ( jseVariable )
jseIndexMemberEx(jseContext jsecontext,jseVariable objectVar,
                 JSE_POINTER_SINDEX index,jseDataType dType,uword16 flags)
{
   if( !isValidVarType(dType) )
   {
#     if ( 0 < JSE_API_ASSERTLEVEL )
#           if (0!=JSE_API_ASSERTNAMES)
               SetLastApiError(
      UNISTR("jseIndexMemberEx: Invalid data type"));
#        else
               SetLastApiError(UNISTR("Invalid data type"));
#        endif
#     endif
      dType = VUndefined;
   }
   return DO_MEMBER(UNISTR("jseIndexMemberEx"),jsecontext,objectVar,
                    EnterNumberIntoStringTable(index),
                    dType,flags);
}

   JSECALLSEQ( jseVariable )
jseGetNextMember(jseContext jsecontext,jseVariable objectVariable,
                 jseVariable prevMemberVariable,
                 const jsechar * * name)
{
   VarRead *var;
   Var *ret;
   VarName varname;
   JSE_API_STRING(ThisFuncName,UNISTR("jseGetNextMember"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
   JSE_API_ASSERT_(name,4,ThisFuncName,return NULL);
#  if ( 0 < JSE_API_ASSERTLEVEL )
      if ( NULL != objectVariable )
         JSE_API_ASSERT_C(objectVariable,2,jseVariable_cookie,ThisFuncName,
                        return NULL);
      if ( NULL != prevMemberVariable )
         JSE_API_ASSERT_C(prevMemberVariable,3,jseVariable_cookie,ThisFuncName,
                        return NULL);
#  endif

   if ( objectVariable == NULL )
      objectVariable = jsecontext->session.GlobalVariable;
   assert( NULL != objectVariable );
   var = GET_READABLE_VAR(objectVariable,jsecontext);
   if ( VObject != VAR_TYPE(var)
     || NULL == (ret = varGetNext(var,jsecontext,prevMemberVariable,&varname)) )
   {
      ret = NULL;
      *name = NULL;
   }
   else
   {
      *name = GetStringTableEntry(jsecontext,varname,NULL);
   }
   VAR_REMOVE_USER(var,jsecontext);
   return ret;
}

   JSECALLSEQ( void )
jseDeleteMember(jseContext jsecontext,jseVariable objectVar,
                const jsechar *name)
{
   VarRead *var;
   VarName varname;
   
   JSE_API_STRING(ThisFuncName,UNISTR("jseDeleteMember"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_(name,3,ThisFuncName,return);
#  if ( 0 < JSE_API_ASSERTLEVEL )
      if ( NULL != objectVar )
         JSE_API_ASSERT_C(objectVar,2,jseVariable_cookie,ThisFuncName,return);
#  endif

   if ( objectVar == NULL )
      objectVar = jsecontext->session.GlobalVariable;
   assert( NULL != objectVar );
   var = GET_READABLE_VAR(objectVar,jsecontext);
   if ( VObject == VAR_TYPE(var) )
   {
      varname = EnterIntoStringTable(jsecontext,name,strlen_jsechar(name));
      
      varDeleteMember(var,jsecontext,varname);
      RemoveFromStringTable(jsecontext,varname);
   } /* endif */
   VAR_REMOVE_USER(var,jsecontext);
}

   JSECALLSEQ( jseVariable )
jseActivationObject(jseContext jsecontext)
{
   VarRead *ActiveObject;

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseActivationObject"),
                  return NULL);

   /* find activation object for most-recent non-global activeobject */
   do {
      if ( NULL != (ActiveObject = jsecontext->VariableObject)
        && !varCompareEquality(jsecontext,ActiveObject,
                               jsecontext->session.GlobalVariable) )
      {
         return ActiveObject;
      }
   } while ( NULL != (jsecontext = callPrevious(jsecontext)) );
   return (jseVariable)NULL;
}

   static VarRead * NEAR_CALL
jselibFindFunction(jseContext jsecontext,jseVariable FunctionVar,struct Function **FindFunc,
                   const jsechar *FunctionName /*null if don't use sub-function*/)
     /* return NULL if not a function object */
{
   VarRead *var;
   VarRead *ret;

   assert( NULL != jsecontext );
   if ( FunctionVar == NULL )
      FunctionVar = jsecontext->session.GlobalVariable;
   assert( NULL != FunctionVar );
   var = GET_READABLE_VAR(FunctionVar,jsecontext);
   ret = NULL;
   if ( VObject == VAR_TYPE(var) ) {
      if( FunctionName!=NULL )
      {
         VarRead *tmp;
         VarName funcname = EnterIntoStringTable(jsecontext,FunctionName,strlen_jsechar(FunctionName));
         tmp = varGetMember(jsecontext,var,funcname);
         RemoveFromStringTable(jsecontext,funcname);
         if ( NULL == tmp )
            goto EndFunctionSearch;
         VAR_REMOVE_USER(var,jsecontext);
         varAddUser(var=tmp);
      }
      if( VAR_TYPE(var)==VObject )
      {
         if ( NULL != (*FindFunc = varGetFunction(var,jsecontext)) )
         {
            ret = CONSTRUCT_VALUE_LOCK(jsecontext,var);
         }
      } /* endif */
   } /* endif */
EndFunctionSearch:
   VAR_REMOVE_USER(var,jsecontext);
   return ret;
}

   JSECALLSEQ( jseVariable )
jseGetFunction(jseContext jsecontext,jseVariable object,
               const jsechar *functionName,jsebool errorIfNotFound)
{
   struct Function *func;
   VarRead *FunctionVar;
   JSE_API_STRING(ThisFuncName,UNISTR("jseGetFunction"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
#  if ( 0 < JSE_API_ASSERTLEVEL )
      if ( NULL != object )
         JSE_API_ASSERT_C(object,2,jseVariable_cookie,ThisFuncName,return NULL);
#  endif
   JSE_API_ASSERT_(functionName,3,ThisFuncName,return NULL);

   FunctionVar = jselibFindFunction(jsecontext,object,&func,functionName);
   if ( NULL == FunctionVar )
   {
      if ( errorIfNotFound )
      {
         callQuit(jsecontext,textcoreFUNCTION_NAME_NOT_FOUND,functionName);
      }
   }
   else
   {
      CALL_ADD_TEMP_VAR(jsecontext,FunctionVar);
   }

   return FunctionVar;
}

   static jsebool NEAR_CALL
IsItAFunction(jseContext jsecontext,jseVariable functionVariable,
              jsebool TestIfLibraryFunction)
{
   struct Function *func;
   VarRead *v;
   jsebool ret;

   assert( NULL != jsecontext );
   v =  jselibFindFunction(jsecontext,functionVariable,&func,NULL);
   if ( NULL == v )
   {
      ret = False;
   }
   else
   {
      ret = ( !TestIfLibraryFunction || !functionTestIfLocal(func) );
      VAR_REMOVE_USER(v,jsecontext);
   }
   return ret;
}

   JSECALLSEQ( jsebool )
jseIsFunction(jseContext jsecontext,jseVariable functionVariable)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jseIsFunction"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(functionVariable,2,jseVariable_cookie,ThisFuncName,
                  return False);

   return IsItAFunction(jsecontext,functionVariable,False);
}

   JSECALLSEQ(jsebool)
jseIsLibraryFunction(jseContext jsecontext,jseVariable functionVariable)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jseIsLibraryFunction"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(functionVariable,2,jseVariable_cookie,ThisFuncName,
                  return False);

   return IsItAFunction(jsecontext,functionVariable,True);
}

   JSECALLSEQ( jsebool )
jseCallFunction(jseContext jsecontext,jseVariable jsefunc,jseStack jsestack,
                jseVariable *retvar,jseVariable thisVar)
{
   VarRead *var;
   VarRead *v;
   struct Function *func;
   VarRead *var2;
   uint depth;
   VarRead *lock;
   Var *ret;
   struct secodeStack *thestack;
   
   JSE_API_STRING(ThisFuncName,UNISTR("jseCallFunction"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(jsefunc,2,jseVariable_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(jsestack,3,jseStack_cookie,ThisFuncName,return False);
#  if ( 0 < JSE_API_ASSERTLEVEL )
      if ( NULL != thisVar )
         JSE_API_ASSERT_C(thisVar,5,jseVariable_cookie,ThisFuncName,
                        return False);
#  endif
 
   callSetReasonToQuit(jsecontext,FlowNoReasonToQuit);
   
   var = GET_READABLE_VAR(jsefunc,jsecontext);
   v =  jselibFindFunction(jsecontext,var,&func,NULL);
#  if ( 0 < JSE_API_ASSERTLEVEL )
      if ( NULL == v )
      {
         VAR_REMOVE_USER(var,jsecontext);
         SetLastApiError( UNISTR("%s: parameter 2 not a function"),ThisFuncName );
         return False;
      }
#  endif
   assert( NULL != v );
   assert( NULL != func );

   /* we only want the function, so free up the value */
   VAR_REMOVE_USER(v,jsecontext);

   var2 = NULL;
   /* ECMAScript rule */
   if( thisVar!=NULL )
   {
      var2 = GET_READABLE_VAR(thisVar,jsecontext);
      if( VAR_TYPE(var2)!=VObject )
      {
         VAR_REMOVE_USER(var2,jsecontext);
         var2 = NULL;
      }
   }

   /* transfer the parameters to the real stack */
   depth = jsecallstackDepth(jsestack);
   thestack = jsecontext->Global->thestack;
   while ( depth-- )
   {
      /* top of the stack is 0 */
      Var *param = jsecallstackPeek(jsestack,depth);
      varAddUser(param);
      varSetLvalue(param,True);
      SECODE_STACK_PUSH(thestack,param);
   }

   lock = CONSTRUCT_VALUE_LOCK(jsecontext,var);

   /* functions always return a value in ECMAScript */
   assert( ((struct Call *)jsecontext)->pChildCall==NULL );
   
   functionFullCall(func,jsecontext,lock,jsecallstackDepth(jsestack),var2);

   ret = SECODE_STACK_POP(thestack);

   /* we've just finished this function, the top value of the stack is
    * our return.
    */
   jsecallstackPush(jsestack,ret,True);
   if( retvar ) *retvar = ret;

   VAR_REMOVE_USER(lock,jsecontext);
   VAR_REMOVE_USER(var,jsecontext);
   if( var2 )
   {
      VAR_REMOVE_USER(var2,jsecontext);
   }
   {
      jsebool ret = ( FlowNoReasonToQuit==callReasonToQuit(jsecontext) );
      callSetReasonToQuit(jsecontext,FlowNoReasonToQuit);
      return ret;
   }
}

#if defined(JSE_CREATEFUNCTIONTEXTVARIABLE) && \
    (0!=JSE_CREATEFUNCTIONTEXTVARIABLE)
   JSECALLSEQ( jseVariable )
jseCreateFunctionTextVariable(jseContext jsecontext,jseVariable FuncVar)
{
   struct Function *func;
   VarRead *v;
   JSE_API_STRING(ThisFuncName,UNISTR("jseCreateFunctionTextVariable"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
   JSE_API_ASSERT_C(FuncVar,2,jseVariable_cookie,ThisFuncName,return NULL);

   v =  jselibFindFunction(jsecontext,FuncVar,&func,NULL);
#  if ( 0 < JSE_API_ASSERTLEVEL )
      if ( NULL == v )
      {
         SetLastApiError( UNISTR("%s: parameter 2 not a function"),ThisFuncName );
         return NULL;
      }
#  endif

   assert( NULL != v );
   assert( NULL != func );
    /* we only want the function so free up the variable */
   VAR_REMOVE_USER(v,jsecontext);

   return functionTextAsVariable(func,jsecontext);
}
#endif

JSECALLSEQ(jseVariable) jseCreateWrapperFunction(jseContext jsecontext,
      const jsechar *functionName,
      void (JSE_CFUNC FAR_CALL *funcPtr)(jseContext jsecontext),
                sword8 minVariableCount, sword8 maxVariableCount,
      jseVarAttributes varAttributes, jseFuncAttributes funcAttributes, void _FAR_ *fData)
{
   struct LibraryFunction *libfunc;
   VarRead *var;
   JSE_API_STRING(ThisFuncName,UNISTR("jseCreateWrapperFunction"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
   JSE_API_ASSERT_(functionName,2,ThisFuncName,return NULL);
   JSE_API_ASSERT_(funcPtr,3,ThisFuncName,return NULL);

   libfunc = libfuncNewWrapper(jsecontext,functionName,funcPtr,
                               minVariableCount,maxVariableCount,
                               varAttributes,funcAttributes,fData);
   var = constructVarRead(jsecontext,VObject);
   varSetFunctionPtr(var,jsecontext,(struct Function *)libfunc);
   return var;
}

   JSECALLSEQ( void )
jseTerminateExternalLink(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseTerminateExternalLink"),
                  return);
   callDelete(jsecontext);
}

#if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
   JSECALLSEQ(jsebool)
jseTellSecurity(jseContext jsecontext,jseVariable infoVar)
{
   VarRead *avar;
   JSE_API_STRING(ThisFuncName,UNISTR("jseTellSecurity"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_C(infoVar,2,jseVariable_cookie,ThisFuncName,return False);

   if(NULL == jsecontext->session.SecurityGuard)
   {
      return False;
   }
   if ( !securityEnabled(jsecontext->session.SecurityGuard) )
   {
      return False;
   }
   assert( NULL != jsecontext->session.SecurityGuard );
   avar = GET_READABLE_VAR(infoVar,jsecontext);
   securityTell(jsecontext->session.SecurityGuard,jsecontext,avar);
   VAR_REMOVE_USER(avar,jsecontext);
   return True;
}
#endif

#if defined(JSE_BREAKPOINT_TEST) && (0!=JSE_BREAKPOINT_TEST)
   JSECALLSEQ(jsebool)
jseBreakpointTest(jseContext jsecontext,const jsechar *FileName,
                  uint LineNumber)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jseBreakpointTest"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return False);
   JSE_API_ASSERT_(FileName,2,ThisFuncName,return False);

   return BreakpointTest(jsecontext,FileName,LineNumber);
}
#endif

