/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: mp3ssc.h
 *   project : ---
 *   author  : Martin Sieler
 *   date    : 1999-02-15
 *   contents/description: ssc helper class (Structured Status Code)
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 09:15:08 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3ssc.h,v 1.1 1999/02/16 09:15:08 sir Exp $
 */

#ifndef __MP3SSC_H__
#define __MP3SSC_H__

/* ------------------------ includes --------------------------------------*/

#include "mp3sscdef.h"

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

/** Helper class for more information about SSC codes.
*/
class CMp3Ssc
{
public:
  /** Object constructor

    @param An SSC staus code to initialize the object with.

  */
  CMp3Ssc(SSC ssc);
  ~CMp3Ssc() {}

  /** Operator for conversion to a text string.

    @return Textual description.

  */
  operator const char*();

private:
  SSC  m_ssc;
};

/*-------------------------------------------------------------------------*/
#endif
