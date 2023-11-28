/* secode.h   Defines to use the secode compiler
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

#ifndef SECODE_H
#define SECODE_H

struct secompile;
struct Code;

/* The codes we compile down to. This will break lots of stuff if the number
 * of runtime codes goes>255
 */
typedef sword8  codeval;
   /* DO NOT change the order without updating 'secodeListing()' */

   /* to speed up knowing how many opcodes are taken up my any field,
    * we bunch up the first opcodes to be those that take values.
    * See CODELIST_DATUM_SIZE and IS_CODELIST_DATUM for macros that make
    * quick use of these values.
    */
#define START_INT_CODES    (codeval)0
#define codevalMin         (codeval)0
#   define START_UINT_CODES START_INT_CODES
#     define START_GOTO_CODES  START_UINT_CODES
#        define gotoAlways START_GOTO_CODES
               /* perform a jump */
#        define gotoFalse  (codeval)1
               /* test the top item on the stack, and perform the goto if it is
                * False.turn it into a boolean using ECMAScript conversion if
                * needed It uses the statementPtr to tell it where to go.
                */
#        define gotoTrue   (codeval)2
                /* Just like gotoFalse but branch if true */
#        define gotoForIn  (codeval)3
               /* special ones to handle for..in */
#     define END_GOTO_CODES (codeval)gotoForIn
#     define sourceLineNumber  (codeval)4
#     define pushNumber        (codeval)5
            /* create and push uint value */
#      define getParamValue     (codeval)6
#   define END_UINT_CODES  getParamValue
#   define START_SINT_CODES    (codeval)7
#     define functionCall      START_SINT_CODES
            /* just like new, but only call the function (the only differences
             * are _call vs _construct and the value of the 'this' and the
             * return.)
             */
#     define functionNew       (codeval)8
            /* perform function new. There are numargs args on the stack and
             * below that is the function to call.
             */
#   define END_SINT_CODES      functionNew
#define END_INT_CODES          END_SINT_CODES
#define START_PTR_CODES        (codeval)9
#   define START_VARNAME_CODES START_PTR_CODES
#      define sourceFilename   START_VARNAME_CODES
#      define pushVariable     (codeval)10
            /* lookup and push variable */
#      define declareVar       (codeval)11
            /* add variable to activation object */
#      define structureMemberName (codeval)12
            /* The top of the stack is an object. Get the named member. */
#   define END_VARNAME_CODES   structureMemberName
#   define START_VARREAD_CODES (codeval)13
#      define pushValue        START_VARREAD_CODES
            /* push a literal variable value */
#   define END_VARREAD_CODES   pushValue
#define END_PTR_CODES     END_VARREAD_CODES
#define MAX_DATUM_CODES   END_PTR_CODES  
      /* all codes here and up have more data */

      /* keep track of source location */
#define mayIContinue     (codeval)14
      /* call may I continue hook. Can cause program execution to terminate. */

#define withoutVar       (codeval)15
      /* remove the with statement from the scope chain */
   
#define   returnVoid     (codeval)16
#define   returnVar      (codeval)17
      /* both execute a return, but the second means return with a value,
       * namely the top stack element
       */
   
#define   pushTrue       (codeval)18
#define   pushFalse      (codeval)19
      /* push variable representing boolean truth value */
#define   pushObject     (codeval)20
      /* create and push blank object */
#define   pushClone      (codeval)21
     /* clone top item of stack */
#define   checkFunctionCall   (codeval)22
#define   checkFunctionNew    (codeval)23
      /* replace the top of the stack with the new function it should call,
       * making sure it is a valid function and all that
       */
#define   getValue       (codeval)24
#define   pushIterator   (codeval)25
#define   popIterator    (codeval)26
#define   popDiscard     (codeval)27
      /* discard top item on stack */

#define   MORE_TO_FOLLOW (codeval)28 
                           /* all following codes, up until NO_MORE_TO_FOLLOW,
                            * plus getParamValue tell the parser that another
                            * parameter is expected
                            */

#define   popEval        MORE_TO_FOLLOW
      /* pop top value from stack and eval it for reading (to force a
       * getValue() on it. */
#define   getReferenceValue   (codeval)29
#define   withVar         (codeval)30
      /* add the top of the stack to the scope chain */
#define   structureMember (codeval)31
      /* the top two items on the stack are an object and a variable
       * representing the index (a number or a string var.) Evaluate and
       * replace.
       */

   /* ---------------------------------------------------------------------- */
   /* lots and lots of numeric operators                                     */
   /* ---------------------------------------------------------------------- */
#define   BeginOpList            (codeval)32

#define   opDelete               (codeval)32
#define   opTypeof               (codeval)33

/* Assign does not fall under these groupings, so keep it separate.
 * opAssignOnly does not push the value afterward (it is an optimization
 * with popDiscard immediately following.)
 */
#define   opAssignOnly           (codeval)34
#define   opAssign               (codeval)35

#define   BEGIN_OP_WRITE_LVAR    opAssignOnly
#define   BEGIN_OP_READ_LVAR     (codeval)36
#define   BEGIN_OP_POST_WRITE    BEGIN_OP_READ_LVAR
#define   opPostDecrement        BEGIN_OP_READ_LVAR
#define   opPostIncrement        (codeval)37
#define   END_OP_POST_WRITE      opPostIncrement

#define   opPreIncrement         (codeval)38
#define   opPreDecrement         (codeval)39

#define   BEGIN_OP_READ_RVAR     (codeval)40

#define   opAssignMultiply       BEGIN_OP_READ_RVAR
#define   opAssignDivide         (codeval)41
#define   opAssignModulo         (codeval)42
#define   opAssignAdd            (codeval)43
#define   opAssignSubtract       (codeval)44
#define   opAssignBitAnd         (codeval)45
#define   opAssignBitXor         (codeval)46
#define   opAssignBitOr          (codeval)47
#define   opAssignShiftLeft      (codeval)48
#define   opAssignSignedShiftRight   (codeval)49
#define   opAssignUnsignedShiftRight (codeval)50

#define   END_OP_WRITE_LVAR      opAssignUnsignedShiftRight

#define   opMultiply             (codeval)51
#define   opDivide               (codeval)52
#define   opModulo               (codeval)53
#define   opAdd                  (codeval)54
#define   opSubtract             (codeval)55
#define   opShiftLeft            (codeval)56
#define   opSignedShiftRight     (codeval)57
#define   opUnsignedShiftRight   (codeval)58
   /* keep these 6 compare operators together and in this order */
#define   opLess                 (codeval)59  
#define   opLessEqual            (codeval)60
#define   opGreater              (codeval)61
#define   opGreaterEqual         (codeval)62
#define   opEqual                (codeval)63
#define   opNotEqual             (codeval)64
#define   opBitAnd               (codeval)65
#define   opBitOr                (codeval)66
#define   opBitXor               (codeval)67

#define   END_OP_READ_RVAR       opBitXor

#define   opVoid                 (codeval)68
#define   opPositive             (codeval)69
#define   opNegative             (codeval)70
#define   opBoolNot              (codeval)71
#define   opBitNot               (codeval)72


#define   END_OP_READ_LVAR       opBitNot


   /* following types are used only in initial parsing, not during runtime. They
    * can be made >255, but all preceding bytecodes must be less than 256 so
    * they can fit in a single byte.
    */
#define   NotUsedAfterCompilation  (codeval)73

#define   LogicalAND             NotUsedAfterCompilation
#define   LogicalOR              (codeval)74
#define   ConditionalTrue        (codeval)75

#define   EndOpList              ConditionalTrue

#define   statementIf            (codeval)76
#define   statementElse          (codeval)77
#define   statementSwitch        (codeval)78
#define   SwitchCase             (codeval)79
#define   SwitchDefault          (codeval)80
#define   statementWhile         (codeval)81
#define   statementDo            (codeval)82
#define   statementFor           (codeval)83
#define   statementIn            (codeval)84
#define   EvaluationGroup        (codeval)85
#define   UnresolvedFunctionCall (codeval)86
#define   ConditionalFalse       (codeval)87

#define   NO_MORE_TO_FOLLOW      (codeval)88

#define   UnknownCardType        NO_MORE_TO_FOLLOW
#define   BeginBlock             (codeval)89
#define   EndBlock               (codeval)90
#define   EndFunctionCall        (codeval)91
#define   EndEvaluationGroup     (codeval)92
#define   EndArrayIndex          (codeval)93
#define   DeclareFunction        (codeval)94
#define   DeclareCFunction       (codeval)95
#define   StatementEnd           (codeval)96
#define   Label                  (codeval)97
#define   CaseColon              (codeval)98
#define   statementBreak         (codeval)99
#define   statementContinue      (codeval)100
#define   passByReference        (codeval)101

    /* record of some types Brent changed ;
    * the obvious changes aren't listed here
    * card::Variable -> pushValue
    * card::Variable_Name -> pushVariable
    * card::SourceFileLineNumber -> sourceLineNumber
    * card::SourceFileName -> sourceFilename
    * card::Increment -> opPreIncrement   but may be changed to opPostIncrement
    * card::Decrement -> opPreDecrement   but may be changed to opPostDecrement
    * card::VariableFunctionCall -> functionCall
    * card::ArrayIndex -> structureMember
    * card::StructureMemberDot -> structureMemberName
    * card::Remainder -> opModulo
    * card::CommaOperator -> popEval
    * card::Goto -> gotoAlways
    * card::SeparateFunctionCallArgs -> getParamValue
    * card::NewOp -> functionNew
    */

#define   NUM_SECODES            (codeval)102
#define   codevalMax             (NUM_SECODES-1)

#define   isValidCodeval(c)  ((c) >= codevalMin && (c) <= codevalMax)

#if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
   /* pack codeptr as tightly as possible */
   typedef uword8 code_elem;
#elif defined(__osf__)
   typedef ulong code_elem;     /* ptrs are 8 bytes! not ints. */
#else
   typedef uint code_elem;
#endif

#ifdef __osf__
   typedef ulong code_elem_type;
#else
   typedef uint code_elem_type;
#endif

#define CODELIST_DATUM(CODEPTR,DATUM_TYPE)  (*((DATUM_TYPE *)(CODEPTR+1)))
#define IS_CODELIST_DATUM(CODEVAL)     ( (CODEVAL) <= MAX_DATUM_CODES )

#  define INCREMENT_DATUM_CODEPTR(CODEPTR) \
      ( CODEPTR += ( (((*CODEPTR) <= END_INT_CODES)) \
    ? (sizeof(code_elem_type)/sizeof(*CODEPTR)) \
    : (sizeof(void *)/sizeof(*CODEPTR))) )
/* datum always takes up at least one slot */
#  define SKIP_CODELIST_DATUM(CODEPTR,DATUM_TYPE) \
             (CODEPTR += (sizeof(DATUM_TYPE)/sizeof(*CODEPTR))?\
              (sizeof(DATUM_TYPE)/sizeof(*CODEPTR)):1)
  /* call only if IS_CODELIST_DATUM */

#if (0!=JSE_COMPILER) || ( defined(JSE_TOKENDST) && (0!=JSE_TOKENDST) )
   struct secompile
   {
      struct Call *call; /* the call initialized in secompileCompile */
      struct LocalFunction *locfunc;
      code_elem * opcodes;
      uint opcodesUsed,opcodesAlloced;
      uint varsUsed, varsAlloced;
      VarName *vars;
#     if (0!=JSE_COMPILER)
         struct loopTracker *loopTrack;
         struct gotoTracker *gotoTrack;
         struct Code *srcCodes; /* the codes we'll be compiling from */
         uint with_depth;
#     endif
   };
#endif

#if (0!=JSE_COMPILER)
   struct loopTracker
   {
      /* it can track loop 'breaks'/'continues' which may require multiple
       * levels of these things. This points back to the last level
       */
      struct loopTracker *next;

      /* We store the address of the goto for the 'breaks' and 'continues'
       * when the loop is done, go back and patch all of these.
       */
      uint breaksUsed,breaksAlloced;
      uint continuesUsed,continuesAlloced;
      uint *breaks,*continues;
   };


   struct loopTracker *NEAR_CALL looptrackerNew(void);
   void NEAR_CALL looptrackerDelete(struct loopTracker *handle,
                                    struct secompile *inwhat,uint breakPtr,
                                    uint continuePtr);
   void NEAR_CALL looptrackerAddBreak(struct loopTracker *handle,uint addr);
   void NEAR_CALL looptrackerAddContinue(struct loopTracker *handle,uint addr);


   struct gotoItem {
      uint sptr;
      VarName label;
   };


   struct gotoTracker
   {
      uint gotosUsed,gotosAlloced;
      uint labelsUsed,labelsAlloced;
      struct gotoItem *gotos,*labels;
   };


   struct gotoTracker * NEAR_CALL gototrackerNew(void);
   void NEAR_CALL gototrackerDelete(struct gotoTracker *handle,
                                           struct Call *call,
                                    struct secompile *inwhat);
   void NEAR_CALL gototrackerSave(struct gotoTracker *handle,uint sptr,
                                  VarName label);
   void NEAR_CALL gototrackerResolveLabel(struct gotoTracker *handle,
                                          uint sptr,VarName label);

   jsebool NEAR_CALL secompileCompile(struct LocalFunction *locfunc,
                                   struct Call *call);

   void NEAR_CALL secompileAddItem(struct secompile *handle,
                                   int/*codeval*/ code,...);
      /* the second argument above is really a codeval and not an int, but
       * ANSI standard states that the argument before a "..." must be one
       * for which promotion is not applied.  Making an int solves
       * this problem and is safe to convert up to.
       */
   jsebool NEAR_CALL secompileAddBreak(struct secompile *handle);
   jsebool NEAR_CALL secompileAddContinue(struct secompile *handle);

   jsebool NEAR_CALL secompileArray(struct secompile *handle);
   sint    NEAR_CALL secompileFunctionArgs(struct secompile *handle);
   jsebool NEAR_CALL secompileExpression(struct secompile *handle);
   jsebool NEAR_CALL secompilePostfixExpression(struct secompile *handle);
   jsebool NEAR_CALL secompileMemberExpression(struct secompile *handle);
   jsebool NEAR_CALL secompilePrimaryExpression(struct secompile *handle);

   jsebool NEAR_CALL secompileOperatorExpression(struct secompile *handle,
                                                 uint Priority);

   jsebool NEAR_CALL secompileIf(struct secompile *handle);
   jsebool NEAR_CALL secompileWhile(struct secompile *handle);
   jsebool NEAR_CALL secompileDo(struct secompile *handle);
   jsebool NEAR_CALL secompileWith(struct secompile *handle);
   jsebool NEAR_CALL secompileVar(struct secompile *handle);
   jsebool NEAR_CALL secompileFor(struct secompile *handle);
   jsebool NEAR_CALL secompileSwitch(struct secompile *handle);
   jsebool NEAR_CALL secompileStatement(struct secompile *handle);

   jsebool NEAR_CALL secompileAddLabel(struct secompile *handle,VarName label);
   jsebool NEAR_CALL secompileAddGoto(struct secompile *handle,VarName label);
   void NEAR_CALL secompileAdvancePtr(struct secompile *handle);

   void NEAR_CALL secompileNewLoop(struct secompile *handle);
   void NEAR_CALL secompileEndLoop(struct secompile *handle,
                                   uint break_address,uint continue_address);
   void NEAR_CALL secompileFixupGotoItem(struct secompile *handle,
                                         uint item,uint newdest);

   void NEAR_CALL secompileFileInfoCard(struct secompile *handle);

#  define secompileCurrentItem(this) ((this)->opcodesUsed)

#endif /* #if (0!=JSE_COMPILER) */

struct Call * NEAR_CALL callFunction(struct Call *call,uint numargs,jsebool newfunc);

void NEAR_CALL secodeInterpInit(struct LocalFunction *locfunc,struct Call *call);
struct Call * NEAR_CALL secodeInterpret(struct Call *call);

void NEAR_CALL secodeDeleteOpcode(code_elem *c,struct Call *call);

void NEAR_CALL secodeDeleteOpcodes(code_elem *opcodes,uint opcodesUsed,
                                   struct Call *call);

jsebool secodeSourceLocation(struct Call *call,
                             const jsechar **FileName,uint *LineNumber);

/*
 * A faster stack. Currently it never shrinks, but if that is a problem put it
 * in pop(). The idea is to cut the stack in half when it is down to 1/4 (no
 * typo, (1/4) full. One stack is used for the whole program (it is in
 * Call.Global).
 */
struct secodeStack
{
   uint used,alloced;
   struct Var **items;
};


struct secodeStack * NEAR_CALL secodestackNew(void);
void NEAR_CALL secodestackDelete(struct secodeStack *handle,struct Call *call);
void NEAR_CALL secodestackExtend(struct secodeStack *);


/* pop and return top stack item. YOU now control the lock on this
 * item and must delete it when done.
 */
#define secodestackDepth(this) ((this)->used)
#if !defined(JSE_INLINES) || (0==JSE_INLINES)
   struct Var * NEAR_CALL SECODE_STACK_POP(struct secodeStack *This);
#else
#  define SECODE_STACK_POP(this) ((this)->items[--((this)->used)])
#endif
#define SECODE_STACK_MULTIPOP(this,popcount)  this->used -= (popcount)

/* Make sure to lock the variable (not its value - just add user) before
 * passing it to this macro
 */
#if !defined(JSE_INLINES) || (0==JSE_INLINES)
   void NEAR_CALL SECODE_STACK_PUSH(struct secodeStack *This,struct Var *var);
#else
#  define SECODE_STACK_PUSH(this,var) \
   { \
      if( (this)->used>=(this)->alloced ) secodestackExtend(this); \
      (this)->items[((this)->used)++] = (var); \
   }
#endif

/* depth 0==top */
#if !defined(JSE_INLINES) || (0==JSE_INLINES)
   Var * NEAR_CALL SECODE_STACK_PEEK(struct secodeStack *This,uint offset);
#else
#  define SECODE_STACK_PEEK(this,offset) \
      ((this)->items[(this)->used-(offset)-1])
#endif

/* some stuff Brent added */

#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   void NEAR_CALL secodeTokenWriteList(struct Call *call,struct TokenSrc *tSrc,
                                       struct LocalFunction *locfunc);
#endif
#if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)
   void NEAR_CALL secodeTokenReadList(struct Call *call,struct TokenDst *tDst,
                                      struct LocalFunction *locfunc);
#endif

#endif
