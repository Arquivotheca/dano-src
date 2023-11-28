/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: meanvalue.h
 *   project : ---
 *   author  : Martin Sieler
 *   date    : 1998-02-14
 *   contents/description: HEADER - calc mean value
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/18 09:36:42 $
 * $Header: /home/cvs/mms/common/util/meanvalue.h,v 1.1 1999/02/18 09:36:42 sir Exp $
 */

#ifndef __MEANVALUE_H__
#define __MEANVALUE_H__

/* ------------------------ includes --------------------------------------*/

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

class CMeanValue
{
public:

  CMeanValue() { Reset(); }

  void  Reset();
  CMeanValue& operator+= (int nValue);

  operator int()   const { return m_Count ? m_Sum/m_Count : 0; }
  operator float() const { return m_Count ? float(m_Sum)/float(m_Count) : 0.0f; }

  int   GetSum()   const { return m_Sum;    }
  int   GetCount() const { return m_Count;  }
  int   GetMin()   const { return m_Min;    }
  int   GetMax()   const { return m_Max;    }
  bool  IsFixed()  const { return m_bFixed; }

protected:

private:

  int  m_Count;
  int  m_Sum;
  int  m_FirstValue;
  int  m_Min;
  int  m_Max;
  bool m_bFixed;
};

/*-------------------------------------------------------------------------*/
#endif
