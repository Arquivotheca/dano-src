/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: mp3ssc.cpp
 *   project : ---
 *   author  : Martin Sieler
 *   date    : 1999-02-15
 *   contents/description: ssc helper class (Structured Status Code)
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 09:15:07 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3ssc.cpp,v 1.1 1999/02/16 09:15:07 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mp3ssc.h"

/*-------------------------- defines --------------------------------------*/

#ifndef NULL
  #define NULL 0
#endif

/*-------------------------------------------------------------------------*/

typedef struct
  {
  SSC         ssc;
  const char *pszText;
  } SSCTEXT;

/*-------------------------------------------------------------------------*/

static const char *pszTemplate = "(Mp3Ssc) unknown SSC";

static const SSCTEXT sscTable[] =
{
  // success
  {SSC_OK, "(Mp3Ssc) success: no error" },

  // generic
  {SSC_E_WRONGPARAMETER, "(Mp3Ssc) error: at least one parameter is wrong"},
  {SSC_E_OUTOFMEMORY,    "(Mp3Ssc) error: not enough memory"},
  {SSC_E_INVALIDHANDLE,  "(Mp3Ssc) error: invalid handle"},

  // I/O
  {SSC_E_IO_GENERIC,     "(Mp3Ssc) error: I/O - generic error"},
  {SSC_E_IO_OPENFAILED,  "(Mp3Ssc) error: I/O - open failed"},
  {SSC_E_IO_CLOSEFAILED, "(Mp3Ssc) error: I/O - close failed"},
  {SSC_E_IO_READFAILED,  "(Mp3Ssc) error: I/O - read error"},

  // mpga
  {SSC_I_MPGA_CRCERROR,       "(Mp3Ssc) info: Decoder - CRC error"},
  {SSC_I_MPGA_NOMAINDATA,     "(Mp3Ssc) info: Decoder - not enough main data present"},

  {SSC_E_MPGA_GENERIC,        "(Mp3Ssc) error: Decoder - generic error"},
  {SSC_E_MPGA_WRONGLAYER,     "(Mp3Ssc) error: Decoder - wrong MPEG layer"},
  {SSC_E_MPGA_BUFFERTOOSMALL, "(Mp3Ssc) error: Decoder - buffer is too small"},

  {SSC_W_MPGA_SYNCSEARCHED,   "(Mp3Ssc) warning: Decoder - sync searched"},
  {SSC_W_MPGA_SYNCLOST,       "(Mp3Ssc) warning: Decoder - sync lost"},
  {SSC_W_MPGA_SYNCNEEDDATA,   "(Mp3Ssc) warning: Decoder - sync need data"},
  {SSC_W_MPGA_SYNCEOF,        "(Mp3Ssc) warning: Decoder - sync eof"},
};

static const int nsscTable = sizeof(sscTable) / sizeof(sscTable[0]);

/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   C M p 3 S s c
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CMp3Ssc::CMp3Ssc(SSC ssc)
{
  m_ssc = ssc;
}

//-------------------------------------------------------------------------*
//   operator const char*
//-------------------------------------------------------------------------*

CMp3Ssc::operator const char*()
{
  const char *pszText = NULL;
  int         i;

  // lookup table for matching ssc
  for ( i=0; i<nsscTable; i++ )
    {
    if ( m_ssc == sscTable[i].ssc )
      {
      pszText = sscTable[i].pszText;
      break;
      }
    }

  // not found - use template
  if ( pszText == NULL )
    pszText = pszTemplate;

  return pszText;
}

/*-------------------------------------------------------------------------*/
