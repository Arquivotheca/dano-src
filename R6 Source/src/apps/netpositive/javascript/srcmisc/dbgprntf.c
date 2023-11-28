/* dbgprntf.c
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

#include <stdlib.h>
#if !defined(__JSE_WINCE__)
   #include <stdio.h>  
#endif
#include <stdarg.h>
#if defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)
   #include <windows.h>
#endif
#include "jseopt.h"
#include "dbgprntf.h"

#if !defined(NDEBUG)

   void
DebugPrintf(jsechar *Fmt,...)
{
   FILE *fp;
   va_list ap;
   va_start(ap,Fmt);
  #ifdef __JSE_MAC__
   while ( NULL == (fp = fopen(UNISTR("jseDebug.log"),UNISTR("at"))) )
      /* prevent two threads writing at same time */
  #elif defined(__JSE_NWNLM__)
   while ( NULL == (fp = fopen(UNISTR("SYS:/jseDebug.log"),UNISTR("at"))) )
      /* prevent two threads writing at same time */
  #else
   while ( NULL == (fp = fopen_jsechar(UNISTR("C:\\jseDebug.log"),UNISTR("at"))) )
      /* prevent two threads writing at same time */
  #endif
      ;
   vfprintf_jsechar(fp,Fmt,ap);
   fprintf_jsechar(fp,UNISTR("\n"));
   fclose(fp);
   va_end(ap);
}


#if defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)
   void
WinDebug(jsechar *format,...)
{
   va_list ap;
   jsechar buffer[1000];
   va_start(ap,format);
   vsprintf_jsechar(buffer,format,ap);
   if ( IDOK != MessageBox((HWND)0,buffer,NULL,
                           MB_TASKMODAL|MB_ICONHAND|MB_OKCANCEL) )
      exit(EXIT_FAILURE);
   va_end(ap);
}
#endif

#else
   void
DebugPrintf(jsechar *Fmt,...)
{
#  if !defined(__BORLANDC__)
     Fmt = Fmt;  /* ignored parameter */
#  endif
  /* nothing */
}
#endif
