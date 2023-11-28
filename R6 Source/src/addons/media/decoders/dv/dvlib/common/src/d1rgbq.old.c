//=============================================================================
// Description:
//		Software DV codec render engine for RGB32.
//
//
//
//
// Copyright:
//		Copyright (c) 1998 Canopus Co.,Ltd. All Rights Reserved.
//		Developed in San Jose, CA, U.S.A.
// History:
//		09/10/98 Tom > Creaded from RGBT routine.
//
//=============================================================================
#include <windows.h>
#include "csedv.h"
#include "putimage.h"
#include "yuvtorgb.h"

#define	USE_ASSEMBLER		1	// (1 default)

#define	SIZE_RGBQ			4
#define	SCALE( d, s )		( ( ( d ) + ( 1 << ( ( s ) - 1 ) ) ) >> ( s ) )
#define	CLIP( rgb )			( ( BYTE )( rgb < 0 ? 0 : rgb > 255 ? 255 : rgb ) )

//------------------------------------------------------------------------------
// Call back function for writing decompressed RGB image data.
//------------------------------------------------------------------------------

void PASCAL PutImage525_RGBQ( PBYTE PutImageBuffer,int StartX, int StartY, int SizeX, int SizeY, int PutImageStride,PDCT_DATA pY, PDCT_DATA pUV )
{
	PBYTE	pDestBuffer;
	int		SkipRGB, SkipY, SkipUV;
	int		data, x, y, u, v, Ruv, Guv, Buv, i;

	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * SIZE_RGBQ;
	SkipRGB = PutImageStride - SizeX * SIZE_RGBQ;
	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	SizeX >>= 2;
	SkipUV = ( DCT_BUFFER_STRIDE - SizeX ) * 2;

	for( ; SizeY ; SizeY-- )
	{
		for( x = SizeX ; x ; x-- )
		{
			v = *pUV;
			u = *( pUV + 8 );
			Ruv = SCALE( v * R_FROM_V               , COEFF_PRECISION );
			Guv = SCALE( u * G_FROM_U + v * G_FROM_V, COEFF_PRECISION );
			Buv = SCALE( u * B_FROM_U               , COEFF_PRECISION );

			for( i = 4 ; i ; i-- )
			{
				y = *pY;
				data = SCALE( y + Ruv, DCT_SHIFT );
				*( pDestBuffer + 2 ) = CLIP( data );
				data = SCALE( y + Guv, DCT_SHIFT );
				*( pDestBuffer + 1 ) = CLIP( data );
				data = SCALE( y + Buv, DCT_SHIFT );
				*( pDestBuffer + 0 ) = CLIP( data );
				pDestBuffer += SIZE_RGBQ;
				pY++;
			}
			pUV++;
		}
		( PBYTE )pDestBuffer += SkipRGB;
		( PBYTE )pY += SkipY;
		( PBYTE )pUV += SkipUV;
	}
}

void asmPutImage525_RGBQ(PBYTE pDestBuffer,PBYTE pY,PBYTE pUV,int SizeX,int SizeY,int SkipRGB,int SkipY,int SkipUV)
{
}
//------------------------------------------------------------------------------
// Call back function for writing decompressed RGB image data.
//------------------------------------------------------------------------------

void PASCAL mmxPutImage525_RGBQ( PBYTE PutImageBuffer,int StartX, int StartY, int SizeX, int SizeY, int PutImageStride,PDCT_DATA pY, PDCT_DATA pUV )
{
	PBYTE	pDestBuffer;
	int	SkipRGB, SkipY, SkipUV;

	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * SIZE_RGBQ;
	SkipRGB = PutImageStride - SizeX * SIZE_RGBQ;
	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * sizeof( DCT_DATA );
	SkipUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * sizeof( DCT_DATA );
	SizeX >>= 3;

	asmPutImage525_RGBQ(pDestBuffer,(PBYTE)pY,(PBYTE)pUV,SizeX,SizeY,SkipRGB,SkipY,SkipUV);
#ifndef __BEOS__
	mmxDoIt(
	_asm {
		mov			edi,pDestBuffer
		mov			esi,pY
		mov			ecx,pUV
	LoopY:
		mov			eax,SizeX
	LoopX:
		push		eax
		movd		mm2,[ecx]					; v1:v0
		movd		mm3,[ecx+8*(TYPE DCT_DATA)]	; u1:u0
		movq		mm0,mm3
		movq		mm4,mm2
		pmulhw		mm2,mmxRFromV
		pmulhw		mm3,mmxBFromU
		pmulhw		mm4,mmxGFromV
		pmulhw		mm0,mmxGFromU
		paddw		mm4,mm0
		punpcklwd	mm2,mm2			; v1:v1:v0:v0
		punpcklwd	mm3,mm3			; v1:v1:v0:v0
		punpcklwd	mm4,mm4			; v1:v1:v0:v0
		movq		mm5,mm2
		movq		mm6,mm3
		movq		mm7,mm4
		punpcklwd	mm2,mm2			; uvR0:uvR0:uvR0:uvR0
		punpcklwd	mm3,mm3			; uvB0:uvB0:uvB0:uvB0
		punpcklwd	mm4,mm4			; uvG0:uvG0:uvG0:uvG0
		punpckhwd	mm5,mm5			; uvR1:uvR1:uvR1:uvR1
		punpckhwd	mm6,mm6			; uvB1:uvB1:uvB1:uvB1
		punpckhwd	mm7,mm7			; uvG1:uvG1:uvG1:uvG1

		movq		mm1,[esi]		; y3:y2:y1:y0
		psraw		mm1,DCT_SHIFT
		paddw		mm2,mm1			; mm2 = r3:r2:r1:r0
		paddw		mm3,mm1
		paddw		mm4,mm1
		packuswb	mm2,mm2			; mm2 = ??:??:??:??:r3:r2:r1:r0
		packuswb	mm3,mm3			; mm3 = ??:??:??:??:b3:b2:b1:b0
		packuswb	mm4,mm4			; mm4 = ??:??:??:??:g3:g2:g1:g0

		punpcklbw	mm3,mm4			; mm3 = g3:b3:g2:b2:g1:b1:g0:b0
		punpcklbw	mm2,mm2			; mm2 = r3:r3:r2:r2:r1:r1:r0:r0
		movq		mm1,mm3
		punpcklwd	mm3,mm2			; mm3 = r1:r1:g1:b1:r0:r0:g0:b0
		punpckhwd	mm1,mm2			; mm1 = r3:r3:g3:b3:r2:r2:g2:b2

		movq		[edi+SIZE_RGBQ*0],mm3
		movq		[edi+SIZE_RGBQ*2],mm1

		movq		mm1,[esi+4*(TYPE DCT_DATA)]	; y3:y2:y1:y0
		psraw		mm1,DCT_SHIFT
		paddw		mm5,mm1
		paddw		mm6,mm1
		paddw		mm7,mm1
		packuswb	mm5,mm5
		packuswb	mm6,mm6
		packuswb	mm7,mm7

		punpcklbw	mm6,mm7			; mm6 = g3:b3:g2:b2:g1:b1:g0:b0
		punpcklbw	mm5,mm5			; mm5 = r3:r3:r2:r2:r1:r1:r0:r0
		movq		mm1,mm6
		punpcklwd	mm6,mm5			; mm6 = r1:r1:g1:b1:r0:r0:g0:b0
		punpckhwd	mm1,mm5			; mm1 = r3:r3:g3:b3:r2:r2:g2:b2
		movq		[edi+SIZE_RGBQ*4],mm6
		movq		[edi+SIZE_RGBQ*6],mm1

		add			ecx,2*(TYPE DCT_DATA)
		add			esi,8*(TYPE DCT_DATA)
		add			edi,8*SIZE_RGBQ
		pop			eax
		dec			eax
		jne			LoopX

		add			ecx,SkipUV
		add			esi,SkipY
		add			edi,SkipRGB
		dec			SizeY
		jne			LoopY
		emms
	}
#endif
}

//------------------------------------------------------------------------------
// Call back function for writing decompressed RGB image data.
//------------------------------------------------------------------------------

void PASCAL PutImage625_RGBQ( PBYTE PutImageBuffer,int StartX, int StartY, int PutImageStride,PDCT_DATA pY )
{
	PBYTE	pDestBuffer;
	int	SkipRGB, data, xs, ys, y, u, v, Ruv, Guv, Buv;
	PDCT_DATA pUV;

	pUV = pY + 16;
	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * SIZE_RGBQ;
	SkipRGB = PutImageStride * 2 - 8 * 2 * SIZE_RGBQ;

	for( ys = 8 ; ys ; ys-- )
	{
		for( xs = 8 ; xs ; xs-- )
		{
			v = *pUV;
			u = *( pUV + 8 );
			Ruv = SCALE( v * R_FROM_V               , COEFF_PRECISION );
			Guv = SCALE( u * G_FROM_U + v * G_FROM_V, COEFF_PRECISION );
			Buv = SCALE( u * B_FROM_U               , COEFF_PRECISION );

			y = *pY;
			data = SCALE( y + Ruv, DCT_SHIFT );
			*( pDestBuffer + 2 ) = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			*( pDestBuffer + 1 ) = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			*( pDestBuffer + 0 ) = CLIP( data );

			y = *( pY + 1 );
			data = SCALE( y + Ruv, DCT_SHIFT );
			*( pDestBuffer + SIZE_RGBQ + 2 ) = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			*( pDestBuffer + SIZE_RGBQ + 1 ) = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			*( pDestBuffer + SIZE_RGBQ + 0 ) = CLIP( data );

			y = *( pY + DCT_BUFFER_STRIDE );
			data = SCALE( y + Ruv, DCT_SHIFT );
			*( pDestBuffer + PutImageStride + 2 ) = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			*( pDestBuffer + PutImageStride + 1 ) = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			*( pDestBuffer + PutImageStride + 0 ) = CLIP( data );

			y = *( pY + DCT_BUFFER_STRIDE + 1 );
			data = SCALE( y + Ruv, DCT_SHIFT );
			*( pDestBuffer + PutImageStride + SIZE_RGBQ + 2 ) = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			*( pDestBuffer + PutImageStride + SIZE_RGBQ + 1 ) = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			*( pDestBuffer + PutImageStride + SIZE_RGBQ + 0 ) = CLIP( data );

			pDestBuffer += SIZE_RGBQ * 2;
			pY += 2;
			pUV++;
		}
		pDestBuffer += SkipRGB;
		pY += DCT_BUFFER_STRIDE * 2 - 8 * 2;
		pUV += DCT_BUFFER_STRIDE - 8;
	}
}

//------------------------------------------------------------------------------
// Call back function for writing decompressed RGB image data.
//------------------------------------------------------------------------------

void PASCAL mmxPutImage625_RGBQ( PBYTE PutImageBuffer,int StartX, int StartY, int PutImageStride,PDCT_DATA pY )
{
	PBYTE	pDestBuffer;
	int	SkipRGB, xs, ys;
	PDCT_DATA pUV;

	pUV = pY + 16;
	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * SIZE_RGBQ;
	SkipRGB = PutImageStride * 2 - 8 * 2 * SIZE_RGBQ;
	ys = 8;

#ifndef __BEOS__
	_asm {
		mov			edi,pDestBuffer
		mov			esi,pY
		mov			ecx,PutImageStride
	LoopY:
		mov			xs,4
	LoopX:
		mov			eax,pUV
		movd		mm2,[eax]			; v1:v0
		movd		mm3,[eax+8*(TYPE DCT_DATA)]	; u1:u0

		movq		mm0,mm3
		movq		mm4,mm2
		pmulhw		mm2,mmxRFromV
		pmulhw		mm3,mmxBFromU
		pmulhw		mm4,mmxGFromV
		pmulhw		mm0,mmxGFromU
		paddw		mm4,mm0
		punpcklwd	mm2,mm2			; uvR1:uvR1:uvR0:uvR0
		punpcklwd	mm3,mm3			; uvB1:uvB1:uvB0:uvB0
		punpcklwd	mm4,mm4			; uvG1:uvG1:uvG0:uvG0

		movq		mm5,[esi]		; y03:y02:y01:y00
		movq		mm1,[esi+DCT_BUFFER_STRIDE*(TYPE DCT_DATA)]	; y13:y12:y11:y10
		psraw		mm5,DCT_SHIFT
		psraw		mm1,DCT_SHIFT
		movq		mm6,mm5
		movq		mm7,mm5

		paddw		mm5,mm2
		paddw		mm6,mm3
		paddw		mm7,mm4
		paddw		mm2,mm1
		paddw		mm3,mm1
		paddw		mm4,mm1

		packuswb	mm5,mm5
		packuswb	mm6,mm6
		packuswb	mm7,mm7

		punpcklbw	mm6,mm7			; mm6 = g3:b3:g2:b2:g1:b1:g0:b0
		punpcklbw	mm5,mm5			; mm5 = r3:r3:r2:r2:r1:r1:r0:r0
		movq		mm1,mm6
		punpcklwd	mm6,mm5			; mm6 = r1:r1:g1:b1:r0:r0:g0:b0
		punpckhwd	mm1,mm5			; mm1 = r3:r3:g3:b3:r2:r2:g2:b2

		movq		[edi+0],mm6
		movq		[edi+8],mm1

		packuswb	mm2,mm2			; mm2 = ??:??:??:??:r3:r2:r1:r0
		packuswb	mm3,mm3			; mm3 = ??:??:??:??:b3:b2:b1:b0
		packuswb	mm4,mm4			; mm4 = ??:??:??:??:g3:g2:g1:g0

		punpcklbw	mm3,mm4			; mm3 = g3:b3:g2:b2:g1:b1:g0:b0
		punpcklbw	mm2,mm2			; mm2 = r3:r3:r2:r2:r1:r1:r0:r0
		movq		mm1,mm3
		punpcklwd	mm3,mm2			; mm3 = r1:r1:g1:b1:r0:r0:g0:b0
		punpckhwd	mm1,mm2			; mm1 = r3:r3:g3:b3:r2:r2:g2:b2

		movq		[edi+ecx+0],mm3
		movq		[edi+ecx+8],mm1

		add			pUV,2*(TYPE DCT_DATA)
		add			esi,4*(TYPE DCT_DATA)
		add			edi,4*SIZE_RGBQ

		dec			xs
		jne			LoopX

		add			pUV,( DCT_BUFFER_STRIDE - 8 ) * (TYPE DCT_DATA)
		add			esi,( DCT_BUFFER_STRIDE * 2 - 8 * 2 ) * (TYPE DCT_DATA)
		add			edi,SkipRGB
		dec			ys
		jne			LoopY
		emms
	}
#endif
}
