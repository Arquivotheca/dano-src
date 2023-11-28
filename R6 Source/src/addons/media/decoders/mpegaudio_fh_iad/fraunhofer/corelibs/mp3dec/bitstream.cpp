/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1997)
 *                        All Rights Reserved
 *
 *   filename: bitstream.cpp
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1997-12-05
 *   contents/description: generic bitbuffer
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 09:35:16 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/bitstream.cpp,v 1.10 1999/02/16 09:35:16 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "bitstream.h"
#include "giobase.h"

#include <Debug.h>

/*-------------------------- defines --------------------------------------*/

#ifndef min
  #define min(a,b) (((a) < (b)) ? (a):(b))
#endif

#ifndef NULL
  #define NULL 0
#endif

/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   C B i t S t r e a m
//
//-------------------------------------------------------------------------*

#if _USE_MMX

CBitStream::CBitStream(int cbSize)
{
	int i;
	
	// check cbSize (must be power of 2)
	for ( i=0; i<16; i++ )
		if ( (1 << i) >= cbSize )
			break;
	
	m_nBytes        = (1 << i);
	m_nBits         = m_nBytes * 8;
	m_Buf           = new unsigned char[m_nBytes];
	m_BufEndPtr     = m_Buf + m_nBytes;
	m_pGB           = NULL;
	m_fBufferIntern = true;
	m_fMMXActive    = false;
	
	Reset();
}


CBitStream::CBitStream(unsigned char *pBuf, int cbSize, bool fDataValid)
{
	int i;
	
	// check cbSize (must be power of 2)
	for ( i=0; i<16; i++ )
		if ( (1 << i) >= cbSize )
			break;
	
	m_nBytes        = (1 << i);
	m_nBits         = m_nBytes * 8;
	m_Buf           = pBuf;
	m_BufEndPtr     = m_Buf + m_nBytes;
	m_pGB           = NULL;
	m_fBufferIntern = false;
	m_fMMXActive    = false;
	
	Reset();
	
	if ( fDataValid )
	{
		m_ValidBits   = m_nBits;
	}
}


CBitStream::~CBitStream()
{
	if ( m_fBufferIntern && m_Buf )
		delete[] m_Buf;
}

void 
CBitStream::Reset()
{
	m_ValidBits  = 0;
	m_ReadOffset = 0;
	m_nBitsQueued = 64;
	m_BufPtr = m_Buf;
	m_fEof       = false;
}

void 
CBitStream::BeginRead()
{
	ASSERT(!m_fMMXActive);
	CBitStream__init_asm(m_nBitsQueued, &m_BufPtr);
	m_fMMXActive = true;
}

void 
CBitStream::EndRead()
{
	ASSERT(m_fMMXActive);
	// unload (8 bytes) - (m_nBitsQueued bits)
	CBitStream__reset_asm();
	m_BufPtr -= 8;
	m_fMMXActive = false;
}

void 
CBitStream::Connect(CGioBase *pGB)
{
	m_pGB = pGB;
}

void 
CBitStream::ResetBitCnt()
{
	m_nBookmark = CalcBitOffset();
}

int 
CBitStream::GetBitCnt() const
{
	int offset = CalcBitOffset();
	if (m_nBookmark > offset)
		return (m_nBits - m_nBookmark) + offset;
	else
		return offset - m_nBookmark;
}

bool 
CBitStream::Seek(int nBits)
{
	m_ValidBits -= nBits;
	int offset = CalcBitOffset();
	int newOffset = (offset + nBits) % m_nBits;
	while(newOffset < 0)
		newOffset += m_nBits;
//	fprintf(stderr, "CBitStream(%p)::Seek(+%ld): @ %ld -> %ld/%ld\n", this, nBits, CalcBitOffset(), offset + nBits, newOffset);
	int byteOffset = (newOffset / 64) * 8;
	int bitOffset = newOffset % 64;
	
	m_BufPtr = m_Buf + byteOffset;
	m_nBitsQueued = 64 - bitOffset;
	if(m_fMMXActive)
	{
		CBitStream__init_asm(m_nBitsQueued, &m_BufPtr);
	}
	return true;
}

int 
CBitStream::GetFree() const
{
	return (m_nBits - m_ValidBits) / 8;
}

void 
CBitStream::SetEof()
{
	m_fEof = true;
}

int 
CBitStream::Fill(const unsigned char *pBuf, int cbSize)
{
	const unsigned char *ptr    = pBuf;
	int                  bTotal = 0;
	int                  noOfBytes;
	int                  bToRead;
	
	bToRead   = GetFree();
	noOfBytes = min(bToRead, cbSize);
	
	while ( noOfBytes > 0 )
	{
		// Split Read to buffer size
		bToRead = min(m_nBytes - m_ReadOffset, noOfBytes);
		
		// copy 'bToRead' bytes from 'ptr' to buffer
		for ( int i=0; i<bToRead; i++ )
			m_Buf[m_ReadOffset + i] = ptr[i];
	
		// add noOfBits to number of valid bits in buffer
		m_ValidBits  += bToRead * 8;
		bTotal       += bToRead;
		ptr          += bToRead;
		
		m_ReadOffset  = (m_ReadOffset + bToRead) & (m_nBytes-1);
		noOfBytes    -= bToRead;
	}
	
	return bTotal;
}

int 
CBitStream::Fill(CBitStream &Bs, int cbSize)
{
	int bTotal = 0;
	int noOfBytes;
	int bToRead;
	int i;
	
	// limit cbSize to number of valid bytes in 'Bs'
	bToRead   = Bs.GetValidBits() / 8;
	cbSize    = min(cbSize, bToRead);
	
	// limit to number of free bytes of this object
	bToRead   = GetFree();
	noOfBytes = min(bToRead, cbSize);
	
	while ( noOfBytes > 0 )
	{
		// Split Read to buffer size
		bToRead = min(m_nBytes - m_ReadOffset, noOfBytes);
		
		// copy 'bToRead' bytes from 'Bs' to buffer
		for ( i=0; i<bToRead; i++ )
			m_Buf[m_ReadOffset + i] = (unsigned char)Bs.GetBits(8);
		
		// add noOfBits to number of valid bits in buffer
		m_ValidBits  += bToRead * 8;
		bTotal       += bToRead;
		
		m_ReadOffset  = (m_ReadOffset + bToRead) & (m_nBytes-1);
		noOfBytes    -= bToRead;
	}
	
	return bTotal;
}

int 
CBitStream::Refill()
{
	int noOfBytes = GetFree();
	int bTotal    = 0;
	int bToRead, bRead;
	
	// check if connected
	if ( !IsConnected() )
	{
		return 0;
	}
	
	while ( noOfBytes > 0 )
	{
		// split read to buffer size
		bToRead = min(m_nBytes - m_ReadOffset, noOfBytes);
		
		m_pGB->Read(m_Buf+m_ReadOffset, bToRead, &bRead);
		
		// missing: check for read errors!!
		
		// add noOfBits to number of valid bits in buffer
		m_ValidBits  += bRead * 8;
		bTotal       += bRead;
		
		m_ReadOffset  = (m_ReadOffset + bRead) & (m_nBytes-1);
		noOfBytes    -= bToRead;
		
		if ( bRead < bToRead )
			break;
	}
	
	// check for EOF
	if ( m_pGB->IsEof() )
		SetEof();
	
	return bTotal;
}

bool 
CBitStream::IsEof() const
{
	return m_fEof;
}

bool 
CBitStream::IsConnected() const
{
	return (m_pGB != NULL);
}

#else // _USE_MMX

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CBitStream::CBitStream(int cbSize)
{
  int i;

  // check cbSize (must be power of 2)
  for ( i=0; i<16; i++ )
    if ( (1 << i) >= cbSize )
      break;

  m_nBytes        = (1 << i);
  m_nBits         = m_nBytes * 8;
  m_Buf           = new unsigned char[m_nBytes];
  m_pGB           = NULL;
  m_fBufferIntern = true;

  Reset();
}

//-------------------------------------------------------------------------*

CBitStream::CBitStream(unsigned char *pBuf, int cbSize, bool fDataValid)
{
  int i;

  // check cbSize (must be power of 2)
  for ( i=0; i<16; i++ )
    if ( (1 << i) >= cbSize )
      break;

  m_nBytes        = (1 << i);
  m_nBits         = m_nBytes * 8;
  m_Buf           = pBuf;
  m_pGB           = NULL;
  m_fBufferIntern = false;

  Reset();

  if ( fDataValid )
    {
    m_ValidBits   = m_nBits;
    }
}

//-------------------------------------------------------------------------*
//   destructor
//-------------------------------------------------------------------------*

CBitStream::~CBitStream()
{
  if ( m_fBufferIntern && m_Buf )
    delete[] m_Buf;
}

//-------------------------------------------------------------------------*
//   Reset
//-------------------------------------------------------------------------*

void CBitStream::Reset()
{
  m_ValidBits  = 0;
  m_ReadOffset = 0;
  m_BitCnt     = 0;
  m_BitNdx     = 0;
  m_fEof       = false;
}

//-------------------------------------------------------------------------*
//   Connect
//-------------------------------------------------------------------------*

void CBitStream::Connect(CGioBase *pGB)
{
  m_pGB = pGB;
}

//-------------------------------------------------------------------------*
//   GetBits
//-------------------------------------------------------------------------*

unsigned int CBitStream::GetBits(unsigned int nBits)
{
  unsigned short tmp, tmp1;

  unsigned int nWordNdx   = (m_BitNdx>>4)<<1;
  unsigned int nBitsAvail = 16 - (m_BitNdx & 15);
  
  tmp   = m_Buf[nWordNdx];
  tmp <<= 8;
  tmp  |= m_Buf[nWordNdx+1];
  tmp <<= (m_BitNdx&15);

  if ( nBits > nBitsAvail )
    {
    nWordNdx  = (nWordNdx+2) & (m_nBytes-1);
    tmp1      = m_Buf[nWordNdx];
    tmp1    <<= 8;
    tmp1     |= m_Buf[nWordNdx+1];

    tmp1    >>= (16-(m_BitNdx&15));
    tmp      |= tmp1;
    }

  tmp >>= (16-nBits);

  m_BitNdx     = (m_BitNdx+nBits) & (m_nBits-1);
  m_BitCnt    += nBits;
  m_ValidBits -= nBits;

//  printf("getbits(%ld): %x, @%ld\n", nBits, tmp, GetBitCnt());
  return tmp;
}

//-------------------------------------------------------------------------*
//   Get32Bits
//-------------------------------------------------------------------------*

unsigned long CBitStream::Get32Bits()
{
  unsigned long tmp;

  tmp  = (unsigned long)(GetBits(16)) << 16;
  tmp |= GetBits(16);

  return tmp;
}

//-------------------------------------------------------------------------*
//   GetFree
//-------------------------------------------------------------------------*

int CBitStream::GetFree() const
{
  return (m_nBits - m_ValidBits) / 8;
}

//-------------------------------------------------------------------------*
//   SetEof
//-------------------------------------------------------------------------*

void CBitStream::SetEof()
{
  m_fEof = true;
}

//-------------------------------------------------------------------------*
//   Fill
//-------------------------------------------------------------------------*

int CBitStream::Fill(const unsigned char *pBuf, int cbSize)
{
  const unsigned char *ptr    = pBuf;
  int                  bTotal = 0;
  int                  noOfBytes;
  int                  bToRead;

  bToRead   = GetFree();
  noOfBytes = min(bToRead, cbSize);

  while ( noOfBytes > 0 )
    {
    // Split Read to buffer size
    bToRead = min(m_nBytes - m_ReadOffset, noOfBytes);

    // copy 'bToRead' bytes from 'ptr' to buffer
    for ( int i=0; i<bToRead; i++ )
      m_Buf[m_ReadOffset + i] = ptr[i];

    // add noOfBits to number of valid bits in buffer
    m_ValidBits  += bToRead * 8;
    bTotal       += bToRead;
    ptr          += bToRead;

    m_ReadOffset  = (m_ReadOffset + bToRead) & (m_nBytes-1);
    noOfBytes    -= bToRead;
    }

  return bTotal;
}

//-------------------------------------------------------------------------*

int CBitStream::Fill(CBitStream &Bs, int cbSize)
{
  int bTotal = 0;
  int noOfBytes;
  int bToRead;
  int i;

  // limit cbSize to number of valid bytes in 'Bs'
  bToRead   = Bs.GetValidBits() / 8;
  cbSize    = min(cbSize, bToRead);

  // limit to number of free bytes of this object
  bToRead   = GetFree();
  noOfBytes = min(bToRead, cbSize);

  while ( noOfBytes > 0 )
    {
    // Split Read to buffer size
    bToRead = min(m_nBytes - m_ReadOffset, noOfBytes);

    // copy 'bToRead' bytes from 'Bs' to buffer
    for ( i=0; i<bToRead; i++ )
      m_Buf[m_ReadOffset + i] = (unsigned char)Bs.GetBits(8);

    // add noOfBits to number of valid bits in buffer
    m_ValidBits  += bToRead * 8;
    bTotal       += bToRead;

    m_ReadOffset  = (m_ReadOffset + bToRead) & (m_nBytes-1);
    noOfBytes    -= bToRead;
    }

  return bTotal;
}

//-------------------------------------------------------------------------*
//   Refill
//-------------------------------------------------------------------------*

int CBitStream::Refill()
{
  int noOfBytes = GetFree();
  int bTotal    = 0;
  int bToRead, bRead;

  // check if connected
  if ( !IsConnected() )
    {
    return 0;
    }

  while ( noOfBytes > 0 )
    {
    // split read to buffer size
    bToRead = min(m_nBytes - m_ReadOffset, noOfBytes);

    m_pGB->Read(m_Buf+m_ReadOffset, bToRead, &bRead);

    // missing: check for read errors!!

    // add noOfBits to number of valid bits in buffer
    m_ValidBits  += bRead * 8;
    bTotal       += bRead;

    m_ReadOffset  = (m_ReadOffset + bRead) & (m_nBytes-1);
    noOfBytes    -= bToRead;

    if ( bRead < bToRead )
      break;
    }

  // check for EOF
  if ( m_pGB->IsEof() )
    SetEof();

  return bTotal;
}

//-------------------------------------------------------------------------*
//   IsEof
//-------------------------------------------------------------------------*

bool CBitStream::IsEof() const
{
  return m_fEof;
}

//-------------------------------------------------------------------------*
//   IsConnected
//-------------------------------------------------------------------------*

bool CBitStream::IsConnected() const
{
  return (m_pGB != NULL);
}

#endif // _USE_MMX
/*-------------------------------------------------------------------------*/
