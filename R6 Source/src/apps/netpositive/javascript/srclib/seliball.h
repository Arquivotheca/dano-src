/* seliball.h
 *
 * This header file will include all the necessary headers depending on what
 * Libraries the user has defined.  This takes the place of including every
 * one separately, like #include "seclib.h" #include "seecma.h" etc.  It 
 * also defines the function LoadLibrary_All, which will initialize
 * all of the appropriate libraries depending on the settings.  It should be
 * included after selibdef.h
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

#ifndef __SELIBALL_H
#define __SELIBALL_H

/* Lang */
#ifdef JSE_LANG_ANY
#  if defined(__JSE_UNIX__)
#     include "lang/selang.h"
#  elif defined(__JSE_MAC__)
#     include "selang.h"
#  else
#     include "lang\selang.h"
#  endif
#endif
/* ECMA */
#ifdef JSE_ECMA_ANY
#  if defined(__JSE_UNIX__)
#     include "ecma/seecma.h"
#  elif defined(__JSE_MAC__)
#     include "seecma.h"
#  else
#     include "ecma\seecma.h"
#  endif
#endif
/* SElib */
#ifdef JSE_SELIB_ANY
#  if defined(__JSE_UNIX__)
#     include "selib/selib.h"
#  elif defined(__JSE_MAC__)
#     include "selib.h"
#  else
#     include "selib\selib.h"
#  endif
#endif
/* Clib */
#ifdef JSE_CLIB_ANY
#  if defined(__JSE_UNIX__)
#     include "clib/seclib.h"
#  elif defined(__JSE_MAC__)
#     include "seclib.h"
#  else
#     include "clib\seclib.h"
#  endif
#endif
/* Unix */
#ifdef JSE_UNIX_ANY
#  include "unix/seunix.h"
#endif
/* Win */
#ifdef JSE_WIN_ANY
#  include "win/sewin.h"
#endif
/* OS2 */
#ifdef JSE_OS2_ANY
#  include "os2\seos2.h"
#endif
/* Mac */
#ifdef JSE_MAC_ANY
#  include "semac.h"
#endif
/* Dos */
#ifdef JSE_DOS_ANY
#  include "dos\sedos.h"
#endif
/* NLM */
#ifdef __JSE_NWNLM__
#  include "nlm\senetwar.h"
#endif
/* Screen */
#ifdef JSE_SCREEN_ANY
#  include "sescreen.h"
#endif
/* Test */
#ifdef JSE_TEST_ANY
#  if defined(__JSE_UNIX__)
#     include "test/setest.h"
#  elif defined(__JSE_MAC__)
#     include "setest.h"
#  else
#     include "test\setest.h"
#  endif
#endif
/* MD5 */
#ifdef JSE_MD5_ALL
#  if defined(__JSE_UNIX__)
#     include "md5/semd5.h"
#  elif defined(__JSE_MAC__)
#     include "semd5.h"
#  else
#     include "md5\semd5.h"
#  endif
#endif
/* RegExp */
#ifdef JSE_REGEXP_ALL
#  if defined(__JSE_UNIX__)
#     include "regexp/seregexp.h"
#  elif defined(__JSE_MAC__)
#     include "seregexp.h"
#  else
#     include "regexp\seregexp.h"
#  endif
#endif
/* UUcode */
#ifdef JSE_UUCODE_ANY
#  if defined(__JSE_UNIX__)
#     include "uucode/seuucode.h"
#  elif defined(__JSE_MAC__)
#     include "seuucode.h"
#  else
#     include "uucode\seuucode.h"
#  endif
#endif

/* Common header for all libraries */
#if defined(__JSE_UNIX__)
#  include "common/selibcom.h"
#elif defined(__JSE_MAC__)
#  include "selibcom.h"
#else
#  include "common\selibcom.h"
#endif

#ifdef __cplusplus
   extern "C" {
#endif
jsebool LoadLibrary_All(jseContext jsecontext);
#ifdef __cplusplus
   }
#endif


#endif /* __SELIBALL_H */
