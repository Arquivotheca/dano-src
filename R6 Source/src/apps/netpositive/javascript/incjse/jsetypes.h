/* jseTypes.h     Recognized types for jse code
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

#ifndef _JSETYPES_H
#define _JSETYPES_H

#if !defined(JSE_FLOATING_POINT)
   /* unless specifically defined elsewhere, assume floating point */
#  define JSE_FLOATING_POINT 1
#endif

#include "jsedef.h"

#ifdef __cplusplus
extern "C" {
#endif


/************************************
 *** MUST HAVE DEFINED THE TARGET ***
 ************************************/
#if   defined(__JSE_DOS16__)
#elif defined(__JSE_DOS32__)
#elif defined(__JSE_OS2TEXT__)
#elif defined(__JSE_OS2PM__)
#elif defined(__JSE_WIN16__)
#elif defined(__JSE_WIN32__)
#elif defined(__JSE_CON32__)
#elif defined(__JSE_NWNLM__)
#elif defined(__JSE_UNIX__)
#elif defined(__JSE_390__)
#elif defined(__JSE_MAC__)
#elif defined(__JSE_PALMOS__)
#elif defined(__JSE_PSX__)
#elif defined(__JSE_EPOC32__)
#else
#  error UNDEFINED TARGET ENVIRONMENT
#endif

/**************************************
 *** MUST HAVE DEFINED THE COMPILER ***
 **************************************/
#if   defined(__BORLANDC__)
#elif defined(__WATCOMC__)
#elif defined(_MSC_VER)
#elif defined(__GNUC__)
#elif defined(__MWERKS__)
#elif defined(__IBMCPP__)
#elif defined(__IBMC__)
#else
#  error UNDEFINED COMPILER
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
#  define __near    _near
#  define __far     _far
#  define __huge    _huge
#  define __pascal  _pascal
#  define __cdecl   _cdecl
#endif

#if defined(_MSC_VER)
#  define inline    _inline
#endif

/* not all systems agree on the meaning of NULL always reflecting a full
 * pointer size.  On such system the following macros can be used.
 */
#ifndef NULLPTR
#  define NULLPTR ((void *)0)
#endif
#ifndef NULLCAST
#  define NULLCAST(DATATYPE) ((DATATYPE)0)
#endif

typedef signed char     sbyte;
typedef unsigned char   ubyte;
typedef signed int      sint;
#if !defined(__JSE_UNIX__) && !defined(__JSE_PALMOS__)
   /* already typedefed in system headers, don't do it again */
   typedef unsigned int    uint;
#else
#  include <sys/types.h>
#endif
#if !defined(__linux__) && !defined(__sgi__) && !defined(__osf__) && !defined(__JSE_BEOS__) && (!defined(__sun__) || !defined(__svr4__)) /* solaris */ // seb 98.11.12
typedef unsigned long   ulong;
#endif
typedef signed long     slong;

#define MAX_SBYTE       ((sbyte)0x7F)
#define MIN_SBYTE       ((sbyte)0x80)
#define MAX_UBYTE       ((ubyte)0xFF)
#if defined(__JSE_DOS16__) || defined(__JSE_DOS32__) \
 || defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) \
 || defined(__JSE_WIN16__) || defined(__JSE_390__) \
 || defined(__JSE_WIN32__) || defined(__JSE_CON32__) \
 || defined(__JSE_NWNLM__) || defined(__JSE_UNIX__) \
 || defined(__JSE_MAC__) || defined(__JSE_PSX__) \
 || defined(__JSE_PALMOS__) || defined(__JSE_EPOC32__)
#  define MAX_SLONG       ((slong)0x7FFFFFFFL)
#  define MIN_SLONG       ((slong)0x80000000L)
#  define MAX_ULONG       ((ulong)0xFFFFFFFFL)
#else
#  error Must define MAX_SLONG, MIN_SLONG, and MAX_ULONG
#endif

#if defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) \
 || defined(__JSE_DOS32__) \
 || defined(__JSE_WIN32__) || defined(__JSE_CON32__) \
 || defined(__JSE_NWNLM__) || defined(__JSE_UNIX__) \
 || defined(__JSE_MAC__) || defined(__JSE_PSX__) \
 || defined(__JSE_PALMOS__) || defined(__JSE_EPOC32__)
#  define MAX_SINT        MAX_SLONG
#  define MIN_SINT        MIN_SLONG
#  define MAX_UINT        MAX_ULONG
#elif defined(__JSE_DOS16__) || defined(__JSE_WIN16__) \
   || defined(__JSE_390__)
#  define MAX_SINT        ((sint)0x7FFF)
#  define MIN_SINT        ((sint)0x8000)
#  define MAX_UINT        ((uint)0xFFFF)
#else
#  error Must define MAX_SINT, MIN_SINT, and MAX_UINT
#endif

typedef sbyte           sword8;
typedef ubyte           uword8;
typedef signed short    sword16;
typedef unsigned short  uword16;
#if defined(__osf__)
   typedef signed int      sword32;
   typedef unsigned int    uword32;
#else
   typedef slong           sword32;
   typedef ulong           uword32;
#endif

/* JavaScript by default uses a float64 as the number
 * types.  But for implementations that do not want to support
 * floating point numbers the following can be changed to represent
 * any type for a jseNumber
 */
#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   typedef float           float32;
   typedef double          float64;
#  if !defined(_MSC_VER)
      typedef long double     float80;
#  endif
   typedef float64         jsenumber;
#else
   typedef sword32         jsenumber;
#endif


/* JavaScript by default uses unicode character.  But some
 * implementations may wish to be more compact, faster, or
 * back-dated and will instead choose ascii.  In either case
 * if jsechar is used to represent characters then JSE_UNICODE
 * only needs to be redefined.
 */
#if !defined(JSE_UNICODE)
#  if defined(__JSE_WINCE__)
#     define JSE_UNICODE 1
#  elif defined(__JSE_EPOC32__)
#     if defined(_UNICODE)
#        define JSE_UNICODE 1
#     else
#        define JSE_UNICODE 0
#     endif
#  else
#     define JSE_UNICODE 0
#  endif
#endif
#if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
   typedef wchar_t   jsechar;
#else
   typedef char    jsechar;
#endif

#define MAX_SWORD8      ((sword8)0x7F)
#define MIN_SWORD8      ((sword8)0x80)
#define MAX_UWORD8      ((uword8)0xFF)
#define MAX_SWORD16     ((sword16)0x7FFF)
#define MIN_SWORD16     ((sword16)0x8000)
#define MAX_UWORD16     ((uword16)0xFFFF)
#define MAX_SSHORT      MAX_SWORD16
#define MIN_SSHORT      MIN_SWORD16
#define MAX_USHORT      MAX_UWORD16
#define MAX_SWORD32     ((sword32)0x7FFFFFFF)
#define MIN_SWORD32     ((sword32)0x80000000)
#define MAX_UWORD32     ((uword32)0xFFFFFFFF)


typedef sint   jsebool;
#if defined(__JSE_390__)
   typedef sint jsetinybool;
#else
   typedef uword8 jsetinybool;
#endif
#if !defined(False) || !defined(True)
#  define  False 0
#  define  True  1
#endif
#if !defined(FALSE) || !defined(TRUE)
#  define FALSE 0
#  define TRUE 1
#endif

#if defined(__linux__)
#  include <endian.h>
#  include <bytesex.h>
#endif
#if defined(__FreeBSD__)
#  include <machine/endian.h>
#endif
#if defined(_AIX)
#  include <sys/machine.h>
#endif
#if defined(__osf__)
#  include <alpha/endian.h>
#endif
#if defined(__sgi__)
#  include <sys/endian.h>
#endif
#if defined(__BEOS__)
#  include <endian.h>
#endif

/* define the Endianness of this system; define False if not
 * BIG_ENDIAN, else True
 */
#if defined(__BYTE_ORDER)
#  if __BYTE_ORDER==BIG_ENDIAN
#     undef BIG_ENDIAN
#     define BIG_ENDIAN True
#  else
#     undef BIG_ENDIAN
#     define BIG_ENDIAN False
#   endif
#elif defined(BYTE_ORDER)
#   if BYTE_ORDER==BIG_ENDIAN
#     undef BIG_ENDIAN
#     define BIG_ENDIAN True
#   else
#     undef BIG_ENDIAN
#     define BIG_ENDIAN False
#   endif
#elif defined(__JSE_DOS16__) || defined(__JSE_DOS32__) \
 || defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) \
 || defined(__JSE_WIN16__) || defined(__JSE_390__) \
 || defined(__JSE_WIN32__) || defined(__JSE_CON32__) \
 || defined(__JSE_NWNLM__) || defined(__JSE_PSX__)
#  define BIG_ENDIAN False
#elif defined(__JSE_MAC__) || defined(__JSE_PALMOS__)
#  define BIG_ENDIAN True
#elif defined(__sun__)
#  define BIG_ENDIAN True
#elif defined(__hpux__)
#  define BIG_ENDIAN True
#elif defined(__JSE_EPOC32__)
#  if defined(__WINS__)
#     define BIG_ENDIAN False
#  else
#     define BIG_ENDIAN True
#  endif
// seb 98.1.25
#elif defined(__JSE_BEOS__) && (defined(__POWERPC__) || defined(__ARMEB__))	/* FIXME: This should probably use <endian.h> for the right define */
#	define BIG_ENDIAN True
#elif defined(__JSE_BEOS__) && (defined(__INTEL__) || defined(__ARMEL__))	/* FIXME: This should probably use <endian.h> for the right define */
#	define BIG_ENDIAN False
#else
#  error cant figure byte order for this machine
#endif

#if !defined(JSE_POINTER_SIZE)
   /* if pointer size not defined, assume 32 bits */
#  define JSE_POINTER_SIZE  32
#endif
#if JSE_POINTER_SIZE == 32
#  define JSE_POINTER_SINT    sword32
#  define JSE_POINTER_UINT    uword32
#elif JSE_POINTER_SIZE == 16
#  define JSE_POINTER_SINT    sword16
#  define JSE_POINTER_UINT    uword16
#elif JSE_POINTER_SIZE == 8
#  define JSE_POINTER_SINT    sword8
#  define JSE_POINTER_UINT    uword8
#else
#  error JSE_POINTER_SIZE not understood on this system
#endif

#if !defined(_NEAR_)
#  if defined(__JSE_DOS16__) || defined(__JSE_WIN16__)
#     define  _NEAR_   __near
#  else
#     define  _NEAR_   /* */
#  endif
#endif

#if !defined(_FAR_)
#  if defined(__JSE_DOS16__) || defined(__JSE_WIN16__)
#     define  _FAR_   __far
#  else
#     define  _FAR_   /* */
#  endif
#endif

#if !defined(FAR_CALL)
#  if defined(__JSE_DOS16__) || defined(__JSE_WIN16__)
#     define  FAR_CALL   __far
#  else
#     define  FAR_CALL   /* */
#  endif
#endif

#if !defined(JSE_POINTER_SINDEX)
#  if defined(__JSE_DOS16__) || defined(__JSE_WIN16__)
#     if defined(JSE_NO_HUGE)
#        define  _HUGE_   /* */
#        define JSE_POINTER_SINDEX sint
#        define JSE_POINTER_UINDEX uint
#        define JSE_PTR_MAX_SINDEX MAX_SINT
#        define JSE_PTR_MIN_SINDEX MIN_SINT
#        define JSE_PTR_MAX_UINDEX MAX_UINT
#     else
#        define  _HUGE_   __huge
#        define JSE_POINTER_SINDEX slong
#        define JSE_POINTER_UINDEX ulong
#        define JSE_PTR_MAX_SINDEX MAX_SLONG
#        define JSE_PTR_MIN_SINDEX MIN_SLONG
#        define JSE_PTR_MAX_UINDEX MAX_ULONG
#     endif
#  else
#     define  _HUGE_   /* */
#     define JSE_POINTER_SINDEX slong
#     define JSE_POINTER_UINDEX ulong
#     define JSE_PTR_MAX_SINDEX MAX_SLONG
#     define JSE_PTR_MIN_SINDEX MIN_SLONG
#     define JSE_PTR_MAX_UINDEX MAX_ULONG
#  endif
#endif

#define  VAR_DATA(DATADEF) DATADEF _NEAR_
#define  CONST_DATA(DATADEF) const DATADEF _NEAR_

/* Borland DOS16 and Win16 should always be far_calls until the core
 * can fit in 64k
 */
#if !defined(NEAR_CALL)
#  if defined(NDEBUG) && (defined(__JSE_DOS16__) \
 /*|| defined(__JSE_WIN16__)*/) && !defined(__BORLANDC__)
#     define  NEAR_CALL   __near
#  else
#     define  NEAR_CALL   /*  */
#  endif
#endif


#  if defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)
#    if defined(__JSE_WIN32__) && defined(__OPEN32__)
#      define WINDOWS_CALLBACK_FUNCTION(FUNCTION_TYPE)   FUNCTION_TYPE WINAPI
#    elif (defined(__JSE_WIN32__) || defined(__JSE_CON32__)) && defined(__BORLANDC__)
#      define WINDOWS_CALLBACK_FUNCTION(FUNCTION_TYPE)   __export FUNCTION_TYPE CALLBACK
#    elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)
#      define WINDOWS_CALLBACK_FUNCTION(FUNCTION_TYPE)   __declspec(dllexport) FUNCTION_TYPE CALLBACK
#    else
#      define WINDOWS_CALLBACK_FUNCTION(FUNCTION_TYPE)   FUNCTION_TYPE __export __far __pascal
#    endif
#  endif


/* methods for working with special number types that work in any
 * operating system and with any link method.  The following field
 * is initialized in jseengin.cpp for unix systems, and in globldat.cpp
 * for non-unix systems.
 */
#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
#  if defined(__JSE_UNIX__)
      extern VAR_DATA(jsenumber) jse_special_math[5];
#     define jseZero        jse_special_math[0]
#     define jseNegZero     jse_special_math[1]
#     define jseInfinity    jse_special_math[2]
#     define jseNegInfinity jse_special_math[3]
#     define jseNaN         jse_special_math[4]
#  else
      extern CONST_DATA(uword32) jse_special_math[10];
#     define jseZero        (*(jsenumber *)(jse_special_math))
#     define jseNegZero     (*(jsenumber *)(jse_special_math+2))
#     define jseInfinity    (*(jsenumber *)(jse_special_math+4))
#     define jseNegInfinity (*(jsenumber *)(jse_special_math+6))
#     define jseNaN         (*(jsenumber *)(jse_special_math+8))
#  endif

   /* I've replaced the calls to memcmp with this junk. I know it looks ugly,
    * but it is a *lot* faster (wrldfrct is .2 secs faster due to it. Basically,
    * since you can't compare doubles vs NAN (it always succeeds), we take the
    * address of the double to check and the NaN value and cast them to uword32s.
    * Then we can do regular compares on the 8 bytes of memory.
    */
#  define JSE_NOTFINITE_LOW   0  /* all non-finite number low bits are 0 */
#  define JSE_NAN_HIGH        0x7fffffffL
#  define JSE_INF_HIGH        0x7ff00000L
#  define JSE_NEGNUM_BIT      0x80000000L
#  if BIG_ENDIAN == True
#     define JSE_FASW_HI 0
#     define JSE_FASW_LO 1
#  else
#     define JSE_FASW_HI 1
#     define JSE_FASW_LO 0
#  endif

#  define jseIsNaN(num) ( (JSE_NAN_HIGH==((uword32 *)&num)[JSE_FASW_HI]) \
                     &&   (JSE_NOTFINITE_LOW==((uword32 *)&num)[JSE_FASW_LO]) )
#  define jseIsNegInfinity(num) \
       (((JSE_NEGNUM_BIT|JSE_INF_HIGH)==((uword32 *)&num)[JSE_FASW_HI]) \
     && (JSE_NOTFINITE_LOW==((uword32 *)&num)[JSE_FASW_LO]))
#  define jseIsInfinity(num) \
       ((JSE_INF_HIGH==((uword32 *)&num)[JSE_FASW_HI]) \
     && (JSE_NOTFINITE_LOW==((uword32 *)&num)[JSE_FASW_LO]))
#  define jseIsInfOrNegInf(num) \
       ((JSE_INF_HIGH == ((((uword32 *)&num)[JSE_FASW_HI]) & ~JSE_NEGNUM_BIT)) \
     && (JSE_NOTFINITE_LOW==((uword32 *)&num)[JSE_FASW_LO]))
   /* is finite determines quickest if the number is finite in any */
#  define jseIsFinite(num) \
      ( (JSE_INF_HIGH != (((uword32 *)&num)[JSE_FASW_HI] & JSE_INF_HIGH)) \
     || (JSE_NOTFINITE_LOW != ((uword32 *)&num)[JSE_FASW_LO]) \
     || ( (JSE_NAN_HIGH != ((uword32 *)&num)[JSE_FASW_HI]) \
       && (JSE_INF_HIGH != ((((uword32 *)&num)[JSE_FASW_HI]) & ~JSE_NEGNUM_BIT))  \
        ) \
      )
#  define jseIsZero(num) \
      ( 0 == ((uword32 *)&num)[JSE_FASW_LO] \
     && 0 == (((uword32 *)&num)[JSE_FASW_HI] & ~JSE_NEGNUM_BIT) )
#  define jseIsPosZero(num) \
      ( 0 == ((uword32 *)&num)[JSE_FASW_LO] \
     && 0 == ((uword32 *)&num)[JSE_FASW_HI] )
#  define jseIsNegZero(num) \
      ( JSE_NEGNUM_BIT == ((uword32 *)&num)[JSE_FASW_HI] \
     && 0 == ((uword32 *)&num)[JSE_FASW_LO] )
#  define jseIsNegative(num) \
      ( 0 != (((uword32 *)&num)[JSE_FASW_HI] & JSE_NEGNUM_BIT) )

#else

   extern CONST_DATA(jsenumber) jse_special_math[5];

#  define jseZero        jse_special_math[0]
#  define jseNegZero     jse_special_math[1]
#  define jseInfinity    jse_special_math[2]
#  define jseNegInfinity jse_special_math[3]
#  define jseNaN         jse_special_math[4]

#  define jseIsNaN(A_Number)     ( A_Number == jseNaN )
#  define jseIsInfinity(num)     ( num == jseInfinity )
#  define jseIsNegInfinity(num)  ( num == jseNegInfinity )
#  define jseIsZero(num)         ( 0 == num )
#  define jseIsPosZero(num)      ( 0 == num )
#  define jseIsNegZero(num)      ( 0 == num )
#  define jseIsNegative(num)     ( num < 0 )
#  define jseIsInfOrNegInf(num)  ( jseIsInfinity(num) || jseIsNegInfinity(num) )
#  define jseIsFinite(num)       ( !jseIsNaN(num) && !jseIsInfinity(num) && !jseIsNegInfinity(num) )

#endif

#ifdef __cplusplus
}
#endif

#endif
