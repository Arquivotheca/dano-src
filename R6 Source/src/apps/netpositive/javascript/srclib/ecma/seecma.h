/* seecma.h   Header file for ECMA library
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

#if !defined(__SEECMA_H)
#define __SEECMA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Do checks for incompatible option flags */
#if defined(JSE_ECMA_OBJECT) && !defined(JSE_ECMA_FUNCTION)
#  error You MUST define JSE_ECMA_FUNCTION to use JSE_ECMA_OBJECT
#endif
#if defined(JSE_ECMA_FUNCTION) && !defined(JSE_ECMA_OBJECT)
#  error You MUST define JSE_ECMA_OBJECT to use JSE_ECMA_FUNCTION
#endif
#if defined(JSE_ECMA_ARRAY)   || \
    defined(JSE_ECMA_BOOLEAN) || \
    defined(JSE_ECMA_BUFFER)  || \
    defined(JSE_ECMA_DATE)    || \
    defined(JSE_ECMA_NUMBER)  || \
    defined(JSE_ECMA_STRING)
#  if !defined(JSE_ECMA_OBJECT) || \
      !defined(JSE_ECMA_FUNCTION)
#     error You MUST define JSE_ECMA_OBJECT and JSE_ECMA_FUNCTION to use Ecma objects
#  endif
#endif

void NEAR_CALL InitializeLibrary_Ecma_Date(jseContext jsecontext);
void NEAR_CALL InitializeLibrary_Ecma_Math(jseContext jsecontext);
void NEAR_CALL InitializeLibrary_Ecma_Buffer(jseContext jsecontext);
void NEAR_CALL InitializeLibrary_Ecma_Browser(jseContext jsecontext);
void NEAR_CALL InitializeLibrary_Ecma_Objects(jseContext jsecontext);
void NEAR_CALL InitializeLibrary_Ecma_Misc(jseContext jsecontext);

jsebool LoadLibrary_Ecma(jseContext jsecontext);

jseVariable CreateNewObject(jseContext jsecontext,const jsechar *objname);
jseVariable MyjseMember(jseContext jsecontext,jseVariable obj,const jsechar *name,jseDataType t);

jsebool ensure_type(jseContext jsecontext,jseVariable what,const jsechar *type);

#ifdef __cplusplus
}
#endif
#endif
