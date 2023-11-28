/* seshare.c  Shared Data access
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

static CONST_DATA(jsechar) globalMemberName[] = UNISTR("**Special Global Lib Var**");

   static jseContext NEAR_CALL
FindRootContext(jseContext jsecontext)
{
   while ( NULL != jsePreviousContext(jsecontext) )
      jsecontext = jsePreviousContext(jsecontext);
   return jsecontext;
}

   static jseVariable NEAR_CALL
FindSharedVar(jseContext rootcontext,const jsechar *dataName)
   /* return variable for reading or writing; caller must free it */
{
   jseVariable glob, datavar;

   assert( NULL != rootcontext );
   assert( rootcontext == FindRootContext(rootcontext) );

   glob = jseGetMemberEx(rootcontext,NULL,globalMemberName,
                         jseCreateVar|jseDontSearchPrototype);
   if( NULL == glob )/* Wasn't found, so create it */
   {
      glob = jseMemberEx(rootcontext,NULL,globalMemberName,jseTypeObject,jseCreateVar);
      jseSetAttributes(rootcontext,glob,jseDontEnum|jseDontDelete);
   }

   datavar = jseMemberEx(rootcontext,glob,dataName,jseTypeNumber,
                         jseCreateVar|jseDontSearchPrototype);
   assert( NULL != datavar );
   jseDestroyVariable(rootcontext,glob);
   return datavar;
}

   void _FAR_ *
jseGetSharedData(jseContext jsecontext, const jsechar *name)
{
   jseContext rootcontext = FindRootContext(jsecontext);
   jseVariable dataVar = FindSharedVar(rootcontext,name);
   void _FAR_ *data = (void _FAR_ *)jseGetLong(rootcontext,dataVar);
   jseDestroyVariable(rootcontext,dataVar);
   return data;
}

struct cleanupSharedDataStr_
{
   void _FAR_ *data;
   jseShareCleanupFunc cleanupFunc;
};

static jseLibTermFunc(cleanupSharedData)
{
   struct cleanupSharedDataStr_ * data = (struct cleanupSharedDataStr_ *) InstanceLibraryData;
   if ( jsecontext == FindRootContext(jsecontext) )
   {
      (*(data->cleanupFunc))(jsecontext,data->data);
      jseMustFree(data);
   }
}

static CONST_DATA(struct jseFunctionDescription) seshareDummyFuncList[] = {
   JSE_FUNC_END
};

   void
jseSetSharedData(jseContext jsecontext, const jsechar *name, void _FAR_ *data,
                 jseShareCleanupFunc cleanupFunc)
{
   jseContext rootcontext = FindRootContext(jsecontext);
   jseVariable dataVar = FindSharedVar(rootcontext,name);
   jsePutLong(rootcontext,dataVar,(slong)data);
   jseDestroyVariable(rootcontext,dataVar);

   if ( NULL != cleanupFunc )
   {
      /* create function to be called when libraries all clean up.
       * The trick here is to add a dummy library.  Its cleanup
       * function will cleanup when this variable goes away.
       */
      struct cleanupSharedDataStr_ *cleanup
      = jseMustMalloc(struct cleanupSharedDataStr_,sizeof(struct cleanupSharedDataStr_));
      cleanup->data = data;
      cleanup->cleanupFunc = cleanupFunc;
      jseAddLibrary(rootcontext,NULL,seshareDummyFuncList,
                    cleanup,NULL,cleanupSharedData);
   }
}
