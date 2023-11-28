/* globldat.h
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

#ifndef _GLOBLDAT_H
#define _GLOBLDAT_H

/* The various 'standard' JS properties we have to work with. */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(JSE_PROTOTYPES) && (0!=JSE_PROTOTYPES)
   extern CONST_DATA(jsechar) PROTOTYPE_PROPERTY[];
#endif
#if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS) 
   extern CONST_DATA(jsechar) DELETE_PROPERTY[];
   extern CONST_DATA(jsechar) PUT_PROPERTY[];
   extern CONST_DATA(jsechar) CANPUT_PROPERTY[];
   extern CONST_DATA(jsechar) GET_PROPERTY[];
   extern CONST_DATA(jsechar) HASPROPERTY_PROPERTY[];
   extern CONST_DATA(jsechar) CALL_PROPERTY[];
#endif
#if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
   extern CONST_DATA(jsechar) OPERATOR_PROPERTY[];
   extern CONST_DATA(jsechar) OP_NOT_SUPPORTED_PROPERTY[];
#endif
extern CONST_DATA(jsechar) CLASS_PROPERTY[];
extern CONST_DATA(jsechar) ORIG_PROTOTYPE_PROPERTY[];
extern CONST_DATA(jsechar) VALUE_PROPERTY[];
extern CONST_DATA(jsechar) CONSTRUCT_PROPERTY[];
extern CONST_DATA(jsechar) CONSTRUCTOR_PROPERTY[];
extern CONST_DATA(jsechar) LENGTH_PROPERTY[];
extern CONST_DATA(jsechar) DEFAULT_PROPERTY[];
extern CONST_DATA(jsechar) PREFERRED_PROPERTY[];
extern CONST_DATA(jsechar) ARGUMENTS_PROPERTY[];
extern CONST_DATA(jsechar) OLD_ARGS_PROPERTY[];
extern CONST_DATA(jsechar) CALLEE_PROPERTY[];
extern CONST_DATA(jsechar) CALLER_PROPERTY[];
extern CONST_DATA(jsechar) VALUEOF_PROPERTY[];
extern CONST_DATA(jsechar) TOSTRING_PROPERTY[];
extern CONST_DATA(jsechar) PARENT_PROPERTY[];

/* Standard object names */
extern CONST_DATA(jsechar) ARRAY_PROPERTY[];
extern CONST_DATA(jsechar) DATE_PROPERTY[];
extern CONST_DATA(jsechar) OBJECT_PROPERTY[];
extern CONST_DATA(jsechar) FUNCTION_PROPERTY[];
extern CONST_DATA(jsechar) NUMBER_PROPERTY[];
extern CONST_DATA(jsechar) BUFFER_PROPERTY[];
extern CONST_DATA(jsechar) STRING_PROPERTY[];
extern CONST_DATA(jsechar) BOOLEAN_PROPERTY[];

#ifdef __cplusplus
}
#endif

#endif
