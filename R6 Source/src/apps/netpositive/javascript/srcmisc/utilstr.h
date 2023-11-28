/* utilstr.h  string handling utility functions
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

#ifndef _UTILSTR_H
#define _UTILSTR_H

#include "jsemem.h"
#include "jseopt.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IS_WHITESPACE(C)               (NULL != strchr_jsechar(WhiteSpace,C))
#define IS_SAMELINE_WHITESPACE(C)      (NULL != strchr_jsechar(SameLineWhiteSpace,C))
#define SKIP_WHITESPACE(C)             (C += strspn_jsechar( C, WhiteSpace ))
#define SKIP_SAMELINE_WHITESPACE(C)    \
   while( 0 != *C  &&  IS_SAMELINE_WHITESPACE(*C) ) { C++; }
#define IS_NEWLINE(C)                  (NULL != strchr_jsechar(NewLine,C))

extern CONST_DATA(jsechar) WhiteSpace[];
extern CONST_DATA(jsechar) SameLineWhiteSpace[]; /* doesn't include new line */
extern CONST_DATA(jsechar) NewLine[]; /* any of these characters say it's a new
                                      line */

#if defined(__WATCOMC__) || defined(__BORLANDC__) \
 || (defined(_MSC_VER) && !defined(__JSE_EPOC32__))
#  define long_to_string(LNUM,BUF)  ltoa_jsechar(LNUM,BUF,10)
#else
   void NEAR_CALL long_to_string(long i,jsechar *StringBuffer);
   /* assume that string buffer will be big enough to hold the integer */
#endif

void RemoveWhitespaceFromHeadAndTail(jsechar * const buf);
  /* memmoves from beyond whitespace to buffer, then truncates whitespace
     at end */

jsechar *StrCpyMallocLen(const jsechar *Src,size_t len);

#define StrCpyMalloc(s) StrCpyMallocLen(s,strlen_jsechar(s))
#define FreeIfNotNull(ptr) if( ptr!=NULL ) jseMustFree((void *)ptr);

#if (defined(__JSE_MAC__) && defined(__MWERKS__)) || defined(__JSE_UNIX__) \
 || defined(__JSE_390__) || defined(__JSE_EPOC32__)

   int strnicmp(const jsechar *str1,const jsechar *str2,size_t len);
   int stricmp(const jsechar *str1,const jsechar *str2);
   jsechar * strupr(jsechar *string);
   jsechar * strlwr(jsechar *string);

#endif /* __JSE_MAC__ || __JSE_UNIX__ || __JSE_390__ */


/* common stuff for all versions */


sint jsecharCompare(const jsechar _HUGE_ *str1,JSE_POINTER_UINDEX len1,
                    const jsechar _HUGE_ *str2,JSE_POINTER_UINDEX len2);
   /* acts a lot like strcmp, but with unicode and length and possibly
    * embedded null characters.  returns 0, >0, or <0
    */
ulong BaseToULong(const jsechar *HexStr,uint Base,uint MaxStrLen,
                  ulong MaxResult,uint *CharsUsed);
   /* CharsUsed==NULL if you don't care */
slong MY_strtol(const jsechar *s,jsechar **endptr,int radix);
   /* because Borland's was failing with big hex numbers */

#if !defined(JSE_FLOATING_POINT) || (0==JSE_FLOATING_POINT) || \
    defined(__JSE_PSX__)
   /* simplified sprintf taking only these types:
    *  %s,  %d,  %u,  %X,  %ld,  %lu,  %lX,  %c
    */
   void jse_vsprintf(jsechar *buf,const jsechar *FormatString,va_list arglist);
   void jse_sprintf(jsechar *buf,const jsechar *FormatString,...);
#else
#  define jse_vsprintf vsprintf_jsechar
#  define jse_sprintf  sprintf_jsechar
#endif


#ifdef __cplusplus
}
#endif
#endif
