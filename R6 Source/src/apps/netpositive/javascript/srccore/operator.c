/* operator.c
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

   static jsenumber NEAR_CALL
SpecialMathOnNonFiniteNumbers(struct Call *call,jsenumber lnum,
                              jsenumber rnum,codeval operation)
   /* follow special rules for math on non-finite numbers */
{
   jsenumber ret = jseNaN;
   assert( !jseIsFinite(lnum) || !jseIsFinite(rnum) );

   /* all operators return NaN is either is NaN */
   if ( jseIsNaN(lnum) || jseIsNaN(rnum) )
   {
      if( (jseOptWarnBadMath & call->Global->ExternalLinkParms.options) )
         callError(call,textcoreIS_NAN);
   }
   else
   {
      jsebool lneg, rneg, linf, rinf;
      DoMathAgain:
      /* one or the other of our operators is Infinity or NegInfinty */
      lneg = jseIsNegative(lnum);
      rneg = jseIsNegative(rnum);
      linf = jseIsInfOrNegInf(lnum);
      rinf = jseIsInfOrNegInf(rnum);

      assert( linf || rinf );
      assert( lneg == True  ||  lneg == False );
      assert( rneg == True  ||  rneg == False );
      assert( linf == True  ||  linf == False );
      assert( rinf == True  ||  rinf == False );

      switch( operation )
      {
         case opAdd:
         case opAssignAdd:
         case opSubtract:
            if ( opSubtract == operation )
            {
               rnum = -rnum;
               operation = opAdd;
               goto DoMathAgain;
            }
            if ( !linf )
            {
               /* eaiest to assume loperator is inf; try again */
               goto SwitchNumbersAndTryAgain;
            }
            assert( linf  &&  opSubtract != operation );
            if ( rinf )
            {
               /* left and right are both inf; if same sign inf else NaN */
               ret = ( lneg == rneg ) ? lnum : jseNaN ;
            }
            else
            {
               /* lnum is inf, and right is finite, so return lnum */
               ret = lnum;
            }
            break;
         case opDivide:
         case opAssignDivide:
            /* anyinf / anyinf == NaN */
            if ( linf )
            {
               /* numerator is infinite */
               if ( rinf )
               {
                  /* inf / inf is NaN, which is the default */
               }
               else
               {
                  /* inf / anything = inf */
                  ret = ( lneg == rneg ) ? jseInfinity : jseNegInfinity ;
               }
            }
            else 
            {
               /* numerator is not inf; denominator is inf */
               assert( rinf );
               ret = (rneg != lneg )? jseNegZero : jseZero ;
            }
            break;
         case opMultiply:
         case opAssignMultiply:
            if ( !linf )
            {
               /* easiest to assume loperator is inf; try again */
               goto SwitchNumbersAndTryAgain;
            }
            if ( rinf || (0!=rnum) )
            {
               /* inf * inf == inf */
               ret = ( lneg == rneg ) ? jseInfinity : jseNegInfinity ;
            }
            else
            {
               assert( 0 == rnum );
               /* inf * 0 == NaN */
            }
            break;
         case opModulo:
         case opAssignModulo:
            if ( linf )
            {
               /* inf % anything == NaN */
            }
            else
            {
               assert( rinf );
               /* anything % inf == anything */
               ret = lnum;
            }
            break;

#        ifndef NDEBUG
         default: assert( False ); break;
#        endif
      }
   }
   return ret;
SwitchNumbersAndTryAgain:
   {
      /* want to swith lnum and rnum */
      jsenumber temp = lnum;
      lnum = rnum;
      rnum = temp;
      goto DoMathAgain;
   }
}



   static jsenumber NEAR_CALL
GetNumberForOperator(struct Call *call,VarRead *OperatorVar)
{
   jsenumber ret;
   /* For the other types, GetNumber() will get the correct value, so
    * we don't need to waste the time doing the conversion
    */
   VarType oType = VAR_TYPE(OperatorVar);
   if( (VString==oType || VObject==oType)
#   if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
    && !CBehavior(call)
#   endif
     )
   {
      VarRead * tmp;
      
      assert(VString == VAR_TYPE(OperatorVar) || VObject == VAR_TYPE(OperatorVar));
      tmp = convert_var(call,OperatorVar,jseToNumber);
      ret = varGetNumber(tmp);
      
      VAR_REMOVE_USER(tmp,call);
   }
   else
      ret = varGetNumber(OperatorVar);

   if( (jseOptWarnBadMath & call->Global->ExternalLinkParms.options)
    && jseIsNaN(ret) )
   {
      callError(call,textcoreIS_NAN);
   }
   
   return ret;
}

   VarRead * JSE_CFUNC NEAR_CALL
do_op_crement(struct Call *call,VarRead *lrvar,
              VarRead *foo,codeval code)
{
   jsenumber Delta = (opPostIncrement==code || opPreIncrement==code) ? 1 : -1 ;
   VarRead *newVar;
   VarType type = VAR_TYPE(lrvar);

   UNUSED_PARAMETER(foo);
  
   assert( opPostIncrement==code || opPostDecrement==code ||
           opPreIncrement==code || opPreDecrement==code );

#if (defined(JSE_C_EXTENSIONS) && (0 !=JSE_C_EXTENSIONS)) \
 || (defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER))
   if (  
#      if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
         (CBehavior(call) && VString == type)
#        if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
           ||
#        endif
#      endif
#      if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
         VBuffer == type
#      endif
      )
   {  /* Needs to be C-function like */
      VarRead *addvar = constructVarRead(call,VNumber);
      varPutNumber(addvar,Delta);
      newVar = do_add_or_subtract(call,lrvar,addvar,opAdd);
      VAR_REMOVE_USER(addvar,call);   
   }
   else
#  endif
   {
      newVar = constructVarRead(call,VNumber);
      varPutNumber(newVar,GetNumberForOperator(call,lrvar)+Delta);
   }

   return newVar;
}


   VarRead * JSE_CFUNC NEAR_CALL
do_add_or_subtract(struct Call *call,VarRead *rlvar,
                   VarRead *rrvar,codeval direction)
{
   VarRead *newVar;
   VarType tx = VAR_TYPE(rlvar);
   VarType ty = VAR_TYPE(rrvar);
   jsenumber lnum, rnum;
   
   assert(opAdd==direction || opAssignAdd==direction || opSubtract==direction);

   /* This is the most common occurrence, so handle it specially here */
   if ( VNumber == tx && VNumber == ty )
   {
      /* We know both of these are VNumbers, so cheat a little */
      jsenumber result;
      lnum = rlvar->varmem->data.vnumber.value;
      rnum = rrvar->varmem->data.vnumber.value;

      QuickAddTwoNumbers:
      if ( jseIsFinite(lnum) && jseIsFinite(rnum) )
      {
         /* easiest case; two regular numbers */
         result = (opSubtract == direction) ? (lnum - rnum) : (lnum + rnum) ;
      }
      else
      {
         result = SpecialMathOnNonFiniteNumbers(call,lnum,rnum,direction);
      }
      varPutNumber(newVar=constructVarRead(call,VNumber),
                result);
      return newVar;   
   }

   if( tx ==VObject )
   {
      rlvar = convert_var(call,rlvar,jseToPrimitive);
      tx = VAR_TYPE(rlvar);
   }
   else
      varAddUser(rlvar);
   if( ty ==VObject )
   {
      rrvar = convert_var(call,rrvar,jseToPrimitive);
      ty = VAR_TYPE(rrvar);
   }
   else
      varAddUser(rrvar);

   assert( VAR_TYPE(rlvar)!=VObject  &&  tx==VAR_TYPE(rlvar) );
   assert( VAR_TYPE(rrvar)!=VObject  &&  ty==VAR_TYPE(rrvar) );

   if ( 
#       if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
        !CBehavior(call)  &&
#       endif
        (VString==tx || VString==ty) )
   {
      /* pure javascript string behavior */
      if ( opSubtract == direction ) {
         /* subtraction will treat both sides as numbers */
         newVar = constructVarRead(call,VNumber);
         varPutNumber(newVar,GetNumberForOperator(call,rlvar) -
                   GetNumberForOperator(call,rrvar) );
      } else {
         /* addition; concatenate strings together */
         VarRead *temp_lvar = convert_var(call,rlvar,jseToString);
         VarRead *temp_rvar = convert_var(call,rrvar,jseToString);
         newVar = constructVarRead(call,VString);
         ConcatenateStrings(call,newVar,temp_lvar,temp_rvar);
         VAR_REMOVE_USER(temp_rvar,call);
         VAR_REMOVE_USER(temp_lvar,call);
      }
   } else if ( (VAR_ARRAY_POINTER(rlvar) && !varIsDeref(rlvar))
            || (VAR_ARRAY_POINTER(rrvar) && !varIsDeref(rrvar)) )
   {
      /* Not necessarily c-behavior - buffers always act this way
       * array can only add to an integer
       */
      VarRead *vars[2];
      int which = ( VAR_ARRAY_POINTER(rlvar) || VObject == tx )
         ? 0 : 1 ;
      vars[0] = rlvar, vars[1] = rrvar;
      #define other which ^ 1
      if ( VAR_ARRAY_POINTER(vars[other]) || VObject == VAR_TYPE(vars[other]) )
      {
         callError(call,textcoreCANNOT_ADD_ARRAYS);
         newVar = NULL;
      } else {
         JSE_POINTER_SINDEX offset = varGetLong(vars[other]);
         if ( opSubtract == direction )
            offset = -offset;
         newVar = CONSTRUCT_SIBLING(call,vars[which],offset,False);
      }
   } else {
      /* Not an array; Must add and convert to the largest element type. */
      lnum = varGetNumber(rlvar);
      rnum = varGetNumber(rrvar);
      VAR_REMOVE_USER(rrvar,call);
      VAR_REMOVE_USER(rlvar,call);
      goto QuickAddTwoNumbers;
   }

   VAR_REMOVE_USER(rrvar,call);
   VAR_REMOVE_USER(rlvar,call);

   return newVar;
}


   VarRead * JSE_CFUNC NEAR_CALL
do_op_subtract(struct Call *call,VarRead *lrvar, VarRead *rrvar,codeval code)
{
   assert( isValidCodeval(code) );
   UNUSED_PARAMETER(code);
   /* check for special case of subtracting arrays, which is only possible
    * if both are the same dimension and point to the same varmem
    * (Brent asks? does this really check that they're the same varmem?)
    */
#  if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
   if ( CBehavior(call) &&
        (VAR_ARRAY_POINTER(lrvar) && VAR_ARRAY_POINTER(rrvar)) )
   {
      JSE_POINTER_SINDEX ArrayDifference;
      VarRead *var = constructVarRead(call,VNumber);

      ArrayIndexDifference(call,lrvar,rrvar,&ArrayDifference);
      varPutLong(var,ArrayDifference);
      return var;
   }
#  endif
   return do_add_or_subtract(call,lrvar,rrvar,opSubtract);
}


   VarRead * JSE_CFUNC NEAR_CALL
do_op_bitNot(struct Call *call,VarRead *lrvar, VarRead *tmp,codeval code)
{
   slong val = (slong)GetNumberForOperator(call,lrvar);
   VarRead * newVar = constructVarRead(call,VNumber);

   assert( isValidCodeval(code) );
   UNUSED_PARAMETER(code);
   UNUSED_PARAMETER(tmp);
   varPutLong(newVar,~val);
   return newVar;
}


   VarRead * JSE_CFUNC NEAR_CALL
do_op_typeof(struct Call *call,VarRead *lrvar, VarRead *tmp,codeval code)
{
   /* secode "cheated" casting the var to be a readable var,
      which isn't necesarily so */
   const jsechar *type = textcorevtype_undefined;
   VarRead *newvar;

   assert( isValidCodeval(code) );
   UNUSED_PARAMETER(code);
   UNUSED_PARAMETER(tmp);
   if ( VAR_HAS_DATA(lrvar) || NULL!=(lrvar)->reference.parentObject )
   {
      VarRead *avar = GET_READABLE_VAR(lrvar,call);
      switch( VAR_TYPE(avar) )
      {
         case VNull:
            type = textcorevtype_object; break;
         case VBoolean:
            type = UNISTR("boolean"); break;
         case VNumber:
            type = UNISTR("number"); break;
         case VString:
            type = UNISTR("string"); break;
#        if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
         case VBuffer:
            type = UNISTR("buffer"); break;
#        endif
         case VObject:
            type = ( NULL == varGetFunction(avar,call) ) ?
               textcorevtype_object : textcoreFunctionKeyword ;
            break;
      }
      VAR_REMOVE_USER(avar,call);
   }
   newvar = constructVarRead(call,VString);
   varPutString(newvar,call,type);
   return newvar;
}


   VarRead * JSE_CFUNC NEAR_CALL
do_op_void(struct Call *call,VarRead *tmp1, VarRead *tmp2,codeval code)
{
   assert( isValidCodeval(code) );
   UNUSED_PARAMETER(code);
   UNUSED_PARAMETER(tmp1);
   UNUSED_PARAMETER(tmp2);
   return constructVarRead(call,VUndefined);
}


   VarRead * JSE_CFUNC NEAR_CALL
do_op_delete(struct Call * call,VarRead * lrvar, VarRead *tmp,codeval code)
{
   /* secode "cheated" casting the var to be a readable var,
    * which isn't necesarily so
    */
   Var *var = lrvar;
   VarRead *newVar;
   VarRead *parentObject;
   VarName memberName;
   VarRead *prop;
   
   assert( isValidCodeval(code) );
   UNUSED_PARAMETER(tmp);
   UNUSED_PARAMETER(code);

#  if defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS)
   {
      uint i;
      for( i = 0; i < call->localCacheCount; i++ )
      {
         if( var == call->Global->thestack->items[call->localCacheLocation+i*2+1] )
         {
            var = constructReference(call,call->VariableObject,call->Global->thestack->items[call->localCacheLocation+i*2]);
            break;
         }    
      
      }
   }
#  endif
   
   if( !VAR_HAS_REFERENCE(var) )
   {
      callQuit(call,textcoreBAD_DELETE_VAR);
      return NULL;
   }

   newVar = constructVarRead(call,VBoolean);
   /* it is 'True' if not an object */

   parentObject = var->reference.parentObject;
   if( NULL == parentObject )
   {
      /* delete from the global object */
      parentObject = call->session.GlobalVariable;
   }
   assert( NULL != parentObject );
   memberName = var->reference.memberName;
   assert( NULL != memberName );

   prop = varGetDirectMember(call,parentObject,memberName,False);
   if( prop!=NULL && (varGetAttributes(prop) & jseDontDelete)==0 )
   {
      varDeleteMember(parentObject,call,memberName);
      varPutBoolean(newVar,True);
   }
   else if ( NULL == var->reference.parentObject
          && !(jseOptReqVarKeyword & call->Global->ExternalLinkParms.options) )
   {
      /* non-existing variable is by default part of the global.  Assume it has safely
       * been deleted.  The ECMAScript documentation does not clearly state that this
       * is how hasproperty will behave, but NS and IE both act this way.
       */
      varPutBoolean(newVar,True);
   }
   else
   {
      /* default is already false */
      assert( !varGetBoolean(newVar) );
   }

#  if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
   {
      struct variableCacheEntry *vCacheStart, *vCacheEnd;

      /* Reset cache because we don't know what this deleted */
      if( call->Global->currentCacheSize != -1 )
      {
         vCacheEnd = (vCacheStart = call->Global->variableCache)
                   + call->Global->currentCacheSize;
         while ( vCacheEnd-- != vCacheStart )
         {
             VAR_REMOVE_USER(vCacheEnd->var,call);
         }
         call->Global->currentCacheSize = 0;
      }
   }
#  endif
   
   if( var != lrvar )
   {
      VAR_REMOVE_USER(var,call);
   }

   return newVar;
}


   VarRead * JSE_CFUNC NEAR_CALL
do_op_positive_or_negative(struct Call *call,VarRead *lrvar,
                           VarRead *tmp,codeval code)
{
   jsenumber val = GetNumberForOperator(call,lrvar);
   VarRead *ret = constructVarRead(call,VNumber);

   assert( opNegative == code || opPositive == code );
   UNUSED_PARAMETER(tmp);

   varPutNumber(ret,
             (opNegative == code  &&  !jseIsNaN(val)) ? -val : val );
   return ret;
}


   VarRead * JSE_CFUNC NEAR_CALL
do_op_assign(struct Call * call,
             VarRead * lrvar,VarRead *rrvar,codeval t)
{
   UNUSED_PARAMETER(call);
   UNUSED_PARAMETER(lrvar);
   UNUSED_PARAMETER(t);
   assert( opAssign == t || t==opAssignOnly );
   /* add an extra lock on the rrvar */
   varAddUser(rrvar);
   return rrvar;
}


   VarRead * JSE_CFUNC NEAR_CALL
do_op_boolNot(struct Call *call,VarRead *lrvar, VarRead *tmp,codeval code)
{
   VarRead * new_var = constructVarRead(call,VBoolean);
   UNUSED_PARAMETER(code);
   UNUSED_PARAMETER(tmp);
   assert( 0 == jseGetLong(call,new_var) );
   if ( !ToBoolean(call,lrvar) )
      varPutBoolean(new_var,True);
   return new_var;
}


   VarRead * JSE_CFUNC NEAR_CALL
do_twonumbers(struct Call *call,VarRead *lrvar,VarRead *rrvar,codeval code)
{
   jsenumber Result;
   jsenumber one = GetNumberForOperator(call,lrvar);
   jsenumber two = GetNumberForOperator(call,rrvar);
   VarRead * newVar;

   switch( code )
   {
      case opDivide:
      case opAssignDivide:
         if ( !jseIsFinite(one) || !jseIsFinite(two) )
         {
            Result = SpecialMathOnNonFiniteNumbers(call,one,two,code);
         }
         else if ( 0 == two )
         {
            if ( (jseOptWarnBadMath & call->Global->ExternalLinkParms.options) )
               callError(call,textcoreCANNOT_DIVIDE_BY_ZERO);
            if ( one == 0 )
               Result = jseNaN ;
            else
               Result = ( jseIsNegative(two) == jseIsNegative(one) )
                      ? jseInfinity : jseNegInfinity ;
         }
         else
         {
            Result = one / two;
         }
         break;
      case opMultiply:
      case opAssignMultiply:
         if ( !jseIsFinite(one) || !jseIsFinite(two) )
         {
            Result = SpecialMathOnNonFiniteNumbers(call,one,two,code);
         }
         else
         {
            Result = one * two;
         }
         break;
      case opModulo:
      case opAssignModulo:
         if ( !jseIsFinite(one) || !jseIsFinite(two) )
         {
            Result = SpecialMathOnNonFiniteNumbers(call,one,two,code);
         }
         else if ( 0 == two )
         {
            if ( (jseOptWarnBadMath & call->Global->ExternalLinkParms.options) )
               callError(call,textcoreCANNOT_DIVIDE_BY_ZERO);
            Result = jseNaN;
         }
         else
         {
            /* ecmascript % is more like C library fmod */
            slong div = (slong)(one / two) ;
            Result = one - (div * two);
         }
         break;
      case opBitOr:
      case opAssignBitOr:
         Result = (slong)(((ulong)one) | ((ulong)two));
         break;
      case opBitAnd:
      case opAssignBitAnd:
         Result = (slong)(((ulong)one) & ((ulong)two));
         break;
      case opBitXor:
      case opAssignBitXor:
         Result = (slong)(((ulong)one) ^ ((ulong)two));
         break;
      case opShiftLeft:
      case opAssignShiftLeft:
         Result = (slong)(((ulong)one) << ((uint)two));
         break;
      case opSignedShiftRight:
      case opAssignSignedShiftRight:
      case opUnsignedShiftRight:
      case opAssignUnsignedShiftRight:
      {  /* these shift algorithms might be a bit slower, but they will work on
          * all systems regardless of the way that systems >> operator works.
          */
         uint bits = (uint)two & 0x1f;
         /* construct a mask of 0x80000000 signed shifted right
          * bits times and or it with the value. We do this by
          * shifting right 1 32-bits times and oring in ones
          * each time, then notting the value.
          */
         ulong mask = 1;
         uint maskbits = 32 - bits;
         slong sval = (slong)one;
   
         while( (--maskbits)>0 )
         {
            mask = (mask<<1) | 1;
         }
         if ( (sval < 0)  &&
              (opSignedShiftRight==code || opAssignSignedShiftRight==code) )
         {
            /* signed shifting */
            Result = (jsenumber)( (sval >> bits) | (slong)(~mask) ) ;
         }
         else
         {
            /* unsigned shifting */
            Result = ((ulong)sval >> bits) & mask ;
         }
      }  break;
#     ifndef NDEBUG
      default: assert( False ); break;
#     endif
   }
   newVar = constructVarRead(call,VNumber);
   varPutNumber(newVar,Result);
   return newVar;
}


   void NEAR_CALL
do_op_structureMember(struct Call *call,struct secodeStack *thestack,
                      jsebool ByName,VarName MemberName,
                      JSE_POINTER_SINDEX Index)
{
   /* when building reference, the base must be the value locked - it
    * doesn't lock the base for speed */
   VarRead *lockedVar;
   Var *origObjVar = SECODE_STACK_POP(thestack);
   VarRead *objVar;
   Var *var;
   VarType oType;

   /* to allow unresolved member variables to automatically become objects
    * (or array-like objects) on their first call, and if not in a dynamic
    * object, then convert immediately to object here */
   if ( VAR_HAS_REFERENCE(origObjVar)  &&  NULL == origObjVar->reference.parentObject )
   {
      VarWrite *orig = getWriteableVar(origObjVar,call);
      varConvert(orig,call,VObject);
      VAR_REMOVE_USER(origObjVar,call);
      origObjVar = orig;
   }

   objVar = GET_READABLE_VAR(origObjVar,call);
   assert( VAR_HAS_DATA(objVar) );
   oType = VAR_TYPE(objVar);

   /* if objvar does not yet have a type, let's just make it an object
    * You cannot do this on the readable version because that is the actual
    */
   if ( VUndefined == oType || VNull == oType )
   {
      /* The reason is that the readable var may not be the same
       * as the original var (which is likely to itself be a reference)
       * and we need to convert the original thing and read it again.
       * If that doesn't work (i.e. it is not an object), we are probably
       * doing a dynamic thingee. In that case, we convert the result
       * to a temp object and use it, which is what happens when we
       * fall thru to the next section
       */
      VarWrite *orig = getWriteableVar(origObjVar,call);
      varConvert(orig,call,VObject);
      VAR_REMOVE_USER(orig,call);
      VAR_REMOVE_USER(objVar,call);
      objVar = GET_READABLE_VAR(origObjVar,call);
      oType = VAR_TYPE(objVar);
   }

   /* WE MUST ALWAYS GET HERE! we popped a variable from the stack and
    * must get rid of it. Don't return above!
    */
   VAR_REMOVE_USER(origObjVar,call);

   /* Convert to object if not one already, except for buffer types using indices, or
    * string types using indices if in a C-Function
    */
   if ( VObject != oType )
   {
#     if (defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)) \
      || (defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS))
         if ( !ByName
           && (
#             if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
                VBuffer == oType 
#                if  defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                   ||
#                endif
#             endif
#             if  defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                (VString == oType  &&  CBehavior(call))
#             endif
              ) )
         {
            /* numeric index on buffer or c-style string just makes a sibling */
            lockedVar = CONSTRUCT_VALUE_LOCK(call,objVar);
            var = CONSTRUCT_SIBLING(call,lockedVar,Index,True);
            goto PushResults;
         }
         else
#     endif
         {
            /* item is not currently an object type.  Convert it into one. */
            VarRead *tmp = objVar;
            objVar = convert_var(call,objVar,jseToObject);
            assert( VObject == VAR_TYPE(objVar) );
            VAR_REMOVE_USER(tmp,call);
         }
   }
   assert( VObject == VAR_TYPE(objVar) );
   
   lockedVar = CONSTRUCT_VALUE_LOCK(call,objVar);
   /* get structure member and put it on the stack to replace structure variable there */
   if ( ByName ) {
      var = constructReference(call,lockedVar,MemberName);
   }
   else
   {
      var = constructReference(call,lockedVar,
        EnterNumberIntoStringTable(Index+varOffsetFromZero(lockedVar)));
   }
#if (defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)) \
 || (defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS))
   PushResults:
#endif
   VAR_REMOVE_USER(lockedVar,call);
   assert( NULL != var );
   SECODE_STACK_PUSH(thestack,var);

   VAR_REMOVE_USER(objVar,call);
}



   void NEAR_CALL
do_op_arrayMember(struct Call *call,struct secodeStack *thestack)
   /* converts to number, if needed, then gets valid number */
{
   Var *ivar = SECODE_STACK_POP(thestack);
   VarRead *indexVar = GET_READABLE_VAR(ivar,call);
   VarType iType = VAR_TYPE(indexVar);
   
   VAR_REMOVE_USER(ivar,call);

   if( iType==VNumber )
   {
      do_op_structureMember(call,thestack,False,0,varGetLong(indexVar));
   }
   else
   {
      VarName name;
      if( iType!=VString )
      {
         VarRead *tmp = convert_var(call,indexVar,jseToString);
         VAR_REMOVE_USER(indexVar,call);
         indexVar = tmp;
      }
      name = EnterIntoStringTable(call,(const jsechar*)varGetData(indexVar,0),
                                  (stringLengthType)varGetArrayLength(indexVar,call,NULL));
      do_op_structureMember(call,thestack,True,name,0);
      RemoveFromStringTable(call,name);
   }
   VAR_REMOVE_USER(indexVar,call);
}

   VarRead * JSE_CFUNC NEAR_CALL
do_op_compare(struct Call *call,VarRead *lrvar,
              VarRead *rrvar,codeval code)
{
   jsebool result;
   VarRead *ret;

   /* ANSI C dictates that this is true, but check it just in case */
   assert( !(-1) == 0 );
 
   /* ecmascript has two different compares, one for equality and one for
    * relational possition (i.e. is it greater than for example)
    */
      
   switch( code )
   {
      case opLess:
         result = (1 == varCompareLess(call,lrvar,rrvar));         
         break;
      case opLessEqual:
         result = !varCompareLess(call,rrvar,lrvar);         
         break;
      case opGreater:
         result = (1 == varCompareLess(call,rrvar,lrvar));
         break;
      case opGreaterEqual:
         result = !varCompareLess(call,lrvar,rrvar);
         break;
      case opEqual:
         result = (1 == varCompareEquality(call,lrvar,rrvar));
         break;
      case opNotEqual:
         result = !varCompareEquality(call,lrvar,rrvar);
         break;
   }
   
   ret = constructVarRead(call,VBoolean);
   varPutBoolean(ret,result);
   return ret;
}


CONST_DATA(struct OpDescription) OpDescriptionList[] = {
   OP_DESC( opDelete,textcoreDeleteKeyword,   PRI_UNARY, do_op_delete,   0),
   OP_DESC( opTypeof,textcoreTypeofKeyword,PRI_UNARY, do_op_typeof,      0),
   OP_DESC( opAssignOnly,UNISTR(""),        PRI_ASSIGN, do_op_assign,
            OP_READ_RVAR|OP_WRITE_LVAR),
   OP_DESC( opAssign,UNISTR("="),        PRI_ASSIGN, do_op_assign,
            OP_READ_RVAR|OP_WRITE_LVAR),

   OP_DESC( opPostDecrement,UNISTR("--"),PRI_UNARY, do_op_crement,
            OP_READ_LVAR|OP_WRITE_LVAR|OP_POST_WRITE),
   OP_DESC( opPostIncrement,UNISTR("++"),PRI_UNARY, do_op_crement,
            OP_READ_LVAR|OP_WRITE_LVAR|OP_POST_WRITE),

   OP_DESC( opPreIncrement,UNISTR("++"), PRI_UNARY, do_op_crement,
            OP_READ_LVAR|OP_WRITE_LVAR),
   OP_DESC( opPreDecrement,UNISTR("--"), PRI_UNARY, do_op_crement,
            OP_READ_LVAR|OP_WRITE_LVAR),

   OP_DESC( opAssignMultiply,UNISTR("*="),PRI_ASSIGN,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR|OP_WRITE_LVAR),
   OP_DESC( opAssignDivide,UNISTR("/="), PRI_ASSIGN,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR|OP_WRITE_LVAR),
   OP_DESC( opAssignModulo,UNISTR("%="), PRI_ASSIGN,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR|OP_WRITE_LVAR),
   OP_DESC( opAssignAdd,UNISTR("+="),    PRI_ASSIGN, do_add_or_subtract,
            OP_READ_RVAR|OP_READ_LVAR|OP_WRITE_LVAR),
   OP_DESC( opAssignSubtract,UNISTR("-="),PRI_ASSIGN,do_op_subtract,
            OP_READ_RVAR|OP_READ_LVAR|OP_WRITE_LVAR),
   OP_DESC( opAssignBitAnd,UNISTR("&="), PRI_ASSIGN, do_twonumbers,
            OP_READ_RVAR|OP_READ_LVAR|OP_WRITE_LVAR),
   OP_DESC( opAssignBitXor,UNISTR("^="), PRI_ASSIGN, do_twonumbers,
            OP_READ_RVAR|OP_READ_LVAR|OP_WRITE_LVAR),
   OP_DESC( opAssignBitOr,UNISTR("|="),  PRI_ASSIGN, do_twonumbers,
            OP_READ_RVAR|OP_READ_LVAR|OP_WRITE_LVAR),
   OP_DESC( opAssignShiftLeft,UNISTR("<<="),PRI_ASSIGN,do_twonumbers,
            OP_READ_RVAR|OP_READ_LVAR|OP_WRITE_LVAR),
   OP_DESC( opAssignSignedShiftRight,UNISTR(">>="),PRI_ASSIGN,do_twonumbers,
            OP_READ_RVAR|OP_READ_LVAR|OP_WRITE_LVAR),
   OP_DESC( opAssignUnsignedShiftRight,UNISTR(">>>="),PRI_ASSIGN,do_twonumbers,
            OP_READ_RVAR|OP_READ_LVAR|OP_WRITE_LVAR),

   OP_DESC( opMultiply,UNISTR("*"),      PRI_MULTIPLICATIVE,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opDivide,UNISTR("/"),        PRI_MULTIPLICATIVE,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opModulo,UNISTR("%"),        PRI_MULTIPLICATIVE,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opAdd,UNISTR("+"),           PRI_ADDITIVE,do_add_or_subtract,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opSubtract,UNISTR("-"),      PRI_ADDITIVE,do_op_subtract,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opShiftLeft,UNISTR("<<"),    PRI_SHIFT,  do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opSignedShiftRight,UNISTR(">>"),PRI_SHIFT,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opUnsignedShiftRight,UNISTR(">>>"),PRI_SHIFT,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opLess,UNISTR("<"),          PRI_RELATIONAL,do_op_compare,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opLessEqual,UNISTR("<="),    PRI_RELATIONAL,do_op_compare,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opGreater,UNISTR(">"),       PRI_RELATIONAL,do_op_compare,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opGreaterEqual,UNISTR(">="), PRI_RELATIONAL,do_op_compare,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opEqual,UNISTR("=="),        PRI_EQUALITY,do_op_compare,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opNotEqual,UNISTR("!="),     PRI_EQUALITY,do_op_compare,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opBitAnd,UNISTR("&"),        PRI_BITWISE_AND,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opBitOr,UNISTR("|"),         PRI_BITWISE_OR,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR),
   OP_DESC( opBitXor,UNISTR("^"),        PRI_BITWISE_XOR,do_twonumbers,
            OP_READ_LVAR|OP_READ_RVAR),

   OP_DESC( opVoid,textcoreVoidKeyword,       PRI_UNARY, do_op_void,
            OP_READ_LVAR),
   OP_DESC( opPositive,UNISTR("+"),      PRI_UNARY, do_op_positive_or_negative,
            OP_READ_LVAR),
   OP_DESC( opNegative,UNISTR("-"),      PRI_UNARY, do_op_positive_or_negative,
            OP_READ_LVAR),
   OP_DESC( opBoolNot,UNISTR("!"),       PRI_UNARY, do_op_boolNot,  OP_READ_LVAR),
   OP_DESC( opBitNot,UNISTR("~"),        PRI_UNARY, do_op_bitNot,   OP_READ_LVAR)

#  if (0!=JSE_COMPILER)
      /* The following aren't used after compilation,
         but are needed for their priorities */
     ,OP_DESC( LogicalAND,UNISTR("&&"),     PRI_LOGICAL_AND,  NULL,    0),
      OP_DESC( LogicalOR,UNISTR("||"),      PRI_LOGICAL_OR,   NULL,    0),
      OP_DESC( ConditionalTrue,UNISTR("?"), PRI_CONDITIONAL,  NULL,    0)
#  endif
};
