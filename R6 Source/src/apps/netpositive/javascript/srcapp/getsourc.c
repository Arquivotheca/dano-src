/* getsource.c    Standard GetSource function
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

/* Every Toolkit Application that needs to do File I/O requires a
   GetSourceFunc function. The GetSourceFunc is a single callback
   that will manage a file stream.  It needs to open, read from and
   close files.  This callback should return True if it was successful
   else it should return False.  Creative devlopers will quickly realize
   that there is potential here for reading source code from more than
   just files.
*/
jsebool JSE_CFUNC FAR_CALL
ToolkitAppGetSource(jseContext jsecontext,
                    struct jseToolkitAppSource * ToolkitAppSource,
                    jseToolkitAppSourceFlags flag)
{
   jsechar buf[255];

   UNUSED_PARAMETER(jsecontext);

   switch(flag)
   {
      /* You are called with flag == jseNewOpen when the file needs to
         be initially opend.  You can be assured that you will not be
         called multiple times for the same source file name
       */
      case jseNewOpen:
         ToolkitAppSource->userdata = (void*)fopen_jsechar(ToolkitAppSource->name, "rt");
         if (NULL == ToolkitAppSource->userdata)
         {
            return False;
         }
         ToolkitAppSource->code = jseMustMalloc(jsechar,sizeof(buf));
         break;

      /* Each time that the interpreter need the next line of code, you
         are called with flag == jseGetNext.  If there is a read error
         or the file is finished, return False, otherwise return True.
       */
      case jseGetNext :
         if(fgets_jsechar( buf, sizeof(buf)/sizeof(buf[0])-1, (FILE*)ToolkitAppSource->userdata ))
         {
            strcpy(ToolkitAppSource->code,buf);
         }
         else
         {
            strcpy(ToolkitAppSource->code,"");
            return False;
         }
         break;

      /* For each call with jseNewOpen, there will be a call with jseClose.
       */
      case jseClose:
         fclose((FILE*)ToolkitAppSource->userdata);
         jseMustFree(ToolkitAppSource->code);
         break;

   }
   return True;
}
