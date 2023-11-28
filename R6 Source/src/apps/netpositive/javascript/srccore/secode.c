/* Secode.c   The secode compiler/interpreter
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

/* SECODE Compiler/Interpreter
 *
 * After our existing code has parsed the program into the tokens we
 * used to use,the code now calls secompileCompile() to turn that into an
 * intermediate code (called secode, big surprise) that can be
 * interpretted quickly. Once this is done, the program continues as
 * normal. When that function is going to be interpreted, it calls
 * secodeInterpret() instead of the old code (which executed the
 * tokens as a series of statements.)
 *
 * To ease in debugging, or if you just want to see what gets generated,
 * there is a routine called secodeListing() which prints out (using
 * printf so you'll have to use a console version) a human-readable
 * version of the secode program. You can have secompileCompile() call this
 * as the last thing to have it print out all of the programs (one for
 * each function) it makes for your script.
 *
 * The secodes are stored internally in an array of secodeItems, each
 * of which is a secode opcode and its extended information. Each extended
 * information type is stored in a union, and they are all currently
 * 4 bytes long (except in 16-bit environs may be smaller), so a single
 * secode opcode is 8 bytes (assuming enums as 4 bytes.)
 *
 * This is no longer true. Brent changed it so that each secode takes
 * up as much space as it needs.
 *
 *
 * The compilation uses a recursive-descent, one token look ahead scheme.
 * That means that the 'next' token is the one we are looking at, and
 * we make all parsing decisions by the value of the next token. All parsing
 * routines take a call, and the token chain, and are responsible for
 * advancing the token chain over any tokens it uses (it eats them), and
 * outputting appropriate code via secodeAddItem((). All such routines
 * return False if there was a parsing error and will have printed out
 * some error message. If there is an error, the code produced is considered
 * to be garbage.
 *
 * There are two main sections of the compiler - statements and expressions.
 * The last option for a statement is for it to be an expression, which
 * is why the 'this is not a valid expression' error pops up in a lot of
 * bad programs. At runtime, the code generated for a statement must finish
 * processing with nothing on the stack. The code for an expression finishes
 * processing with the value of the expression on the stack. The code to
 * process For..in and With both keep information on the stack and the
 * secode interpretter knows to get rid of it if there is a return in
 * the middle of one of those loops.
 */

#include "srccore.h"

/*
 * Figure out the current source location
 */
   jsebool
secodeSourceLocation(struct Call *call,const jsechar **name,uint *line)
{
   /* compatibility with existing code */
#  if (0!=JSE_COMPILER)
      if ( 0 != call->Global->CompileStatus.NowCompiling )
      {
         *name = call->Global->CompileStatus.CompilingFileName;
         *line = call->Global->CompileStatus.CompilingLineNumber;
      }
      else
#  endif
      {
         *name = call->filename?GetStringTableEntry(call,call->filename,NULL):
            (const jsechar *)NULL;
         *line = (uint)call->linenum;
      }

   return True;
}


#if (0!=JSE_COMPILER) || ( defined(JSE_TOKENDST) && (0!=JSE_TOKENDST) )
/*
 * Add an secode item. This is really a virtual assembly instruction for this
 * simple 'ScriptEase' machine. Some of the codes have extra parameters which
 * are retrieved and stored.
 */
   void NEAR_CALL
secompileAddItem(struct secompile *This,int/*codeval*/ code,...)
{
   code_elem *opcodeptr;
   uint ItemsUsedByThisCode;
   
   assert( isValidCodeval(code) );

   /* For declareVar, we do something special */
   if( code==declareVar && This->varsAlloced>0 )
   {
      va_list arg;
      if( This->varsAlloced<=This->varsUsed )
      {
         This->varsAlloced += 10;
         This->vars = jseMustReMalloc(VarName,This->vars,This->varsAlloced*sizeof(VarName));
         assert( This->vars!=NULL );
      }
      va_start(arg,code);
      This->vars[This->varsUsed++] = va_arg(arg,VarName);
      va_end(arg);
      return;
   }

   
   /* check that we have enough memory allocated, plus extra in case this
    * code uses extra data */
   if( This->opcodesAlloced <= (This->opcodesUsed +
                                (sizeof(void *)/sizeof(code_elem))) )
   {
      This->opcodesAlloced += 100;
      This->opcodes = jseMustReMalloc(code_elem,This->opcodes,
         (uint)(This->opcodesAlloced*sizeof(*(This->opcodes))));
      assert( NULL != This->opcodes );
   }
   assert( This->opcodesUsed < This->opcodesAlloced );

   opcodeptr = This->opcodes + This->opcodesUsed;
   *opcodeptr = (uint) code;
   ItemsUsedByThisCode = 1;
      /* all items use at least the one code for storage */

   if ( IS_CODELIST_DATUM(code) )
   {

      va_list arg;
      va_start(arg,code);

      /* NOTE: we cannot blindly do optimizations like concatenating two
       *       adjacent line number ops because a goto may target the second
       *       one. A peephole optimizer does this work later.
       */

      if ( code <= END_INT_CODES )
      {
         assert( START_INT_CODES <= code  &&  code <= END_INT_CODES );
         if ( code <= END_UINT_CODES )
         {
            assert( START_UINT_CODES <= code  &&  code <= END_UINT_CODES );
            assert( gotoAlways==code || gotoFalse==code || gotoTrue==code || \
                    gotoForIn==code || sourceLineNumber==code || \
                    getParamValue==code || pushNumber==code );

            CODELIST_DATUM(opcodeptr,uint) = va_arg(arg,uint);
         }
         else
         {
            assert( START_SINT_CODES <= code  &&  code <= END_SINT_CODES );
            assert( functionCall==code || functionNew==code );
            CODELIST_DATUM(opcodeptr,sint) = va_arg(arg,sint);
         }
         ItemsUsedByThisCode += (sizeof(code_elem_type))/(sizeof(*opcodeptr));
      }
      else
      {
         assert( START_PTR_CODES <= code  &&  code <= END_PTR_CODES );
         if ( code <= END_VARNAME_CODES )
         {
            VarName v = va_arg(arg,VarName);
            assert( START_VARNAME_CODES <= code && code <= END_VARNAME_CODES);
            AddStringUser(v);

            if ( sourceFilename != code )
            {
               assert( pushVariable==code || declareVar==code || \
                       structureMemberName==code );
               /* we note if the names 'arguments' or the name of the function
                * we are compiling are used. If not, we don't bother to set
                * up certain ECMA thingees in that function (they are slow, and
                * are not used.) Unfortunately, if we could possibly refer to
                * them, we have to set them up. See 'functionDoCall()' for
                * where these optimizations come into play.
                */
               if( v==GLOBAL_STRING(This->call,arguments_entry) )
                  This->locfunc->function.flags |= Func_UsesArguments;
               if( v==(VarName)This->locfunc->FunctionName )
                  This->locfunc->function.flags |= Func_UsesItsName;
            }
            CODELIST_DATUM(opcodeptr,VarName) = v;
         }
         else
         {
            assert( START_VARREAD_CODES <= code && code <= END_VARREAD_CODES );
            assert( pushValue == code );
            CODELIST_DATUM(opcodeptr,VarRead *) = va_arg(arg,VarRead *);
         }
         ItemsUsedByThisCode += (sizeof(void *))/(sizeof(*opcodeptr));
      }

      va_end(arg);
   }

   This->opcodesUsed += ItemsUsedByThisCode;
   assert( This->opcodesUsed <= This->opcodesAlloced );
}
#endif

#if (0!=JSE_COMPILER)

/* ---------------------------------------------------------------------- */
/* secode compiler routines                                               */
/* ---------------------------------------------------------------------- */


/*
 * Handle a card telling us what source file or line number we are on.
 * Updates our call so if an error happens we know what line number we
 * are on, and generates secodes so the same thing will happen at runtime.
 */
   void NEAR_CALL
secompileFileInfoCard(struct secompile *this)
{
   if( codeGetType(this->srcCodes)==sourceFilename )
   {
      secompileAddItem(this,sourceFilename,codeGetName(this->srcCodes));
      if( this->call->filename != NULL )
         RemoveFromStringTable(this->call,this->call->filename);
      AddStringUser(codeGetName(this->srcCodes));
      this->call->filename = codeGetName(this->srcCodes);
      /* compatibility with existing code */
      if ( 0 != this->call->Global->CompileStatus.NowCompiling )
      {
         this->call->Global->CompileStatus.CompilingFileName =
            GetStringTableEntry(this->call,this->call->filename,NULL);
      }
   }
   else
   {
      assert( codeGetType(this->srcCodes)==sourceLineNumber );
      secompileAddItem(this,sourceLineNumber,codeGetLine(this->srcCodes));
      this->call->linenum = codeGetLine(this->srcCodes);
      /* compatibility with existing code */
      if ( 0 != this->call->Global->CompileStatus.NowCompiling )
      {
         this->call->Global->CompileStatus.CompilingLineNumber =
            (uint)this->call->linenum;
      }
   }
   this->srcCodes = codeTrueNext(this->srcCodes);
}


/*
 * We handle any source file/line num stuff. We generate code to set those
 * at run time (for error reporting.) We also set them now for compile-time
 * errors. This means that error line number reporting is transparent to
 * the rest of the compiler.
 */
   void NEAR_CALL
secompileAdvancePtr(struct secompile *this)
{
   this->srcCodes = codeTrueNext(this->srcCodes);

   while( (this->srcCodes)!=NULL &&
          (codeGetType(this->srcCodes)==sourceFilename ||
          codeGetType(this->srcCodes)==sourceLineNumber) )
   {
      secompileFileInfoCard(this);
   }
}

/* Backpatch a single goto since we now know what address it should
 * be going to.
 */
   void NEAR_CALL
secompileFixupGotoItem(struct secompile *This,uint item,uint newdest)
{
   code_elem *c = This->opcodes + item;
   assert( gotoAlways == *c || gotoFalse == *c || gotoTrue == *c );
   CODELIST_DATUM(c,uint) = newdest;
}


/* ----------------------------------------------------------------------
 * The gotoTracker class keeps track of all gotos and labels in the
 * function. When the function completes, it points all the gotos at the
 * correct label. If any labels are referred to but not found, this is
 * an error.
 * ---------------------------------------------------------------------- */


   struct gotoTracker * NEAR_CALL
gototrackerNew(void)
{
   struct gotoTracker *this =
      jseMustMalloc(struct gotoTracker,sizeof(struct gotoTracker));

   this->gotosUsed = this->gotosAlloced = 0;
   this->gotos = NULL;
   this->labelsUsed = this->labelsAlloced = 0;
   this->labels = NULL;

   return this;
}


   void NEAR_CALL
gototrackerDelete(struct gotoTracker *this,struct Call *call,struct secompile *inwhat)
{
   uint x,y;
   for( x = 0;x<this->gotosUsed;x++ )
   {
      for( y = 0;y<this->labelsUsed;y++ )
      {
         if( this->gotos[x].label==this->labels[y].label )
         {
            secompileFixupGotoItem(inwhat,this->gotos[x].sptr,
                                   this->labels[y].sptr);
            break;
         }
      }
      if( y==this->labelsUsed )
      {
         callQuit(call,textcoreGOTO_LABEL_NOT_FOUND,
                  GetStringTableEntry(call,this->gotos[x].label,NULL));
         break;
      }
   }

   if( this->gotos ) jseMustFree(this->gotos);
   if( this->labels ) jseMustFree(this->labels);

   jseMustFree(this);
}


   void NEAR_CALL
gototrackerSave(struct gotoTracker *this,uint sptr,VarName label)
{
   if( this->gotosUsed>=this->gotosAlloced )
   {
      this->gotosAlloced += 10;
      this->gotos = jseMustReMalloc(struct gotoItem,this->gotos,
                              this->gotosAlloced * sizeof(struct gotoItem));
   }
   assert( this->gotosUsed<this->gotosAlloced );
   this->gotos[this->gotosUsed].sptr = sptr;
   this->gotos[this->gotosUsed].label = label;
   this->gotosUsed++;
}


   void NEAR_CALL
gototrackerResolveLabel(struct gotoTracker *this,uint sptr,VarName label)
{
   if( this->labelsUsed>=this->labelsAlloced )
   {
      this->labelsAlloced += 10;
      this->labels = jseMustReMalloc(struct gotoItem,this->labels,
                              this->labelsAlloced * sizeof(struct gotoItem));
   }
   assert( this->labelsUsed<this->labelsAlloced );
   this->labels[this->labelsUsed].sptr = sptr;
   this->labels[this->labelsUsed].label = label;
   this->labelsUsed++;
}


/* labels and gotos cannot appear within with() statements. Getting
 * this to work would be very difficult. For now at least, ignore it.
 * can't be in for..in either. The reason is both keep information
 * on our runtime stack as well as making changes in the call that
 * have to be in sync. It is a royal PITA to try to make a goto
 * sync these up as it leaves or enters these constructs.
 */

   jsebool NEAR_CALL
secompileAddGoto(struct secompile *this,VarName label)
{
   /* add a goto, and mark it for later fixup when we know what this
    * label refers to
    */
   if( this->with_depth!=0 )
   {
      callQuit(this->call,textcoreBAD_GOTO_PLACE);
      return False;
   }
   gototrackerSave(this->gotoTrack,secompileCurrentItem(this),label);
   secompileAddItem(this,gotoAlways,0);
   return True;
}


   jsebool NEAR_CALL
secompileAddLabel(struct secompile *this,VarName label)
{
   /* this address is what this label refers to. Error if already there */
   if( this->with_depth!=0 )
   {
      callQuit(this->call,textcoreBAD_GOTO_PLACE);
      return False;
   }
   gototrackerResolveLabel(this->gotoTrack,secompileCurrentItem(this),label);
   return True;
}


/*
 * ----------------------------------------------------------------------
 * The loopTracker works very much like the gotoTracker, keeping track
 * of all breaks and continues. When the loop finishes parsing, we
 * patch all of the breaks and continues to point to the correct address.
 *
 * Because loops can be inside loops, we keep a stack of these loops
 * linking them togethor in a list.
 * ----------------------------------------------------------------------
 */

   void NEAR_CALL
looptrackerAddBreak(struct loopTracker *this,uint addr)
{
   if( this->breaksUsed>=this->breaksAlloced )
   {
      this->breaksAlloced += 10;
      this->breaks = jseMustReMalloc(uint,this->breaks,this->breaksAlloced *
                                     sizeof(uint));
   }
   assert( this->breaks!=NULL );
   assert( this->breaksUsed<this->breaksAlloced );
   this->breaks[this->breaksUsed++] = addr;
}


   void NEAR_CALL
looptrackerAddContinue(struct loopTracker *this,uint addr)
{
   if( this->continuesUsed>=this->continuesAlloced )
   {
      this->continuesAlloced += 10;
      this->continues = jseMustReMalloc(uint,this->continues,
                                        this->continuesAlloced * sizeof(uint));
   }
   assert( this->continues!=NULL );
   assert( this->continuesUsed<this->continuesAlloced );
   this->continues[this->continuesUsed++] = addr;
}


   struct loopTracker * NEAR_CALL
looptrackerNew(void)
{
   struct loopTracker *this =
      jseMustMalloc(struct loopTracker,sizeof(struct loopTracker));

   this->next = NULL;
   this->breaksUsed = 0;
   this->breaksAlloced = 0;
   this->breaks = NULL;
   this->continuesUsed = 0;
   this->continuesAlloced = 0;
   this->continues = NULL;

   return this;
}


   void NEAR_CALL
looptrackerDelete(struct loopTracker *this,
                  struct secompile *inwhat,uint breakPtr,
                  uint continuePtr)
{
   uint x;

   for( x=0;x<this->breaksUsed;x++ )
      secompileFixupGotoItem(inwhat,this->breaks[x],breakPtr);
   for( x=0;x<this->continuesUsed;x++ )
      secompileFixupGotoItem(inwhat,this->continues[x],continuePtr);

   if( this->breaks ) jseMustFree(this->breaks);
   if( this->continues ) jseMustFree(this->continues);

   jseMustFree(this);
}


/* Mark that we are in a loop, and set up all breaks and continues to be
 * be saves. This must save any old loop behind it
 */
   void NEAR_CALL
secompileNewLoop(struct secompile *this)
{
   struct loopTracker *t = looptrackerNew();
   t->next = this->loopTrack;
   this->loopTrack = t;
}


/* Patch up all the breaks and continues of the loop, restore the old loop
 * (if any.)
 */
   void NEAR_CALL
secompileEndLoop(struct secompile *this,
                 uint break_address,uint continue_address)
{
   struct loopTracker *t = this->loopTrack;

   assert( this->loopTrack!=NULL );
   this->loopTrack = t->next;
   looptrackerDelete(t,this,break_address,continue_address);
}


   jsebool NEAR_CALL
secompileAddBreak(struct secompile *this)
{
   /* add a break (goto), but mark it so it can be back-filled later
    * when we figure out where break goes to!
    */
   if( !this->loopTrack )
   {
      callQuit(this->call,textcoreBAD_BREAK);
      return False;
   }
   looptrackerAddBreak(this->loopTrack,secompileCurrentItem(this));
   secompileAddItem(this,gotoAlways,0);
   return True;
}


   jsebool NEAR_CALL
secompileAddContinue(struct secompile *this)
{
   /* add a continue (goto), but mark it so it can be back-filled later
    * when we figure out where break goes to!
    */
   if( !this->loopTrack )
   {
      callQuit(this->call,textcoreBAD_BREAK);
      return False;
   }
   looptrackerAddContinue(this->loopTrack,secompileCurrentItem(this));
   secompileAddItem(this,gotoAlways,0);
   return True;
}


#if 0
/* Human-readable text strings for the secodes */
static jsechar *codeName[NotUsedAfterCompilation] = {
   "gotoAlways",
   "gotoFalse",
   "gotoTrue",
   "gotoForIn",
   "sourceLineNumber",
   "pushNumber",
   "getParamValue",
   "functionCall",
   "functionNew",
   "sourceFilename",
   "pushVariable",
   "declareVar",
   "structureMemberName",
   "pushValue",
   "mayIContinue",
   "withoutVar",
   "returnVoid",
   "returnVar",
   "pushTrue",
   "pushFalse",
   "pushObject",
   "pushClone",
   "checkFunctionCall",
   "checkFunctionNew",
   "getValue",
   "pushIterator",
   "popIterator",
   "popDiscard",
   "popEval",
   "getReferenceValue",
   "withVar",
   "structureMember",
   
   "opDelete",
   "opTypeof",
   "opAssignOnly",
   "opAssign",

   "opPostDecrement",
   "opPostIncrement",
   
   "opPreIncrement",
   "opPreDecrement",

   "opAssignMultiply",
   "opAssignDivide",
   "opAssignModulo",
   "opAssignAdd",
   "opAssignSubtract",
   "opAssignBitAnd",
   "opAssignBitXor",
   "opAssignBitOr",
   "opAssignShiftLeft",
   "opAssignSignedShiftRight",
   "opAssignUnsignedShiftRight",

   "opMultiply",
   "opDivide",
   "opModulo",
   "opAdd",
   "opSubtract",
   "opShiftLeft",
   "opSignedShiftRight",
   "opUnsignedShiftRight",
   "opLess",
   "opLessEqual"
   "opGreater",
   "opGreaterEqual",
   "opEqual",
   "opNotEqual",
   "opBitAnd",
   "opBitOr",
   "opBitXor",

   "opVoid",
   "opPositive",
   "opNegative",
   "opBoolNot",
   "opBitNot",
};


void NEAR_CALL secodeListing(struct secompile *this,struct Call *call)
{
   code_elem *EndOpcodes, *c;
   
   printf("---------------------------------------------------------------\n");
   printf("Function %s(%d args)%s%s\n",
          GetStringTableEntry(call,(VarName)this->locfunc->FunctionName,NULL),
          this->locfunc->InputParameterCount,
          this->locfunc->function.flags&Func_UsesArguments?
          " uses arguments":"",
          this->locfunc->function.flags&Func_UsesItsName?
          " refers to itself":"");
   printf("---------------------------------------------------------------\n");

   EndOpcodes = this->opcodes + this->opcodesUsed;
   for( c = this->opcodes; c != EndOpcodes; c++ )
   {
      printf("%03d: %20s   ",c-this->opcodes,codeName[*c]);
      if( *c==gotoAlways || *c==gotoFalse || *c==gotoTrue )
      {
         printf("%03d",CODELIST_DATUM(c,uint));
      }
      if( *c==pushVariable || *c==structureMemberName || *c==sourceFilename ||
          *c==declareVar )
      {
         printf("%s",GetStringTableEntry(call,CODELIST_DATUM(c,VarName),NULL));
      }
      if( *c==sourceLineNumber )
      {
         printf("%d",CODELIST_DATUM(c,uint));
      }
      if( *c==pushValue )
      {
         VarRead *v = CODELIST_DATUM(c,VarRead *);
         if( VAR_TYPE(v)==VNumber )
         {
            printf("%d",varGetNumber(v));
         }
         else if( VAR_TYPE(v)==VString )
         {
            printf("%s",varGetData(v,0));
         }
      }
      if( *c==functionCall || *c==functionNew )
      {
         printf("%d args",CODELIST_DATUM(c,uint));
      }
      printf("\n");
      if ( IS_CODELIST_DATUM(*c) )
         INCREMENT_DATUM_CODEPTR(c);
   }
   printf("---------------------------------------------------------------\n");
}
#endif


/* All instructions that point to another instruction in the range of
 * start..end have their instruction pointer modified by offset. This
 * will allow instructions to be moved around during optimization.
 */
   static void NEAR_CALL
secompileUpdate(struct secompile *This,int offset,int start,int end)
{
   code_elem *sptr, *StartOpcodes, *EndOpcodes;
   uint ptr;

   EndOpcodes = (StartOpcodes=This->opcodes) + This->opcodesUsed;
   for( sptr = StartOpcodes; sptr != EndOpcodes; sptr++ )
   {
      assert( START_GOTO_CODES==0 );
      if( /* (*sptr)>=START_GOTO_CODES && */ (*sptr)<=END_GOTO_CODES )
      {
         ptr = CODELIST_DATUM(sptr,uint);
         if( ptr>=(uint)start && ptr<=(uint)end )
            CODELIST_DATUM(sptr,uint) = offset + ptr;
      }
      if ( IS_CODELIST_DATUM(*sptr) )
      {
         INCREMENT_DATUM_CODEPTR(sptr);
      }
   }
   assert( sptr == EndOpcodes );
}


#if !defined(JSE_PEEPHOLE_OPTIMIZER) || (0!=JSE_PEEPHOLE_OPTIMIZER)
/* ---------------------------------------------------------------------- *
 * This is the code for the peephole optimizer. Its job is to find common
 * inefficient code sequences that are generated by the compile and turn them
 * into equivelent, but faster sequences. This generally involves making sure
 * that there is no entry into the middle of the sequence (i.e. no goto jumps
 * into the middle) and then replacing the sequence with a better one. This
 * is done in one place so as to not bloat the code with many repetitive checks.
 * ---------------------------------------------------------------------- */

/* Build up an array of which secodes have jumps targetted at them
 */
   static ubyte * NEAR_CALL
secompileTargetted(struct secompile *This)
{
   ubyte *ret;
   code_elem *sptr, *StartOpcodes, *EndOpcodes;
   int i;
   
   ret = jseMustMalloc(ubyte,This->opcodesUsed);
   assert( ret!=NULL );

   for( i=0;i<(int)This->opcodesUsed;i++ ) ret[i] = 0;
   
   EndOpcodes = (StartOpcodes=This->opcodes) + This->opcodesUsed;
   for( sptr = StartOpcodes; sptr != EndOpcodes; sptr++ )
   {
      assert( START_GOTO_CODES==0 );
      if( /* (*sptr)>=START_GOTO_CODES && */ (*sptr)<=END_GOTO_CODES )
      {
         assert( CODELIST_DATUM(sptr,uint)<This->opcodesUsed );

         ret[CODELIST_DATUM(sptr,uint)] = 1;
      }
      if ( IS_CODELIST_DATUM(*sptr) )
      {
         INCREMENT_DATUM_CODEPTR(sptr);
      }
   }
   assert( sptr == EndOpcodes );

   return ret;
}


/* Remove 'size' instructions starting at 'sptr' and update the 'targetted' array
 * as well.
 */
   static void NEAR_CALL
secompileDelete(struct secompile *This,code_elem *sptr,
                ubyte *targetted,uint size)
{
   size_t offsetTo = (size_t)(sptr - This->opcodes) ;
   size_t offsetFrom = offsetTo + size ;
   size_t elementMoveCount = This->opcodesUsed - offsetFrom ;

   memmove( sptr, sptr+size, elementMoveCount * sizeof(code_elem) );
   memmove( targetted+offsetTo, targetted+offsetFrom, elementMoveCount );
   This->opcodesUsed -= size;
}


/* Scan for and replace common inefficient instruction sequences
 */
   static void NEAR_CALL
secompilePeephole(struct secompile *This,struct Call *call)
{
   code_elem *sptr, *StartOpcodes, *EndOpcodes, *nxt;
   jsebool again;
   ubyte *targetted;     /* an array of booleans */

   targetted = secompileTargetted(This);
   
   do {
      again = False;
      EndOpcodes = (StartOpcodes=This->opcodes) + This->opcodesUsed;
      for( sptr = StartOpcodes; sptr != EndOpcodes; sptr++ )
      {
         nxt = sptr+1;
         if( IS_CODELIST_DATUM(*sptr) ) INCREMENT_DATUM_CODEPTR(nxt);


         if( nxt<EndOpcodes )
         {
            /* get rid of multiple sourceLineNumber statements in a row */
            if( *nxt==sourceLineNumber && *sptr==sourceLineNumber &&
                !targetted[(size_t)(nxt-This->opcodes)] )
            {
               uint size = (uint) (nxt-sptr);
               secodeDeleteOpcode(sptr,call);
               secompileUpdate(This,-(sint)size,(int)(nxt-This->opcodes)+1,(sint)This->opcodesUsed);
               secompileDelete(This,sptr,targetted,size);
               EndOpcodes -= size;
               again = True;
               continue;
            }
            
            /* get rid of cases where we push something just to pop it again */
            else if( *nxt==popDiscard &&
                ((*sptr)==pushTrue || (*sptr)==pushFalse || (*sptr)==pushClone ||
                 (*sptr)==pushVariable || *sptr==pushValue) &&
                !targetted[(size_t)(nxt-This->opcodes)] )
            {
               uint size = (uint)((nxt+1)-sptr);

               /* nxt+1 because popDiscard has no extension data so uses 1 slot */
               secodeDeleteOpcode(sptr,call);
               secodeDeleteOpcode(nxt,call);
               secompileUpdate(This,-(sint)size,(int)(nxt-This->opcodes)+1,(sint)This->opcodesUsed);
               secompileDelete(This,sptr,targetted,size);

               sptr--;
               EndOpcodes -= size;
               again = True;
               continue;
            }

            /* Optimize the 'opAssign' followed by a pop sequence. popEval acts
             * like a popDiscard since opAssign has already done the forcing of
             * the retrieval of the value (i.e. forced its side effects which is
             * the difference between popDiscard and popEval.)
             */
            else if( ((*nxt)==popDiscard || (*nxt)==popEval) && (*sptr)==opAssign &&
                !targetted[(size_t)(nxt-This->opcodes)] )
            {
               secodeDeleteOpcode(nxt,call);
               secompileUpdate(This,-1,(int)(nxt-This->opcodes)+1,(sint)This->opcodesUsed);
               secompileDelete(This,nxt,targetted,1);
               *sptr = opAssignOnly;
               EndOpcodes -= 1;
               again = True;
               continue;
            }
         }
         if( IS_CODELIST_DATUM(*sptr) ) INCREMENT_DATUM_CODEPTR(sptr);
      }
   } while( again );

   jseMustFree(targetted);
}
#endif /* !defined(JSE_PEEPHOLE_OPTIMIZER) || (0!=JSE_PEEPHOLE_OPTIMIZER) */


/* ---------------------------------------------------------------------- */


/*
 * Fills in an secode class which is the compiled version of the given codes.
 * The given codes comprise a statement (they are actually the function text
 * without the header, which looks just like a block statement.)
 *
 * Compilation is performed using a one token look-ahead recursive descent
 * scheme.
 */
   jsebool NEAR_CALL
secompileCompile(struct LocalFunction *locfunc,struct Call *c)
{
   struct secompile This;
   uint size,oldsize;
   int x;
   jsebool success;   
   
   This.gotoTrack = gototrackerNew();
   This.loopTrack = NULL;
   This.opcodesUsed = 0;
   This.opcodesAlloced = 50;
   This.with_depth = 0;
   This.call = c;
   This.opcodes = jseMustMalloc(code_elem,(uint)(This.opcodesAlloced * \
                                                 sizeof(*(This.opcodes))));
   assert( This.opcodes!=NULL );
   /* must be >0 for things to work right - see below and additem() */
   This.varsAlloced = 10;
   This.varsUsed = 0;
   This.vars = jseMustMalloc(VarName,This.varsAlloced*sizeof(VarName));
   assert( This.vars!=NULL );
   
#  if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
      /* DOS builds, to use minimal memory, make assumptions about using
         only 1 byte */
      assert( 1 == sizeof(*(This.opcodes)) );
      assert( 1 == ( ((ubyte *)(&(This.opcodes[1]))) - \
                     ((ubyte *)(&(This.opcodes[0]))) ) );
#  endif

   This.locfunc = locfunc;
   assert( !(Func_UsesArguments & locfunc->function.flags) );
   assert( !(Func_UsesItsName & locfunc->function.flags) );

   /* back up to encompass source line and file information */
   This.srcCodes = locfunc->FirstCode;
   while( codeTruePrev(This.srcCodes)!=NULL &&
          (codeGetType(codeTruePrev(This.srcCodes))==sourceLineNumber ||
           codeGetType(codeTruePrev(This.srcCodes))==sourceFilename) )
   {
      This.srcCodes = codeTruePrev(This.srcCodes);
   }

   while( codeGetType(This.srcCodes)==sourceLineNumber ||
          codeGetType(This.srcCodes)==sourceFilename )
   {
      /* insert this file info information */
      secompileFileInfoCard(&This);
   }

   success = secompileStatement(&This);
   if( success )
   {
      if( This.srcCodes!=NULL )
      {
         callQuit(c,textcoreEXTRA);
         success = False;
      }

      /* if we fall off the end of the function */
      secompileAddItem(&This,returnVoid);
   }
   else
   {
      /* unsuccessful, free the memory for any loops we are still in */
      while( This.loopTrack )
         secompileEndLoop(&This,0,0);
   }

   /* successful or unsuccessful compile, nothing ought to be left around */
   assert( This.loopTrack==NULL );
   assert( This.with_depth==0 );

   /* patch up all gotos and such */
   gototrackerDelete(This.gotoTrack,c,&This);
   
   /* make room for all of the 'vars' */

   assert( sizeof(*(This.opcodes)) <= sizeof(VarName) );
   size = (1+sizeof(VarName)/sizeof(*(This.opcodes)))*This.varsUsed;
   secompileUpdate(&This,(sint)size,0,(sint)This.opcodesUsed);
   This.opcodes = jseMustReMalloc(code_elem,This.opcodes,(This.opcodesUsed+size)*
                                  sizeof(*(This.opcodes)));
   memmove(This.opcodes+size,This.opcodes,This.opcodesUsed*sizeof(*(This.opcodes)));

   oldsize = This.opcodesUsed;
   This.opcodesUsed = 0;

   This.varsAlloced = 0;
   for( x = 0;x<(int)This.varsUsed;x++ )
   {
      secompileAddItem(&This,declareVar,This.vars[x]);
   }
   assert( This.opcodesUsed==size );
   This.opcodesUsed += oldsize;
   jseMustFree(This.vars);

#  if 0
      secodeListing(&This,c);
#  endif

#  if !defined(JSE_PEEPHOLE_OPTIMIZER) || (0!=JSE_PEEPHOLE_OPTIMIZER)
      secompilePeephole(&This,c);
#  endif
   
   /* clean up any allocated item holders that aren't used */
   locfunc->opcodes = jseMustReMalloc(code_elem,This.opcodes,
                        (locfunc->opcodesUsed = This.opcodesUsed) *
                                      sizeof(*(This.opcodes)));

   
#  if 0
      secodeListing(&This,c);
#  endif
   return success;
}

#endif /* #if (0!=JSE_COMPILER) */

/* ---------------------------------------------------------------------- */
/* secode interpreter                                                     */
/* ---------------------------------------------------------------------- */

   void NEAR_CALL
secodeDeleteOpcode(code_elem *c,struct Call *call)
{
   if ( IS_CODELIST_DATUM(*c) )
   {
      if( *c >= START_VARNAME_CODES &&
          *c <= END_VARNAME_CODES )
      {
         assert( NULL != CODELIST_DATUM(c,VarName) );
         RemoveFromStringTable(call,CODELIST_DATUM(c,VarName));
      }
      else if( pushValue == *c )
      {
         /*
          * If this item is locking something else, free that lock. Currently,
          * only the 'literal' Var * is a lock.
          */
         VarRead *literal = CODELIST_DATUM(c,VarRead *);
         VAR_REMOVE_USER(literal,call);
      }
   }
}

/*
 * requisite cleanup call
 */
   void NEAR_CALL
secodeDeleteOpcodes(code_elem *opcodes,uint opcodesUsed,struct Call *call)
{
   code_elem *EndOpcodes, *c;
   
   assert( NULL != opcodes );
   EndOpcodes = opcodes + opcodesUsed;
   for( c = opcodes; c != EndOpcodes; c++ )
   {
      secodeDeleteOpcode(c,call);
      if ( IS_CODELIST_DATUM(*c) )
      {
         INCREMENT_DATUM_CODEPTR(c);
      }
   }
   jseMustFree(opcodes);
}

/* call any function */

static struct Call * NEAR_CALL secodeEndFunction(struct Call *call);

   struct Call * NEAR_CALL
callFunction(struct Call *call,uint numargs,jsebool newfunc)
{
   /* look back behind the args to find the function to call
    * it has to be a VarRead, by the previous checkFunctionx call
    */
   struct secodeStack *thestack = call->Global->thestack;
   Var *funcvar = SECODE_STACK_PEEK(thestack,numargs+1);
   VarRead *thefunc = (VarRead *)SECODE_STACK_PEEK(thestack,numargs);
   struct Function *function = NULL;
   VarRead *thisvar;
   

   if( VAR_TYPE(thefunc)==VObject )
   {
      if( newfunc )
      {
         VarRead *ConstructorVar;
         /* don't inherit constructors! */
         ConstructorVar = varGetDirectMember(call,thefunc,
                                      GLOBAL_STRING(call,constructor_entry),False);
         if ( NULL != ConstructorVar )
         {
            function = VAR_IS_FUNCTION(ConstructorVar,call)?
               varGetFunction(ConstructorVar,call):
               (struct Function *)NULL;
         }
         else
         {
            function = varGetFunction(thefunc,call);
         }
      }
      else
      {
         function = varGetFunction(thefunc,call);
      }
   }

   if( newfunc )
   {
      VarRead *prop;

      thisvar = constructVarRead(call,VObject);

      /* Copy the _prototype from the constructor.prototype, but if there
       * is not a .prototype then do nothing and let this object
       * default to Function.prototype
       */
      prop = varGetMember(call,thefunc,GLOBAL_STRING(call,orig_prototype_entry));
      if( prop!=NULL
// seb 99.3.16 - This fix is from Nombas.
       && prop != call->Global->ObjectPrototype /* default */
//       && prop != call->Global->FunctionPrototype /* default */
       && VAR_TYPE(prop)==VObject )
      {
         VarRead *ourprop = varCreateMember(thisvar,call,
            GLOBAL_STRING(call,prototype_entry),VUndefined);
         varAssign(ourprop,call,prop);
#        if ( 0 != JSE_DYNAMIC_OBJ_INHERIT )
            /* inherit any dynamic properties of the superclass */
            assert( VObject == VAR_TYPE(thisvar) );
            assert( VObject == VAR_TYPE(ourprop) );
            thisvar->varmem->data.vobject.members.flags
            |= (ourprop->varmem->data.vobject.members.flags & (uword16)HAS_ALL_PROP);
#        endif
         varSetAttributes(ourprop,jseDontEnum);
      }
   }
   else
   {
      if ( !VAR_HAS_REFERENCE(funcvar) || NULL==funcvar->reference.parentObject
        || varIsActivation(funcvar->reference.parentObject) )
         thisvar = CONSTRUCT_VALUE_LOCK(call,call->session.GlobalVariable);
      else
         thisvar = CONSTRUCT_VALUE_LOCK(call,funcvar->reference.parentObject);
   }

   if( function==NULL )
   {
      struct InvalidVarDescription BadDesc;
      if ( FindNames(call,funcvar,BadDesc.VariableName+1,
                     sizeof(BadDesc.VariableName)/sizeof(BadDesc.VariableName[0])-3) ) {
         BadDesc.VariableName[0] = '\"';
         strcat_jsechar(BadDesc.VariableName,UNISTR("\" "));
      } else {
         BadDesc.VariableName[0] = '\0';
      } /* endif */
      callQuit(call,textcoreNOT_FUNCTION_VARIABLE,BadDesc.VariableName);
      VAR_REMOVE_USER(thisvar,call);
      return call;
   }

   assert( function!=NULL );
   function->stored_in = thefunc;

   if( functionDoCall(function,call,thefunc,(uint)numargs,thisvar) )
   {
      assert( call->pChildCall!=NULL );
      /* a new call has been stacked */
      call->pChildCall->newfunc = newfunc;
      return call->pChildCall;
   }
   else
   {
      assert( call->pChildCall!=NULL );
      /* cleanup function call immediately */
      call->pChildCall->newfunc = newfunc;
      secodeEndFunction(call->pChildCall);
      assert( call->pChildCall==NULL );
      return call;
   }
}


   static struct Call * NEAR_CALL
secodeEndFunction(struct Call *call)
{
   VarRead *retval;             /* return of the function */
   Var *tmpVar;
   struct secodeStack *stack = call->Global->thestack;
   VarRead *thefunc;
   jsebool newfunc = call->newfunc;
   jsebool no_clean = call->no_clean;
   

   /* thisvar still has an extra user on it from the function call */
   VarRead *thisvar = call->pCurrentThisVar;


   struct Call *oldcall = call->pPreviousCall;
   retval = functionFinishCall(call->pFunction,call,call->pFunction->stored_in,
                               call->pInputVariableCount,call->pCurrentThisVar);
   call = oldcall;

   if( no_clean )
   {
      SECODE_STACK_PUSH(stack,retval);
   }
   else
   {
      /* remove the function and its readable counterpart */
      thefunc = (VarRead *)SECODE_STACK_POP(stack);
      tmpVar = SECODE_STACK_POP(stack);

      VAR_REMOVE_USER(tmpVar,call);

#     if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
      {
         int i;
         for( i = call->Global->currentCacheSize - 1; i >= 0; i-- )
            VAR_REMOVE_USER(call->Global->variableCache[i].var,call);
         assert(i == -1);
         call->Global->currentCacheSize = 0;
      }
#     endif
      
      assert( retval!=NULL );
      if( newfunc )
      {
         if( VAR_TYPE(retval)==VObject )
         {
            VAR_REMOVE_USER(thisvar,call);
            SECODE_STACK_PUSH(stack,retval);
         }
         else
         {
            /* undefined means no return value, otherwise it is a
             * runtime error */
            if( VAR_TYPE(retval)!=VUndefined )
            {
               callQuit(call,textcoreNOT_OBJECT);
            }

            /* return the constructed object either way - either 'cause its
             * the right thing to do, or because the error still must return
             * something sensible to not confuse our caller.
             */
            VAR_REMOVE_USER(retval,call);
            SECODE_STACK_PUSH(stack,thisvar);
         }
      }
      else
      {
         SECODE_STACK_PUSH(stack,retval);
      }

      /* for calling new, the this var is passed back so we don't free it */
      if( !newfunc )
      {
         VAR_REMOVE_USER(thisvar,call);
      }

      VAR_REMOVE_USER(thefunc,call);
   }
   
   return call;
}


/* main interpreter */


/* Set up a local function to be ready to interpret */
   void NEAR_CALL
secodeInterpInit(struct LocalFunction *locfunc,struct Call *call)
{
   code_elem *sptr;

   /* all variables are initialized as undefined when the scope is enterred but
    * receive their actual initialization when that section of the code is
    * encountered. This does the initial creation to Undefined (section 12.2)
    *
    * The current code makes sure all of these declares are the first things
    * in the function.
    */
   for( sptr = locfunc->opcodes; (*sptr)==declareVar; sptr++ )
   {
      VarName name = CODELIST_DATUM(sptr,VarName);
      VarRead *local = varMakeMember(call->VariableObject,call,
                                     name,VUndefined);

      /* 10.2.3 */
      if( !varSameObject(call->VariableObject,call->session.GlobalVariable) )
      {
         varSetAttributes(local,jseDontDelete);
            
#        if defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS)
            SECODE_STACK_PUSH(call->Global->thestack,name);
            SECODE_STACK_PUSH(call->Global->thestack,local);

            call->localCacheCount++;
#        endif
      }
         
      INCREMENT_DATUM_CODEPTR(sptr);
   }
   
   call->sptr = sptr;
   call->with_count = 0;
}


   struct Call * NEAR_CALL
secodeInterpret(struct Call *call)
{
   struct secodeStack *stack = call->Global->thestack;
#  if (0!=JSE_CYCLIC_CHECK) && (0==JSE_FULL_CYCLIC_CHECK)
      jsebool ThoroughCheckOnPopDiscard = True;
#  endif
#  if (defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS)) \
   || (defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS))
      sint iii;
#  endif
   
   if( call->pFunction==NULL ) return NULL;

   /* now just execute the code */
   for ( ;; (call->sptr)++ )
   {

      codeval t = (codeval)*(call->sptr);
      assert( isValidCodeval(t) );
      assert( (call->sptr) < ((struct LocalFunction *)(call->pFunction))->opcodes +
                             ((struct LocalFunction *)(call->pFunction))->opcodesUsed );

      assert( t < NotUsedAfterCompilation );

      if( callQuitWanted(call) )
      {
         call = secodeEndFunction(call);
         break;
      }
      
      if ( BeginOpList <= t )
      {
         /* operators share many things in common, so bundle all of
          * their behaviors here
          */
         Var *lvar, *rvar;
         VarRead *rlvar, *rrvar = NULL;
         VarWrite *wlvar;
         const struct OpDescription _NEAR_ * OpDesc = getOpDescription(t);
         VarRead *ResultVar = NULL;
#        if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
            VarRead * property;
#        endif

         assert( t <= EndOpList );
         assert( t == OpDesc->WhichOp );
         
         if( (t >= BEGIN_OP_READ_RVAR && t <= END_OP_READ_RVAR) ||
             t == opAssign || t==opAssignOnly)
         {
            assert( OpDesc->Flags & OP_READ_RVAR );
            rvar = SECODE_STACK_POP(stack);
            rrvar = GET_READABLE_VAR(rvar,call);
            VAR_REMOVE_USER(rvar,call);
         }
         lvar = SECODE_STACK_POP(stack);

         if( t <= END_OP_WRITE_LVAR && t >= BEGIN_OP_WRITE_LVAR )
         {
            assert( OpDesc->Flags & OP_WRITE_LVAR );
            wlvar = getWriteableVar(lvar,call);            
         }

         /* No need to check end of READ_LVAR because its the last */
         if ( t >= BEGIN_OP_READ_LVAR )
         {
            assert( OpDesc->Flags & OP_READ_LVAR );
            rlvar = GET_READABLE_VAR(lvar,call);
            VAR_REMOVE_USER(lvar,call);
            /* Don't check BEGIN because its same as READ_LVAR */
            if ( t <= END_OP_POST_WRITE )
            {
               /* postIncrement and postDecrement push on stack before
                  value is used */
               VarWrite *saveval = constructVarRead(call,VAR_TYPE(rlvar));
               assert( OpDesc->Flags & OP_POST_WRITE );
               varAssign(saveval,call,rlvar);
               SECODE_STACK_PUSH(stack,saveval);
            }
         }
         else
         {
            /* even if OP_READ_LVAR is not set (for delete or typeof
             * operator), pretend here that it read that way anyway. Aside
             * from delete or typeof it is also always
             * freed, so make sure it points to something
             */
            rlvar = (VarRead *)lvar;
         }
         assert( NULL != OpDesc->opfunc );
#        if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
            if( (t >= BEGIN_OP_READ_RVAR && t <= END_OP_READ_RVAR) && 
                VObject == VAR_TYPE(rlvar) &&
                HAS_OPERATOR_PROPERTY(&(rlvar->varmem->data.vobject.members)) &&
                (property = varGetMember(call,rlvar,GLOBAL_STRING(call,operator_entry)))
                != NULL &&
                VARMEM_IS_FUNCTION(property->varmem,call) )
            {
               VarName operatorName = EnterIntoStringTable(call,OpDesc->TokenText,strlen_jsechar(OpDesc->TokenText));
               if( rrvar != NULL )
                  varAddUser(rrvar); 
 
               varCallDynamicProperty(rlvar,call,property,OFF_OPERATOR_PROP,
                                      operatorName,rrvar,&ResultVar);
               RemoveFromStringTable(call,operatorName);

               if( ResultVar == call->Global->special_operator_var)
               {
                  VAR_REMOVE_USER(ResultVar,call);
                  ResultVar = (*OpDesc->opfunc)(call,rlvar,rrvar,t);
               }


            }
            else
#        endif
         ResultVar = (*OpDesc->opfunc)(call,rlvar,rrvar,t);
         if ( NULL == ResultVar )
         {
            assert( callQuitWanted(call) );
            ResultVar = constructVarRead(call,VUndefined);
         }
         assert( NULL != ResultVar );
         if ( (t >= BEGIN_OP_WRITE_LVAR && t <= END_OP_WRITE_LVAR) )
         {
            assert( OpDesc->Flags & OP_WRITE_LVAR );
            if ( varIsLvalue(wlvar) )
            {
               /* if result is not an lvalue (such as a constant) then don't
                * assign directly but instead make a copy of it.
                */
               if ( varIsLvalue(ResultVar ) )
                  varAssign(wlvar,call,ResultVar);
               else
                  varCopyAssign(wlvar,call,ResultVar);
            }
            else
            {
#              if !defined(JSE_SHORT_RESOURCE) || (0==JSE_SHORT_RESOURCE)
                  jsechar VarName[60];
                  if ( FindNames(call,wlvar,VarName+1,sizeof(VarName)/sizeof(VarName[0])-5) )
                  {  
                     VarName[0] = '(';
                     strcat_jsechar(VarName,UNISTR(") "));
                  }
                  else
                     VarName[0] = '\0';
                  callQuit(call,textcoreMUST_BE_LVALUE,VarName);
#              else
                  callQuit(call,textcoreMUST_BE_LVALUE,"");
#              endif
            }
            VAR_REMOVE_USER(wlvar,call);
            
         }
         if ( (t >= BEGIN_OP_POST_WRITE && t <= END_OP_POST_WRITE) ||
              t==opAssignOnly )
         {
            /* postIncrement and postDecrement pushed the previous rlvar */
            VAR_REMOVE_USER(ResultVar,call);
         }
         else
         {
            SECODE_STACK_PUSH(stack,ResultVar);
         }
         VAR_THOROUGH_REMOVE_USER(rlvar,call); /* may be linked-list as operator */
         if ( (t >= BEGIN_OP_READ_RVAR && t <= END_OP_READ_RVAR) ||
              opAssign == t || t==opAssignOnly )
         {
            assert( OpDesc->Flags & OP_READ_RVAR );
            VAR_THOROUGH_REMOVE_USER(rrvar,call); /* may be linked-list as operator */
#           if (0!=JSE_CYCLIC_CHECK) && (0==JSE_FULL_CYCLIC_CHECK)
               if ( opAssign == t || t==opAssignOnly )
                  ThoroughCheckOnPopDiscard = False;
#           endif
         }
      }
      else
      {
         switch( t )
         {
            case sourceFilename:
               if( call->filename != NULL )
                  RemoveFromStringTable(call,call->filename);
               call->filename = CODELIST_DATUM((call->sptr),VarName);
               AddStringUser(call->filename);
               SKIP_CODELIST_DATUM((call->sptr),VarName);
               break;
            case sourceLineNumber:
               call->linenum = CODELIST_DATUM((call->sptr),uint);
               SKIP_CODELIST_DATUM((call->sptr),uint);
               break;
            case mayIContinue:
               (call->sptr)++;
               goto exit_out;
            case withoutVar:
               assert( call->with_count>0 );
               call->with_count--;
               RemoveScopeObject(call);
#              if (defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)) || \
                  (defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS))
                  assert( call->Global->currentCacheSize == -1 );
                  call->Global->currentCacheSize = 0;
#              endif
               break;
            case returnVoid:
               {
                  struct Call *newc = secodeEndFunction(call);
                  assert( newc!=call );
                  call = newc;
                  goto exit_out;
               }
            case returnVar:
               callReturnVar(call,SECODE_STACK_POP(stack),jseRetTempVar);
               break;

            case gotoAlways:
               /* we don't want infinite loops - only a bug can generate one */
               assert( ((struct LocalFunction *)(call->pFunction))->opcodes +
                       CODELIST_DATUM((call->sptr),uint) != (call->sptr) );
               call->sptr = ((struct LocalFunction *)(call->pFunction))->opcodes +
                  CODELIST_DATUM((call->sptr),uint) - 1;
               break;
            case gotoFalse:
            case gotoTrue:
            {
               Var *tmp = SECODE_STACK_POP(stack);
               VarRead *val = GET_READABLE_VAR(tmp,call);
               jsebool cond = ToBoolean(call,val);
               VAR_REMOVE_USER(val,call);
               VAR_REMOVE_USER(tmp,call);

               /* we don't want infinite loops - only a bug can generate one */
               assert(((struct LocalFunction *)(call->pFunction))->opcodes +
                      CODELIST_DATUM((call->sptr),uint) != call->sptr );

               if( (gotoTrue==t) == cond )
                  call->sptr = ((struct LocalFunction *)(call->pFunction))->opcodes +
                     CODELIST_DATUM((call->sptr),uint) - 1;
               else
                  SKIP_CODELIST_DATUM((call->sptr),uint);
            }  break;
            case pushValue:
            {
               VarRead *pushval = CODELIST_DATUM((call->sptr),VarRead *);
               SKIP_CODELIST_DATUM((call->sptr),VarRead *);
               assert( !varIsLvalue(pushval) );
               varAddUser(pushval);
               SECODE_STACK_PUSH(stack,pushval);
            }  break;
            case pushVariable:
            {
               VarName name = CODELIST_DATUM(call->sptr,VarName);
               Var * var = NULL;
               struct Global_ * global = call->Global;
               
#              if defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS)
                  struct Var ** current;
                     
                  if( global->currentCacheSize >= 0 )
                  {
                     for( current = global->thestack->items + call->localCacheLocation,iii = (sint)call->localCacheCount; 
                          iii--; current += 2 )
                     {
                        if( (VarName)(*current) == name )
                        {
                           var = *(current+1);
                           varAddUser(var);
                           break;
                        }
                     }
                  }
#              endif
               
#              if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
                  if( var == NULL )
                  {
                     for( iii = 0; iii < global->currentCacheSize; iii++ )
                     {
                        if( global->variableCache[iii].name == name )
                        {
                           var = global->variableCache[iii].var;
                           varAddUser(var);
                           break;
                        }
                     }
                  }
#              endif               
               /* Wasn't found, so we must look it up */
               if( var == NULL )
               {
                  var = FindVariableByName(call,name);
                  if( var == NULL && 
                      (jseOptReqVarKeyword & global->ExternalLinkParms.options) )
                  {
                     var = constructVarRead(call,VUndefined);
                     
                     callQuit(call,textcoreFUNC_OR_VAR_NOT_DECLARED,name);
                  }
                  else if( var == NULL )
                  {
                     var = constructReference(call,NULL,name);
                  }
                  
#                 if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
                     else if( -1 != global->currentCacheSize &&
                           iii != global->maxCacheSize)
                     {
                        global->variableCache[call->Global->currentCacheSize].name = name;
                        global->variableCache[call->Global->currentCacheSize++].var = var;
                        varAddUser(var);
                     }
#                 endif
               }
               
               assert( var != NULL );
               SECODE_STACK_PUSH(stack,var);
               
               SKIP_CODELIST_DATUM(call->sptr,VarName);
               break;
             }
            case pushTrue:
            case pushFalse:
            {
               VarRead *val = constructVarRead(call,VBoolean);
               if ( t == pushTrue )
               {
                  varPutBoolean(val,True);
               }
               else
               {
                  assert( !varGetBoolean(val) );
               }
               SECODE_STACK_PUSH(stack,val);
               break;
            }
            case pushNumber:
            {
               VarRead *val = constructVarRead(call,VNumber);
               varPutLong(val,CODELIST_DATUM((call->sptr),uint));
               SKIP_CODELIST_DATUM((call->sptr),uint);
               SECODE_STACK_PUSH(stack,val);
               break;
            }
            case pushObject:
            {
               VarRead *vr = constructVarRead(call,VObject);
               SECODE_STACK_PUSH(stack,vr);
               break;
            }
            case pushClone:
            {
               Var *v = SECODE_STACK_PEEK(stack,0);
               varAddUser(v);
               SECODE_STACK_PUSH(stack,v);
               break;
            }
            case checkFunctionCall:
            case checkFunctionNew:
            {
               Var *v = SECODE_STACK_PEEK(stack,0);
               varAddUser(v);
               SECODE_STACK_PUSH(stack,v);
            }
            /* currently it just adds the function as readable for
             * performance reasons - that is why we fallthru into getValue
             * We need both versions on the stack.
             */
            case getValue:
            {
               Var *v = SECODE_STACK_POP(stack);
               VarRead *vr = GET_READABLE_VAR(v,call);
               SECODE_STACK_PUSH(stack,vr);
               VAR_REMOVE_USER(v,call);
               break;
            }

            /* handling for in. */
            case pushIterator:
            {
               Var *v = SECODE_STACK_POP(stack);
               VarRead *var = GET_READABLE_VAR(v,call);
               VAR_REMOVE_USER(v,call);
               if( VAR_TYPE(var)!=VObject )
               {
                  VarRead *tmp = convert_var(call,var,jseToObject);
                  VAR_REMOVE_USER(var,call);
                  var = tmp;
               }
               assert( VAR_TYPE(var)==VObject );
               SECODE_STACK_PUSH(stack,var);
               SECODE_STACK_PUSH(stack,NULL);
               break;
            }

            case gotoForIn:
            {
               Var *vpeek, *v;
               VarRead *current, *obj, *next;
               VarName vName;
                
               v = SECODE_STACK_POP(stack);
               current = (v!=NULL)?GET_READABLE_VAR(v,call):(VarRead *)NULL;
               vpeek = SECODE_STACK_PEEK(stack,0);
               obj = GET_READABLE_VAR(vpeek,call);
               next = varGetNext(obj,call,current,&vName);

               if( v!=NULL )
               {
                  VAR_REMOVE_USER(v,call);
               }

               while( next!=NULL && (varGetAttributes(next) & jseDontEnum) )
                  next = varGetNext(obj,call,next,&vName);
               if( next!=NULL )
               {
                  stringLengthType vNameStrLen;
                  const jsechar * vNameStr;
                  VarWrite *wr = getWriteableVar(SECODE_STACK_PEEK(stack,1),
                                                 call);
                  varConvert(wr,call,VString);
                  vNameStr = GetStringTableEntry(call,vName,&vNameStrLen);
                  varPutStringLen(wr,call,vNameStr,vNameStrLen);
                  VAR_REMOVE_USER(wr,call);
                  /* we don't want infinite loops -
                     only a bug can generate one */
                  assert( ( ((struct LocalFunction *)(call->pFunction))->opcodes +
                            CODELIST_DATUM((call->sptr),uint)) != (call->sptr) );
                  call->sptr =  ((struct LocalFunction *)(call->pFunction))->opcodes +
                     CODELIST_DATUM((call->sptr),uint) - 1;
               }
               else
               {
                  SKIP_CODELIST_DATUM((call->sptr),uint);
               }

               if ( NULL != next )
                  varAddUser(next);
               SECODE_STACK_PUSH(stack,next);
               VAR_REMOVE_USER(obj,call);
               if( current!=NULL )
               {
                  VAR_REMOVE_USER(current,call);
               }
               break;
            }
            case popIterator:
            {
               Var *v = SECODE_STACK_POP(stack);
               if( v!=NULL )
               {
                  VAR_REMOVE_USER(v,call);
               }
               /* get rid of object and iterator */
               v = SECODE_STACK_POP(stack);
               VAR_REMOVE_USER(v,call);
               v = SECODE_STACK_POP(stack);
               VAR_REMOVE_USER(v,call);
               break;
            }

            case popDiscard:
            {
               Var *tmpvar = SECODE_STACK_POP(stack);
#              if (0!=JSE_CYCLIC_CHECK) && (0==JSE_FULL_CYCLIC_CHECK)
                  if ( ThoroughCheckOnPopDiscard )
                  {
                     VAR_THOROUGH_REMOVE_USER(tmpvar,call);
                  }
                  else
#              endif
                  {
                     VAR_REMOVE_USER(tmpvar,call);
                  }
               break;
            }
            
            /* calling functions */
            case functionCall:
            case functionNew:
               {
                  struct Call *newc;
                  
#                 if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
                     for( iii = (sint)(call->Global->currentCacheSize - 1); iii >= 0; iii-- )
                        VAR_REMOVE_USER(call->Global->variableCache[iii].var,call);
                     call->Global->currentCacheSize = (sword16)(iii+1);
#                 endif
                  
                  newc = callFunction(call,CODELIST_DATUM((call->sptr),uint),
                                                   functionNew == t);
                  SKIP_CODELIST_DATUM((call->sptr),uint);
                  if( newc!=call )
                  {
                     /* enterring a new function */
                     (call->sptr)++;
                     call = newc;
                     goto exit_out;
                  }
                  break;
               }
            
            /* variable access */
            case structureMemberName:
               do_op_structureMember(call,stack,True,
                                     CODELIST_DATUM((call->sptr),VarName),0);
               SKIP_CODELIST_DATUM((call->sptr),VarName);
               break;
            case popEval:
            {
               Var *tmp = SECODE_STACK_POP(stack);
               VarRead *val = GET_READABLE_VAR(tmp,call);
               /* there, we've forced any side effects, no cleanup */
               VAR_REMOVE_USER(val,call);
               VAR_REMOVE_USER(tmp,call);
               break;
            }
            case getParamValue:
            {
               struct Function *function = NULL;
               Var *param;
               uint offset;
               VarRead *func;
               uword8 varAttrib = 0;
               /* it is guaranteed to be a varread */
               offset = CODELIST_DATUM((call->sptr),uint);
               func = (VarRead *)SECODE_STACK_PEEK(stack,offset);
               SKIP_CODELIST_DATUM((call->sptr),uint);
               param = SECODE_STACK_PEEK(stack,0);

               if( VAR_TYPE(func)==VObject )
                  function = varGetFunction(func,call);
 
               if( function != NULL && functionTestIfLocal(function) &&
                   offset <= ((struct LocalFunction*)function)->InputParameterCount)
               {
                   varAttrib = ((struct LocalFunction*)function)->VarAttrib
                               [((struct LocalFunction*)function)->InputParameterCount-offset];
                   assert(varAttrib == 0 || varAttrib == 1);
               }

               if( (function!=NULL && !functionCBehavior(function) && !varAttrib) ||
                   !varIsLvalue(param) )
               {
                  VarRead *pvar,*copy;

                  param = SECODE_STACK_POP(stack);
                  pvar = GET_READABLE_VAR(param,call);
                  /* Don't put the actual variable on the stack, but a copy
                   * of it, and so make a copy on the stack and remove the
                   * old one
                   */
                  copy = constructVarRead(call,VAR_TYPE(pvar));
                  /* if type of source is known then assign it, else leave
                     it unknown */
                  varAssign(copy,call,pvar);
                  VAR_REMOVE_USER(pvar,call);
                  varSetLvalue(copy,True);
                  SECODE_STACK_PUSH(stack,copy);
                  VAR_REMOVE_USER(param,call);
                  break;
               }
               /* else it is to be passed by reference-fallthru intentional */
            }
            case getReferenceValue:
            {
               /* for pass-by-reference variable watch out for case where the object is NULL
                * and so is the varmem; in that case the parent is not assigned yet and so
                * determine now that the parent is the variable scope or global object.  This
                * is not quite as perfect as keeping it unreferenced and letting the "unknown
                * variable" message come later, but it is fair to assume that passing-by-reference
                * is as good as assigning-to when determining what the parent object is.
                */
               Var *param = SECODE_STACK_PEEK(stack,0);
               if ( NULL == param->varmem  &&  NULL == param->reference.parentObject )
               {
                  /* assign parent object, much as in getWriteableVar */
                  param->reference.parentObject
                     = (jseOptDefaultLocalVars & call->Global->ExternalLinkParms.options)
                     ? call->VariableObject : call->session.GlobalVariable ;
               }
            }  break;
            case withVar:
            {
               Var *tmp = SECODE_STACK_POP(stack);
               VarRead *val = GET_READABLE_VAR(tmp,call);

               call->with_count++;
               VAR_REMOVE_USER(tmp,call);
               if( VAR_TYPE(val)!=VObject )
               {
                  VarRead *val2 = convert_var(call,val,jseToObject);
                  VAR_REMOVE_USER(val,call);
                  val = val2;
               }
               assert( VAR_TYPE(val)==VObject );
               AddScopeObject(call,val);
#              if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
                  for( iii = (sint)(call->Global->currentCacheSize - 1); iii >= 0; iii-- )
                     VAR_REMOVE_USER(call->Global->variableCache[iii].var,call);
                  /* Turn off cache in with statements */
#              endif
#              if (defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)) || \
                  (defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS))
                  call->Global->currentCacheSize = -1;
#              endif
               break;
            }
            case structureMember:
               do_op_arrayMember(call,stack);
               break;

#           ifndef NDEBUG
            /* shouldn't do the default! */
            case declareVar:
               /* these are done once when the scope is enterred */
               assert( False );
               break;
            default: assert( False ); break;
#           endif
         }
      }
   }

exit_out:
   
   return call;
}

/*
 * ----------------------------------------------------------------------
 * Manage a fast stack - during typical program execution, it never grows
 * which really helps speed.
 * ----------------------------------------------------------------------
 */

   struct secodeStack * NEAR_CALL
secodestackNew(void)
{
   struct secodeStack *this =
      jseMustMalloc(struct secodeStack,sizeof(struct secodeStack));

   this->used = 0;
#  if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
      this->alloced = 10;
#  else
      this->alloced = 200;
#  endif
   this->items = jseMustMalloc(Var *,(uint)(this->alloced * sizeof(Var *)));

   return this;
}


   void NEAR_CALL
secodestackDelete(struct secodeStack *this,struct Call *call)
{
   while( this->used )
   {
      /* in case error, clean up the stack */
      Var *tmp = SECODE_STACK_POP(this);
      VAR_REMOVE_USER(tmp,call);
   }

   assert( this->items!=NULL );
   jseMustFree(this->items);
   jseMustFree(this);
}


   void NEAR_CALL
secodestackExtend(struct secodeStack *this)
{
   /* if the stack is grown, most likely we have some recursion. The
    * bigger it needs to be, the more we assume it will need to be in
    * the future, thus we double it each time.
    */
#  if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
      this->alloced += 10;
#  else
      this->alloced += this->alloced;
#  endif
   this->items =
      jseMustReMalloc(Var *,this->items,(uint)(this->alloced * sizeof(Var *)));
}

/* ---------------------------------------------------------------------- */
#if !defined(JSE_INLINES) || (0==JSE_INLINES)
   struct Var * NEAR_CALL
SECODE_STACK_POP(struct secodeStack *This)
{
   return This->items[--(This->used)];
}
   void NEAR_CALL
SECODE_STACK_PUSH(struct secodeStack *This,struct Var *var)
{
   if( This->used >= This->alloced )
      secodestackExtend(This);
   This->items[This->used++] = (var);
}
   Var * NEAR_CALL
SECODE_STACK_PEEK(struct secodeStack *This,uint offset)
{
   return This->items[ This->used - offset - 1 ];
}
#endif /* defined(__JSE_DOS6__) */


/*****************************************************************/

/***************************************************
 ************ READ/WRITE TOKENIZED CODE ************
 ***************************************************/

#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   static uint NEAR_CALL
RelativeGotoFromAbsolute(code_elem *opcodes,uint AbsoluteGoto)
{
   code_elem *end_opcodes = opcodes + AbsoluteGoto;
   uint RelativeGoto;
   for ( RelativeGoto = 0; opcodes < end_opcodes; opcodes++, RelativeGoto++ )
   {
      if ( IS_CODELIST_DATUM(*opcodes) )
         INCREMENT_DATUM_CODEPTR(opcodes);
   }
   assert( opcodes==end_opcodes );
   return RelativeGoto;
}

   void NEAR_CALL
secodeTokenWriteList(struct Call *call,struct TokenSrc *tSrc,
                     struct LocalFunction *locfunc)
{
   code_elem c, *cptr, *endptr;

   /* write out each token as a byte, followed by data for that token */
   endptr = (cptr = locfunc->opcodes) + locfunc->opcodesUsed ;
   assert( cptr < endptr );
   do {
      tokenWriteByte(tSrc,(uword8)(c = *cptr));
      if ( IS_CODELIST_DATUM(c) )
      {
         if ( c <= END_INT_CODES )
         {
            /* write an integer value */
            /* signed or unsigned can safely be written unsigned, they'll
             * be read back the same way */
            uint i = CODELIST_DATUM(cptr,uint);
            tokenWriteLong( tSrc,
                            ( c <= END_GOTO_CODES )
                          ? (slong) RelativeGotoFromAbsolute(locfunc->opcodes,i)
                          : (slong) i );
            SKIP_CODELIST_DATUM(cptr,uint);
         }
         else
         {
            if ( c <= END_VARNAME_CODES )
            {
               /* write a string value */
               VarName vname = CODELIST_DATUM(cptr,VarName);
               assert( START_PTR_CODES <= c  &&  c <= END_PTR_CODES );
               assert( NULL != GetStringTableEntry(call,vname,NULL) );
               tokenWriteString(call,tSrc,vname);
            }
            else
            {
               /* write a literal variable value */
               VarRead *v = CODELIST_DATUM(cptr,VarRead *);
               TokenWriteVar(call,tSrc,v);
            }
            SKIP_CODELIST_DATUM(cptr,void *);
         }
      }
   } while ( ++cptr < endptr );

   /* write final code to show this is the end of the token list */
   tokenWriteByte(tSrc,(uword8)END_FUNC_OPCODES);
}
#endif


#if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)
   static uint NEAR_CALL
AbsoluteGotoFromRelative(code_elem *opcodes,uint RelativeGoto)
{
   code_elem *c;
   for ( c = opcodes; 0 != RelativeGoto--; c++ )
   {
      if ( IS_CODELIST_DATUM(*c) )
         INCREMENT_DATUM_CODEPTR(c);
   }
   return (uint)(c - opcodes);
}

   void NEAR_CALL
secodeTokenReadList(struct Call *call,struct TokenDst *tDst,
                    struct LocalFunction *locfunc)
{
   struct secompile This;
   code_elem c, *sptr, *EndOpcodes;

   This.call = call;
   This.opcodesAlloced = 50;
   This.opcodesUsed = 0;
   This.locfunc = locfunc;
   This.opcodes = jseMustMalloc(code_elem,(uint)(This.opcodesAlloced * \
                                                 sizeof(*(This.opcodes))));
   This.varsUsed = 0;
   This.varsAlloced = 0;

   while ( (uword8)END_FUNC_OPCODES != (uword8)(c = tokenReadByte(tDst)) )
   {
      if ( IS_CODELIST_DATUM(c) )
      {
         if ( c <= END_INT_CODES )
         {
            /* read an integer value */
            /* was written as unsigned whether it was signed or not,
             * so read it back that way */
            secompileAddItem( &This,(int)c,(uint)tokenReadLong(tDst) );
         }
         else
         {
            if ( c <= END_VARNAME_CODES )
            {
               /* read a string value */
               VarName name = tokenReadString(call,tDst);
               secompileAddItem(&This,(int)c,name);
            }
            else
            {
               /* read a literal variable value */
               secompileAddItem(&This,(int)c,TokenReadVar(call,tDst));
            }
         }
      }
      else
      {
         secompileAddItem(&This,(int)c);
      }
   }

   /* local function will remember the opcodes used */
   locfunc->opcodes = jseMustReMalloc(code_elem,This.opcodes,
      (locfunc->opcodesUsed=This.opcodesUsed) * sizeof(*(This.opcodes)) );

   /* goto addresses are still relative; turn them into absolutes */
   EndOpcodes = (sptr=locfunc->opcodes) + locfunc->opcodesUsed;
   for( ; sptr != EndOpcodes; sptr++ )
   {
      assert( sptr < EndOpcodes );
      if ( IS_CODELIST_DATUM(*sptr) )
      {
         if ( *sptr <= END_GOTO_CODES )
         {
            CODELIST_DATUM(sptr,uint)
            = AbsoluteGotoFromRelative(locfunc->opcodes,
                                       CODELIST_DATUM(sptr,uint));
         }
         INCREMENT_DATUM_CODEPTR(sptr);
      }
   }
   assert( sptr == EndOpcodes );
}
#endif
