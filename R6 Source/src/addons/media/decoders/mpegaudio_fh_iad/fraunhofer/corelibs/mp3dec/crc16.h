/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: crc16.h
 *   project : ISO/MPEG decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: functions to calculate a CRC-16
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 08:46:16 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/crc16.h,v 1.3 1999/02/16 08:46:16 sir Exp $
 */

#ifndef __CRC16_H__
#define __CRC16_H__

/* ------------------------ includes --------------------------------------*/

/* ------------------------------------------------------------------------*/

class CBitStream;

/* ------------------------------------------------------------------------*/

unsigned int CalcCrc(CBitStream &Bs, int len, unsigned int start);

/*-------------------------------------------------------------------------*/
#endif
