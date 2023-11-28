/* expressn.c
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
/* secode expression compiler routines                                    */
/* ---------------------------------------------------------------------- */

#define secompileLeftHandSideExpression secompileMemberExpression

/*
 * Compile the array declarator syntax a = { foo, bar };
 */
jsebool NEAR_CALL secompileArray(struct secompile *this)
{
   uint elem = 0;

   assert( codeGetType(this->srcCodes)==BeginBlock );
   secompileAdvancePtr(this);

   /* create blank object */
   secompileAddItem(this,pushObject);
   /* for each element */
   while( codeGetType(this->srcCodes)!=EndBlock )
   {
      /* so we generate an assignment to the appropriate element number */
      secompileAddItem(this,pushClone);
      secompileAddItem(this,pushNumber,elem);
      secompileAddItem(this,structureMember);
      if( !secompileOperatorExpression(this,PRI_ASSIGN) ) return False;
      secompileAddItem(this,opAssign);
      secompileAddItem(this,popDiscard);
      /* that cleans up the stack leaving the original object still on it */

      if( codeGetType(this->srcCodes)==EndBlock ) break;
      if( codeGetType(this->srcCodes)!=popEval )
      {
         callQuit(this->call,textcoreEXPECT_COMMA_BETWEEN_ARRAY_INITS);
         return False;
      }
      secompileAdvancePtr(this);
      elem++;
      assert( elem < MAX_UINT );
   }

   assert( codeGetType(this->srcCodes)==EndBlock );
   secompileAdvancePtr(this);
   return True;
}


/* Returns number of arguments, -1 for error */
sint NEAR_CALL secompileFunctionArgs(struct secompile *this)
{
   uint count = 0;

   if( codeGetType(this->srcCodes)!=EndFunctionCall )
   {
      do {
         /* This code used to allow foo(=a) pass by reference, but no
          * longer
          */
         /*jsebool byref = False;*/

         /* eat the comma if there */
         if( count++ ) secompileAdvancePtr(this);

         /*
         if( codeGetType(this->srcCodes)==opAssign )
         {
            secompileAdvancePtr(this);
            byref = True;
         }
         */

         /* use this one so commas don't get eaten */
         if( !secompileOperatorExpression(this,PRI_ASSIGN) ) return -1;

         /*if( byref )
            secompileAddItem(this,getReferenceValue);
         else*/
            secompileAddItem(this,getParamValue,count);

      } while( codeGetType(this->srcCodes)==getParamValue );
   }

   if( codeGetType(this->srcCodes)!=EndFunctionCall )
   {
      callQuit(this->call,textcoreMISSING_CLOSE_PAREN);
      return -1;
   }
   secompileAdvancePtr(this);

   return (int) count;
}


jsebool NEAR_CALL secompilePrimaryExpression(struct secompile *this)
{
   jsebool success = True;

   switch( codeGetType(this->srcCodes) )
   {
      case EvaluationGroup:
         secompileAdvancePtr(this);
         if( !secompileExpression(this) ) return False;
         if( codeGetType(this->srcCodes)!=EndEvaluationGroup )
         {
            callQuit(this->call,textcoreMISSING_CLOSE_PAREN);
            success = False;
            break;
         }
         secompileAdvancePtr(this);
         break;
      case pushValue:
      {
         Var *val = codeGetVar(this->srcCodes);
         secompileAddItem(this,pushValue,GET_READABLE_VAR(val,this->call));
         secompileAdvancePtr(this);
         break;
      }
      case declareVar:
         secompileAdvancePtr(this);
         if( codeGetType(this->srcCodes)!=pushVariable )
         {
            callQuit(this->call,textcoreVAR_NEEDS_VARNAME);
            return False;
         }
         secompileAddItem(this,declareVar,codeGetName(this->srcCodes));
         /* fallthru intentional - 'var x' is the same as 'x' with
          * a little extra which is the item just added
          */

      case pushVariable:
         secompileAddItem(this,pushVariable,codeGetName(this->srcCodes));
         secompileAdvancePtr(this);
         break;
      case BeginBlock:
         success = secompileArray(this);
         break;
      default:
         callQuit(this->call,textcoreBAD_PRIMARY);
         success = False;
         break;
   }
   return success;
}


jsebool NEAR_CALL secompileMemberExpression(struct secompile *this)
{
   codeval t = codeGetType(this->srcCodes);

   if( t==functionNew )
   {
      sint numargs = 0;

      secompileAdvancePtr(this);
      if( !secompilePrimaryExpression(this) ) return False;
      secompileAddItem(this,checkFunctionNew);
      if( codeGetType(this->srcCodes)==functionCall )
      {
         secompileAdvancePtr(this);
         numargs = secompileFunctionArgs(this);
         if( numargs<0 ) return False;
      }
      secompileAddItem(this,functionNew,numargs);
   }
   else
   {
      if( !secompilePrimaryExpression(this) ) return False;
      while( (t=codeGetType(this->srcCodes))==structureMember
          || t==structureMemberName || t==functionCall )
      {
         if( t==structureMember )
         {
            secompileAdvancePtr(this);
            if( !secompileExpression(this) ) return False;
            if( codeGetType(this->srcCodes)!=EndArrayIndex )
            {
               callQuit(this->call,textcoreMISSING_CLOSE_BRACKET);
               return False;
            }
            secompileAddItem(this,structureMember);
            secompileAdvancePtr(this);
         }
         else if( t==structureMemberName )
         {
            VarName vn = codeGetName(this->srcCodes);
            if ( NULL == vn )
            {
               callQuit(this->call,textcoreMISSING_PROPERTY_NAME);
               return False;
            }
            secompileAddItem(this,structureMemberName,vn);
            secompileAdvancePtr(this);
         }
         else
         {
            sint numargs;

            assert( t==functionCall );
            secompileAddItem(this,checkFunctionCall);
            secompileAdvancePtr(this);
            numargs = secompileFunctionArgs(this);
            if( numargs<0 ) return False;
            secompileAddItem(this,functionCall,numargs);
         }
      }
   }
   return True;
}


jsebool NEAR_CALL secompilePostfixExpression(struct secompile *this)
{
   codeval t;

   if( !secompileLeftHandSideExpression(this) ) return False;

   t = codeGetType(this->srcCodes);

   if( opPreIncrement==t  ||  opPreDecrement==t )
   {
      secompileAddItem(this,
         opPreIncrement==t ? opPostIncrement : opPostDecrement );
      secompileAdvancePtr(this);
   }
   return True;
}


jsebool NEAR_CALL secompileOperatorExpression(struct secompile *this,
                                              uint Priority)
{
   assert( PRI_ASSIGN <= Priority  &&  Priority <= PRI_UNARY );

   /* give higher-order operator first chance to compile */
   if ( PRI_UNARY != Priority &&
        !secompileOperatorExpression(this,Priority+1) )
      return False;

   for ( ; ; )
   {
      uint ptr, ptr2;

      /* if the present operator is not of this level then don't compile it */
      codeval t = codeGetType(this->srcCodes);
      if ( Priority != GET_OP_PRIORITY(t) )
      {
         if ( PRI_UNARY == Priority )
            return secompilePostfixExpression(this);
         return True;
      }

      switch ( Priority )
      {
         case PRI_CONDITIONAL:
            secompileAdvancePtr(this);
            ptr = secompileCurrentItem(this);
            secompileAddItem(this,gotoFalse,0);
            if( !secompileOperatorExpression(this,PRI_ASSIGN) ) return False;
            if( codeGetType(this->srcCodes)!=ConditionalFalse )
            {
               callQuit(this->call,textcoreCONDITIONAL_MISSING_COLON);
               return False;
            }
            secompileAdvancePtr(this);
            ptr2 = secompileCurrentItem(this);
            secompileAddItem(this,gotoAlways,0);
            secompileFixupGotoItem(this,ptr,secompileCurrentItem(this));
            if( !secompileOperatorExpression(this,PRI_ASSIGN) )
               return False;
            secompileFixupGotoItem(this,ptr2,secompileCurrentItem(this));
            return True;
         case PRI_LOGICAL_OR:
         case PRI_LOGICAL_AND:
            secompileAdvancePtr(this);
            secompileAddItem(this,pushClone);
            ptr = secompileCurrentItem(this);
            secompileAddItem(this, LogicalOR == t ? gotoTrue : gotoFalse,0);
            secompileAddItem(this,popDiscard);
            if( !secompileOperatorExpression(this,Priority+1) )
               return False;
            secompileFixupGotoItem(this,ptr,secompileCurrentItem(this));
            break;
         default:
            secompileAdvancePtr(this);
            if ( PRI_ASSIGN == Priority  ||  PRI_UNARY == Priority )
            {
               if( !secompileOperatorExpression(this,Priority) )
                  return False;
               secompileAddItem(this,t);
               return True;
            }
            else
            {
               if( !secompileOperatorExpression(this,Priority+1) )
                  return False;
            }
            secompileAddItem(this,t);
            break;
      }
   }
}

jsebool NEAR_CALL secompileExpression(struct secompile *this)
{
   if( !secompileOperatorExpression(this,PRI_ASSIGN) ) return False;

   while( codeGetType(this->srcCodes)==popEval )
   {
      secompileAddItem(this,popEval);
      secompileAdvancePtr(this);
      if( !secompileOperatorExpression(this,PRI_ASSIGN) ) return False;
   }
   return True;
}

#endif /* #if (0!=JSE_COMPILER) */
