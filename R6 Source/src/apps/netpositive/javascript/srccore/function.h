/* Function.h  Generic function stuff
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

#ifndef _FUNCTION_H
#define _FUNCTION_H

struct Function
{
   uint params;
   VarRead *stored_in;    /* so we can set length when we figure it out */
#  if 0 != JSE_MULTIPLE_GLOBAL
      VarRead * global_object;
#  endif
   uword8 flags;          /* or of any of the following (used here and in
                             loclfunc.h and library.h) */
};

#define Func_LocalFunction   0x01   /* else library function */
#define Func_CBehavior       0x02   /* else default javascript */
#define Func_UsesArguments   0x04   /* whether local function uses its
                                       arguments variable */
#define Func_UsesItsName     0x08   /* whether local function uses its own
                                       name */
#if (0!=JSE_COMPILER)
 #define Func_VarNamesResolved 0x10 /* whether local variable names have
                                       been resolved */
#endif
#define Func_StaticLibrary   0x80   /* for library function: if True then must
                                     * free no data, else it's a dynamic
                                     * wrapper and data is not static */

void NEAR_CALL functionInit(struct Function *func,
                          struct Call *call,
                          VarRead *ObjectToAddTo, /* NULL if not to add to any
                                                     object */
                          jseVarAttributes FunctionVariableAttributes,
                             /*not used if ObejctToAddTo is NULL*/
                          jsebool iLocalFunction, /*else library*/
                          jsebool iCBehavior,
                            /*else default javascript behavior*/
                          uint Params);
void NEAR_CALL functionDelete(struct Function *func,struct Call *call);

void NEAR_CALL functionFullCall(struct Function *func,struct Call *call,
                                VarRead *FuncVar,uint InputVariableCount,
                                VarRead *ThisVar);
jsebool NEAR_CALL functionDoCall(struct Function *func,struct Call *call,
                                 VarRead *FuncVar,uint InputVariableCount,
                                 VarRead *ThisVar);
   /* execute the code in this function; use global stack to get args.  This
    * function will pop the args from the caller stack before returning.  It
    * will leave one variable on the stack.
    * Returns ReasonToQuit and ExitCode (in case it's needed).  The only
    * ReasonToQuit passed up from this call are:
    *     FlowNoReasonToQuit or a FlowQuitFlag is set
    */
VarRead * NEAR_CALL functionFinishCall(struct Function *this,struct Call *call,
                                  VarRead *FuncVar,
                                  uint InputVariableCount,VarRead *ThisVar);

#if defined(JSE_BREAKPOINT_TEST) && (0!=JSE_BREAKPOINT_TEST)
   jsebool BreakpointTest(struct Call *call,const jsechar *FileName,
                          uword32 LineNumber);
#endif

const jsechar * functionName(struct Function *func,struct Call *call);

#if defined(JSE_CREATEFUNCTIONTEXTVARIABLE) && \
    (0!=JSE_CREATEFUNCTIONTEXTVARIABLE)
   VarRead * NEAR_CALL functionTextAsVariable(struct Function *func,
                                              struct Call *call);
#endif


#define functionParamCount(this) ((this)->params)

#define functionCBehavior(this) ( Func_CBehavior & (this)->flags )
#define functionTestIfLocal(this) ( Func_LocalFunction & (this)->flags )

#endif
