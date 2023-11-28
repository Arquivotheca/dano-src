/* CmdLine.c    Parse command line into source and parameters 
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

#ifndef JSETOOLKIT_CORE

void
ParseIntoSourcefileAndSourcetext(jseContext jsecontext,jseFileFindFunc FileFindFunc,
                              const jsechar *CmdLine,jsechar **Sourcefile,jsechar **Sourcetext)
{
   jsechar *TempCmdLine;
   const jsechar * Quotables;
   jsechar *QuoteChar, *EndQuote;

   assert( NULL != CmdLine );
   assert( NULL != Sourcefile  &&  NULL != Sourcetext );
   assert( NULL != jsecontext );
   assert( NULL != FileFindFunc );
   TempCmdLine = StrCpyMalloc(CmdLine);
   RemoveWhitespaceFromHeadAndTail(TempCmdLine);
   *Sourcefile = NULL; /* assume source file not found */

   /* if the whole string is within the same quote, then it is all text */
   Quotables = UNISTR("\"\'");
   if ( '\0' == *TempCmdLine )
   {
      /* no text, so sure cannot be file name */
      *Sourcetext = StrCpyMalloc(TempCmdLine);
   }
   else if ( NULL != (QuoteChar = strchr_jsechar(Quotables,TempCmdLine[0]))
   && NULL != (EndQuote = strchr_jsechar(TempCmdLine+1,*QuoteChar))
   && ( '\0' == EndQuote[1] ))
   {
      *EndQuote = '\0';
      *Sourcetext = StrCpyMalloc(TempCmdLine+1);
   }
   else
   {

      /* look for potential files found.  If file found then it is the source file.  In systems
       * that can have whitespace in file names this may take a lot of checks. 
       */
      jsechar *BeyondName = TempCmdLine;
      do
      {
         jsechar SaveBeyondName;
         jsechar FindBuffer[_MAX_PATH+50];

         BeyondName = BeyondName + strcspn_jsechar(BeyondName,WhiteSpace);
         SaveBeyondName = *BeyondName;
         *BeyondName = '\0';

         if ((*FileFindFunc)(jsecontext,TempCmdLine,FindBuffer,(sizeof(FindBuffer)-2)/sizeof(jsechar),False))
         {
            *Sourcefile = StrCpyMalloc((const jsechar *)FindBuffer);
         } /* endif */

         if ( '\0' == (*BeyondName = SaveBeyondName))
            break;
         BeyondName++;
      } while ( NULL == *Sourcefile );

      if ( NULL != *Sourcefile )
      {
         /* file was found.  Yea! */
         *Sourcetext = StrCpyMalloc(BeyondName);
      }
      else
      {
         /* file was not found.  Assume file anyway, ending at first whitespace. */
         size_t FileNameLen = strcspn_jsechar(TempCmdLine,WhiteSpace);
         *Sourcefile = StrCpyMallocLen(TempCmdLine,FileNameLen);
         if ( '\0' != *(BeyondName = TempCmdLine + FileNameLen))
            BeyondName++;
         *Sourcetext = StrCpyMalloc(BeyondName);
      } /* endif */

   } /* endif */

   /* search for first space following and see if */
   jseMustFree(TempCmdLine);
}

#endif /* !JSETOOLKIT_CORE */
