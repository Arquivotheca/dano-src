/* interprt.cpp  Entry point and main loop to Interpret() function
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


   static jsebool NEAR_CALL
RunCompiledCode(struct Call *call,int argc,
                jsechar *argv[],jsebool CallMain,
                VarRead *OldMainVar,
                struct Call *PreviousCall)
{
   VarRead *ArgcVar;
   VarRead *GlobalVar;
   VarRead *FuncVar;
   struct secodeStack *thestack = call->Global->thestack;
   jsebool Success = False;
   int i;
   VarName Neg1;
   VarRead *PrevArgvVar, *PrevArgvNeg1;

   GlobalVar = call->session.GlobalVariable;
   assert( NULL != GlobalVar  &&  VObject == VAR_TYPE(GlobalVar));

   /* create the global _argv and _argc variables
    * (same as passed to main later) */
   if( CallMain )
   {
      VarRead *ArgvVar;

      /* create the global _argc and _argv variables (note that if bind running
       * then it's possible the variables already exist)
       */
      if( NULL != (ArgcVar = varCreateMember(GlobalVar,call,
                                GLOBAL_STRING(call,argc_entry),
                                VNumber)))
      {
         varPutLong(ArgcVar,argc);

         if( NULL != (ArgvVar = varCreateMember(GlobalVar,call,
                                   GLOBAL_STRING(call,argv_entry),
                                   VObject)))
         {
            assert( VObject == VAR_TYPE(ArgvVar));

            for( i = 0; i < argc; i++ )
            {
               VarName name = EnterNumberIntoStringTable(i);

               VarRead *argvstr = varCreateMember(ArgvVar,call,
                                                  name,
                                                  VString);
               if( NULL == argvstr )
                  break;

               varPutString(argvstr,call,argv[i]);
            }
            if( i == argc )
            {
               Success = True;
               /* SPECIAL CASE FOR ARGV[-1] - As Nombas' products introduce argv[-1]
                * to represent the name of the executable, it is possible that other
                * SE products may want to duplicate this behavior.  And so if this
                * instance does not have an argv[-1] but the previous instance does,
                * propogate this argv[-1].
                */
               Neg1 = EnterNumberIntoStringTable(-1);
               if ( NULL == varGetDirectMember(call,ArgvVar,Neg1,False))
               {
                  /* this has no argv[-1] see if previous does */
                  PrevArgvVar = varGetDirectMember(call,PreviousCall->session.GlobalVariable,
                     GLOBAL_STRING(call,argv_entry),False);
                  if ( NULL != PrevArgvVar )
                  {
                     assert( Neg1 == EnterNumberIntoStringTable(-1));
                     PrevArgvNeg1 = varGetDirectMember(call,PrevArgvVar,Neg1,False);
                     if ( NULL != PrevArgvNeg1 )
                     {
                        VarRead *member = varCreateMember(ArgvVar,call,Neg1,VString);
                        if( NULL == member )
                           Success = False;
                        else
                           varCopyAssign(member,call,
                                         PrevArgvNeg1);
                     }
                  }
               }

               if( Success )
               {
                  FuncVar = varGetMember(call,GlobalVar,GLOBAL_STRING(call,main_entry));
                  if( NULL != FuncVar  &&  FuncVar != OldMainVar &&
                      VAR_IS_FUNCTION(FuncVar,call))
                  {
                     varAddUser(FuncVar);
                     varAddUser(FuncVar);
                     SECODE_STACK_PUSH(thestack,FuncVar);
                     SECODE_STACK_PUSH(thestack,FuncVar);
                     /* pass _argc and _argv to main */
                     varAddUser(ArgcVar);
                     SECODE_STACK_PUSH(thestack,ArgcVar);
                     varAddUser(ArgvVar);
                     SECODE_STACK_PUSH(thestack,ArgvVar);
                     callFunction(call,2,False);

                     call = call->pChildCall;
                  }
               }
            }
         }
      }
   }
   else
      Success = True;

   /* execute the InitFunction first by pushing it last */
   FuncVar = varGetMember(call,GlobalVar,GLOBAL_STRING(call,init_function_entry));
   assert( NULL != FuncVar  && VAR_IS_FUNCTION(FuncVar,call));
   varAddUser(FuncVar);
   varAddUser(FuncVar);
   SECODE_STACK_PUSH(thestack,FuncVar);
   SECODE_STACK_PUSH(thestack,FuncVar);
   callFunction(call,0,False);

   return Success;
}

   struct Call * NEAR_CALL
interpretInit(struct Call * call,const jsechar *OriginalSourceFile,
              const jsechar * OriginalSourceText,
              const void *PreTokenizedSource,
              jseNewContextSettings NewContextSettings,
              int HowToInterpret,struct Call *LocalVariableCall)
{
   struct Call *InterpretCall;
   int argc;
   jsechar **argv,*SourceText;
   VarRead *main_place,*init_place;
   jsebool CallMain = (HowToInterpret & JSE_INTERPRET_CALL_MAIN);

   /* ----------------------------------------------------------------------
    * First, set up for the interpret by doing some checking on the input
    * parameters.
    * ---------------------------------------------------------------------- */

   if( HowToInterpret & JSE_INTERPRET_LOAD )
   {
      /* if we are loading, we are saying that we want to put the stuff
       * in the original context.
       */
      NewContextSettings &= ~jseNewFunctions;
   }

   /* caller may not know exactly how far up to find local variables,
    * so find most recent local function
    */
   while( NULL != LocalVariableCall &&
          NULL != callFunc(LocalVariableCall) &&
          !functionTestIfLocal(callFunc(LocalVariableCall)))
   {
      LocalVariableCall = callPrevious(LocalVariableCall);
   }

   call->saveexit = call->pExitVar;
   call->pExitVar = NULL;

   /* determine all the argv and argc parameter for source
    * (always at least 1) */
   argc = 1;
   argv = jseMustMalloc(jsechar *,sizeof(jsechar *));
   SourceText = StrCpyMalloc(( NULL != OriginalSourceText ) ?
                             OriginalSourceText : UNISTR("") );

   if ( NULL == OriginalSourceFile )
   {
      /* no source file, so this is pure text to interrpet */
      argv[0] = StrCpyMalloc(UNISTR(""));
   }
   else
   {
      /* source file is supplied; argv[0] is that file;
       * pull other parameters out of SourceText */
      argv[0] = StrCpyMalloc(OriginalSourceFile);
      ParseSourceTextIntoArgv(SourceText,&argc,&argv);
   }

   /* ---------------------------------------------------------------------- */
   /* then get the new call */
   /* ---------------------------------------------------------------------- */

   InterpretCall = callNew(call,NULL,0,NewContextSettings);

   InterpretCall->LocalVariableCall = LocalVariableCall;

   varAddUser(InterpretCall->VariableObject =
              InterpretCall->session.GlobalVariable);

   if((HowToInterpret & JSE_INTERPRET_NO_INHERIT)==0 )
   {
      if( LocalVariableCall )
      {
         CloneScopeChain(InterpretCall,LocalVariableCall);
      }
   }
   else
   {
      IgnorePastNewFunctionObjects(InterpretCall);
   }

   assert( InterpretCall->oldMain==NULL && InterpretCall->oldInit==NULL );

   main_place = varMakeMember(InterpretCall->session.GlobalVariable,
                              InterpretCall,
                              GLOBAL_STRING(call,main_entry),
                              VUndefined);

   InterpretCall->oldMain = CONSTRUCT_VALUE_LOCK(InterpretCall,main_place);
   varConvert(main_place,InterpretCall,VUndefined);
   varSetAttributes(main_place,jseDontEnum);

   init_place = varMakeMember(InterpretCall->session.GlobalVariable,
                              InterpretCall,
                              GLOBAL_STRING(call,init_function_entry),
                              VUndefined);
   InterpretCall->oldInit = CONSTRUCT_VALUE_LOCK(InterpretCall,init_place);
   varConvert(init_place,InterpretCall,VUndefined);
   varSetAttributes(init_place,jseDontEnum);

   /* ---------------------------------------------------------------------- */
   /* Compile the stuff into it. */
   /* ---------------------------------------------------------------------- */

#if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)
   if ( NULL != PreTokenizedSource )
   {
      CompileFromTokens(InterpretCall,PreTokenizedSource);
   }
#endif

#if (0!=JSE_COMPILER)
   if ((NULL == PreTokenizedSource &&
       !CompileFromText(InterpretCall,OriginalSourceFile ?
                        &(argv[0]) : &SourceText,
                        NULL != OriginalSourceFile))
       || callQuitWanted(InterpretCall))
   {
      /* there was an error - restore main/init, return */
      varAssign(main_place,InterpretCall,InterpretCall->oldMain);
      varAssign(init_place,InterpretCall,InterpretCall->oldInit);
      varSetAttributes(init_place,jseDontEnum);
      VAR_REMOVE_USER(InterpretCall->oldMain,call);
      VAR_REMOVE_USER(InterpretCall->oldInit,call);

      callDelete(InterpretCall);
      jseMustFree(SourceText);
      FreeArgv(argc,argv);

      return NULL;
   }
#endif

   /* enable security */

#if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
   if( securityEnabled(InterpretCall->session.SecurityGuard))
   {
      if( !securityInit(InterpretCall->session.SecurityGuard,call))
      {
         return False;
      }
   }
#endif


   /* ---------------------------------------------------------------------- */
   /* Set up 'main' and 'global initialization' calls */
   /* ---------------------------------------------------------------------- */

   RunCompiledCode(InterpretCall,argc,argv,CallMain,
                   NULL,call);
   while( InterpretCall->pChildCall!=NULL )
      InterpretCall = InterpretCall->pChildCall;

   /* ---------------------------------------------------------------------- */
   /* some cleanup                                                           */
   /* ---------------------------------------------------------------------- */

   jseMustFree(SourceText);
   FreeArgv(argc,argv);

   /* ---------------------------------------------------------------------- */
   /* all set, return the result */
   /* ---------------------------------------------------------------------- */

   return InterpretCall;
}


   VarRead * NEAR_CALL
interpretTerm(struct Call *call)
{
   Var *stackRet, *interpRet;
   VarRead *main_place,*init_place;
   struct Call *InterpretCall,*tmpcall;

   InterpretCall = call->pChildCall;
   assert( InterpretCall!=NULL && InterpretCall->oldMain!=NULL &&
           InterpretCall->oldInit!=NULL );

   while( InterpretCall->pChildCall )
   {
      tmpcall = InterpretCall->pChildCall;
      while( tmpcall->pChildCall!=NULL ) tmpcall = tmpcall->pChildCall;
      /* abort the call */
      callSetReasonToQuit(tmpcall,FlowError);
      callDelete(tmpcall);
   }

   assert( InterpretCall->pChildCall==NULL );

   if((InterpretCall->CallSettings & JSE_INTERPRET_NO_INHERIT)==0 &&
      InterpretCall->LocalVariableCall )
      RemoveClonedScopeChain(InterpretCall,InterpretCall->LocalVariableCall);

   stackRet = SECODE_STACK_POP(InterpretCall->Global->thestack);
   assert( InterpretCall->pReturnVar==NULL );

   if( FlowExit == callReasonToQuit(InterpretCall))
   {
      interpRet = callGetExitVar(InterpretCall);
   }
   else
   {
      interpRet = GET_READABLE_VAR(stackRet,InterpretCall);
   }
   VAR_THOROUGH_REMOVE_USER(stackRet,InterpretCall);

   /* restore original main and init function */

   main_place = varMakeMember(InterpretCall->session.GlobalVariable,
                              InterpretCall,
                              GLOBAL_STRING(call,main_entry),
                              VUndefined);
   init_place = varMakeMember(InterpretCall->session.GlobalVariable,
                              InterpretCall,
                              GLOBAL_STRING(call,init_function_entry),
                              VUndefined);
   varAssign(main_place,InterpretCall,InterpretCall->oldMain);
   varAssign(init_place,InterpretCall,InterpretCall->oldInit);
   varSetAttributes(init_place,jseDontEnum);
   VAR_REMOVE_USER(InterpretCall->oldMain,InterpretCall);
   VAR_REMOVE_USER(InterpretCall->oldInit,InterpretCall);

   callDelete(InterpretCall);

   if ( NULL != call->pExitVar )
   {
      VAR_THOROUGH_REMOVE_USER(call->pExitVar,call);
   }
   call->pExitVar = call->saveexit;

   return interpRet;
}
