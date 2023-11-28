/* varutil.c  Access to variables of all kinds.
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

/* 12/17/97 - complete rewrite of var started
 * 02/16/98 - transated core to C, Rich Robinson
 */

#include "srccore.h"
#if !defined(__JSE_PALMOS__) && !defined(__JSE_PSX__)
#  include <math.h>
#endif
#define sign(x) ((x<0)?-1:1)

   static jsenumber NEAR_CALL
convertStringToNumber(struct Call *call,VarRead *var)
{
   jsenumber val;
   jsechar *str = (jsechar *)varGetData(var,0);
   jsechar *parseEnd;
   jsechar *strEnd = str + (size_t)varGetArrayLength(var,call,NULL);
   /* some of the following code relies on our knowledge that data
    * returned from varGetData always has at least one extra null
    * character at the end that is not reported in the length
    */
   assert( 0 == *strEnd );

   SKIP_WHITESPACE(str);

   if ( str == strEnd )
   {
      /* a string with only whitespace is 0 for pure ecmascript,
       * NaN if implementor wants math errors warned about
       */
      val = ( jseOptWarnBadMath & call->Global->ExternalLinkParms.options )
          ? jseNaN : 0 ;
   }
   else
   {
      val = ( '0' == *str  &&  ('x'==str[1] || 'X'==str[1]) )
          ? /* hex number */ MY_strtol(str,&parseEnd,16)
#         if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
             : /* dec number */ strtod_jsechar(str,&parseEnd) ;
#         else
             : /* dec number */ MY_strtol(str,&parseEnd,10) ;
#         endif
      SKIP_WHITESPACE(parseEnd);
      if ( parseEnd != strEnd )
      {
         /* a character caused parsing to fail.  The only valid
          * characters to cause this are Infinity, +Infinity, or
          * -Infinity, else is jseNaN
          */
         jsebool neg = False;
         if ( '-' == *str )
            str++, neg = True;
         else if ( '+' == *str )
            str++;
         if ( !strncmp_jsechar(str,textcorevtype_Infinity,8) )
         {
            parseEnd = str + 8;
            SKIP_WHITESPACE(parseEnd);
         }
         if ( parseEnd == strEnd )
         {
            val = neg ? jseNegInfinity : jseInfinity ;
         }
         else
         {
            /* all attempts at making a number failed; it is NaN */
            val = jseNaN;
         }
      }
   }
   return val;
}

   static VarRead * NEAR_CALL
convert_var_ToPrimitive(struct Call *call,VarRead *SourceVar,jsebool hintstring)
{
   VarRead *new_var;
   VarType st = VAR_TYPE(SourceVar);

   if ( VObject == st )
   {
      new_var = varDefaultValue(SourceVar,call,hintstring);
      if( new_var==NULL || VAR_TYPE(new_var)==VObject )
      {
         callError(call,textcoreCANNOT_CONVERT_OBJECT);
         if( new_var )
         {
            VAR_THOROUGH_REMOVE_USER(new_var,call);
         }
         new_var = constructVarRead(call,VUndefined);
      }
   }
   else
   {
      new_var = constructVarRead(call,st);
      switch( st )
      {
         case VUndefined:
         case VNull:
            break;
         case VBoolean:
            varPutBoolean(new_var,varGetBoolean(SourceVar));
            break;
         case VNumber:
            varPutNumber(new_var,varGetNumber(SourceVar));
            break;
         case VString:
#        if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)         
         case VBuffer:
#        endif         
         {
            JSE_POINTER_UINDEX bmax;

            bmax = varGetArrayLength(SourceVar,call,NULL);
            varPutData(new_var,call,varGetData(SourceVar,0),bmax,st);
            break;
         }
         case VObject:
            /* should never get here, but it quites compiler warning */
            break;
      }
   }
   return new_var;
}

   jsebool NEAR_CALL
ToBoolean(struct Call *call,VarRead *var)
{
   jsebool result;
   /* return true or false on how ecma would evaluate as a boolean */
   switch( VAR_TYPE(var) )
   {
      case VUndefined:
      case VNull:
         result = False;
         break;
      case VBoolean:
      case VNumber:
      {
         jsenumber value = varGetNumber(var);
         result = (value==0 || jseIsNaN(value)) ? False : True ;
         break;
      }
      case VString:
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)         
      case VBuffer:
#     endif      
      {
         JSE_POINTER_UINDEX bmax;
         JSE_POINTER_SINDEX bmin;
         bmax = varGetArrayLength(var,call,&bmin);
         result = (bmax!=0 || bmin!=0) ? True : False ;
         break;
      }
      case VObject:
         result = True;
         break;
   }
   return result;
}

   static VarRead * NEAR_CALL
convert_var_ToNumber(struct Call *call,VarRead *SourceVar)
{
   jsenumber val;
   VarRead *new_var = NULL;
   switch( VAR_TYPE(SourceVar) )
   {
      case VUndefined:
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)         
      case VBuffer:
#     endif      
         val = jseNaN; break;
      case VNull:
         val = 0; break;
      case VBoolean: case VNumber:
         /* Both these store their value in the number field */
         val = varGetNumber(SourceVar); break;
      case VString:
      {
         val = convertStringToNumber(call,SourceVar);
         break;
      }
      case VObject:
      {
         VarRead *vartemp = convert_var(call,SourceVar,jseToPrimitive);
         new_var = convert_var(call,vartemp,jseToNumber);
         VAR_REMOVE_USER(vartemp,call);
         break;
      }
   }
   /* for the routines that don't set new var, they make 'val'
    * the correct value */
   if( NULL == new_var )
   {
      new_var = constructVarRead(call,VNumber);
      varPutNumber(new_var,val);
   }
   return new_var;
}

   static VarRead * NEAR_CALL
convert_var_ToSomeInteger(struct Call *call,VarRead *SourceVar,
                          jseConversionTarget dest_type)
{
   VarRead * new_var = convert_var(call,SourceVar,jseToNumber);
   jsenumber val = varGetNumber(new_var);

   assert( jseToInteger==dest_type || jseToInt32==dest_type ||
           jseToUint32==dest_type || jseToUint16==dest_type );
   if ( 0 == val  ||  !jseIsFinite(val) )
   {
      varPutNumber(new_var,0);
   }
   else
   {
      /* Note for jseToInt32 and jseToUint32: As far as I can tell, the spec
       * simply does the standard'C' conversion to an int. Fortunately, due to
       * 2's complement integer storage, the correct value is automatically
       * generated regardless of whether the target is signed or not.
       * Gotta love it.
       */
      jsenumber value;
      if( jseToUint16 == dest_type )
      {
         /* convert to 16 bits. This would also work signed or unsigned. */
         value = (sword32)val & 0xffff;
      }
      else
      {
         assert( jseToInt32==dest_type || jseToUint32==dest_type ||
                 jseToInteger==dest_type );
#        if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
            value = sign(val) * floor(fabs(val));
#        else
            value = val;
#        endif
         if ( jseToInteger != dest_type )
            value = (sword32)value;
      }
      varPutNumber(new_var,value);
   }
   
   return new_var;
}


   static VarRead * NEAR_CALL
convert_var_ToString(struct Call *call,VarRead *SourceVar)
{
   VarType st = VAR_TYPE(SourceVar);
   VarRead *new_var;

   if( st!=VObject )
      new_var = constructVarRead(call,VString);

   switch( st )
   {
      case VUndefined:
         varPutString(new_var,call,UNISTR("undefined"));
         break;
      case VNull:
         varPutString(new_var,call,UNISTR("null"));
         break;
      case VBoolean:
         varPutString(new_var,call,varGetBoolean(SourceVar)?UNISTR("true"):
                              UNISTR("false"));
         break;
      case VObject:
      {
         VarRead *vartemp = convert_var_ToPrimitive(call,SourceVar,True);
         if( VAR_TYPE(vartemp)==VString )
         {
            new_var = vartemp;
         }
         else
         {
            new_var = convert_var(call,vartemp,jseToString);
            VAR_REMOVE_USER(vartemp,call);
         }
         break;
      }
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)         
      case VBuffer:
      {
         JSE_POINTER_SINDEX bmin;
         JSE_POINTER_UINDEX bmax = varGetArrayLength(SourceVar,call,&bmin);
         jsechar *tmpbuf = jseMustMalloc(jsechar,(uint)(bmax*sizeof(jsechar)));
         ubyte _HUGE_ * origbuf = (ubyte _HUGE_ *)varGetData(SourceVar,0);
         uint x;

         for( x=0;x<bmax;x++ )
         {
            tmpbuf[x] = (jsechar) origbuf[x]; /* copy each to its slot,
                                     * but since the two arrays have different
                                     * types, some real work is done */
         }
         varPutStringLen(new_var,call,tmpbuf,bmax);
         jseMustFree(tmpbuf);
         break;
      }
#     endif      
      case VString:
      {
         JSE_POINTER_UINDEX bmax;
         bmax = varGetArrayLength(SourceVar,call,NULL);
         varPutStringLen(new_var,call,(jsechar _HUGE_ *)
                         varGetData(SourceVar,0),bmax);
         break;
      }
      case VNumber:
      {
         jsechar buffer[100];
         const jsechar *nString = buffer;
         jsenumber n = varGetNumber(SourceVar);
         if ( !jseIsFinite(n) )
         {
            if( jseIsNaN(n) )
            {
               nString = textcorevtype_NaN;
            }
            else
            {
               assert( jseIsInfinity(n) || jseIsNegInfinity(n) );
               strcpy_jsechar(buffer,jseIsInfinity(n)?UNISTR(""):UNISTR("-"));
               strcat_jsechar(buffer,textcorevtype_Infinity);
            }
         }
         else if ( 0 == n )
         {
            nString = UNISTR("0");
         }
         else
         {
            /* otherwise just a normal number */
#           if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
//; seb 99.2.3 - This fix is from Nombas.
               jse_sprintf(buffer,UNISTR("%.21g"),n);
//               jse_sprintf(buffer,UNISTR("%g"),n);
#           else
               long_to_string(n,buffer);
#           endif
         }

         {
            varPutString(new_var,call,nString);
         }
         break;
      }
   } /* endswitch */
   return new_var;
}


#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)         
   static VarRead * NEAR_CALL
convert_var_ToBytes(struct Call *call,VarRead *SourceVar)
{
   VarRead * new_var = constructVarRead(call,VBuffer);

   switch( VAR_TYPE(SourceVar) )
   {
      case VUndefined: break;
      case VNull:
      {
         void *ptr = NULL;
         varPutBuffer(new_var,call,&ptr,sizeof(void *));
         break;
      }
      case VBoolean:
      {
         jsebool val = varGetBoolean(SourceVar);
         varPutBuffer(new_var,call,(void *)&val,sizeof(val));
         break;
      }
      case VObject:
      {
         VarRead *tmp = convert_var(call,SourceVar,jseToString);
         varPutBuffer(new_var,call,varGetData(new_var,0),
                            varGetArrayLength(new_var,call,NULL)*
                      sizeof(jsechar));
         VAR_REMOVE_USER(tmp,call);
         break;
      }
      case VString:
      {
         varPutBuffer(new_var,call,varGetData(SourceVar,0),
                            varGetArrayLength(SourceVar,call,NULL)*
                      sizeof(jsechar));
         break;
      }
      case VBuffer:
      {
         varPutBuffer(new_var,call,varGetData(SourceVar,0),
                            varGetArrayLength(SourceVar,call,NULL));
         break;
      }
      case VNumber:
      {
         jsenumber val = varGetNumber(SourceVar);
         varPutBuffer(new_var,call,(void *)&val,sizeof(val));
         break;
      }
   }
   return new_var;
}
#endif


#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)         
   static VarRead * NEAR_CALL
convert_var_ToBuffer(struct Call *call,VarRead *SourceVar)
{
   VarRead * new_var = convert_var(call,SourceVar,jseToString);

   VarRead *tmp = constructVarRead(call,VBuffer);

   JSE_POINTER_UINDEX length = varGetArrayLength(new_var,call,NULL);

   ubyte _HUGE_ *buffer = (ubyte _HUGE_ *)HugeMalloc(length);
   jsechar _HUGE_ *oldbuf = (jsechar _HUGE_ *)(varGetData(new_var,0));
   JSE_POINTER_UINDEX x;

   assert( NULL != buffer );

   for( x=0;x<length;x++ )
      buffer[x] = (ubyte)(oldbuf[x]);
   varPutBuffer(tmp,call,buffer,length);
   HugeFree(buffer);
   VAR_REMOVE_USER(new_var,call);
   return tmp;
}
#endif


   static VarRead * NEAR_CALL
convert_var_ToObject(struct Call *call,VarRead *SourceVar)
{
   VarRead * new_var;
   const jsechar *type = NULL;
   switch ( VAR_TYPE(SourceVar) )
   {
      case VUndefined:
      case VNull:
         callError(call,textcoreCANNOT_CONVERT_TO_OBJECT);
         /* expect to return some variable, so create a dummy */
         new_var = constructVarRead(call,VUndefined);
         break;
      case VBoolean:
         type = UNISTR("Boolean");
         break;
      case VNumber:
         type = UNISTR("Number");
         break;
      case VString:
         type = UNISTR("String");
         break;
#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)         
      case VBuffer:
         type = UNISTR("Buffer");
         break;
#endif         
      case VObject:
         new_var = CONSTRUCT_VALUE_LOCK(call,SourceVar);
         break;
   }
   if ( NULL != type )
   {
      VarName name = EnterIntoStringTable(call,type,strlen_jsechar(type));
      VarRead *var = varGetMember(call,call->session.GlobalVariable,name);
      RemoveFromStringTable(call,name);
      new_var = ( NULL == var ) ? constructVarRead(call,VObject) :
         varCallConstructor(var,call,SourceVar);
   }
   return new_var;
}


/* Take the given variable and convert it to a new variable using the
 * ECMAScript conversion operators
 */
   VarRead * NEAR_CALL
convert_var(struct Call *call,VarRead *SourceVar,
            jseConversionTarget dest_type)
{
   VarRead *new_var;
#  ifndef NDEBUG
      new_var = NULL;
#  endif

   /* NOTE: this is all very ugly. Just see the section 9.x of the language
    * spec for what it is doing. */
   switch( dest_type )
   {
      case jseToPrimitive:
         new_var = convert_var_ToPrimitive(call,SourceVar,False);
         break;
      case jseToBoolean:
         new_var = constructVarRead(call,VBoolean);
         varPutBoolean(new_var,ToBoolean(call,SourceVar));
         break;
      case jseToNumber:
         new_var = convert_var_ToNumber(call,SourceVar);
         break;
      case jseToInteger:
      case jseToInt32:
      case jseToUint32:
      case jseToUint16:
         new_var = convert_var_ToSomeInteger(call,SourceVar,dest_type);
         break;
      case jseToString:
         new_var = convert_var_ToString(call,SourceVar);
         break;
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)         
      case jseToBytes:
         new_var = convert_var_ToBytes(call,SourceVar);
         break;
      case jseToBuffer:
         new_var = convert_var_ToBuffer(call,SourceVar);
         break;
#     endif         
      case jseToObject:
         new_var = convert_var_ToObject(call,SourceVar);
         break;
   } /* end switch( dest_type ) */

   assert( NULL != new_var );
   return new_var;
}


   void NEAR_CALL
ConcatenateStrings(struct Call *call,VarWrite *dest,
                   VarRead *str1,VarRead *str2)
{
   JSE_POINTER_UINDEX len1,len2;
   jsechar _HUGE_ *ConcatMem;

   assert( VString == VAR_TYPE(str1) );
   assert( VString == VAR_TYPE(str2) );
   len1 = varGetArrayLength(str1,call,NULL);
   len2 = varGetArrayLength(str2,call,NULL);
   ConcatMem = (jsechar _HUGE_ *)HugeMalloc(1+(sizeof(jsechar)*(len1+len2)));
      /* allocated extra byte in case total length is zero.  OK because it will
       * be freed almost immediately
       */
   HugeMemCpy(ConcatMem,varGetData(str1,0),len1);
   HugeMemCpy(ConcatMem+len1,varGetData(str2,0),len2);
   varPutStringLen(dest,call,ConcatMem,len1+len2);
   HugeFree(ConcatMem);
}


/* ----------------------------------------------------------------------
 * These routines are used to find the name of a variable to print out
 * in error messages.  This function need not be fast.
 * ---------------------------------------------------------------------- */


   static jsebool NEAR_CALL
IsThisTheVariable(struct Call *call,VarRead *me,
                  VarRead *TheVariable,VarName vn,
                  jsechar * const Buffer,uint BufferLength,
                  jsebool first)
{
   if( me==TheVariable )
   {
      const jsechar *name = GetStringTableEntry(call,vn,NULL);
      if ( name!=NULL && (uint)(strlen_jsechar(name)+2) < BufferLength )
      {
         strcpy_jsechar(Buffer,name);
         if( !first && IsNumericStringTableEntry(call,vn) )
         {
            Buffer[-1] = '[';
            strcat_jsechar(Buffer,UNISTR("]"));
         }
         return True;
      }
      return False;
   }

   /* this is a structure, so check each element of structure to see if it's
      the variable */
   if ( VObject == VAR_TYPE(me) && me->varmem->data.vobject.been_here==False )
   {
      const jsechar *name;
      uint VarNameLen;

      me->varmem->data.vobject.been_here = True;

      name = ( NULL == vn ) ? UNISTR("") : GetStringTableEntry(call,vn,NULL);
      assert( NULL != name );
      VarNameLen = 1 + strlen_jsechar(name);

      if ( VarNameLen+2 < BufferLength )
      {
         VarRead *MemberVar = NULL;
         VarName memberName;

         strcpy_jsechar(Buffer,name);
         if( name[0]!='\0' )
            Buffer[VarNameLen-1] = '.';
         else
            VarNameLen--;
         if( !first && IsNumericStringTableEntry(call,vn) )
         {
            Buffer[-1] = '[';
            Buffer[VarNameLen-1] = ']';
            Buffer[VarNameLen++] = '.';
         }
         while ( NULL != (MemberVar =
                          varGetNext(me,call,MemberVar,&memberName)) )
         {
            if ( IsThisTheVariable(call,MemberVar,TheVariable,
                                   memberName,
                                   Buffer+VarNameLen,BufferLength-VarNameLen,
                                   False) )
            {
               me->varmem->data.vobject.been_here = False;
               return True;
            }
         }
      }
      me->varmem->data.vobject.been_here = False;
   }

   return False;
}


   jsebool
FindNames(struct Call *call,Var *generic_me,
          jsechar * const Buffer,uint BufferLength)
{
   VarRead *me = GET_READABLE_VAR(generic_me,call);
   jsebool FoundName = False;
   struct TempVar *lookin;

   if( VAR_TYPE(me)==VUndefined )
    {
       /* If a variable is undefined in an object, the routine returns a
        * new variable of value 'undefined', which cannot be found in the
        * object (it is temporary.) This code makes sure that the variable
        * we are looking for really is a member of an object so we can
        * find its name
        */
       VarWrite *meto;

       VAR_REMOVE_USER(me,call);
       meto = getWriteableVar(generic_me,call);
       varConvert(meto,call,VUndefined);
       VAR_REMOVE_USER(meto,call);
       me = GET_READABLE_VAR(generic_me,call);
    }

   /* first we look in the global variables */

   if(IsThisTheVariable(call,call->session.GlobalVariable,me,
                        NULL,Buffer,BufferLength,True) )
      FoundName = True;

   /* next, lets search the scope chain for it. */

   for ( lookin = call->ScopeChain; !FoundName && (NULL != lookin);
         lookin = lookin->prev )
   {
      VarRead *vr = (VarRead *)(lookin->var);
      /* assert that this is already a varread so above case is valid */
      if( IsThisTheVariable(call,vr,me,NULL,Buffer,BufferLength,True) )
         FoundName = True;
   }

   /* lastly, lets look in the parent's local variable object so that
    * something like this:
    *
    * printf(UNISTR("%d\n"),a);
    *
    * where a is undefined will find the name 'a' to attach to it
    */

   if( !FoundName )
   {
      lookin = callPrevious(call)->ScopeChain;
      if( lookin!=NULL && IsThisTheVariable(call,lookin->var,me,NULL,Buffer,
                                            BufferLength,True) )
         FoundName = True;
   }

   VAR_REMOVE_USER(me,call);

   return FoundName;
}


   VarRead * NEAR_CALL
GetDotNamedVar(struct Call *call,VarRead *me,const jsechar *NameWithDots,
               jsebool FinalMustBeVObject)
{
   jsechar * dotFound;
   VarRead *mem;
   VarName dotsName;

   assert( NULL == NameWithDots  ||  0 != *NameWithDots );
   /* first make sure that "this" variable is an object variable */
   if ( (NULL != NameWithDots || FinalMustBeVObject) &&
        VObject != VAR_TYPE(me) )
   {
      varConvert(me,call,VObject);
   }
   if ( NULL == NameWithDots )
   {
      /* nowhere to go further down the loop; return */
      return me;
   }
   /* for each element in NameWithDots add a sub object */
   dotFound = ( NULL == NameWithDots ) ? (jsechar *)NULL :
      strchr_jsechar(NameWithDots,'.');
   if ( NULL != dotFound )
   {
      uint namelen = (uint)(dotFound - NameWithDots);
      jsechar *name = jseMustMalloc(jsechar,sizeof(jsechar)*(namelen+1));
      VarRead *vObj,*ret;
      VarName varname;

      memcpy(name,NameWithDots,sizeof(jsechar)*namelen);
      name[namelen] = 0;

      /* NYI:  length */
      varname = EnterIntoStringTable(call,name,strlen_jsechar(name));
      vObj = varGetDirectMember(call,me,varname,False);
      if( vObj==NULL )
         vObj = varCreateMember(me,call,varname,VObject);
      RemoveFromStringTable(call,varname);
      ret = GetDotNamedVar(call,vObj,dotFound+1,FinalMustBeVObject);
      jseMustFree(name);
      return ret;
   }
   /* no dot, so just return member */
   /* NYI: Length */
   dotsName = EnterIntoStringTable(call,NameWithDots,strlen_jsechar(NameWithDots));
   mem = varGetDirectMember(call,me,dotsName,False);
   if( mem==NULL )
      mem = varCreateMember(me,call,dotsName,
               (VarType)(FinalMustBeVObject?VObject:VUndefined));
   RemoveFromStringTable(call,dotsName);
   return GetDotNamedVar(call,mem,NULL,FinalMustBeVObject);
}

   jsebool NEAR_CALL
ArrayIndexDifference(struct Call *call,VarRead *vLeft,VarRead *vRight,
                     JSE_POINTER_SINDEX *Difference)
{
   assert( VAR_ARRAY_POINTER(vLeft)  &&  VAR_ARRAY_POINTER(vRight) );
   if( vLeft->varmem != vRight->varmem )
   {
      callError(call,textcoreCAN_ONLY_SUBTRACT_SAME_ARRAY);
      return False;
   } /* endif */
   *Difference = varOffsetFromZero(vLeft) - varOffsetFromZero(vRight);
   return True;
}


   static VarRead * NEAR_CALL
CreateFromNumberText(struct Call *call,jsechar *Source,jsechar **End)
{
   VarRead *var;
   jsenumber val;

   assert( NULL != End  &&  NULL != Source );

   /* convert to float and to long and use the one that uses the
    * most characters.  If they both use the same number of
    * characters then floating-point wins (to handle very very
    * long values) except if the first character is 0 (to
    * handle octal orhex conversion, in which case integer
    * wins)
    */
   val = MY_strtol(Source,End,0);
#  if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   {
      jsechar *fEnd;
      jsenumber fVal = strtod_jsechar(Source,&fEnd);
      if ( *End <= fEnd )
      {
         if ( *End == fEnd )
         {
            /* resolve dispute over long or float, who has better value */
            SKIP_WHITESPACE(Source);
            if ( '+' == *Source  ||  '-' == *Source )
               Source++;
            if ( '0' == *Source )
               goto ConversionFinished;
         }
         *End = fEnd;
         val = fVal;
      }
   }
   ConversionFinished:
#  endif
   if ( Source < *End ) {
      var = constructVarRead(call,VNumber);
      varPutNumber(var,val);
   } else {
      var = NULL; /* no characters were processed */
   } /* endif */
   return var;
}


/* ---------------------------------------------------------------------- */

   static VarRead * NEAR_CALL
CreateFromString(struct Call *call,jsechar *Source,jsechar **iEnd)
{
   jsechar QuoteChar = *Source;
   sint Count;
   uint used;
   jsechar *AllocStr = jseMustMalloc(jsechar,sizeof(jsechar));
   VarRead *var;

   assert( NULL != iEnd  &&  NULL != Source );
   assert( '\"' == *Source  ||  '\'' == *Source  ||  '`' == *Source );

   for ( Count = 0, Source++;
         *Source!='\0' && QuoteChar!=*Source;
         Source++, Count++ ) {
      /* code to handle the unicode constants */
      jsechar c;
      if ( '\\' == (c = *Source)  &&  '`' != QuoteChar ) {
         switch ( *(++Source) ) {
            case 'a': c = '\a'; break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'v': c = '\v'; break;
            case 'u': case 'U':
               c = (jsechar)BaseToULong(++Source,16,4,MAX_UWORD16,&used);
               Source += used - 1;
               break;
            case 'x': case 'X':
               c = (jsechar)BaseToULong(++Source,16,2,MAX_UWORD8,&used);
               Source += used - 1;
               break;
            case '0':case'1':case'2':case'3':case'4':case'5':case'6':case'7':
               c = (jsechar)BaseToULong(Source,8,3,MAX_UWORD8,&used);
               Source += used - 1;
               break;
            default: c = *Source; break;
         } /* endswitch */
      } /* endif */
      AllocStr = jseMustReMalloc(jsechar,AllocStr,(Count+1) *
                                 sizeof(*AllocStr));
      AllocStr[Count] = c;
   }

   /* always return a valid var so other routines don't crash */
   var = constructVarRead(call,VString);
   if ( QuoteChar == *Source ) {
      *iEnd = Source + 1;
      varPutStringLen(var,call,AllocStr,(ulong)Count);
   } else {
      callError(call,textcoreCANNOT_PROCESS_BETWEEN_QUOTES,QuoteChar);
   } /* endif */
   jseMustFree(AllocStr);
   return var;
}

   jsebool NEAR_CALL
AssignFromText(VarWrite *target,struct Call *call,jsechar *Source,
               jsebool *AssignSuccess,
               jsebool MustUseFullSourceString,jsechar **End)
{
   jsechar *EndOfProcessing;
   jsebool success = True;
   VarRead * (NEAR_CALL *WhichCreate)
      (struct Call *call,jsechar *Source,jsechar **End);
   VarRead *TempVar;

   assert(NULL != Source);
   *AssignSuccess = False;

   SKIP_WHITESPACE(Source);
   if ( NULL != strchr_jsechar(UNISTR("\"\'`"),*Source) ) {
      /* convert to string(" or `) or character or character array(') */
      WhichCreate = CreateFromString;
   } else {
      assert( '{' != *Source );
      WhichCreate = CreateFromNumberText;
   } /* endif */
   TempVar = (*WhichCreate)(call,Source,&EndOfProcessing);
   if ( NULL == TempVar ) {
      success = False;
   } else {
      /* was able to create, so assign to the return variable */
      if ( !MustUseFullSourceString  ||  0 == *EndOfProcessing ) {
         if ( !varAssign(target,call,TempVar) )
            success = False;
         *AssignSuccess = True;
         if ( NULL != End ) {
            *End = EndOfProcessing;
         }
      }
      VAR_REMOVE_USER(TempVar,call);
   }
   return success;
}


/* ---------------------------------------------------------------------- */

   VarRead * NEAR_CALL
AutoConvert(struct Call *call,VarRead *me,jseVarNeeded need)
{
   VarRead *v;

   if( need & (JSE_VN_NUMBER | JSE_VN_BYTE | JSE_VN_INT) )
   {
      /* convert to some numeric type */
      v = convert_var(call,me,jseToNumber);
      if ( 0 == (need & JSE_VN_NUMBER) )
      {
         /* convert to a smaller number */
         ulong Mask = (ulong) (( 0 != (need & JSE_VN_INT) ) ? 0xffff : 0xff);
         slong value = varGetLong(v);
         varPutLong(v,value & Mask);
      }
   }
#  if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)   
   else if ( need & (JSE_VN_STRING | JSE_VN_BOOLEAN | JSE_VN_BUFFER |
                     JSE_VN_OBJECT | JSE_VN_FUNCTION) )
#  else
   else if ( need & (JSE_VN_STRING | JSE_VN_BOOLEAN | JSE_VN_OBJECT |
                     JSE_VN_FUNCTION) )
#  endif   
   {
      /* convert to variable of defined type */
      jseConversionTarget ConvertToType;
      if ( need & (JSE_VN_STRING | JSE_VN_BOOLEAN) )
      {
         ConvertToType = (need & JSE_VN_STRING) ? jseToString : jseToBoolean ;
      }
      else
      {
#        if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)   
            assert( need & (JSE_VN_BUFFER | JSE_VN_OBJECT | JSE_VN_FUNCTION) );
#        else
            assert( need & (JSE_VN_OBJECT | JSE_VN_FUNCTION) );
#        endif         
         ConvertToType =
#           if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)   
               (need & JSE_VN_BUFFER) ? jseToBuffer :
#           endif               
            jseToObject ;
      }
      v = convert_var(call,me,ConvertToType);
   } else {
      /* directly specifying to convert to NULL or UNDEFINED */
      assert( need & (JSE_VN_NULL | JSE_VN_UNDEFINED) );
      v = constructVarRead(call,
             (VarType)(( need & JSE_VN_NULL ) ? VNull : VUndefined) );
   }
   varSetLvalue(v,True);
   return v;
}


#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   void
TokenWriteVar(struct Call *call,struct TokenSrc *tSrc,VarRead *me)
{
   VarType vType = VAR_TYPE(me);

   /* these days, only a constant variable would have the variable name here */
   assert( !varIsLvalue(me) );
   tokenWriteByte(tSrc,(ubyte)vType);
   switch ( vType ) {
      case VNumber:
      {
         jsenumber n = varGetNumber(me);
         tokenWriteNumber(tSrc,n);
         break;
      }
      case VString:
      {
         jsechar * str = (jsechar *)varGetData(me,0);
         stringLengthType length = (stringLengthType)varGetArrayLength(me,call,NULL);
         VarName n = EnterIntoStringTable(call,str,length);
         tokenWriteString(call,tSrc,n);
         RemoveFromStringTable(call,n);
      }  break;
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
      case VBuffer:
      {
         uint length = (uint)varGetArrayLength(me,call,NULL);
         void * data = (void *)varGetData(me,0);
         tokenWriteLong(tSrc,(slong)length);
         tokenWriteBuffer(tSrc,data,length);
      }  break;
#     endif
      case VBoolean:
         tokenWriteByte(tSrc,(uword8)varGetBoolean(me));
      default:
        /* it's enough just to have written the type for any other data type */
         break;
   }
}
#endif

#if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)
   VarRead *
TokenReadVar(struct Call *call,struct TokenDst *tDst)
{
   VarType vType = (VarType)tokenReadCode(tDst);
   VarRead *var = constructVarRead(call,vType);
   switch ( vType ) {
      case VNumber:
         varPutNumber(var,tokenReadNumber(tDst));
         break;
      case VString:
      {
         VarName n = tokenReadString(call,tDst);
         stringLengthType len;
         const jsechar * str = GetStringTableEntry(call,n,&len);
         varPutStringLen(var,call,str,len);
      }  break;
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
      case VBuffer:
      {  uint length = (uint)tokenReadLong(tDst);
         /* allocate extra byte just so cannot allocate 0 */
         void * buf = jseMustMalloc(void,(length+1));
         assert( NULL != buf );
         tokenReadBuffer(tDst,buf,length);
         varPutData(var,call,buf,length,vType);
         jseMustFree(buf);
      }  break;
#     endif
      case VBoolean:
         varPutBoolean(var,tokenReadByte(tDst));
         break;
      default:
         /* any other types do nothing special */
         break;
   }
   return var;
}
#endif


/* ---------------------------------------------------------------------- */
/* ECMA comparison operators, part 1 - relational comparisons             */
/* ---------------------------------------------------------------------- */
     

/* I've also greatly extended the 'tests\compare.jse' test to give these
 * functions a good workout. They handled all the cases I put to them
 * (about 50 comparisons of every way shape and form, regular and cfunction.)
 * I feel better about these functions now and I think they should be
 * fully ECMA-compliant (plus our cfunction/buffer additions.)
 */

#if defined(JSE_INLINES) && (0!=JSE_INLINES)
#  define VarTypeConvertToPrimitive(call,var) \
   ((VObject == VAR_TYPE(*var)) ? _VarTypeConvertToPrimitive(call,var) : varAddUser(*var), VAR_TYPE(*var))
#else
#  define VarTypeConvertToPrimitive  _VarTypeConvertToPrimitive
#endif

   static VarType NEAR_CALL
_VarTypeConvertToPrimitive(struct Call *call,VarRead **v)
   /* return variable type, convertrf to number if object.  Will Add user. */
{
#if !defined(JSE_INLINES) || (0==JSE_INLINES)
   if( VObject != VAR_TYPE(*v) )
   {
      varAddUser(*v);
      return VAR_TYPE(*v);
   }
#endif

   assert( VObject == VAR_TYPE(*v) );

   /* convert variable to a primitive number */
   *v = varDefaultValue(*v,call,False);
   if( *v==NULL || VAR_TYPE(*v)==VObject )
   {
      callError(call,textcoreCANNOT_CONVERT_OBJECT);
      if( *v != NULL )
      {
         VAR_REMOVE_USER(*v,call);
      }
      *v = constructVarRead(call,VUndefined);
   }
   return VAR_TYPE(*v);
}

#if defined(JSE_INLINES) && (0!=JSE_INLINES)
#  define ConvertVarToNumberInPlace(call,v);  \
   {  \
      VarRead * _tmp_ = convert_var_ToNumber(call,*v); \
      VAR_REMOVE_USER(*v,call);  \
      *v = _tmp_;  \
   }
#else
   static void NEAR_CALL
ConvertVarToNumberInPlace(struct Call *call,VarRead **v)
{
   VarRead *tmp = convert_var_ToNumber(call,*v);
   VAR_REMOVE_USER(*v,call);
   *v = tmp;
}
#endif


/* For the relational operators, everything is described in terms of
 * 'less-than'. This routine does the comparison. It returns '1' (true)
 * '0' (false) or -1 (undefined). See 11.8.5. This routine knows nothing
 * about our special buffer type or about strings in cfunctions. Buffers are
 * treated just like strings.
 */
#if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
 static
#endif
   int NEAR_CALL
varECMACompareLess(struct Call *call,VarRead *vx,VarRead *vy)
{
   sint result; /* all paths must set this to some value */
   jsenumber x,y;
   VarType tx,ty;
   
   /* steps 1 and 2 - to save time, we only convert to primitive if it is
    * already not a primitive.
    */
   tx = VarTypeConvertToPrimitive(call,&vx);
   ty = VarTypeConvertToPrimitive(call,&vy);

   /* at this point, both vx and vy might have locks we must free - no
    * 'return'ing from the middle of this function!
    */

   if ( (tx==ty)  &&  TYPE_IS_ARRAY(tx) )
   {
      /* Both are strings or both are buffers */
      const jsechar _HUGE_ *str1 = varGetData(vx,0);
      const jsechar _HUGE_ *str2 = varGetData(vy,0);
      JSE_POINTER_UINDEX lx = varGetArrayLength(vx,call,NULL);
      JSE_POINTER_UINDEX ly = varGetArrayLength(vy,call,NULL);

#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
      if ( VBuffer == tx )
      {
         /* compare two buffers */
         JSE_POINTER_UINDEX lmin = min(lx,ly);
         result = HugeMemCmp(str1,str2,lmin);
         if ( 0 == result )
            result = ( lx < ly ) ? 1 : 0 ;
      }
      else
#     endif
      {
         /* compare two strings */
         result = jsecharCompare(str1,lx,str2,ly);
      }
      result = ( result < 0 ) ? 1 : 0 ;

   }
   else
   {
      /* step 4, they are not both strings or both buffers */

      /* neither are objects, so only strings and buffers need be translated.
       * The reason is that varGetNumber() knows how to extract the correct value
       * from all the other types without going through the hassle of
       * explictly translating them. It helps speed.
       */
      if( TYPE_IS_ARRAY(tx) )
      {
         ConvertVarToNumberInPlace(call,&vx);
         /* tx is not used again along this path */
      }
      if( TYPE_IS_ARRAY(ty) )
      {
         ConvertVarToNumberInPlace(call,&vy);
         /* ty is not used again along this path */
      }

      /* step 6 */

      x = varGetNumber(vx);
      y = varGetNumber(vy);
      if( jseIsNaN(x) || jseIsNaN(y) )
      {
         if( jseOptWarnBadMath & call->Global->ExternalLinkParms.options )
         {
            callError(call,textcoreIS_NAN);
         }
         result = -1;
      }
      else
      {
         /* step 8 */
         /* NYI: do C compilers understand '-0'?? */
         if( x==y /* || (x==+0 && y==-0) || (x==-0 && y==+0) */ )
         {
            result = 0;
         }
         else
         {
            /* step 11 */
            if( jseIsInfinity(x) )
               result = 0;
            /* step 12 */
            else if( jseIsInfinity(y) )
               result = 1;
            /* step 13 */
            else if( jseIsNegInfinity(y) )
               result = 0;
            /* step 14 */
            else if( jseIsNegInfinity(x) )
               result = 1;

            /* step 15 */
            else if( x<y )
               result = 1;
            else
               result = 0;
         }
      }
   }

   VAR_REMOVE_USER(vx,call);
   VAR_REMOVE_USER(vy,call);

   return result;
}


/* This routine understands the special cases that require us to compare
 * unusually. If it is not one of those cases, we use the ECMA comparison.
 * We return 1,-0, or -1 exactly like the ECMA compare routine.
 */
#if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
   int NEAR_CALL
varCompareLess(struct Call *call,VarRead *vx,VarRead *vy)
{
   /* do C-like pointer behavor if this is a C-function and both are
    * strings or both are buffers.  Also, if one of the two types
    * is a literal then default to standard ecma behavior
    */
   VarType tx;
   if ( ( !CBehavior(call) )
     || ( (tx=VAR_TYPE(vx)) != VAR_TYPE(vy) )
     || ( !TYPE_IS_ARRAY(tx) )
     || ( !varIsLvalue(vx) || !varIsLvalue(vy) ) )
      return varECMACompareLess(call,vx,vy);

   /* C-like comparison of strings or buffers */
   if ( vx->varmem != vy->varmem )
      return -1; /* cannot compare from different addresses */

   return (vx->offset_or_createType<vy->offset_or_createType) ? 1 : 0 ;
}
#endif  /* defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS) */


/* Unfortunately, our previous comparison routine did not match well to
 * Javascript. Javascript expects to know what you are looking for. One
 * can use one 'less' comparison to do any relational comparison but you
 * have to know which comparison (less than, greater than, etc) you want in
 * advance to determine if you need to swap the operands. We cannot do that,
 * hence we need to do the comparison twice. Yuck.
 */
   jsebool NEAR_CALL
varCompare(struct Call *call,VarRead *vx,VarRead *vy,slong *CompareResult)
{
   /* Unfortunately, ECMA only compares based on less-than. To simulate gathering
    * all three possibilities, we must call this routine twice. This routine
    * will mimic the translation done by the less-than so that the values are
    * not translated more than once.
    *
    * Less than can give you any information you need (by swapping the order
    * of the comparison if necessary) but you need to know which information
    * you are searching for.
    */

   int less_than,greater_than;
   VarType tx, ty;
   
   tx = VarTypeConvertToPrimitive(call,&vx);
   ty = VarTypeConvertToPrimitive(call,&vy);

   if( tx!=VString || ty!=VString )
   {
      /* see the ecma compare for comments on why only translate these */
      if( TYPE_IS_ARRAY(tx) )
      {
         ConvertVarToNumberInPlace(call,&vx);
      }
      if( TYPE_IS_ARRAY(ty) )
      {
         ConvertVarToNumberInPlace(call,&vy);
      }
   }

   less_than = varCompareLess(call,vx,vy);
   greater_than = varCompareLess(call,vy,vx);

   VAR_REMOVE_USER(vy,call);
   VAR_REMOVE_USER(vx,call);

   if( less_than==-1 || greater_than==-1 )
   {
      /* if one of the operands is NaN, it should show up both ways */
      assert( less_than==-1 && greater_than==-1 );
      *CompareResult = 0;
      /* it means that any test ought to fail. We can't give a relationship
       * between them, ALL relationships are untrue (i.e. it is not less than,
       * not greater than, not less than or equal, etc.) So, the comparison
       * 'fails'.
       */
      return False;
   }

   /* it can't be both, but it can be neither */
   assert( less_than==0 || greater_than==0 );
   if( less_than==1 )
      *CompareResult = -1;
   else if( greater_than==1 )
      *CompareResult = 1;
   else
      *CompareResult = 0;
   return True;
}


/* ---------------------------------------------------------------------- */
/* ECMA comparison operators, part 2 - equality comparisons               */
/* ---------------------------------------------------------------------- */


/* This routine implements ECMAscript spec 11.9.3 determining equality.
 * Like its relational counterpart, it knows nothing about our special
 * c-function string rules and treats buffers identically to strings.
 */
#if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
 static
#endif
   jsebool NEAR_CALL
varECMACompareEquality(struct Call *call,VarRead *vx,VarRead *vy)
{
   VarType tx = VAR_TYPE(vx), ty = VAR_TYPE(vy);
   
   /* don't initialize it as all paths ought to set it */
   jsebool result;

   if( tx==ty )
   {
      /* step 2 */

      if( tx==VUndefined || tx==VNull )
      {
         result = True;
      }
      else
      {
         if( tx==VNumber )
         {
            /* step 5 */
            jsenumber x = varGetNumber(vx);
            jsenumber y = varGetNumber(vy);
            if( jseIsNaN(x) || jseIsNaN(y) )
            {
               /* any comparison to NaN fails, even comparing it to itself! */
               result = False;
            }
            else
            {
               /* step 7 */
               /* NYI: do C compilers understand '-0'?? */
               if( x==y /* || (x==+0 && y==-0) || (x==-0 && y==+0) */ )
                  result = True;
               else
                  result = False;
            }
         }
         else
         {
            /* step 11 */
            if( TYPE_IS_ARRAY(tx) )
            {
               JSE_POINTER_UINDEX lx = varGetArrayLength(vx,call,NULL);
               JSE_POINTER_UINDEX ly = varGetArrayLength(vy,call,NULL);

               if ( lx != ly )
               {
                  result = False;
               }
               else
               {
                  const jsechar _HUGE_ *sx = varGetData(vx,0);
                  const jsechar _HUGE_ *sy = varGetData(vy,0);
#                 if defined(JSE_UNICODE) && (0!=JSE_UNICODE)         
#                    if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
                        if ( VString == tx )
#                    endif
                           lx *= sizeof(jsechar);
#                 endif
                  result = ( 0 == HugeMemCmp(sx,sy,lx) );
               }
            }
            else
            {
               /* step 12 */
               if( tx==VBoolean )
               {
                  result = varGetBoolean(vx)==varGetBoolean(vy);
               }
               else
               {
                  /* step 13 */
                  assert( tx==VObject );
                  /* true if they refer to the same object (i.e. the same varmem) */
                  result = vx->varmem==vy->varmem;
               }
               
            }
         }
      }
   }
   else
   {
      /* step 14 - the types are not the same */

      if( tx==VNull && ty==VUndefined )
         result = True;
      else if( tx==VUndefined && ty==VNull )
         result = True;
      else if( tx==VNumber && ty==VString )
      {
         /* step 16 */
         vy = convert_var_ToNumber(call,vy);
         result = varECMACompareEquality(call,vx,vy);
         VAR_REMOVE_USER(vy,call);
      }
      else if( ty==VNumber && TYPE_IS_ARRAY(tx) )
      {
         /* step 17 */
         vx = convert_var_ToNumber(call,vx);
         result = varECMACompareEquality(call,vx,vy);
         VAR_REMOVE_USER(vx,call);
      }
      else if( tx==VBoolean )
      {
         /* step 18 */
         vx = convert_var_ToNumber(call,vx);
         result = varECMACompareEquality(call,vx,vy);
         VAR_REMOVE_USER(vx,call);
      }
      else if( ty==VBoolean )
      {
         /* step 19 */
         vy = convert_var_ToNumber(call,vy);
         result = varECMACompareEquality(call,vx,vy);
         VAR_REMOVE_USER(vy,call);
      }
      else if( ty==VObject && (TYPE_IS_ARRAY(tx) || tx==VNumber) )
      {
         /* step 20 - use generic ToPrimitive, no hint, should the hint match
          * x? The spec currently says no. */
         vy = convert_var_ToPrimitive(call,vy,tx!=VNumber);
         result = varECMACompareEquality(call,vx,vy);
         VAR_REMOVE_USER(vy,call);
      }
      else if( tx==VObject && (TYPE_IS_ARRAY(ty) || ty==VNumber) )
      {
         /* step 21 */
         vx = convert_var_ToPrimitive(call,vx,ty!=VNumber);
         result = varECMACompareEquality(call,vx,vy);
         VAR_REMOVE_USER(vx,call);
      }
      else
         result = False;
   }

   return result;
}

#if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
   jsebool NEAR_CALL
varCompareEquality(struct Call *call,VarRead *vx,VarRead *vy)
{
   /* do C-like pointer behavor if this is a C-function and both are
    * strings or both are buffers.  Also, if one of the two types
    * is a literal then default to standard ecma behavior
    */
   VarType tx;
   return ( ( !CBehavior(call) )
         || ( (tx=VAR_TYPE(vx)) != VAR_TYPE(vy) )
         || ( !TYPE_IS_ARRAY(tx) )
         || ( !varIsLvalue(vx) || !varIsLvalue(vy) ) )
      ? varECMACompareEquality(call,vx,vy)
      : ( ( vx->varmem == vy->varmem )
       && ( vx->offset_or_createType == vy->offset_or_createType ) );
}
#endif  /* defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS) */
