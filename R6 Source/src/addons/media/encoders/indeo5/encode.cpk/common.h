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
#ifdef __COMMON_H__
#pragma message("***** COMMON.H Included Multiple Times")
#endif
#endif

#ifndef __COMMON_H__
#define __COMMON_H__

#define MaxYDecompLevels  1
#define MaxVU9DecompLevels 0
#define MaxVU12DecompLevels 0

extern const U8 Default_Y_Xfrm[];
extern const U8 Default_VU9_Xfrm[];
extern const U8 Default_VU12_Xfrm[];
extern const U8 Default_Y_Quant[MaxYDecompLevels+1][MaxYDecompLevels*3+1];
extern const U8 Default_VU9_Quant[MaxVU9DecompLevels+1][MaxVU9DecompLevels*3+1];
extern const U8 Default_VU12_Quant[MaxVU12DecompLevels+1][MaxVU12DecompLevels*3+1];

#define XFORM_SLANT_8x8	0
#define XFORM_SLANT_1x8	1
#define XFORM_SLANT_8x1	2
#define XFORM_NONE_8x8	3
#define XFORM_SLANT_4x4	4

#define Q_NB_SL88	0
#define Q_B0_SL88	1
#define Q_B1_SL18	2
#define Q_B2_SL81	3
#define Q_NONE_88	4

#define Q_NB_SL44	5

#define Q_RESERVED    6

#define SCAN_ZIGZAG88	0
#define SCAN_COLUMN88	1
#define SCAN_ROW88		2
#define SCAN_ZIGZAG44	3

#define SCAN_RESERVED	5

#define ENC_MODE_ALL 0
#define ENC_MODE_TEST_WATER 1
#define ENC_MODE_TRIAL 2
#define ENC_MODE_FINAL 3
#define ENC_MODE_NULL 4

#endif /* _COMMON_H_ */
