/****************************************************************************
 * Radeon 3D Library Header                                                 *
 *                                                                          *
 * TCL state loading functions                                              *
 *                                                                          *
 * Copyright (c) 2000 ATI Technologies Inc.  All rights reserved.           *
 ****************************************************************************/

#ifndef _RADEON_TCLUTIL_H
#define _RADEON_TCLUTIL_H

typedef enum
{
	MATRIX_MODELVIEW_0 = 0,
	MATRIX_MODELVIEW_1,
	MATRIX_MODELVIEW_2,
	MATRIX_MODELVIEW_3,
	MATRIX_MODELVIEWINV_0,
	MATRIX_MODELVIEWINV_1,
	MATRIX_MODELVIEWINV_2,
	MATRIX_MODELVIEWINV_3,
	MATRIX_MODEL2CLIP_0,
	MATRIX_MODEL2CLIP_1,
	MATRIX_MODEL2CLIP_2,
	MATRIX_MODEL2CLIP_3,
	MATRIX_TEXTURE_0,
	MATRIX_TEXTURE_1,
	MATRIX_TEXTURE_2,
	MATRIX_TEXTURE_3,
	MATRIX_EYE2CLIP,
	MATRIX_FORCE_DWORD = 0x7FFFFFFF
} EMATRIX;

#define VS_MATRIX_MODELVIEW_0_ADDR        0
#define VS_MATRIX_MODELVIEW_1_ADDR        4
#define VS_MATRIX_MODELVIEW_2_ADDR        8
#define VS_MATRIX_MODELVIEW_3_ADDR        12
#define VS_MATRIX_MODELVIEWINV_0_ADDR     16
#define VS_MATRIX_MODELVIEWINV_1_ADDR     20
#define VS_MATRIX_MODELVIEWINV_2_ADDR     24
#define VS_MATRIX_MODELVIEWINV_3_ADDR     28
#define VS_MATRIX_MODEL2CLIP_0_ADDR       32
#define VS_MATRIX_MODEL2CLIP_1_ADDR       36
#define VS_MATRIX_MODEL2CLIP_2_ADDR       40
#define VS_MATRIX_MODEL2CLIP_3_ADDR       44
#define VS_MATRIX_TEXTURE_0_ADDR          48
#define VS_MATRIX_TEXTURE_1_ADDR          52
#define VS_MATRIX_TEXTURE_2_ADDR          56
#define VS_MATRIX_TEXTURE_3_ADDR          60
#define VS_LIGHT_AMBIENT_ADDR             64
#define VS_LIGHT_DIFFUSE_ADDR             72
#define VS_LIGHT_SPECULAR_ADDR            80
#define VS_LIGHT_DIRPOS_ADDR              88
#define VS_LIGHT_HWVSPOT_ADDR             96
#define VS_LIGHT_ATTENUATION_ADDR        104
#define VS_MATRIX_EYE2CLIP_ADDR          112
#define VS_UCP_ADDR                      116
#define VS_GLOBAL_AMBIENT_ADDR           122
#define VS_FOG_PARAM_ADDR                123
#define VS_EYE_VECTOR_ADDR               124

#define SS_LIGHT_SPOT_EXPONENT_ADDR        8
#define SS_LIGHT_SPOT_CUTOFF_ADDR         16
#define SS_LIGHT_SPECULAR_THRESH_ADDR     24
#define SS_LIGHT_RANGE_CUTOFF_ADDR        32
#define SS_VERT_GUARD_CLIP_ADJ_ADDR       48
#define SS_VERT_GUARD_DISCARD_ADJ_ADDR    49
#define SS_HORZ_GUARD_CLIP_ADJ_ADDR       50
#define SS_HORZ_GUARD_DISCARD_ADJ_ADDR    51


#endif // _RADEON_TCLUTIL_H
