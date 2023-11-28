/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: sequencedetector.h
 *   project : ---
 *   author  : Martin Sieler
 *   date    : 1998-02-14
 *   contents/description: HEADER - sequence detector
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/18 09:36:44 $
 * $Header: /home/cvs/mms/common/util/sequencedetector.h,v 1.1 1999/02/18 09:36:44 sir Exp $
 */

#ifndef __SEQUENCEDETECTOR_H__
#define __SEQUENCEDETECTOR_H__

/* ------------------------ includes --------------------------------------*/

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

class CSequenceDetector
{
public:
  CSequenceDetector(int nLimit);
  ~CSequenceDetector();

  void Reset();
  CSequenceDetector& operator+= (int nValue);

  int  GetLength()          const;
  int  GetValue(int nIndex) const;
  int  GetSum()             const;

protected:

private:
  int   m_Limit;
  int   m_Count;
  bool *m_pDisabled;
  int  *m_pArray;
};

/*-------------------------------------------------------------------------*/
#endif
