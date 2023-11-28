/* operator.h  handle all numeric Javascript operators such as +, *, etc.
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

#ifndef _OPERATOR_H
#define _OPERATOR_H

void NEAR_CALL do_op_structureMember(struct Call *call,
                                     struct secodeStack *thestack,
                                     jsebool ByName,VarName MemberName,
                                     JSE_POINTER_SINDEX Index);
void NEAR_CALL do_op_arrayMember(struct Call *call,
                                 struct secodeStack *thestack);
VarRead * JSE_CFUNC NEAR_CALL do_op_crement(struct Call *call,
  VarRead *lrvar,VarRead *rrvar,codeval code);
VarRead * JSE_CFUNC NEAR_CALL do_op_subtract(struct Call *call,
  VarRead *lrvar, VarRead *rrvar,codeval code);
VarRead * JSE_CFUNC NEAR_CALL do_twonumbers(struct Call *call,
  VarRead *lrvar,VarRead *rrvar,codeval code);
VarRead * JSE_CFUNC NEAR_CALL do_op_boolNot(struct Call *call,
  VarRead *lrvar, VarRead *rrvar,codeval code);
VarRead * JSE_CFUNC NEAR_CALL do_op_bitNot(struct Call *call,
  VarRead *lrvar, VarRead *rrvar,codeval code);
VarRead * JSE_CFUNC NEAR_CALL do_op_typeof(struct Call *call,
  VarRead *lrvar, VarRead *rrvar,codeval code);
VarRead * JSE_CFUNC NEAR_CALL do_add_or_subtract(struct Call *call,
  VarRead *rlvar,VarRead *rrvar,codeval direction);
VarRead * JSE_CFUNC NEAR_CALL do_op_void(struct Call *call,
  VarRead *lrvar, VarRead *rrvar,codeval code);
VarRead * JSE_CFUNC NEAR_CALL do_op_delete(struct Call *call,
  VarRead *lrvar, VarRead *rrvar,codeval code);
VarRead * JSE_CFUNC NEAR_CALL do_op_positive_or_negative(struct Call *call,
  VarRead *lrvar, VarRead *rrvar,codeval code);
VarRead * JSE_CFUNC NEAR_CALL do_op_assign(struct Call *call,
  VarRead *lrvar,VarRead *rrvar,codeval t);
VarRead * JSE_CFUNC NEAR_CALL do_op_compare(struct Call *call,
  VarRead *lrvar,VarRead *rrvar,codeval code);

typedef VarRead * (JSE_CFUNC NEAR_CALL * DoOpFunc)
     (struct Call *call,VarRead *rlvar,VarRead *rrvar,codeval t);

/* OpDescription is an array describing all of the mathish operators */
struct OpDescription {
   DoOpFunc opfunc;        /* function to call to act on this operator */
#  if (defined(JSE_CREATEFUNCTIONTEXTVARIABLE) && (0!=JSE_CREATEFUNCTIONTEXTVARIABLE)) \
   || (defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING))
      const jsechar * TokenText; /* text-representation of this token
                                 (for output messages) */
#  endif
#  if (0!=JSE_COMPILER)
      uword8 Priority;        /* higher-priority operators must happen first;
                                 see following list */
#  endif
   uword8 Flags;      /* flags to describe stack behavior of this operator */
#  ifndef NDEBUG
      codeval WhichOp; /* same order in array, this is redundant */
#  endif
};

#if (0!=JSE_COMPILER)
#  define PRI_ASSIGN         1
#  define PRI_CONDITIONAL    2
#  define PRI_LOGICAL_OR     3
#  define PRI_LOGICAL_AND    4
#  define PRI_BITWISE_OR     5
#  define PRI_BITWISE_XOR    6
#  define PRI_BITWISE_AND    7
#  define PRI_EQUALITY       8
#  define PRI_RELATIONAL     9
#  define PRI_SHIFT          10
#  define PRI_ADDITIVE       11
#  define PRI_MULTIPLICATIVE 12
#  define PRI_UNARY          13

/* cast to uint to prevent tons of warnings in Microsoft - Rich */
#  define GET_OP_PRIORITY(WhichOp) (uint)(( (WhichOp)<BeginOpList || \
                                            EndOpList<(WhichOp) ) \
        ? 0 \
        : OpDescriptionList[(WhichOp)-BeginOpList].Priority)
#endif

#define OP_READ_RVAR    0x01
#define OP_READ_LVAR    0x02
#define OP_WRITE_LVAR   0x04
#define OP_POST_WRITE   0x08  /* postincrement or postdecrement */

extern CONST_DATA(struct OpDescription) OpDescriptionList[];
   /* list of tokens */

#define getOpDescription(WhichOp) (&(OpDescriptionList[(WhichOp)-BeginOpList]))

#ifndef NDEBUG
#  if (0!=JSE_COMPILER)
#     if (defined(JSE_CREATEFUNCTIONTEXTVARIABLE) && \
         (0!=JSE_CREATEFUNCTIONTEXTVARIABLE)) || \
         (defined(JSE_OPERATOR_OVERLOADING) && \
         (0!=JSE_OPERATOR_OVERLOADING))
#        define OP_DESC(CODE,TEXT,PRIORITY,FUNC,FLAGS) \
                   {FUNC,TEXT,PRIORITY,FLAGS,CODE}
#     else
#        define OP_DESC(CODE,TEXT,PRIORITY,FUNC,FLAGS) \
                   {FUNC,PRIORITY,FLAGS,CODE}
#     endif
#  elif (defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING))
#     define OP_DESC(CODE,TEXT,PRIORITY,FUNC,FLAGS) {FUNC,TEXT,FLAGS,CODE}
#  else
#     define OP_DESC(CODE,TEXT,PRIORITY,FUNC,FLAGS) {FUNC,FLAGS,CODE}
#  endif
#else
#  if (0!=JSE_COMPILER)
#     if (defined(JSE_CREATEFUNCTIONTEXTVARIABLE) && \
         (0!=JSE_CREATEFUNCTIONTEXTVARIABLE)) || \
         (defined(JSE_OPERATOR_OVERLOADING) && \
         (0!=JSE_OPERATOR_OVERLOADING))
#        define OP_DESC(CODE,TEXT,PRIORITY,FUNC,FLAGS) \
                   {FUNC,TEXT,PRIORITY,FLAGS}
#     else
#        define OP_DESC(CODE,TEXT,PRIORITY,FUNC,FLAGS) \
                   {FUNC,PRIORITY,FLAGS}
#     endif
#  elif (defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING))
#     define OP_DESC(CODE,TEXT,PRIORITY,FUNC,FLAGS) {FUNC,TEXT,FLAGS}
#  else
#     define OP_DESC(CODE,TEXT,PRIORITY,FUNC,FLAGS) {FUNC,FLAGS}
#  endif
#endif

#endif
