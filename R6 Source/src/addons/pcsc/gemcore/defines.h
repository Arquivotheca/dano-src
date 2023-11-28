#ifndef _defines_h_
#define _defines_h_

/*
 * $Id: defines.h,v 1.2 1999/03/03 15:34:09 corcoran Exp $
 *
 * NAME:
 *	defines.h -- Copyright (C) 1998 David Corcoran
 *                corcordt@cs.purdue.edu
 *
 * DESCRIPTION:
 *       Some global definitions and typedefs
 *
 * AUTHOR:
 *	David Corcoran, 3/17/98
 *
 *	Modified for Macintosh by Mark Hartman
 */

#include "config.h"

/*
 * Boolean constants
 */

#ifndef TRUE
  #define TRUE	1
  #define FALSE	0
#endif

/*
 * Type definitions
 */

//#ifndef _BYTE
//  typedef unsigned char BYTE;
//#endif

#ifndef HANDLE

 #ifdef CPU_MAC_PPC
   typedef Handle HANDLE;
 #endif

 #ifdef CPU_ICAP_PC
   typedef int HANDLE;
 #endif

 #ifdef CPU_SUN_SPARC
   typedef int HANDLE;
 #endif

#endif

//#ifndef bool
// typedef short bool;
//#endif

/*
 * Simple Max Defines
 */

  #define MAX_BUFFER_SIZE    264
  #define STATUS_SIZE        2

/*
 * Compiler dependencies
 */

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#endif
