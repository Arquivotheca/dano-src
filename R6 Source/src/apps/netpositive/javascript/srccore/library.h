/* library.h 
 *
 * Handles 'library' functions (i.e. wrapper functions) as well as libraries 
 * (collections of such functions.)
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

#ifndef _LIBRARY_H
#define _LIBRARY_H

struct LibraryFunction
{
   struct Function function;

   struct jseFunctionDescription *FuncDesc;
   union {
      void _FAR_ * Data;
      void _FAR_ * *DataPtr;
   } LibData;  /* unique data specific to the library; doesn't change;
                * for StaticLibraryDescription this is a pointer to the data;
                * for !StatisLibraryDescription this is the data itself
                */
};


void NEAR_CALL libfuncExecute(struct LibraryFunction *libfunc,
                              struct Call *call,VarRead *Var);
  /* execute this function; if there was an error then Oops was called */
struct LibraryFunction * NEAR_CALL libfuncNew(
               struct Call *call,VarRead *ObjectToAddTo,
               struct jseFunctionDescription const *FuncDesc,
               void _FAR_ * * LibraryDataPtr);
struct LibraryFunction * NEAR_CALL libfuncNewWrapper(
               struct Call *call,const jsechar *FunctionName,
               void (JSE_CFUNC FAR_CALL *FuncPtr)(jseContext jsecontext),
               sword8 MinVariableCount, sword8 MaxVariableCount,
               jseVarAttributes VarAttributes, jseFuncAttributes FuncAttributes,
               void _FAR_ * fData);
void NEAR_CALL libfuncDelete(struct LibraryFunction *);


/* and a library itself */


struct Library
{
   struct Library *prev;
#  ifndef NDEBUG
      struct Call *RememberAddCall;
         /* assure it's context adding as it is leaving */
#   endif
   struct jseFunctionDescription const *FunctionList;
   const jsechar * ObjectVarName;
   jseLibraryInitFunction LibInit;
   jseLibraryTermFunction LibTerm;
   void _FAR_ * LibraryData;
      /* unique data specific to the library; doesn't change */
};

struct Library * NEAR_CALL libraryNew(struct Call *call,struct Library *Parent);
void NEAR_CALL libraryDelete(struct Library *lib,struct Call *call);

void NEAR_CALL libraryAddFunctions(struct Library *lib,struct Call *call,
                    const jsechar * ObjectVarName,
                    struct jseFunctionDescription const *FunctionList,
                    jseLibraryInitFunction LibInit,
                    jseLibraryTermFunction LibTerm,
                    void _FAR_ *ParentLibData);

#endif
