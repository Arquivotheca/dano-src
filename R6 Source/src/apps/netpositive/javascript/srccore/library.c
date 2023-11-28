/* library.c
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

struct Library * NEAR_CALL libraryNew(struct Call *call,struct Library *Parent)
{
   struct Library *this = jseMustMalloc(struct Library,sizeof(struct Library));

   this->prev = NULL;
   if ( NULL != Parent )
   {
      /* This 'count' stuff is necessary since we must link the libraries in
       * the same order they were originally added - the stuff at the front
       * of the list was added last.
       */

     int count = 0;
     struct Library *ll;
     int i;

     for ( ll = Parent->prev; NULL != ll; ll = ll->prev ) {
       count ++;
     }

     for( i = count - 1;i>=0;i-- )
       {
          int j;
          ll = Parent->prev;
          for( j = 0;j<i;j++ ) ll = ll->prev;

          libraryAddFunctions(this,call,ll->ObjectVarName,ll->FunctionList,
                              ll->LibInit,
                              ll->LibTerm,ll->LibraryData);
       }
   }
   return this;
}


void NEAR_CALL libraryDelete(struct Library *this,struct Call *call)
{
#ifndef NDEBUG
   /* Nombas-West sez:
    *    commented out the following line because it asserts
    *    when using polymorphic functions from jse gadgets.
    *    Brent indicates that this assumption should not be
    *    significant unless we use a termination function when
    *    adding the functions.
    *    assert( LinkedLibraryList->RememberAddCall == &call );
    * Brent sez:
    *    I'm not sure that RememberAddCall == &call is even necessary. It's
    *    probaby just an anal check to make sure data is OK?  My guess
    *    to Nombas-West was that it only applies for a termination function
    *    and so I'll add that check here.  If this is still a problem then I
    *    suggest seeing why a term function may want the call and adding
    *    a specifiv check there, or removing the anal RememberAddCall
    *    altogether.
    * I've commented out this line based on the above while I'm working
    * on SE:VB - JMC
    *    assert( LinkedLibraryList->RememberAddCall == &call  ||\
    *    NULL == LinkedLibraryList->LibTerm );
    */
#endif
   struct Library *loop = this->prev,*tmp;
   while( loop )
   {
      if ( NULL != (loop->LibTerm) )
      {
#        if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) &&\
            (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
            DispatchToClient(call->Global->ExternalDataSegment,
                             (ClientFunction)(loop->LibTerm),
                             (void *)(&call),loop->LibraryData);
#        else
            (*(loop->LibTerm))(call,loop->LibraryData);
#        endif
#        if !defined(NDEBUG) && (0<JSE_API_ASSERTLEVEL) && defined(_DBGPRNTF_H)
            if ( !jseApiOK )
            {
               DebugPrintf(UNISTR("Error calling library terminate function"));
               DebugPrintf(UNISTR("Error message: %s"),jseGetLastApiError());
            }
#        endif
         assert( jseApiOK );
      }

      tmp = loop->prev;
      jseMustFree(loop);
      loop = tmp;
   }
   jseMustFree(this);
}


void NEAR_CALL libraryAddFunctions(struct Library *this,struct Call *call,
                                   const jsechar * ObjectVarName,
                                   struct jseFunctionDescription const *
                                      FunctionList,
                                   jseLibraryInitFunction pLibInit,
                                   jseLibraryTermFunction pLibTerm,
                                   void _FAR_ *ParentLibData)
{
   struct Library *ll = jseMustMalloc(struct Library,sizeof(struct Library));
   /* determine which object to add this set of functions to */
   VarRead * BeginObjectToAddTo = call->session.GlobalVariable;
   VarRead * ObjectToAddTo;

   ll->ObjectVarName = ObjectVarName;
   ll->FunctionList = FunctionList;

   if ( NULL != ObjectVarName )
   {
      /* find object by name in global variable */
      BeginObjectToAddTo = GetDotNamedVar(call,BeginObjectToAddTo,
                                          ObjectVarName,True);
   }
   /* ensure that variable is an object;
    * this variable will be set to initialize, and then if FuncObject */
   ObjectToAddTo = GetDotNamedVar(call,BeginObjectToAddTo,NULL,True);

   /* add each function within this library to our function list */
   for ( ; NULL != FunctionList->FunctionName; FunctionList++ )
   {
      VarRead *SetAttrOnVar = NULL;
      switch ( FunctionList->FuncAttributes & 0x0F )
      {
         case jseFunc_FuncObject:
            libfuncNew(call,BeginObjectToAddTo,FunctionList,
                       &(ll->LibraryData));
            /* ObjectToAddTo should now be switched to the object we
             * just added */
            ObjectToAddTo = GetDotNamedVar(call,BeginObjectToAddTo,
                                           FunctionList->FunctionName,True);
            break;
         case jseFunc_ObjectMethod:
            libfuncNew(call,ObjectToAddTo,FunctionList,&(ll->LibraryData));
            break;
         case jseFunc_PrototypeMethod:
            libfuncNew(call,GetDotNamedVar(call,ObjectToAddTo,
                                           ORIG_PROTOTYPE_PROPERTY,True),
                       FunctionList,&(ll->LibraryData));
            break;
         case jseFunc_AssignToVariable:
         {
            SetAttrOnVar = GetDotNamedVar(call,ObjectToAddTo,
                                          FunctionList->FunctionName,False);
            /* variable may already exist, so clear its attributes */
            varSetAttributes(SetAttrOnVar,0);
            varAssign(SetAttrOnVar,call,
               GetDotNamedVar(call,call->session.GlobalVariable,
                  (const jsechar *)(JSE_POINTER_UINT)(FunctionList->FuncPtr),
                  False));
            break;
         }
         case jseFunc_LiteralValue:
         {
            Var *result;
            SetAttrOnVar = GetDotNamedVar(call,ObjectToAddTo,
                                          FunctionList->FunctionName,False);
            /* variable may already exist, so clear its attributes */
            varSetAttributes(SetAttrOnVar,0);
            if ( jseInterpret(call,NULL,(jsechar *)(JSE_POINTER_UINT)(FunctionList->FuncPtr),
                              NULL,jseNewNone,JSE_INTERPRET_DEFAULT,NULL,&result) )
            {
               varAssign(SetAttrOnVar,call,result);
               VAR_REMOVE_USER(result,call);
            }
            break;
         }
         case jseFunc_LiteralNumberPtr:
         {
            SetAttrOnVar = GetDotNamedVar(call,ObjectToAddTo,
                                          FunctionList->FunctionName,False);
            varConvert(SetAttrOnVar,call,VNumber);
            varPutNumberFix(SetAttrOnVar,call,
               *((jsenumber *)(JSE_POINTER_UINT)(FunctionList->FuncPtr)),VNumber);
            break;
         }
         case jseFunc_SetAttributes:
         {
            SetAttrOnVar = GetDotNamedVar(call,ObjectToAddTo,
                                          FunctionList->FunctionName,False);
            break;
         }
        #ifndef NDEBUG
         default:
            InstantDeath(textcoreUNKNOWN_FUNCATTRIBUTE,
                         FunctionList->FuncAttributes & 0x0F);
        #endif
      }
      if ( NULL != SetAttrOnVar )
      {
         varSetAttributes(SetAttrOnVar,FunctionList->VarAttributes);
      }
   }

   /* add this library into the linked library list and
    * call its initialization function */
   ll->prev = this->prev;
   #ifndef NDEBUG
      ll->RememberAddCall = call;
   #endif
   ll->LibInit = pLibInit;
   ll->LibTerm = pLibTerm;
   ll->LibraryData =
                 ( pLibInit ) ?
               #if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) \
                && (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
                   (void _FAR_ *)DispatchToClient(call->Global->
                                                     ExternalDataSegment,
                                                  (ClientFunction)pLibInit,
                                                  (void *)&call,ParentLibData)
               #else
                  (*pLibInit)(call,ParentLibData)
               #endif
               : ParentLibData ;
#  if !defined(NDEBUG) && (0<JSE_API_ASSERTLEVEL) && defined(_DBGPRNTF_H)
      if ( !jseApiOK )
      {
         DebugPrintf(UNISTR("Error calling library init function"));
         DebugPrintf(UNISTR("Error message: %s"),jseGetLastApiError());
      }
#  endif
   assert( jseApiOK );
   this->prev = ll;
}

struct LibraryFunction * NEAR_CALL libfuncNew(struct Call *call,
            VarRead *ObjectToAddTo,
            struct jseFunctionDescription const *iFuncDesc,
            void _FAR_ * * LibraryDataPtr)
{
   struct LibraryFunction *this =
      jseMustMalloc(struct LibraryFunction,sizeof(struct LibraryFunction));

   functionInit(&(this->function),call,
                GetDotNamedVar(call,ObjectToAddTo,
                               iFuncDesc->FunctionName,True),
                (uword8)iFuncDesc->VarAttributes,False,
                iFuncDesc->FuncAttributes & jseFunc_PassByReference,
                (iFuncDesc->MaxVariableCount==-1)?iFuncDesc->MinVariableCount:
                  iFuncDesc->MaxVariableCount);
   assert(NULL != iFuncDesc);
   assert(NULL != iFuncDesc->FuncPtr);
   this->FuncDesc = (struct jseFunctionDescription *)iFuncDesc;
   this->LibData.DataPtr =  LibraryDataPtr;
   this->function.flags |= Func_StaticLibrary;

   return this;
}


struct LibraryFunction * NEAR_CALL libfuncNewWrapper(
               struct Call *call,const jsechar *iFunctionName,
               void (JSE_CFUNC FAR_CALL *FuncPtr)(jseContext jsecontext),
               sword8 MinVariableCount, sword8 MaxVariableCount,
               jseVarAttributes VarAttributes, jseFuncAttributes FuncAttributes, void _FAR_ *fData)
{
   struct LibraryFunction *this =
      jseMustMalloc(struct LibraryFunction,sizeof(struct LibraryFunction));

   functionInit(&(this->function),call,NULL,0,False,FuncAttributes &
                jseFunc_PassByReference,
           (MaxVariableCount==-1)?MinVariableCount:MaxVariableCount);
   assert(NULL != iFunctionName);
   assert(NULL != FuncPtr);
   assert( !(Func_StaticLibrary & this->function.flags) );
   this->FuncDesc =
      jseMustMalloc(struct jseFunctionDescription, \
                    sizeof(struct jseFunctionDescription));
   this->FuncDesc->FunctionName = StrCpyMalloc(iFunctionName);
   this->FuncDesc->FuncPtr = FuncPtr;
   this->FuncDesc->MinVariableCount = MinVariableCount;
   this->FuncDesc->MaxVariableCount = MaxVariableCount;
   this->FuncDesc->VarAttributes = VarAttributes;
   this->FuncDesc->FuncAttributes = FuncAttributes;
   this->LibData.Data = fData;

   return this;
}

void NEAR_CALL libfuncDelete(struct LibraryFunction *this)
{
   if ( !(Func_StaticLibrary & this->function.flags) )
   {
      assert( NULL != this->FuncDesc );
      assert( NULL != this->FuncDesc->FunctionName );
      assert( NULL != this->FuncDesc->FunctionName );
      jseMustFree( (jsechar *)(this->FuncDesc->FunctionName) );
      jseMustFree(this->FuncDesc);
   }
}


void NEAR_CALL libfuncExecute(struct LibraryFunction *this,
                              struct Call *call,VarRead *ThisVar)
{
   sword8 VarCount = (sword8)callParameterCount(call);
   struct jseFunctionDescription *funcDesc = this->FuncDesc;

/*
// seb 98.11.15
if (ThisVar) {
	int i;
printf("libfuncExecute setting This to 0x%x\n", ThisVar ? ThisVar : call->session.GlobalVariable);
printf("---\n");
printf("This has %d members\n", ThisVar->varmem->data.vobject.members.used);
	for (i = 0; i < ThisVar->varmem->data.vobject.members.used; i++)
		printf("%s  0x%x\n", ThisVar->varmem->data.vobject.members.members[i].Name,
							 ThisVar->varmem->data.vobject.members.members[i].var);
printf("---\n");
}
*/
   SetCurrentThisVar(call,ThisVar ? ThisVar : call->session.GlobalVariable );
   assert( 0 <= this->FuncDesc->MinVariableCount );
   assert( this->FuncDesc->MinVariableCount <=
           this->FuncDesc->MaxVariableCount  ||
           -1 == this->FuncDesc->MaxVariableCount );
   if ( ( VarCount < funcDesc->MinVariableCount )
     || ( VarCount > funcDesc->MaxVariableCount
       && !( -1 == funcDesc->MaxVariableCount
          || (call->Global->ExternalLinkParms.options & jseOptIgnoreExtraParameters)
           )
        )
      )
   {
      callQuit(call,textcoreINVALID_PARAMETER_COUNT,VarCount,
               funcDesc->FunctionName);
   }
   else
   {
      SetTemporaryLibraryData(call,(Func_StaticLibrary & this->function.flags) ?
                              *(this->LibData.DataPtr) : this->LibData.Data );
      #if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) \
       && (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
         DispatchToClient(call->Global->ExternalDataSegment,
                          (ClientFunction)(this->FuncDesc->FuncPtr),
                          (void *)call);
      #else
         (*(this->FuncDesc->FuncPtr))(call);
      #endif
#     if !defined(NDEBUG) && (0<JSE_API_ASSERTLEVEL) && defined(_DBGPRNTF_H)
         if ( !jseApiOK )
         {
            DebugPrintf(UNISTR("Error calling library function \"%s\""),
                        functionName(&(this->function),call));
            DebugPrintf(UNISTR("Error message: %s"),jseGetLastApiError());
         }
#     endif
      assert( jseApiOK );
   }
}
