/* seinterp.c   Default intperet application service
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

static jsebool
Interpret(jsebool newWindow,jseContext jsecontext, const jsechar * SourceFile,
          const jsechar * SourceText, const void * preTokenizedSource,
          jseNewContextSettings NewContextSettings, int howToInterpret,
          jseContext localVariableContext, jseVariable * returnVar)
{
   struct jseExternalLinkParameters *linkparms = jseGetExternalLinkParameters(jsecontext);
   jseLinkOptions SaveOptions = linkparms->options;
   jsebool ret;
#  if defined(__CENVI__)  &&  (defined(__JSE_WIN16__) || defined(__JSE_WIN32__))
      if( newWindow && SourceFile )
         jsemainwindowNewSubTitle(ToolkitAppDataFromContext(jsecontext)->MainWindow,threadinfo->FileSpec);
#  endif
      /* Insert code around this - i.e. setting window titles / etc. */

      /* NYI: this line makes no sense: preserve_flags */
   ret = jseInterpret(jsecontext,SourceFile,SourceText,preTokenizedSource,NewContextSettings,
                   howToInterpret,localVariableContext,returnVar);
                   
#  if defined(__CENVI__)  &&  (defined(__JSE_WIN16__) || defined(__JSE_WIN32__))
      if( newWindow && SourceFile )
         jsemainwindowOldSubTitleText(ToolkitAppDataFromContext(jsecontext)->MainWindow);
#  endif
   linkparms->options = SaveOptions;

#  if defined(__CENVI__)
   {
      /* if they want to kill this process, then reset the break flag (interpret may have cleared it) */
      struct QSignal *qsignal = &QSIGNAL(jsecontext);
      if ( qsignalDeath(qsignal))
      {
         qsignalSetBreak(qsignal);
      }
   }
#  endif

   return ret;
}

void 
AddStandardService_Interpret(jseContext jsecontext)
{
   assert( INTERPRET_CONTEXT == NULL );
   
   /* NYI: won't compile: jseSetSharedData(jsecontext,INTERPRET_NAME,(void *)Interpret); */
}
