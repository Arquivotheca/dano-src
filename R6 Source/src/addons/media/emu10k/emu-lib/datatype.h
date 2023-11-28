/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1994. All rights Reserved.
*
*******************************************************************************
*/

/*****************************************************************************
* $Id: datatype.h,v 1.21 1998/12/05 02:22:45 jfk Exp $
*
* Filename: datatype.h
*
* Description: Header file for Emu 8200 Data Types
*              For use with E-mu 8200 Sound Engine Software
*
* Person           Date            Reason
* ------------     ----------      -------------------------------
* MG                4/26/96        Add __BYTE_DONTCARE, __SYS_TMS320C32 
* MG               10/1/95         Distinguish Windows seperate from 16 bit
*                                  bounded systems for WinNT, Win95
* RSC              7/1/94          Added Windows preprocessor conditionals
* MW               3/3/95          Update for V2.0 data structs. UNIT and INT
*                                  replaced with WORD and SHORT respectivly..
* RSC              5/4/95          Update preprocessor conditionals and
*				   added VOIDPTR and BYTEPTR
*******************************************************************************
*/


#ifndef __DATATYPE_H
#define __DATATYPE_H

#include "baseconfig.h"

/************
* Includes
************/
#include <limits.h>  /* This is where limits checking values are */

#ifndef NULL
  #include <stddef.h>  /* This is where NULL is defined */
#endif

#if __BEOS__
  #define __BYTE_COHERENT
  #define __BOUND_32BIT
  #define __SYS_GENERAL
  #define __8010__

  /* for debugging */
  #define X(N) {printf("<%d>\n", N);}

/*
  #define NewAlloc(S) malloc(S)
  #define DeleteAlloc(P) free(P)
*/
#endif

/*****************************************
* Datatype specific compile error checking
*****************************************/

/********************************************************************
* 1. Check system dependent flags
*
* Datatype compensates for system differences w/r/t endian nature and memory
* boundaries. Compile flags allow automatic switches within the 
* codebase to account for differences between systems in these areas.
* This checks to make sure Datatype is compiled with the proper compile
* flags set. Unfortunately, there is no universal way to have a compiler
* display a custom warning, so we must trigger an error if the 
* proper compile flags are not set.
********************************************************************/

/* Endian nature considerations */
#if !defined (__BYTE_INCOHERENT) && !defined (__BYTE_COHERENT)
#  ifndef __BYTE_DONTCARE
/*   If you get an error here, set one of the above defines as a global 
     compile flag. These flags indicate ENDIAN NATURE of your system.   */
#    error Datatype.H: No Endian Nature flag assigned
#  endif
#endif

/* System memory boundary considerations */
#if !defined (__BOUND_16BIT) && !defined (__BOUND_32BIT)
/* If you get an error here, set one of the above defines as a global compile
   flag. These flags indicate the MEMORY BOUNDARY of your system. */
#  error Datatype.H: No Memory boundary flag assigned
#endif

/* System type considerations */
#if !defined (__SYS_GENERAL) && !defined (__SYS_WINDOWS) 
# if !defined (__SYS_MSDOS) && !defined (__SYS_MACINTOSH) 
#  if !defined (__SYS_UNIX) && !defined (__SYS_TMS320C32)
#    if !defined (__SYS_SGI)
     /* If you get an error here, set one of the above defines as a global compile
        flag. These flags SYSTEM SPECIFIC considerations of your system. */
#     error Datatype.H: No System type flag assigned
#    endif
#  endif
# endif
#endif

/********************************************************************
* 2. Check basic datatype limits
*
* Compare <limits.h> values with E-mu 8200 limits.  Fail compile
* if discrepency. If compile fails due to one of these errors, then
* the E-mu 8200 will not work with your system or the state of your
* development environment.
********************************************************************/

#ifdef __SYS_TMS320C32
#  define DATATYPE_TEST_32BIT_ONLY
#endif

/* Expected data values */
#define CHAR_MINVAL  -128
#define CHAR_MAXVAL   127
#define BYTE_MAXVAL   255
#define SHRT_MINVAL  -32768
#define SHRT_MAXVAL   32767
#define LONG_MINVAL  -2147483648L
#define LONG_MAXVAL   2147483647L
#define DWORD_MAXVAL  4294967295L


#ifndef DATATYPE_TEST_32BIT_ONLY
#  if (SCHAR_MAX != CHAR_MAXVAL) || (UCHAR_MAX != BYTE_MAXVAL)
#   error Datatype.H: char is out of range! Expected 8 bit.
#  endif

#  if (SHRT_MAX != SHRT_MAXVAL)
#   error Datatype.H: short is out of range! Expected 16 bit.
#  endif
#endif

#if (LONG_MAX != LONG_MAXVAL)
# error Datatype.H: long is out of range! Expected 32 bit.
#endif

/******************************
 * Defines and other switches
 *****************************/

/* Switches based on system memory boundaries and system types */
#ifdef __BOUND_16BIT
#  if defined (__SYS_WINDOWS) || defined (__SYS_MSDOS)
#    define PTR_32BIT huge
#  else
#    define PTR_32BIT
#  endif
#elif defined (__BOUND_32BIT)
#  define PTR_32BIT
#endif

/* System header file considerations */

#ifdef __SYS_WINDOWS
#  ifdef __EMU_WIN_DLL
#    ifdef __BOUND_16BIT
#      define EMUEXPORT _export
#    else
#      define EMUEXPORT __declspec(dllexport)
#    endif
#  else
#    define EMUEXPORT
#  endif
#  if !defined (VXD) && !defined (VTOOLSD)
#    include <windows.h>
#  else
#    ifdef VXD
#      include <basedef.h>
#    elif defined (VTOOLSD)
#      include <string.h>
#      ifdef __cplusplus
#        include <vtoolscp.h>
#      else
#        include <vtoolsc.h>
#      endif
#    endif
#  endif
#else
#  define EMUEXPORT
#endif

#ifdef __SYS_MSDOS
#  include <dos.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#endif

/* Debug assertions */

#ifdef DEBUG

# if defined(__SYS_WINDOWS) || defined(__SYS_MSDOS)


#   ifdef DEBUG_DISPLAY
#     ifdef VTOOLSD 
#       define DPRINTF(_x) dprintf _x
#     else
#       include <stdio.h>
#       define DPRINTF(_x)  printf _x
#     endif
#   else
#    define DPRINTF(_x)  
#   endif

#   if !defined(VTOOLSD)
#     include <assert.h>
#     define ASSERT(_x) assert(_x)
#   endif

# else /* Not PC based, just turn everything off */

#   define DPRINTF(_x)
#   define ASSERT(_x)
# endif  /* General non-PC case */

/* The following values are system-independent DEBUG definitions */
# define RETURN_ERROR(_x) \
	{ if ((_x) != SUCCESS) ASSERT(0); return (_x); }

#else /* !defined(DEBUG) */

# define DPRINTF(_x) 
# define ASSERT(_x) 
# define RETURN_ERROR(_x) { return(_x); }

#endif

/* 
   If you are developing in C, please add this to the prototypes of all API 
   calls so that folks who use C++ may be able to use your code without
   writing their code any special way.

   If you are developing in C++, please add this to the prototypes of 
   all classic C function type API calls so that folks who use C code may
   be albe to use your code without recompiling your code.

   IE: 
   EMUCTYPE void ApiCall(WORD val1, WORD val2);
   EMUCTYPE WORD * GetPtr(WORD val);
   etc.

   Or you could use the "series" macros to define a whole list of functions
   in one shot.

   IE:
   BEGINEMUCTYPE
   void ApiCal(WORD val1, WORD val2);
   WORD * GetPtr(WORD val);
   ENDEMUCTYPE

   These macros act as compile time switchable 'extern "C"' commands
   based on C or C++ compiler.
*/

#ifdef __cplusplus
#define EMUCTYPE extern "C"
#define BEGINEMUCTYPE EMUCTYPE {
#define ENDEMUCTYPE   }
#else
#define EMUCTYPE
#define BEGINEMUCTYPE
#define ENDEMUCTYPE
#endif

/*************
* Typedefs
*************/

typedef char                CHAR;      /*  8 bit signed value          */
typedef short               SHORT;     /* 16 bit signed value was: INT */

  /*****************************************************************
   * These datatypes and macros are already included with <windows.h>
  *****************************************************************/

#ifndef __SYS_WINDOWS
  typedef unsigned char       BYTE;    /*  8 bit unsigned value   */
  typedef short               BOOL;    /* 16 bit signed value     */
  typedef unsigned short      WORD;    /* 16 bit signed value     */
  typedef unsigned short      USHORT;
  typedef long                LONG;    /* 32 bit signed value     */
  typedef unsigned long       DWORD;   /* 32 bit unsigned value   */
  typedef unsigned long       ULONG;
#  define LOBYTE(x)  ((x) & 0x00FF)
#  define HIBYTE(x)  (((x) & 0xFF00) >> 8)
#endif


#ifdef __SYS_MACINTOSH
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  define  MAX_PATH  1024
#endif

  typedef float               FLOAT;   /* 32 bit floating point value */
  typedef double              DOUBLE;  /* 64 bit floating point value */

#ifndef __SYS_SGI
  typedef long double         LDOUBLE; /* 80 bit floating point value */
#endif

/* Take care of typically used macros, if not defined under
   the various operating systems */

#ifndef FALSE
#  define FALSE  (0)
#endif

#ifndef TRUE
#  define TRUE   (1)
#endif

#ifndef NULL
#  define NULL   (0)
#endif

   /*****************************************************************
   * These idiosyncratic pointer definitions for memory allocations
   * which are greater than 64K and are Intel-centric compiling
   * environment necessities, at least for Windows environments.
   * Note, however, that with 32 bit bounded systems, the PTR_32BIT
   * has no definition, thus making these simple pointers.
   *****************************************************************/
typedef BYTE  PTR_32BIT* BYTEPTR;
typedef CHAR  PTR_32BIT* CHARPTR;
typedef WORD  PTR_32BIT* WORDPTR;
typedef WORD  PTR_32BIT* UINTPTR;
typedef SHORT PTR_32BIT* SHORTPTR;
typedef LONG  PTR_32BIT* LONGPTR;
typedef DWORD PTR_32BIT* DWORDPTR;
typedef void  PTR_32BIT* VOIDPTR;

   /********************************************************************
   * This 16 bit unsigned value is used for routines which return
   * standard E-mu error codes (see emuerrs.h)
   ********************************************************************/
typedef unsigned short        EMUSTAT;

#ifndef __BYTE_DONTCARE

   /********************************************************************
   * Data structures and methods to handle endian nature diffeneces
   * between archetectures
   ********************************************************************/

/**** For Little Endian Systems ****/

#ifdef __BYTE_COHERENT

typedef struct twoBytesTag
{
  BYTE by0;
  BYTE by1;
} twoBytes;


typedef struct fourBytesTag
{
  BYTE by0;
  BYTE by1;
  BYTE by2;
  BYTE by3;
} fourBytes;


typedef struct twoWordsTag
{
  WORD w0;
  WORD w1;
} twoWords;

/**** For Big Endian Systems ****/

#elif defined(__BYTE_INCOHERENT)

typedef struct twoBytesTag
{
  BYTE by1;
  BYTE by0;
} twoBytes;


typedef struct fourBytesTag
{
  BYTE by3;
  BYTE by2;
  BYTE by1;
  BYTE by0;
} fourBytes;


typedef struct twoWordsTag
{
  WORD w1;
  WORD w0;
} twoWords;
 
#endif  /*  BYTE COHERENCY  */


typedef union twoByteUnionTag
{
  twoBytes byVals;
  WORD     wVal;
} twoByteUnion;


typedef union fourByteUnionTag  /* make the three 32 bit definitions  */
{                               /* interchangeable                    */
  fourBytes byVals;
  twoWords  wVals;
  DWORD     dwVal;
} fourByteUnion;

#endif /* __BYTE_DONTCARE */

   /*********************************
    * Byte swapping inline methods 
    *********************************/

/* This chip cannot use unions for byte swapping... */

#ifndef __SYS_TMS320C32

   /************************************************************
    * These are the VARIABLES which are used for byte swapping
    ***********************************************************/

#define SWAP_WORD_DECLARATIONS \
  twoByteUnion tbUnion;        \
  twoByteUnion *ptbUnion;
  
#define SWAP_DWORD_DECLARATIONS \
  fourByteUnion fbUnion;        \
  fourByteUnion *pfbUnion;
  
   /************************************************************
    * These are the GENERIC byte software macros. Note they use
    * the variables defined above!
    ***********************************************************/

#define SWAP_WORD(wValue)                               \
{                                                       \
  tbUnion.wVal            = wValue;                     \
  ptbUnion                = (twoByteUnion *)((WORD *)(&(wValue)));\
  ptbUnion->byVals.by0    = tbUnion.byVals.by1;         \
  ptbUnion->byVals.by1    = tbUnion.byVals.by0;         \
}

#define SWAP_DWORD(dwValue)                            \
{                                                      \
  fbUnion.dwVal        = dwValue;                      \
  pfbUnion             = (fourByteUnion *)((DWORD *)(&(dwValue)));\
  pfbUnion->byVals.by0 = fbUnion.byVals.by3;           \
  pfbUnion->byVals.by1 = fbUnion.byVals.by2;           \
  pfbUnion->byVals.by2 = fbUnion.byVals.by1;           \
  pfbUnion->byVals.by3 = fbUnion.byVals.by0;           \
}


#else  /* For the TMS system, these may need to be 
          defined at a later date */

#define SWAP_WORD_DECLARATIONS
#define SWAP_DWORD_DECLARATIONS
#define SWAP_WORD(wValue)
#define SWAP_DWORD(dwValue)

#endif /* Byte swappings based on system */

   /************************************************************
    * These are DEFINES to allow one to have byte swapping
    * routines exist or not exist based on ENDIAN NATURE.
    * (IE byte swap ONLY if LITTLE ENDIAN system).
    ***********************************************************/

   /********************************************************************
    * Byte Coherent (little endian) specific byte swapping methods 
    *******************************************************************/
#ifdef __BYTE_COHERENT

/* For little endian systems, the BIG ENDIAN methods are NOT defined */
#define SWAP_WORD_BYTE_INCOH_DECLARATIONS 
#define SWAP_DWORD_BYTE_INCOH_DECLARATIONS
#define SWAP_WORD_BYTE_INCOH_ONLY(a) 
#define SWAP_DWORD_BYTE_INCOH_ONLY(a) 

/* For little endian systems, the LITTLE ENDIAN methods are defined as the
   generic methods */
#define SWAP_WORD_BYTE_COH_DECLARATIONS  SWAP_WORD_DECLARATIONS  
#define SWAP_DWORD_BYTE_COH_DECLARATIONS  SWAP_DWORD_DECLARATIONS
#define SWAP_WORD_BYTE_COH_ONLY(a) SWAP_WORD(a)
#define SWAP_DWORD_BYTE_COH_ONLY(a) SWAP_DWORD(a)


   /********************************************************************
    * Byte Incoherent (big endian) specific byte swapping methods 
    *******************************************************************/
#elif defined (__BYTE_INCOHERENT)

/* For big endian systems, the BIG ENDIAN methods are defined as the
   generic methods */
#define SWAP_WORD_BYTE_INCOH_DECLARATIONS SWAP_WORD_DECLARATIONS
#define SWAP_DWORD_BYTE_INCOH_DECLARATIONS SWAP_DWORD_DECLARATIONS
#define SWAP_WORD_BYTE_INCOH_ONLY(a) SWAP_WORD(a)
#define SWAP_DWORD_BYTE_INCOH_ONLY(a) SWAP_DWORD(a)

/* For big endian systems, the LITTLE ENDIAN methods are NOT defined */
#define SWAP_WORD_BYTE_COH_DECLARATIONS
#define SWAP_DWORD_BYTE_COH_DECLARATIONS
#define SWAP_WORD_BYTE_COH_ONLY(a)
#define SWAP_DWORD_BYTE_COH_ONLY(a)


   /********************************************************************
    * No endian nature considered: don't define any endian nature 
    * switchable byte swapping macros
    *******************************************************************/

#elif defined (__BYTE_DONTCARE)

#define SWAP_WORD_BYTE_INCOH_DECLARATIONS 
#define SWAP_DWORD_BYTE_INCOH_DECLARATIONS
#define SWAP_WORD_BYTE_INCOH_ONLY(a) 
#define SWAP_DWORD_BYTE_INCOH_ONLY(a) 

#define SWAP_WORD_BYTE_COH_DECLARATIONS
#define SWAP_DWORD_BYTE_COH_DECLARATIONS
#define SWAP_WORD_BYTE_COH_ONLY(a)
#define SWAP_DWORD_BYTE_COH_ONLY(a)

#endif /* Endian nature */

#endif /* __DATATYPE_H */
