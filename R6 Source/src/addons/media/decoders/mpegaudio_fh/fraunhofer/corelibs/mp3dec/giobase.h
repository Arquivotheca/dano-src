/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: giobase.h
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1998-02-11
 *   contents/description: HEADER - basic I/O class for MPEG Decoder
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 08:47:39 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/giobase.h,v 1.4 1999/02/16 08:47:39 sir Exp $
 */

#ifndef __GIOBASE_H__
#define __GIOBASE_H__

/* ------------------------ includes --------------------------------------*/

#include "mp3sscdef.h"

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

class CGioBase
{
public:
  virtual ~CGioBase() {}

  virtual SSC  Read(void *pBuffer, int cbToRead, int *pcbRead) = 0;
  virtual bool IsEof() const = 0;

protected:

private:

};

/*-------------------------------------------------------------------------*/
#endif
