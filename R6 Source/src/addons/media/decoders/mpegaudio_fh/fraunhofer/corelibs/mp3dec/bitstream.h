/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1997)
 *                        All Rights Reserved
 *
 *   filename: bitstream.h
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1997-12-05
 *   contents/description: generic bitbuffer - HEADER
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/03/11 08:51:51 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/bitstream.h,v 1.9 1999/03/11 08:51:51 sir Exp $
 */

#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

/* ------------------------ includes --------------------------------------*/

/*-------------------------- defines --------------------------------------*/

class CGioBase;

/*-------------------------------------------------------------------------*/

//
// Bitstream input class.
//
//    This class defines the interface that the mp3 decoder object will
//    read all of its bitstream input data from.
//

class CBitStream
{
public:

  CBitStream(int cbSize);
  CBitStream(unsigned char *pBuf, int cbSize, bool fDataValid = false);
  virtual ~CBitStream();

  virtual void   Reset();

  void           Connect(CGioBase *pGB);

  void           ResetBitCnt()     { m_BitCnt = 0;    }
  int            GetBitCnt() const { return m_BitCnt; }

  unsigned int   GetBits(unsigned int nBits);
  unsigned long  Get32Bits();

  bool           Ff(int nBits)     { return ( (nBits > 0) ? Seek(nBits)  : false); }
  bool           Rewind(int nBits) { return ( (nBits > 0) ? Seek(-nBits) : false); }
  bool           Seek(int nBits)
    {  
    m_BitCnt    += nBits;
    m_ValidBits -= nBits;
    m_BitNdx     = (m_BitNdx+nBits) & (m_nBits-1);
    return true;
    }

  int            GetValidBits() const { return m_ValidBits; }
  int            GetFree()      const;

  void           SetEof();
  int            Fill(const unsigned char *pBuf, int cbSize);
  int            Fill(CBitStream &Bs, int cbSize);

protected:

  int            Refill();
  bool           IsEof()       const;
  bool           IsConnected() const;

private:

  CGioBase      *m_pGB;           // I/O object
  int            m_nBytes;        // size of buffer in bytes
  int            m_nBits;         // size of buffer in bits
  int            m_ValidBits;     // valid bits in buffer
  int            m_ReadOffset;    // where to write next
  int            m_BitCnt;        // bit counter
  int            m_BitNdx;        // position of next bit in byte
  bool           m_fEof;          // indication of input eof
  unsigned char *m_Buf;           // the buffer
  bool           m_fBufferIntern; // did we allocate the buffer ourselves
};

/*-------------------------------------------------------------------------*/
#endif
