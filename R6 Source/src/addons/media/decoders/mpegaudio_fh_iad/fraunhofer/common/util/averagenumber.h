/***************************************************************************\
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: averagenumber.h
 *   project : ---
 *   author  : Martin Sieler
 *   date    : 1999-02-17
 *   contents/description: calc average number
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/18 09:36:41 $
 * $Header: /home/cvs/mms/common/util/averagenumber.h,v 1.1 1999/02/18 09:36:41 sir Exp $
 */

#ifndef __AVERAGENUMBER_H__
#define __AVERAGENUMBER_H__

/* ------------------------ includes --------------------------------------*/

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   C A v e r a g e N u m b e r
//
//-------------------------------------------------------------------------*

class CAverageNumber
{
public:
   
  CAverageNumber(unsigned int nSize)
    {
    m_nSize   = nSize;
    m_pnValue = new int[m_nSize];
    Reset();
    }
    
  ~CAverageNumber()
    {
    if ( m_pnValue )
      delete[] m_pnValue;
    }

  void Reset()
    {
    m_nIndex  = 0;

    for ( int i=0 ; i<m_nSize; i++ )
      m_pnValue[i] = 0;
    }

  CAverageNumber &operator += (int nValue)
    {
    m_pnValue[m_nIndex++ % m_nSize] = nValue;
    return *this;
    }
    
  operator int() const
    {
    int nValue = 0 ;
    for (int i=0; i<m_nSize; i++)
      nValue += m_pnValue[i];

    return (nValue/m_nSize) ;
    }
    
protected :
  int  m_nSize;
  int  m_nIndex;
  int *m_pnValue;
};

/*-------------------------------------------------------------------------*/
#endif
