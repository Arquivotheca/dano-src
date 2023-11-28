/* Var.c  Access to variables of all kinds.
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


#  if (0!=JSE_CYCLIC_GC)
   static void NEAR_CALL
varmemGCListAdd(struct Varmem *varmem,struct Call *call)
{
   struct Varmem *next = *(varmem->data.vobject.gcPrevPtr = &(call->Global->gcStartList));
   call->Global->gcStartList = varmem;
   if ( NULL != (varmem->data.vobject.gcNext = next) )
   {
      next->data.vobject.gcPrevPtr = &(varmem->data.vobject.gcNext);
   }
}

   static void NEAR_CALL
varmemGCListRemove(struct Varmem *varmem)
{
   struct Varmem **prevPtr = varmem->data.vobject.gcPrevPtr;
   if ( NULL != (*prevPtr = varmem->data.vobject.gcNext) )
   {
      (*prevPtr)->data.vobject.gcPrevPtr = prevPtr;
   }
}
#endif

   static void NEAR_CALL 
varmemRemoveUser(struct Varmem *varmem,struct Call *call
#if (0!=JSE_CYCLIC_CHECK) && (0==JSE_FULL_CYCLIC_CHECK)
   ,jsebool look_for_orphan_loops
#endif
);

/* These are here because they need to be macro-ized in some way that will
 * work. It is too slow to make them real functions */

   void _HUGE_ * NEAR_CALL
varGetData(VarRead *th,JSE_POINTER_SINDEX index)
{
   /* make sure the data place we're about to read has room to fit */
   varmemValidateIndex(th->varmem,
                       index,0,
                       False,False);
   return HugePtrAddition(th->varmem->data.vpointer.memory,
                          (index+th->offset_or_createType-th->varmem->
                           data.vpointer.zeroIndex)*
                          varmemArrayElementSize(th->varmem));
}

   void NEAR_CALL
varReplaceVarmem(struct Var *th,struct Call *call,struct Varmem *newmem)
{
   jseVarAttributes attr = varGetAttributes(th);

   varmemAddUser(newmem);
   if( th->varmem )
   {
#     if (0!=JSE_CYCLIC_CHECK) && (0==JSE_FULL_CYCLIC_CHECK)
         varmemRemoveUser(th->varmem,call,True);
#     else
         varmemRemoveUser(th->varmem,call);
#     endif
   }
   th->varmem = newmem;

   varSetAttributes(th,attr);
}

/* ----------------------------------------------------------------------
 * class Var's members
 * ---------------------------------------------------------------------- */

   void NEAR_CALL
varReallyRemoveUser(struct Var *this,struct Call *call
#if (0!=JSE_CYCLIC_CHECK) && (0==JSE_FULL_CYCLIC_CHECK)
   ,jsebool look_hard
#endif
) {
#  if !defined(JSE_INLINES) || (0==JSE_INLINES)
      if ( 0 != --(this->userCount) )
         return;
#  endif
   assert( this->userCount==0 );
   if ( VAR_HAS_REFERENCE(this) )
   {
      if ( VAR_HAS_DATA(this) )
      {
         /* this is a dynamic-write ready to be output */
         VarRead *value;
         assert( VAR_DYNAMIC_WRITE(this) );
         value = constructVarRead(call,
                             (VarType)(this->offset_or_createType));
         varReplaceVarmem(value,call,this->varmem);
         varPutValue(this,call,value);
#warning Look_into_this_some_more
// seb 98.11.14 -- This is causing the block to be written to after being freed.
//         VAR_REMOVE_USER(value,call);
      }
      if ( NULL != this->reference.parentObject )
      {
         VAR_REMOVE_USER(this->reference.parentObject,call);
      }
      RemoveFromStringTable(call,this->reference.memberName);
   }

   /* no more users, free the varmem as well. For references,
    * it has no varmem
    */
   if ( VAR_HAS_DATA(this) )
   {
#     if (0!=JSE_CYCLIC_CHECK) && (0==JSE_FULL_CYCLIC_CHECK)
         varmemRemoveUser(this->varmem,call,look_hard);
#     else
         varmemRemoveUser(this->varmem,call);
#     endif
   }

   /* we are done with this var structure as well */
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      this->cookie = 0;
#  endif
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      allocatorFreeItem(&(call->Global->allocate_var),(void *)this);
#  else
      jseMustFree(this);
#  endif
#  if !defined(NDEBUG)
      call->Global->totalVarCount--;
#  endif
}


/* translate to Readable and Writeable vars */


   VarRead * NEAR_CALL
reallyGetReadableVar(struct Var *this,struct Call *call)
{
   VarRead *property, *it, *thisObject;

   assert( this!=NULL );

#  if !defined(JSE_INLINES) || (0==JSE_INLINES)
      if( VAR_HAS_DATA(this) )
      {
         varAddUser(this);
         return this;
      }
#  endif

   /* should have been taken care of by the inline wrapper */
   assert( !VAR_HAS_DATA(this) );
   assert( VAR_HAS_REFERENCE(this) );
   assert( NULL != this->reference.memberName );
   
   thisObject = this->reference.parentObject;

   /* First, if the base is NULL, this is an access of an undefined variable */
   if( NULL == thisObject )
   {
      VarRead *var = varGetDirectMember(call,call->session.GlobalVariable,
                                        this->reference.memberName,True);
      if( var )
      {
         varAddUser(var);
         return var;
      }

      var = constructVarRead(call,(VarType)(this->offset_or_createType));
      callError(call,textcoreVAR_TYPE_UNKNOWN,
                GetStringTableEntry(call,this->reference.memberName,NULL));
      return var;
   }

   assert( NULL != thisObject && VObject == thisObject->varmem->data.vall.dataType );

#  if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
   {
      /* we must determine if we need to do dynamic GET */
      if( HAS_GET_PROPERTY(&(thisObject->varmem->data.vobject.members)) &&
          (property = varGetMember(call,thisObject,GLOBAL_STRING(call,get_entry)))
            !=NULL &&
          VARMEM_IS_FUNCTION(property->varmem,call) )
      {
         VarRead *retvar;
         varCallDynamicProperty(thisObject,call,property,OFF_GET_PROP,
                                this->reference.memberName,NULL,&retvar);
         assert( retvar!=NULL );
         return retvar;
      }
   }
#  else
      UNUSED_PARAMETER(property);
#  endif

   /* at this point, we need to look for the variable. It should be there. */
   it = varGetMember(call,thisObject,this->reference.memberName);

   if( it==NULL )
   {
      /* This shouldn't happen, but here is some error recovery */
      it = constructVarRead(call,VUndefined);
      varSetLvalue(it,True);
   } else {
      assert( it->userCount>0 );
      varAddUser(it);
   }

   return it;
}


   VarWrite * NEAR_CALL
getWriteableVar(struct Var *this,struct Call *call)
{
   VarRead *property;
   VarWrite *new_var, *thisObject;
   struct Global_ * global;
   VarName Name;

   assert( this!=NULL );

   if( VAR_HAS_DATA(this) )
   {
      /* either it is a VarRead || a VarWrite already */
      varAddUser(this);
      return (VarWrite *)this;
   }
   assert( VAR_HAS_REFERENCE(this) );

   global = call->Global;

   if ( NULL == (thisObject = this->reference.parentObject) )
   {
      /* must have a thisObject. default to local or global object */
      thisObject = (jseOptDefaultLocalVars & global->ExternalLinkParms.options)
                 ? call->VariableObject : call->session.GlobalVariable ;
   }
   assert( NULL != thisObject );
   assert( VObject == thisObject->varmem->data.vall.dataType );
   Name = this->reference.memberName;
   assert( NULL != Name );

   if ( HAS_CANPUT_PROPERTY(&(thisObject->varmem->data.vobject.members))
     && NULL != (property = varGetMember(call,thisObject,GLOBAL_STRING(call,canput_entry)))
     && VAR_IS_FUNCTION(property,call)
     && !varCanPut(thisObject,call,Name)
       )
   {
      /* Ugh, that is ugly! It means if we have an object that does have
       * a CanPut property and that it turns out we can't put, then we
       * do just like a dynamic put and flag that it goes nowhere when
       * completed
       */
      thisObject = NULL;
      Name = (VarName)0;
   }
   else
   {
      if ( !HAS_PUT_PROPERTY(&(thisObject->varmem->data.vobject.members))
        || NULL == (property = varGetMember(call,thisObject,GLOBAL_STRING(call,put_entry)))
        || !VAR_IS_FUNCTION(property,call) )
      {
         /* it does not have a dynamic put, so return this object's property
          * or create it if not there (do not use the prototype when writing.)
          */
         /* if wasn't found, or if it was found but is the default
            * Object.prototype or Function.prototype, then create a real
            * version.
            */
         if ( NULL == (new_var = varGetDirectMember(call,thisObject,Name,False)) )
         {
            new_var = varCreateMember(thisObject,call,Name,
                                      (VarType)(this->offset_or_createType));
         }
         assert( NULL != new_var );
         assert( 0 < new_var->userCount );
         varAddUser(new_var);
         return new_var;
      }
   }

   /* last case, it is a dynamic put thing, create a variable for this */
   new_var =
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      (VarWrite *)allocatorAllocItem(&(global->allocate_var));
#  else
      jseMustMalloc(VarWrite,sizeof(*new_var));
#  endif
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      new_var->cookie = jseVariable_cookie;
#  endif
#  if !defined(NDEBUG)
      call->Global->totalVarCount++;
#  endif
   new_var->userCount = 1;
   if ( 0 != (new_var->reference.memberName = Name) )
      AddStringUser(Name);
   new_var->reference.parentObject = ( NULL == thisObject )
                   ? (VarRead *)NULL : CONSTRUCT_VALUE_LOCK(call,thisObject) ;
   new_var->offset_or_createType = 0;
   /* create a blank undefined varmem which we will write into, and that
    * is what gets dynamically put
    */
   varmemAddUser(new_var->varmem = varmemNew(call,
                                      (VarType)(this->offset_or_createType)));
   new_var->deref = False;
   new_var->lvalue = True;

   return new_var;
}


/*
 * This code assigns each member to make sure it is what it should be.
 * Old code used memset() to do this, but tests have shown that memset()
 * is a lot slower. (wrldtest.jse was 11.2 versus 13.2 seconds - big
 * difference. For this test, the three construct_var() and the
 * init_varmem() were changes to use memset())
 *
 * NOTE: the code has since changed slightly, but the idea hasn't.
 */

/* construct a blank variable of some given data type */
   VarRead * NEAR_CALL
constructVarRead(struct Call *call,VarType dataType)
{
   VarRead *new_var =
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      (VarRead *)allocatorAllocItem(&(call->Global->allocate_var));
#  else
      jseMustMalloc(VarRead,sizeof(*new_var));
#  endif
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      new_var->cookie = jseVariable_cookie;
#  endif
#  if !defined(NDEBUG)
      call->Global->totalVarCount++;
#  endif

   assert(isValidVarType(dataType));

   new_var->userCount = 1;
   varmemAddUser(new_var->varmem = varmemNew(call,dataType));

   new_var->reference.memberName = NULL;
   new_var->reference.parentObject = NULL;
   new_var->offset_or_createType = 0;
   new_var->deref = False;
   new_var->lvalue = False;

   return new_var;
}


   struct Var * NEAR_CALL
constructReference(struct Call *call,VarRead *object,VarName membername)
{
   Var *new_var;
   assert( NULL==object || VObject == object->varmem->data.vall.dataType );
   assert( NULL != membername );

   new_var =
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      (Var *)allocatorAllocItem(&(call->Global->allocate_var));
#  else
      jseMustMalloc(Var,sizeof(*new_var));
      UNUSED_PARAMETER(call);
#  endif
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      new_var->cookie = jseVariable_cookie;
#  endif
#  if !defined(NDEBUG)
      call->Global->totalVarCount++;
#  endif

   /* for speed we construct just a lock. Most of the time, the reference
    * is to an item in the scope chain which will not change. The rest,
    * the routines that call this now provide a value-locked variable
    */
   if ( NULL != (new_var->reference.parentObject = object) )
      varAddUser(object);

   new_var->userCount = 1;
   new_var->reference.memberName = membername;
   AddStringUser(membername);

   new_var->lvalue = True;      /* references are always lvalues */

   new_var->offset_or_createType = 0;
   new_var->varmem = NULL;
   new_var->deref = False;

   return new_var;
}


   VarRead * NEAR_CALL
constructSibling(
#  if (defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)) || !defined(NDEBUG)
      struct Call *call,
#  endif
   VarRead *relativeVar,
   JSE_POINTER_SINDEX offset,jsebool deref)
{
   VarRead *new_var;
   assert( !deref || VAR_ARRAY_POINTER(relativeVar) );
   assert( ! (deref && varIsDeref(relativeVar)) );

   new_var =
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      (VarRead *)allocatorAllocItem(&(call->Global->allocate_var));
#  else
      jseMustMalloc(VarRead,sizeof(*new_var));
#  endif
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      new_var->cookie = jseVariable_cookie;
#  endif
#  if !defined(NDEBUG)
      call->Global->totalVarCount++;
#  endif

   new_var->userCount = 1;
   new_var->offset_or_createType = relativeVar->offset_or_createType + offset;
   varmemAddUser(new_var->varmem = relativeVar->varmem);
   new_var->deref = (jsetinybool)deref;
   new_var->lvalue = relativeVar->lvalue;

   new_var->reference.memberName = NULL;
   new_var->reference.parentObject = NULL;

   if( (offset!=0 || deref) && VAR_ARRAY_POINTER(relativeVar) )
   {
      varmemValidateIndex(new_var->varmem,new_var->offset_or_createType,1,False,False);
   }

   return new_var;
}


   VarRead * NEAR_CALL
constructValueLock(
#  if (defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)) || !defined(NDEBUG)
      struct Call *call,
#  endif
      VarRead *orig)
{
   VarRead *new_var =
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      (VarRead *)allocatorAllocItem(&(call->Global->allocate_var));
#  else
      jseMustMalloc(VarRead,sizeof(*new_var));
#  endif
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      new_var->cookie = jseVariable_cookie;
#  endif
#  if !defined(NDEBUG)
      call->Global->totalVarCount++;
#  endif

   new_var->userCount = 1;
   new_var->offset_or_createType = orig->offset_or_createType;
   varmemAddUser(new_var->varmem = orig->varmem);
   new_var->lvalue = orig->lvalue;

   new_var->deref = 0;
   new_var->reference.memberName = NULL;
   new_var->reference.parentObject = NULL;

   return new_var;
}


/* miscellaneous routines */


/* even though this routine is forced to save a temp var, that is not too
 * bad as this routine can only called via the API from a wrapper function,
 * and as soon as that function returns, the temp var is freed up
 */
   void _HUGE_ * NEAR_CALL
GetWriteableData(struct Var *this,VarRead *thisReadable,
                 struct Call *call,JSE_POINTER_UINDEX *len)
{
   VarRead *newit;

   if ( !VAR_HAS_DATA(this) && NULL!=this->reference.parentObject
     && varGetDirectMember(call,this->reference.parentObject,this->reference.memberName,False)!=NULL )
   {
      /* in this case the variable is a real var not one in the prototype so
       * we don't have to do any inefficient copying
       */
      void _HUGE_ *ret;
      assert( VAR_ARRAY_POINTER(thisReadable) );
      ret = varGetData(thisReadable,0);
      if( len ) *len = varGetArrayLength(thisReadable,call,NULL);
      return ret;
   }

   newit = constructVarRead(call,(VarType)(this->offset_or_createType));
   varCopyAssign(newit,call,thisReadable);
   CALL_ADD_TEMP_VAR(call,newit);

   assert( VAR_ARRAY_POINTER(newit) );
   if( len ) *len = varGetArrayLength(newit,call,NULL);
   return varGetData(newit,0);
}


#if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
   void NEAR_CALL
varPutValue(struct Var *this,struct Call *call,VarRead *value)
{
   VarRead *property;
   assert( VAR_HAS_REFERENCE(this)  &&  NULL != this->reference.parentObject );
// seb 99.2.3 - This fix is from Nombas.
   property = varMemberGet(call,this->reference.parentObject,GLOBAL_STRING(call,put_entry),False,True);
//   property = varGetDirectMember(call,this->reference.parentObject,GLOBAL_STRING(call,put_entry),False);
   assert( property!=NULL && VAR_IS_FUNCTION(property,call) );

   varCallDynamicProperty(this->reference.parentObject,call,property,OFF_PUT_PROP,
                          this->reference.memberName,value,NULL);
}
#endif


/* ----------------------------------------------------------------------
 * varobj functions
 * ---------------------------------------------------------------------- */

   static void NEAR_CALL
varobjTerm(struct Varobj *this,struct Call *call)
{
   MemCountUInt x;

   if ( NULL == this->members )
      return;

#if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)   
   if ( HAS_DELETE_PROPERTY(this) )
    {
       /* construct a variable to do the dynamic delete in since
        * our variable is already in the middle of being deleted
        */
       VarRead *tmp = constructVarRead(call,VObject);
       struct Varobj *it = &(tmp->varmem->data.vobject.members);

       struct Varobj save = *it;
       *it = *this;

       varDeleteMember(tmp,call,GLOBAL_STRING(call,delete_entry));

       *this = *it;     /* in case it got modified */
       *it = save;

       VAR_REMOVE_USER(tmp,call);
    }
#endif


   /* Since we are just removing user counts, no need to do all the
    * fancy remove member stuff
    */
   for( x=this->used; 0 != x--; )
   {
      RemoveFromStringTable(call,this->members[x].Name);
      VAR_THOROUGH_REMOVE_USER(this->members[x].var,call);
   }

   if( NULL != this->members )
   {
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      assert( this->alloced>=VAROBJSIZE );
      /* Even if it is bigger than the minimum, it can still be pooled. However,
       * if it is much bigger, we don't want to waste lots of memory by storing
       * it.
       */
      if( this->alloced>3*VAROBJSIZE )
         jseMustFree(this->members);
      else
         allocatorFreeItem(&(call->Global->allocate_objmem),(void *)(this->members));
#  else
      jseMustFree(this->members);
#  endif
   }
}


   struct StructureMember * NEAR_CALL
varobjCreateMemberWithHint(struct Varobj *this,struct Call *call,
                           VarName Name,VarType DType, MemCountUInt hint)
{
   MemCountUInt x;
   MemberArrayPtr member;
   struct Global_ *global = call->Global;

   assert( isValidVarType(DType) );

#  if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
      this->members = jseMustReMalloc(struct StructureMember,this->members,
            (uint)((this->used+1)*sizeof(struct StructureMember)));
#  else
      assert( this->used<=this->alloced );
      if( this->used==this->alloced )
      {
         this->alloced *= 2;

         if( this->alloced== 0 )
         {
            this->alloced = VAROBJSIZE;
            this->members =
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
               (struct StructureMember *)allocatorAllocItem(&(global->allocate_objmem));
#  else
            jseMustMalloc(struct StructureMember,this->alloced*sizeof(struct StructureMember));
#  endif
         }
         else
         {
            assert( this->members!=NULL );
            this->members = jseMustReMalloc(struct StructureMember,this->members,
                                            (uint)(this->alloced*sizeof(struct StructureMember)));
         }
      }
#  endif

   /* find out where this member ought to be put. */
   for( x=hint;x<this->used;x++ )
   {
      if( Name<this->members[x].Name ) break;
   }

   if( x < this->used )
   {
#     if ( defined(__JSE_DOS16__) || defined(__JSE_WIN16__) ) \
      && (0!=JSE_MIN_MEMORY) && !defined(JSE_NO_HUGE)
         HugeMemMove(this->members+x+1,this->members+x,
                     (uint)((this->used-x)*sizeof(struct StructureMember)));
#     else
         memmove(this->members+x+1,this->members+x,
                 (uint)((this->used-x)*sizeof(struct StructureMember)));
#     endif
   }
   this->used++;
   member = this->members+x;
   member->var = constructVarRead(call,DType);
   /* structure members are lvalues. */
   varSetLvalue(member->var,True);
   member->Name = Name;
   AddStringUser(Name);

#  if defined(JSE_DYNAOBJ_HASHLIST) && (0!=JSE_DYNAOBJ_HASHLIST) 
      /* if this is one of the dynamic objects, which are all within global,
       * and if not a number that just happens to be in that range, then
       * set local flag
       */
      if ( ( (void *)global < (void *)Name )
        && ( (void *)Name < (void *)(global + 1) )
        && ( 0 == ((uint)(JSE_POINTER_UINT)Name & 1) ) )
      {
         this->flags |= DynamicHashListFromName(Name)->HasFlag;
         varSetAttributes(member->var,jseDontEnum);
      }
#  elif defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
      if( Name==global->global_strings[prototype_entry] )
         this->flags |= HAS_PROTOTYPE_PROP;
      else if( Name==global->global_strings[delete_entry] )
         this->flags |= HAS_DELETE_PROP;
      else if( Name==global->global_strings[put_entry] )
         this->flags |= HAS_PUT_PROP;
      else if( Name==global->global_strings[canput_entry] )
         this->flags |= HAS_CANPUT_PROP;
      else if( Name==global->global_strings[get_entry] )
         this->flags |= HAS_GET_PROP;
      else if( Name==global->global_strings[hasproperty_entry] )
         this->flags |= HAS_HAS_PROP;
      else if( Name==global->global_strings[call_entry] )
         this->flags |= HAS_CALL_PROP;
#     if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
      else if( Name==global->global_strings[operator_entry] )
         this->flags |= HAS_OPERATOR_PROP;
#     endif
#  endif

   return (struct StructureMember *)member;
}

   void NEAR_CALL
varobjRemoveMember(struct Varobj *this,struct Call *call,
                   struct StructureMember * member)
{
   MemCountUInt elem_num =
      (MemCountUInt)((MemberArrayPtr)member-this->members);
   VarRead * to_del;
   struct Global_ *global = call->Global;
   VarName Name = member->Name;

   assert( elem_num<this->used );
   assert( NULL != this->members );

#  if defined(JSE_DYNAOBJ_HASHLIST) && (0!=JSE_DYNAOBJ_HASHLIST) 
      /* if this is one of the dynamic objects, which are all within global,
       * and if not a number that just happens to be in that range, then
       * set local flag
       */
      if ( ( (void *)global < (void *)Name )
        && ( (void *)Name < (void *)(global + 1) )
        && ( 0 == ((uint)(JSE_POINTER_UINT)Name & 1) ) )
      {
         this->flags &= (uword16)~(DynamicHashListFromName(Name)->HasFlag);
      }
#  elif defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
      if( Name==global->global_strings[prototype_entry] )
         this->flags &= (uword16) ~HAS_PROTOTYPE_PROP;
      else if( Name==global->global_strings[delete_entry] )
         this->flags &= (uword16) ~HAS_DELETE_PROP;
      else if( Name==global->global_strings[put_entry] )
         this->flags &= (uword16) ~HAS_PUT_PROP;
      else if( Name==global->global_strings[canput_entry] )
         this->flags &= (uword16) ~HAS_CANPUT_PROP;
      else if( Name==global->global_strings[get_entry] )
         this->flags &= (uword16) ~HAS_GET_PROP;
      else if( Name==global->global_strings[hasproperty_entry] )
         this->flags &= (uword16) ~HAS_HAS_PROP;
      else if( Name==global->global_strings[call_entry] )
         this->flags &= (uword16) ~HAS_CALL_PROP;
#     if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
      else if( Name==global->global_strings[operator_entry] )
         this->flags &= (uword16) ~HAS_OPERATOR_PROP;
#     endif
#  endif

   RemoveFromStringTable( call, Name );

   to_del = member->var;

   if ( 0 == --(this->used) )
   {
      /* no more elements in this object; free pointer */
#     if !defined(JSE_MIN_MEMORY) || (0==JSE_MIN_MEMORY)
         this->alloced = 0;
#     endif
      jseMustFree(this->members);
      this->members = NULL;
   }
   else
   {
      if( elem_num < this->used )
      {
#        if ( defined(__JSE_DOS16__) || defined(__JSE_WIN16__) ) \
         && (0!=JSE_MIN_MEMORY) && !defined(JSE_NO_HUGE)
            HugeMemMove(this->members+elem_num,this->members+elem_num+1,
                        (uint)((this->used-elem_num)*
                               sizeof(struct StructureMember)));
#        else
            memmove(this->members+elem_num,this->members+elem_num+1,
                 (uint)((this->used-elem_num)*sizeof(struct StructureMember)));
#      endif
      }

#     if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
         this->members = jseMustReMalloc(struct StructureMember,this->members,
               (uint)(this->used*sizeof(struct StructureMember)));
#     endif
   }

   /* we are no longer using this variable */
   VAR_THOROUGH_REMOVE_USER(to_del,call);
}


/* ----------------------------------------------------------------------
 * varmem functions
 * ---------------------------------------------------------------------- */


   struct Function * NEAR_CALL
varmemGetFunction(struct Varmem *this,struct Call *call)
{
   assert( this->data.vall.dataType==VObject );

#  if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
      /* Prevent looping call chains */
      if( !this->data.vobject.been_here &&
          hasCallProperty(&(this->data.vobject.members)) )
      {
         MemCountUInt hint;
         struct StructureMember *callmem =
            varobjFindMember(&(this->data.vobject.members),
                             GLOBAL_STRING(call,call_entry),
                             &hint);
         if( callmem!=NULL &&
             callmem->var->varmem->data.vall.dataType==VObject )
         {
            struct Function *callfunc;
            this->data.vobject.been_here = True;
            callfunc = varGetFunction(callmem->var,call);
            this->data.vobject.been_here = False;
            if( callfunc!=NULL ) return callfunc;
         }
      }
#  else
      UNUSED_PARAMETER(call);
#  endif

   return this->data.vobject.function;
}

   struct Varmem * NEAR_CALL
varmemNew(struct Call *call,VarType dataType)
{
   struct Varmem *This;
   
   assert( isValidVarType(dataType) );
   
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      This =
         (struct Varmem *)allocatorAllocItem(&(call->Global->allocate_varmem));
#  else
      UNUSED_PARAMETER(call);
#  endif
#  if !defined(NDEBUG)
      call->Global->totalVarmemCount++;
#  endif

   if ( VNumber==dataType || VBoolean==dataType )
   {
#     if !defined(JSE_FAST_MEMPOOL) || (0==JSE_FAST_MEMPOOL)
         This = jseMustMalloc(struct Varmem,sizeof(This->data.vnumber));
#     endif
      This->data.vnumber.value = 0;
   }
   else if( VObject == dataType )
   {
#     if !defined(JSE_FAST_MEMPOOL) || (0==JSE_FAST_MEMPOOL)
         This = jseMustMalloc(struct Varmem,sizeof(This->data.vobject));
#     endif
#     if !defined(JSE_MIN_MEMORY) || (0==JSE_MIN_MEMORY)
         This->data.vobject.members.alloced = 0;
#     endif
      This->data.vobject.members.used = 0;
      This->data.vobject.members.members = NULL;
#     if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
         This->data.vobject.members.flags = 0;
#     endif
      This->data.vobject.function = NULL;
      This->data.vobject.been_here = 0;
#     if (0!=JSE_CYCLIC_GC)
         varmemGCListAdd(This,call);
#     endif
   }
   else if ( VString==dataType
#          if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
           || VBuffer==dataType
#          endif
            )
   {
#     if !defined(JSE_FAST_MEMPOOL) || (0==JSE_FAST_MEMPOOL)
         This = jseMustMalloc(struct Varmem,sizeof(This->data.vpointer));
#     endif
#     if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
         *(jsechar *)(This->data.vpointer.memory =
                      jseMustMalloc(void,(uint)
                                    (This->data.vpointer.alloced=2))) = 0;
#     else
         *(jsechar *)(This->data.vpointer.memory =
                      jseMustMalloc(void,(uint)
                                    (This->data.vpointer.alloced=25))) = 0;
#     endif
      This->data.vpointer.count = 0;
      This->data.vpointer.zeroIndex = 0;
   }
   else
   {
      assert( VNull==dataType || VUndefined==dataType );
#     if !defined(JSE_FAST_MEMPOOL) || (0==JSE_FAST_MEMPOOL)
         This = jseMustMalloc(struct Varmem,sizeof(This->data.vall));
#     endif
   }

   assert( &(This->data.vall) == &(This->data.vnumber.vall) );
   assert( &(This->data.vall) == &(This->data.vobject.vall) );
   assert( &(This->data.vall) == &(This->data.vpointer.vall) );

   /* fill in data that is common to all varmem types */
   This->data.vall.userCount = 0;
   This->data.vall.attributes = 0;
   This->data.vall.dataType = dataType;
      
   return This;
}


   static void NEAR_CALL
varmemDelete(struct Varmem *this,struct Call *call)
{
   assert( 0 == this->data.vall.userCount );

   if( VObject == this->data.vall.dataType )
   {
      /* Added, remove it for other reference count scheme */
      varobjTerm(&(this->data.vobject.members),call);
      if( NULL != this->data.vobject.function )
      {
         functionDelete(this->data.vobject.function,call);
      }
#     if (0!=JSE_CYCLIC_GC)
         varmemGCListRemove(this);
#     endif
   }
   else if( VString==this->data.vall.dataType
#    if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)   
     || VBuffer==this->data.vall.dataType
#    endif     
      )
   {
      assert( NULL != this->data.vpointer.memory );
      /* free the array memory used by this object */
#     if defined(HUGE_MEMORY)
      {
         ulong ByteCount = this->data.vpointer.count;
         if( VBuffer != this->data.vall.dataType )
            ByteCount *= sizeof(jsechar);
         if( HUGE_MEMORY <= ByteCount )
         {
            HugeFree(this->data.vpointer.memory);
         }
         else
         {
            jseMustFree(this->data.vpointer.memory);
         }
      }
#     else
         jseMustFree(this->data.vpointer.memory);
#     endif
   }

#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      allocatorFreeItem(&(call->Global->allocate_varmem),(void *)this);
#  else
      jseMustFree(this);
#  endif
#  if !defined(NDEBUG)
      call->Global->totalVarmemCount--;
#  endif
}

#if (0!=JSE_CYCLIC_CHECK)
#define CYC_LOOP_NODE   0x01
#define CYC_BEEN_HERE   0x02
   static jsebool NEAR_CALL
CountPathsAndUsers(struct Varmem *this,uint *UserCountTotal,
                   uint *PathCountTotal)
   /* return False if this does not lead to original object, else !False */
{
   MemberArrayPtr member;
   MemCountUInt x;
   jsebool Loopy;
   assert( 0 == (CYC_BEEN_HERE & this->data.vobject.been_here) );
   Loopy = False;
   this->data.vobject.been_here += (uword8)CYC_BEEN_HERE;
   member = this->data.vobject.members.members;

   for( x = this->data.vobject.members.used; 0 != x--; member++ )
   {
      struct Varmem *vmem = member->var->varmem;
      assert( NULL != vmem );

#warning Take_this_code_out
if (vmem == NULL) {
	printf("Skipping reference in CountPathsAndUsers\n");
	continue;
}
      if( VObject == vmem->data.vall.dataType )
      {
         if ( CYC_BEEN_HERE <= vmem->data.vobject.been_here )
         {
            if ( (CYC_BEEN_HERE|CYC_LOOP_NODE) != vmem->data.vobject.been_here )
               continue;
         }
         else
         {
            if ( !CountPathsAndUsers(vmem,UserCountTotal,PathCountTotal) )
               continue;
         }
         Loopy = True;
         (*PathCountTotal)++;
      }
   }
   if ( Loopy )
   {
      (*UserCountTotal) += this->data.vall.userCount;
      this->data.vobject.been_here = CYC_LOOP_NODE|CYC_BEEN_HERE;
      return True;
   }
   return False;
}

   static void NEAR_CALL
RestoreBeenHereFlag(struct Varmem *this)
{
   MemberArrayPtr member;
   MemCountUInt x;
   assert( 0 != this->data.vobject.been_here );
   this->data.vobject.been_here = 0;
   member = this->data.vobject.members.members;
   for( x = this->data.vobject.members.used; 0 != x--; member++ )
   {
      struct Varmem *vmem = member->var->varmem;
      assert( NULL != vmem );
#warning Take_this_code_out
if (vmem == NULL) {
	printf("Skipping reference in RestoreBeenHereFlag\n");
	continue;
}
      if( VObject == vmem->data.vall.dataType  &&  0 != vmem->data.vobject.been_here )
      {
         RestoreBeenHereFlag(vmem);
      }
   }
}

   static jsebool NEAR_CALL
varmemDeleteIfClosed(struct Varmem *this,struct Call *call)
{
   uint UserCountTotal = 0; uint PathCountTotal = 0;

   assert( 0 < this->data.vall.userCount );
   assert( VObject == this->data.vall.dataType );
   assert( !this->data.vobject.been_here );

   /* tally user counts wrapping around all objects this points to */
   this->data.vobject.been_here = CYC_LOOP_NODE;
   CountPathsAndUsers(this,&UserCountTotal,&PathCountTotal);
   RestoreBeenHereFlag(this);
   assert( PathCountTotal <= UserCountTotal );
   if( ( 0 != UserCountTotal )  &&  ( UserCountTotal == PathCountTotal ) )
   {
      /* to delete this object, must unlink the vobject from the chain so that
       * it is not screwed up while trying to free a cyclic link */
      struct Function *OldFunc = this->data.vobject.function;
      this->data.vall.dataType = VUndefined;
      this->data.vall.userCount++;
      varobjTerm(&(this->data.vobject.members),call);
      if( NULL != OldFunc ) functionDelete(OldFunc,call);
#     if (0!=JSE_CYCLIC_GC)
         varmemGCListRemove(this);
#     endif
      assert( --(this->data.vall.userCount) == 0 );
      varmemDelete(this,call);
      return True;
   }
   return False;
}
#endif

#if (0!=JSE_CYCLIC_GC)
   ulong
CollectCyclicGarbage(struct Call *call)
{
   ulong count = 0;
   struct Varmem *vm, *next = call->Global->gcStartList;
   call->Global->CollectingCyclicGarbage = True;
   while ( (NULL != (vm=next))  && call->Global->CollectingCyclicGarbage )
   {
      next = vm->data.vobject.gcNext;
      if ( NULL == call->session.GlobalVariable
        || vm != call->session.GlobalVariable->varmem )
      {
         if ( varmemDeleteIfClosed(vm,call) )
         {
            count++;
            /* restart list from beginning, in case new loops may be freed */
            next = call->Global->gcStartList;
         }
      }
   }
   call->Global->CollectingCyclicGarbage = False;
   return count;
}
#endif

#if defined(HUGE_MEMORY)

   static void NEAR_CALL
varmemArrayReMalloc(struct Varmem *this,JSE_POINTER_UINDEX NewElementCount)
{
   JSE_POINTER_UINDEX OldElementCount;
   void *OldPtr;

   /* convert elements to bytes */
   OldElementCount = this->data.vpointer.count;
#  if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
      if ( VBuffer != this->data.vall.dataType ) {
         NewElementCount <<= 1;
         OldElementCount <<= 1;
      } /* endif */
#  endif

   if ( OldElementCount < HUGE_MEMORY ) {
      /* using regular memory so far */
      if ( NewElementCount < HUGE_MEMORY ) {
         /* moving from regular memory to regular memory */
         this->data.vpointer.memory =
            jseMustReMalloc(void,this->data.vpointer.memory,
                            (uint)NewElementCount+sizeof(jsechar));
      } else {
         /* moving from small memory to big memory */
         OldPtr = this->data.vpointer.memory;
         this->data.vpointer.memory =
            HugeMalloc(NewElementCount+sizeof(jsechar));
         memcpy(this->data.vpointer.memory,OldPtr,(uint)OldElementCount);
         jseMustFree(OldPtr);
      } /* endif */
   } else {
      /* using huge memory so far */
      if ( HUGE_MEMORY <= NewElementCount ) {
         /* moving from huge memory to huge memory */
         this->data.vpointer.memory =
            HugeReMalloc(this->data.vpointer.memory,
                         NewElementCount+sizeof(jsechar));
      } else {
         /* moving from huge memory to regular memory */
         OldPtr = this->data.vpointer.memory;
         this->data.vpointer.memory =
            jseMustMalloc(void,(uint)NewElementCount+sizeof(jsechar));
         memcpy(this->data.vpointer.memory,OldPtr,(uint)NewElementCount);
         HugeFree(OldPtr);
      } /* endif */
   } /* endif */
   *(jsechar _HUGE_ *)(((ubyte _HUGE_ *)(this->data.vpointer.memory))+
                       NewElementCount) = 0;
   /* always end in null just in case */
}

#else

#if !defined(JSE_MIN_MEMORY) || (0==JSE_MIN_MEMORY)
#  define TOO_BIG 100
#endif
   static void NEAR_CALL
varmemArrayReMalloc(struct Varmem *this,JSE_POINTER_UINDEX NewElementCount)
{
#  if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
         if ( VBuffer != this->data.vall.dataType )
#     endif
            NewElementCount <<= 1;
#  endif
   /* if number of bytes needed with space for the terminating '\0'
    * is more than we have currently alloced, allocate it
    */
#  if !defined(JSE_MIN_MEMORY) || (0==JSE_MIN_MEMORY)
   if( (NewElementCount+sizeof(jsechar))>this->data.vpointer.alloced ||
       (this->data.vpointer.alloced>TOO_BIG &&
        NewElementCount<this->data.vpointer.alloced-TOO_BIG) )
#  endif
   {
#     if !defined(JSE_MIN_MEMORY) || (0==JSE_MIN_MEMORY)
         this->data.vpointer.alloced = NewElementCount + 50;
#     else
         this->data.vpointer.alloced = NewElementCount + 2;
#endif
      this->data.vpointer.memory =
         jseMustReMalloc(void,this->data.vpointer.memory,
                         (uint)this->data.vpointer.alloced);
   }
   *(jsechar *)(((ubyte *)(this->data.vpointer.memory))+NewElementCount)
      = '\0'; /* always end in null just in case */
}
#endif

   static void NEAR_CALL
varmemInitializeMemory(struct Varmem *this,
                       JSE_POINTER_UINDEX StartOffset,
                       JSE_POINTER_UINDEX ElementCount)
{
   uint ElemSize;

#  if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
      assert( VString == this->data.vall.dataType ||
              VBuffer == this->data.vall.dataType );
#  else      
      assert( VString == this->data.vall.dataType );
#  endif      
   assert( NULL != this->data.vpointer.memory );
   assert( 0 < ElementCount );
   assert( JSE_DEBUG_FEEDBACK(1) == sizeof(ubyte) );
   ElemSize = varmemArrayElementSize(this);
   HugeMemSet(HugePtrAddition(this->data.vpointer.memory,
                              StartOffset * ElemSize),
              0,ElemSize * ElementCount);
}

   void NEAR_CALL
varmemValidateIndex(struct Varmem *this,JSE_POINTER_SINDEX Index,
                    JSE_POINTER_UINDEX Count,
                    jsebool DeleteFromStartIfTooMany,
                    jsebool DeleteFromEndIfTooMany)
{
   uint ElementSize;
   JSE_POINTER_UINDEX NewElementCount, OldElementCount;
   JSE_POINTER_SINDEX NewElementsNeeded, UnwantedElementCount;

#  if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
      assert( this->data.vall.dataType==VString ||
              this->data.vall.dataType==VBuffer );
#  else      
      assert( this->data.vall.dataType==VString );
#  endif      
   assert( NULL != this->data.vpointer.memory );
   ElementSize = varmemArrayElementSize(this);

   if( Index < this->data.vpointer.zeroIndex )
   {
      /* create data before previous ElementZeroIndex */
      NewElementsNeeded = -(Index-this->data.vpointer.zeroIndex);
      assert( 0 < NewElementsNeeded );
      varmemArrayReMalloc(this,NewElementCount =
                          this->data.vpointer.count+NewElementsNeeded);
      assert( JSE_DEBUG_FEEDBACK(1) == sizeof(ubyte) );
      HugeMemMove(HugePtrAddition(this->data.vpointer.memory,
                                  ElementSize*NewElementsNeeded),
                  this->data.vpointer.memory,ElementSize *
                  this->data.vpointer.count);
      /* update indexes */
      this->data.vpointer.count = NewElementCount;
      this->data.vpointer.zeroIndex = Index;
      /* initialize new elements of array to 0 */
      varmemInitializeMemory(this,0,(ulong)NewElementsNeeded);
   }
   else if( this->data.vpointer.zeroIndex < Index  &&
            DeleteFromStartIfTooMany )
   {
      /* too many elements, and so remove the extras */
      UnwantedElementCount = Index-this->data.vpointer.zeroIndex;
      assert( 0 < UnwantedElementCount );
      assert( JSE_DEBUG_FEEDBACK(1) == sizeof(ubyte) );
      HugeMemMove(this->data.vpointer.memory,
                  HugePtrAddition(this->data.vpointer.memory,
                                  ElementSize*UnwantedElementCount),
                  ElementSize * (NewElementCount =
                                 this->data.vpointer.count -
                                 UnwantedElementCount));
      varmemArrayReMalloc(this,NewElementCount);
      this->data.vpointer.count = NewElementCount;
      this->data.vpointer.zeroIndex = Index;
   } /*endif */
   NewElementsNeeded = (slong)(Index - this->data.vpointer.zeroIndex +
      Count - this->data.vpointer.count);
   if ( 0 < NewElementsNeeded )
   {
      /* add new elements at the end of the current array */
      OldElementCount = this->data.vpointer.count;
      NewElementCount = this->data.vpointer.count + NewElementsNeeded;
      varmemArrayReMalloc(this,NewElementCount);
      /* update indexes */
      this->data.vpointer.count = NewElementCount;
      varmemInitializeMemory(this,OldElementCount,(ulong)NewElementsNeeded);
   }
   else if ( NewElementsNeeded < 0  &&  DeleteFromEndIfTooMany )
   {
      /* too many elements, and so remove the extras */
      UnwantedElementCount = -NewElementsNeeded;
      assert( 0 < UnwantedElementCount );
      varmemArrayReMalloc(this,NewElementCount =
                          this->data.vpointer.count - UnwantedElementCount);
      this->data.vpointer.count = NewElementCount;
   }
}

   static void NEAR_CALL
varmemRemoveUser(struct Varmem *this,struct Call *call
#if (0!=JSE_CYCLIC_CHECK) && (0==JSE_FULL_CYCLIC_CHECK)
   ,jsebool look_for_orphan_loops
#endif
) {
   assert( (this)->data.vall.userCount!=0 );
   if( --(this->data.vall.userCount)==0 )
   {
      varmemDelete(this,call);
   }
#  if (0!=JSE_CYCLIC_CHECK)
   else
   {
      if (
#          if (0==JSE_FULL_CYCLIC_CHECK)
              look_for_orphan_loops &&
#          endif
           VObject==this->data.vall.dataType
         )
      {
         varmemDeleteIfClosed(this,call);
      }
   }
#  endif
}

   struct StructureMember * NEAR_CALL
varobjFindMember(struct Varobj *this,VarName Name,MemCountUInt *hint)
{
   register MemCountUInt lower, upper, middle;
   register struct StructureMember * mem;
   *hint = 0;

   if ( NULL == this->members )
      return NULL;

   upper = this->used - 1;

#  if 0
   // if ( upper < BSEARCH_CUTOFF )
   // {
   //    MemCountUInt x;
   //    for( mem = this->members, x = 0;x <= upper; x++, mem++ )
   //    {
   //       if( Name <= mem->Name )
   //       {
   //          if ( Name == mem->Name )
   //             return mem;
   //          break;
   //       }
   //    }
   //    *hint = x;
   //    return NULL;
   // }
#  endif

   lower = 0;
   for ( ; ; )
   {
      assert( lower<=upper );
      mem = (struct StructureMember *)(this->members + (middle = ((lower+upper) >> 1)));
         /* casting for systems where addition must be HUGE */

      if( mem->Name <= Name )
      {
         *hint = middle;
         if ( mem->Name == Name )
            return mem;
         if( middle==upper ) return NULL;
         lower = middle + 1;
      }
      else
      {
         if( middle==lower ) return NULL;
         upper = middle - 1;
      }
   }
}
