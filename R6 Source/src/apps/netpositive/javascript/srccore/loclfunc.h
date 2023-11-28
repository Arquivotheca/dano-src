/* loclfunc.h
 *
 * Handles 'local' functions. These are functions written in Javascript as 
 * opposed to wrapper functions in compiled C/C++.
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

#ifndef _LOCLFUNC_H
#define _LOCLFUNC_H

struct Code;

struct LocalFunction
{
   struct Function function;

#  if (0!=JSE_COMPILER)
      /* The list of tokens and the compiled code it is translated into */
      struct Code *FirstCode; /* this may be NULL if freed after compiling */
#  endif

   /* after the code has been compiled, the following fields will contained
      the compiled information */
   code_elem *opcodes;
   uint opcodesUsed;

   uint InputParameterCount;

   VarName FunctionName;

   VarName * VarNames;

   uword8 * VarAttrib;
};


struct LocalFunction * NEAR_CALL localNew(struct Call *call,
   VarName FunctionName,struct Code *BeginBlockCode
#  if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
      ,jsebool CBehavior);
#  else
      );
#  endif
void NEAR_CALL localDelete(struct LocalFunction *lf,struct Call *call);

void NEAR_CALL localExecute(struct LocalFunction *lf,
                                   struct Call *call,VarRead *Var);
  /* execute this function; if there was an error then Oops was called */
#if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)
   void localTokenRead(struct Call *call,struct TokenDst *tDst);
#endif

#if (0!=JSE_COMPILER)
   jsebool NEAR_CALL localResolveVariableNames(struct LocalFunction *lf,
                                            struct Call *call);
     /* Called only on !HasBeenCalledBefore to figure out all the local
      * variables for if they belong locally or in a. Return False if there
      * was an error (message will have been written but no call.return set)
      */
#endif

#define LOCAL_TEST_IF_INIT_FUNCTION(this,call) \
           (GLOBAL_STRING(call,init_function_entry) == (this)->FunctionName)

#endif
