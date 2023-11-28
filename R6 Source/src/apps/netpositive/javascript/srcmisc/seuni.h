/* seuni.h     Handles translation from Unicode to Ascii and vice-versa.
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

/*
 * Unicode stuff:
 *
 * The following functions are unicode versions of the standard C library
 * versions:
 *
 * stricmp_jsechar, strcpy_jsechar, etc...
 *
 *
 * UNISTR(x)
 *
 *    Provides a Unicode string constant (x must be a string literal
 *    such as "hi")
 *
 *
 * const char *UnicodeToAscii(jsechar *unistring)
 *
 *      Translates the given unicode string into ascii and returns a buffer
 *      describing it. When you are finished with it, you must use
 *      'FreeAsciiString().'
 *
 *
 * const jsechar *AsciiToUnicode(jsechar char *asciistring)
 *
 *      Translates the given ascii string into unicode and returns a buffer
 *      describing it. When you are finished with it, you must use
 *      'FreeUnicodeString().'
 *
 *
 * const jsechar *AsciiLenToUnicode(jsechar char *asciistring,uword32 length)
 *
 *      Translates the given ascii string into unicode (max of length chars)
 *      and returns a buffer describing it. When you are finished with it,
 *      you must use 'FreeUnicodeString().'
 *
 *
 * void FreeAsciiString(const char *asciistring)
 *
 *      Frees up an ascii string returned from one of the other functions.
 *
 *
 * void FreeUnicodeString(const jsechar *asciistring)
 *
 *      Frees up an ascii string returned from one of the other functions.
 */

/*** revision history ***/
/*** revision history ***/

#ifndef _UNICODE_H
#define _UNICODE_H
#ifdef __cplusplus
   extern "C" {
#endif

#if !defined(JSE_UNICODE) || (0==JSE_UNICODE)

#  define UNISTR(x) x

#  define int_jsechar         int

#  define stricmp_jsechar     stricmp
#  define vsprintf_jsechar    vsprintf
#  define sprintf_jsechar     sprintf
#  define strstr_jsechar      strstr
#  define strspn_jsechar      strspn
#  define strcspn_jsechar     strcspn
#  define strtol_jsechar      strtol
#  define strtod_jsechar      strtod
#  define strpbrk_jsechar     strpbrk
#  define strrchr_jsechar     strrchr
#  define strchr_jsechar      strchr
#  define strlwr_jsechar      strlwr
#  define strupr_jsechar      strupr
#  define strnicmp_jsechar    strnicmp
#  define strncmp_jsechar     strncmp
#  define sscanf_jsechar      sscanf
#  define atoi_jsechar        atoi

#  define isalnum_jsechar     isalnum
#  define isalpha_jsechar     isalpha
#  define isascii_jsechar     isascii
#  define iscntrl_jsechar     iscntrl
#  define isdigit_jsechar     isdigit
#  define isgraph_jsechar     isgraph
#  define islower_jsechar     islower
#  define isprint_jsechar     isprint
#  define ispunct_jsechar     ispunct
#  define isspace_jsechar     isspace
#  define isupper_jsechar     isupper
#  define isxdigit_jsechar    isxdigit
#  define toascii_jsechar     toascii
#  define tolower_jsechar     tolower
#  define toupper_jsechar     toupper

#  define strcmpi_jsechar     strcmpi
#  define strcpy_jsechar      strcpy
#  define strncpy_jsechar     strncpy
#  define strcat_jsechar      strcat
#  define strncat_jsechar     strncat
#  define strcmp_jsechar      strcmp
#  define strlen_jsechar      strlen
#  define strtok_jsechar      strtok

#  define ltoa_jsechar        ltoa
#  define atol_jsechar        atol
#  define atof_jsechar        atof
#  define perror_jsechar      perror
#  define system_jsechar      system

#  define fread_jsechar       fread
#  define fopen_jsechar       fopen
#  define freopen_jsechar     freopen
#  define fgets_jsechar       fgets
#  define fprintf_jsechar     fprintf
#  define printf_jsechar      printf
#  define vfprintf_jsechar    vfprintf
#  define ungetc_jsechar      ungetc
#  define EOF_jsechar         EOF

#  if defined(__JSE_WINCE__)
#     define stat_jsechar     _stat
#  else
#     define stat_jsechar     stat
#  endif
#  define chdir_jsechar       chdir
#  define getcwd_jsechar      getcwd
#  define tmpnam_jsechar      tmpnam
#  define remove_jsechar      remove
#  define rename_jsechar      rename
#  define mkdir_jsechar       mkdir
#  define rmdir_jsechar       rmdir

#  define ctime_jsechar       ctime
#  define asctime_jsechar     asctime
#  define strftime_jsechar    strftime
#  define _fullpath_jsechar   _fullpath
#  define access_jsechar      access

#  define environ_jsechar     environ

  /* The following now have explicit casts in case they are used with const
     strings */
#  define UnicodeToAscii(x) (char *) x
#  define AsciiLenToUnicode(x,y) (const jsechar *) x
#  define AsciiToUnicode(x) (const jsechar *) x
#  define FreeAsciiString(x)
#  define FreeUnicodeString(x)

#else

#  ifndef UNICODE
#     define UNICODE
#  endif
#  ifndef __UNICODE__
#     define __UNICODE__
#  endif
#  ifndef _UNICODE
#     define _UNICODE
#  endif

#  include <tchar.h>
#  if defined(__WATCOMC__)
#     include <unistd.h>
#  endif

#  if defined(__JSE_WINCE__)
#     include "winceutl.h"
#  endif

#  define int_jsechar         wint_t

#  define strstr_jsechar      _tcsstr
#  define strspn_jsechar      _tcsspn
#  define strcspn_jsechar     _tcscspn
#  define vsprintf_jsechar    _vstprintf
#  define sprintf_jsechar     _stprintf
#  define stricmp_jsechar     _tcsicmp
#  define strcat_jsechar      _tcscat
#  define strncat_jsechar     _tcsncat
#  define strcmp_jsechar      _tcscmp
#  define strlen_jsechar      _tcslen

#  define strnicmp_jsechar    _tcsnicmp
#  define strncmp_jsechar     _tcsncmp
#  define sscanf_jsechar      _stscanf
#  define strtol_jsechar      _tcstol
#  define strtod_jsechar      _tcstod
#  define strpbrk_jsechar     _tcspbrk
#  define strrchr_jsechar     _tcsrchr
#  define strchr_jsechar      _tcschr
#  define strlwr_jsechar      _tcslwr
#  define strupr_jsechar      _tcsupr
#  define strtok_jsechar      _tcstok

#  define atoi_jsechar        _ttoi

#  define isalnum_jsechar     _istalnum
#  define isalpha_jsechar     _istalpha
#  define isascii_jsechar     _istascii
#  define iscntrl_jsechar     _istcntrl
#  define isdigit_jsechar     _istdigit
#  define isgraph_jsechar     _istgraph
#  define islower_jsechar     _istlower
#  define isprint_jsechar     _istprint
#  define ispunct_jsechar     _istpunct
#  define isspace_jsechar     _istspace
#  define isupper_jsechar     _istupper
#  define isxdigit_jsechar    _istxdigit
#  define toascii_jsechar     __toascii
#  define tolower_jsechar     _totlower
#  define toupper_jsechar     _totupper

#  define strcpy_jsechar      _tcscpy
#  define strncpy_jsechar     _tcsncpy

#  define ltoa_jsechar        _ltot
#  define atol_jsechar        _ttol
#  define atof_jsechar        atof
#  define perror_jsechar      _tperror

#  define system_jsechar      _tsystem

#  define fopen_jsechar       _tfopen
#  define freopen_jsechar     _tfreopen
#  define fgets_jsechar       _fgetts
#  define fprintf_jsechar     _ftprintf
#  define printf_jsechar      _tprintf
#  define vfprintf_jsechar    _vftprintf
#  define ungetc_jsechar      _ungettc
#  define chdir_jsechar       _tchdir
#  define getcwd_jsechar      _tgetcwd
#  define EOF_jsechar         _TEOF

#  define tmpnam_jsechar      _ttmpnam
#  define remove_jsechar      _tremove
#  define rename_jsechar      _trename
#  define mkdir_jsechar       _tmkdir
#  define rmdir_jsechar       _trmdir

#  define ctime_jsechar       _tctime
#  define asctime_jsechar     _tasctime
#  define strftime_jsechar    _tcsftime
#  define _fullpath_jsechar   _tfullpath
#  define access_jsechar      _taccess
#  define stat_jsechar        _tstat

#  define UNISTR(x) L##x

#  define environ_jsechar     _tenviron

#  define FreeAsciiString(x) jseMustFree((void *)x)
#  define FreeUnicodeString(x) jseMustFree((void *)x)

   const char * UnicodeToAscii(const jsechar * src);
   const jsechar * AsciiLenToUnicode(const char * src,uword32 count);
   const jsechar * AsciiToUnicode(const char * src);

#endif

#ifdef __cplusplus
}
#endif
#endif
