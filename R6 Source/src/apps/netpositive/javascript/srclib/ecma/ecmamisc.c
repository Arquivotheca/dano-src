/* ecmamisc.c 
 *
 * Set of miscellaneous global functions for the Ecma library.  Contains source
 * for the following:
 *   eval, parseInt, parseFloat, escape, unescape, isNaN, isFinite
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

/* eval() */
#if defined(JSE_ECMA_EVAL)
static jseLibFunc(Ecma_eval)
{
   jseVariable v = jseFuncVar(jsecontext,0);
   jseVariable ReturnVar;
   const jsechar *text;
   
   if( v==NULL || jseGetType(jsecontext,v)!=jseTypeString )
   {
      jseReturnVar(jsecontext,jseCreateSiblingVariable(jsecontext,v,0),jseRetTempVar);
      return;
   }

   text = (const jsechar *)jseGetString(jsecontext,v,NULL);
   if ( !jseInterpret(jsecontext,NULL,text,NULL,
                      jseNewNone,JSE_INTERPRET_CALL_MAIN,
                      jsePreviousContext(jsecontext),&ReturnVar))
   {
      assert( NULL == ReturnVar );
      /* if erro while evaluating then create an undefined var to return */
      ReturnVar = jseCreateVariable(jsecontext,jseTypeUndefined);
   }
   else
   {
      assert( NULL != ReturnVar );
   }
   jseReturnVar(jsecontext,ReturnVar,jseRetTempVar);
}
#endif

/* parseInt() */
#if defined(JSE_ECMA_PARSEINT)
static jseLibFunc(Ecma_parseInt)
{
   jseVariable strvar;
   const jsechar *str;
   int sign = 1;
   long radix = 0;
   jsenumber result = jseNaN;

   if ( 0 < jseFuncVarCount(jsecontext) )
   {
      JSE_FUNC_VAR_NEED(strvar,jsecontext,0,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_STRING));
      str = jseGetString( jsecontext, strvar, NULL);
      
      SKIP_WHITESPACE(str);

      if( str[0]=='-' || str[0]=='+' )
      {
         if( str[0]=='-' )
            sign = -1;
         str++;
      }


      if( 1 < jseFuncVarCount(jsecontext) )
      {
         jseVariable r = jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,1),
                                                    jseToInt32);

         radix = jseGetLong(jsecontext,r);
         jseDestroyVariable(jsecontext,r);

         if( radix != 0 && (radix<2 || radix>36) )
         {
            result = jseNaN; radix = -1;
         }
      }

      if( radix!=-1 )
      {
         int ok;

         if( radix==0 )
         {
            radix = 10;

            if( '0' == str[0] )
            {
               radix = 8;

               if( 'X' == toupper_jsechar(str[1]) )
               {
                  str+=2; radix = 16;
               }
            }
         }

         result = 0.0;
         ok = 0;
         while( str[0] )
         {
            int val;

            if( isdigit_jsechar(str[0]))
            {
               val = str[0]-'0';
            }
            else
            {
               val = toupper_jsechar(str[0])-'A'+10;
            }
            str++;

            if( val<0 || val>radix-1 ) break;
            ok = 1;
            result = radix*result + val;
         }
         if( !ok ) result = jseNaN;
      }
   }
   jseReturnNumber(jsecontext,result*sign);
}
#endif


/* parseFloat() */
#ifdef JSE_ECMA_PARSEFLOAT
/* This function is basically identical to 'ToNumber' for strings. */
static jseLibFunc(Ecma_parseFloat)
{
   jseVariable strvar;
   const jsechar *str;
   jsechar *parseEnd;
   jsenumber val;
   
   // seb 99.2.2 - We should initialize this.
   jsebool neg = False;

   if ( jseFuncVarCount(jsecontext) < 1 )
   {
      val = jseNaN;
   }
   else
   {
      JSE_FUNC_VAR_NEED(strvar,jsecontext,0,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_STRING));
      str = jseGetString( jsecontext, strvar, NULL);
      
      SKIP_WHITESPACE(str);

      if ( '-' == *str )
      {
         str++;
         neg = True;
      }
      else if ( '+' == *str )
      {
         str++;
         neg = False;
      }

      val = strtod_jsechar(str,&parseEnd);
      if ( val == 0 )
      {
         if ( parseEnd == str )
         {
            /* either parsed nothing, or Infinity */
            val = strncmp_jsechar(str,UNISTR("Infinity"),8) ? jseNaN : jseInfinity ;
         }
      }
      if ( neg  &&  !jseIsNaN(val) )
         val = -val;
   }
   jseReturnNumber(jsecontext,val);
}
#endif


#  define CHARS_OK UNISTR("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@*_+-./")

/* escape() */
#ifdef JSE_ECMA_ESCAPE
static jseLibFunc(Ecma_escape)
{
   jseVariable ret;

   if ( jseFuncVarCount(jsecontext) < 1 )
   {
      /* if no parameter passed in then return string "undefined" */
      ret = jseCreateVariable(jsecontext,jseTypeString);
      jsePutString(jsecontext,ret,UNISTR("undefined"));
   }
   else
   {
      jseVariable strvar;
      const jsechar *str;
      JSE_POINTER_UINDEX srcLength;

      JSE_FUNC_VAR_NEED(strvar,jsecontext,0,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_STRING));
      str = jseGetString( jsecontext, strvar, &srcLength);

      ret = jseCreateVariable(jsecontext,jseTypeString);

      if ( 0 != srcLength )
      {
         jsechar *string = NULL;
         uint x, count = 0;
         jsechar c;

         for( x=0;x<srcLength;x++ )
         {
            if( (0 != (c=str[x]))  &&  strchr_jsechar(CHARS_OK,c))
            {
               string = jseMustReMalloc(jsechar,string,(count+2)*sizeof(jsechar));
               string[count++] = c;
            }
            else
            {
               jsechar buffer[10];
               sprintf_jsechar(buffer,UNISTR("%c%02X"),'%',c);
               string = jseMustReMalloc(jsechar,string,(count+4)*sizeof(jsechar));
               string[count++] = buffer[0];
               string[count++] = buffer[1];
               string[count++] = buffer[2];
            }
         }
         assert( NULL != string );
         jsePutStringLength(jsecontext,ret,string,count);
         jseMustFree(string);
      }
   }

   jseReturnVar(jsecontext,ret,jseRetTempVar);
}
#endif

/* unescape() */
#ifdef JSE_ECMA_UNESCAPE
static jseLibFunc(Ecma_unescape)
{
   jseVariable ret;

   if ( jseFuncVarCount(jsecontext) < 1 )
   {
      /* if no parameter passed in then return string "undefined" */
      ret = jseCreateVariable(jsecontext,jseTypeString);
      jsePutString(jsecontext,ret,UNISTR("undefined"));
   }
   else
   {
      jseVariable strvar;
      const jsechar *str;
      jsechar *string = NULL;
      JSE_POINTER_UINDEX count, x, strLength;

      JSE_FUNC_VAR_NEED(strvar,jsecontext,0,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_STRING));
      str = (const jsechar *)jseGetString(jsecontext,strvar,&strLength);

      ret = jseCreateVariable(jsecontext,jseTypeString);

      count = 0;
      for( x=0; x < strLength; x++ )
      {
         string = jseMustReMalloc(jsechar,string,(count+1)*sizeof(jsechar));
         if( '%' == (string[count++] = str[x]) )
         {
            int v1,v2;

            if( str[x+1]>='A' ) v1 = toupper_jsechar(str[x+1])-'A'+10;
            else v1 = str[x+1]-'0';
            if( str[x+2]>='A' ) v2 = toupper_jsechar(str[x+2])-'A'+10;
            else v2 = str[x+2]-'0';

            x += 2;
            string[count-1] = (jsechar)(v1*16+v2);
         }
      }

      jsePutStringLength(jsecontext,ret,string,count);
      if ( NULL != string )
         jseMustFree(string);
   }

   jseReturnVar(jsecontext,ret,jseRetTempVar);
}
#endif

/* isNaN() */
#ifdef JSE_ECMA_ISNAN
static jseLibFunc(Ecma_isNaN)
{
   jseVariable num;
   jsenumber val;
   jseVariable retVar;

   JSE_FUNC_VAR_NEED(num,jsecontext,0,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
   val = jseGetNumber(jsecontext,num);
   retVar = jseCreateVariable(jsecontext,jseTypeBoolean);
   jsePutBoolean(jsecontext,retVar, jseIsNaN(val) );
   jseReturnVar(jsecontext,retVar,jseRetTempVar);
}
#endif

/* isFinite() */
#ifdef JSE_ECMA_ISFINITE
static jseLibFunc(Ecma_isFinite)
{
   jseVariable num;
   jsenumber val;
   jseVariable retVar;

   JSE_FUNC_VAR_NEED(num,jsecontext,0,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
   val = jseGetNumber(jsecontext,num);
   retVar = jseCreateVariable(jsecontext,jseTypeBoolean);
   jsePutBoolean(jsecontext,retVar, jseIsFinite(val) );
   jseReturnVar(jsecontext,retVar,jseRetTempVar);
}
#endif

#if defined(JSE_ECMA_EVAL)       || \
    defined(JSE_ECMA_PARSEINT)   || \
    defined(JSE_ECMA_PARSEFLOAT) || \
    defined(JSE_ECMA_ESCAPE)     || \
    defined(JSE_ECMA_UNESCAPE)   || \
    defined(JSE_ECMA_ISNAN)      || \
    defined(JSE_ECMA_ISFINITE)
    
static CONST_DATA(struct jseFunctionDescription) EcmaMiscFunctionList[] =
{
JSE_LIBMETHOD( UNISTR("eval"),        Ecma_eval,          1,      1,      jseDontEnum,  jseFunc_Secure ),
JSE_LIBMETHOD( UNISTR("parseInt"),    Ecma_parseInt,      0,      2,      jseDontEnum,  jseFunc_Secure ),
JSE_LIBMETHOD( UNISTR("parseFloat"),  Ecma_parseFloat,    0,      1,      jseDontEnum,  jseFunc_Secure ),
JSE_LIBMETHOD( UNISTR("escape"),      Ecma_escape,        0,      1,      jseDontEnum,  jseFunc_Secure ),
JSE_LIBMETHOD( UNISTR("unescape"),    Ecma_unescape,      0,      1,      jseDontEnum,  jseFunc_Secure ),
JSE_LIBMETHOD( UNISTR("isNaN"),       Ecma_isNaN,         1,      1,      jseDontEnum,  jseFunc_Secure ),
JSE_LIBMETHOD( UNISTR("isFinite"),    Ecma_isFinite,      1,      1,      jseDontEnum,  jseFunc_Secure ),
JSE_FUNC_END
};

void NEAR_CALL InitializeLibrary_Ecma_Misc(jseContext jsecontext)
{
   jseAddLibrary(jsecontext,NULL,EcmaMiscFunctionList,NULL,NULL,NULL);
}

#endif

ALLOW_EMPTY_FILE
