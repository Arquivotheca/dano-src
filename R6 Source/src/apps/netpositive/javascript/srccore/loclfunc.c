/* loclfunc.c   Parameters and code cards within a local function.
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

   struct LocalFunction * NEAR_CALL
localNew(struct Call *call,VarName iFunctionName,struct Code *BeginBlockCode
#  if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
      ,jsebool CBehavior)
#  else
      )
#  endif
{
   struct LocalFunction *this =
      jseMustMalloc(struct LocalFunction,sizeof(struct LocalFunction));
   functionInit(&(this->function),call,
                GetDotNamedVar(call,call->session.GlobalVariable,
                               GetStringTableEntry(call,iFunctionName,NULL),True),
                (uword8)(iFunctionName==GLOBAL_STRING(call,init_function_entry) ?
                         jseDontEnum :
                         jseDefaultAttr),
                True,
#               if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                   CBehavior,
#               else
                   False,
#               endif
                0);

   AddStringUser(iFunctionName);

   this->FunctionName = iFunctionName;
#  if (0!=JSE_COMPILER)   
      assert( !(Func_VarNamesResolved & this->function.flags) );
      this->FirstCode = BeginBlockCode;
#  endif

   this->InputParameterCount = 0;
   this->VarNames = NULL;
   this->VarAttrib = NULL;
    
   return this;
}


   void NEAR_CALL
localDelete(struct LocalFunction *this,struct Call *call)
{
   secodeDeleteOpcodes(this->opcodes,this->opcodesUsed,call);

#  if (0!=JSE_COMPILER)
      if ( NULL != this->FirstCode )
      {
         assert( BeginBlock == codeGetType(this->FirstCode) );
         assert( NULL != codePair(this->FirstCode) &&
                 this->FirstCode == codePair(codePair(this->FirstCode)) );
         assert( EndBlock == codeGetType(codePair(this->FirstCode)) );
         codeFreeAllMemory(this->FirstCode,call);
      }
#  endif      
   if ( NULL != this->VarNames )
   {
      uint i;
      for( i = 0; i < this->InputParameterCount; i++ )
         RemoveFromStringTable(call,this->VarNames[i]);
         
      jseMustFree(this->VarNames);
   }
   if ( NULL != this->VarAttrib )
      jseMustFree(this->VarAttrib);

   RemoveFromStringTable(call,this->FunctionName);
}


   static sint NEAR_CALL
localAddVarName(struct LocalFunction *this,struct Call * call,VarName name)
{
   UNUSED_PARAMETER(call);

   this->VarNames = jseMustReMalloc(VarName,this->VarNames,
                                    (this->InputParameterCount+1) *
                                    sizeof(VarName));
   this->VarNames[this->InputParameterCount] = name;
   AddStringUser(name);

   this->VarAttrib = jseMustReMalloc(uword8,this->VarAttrib,
                                     (this->InputParameterCount+1) *
                                     sizeof(uword8));
   this->VarAttrib[this->InputParameterCount] = 0;

   return (sint) this->InputParameterCount++;
}


#if (0!=JSE_COMPILER)

   jsebool NEAR_CALL
localResolveVariableNames(struct LocalFunction *this,struct Call *call)
{
   jsebool success = True;
   struct Code *c;
   struct Code *next;
   VarName thename;

   if ( (Func_VarNamesResolved & this->function.flags) )
      return True;

   if ( !LOCAL_TEST_IF_INIT_FUNCTION(this,call) ) {
      struct Code *EndFunctionCallCode = codePrev(this->FirstCode);
      /* This is not the initial globalization function, and so create new
       * Function, with proper links and parameter group offsets.
       */
      struct Code *BeginFunctionCallCode = codePair(EndFunctionCallCode);
      assert( NULL != BeginFunctionCallCode  &&
              EndFunctionCallCode == codePair(BeginFunctionCallCode) );
      for ( c = codePrev(EndFunctionCallCode);
            c != BeginFunctionCallCode;
            c = codePrev(c) ) {
         assert( pushVariable == codeGetType(c) ||
                 passByReference == codeGetType(c) );
         if( pushVariable == codeGetType(c) )
         {
            assert( NULL != codeGetName(c) );
            localAddVarName(this,call,codeGetName(c));
         }
         else
            this->VarAttrib[this->InputParameterCount-1] = 1;
      }
      /* delete all these code cards, for they are used by no one */
      assert( NULL == codePrev(BeginFunctionCallCode) );
      for ( next = BeginFunctionCallCode; next != this->FirstCode;
            next = codeNext(c=next), codeDelete(c,call))
         ;
   }

   if ( success )
   {
      /* Unresolved function calls must be resolved now;
         change to FunctionCall or to VariableFunctionCall */
      for ( c = this->FirstCode; NULL != c; c = codeNext(c) )
      {
         codeval type = codeGetType(c);
         if ( UnresolvedFunctionCall == type )
         {
            /* insert card which is the var to evaluate */
            struct Code *cc = codeNew(codeTruePrev(c));
            thename = codeGetName(c);

            codeSetType(cc,pushVariable);
            AddStringUser(thename);
            codeSetName(cc,thename);

            codeSetType(c,functionCall);
         } else if ( DeclareFunction == type || DeclareCFunction == type )
         {
            callQuit(call,textcoreMISPLACED_KEYWORD,
#                    if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                        DeclareCFunction == type ? textcoreCFunctionKeyword :
#                    endif
                     textcoreFunctionKeyword);
            success = False;
            break;
         }
      }
   }

   this->function.params = this->InputParameterCount;

   if( this->function.stored_in )
     {
       varSetFunctionPtr(this->function.stored_in,call,
                         (struct Function *)this);
          /* reset function to update param count */
     }

   this->function.flags |= Func_VarNamesResolved;

   if ( !secompileCompile(this,call) )
      success = False;

#  if !defined(JSE_CREATEFUNCTIONTEXTVARIABLE) || \
      (0==JSE_CREATEFUNCTIONTEXTVARIABLE)
      /* free code memory for DOS because we need all the memory we can get */
      codeFreeAllMemory(this->FirstCode,call);
      this->FirstCode = NULL;
#  endif

   return success;
}
#endif /* #if (0!=JSE_COMPILER) */


   void NEAR_CALL
localExecute(struct LocalFunction *this,struct Call *call,VarRead *ThisVar)
{
#  if (0!=JSE_COMPILER)
      assert( (Func_VarNamesResolved & this->function.flags) );
#  endif

#if defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS)
   call->localCacheLocation = secodestackDepth(call->Global->thestack);
   call->localCacheCount = 0;
#endif
   
   if ( !LOCAL_TEST_IF_INIT_FUNCTION(this,call) )
   {
      int count;
      uint limit = callParameterCount(call);
      VarRead *VariableObject = call->VariableObject;

      assert( NULL != VariableObject );

      /* For whatever reason, the names are stored in opposite order */
      for( count= (int) this->InputParameterCount-1;count>=0;count-- )
      {
         /* The names are in opposite order, but the parameters are not */
         uint stackcount = this->InputParameterCount-count-1;
         Var *var;

         if( stackcount<limit )
         {
            struct StructureMember *mem;
            struct Varobj *obj;
            MemCountUInt hint;
   
            var = callGetVar(call,stackcount);
            obj = &(VariableObject->varmem->data.vobject.members);
            
            if( NULL == (mem = varobjFindMember(obj,this->VarNames[count],&hint)) )
               mem = varobjCreateMemberWithHint(obj,call,this->VarNames[count],VUndefined,hint);
            
            varAddUser(var);
            VAR_REMOVE_USER(mem->var,call);
            mem->var = var;
         }
         else
            var = varCreateMember(VariableObject,call,this->VarNames[count],
                            VUndefined);
                            
#if defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS)
         SECODE_STACK_PUSH(call->Global->thestack,this->VarNames[count]);
         SECODE_STACK_PUSH(call->Global->thestack,var);

         call->localCacheCount++;
#endif
      }
   }

   /* all calls get the this var initially set to the global object.
    * That is what we want if ThisVar is NULL. If it is not, set it
    */
   if( ThisVar )
      SetCurrentThisVar(call,ThisVar);


   /* Set up for interpretting */
   secodeInterpInit(this,call);
}

#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   static void NEAR_CALL
localTokenWrite(struct LocalFunction *this,struct Call *call,
                struct TokenSrc *tSrc)
{
   uint i;

   /* save functions indicator, name, and local variable count */
   tokenWriteString(call,tSrc,this->FunctionName);
   assert( Func_VarNamesResolved & this->function.flags );
   /* write byte for boolean about whether this is a C function
      and other flags */
   tokenWriteByte(tSrc, (uword8)( functionCBehavior(&(this->function))
                                  ? '\1' : '\0' ) );
   /* write all thetokens that make up this function */
   secodeTokenWriteList(call,tSrc,this);
   /* write all the varnames for this function */
   tokenWriteLong(tSrc,(long)this->InputParameterCount);
   for ( i = 0; i < this->InputParameterCount; i++ )
   {
      tokenWriteString(call,tSrc,this->VarNames[i]);
   }
}

   void
tokenWriteAllLocalFunctions(struct TokenSrc *this,struct Call *call)
{
   VarName FunctionName;
   VarRead *member = NULL;

   assert( NULL != call->session.GlobalVariable );
   assert( VObject == VAR_TYPE(call->session.GlobalVariable) );
   while ( NULL != (member = varGetNext(call->session.GlobalVariable,
                                        call,member,&FunctionName)) )
   {
      if ( VObject == VAR_TYPE(member) )
      {
         struct Function * func = varGetFunction(member,call);
         if ( NULL != func )
         {
            if ( functionTestIfLocal(func) )
            {
               /* yea!  a local function.  Write it out in tokenized form. */
               tokenWriteByte(this,(uword8)NEW_FUNCTION_NAME);
               localTokenWrite((struct LocalFunction *)(func),call,this);
            }
         }
      }
   }
}
#endif

#if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)
   void
localTokenRead(struct Call *call,struct TokenDst *tDst)
{
   uint i;
   struct LocalFunction *func;
   uint InputParameterCount;
   VarName varname;

   varname = tokenReadString(call,tDst);

   /* read list of all the tokens that make up this function */
   func = localNew(call,varname,NULL
#                  if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                      ,tokenReadByte(tDst)/*C_function*/
#                  endif
                   );
#  if (0!=JSE_COMPILER)
      func->function.flags |= Func_VarNamesResolved;
#  endif
   /* read the tokens that make the compiled form of this function */
   secodeTokenReadList(call,tDst,func);
   /* read all the varnames for this function */
   InputParameterCount = (uint)tokenReadLong(tDst);
   assert( 0 == func->InputParameterCount );
   for( i = 0; i < InputParameterCount; i++ ) {
      varname = tokenReadString(call,tDst);
      localAddVarName(func,call,varname);
   }
   assert( func->InputParameterCount == InputParameterCount );
}
#endif
