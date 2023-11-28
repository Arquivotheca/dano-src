/* seobject.c
 *
 * Handles the ECMAScript objects for predefined types.
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

#include "jseopt.h"
// seb 98.11.12 -- Added to pick up prototype for CreateNewObject
#include "seecma.h"

#ifdef JSE_ECMA_NUMBER
#  include <float.h>
#endif


/* ---------------------------------------------------------------------- */


/* This is the 'builtin' object constructor. It is the most basic type.
 * It just initializes a new object and returns it.
 */
#if defined(JSE_ECMA_OBJECT)
static InternalLibFunc(Ecma_Object_builtin)
{
   // seb 98.11.12 -- Added cast to jseVariable
   jseReturnVar(jsecontext,(jseVariable)CreateNewObject(jsecontext,OBJECT_PROPERTY),jseRetTempVar);
}
#endif

/* This is the 'builtin' array construct. */
#if defined(JSE_ECMA_FUNCTION)
static jseLibFunc(Ecma_Function_builtin)
{
   // seb 98.11.12 -- Added cast to jseVariable
   jseReturnVar(jsecontext,(jseVariable)CreateNewObject(jsecontext,FUNCTION_PROPERTY),jseRetTempVar);
}
#endif

/* This is the 'builtin' array construct. */
#if defined(JSE_ECMA_ARRAY)
static jseLibFunc(Ecma_Array_builtin)
{
   // seb 98.11.12 -- Added cast to jseVariable
   jseReturnVar(jsecontext,(jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY),jseRetTempVar);
}
#endif


/* This is the 'builtin' string construct. */
#if defined(JSE_ECMA_STRING)
static jseLibFunc(Ecma_String_builtin)
{
   // seb 98.11.12 -- Added cast to jseVariable
   jseReturnVar(jsecontext,(jseVariable)CreateNewObject(jsecontext,STRING_PROPERTY),jseRetTempVar);
}
#endif

/* This is the 'builtin' boolean construct. */
#if defined(JSE_ECMA_BOOLEAN)
static jseLibFunc(Ecma_Boolean_builtin)
{
   // seb 98.11.12 -- Added cast to jseVariable
   jseReturnVar(jsecontext,(jseVariable)CreateNewObject(jsecontext,BOOLEAN_PROPERTY),jseRetTempVar);
}
#endif

/* This is the 'builtin' number construct. */
#if defined(JSE_ECMA_NUMBER)
static jseLibFunc(Ecma_Number_builtin)
{
   // seb 98.11.12 -- Added cast to jseVariable
   jseReturnVar(jsecontext,(jseVariable)CreateNewObject(jsecontext,NUMBER_PROPERTY),jseRetTempVar);
}
#endif


/* ---------------------------------------------------------------------- */

#if defined(JSE_ECMA_OBJECT)
/* Object() */
static jseLibFunc(Ecma_Object_call)
{
   jseVariable var;

   if( jseFuncVarCount(jsecontext)==0 )
   {
      PlainNewObject:
      jseReturnVar(jsecontext,jseCreateVariable(jsecontext,jseTypeObject),jseRetTempVar);
      return;
   }

   var = jseFuncVar(jsecontext,0);

   if( jseGetType(jsecontext,var)==jseTypeNull ||
       jseGetType(jsecontext,var)==jseTypeUndefined )
   {
      goto PlainNewObject;
   }

   jseReturnVar(jsecontext,jseCreateConvertedVariable(jsecontext,var,jseToObject),
                jseRetTempVar);
}

#endif /* JSE_ECMA_OBJECT */


/* ---------------------------------------------------------------------- */
/* The 'Function' object */
/* ---------------------------------------------------------------------- */

#ifdef JSE_ECMA_FUNCTION

/* add_to_buffer - manage a dynamic buffer, used by Function.construct */
   static void NEAR_CALL
add_to_buffer(jsechar **buf,uint *size,const jsechar *text)
{
   uint newsize;
   *buf = jseMustReMalloc(jsechar,*buf,sizeof(jsechar)*(newsize = *size + strlen_jsechar(text))+1);
   strcpy_jsechar((*buf)+(*size),text);
   (*size) = newsize;
}

/* Function.construct */
static jseLibFunc(Ecma_Function_construct)
{
   /* Current method: interpret() a creation of the function with a
    * name 'anonymous' (first saving anything with that name),
    * then find and lock that function, replace its old value, and
    * return the lock.
    */
   uint num = jseFuncVarCount(jsecontext);
   jsechar *buf = NULL;
   uint size = 0;
   uint x;
   jseVariable ret,var,args/*,proto,newproto,con*/;
   static CONST_DATA(jsechar) anonymous_property[] = UNISTR("anonymous");

   add_to_buffer(&buf,&size,UNISTR("function anonymous("));
   for( x = 1; x < num; x++ )
   {
      jseVariable tmp = jseCreateConvertedVariable(jsecontext,
                                                   jseFuncVar(jsecontext,x-1),
                                                   jseToString);
      const jsechar *str = jseGetString(jsecontext,tmp,0);
      
      if( x!=1 ) add_to_buffer(&buf,&size,UNISTR(","));
      add_to_buffer(&buf,&size,str);
      jseDestroyVariable(jsecontext,tmp);
   }
   add_to_buffer(&buf,&size,UNISTR(")\n{\n"));


   if( num!=0 )
   {
      jseVariable tmp = jseCreateConvertedVariable(jsecontext,
                                                   jseFuncVar(jsecontext,num-1),
                                                   jseToString);
      const jsechar *str = jseGetString(jsecontext,tmp,0);
      
      add_to_buffer(&buf,&size,str);
      jseDestroyVariable(jsecontext,tmp);
   }

   add_to_buffer(&buf,&size,UNISTR("\n}\n"));


   var = jseGetMember(jsecontext,NULL,anonymous_property);
   if( var )
   {
      var = jseCreateSiblingVariable(jsecontext,var,0);
      jseDeleteMember(jsecontext,NULL,anonymous_property);
   }

   jseInterpret(jsecontext,NULL,buf,NULL,jseNewNone,JSE_INTERPRET_LOAD,
                jsePreviousContext(jsecontext),NULL);

   ret = jseGetMember(jsecontext,NULL,anonymous_property);
   if( ret )
   {
      ret = jseCreateSiblingVariable(jsecontext,ret,0);
   }
   else
   {
      /* error parsing it. */
      ret = jseCreateVariable(jsecontext,jseTypeObject);
   }

   if( var )
   {
      jseVariable old = jseMember(jsecontext,NULL,anonymous_property,jseTypeUndefined);
      jseAssign(jsecontext,old,var);
      jseDestroyVariable(jsecontext,var);
   }
   else
   {
      jseDeleteMember(jsecontext,NULL,anonymous_property);
   }

   {
      jseVariable proto, newproto, con;
      /* set up some properties - 15.3 says it will be set up like this, where F is this
       * new function object:
       *    F._prototype = Function.prototype;  this is handled internaly because all function
       *                                        objects implicitly inherit Function.prototype
       *    F._call = {this function};  if no _call then will default to calling this function
       *                                anyway, so no need to explicitly add it
       *    F.prototype = new Object(); see following lines */
	  // seb 98.11.12 -- Added cast to jseVariable
      proto = (jseVariable)CreateNewObject(jsecontext,OBJECT_PROPERTY);
      newproto = jseMember(jsecontext,ret,ORIG_PROTOTYPE_PROPERTY,jseTypeUndefined);
      jseAssign(jsecontext,newproto,proto);
      jseSetAttributes(jsecontext,newproto,jseDontEnum);
      /*    F.prototype.constructor = F;  see following lines */
      con = jseMember(jsecontext,ret,CONSTRUCTOR_PROPERTY,jseTypeUndefined);
      jseAssign(jsecontext,con,ret);
      jseSetAttributes(jsecontext,con,jseDontEnum);
      /* other properties F._construct, F.toString, F.length are handled automaically
       * by a function object inheriting from Function.prototype implicitly.
       * cleanup
       */
      jseDestroyVariable(jsecontext,proto);
   }
   args = jseMember(jsecontext,ret,ARGUMENTS_PROPERTY,jseTypeNull);
   jseSetAttributes(jsecontext,args,jseDontEnum | jseDontDelete | jseReadOnly);

   jseReturnVar(jsecontext,ret,jseRetTempVar);
   if( buf ) jseMustFree(buf);
}

#endif /* JSE_ECMA_FUNCTION */

/* ---------------------------------------------------------------------- */
/* The 'Array' object */
/* ---------------------------------------------------------------------- */

#ifdef JSE_ECMA_ARRAY

/* Array() */
static jseLibFunc(Ecma_Array_call)
{
   // seb 98.11.12 -- Added cast to jseVariable 
   jseVariable var = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);
   int c = (int) jseFuncVarCount(jsecontext);

   if( c>1 || (c==1 && jseGetType(jsecontext,jseFuncVar(jsecontext,0))!=jseTypeNumber))
   {
      int x;

      jsePutLong(jsecontext,jseMember(jsecontext,var,LENGTH_PROPERTY,jseTypeNumber),c);
      for( x=0;x<c;x++ )
      {
         jsechar buffer[50];
         sprintf_jsechar(buffer,UNISTR("%d"),x);
         jseAssign(jsecontext,jseMember(jsecontext,var,buffer,jseTypeUndefined),
                   jseFuncVar(jsecontext,(uint)x));
      }
   }
   else if( c==1 )
   {
      jsePutLong(jsecontext,jseMember(jsecontext,var,LENGTH_PROPERTY,jseTypeNumber),
                 jseGetLong(jsecontext,jseFuncVar(jsecontext,0)));
   }
   else
   {
      jsePutLong(jsecontext,jseMember(jsecontext,var,LENGTH_PROPERTY,jseTypeNumber),0);
   }

   jseReturnVar(jsecontext,var,jseRetTempVar);
}

/* Array.put */
static jseLibFunc(Ecma_Array_put)
{
   jseVariable property,obj;
   jseVariable value = jseFuncVar(jsecontext,1);
   const jsechar *pname;
   jseVariable prop;
   uint loc;
   ulong stringLength;

   JSE_FUNC_VAR_NEED(property,jsecontext,0,JSE_VN_STRING);

   pname = jseGetString(jsecontext,property,&stringLength);

   obj = jseGetCurrentThisVariable(jsecontext);

   /* ECMA section 15.3.4.1 */
   prop = jseGetMember(jsecontext,obj,pname);

   if( NULL == prop )
   {  /* Step 7 */
      prop = jseMember(jsecontext,obj,pname,jseGetType(jsecontext,value));
      jseAssign(jsecontext,prop,value);
   }
   else if ( strcmp_jsechar(pname,LENGTH_PROPERTY) == 0 )
   {  /* Step 12 */
      jseVariable len;
      jseVariable val = jseCreateConvertedVariable(jsecontext,value,jseToNumber);
      jseSetArrayLength(jsecontext,obj,0,
                        (unsigned long)jseGetNumber(jsecontext,val));
      jseDestroyVariable(jsecontext,val);
      len = jseMember(jsecontext,
                      obj,pname,jseTypeUndefined);
      jseAssign(jsecontext,len,value);
      jseSetAttributes(jsecontext,len,jseDontEnum | jseDontDelete);
      return;   
   }
   else
   {  /* Step 5 */
      jseAssign(jsecontext,prop,value);
   }

   /* Step 8 */
   loc = 0;
   while( isdigit_jsechar(pname[loc]) )
      loc++;

   if(  stringLength == loc )
   {  /* Step 9 */
      int max_val = atoi_jsechar(pname)+1;

// seb 99.4.28.  Let's not leak this, please.
//      jseVariable length = jseMemberEx(jsecontext,obj,LENGTH_PROPERTY,jseTypeNumber, jseCreateVariable);
      jseVariable length = jseMember(jsecontext,obj,LENGTH_PROPERTY,jseTypeNumber);
      if( max_val>jseGetNumber(jsecontext,length))
         jsePutNumber(jsecontext,length,max_val);
//	  jseDestroyVariable(jsecontext, length);
   }
}

/* Array.join() */
static jseLibFunc(Ecma_Array_join)
{
   jsechar *string = NULL;

   const jsechar *separator;
   jseVariable str = NULL;
   JSE_POINTER_UINDEX s;
   jseVariable lengthvar;
   ulong length;
   jseVariable ret;
   uint loc,x;

   if( jseFuncVarCount(jsecontext)==1 )
   {
      str = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),jseToString);
      separator = jseGetString( jsecontext, str, &s );
   }
   else
   {
      separator = UNISTR(",");
      s = 1;
   }
   lengthvar = jseMember(jsecontext,jseGetCurrentThisVariable(jsecontext),
                         LENGTH_PROPERTY,jseTypeNumber);
   length = (ulong) jseGetLong(jsecontext,lengthvar);

   ret = jseCreateVariable(jsecontext,jseTypeString);
   loc = 0;

   for( x = 0;x<length;x++ )
   {
      jseVariable v;

      if( x!=0 )
      {
         string = jseMustReMalloc(jsechar,string,1/*don't alloc 0 bytes*/+(loc+s)*sizeof(jsechar));
         memcpy(string+loc,separator,s*sizeof(jsechar));
         loc += (uint)s;
      }

      v = jseIndexMember(jsecontext,jseGetCurrentThisVariable(jsecontext),
                         (slong)x,jseTypeUndefined);

      if( jseGetType(jsecontext,v)!=jseTypeUndefined &&
          jseGetType(jsecontext,v)!=jseTypeNull )
      {
         JSE_POINTER_UINDEX count;
         const jsechar * srcStr;
         jseVariable v2 = jseCreateConvertedVariable(jsecontext,v,jseToString);

         srcStr = jseGetString(jsecontext,v2,&count);
         string = jseMustReMalloc(jsechar,string,1/*don't alloc 0 bytes*/+((loc+count)*sizeof(jsechar)));
         memcpy(string+loc,srcStr,count * sizeof(jsechar));
         loc += count;
         jseDestroyVariable(jsecontext,v2);
      }
   }

   if( str ) jseDestroyVariable(jsecontext,str);

   jsePutStringLength(jsecontext,ret,string?string:UNISTR(""),loc);
   if( string ) jseMustFree(string);
   jseReturnVar(jsecontext,ret,jseRetTempVar);
}

/* Array.toString() */
static jseLibFunc(Ecma_Array_toString)
{
   Ecma_Array_join(jsecontext);
}

/* Array.reverse() */
static jseLibFunc(Ecma_Array_reverse)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jseVariable lengthvar = jseMember(jsecontext,thisvar,LENGTH_PROPERTY,jseTypeNumber);
   ulong max = (ulong)jseGetLong(jsecontext,lengthvar) - 1;

   if( jseGetLong(jsecontext,lengthvar))
   {
      ulong x;

      for( x=0;x<=max/2;x++ )
      {
         ulong y = max - x;
         jseVariable t;

         if( x==y ) continue;

         t = jseCreateVariable(jsecontext,jseTypeUndefined);

         jseAssign(jsecontext,t,jseIndexMember(jsecontext,thisvar,(slong)x,jseTypeUndefined));
         jseAssign(jsecontext,jseIndexMember(jsecontext,thisvar,(slong)x,jseTypeUndefined),
                   jseIndexMember(jsecontext,thisvar,(slong)y,jseTypeUndefined));
         jseAssign(jsecontext,jseIndexMember(jsecontext,thisvar,(slong)y,jseTypeUndefined),t);

         jseDestroyVariable(jsecontext,t);
      }

      jseReturnVar(jsecontext,thisvar,jseRetCopyToTempVar);
   }
}


struct array_sort_struct {
  jseContext jsecontext;
  jseVariable func;
  jseVariable item;
};

/* array_sort_func - passed to qsort to do array sorting */
   static int
array_sort_func(void const *one,void const *two)
{
   struct array_sort_struct *v1 = (struct array_sort_struct *)one;
   struct array_sort_struct *v2 = (struct array_sort_struct *)two;
   jseContext jsecontext = v1->jsecontext;
   jseStack stack;
   jseVariable retvar;
   int val = 0;

   if( v1->func==NULL )
   {
      jseVariable vc1,vc2;
      const jsechar *s1, *s2;
      JSE_POINTER_UINDEX len1, len2;
      int retval;

      if( jseGetType(jsecontext,v1->item)==jseTypeUndefined )
      {
         return (jseGetType(jsecontext,v2->item)==jseTypeUndefined)?0:1;
      }
      else if( jseGetType(jsecontext,v2->item)==jseTypeUndefined )
      {
         return -1;
      }

      vc1 = jseCreateConvertedVariable(jsecontext,v1->item,jseToString);
      s1 = jseGetString(jsecontext,vc1,&len1);
      vc2 = jseCreateConvertedVariable(jsecontext,v2->item,jseToString);
      s2 = jseGetString(jsecontext,vc2,&len2);

      retval = jsecharCompare(s1,len1,s2,len2);

      jseDestroyVariable(jsecontext,vc1);
      jseDestroyVariable(jsecontext,vc2);

      return retval;
   }

   /* in the case of an error in the sort function, we kick the rest
    * out immediately
    */
   if( jseQuitFlagged(jsecontext)) return 0;

   stack = jseCreateStack(jsecontext);
   jsePush(jsecontext,stack,v1->item,False);
   jsePush(jsecontext,stack,v2->item,False);

   if( jseCallFunction(jsecontext,v1->func,stack,&retvar,NULL))
   {
      if( jseGetType(jsecontext,retvar)==jseTypeNumber )
         val = (int)jseGetLong(jsecontext,retvar);
   }
   jseDestroyStack(jsecontext,stack);

   return val;
}

/* Array.sort() */
static jseLibFunc(Ecma_Array_sort)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jseVariable func = NULL;
   jseVariable lengthvar;
   ulong num_items;
   struct array_sort_struct _HUGE_ *items;
   ulong x;

   if( jseFuncVarCount(jsecontext)==1 )
   {
      func = jseFuncVar(jsecontext,0);
   }

   lengthvar = jseMember(jsecontext,thisvar,LENGTH_PROPERTY,jseTypeNumber);
   num_items = (ulong) jseGetLong(jsecontext,lengthvar);

   items = (struct array_sort_struct _HUGE_ *)HugeMalloc(
                         sizeof(struct array_sort_struct) * num_items);

   for( x=0;x<num_items;x++ )
   {
      items[x].jsecontext = jsecontext;
      items[x].item = jseCreateVariable(jsecontext,jseTypeUndefined);
      jseAssign(jsecontext,items[x].item,
                jseIndexMember(jsecontext,thisvar,(slong)x,jseTypeUndefined));
      items[x].func = func;
   }

   qsort(items,(size_t)num_items,sizeof(struct array_sort_struct),array_sort_func);

   for( x=0;x<num_items;x++ )
   {
      if( !jseQuitFlagged(jsecontext))
         jseAssign(jsecontext,jseIndexMember(jsecontext,thisvar,(slong)x,jseTypeUndefined),
                   items[x].item);
      jseDestroyVariable(jsecontext,items[x].item);
   }

   HugeFree(items);

   jseReturnVar(jsecontext,thisvar,jseRetCopyToTempVar);
}

#endif /* JSE_ECMA_ARRAY */

/* ----------------------------------------------------------------------
 * 'String' object
 * ---------------------------------------------------------------------- */

#ifdef JSE_ECMA_STRING

/* String() call */
static jseLibFunc(Ecma_String_call)
{
   jseVariable ret;

   if( jseFuncVarCount(jsecontext)==1 )
   {
      ret = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),jseToString);
   }
   else
   {
      ret = jseCreateVariable(jsecontext,jseTypeString);
   }
   jseReturnVar(jsecontext,ret,jseRetTempVar);
}

/* new String() - String.construct */
static jseLibFunc(Ecma_String_construct)
{
   jseVariable value, tmp, thisvar;

   if ( 0 < jseFuncVarCount(jsecontext ) )
   {
      JSE_FUNC_VAR_NEED(value,jsecontext,0,
                        JSE_VN_LOCKREAD|JSE_VN_CREATEVAR
                       |JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_STRING));
   }
   else
   {
      value = jseCreateVariable(jsecontext,jseTypeString);
   }
   thisvar = jseGetCurrentThisVariable(jsecontext);
   jseAssign(jsecontext,tmp = jseMemberEx(jsecontext,thisvar,VALUE_PROPERTY,jseTypeString,jseLockWrite),value);
   jseSetAttributes(jsecontext,tmp,jseDontEnum | jseDontDelete | jseReadOnly);
   jsePutLong(jsecontext,tmp = jseMemberEx(jsecontext,thisvar,LENGTH_PROPERTY,jseTypeNumber,jseLockWrite),
              (slong)jseGetArrayLength(jsecontext,value,NULL));
   jseSetAttributes(jsecontext,tmp,jseDontEnum | jseDontDelete | jseReadOnly);
   jseDestroyVariable(jsecontext,value);
}

/* String.fromCharCode() */
static jseLibFunc(Ecma_String_fromCharCode)
{
   jseVariable ret = jseCreateVariable(jsecontext,jseTypeString);

   jsechar *string = jseMustMalloc(jsechar,(jseFuncVarCount(jsecontext)+1)*sizeof(jsechar));
   uint i;
   for( i=0;i<jseFuncVarCount(jsecontext);i++ )
   {
      jseVariable t = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,i),
                                                 jseToUint16);
      string[i] = (jsechar)jseGetLong(jsecontext,t);
      jseDestroyVariable(jsecontext,t);
   }
   string[i] = 0;
   jsePutStringLength(jsecontext,ret,string,i);
   jseMustFree(string);
   jseReturnVar(jsecontext,ret,jseRetTempVar);
}


/* String.toString(), String.valueOf() */
static jseLibFunc(Ecma_String_toString)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

   ensure_type(jsecontext,thisvar,STRING_PROPERTY);

   jseReturnVar(jsecontext,
                jseCreateSiblingVariable(jsecontext,
                   jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeString),0),
                jseRetTempVar);
}

/* String.charAt() */
static jseLibFunc(Ecma_String_charAt)
{
   jseVariable string = jseCreateConvertedVariable(jsecontext,
                                                   jseGetCurrentThisVariable(jsecontext),
                                                   jseToString);
   jseVariable pos = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),
                                                jseToInteger);
   ulong thepos = (ulong) jseGetLong(jsecontext,pos);

   jseVariable ret = jseCreateVariable(jsecontext,jseTypeString);

   if( jseGetArrayLength(jsecontext,string,NULL)>thepos )
   {
      jsePutStringLength(jsecontext,ret,jseGetString(jsecontext,string,NULL)+thepos,1);
   }

   jseReturnVar(jsecontext,ret,jseRetTempVar);

   jseDestroyVariable(jsecontext,string);
   jseDestroyVariable(jsecontext,pos);
}

/* String.charCodeAt() */
static jseLibFunc(Ecma_String_charCodeAt)
{
   jseVariable string = jseCreateConvertedVariable(jsecontext,
                                                   jseGetCurrentThisVariable(jsecontext),
                                                   jseToString);
   jseVariable pos = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),
                                                jseToInteger);
   ulong thepos = (ulong) jseGetLong(jsecontext,pos);

   jseVariable ret = jseCreateVariable(jsecontext,jseTypeNumber);

   if( jseGetArrayLength(jsecontext,string,NULL)>thepos )
   {
      jsePutLong(jsecontext,ret,*(jseGetString(jsecontext,string,NULL)+thepos));
   }
   else
   {
      jsePutNumber(jsecontext,ret,jseNaN);
   }

   jseReturnVar(jsecontext,ret,jseRetTempVar);

   jseDestroyVariable(jsecontext,string);
   jseDestroyVariable(jsecontext,pos);
}

/* String.indexOf() */
static jseLibFunc(Ecma_String_indexOf)
{
   jseVariable string = jseCreateConvertedVariable(jsecontext,
                                                   jseGetCurrentThisVariable(jsecontext),
                                                   jseToString);
   jseVariable searchstring = jseCreateConvertedVariable(jsecontext,
                                                   jseFuncVar(jsecontext,0),
                                                   jseToString);
   JSE_POINTER_SINDEX start, found;
   JSE_POINTER_UINDEX stringLen, searchStringLen;
   jseVariable ret;
   const jsechar *st,*sub;
   jsechar firstChar;

   if( jseFuncVarCount(jsecontext) < 2 )
   {
      start = 0;
   }
   else
   {
      jseVariable pos = jseFuncVarNeed(jsecontext,1,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
      jsenumber startf = jseGetNumber(jsecontext,pos);
      start = jseIsFinite(startf) ? (slong)startf : 0 ;
      if ( start < 0 )
         start = 0;
   }

   ret = jseCreateVariable(jsecontext,jseTypeNumber);

   st =  jseGetString(jsecontext,string,&stringLen);
   sub = jseGetString(jsecontext,searchstring,&searchStringLen);

   found = -1;

   if ( 0 < searchStringLen  &&  searchStringLen <= stringLen )
   {
      if ( (JSE_POINTER_UINT)start <= (stringLen-=searchStringLen) )
      {
         JSE_POINTER_UINDEX SearchSize = searchStringLen*sizeof(jsechar);
         firstChar = sub[0];
         st += start;
         while( (JSE_POINTER_UINT)start <= stringLen )
         {
            if ( *st == firstChar
              && !memcmp(st,sub,SearchSize) )
            {
               found = start;
               break;
            }
            start++;
            st++;
         }
      }
   }

   jsePutLong(jsecontext,ret,found);
   jseReturnVar(jsecontext,ret,jseRetTempVar);

   jseDestroyVariable(jsecontext,string);
   jseDestroyVariable(jsecontext,searchstring);
}

/* String.lastIndexOf() */
static jseLibFunc(Ecma_String_lastIndexOf)
{
   jseVariable string = jseCreateConvertedVariable(jsecontext,
                                                   jseGetCurrentThisVariable(jsecontext),
                                                   jseToString);
   jseVariable searchstring = jseCreateConvertedVariable(jsecontext,
                                                   jseFuncVar(jsecontext,0),
                                                   jseToString);
   JSE_POINTER_SINDEX start, lastindex;
   JSE_POINTER_UINDEX stringLen, searchStringLen;
   jseVariable ret;
   const jsechar *st,*sub;
   jsechar firstChar;

   if( jseFuncVarCount(jsecontext) < 2 )
   {
      start = JSE_PTR_MAX_SINDEX;
   }
   else
   {
      jseVariable pos = jseFuncVarNeed(jsecontext,1,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
      jsenumber startf = jseGetNumber(jsecontext,pos);
      start = jseIsFinite(startf) ? (slong)startf : JSE_PTR_MAX_SINDEX ;
      if ( start < 0 )
         start = -1;
   }

   ret = jseCreateVariable(jsecontext,jseTypeNumber);
   lastindex = -1;

   if ( 0 <= start )
   {
      st =  jseGetString(jsecontext,string,&stringLen);
      sub = jseGetString(jsecontext,searchstring,&searchStringLen);

      if ( 0 < searchStringLen  &&  searchStringLen <= stringLen )
      {
         JSE_POINTER_UINDEX SearchSize = searchStringLen*sizeof(jsechar);
         if ( (stringLen-=searchStringLen) < (JSE_POINTER_UINT)start )
            start = (JSE_POINTER_SINDEX)stringLen;

         firstChar = sub[0];
         st += start;
         while( lastindex<start )
         {
            if ( *st == firstChar
              && !memcmp(st,sub,SearchSize) )
            {
               lastindex = start;
               break;
            }
            start--;
            st--;
         }
      }
   }

   jsePutLong(jsecontext,ret,lastindex);
   jseReturnVar(jsecontext,ret,jseRetTempVar);

   jseDestroyVariable(jsecontext,string);
   jseDestroyVariable(jsecontext,searchstring);
}

/* String.split() */
static jseLibFunc(Ecma_String_split)
{
   // seb 98.11.12 -- Added cast to jseVariable
   jseVariable ret = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);
   jseVariable str = jseCreateConvertedVariable(jsecontext,
                                                jseGetCurrentThisVariable(jsecontext),
                                                jseToString);

   if( jseFuncVarCount(jsecontext)==0 )
   {
      ulong length = jseGetArrayLength(jsecontext,str,NULL);
      jsePutStringLength(jsecontext,jseIndexMember(jsecontext,ret,0,jseTypeString),
                 jseGetString(jsecontext,str,NULL),length);
   }
   else
   {
      JSE_POINTER_UINDEX dataLen;
      const jsechar *data = jseGetString(jsecontext,str,&dataLen);
      jseVariable sepvar = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),
                                                      jseToString);
      JSE_POINTER_UINDEX sepLen;
      const jsechar *sep = (const jsechar *)jseGetString(jsecontext,sepvar,&sepLen);

      if( 0 < sepLen )
      {
         ulong count = 0;
         if ( sepLen <= dataLen )
         {
            const jsechar firstChar = sep[0];
            const jsechar *next, *last;
            JSE_POINTER_UINDEX SearchSize = sepLen*sizeof(jsechar);

            for ( last = (next = data) + (dataLen-sepLen);
                  next <= last;
                  next++ )
            {
               if ( *next == firstChar  &&  !memcmp(next,sep,SearchSize) )
               {
                  jsePutStringLength(jsecontext,jseIndexMember(jsecontext,ret,(slong)count++,jseTypeString),
                             data,(JSE_POINTER_UINDEX)(next-data));
                  dataLen -= (next - data) + sepLen;
                  assert( 0 <= dataLen );
                  next = (data = next + sepLen) - 1;
               }
            }
         }
         if( 0 < dataLen )
         {
            jsePutStringLength(jsecontext,jseIndexMember(jsecontext,ret,(slong)count,jseTypeString),data,dataLen);
         }
      }
      else
      {
         uint length = (uint)jseGetArrayLength(jsecontext,str,NULL);
         uint x;
         for( x = 0;x<length;x++ )
         {
            jsePutStringLength(jsecontext,jseIndexMember(jsecontext,ret,(slong)x,jseTypeString),data+x,1);
         }
      }
      jseDestroyVariable(jsecontext,sepvar);
   }


   jseDestroyVariable(jsecontext,str);
   jseReturnVar(jsecontext,ret,jseRetTempVar);
}

/* String.substring() */
static jseLibFunc(Ecma_String_substring)
{
   jseVariable string = jseCreateConvertedVariable(jsecontext,
                                                   jseGetCurrentThisVariable(jsecontext),
                                                   jseToString);
   JSE_POINTER_SINDEX start_pos, end_pos = -1;
   long distance;
   jseVariable ret;
   sint length;

   jseVariable pos = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),
                                                    jseToInteger);
   start_pos = jseGetLong(jsecontext,pos);
   if ( start_pos < 0 )
      start_pos = 0;
   jseDestroyVariable(jsecontext,pos);


   if( 1 < jseFuncVarCount(jsecontext) )
   {
      pos = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,1),jseToInteger);
      end_pos = jseGetLong(jsecontext,pos);
      if ( end_pos < 0 )
         end_pos = 0;
      jseDestroyVariable(jsecontext,pos);
   }


   ret = jseCreateVariable(jsecontext,jseTypeString);

   if((length = (sint)jseGetArrayLength(jsecontext,string,NULL))>start_pos )
   {
      const jsechar *s;

      if( end_pos==-1 || end_pos>length )
      {
         end_pos = length;
      }

      if( start_pos > end_pos ) /* parameters where passed in, in reverse order */
      {
         distance = start_pos - end_pos;
         start_pos = end_pos;
      }
      else
      {
         distance = end_pos - start_pos;
      }

      s = (const jsechar *)jseGetString(jsecontext,string,NULL);
      jsePutStringLength(jsecontext,ret,s+start_pos,(ulong)distance);
   }

   jseReturnVar(jsecontext,ret,jseRetTempVar);

   jseDestroyVariable(jsecontext,string);
}

/* String.toLowerCase() */
static jseLibFunc(Ecma_String_toLowerCase)
{
   jseVariable ret = jseCreateConvertedVariable(jsecontext,
                                                jseGetCurrentThisVariable(jsecontext),
                                                jseToString);
   JSE_POINTER_UINDEX limit, i;
   jsechar *value = (jsechar *)jseGetWriteableString(jsecontext,ret,&limit);

   for( i=0;i<limit;i++ )
   {
#     if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
      if( value[i]<256 )
#     endif
         value[i] = (jsechar) tolower(value[i]);
   }
   jsePutStringLength(jsecontext,ret,value,limit);
   jseReturnVar(jsecontext,ret,jseRetTempVar);
}

/* String.toUpperCase() */
static jseLibFunc(Ecma_String_toUpperCase)
{
   jseVariable ret = jseCreateConvertedVariable(jsecontext,
                                                jseGetCurrentThisVariable(jsecontext),
                                                jseToString);
   JSE_POINTER_UINDEX limit, i;
   jsechar *value = (jsechar *)jseGetWriteableString(jsecontext,ret,&limit);

   for( i=0;i<limit;i++ )
   {
#     if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
      if( value[i]<256 )
#     endif
         value[i] = (jsechar) toupper_jsechar(value[i]);
   }
   jsePutStringLength(jsecontext,ret,value,limit);
   jseReturnVar(jsecontext,ret,jseRetTempVar);
}

#endif /* JSE_ECMA_STRING */

/* ---------------------------------------------------------------------- */

/* Boolean object functions */

#ifdef JSE_ECMA_BOOLEAN

/* Boolean() call */
static jseLibFunc(Ecma_Boolean_call)
{
   if( jseFuncVarCount(jsecontext)==1 )
   {
      jseReturnVar(jsecontext,
                   jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),jseToBoolean),
                   jseRetTempVar);
   }
   else
   {
      jseVariable ret = jseCreateVariable(jsecontext,jseTypeBoolean);
      jsePutBoolean(jsecontext,ret,FALSE);
      jseReturnVar(jsecontext,ret,jseRetTempVar);
   }
}

/* new Boolean() - Boolean.construct */
static jseLibFunc(Ecma_Boolean_construct)
{
   jsebool value;
   jseVariable val, thisvar;

   if( 0 < jseFuncVarCount(jsecontext) )
   {
      JSE_FUNC_VAR_NEED(val,jsecontext,0,
                        JSE_VN_LOCKREAD
                       |JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_BOOLEAN));
      value = (jsebool)jseGetLong(jsecontext,val);
   }
   else
   {
      value = False;
   }
   thisvar = jseGetCurrentThisVariable(jsecontext);
   jsePutBoolean(jsecontext,
                 jseMemberEx(jsecontext,thisvar,VALUE_PROPERTY,jseTypeBoolean,jseLockWrite),
                 value);
}

/* Boolean.toString() */
static jseLibFunc(Ecma_Boolean_toString)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jseVariable ret,val;
   jsebool value;

   ensure_type(jsecontext,thisvar,BOOLEAN_PROPERTY);

   ret = jseCreateVariable(jsecontext,jseTypeString);

   val = jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeBoolean);
   value = (jseGetType(jsecontext,val)==jseTypeBoolean)?
      (jsebool)jseGetLong(jsecontext,val):FALSE;

// seb 98.11.12 -- textlibstrEcmaTRUE and FALSE don't exist.
   jsePutString(jsecontext,ret,value?"true":"false");
//   jsePutString(jsecontext,ret,value?textlibstrEcmaTRUE:textlibstrEcmaFALSE);
   jseReturnVar(jsecontext,ret,jseRetTempVar);
}

/* Boolean.valueOf() */
static jseLibFunc(Ecma_Boolean_valueOf)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

   ensure_type(jsecontext,thisvar,BOOLEAN_PROPERTY);

   jseReturnVar(jsecontext,
                jseCreateSiblingVariable(jsecontext,
                   jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeBoolean),0),
                jseRetTempVar);
}
#endif /* JSE_ECMA_BOOLEAN */

/* ----------------------------------------------------------------------
 * 'Number' Object methods
 * ---------------------------------------------------------------------- */

#ifdef JSE_ECMA_NUMBER

/* Number() call */
static jseLibFunc(Ecma_Number_call)
{
   if( jseFuncVarCount(jsecontext)==1 )
   {
      jseReturnVar(jsecontext,
                   jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),jseToNumber),
                   jseRetTempVar);
   }
   else
   {
      jseVariable ret = jseCreateVariable(jsecontext,jseTypeNumber);
      jsePutLong(jsecontext,ret,0);
      jseReturnVar(jsecontext,ret,jseRetTempVar);
   }
}

/* new Number() - Number.construct */
static jseLibFunc(Ecma_Number_construct)
{
   jsenumber value;
   jseVariable val, tmp, thisvar;

   if( 0 < jseFuncVarCount(jsecontext) )
   {
      JSE_FUNC_VAR_NEED(val,jsecontext,0,
                        JSE_VN_LOCKREAD
                       |JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
      value = jseGetNumber(jsecontext,val);
   }
   else
   {
      value = 0;
   }
   thisvar = jseGetCurrentThisVariable(jsecontext);
   jsePutNumber(jsecontext,
                tmp=jseMemberEx(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber,jseLockWrite),
                value);
   jseSetAttributes(jsecontext,tmp,jseDontEnum);
}

/* Number.toString() */
static jseLibFunc(Ecma_Number_toString)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jseVariable ret;
   jsechar buffer[128];
   jsechar *bufptr;
   
   ensure_type(jsecontext,thisvar,NUMBER_PROPERTY);

   ret = jseCreateVariable(jsecontext,jseTypeString);

// seb 99.2.3 - This fix is from Nombas.
   sprintf_jsechar(buffer,UNISTR("%.21g"),jseGetNumber(jsecontext,
            jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber)));
//   sprintf_jsechar(buffer,UNISTR("%.20g"),jseGetNumber(jsecontext,
//            jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber)));
//   for( bufptr=buffer;isspace_jsechar(*bufptr);bufptr++ ) /* noop */ ;
//   if( strchr_jsechar(buffer,'.')!=NULL )
//   {
      /* get rid of trailing zeroes. We need to have all the precision forced
       * otherwise the value gets rounded which is bad */
//      while( buffer[strlen_jsechar(buffer)-1]=='0' && strlen_jsechar(buffer)>1 )
//         buffer[strlen_jsechar(buffer)-1] = '\0';
//   }

   jsePutString(jsecontext,ret,bufptr);
   jseReturnVar(jsecontext,ret,jseRetTempVar);
}

/* Number.valueOf() */
static jseLibFunc(Ecma_Number_valueOf)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

   ensure_type(jsecontext,thisvar,NUMBER_PROPERTY);

   jseReturnVar(jsecontext,
                jseCreateConvertedVariable(jsecontext,
                   jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeNumber),jseToNumber),
                jseRetTempVar);
}
#endif /* JSE_ECMA_NUMBER */

#if defined(JSE_ECMA_ARRAY)    || \
    defined(JSE_ECMA_BOOLEAN)  || \
    defined(JSE_ECMA_FUNCTION) || \
    defined(JSE_ECMA_NUMBER)   || \
    defined(JSE_ECMA_OBJECT)   || \
    defined(JSE_ECMA_STRING)

/* ---------------------------------------------------------------------- */
#ifdef JSE_ECMA_NUMBER
#  if defined(__DJGPP__) || defined(__BORLANDC__) || defined(__MWERKS__)
   /* Under Metrowerks,DBL_MAX and DBL_MIN are not simply defines, and cannot
    * be used in static assignments
    */
// seb 99.3.24 -- This pukes under the new Metrowerks compiler.  I'm changing it to something bogus.
static CONST_DATA(jsenumber) jse_DBL_MAX = 1.7976931348623157E+307;
static CONST_DATA(jsenumber) jse_DBL_MIN = 0.0;
#  else
static CONST_DATA(jsenumber) jse_DBL_MAX = DBL_MAX;
static CONST_DATA(jsenumber) jse_DBL_MIN = DBL_MIN; 
#  endif

static jsenumber seNaN,seInfinity,seNegInfinity;
#endif

static CONST_DATA(struct jseFunctionDescription) ObjectLibFunctionList[] =
{
   /*
    * This table builds up all of the pre-defined links between the
    * various types of builtin objects. Its all pretty confusing. I
    * built it by reading the spec and doing exactly what they said (or
    * the best I could gather from it.) I hope I didn't make too many
    * mistakes.
    */

   /* ---------------------------------------------------------------------- */
   /* Some global values */
   /* ---------------------------------------------------------------------- */
#  ifdef JSE_ECMA_NUMBER
      JSE_VARNUMBER( UNISTR("NaN"), &seNaN, jseDontEnum ),
      JSE_VARNUMBER( UNISTR("Infinity"), &seInfinity, jseDontEnum ),
#  endif

   /*
    * Notes: all prototype objects have object prototype as their _prototype
    * except the object prototype object itself.
    */

   /* ---------------------------------------------------------------------- */
   /* Next we build up the 'Function' global function/object. */
   /* ---------------------------------------------------------------------- */
   /*
    * functions in the function prototype object dont point back to itself
    * anymore, section 15 pg 58.
    */
#  ifdef JSE_ECMA_FUNCTION
      JSE_LIBOBJECT( FUNCTION_PROPERTY,    Ecma_Function_construct, 0, -1,      jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( CONSTRUCTOR_PROPERTY, Ecma_Function_builtin,   0,  0,      jseDontEnum,  jseFunc_Secure ),
      /* Spec says length is 1. */
      JSE_VARSTRING( LENGTH_PROPERTY, UNISTR("1"), jseDontEnum | jseDontDelete | jseReadOnly ),
      /* the function prototype object itself is a function */
#  endif

   /* ---------------------------------------------------------------------- */
   /* Set up the 'Object' global object. It is a function. We change its */
   /* construct property because it behaives differently depending on if */
   /* it is called as a function or a construct. */
   /* ---------------------------------------------------------------------- */
#  ifdef JSE_ECMA_OBJECT
      JSE_LIBOBJECT( OBJECT_PROPERTY,      Ecma_Object_call,    0,      1,      jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( CONSTRUCTOR_PROPERTY, Ecma_Object_builtin, 0,      0,      jseDontEnum,  jseFunc_Secure ),
#  endif
 
   /* ---------------------------------------------------------------------- */
   /* The 'Array' object */
   /* ---------------------------------------------------------------------- */
#  ifdef JSE_ECMA_ARRAY
      JSE_LIBOBJECT( ARRAY_PROPERTY,       Ecma_Array_call,     0,     -1,      jseDontEnum,  jseFunc_Secure ),
      /* Ecma_Array_call used for both _call and _construct, so don't need them individually */
      /* Spec says length is 1. */
      JSE_VARSTRING( LENGTH_PROPERTY, UNISTR("1"), jseDontEnum | jseDontDelete | jseReadOnly ),
      JSE_VARSTRING( UNISTR("prototype.length"), UNISTR("0"), jseDontEnum ),
      JSE_VARSTRING( UNISTR("prototype._class"), UNISTR("\"Array\""), jseDontEnum ),
      JSE_PROTOMETH( CONSTRUCTOR_PROPERTY, Ecma_Array_builtin, 0, 0, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( TOSTRING_PROPERTY, Ecma_Array_toString, 0, 0, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("join"), Ecma_Array_join,0,1, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("reverse"), Ecma_Array_reverse,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("sort"), Ecma_Array_sort,0,1, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("put"), Ecma_Array_put,2,2, jseDontEnum,  jseFunc_Secure ),
      JSE_ATTRIBUTE( ORIG_PROTOTYPE_PROPERTY, jseDontEnum | jseReadOnly | jseDontDelete ),
#  endif

   /* ---------------------------------------------------------------------- */
   /* The 'String' object */
   /* ---------------------------------------------------------------------- */
#  ifdef JSE_ECMA_STRING
      JSE_LIBOBJECT( STRING_PROPERTY,       Ecma_String_call,    0,      1,      jseDontEnum,  jseFunc_Secure ),
      /* Set its prototype to 'Object.prototype' */
      JSE_VARSTRING( UNISTR("prototype._class"), UNISTR("\"String\""), jseDontEnum ),
      JSE_LIBMETHOD( CONSTRUCT_PROPERTY, Ecma_String_construct,0,1, jseDontEnum,  jseFunc_Secure ),
      JSE_LIBMETHOD( UNISTR("fromCharCode"), Ecma_String_fromCharCode,0,-1, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( CONSTRUCTOR_PROPERTY, Ecma_String_builtin,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( TOSTRING_PROPERTY, Ecma_String_toString,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_VARASSIGN( UNISTR("prototype.valueOf"), UNISTR("String.prototype.toString"), jseDontEnum ),
      JSE_PROTOMETH( UNISTR("charAt"), Ecma_String_charAt,1,1, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("charCodeAt"), Ecma_String_charCodeAt,1,1, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("indexOf"), Ecma_String_indexOf,1,2, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("lastIndexOf"),  Ecma_String_lastIndexOf,1,2, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("split"),  Ecma_String_split,0,1, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("substring"),  Ecma_String_substring,1,2, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("toLowerCase"),  Ecma_String_toLowerCase,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( UNISTR("toUpperCase"),  Ecma_String_toUpperCase,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_ATTRIBUTE( ORIG_PROTOTYPE_PROPERTY, jseDontEnum | jseReadOnly | jseDontDelete ),
#  endif
 
   /* ---------------------------------------------------------------------- */
   /* The 'Boolean' object */
   /* ---------------------------------------------------------------------- */
#  ifdef JSE_ECMA_BOOLEAN
      JSE_LIBOBJECT( BOOLEAN_PROPERTY,     Ecma_Boolean_call,    0,      1,      jseDontEnum,  jseFunc_Secure ),
      JSE_LIBMETHOD( CONSTRUCT_PROPERTY,  Ecma_Boolean_construct,0,1, jseDontEnum,  jseFunc_Secure ),
      JSE_VARSTRING( UNISTR("prototype._class"), UNISTR("\"Boolean\""), jseDontEnum ),
      JSE_VARSTRING( UNISTR("prototype._value"), UNISTR("false"), jseDontEnum ),
      JSE_PROTOMETH( CONSTRUCTOR_PROPERTY, Ecma_Boolean_builtin,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( TOSTRING_PROPERTY, Ecma_Boolean_toString,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( VALUEOF_PROPERTY, Ecma_Boolean_valueOf,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_ATTRIBUTE( ORIG_PROTOTYPE_PROPERTY, jseDontEnum | jseReadOnly | jseDontDelete ),
#  endif
 
   /* ---------------------------------------------------------------------- */
   /* The 'Number' object. */
   /* ---------------------------------------------------------------------- */
#  ifdef JSE_ECMA_NUMBER
      JSE_LIBOBJECT(  NUMBER_PROPERTY,      Ecma_Number_call,    0,      1,      jseDontEnum,  jseFunc_Secure ),
      JSE_LIBMETHOD( CONSTRUCT_PROPERTY,  Ecma_Number_construct,0,1, jseDontEnum,  jseFunc_Secure ),
      JSE_VARSTRING( UNISTR("prototype._class"), UNISTR("\"Number\""), jseDontEnum ),
      JSE_VARSTRING( UNISTR("prototype._value"), UNISTR("0.0"), jseDontEnum ),
      JSE_VARNUMBER( UNISTR("MAX_VALUE"), &jse_DBL_MAX, jseDontEnum | jseDontDelete | jseReadOnly ),
      JSE_VARNUMBER( UNISTR("MIN_VALUE"), &jse_DBL_MIN, jseDontEnum | jseDontDelete | jseReadOnly ),
      JSE_VARNUMBER( UNISTR("NaN"), &seNaN, jseDontEnum | jseDontDelete | jseReadOnly ),
      JSE_VARNUMBER( UNISTR("NEGATIVE_INFINITY"), &seNegInfinity, jseDontEnum | jseDontDelete | jseReadOnly ),
      JSE_VARNUMBER( UNISTR("POSITIVE_INFINITY"), &seInfinity, jseDontEnum | jseDontDelete | jseReadOnly ),
      JSE_PROTOMETH( CONSTRUCTOR_PROPERTY, Ecma_Number_builtin,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( TOSTRING_PROPERTY, Ecma_Number_toString,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_PROTOMETH( VALUEOF_PROPERTY, Ecma_Number_valueOf,0,0, jseDontEnum,  jseFunc_Secure ),
      JSE_ATTRIBUTE( ORIG_PROTOTYPE_PROPERTY, jseDontEnum | jseReadOnly | jseDontDelete ),
#  endif

   JSE_FUNC_DESC_END
};

   void NEAR_CALL
InitializeLibrary_Ecma_Objects(jseContext jsecontext)
{
   /* needed because the compiler doesn't recognize constants correctly */
   #ifdef JSE_ECMA_NUMBER
      seNaN = jseNaN;
      seInfinity = jseInfinity;
      seNegInfinity = jseNegInfinity;
   #endif

   jseAddLibrary(jsecontext,NULL,ObjectLibFunctionList,NULL,NULL,NULL);
}
#endif /* ECMA objects */


#if defined(JSE_ECMA_ARRAY)    || \
    defined(JSE_ECMA_BOOLEAN)  || \
    defined(JSE_ECMA_BROWSER)  || \
    defined(JSE_ECMA_BUFFER)   || \
    defined(JSE_ECMA_DATE)     || \
    defined(JSE_ECMA_FUNCTION) || \
    defined(JSE_ECMA_NUMBER)   || \
    defined(JSE_ECMA_OBJECT)   || \
    defined(JSE_ECMA_STRING)
    
/* Creates a blank object of one of the default types. It doesn't
 * check the types, so you can add new ones. For this to work as
 * expected, you need to have already set up the type in the global
 * context. If you don't, this new object will have prototype that
 * doesn't implement anything. 
 */
   jseVariable
CreateNewObject(jseContext jsecontext,const jsechar *objname)
{
   jseVariable var = jseCreateVariable(jsecontext,jseTypeObject);
   
   if ( objname != OBJECT_PROPERTY )
   {
      jseInitializeObject(jsecontext,var,objname);

#     if defined(JSE_ECMA_ARRAY)
      /* All arrays must have the special put property. I don't think it is a good
       * idea for these properties to be inherited, as they are supposed to be internal
       * properties of the object. 
       */
      if( objname == ARRAY_PROPERTY )
      {
         jseVariable t2;
         t2 = jseMemberWrapperFunction(jsecontext,var,PUT_PROPERTY,Ecma_Array_put,2,2,jseDontEnum | jseDontDelete | jseReadOnly,0,NULL);

         /* set up its length */
         t2 = MyjseMember(jsecontext,var,LENGTH_PROPERTY,jseTypeNumber);
         jsePutLong(jsecontext,t2,0);
         jseSetAttributes(jsecontext,t2,jseDontEnum);
      }
#     endif
   }
   return var;
}

/* NOTE: For most string routines, it is an error if the 'this' is not
 * actually a string. This routine checks for it and bombs out if it
 * fails.
 */
#if defined(JSE_ECMA_DATE)    || \
    defined(JSE_ECMA_BOOLEAN) || \
    defined(JSE_ECMA_STRING)  || \
    defined(JSE_ECMA_NUMBER) 
    
   jsebool
ensure_type(jseContext jsecontext,jseVariable what,const jsechar *type)
{
   jsebool success = TRUE;

   if( jseGetType(jsecontext,what)!=jseTypeObject )
      success = FALSE;
   else
   {
      jseVariable v = jseGetMember(jsecontext,what,CLASS_PROPERTY);
      jsechar *str = (jsechar *)jseGetString(jsecontext,v,NULL);
      
      if( v==NULL || jseGetType(jsecontext,v)!=jseTypeString ||
          strcmp_jsechar(type,str)!=0 )
         success = FALSE;
   }

   if( !success )
   {
      jsechar buffer[256];

      sprintf_jsechar(buffer,UNISTR("'this' for %s method is not really a %s object."),type,type);
      jseLibErrorPrintf(jsecontext,buffer);
   }

   return success;
}

#endif

#if defined(JSE_ECMA_ARRAY) || \
    defined(JSE_ECMA_DATE)  || \
    defined(JSE_BROWSEROBJECTS)
jseVariable MyjseMember(jseContext jsecontext,jseVariable obj,const jsechar *name,jseDataType t)
{
   jseVariable var = jseMember(jsecontext,obj,name,t);
   jseSetAttributes(jsecontext,var,(jseVarAttributes)
                    (jseGetAttributes(jsecontext,var)&~jseReadOnly)); /* make sure not readonly */
   if( t!=jseGetType(jsecontext,var)) jseConvert(jsecontext,var,t);
   return var;
}

#endif

#endif /* ECMA objects */

ALLOW_EMPTY_FILE
