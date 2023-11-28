/* function.c   Parameters and code cards within a function.
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

   void NEAR_CALL
functionInit(struct Function *this,
             struct Call *call,
             VarRead *ObjectToAddTo,
                  /*NULL if not to add to any object*/
               jseVarAttributes FunctionVariableAttributes,
                  /*not used if ObejctToAddTo is NULL*/
               jsebool iLocalFunction, /*else library*/
               jsebool iCBehavior,
                  /*else default javascript behavior*/
               uint Params)
{
   this->stored_in = ObjectToAddTo;
   this->params = Params;
   this->flags = 0;
   if ( iLocalFunction )
      this->flags |= Func_LocalFunction;
   if ( iCBehavior )
      this->flags |= Func_CBehavior;
   if ( NULL != ObjectToAddTo ) {
      struct Function *OldFunction = varGetFunction(ObjectToAddTo,call);

      assert( VObject == VAR_TYPE(ObjectToAddTo) );
      if ( NULL != OldFunction )
         functionDelete(OldFunction,call);

      /* why??? seems unnecesary - varSetAttributes(ObjectToAddTo,0); */
      varSetFunctionPtr(ObjectToAddTo,call,this);

      varSetAttributes(ObjectToAddTo,FunctionVariableAttributes);
   }
#  if 0 != JSE_MULTIPLE_GLOBAL
      this->global_object = call->session.GlobalVariable;
#  endif
}


   void NEAR_CALL
functionDelete(struct Function *this,struct Call *call)
{
   if ( functionTestIfLocal(this) )
      localDelete((struct LocalFunction *)this,call);
   else
      libfuncDelete((struct LibraryFunction *)this);
#  if 0 != JSE_MULTIPLE_GLOBAL
      assert( this->global_object == call->session.GlobalVariable );
#  endif
   jseMustFree(this);
}

   static uint
FindParents(VarRead *thisVar,struct Call *call,VarRead **list,
            int offset)
{
   uint ret = 0;
   
   assert( VAR_TYPE(thisVar) == VObject );
   
   /* Because this is the first time being called we want to mark the 
    * current this variable as having being visited, since it will already
    * be added to the scope chain later
    */
   if( offset == 0 )
      thisVar->varmem->data.vobject.been_here = True;
   
   if( varHasProperty(thisVar,call, GLOBAL_STRING(call,parent_entry)) )
   {
      VarRead * parent= varGetMember(call,thisVar,
                                     GLOBAL_STRING(call,parent_entry));
      
      assert( parent != NULL );
                                      
      if( VAR_TYPE(parent) == VObject &&
          !parent->varmem->data.vobject.been_here )
      {
         parent->varmem->data.vobject.been_here = True;

         if( list != NULL )
            list[offset] = parent;
            
         ret = 1 + FindParents(parent,call,list,offset+1);
            
         parent->varmem->data.vobject.been_here = False;
      }
   }
   
   if( offset == 0 )
      thisVar->varmem->data.vobject.been_here = False;
   
   return ret;
}


/* Similar to DoCall, but must get the return before we can continue. It
 * is therefore recursive.
 */
   void NEAR_CALL
functionFullCall(struct Function *this,struct Call *CallerCall,VarRead *FuncVar,
                 uint InputVariableCount,VarRead *ThisVar)
{
   struct Call *newc;
   jsebool local = functionTestIfLocal(this);

   if( functionDoCall(this,CallerCall,FuncVar,InputVariableCount,ThisVar) )
   {
      newc = CallerCall->pChildCall;
      assert( newc->pChildCall==NULL );

      newc->no_clean = True;
      
      /* We've set up the call, let's go until it returns. */
   
      while( (newc = secodeInterpret(newc))!=NULL )
      {
         /* we've finished the function call when we return back to our
          * original context
          */
         if( newc==CallerCall ) break;
         if( !MayIContinue(newc) ) break;
      }
   }

   if( !local )
   {
      assert( NULL != CallerCall->pChildCall );
      assert( NULL == CallerCall->pChildCall->pChildCall );
   
      SECODE_STACK_PUSH(CallerCall->Global->thestack,
                        functionFinishCall(this,CallerCall->pChildCall,FuncVar,InputVariableCount,
                                           ThisVar));
   }
}


/* return whether this created a new context (if False, it finished
 * immediately).
 */
   jsebool NEAR_CALL
functionDoCall(struct Function *this,struct Call *CallerCall,VarRead *FuncVar,
               uint InputVariableCount,VarRead *ThisVar)
{
   struct Call * call;
   VarRead *activation;
   VarRead *o_c;
   uint x;
   jseVarAttributes attributes = varGetAttributes(FuncVar);
   jsebool InitializationFunction;
   jsebool local = functionTestIfLocal(this);
   

   assert( NULL != CallerCall );

#  if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
      if ( !local )
         /* local function are always secure, so don't bother to check those */
      {
         if ( securityEnabled(CallerCall->session.SecurityGuard)
           && !(((struct LibraryFunction *)this)->FuncDesc->FuncAttributes &
                jseFunc_Secure) )
         {
            struct secodeStack *thestack = CallerCall->Global->thestack;
            assert( NULL != CallerCall->session.SecurityGuard );
            assert( securityEnabled(CallerCall->session.SecurityGuard) );
            /* local functions, if found, are always secure */
            assert( !local );
            if ( !securityGuard(CallerCall->session.SecurityGuard,CallerCall,
                                InputVariableCount,ThisVar) )
            {
               VarRead *vr = constructVarRead(CallerCall,VUndefined);
               assert( callQuitWanted(CallerCall) );
               SECODE_STACK_PUSH(thestack,vr);
               return False;
            }
         }
      }
      assert( !callQuitWanted(CallerCall) );
#   endif

   /* initialize a new context for this function */
   call = callNew(CallerCall,this,InputVariableCount,0);

   call->parentCount = 0;
   call->lockFunc = CONSTRUCT_SIBLING(call,FuncVar,0,0);

   InitializationFunction = local &&
      LOCAL_TEST_IF_INIT_FUNCTION((struct LocalFunction *)this,CallerCall);

   if( local )
   {
      if ( InitializationFunction )
      {
         /* For the initialization, we have it execute in the parent context
          * (so if we do an interpret, the initialization stuff goes in the
          * right place.)
          */
         CloneScopeChain(call,CallerCall);
         if ( NULL == (activation = call->VariableObject) )
            activation = call->session.GlobalVariable;
         varAddUser(activation);
      }
      else
      {
         /* if we are executing a JavaScript function, we do all the special
          * Javascript scope chaining and such.
          */
         if( (attributes & jseImplicitParents) &&
             ThisVar != NULL &&
             VAR_TYPE(ThisVar) == VObject )
         {
            VarRead ** parentList;
             
            /* We have decided that we should search the _parent property */
            /* Because we actually want to search them in the opposite order,
             * we must build an array and then push them in reverse 
             */
            
            call->parentCount = FindParents(ThisVar,call,NULL,0);

            if( call->parentCount != 0 )
            {
               uint i;
               parentList = jseMustMalloc(VarRead *,call->parentCount*sizeof(VarRead *));

               FindParents(ThisVar,call,parentList,0);

               for( i = call->parentCount; 0 != i--; )
               {
                  varAddUser(parentList[i]);
                  AddScopeObject(call,parentList[i]);
               }

               jseMustFree(parentList);
            }

         }

         activation = constructVarRead(call,VObject);
         varSetActivation(activation);

         if( attributes & jseImplicitThis )
         {
			// seb 98.12.30 -- Added check for ThisVar == NULL
            if( ThisVar != NULL && !varSameObject(ThisVar,call->session.GlobalVariable) )
            {
               varAddUser(ThisVar);
               AddScopeObject(call,ThisVar);
            }
         }

         varAddUser(activation);
         AddScopeObject(call,activation);
         
      }
   }
   else
   {
      /* For library functions, they don't get an activation object
         * since there is no way for them to use it anyway and it is
         * slow. Just use the global object. Please note that this
         * whole darn function is slow and used a lot, so every little bit
         * helps
         */
      varAddUser(activation = call->session.GlobalVariable);
   }

   assert( activation!=NULL );
   varAddUser( call->VariableObject = activation );

   /* ---------------------------------------------------------------------- */
   /* Set up the 'arguments' object if needed                                */
   /* ---------------------------------------------------------------------- */

   call->arg_obj = NULL; call->oldcaller = NULL;

   if( InitializationFunction )
   {
      /* yes, yes, evil and ugly. But a marker is needed so that
       * we don't pick up the shell functions which shouldn't show up.
       * Each initialization function calls all its functions
       * with caller of null, so this marker makes it so.
       */
      SetCallVariable(call,(VarRead *)0x01);
   }
   else
   {
      /* store current call variable so future calls may reference it */
      assert( FuncVar!=(VarRead *)0x01 );
      SetCallVariable(call,FuncVar);

      if( local )
      {
         if ( (Func_UsesArguments|Func_UsesItsName) & this->flags )
         {
            VarRead *oldargs, *callee, *length;
             
            /* we keep track of whether the function ever uses the 'arguments'
             * or its own function name, so if it doesn't we can skip this
             * stuff which is slow.
             */
            if( Func_UsesItsName & this->flags )
            {
               /* if it is a wrapper function, we don't do this stuff
                * see 10.1.1, 4th dot.
                */
               /* retrieve the previous call variable */
               VarRead *OldFuncVar = NULL;
               struct Call *loop = CallerCall;

               assert( NULL != loop );
               assert( loop == callPrevious(call) );
               do {
                  if ( GetCallVariable(loop) ) {
                     OldFuncVar = GetCallVariable(loop);
                     break;
                  }
               } while ( NULL != (loop = callPrevious(loop)) );

               /* Set up the 'caller' property */

               call->oldcaller = varGetDirectMember(call,FuncVar,
                                                    GLOBAL_STRING(call,caller_entry),False);
               if( call->oldcaller )
               {
                  call->oldcaller = CONSTRUCT_VALUE_LOCK(call,call->oldcaller);
               }

               /* this trashes any previous member. Fortunately we just saved
                * it so it can be restored.
                */
               o_c = varCreateMember(FuncVar,call,GLOBAL_STRING(call,caller_entry),
                                     VNull);

               if( OldFuncVar && OldFuncVar!=(VarRead *)0x01 )
               {
                  varAssign(o_c,call,OldFuncVar);
                  varSetAttributes(o_c,jseDontEnum | jseDontDelete);
               }
            }
         
            /* technically supposed to be an array, but that array is read-only
             * so it is good enough to make it array-like. Making it an array
             * hurts performance
             */
            oldargs = varGetDirectMember(call,FuncVar,GLOBAL_STRING(call,arguments_entry),False);

            call->arg_obj = constructVarRead(call,VObject);
            if( oldargs )
            {
               varSetAttributes(oldargs,0);
               varAssign(varCreateMember(call->arg_obj,call,
                                         GLOBAL_STRING(call,old_arguments_entry),
                                         VUndefined),
                         call,oldargs);
            } else {
               oldargs = varCreateMember(FuncVar,call,GLOBAL_STRING(call,arguments_entry),VUndefined);
            }
            varAssign(oldargs,call,call->arg_obj);
            varSetAttributes(oldargs,jseDontDelete | jseDontEnum);

            varAssign(varCreateMember(activation,call,GLOBAL_STRING(call,arguments_entry),VUndefined),
                      call,call->arg_obj);

            callee = varCreateMember(call->arg_obj,call,GLOBAL_STRING(call,callee_entry),
                                     VUndefined);
            varAssign(callee,call,FuncVar);

            length = varCreateMember(call->arg_obj,call,GLOBAL_STRING(call,length_entry),
                                     VNumber);
            varPutLong(length,InputVariableCount);
            varSetAttributes(length,jseDontEnum);

            for( x=0;x<InputVariableCount;x++ )
            {
               VarName name = EnterNumberIntoStringTable(x);
               struct StructureMember * mem;
               Var *cgVar = callGetVar(call,x);
               VarRead *tmpvar = GET_READABLE_VAR(cgVar,call);

               mem = varobjCreateMember(&(call->arg_obj->varmem->data.vobject.members),
                                        call,name,VNumber);
               varAddUser(tmpvar);
               VAR_REMOVE_USER(mem->var,call);
               mem->var = tmpvar;
               
               VAR_REMOVE_USER(tmpvar,call);
               
               varSetAttributes(mem->var,jseDontEnum);
            }
         }
      }
   }
   /* ---------------------------------------------------------------------- */


   /* finally, run the function */
   if( local )
   {
      localExecute((struct LocalFunction *)this,call,ThisVar);
      return True;
   }
   else
   {
      libfuncExecute((struct LibraryFunction *)this,call,ThisVar);
      return False;
   }
}

   VarRead * NEAR_CALL
functionFinishCall(struct Function *this,struct Call *call,
                   VarRead *FuncVar,
                   uint InputVariableCount,VarRead *ThisVar)
{
   VarRead *o_c;
   struct Call *CallerCall = call->pPreviousCall;
   jsebool InitializationFunction = ( functionTestIfLocal(this) &&
                LOCAL_TEST_IF_INIT_FUNCTION((struct LocalFunction *)this,
                                            CallerCall) );
   jseVarAttributes attributes = varGetAttributes(FuncVar);
   struct secodeStack *thestack = CallerCall->Global->thestack;
   uint x;
   VarRead *result;                     /* the variable to be returned */
   
   
   if( functionTestIfLocal(this) && !InitializationFunction )
   {
      if( (Func_UsesArguments|Func_UsesItsName) & this->flags )
      {
         VarRead *orig_args;
         VarRead *args;
            
         if( Func_UsesItsName & this->flags )
         {

            o_c = varCreateMember(FuncVar,call,GLOBAL_STRING(call,caller_entry),
                                  VUndefined);
            assert( NULL != o_c );
            varSetAttributes(o_c,0);
            if( call->oldcaller!=NULL )
            {
               varAssign(o_c,call,call->oldcaller);
               varSetAttributes(o_c,jseDontEnum | jseDontDelete);
               VAR_REMOVE_USER(call->oldcaller,call);
            } else {
               varDeleteMember(FuncVar,call,GLOBAL_STRING(call,caller_entry));
            }
         }

         assert( call->arg_obj!=NULL );
         orig_args = varGetDirectMember(call,call->arg_obj,GLOBAL_STRING(call,old_arguments_entry),False);
         args = varGetDirectMember(call,FuncVar,GLOBAL_STRING(call,arguments_entry),False);
         assert( NULL != args );

         varSetAttributes(args,0);
         if( orig_args!=NULL )
         {
            varAssign(args,call,orig_args);
            varSetAttributes(args,jseDontDelete | jseDontEnum);
         } 
         else 
         {
            varDeleteMember(FuncVar,call,GLOBAL_STRING(call,arguments_entry));
         }

         assert( call->arg_obj!=NULL );
         VAR_REMOVE_USER(call->arg_obj,call);
      }
   }

   VAR_REMOVE_USER(call->VariableObject,call);

   /* ---------------------------------------------------------------------- */

   if( functionTestIfLocal(this) )
   {
      if ( InitializationFunction )
      {
         RemoveClonedScopeChain(call,CallerCall);
      }
      else
      {
         RemoveScopeObject(call);
      }
   }


   if( ((attributes & jseImplicitThis)!=0) &&
       !varSameObject(ThisVar,call->session.GlobalVariable) )
      RemoveScopeObject(call);

   if( call->parentCount > 0 )
   {
      ulong i;
      for( i = 0; i < call->parentCount; i++ )
         RemoveScopeObject(call);
   }

   /* Preserve reason to quit from executed function. Only save exit reasons */
   assert( !callQuitWanted(CallerCall) );
   callSetReasonToQuit(CallerCall,callReasonToQuit(call));

#if defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS)
   /* dump extra things off stack */
   while( secodestackDepth(thestack)>call->stackMark+call->localCacheCount*2 )
   {
      Var *it = SECODE_STACK_POP(thestack);
      VAR_REMOVE_USER(it,call);
   }
   
   /* pop off local cache */
   SECODE_STACK_MULTIPOP(thestack,call->localCacheCount*2);
#endif
   

   /* pop off the parameters */
   for( x=0;x<InputVariableCount;x++ )
   {
      Var *sv = SECODE_STACK_POP(thestack);
      VAR_THOROUGH_REMOVE_USER(sv,call);
   }

   /* Finally, return the result of the function */
   result = call->pReturnVar;

   VAR_REMOVE_USER(call->lockFunc,call);
   
   callDelete(call);

   if( result==NULL ) result = constructVarRead(CallerCall,VUndefined);

   return result;
}


#if defined(JSE_CREATEFUNCTIONTEXTVARIABLE) && \
    (0!=JSE_CREATEFUNCTIONTEXTVARIABLE)
/* ----------------------------------------------------------------------
   ------- start of writing function out as text ------------------------ */

/* I added these functions here to move them out of the core since
   they are big and rarely called - Rich */

struct GrowingBuffer
{
   jsechar *data;
   uint datalen;
};


   static void
growingInit(struct GrowingBuffer *this)
{
   this->data = jseMustMalloc(jsechar,sizeof(jsechar));
   this->datalen = 0;
}


   static void
growingTerm(struct GrowingBuffer *this)
{
   jseMustFree(this->data);
}

   static void NEAR_CALL
growingAddTo(struct GrowingBuffer *this,const jsechar *text)
{
   uint newsize;
   this->data = jseMustReMalloc(jsechar,this->data,((newsize = this->datalen +
                                                 strlen_jsechar(text))+1)*sizeof(jsechar));
   strcpy_jsechar(this->data+this->datalen,text);
   this->datalen = newsize;
}


   VarRead * NEAR_CALL
functionTextAsVariable(struct Function *this,struct Call *call)
{
  VarRead *ret = constructVarRead(call,VString);
  struct GrowingBuffer buff;

  growingInit(&buff);

# if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
    growingAddTo(&buff,( Func_CBehavior & this->flags )
                       ? textcoreCFunctionKeyword : textcoreFunctionKeyword );
# else
    growingAddTo(&buff, textcoreFunctionKeyword );
# endif
  growingAddTo(&buff,UNISTR(" "));
  growingAddTo(&buff,functionName(this,call));

  /* We can only print out the actual text of the function if it is a
   * localfunction (i.e. constructed from ScriptEase text.)
   */
  if( functionTestIfLocal(this) )
  {
     int count;
     struct LocalFunction *func = (struct LocalFunction *)this;
     struct Code *c;

     /* ----------------------------------------------------------------------
      * First spit out the parameters
      * ----------------------------------------------------------------------
      */

      growingAddTo(&buff,UNISTR("("));

      assert( !LOCAL_TEST_IF_INIT_FUNCTION((struct LocalFunction*)this,call) );

      for( count=(int)func->InputParameterCount-1;count>=0;count-- )
      {
         if( (uint)(count)!=func->InputParameterCount-1 )
            growingAddTo(&buff,UNISTR(","));
         growingAddTo(&buff,GetStringTableEntry(call,func->VarNames[count],NULL));
      }
      growingAddTo(&buff,UNISTR(")\r\n{\r\n  "));

      /* ----------------------------------------------------------------------
       * Then the body of the function
       * ----------------------------------------------------------------------
       */

      c = codeNext(func->FirstCode);

      while( c )
        {
          switch( codeGetType(c) )
          {
             case sourceLineNumber:
                growingAddTo(&buff,UNISTR("\r\n  ")); break;
             case pushVariable:
                growingAddTo(&buff,GetStringTableEntry(call,codeGetName(c),NULL));
                break;
             case pushValue:
             {
                Var *cgVar = codeGetVar(c);
                VarRead *var = GET_READABLE_VAR(cgVar,call);
                if( VAR_TYPE(var)!=VString )
                {
                   VarRead *val = convert_var(call,var,jseToString);
                   growingAddTo(&buff,(const jsechar *)varGetData(val,0));
                   VAR_REMOVE_USER(val,call);
                } else {
                   uint x;
                   const jsechar *val = (const jsechar *)varGetData(var,0);

                   growingAddTo(&buff,UNISTR("\""));

                   for( x = 0;val[x];x++ )
                   {
                      jsechar tbuf[10];

                      if( isprint_jsechar(val[x]) )
                      {
                         tbuf[0] = val[x]; tbuf[1] = '\0';
                      } else {
                         jse_sprintf(tbuf,UNISTR("\\x%x"),val[x]);
                         if ( 0 == tbuf[3] )
                         {
                            /* not long enough, need extra 0 */
                            tbuf[4] = 0;
                            tbuf[3] = tbuf[2];
                            tbuf[2] = '0';
                         }
                      }
                      growingAddTo(&buff,tbuf);
                   }
                   growingAddTo(&buff,UNISTR("\""));
                }
                VAR_REMOVE_USER(var,call);
                break;
             }
             case Label:
                growingAddTo(&buff,GetStringTableEntry(call,codeGetName(c),NULL));
                growingAddTo(&buff,UNISTR(":")); break;
             case gotoAlways:
                growingAddTo(&buff,UNISTR("goto "));
                growingAddTo(&buff,GetStringTableEntry(call,codeGetName(c),NULL));
                break;
             case UnresolvedFunctionCall:
                growingAddTo(&buff,GetStringTableEntry(call,codeGetName(c),NULL));
                growingAddTo(&buff,UNISTR("(")); break;
             case structureMemberName:
                growingAddTo(&buff,UNISTR("."));
                growingAddTo(&buff,GetStringTableEntry(call,codeGetName(c),NULL));
                break;
             case structureMember:
                growingAddTo(&buff,UNISTR("["));
                break;
             case functionCall: case EvaluationGroup:
                growingAddTo(&buff,UNISTR("(")); break;
             case getParamValue:
                growingAddTo(&buff,UNISTR(",")); break;
             case ConditionalFalse: case CaseColon:
                growingAddTo(&buff,UNISTR(":")); break;
             case BeginBlock:
                growingAddTo(&buff,UNISTR("{")); break;
             case EndBlock:
                growingAddTo(&buff,UNISTR("}")); break;
             case EndFunctionCall: case EndEvaluationGroup:
                growingAddTo(&buff,UNISTR(")")); break;
             case EndArrayIndex:
                growingAddTo(&buff,UNISTR("]")); break;
             case StatementEnd:
                growingAddTo(&buff,UNISTR(";")); break;
            default:
               if ( !(BeginOpList <= codeGetType(c) &&
                      EndOpList >= codeGetType(c)) )
               {
                  struct KeyWords_ const *Key;
                  for ( Key = KeyWords; NULL != Key->Word; Key++ )
                     if( Key->Type==codeGetType(c) &&
                         Key->Type!=UnknownCardType )
                     {
                        growingAddTo(&buff,Key->Word);
                        growingAddTo(&buff,UNISTR(" "));
                        break;
                     }
                  if( Key->Word!=NULL ) break;
               }
              assert( BeginOpList <= codeGetType(c) &&
                      EndOpList >= codeGetType(c) );
              growingAddTo(&buff,getOpDescription(codeGetType(c))->TokenText);
              break;
            }

          growingAddTo(&buff,UNISTR(" "));
          c = codeTrueNext(c);
        }

      growingAddTo(&buff,UNISTR("\r\n"));
    } else {
       /* While this is not technically valid Javascript, it is a close
        * as we can come - there is no way to print out the text of a wrapper
        * function, which is written in C.
        */
       growingAddTo(&buff,UNISTR("();\r\n"));
    }

  varPutStringLen(ret,call,buff.data,buff.datalen);

  growingTerm(&buff);

  return ret;
}

/* ------- end of writing function out as text --------------------------
   ---------------------------------------------------------------------- */
#endif /* defined(JSE_CREATEFUNCTIONTEXTVARIABLE) */
