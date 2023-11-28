/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: dbg.h
 *   project : <none>
 *   author  : Martin Sieler
 *   date    : 1999-02-02
 *   contents/description: debug printout
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/05/10 12:32:30 $
 * $Header: /home/cvs/mms/common/util/dbg.h,v 1.2 1999/05/10 12:32:30 sir Exp $
 */

#ifndef __DBG_H__
#define __DBG_H__

/* ------------------------ includes --------------------------------------*/

/*-------------------------- defines --------------------------------------*/

#ifndef REMIND
  //  REMIND macro - generates warning as reminder to complete coding
  //  (eg) usage:
  //
  //  #pragma message (REMIND("Add automation support"))
  #define QUOTE(x) #x
  #define QQUOTE(y) QUOTE(y)
  #define REMIND(str) __FILE__ "(" QQUOTE(__LINE__) "): " str
#endif

/*-------------------------------------------------------------------------*/

#ifndef WIN32
  // only for Windows
  #undef DBG_ENABLE
#endif

#ifdef DBG_ENABLE
  //
  // debug printout enabled
  //
  void sgDbg(int nLevel, const char*, ...);
  
  void DbgInit(const char *pszModule);
  void DbgSetLevel(int nLevel);
  #define Dbg sgDbg

#else
  //
  // debug printout disabled
  //
  inline void sgDbg(int, const char*, ...) { }

  #define DbgInit(_x_)
  #define DbgSetLevel(_x_)
  #define Dbg 1 ? (void)0 : sgDbg

#endif

/*-------------------------------------------------------------------------*/
#endif
