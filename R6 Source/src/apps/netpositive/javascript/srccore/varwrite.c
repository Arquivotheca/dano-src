/* varwrite.cpp  Access to variables of all kinds.
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

/*
 * 12/17/97 - complete rewrite of var started
 * 12/18/97 - this file started
 * 02/16/98 - transated core to C, Rich Robinson
 */


#include "srccore.h"


   jsebool NEAR_CALL
varDoAssign(VarWrite *this,struct Call *call,
            VarRead *Original,jsebool copy)
{
   VarType oType;

   assert( VAR_HAS_DATA(Original) );
   assert( VAR_HAS_DATA(this) );
   assert( Original->userCount!=0 );

   /* if it is read only, we just ignore the assign, but it didn't fail */
   if( varGetAttributes(this) & jseReadOnly ) return True;

   /* easy as pie; if this already is itself so don't need to assign to
      itself; ha ha */
   if( (VarRead *)this == Original ) return True;

   oType = VAR_TYPE(Original);
   if( varIsDeref(this) )
   {
      if( !varIsDeref(Original) && oType!=VNumber )
      {
         callError(call,textcoreCANNOT_ASSIGN_NONNUMERIC_TO_ARRAY_ELEMENT);
         return False;
      }
      varPutNumberFix(this,call,varGetNumber(Original),VNumber);
   }
   else if( VObject==oType
#         if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
         || ( copy==False && CBehavior(call) && VAR_ARRAY_POINTER(Original)
          &&  !varIsDeref(Original) )
#         endif
          )
   {
      /* copy by reference */
      varReplaceVarmem(this,call,Original->varmem);
      this->offset_or_createType = Original->offset_or_createType;
   }
   else
   {
      /* make new variable for left side, copy everything */
      varConvert(this,call,(VarType)(varIsDeref(Original)?VNumber:oType));
      if( oType==VNumber || oType==VBoolean )
      {
         varPutNumber(this,varGetNumber(Original));
      }
      else if( VAR_ARRAY_POINTER(Original) )
      {
         /* copy the data from 0 to size */
#        if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)   
         if( oType==VBuffer )
         {
            varPutBuffer(this,call,varGetData(Original,0),
                         varGetArrayLength(Original,call,NULL));
         }
         else
#        endif
         {
            varPutStringLen(this,call,(jsechar _HUGE_ *)varGetData(Original,0),
                            varGetArrayLength(Original,call,NULL));
         }
      }
      else
      {
         /* do nothing for these types */
         assert( oType == VUndefined  ||  oType == VNull );
      }
   }
   return True;
}


   void NEAR_CALL
varPutString(VarWrite *this,struct Call *call,const jsechar _HUGE_ *data)
{
   JSE_POINTER_UINDEX Count;

   assert( VAR_HAS_DATA(this) );

   DYNAMIC_PUT_FIX(this,call,VString);

#  if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
      /* assume not doing unicode with huge memory, because that's
         not written yet */
#     if defined(HUGE_MEMORY)
#        error Auto String Length not written yet for HUGE_MEMORY with Unicode
#     else
         Count = strlen_jsechar(data);
#     endif
#  elif defined(HUGE_MEMORY)
   {
      /* use HugeMemChr to find the first null byte */
      const void _HUGE_ * FirstNull =
         HugeMemChr((jsechar _HUGE_ *)data,'\0',MAX_ULONG);
      Count = (ulong)(((jsechar _HUGE_ *)FirstNull) - ((jsechar _HUGE_ *)data));
   }
#  else
      Count = strlen_jsechar(data);
#  endif
   varPutStringLen(this,call,data,Count);
}


   void NEAR_CALL
varPutNumber(VarWrite *this,jsenumber f)
{
   assert( VAR_HAS_DATA(this) );

   assert( this->varmem->data.vall.dataType==VBoolean ||
           this->varmem->data.vall.dataType==VNumber || varIsDeref(this) );
   if( varIsDeref(this) )
   {
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)   
      if( VBuffer == this->varmem->data.vall.dataType )
      {
         ubyte *text = (ubyte *)
            HugePtrAddition(this->varmem->data.vpointer.memory,
                            (this->offset_or_createType-this->varmem->
                             data.vpointer.zeroIndex)*
                            sizeof(ubyte));
         text[0] = (ubyte)f;
      }
      else
#     endif      
      {
         jsechar *text = (jsechar *)
            HugePtrAddition(this->varmem->data.vpointer.memory,
                            (this->offset_or_createType-this->varmem->
                             data.vpointer.zeroIndex)*
                            sizeof(jsechar));
         text[0] = (jsechar)f;
      }
   }
   else
   {
#ifdef __FreeBSD__
      /* this line crashes w/ Infinity on FreeBSD without using this
       * weird stuff (I think to get around a compiler bug?)
       */
      *(&this->varmem->data.vnumber.value) = *(&f);
#else
      this->varmem->data.vnumber.value = f;
#endif
   };
}

   void NEAR_CALL
varPutNumberFix(VarWrite *this,struct Call *call,jsenumber f,sword8 varType)
{
   assert( VAR_HAS_DATA(this) );
   assert( VNumber == varType  ||  VBoolean == varType );
   DYNAMIC_PUT_FIX(this,call,varType);
   varPutNumber(this,f);
}

   void NEAR_CALL
varPutData(VarWrite *this,struct Call *call,
           const void _HUGE_ *data,JSE_POINTER_UINDEX Count,
           VarType vType)
{
   assert( VAR_HAS_DATA(this) );
   assert( isValidVarType(vType) );

   DYNAMIC_PUT_FIX(this,call,vType);

   assert( VAR_ARRAY_POINTER(this) );
   assert( !varIsDeref(this) );

   /* just insert for buffers, but if strings then delete from end
    * Rich: I've moved the delete from end in case the source string and the
    * dest string are identical. We do it after we've moved the data
    */
   varmemValidateIndex(this->varmem,this->offset_or_createType,Count,False,False);

   HugeMemMove(
               HugePtrAddition(this->varmem->data.vpointer.memory,
                               (this->offset_or_createType-this->varmem->
                                data.vpointer.zeroIndex)*
                               varmemArrayElementSize(this->varmem)),
               data,Count*varmemArrayElementSize(this->varmem));

   varmemValidateIndex(this->varmem,this->offset_or_createType,Count,False,VString==vType);
}


   void NEAR_CALL
varConvert(VarWrite *this,struct Call *call,VarType newType)
{
   assert( VAR_HAS_DATA(this) );
   assert( isValidVarType(newType) );

   /* if it is read only, we just ignore the convert */
   if( varGetAttributes(this) & jseReadOnly ) return;

   if( newType!=this->varmem->data.vall.dataType || varIsDeref(this) )
   {
      /* lose old varmem and make a new variable type */
     varReplaceVarmem(this,call,varmemNew(call,newType));
     this->deref = False;
   }
}
