/* DirParts.c   Find parts of a directory specification
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
#include <string.h>
#if !defined(__JSE_UNIX__) && !defined(__JSE_MAC__) \
 && !defined(__IBMCPP__) && !defined(__JSE_WINCE__)
   #include <io.h>
#endif
#include "jseopt.h"
#include "dirparts.h"


   void 
FileNameParts( const jsechar * const FileName,
               jsechar **dir, uint *dirlen,
               jsechar **name, uint *namelen,
               jsechar **ext, uint *extlen )
{
   /* Figure out char_to_find here */
   jsechar dir_tail;
   jsechar *c;

   assert( NULL != FileName );

#if defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) \
 || defined(__JSE_DOS16__) || defined(__JSE_DOS32__) \
 || defined(__JSE_WIN16__) \
 || defined(__JSE_WIN32__) || defined(__JSE_CON32__)
   dir_tail = '\\';
#elif defined(__JSE_NWNLM__)
   /* Netware accepts DOS specifications, but it also has it's own stuff.
      If it contains a '/', it is a netware filespec, something like:
      OUTLAND/SYS:/NOWHERE/WHATEVER.JOE
      Otherwise, everything up to a final : or a \ is part of the dir. */
   if( strchr( FileName, '/' ) != NULL )
      dir_tail = '/';
   else
      dir_tail = '\\';
#elif defined(__JSE_UNIX__)
   dir_tail = '/';
#elif defined(__JSE_MAC__)
   /* This duplicates the comparison with ':' below, but it keeps the code simpler. */
   dir_tail = ':';
#else
   #error No trailing character for directory name for this platform defined.
#endif

   *dir = (jsechar *) FileName;

   /* Compute position of start of name
      Everything up to and including a final dir_tail or ':' is part of the dir.
      By default, *name points at first char in FileName. */
   *name = *dir;
   for ( c = *name; 0 != *c; c++ ) {
      if ( dir_tail == *c  ||  ':' == *c ) {
         *name = c + 1;
      }
   }

   /* Compute position of start of extension
      Anything after and including the last '.' in name is the extension.
      By default, *ext points at the terminating null. */
   *ext = *name + strlen_jsechar( *name );
   for ( c = *name; 0 != *c; c++ ) {
      if ( '.' == *c ) {
         *ext = c;
      }
   }

   /* Calculate lengths of parts */
   assert( *name >= *dir );
   *dirlen = (uint) (*name - *dir);
   
   assert( *ext >= *name );
   *namelen = (uint) (*ext - *name);

   *extlen = strlen_jsechar( *ext );
   
   return;
}

