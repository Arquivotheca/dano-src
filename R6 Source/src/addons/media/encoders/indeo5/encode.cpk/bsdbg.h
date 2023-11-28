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

#ifdef INCLUDE_NESTING_CHECK
#ifdef __BSDBG_H__
#pragma message("***** ENDBG.H Included Multiple Times")
#endif
#endif

#ifndef __BSDBG_H__
#define __BSDBG_H__

/*
 * This file is dependent on the following include files:
 * <setjmp.h>
 * datatype.h
 * matrix.h
 * bsutil.h
 */

/*
 * Calculate the motion compensated block checksums and save them in
 * the debug info
 */
extern void CalcMcBlkChkSum(pBandSt pBand, MatrixSt PicX, jmp_buf jbEnv);

/*
 * Calculate the motion compensated block checksums and save them in
 * the debug info
 */
extern void CheckMcBlkChkSum(pBandSt pBand, MatrixSt PicX, jmp_buf jbEnv);

#endif /* __BSDBG_H__ */
