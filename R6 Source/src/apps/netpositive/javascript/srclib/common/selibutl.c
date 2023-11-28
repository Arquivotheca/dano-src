/* selibutl.c  - Misc. uitilities for various libraries
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

#if defined(JSE_CLIB_MEMSET)    || \
    defined(JSE_CLIB_MEMCHR)    || \
    defined(JSE_CLIB_STRCHR)    || \
    defined(JSE_CLIB_STRCSPN)   || \
    defined(JSE_CLIB_STRRCHR)   || \
    defined(JSE_CLIB_ISALNUM)   || \
    defined(JSE_CLIB_ISALPHA)   || \
    defined(JSE_CLIB_ISASCII)   || \
    defined(JSE_CLIB_ISCNTRL)   || \
    defined(JSE_CLIB_ISDIGIT)   || \
    defined(JSE_CLIB_ISGRAPH)   || \
    defined(JSE_CLIB_ISLOWER)   || \
    defined(JSE_CLIB_ISPRINT)   || \
    defined(JSE_CLIB_ISPUNCT)   || \
    defined(JSE_CLIB_ISSPACE)   || \
    defined(JSE_CLIB_ISUPPER)   || \
    defined(JSE_CLIB_ISXDIGIT)  || \
    defined(JSE_CLIB_TOASCII)   || \
    defined(JSE_CLIB_TOLOWER)   || \
    defined(JSE_CLIB_TOUPPER)   || \
    defined(JSE_CLIB_PRINTF)    || \
    defined(JSE_CLIB_FPRINTF)   || \
    defined(JSE_CLIB_VPRINTF)   || \
    defined(JSE_CLIB_SPRINTF)   || \
    defined(JSE_CLIB_VSPRINTF)  || \
    defined(JSE_CLIB_RVSPRINTF) || \
    defined(JSE_CLIB_SYSTEM)    || \
    defined(JSE_CLIB_FSCANF)    || \
    defined(JSE_CLIB_VFSCANF)   || \
    defined(JSE_CLIB_SCANF)     || \
    defined(JSE_CLIB_VSCANF)    || \
    defined(JSE_CLIB_SSCANF)    || \
    defined(JSE_CLIB_VSSCANF)   || \
    defined(JSE_CLIB_VA_ARG)    || \
    defined(JSE_CLIB_VA_START)  || \
    defined(JSE_CLIB_VA_END)

jsebool 
GetJsecharFromStringOrNumber(jseContext jsecontext,jseVariable var,jsechar *value)
{
   if( jseTypeNumber == jseGetType(jsecontext,var) )
   {
      *value = (jsechar) jseGetByte(jsecontext,var);
      return True;
   }
   else if( jseTypeString == jseGetType(jsecontext,var) )
   {
      ulong length;
      const jsechar * str = jseGetString(jsecontext,var,&length);
      
      if( length == 1 )
      {
         *value = *str;
         return True;
      }
   }
   if ( jseTypeBuffer == jseGetType(jsecontext,var) )
   {
      ulong length;
      const ubyte * buf;
      
       buf = jseGetBuffer(jsecontext,var,&length);
       
       if( length == 1 )
       {
          *value = (jsechar)(*buf);
          return True;
       }
   }

   /* This will cause an error to be reported */
   jseVarNeed(jsecontext,var,JSE_VN_BYTE);
   return False;
}

#else
   ALLOW_EMPTY_FILE
#endif
