/* varread.c  Access to variables of all kinds.
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


   jsenumber NEAR_CALL
varGetNumber(VarRead *this)
{
   jsenumber ret;
   VarType vType;

   assert( VAR_HAS_DATA(this) );

   vType = this->varmem->data.vall.dataType;

   if ( VNumber == vType  ||  VBoolean == vType )
   {
      return this->varmem->data.vnumber.value;
   }
   else if( varIsDeref(this) )
   {
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)       
      if( VBuffer == vType )
      {
         ubyte *text;
         assert( VBuffer == vType );
         text = (ubyte *)
            HugePtrAddition(this->varmem->data.vpointer.memory,
                            (this->offset_or_createType-this->varmem->
                             data.vpointer.zeroIndex) *
                            sizeof(ubyte));
         ret = (jsenumber)text[0];
      }
      else
#     endif      
      {
         jsechar *text;
         assert( VString == vType );
         text = (jsechar *)
            HugePtrAddition(this->varmem->data.vpointer.memory,
                            (this->offset_or_createType-this->varmem->data.
                             vpointer.zeroIndex) *
                            sizeof(jsechar));
         ret = (jsenumber)text[0];
      }
   }
   else if( VNull==vType )
   {
      ret = 0;
   }
   else if( VUndefined==vType )
   {
      ret = jseNaN;
   }
   else
   {
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)       
         assert( VString==vType || VBuffer==vType || VObject==vType );
#     else
         assert( VString==vType || VObject==vType );
#     endif         
      ret = 1;
   }
   return ret;
}


   jsenumber NEAR_CALL
varGetValidNumber(VarRead *this,struct Call *call)
{
   jsenumber num;
   
   assert( VAR_HAS_DATA(this) );
   num = varGetNumber(this);

   /* jseIsNaN is slow, so don't call it unless needed */
   if( (jseOptWarnBadMath & call->Global->ExternalLinkParms.options)
    && jseIsNaN(num) )
   {
      callError(call,textcoreIS_NAN);
   }
   
   return num;
}

   JSE_POINTER_UINDEX NEAR_CALL
varGetArrayLength(VarRead *this,struct Call *call,
                  JSE_POINTER_SINDEX *MinIndex)
{
   assert( VAR_HAS_DATA(this) );

   UNUSED_PARAMETER(call);

   if( VAR_ARRAY_POINTER(this) )
   {
      /* we shift the values based on our offset into this array */
      if( MinIndex )
         *MinIndex = this->varmem->data.vpointer.zeroIndex-this->offset_or_createType;
      return this->varmem->data.vpointer.count
           - (this->offset_or_createType - this->varmem->data.vpointer.zeroIndex);
   }
   else
   {
      JSE_POINTER_SINDEX MinIdx, MaxIdx;
      MemCountUInt x;
      struct Varobj *obj = &(this->varmem->data.vobject.members);

      MaxIdx = -1;
      MinIdx = 0;
      for( x=0;x<obj->used;x++ )
      {
         VarName entry = obj->members[x].Name;
         if( IsNumericStringTableEntry(call,entry) )
         {
            JSE_POINTER_SINDEX Idx = GetNumericStringTableEntry(call,entry);
            if ( Idx < MinIdx )
               MinIdx = Idx;
            if ( MaxIdx < Idx )
               MaxIdx = Idx;
         }
      }
      if( MinIndex ) *MinIndex = MinIdx;
      return (ulong)(1+MaxIdx);
   }
}

   VarRead * NEAR_CALL
varMemberGet(struct Call *call,VarRead *this,VarName Name,
             jsebool AllowDefaultPrototypes,jsebool SearchPrototypeChain)
{
   struct StructureMember *mptr;
   MemCountUInt hint;

   assert( VAR_HAS_DATA(this) );
   assert( this->varmem->data.vall.dataType==VObject );

   if ( NULL != (mptr = varobjFindMember(&(this->varmem->data.vobject.members),Name,&hint)) )
   {
      return mptr->var;
   }

   /* if was looking for prototype and didn't find then search here */
   if ( AllowDefaultPrototypes  &&  Name == GLOBAL_STRING(call,prototype_entry) )
   {
      assert( !HAS_PROTOTYPE_PROPERTY(&(this->varmem->data.vobject.members)) );
      return ( NULL == this->varmem->data.vobject.function
            || varSameObject(this,call->Global->FunctionPrototype) )
             ? call->Global->ObjectPrototype
             : call->Global->FunctionPrototype ;
   }

   if ( SearchPrototypeChain )
   {
      VarRead * ret;
      this = varMemberGet(call,this,GLOBAL_STRING(call,prototype_entry),AllowDefaultPrototypes,True);
      if( VAR_TYPE(this)==VObject )
      {
         /* Prevent looping prototype chains by using the been_here flag */
         if( !this->varmem->data.vobject.been_here )
         {
            this->varmem->data.vobject.been_here = True;
            ret = varMemberGet(call,this,Name,True,True);
            this->varmem->data.vobject.been_here = False;
            return ret;
         }
      }
   }

   return NULL;
}

   VarRead * NEAR_CALL
varGetNext(VarRead *this,struct Call *call,
           struct Var *Prev,VarName *vName)
{
   struct Varobj *obj;
   MemCountUInt x;

   assert( VAR_HAS_DATA(this) );
   assert( this->varmem->data.vall.dataType==VObject );
   assert( NULL != vName );

   UNUSED_PARAMETER(call);

   obj = &(this->varmem->data.vobject.members);

   if ( NULL != obj->members )
   {
      if( NULL == Prev )
      {
         *vName = obj->members[0].Name;
         return obj->members[0].var;
      }

      /* go to used-1 since the last one has no next! */
      for( x=0;x<obj->used-1;x++ )
      {
         if( obj->members[x].var==Prev )
         {
            *vName = obj->members[x+1].Name;
            return obj->members[x+1].var;
         }
      }
   }
   *vName = 0;
   return NULL;
}

   VarRead * NEAR_CALL
varBringMemberIntoExistance(VarRead *this,struct Call *call,
                            VarName Name,VarType DType,
                            jsebool ForceTypeConversion)
{
   struct Varobj *obj;
   struct StructureMember *mem;
   MemCountUInt offset;

   assert( isValidVarType(DType) );
   assert( VAR_HAS_DATA(this) );
   assert( this->varmem->data.vall.dataType==VObject );
   assert( Name!=NULL );

   obj = &(this->varmem->data.vobject.members);
   mem = varobjFindMember(obj,Name,&offset);
   
   if( NULL != mem )
   {
      VarRead *memvar = mem->var;
      if ( ForceTypeConversion )
      {
         if( memvar->varmem->data.vall.dataType!=DType )
         {
            varSetAttributes(memvar,0);
            varConvert(memvar,call,DType);
         }
      }
      return memvar;
   }

   mem = varobjCreateMemberWithHint(obj,call,Name,DType,offset);
   assert( mem!=NULL );
   return mem->var;
}

   void NEAR_CALL
varDeleteMember(VarRead *this,struct Call *call,VarName Name)
{
#  if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
      VarRead *property;
#  endif
   struct Varobj *obj;
   struct StructureMember *mem;
   MemCountUInt hint;

   assert( VAR_HAS_DATA(this) );
   assert( this->varmem->data.vall.dataType==VObject );
   assert( Name!=NULL );

   obj = &(this->varmem->data.vobject.members);

#  if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
      if( HAS_DELETE_PROPERTY(obj)
       && NULL!=(property=varGetMember(call,this,GLOBAL_STRING(call,delete_entry)))
       && VAR_IS_FUNCTION(property,call) )
      {
         /* Since the dynamic delete can itself delete the delete property,
          * we must add another reference to it so it doesn't go away on us
          * which can be bad.
          */
         varAddUser(property);
         varCallDynamicProperty(this,call,property,OFF_DELETE_PROP,
                                Name,NULL,NULL);
         VAR_REMOVE_USER(property,call);
      }
      else
#  endif
      {
         if ( NULL != (mem = varobjFindMember(obj,Name,&hint)) )
            varobjRemoveMember(obj,call,mem);
      }
}

   void NEAR_CALL
varSetArrayLength(VarRead *this,struct Call *call,
                  JSE_POINTER_SINDEX MinIndex,
                  JSE_POINTER_UINDEX Length)
{
   assert( VAR_HAS_DATA(this) );
   if( VAR_ARRAY_POINTER(this) )
   {
      varmemValidateIndex(this->varmem,this->offset_or_createType+MinIndex,
                          Length-MinIndex,True,True);
   }
   else
   {
      struct Varobj *obj;
      MemCountUInt x;
      jsebool MaxFound = ( 0 < (JSE_POINTER_SINDEX)Length ) ? False : True ;
      JSE_POINTER_UINDEX maxIndex = Length - 1;

      assert( this->varmem->data.vall.dataType==VObject );

      obj = &(this->varmem->data.vobject.members);
      for( x=0;x<obj->used;x++ )
      {
         VarName entry = obj->members[x].Name;
         if( IsNumericStringTableEntry(call,entry) )
         {
            JSE_POINTER_SINDEX value = GetNumericStringTableEntry(call,entry);
            /* we need the strange looking double compare to make sure all
             * negative entries aren't deleted
             */
            if( value<MinIndex ||
                (value>=0 && ((JSE_POINTER_UINDEX)value)>=Length) )
            {
               varobjRemoveMember(obj,call,(struct StructureMember *)
                                  (obj->members+x));
               /* decrement 'x' to do this slot over again since its former
                * occupant is gone, and the next entry has slid into its place
                */
               x--;
            }
            else if ( (JSE_POINTER_UINDEX)value == maxIndex  &&  0 <= value )
            {
               MaxFound = True;   
            }
         }
      }
      if ( !MaxFound )
      {
         /* "grow" the array to this new size */
         varCreateMember(this,call,EnterNumberIntoStringTable(maxIndex),VUndefined);
      }
   }
}

   void NEAR_CALL
varSetFunctionPtr(VarRead *this,struct Call *call,struct Function *func)
{
   assert( VAR_HAS_DATA(this) );
   assert( this->varmem->data.vall.dataType==VObject );

   this->varmem->data.vobject.function = func;

   /* if functions were to explicitly inherit from the default Function then
    * this would take up a lot of memory. But because the implicit _prototype
    * for a function object is Function.prototype we only need to set here
    * what may be different.
    */
   if ( 0 != functionParamCount(func) )
   {
      VarRead * length = varCreateMember(this,call,GLOBAL_STRING(call,length_entry),VNumber);
      varPutNumberFix(length,call,functionParamCount(func),VNumber);
      varSetAttributes(length,jseDontDelete | jseDontEnum | jseReadOnly );
   }
}

/* The dynamic object functions */

#if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)

/*
 * This is it, the dynamic property caller - all of the dynamic property
 * functions do some setup work then go through this call-> It returns a
 * boolean indicating if some return variable is waiting on the stack.
 */
   void NEAR_CALL
varCallDynamicProperty(VarRead *this,struct Call *call,
                       VarRead *property,uword16 off_flag,
                       VarName PropertyName,VarRead *Parameter2,
                       VarRead ** ResultVar)
{
   VarRead *namevar;
   const jsechar *nameval;
   uint depth = 0;
   struct Function *func;
   uint saveflags;
   VarRead *proplock;
   Var *tmp;
   struct secodeStack *thestack;

   assert( VAR_HAS_DATA(this) );
   assert( this->varmem->data.vall.dataType==VObject );

   thestack = call->Global->thestack;
   if( PropertyName!=NULL )
   {
      stringLengthType PropertyNameLen;
      depth = 1;
      namevar = constructVarRead(call,VString);
      nameval = GetStringTableEntry(call,PropertyName,&PropertyNameLen);
      varPutStringLen(namevar,call,nameval,PropertyNameLen);
      SECODE_STACK_PUSH(thestack,namevar);
   }
   
   if ( NULL != Parameter2 )
   {
      SECODE_STACK_PUSH(thestack,Parameter2);
      depth++;
   }

   func = varGetFunction(property,call);
   assert( func!=NULL );

   /* pretend we don't have this dynamic property for now. */
   saveflags = this->varmem->data.vobject.members.flags;
   this->varmem->data.vobject.members.flags |= off_flag;

   proplock = CONSTRUCT_VALUE_LOCK(call,property);

   functionFullCall(func,call,proplock,depth,this);

   tmp = SECODE_STACK_POP(thestack);
   if( ResultVar!=NULL )
      *ResultVar = GET_READABLE_VAR(tmp,call);
   VAR_REMOVE_USER(tmp,call);

   VAR_REMOVE_USER(proplock,call);

   if( (saveflags & off_flag)==0 )
      this->varmem->data.vobject.members.flags &= (uword16)(~off_flag);
}


   jsebool NEAR_CALL
_varHasProperty(VarRead *this,struct Call *call,VarName propname)
{
   VarRead *property;

   assert( VAR_HAS_DATA(this) );
   assert( this->varmem->data.vall.dataType==VObject );

#  if defined(JSE_INLINES) && (0!=JSE_INLINES)
   assert( HAS_HAS_PROPERTY(&(this->varmem->data.vobject.members)) );
#endif

   if( 
#      if !defined(JSE_INLINES) || (JSE_INLINES==0)
       HAS_HAS_PROPERTY(&(this->varmem->data.vobject.members)) &&
#      endif
       NULL!=(property =varGetMember(call,this,GLOBAL_STRING(call,hasproperty_entry))) &&
       VAR_IS_FUNCTION(property,call) )
   {
      VarRead *value;
      jsebool ret;

      varCallDynamicProperty(this,call,property,OFF_HAS_PROP,
                             propname,NULL,&value);
      ret = varGetBoolean(value);
      VAR_REMOVE_USER(value,call);

      return ret;
   }
   else
   {
      return varGetMember(call,this,propname)!=NULL;
   }
}


/* The variable is the
 * return value which you must destroy when done.
 */
   VarRead * NEAR_CALL
varCallConstructor(VarRead *this,struct Call *call,VarRead *SourceVar)
{
   VarRead *property;
   VarRead *new_var;
   VarRead *prototype;
   Var *tmp;
   VarRead *it;
   struct secodeStack *thestack;

   assert( VAR_HAS_DATA(this) );
   assert( this->varmem->data.vall.dataType==VObject );

   property = varGetDirectMember(call,this,GLOBAL_STRING(call,constructor_entry),False);
   if( property==NULL || !VAR_IS_FUNCTION(property,call) )
   {
      /* return blank object so we don't crash */
      return constructVarRead(call,VObject);
   }

   thestack = call->Global->thestack;
   if( varIsLvalue(SourceVar) )
   {
      varAddUser(SourceVar);
   }
   else
   {
      VarRead *cpy = SourceVar;
      SourceVar = constructVarRead(call,VAR_TYPE(SourceVar));
      varAssign(SourceVar,call,cpy);
   }
   SECODE_STACK_PUSH(thestack,SourceVar);

   new_var = constructVarRead(call,VObject);

   /* Copy the prototype from the constructor unless it is already
    * Function.prototype (which is the default, so keep it
    */
   prototype = varGetMember(call,this,GLOBAL_STRING(call,orig_prototype_entry));
   if( NULL != prototype
    && prototype != call->Global->FunctionPrototype )
   {
      VarRead *ourprop = varCreateMember(new_var,call,
                                         GLOBAL_STRING(call,prototype_entry),
                                         VUndefined);
      varAssign(ourprop,call,prototype);
#     if ( 0 != JSE_DYNAMIC_OBJ_INHERIT )
         /* inherit any dynamic properties of the superclass */
         assert( VObject == VAR_TYPE(new_var) );
         assert( VObject == VAR_TYPE(ourprop) );
         new_var->varmem->data.vobject.members.flags
         |= (ourprop->varmem->data.vobject.members.flags & (uword16)HAS_ALL_PROP);
#     endif
      varSetAttributes(ourprop,jseDontEnum);
   }

   functionFullCall(varGetFunction(property,call),call,property,1,new_var);
   tmp = SECODE_STACK_POP(thestack);
   it = GET_READABLE_VAR(tmp,call);
   VAR_REMOVE_USER(tmp,call);
   if( VAR_TYPE(it)==VObject )
   {
      VAR_REMOVE_USER(new_var,call);
      new_var = CONSTRUCT_VALUE_LOCK(call,it);
   }

   VAR_REMOVE_USER(it,call);

   return new_var;
}


   jsebool NEAR_CALL
varCanPut(VarRead *this,struct Call *call,VarName name)
{
   VarRead *property;
   jsebool ret;
   VarRead *value;

   assert( VAR_HAS_DATA(this) );

   /* if doesn't have a function as a canput property, then can put. */
   if( !HAS_CANPUT_PROPERTY(&(this->varmem->data.vobject.members)) )
      return True;

   property = varGetMember(call,this,GLOBAL_STRING(call,canput_entry));
   if( property==NULL || !VAR_IS_FUNCTION(property,call) ) return True;

   varCallDynamicProperty(this,call,property,OFF_CANPUT_PROP,name,NULL,&value);
   ret = ( VUndefined == VAR_TYPE(value) )
       ? True : varGetBoolean(value) ;
   VAR_REMOVE_USER(value,call);

   return ret;
}


/* Must provide a variable that will be destroyed later */
   VarRead * NEAR_CALL
varDefaultValue(VarRead *this,struct Call *call,jsebool hintstring)
{
   int start,end,step;
   VarRead *ret;
   int count;

   assert( VObject == VAR_TYPE(this) );

   /* check to see if its one of the ECMA ones. This means a BIG
    * performance boost to not have to call the function
    */
   /*VarRead *classvar = varGetDirectMember(this,call->Global->class_entry);*/
   /*if( classvar!=NULL && VAR_TYPE(classvar)==VString )*/
   /*{*/
      /*char *cl = (char *)varGetData(classvar,0);*/
      /*if( strcmp(cl,"String")==0 || strcmp(cl,"Boolean")==0 ||*/
      /*    strcmp(cl,"Number")==0 )*/
      {
         VarRead *valuevar =
            varGetDirectMember(call,this,GLOBAL_STRING(call,value_entry),False);
         if( NULL != valuevar)
         {
            /* only valid if this is same type as return wanted */
            if ( VAR_TYPE(valuevar) == (hintstring ? VString : VNumber) )
            {
               varAddUser(valuevar);
               return valuevar;
            }
         }
      }
   /*}*/

   /* next see if it has been redefined */

   {
   /* if doesn't have a function as a canput property, then can put. */
      VarRead *property;
      VarRead *value;


      property = varGetDirectMember(call,this,GLOBAL_STRING(call,defaultvalue_entry),False);
      if( property!=NULL && VAR_IS_FUNCTION(property,call) )
      {
         /* call the default value routine */
         VarRead *hint = constructVarRead(call,(VarType)(hintstring?VString:VNumber));
         varCallDynamicProperty(this,call,property,0,NULL,hint,&value);
         assert( value!=NULL );
         return value;
      }
   }

   /* if not, do the standard thing */
   
   if( hintstring )
   {
      start = 0; end = 2; step = 1;
   }
   else
   {
      start = 1; end = -1; step = -1;
   }

   ret = NULL;
   for( count = start;ret==NULL && count!=end;count += step )
   {
      VarName funcname = (count)
                       ? GLOBAL_STRING(call,valueof_entry)
                       : GLOBAL_STRING(call,tostring_entry);

      VarRead *tostring = varGetMember(call,this,funcname);
      if( tostring!=NULL )
      {
         struct Function *func = varGetFunction(tostring,call);
         if( func!=NULL )
         {
            Var *tmp;

            functionFullCall(func,call,tostring,0,this);
            tmp = SECODE_STACK_POP(call->Global->thestack);
            ret = GET_READABLE_VAR(tmp,call);
            VAR_REMOVE_USER(tmp,call);
         }
      }
   }
   if( ret==NULL )
   {
#     if !defined(JSE_SHORT_RESOURCE) || (0==JSE_SHORT_RESOURCE)
         jsechar VarName[60];
         if ( FindNames(call,this,VarName+1,sizeof(VarName)/sizeof(VarName[0])-5) )
         {  
            VarName[0] = '(';
            strcat_jsechar(VarName,UNISTR(") "));
         }
         else
            VarName[0] = '\0';
         callQuit(call,textcoreNO_DEFAULT_VALUE,VarName);
#     else
         callQuit(call,textcoreNO_DEFAULT_VALUE,"");
#     endif
      ret = constructVarRead(call,VUndefined);
   }
   return ret;
}
# endif
