/* utilstr.c    Random utilities used by ScriptEase.
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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifndef __JSE_MAC__
#if !defined(__JSE_WINCE__)
#  include <assert.h>
#endif
/* I've overloaded the asert macro, and this will ruin it */
#endif
#include "jseopt.h"
#include "jsetypes.h"
#include "utilstr.h"
#include "jsemem.h"


#if defined(JSE_UNICODE) && (0!=JSE_UNICODE)

#  include "seuni.h"

/* unicode conversion functions */
const char * UnicodeToAscii(const jsechar * src)
{
   size_t count = strlen_jsechar(src);
   char * dest = jseMustMalloc(char,(count+1)*sizeof(char));
   char * dst = dest;
   while ( count-- )
   {
      *(dst++) = (char)(*(src++));
   }
   *dst = 0;
   return dest;
}

const jsechar * AsciiLenToUnicode(const char *src,uword32 count)
{
   jsechar * dest = jseMustMalloc(jsechar,(count+1)*sizeof(jsechar));
   jsechar * dst = dest;
   while ( count-- )
   {
      *(dst++) = (jsechar)(*(src++));
   }
   *dst = 0;    
   return dest;
}

const jsechar * AsciiToUnicode(const char * src)
{
   return AsciiLenToUnicode(src,strlen(src));
}
#endif

sint jsecharCompare(const jsechar *str1,JSE_POINTER_UINDEX len1,
                    const jsechar *str2,JSE_POINTER_UINDEX len2)
{
   sint result;
   JSE_POINTER_UINDEX lmin = min(len1,len2);
#if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
   result = 0;
   while ( (0 != lmin--)  &&  (0 == (result = *(str1++) - *(str2++)))  ) ;
#else
   result = HugeMemCmp(str1,str2,lmin);
#endif
   if ( 0 == result  &&   len2 != len1 )
   {
      result = ( len1 < len2 ) ? -1 : 1 ;
   }
   return result;
}

ulong BaseToULong(const jsechar *HexStr,uint Base,uint MaxStrLen,
                  ulong MaxResult,uint *CharsUsed)
{
   ulong i, prev_i;
   uint Used;
   uint c;

   for ( i = prev_i = 0, Used = 0; 0 < MaxStrLen;
         MaxStrLen--, HexStr++, Used++ )
   {
      c = (uint) toupper_jsechar((ubyte)(*HexStr));
      if ( '0' <= c  &&  c <= '9' )
      {
         c -= '0';
      }
      else if ( 'A' <= c )
      {
         c -= ('A' - 10);
      }
      else
      {
         break;
      } /* endif */
      if ( Base <= c )
      {
         break;
      } /* endif */
      i = (i * Base) + c;
      if ( MaxResult < i  ||  i < prev_i/*rollover*/ )
      {
         i = prev_i;
         break;
      } /* endif */
   } /* endfor */
   if ( NULL != CharsUsed )
   {
      *CharsUsed = Used;
   } /* endif */
   return(i);
}

   slong
MY_strtol(const jsechar *s,jsechar **endptr,int radix)
/* because Borland's was failing with big hex numbers */
{
   jsechar *source = (jsechar *)s;
   uint used;
   slong result;

   assert( s != NULL );
   source += strspn_jsechar(source, UNISTR("\t\r\n"));

   if ((0 == strncmp_jsechar(source,UNISTR("0x"),2) ||
        0 == strncmp_jsechar(source,UNISTR("0X"),2))
       && (radix == 0  ||  radix == 16))
   {
      source += 2;
   }
   else if ( radix != 16 )
   {
      return(strtol_jsechar(s,endptr,radix));
   } /* endif */

   /* convert using my base conversion routines */
   result = (slong) BaseToULong(source,16,
            #if defined(__JSE_DOS16__) || defined(__JSE_DOS32__) \
             || defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) \
             || defined(__JSE_WIN16__) || defined(__JSE_390__) \
             || defined(__JSE_WIN32__) || defined(__JSE_CON32__) \
             || defined(__JSE_NWNLM__) || defined(__JSE_UNIX__) \
             || defined(__JSE_MAC__) || defined(__JSE_PALMOS__) \
             || defined(__JSE_PSX__)
               32,
            #else
               #  error How many bits in the largest ulong?
            #endif
                          MAX_ULONG,&used);
   if ( endptr != NULL )
   {
      *endptr = ( 0 == used ) ? (jsechar *)s : source + used ;
   } /* endif */
   return(result);
}

CONST_DATA(jsechar) WhiteSpace[] = UNISTR(" \t\r\n\f\v");
CONST_DATA(jsechar) SameLineWhiteSpace[] = UNISTR(" \t");
#ifdef __JSE_MAC__
  CONST_DATA(jsechar) NewLine[] = UNISTR("\r");
#else
  CONST_DATA(jsechar) NewLine[] = UNISTR("\r\n");
#endif

   void
RemoveWhitespaceFromHeadAndTail(jsechar * const buf)
/* remove whitespace from beginning and end of SourceCmdLine */
{
   /* skip beyond any beginning whitespace */
   uint Len = strlen_jsechar(buf);
   uint Remove = strspn_jsechar(buf,WhiteSpace);
   if ( Remove )
      memmove(buf,buf+Remove,sizeof(jsechar)*((Len -= Remove)+1));
   /* remove any whitespace at the end */
   while ( Len  &&  IS_WHITESPACE(buf[--Len]))
   {
      buf[Len] = '\0';
   }
}


jsechar * StrCpyMallocLen(const jsechar *Src,size_t len)
{
   jsechar *dest;

   assert( NULL != Src  ||  0 == len );
   dest = jseMustMalloc(jsechar,sizeof(jsechar)*(len+1));
   memcpy(dest,Src,sizeof(jsechar)*len);
   dest[len] = 0;
   return(dest);
}

#if defined(__JSE_MAC__) || defined(__DJGPP__) || defined(__JSE_UNIX__) \
 || defined(__JSE_PSX__) || defined(__JSE_390__) || defined(__JSE_EPOC32__)
void NEAR_CALL
long_to_string(long i,jsechar *StringBuffer)
{
   sprintf_jsechar(StringBuffer,UNISTR("%d"),i);
}
#endif /* JSE_MAC */


#if !defined(JSE_FLOATING_POINT) || (0==JSE_FLOATING_POINT) \
  || defined(__JSE_PSX__)
void jse_vsprintf(jsechar *buf,const jsechar *FormatString,va_list arglist)
{
   /* NYI: Build a more-through version of non-floating sprintf */
   strcpy(buf,FormatString);
}

void jse_sprintf(jsechar *buf,const jsechar *FormatString,...)
{
   va_list arglist;
   va_start(arglist,FormatString);
   jse_vsprintf(buf,FormatString,arglist);
   va_end(arglist);
}
#endif /* !defined(JSE_FLOATING_POINT) */

#if defined(__JSE_MAC__) || defined(__JSE_UNIX__) || defined(__JSE_PSX__) \
 || defined(__JSE_390__) || defined(__JSE_EPOC32__)

jsechar * 
#if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
lstrlwr(jsechar *str)
#else
strlwr(jsechar * str)
#endif
{
   jsechar *c;
   for ( c = str; *c; c++ )
      *c = (jsechar) tolower_jsechar(*c);
   return str;
}


jsechar * 
#if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
lstrupr(jsechar *str)
#else
strupr(jsechar * str)
#endif
{
   jsechar *c;
   for ( c = str; *c; c++ )
      *c = (jsechar) toupper_jsechar(*c);
   return str;
}


int strnicmp_jsechar(const jsechar *str1,const jsechar *str2,size_t len)
{
   int result = 0;
   while ( 0 != len--
        && 0 == (result = tolower(*str1) - tolower(*str2))
        && 0 != *str1 )
      str1++, str2++;
   return result;
}

int stricmp_jsechar(const jsechar *str1,const jsechar *str2)
{
   int result;
   while ( 0 == (result = tolower(*str1) - tolower(*str2))
       &&  0 != *str1 )
      str1++, str2++;
   return result;
}

#endif /* __JSE_MAC__ || __JSE_UNIX__ */
