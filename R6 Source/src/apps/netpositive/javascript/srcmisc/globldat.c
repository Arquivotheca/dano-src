/* globldat.c
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

#if !defined(__JSE_WINCE__)
#   include <assert.h>
#endif
#include "jseopt.h"
#include "globldat.h"

/* All of these strings MUST be NULL-terminated. No embedded NULLs */

#if defined(JSE_PROTOTYPES) && (0!=JSE_PROTOTYPES)
   CONST_DATA(jsechar) PROTOTYPE_PROPERTY[] = UNISTR("_prototype");
#endif
#if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS) 
   CONST_DATA(jsechar) DELETE_PROPERTY[] =    UNISTR("_delete");
   CONST_DATA(jsechar) PUT_PROPERTY[] =       UNISTR("_put");
   CONST_DATA(jsechar) CANPUT_PROPERTY[] =    UNISTR("_canPut");
   CONST_DATA(jsechar) GET_PROPERTY[] =       UNISTR("_get");
   CONST_DATA(jsechar) HASPROPERTY_PROPERTY[] = UNISTR("_hasProperty");
   CONST_DATA(jsechar) CALL_PROPERTY[] =      UNISTR("_call");
#endif
#if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
   CONST_DATA(jsechar) OPERATOR_PROPERTY[] =  UNISTR("_operator");
   CONST_DATA(jsechar) OP_NOT_SUPPORTED_PROPERTY[] = UNISTR("OPERATOR_DEFAULT_BEHAVIOR");
#endif

CONST_DATA(jsechar) CLASS_PROPERTY[] =     UNISTR("_class");
CONST_DATA(jsechar) VALUE_PROPERTY[] =     UNISTR("_value");
CONST_DATA(jsechar) CONSTRUCT_PROPERTY[] = UNISTR("_construct");
CONST_DATA(jsechar) CONSTRUCTOR_PROPERTY[] = UNISTR("constructor");
CONST_DATA(jsechar) DEFAULT_PROPERTY[] =   UNISTR("_defaultValue");
CONST_DATA(jsechar) PARENT_PROPERTY[] =    UNISTR("__parent__");

CONST_DATA(jsechar) ORIG_PROTOTYPE_PROPERTY[] = UNISTR("prototype");
CONST_DATA(jsechar) LENGTH_PROPERTY[] =    UNISTR("length");
CONST_DATA(jsechar) PREFERRED_PROPERTY[] = UNISTR("preferredType");
CONST_DATA(jsechar) ARGUMENTS_PROPERTY[] = UNISTR("arguments");
CONST_DATA(jsechar) OLD_ARGS_PROPERTY[] =  UNISTR("oldArguments");
CONST_DATA(jsechar) CALLEE_PROPERTY[] =    UNISTR("callee");
CONST_DATA(jsechar) CALLER_PROPERTY[] =    UNISTR("caller");
CONST_DATA(jsechar) VALUEOF_PROPERTY[] =   UNISTR("valueOf");
CONST_DATA(jsechar) TOSTRING_PROPERTY[] =  UNISTR("toString");

CONST_DATA(jsechar) OBJECT_PROPERTY[] =    UNISTR("Object");
CONST_DATA(jsechar) FUNCTION_PROPERTY[] =  UNISTR("Function");
CONST_DATA(jsechar) ARRAY_PROPERTY[] =     UNISTR("Array");
CONST_DATA(jsechar) NUMBER_PROPERTY[] =    UNISTR("Number");
CONST_DATA(jsechar) BUFFER_PROPERTY[] =    UNISTR("Buffer");
CONST_DATA(jsechar) STRING_PROPERTY[] =    UNISTR("String");
CONST_DATA(jsechar) BOOLEAN_PROPERTY[] =   UNISTR("Boolean");
CONST_DATA(jsechar) DATE_PROPERTY[] =      UNISTR("Date");

/* ----------------------------------------------------------------------
 * special math values
 * ---------------------------------------------------------------------- */

/* two 32-bit values per, note these are little endian so the
 * least significant long is first. These do not change among
 * threads.
 */

#if !defined(JSE_FLOATING_POINT) || (0==JSE_FLOATING_POINT)

   CONST_DATA(jsenumber) jse_special_math[5] = {
      0L,          /* 0 */
      0L,          /* -0 */
      0x7FFFFFFFL, /* Infinity */
      0x80000001L, /* neginfinity */
      0x80000000L  /* NaN */
   };

#else
   /* floating point */

#  if defined(__JSE_UNIX__)

      /* unix initialization of special_math occurs in jseengin.cpp */
      VAR_DATA(jsenumber) jse_special_math[5];

#  else

#     if BIG_ENDIAN==True

         CONST_DATA(uword32) jse_special_math[10] = {
           0x00000000L, 0x00000000L, /* 0 */
           JSE_NEGNUM_BIT, 0x00000000L, /* -0 */
           JSE_INF_HIGH, JSE_NOTFINITE_LOW, /* infinity */
           JSE_NEGNUM_BIT | JSE_INF_HIGH, JSE_NOTFINITE_LOW, /* -infinity */
           JSE_NAN_HIGH, JSE_NOTFINITE_LOW  /* NaN */
         };

#     else

      /* This information can be found on the Watcom help system, using
       * 'find' type in 'infinity' and choose '32-bit: Type Double"
       */
         CONST_DATA(uword32) jse_special_math[10] = {
           0x00000000L, 0x00000000L, /* 0 */
           0x00000000L, JSE_NEGNUM_BIT, /* -0 */
           JSE_NOTFINITE_LOW, JSE_INF_HIGH, /* infinity */
           JSE_NOTFINITE_LOW, JSE_NEGNUM_BIT | JSE_INF_HIGH, /* -infinity */
           JSE_NOTFINITE_LOW, JSE_NAN_HIGH  /* NaN */
         };

#     endif

#  endif

#endif


/* ----------------------------------------------------------------------
 * retrieving float from the core in a compiler-independent way
 * ---------------------------------------------------------------------- */

#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT) && \
    !defined(JSETOOLKIT_CORE) && !defined(JSETOOLKIT_LINK)
jsenumber jseGetNumber(jseContext jsecontext ,jseVariable variable)
{
   jsenumber GetFloat;
   jseGetFloatIndirect(jsecontext,variable,&GetFloat);
   return GetFloat;
}
#endif
