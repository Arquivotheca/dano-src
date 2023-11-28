/* setxtlib.h   All access to text strings.
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

#if defined(_TEXTLIB_CPP)
#  if !defined(_TEXTLIB_H)
#    error MUST INCLUDE TEXTLIB.H BEFORE DEFINING TEXTLIB_CPP, AND THEN AGAIN AFTER DEFINING _TEXTLIB_CPP
#  endif
#  undef _TEXTLIB_H
#endif

#ifndef _TEXTLIB_H
#  define _TEXTLIB_H

#  ifdef __cplusplus
extern "C" {
#  endif

#if defined(_TEXTLIB_CPP)
#  undef   TL_RESOURCE
#  if !defined(JSE_SHORT_RESOURCE) || (0==JSE_SHORT_RESOURCE)
#     define TL_RESOURCE(ID,SHORT,DETAILS) UNISTR(SHORT) UNISTR(DETAILS),
#  else
#     define TL_RESOURCE(ID,SHORT,DETAILS) UNISTR(SHORT),
#  endif
#  undef   TL_TEXT_STRING
#  define  TL_TEXT_STRING(ID,STRING)     CONST_DATA(jsechar) textlib##ID##[] = UNISTR(STRING);
#else
#  define  TL_RESOURCE(ID,SHORT,DETAILS) textlib##ID##,
#  undef   TL_TEXT_STRING
#  define  TL_TEXT_STRING(ID,STRING)     extern CONST_DATA(jsechar) textlib##ID##[];
#endif


#if defined(_TEXTLIB_CPP)
   CONST_DATA(jsechar *) textlibStrings[TEXTLIB_ID_COUNT] = {
            UNISTR("Resource String Not Found."),
#else
   enum textlibID {
            TL_RESOURCE_STRING_NOT_FOUND = 0,
#endif

/********** This is the basic concept behind the error number scheme. ********
   *
   * 0xxx:  Internal CENVI errors.
   *  00xx:  Finding source stuff.
   *  01xx:  Binding errors
   *
   * 1xxx:  Language errors
   *  10xx:  Preprocessor errors
   *  11xx:  Parsing errors
   *  12xx:  Parse error; missing a piece of a loop or something.
   *  13xx:  Misc parsing errors.
   *  14xx:  Function declaration stuff.
   *  15xx:  Expression evaluation errors
   *  16xx:  Data type errors
   *  17xx:  Math errors
   *
   * 5xxx:  Library errors
   *  50xx:  Parameter types
   *
   * 6xxx - 7xxxx:  Library error messages
   *  60xx:  Common
   *  61xx:  SElib
   *  62xx:  Clib
   *  63xx:  Ecma
   *  64xx:  Lang
   *  65xx:  Unix
   *  66xx:  Win
   *  67xx:  Dos
   *  68xx:  Mac
   *  69xx:  OS2
   *  70xx:  NLM
   *  71xx:  Link libraries (md5,gd,rx)
   *
   * 8xxx:  Misc.
   *  81xx: Security
   *
   * 9xxx:  Debug version messages
   *  90xx:  Memory stuff
   *  91xx:  Unimplemented hooks
   *  92xx:  We're confused.
   *  99xx:  I haven't a clue what these mean
   *
   ******************************************************************************/

#  if defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) || defined(__JSE_WIN32__) || defined(__JSE_CON32__)
      TL_RESOURCE(INSUFFICIENT_MEMORY,"0003",": Insufficient Memory to continue operation.")
#  endif

#  if defined(JSE_CLIB_LDIV) || \
      defined(JSE_CLIB_DIV)
   TL_RESOURCE(CANNOT_DIVIDE_BY_ZERO,"1702",": Cannot divide by zero.")
#  endif

   /*** Common ****/
#if defined(JSE_SELIB_BLOB_SIZE) || \
    defined(JSE_LANG_SETARRAYLENGTH)
   TL_RESOURCE(BAD_MAX_ARRAY_LENGTH,"6000",": Array length must be 0 or positive.")
#endif
#if defined(JSE_SELIB_BLOB_GET) || \
    defined(JSE_SELIB_BLOB_PUT) || \
    defined(JSE_SELIB_BLOB_SIZE) || \
    defined(JSE_CLIB_FREAD)   || \
    defined(JSE_CLIB_FWRITE)  || \
    defined(JSE_SELIB_PEEK)    || \
    defined(JSE_SELIB_POKE)
   TL_RESOURCE(INVALID_DATA_DESCRIPTION,"6001",": Blob data description variable is invalid for data conversion.")
   TL_RESOURCE(INVALID_BLOB_DESC_MEMBER,"6002",": Invalid data type for a member of a %s.")
#endif
#  if defined(JSE_SELIB_DYNAMICLINK) || defined(JSE_OS2_PMDYNAMICLINK)
      TL_RESOURCE(DYNA_CANNOT_LOAD_MODULE,"6003",": Cannot load %s \"%s\" Error code: %d.")
      TL_RESOURCE(DYNA_CANNOT_FIND_NAMED_SYM,"6004",": Cannot find %s \"%s\" in %s \"%s\"; Error code: %d.")
      TL_RESOURCE(DYNA_CANNOT_FIND_ORDINAL_SYM,"6005",": Cannot find %s %d in %s \"%s\"; Error code: %d.")
      TL_RESOURCE(DYNA_INVALID_BIT_SIZE,"6006",": Invalid bit size for dynamic link call, must be BIT16 or BIT32.")
#     if defined(__JSE_WIN16__)
         TL_RESOURCE(DYNA_INVALID_DYNA_RETURN_TYPE,"6007",": Invalid return type for dynamic link call.")
#     endif
      TL_RESOURCE(DYNA_INVALID_CALLING_CONVENTION,"6008",": Invalid calling convention for dynamic link call.")
      TL_RESOURCE(DYNA_BAD_PARAMETER,"6009",": Bad parameter %d for dynamic link call")
#  endif
   /*** SElib ***/
#if defined(JSE_SELIB_SPAWN)
   TL_RESOURCE(INVALID_SPAWN_MODE,"6100",": Unrecognized spawn() mode %d.")
   TL_RESOURCE(SPAWN_CANNOT_CONVERT_TO_STRING,"6101",": Cannot convert the %d spawn parameter to a string.")
#endif
#if defined(JSE_SELIB_BLOB_GET)
   TL_RESOURCE(BLOB_GET_INVALID_SIZE,"6102",": The blob is not big enough to contain data; offset too low or size too big.")
#endif
#if defined(JSE_SELIB_INTERPRET)
   TL_RESOURCE(NOT_TOKEN_BUFFER,"6103",": This does not appear to be a compiled script")
   TL_RESOURCE(CANNOT_READ_TOKEN_FILE,"6104", ": Compiled scripts cannot be read from a file")
#endif
   /*** Clib ***/
#if defined(JSE_CLIB_FREOPEN)  || \
    defined(JSE_CLIB_FOPEN)    || \
    defined(JSE_CLIB_FPRINTF)  || \
    defined(JSE_CLIB_FSCANF)   || \
    defined(JSE_CLIB_FPUTS)    || \
    defined(JSE_CLIB_FGETS)    || \
    defined(JSE_CLIB_VFPRINTF) || \
    defined(JSE_CLIB_FCLOSE)   || \
    defined(JSE_CLIB_FLOCK)    || \
    defined(JSE_CLIB_FSEEK)    || \
    defined(JSE_CLIB_FTELL)    || \
    defined(JSE_CLIB_FGETC)    || \
    defined(JSE_CLIB_UNGETC)   || \
    defined(JSE_CLIB_FPUTC)    || \
    defined(JSE_CLIB_VFSCANF)  || \
    defined(JSE_CLIB_TMPFILE)  || \
    defined(JSE_CLIB_FFLUSH)   || \
    defined(JSE_CLIB_FREAD)    || \
    defined(JSE_CLIB_FWRITE)   || \
    defined(JSE_CLIB_FGETPOS)  || \
    defined(JSE_CLIB_FSETPOS)  || \
    defined(JSE_CLIB_CLEARERROR)  || \
    defined(JSE_CLIB_REWIND)   || \
    defined(JSE_CLIB_FEOF)     || \
    defined(JSE_CLIB_FERROR)   || \
    defined(JSE_CLIB_PRINTF)   || \
    defined(JSE_CLIB_GETCH)    || \
    defined(JSE_CLIB_GETCHE)   || \
    defined(JSE_CLIB_KBHIT)    || \
    defined(JSE_CLIB_FPRINTF)  || \
    defined(JSE_CLIB_VPRINTF)  || \
    defined(JSE_CLIB_VFPRINTF) || \
    defined(JSE_CLIB_GETS)     || \
    defined(JSE_CLIB_GETCHAR)  || \
    defined(JSE_CLIB_PUTCHAR)  || \
    defined(JSE_CLIB_PERROR)
   TL_RESOURCE(INVALID_FILE_VAR,"6200",": File variable is not valid")
#endif
#if   defined(JSE_CLIB_PRINTF)    || \
      defined(JSE_CLIB_FPRINTF)   || \
      defined(JSE_CLIB_VPRINTF)   || \
      defined(JSE_CLIB_SPRINTF)   || \
      defined(JSE_CLIB_VSPRINTF)  || \
      defined(JSE_CLIB_RVSPRINTF) || \
      defined(JSE_CLIB_SYSTEM)
   TL_RESOURCE(UNKNOWN_FORMAT_SPECIFIER,"6201",": Unknown Format Type Specifier \"%c\" in ?printf string.")
   TL_RESOURCE(ZERO_WIDTH_IS_INVALID,"6202",": Zero width is invalid.")
   TL_RESOURCE(SCANF_BRACKET_NOT_FOUND,"6203",": \"]\" character not found in scanf format string.")
   TL_RESOURCE(SCANF_TYPE_UNKNOWN,"6204",": Unrecognized Type character \"%c\" in ?scanf format.")
#endif
#if   defined(JSE_CLIB_ASSERT)
   TL_RESOURCE(ASSERTION_FAILED,"6205",": Assertion Failed:")
#endif
#  if defined(JSE_CLIB_VA_ARG)    || \
      defined(JSE_CLIB_VA_START)  || \
      defined(JSE_CLIB_VA_END)    || \
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
      defined(JSE_CLIB_VSSCANF)
   TL_RESOURCE(INVALID_VA_LIST,"6206",": Invalid VA_LIST.")
   TL_RESOURCE(VA_VAR_NOT_FOUND,"6207",": Variable not in va_xxx parameter list")
#endif
#if defined(JSE_CLIB_FSETPOS)
   TL_RESOURCE(INVALID_FPOS_T,"6208",": Invalid fpos_t structure to fsetpos().")
#endif
#if defined(JSE_CLIB_QSORT)
   TL_RESOURCE(QSORT_MUST_RETURN_INTEGER,"6209",": qsort compare function must return an integer.")
   TL_RESOURCE(QSORT_ELEMENT_COUNT_TOO_BIG,"6210",": qsort element count is bigger than array.")
#endif
   /*** Ecma ***/
   /*** Lang ***/
#if defined(JSE_LANG_SETARRAYLENGTH)
   TL_RESOURCE(BAD_MIN_ARRAY_SPAN,"6400",": Array span Minimum index must be 0 or negative.")
#endif
   /*** Unix ***/
#  if defined(__JSE_UNIX__)
      TL_RESOURCE(NO_SHARED_OBJECT,"6500",": Couldn't open shared object \"%s\".")
      TL_RESOURCE(NO_SHARED_SYMBOL,"6501",": Couldn't find symbol \"%s\" in shared object \"%s\".")
      TL_RESOURCE(UNIX_BAD_ARRAY,"6502",": Parameter %d must be a single dimensional array.")
#  endif
   /*** Win ***/
#  if defined(__JSE_WIN32__) || defined(__JSE_CON32__)
      TL_RESOURCE(CANNOT_LOAD_MODULE,"6600",": Error %d loading dll module \"%s\".")
      TL_RESOURCE(DOS_QUERY_PROC_ADDRESS,"6601",": Error %d getting procedure address.")
      TL_RESOURCE(NO_THREAD_ID,"6602",": Couldn't get thread id for window.")
#  endif
#  if defined(__JSE_WIN16__) || defined(__JSE_WIN32__)
      TL_RESOURCE(NOT_ENOUGH_TIMERS,"6603",": Not enough timer resources available for Suspend().")
#  endif
#  if defined(__JSE_WIN16__)
      TL_RESOURCE(CANNOT_ALLOCATE_SELECTOR,"6604",": Cannot allocated selector.")
      TL_RESOURCE(CANNOT_MYSPAWN,"6605",": Error starting task, check paths, filenames, and SEWINDOS.COM location.")
#  endif
   /*** Dos ***/
   /*** Mac ***/
   /*** OS2 ***/
#  if defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
      TL_RESOURCE(CANNOT_LOAD_MODULE,"6900",": Error %d loading dll module \"%s\".")
      TL_RESOURCE(SOM_METHOD_NO_WORKY,"6901",": somMethod() doesn't work in this beta.  Sorry.")
      TL_RESOURCE(DOSQPROCSTATUS_ERROR,"6902",": DosQProcStatus() error %d.")
      TL_RESOURCE(ENTRY_IS_CALLGATE,"6903",": Specified procedure may only be accessed via the CallGate form of DynamicLink().")
      TL_RESOURCE(DOS_QUERY_PROC_ADDRESS,"6904",": Error %d getting procedure address.")
#  endif
#  if defined(__JSE_OS2TEXT__)
      TL_RESOURCE(CANNOT_START_CENVI2PM,"6905",": Error %d: Unable to start SEOS22PM.exe.")
      TL_RESOURCE(CENVI2PM_SEMAPHORE_ERROR,"6906",": Error %d: Unable to communicate with SEOS22PM.exe.")
#  endif
   /*** NLM ***/
#  ifdef __JSE_NWNLM__
      TL_RESOURCE(NETWARE_UNRECOGNIZED_CHAR_INI,"7000",": .ini Line %d: Unrecognized character in Netware initialization file - %c.\n")
      TL_RESOURCE(NETWARE_MISSING_TYPE,"7001",": .ini Line %d: A variable's type must be specified.\n")
      TL_RESOURCE(NETWARE_MISSING_IDENTIFIER,"7002",": .ini Line %d: Missing function name.\n")
      TL_RESOURCE(NETWARE_MISSING_OPEN_PAREN,"7003",": .ini Line %d: Opening parenthesis expected.\n")
      TL_RESOURCE(NETWARE_MISSING_CLOSE_PAREN,"7004",": .ini Line %d: Closing parenthesis expected.\n")
      TL_RESOURCE(NETWARE_NO_VOID,"7005",": .ini Line %d: Argument type VOID not meaningful.\n")
      TL_RESOURCE(NETWARE_TOO_MANY_ARGS,"7006",": .ini Line %d: Too many arguments.\n")
      TL_RESOURCE(NETWARE_MISSING_SEMICOLON,"7007",": .ini Line %d: Missing semicolon.\n")
      TL_RESOURCE(NETWARE_GARBAGE,"7008",": .ini Line %d: Garbage after legal declaration.\n")
      TL_RESOURCE(NETWARE_BAD_INDIRECT,"7009",": .ini Line %d: Only integral types can be indirected.\n")
      TL_RESOURCE(NETWARE_BADNAME,"7010",": .ini Line %d: Only 'SEDESKPATH = value' is supported.\n")
      TL_RESOURCE(NETWARE_BADEQUAL,"7011",": .ini Line %d: Only 'SEDESKPATH = value' is supported.\n")
      TL_RESOURCE(NETWARE_BAD_SYMBOL,"7012",": Unable to import Netware symbol \"%s\".")
      TL_RESOURCE(NETWARE_UNKNOWN_IMPORT,"7013",": Function \"%s\" does not exist in Netware or any loaded NLM.")
      TL_RESOURCE(NETWARE_STUCK_SYMBOL,"7014",": Unable to remove Netware symbol \"%s\".")
      TL_RESOURCE(NETWARE_BAD_PARAM,"7015",": Unable to send parameter %d to NLMLink functions.")
      TL_RESOURCE(NETWARE_ON_CONSOLE,"7016",": Input is not allowed on Netware System Console.")
      TL_RESOURCE(NETWARE_PANIC_CORRUPT,"7017",": PANIC! Netware dispatcher internal table corrupt!")
      TL_RESOURCE(NETWARE_PANIC_UNKNOWN,"7018",": PANIC! Netware dispatcher called with non-Netware function!")
#  endif

#  ifdef JSE_MD5_ALL
      TL_RESOURCE(MD5_NOT_VALID_HANDLE,"7100",": First parameter is not a valid md5 handle")
#  endif

#  if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
      TL_RESOURCE(NO_INSECURE_WITHOUT_SECURITY,"8002",": No security file running. InSecurity() not valid.")
#  endif

#  ifndef NDEBUG
      TL_RESOURCE(VARARGS_CTYPE_UNDEFINED,"9203",": CType %d is undefined.")
#  endif


#  if defined(_TEXTLIB_CPP)
      };

#  else
            TEXTLIB_ID_COUNT
         };
#  endif

   TL_TEXT_STRING(PathEnvironmentVariable,"PATH")

#  if defined(JSE_ECMA_BOOLEAN)
   TL_TEXT_STRING(strEcmaFALSE,"false")
   TL_TEXT_STRING(strEcmaTRUE,"true")
#endif

#  if defined(JSE_CLIB_ANY)
   TL_TEXT_STRING(Clib,"Clib")
#  endif
#  if defined(JSE_SELIB_ANY)
   TL_TEXT_STRING(SElib,"SElib")
#  endif
#  if defined(JSE_UNIX_ANY)
   TL_TEXT_STRING(Unix,"Unix")
#endif
#  if defined(JSE_DOS_ANY)
   TL_TEXT_STRING(Dos,"Dos")
#endif
#  if defined(JSE_MAC_ANY)
   TL_TEXT_STRING(Mac,"Mac")
#endif

#  if !defined(_TEXTLIB_CPP)

#    define textlibGet(id) (textlibStrings[id])

      extern CONST_DATA(jsechar *) textlibStrings[TEXTLIB_ID_COUNT];

#  endif

#  ifdef __cplusplus
}
#  endif

#endif
