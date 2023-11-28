/***************************************************************************\
 *
 *               (C) copyright Fraunhofer - IIS (1997)
 *                        All Rights Reserved
 *
 *   filename: bitsequence.h
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1997-12-23
 *   contents/description: HEADER - bitsequence object
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/03/11 08:51:50 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/bitsequence.h,v 1.6 1999/03/11 08:51:50 sir Exp $
 */

#ifndef __BITSEQUENCE_H__
#define __BITSEQUENCE_H__

/* ------------------------ includes --------------------------------------*/

#include "bitstream.h"

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

//
// Bitstream parser class.
//
//  This helper class is basically a numerical value that can read itself from
//  a CBitStream interface for convenience. The decoder almost completely
//  does the bitstream parsing through CBitSequence rather than CBitStream
//  directly.
//

class CBitSequence
{
public:

  CBitSequence(int nBits = 0) { m_nBits = nBits; m_nValue = 0; }
  virtual ~CBitSequence() {}

  void SetNumberOfBits(int nBits) { m_nBits = nBits; }
  int  GetNumberOfBits() const    { return m_nBits; }

  bool ReadFrom(CBitStream &Bs) { m_nValue = Bs.GetBits(m_nBits); return true; }
  bool ReadFrom(CBitStream &Bs, int nBits) { SetNumberOfBits(nBits); return ReadFrom(Bs); }

  bool Equals(int nValue) const { return (m_nValue == nValue); }

  int  ToInt() const        { return m_nValue; }
  void FromInt(int nValue) { m_nValue = nValue; }

protected:

private:

  int m_nBits;
  int m_nValue;
};

/*-------------------------------------------------------------------------*/
#endif
