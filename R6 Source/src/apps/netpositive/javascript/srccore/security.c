/* security.c - Secure-jse
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

#if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
#if defined(__DJGPP__)
   #define _MAX_PATH   255
#endif
#if defined(__JSE_390__)
   #define _MAX_PATH   80
#endif

   jsebool NEAR_CALL
securityGuard(struct Security *this,struct Call *call,uint InputVariableCount,
              VarRead *ThisVar)
{
   Var *ans;
   VarRead *answer;
   jsebool GuardSaysYes;
   struct secodeStack *thestack;
   uint parmidx;

   assert( securityEnabled(this) );
   /* safe to call security functions from within security */
   if ( this->InSecurityCall )
   {
      return True;
   } /* endif */

#  if (0!=JSE_COMPILER)
      /* if within conditional compilation then don't allow insecure
         function */
      if ( call->Global->CompileStatus.NowCompiling )
      {
         callQuit(call,textcoreNO_SECURITY_WHILE_COMPILING);
         return False;
      }
#  endif

   assert( NULL == this->NotYetInitializedTextOrCode );

   assert( !callQuitWanted(call) );
   /* if there is no guard function, then no security */
   if ( NULL == this->GuardFunc )
   {
      /* cannot be called from init or term, which is only times of no
         function name */
      callQuit(call,textcoreNO_SECURITY_GUARD_FUNC,
               textcoreSecurityGuardFunctionName);
      return False;
   } /* endif */

   thestack = call->Global->thestack;

   /* push the security variable on the stack, then the function being called,
    * and finally all the variables being passed to the real function
    */
   varAddUser(this->PrivateVariable);
   SECODE_STACK_PUSH(thestack,this->PrivateVariable);
   for ( parmidx = InputVariableCount + 1; parmidx--; )
   {
      Var * v = thestack->items[thestack->used-InputVariableCount-2];
      varAddUser(v);
      SECODE_STACK_PUSH(thestack,v);
   }

   /* temporarily link in our functions and do the call */
   assert( !this->InSecurityCall );
   this->InSecurityCall = True;
   functionFullCall(this->GuardFunc,call,this->GuardFuncVar,
                    2 + InputVariableCount,ThisVar);
   assert( this->InSecurityCall );
   this->InSecurityCall = False;

   /* determine if the security guard said OK */
   ans = SECODE_STACK_POP(thestack);
   answer = GET_READABLE_VAR(ans,call);
   VAR_REMOVE_USER(ans,call);
   GuardSaysYes = varGetBoolean(answer);
   VAR_REMOVE_USER(answer,call);

   if ( !GuardSaysYes )
   {
      callQuit(call,textcoreNO_APPROVAL_FROM_SECURITY_GUARD);
      assert( callQuitWanted(call) );
   } /* endif */

   return ( !callQuitWanted(call) );
}


   struct Security * NEAR_CALL
securityNew(const jsechar * jseSecureCode)
{
   struct Security *this =
      jseMustMalloc(struct Security,sizeof(struct Security));

   memset( this, 0, sizeof(*this) );
   assert( NULL == this->InitFunc );
   assert( NULL == this->GuardFunc );
   assert( NULL == this->TermFunc );
   assert( NULL == this->PrivateVariable );
   assert( NULL == this->NotYetInitializedTextOrCode );
   assert( !this->InSecurityCall );
   assert( !this->SecurityHasBeenEnabled );

   if ( NULL != jseSecureCode )
   {
      this->NotYetInitializedTextOrCode = StrCpyMalloc(jseSecureCode);
      this->SecurityHasBeenEnabled = True;
   }

   return this;
}


   void NEAR_CALL
securityDelete(struct Security *this)
{
   FreeIfNotNull(this->NotYetInitializedTextOrCode);
   jseMustFree(this);
}


jsebool NEAR_CALL securityInit(struct Security *this,struct Call *call)
{
   jsebool success;

   assert( securityEnabled(this) );
   assert( !this->InSecurityCall );

#  if (0!=JSE_COMPILER)
      if ( call->Global->CompileStatus.NowCompiling )
         return True;    /* don't initialize during a compilation */
#  endif

   if ( NULL == this->NotYetInitializedTextOrCode )
      return True;    /* only need initialize once at each level */

   assert( NULL == this->InitFunc );
   assert( NULL == this->GuardFunc );
   assert( NULL == this->TermFunc );
   assert( NULL == this->PrivateVariable );

#  if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
   {
      jsechar *FileName;
      /* if no such file or code then there will be no security if needed
       * if cannot open file, then treat security as text
       */
      FileName = jseMustMalloc(jsechar, _MAX_PATH * sizeof(jsechar));
#     if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) && \
         (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
         success = (jsebool)DispatchToClient(call->Global->ExternalDataSegment,
           (ClientFunction)(call->Global->ExternalLinkParms.FileFindFunc),
                                 (void *)&call,
                                 (void *)this->NotYetInitializedTextOrCode,
                                 (void *)FileName,
                                 (void *)(_MAX_PATH-1),
                                 (void *)False);
#     else
         success = ( (*(call->Global->ExternalLinkParms.FileFindFunc))
                      (call,this->NotYetInitializedTextOrCode, FileName,
                       _MAX_PATH-1, False) );
#     endif
#     if !defined(NDEBUG) && (0<JSE_API_ASSERTLEVEL) && defined(_DBGPRNTF_H)
         if ( !jseApiOK )
         {
            DebugPrintf(UNISTR("Error calling security file-find function"));
            DebugPrintf(UNISTR("Error message: %s"),jseGetLastApiError());
         }
#     endif
      assert( jseApiOK );
      jseMustFree(FileName);
   }
#  else
      success = False;
#  endif

#  if (0!=JSE_COMPILER)
      {
         /* We want the security functions to go in their own 'section'.
          * this we create a global object solely for them
          */
         jseVariable oldglob = call->session.GlobalVariable;
         this->PrivateVariable = constructVarRead(call,VObject);
         call->session.GlobalVariable = this->PrivateVariable;
         
         success = CompileFromText(call,&(this->NotYetInitializedTextOrCode),
                                   success);
         /* first call will set the guard function to initialize */
         this->InitFunc = FindFunction(call,textcoreSecurityInitFunctionName,
                                       &(this->InitFuncVar));
         this->GuardFunc = FindFunction(call,textcoreSecurityGuardFunctionName,
                                        &(this->GuardFuncVar));
         this->TermFunc = FindFunction(call,textcoreSecurityTermFunctionName,
                                       &(this->TermFuncVar));

         call->session.GlobalVariable = oldglob;
         if( !success ) VAR_REMOVE_USER(this->PrivateVariable,call);
      }
#  else
      #error SECURITY NOT SETUP YET TO HANDLE !JSE_COMPILER
#  endif

   jseMustFree(this->NotYetInitializedTextOrCode);
   this->NotYetInitializedTextOrCode = NULL;
      /* indicate that it has been initialized */

   if ( success )
   {
      if ( NULL != this->InitFunc )
      {
         assert( success );
         success = securityCallInitOrTerm(this,call,this->InitFunc,
                                          this->InitFuncVar,NULL);
      }
   }
   else
   {
      assert( callQuitWanted(call) );
      assert( !success );
   }

   return success;
}


   struct Function * NEAR_CALL
FindFunction(struct Call *call,const jsechar *FunctionName,VarRead **FunctionVar)
{
   VarName funcname = EnterIntoStringTable(call,FunctionName,strlen_jsechar(FunctionName));

   assert( NULL != call->session.GlobalVariable );
   assert( VObject == VAR_TYPE(call->session.GlobalVariable) );
   *FunctionVar =
     varGetMember(call,call->session.GlobalVariable,funcname);
   RemoveFromStringTable(call,funcname);
   if ( NULL != *FunctionVar )
   {
      if ( VObject == VAR_TYPE(*FunctionVar) )
      {
         return varGetFunction(*FunctionVar,call);
      }
   }
   return NULL;
}


   jsebool NEAR_CALL
securityCallInitOrTerm(struct Security *this,struct Call *call,
                       struct Function *InitOrTermFunc,VarRead *FuncVar,
                       VarRead *var2)
{
   struct Function *RememberGuard = this->GuardFunc;
   VarRead *RememberGuardVar = this->GuardFuncVar;
   FlowFlag RememberReasonToQuit = callReasonToQuit(call);
   jsebool success;
   VarRead *v2;

   if ( NULL == var2 )
   {
      var2 = constructVarRead(call,VUndefined);
   }
   else
   {
      varAddUser(var2);
   }
   assert( securityEnabled(this) );
   assert( NULL == this->NotYetInitializedTextOrCode );
   this->GuardFunc = InitOrTermFunc;
   this->GuardFuncVar = FuncVar;
   callSetReasonToQuit(call,FlowNoReasonToQuit);
   SECODE_STACK_PUSH(call->Global->thestack,var2);
   success = securityGuard(this,call,0,GetCurrentThisVar(call));
   v2 = SECODE_STACK_POP(call->Global->thestack);
   assert( v2 == var2 );
   callSetReasonToQuit(call,RememberReasonToQuit);
   this->GuardFunc = RememberGuard;
   this->GuardFuncVar = RememberGuardVar;
   VAR_REMOVE_USER(v2,call);
   return success;
}


void NEAR_CALL securityTerm(struct Security *this,struct Call *call)
{
   assert( securityEnabled(this) );
   if ( NULL != this->NotYetInitializedTextOrCode )
      return;    /* never really initialized, so don't need to clean up much */

   /* temporarily turn off error to allow security call to execute */
   callSetReasonToQuit(call,FlowNoReasonToQuit);
   /* if there is a terminate function then give it a change to be called */
   if ( NULL != this->TermFunc )
   {
      callSetReasonToQuit(call,FlowNoReasonToQuit);
      securityCallInitOrTerm(this,call,this->TermFunc,this->TermFuncVar,NULL);
   }
   VAR_REMOVE_USER(this->PrivateVariable,call);
}


   void NEAR_CALL
securityTell(struct Security *this,struct Call *call,VarRead *InfoVar)
{
   if ( NULL != this->InitFunc )
   {
      securityCallInitOrTerm(this,call,this->InitFunc,this->InitFuncVar,
                             InfoVar);
   }
}

#endif
