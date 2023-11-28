/* Automatically generated definition header file *//* !!! DO NOT EDIT !!! */

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

#ifndef __SELIBDEF_H
#define __SELIBDEF_H

/*****************
 * LANG          *
 ****************/

/* Check for JSE_LANG_ALL */
#if defined(JSE_LANG_ALL)
#  if !defined(JSE_LANG_DEFINED) 
#     define JSE_LANG_DEFINED                 1
#  endif
#  if !defined(JSE_LANG_GETARRAYLENGTH) 
#     define JSE_LANG_GETARRAYLENGTH          1
#  endif
#  if !defined(JSE_LANG_SETARRAYLENGTH) 
#     define JSE_LANG_SETARRAYLENGTH          1
#  endif
#  if !defined(JSE_LANG_TOBOOLEAN) 
#     define JSE_LANG_TOBOOLEAN               1
#  endif
#  if !defined(JSE_LANG_TOBUFFER) 
#     define JSE_LANG_TOBUFFER                1
#  endif
#  if !defined(JSE_LANG_TOBYTES) 
#     define JSE_LANG_TOBYTES                 1
#  endif
#  if !defined(JSE_LANG_TOINT32) 
#     define JSE_LANG_TOINT32                 1
#  endif
#  if !defined(JSE_LANG_TOINTEGER) 
#     define JSE_LANG_TOINTEGER               1
#  endif
#  if !defined(JSE_LANG_TONUMBER) 
#     define JSE_LANG_TONUMBER                1
#  endif
#  if !defined(JSE_LANG_TOOBJECT) 
#     define JSE_LANG_TOOBJECT                1
#  endif
#  if !defined(JSE_LANG_TOPRIMITIVE) 
#     define JSE_LANG_TOPRIMITIVE             1
#  endif
#  if !defined(JSE_LANG_TOSTRING) 
#     define JSE_LANG_TOSTRING                1
#  endif
#  if !defined(JSE_LANG_TOUINT16) 
#     define JSE_LANG_TOUINT16                1
#  endif
#  if !defined(JSE_LANG_TOUINT32) 
#     define JSE_LANG_TOUINT32                1
#  endif
#  if !defined(JSE_LANG_UNDEFINE) 
#     define JSE_LANG_UNDEFINE                1
#  endif
#  if !defined(JSE_LANG_GETATTRIBUTES) 
#     define JSE_LANG_GETATTRIBUTES           1
#  endif
#  if !defined(JSE_LANG_SETATTRIBUTES) 
#     define JSE_LANG_SETATTRIBUTES           1
#  endif
#endif /* JSE_LANG_ALL */

/* Convert zeros to undefines */
#if defined(JSE_LANG_DEFINED) && JSE_LANG_DEFINED == 0
#  undef JSE_LANG_DEFINED
#endif
#if defined(JSE_LANG_GETARRAYLENGTH) && JSE_LANG_GETARRAYLENGTH == 0
#  undef JSE_LANG_GETARRAYLENGTH
#endif
#if defined(JSE_LANG_SETARRAYLENGTH) && JSE_LANG_SETARRAYLENGTH == 0
#  undef JSE_LANG_SETARRAYLENGTH
#endif
#if defined(JSE_LANG_TOBOOLEAN) && JSE_LANG_TOBOOLEAN == 0
#  undef JSE_LANG_TOBOOLEAN
#endif
#if defined(JSE_LANG_TOBUFFER) && JSE_LANG_TOBUFFER == 0
#  undef JSE_LANG_TOBUFFER
#endif
#if defined(JSE_LANG_TOBYTES) && JSE_LANG_TOBYTES == 0
#  undef JSE_LANG_TOBYTES
#endif
#if defined(JSE_LANG_TOINT32) && JSE_LANG_TOINT32 == 0
#  undef JSE_LANG_TOINT32
#endif
#if defined(JSE_LANG_TOINTEGER) && JSE_LANG_TOINTEGER == 0
#  undef JSE_LANG_TOINTEGER
#endif
#if defined(JSE_LANG_TONUMBER) && JSE_LANG_TONUMBER == 0
#  undef JSE_LANG_TONUMBER
#endif
#if defined(JSE_LANG_TOOBJECT) && JSE_LANG_TOOBJECT == 0
#  undef JSE_LANG_TOOBJECT
#endif
#if defined(JSE_LANG_TOPRIMITIVE) && JSE_LANG_TOPRIMITIVE == 0
#  undef JSE_LANG_TOPRIMITIVE
#endif
#if defined(JSE_LANG_TOSTRING) && JSE_LANG_TOSTRING == 0
#  undef JSE_LANG_TOSTRING
#endif
#if defined(JSE_LANG_TOUINT16) && JSE_LANG_TOUINT16 == 0
#  undef JSE_LANG_TOUINT16
#endif
#if defined(JSE_LANG_TOUINT32) && JSE_LANG_TOUINT32 == 0
#  undef JSE_LANG_TOUINT32
#endif
#if defined(JSE_LANG_UNDEFINE) && JSE_LANG_UNDEFINE == 0
#  undef JSE_LANG_UNDEFINE
#endif
#if defined(JSE_LANG_GETATTRIBUTES) && JSE_LANG_GETATTRIBUTES == 0
#  undef JSE_LANG_GETATTRIBUTES
#endif
#if defined(JSE_LANG_SETATTRIBUTES) && JSE_LANG_SETATTRIBUTES == 0
#  undef JSE_LANG_SETATTRIBUTES
#endif
/* Define generic JSE_LANG_ANY */
#if defined(JSE_LANG_DEFINED) \
 || defined(JSE_LANG_GETARRAYLENGTH) \
 || defined(JSE_LANG_SETARRAYLENGTH) \
 || defined(JSE_LANG_TOBOOLEAN) \
 || defined(JSE_LANG_TOBUFFER) \
 || defined(JSE_LANG_TOBYTES) \
 || defined(JSE_LANG_TOINT32) \
 || defined(JSE_LANG_TOINTEGER) \
 || defined(JSE_LANG_TONUMBER) \
 || defined(JSE_LANG_TOOBJECT) \
 || defined(JSE_LANG_TOPRIMITIVE) \
 || defined(JSE_LANG_TOSTRING) \
 || defined(JSE_LANG_TOUINT16) \
 || defined(JSE_LANG_TOUINT32) \
 || defined(JSE_LANG_UNDEFINE) \
 || defined(JSE_LANG_GETATTRIBUTES) \
 || defined(JSE_LANG_SETATTRIBUTES)
#  define JSE_LANG_ANY
#endif

/*****************
 * ECMA          *
 ****************/

/* Check for JSE_ECMA_ALL */
#if defined(JSE_ECMA_ALL)
#  if !defined(JSE_ECMA_ARRAY) 
#     define JSE_ECMA_ARRAY                   1
#  endif
#  if !defined(JSE_ECMA_BOOLEAN) 
#     define JSE_ECMA_BOOLEAN                 1
#  endif
#  if !defined(JSE_ECMA_BUFFER) \
   && defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER) 
#     define JSE_ECMA_BUFFER                  1
#  endif
#  if !defined(JSE_ECMA_DATE) 
#     define JSE_ECMA_DATE                    1
#  endif
#  if !defined(JSE_ECMA_ESCAPE) 
#     define JSE_ECMA_ESCAPE                  1
#  endif
#  if !defined(JSE_ECMA_EVAL) 
#     define JSE_ECMA_EVAL                    1
#  endif
#  if !defined(JSE_ECMA_FUNCTION) 
#     define JSE_ECMA_FUNCTION                1
#  endif
#  if !defined(JSE_ECMA_ISFINITE) 
#     define JSE_ECMA_ISFINITE                1
#  endif
#  if !defined(JSE_ECMA_ISNAN) 
#     define JSE_ECMA_ISNAN                   1
#  endif
#  if !defined(JSE_ECMA_MATH) 
#     define JSE_ECMA_MATH                    1
#  endif
#  if !defined(JSE_ECMA_NUMBER) 
#     define JSE_ECMA_NUMBER                  1
#  endif
#  if !defined(JSE_ECMA_OBJECT) 
#     define JSE_ECMA_OBJECT                  1
#  endif
#  if !defined(JSE_ECMA_PARSEINT) 
#     define JSE_ECMA_PARSEINT                1
#  endif
#  if !defined(JSE_ECMA_PARSEFLOAT) 
#     define JSE_ECMA_PARSEFLOAT              1
#  endif
#  if !defined(JSE_ECMA_STRING) 
#     define JSE_ECMA_STRING                  1
#  endif
#  if !defined(JSE_ECMA_UNESCAPE) 
#     define JSE_ECMA_UNESCAPE                1
#  endif
#endif /* JSE_ECMA_ALL */

/* Convert zeros to undefines */
#if defined(JSE_ECMA_ARRAY) && JSE_ECMA_ARRAY == 0
#  undef JSE_ECMA_ARRAY
#endif
#if defined(JSE_ECMA_BOOLEAN) && JSE_ECMA_BOOLEAN == 0
#  undef JSE_ECMA_BOOLEAN
#endif
#if defined(JSE_ECMA_BUFFER) && JSE_ECMA_BUFFER == 0
#  undef JSE_ECMA_BUFFER
#endif
#if defined(JSE_ECMA_DATE) && JSE_ECMA_DATE == 0
#  undef JSE_ECMA_DATE
#endif
#if defined(JSE_ECMA_ESCAPE) && JSE_ECMA_ESCAPE == 0
#  undef JSE_ECMA_ESCAPE
#endif
#if defined(JSE_ECMA_EVAL) && JSE_ECMA_EVAL == 0
#  undef JSE_ECMA_EVAL
#endif
#if defined(JSE_ECMA_FUNCTION) && JSE_ECMA_FUNCTION == 0
#  undef JSE_ECMA_FUNCTION
#endif
#if defined(JSE_ECMA_ISFINITE) && JSE_ECMA_ISFINITE == 0
#  undef JSE_ECMA_ISFINITE
#endif
#if defined(JSE_ECMA_ISNAN) && JSE_ECMA_ISNAN == 0
#  undef JSE_ECMA_ISNAN
#endif
#if defined(JSE_ECMA_MATH) && JSE_ECMA_MATH == 0
#  undef JSE_ECMA_MATH
#endif
#if defined(JSE_ECMA_NUMBER) && JSE_ECMA_NUMBER == 0
#  undef JSE_ECMA_NUMBER
#endif
#if defined(JSE_ECMA_OBJECT) && JSE_ECMA_OBJECT == 0
#  undef JSE_ECMA_OBJECT
#endif
#if defined(JSE_ECMA_PARSEINT) && JSE_ECMA_PARSEINT == 0
#  undef JSE_ECMA_PARSEINT
#endif
#if defined(JSE_ECMA_PARSEFLOAT) && JSE_ECMA_PARSEFLOAT == 0
#  undef JSE_ECMA_PARSEFLOAT
#endif
#if defined(JSE_ECMA_STRING) && JSE_ECMA_STRING == 0
#  undef JSE_ECMA_STRING
#endif
#if defined(JSE_ECMA_UNESCAPE) && JSE_ECMA_UNESCAPE == 0
#  undef JSE_ECMA_UNESCAPE
#endif
/* Define generic JSE_ECMA_ANY */
#if defined(JSE_ECMA_ARRAY) \
 || defined(JSE_ECMA_BOOLEAN) \
 || defined(JSE_ECMA_BUFFER) \
 || defined(JSE_ECMA_DATE) \
 || defined(JSE_ECMA_ESCAPE) \
 || defined(JSE_ECMA_EVAL) \
 || defined(JSE_ECMA_FUNCTION) \
 || defined(JSE_ECMA_ISFINITE) \
 || defined(JSE_ECMA_ISNAN) \
 || defined(JSE_ECMA_MATH) \
 || defined(JSE_ECMA_NUMBER) \
 || defined(JSE_ECMA_OBJECT) \
 || defined(JSE_ECMA_PARSEINT) \
 || defined(JSE_ECMA_PARSEFLOAT) \
 || defined(JSE_ECMA_STRING) \
 || defined(JSE_ECMA_UNESCAPE)
#  define JSE_ECMA_ANY
#endif

/*****************
 * DATE - EXTRA  *
 ****************/
#if defined(JSE_ECMA_DATE)
   /* some date function are added by Nombas */
#  if !defined(JSE_ECMA_DATE_FROMSYSTEM)
#     define JSE_ECMA_DATE_FROMSYSTEM 1
#  endif
#  if !defined(JSE_ECMA_DATE_TOSYSTEM)
#     define JSE_ECMA_DATE_TOSYSTEM 1
#  endif
#else
#  define JSE_ECMA_DATE_FROMSYSTEM 0
#  define JSE_ECMA_DATE_TOSYSTEM 0
#endif
#if defined(JSE_ECMA_DATE_FROMSYSTEM) && JSE_ECMA_DATE_FROMSYSTEM == 0
#  undef JSE_ECMA_DATE_FROMSYSTEM
#endif
#if defined(JSE_ECMA_DATE_TOSYSTEM) && JSE_ECMA_DATE_TOSYSTEM == 0
#  undef JSE_ECMA_DATE_TOSYSTEM
#endif


/*****************
 * SELIB         *
 ****************/

/* Check for JSE_SELIB_ALL */
#if defined(JSE_SELIB_ALL)
#  if !defined(JSE_SELIB_BLOB_GET) 
#     define JSE_SELIB_BLOB_GET               1
#  endif
#  if !defined(JSE_SELIB_BLOB_PUT) 
#     define JSE_SELIB_BLOB_PUT               1
#  endif
#  if !defined(JSE_SELIB_BLOB_SIZE) 
#     define JSE_SELIB_BLOB_SIZE              1
#  endif
#  if !defined(JSE_SELIB_BOUND) \
   && defined(JSE_BINDABLE) && (0!=JSE_BINDABLE) 
#     define JSE_SELIB_BOUND                  1
#  endif
#  if !defined(JSE_SELIB_COMPILESCRIPT) 
#     define JSE_SELIB_COMPILESCRIPT          1
#  endif
#  if !defined(JSE_SELIB_CREATECALLBACK) \
   && (defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)) 
#     define JSE_SELIB_CREATECALLBACK         0 /* these need a lot more testing */
#  endif
#  if !defined(JSE_SELIB_DESTROYCALLBACK) \
   && (defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)) 
#     define JSE_SELIB_DESTROYCALLBACK        0 /* these need a lot more testing */
#  endif
#  if !defined(JSE_SELIB_DIRECTORY) 
#     define JSE_SELIB_DIRECTORY              1
#  endif
#  if !defined(JSE_SELIB_DYNAMICLINK) \
   && !defined(__JSE_WINCE__) && !defined(__JSE_NWNLM__) \
   && !defined(__JSE_DOS16__) && !defined(__JSE_DOS32__)
#     define JSE_SELIB_DYNAMICLINK            1
#  endif
#  if !defined(JSE_SELIB_FULLPATH) 
#     define JSE_SELIB_FULLPATH               1
#  endif
#  if !defined(JSE_SELIB_GETOBJECTPROPERTIES) 
#     define JSE_SELIB_GETOBJECTPROPERTIES    1
#  endif
#  if !defined(JSE_SELIB_INSECURITY) \
   && defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE) 
#     define JSE_SELIB_INSECURITY             1
#  endif
#  if !defined(JSE_SELIB_INTERPRET) 
#     define JSE_SELIB_INTERPRET              1
#  endif
#  if !defined(JSE_SELIB_INTERPRETINNEWTHREAD) \
   && ( defined(__JSE_WIN32__) || defined(__JSE_CON32__) || \
        defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) || defined(__JSE_UNIX__) || \
        defined(__JSE_UNIX__) || defined(__JSE_MAC__) ) 
#     define JSE_SELIB_INTERPRETINNEWTHREAD   1
#  endif
#  if !defined(JSE_SELIB_MEMDEBUG) && defined(JSE_MEM_DEBUG) 
#     define JSE_SELIB_MEMDEBUG               1
#  endif
#  if !defined(JSE_SELIB_MEMVERBOSE) && defined(JSE_MEM_DEBUG) 
#  define JSE_SELIB_MEMVERBOSE             1
#  endif
#  if !defined(JSE_SELIB_PEEK) 
#     define JSE_SELIB_PEEK                   1
#  endif
#  if !defined(JSE_SELIB_POINTER) 
#     define JSE_SELIB_POINTER                1
#  endif
#  if !defined(JSE_SELIB_POKE) 
#     define JSE_SELIB_POKE                   1
#  endif
#  if !defined(JSE_SELIB_SPAWN) 
#     define JSE_SELIB_SPAWN                  1
#  endif
#  if !defined(JSE_SELIB_SPLITFILENAME) 
#     define JSE_SELIB_SPLITFILENAME          1
#  endif
#  if !defined(JSE_SELIB_SUSPEND) 
#     define JSE_SELIB_SUSPEND                1
#  endif
#endif /* JSE_SELIB_ALL */

/* Convert zeros to undefines */
#if defined(JSE_SELIB_BLOB_GET) && JSE_SELIB_BLOB_GET == 0
#  undef JSE_SELIB_BLOB_GET
#endif
#if defined(JSE_SELIB_BLOB_PUT) && JSE_SELIB_BLOB_PUT == 0
#  undef JSE_SELIB_BLOB_PUT
#endif
#if defined(JSE_SELIB_BLOB_SIZE) && JSE_SELIB_BLOB_SIZE == 0
#  undef JSE_SELIB_BLOB_SIZE
#endif
#if defined(JSE_SELIB_BOUND) && JSE_SELIB_BOUND == 0
#  undef JSE_SELIB_BOUND
#endif
#if defined(JSE_SELIB_COMPILESCRIPT) && JSE_SELIB_COMPILESCRIPT == 0
#  undef JSE_SELIB_COMPILESCRIPT
#endif
#if defined(JSE_SELIB_CREATECALLBACK) && JSE_SELIB_CREATECALLBACK == 0
#  undef JSE_SELIB_CREATECALLBACK
#endif
#if defined(JSE_SELIB_DESTROYCALLBACK) && JSE_SELIB_DESTROYCALLBACK == 0
#  undef JSE_SELIB_DESTROYCALLBACK
#endif
#if defined(JSE_SELIB_DIRECTORY) && JSE_SELIB_DIRECTORY == 0
#  undef JSE_SELIB_DIRECTORY
#endif
#if defined(JSE_SELIB_DYNAMICLINK) && JSE_SELIB_DYNAMICLINK == 0
#  undef JSE_SELIB_DYNAMICLINK
#endif
#if defined(JSE_SELIB_FULLPATH) && JSE_SELIB_FULLPATH == 0
#  undef JSE_SELIB_FULLPATH
#endif
#if defined(JSE_SELIB_GETOBJECTPROPERTIES) && JSE_SELIB_GETOBJECTPROPERTIES == 0
#  undef JSE_SELIB_GETOBJECTPROPERTIES
#endif
#if defined(JSE_SELIB_INSECURITY) && JSE_SELIB_INSECURITY == 0
#  undef JSE_SELIB_INSECURITY
#endif
#if defined(JSE_SELIB_INTERPRET) && JSE_SELIB_INTERPRET == 0
#  undef JSE_SELIB_INTERPRET
#endif
#if defined(JSE_SELIB_INTERPRETINNEWTHREAD) && JSE_SELIB_INTERPRETINNEWTHREAD == 0
#  undef JSE_SELIB_INTERPRETINNEWTHREAD
#endif
#if defined(JSE_SELIB_MEMDEBUG) && JSE_SELIB_MEMDEBUG == 0
#  undef JSE_SELIB_MEMDEBUG
#endif
#if defined(JSE_SELIB_MEMVERBOSE) && JSE_SELIB_MEMVERBOSE == 0
#  undef JSE_SELIB_MEMVERBOSE
#endif
#if defined(JSE_SELIB_PEEK) && JSE_SELIB_PEEK == 0
#  undef JSE_SELIB_PEEK
#endif
#if defined(JSE_SELIB_POINTER) && JSE_SELIB_POINTER == 0
#  undef JSE_SELIB_POINTER
#endif
#if defined(JSE_SELIB_POKE) && JSE_SELIB_POKE == 0
#  undef JSE_SELIB_POKE
#endif
#if defined(JSE_SELIB_SPAWN) && JSE_SELIB_SPAWN == 0
#  undef JSE_SELIB_SPAWN
#endif
#if defined(JSE_SELIB_SPLITFILENAME) && JSE_SELIB_SPLITFILENAME == 0
#  undef JSE_SELIB_SPLITFILENAME
#endif
#if defined(JSE_SELIB_SUSPEND) && JSE_SELIB_SUSPEND == 0
#  undef JSE_SELIB_SUSPEND
#endif
/* Define generic JSE_SELIB_ANY */
#if defined(JSE_SELIB_BLOB_GET) \
 || defined(JSE_SELIB_BLOB_PUT) \
 || defined(JSE_SELIB_BLOB_SIZE) \
 || defined(JSE_SELIB_BOUND) \
 || defined(JSE_SELIB_COMPILESCRIPT) \
 || defined(JSE_SELIB_CREATECALLBACK) \
 || defined(JSE_SELIB_DESTROYCALLBACK) \
 || defined(JSE_SELIB_DIRECTORY) \
 || defined(JSE_SELIB_DYNAMICLINK) \
 || defined(JSE_SELIB_FULLPATH) \
 || defined(JSE_SELIB_GETOBJECTPROPERTIES) \
 || defined(JSE_SELIB_INSECURITY) \
 || defined(JSE_SELIB_INTERPRET) \
 || defined(JSE_SELIB_INTERPRETINNEWTHREAD) \
 || defined(JSE_SELIB_MEMDEBUG) \
 || defined(JSE_SELIB_MEMVERBOSE) \
 || defined(JSE_SELIB_PEEK) \
 || defined(JSE_SELIB_POINTER) \
 || defined(JSE_SELIB_POKE) \
 || defined(JSE_SELIB_SPAWN) \
 || defined(JSE_SELIB_SPLITFILENAME) \
 || defined(JSE_SELIB_SUSPEND)
#  define JSE_SELIB_ANY
#endif
/*****************
 * SCREEN        *
 ****************/

/* Check for JSE_SCREEN_ALL */
#if defined(JSE_SCREEN_ALL)
#  if !defined(JSE_SCREEN_CLEAR) 
#     define JSE_SCREEN_CLEAR                 1
#  endif
#  if !defined(JSE_SCREEN_CURSOR) 
#     define JSE_SCREEN_CURSOR                1
#  endif
#  if !defined(JSE_SCREEN_HANDLE) \
   && (defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)) 
#     define JSE_SCREEN_HANDLE                1
#  endif
#  if !defined(JSE_SCREEN_SETBACKGROUND) \
   && defined(__CENVI__) && (defined(__JSE_WIN16__) || defined(__JSE_WIN32__)) 
#     define JSE_SCREEN_SETBACKGROUND         1
#  endif
#  if !defined(JSE_SCREEN_SETFOREGROUND) \
   && defined(__CENVI__) && (defined(__JSE_WIN16__) || defined(__JSE_WIN32__)) 
#     define JSE_SCREEN_SETFOREGROUND         1
#  endif
#  if !defined(JSE_SCREEN_SIZE) 
#     define JSE_SCREEN_SIZE                  1
#  endif
#  if !defined(JSE_SCREEN_WRITE) 
#     define JSE_SCREEN_WRITE                 1
#  endif
#  if !defined(JSE_SCREEN_WRITELN) 
#     define JSE_SCREEN_WRITELN               1
#  endif
#endif /* JSE_SCREEN_ALL */

/* Convert zeros to undefines */
#if defined(JSE_SCREEN_CLEAR) && JSE_SCREEN_CLEAR == 0
#  undef JSE_SCREEN_CLEAR
#endif
#if defined(JSE_SCREEN_CURSOR) && JSE_SCREEN_CURSOR == 0
#  undef JSE_SCREEN_CURSOR
#endif
#if defined(JSE_SCREEN_HANDLE) && JSE_SCREEN_HANDLE == 0
#  undef JSE_SCREEN_HANDLE
#endif
#if defined(JSE_SCREEN_SETBACKGROUND) && JSE_SCREEN_SETBACKGROUND == 0
#  undef JSE_SCREEN_SETBACKGROUND
#endif
#if defined(JSE_SCREEN_SETFOREGROUND) && JSE_SCREEN_SETFOREGROUND == 0
#  undef JSE_SCREEN_SETFOREGROUND
#endif
#if defined(JSE_SCREEN_SIZE) && JSE_SCREEN_SIZE == 0
#  undef JSE_SCREEN_SIZE
#endif
#if defined(JSE_SCREEN_WRITE) && JSE_SCREEN_WRITE == 0
#  undef JSE_SCREEN_WRITE
#endif
#if defined(JSE_SCREEN_WRITELN) && JSE_SCREEN_WRITELN == 0
#  undef JSE_SCREEN_WRITELN
#endif
/* Define generic JSE_SCREEN_ANY */
#if defined(JSE_SCREEN_CLEAR) \
 || defined(JSE_SCREEN_CURSOR) \
 || defined(JSE_SCREEN_HANDLE) \
 || defined(JSE_SCREEN_SETBACKGROUND) \
 || defined(JSE_SCREEN_SETFOREGROUND) \
 || defined(JSE_SCREEN_SIZE) \
 || defined(JSE_SCREEN_WRITE) \
 || defined(JSE_SCREEN_WRITELN)
#  define JSE_SCREEN_ANY
#endif
/*****************
 * UNIX          *
 ****************/

/* Check for JSE_UNIX_ALL */
#if defined(JSE_UNIX_ALL) && defined(__JSE_UNIX__)
#  if !defined(JSE_UNIX_FORK) 
#     define JSE_UNIX_FORK                    1
#  endif
#  if !defined(JSE_UNIX_KILL) 
#     define JSE_UNIX_KILL                    1
#  endif
#  if !defined(JSE_UNIX_SETGID) 
#     define JSE_UNIX_SETGID                  1
#  endif
#  if !defined(JSE_UNIX_SETSID) 
#     define JSE_UNIX_SETSID                  1
#  endif
#  if !defined(JSE_UNIX_SETUID) 
#     define JSE_UNIX_SETUID                  1
#  endif
#  if !defined(JSE_UNIX_WAIT) 
#     define JSE_UNIX_WAIT                    1
#  endif
#  if !defined(JSE_UNIX_WAITPID) 
#     define JSE_UNIX_WAITPID                 1
#  endif
#endif /* JSE_UNIX_ALL */

/* Convert zeros to undefines */
#if defined(JSE_UNIX_FORK) && JSE_UNIX_FORK == 0
#  undef JSE_UNIX_FORK
#endif
#if defined(JSE_UNIX_KILL) && JSE_UNIX_KILL == 0
#  undef JSE_UNIX_KILL
#endif
#if defined(JSE_UNIX_SETGID) && JSE_UNIX_SETGID == 0
#  undef JSE_UNIX_SETGID
#endif
#if defined(JSE_UNIX_SETSID) && JSE_UNIX_SETSID == 0
#  undef JSE_UNIX_SETSID
#endif
#if defined(JSE_UNIX_SETUID) && JSE_UNIX_SETUID == 0
#  undef JSE_UNIX_SETUID
#endif
#if defined(JSE_UNIX_WAIT) && JSE_UNIX_WAIT == 0
#  undef JSE_UNIX_WAIT
#endif
#if defined(JSE_UNIX_WAITPID) && JSE_UNIX_WAITPID == 0
#  undef JSE_UNIX_WAITPID
#endif
/* Define generic JSE_UNIX_ANY */
#if defined(JSE_UNIX_FORK) \
 || defined(JSE_UNIX_KILL) \
 || defined(JSE_UNIX_SETGID) \
 || defined(JSE_UNIX_SETSID) \
 || defined(JSE_UNIX_SETUID) \
 || defined(JSE_UNIX_WAIT) \
 || defined(JSE_UNIX_WAITPID)
#  define JSE_UNIX_ANY
#endif

/* Check for error condition */
#if defined(JSE_UNIX) && !(defined(__JSE_UNIX__))
#  error Current options not compatible with library UNIX
#endif

/*****************
 * DOS           *
 ****************/

/* Check for JSE_DOS_ALL */
#if defined(JSE_DOS_ALL) \
 && !defined(__WILLOWS__) \
 && (defined(__JSE_DOS16__) || defined(__JSE_DOS32__) || defined(__JSE_WIN16__))
#  if !defined(JSE_DOS_ADDRESS) 
#     define JSE_DOS_ADDRESS                  1
#  endif
#  if !defined(JSE_DOS_ASM) 
#     define JSE_DOS_ASM                      1
#  endif
#  if !defined(JSE_DOS_INPORT) 
#     define JSE_DOS_INPORT                   1
#  endif
#  if !defined(JSE_DOS_INPORTW) 
#     define JSE_DOS_INPORTW                  1
#  endif
#  if !defined(JSE_DOS_INTERRUPT) 
#     define JSE_DOS_INTERRUPT                1
#  endif
#  if !defined(JSE_DOS_OFFSET) 
#     define JSE_DOS_OFFSET                   1
#  endif
#  if !defined(JSE_DOS_OUTPORT) 
#     define JSE_DOS_OUTPORT                  1
#  endif
#  if !defined(JSE_DOS_OUTPORTW) 
#     define JSE_DOS_OUTPORTW                 1
#  endif
#  if !defined(JSE_DOS_SEGMENT) 
#     define JSE_DOS_SEGMENT                  1
#  endif
#endif /* JSE_DOS_ALL */

/* Convert zeros to undefines */
#if defined(JSE_DOS_ADDRESS) && JSE_DOS_ADDRESS == 0
#  undef JSE_DOS_ADDRESS
#endif
#if defined(JSE_DOS_ASM) && JSE_DOS_ASM == 0
#  undef JSE_DOS_ASM
#endif
#if defined(JSE_DOS_INPORT) && JSE_DOS_INPORT == 0
#  undef JSE_DOS_INPORT
#endif
#if defined(JSE_DOS_INPORTW) && JSE_DOS_INPORTW == 0
#  undef JSE_DOS_INPORTW
#endif
#if defined(JSE_DOS_INTERRUPT) && JSE_DOS_INTERRUPT == 0
#  undef JSE_DOS_INTERRUPT
#endif
#if defined(JSE_DOS_OFFSET) && JSE_DOS_OFFSET == 0
#  undef JSE_DOS_OFFSET
#endif
#if defined(JSE_DOS_OUTPORT) && JSE_DOS_OUTPORT == 0
#  undef JSE_DOS_OUTPORT
#endif
#if defined(JSE_DOS_OUTPORTW) && JSE_DOS_OUTPORTW == 0
#  undef JSE_DOS_OUTPORTW
#endif
#if defined(JSE_DOS_SEGMENT) && JSE_DOS_SEGMENT == 0
#  undef JSE_DOS_SEGMENT
#endif
/* Define generic JSE_DOS_ANY */
#if defined(JSE_DOS_ADDRESS) \
 || defined(JSE_DOS_ASM) \
 || defined(JSE_DOS_INPORT) \
 || defined(JSE_DOS_INPORTW) \
 || defined(JSE_DOS_INTERRUPT) \
 || defined(JSE_DOS_OFFSET) \
 || defined(JSE_DOS_OUTPORT) \
 || defined(JSE_DOS_OUTPORTW) \
 || defined(JSE_DOS_SEGMENT)
#  define JSE_DOS_ANY
#endif

/* Check for error condition */
#if defined(JSE_DOS) && !(!defined(__WILLOWS__) \
 && (defined(__JSE_DOS16__) || defined(__JSE_DOS32__) || defined(__JSE_WIN16__)))
#  error Current options not compatible with library DOS
#endif

/*****************
 * WIN           *
 ****************/

/* Check for JSE_WIN_ALL */
#if defined(JSE_WIN_ALL) \
 && (defined(__JSE_WIN16__) || defined(__JSE_WIN32__))
#  if !defined(JSE_WIN_ASM) 
#     define JSE_WIN_ASM                      1
#  endif
#  if !defined(JSE_WIN_BASEWINDOWFUNCTION) 
#     define JSE_WIN_BASEWINDOWFUNCTION       1
#  endif
#  if !defined(JSE_WIN_BREAKWINDOW) 
#     define JSE_WIN_BREAKWINDOW              1
#  endif
#  if !defined(JSE_WIN_DOWINDOWS) 
#     define JSE_WIN_DOWINDOWS                1
#  endif
#  if !defined(JSE_WIN_INSTANCE) 
#     define JSE_WIN_INSTANCE                 1
#  endif
#  if !defined(JSE_WIN_MAKEWINDOW) 
#     define JSE_WIN_MAKEWINDOW               1
#  endif
#  if !defined(JSE_WIN_MESSAGEFILTER) 
#     define JSE_WIN_MESSAGEFILTER            1
#  endif
#  if !defined(JSE_WIN_MULTITASK) 
#     define JSE_WIN_MULTITASK                1
#  endif
#  if !defined(JSE_WIN_SUBCLASSWINDOW) 
#     define JSE_WIN_SUBCLASSWINDOW           1
#  endif
#  if !defined(JSE_WIN_WINDOWLIST) 
#     define JSE_WIN_WINDOWLIST               1
#  endif
#endif /* JSE_WIN_ALL */

/* Convert zeros to undefines */
#if defined(JSE_WIN_ASM) && JSE_WIN_ASM == 0
#  undef JSE_WIN_ASM
#endif
#if defined(JSE_WIN_BASEWINDOWFUNCTION) && JSE_WIN_BASEWINDOWFUNCTION == 0
#  undef JSE_WIN_BASEWINDOWFUNCTION
#endif
#if defined(JSE_WIN_BREAKWINDOW) && JSE_WIN_BREAKWINDOW == 0
#  undef JSE_WIN_BREAKWINDOW
#endif
#if defined(JSE_WIN_DOWINDOWS) && JSE_WIN_DOWINDOWS == 0
#  undef JSE_WIN_DOWINDOWS
#endif
#if defined(JSE_WIN_INSTANCE) && JSE_WIN_INSTANCE == 0
#  undef JSE_WIN_INSTANCE
#endif
#if defined(JSE_WIN_MAKEWINDOW) && JSE_WIN_MAKEWINDOW == 0
#  undef JSE_WIN_MAKEWINDOW
#endif
#if defined(JSE_WIN_MESSAGEFILTER) && JSE_WIN_MESSAGEFILTER == 0
#  undef JSE_WIN_MESSAGEFILTER
#endif
#if defined(JSE_WIN_MULTITASK) && JSE_WIN_MULTITASK == 0
#  undef JSE_WIN_MULTITASK
#endif
#if defined(JSE_WIN_SUBCLASSWINDOW) && JSE_WIN_SUBCLASSWINDOW == 0
#  undef JSE_WIN_SUBCLASSWINDOW
#endif
#if defined(JSE_WIN_WINDOWLIST) && JSE_WIN_WINDOWLIST == 0
#  undef JSE_WIN_WINDOWLIST
#endif
/* Define generic JSE_WIN_ANY */
#if defined(JSE_WIN_ASM) \
 || defined(JSE_WIN_BASEWINDOWFUNCTION) \
 || defined(JSE_WIN_BREAKWINDOW) \
 || defined(JSE_WIN_DOWINDOWS) \
 || defined(JSE_WIN_INSTANCE) \
 || defined(JSE_WIN_MAKEWINDOW) \
 || defined(JSE_WIN_MESSAGEFILTER) \
 || defined(JSE_WIN_MULTITASK) \
 || defined(JSE_WIN_SUBCLASSWINDOW) \
 || defined(JSE_WIN_WINDOWLIST)
#  define JSE_WIN_ANY
#endif

/* Check for error condition */
#if defined(JSE_WIN) && !(defined(__JSE_WIN16__) || defined(__JSE_WIN32__))
#  error Current options not compatible with library WIN
#endif

/*****************
 * OS2           *
 ****************/

/* Check for JSE_OS2_ALL */
#if defined(JSE_OS2_ALL) \
 && (defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__))
#  if !defined(JSE_OS2_ASM) 
#     define JSE_OS2_ASM                      1
#  endif
#  if !defined(JSE_OS2_BEGINTHREAD) 
#     define JSE_OS2_BEGINTHREAD              0 /* NYI */
#  endif
#  if !defined(JSE_OS2_ENDTHREAD) 
#     define JSE_OS2_ENDTHREAD                0 /* NYI */
#  endif
#  if !defined(JSE_OS2_ESET) 
#     define JSE_OS2_ESET                     1
#  endif
#  if !defined(JSE_OS2_INFO) 
#     define JSE_OS2_INFO                     1
#  endif
#  if !defined(JSE_OS2_INPORT) 
#     define JSE_OS2_INPORT                   1
#  endif
#  if !defined(JSE_OS2_INPORTW) 
#     define JSE_OS2_INPORTW                  1
#  endif
#  if !defined(JSE_OS2_OUTPORT) 
#     define JSE_OS2_OUTPORT                  1
#  endif
#  if !defined(JSE_OS2_OUTPORTW) 
#     define JSE_OS2_OUTPORTW                 1
#  endif
#  if !defined(JSE_OS2_PMDYNAMICLINK) 
#     define JSE_OS2_PMDYNAMICLINK            1
#  endif
#  if !defined(JSE_OS2_PMINFO) 
#     define JSE_OS2_PMINFO                   1
#  endif
#  if !defined(JSE_OS2_PMPEEK) 
#     define JSE_OS2_PMPEEK                   1
#  endif
#  if !defined(JSE_OS2_PMPOKE) 
#     define JSE_OS2_PMPOKE                   1
#  endif
#  if !defined(JSE_OS2_PROCESSLIST) 
#     define JSE_OS2_PROCESSLIST              1
#  endif
#  if !defined(JSE_OS2_SOMMETHOD) 
#     define JSE_OS2_SOMMETHOD                1
#  endif
#endif /* JSE_OS2_ALL */
/* Convert zeros to undefines */
#if defined(JSE_OS2_ASM) && JSE_OS2_ASM == 0
#  undef JSE_OS2_ASM
#endif
#if defined(JSE_OS2_BEGINTHREAD) && JSE_OS2_BEGINTHREAD == 0
#  undef JSE_OS2_BEGINTHREAD
#endif
#if defined(JSE_OS2_ENDTHREAD) && JSE_OS2_ENDTHREAD == 0
#  undef JSE_OS2_ENDTHREAD
#endif
#if defined(JSE_OS2_ESET) && JSE_OS2_ESET == 0
#  undef JSE_OS2_ESET
#endif
#if defined(JSE_OS2_INFO) && JSE_OS2_INFO == 0
#  undef JSE_OS2_INFO
#endif
#if defined(JSE_OS2_INPORT) && JSE_OS2_INPORT == 0
#  undef JSE_OS2_INPORT
#endif
#if defined(JSE_OS2_INPORTW) && JSE_OS2_INPORTW == 0
#  undef JSE_OS2_INPORTW
#endif
#if defined(JSE_OS2_OUTPORT) && JSE_OS2_OUTPORT == 0
#  undef JSE_OS2_OUTPORT
#endif
#if defined(JSE_OS2_OUTPORTW) && JSE_OS2_OUTPORTW == 0
#  undef JSE_OS2_OUTPORTW
#endif
#if defined(JSE_OS2_PMDYNAMICLINK) && JSE_OS2_PMDYNAMICLINK == 0
#  undef JSE_OS2_PMDYNAMICLINK
#endif
#if defined(JSE_OS2_PMINFO) && JSE_OS2_PMINFO == 0
#  undef JSE_OS2_PMINFO
#endif
#if defined(JSE_OS2_PMPEEK) && JSE_OS2_PMPEEK == 0
#  undef JSE_OS2_PMPEEK
#endif
#if defined(JSE_OS2_PMPOKE) && JSE_OS2_PMPOKE == 0
#  undef JSE_OS2_PMPOKE
#endif
#if defined(JSE_OS2_PROCESSLIST) && JSE_OS2_PROCESSLIST == 0
#  undef JSE_OS2_PROCESSLIST
#endif
#if defined(JSE_OS2_SOMMETHOD) && JSE_OS2_SOMMETHOD == 0
#  undef JSE_OS2_SOMMETHOD
#endif
/* Define generic JSE_OS2_ANY */
#if defined(JSE_OS2_ASM) \
 || defined(JSE_OS2_BEGINTHREAD) \
 || defined(JSE_OS2_ENDTHREAD) \
 || defined(JSE_OS2_ESET) \
 || defined(JSE_OS2_INFO) \
 || defined(JSE_OS2_INPORT) \
 || defined(JSE_OS2_INPORTW) \
 || defined(JSE_OS2_OUTPORT) \
 || defined(JSE_OS2_OUTPORTW) \
 || defined(JSE_OS2_PMDYNAMICLINK) \
 || defined(JSE_OS2_PMINFO) \
 || defined(JSE_OS2_PMPEEK) \
 || defined(JSE_OS2_PMPOKE) \
 || defined(JSE_OS2_PROCESSLIST) \
 || defined(JSE_OS2_SOMMETHOD)
#  define JSE_OS2_ANY
#endif

/* Check for error condition */
#if defined(JSE_OS2) && !(defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__))
#  error Current options not compatible with library OS2
#endif

/*****************
 * MAC           *
 ****************/

/* Check for JSE_MAC_ALL */
#if defined(JSE_MAC_ALL) && defined(__JSE_MAC__)
#  if !defined(JSE_MAC_ADDCOMMANDHOOK) && defined(__CENVI__) 
#     define JSE_MAC_ADDCOMMANDHOOK           1
#  endif
#  if !defined(JSE_MAC_ADDEVENTHANDLER) && defined(__CENVI__) 
#     define JSE_MAC_ADDEVENTHANDLER          1
#  endif
#  if !defined(JSE_MAC_MULTITHREAD) && defined(USE_MAC_THREADS) 
#     define JSE_MAC_MULTITHREAD              1
#  endif
#  if !defined(JSE_MAC_PROCESSEVENT) && defined(__CENVI__) 
#     define JSE_MAC_PROCESSEVENT             1
#  endif
#  if !defined(JSE_MAC_REMOVECOMMANDHOOK) && defined(__CENVI__) 
#     define JSE_MAC_REMOVECOMMANDHOOK        1
#  endif
#  if !defined(JSE_MAC_REMOVEEVENTHANDLER) && defined(__CENVI__) 
#     define JSE_MAC_REMOVEEVENTHANDLER       1
#  endif
#  if !defined(JSE_MAC_RUNAPPLESCRIPT)
#     define JSE_MAC_RUNAPPLESCRIPT           1
#  endif
#endif /* JSE_MAC_ALL */
/* Convert zeros to undefines */
#if defined(JSE_MAC_ADDCOMMANDHOOK) && JSE_MAC_ADDCOMMANDHOOK == 0
#  undef JSE_MAC_ADDCOMMANDHOOK
#endif
#if defined(JSE_MAC_ADDEVENTHANDLER) && JSE_MAC_ADDEVENTHANDLER == 0
#  undef JSE_MAC_ADDEVENTHANDLER
#endif
#if defined(JSE_MAC_MULTITHREAD) && JSE_MAC_MULTITHREAD == 0
#  undef JSE_MAC_MULTITHREAD
#endif
#if defined(JSE_MAC_PROCESSEVENT) && JSE_MAC_PROCESSEVENT == 0
#  undef JSE_MAC_PROCESSEVENT
#endif
#if defined(JSE_MAC_REMOVECOMMANDHOOK) && JSE_MAC_REMOVECOMMANDHOOK == 0
#  undef JSE_MAC_REMOVECOMMANDHOOK
#endif
#if defined(JSE_MAC_REMOVEEVENTHANDLER) && JSE_MAC_REMOVEEVENTHANDLER == 0
#  undef JSE_MAC_REMOVEEVENTHANDLER
#endif
#if defined(JSE_MAC_RUNAPPLESCRIPT) && JSE_MAC_RUNAPPLESCRIPT == 0
#  undef JSE_MAC_RUNAPPLESCRIPT
#endif
/* Define generic JSE_MAC_ANY */
#if defined(JSE_MAC_ADDCOMMANDHOOK) \
 || defined(JSE_MAC_ADDEVENTHANDLER) \
 || defined(JSE_MAC_MULTITHREAD) \
 || defined(JSE_MAC_PROCESSEVENT) \
 || defined(JSE_MAC_REMOVECOMMANDHOOK) \
 || defined(JSE_MAC_REMOVEEVENTHANDLER) \
 || defined(JSE_MAC_RUNAPPLESCRIPT)
#  define JSE_MAC_ANY
#endif

/* Check for error condition */
#if defined(JSE_MAC) && !(defined(__JSE_MAC__))
#  error Current options not compatible with library MAC
#endif

/*****************
 * CLIB          *
 ****************/

/* Check for JSE_CLIB_ALL */
#if defined(JSE_CLIB_ALL)
#  if !defined(JSE_CLIB_ABORT) 
#     define JSE_CLIB_ABORT                   1
#  endif
#  if !defined(JSE_CLIB_ABS) 
#     define JSE_CLIB_ABS                     1
#  endif
#  if !defined(JSE_CLIB_ACOS) 
#     define JSE_CLIB_ACOS                    1
#  endif
#  if !defined(JSE_CLIB_ASCTIME) 
#     define JSE_CLIB_ASCTIME                 1
#  endif
#  if !defined(JSE_CLIB_ASIN) 
#     define JSE_CLIB_ASIN                    1
#  endif
#  if !defined(JSE_CLIB_ASSERT) 
#     define JSE_CLIB_ASSERT                  1
#  endif
#  if !defined(JSE_CLIB_ATAN) 
#     define JSE_CLIB_ATAN                    1
#  endif
#  if !defined(JSE_CLIB_ATAN2) 
#     define JSE_CLIB_ATAN2                   1
#  endif
#  if !defined(JSE_CLIB_ATEXIT) 
#     define JSE_CLIB_ATEXIT                  1
#  endif
#  if !defined(JSE_CLIB_ATOI) 
#     define JSE_CLIB_ATOI                    1
#  endif
#  if !defined(JSE_CLIB_ATOF) 
#     define JSE_CLIB_ATOF                    1
#  endif
#  if !defined(JSE_CLIB_ATOL) 
#     define JSE_CLIB_ATOL                    1
#  endif
#  if !defined(JSE_CLIB_BSEARCH) 
#     define JSE_CLIB_BSEARCH                 1
#  endif
#  if !defined(JSE_CLIB_CEIL) 
#     define JSE_CLIB_CEIL                    1
#  endif
#  if !defined(JSE_CLIB_CHDIR) 
#     define JSE_CLIB_CHDIR                   1
#  endif
#  if !defined(JSE_CLIB_CLEARERR) 
#     define JSE_CLIB_CLEARERR                1
#  endif
#  if !defined(JSE_CLIB_CLOCK) 
#     define JSE_CLIB_CLOCK                   1
#  endif
#  if !defined(JSE_CLIB_CTIME) 
#     define JSE_CLIB_CTIME                   1
#  endif
#  if !defined(JSE_CLIB_COS) 
#     define JSE_CLIB_COS                     1
#  endif
#  if !defined(JSE_CLIB_COSH) 
#     define JSE_CLIB_COSH                    1
#  endif
#  if !defined(JSE_CLIB_DIFFTIME) 
#     define JSE_CLIB_DIFFTIME                1
#  endif
#  if !defined(JSE_CLIB_DIV) 
#     define JSE_CLIB_DIV                     1
#  endif
#  if !defined(JSE_CLIB_ERRNO) 
#     define JSE_CLIB_ERRNO                   1
#  endif
#  if !defined(JSE_CLIB_EXIT) 
#     define JSE_CLIB_EXIT                    1
#  endif
#  if !defined(JSE_CLIB_EXP) 
#     define JSE_CLIB_EXP                     1
#  endif
#  if !defined(JSE_CLIB_FABS) 
#     define JSE_CLIB_FABS                    1
#  endif
#  if !defined(JSE_CLIB_FCLOSE) 
#     define JSE_CLIB_FCLOSE                  1
#  endif
#  if !defined(JSE_CLIB_FEOF) 
#     define JSE_CLIB_FEOF                    1
#  endif
#  if !defined(JSE_CLIB_FERROR) 
#     define JSE_CLIB_FERROR                  1
#  endif
#  if !defined(JSE_CLIB_FFLUSH) 
#     define JSE_CLIB_FFLUSH                  1
#  endif
#  if !defined(JSE_CLIB_FGETC) 
#     define JSE_CLIB_FGETC                   1
#  endif
#  if !defined(JSE_CLIB_FGETPOS) 
#     define JSE_CLIB_FGETPOS                 1
#  endif
#  if !defined(JSE_CLIB_FGETS) 
#     define JSE_CLIB_FGETS                   1
#  endif
#  if !defined(JSE_CLIB_FLOCK) 
#     define JSE_CLIB_FLOCK                   1
#  endif
#  if !defined(JSE_CLIB_FLOOR) 
#     define JSE_CLIB_FLOOR                   1
#  endif
#  if !defined(JSE_CLIB_FMOD) 
#     define JSE_CLIB_FMOD                    1
#  endif
#  if !defined(JSE_CLIB_FOPEN) 
#     define JSE_CLIB_FOPEN                   1
#  endif
#  if !defined(JSE_CLIB_FPUTC) 
#     define JSE_CLIB_FPUTC                   1
#  endif
#  if !defined(JSE_CLIB_FPRINTF) 
#     define JSE_CLIB_FPRINTF                 1
#  endif
#  if !defined(JSE_CLIB_FPUTS) 
#     define JSE_CLIB_FPUTS                   1
#  endif
#  if !defined(JSE_CLIB_FREAD) 
#     define JSE_CLIB_FREAD                   1
#  endif
#  if !defined(JSE_CLIB_FREXP) 
#     define JSE_CLIB_FREXP                   1
#  endif
#  if !defined(JSE_CLIB_FREOPEN) 
#     define JSE_CLIB_FREOPEN                 1
#  endif
#  if !defined(JSE_CLIB_FSCANF) 
#     define JSE_CLIB_FSCANF                  1
#  endif
#  if !defined(JSE_CLIB_FSEEK) 
#     define JSE_CLIB_FSEEK                   1
#  endif
#  if !defined(JSE_CLIB_FSETPOS) 
#     define JSE_CLIB_FSETPOS                 1
#  endif
#  if !defined(JSE_CLIB_FTELL) 
#     define JSE_CLIB_FTELL                   1
#  endif
#  if !defined(JSE_CLIB_FWRITE) 
#     define JSE_CLIB_FWRITE                  1
#  endif
#  if !defined(JSE_CLIB_GETC) 
#     define JSE_CLIB_GETC                    1
#  endif
#  if !defined(JSE_CLIB_GETCH) 
#     define JSE_CLIB_GETCH                   1
#  endif
#  if !defined(JSE_CLIB_GETCHAR) 
#     define JSE_CLIB_GETCHAR                 1
#  endif
#  if !defined(JSE_CLIB_GETCHE) 
#     define JSE_CLIB_GETCHE                  1
#  endif
#  if !defined(JSE_CLIB_GETCWD) 
#     define JSE_CLIB_GETCWD                  1
#  endif
#  if !defined(JSE_CLIB_GETENV) 
#     define JSE_CLIB_GETENV                  1
#  endif
#  if !defined(JSE_CLIB_GETS) 
#     define JSE_CLIB_GETS                    1
#  endif
#  if !defined(JSE_CLIB_GMTIME) 
#     define JSE_CLIB_GMTIME                  1
#  endif
#  if !defined(JSE_CLIB_ISALNUM) 
#     define JSE_CLIB_ISALNUM                 1
#  endif
#  if !defined(JSE_CLIB_ISALPHA) 
#     define JSE_CLIB_ISALPHA                 1
#  endif
#  if !defined(JSE_CLIB_ISASCII) 
#     define JSE_CLIB_ISASCII                 1
#  endif
#  if !defined(JSE_CLIB_ISCNTRL) 
#     define JSE_CLIB_ISCNTRL                 1
#  endif
#  if !defined(JSE_CLIB_ISDIGIT) 
#     define JSE_CLIB_ISDIGIT                 1
#  endif
#  if !defined(JSE_CLIB_ISGRAPH) 
#     define JSE_CLIB_ISGRAPH                 1
#  endif
#  if !defined(JSE_CLIB_ISLOWER) 
#     define JSE_CLIB_ISLOWER                 1
#  endif
#  if !defined(JSE_CLIB_ISPRINT) 
#     define JSE_CLIB_ISPRINT                 1
#  endif
#  if !defined(JSE_CLIB_ISPUNCT) 
#     define JSE_CLIB_ISPUNCT                 1
#  endif
#  if !defined(JSE_CLIB_ISSPACE) 
#     define JSE_CLIB_ISSPACE                 1
#  endif
#  if !defined(JSE_CLIB_ISUPPER) 
#     define JSE_CLIB_ISUPPER                 1
#  endif
#  if !defined(JSE_CLIB_ISXDIGIT) 
#     define JSE_CLIB_ISXDIGIT                1
#  endif
#  if !defined(JSE_CLIB_KBHIT) 
#     define JSE_CLIB_KBHIT                   1
#  endif
#  if !defined(JSE_CLIB_LABS) 
#     define JSE_CLIB_LABS                    1
#  endif
#  if !defined(JSE_CLIB_LDEXP) 
#     define JSE_CLIB_LDEXP                   1
#  endif
#  if !defined(JSE_CLIB_LDIV) 
#     define JSE_CLIB_LDIV                    1
#  endif
#  if !defined(JSE_CLIB_LOCALTIME) 
#     define JSE_CLIB_LOCALTIME               1
#  endif
#  if !defined(JSE_CLIB_LOG) 
#     define JSE_CLIB_LOG                     1
#  endif
#  if !defined(JSE_CLIB_LOG10) 
#     define JSE_CLIB_LOG10                   1
#  endif
#  if !defined(JSE_CLIB_MAX) 
#     define JSE_CLIB_MAX                     1
#  endif
#  if !defined(JSE_CLIB_MIN) 
#     define JSE_CLIB_MIN                     1
#  endif
#  if !defined(JSE_CLIB_MKDIR) 
#     define JSE_CLIB_MKDIR                   1
#  endif
#  if !defined(JSE_CLIB_MEMCHR) 
#     define JSE_CLIB_MEMCHR                  1
#  endif
#  if !defined(JSE_CLIB_MEMCMP) 
#     define JSE_CLIB_MEMCMP                  1
#  endif
#  if !defined(JSE_CLIB_MEMCPY) 
#     define JSE_CLIB_MEMCPY                  1
#  endif
#  if !defined(JSE_CLIB_MEMMOVE) 
#     define JSE_CLIB_MEMMOVE                 1
#  endif
#  if !defined(JSE_CLIB_MEMSET) 
#     define JSE_CLIB_MEMSET                  1
#  endif
#  if !defined(JSE_CLIB_MKTIME) 
#     define JSE_CLIB_MKTIME                  1
#  endif
#  if !defined(JSE_CLIB_MODF) 
#     define JSE_CLIB_MODF                    1
#  endif
#  if !defined(JSE_CLIB_PERROR) 
#     define JSE_CLIB_PERROR                  1
#  endif
#  if !defined(JSE_CLIB_POW) 
#     define JSE_CLIB_POW                     1
#  endif
#  if !defined(JSE_CLIB_PRINTF) 
#     define JSE_CLIB_PRINTF                  1
#  endif
#  if !defined(JSE_CLIB_PUTC) 
#     define JSE_CLIB_PUTC                    1
#  endif
#  if !defined(JSE_CLIB_PUTCHAR) 
#     define JSE_CLIB_PUTCHAR                 1
#  endif
#  if !defined(JSE_CLIB_PUTENV) 
#     define JSE_CLIB_PUTENV                  1
#  endif
#  if !defined(JSE_CLIB_PUTS) 
#     define JSE_CLIB_PUTS                    1
#  endif
#  if !defined(JSE_CLIB_QSORT) 
#     define JSE_CLIB_QSORT                   1
#  endif
#  if !defined(JSE_CLIB_RAND) 
#     define JSE_CLIB_RAND                    1
#  endif
#  if !defined(JSE_CLIB_REMOVE) 
#     define JSE_CLIB_REMOVE                  1
#  endif
#  if !defined(JSE_CLIB_RENAME) 
#     define JSE_CLIB_RENAME                  1
#  endif
#  if !defined(JSE_CLIB_REWIND) 
#     define JSE_CLIB_REWIND                  1
#  endif
#  if !defined(JSE_CLIB_RMDIR) 
#     define JSE_CLIB_RMDIR                   1
#  endif
#  if !defined(JSE_CLIB_RSPRINTF) 
#     define JSE_CLIB_RSPRINTF                1
#  endif
#  if !defined(JSE_CLIB_RVSPRINTF) 
#     define JSE_CLIB_RVSPRINTF               1
#  endif
#  if !defined(JSE_CLIB_SCANF) 
#     define JSE_CLIB_SCANF                   1
#  endif
#  if !defined(JSE_CLIB_SIN) 
#     define JSE_CLIB_SIN                     1
#  endif
#  if !defined(JSE_CLIB_SINH) 
#     define JSE_CLIB_SINH                    1
#  endif
#  if !defined(JSE_CLIB_SPRINTF) 
#     define JSE_CLIB_SPRINTF                 1
#  endif
#  if !defined(JSE_CLIB_SQRT) 
#     define JSE_CLIB_SQRT                    1
#  endif
#  if !defined(JSE_CLIB_SRAND) 
#     define JSE_CLIB_SRAND                   1
#  endif
#  if !defined(JSE_CLIB_SSCANF) 
#     define JSE_CLIB_SSCANF                  1
#  endif
#  if !defined(JSE_CLIB_STRCAT) 
#     define JSE_CLIB_STRCAT                  1
#  endif
#  if !defined(JSE_CLIB_STRCHR) 
#     define JSE_CLIB_STRCHR                  1
#  endif
#  if !defined(JSE_CLIB_STRCMP) 
#     define JSE_CLIB_STRCMP                  1
#  endif
#  if !defined(JSE_CLIB_STRCPY) 
#     define JSE_CLIB_STRCPY                  1
#  endif
#  if !defined(JSE_CLIB_STRCSPN) 
#     define JSE_CLIB_STRCSPN                 1
#  endif
#  if !defined(JSE_CLIB_STRERROR) 
#     define JSE_CLIB_STRERROR                1
#  endif
#  if !defined(JSE_CLIB_STRFTIME) 
#     define JSE_CLIB_STRFTIME                1
#  endif
#  if !defined(JSE_CLIB_STRICMP) 
#     define JSE_CLIB_STRICMP                 1
#  endif
#  if !defined(JSE_CLIB_STRLEN) 
#     define JSE_CLIB_STRLEN                  1
#  endif
#  if !defined(JSE_CLIB_STRLWR) 
#     define JSE_CLIB_STRLWR                  1
#  endif
#  if !defined(JSE_CLIB_STRNCAT) 
#     define JSE_CLIB_STRNCAT                 1
#  endif
#  if !defined(JSE_CLIB_STRNCMP) 
#     define JSE_CLIB_STRNCMP                 1
#  endif
#  if !defined(JSE_CLIB_STRNICMP) 
#     define JSE_CLIB_STRNICMP                1
#  endif
#  if !defined(JSE_CLIB_STRNCPY) 
#     define JSE_CLIB_STRNCPY                 1
#  endif
#  if !defined(JSE_CLIB_STRPBRK) 
#     define JSE_CLIB_STRPBRK                 1
#  endif
#  if !defined(JSE_CLIB_STRRCHR) 
#     define JSE_CLIB_STRRCHR                 1
#  endif
#  if !defined(JSE_CLIB_STRSPN) 
#     define JSE_CLIB_STRSPN                  1
#  endif
#  if !defined(JSE_CLIB_STRSTR) 
#     define JSE_CLIB_STRSTR                  1
#  endif
#  if !defined(JSE_CLIB_STRSTRI) 
#     define JSE_CLIB_STRSTRI                 1
#  endif
#  if !defined(JSE_CLIB_STRTOD) 
#     define JSE_CLIB_STRTOD                  1
#  endif
#  if !defined(JSE_CLIB_STRTOK) 
#     define JSE_CLIB_STRTOK                  1
#  endif
#  if !defined(JSE_CLIB_STRTOL) 
#     define JSE_CLIB_STRTOL                  1
#  endif
#  if !defined(JSE_CLIB_STRUPR) 
#     define JSE_CLIB_STRUPR                  1
#  endif
#  if !defined(JSE_CLIB_SUSPEND) 
#     define JSE_CLIB_SUSPEND                 1
#  endif
#  if !defined(JSE_CLIB_SYSTEM) 
#     define JSE_CLIB_SYSTEM                  1
#  endif
#  if !defined(JSE_CLIB_TAN) 
#     define JSE_CLIB_TAN                     1
#  endif
#  if !defined(JSE_CLIB_TANH) 
#     define JSE_CLIB_TANH                    1
#  endif
#  if !defined(JSE_CLIB_TIME) 
#     define JSE_CLIB_TIME                    1
#  endif
#  if !defined(JSE_CLIB_TMPFILE) 
#     define JSE_CLIB_TMPFILE                 1
#  endif
#  if !defined(JSE_CLIB_TMPNAME) 
#     define JSE_CLIB_TMPNAME                 1
#  endif
#  if !defined(JSE_CLIB_TOASCII) 
#     define JSE_CLIB_TOASCII                 1
#  endif
#  if !defined(JSE_CLIB_TOLOWER) 
#     define JSE_CLIB_TOLOWER                 1
#  endif
#  if !defined(JSE_CLIB_TOUPPER) 
#     define JSE_CLIB_TOUPPER                 1
#  endif
#  if !defined(JSE_CLIB_UNGETC) 
#     define JSE_CLIB_UNGETC                  1
#  endif
#  if !defined(JSE_CLIB_VA_ARG) 
#     define JSE_CLIB_VA_ARG                  1
#  endif
#  if !defined(JSE_CLIB_VA_END) 
#     define JSE_CLIB_VA_END                  1
#  endif
#  if !defined(JSE_CLIB_VA_START) 
#     define JSE_CLIB_VA_START                1
#  endif
#  if !defined(JSE_CLIB_VFSCANF) 
#     define JSE_CLIB_VFSCANF                 1
#  endif
#  if !defined(JSE_CLIB_VFPRINTF) 
#     define JSE_CLIB_VFPRINTF                1
#  endif
#  if !defined(JSE_CLIB_VPRINTF) 
#     define JSE_CLIB_VPRINTF                 1
#  endif
#  if !defined(JSE_CLIB_VSCANF) 
#     define JSE_CLIB_VSCANF                  1
#  endif
#  if !defined(JSE_CLIB_VSPRINTF) 
#     define JSE_CLIB_VSPRINTF                1
#  endif
#  if !defined(JSE_CLIB_VSSCANF) 
#     define JSE_CLIB_VSSCANF                 1
#  endif
#endif /* JSE_CLIB_ALL */
/* Convert zeros to undefines */
#if defined(JSE_CLIB_ABORT) && JSE_CLIB_ABORT == 0
#  undef JSE_CLIB_ABORT
#endif
#if defined(JSE_CLIB_ABS) && JSE_CLIB_ABS == 0
#  undef JSE_CLIB_ABS
#endif
#if defined(JSE_CLIB_ACOS) && JSE_CLIB_ACOS == 0
#  undef JSE_CLIB_ACOS
#endif
#if defined(JSE_CLIB_ASCTIME) && JSE_CLIB_ASCTIME == 0
#  undef JSE_CLIB_ASCTIME
#endif
#if defined(JSE_CLIB_ASIN) && JSE_CLIB_ASIN == 0
#  undef JSE_CLIB_ASIN
#endif
#if defined(JSE_CLIB_ASSERT) && JSE_CLIB_ASSERT == 0
#  undef JSE_CLIB_ASSERT
#endif
#if defined(JSE_CLIB_ATAN) && JSE_CLIB_ATAN == 0
#  undef JSE_CLIB_ATAN
#endif
#if defined(JSE_CLIB_ATAN2) && JSE_CLIB_ATAN2 == 0
#  undef JSE_CLIB_ATAN2
#endif
#if defined(JSE_CLIB_ATEXIT) && JSE_CLIB_ATEXIT == 0
#  undef JSE_CLIB_ATEXIT
#endif
#if defined(JSE_CLIB_ATOI) && JSE_CLIB_ATOI == 0
#  undef JSE_CLIB_ATOI
#endif
#if defined(JSE_CLIB_ATOF) && JSE_CLIB_ATOF == 0
#  undef JSE_CLIB_ATOF
#endif
#if defined(JSE_CLIB_ATOL) && JSE_CLIB_ATOL == 0
#  undef JSE_CLIB_ATOL
#endif
#if defined(JSE_CLIB_BSEARCH) && JSE_CLIB_BSEARCH == 0
#  undef JSE_CLIB_BSEARCH
#endif
#if defined(JSE_CLIB_CEIL) && JSE_CLIB_CEIL == 0
#  undef JSE_CLIB_CEIL
#endif
#if defined(JSE_CLIB_CHDIR) && JSE_CLIB_CHDIR == 0
#  undef JSE_CLIB_CHDIR
#endif
#if defined(JSE_CLIB_CLEARERR) && JSE_CLIB_CLEARERR == 0
#  undef JSE_CLIB_CLEARERR
#endif
#if defined(JSE_CLIB_CLOCK) && JSE_CLIB_CLOCK == 0
#  undef JSE_CLIB_CLOCK
#endif
#if defined(JSE_CLIB_CTIME) && JSE_CLIB_CTIME == 0
#  undef JSE_CLIB_CTIME
#endif
#if defined(JSE_CLIB_COS) && JSE_CLIB_COS == 0
#  undef JSE_CLIB_COS
#endif
#if defined(JSE_CLIB_COSH) && JSE_CLIB_COSH == 0
#  undef JSE_CLIB_COSH
#endif
#if defined(JSE_CLIB_DIFFTIME) && JSE_CLIB_DIFFTIME == 0
#  undef JSE_CLIB_DIFFTIME
#endif
#if defined(JSE_CLIB_DIV) && JSE_CLIB_DIV == 0
#  undef JSE_CLIB_DIV
#endif
#if defined(JSE_CLIB_ERRNO) && JSE_CLIB_ERRNO == 0
#  undef JSE_CLIB_ERRNO
#endif
#if defined(JSE_CLIB_EXIT) && JSE_CLIB_EXIT == 0
#  undef JSE_CLIB_EXIT
#endif
#if defined(JSE_CLIB_EXP) && JSE_CLIB_EXP == 0
#  undef JSE_CLIB_EXP
#endif
#if defined(JSE_CLIB_FABS) && JSE_CLIB_FABS == 0
#  undef JSE_CLIB_FABS
#endif
#if defined(JSE_CLIB_FCLOSE) && JSE_CLIB_FCLOSE == 0
#  undef JSE_CLIB_FCLOSE
#endif
#if defined(JSE_CLIB_FEOF) && JSE_CLIB_FEOF == 0
#  undef JSE_CLIB_FEOF
#endif
#if defined(JSE_CLIB_FERROR) && JSE_CLIB_FERROR == 0
#  undef JSE_CLIB_FERROR
#endif
#if defined(JSE_CLIB_FFLUSH) && JSE_CLIB_FFLUSH == 0
#  undef JSE_CLIB_FFLUSH
#endif
#if defined(JSE_CLIB_FGETC) && JSE_CLIB_FGETC == 0
#  undef JSE_CLIB_FGETC
#endif
#if defined(JSE_CLIB_FGETPOS) && JSE_CLIB_FGETPOS == 0
#  undef JSE_CLIB_FGETPOS
#endif
#if defined(JSE_CLIB_FGETS) && JSE_CLIB_FGETS == 0
#  undef JSE_CLIB_FGETS
#endif
#if defined(JSE_CLIB_FLOCK) && JSE_CLIB_FLOCK == 0
#  undef JSE_CLIB_FLOCK
#endif
#if defined(JSE_CLIB_FLOOR) && JSE_CLIB_FLOOR == 0
#  undef JSE_CLIB_FLOOR
#endif
#if defined(JSE_CLIB_FMOD) && JSE_CLIB_FMOD == 0
#  undef JSE_CLIB_FMOD
#endif
#if defined(JSE_CLIB_FOPEN) && JSE_CLIB_FOPEN == 0
#  undef JSE_CLIB_FOPEN
#endif
#if defined(JSE_CLIB_FPUTC) && JSE_CLIB_FPUTC == 0
#  undef JSE_CLIB_FPUTC
#endif
#if defined(JSE_CLIB_FPRINTF) && JSE_CLIB_FPRINTF == 0
#  undef JSE_CLIB_FPRINTF
#endif
#if defined(JSE_CLIB_FPUTS) && JSE_CLIB_FPUTS == 0
#  undef JSE_CLIB_FPUTS
#endif
#if defined(JSE_CLIB_FREAD) && JSE_CLIB_FREAD == 0
#  undef JSE_CLIB_FREAD
#endif
#if defined(JSE_CLIB_FREXP) && JSE_CLIB_FREXP == 0
#  undef JSE_CLIB_FREXP
#endif
#if defined(JSE_CLIB_FREOPEN) && JSE_CLIB_FREOPEN == 0
#  undef JSE_CLIB_FREOPEN
#endif
#if defined(JSE_CLIB_FSCANF) && JSE_CLIB_FSCANF == 0
#  undef JSE_CLIB_FSCANF
#endif
#if defined(JSE_CLIB_FSEEK) && JSE_CLIB_FSEEK == 0
#  undef JSE_CLIB_FSEEK
#endif
#if defined(JSE_CLIB_FSETPOS) && JSE_CLIB_FSETPOS == 0
#  undef JSE_CLIB_FSETPOS
#endif
#if defined(JSE_CLIB_FTELL) && JSE_CLIB_FTELL == 0
#  undef JSE_CLIB_FTELL
#endif
#if defined(JSE_CLIB_FWRITE) && JSE_CLIB_FWRITE == 0
#  undef JSE_CLIB_FWRITE
#endif
#if defined(JSE_CLIB_GETC) && JSE_CLIB_GETC == 0
#  undef JSE_CLIB_GETC
#endif
#if defined(JSE_CLIB_GETCH) && JSE_CLIB_GETCH == 0
#  undef JSE_CLIB_GETCH
#endif
#if defined(JSE_CLIB_GETCHAR) && JSE_CLIB_GETCHAR == 0
#  undef JSE_CLIB_GETCHAR
#endif
#if defined(JSE_CLIB_GETCHE) && JSE_CLIB_GETCHE == 0
#  undef JSE_CLIB_GETCHE
#endif
#if defined(JSE_CLIB_GETCWD) && JSE_CLIB_GETCWD == 0
#  undef JSE_CLIB_GETCWD
#endif
#if defined(JSE_CLIB_GETENV) && JSE_CLIB_GETENV == 0
#  undef JSE_CLIB_GETENV
#endif
#if defined(JSE_CLIB_GETS) && JSE_CLIB_GETS == 0
#  undef JSE_CLIB_GETS
#endif
#if defined(JSE_CLIB_GMTIME) && JSE_CLIB_GMTIME == 0
#  undef JSE_CLIB_GMTIME
#endif
#if defined(JSE_CLIB_ISALNUM) && JSE_CLIB_ISALNUM == 0
#  undef JSE_CLIB_ISALNUM
#endif
#if defined(JSE_CLIB_ISALPHA) && JSE_CLIB_ISALPHA == 0
#  undef JSE_CLIB_ISALPHA
#endif
#if defined(JSE_CLIB_ISASCII) && JSE_CLIB_ISASCII == 0
#  undef JSE_CLIB_ISASCII
#endif
#if defined(JSE_CLIB_ISCNTRL) && JSE_CLIB_ISCNTRL == 0
#  undef JSE_CLIB_ISCNTRL
#endif
#if defined(JSE_CLIB_ISDIGIT) && JSE_CLIB_ISDIGIT == 0
#  undef JSE_CLIB_ISDIGIT
#endif
#if defined(JSE_CLIB_ISGRAPH) && JSE_CLIB_ISGRAPH == 0
#  undef JSE_CLIB_ISGRAPH
#endif
#if defined(JSE_CLIB_ISLOWER) && JSE_CLIB_ISLOWER == 0
#  undef JSE_CLIB_ISLOWER
#endif
#if defined(JSE_CLIB_ISPRINT) && JSE_CLIB_ISPRINT == 0
#  undef JSE_CLIB_ISPRINT
#endif
#if defined(JSE_CLIB_ISPUNCT) && JSE_CLIB_ISPUNCT == 0
#  undef JSE_CLIB_ISPUNCT
#endif
#if defined(JSE_CLIB_ISSPACE) && JSE_CLIB_ISSPACE == 0
#  undef JSE_CLIB_ISSPACE
#endif
#if defined(JSE_CLIB_ISUPPER) && JSE_CLIB_ISUPPER == 0
#  undef JSE_CLIB_ISUPPER
#endif
#if defined(JSE_CLIB_ISXDIGIT) && JSE_CLIB_ISXDIGIT == 0
#  undef JSE_CLIB_ISXDIGIT
#endif
#if defined(JSE_CLIB_KBHIT) && JSE_CLIB_KBHIT == 0
#  undef JSE_CLIB_KBHIT
#endif
#if defined(JSE_CLIB_LABS) && JSE_CLIB_LABS == 0
#  undef JSE_CLIB_LABS
#endif
#if defined(JSE_CLIB_LDEXP) && JSE_CLIB_LDEXP == 0
#  undef JSE_CLIB_LDEXP
#endif
#if defined(JSE_CLIB_LDIV) && JSE_CLIB_LDIV == 0
#  undef JSE_CLIB_LDIV
#endif
#if defined(JSE_CLIB_LOCALTIME) && JSE_CLIB_LOCALTIME == 0
#  undef JSE_CLIB_LOCALTIME
#endif
#if defined(JSE_CLIB_LOG) && JSE_CLIB_LOG == 0
#  undef JSE_CLIB_LOG
#endif
#if defined(JSE_CLIB_LOG10) && JSE_CLIB_LOG10 == 0
#  undef JSE_CLIB_LOG10
#endif
#if defined(JSE_CLIB_MAX) && JSE_CLIB_MAX == 0
#  undef JSE_CLIB_MAX
#endif
#if defined(JSE_CLIB_MIN) && JSE_CLIB_MIN == 0
#  undef JSE_CLIB_MIN
#endif
#if defined(JSE_CLIB_MKDIR) && JSE_CLIB_MKDIR == 0
#  undef JSE_CLIB_MKDIR
#endif
#if defined(JSE_CLIB_MEMCHR) && JSE_CLIB_MEMCHR == 0
#  undef JSE_CLIB_MEMCHR
#endif
#if defined(JSE_CLIB_MEMCMP) && JSE_CLIB_MEMCMP == 0
#  undef JSE_CLIB_MEMCMP
#endif
#if defined(JSE_CLIB_MEMCPY) && JSE_CLIB_MEMCPY == 0
#  undef JSE_CLIB_MEMCPY
#endif
#if defined(JSE_CLIB_MEMMOVE) && JSE_CLIB_MEMMOVE == 0
#  undef JSE_CLIB_MEMMOVE
#endif
#if defined(JSE_CLIB_MEMSET) && JSE_CLIB_MEMSET == 0
#  undef JSE_CLIB_MEMSET
#endif
#if defined(JSE_CLIB_MKTIME) && JSE_CLIB_MKTIME == 0
#  undef JSE_CLIB_MKTIME
#endif
#if defined(JSE_CLIB_MODF) && JSE_CLIB_MODF == 0
#  undef JSE_CLIB_MODF
#endif
#if defined(JSE_CLIB_PERROR) && JSE_CLIB_PERROR == 0
#  undef JSE_CLIB_PERROR
#endif
#if defined(JSE_CLIB_POW) && JSE_CLIB_POW == 0
#  undef JSE_CLIB_POW
#endif
#if defined(JSE_CLIB_PRINTF) && JSE_CLIB_PRINTF == 0
#  undef JSE_CLIB_PRINTF
#endif
#if defined(JSE_CLIB_PUTC) && JSE_CLIB_PUTC == 0
#  undef JSE_CLIB_PUTC
#endif
#if defined(JSE_CLIB_PUTCHAR) && JSE_CLIB_PUTCHAR == 0
#  undef JSE_CLIB_PUTCHAR
#endif
#if defined(JSE_CLIB_PUTENV) && JSE_CLIB_PUTENV == 0
#  undef JSE_CLIB_PUTENV
#endif
#if defined(JSE_CLIB_PUTS) && JSE_CLIB_PUTS == 0
#  undef JSE_CLIB_PUTS
#endif
#if defined(JSE_CLIB_QSORT) && JSE_CLIB_QSORT == 0
#  undef JSE_CLIB_QSORT
#endif
#if defined(JSE_CLIB_RAND) && JSE_CLIB_RAND == 0
#  undef JSE_CLIB_RAND
#endif
#if defined(JSE_CLIB_REMOVE) && JSE_CLIB_REMOVE == 0
#  undef JSE_CLIB_REMOVE
#endif
#if defined(JSE_CLIB_RENAME) && JSE_CLIB_RENAME == 0
#  undef JSE_CLIB_RENAME
#endif
#if defined(JSE_CLIB_REWIND) && JSE_CLIB_REWIND == 0
#  undef JSE_CLIB_REWIND
#endif
#if defined(JSE_CLIB_RMDIR) && JSE_CLIB_RMDIR == 0
#  undef JSE_CLIB_RMDIR
#endif
#if defined(JSE_CLIB_RSPRINTF) && JSE_CLIB_RSPRINTF == 0
#  undef JSE_CLIB_RSPRINTF
#endif
#if defined(JSE_CLIB_RVSPRINTF) && JSE_CLIB_RVSPRINTF == 0
#  undef JSE_CLIB_RVSPRINTF
#endif
#if defined(JSE_CLIB_SCANF) && JSE_CLIB_SCANF == 0
#  undef JSE_CLIB_SCANF
#endif
#if defined(JSE_CLIB_SIN) && JSE_CLIB_SIN == 0
#  undef JSE_CLIB_SIN
#endif
#if defined(JSE_CLIB_SINH) && JSE_CLIB_SINH == 0
#  undef JSE_CLIB_SINH
#endif
#if defined(JSE_CLIB_SPRINTF) && JSE_CLIB_SPRINTF == 0
#  undef JSE_CLIB_SPRINTF
#endif
#if defined(JSE_CLIB_SQRT) && JSE_CLIB_SQRT == 0
#  undef JSE_CLIB_SQRT
#endif
#if defined(JSE_CLIB_SRAND) && JSE_CLIB_SRAND == 0
#  undef JSE_CLIB_SRAND
#endif
#if defined(JSE_CLIB_SSCANF) && JSE_CLIB_SSCANF == 0
#  undef JSE_CLIB_SSCANF
#endif
#if defined(JSE_CLIB_STRCAT) && JSE_CLIB_STRCAT == 0
#  undef JSE_CLIB_STRCAT
#endif
#if defined(JSE_CLIB_STRCHR) && JSE_CLIB_STRCHR == 0
#  undef JSE_CLIB_STRCHR
#endif
#if defined(JSE_CLIB_STRCMP) && JSE_CLIB_STRCMP == 0
#  undef JSE_CLIB_STRCMP
#endif
#if defined(JSE_CLIB_STRCPY) && JSE_CLIB_STRCPY == 0
#  undef JSE_CLIB_STRCPY
#endif
#if defined(JSE_CLIB_STRCSPN) && JSE_CLIB_STRCSPN == 0
#  undef JSE_CLIB_STRCSPN
#endif
#if defined(JSE_CLIB_STRERROR) && JSE_CLIB_STRERROR == 0
#  undef JSE_CLIB_STRERROR
#endif
#if defined(JSE_CLIB_STRFTIME) && JSE_CLIB_STRFTIME == 0
#  undef JSE_CLIB_STRFTIME
#endif
#if defined(JSE_CLIB_STRICMP) && JSE_CLIB_STRICMP == 0
#  undef JSE_CLIB_STRICMP
#endif
#if defined(JSE_CLIB_STRLEN) && JSE_CLIB_STRLEN == 0
#  undef JSE_CLIB_STRLEN
#endif
#if defined(JSE_CLIB_STRLWR) && JSE_CLIB_STRLWR == 0
#  undef JSE_CLIB_STRLWR
#endif
#if defined(JSE_CLIB_STRNCAT) && JSE_CLIB_STRNCAT == 0
#  undef JSE_CLIB_STRNCAT
#endif
#if defined(JSE_CLIB_STRNCMP) && JSE_CLIB_STRNCMP == 0
#  undef JSE_CLIB_STRNCMP
#endif
#if defined(JSE_CLIB_STRNICMP) && JSE_CLIB_STRNICMP == 0
#  undef JSE_CLIB_STRNICMP
#endif
#if defined(JSE_CLIB_STRNCPY) && JSE_CLIB_STRNCPY == 0
#  undef JSE_CLIB_STRNCPY
#endif
#if defined(JSE_CLIB_STRPBRK) && JSE_CLIB_STRPBRK == 0
#  undef JSE_CLIB_STRPBRK
#endif
#if defined(JSE_CLIB_STRRCHR) && JSE_CLIB_STRRCHR == 0
#  undef JSE_CLIB_STRRCHR
#endif
#if defined(JSE_CLIB_STRSPN) && JSE_CLIB_STRSPN == 0
#  undef JSE_CLIB_STRSPN
#endif
#if defined(JSE_CLIB_STRSTR) && JSE_CLIB_STRSTR == 0
#  undef JSE_CLIB_STRSTR
#endif
#if defined(JSE_CLIB_STRSTRI) && JSE_CLIB_STRSTRI == 0
#  undef JSE_CLIB_STRSTRI
#endif
#if defined(JSE_CLIB_STRTOD) && JSE_CLIB_STRTOD == 0
#  undef JSE_CLIB_STRTOD
#endif
#if defined(JSE_CLIB_STRTOK) && JSE_CLIB_STRTOK == 0
#  undef JSE_CLIB_STRTOK
#endif
#if defined(JSE_CLIB_STRTOL) && JSE_CLIB_STRTOL == 0
#  undef JSE_CLIB_STRTOL
#endif
#if defined(JSE_CLIB_STRUPR) && JSE_CLIB_STRUPR == 0
#  undef JSE_CLIB_STRUPR
#endif
#if defined(JSE_CLIB_SUSPEND) && JSE_CLIB_SUSPEND == 0
#  undef JSE_CLIB_SUSPEND
#endif
#if defined(JSE_CLIB_SYSTEM) && JSE_CLIB_SYSTEM == 0
#  undef JSE_CLIB_SYSTEM
#endif
#if defined(JSE_CLIB_TAN) && JSE_CLIB_TAN == 0
#  undef JSE_CLIB_TAN
#endif
#if defined(JSE_CLIB_TANH) && JSE_CLIB_TANH == 0
#  undef JSE_CLIB_TANH
#endif
#if defined(JSE_CLIB_TIME) && JSE_CLIB_TIME == 0
#  undef JSE_CLIB_TIME
#endif
#if defined(JSE_CLIB_TMPFILE) && JSE_CLIB_TMPFILE == 0
#  undef JSE_CLIB_TMPFILE
#endif
#if defined(JSE_CLIB_TMPNAME) && JSE_CLIB_TMPNAME == 0
#  undef JSE_CLIB_TMPNAME
#endif
#if defined(JSE_CLIB_TOASCII) && JSE_CLIB_TOASCII == 0
#  undef JSE_CLIB_TOASCII
#endif
#if defined(JSE_CLIB_TOLOWER) && JSE_CLIB_TOLOWER == 0
#  undef JSE_CLIB_TOLOWER
#endif
#if defined(JSE_CLIB_TOUPPER) && JSE_CLIB_TOUPPER == 0
#  undef JSE_CLIB_TOUPPER
#endif
#if defined(JSE_CLIB_UNGETC) && JSE_CLIB_UNGETC == 0
#  undef JSE_CLIB_UNGETC
#endif
#if defined(JSE_CLIB_VA_ARG) && JSE_CLIB_VA_ARG == 0
#  undef JSE_CLIB_VA_ARG
#endif
#if defined(JSE_CLIB_VA_END) && JSE_CLIB_VA_END == 0
#  undef JSE_CLIB_VA_END
#endif
#if defined(JSE_CLIB_VA_START) && JSE_CLIB_VA_START == 0
#  undef JSE_CLIB_VA_START
#endif
#if defined(JSE_CLIB_VFSCANF) && JSE_CLIB_VFSCANF == 0
#  undef JSE_CLIB_VFSCANF
#endif
#if defined(JSE_CLIB_VFPRINTF) && JSE_CLIB_VFPRINTF == 0
#  undef JSE_CLIB_VFPRINTF
#endif
#if defined(JSE_CLIB_VPRINTF) && JSE_CLIB_VPRINTF == 0
#  undef JSE_CLIB_VPRINTF
#endif
#if defined(JSE_CLIB_VSCANF) && JSE_CLIB_VSCANF == 0
#  undef JSE_CLIB_VSCANF
#endif
#if defined(JSE_CLIB_VSPRINTF) && JSE_CLIB_VSPRINTF == 0
#  undef JSE_CLIB_VSPRINTF
#endif
#if defined(JSE_CLIB_VSSCANF) && JSE_CLIB_VSSCANF == 0
#  undef JSE_CLIB_VSSCANF
#endif
/* Define generic JSE_CLIB_ANY */
#if defined(JSE_CLIB_ABORT) \
 || defined(JSE_CLIB_ABS) \
 || defined(JSE_CLIB_ACOS) \
 || defined(JSE_CLIB_ASCTIME) \
 || defined(JSE_CLIB_ASIN) \
 || defined(JSE_CLIB_ASSERT) \
 || defined(JSE_CLIB_ATAN) \
 || defined(JSE_CLIB_ATAN2) \
 || defined(JSE_CLIB_ATEXIT) \
 || defined(JSE_CLIB_ATOI) \
 || defined(JSE_CLIB_ATOF) \
 || defined(JSE_CLIB_ATOL) \
 || defined(JSE_CLIB_BSEARCH) \
 || defined(JSE_CLIB_CEIL) \
 || defined(JSE_CLIB_CHDIR) \
 || defined(JSE_CLIB_CLEARERR) \
 || defined(JSE_CLIB_CLOCK) \
 || defined(JSE_CLIB_CTIME) \
 || defined(JSE_CLIB_COS) \
 || defined(JSE_CLIB_COSH) \
 || defined(JSE_CLIB_DIFFTIME) \
 || defined(JSE_CLIB_DIV) \
 || defined(JSE_CLIB_ERRNO) \
 || defined(JSE_CLIB_EXIT) \
 || defined(JSE_CLIB_EXP) \
 || defined(JSE_CLIB_FABS) \
 || defined(JSE_CLIB_FCLOSE) \
 || defined(JSE_CLIB_FEOF) \
 || defined(JSE_CLIB_FERROR) \
 || defined(JSE_CLIB_FFLUSH) \
 || defined(JSE_CLIB_FGETC) \
 || defined(JSE_CLIB_FGETPOS) \
 || defined(JSE_CLIB_FGETS) \
 || defined(JSE_CLIB_FLOCK) \
 || defined(JSE_CLIB_FLOOR) \
 || defined(JSE_CLIB_FMOD) \
 || defined(JSE_CLIB_FOPEN) \
 || defined(JSE_CLIB_FPUTC) \
 || defined(JSE_CLIB_FPRINTF) \
 || defined(JSE_CLIB_FPUTS) \
 || defined(JSE_CLIB_FREAD) \
 || defined(JSE_CLIB_FREXP) \
 || defined(JSE_CLIB_FREOPEN) \
 || defined(JSE_CLIB_FSCANF) \
 || defined(JSE_CLIB_FSEEK) \
 || defined(JSE_CLIB_FSETPOS) \
 || defined(JSE_CLIB_FTELL) \
 || defined(JSE_CLIB_FWRITE) \
 || defined(JSE_CLIB_GETC) \
 || defined(JSE_CLIB_GETCH) \
 || defined(JSE_CLIB_GETCHAR) \
 || defined(JSE_CLIB_GETCHE) \
 || defined(JSE_CLIB_GETCWD) \
 || defined(JSE_CLIB_GETENV) \
 || defined(JSE_CLIB_GETS) \
 || defined(JSE_CLIB_GMTIME) \
 || defined(JSE_CLIB_ISALNUM) \
 || defined(JSE_CLIB_ISALPHA) \
 || defined(JSE_CLIB_ISASCII) \
 || defined(JSE_CLIB_ISCNTRL) \
 || defined(JSE_CLIB_ISDIGIT) \
 || defined(JSE_CLIB_ISGRAPH) \
 || defined(JSE_CLIB_ISLOWER) \
 || defined(JSE_CLIB_ISPRINT) \
 || defined(JSE_CLIB_ISPUNCT) \
 || defined(JSE_CLIB_ISSPACE) \
 || defined(JSE_CLIB_ISUPPER) \
 || defined(JSE_CLIB_ISXDIGIT) \
 || defined(JSE_CLIB_KBHIT) \
 || defined(JSE_CLIB_LABS) \
 || defined(JSE_CLIB_LDEXP) \
 || defined(JSE_CLIB_LDIV) \
 || defined(JSE_CLIB_LOCALTIME) \
 || defined(JSE_CLIB_LOG) \
 || defined(JSE_CLIB_LOG10) \
 || defined(JSE_CLIB_MAX) \
 || defined(JSE_CLIB_MIN) \
 || defined(JSE_CLIB_MKDIR) \
 || defined(JSE_CLIB_MEMCHR) \
 || defined(JSE_CLIB_MEMCMP) \
 || defined(JSE_CLIB_MEMCPY) \
 || defined(JSE_CLIB_MEMMOVE) \
 || defined(JSE_CLIB_MEMSET) \
 || defined(JSE_CLIB_MKTIME) \
 || defined(JSE_CLIB_MODF) \
 || defined(JSE_CLIB_PERROR) \
 || defined(JSE_CLIB_POW) \
 || defined(JSE_CLIB_PRINTF) \
 || defined(JSE_CLIB_PUTC) \
 || defined(JSE_CLIB_PUTCHAR) \
 || defined(JSE_CLIB_PUTENV) \
 || defined(JSE_CLIB_PUTS) \
 || defined(JSE_CLIB_QSORT) \
 || defined(JSE_CLIB_RAND) \
 || defined(JSE_CLIB_REMOVE) \
 || defined(JSE_CLIB_RENAME) \
 || defined(JSE_CLIB_REWIND) \
 || defined(JSE_CLIB_RMDIR) \
 || defined(JSE_CLIB_RSPRINTF) \
 || defined(JSE_CLIB_RVSPRINTF) \
 || defined(JSE_CLIB_SCANF) \
 || defined(JSE_CLIB_SIN) \
 || defined(JSE_CLIB_SINH) \
 || defined(JSE_CLIB_SPRINTF) \
 || defined(JSE_CLIB_SQRT) \
 || defined(JSE_CLIB_SRAND) \
 || defined(JSE_CLIB_SSCANF) \
 || defined(JSE_CLIB_STRCAT) \
 || defined(JSE_CLIB_STRCHR) \
 || defined(JSE_CLIB_STRCMP) \
 || defined(JSE_CLIB_STRCPY) \
 || defined(JSE_CLIB_STRCSPN) \
 || defined(JSE_CLIB_STRERROR) \
 || defined(JSE_CLIB_STRFTIME) \
 || defined(JSE_CLIB_STRICMP) \
 || defined(JSE_CLIB_STRLEN) \
 || defined(JSE_CLIB_STRLWR) \
 || defined(JSE_CLIB_STRNCAT) \
 || defined(JSE_CLIB_STRNCMP) \
 || defined(JSE_CLIB_STRNICMP) \
 || defined(JSE_CLIB_STRNCPY) \
 || defined(JSE_CLIB_STRPBRK) \
 || defined(JSE_CLIB_STRRCHR) \
 || defined(JSE_CLIB_STRSPN) \
 || defined(JSE_CLIB_STRSTR) \
 || defined(JSE_CLIB_STRSTRI) \
 || defined(JSE_CLIB_STRTOD) \
 || defined(JSE_CLIB_STRTOK) \
 || defined(JSE_CLIB_STRTOL) \
 || defined(JSE_CLIB_STRUPR) \
 || defined(JSE_CLIB_SUSPEND) \
 || defined(JSE_CLIB_SYSTEM) \
 || defined(JSE_CLIB_TAN) \
 || defined(JSE_CLIB_TANH) \
 || defined(JSE_CLIB_TIME) \
 || defined(JSE_CLIB_TMPFILE) \
 || defined(JSE_CLIB_TMPNAME) \
 || defined(JSE_CLIB_TOASCII) \
 || defined(JSE_CLIB_TOLOWER) \
 || defined(JSE_CLIB_TOUPPER) \
 || defined(JSE_CLIB_UNGETC) \
 || defined(JSE_CLIB_VA_ARG) \
 || defined(JSE_CLIB_VA_END) \
 || defined(JSE_CLIB_VA_START) \
 || defined(JSE_CLIB_VFSCANF) \
 || defined(JSE_CLIB_VFPRINTF) \
 || defined(JSE_CLIB_VPRINTF) \
 || defined(JSE_CLIB_VSCANF) \
 || defined(JSE_CLIB_VSPRINTF) \
 || defined(JSE_CLIB_VSSCANF)
#  define JSE_CLIB_ANY
#endif
/*****************
 * SCREEN        *
 ****************/

/* Check for JSE_SCREEN_ALL */
#if defined(JSE_SCREEN_ALL) && defined(__CENVI__)
#  if !defined(JSE_SCREEN_WRITE) 
#     define JSE_SCREEN_WRITE                 1
#  endif
#  if !defined(JSE_SCREEN_WRITELN) 
#     define JSE_SCREEN_WRITELN               1
#  endif
#  if !defined(JSE_SCREEN_CLEAR) 
#     define JSE_SCREEN_CLEAR                 1
#  endif
#  if !defined(JSE_SCREEN_CURSOR) 
#     define JSE_SCREEN_CURSOR                1
#  endif
#  if !defined(JSE_SCREEN_SIZE) 
#     define JSE_SCREEN_SIZE                  1
#  endif
#  if !defined(JSE_SCREEN_SETFOREGROUND) \
    && (defined(__JSE_WIN16__) || defined(__JSE_WIN32__)) 
#     define JSE_SCREEN_SETFOREGROUND         1
#  endif
#  if !defined(JSE_SCREEN_SETBACKGROUND) \
    && (defined(__JSE_WIN16__) || defined(__JSE_WIN32__)) 
#  define JSE_SCREEN_SETBACKGROUND         1
#endif
#endif /* JSE_SCREEN_ALL */
/* Convert zeros to undefines */
#if defined(JSE_SCREEN_WRITE) && JSE_SCREEN_WRITE == 0
#  undef JSE_SCREEN_WRITE
#endif
#if defined(JSE_SCREEN_WRITELN) && JSE_SCREEN_WRITELN == 0
#  undef JSE_SCREEN_WRITELN
#endif
#if defined(JSE_SCREEN_CLEAR) && JSE_SCREEN_CLEAR == 0
#  undef JSE_SCREEN_CLEAR
#endif
#if defined(JSE_SCREEN_CURSOR) && JSE_SCREEN_CURSOR == 0
#  undef JSE_SCREEN_CURSOR
#endif
#if defined(JSE_SCREEN_SIZE) && JSE_SCREEN_SIZE == 0
#  undef JSE_SCREEN_SIZE
#endif
#if defined(JSE_SCREEN_SETFOREGROUND) && JSE_SCREEN_SETFOREGROUND == 0
#  undef JSE_SCREEN_SETFOREGROUND
#endif
#if defined(JSE_SCREEN_SETBACKGROUND) && JSE_SCREEN_SETBACKGROUND == 0
#  undef JSE_SCREEN_SETBACKGROUND
#endif
/* Define generic JSE_SCREEN_ANY */
#if defined(JSE_SCREEN_WRITE) \
 || defined(JSE_SCREEN_WRITELN) \
 || defined(JSE_SCREEN_CLEAR) \
 || defined(JSE_SCREEN_CURSOR) \
 || defined(JSE_SCREEN_SIZE) \
 || defined(JSE_SCREEN_SETFOREGROUND) \
 || defined(JSE_SCREEN_SETBACKGROUND)
#  define JSE_SCREEN_ANY
#endif

/* Check for error condition */
#if defined(JSE_SCREEN) && !(defined(__CENVI__))
#  error Current options not compatible with library SCREEN
#endif

/*****************
 * TEST          *
 ****************/

/* Check for JSE_TEST_ALL */
#if defined(JSE_TEST_ALL)
#  if !defined(JSE_TEST_ASSERT) 
#     define JSE_TEST_ASSERT                  1
#  endif
#  if !defined(JSE_TEST_START) 
#     define JSE_TEST_START                   1
#  endif
#  if !defined(JSE_TEST_END) 
#     define JSE_TEST_END                     1
#  endif
#  if !defined(JSE_TEST_ASSERTNUMEQUAL) 
#     define JSE_TEST_ASSERTNUMEQUAL          1
#  endif
#  if !defined(JSE_TEST_SETATTRIBUTES) 
#     define JSE_TEST_SETATTRIBUTES           1
#  endif
#endif /* JSE_TEST_ALL */
/* Convert zeros to undefines */
#if defined(JSE_TEST_ASSERT) && JSE_TEST_ASSERT == 0
#  undef JSE_TEST_ASSERT
#endif
#if defined(JSE_TEST_START) && JSE_TEST_START == 0
#  undef JSE_TEST_START
#endif
#if defined(JSE_TEST_END) && JSE_TEST_END == 0
#  undef JSE_TEST_END
#endif
#if defined(JSE_TEST_ASSERTNUMEQUAL) && JSE_TEST_ASSERTNUMEQUAL == 0
#  undef JSE_TEST_ASSERTNUMEQUAL
#endif
#if defined(JSE_TEST_SETATTRIBUTES) && JSE_TEST_SETATTRIBUTES == 0
#  undef JSE_TEST_SETATTRIBUTES
#endif
/* Define generic JSE_TEST_ANY */
#if defined(JSE_TEST_ASSERT) \
 || defined(JSE_TEST_START) \
 || defined(JSE_TEST_END) \
 || defined(JSE_TEST_ASSERTNUMEQUAL) \
 || defined(JSE_TEST_SETATTRIBUTES)
#define JSE_TEST_ANY
#endif
/*****************
 * UUCODE        *
 ****************/

/* Check for JSE_UUCODE_ALL */
#if defined(JSE_UUCODE_ALL)
#  if !defined(JSE_UUCODE_ENCODE) 
#     define JSE_UUCODE_ENCODE                1
#  endif
#  if !defined(JSE_UUCODE_DECODE) 
#     define JSE_UUCODE_DECODE                1
#  endif
#endif /* JSE_UUCODE_ALL */
/* Convert zeros to undefines */
#if defined(JSE_UUCODE_ENCODE) && JSE_UUCODE_ENCODE == 0
#  undef JSE_UUCODE_ENCODE
#endif
#if defined(JSE_UUCODE_DECODE) && JSE_UUCODE_DECODE == 0
#  undef JSE_UUCODE_DECODE
#endif
/* Define generic JSE_UUCODE_ANY */
#if defined(JSE_UUCODE_ENCODE) || defined(JSE_UUCODE_DECODE)
#  define JSE_UUCODE_ANY
#endif

#endif /* __SELIBDEF_H */
