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

#include <SupportDefs.h>

/*-------------------------- defines --------------------------------------*/

class CGioBase;

/*-------------------------------------------------------------------------*/

//
// Bitstream input class.
//
//    This class defines the interface that the mp3 decoder object will
//    read all of its bitstream input data from.
//

#if _USE_MMX

extern "C"
{
	void CBitStream__init_asm(
		int32 i_bit_count, uint8** io_buf_ptr);
	int32 CBitStream__getbits_asm(
		int32 count, int32* io_bit_count, uint8** io_buf_ptr, uint8* buf_end_ptr, int32 buf_size);
	void CBitStream__reset_asm();
};

class CBitStream
{
public:

  CBitStream(int cbSize);
  CBitStream(unsigned char *pBuf, int cbSize, bool fDataValid = false);
  virtual ~CBitStream();

  virtual void   Reset();
  
  // calls to GetBits()/Get32Bits() must be bracketed with Begin/EndRead().
  void           BeginRead();
  void           EndRead();

  void           Connect(CGioBase *pGB);

  void           ResetBitCnt();
  int            GetBitCnt() const;

  unsigned int   GetBits(unsigned int nBits)
  {
	m_ValidBits -= nBits;
	return CBitStream__getbits_asm(nBits, &m_nBitsQueued, &m_BufPtr, m_BufEndPtr, m_nBytes);
  }

  unsigned long  Get32Bits()
  {
	return GetBits(32);
  }

  bool           Ff(int nBits)     { return ( (nBits > 0) ? Seek(nBits)  : false); }
  bool           Rewind(int nBits) { return ( (nBits > 0) ? Seek(-nBits) : false); }
  
  // init MMX reg only if active
  bool           Seek(int nBits);

  int            GetValidBits() const {	return m_ValidBits; }
  int            GetFree()      const;

  void           SetEof();
  int            Fill(const unsigned char *pBuf, int cbSize);
  int            Fill(CBitStream &Bs, int cbSize);

protected:

  int            Refill();
  bool           IsEof()       const;
  bool           IsConnected() const;
  
  int            CalcBitOffset() const
  {
	int byteOffset = m_BufPtr - m_Buf;
	if(m_fMMXActive)
	{
		// mm0 is filled, so m_BufPtr has been advanced 8 bytes
		byteOffset -= 8;
	}
	return byteOffset*8 + (64 - m_nBitsQueued);
  }

private:

  CGioBase      *m_pGB;           // I/O object
  int            m_nBytes;        // size of buffer in bytes
  int            m_nBits;         // size of buffer in bits
  int            m_ValidBits;     // valid bits in buffer
  int            m_ReadOffset;    // where to write next
  bool           m_fEof;          // indication of input eof
  uint8         *m_Buf;           // the buffer
  bool           m_fBufferIntern; // did we allocate the buffer ourselves
  
  bool           m_fMMXActive;    // true if MMX reg in use by this object
  uint8         *m_BufPtr;        // current read position in buffer
  uint8         *m_BufEndPtr;     // pointer to end of buffer (cached for performance)
  int32          m_nBitsQueued;   // current # of bits queued in MMX reg
  int32          m_nBookmark;     // bit offset at time of last ResetBitCnt() call
};

#else

// non-MMX version

class CBitStream
{
public:

  CBitStream(int cbSize);
  CBitStream(unsigned char *pBuf, int cbSize, bool fDataValid = false);
  virtual ~CBitStream();

  virtual void   Reset();

  // MMX support stubs
  void           BeginRead() {}
  void           EndRead() {}

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
#endif // _USE_MMX

/*-------------------------------------------------------------------------*/
#endif
