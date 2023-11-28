/* brktest.c   Check if code can break at a given line.
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

#if defined(JSE_BREAKPOINT_TEST) && (0!=JSE_BREAKPOINT_TEST)

jsebool BreakpointTest(struct Call *call,const jsechar *WantedName,
                       uword32 LineNumber)
{
   /* A line is a break line if it has a mayIContinue on it. */

   VarRead *f = NULL;
   VarName varName;

   while( (f = varGetNext(call->session.GlobalVariable,call,f,&varName))!=NULL )
   {
      jsebool right_file, right_line;
      struct Function *func;
      code_elem *EndOpcodes, *c;

      if( VAR_TYPE(f)!=VObject ) continue;
      func = varGetFunction(f,call);
      if( func==NULL || !functionTestIfLocal(func) ) continue;

      right_file = False;
      right_line = False;
      EndOpcodes = ((struct LocalFunction *)func)->opcodes
                 + ((struct LocalFunction *)func)->opcodesUsed;
      for( c = ((struct LocalFunction *)func)->opcodes; c != EndOpcodes; c++ )
      {
         if( sourceFilename == *c )
         {
            right_file =
               ( 0 == stricmp_jsechar(
                         GetStringTableEntry(call,CODELIST_DATUM(c,VarName),NULL),
                         WantedName) );
         }
         else if( sourceLineNumber == *c )
         {
            right_line = ( CODELIST_DATUM(c,uint) == LineNumber );
         }
         else if( mayIContinue==(*c) && right_line && right_file )
            return True;
         if ( IS_CODELIST_DATUM(*c) )
            INCREMENT_DATUM_CODEPTR(c);
      }
   }

   return False;
}

#endif
