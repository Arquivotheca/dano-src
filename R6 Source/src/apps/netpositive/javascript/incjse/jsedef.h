/******************************************************
 *  platform.h                                        *
 *  When integrating the SE:ISDK, use this header to  *
 *  automatically define the platform and link method *
 *****************************************************/

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

#ifndef _PLATFORM_H
#define _PLATFORM_H

#if !defined(JSETOOLKIT_CORE) && !defined(JSETOOLKIT_APP) && \
    !defined(JSETOOLKIT_LINK)
#  define JSETOOLKIT_APP
#endif

/* if they havnt alread defined __JSE_DLLLOAD__ or __JSE_DLLRUN__
     then flag to link with the static library
 */
#if !defined(__JSE_DLLLOAD__) && !defined(__JSE_LIB__)
#  define __JSE_LIB__
#endif

#if defined(__DJGPP__)
#  if !defined(__JSE_DOS32__)
#     define __JSE_DOS32__
#  endif
#elif defined(__palmos__)
#  define __JSE_PALMOS__
#elif defined __PSISOFT32__
#  if !defined(__JSE_EPOC32__)
#     define __JSE_EPOC32__
#  endif
#elif defined(__psx__)
#  define __JSE_PSX__
#elif defined(__unix__) && !defined(__JSE_UNIX__)
#  if !defined(__JSE_UNIX__)
#     define __JSE_UNIX__
#  endif
#elif defined(__WATCOMC__)
   /* Watcom - All versions */
#  if defined(__DOS__) && defined(__I86__)
#     if !defined(__JSE_DOS16__)
#        define __JSE_DOS16__
#     endif
#  elif defined(__DOS__) && defined(__386__)
#     if !defined(__JSE_DOS32__)
#        define __JSE_DOS32__
#     endif
#  elif defined(__OS2__) && defined(__I86__)
#    error 16bit OS2 not supported
#  elif defined(__OS2__) && defined(__386__)
#     if !defined(__JSE_OS2TEXT__)
#        if !defined(__JSE_OS2PM__)
#           define __JSE_OS2PM__
#        endif
#     endif
#  elif defined(__QNX__)
#    error QNX currently not supported
#  elif defined(__NETWARE__)
#     if !defined(__JSE_NWNLM__)
#        define __JSE_NWNLM__
#     endif
#  elif defined(__NT__)
#     if !defined(__JSE_CON32__) && !defined(__JSE_WIN32__)
#        define __JSE_WIN32__
#     endif
#  elif defined(__WINDOWS__)
#     if !defined(__JSE_WIN16__)
#        define __JSE_WIN16__
#     endif
#  else
#     error Urecognized WATCOM target platform
#  endif
#elif defined(_MSC_VER) && _MSC_VER == 800
   /* MSVC 1.52 */
#  if defined(_MSDOS) && !defined(_WINDOWS)
#     if !defined(__JSE_DOS16__)
#        define __JSE_DOS16__
#     endif
#  elif defined(_WINDOWS)
#     if !defined(__JSE_WIN16__)
#        define __JSE_WIN16__
#     endif
#  else
#     error Unrecognized MSVC1.52 platform
#  endif
#elif defined(_MSC_VER) && _MSC_VER >= 1000
   /* MSVC 4.X and up */
#  if defined(WIN32) && defined(_WIN32_WCE)
#     if !defined(__JSE_WINCE__)
#        define __JSE_WINCE__
#     endif
#     if !defined(__JSE_WIN32__)
#        define __JSE_WIN32__
#     endif
#  elif defined(WIN32) && defined(_CONSOLE)
#     if !defined(__JSE_CON32__)
#        define __JSE_CON32__
#     endif
#  else
#     if !defined(__JSE_WIN32__)
#        define __JSE_WIN32__
#     endif
#  endif
#elif defined(__BORLANDC__) && __BORLANDC__ >= 0x450
   /* Borland 4.5 and up */
#  if defined(_Windows) && defined(__DPMI16__) /* DOS w/ 16-bit PowerPack */
#    define __JSE_DOS16__
#  elif defined(__WIN32__) && defined(__DPMI32__)/* DOS w/ 32-bit PowerPack */
#    define __JSE_DOS32__
#  elif defined(__MSDOS__) && !defined(_Windows) /* real mode DOS */
#    define __JSE_DOS16__
#  elif defined(__WIN32__) /* 32-bit Windows (95/NT) */
#    define __JSE_WIN32__
#  elif defined(_Windows) && !defined(__WIN32__) /* 16-bit Windows (3.11) */
#    define __JSE_WIN16__
#  else
#     error Unrecognized Borland target platform
#  endif
#endif
#endif

   /* Metrowerks Codewarrior */
#if defined(__MWERKS__)
#  if defined(macintosh)
#     define __JSE_MAC__
#  endif
#endif

   /* To combat empty files on really ANSI-C compilers */
#if defined(__MWERKS__)
#  if __option(ANSI_strict)
#     define  ALLOW_EMPTY_FILE static ubyte DummyVariable;
#  endif
#endif

#if !defined(ALLOW_EMPTY_FILE)
#  define ALLOW_EMPTY_FILE   /* this compiler allows "empty" files */
#endif

