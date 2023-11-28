/* seecma.c  Glue for ECMA library
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

#if defined(JSE_ECMA_ANY)

   jsebool
LoadLibrary_Ecma(jseContext jsecontext)
{
#if defined(JSE_ECMA_ARRAY)    || \
    defined(JSE_ECMA_BOOLEAN)  || \
    defined(JSE_ECMA_FUNCTION) || \
    defined(JSE_ECMA_NUMBER)   || \
    defined(JSE_ECMA_OBJECT)   || \
    defined(JSE_ECMA_STRING)
   InitializeLibrary_Ecma_Objects(jsecontext);
#endif

#if defined(JSE_ECMA_DATE)
   InitializeLibrary_Ecma_Date(jsecontext);
#endif

#if defined(JSE_ECMA_MATH)
   InitializeLibrary_Ecma_Math(jsecontext);
#endif

#if defined(JSE_ECMA_BUFFER)
   InitializeLibrary_Ecma_Buffer(jsecontext);
#endif

#if defined(JSE_ECMA_BROWSER)
   InitializeLibrary_Ecma_Browser(jsecontext);
#endif

#if defined(JSE_ECMA_EVAL)       || \
    defined(JSE_ECMA_PARSEINT)   || \
    defined(JSE_ECMA_PARSEFLOAT) || \
    defined(JSE_ECMA_ESCAPE)     || \
    defined(JSE_ECMA_UNESCAPE)   || \
    defined(JSE_ECMA_ISNAN)      || \
    defined(JSE_ECMA_ISFINITE)
   InitializeLibrary_Ecma_Misc(jsecontext);
#endif

   return True;
}

#if defined(JSETOOLKIT_LINK) && !defined(JSE_NO_AUTO_INIT)

jsebool FAR_CALL
ExtensionLoadFunc(jseContext jsecontext)
{
   return LoadLibrary_Ecma(jsecontext);
}

#endif

#endif /* JSE_ECMA_ANY */

ALLOW_EMPTY_FILE
