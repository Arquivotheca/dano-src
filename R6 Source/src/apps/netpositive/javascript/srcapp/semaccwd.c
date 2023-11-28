/* semaccwd.c    Application Service to handle Mac working dir
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

#ifdef __JSE_MAC__

   static void
RemoveStandardService_MacCWD(jseContext jsecontext,void * cwd)
{
   assert( cwd != NULL );
   UNUSED_PARAMETER(jsecontext);
   jseMustFree( cwd );
}

   void
AddStandardService_MacCWD(jseContext jsecontext)
{
   jsechar * cwd = jseMustMalloc(jsechar,sizeof(jsechar)*MACCWD_SIZE);
   jsechar * execpath = GetExecutablePath();
   jsechar * dir, *name, *ext;
   uint dirlen, namelen, extlen;
   
   assert( MACCWD_CONTEXT == NULL );
   

   FileNameParts( GetExecutablePath(), &dir, &dirlen, &name, &namelen, &ext, &extlen );
   strncpy( cwd, dir, dirlen );
   cwd[dirlen] = '\0';
   
   jseSetSharedData(jsecontext,MACCWD_NAME,(void _FAR_ *)cwd,RemoveStandardService_MacCWD);
}
#endif
