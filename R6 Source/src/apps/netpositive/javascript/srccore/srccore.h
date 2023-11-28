/*  srccore.h
 *      
 *  include file that includes everything needed to compile any scriptease 
 *  source file. This is included first and get precompiled, hopefully greatly 
 *  speeding compilation.
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

#ifndef _SRCCORE_H
#define _SRCCORE_H

#if !defined(JSE_CACHE_LOCAL_VARS)
#  define JSE_CACHE_LOCAL_VARS   1
#endif

#if !defined(JSE_CACHE_GLOBAL_VARS)
#  define JSE_CACHE_GLOBAL_VARS  1
#endif

#if defined(JSE_UNICODE) && (0!=JSE_UNICODE) && !defined(UNICODE)
#  define UNICODE
#endif


/* JSE_HASH_STRINGS is an alternate method of storing strings in the string 
 * table which automatically cleans itself when the strings are no longer 
 * used.  It is also somewhat faster than the normal storage method, but it
 * incurs some overhead when deleting strings.
 */
#if (!defined(JSE_MIN_MEMORY) || (0==JSE_MIN_MEMORY)) && \
    !defined(JSE_HASH_STRINGS)
#  define  JSE_HASH_STRINGS   1
#endif

/* JSE_ONE_STRING_TABLE specifies that a single global string table should 
 * be used, instead of being stored in the call global.  By default, it
 * is disabled.
 */
#if !defined(JSE_ONE_STRING_TABLE)
#  define JSE_ONE_STRING_TABLE  0
#endif

/* JSE_HASH_SIZE determines the default size of the hash table, or if 
 * JSE_ONE_STRING_TABLE is defined, the size of the global table
 */
#if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS) && \
    !defined(JSE_HASH_SIZE)
#  define JSE_HASH_SIZE   256
#endif

/* JSE_FAST_MEMPOOL, if defined, will make many of our commonly-allocated data
 * types re-use pre-allocation memory pools.  This can make the program
 * execute A LOT faster.  However, with memory pools a program can use up more
 * memory, and so where memory is super-critical and performance is not (this
 * can run 40% slower without JSE_FAST_MEMPOOL in DOS16) don't turn on this
 * field.
 */
#if !defined(JSE_FAST_MEMPOOL)
#  if defined(__JSE_DOS16__)
#     define JSE_FAST_MEMPOOL  0
#  else
#     define JSE_FAST_MEMPOOL  1
#  endif
#endif

#if !defined(FALSE) || !defined(TRUE)
  #define FALSE 0
  #define TRUE 1
#endif

/* Undefine the following if want to build without checking for legitimate
   user keys */
#if !defined(__JSE_PSX__)
    /* don't check because system doesn't have 'time()' */
// seb 98.11.12 -- Took this out 'cuz I can.
//#   define CHECK_FOR_VALID_USER_KEYS   1
#endif

#define STRICT  1

#if !defined(__JSE_WINCE__)
#include <assert.h>
#endif

#ifdef __JSE_MAC__
   /* Replace the assert macro */
#  ifndef NDEBUG
#     undef assert
#     define assert(condition) ((condition) ? ((void) 0) : \
                                my_assertion_failed(#condition, __FILE__, \
                                __LINE__)) 
#  endif
#endif

#if !defined(__JSE_WINCE__)
#include <stdio.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#if !defined(__JSE_MAC__) && !defined(__JSE_UNIX__) \
 && !defined(__JSE_390__) && !defined(__JSE_EPOC32__)
  #include <malloc.h>
#endif
#if defined(__JSE_UNIX__)
  #include <stdlib.h>
#endif

#if defined(__WATCOMC__) && (defined(__JSE_DOS16__) || \
                             defined(__JSE_WIN16__) || defined(__JSE_DOS32__))
  #include <i86.h>
#endif
#if (defined(__BORLANDC__) || defined(_MSC_VER)) \
 && !defined(__JSE_EPOC32__) && !defined(__JSE_WINCE__)
  #include <dos.h>
#endif

#if !defined(__BORLANDC__) || (defined(__BORLANDC__) && \
                               defined(__JSE_OS2TEXT__))
   #ifndef max
     #define max(a,b)            (((a) > (b)) ? (a) : (b))
   #endif
   #ifndef min
     #define min(a,b)            (((a) < (b)) ? (a) : (b))
   #endif
#endif

#if defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)
   #include <windows.h>
#endif

#if defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
  #define INCL_DOSDATETIME
  #define INCL_DOSERRORS
  #define INCL_DOSFILEMGR
  #define INCL_DOSMODULEMGR
  #define INCL_DOSPROCESS
  #define INCL_DOSSEMAPHORES
  #define INCL_DOSSESMGR
  #define INCL_ERRORS
  #define INCL_ORDINALS
  #define INCL_WINATOM
  #define INCL_WINSWITCHLIST
  #define INCL_DOSDEVICES
  #if defined(__JSE_OS2PM__)
    #define INCL_PM
  #else
    #define INCL_VIO
    #define INCL_KBD
  #endif
  #include <os2.h>
  #include <os2def.h>
#endif

#define jseContext_ struct Call
#define jseVariable_ struct Var
#define jseFunction_ struct Function
#define jseStack_ struct jseCallStack
#include "jsetypes.h"
#include "jselib.h"
#include "seuni.h"

/* We cheat on varnames. If it is even, it is a regular jsechar *.
 * If it is odd, it is actually a numeric name, derived as
 * (2*number)-1.
 */
typedef void * VarName;

struct Var;
typedef struct Var Var;
typedef struct Var VarWrite;
typedef struct Var VarRead;

struct Source;
#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   struct TokenSrc;
#endif
#if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)
   struct TokenDst;
#endif
struct FunctionParms;
struct LocalFunction;

#ifndef NDEBUG
   #include "dbgprntf.h"
#endif
#include "dirparts.h"
#include "textcore.h"
#include "jsemem.h"
#include "mempool.h"
#include "util.h"
#include "utilstr.h"
#include "utilhuge.h"
#include "globldat.h"

#include "atexit.h"
#include "call.h"
#include "function.h"
#include "secode.h"
#include "jseengin.h"
#include "code.h"
#include "define.h"
#if defined(JSE_LINK) && (0!=JSE_LINK)
#  include "extlib.h"
#endif
#include "library.h"
#include "security.h"
#include "source.h"
#include "token.h"
#include "var.h"
#include "stack.h"
#include "loclfunc.h"
#include "operator.h"

#ifdef __JSE_UNIX__
  #include "unixfunc.h"
#endif

#if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) && \
    (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
#define MayIContinue(call) \
   ( ((call)->Global->ExternalLinkParms.MayIContinue) ? \
         (jsebool)DispatchToClient((call)->Global->ExternalDataSegment,\
                                 (ClientFunction)((call)->Global-> \
                                 ExternalLinkParms.MayIContinue),\
                                 (void *)call) : \
    True)
#else
#define MayIContinue(call) \
   ( ((call)->Global->ExternalLinkParms.MayIContinue) ? \
     ( (*((call)->Global->ExternalLinkParms.MayIContinue))(call) ): \
    True)
#endif

#if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) && \
    (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
#  if defined(__cplusplus)
   extern "C"
#  endif
   uword16 _FAR_ * cdecl FAR_CALL Get_SS_BP();
#endif

#if (defined(__JSE_DOS16__)  ||  defined(__JSE_WIN16__)) \
   && (defined(__SMALL__) || defined(__TINY__) || defined(__MEDIUM__))
   #define  FAR_MEMCPY(d,s,l)  _fmemcpy(d,s,l)
   #define  FAR_MEMCHR(s,c,l)  _fmemchr(s,c,l)
   #define  FAR_STRLEN(s)      _fstrlen(s)
   #define  FAR_STRCPY(d,s)    _fstrcpy(d,s)
#else
   #define  FAR_MEMCPY(d,s,l)  memcpy(d,s,l)
   #define  FAR_MEMCHR(s,c,l)  memchr(s,c,l)
   #define  FAR_STRLEN(s)      strlen(s)
   #define  FAR_STRCPY(d,s)    strcpy(d,s)
#endif

#if defined(__JSE_PALMOS__) || defined(__JSE_PSX__)
#  define EXIT_SUCCESS   0
#  define _MAX_PATH      255
#endif

#endif
