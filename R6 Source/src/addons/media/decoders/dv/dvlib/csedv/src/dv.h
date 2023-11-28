//=============================================================================
// Description:
//		Software DV decoder/encoder main header file.
//
//
//
//
// Copyright:
//		Copyright (c) 1998 Canopus Co.,Ltd. All Rights Reserved.
//		Developed in San Jose, CA, U.S.A.
// History:
//		12/15/98 Tom > Creaded.
//
//=============================================================================
#include "csedv.h"


//------------------------------------------------------------------------------
// Global DV definitions
//------------------------------------------------------------------------------

#define	SCT_HEADER	0
#define	SCT_SUBCODE	1
#define	SCT_VAUX	2
#define	SCT_AUDIO	3
#define	SCT_VIDEO	4

#define	RSV			1

#define	VS_BITS_NUMBER		( ( 14 * 4 + 10 * 2 ) * 5 * 8 )		// = 3040

//------------------------------------------------------------------------------
// Header data definition
//------------------------------------------------------------------------------

typedef	struct {
	BYTE	ID3;
	BYTE	ID4;
	BYTE	ID5;
	BYTE	ID6;
	BYTE	ID7;
	BYTE	Reserved[ 72 ];
} DATA_HEADER;

//------------------------------------------------------------------------------
// Subcode data definition
//------------------------------------------------------------------------------

typedef	struct {
	BYTE	ID0;
	BYTE	ID1;
	BYTE	Reserved;
	BYTE	data[ 5 ];
} SSYB;

typedef	struct {
	SSYB	ssyb[ 6 ];
	BYTE	Reserved[ 29 ];
} DATA_SUBCODE;

//------------------------------------------------------------------------------
// VAUX data definition
//------------------------------------------------------------------------------

typedef	struct {
	BYTE	data[ 77 ];
} DATA_VAUX;

//------------------------------------------------------------------------------
// Audio data definition
//------------------------------------------------------------------------------

typedef	struct {
	BYTE	data[ 77 ];
} DATA_AUDIO;

//------------------------------------------------------------------------------
// Video data definition
//------------------------------------------------------------------------------

typedef	struct {
	BYTE	STA_QNO;
	BYTE	Y0[ 14 ];
	BYTE	Y1[ 14 ];
	BYTE	Y2[ 14 ];
	BYTE	Y3[ 14 ];
	BYTE	CR[ 10 ];
	BYTE	CB[ 10 ];
} DATA_VIDEO;

typedef	struct {
	BYTE		ID0;
	BYTE		ID1;
	BYTE		ID2;
	union {
		DATA_HEADER		header;
		DATA_SUBCODE	subcode;
		DATA_VAUX		vaux;
		DATA_AUDIO		audio;
		DATA_VIDEO		video;
	}
#ifdef __BEOS__
	u;
#define header u.header
#define subcode u.subcode
#define vaux u.vaux
#define video u.video
#define audio u.audio
#else
	;
#endif
} DIF_BLOCK, *PDIF_BLOCK;

