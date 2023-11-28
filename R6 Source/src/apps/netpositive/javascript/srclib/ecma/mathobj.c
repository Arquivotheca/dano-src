/* mathobj.c
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

// seb 98.11.12 -- Added
#if defined(__JSE_BEOS__)
#include <math.h>
#endif

#ifdef JSE_ECMA_MATH

   static jsenumber NEAR_CALL
GetNumberFromStack(jseContext jsecontext,uint ParameterOffset)
{
   jseVariable var;
   var = jseFuncVarNeed(jsecontext,ParameterOffset,JSE_VN_CONVERT(JSE_VN_ANY,JSE_VN_NUMBER));
   return ( NULL == var ) ? jseNaN : jseGetNumber(jsecontext,var) ;
}

#  if !defined(JSE_FLOATING_POINT) || (0==JSE_FLOATING_POINT)
#    error  JSE_FLOATING_POINT must be defined to use Ecma Math object
#  endif

/* Math.abs() */
static jseLibFunc(Ecma_Math_abs)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   jseReturnNumber(jsecontext,jseIsNaN(val)?jseNaN:fabs(val));
}

/* now the methods for the math object */
/* Math.acos() */
static jseLibFunc(Ecma_Math_acos)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   if ( !jseIsNaN(val) )
   {
      if ( 1 < val  ||  val < -1 )
      {
         val = jseNaN;
      }
      else
      {
         val = acos(val);
      }
   }
   jseReturnNumber(jsecontext,val);
}

/* Math.asin() */
static jseLibFunc(Ecma_Math_asin)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   if ( !jseIsNaN(val) )
   {
      if ( 1 < val  ||  val < -1 )
      {
         val = jseNaN;
      }
      else
      {
         val = asin(val);
      }
   }
   jseReturnNumber(jsecontext,val);
}

/* Math.atan() */
static jseLibFunc(Ecma_Math_atan)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   jseReturnNumber(jsecontext,jseIsNaN(val)?val:atan(val));
}

/* Math.atan2() */
static jseLibFunc(Ecma_Math_atan2)
{
   jsenumber y = GetNumberFromStack(jsecontext,0);
   jsenumber x = GetNumberFromStack(jsecontext,1);
   jsenumber val;

   if ( jseIsNaN(y) || jseIsNaN(x) )
   {
      val = jseNaN;
   }
   else
   {
      val = atan2(y,x);
   }
   jseReturnNumber(jsecontext,val);
}

/* Math.ceil() */
static jseLibFunc(Ecma_Math_ceil)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   jseReturnNumber(jsecontext,jseIsFinite(val)?ceil(val):val);
}

/* Math.cos() */
static jseLibFunc(Ecma_Math_cos)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   jseReturnNumber(jsecontext,jseIsFinite(val)?cos(val):jseNaN);
}

/* Math.exp() */
static jseLibFunc(Ecma_Math_exp)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   if ( jseIsFinite(val) )
   {
      val = exp(val);
   }
   else if ( jseIsNegInfinity(val) )
   {
      val = 0;
   }
   jseReturnNumber(jsecontext,val);
}

/* Math.floor() */
static jseLibFunc(Ecma_Math_floor)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   jseReturnNumber(jsecontext,jseIsFinite(val)?floor(val):val);
}

/* Math.log() */
static jseLibFunc(Ecma_Math_log)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   if ( jseIsFinite(val) )
   {
      if ( val < 0 )
         val = jseNaN;
      else if ( val == 0 )
         val = jseNegInfinity;
      else
         val = log(val);
   }
   else
   {
      if ( jseIsNegInfinity(val) )
         val = jseNaN;
   }
   jseReturnNumber(jsecontext,val);
}

/* Math.max() */
static jseLibFunc(Ecma_Math_max)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   jsenumber twonum = GetNumberFromStack(jsecontext,1);
   if ( jseIsNaN(val) || jseIsNaN(twonum) )
      val = jseNaN;
   else if ( val < twonum )
      val = twonum;
   jseReturnNumber(jsecontext,val);
}

/* Math.min() */
static jseLibFunc(Ecma_Math_min)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   jsenumber twonum = GetNumberFromStack(jsecontext,1);
   if ( jseIsNaN(val) || jseIsNaN(twonum) )
      val = jseNaN;
   else if ( twonum < val )
      val = twonum;
   jseReturnNumber(jsecontext,val);
}

/* Math.pow() */
static jseLibFunc(Ecma_Math_pow)
{
   jsenumber x = GetNumberFromStack(jsecontext,0);
   jsenumber y = GetNumberFromStack(jsecontext,1);
   jsenumber val;

   if ( !jseIsFinite(y) || !jseIsFinite(x) )
   {
      if ( jseIsNaN(y) )
      {
         val = jseNaN;
      }
      else if ( jseIsFinite(y)  &&  y == 0 )
      {
         val = 1;
      }
      else if ( jseIsNaN(x) )
      {
         val = jseNaN;
      }
      else if ( jseIsInfOrNegInf(y) )
      {
         jsenumber absx
#        if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
            = fabs(x);
#        else
            = abs(x);
#        endif
         if ( absx == 1 )
         {
            val = jseNaN;
         }
         else if ( absx > 1  ||  jseIsInfinity(x) )
         {
            val = jseIsInfinity(y) ? jseInfinity : 0 ;
         }
         else
         {
            val = jseIsInfinity(y) ? 0 : jseInfinity ;
         }
      }
      else if ( jseIsInfinity(x) )
      {
         val = ( y > 0 ) ? jseInfinity : 0 ;
      }
      else
      {
         assert( jseIsNegInfinity(x) );
         if ( (jsenumber)((slong)y) == y  &&  ((slong)y) & 1 )
         {
            /* y is an odd integer */
            val = ( y > 0 ) ? jseNegInfinity : jseNegZero ;
         }
         else
         {
            /* y is not an odd integer */
            val = ( y > 0 ) ? jseInfinity : 0 ;
         }
      }
   }
   else if ( y == 0 )
   {
      val = 1;
   }
   else if ( x == 0 )
   {
      assert( y != 0 );
      if ( jseIsPosZero(x) )
      {
         val = ( 0 < y ) ? 0 : jseInfinity ;
      }
      else
      {
         slong yInteger = (slong)y;
         jsebool yOddInteger = ( ((jsenumber)yInteger == y)  &&  (yInteger & 1) );
         assert( jseIsNegZero(x) );
         if ( y < 0 )
         {
            val = yOddInteger ? jseNegInfinity : jseInfinity ;
         }
         else
         {
            val = yOddInteger ? jseNegZero : 0 ;
         }
      }
   }
   else if ( x < 0  &&  (jsenumber)((slong)y) != y )
   {
      val = jseNaN;
   }
   else
   {
      val = pow(x,y);
   }
   jseReturnNumber(jsecontext,val);
   jseReturnNumber(jsecontext,val);
}

/* Math.random() */
/* This routine no longer relies on the way IEEE doubles are stored. */
static jseLibFunc(Ecma_Math_random)
{
  uword32 low, high;
  double num;
  double denom = ((double)0x7ffffL)*((double)0xffffffffL) + (double)0xffffffffL;

  /* get 15 bits of random value at a time, need 51 bits */
  int r1 = rand(),r2 = rand(),r3 = rand(),r4 = rand(),r5 = rand();

  low = (uword32) (r1 | (r2<<15) | (r3<<30));
  high = (uword32) ((r4 | (r5<<15)) & 0x7ffffL);

  num = ((double)high)*((double)0xffffffffL) + (double)low;
                                        
  jseReturnNumber(jsecontext,num/denom);
}

/* Math.round() */
static jseLibFunc(Ecma_Math_round)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   jseReturnNumber(jsecontext,jseIsFinite(val)?floor(val+0.5):val);
}

static jseLibFunc(Ecma_Math_sin)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   jseReturnNumber(jsecontext,jseIsFinite(val)?sin(val):jseNaN);
}

/* Math.sqrt() */
static jseLibFunc(Ecma_Math_sqrt)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   if ( jseIsFinite(val) )
   {
      val = ( val < 0 ) ? jseNaN : sqrt(val) ;
   }
   else
   {
      if ( jseIsNegInfinity(val) )
         val = jseNaN;
   }
   jseReturnNumber(jsecontext,val);
}

/* Math.tan() */
static jseLibFunc(Ecma_Math_tan)
{
   jsenumber val = GetNumberFromStack(jsecontext,0);
   jseReturnNumber(jsecontext,jseIsFinite(val)?tan(val):jseNaN);
}

/* ---------------------------------------------------------------------- */

#define E_VALUE       UNISTR("2.7182818284590452354")
#define LN10_VALUE    UNISTR("2.302585092994046")
#define LN2_VALUE     UNISTR("0.6931471805599453")
#define LOG2E_VALUE   UNISTR("1.4426950408889634")
#define LOG10E_VALUE  UNISTR("0.4342944819032518")
#define PI_VALUE      UNISTR("3.14159265358979323846")
#define SQRT1_2_VALUE UNISTR("0.7071067811865476")
#define SQRT_VALUE    UNISTR("1.4142135623730951")

static CONST_DATA(struct jseFunctionDescription) MathLibFunctionList[] = {
 
  /* First the properties for the math object */ 
  JSE_VARSTRING(UNISTR("E"),       E_VALUE,        jseDontEnum|jseDontDelete|jseReadOnly),
  JSE_VARSTRING(UNISTR("LN10"),    LN10_VALUE,     jseDontEnum|jseDontDelete|jseReadOnly),
  JSE_VARSTRING(UNISTR("LN2"),     LN2_VALUE,      jseDontEnum|jseDontDelete|jseReadOnly),
  JSE_VARSTRING(UNISTR("LOG2E"),   LOG2E_VALUE,    jseDontEnum|jseDontDelete|jseReadOnly),
  JSE_VARSTRING(UNISTR("LOG10E"),  LOG10E_VALUE,   jseDontEnum|jseDontDelete|jseReadOnly),
  JSE_VARSTRING(UNISTR("PI"),      PI_VALUE,       jseDontEnum|jseDontDelete|jseReadOnly),
  JSE_VARSTRING(UNISTR("SQRT1_2"), SQRT1_2_VALUE,  jseDontEnum|jseDontDelete|jseReadOnly),
  JSE_VARSTRING(UNISTR("SQRT2"),   SQRT_VALUE,     jseDontEnum|jseDontDelete|jseReadOnly),

  /* now the methods for the math object */
  JSE_LIBMETHOD(UNISTR("abs"),    Ecma_Math_abs,   1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("acos"),   Ecma_Math_acos,   1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("asin"),   Ecma_Math_asin,   1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("atan"),   Ecma_Math_atan,   1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("atan2"),  Ecma_Math_atan2,  2,    2,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("ceil"),   Ecma_Math_ceil,   1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("cos"),    Ecma_Math_cos,    1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("exp"),    Ecma_Math_exp,    1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("floor"),  Ecma_Math_floor,  1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("log"),    Ecma_Math_log,    1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("max"),    Ecma_Math_max,    2,    2,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("min"),    Ecma_Math_min,    2,    2,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("pow"),    Ecma_Math_pow,    2,    2,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("sin"),    Ecma_Math_sin,    1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("sqrt"),   Ecma_Math_sqrt,   1,    1,      jseDontEnum,    jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("tan"),    Ecma_Math_tan,    1,    1,      jseDontEnum,    jseFunc_Secure ),

  JSE_LIBMETHOD(UNISTR("random"),  Ecma_Math_random,  0,       0,      jseDontEnum , jseFunc_Secure ),
  JSE_LIBMETHOD(UNISTR("round"),   Ecma_Math_round,   1,       1,      jseDontEnum , jseFunc_Secure ),
  
  
  JSE_FUNC_END
};

   static void * JSE_CFUNC
MathLibInitFunction(jseContext jsecontext,void *unused)
{
   jseVariable v;
   UNUSED_PARAMETER(unused);
   v = jseMemberEx(jsecontext,NULL,UNISTR("Math"),jseTypeObject,jseCreateVar);
   jseSetAttributes(jsecontext,v,jseDontEnum);
   jseDestroyVariable(jsecontext,v);
   return NULL;
}

   void NEAR_CALL
InitializeLibrary_Ecma_Math(jseContext jsecontext)
{
  jseAddLibrary(jsecontext,UNISTR("Math"),MathLibFunctionList,NULL,MathLibInitFunction,NULL);
}

#endif

ALLOW_EMPTY_FILE
