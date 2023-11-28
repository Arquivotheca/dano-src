/* Statemnt.c   Execute statements in a local function.
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

#include "srccore.h"
#if (0!=JSE_COMPILER)

/* ---------------------------------------------------------------------- */
/* Statement compilers. They mirror the stuff in the old 'statement.cpp'  */
/* as much as possible.                                                   */
/* ---------------------------------------------------------------------- */

jsebool NEAR_CALL secompileIf(struct secompile *this)
{
   uint ptr,elseptr;

   assert( codeGetType(this->srcCodes)==statementIf );
   secompileAdvancePtr(this);
   if( codeGetType(this->srcCodes)!=EvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_IF);
      return False;
   }
   secompileAdvancePtr(this);

   if( !secompileExpression(this) ) return False;

   if( codeGetType(this->srcCodes)!=EndEvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_IF);
      return False;
   }
   secompileAdvancePtr(this);
   ptr = secompileCurrentItem(this);
   secompileAddItem(this,gotoFalse,0);
   if( !secompileStatement(this) ) return False;

   elseptr = secompileCurrentItem(this);
   if( codeGetType(this->srcCodes)==statementElse )
   {
      secompileAddItem(this,gotoAlways,0);
   }
   secompileFixupGotoItem(this,ptr,secompileCurrentItem(this));
   if( codeGetType(this->srcCodes)==statementElse )
   {
      secompileAdvancePtr(this);
      if( !secompileStatement(this) ) return False;
      secompileFixupGotoItem(this,elseptr,secompileCurrentItem(this));
   }
   return True;
}


jsebool NEAR_CALL secompileWhile(struct secompile *this)
{
   uint top_of_loop,ptr;

   assert( codeGetType(this->srcCodes)==statementWhile );
   secompileAdvancePtr(this);

   secompileNewLoop(this);

   top_of_loop = secompileCurrentItem(this);

   if( codeGetType(this->srcCodes)!=EvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_WHILE);
      return False;
   }

   secompileAdvancePtr(this);
   if( !secompileExpression(this) ) return False;

   if( codeGetType(this->srcCodes)!=EndEvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_WHILE);
      return False;
   }
   secompileAdvancePtr(this);
   ptr = secompileCurrentItem(this);
   secompileAddItem(this,gotoFalse,0);

   if( !secompileStatement(this) ) return False;
   secompileAddItem(this,gotoAlways,top_of_loop);
   secompileFixupGotoItem(this,ptr,secompileCurrentItem(this));

   secompileEndLoop(this,secompileCurrentItem(this),top_of_loop);
   return True;
}


jsebool NEAR_CALL secompileDo(struct secompile *this)
{
   uint ptr;

   assert( codeGetType(this->srcCodes)==statementDo );
   secompileAdvancePtr(this);

   ptr = secompileCurrentItem(this);

   secompileNewLoop(this);

   if( !secompileStatement(this) ) return False;
   if( codeGetType(this->srcCodes)!=statementWhile )
   {
      callQuit(this->call,textcoreBAD_DO);
      return False;
   }
   secompileAdvancePtr(this);
   if( codeGetType(this->srcCodes)!=EvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_DO);
      return False;
   }
   secompileAdvancePtr(this);

   if( !secompileExpression(this) ) return False;

   if( codeGetType(this->srcCodes)!=EndEvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_DO);
      return False;
   }
   secompileAdvancePtr(this);

   secompileAddItem(this,gotoTrue,ptr);

   /* eat statement terminator if present */
   if( codeGetType(this->srcCodes)==StatementEnd ) secompileAdvancePtr(this);

   secompileEndLoop(this,secompileCurrentItem(this),ptr);
   return True;
}


jsebool NEAR_CALL secompileVar(struct secompile *this)
{
   assert( codeGetType(this->srcCodes)==declareVar );

   do {
      secompileAdvancePtr(this);
      if( codeGetType(this->srcCodes)!=pushVariable )
      {
         callQuit(this->call,textcoreVAR_NEEDS_VARNAME);
         return False;
      }

      secompileAddItem(this,declareVar,codeGetName(this->srcCodes));

      /* assignment statements can't have commas in them */
      if( !secompileOperatorExpression(this,PRI_ASSIGN) ) return False;
      /* we don't do anything with the value */
      secompileAddItem(this,popDiscard);

   } while( codeGetType(this->srcCodes)==popEval );

   /* eat statement terminator if present */
   if( codeGetType(this->srcCodes)==StatementEnd ) secompileAdvancePtr(this);
   return True;
}


jsebool NEAR_CALL secompileFor(struct secompile *this)
{
   jsebool has_init = False;
   VarName index = NULL;

   assert( codeGetType(this->srcCodes)==statementFor );
   secompileAdvancePtr(this);

   secompileNewLoop(this);

   if( codeGetType(this->srcCodes)!=EvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_FOR_STATEMENT);
      return False;
   }
   secompileAdvancePtr(this);
   if( codeGetType(this->srcCodes)==declareVar )
   {
      index = codeGetName(codeNext(this->srcCodes));
      /* initialization is really For statment. */
      if( !secompileVar(this) ) return False;
   }
   else
   {
      /* possible to have no initialization */
      if( codeGetType(this->srcCodes)!=StatementEnd )
      {
         /* initialization is just some expression */
         if( !secompileExpression(this) ) return False;
         has_init = True;
      }
      if( codeGetType(this->srcCodes)!=StatementEnd &&
          codeGetType(this->srcCodes)!=statementIn )
      {
         callQuit(this->call,textcoreBAD_FOR_STATEMENT);
         return False;
      }
      if( codeGetType(this->srcCodes)==StatementEnd )
         secompileAdvancePtr(this);
   }


   /* Do the for..in part */

   if( codeGetType(this->srcCodes)==statementIn )
   {
      uint ptr,ptr2,ptr3;

      if( !has_init && index==NULL )
      {
         callQuit(this->call,textcoreBAD_FOR_IN_STATEMENT);
         return False;
      }
      if( index!=NULL )
      {
         secompileAddItem(this,pushVariable,index);
      }
      secompileAdvancePtr(this);
      if( !secompileExpression(this) ) return False;

      if( codeGetType(this->srcCodes)!=EndEvaluationGroup )
      {
         callQuit(this->call,textcoreBAD_FOR_IN_STATEMENT);
         return False;
      }
      secompileAdvancePtr(this);

      /* at this point, the top item of the stack is the object,
       * the one right behind it is the iterator
       */

      secompileAddItem(this,pushIterator);
      ptr = secompileCurrentItem(this);
      secompileAddItem(this,gotoAlways,0);

      ptr2 = secompileCurrentItem(this);
      if( !secompileStatement(this) ) return False;

      secompileFixupGotoItem(this,ptr,secompileCurrentItem(this));
      ptr3 = secompileCurrentItem(this);
      secompileAddItem(this,gotoForIn,ptr2);
      secompileAddItem(this,popIterator);

      secompileEndLoop(this,secompileCurrentItem(this),ptr3);
   }
   else
   {
      uint out_of_loop;
      jsebool patch_out_of_loop = False;
      uint test;
      uint go_into_loop,increment;


      /* do the regular for loop part */


      if( has_init ) secompileAddItem(this,popDiscard);

      test = secompileCurrentItem(this);

      if( codeGetType(this->srcCodes)!=StatementEnd )
      {
         /* this is the test */
         if( !secompileExpression(this) ) return False;
         patch_out_of_loop = True;
         out_of_loop = secompileCurrentItem(this);
         secompileAddItem(this,gotoFalse,0);
         if( codeGetType(this->srcCodes)!=StatementEnd )
         {
            callQuit(this->call,textcoreBAD_FOR_STATEMENT);
            return False;
         }
         secompileAdvancePtr(this);
      }
      else
      {
         /* no test */
         secompileAdvancePtr(this);
      }

      go_into_loop = secompileCurrentItem(this);
      secompileAddItem(this,gotoAlways,0);

      increment = secompileCurrentItem(this);
      if( codeGetType(this->srcCodes)!=EndEvaluationGroup )
      {
         /* the increment */
         if( !secompileExpression(this) ) return False;
         /* the increment is only used for side effects */
         secompileAddItem(this,popDiscard);
      }
      secompileAddItem(this,gotoAlways,test);

      if( codeGetType(this->srcCodes)!=EndEvaluationGroup )
      {
         callQuit(this->call,textcoreBAD_FOR_STATEMENT);
         return False;
      }
      secompileAdvancePtr(this);

      secompileFixupGotoItem(this,go_into_loop,secompileCurrentItem(this));
      if( !secompileStatement(this) ) return False;
      /* we go back up and do the test and increment */
      secompileAddItem(this,gotoAlways,increment);

      if( patch_out_of_loop )
         secompileFixupGotoItem(this,out_of_loop,secompileCurrentItem(this));

      secompileEndLoop(this,secompileCurrentItem(this),increment);
   }
   
   return True;
}


/*
 * Whenever any case matches, we clean up the test from the stack.
 * Bigger code, but its another thing I don't have to keep track
 * of in the interpret() loop. It would be easy if the switch always
 * fully executed, in which case we could just do the pop once,
 * but there can be a return in it.
 */
jsebool NEAR_CALL secompileSwitch(struct secompile *this)
{
   sint def_ptr;
   sint goto_ptr2;
   sint goto_ptr;

   assert( codeGetType(this->srcCodes)==statementSwitch );
   secompileAdvancePtr(this);

   if( codeGetType(this->srcCodes)!=EvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_SWITCH);
      return False;
   }
   secompileAdvancePtr(this);

   if( !secompileExpression(this) ) return False;

   if( codeGetType(this->srcCodes)!=EndEvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_WHILE);
      return False;
   }
   secompileAdvancePtr(this);
   secompileAddItem(this,getValue);

   if( codeGetType(this->srcCodes)!=BeginBlock )
   {
      callQuit(this->call,textcoreSWITCH_NEEDS_BRACE);
      return False;
   }
   secompileAdvancePtr(this);

   def_ptr = -1;
   /* goto ptr is the last test for a case if false, so it must always chain
    * to the next test
    */

   secompileNewLoop(this);

   /* set up to jump to the first comparison */
   goto_ptr = (sint) secompileCurrentItem(this);
   secompileAddItem(this,gotoAlways,0);

   while( codeGetType(this->srcCodes)!=EndBlock )
   {
      if( codeGetType(this->srcCodes)==SwitchDefault )
      {
         secompileAdvancePtr(this);
         if( codeGetType(this->srcCodes)!=CaseColon )
         {
            callQuit(this->call,textcoreCONDITIONAL_MISSING_COLON);
            return False;
         }
         secompileAdvancePtr(this);
         if( def_ptr!=-1 )
         {
            callQuit(this->call,textcoreDUPLICATE_DEFAULT);
            return False;
         }
         def_ptr = (sint) secompileCurrentItem(this);
         continue;
      }

      /* each case is an if test. We jump over this test first, so any
       * fallthru will act correctly. If the test succeeds we pop the value
       * from the stack so any executing code doesn't have to try to
       * remember that it is there
       */
      if( codeGetType(this->srcCodes)==SwitchCase )
      {
         secompileAdvancePtr(this);
         if( goto_ptr!=-1 )
         {
            /* if we are already executing the code for a successful comparison
             * jump over this test into the code for the next case (the
             * fallthru.) Otherwise this is the first test, so make it.
             */
            goto_ptr2 = (sint) secompileCurrentItem(this);
            secompileAddItem(this,gotoAlways,0);
            secompileFixupGotoItem(this,(uint)goto_ptr,secompileCurrentItem(this));
         }
         else
         {
            goto_ptr2 = -1;
         }
         secompileAddItem(this,pushClone);
         if( !secompileExpression(this) ) return False;
         if( codeGetType(this->srcCodes)!=CaseColon )
         {
            callQuit(this->call,textcoreCONDITIONAL_MISSING_COLON);
            return False;
         }
         secompileAdvancePtr(this);
         secompileAddItem(this,opEqual);
         goto_ptr = (sint) secompileCurrentItem(this);
         secompileAddItem(this,gotoFalse,0);
         /* this is it, get rid of the item being tested on the stack */
         secompileAddItem(this,popDiscard);
         if( goto_ptr2!=-1 )
            secompileFixupGotoItem(this,(uint)goto_ptr2,secompileCurrentItem(this));
         continue;
      }

      /* else this is a statement in the switch */
      if( goto_ptr==-1 )
      {
         callQuit(this->call,textcoreSWITCH_NEEDS_CASE);
         return False;
      }

      if( !secompileStatement(this) ) return False;
   }

   assert( codeGetType(this->srcCodes)==EndBlock );
   secompileAdvancePtr(this);

   if( goto_ptr!=-1 )
   {
      goto_ptr2 = (sint)secompileCurrentItem(this);
      secompileAddItem(this,gotoAlways,0);
      secompileFixupGotoItem(this,(uint)goto_ptr,secompileCurrentItem(this));
   }

   /* we are at the end of the switch, get rid of the still remaining
    * comparison item, then goto the default if any.
    */
   secompileAddItem(this,popDiscard);
   if( def_ptr!=-1 ) secompileAddItem(this,gotoAlways,def_ptr);

   secompileFixupGotoItem(this,(uint)goto_ptr2,secompileCurrentItem(this));

   /* break and continue on switch mean the same thing - just get out */
   secompileEndLoop(this,secompileCurrentItem(this),
                    secompileCurrentItem(this));

   return True;
}


jsebool NEAR_CALL secompileWith(struct secompile *this)
{
   assert( codeGetType(this->srcCodes)==withVar );
   secompileAdvancePtr(this);

   if( codeGetType(this->srcCodes)!=EvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_WITH_STATEMENT);
      return False;
   }

   secompileAdvancePtr(this);
   if( !secompileExpression(this) ) return False;

   if( codeGetType(this->srcCodes)!=EndEvaluationGroup )
   {
      callQuit(this->call,textcoreBAD_WITH_STATEMENT);
      return False;
   }
   secompileAdvancePtr(this);
   secompileAddItem(this,withVar);
   this->with_depth++;

   if( !secompileStatement(this) ) return False;

   secompileAddItem(this,withoutVar);
   this->with_depth--;
   return True;
}


jsebool NEAR_CALL secompileStatement(struct secompile *this)
{
   jsebool success = True;

   /* we check to see if we should continue at the beginning of each
    * statement. For those people working on the debugger, if you think
    * the debugger should pause at any particular time, put a mayIContinue
    * at that time. The only place I can think of might be parts of
    * a for() loop, but I don't think it should. We don't add it at the
    * beginning of a block, since the first statement in the block is where
    * it should stop - the begin block is just a grouping thing.
    */
   if( codeGetType(this->srcCodes)!=BeginBlock )
   {
      /* The one token lookahead causes the linenumber to appear too early.
       * this fixes the problem
       */
      secompileAddItem(this,sourceLineNumber,this->call->linenum);
      secompileAddItem(this,mayIContinue);
   }
   
   switch( codeGetType(this->srcCodes) )
   {
      case returnVar:
      {
         secompileAdvancePtr(this);
         if( codeGetType(this->srcCodes)!=StatementEnd )
         {
            if( !secompileExpression(this) )
            {
               success = False;
               break;
            }
            secompileAddItem(this,returnVar);
         }
         /* eat statement terminator if present */
         if( codeGetType(this->srcCodes)==StatementEnd )
            secompileAdvancePtr(this);
         secompileAddItem(this,returnVoid);
         break;
      }

      case BeginBlock:
         /* to parse this, we simply have any number of statements followed
          * by an EndBlock
          */
         secompileAdvancePtr(this);
         while( codeGetType(this->srcCodes)!=EndBlock )
         {
            if( !secompileStatement(this) )
            {
               success = False;
               break;
            }
         }
         if( success )
         {
            assert( codeGetType(this->srcCodes)==EndBlock );
            /* eat the '}' */
            secompileAdvancePtr(this);
         }
         break;

      /* several statement types just have us calling further routines to ease
       * readability and modularize
       */
      case statementIf:
         success = secompileIf(this);
         break;
      case statementWhile:
         success = secompileWhile(this);
         break;
      case statementDo:
         success = secompileDo(this);
         break;
      case withVar:
         success = secompileWith(this);
         break;
      case declareVar:
         success = secompileVar(this);
         break;
      case statementFor:
         success = secompileFor(this);
         break;
      case statementSwitch:
         success = secompileSwitch(this);
         break;

      case statementBreak: case statementContinue:
         if( codeGetType(this->srcCodes)==statementBreak )
            success = secompileAddBreak(this);
         else
            success = secompileAddContinue(this);
         if( success )
         {
            secompileAdvancePtr(this);
            /* eat statement terminator if present */
            if( codeGetType(this->srcCodes)==StatementEnd )
               secompileAdvancePtr(this);
         }
         break;

      case gotoAlways:
         assert( codeGetName(this->srcCodes)!=NULL );
         if( !secompileAddGoto(this,codeGetName(this->srcCodes)) )
         {
            success = False;
            break;
         }
         secompileAdvancePtr(this);
         /* eat statement terminator if present */
         if( codeGetType(this->srcCodes)==StatementEnd )
            secompileAdvancePtr(this);
         break;

      case Label:
         assert( codeGetName(this->srcCodes)!=NULL );
         success = secompileAddLabel(this,codeGetName(this->srcCodes));
         if( success )
         {
            secompileAdvancePtr(this);
         }
         break;

      case StatementEnd:
         secompileAdvancePtr(this);
         break;

      default:
         success = secompileExpression(this);
         /* We don't use the value put on the stack for anything except as
          * a possible return
          */
         /* eat statement terminator if present */
         if( codeGetType(this->srcCodes)==StatementEnd )
            secompileAdvancePtr(this);
         /* We return the last expression in a program implicitly */
         secompileAddItem( this,
            ( EndBlock == codeGetType(this->srcCodes)
              && LOCAL_TEST_IF_INIT_FUNCTION(this->locfunc,this->call) )
            ? returnVar : popDiscard );
         break;
   }

   return success;
}

#endif
