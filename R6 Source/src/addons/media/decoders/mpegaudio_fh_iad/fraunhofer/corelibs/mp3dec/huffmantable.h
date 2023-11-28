/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: huffmantable.h
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1998-01-05
 *   contents/description: HEADER - huffman table object
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/03/11 08:51:53 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/huffmantable.h,v 1.7 1999/03/11 08:51:53 sir Exp $
 */

#ifndef __HUFFMANTABLE_H__
#define __HUFFMANTABLE_H__

/* ------------------------ includes --------------------------------------*/

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

// Huffman tables.
//
//  This object holds the huffman table for ISO/MPEG Layer-3.
//

class CHuffmanTable
{
public:
  CHuffmanTable();
  virtual ~CHuffmanTable();

  void SetTableIndex(unsigned int _nTableIndex)
    { nTableIndex = _nTableIndex; }

  unsigned int GetBitsPerLevel() const
    { return BITS_PER_LEVEL; }

  unsigned int GetLinBits() const
    { return ht[nTableIndex].linbits; }

  unsigned int GetCode  (unsigned int nIndex, unsigned int nValue) const
    { return (ht[nTableIndex].table[nIndex][nValue] & 0xff); }

  unsigned int GetLength(unsigned int nIndex, unsigned int nValue) const
    {  return ((ht[nTableIndex].table[nIndex][nValue] >> 8) & 0xff); }

  bool IsTableValid() const
    {  return (ht[nTableIndex].table ? true:false); }

  bool IsLengthZero(unsigned int nIndex, unsigned int nValue) const
    { return ((ht[nTableIndex].table[nIndex][nValue] & 0xff00) == 0); }

  enum { BITS_PER_LEVEL = 2, ENTRIES_PER_LEVEL = 4 };

protected:

private:

  typedef struct 
    {
    unsigned int linbits;
    const unsigned int(*table)[ENTRIES_PER_LEVEL];
    } huffmantab;

  static const huffmantab ht[];

  unsigned int nTableIndex;
};

/*-------------------------------------------------------------------------*/
#endif
