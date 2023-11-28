/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: huffmanbitobj.cpp
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1997-12-29
 *   contents/description: Huffman Bit Object
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 08:52:29 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/huffmanbitobj.cpp,v 1.3 1999/02/16 08:52:29 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "huffmanbitobj.h"
#include "bitstream.h"
#include "huffmantable.h"

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   C H u f f m a n B i t O b j
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CHuffmanBitObj::CHuffmanBitObj(const CHuffmanTable& HT) : m_HuffmanTable(HT)
{
}

//-------------------------------------------------------------------------*
//   destructor
//-------------------------------------------------------------------------*

CHuffmanBitObj::~CHuffmanBitObj()
{
}

//-------------------------------------------------------------------------*
//   ReadFrom
//-------------------------------------------------------------------------*

bool CHuffmanBitObj::ReadFrom(CBitStream &BS)
{
  unsigned int bits;
  unsigned int tab_ndx       = 0;
  int          s_BitCnt      = BS.GetBitCnt();
  unsigned int nBitsPerLevel = m_HuffmanTable.GetBitsPerLevel();

  while ( 1 )
    {
    bits = BS.GetBits(nBitsPerLevel);
    
    if ( m_HuffmanTable.IsLengthZero(tab_ndx, bits) )
      {
      tab_ndx = m_HuffmanTable.GetCode(tab_ndx, bits);
      }
    else
      {
      /* 
       * stuff back bits, that are not part of the actual huffman value
       * (<nBitsPerLevel>bit huffman decoder!)
       *
       * bits read              : GetBitCnt() - <saved bitcount>
       * bits to read           : CHuffmanTable::GetLength()
       *
       * bits to seek (forward!): <bits to read> - <bits read>
       */

      int nBitsRead   = BS.GetBitCnt() - s_BitCnt;
      int nBitsToRead = m_HuffmanTable.GetLength(tab_ndx, bits);

      BS.Seek(nBitsToRead - nBitsRead);
        
      m_nValue = m_HuffmanTable.GetCode(tab_ndx, bits);

      return true;
      }
    }
}

/*-------------------------------------------------------------------------*/
