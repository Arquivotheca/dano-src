/* seobjfun.h    Common utilities for manipulating objects
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

/* Initialize blank object, copying prototypes */
jsebool
jseInitializeObject(jseContext jsecontext, jseVariable object, const jsechar *objName)
{
   jseVariable gl_obj, proto, orig_proto;
   jsebool success = False;

   gl_obj = jseFindVariable(jsecontext,objName,jseCreateVar);
   if( NULL != gl_obj)
   {
      orig_proto = jseMemberEx(jsecontext,gl_obj,ORIG_PROTOTYPE_PROPERTY,jseTypeObject,jseCreateVar|jseDontCreateMember);
      if( orig_proto != NULL )
      { 
         proto = jseMemberEx(jsecontext,object,PROTOTYPE_PROPERTY,jseTypeObject,jseCreateVar);
      
         jseAssign(jsecontext,proto,orig_proto);
         jseSetAttributes(jsecontext,proto,jseDontEnum);
      
         success = True;

         jseDestroyVariable(jsecontext,proto);
         jseDestroyVariable(jsecontext,orig_proto);
      }
   
      jseDestroyVariable(jsecontext,gl_obj);
   }
   
   return success;
}


/* Construct a new standard object, calling _construct property, etc */
jsebool 
jseConstructObject(jseContext jsecontext, jseVariable object, const jsechar *objName,
                   jseStack stack)
{
   jseVariable gl_obj, temp, funcobj;
   jsebool success = False;

   if ( jseInitializeObject(jsecontext,object,objName) )
   {
      gl_obj = jseFindVariable(jsecontext,objName,jseCreateVar);
      if( NULL != gl_obj )
      {
         funcobj = temp = jseMemberEx(jsecontext,gl_obj,CONSTRUCT_PROPERTY,jseTypeObject,jseCreateVar|jseDontCreateMember);
         
         if( funcobj == NULL || !jseIsFunction(jsecontext,funcobj) )
         {
            funcobj = jseIsFunction(jsecontext,gl_obj) ? gl_obj : NULL ;
         }
         
         if( funcobj != NULL )
         {
            jseVariable retvar;
            
            if( jseCallFunction(jsecontext,funcobj,stack,&retvar,object) )
            {
               if ( jseTypeObject == jseGetType(jsecontext,retvar) )
               {
                  /* constructore returned object instead of working with default */
                  jseAssign(jsecontext,object,retvar);
               }
               success = True;
            }
         }     

         if ( NULL != temp )
            jseDestroyVariable(jsecontext,temp);
         jseDestroyVariable(jsecontext,gl_obj);
      }
   }
   return success;
}

// seb 98.12.30 - We need to extract the function structure out of the function
// variable we got back and reassign its stored_in field.  This was set to a temporary
// in the ECMA Function constructor that has since been destroyed, and we will crash if
// we try to call this function and we access this now deleted variable.  We'll re-set
// the value to the function's new home as a property in the container.
void jseSetFunctionParent(jseContext jsecontext, jseVariable parent, const jsechar *funcName,
						  jseVariable funcVar)
{
	jseVariable var = GET_READABLE_VAR(funcVar, jsecontext);
	struct Function* func = varGetFunction(var, jsecontext);
	if (func) {
		jseVariable newloc = jseMemberEx(jsecontext, parent, funcName, jseTypeUndefined,jseCreateVar);
		jseSetAttributes(jsecontext, newloc, jseImplicitThis | jseImplicitParents);
		
		func->stored_in = var;
		jseAssign(jsecontext, newloc, var);
		jseDestroyVariable(jsecontext, newloc);
	}
	VAR_REMOVE_USER(funcVar, jsecontext);
}


void jseSetFunctionGlobal(jseContext jsecontext, jseVariable global, jseVariable funcVar)
{
	jseVariable var = GET_READABLE_VAR(funcVar, jsecontext);
	struct Function* func = varGetFunction(var, jsecontext);
	if (func) {
		if (global) {
			func->global_object = global;
		}
	}
	VAR_REMOVE_USER(funcVar, jsecontext);
}
