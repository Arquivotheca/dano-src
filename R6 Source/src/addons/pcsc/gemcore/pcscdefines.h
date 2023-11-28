#ifndef _pcscdefines_h_
#define _pcscdefines_h_

/*
 * NAME:
 *	pcscdefines.h - Copyright (C) 1998 David Corcoran
 *             corcordt@cs.purdue.edu
 *
 * DESCRIPTION:
 *      This defines a list of type defined variable types.
 *
 * AUTHOR:
 *	David Corcoran, 10/16/98
 *
 * LICENSE: See file LICENSE.
 *
 */


#include "config.h"

/* Defines a list of pseudo types. */

// typedef unsigned long      DWORD;
//#ifndef _DWORD
//typedef unsigned int DWORD; // added by atul
//#endif
//  typedef char*               STR;
//  typedef long               RESPONSECODE;


  typedef void               VOID;

  #define MAX_RESPONSE_SIZE  264
  #define MAX_MASK_SIZE      33
  #define MAX_ATR_SIZE       33

#endif
