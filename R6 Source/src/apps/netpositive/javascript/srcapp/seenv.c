/* seenv.c     Default implementation of Environment
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

static const jsechar *
getEnv(jseContext jsecontext, const jsechar * name)
{
   UNUSED_PARAMETER(jsecontext);
   UNUSED_PARAMETER(name);
   
   return NULL;
}

static int
putEnv(jseContext jsecontext, const jsechar *name, const jsechar *value)
{
   UNUSED_PARAMETER(jsecontext);
   UNUSED_PARAMETER(name);
   UNUSED_PARAMETER(value);
   
   return 0;
}

static void
reSynch(jseContext jsecontext)
{
   UNUSED_PARAMETER(jsecontext);
}


   static void
RemoveStandardService_Environment(jseContext jsecontext,struct Environment * This)
{
   UNUSED_PARAMETER(jsecontext);

   assert( This != NULL );
   jseMustFree(This);
}

   void
AddStandardService_Environment(jseContext jsecontext)
{
   struct Environment * This = jseMustMalloc(struct Environment,sizeof(struct Environment));
   
   This->GetEnv = getEnv;
   This->PutEnv = putEnv;
   This->ReSync = reSynch;
   
   assert( ENVIRONMENT_CONTEXT == NULL );
   
   jseSetSharedData(jsecontext,ENVIRONMENT_NAME,(void _FAR_ *)This,(jseShareCleanupFunc)RemoveStandardService_Environment);
}
