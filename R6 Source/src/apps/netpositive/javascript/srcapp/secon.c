/* secon.c  Default implemntation of Console I/O
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

static ulong _FAR_ 
Console_write(jseContext jsecontext, const jsechar * data, ulong length)
{
   UNUSED_PARAMETER(jsecontext);
   return fwrite(data,1,length,stdout);
}

static ulong _FAR_ 
Console_read(jseContext jsecontext, jsechar * data, ulong length)
{
   UNUSED_PARAMETER(jsecontext);
   return fread(data,1,length,stdin);
}

static jsechar *
Console_readLine(jseContext jsecontext, jsechar * data, ulong length)
{
   UNUSED_PARAMETER(jsecontext);
   return fgets(data,(int)length,stdin);
}

static ulong _FAR_ 
Console_keyboardInput(jseContext jsecontext)
{
   UNUSED_PARAMETER(jsecontext);
   return 0;
}

static int _FAR_ 
Console_ungetchar(jseContext jsecontext, int c)
{
   UNUSED_PARAMETER(jsecontext);
   return ungetc(c,stdin);
}

static slong _FAR_
Console_readCharUntilInput(jseContext jsecontext)
{
   UNUSED_PARAMETER(jsecontext);
   return (slong) getch(); 
}

   static void 
RemoveStandardService_ConsoleIO(jseContext jsecontext,struct ConsoleIO * This)
{
   assert( This != NULL );
   UNUSED_PARAMETER(jsecontext);
   jseMustFree(This);
}

   void 
AddStandardService_ConsoleIO(jseContext jsecontext)
{
   struct ConsoleIO * This = jseMustMalloc(struct ConsoleIO, sizeof(struct ConsoleIO));
   
   This->Read = Console_read;
   This->Write = Console_write;
   This->Gets = Console_readLine;
   This->Getch = Console_readCharUntilInput;
   This->Kbhit = Console_keyboardInput;
   This->Ungetc = Console_ungetchar;
   
   assert( CONSOLEIO_CONTEXT == NULL );  /* We don't want to add it if it exists */
   
   jseSetSharedData(jsecontext,CONSOLEIO_NAME,(void _FAR_ *)This,(void *)RemoveStandardService_ConsoleIO);
}
