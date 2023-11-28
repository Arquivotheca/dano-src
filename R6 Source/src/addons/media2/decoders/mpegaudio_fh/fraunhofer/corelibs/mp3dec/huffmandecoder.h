/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: huffmandecoder.h
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1998-02-08
 *   contents/description: HEADER - huffman decoder
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/03/11 08:51:53 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/huffmandecoder.h,v 1.6 1999/03/11 08:51:53 sir Exp $
 */

#ifndef __HUFFMANDECODER_H__
#define __HUFFMANDECODER_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"
#include "bitsequence.h"
#include "huffmanbitobj.h"
#include "huffmantable.h"

/*-------------------------- defines --------------------------------------*/

class CBitStream;

/*-------------------------------------------------------------------------*/

//
// Huffman decoder (helper) class.
//
//  This object reads and decodes MPEG Layer-3 huffman data.
//

class CHuffmanDecoder
{
public:
  CHuffmanDecoder();
  virtual ~CHuffmanDecoder();

  int ReadHuffmanCode(CBitStream &Bs,
                      int32      *pIsp,
                      const int  *pTableSelect,
                      const int  *pRegionEnd,
                      int         Count1TableSelect,
                      int         Part2_3Length,
                      int         interleave=1); // may be 1 or 2

protected:

private:
  int  ReadBigValues(CBitStream  &Bs,
                     int32       *pIsp,
                     const int   *pTableSelect,
                     const int   *pRegionEnd,
                     int          interleave);

  int  ReadCount1Area(CBitStream &Bs,
                      int32      *pIsp,
                      int         Count1TableSelect,
                      int         Count1Start,
                      int         Part2_3Length,
                      int         interleave);

  bool ReadHuffmanDual   (CBitStream &Bs, int32 *pIsp, int interleave);
  bool ReadHuffmanDualLin(CBitStream &Bs, int32 *pIsp, int interleave);
  bool ReadHuffmanQuad   (CBitStream &Bs, int32 *pIsp, int interleave);

  CHuffmanTable  m_HuffmanTable;
  CHuffmanBitObj m_HuffmanBitObj;
  CBitSequence   m_Sign;
  CBitSequence   m_LinBits;
};

/*-------------------------------------------------------------------------*/
#endif
