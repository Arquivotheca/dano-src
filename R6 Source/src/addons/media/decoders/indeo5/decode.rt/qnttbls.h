/************************************************************************
*                                                                       *
*               INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                       *
*    This listing is supplied under the terms of a license agreement    *
*      with INTEL Corporation and may not be copied nor disclosed       *
*        except in accordance with the terms of that agreement.         *
*                                                                       *
*************************************************************************
*                                                                       *
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

#ifndef __QNTTBLS_H__
#define __QNTTBLS_H__

#define NUM_QUANT_TABLES		6

/* 
 * This file is dependent on:
 * datatype.h
 */

/* Verify inclusion of required files */
#ifndef __DATATYPE_H__
#pragma message("qnttbls.h requires datatype.h")
#endif

extern U8 ubScan[4][64];
extern const U8 xform_to_scan[];

void InitQuantTables(U8 u8Set, U16 au16Quant[2][24][64]);

#endif /* __QNTTBLS_H__ */
