/************************************************************************
*																		*
*				INTEL CORPORATION PROPRIETARY INFORMATION				*
*																		*
*	 This listing is supplied under the terms of a license agreement	*
*	   with INTEL Corporation and may not be copied nor disclosed		*
*		 except in accordance with the terms of that agreement.			*
*																		*
*************************************************************************
*																		*
*				Copyright (C) 1994-1997 Intel Corp.                       *
*						  All Rights Reserved.							*
*																		*
************************************************************************/

/* common.c */
#include "datatype.h"
#include "common.h"

/* default transform for each band in Y */
U8 const Default_Y_Xfrm[MaxYDecompLevels*3+1]={
		XFORM_SLANT_8x8,
		XFORM_SLANT_1x8,
		XFORM_SLANT_8x1,
		XFORM_NONE_8x8
};

/* default transform for each band in U and V , for YVU9*/
U8 const  Default_VU9_Xfrm[MaxVU9DecompLevels*3+1]={
		XFORM_SLANT_4x4
};

/* default transform for each band in U and V , for YVU12 */
U8 const  Default_VU12_Xfrm[MaxVU12DecompLevels*3+1]={
		XFORM_SLANT_8x8,
};

/* default quant matrix for each band in Y, based upon the decomp levels */
const U8 Default_Y_Quant[MaxYDecompLevels+1][MaxYDecompLevels*3+1]={
	{ /* 0 level: not scalable, no banding */
		Q_NB_SL88
	},
	{ /* 1 level: 4 bands */
		Q_B0_SL88,
		Q_B1_SL18,	
		Q_B2_SL81,
		Q_NONE_88	
	}
};

/* default quant matrix for each band in U and V,YVU9 */
const U8 Default_VU9_Quant[MaxVU9DecompLevels+1][MaxVU9DecompLevels*3+1]={
	{ /* 0 level: not scalable, no banding */
		Q_NB_SL44
	}
};

/* default quant matrix for each band in U and V, YVU12 */
const U8 Default_VU12_Quant[MaxVU12DecompLevels+1][MaxVU12DecompLevels*3+1]={
	{ /* 0 level: not scalable, no banding */
		Q_NB_SL88
	}
};
