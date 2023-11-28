//=============================================================================
// Description:
//
//
//
//
//
// Copyright:
//		Copyright (c) 1996,98 Canopus Co.,Ltd. All Rights Reserved.
//
// History:
//		1997 Tom > Creaded.
//
//=============================================================================
#include <windows.h>
#include "csedv.h"

#define	YUV_SHIFT	3
#define	SIZE_YUY2	2

extern	int		PutImageStride;
extern	PBYTE	PutImageBuffer;

static	short	UVOffset[ 4 ] = { 128, 128, 128, 128 };

//------------------------------------------------------------------------------
// Put decompressed data to YUY2 surface.
//------------------------------------------------------------------------------

void PASCAL PutImage525_YUY2( int StartX, int StartY, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pUV )
{
	PBYTE	pDestBuffer;
	int	SkipDest, StrideY, StrideUV;

	StrideY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	StrideUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * 2;
	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * 2;
	SkipDest = PutImageStride - SizeX * 2;

	_asm {
		mov		ch,byte ptr SizeY
		mov		esi,pY
		mov		ebx,pUV
		mov		edi,pDestBuffer
	LoopY:
		mov		cl,byte ptr SizeX
	LoopX:
		mov		dx,[ebx]		; V
		mov		ax,[ebx+16]		; U
		sar		dx,3
		sar		ax,3
		add		dx,128
		add		ax,128
		test	dh,dh
		jz		okU
		sar		dx,15
		not		dl
	okU:
		test	ah,ah
		jz		okV
		sar		ax,15
		not		al
	okV:
		mov		dh,al			; DH:DL = U:V
		mov		eax,[esi]		; y0
		sar		ax,3
		test	ah,ah
		jz		okY0
		sar		ax,15
		not		al
	okY0:
		mov		ah,dh
		ror		eax,16			; y1
		sar		ax,3
		test	ah,ah
		jz		okY1
		sar		ax,15
		not		al
	okY1:
		mov		ah,dl
		ror		eax,16
		mov		[edi],eax

		mov		eax,[esi+4]		; y2
		sar		ax,3
		test	ah,ah
		jz		okY2
		sar		ax,15
		not		al
	okY2:
		mov		ah,dh
		ror		eax,16			; y3
		sar		ax,3
		test	ah,ah
		jz		okY3
		sar		ax,15
		not		al
	okY3:
		mov		ah,dl
		ror		eax,16
		mov		[edi+4],eax
		add		ebx,2
		add		esi,2*4
		add		edi,2*4
		sub		cl,4
		jnz		loopX
		add		ebx,StrideUV
		add		esi,StrideY
		add		edi,SkipDest
		dec		ch
		jnz		LoopY
	}
}

//------------------------------------------------------------------------------
// Put decompressed data to YUY2 surface. (use MMX)
//------------------------------------------------------------------------------

void PASCAL mmxPutImage525_YUY2( int StartX, int StartY, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pUV )
{
	PBYTE	pDestBuffer;
	int		SkipDest;

	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * 2;
	SkipDest = PutImageStride - SizeX * 2;

	_asm {
		mov			eax,SizeX
		mov			ch,byte ptr SizeY
		shr			eax,1							; size / 2
		mov			esi,pY
		neg			eax								; - ( size / 2 )
		mov			ebx,pUV
		lea			edx,[DCT_BUFFER_STRIDE*2+eax*4]	; StrideY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
		mov			edi,pDestBuffer
		add			eax,DCT_BUFFER_STRIDE*2			; StrideUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * 2;
		movq		mm3,UVOffset
	LoopY:
		mov			cl,byte ptr SizeX
	LoopX:
		movq		mm1,[ebx+0*(TYPE DCT_DATA)]	; mm1 = v3:v2:v1:v0
		movq		mm0,[ebx+8*(TYPE DCT_DATA)]	; mm0 = u3:u2:u1:u0
		psraw		mm0,YUV_SHIFT
		psraw		mm1,YUV_SHIFT
		paddw		mm0,mm3
		paddw		mm1,mm3
		packuswb	mm0,mm1				; mm0 = v3:v2:v1:v0:u3:u2:u1:u0
		movq		mm1,mm0
		punpcklbw	mm0,mm0				; mm0 = u3:u3:u2:u2:u1:u1:u0:u0
		punpckhbw	mm1,mm1				; mm1 = v3:v3:v2:v2:v1:v1:v0:v0
		movq		mm2,mm0				; mm2 = u3:u3:u2:u2:u1:u1:u0:u0
		movq		mm4,[esi+0*(TYPE DCT_DATA)]
		punpcklbw	mm0,mm1				; mm0 = v1:u1:v1:u1:v0:u0:v0:u0
		punpckhbw	mm2,mm1				; mm2 = v3:u3:v3:u3:v2:u2:v2:u2
		movq		mm5,[esi+4*(TYPE DCT_DATA)]
		psraw		mm4,YUV_SHIFT
		psraw		mm5,YUV_SHIFT
		movq		mm6,[esi+8*(TYPE DCT_DATA)]
		packuswb	mm4,mm5				; mm4 = y7:y6:y5:y4:y3:y2:y1:y0
		movq		mm5,mm4				; mm5 = y7:y6:y5:y4:y3:y2:y1:y0
		movq		mm7,[esi+12*(TYPE DCT_DATA)]
		punpcklbw	mm4,mm0				; mm4 = v0:y3:u0:y2:v0:y1:u0:y0
		punpckhbw	mm5,mm0				; mm5 = v1:y7:u1:y6:v1:y5:u1:y4
		movq		[edi+0*SIZE_YUY2],mm4
		psraw		mm6,YUV_SHIFT
		psraw		mm7,YUV_SHIFT
		movq		[edi+4*SIZE_YUY2],mm5
		packuswb	mm6,mm7				; mm4 = y7:y6:y5:y4:y3:y2:y1:y0
		movq		mm7,mm6				; mm5 = y7:y6:y5:y4:y3:y2:y1:y0
		punpcklbw	mm6,mm2				; mm4 = v0:y3:u0:y2:v0:y1:u0:y0
		punpckhbw	mm7,mm2				; mm5 = v1:y7:u1:y6:v1:y5:u1:y4
		movq		[edi+ 8*SIZE_YUY2],mm6
		movq		[edi+12*SIZE_YUY2],mm7

		add			ebx,2*4
		add			esi,2*4*4
		add			edi,2*4*4
		sub			cl,4*4
		jnz			loopX
		add			ebx,eax					; StrideUV
		add			esi,edx
		add			edi,SkipDest
		dec			ch
		jnz			LoopY
		emms
	}
}

//------------------------------------------------------------------------------
// Put decompressed data to YUY2 surface.
// Tom (04/08/98)
//------------------------------------------------------------------------------

void PASCAL PutImage625_YUY2( int StartX, int StartY, PDCT_DATA pY )
{
	PBYTE	pDestBuffer;
	int		DestSkip;
	int		LoopXCount, LoopYCount;

	DestSkip = ( PutImageStride * 2 ) - 16 * 2;
	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * 2;

	_asm {
		mov		LoopYCount,8
		mov		esi,pY
		lea		ebx,[esi+16*2]
		mov		edi,pDestBuffer
	LoopY:
		mov		LoopXCount,8
	LoopX:
		movsx	edx,word ptr [ebx]		; ssssssss ssssssss sssssvvv vvvvvvvv
		movsx	eax,word ptr [ebx+8*2]	; ssssssss ssssssss sssssuuu uuuuuuuu

		add		edx,400h
		add		eax,400h

		test	edx,0fffff800h
		jz		okV
		sar		edx,31
		not		edx
	okV:
		test	eax,0fffff800h
		jz		okU
		sar		eax,31
		not		eax
	okU:
		shl		edx,5+8+8
		shl		eax,5
		and		edx,0ff000000h
		and		eax,00000ff00h
		or		edx,eax					; edx = VV:00:UU:00

		mov		eax,[esi]				; sssssyyy yyyyyyyy sssssyyy yyyyyyyy
		movsx	ecx,ax					; ssssssss ssssssss sssssyyy yyyyyyyy
		test	ecx,0fffff800h
		jz		okY00
		sar		ecx,31
		not		ecx
		and		ecx,0007ffh
	okY00:
		test	eax,0f8000000h
		jz		okY01
		sar		eax,31
		not		eax
	okY01:
		and		eax,07f80000h
		sar		ecx,3
		sar		eax,3
		or		eax,ecx
		or		eax,edx
		mov		[edi],eax

		mov		eax,[esi+DCT_BUFFER_STRIDE*2]	; y2
		movsx	ecx,ax
		test	ecx,0fffff800h
		jz		okY10
		sar		ecx,31
		not		ecx
		and		ecx,0007ffh
	okY10:
		test	eax,0f8000000h
		jz		okY11
		sar		eax,31
		not		eax
	okY11:
		and		eax,07f80000h
		sar		ecx,3
		sar		eax,3
		add		edi,PutImageStride
		or		eax,ecx
		or		eax,edx
		mov		[edi],eax
		sub		edi,PutImageStride
		add		ebx,2
		add		esi,2*2
		add		edi,2*2
		dec		LoopXCount
		jnz		loopX
		add		ebx,(DCT_BUFFER_STRIDE-8)*2
		add		esi,(DCT_BUFFER_STRIDE*2-8*2)*2
		add		edi,DestSkip
		dec		LoopYCount
		jnz		LoopY
	}
}

//------------------------------------------------------------------------------
// Put decompressed data to YUY2 surface. (use MMX)
//------------------------------------------------------------------------------

void PASCAL mmxPutImage625_YUY2( int StartX, int StartY, PDCT_DATA pY )
{
	PBYTE	pDestBuffer;

	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * 2;
	_asm {
		mov			esi,pY
		mov			edx,PutImageStride
		lea			ebx,[esi+16*(TYPE DCT_DATA)]
		mov			cl,8
		mov			edi,pDestBuffer
		movq		mm7,UVOffset
	LoopY:
		movq		mm0,[ebx+ 0*(TYPE DCT_DATA)]	; mm0 = v3:v2:v1:v0
		movq		mm1,[ebx+ 4*(TYPE DCT_DATA)]	; mm1 = v7:v6:v5:v4
		movq		mm2,[ebx+ 8*(TYPE DCT_DATA)]	; mm2 = u3:u2:u1:u0
		movq		mm3,[ebx+12*(TYPE DCT_DATA)]	; mm3 = u7:u6:u5:u4

		psraw		mm0,YUV_SHIFT
		psraw		mm1,YUV_SHIFT
		psraw		mm2,YUV_SHIFT
		psraw		mm3,YUV_SHIFT
		paddw		mm0,mm7
		paddw		mm1,mm7
		paddw		mm2,mm7
		paddw		mm3,mm7
		packuswb	mm0,mm1				; mm0 = v7:v6:v5:v4:v3:v2:v1:v0
		packuswb	mm2,mm3				; mm2 = u7:u6:u5:u4:u3:u2:u1:u0
		movq		mm1,mm2
		punpcklbw	mm1,mm0				; mm1 = v3:u3:v2:u2:v1:u1:v0:u0
		punpckhbw	mm2,mm0				; mm2 = v7:u7:v6:u6:v5:u5:v4:u4

		movq		mm4,[esi+0*(TYPE DCT_DATA)]
		movq		mm5,[esi+4*(TYPE DCT_DATA)]
		movq		mm0,[esi+8*(TYPE DCT_DATA)]
		movq		mm3,[esi+12*(TYPE DCT_DATA)]
		psraw		mm4,YUV_SHIFT
		psraw		mm5,YUV_SHIFT
		psraw		mm0,YUV_SHIFT
		psraw		mm3,YUV_SHIFT
		packuswb	mm4,mm5				; mm4 = y7:y6:y5:y4:y3:y2:y1:y0
		packuswb	mm0,mm3				; mm0 = y7:y6:y5:y4:y3:y2:y1:y0
		movq		mm5,mm4				; mm5 = y7:y6:y5:y4:y3:y2:y1:y0
		movq		mm3,mm0				; mm3 = y7:y6:y5:y4:y3:y2:y1:y0
		punpcklbw	mm4,mm1				; mm4 = v1:y3:u1:y2:v0:y1:u0:y0
		punpckhbw	mm5,mm1				; mm5 = v3:y7:u3:y6:v2:y5:u2:y4
		punpcklbw	mm0,mm2				; mm4 = v0:y3:u0:y2:v0:y1:u0:y0
		punpckhbw	mm3,mm2				; mm5 = v1:y7:u1:y6:v1:y5:u1:y4
		movq		[edi+0*SIZE_YUY2],mm4
		movq		[edi+4*SIZE_YUY2],mm5
		movq		[edi+8*SIZE_YUY2],mm0
		movq		[edi+12*SIZE_YUY2],mm3

		movq		mm4,[esi+(DCT_BUFFER_STRIDE+ 0)*(TYPE DCT_DATA)]
		movq		mm5,[esi+(DCT_BUFFER_STRIDE+ 4)*(TYPE DCT_DATA)]
		movq		mm0,[esi+(DCT_BUFFER_STRIDE+ 8)*(TYPE DCT_DATA)]
		movq		mm3,[esi+(DCT_BUFFER_STRIDE+12)*(TYPE DCT_DATA)]
		psraw		mm4,YUV_SHIFT
		psraw		mm5,YUV_SHIFT
		psraw		mm0,YUV_SHIFT
		psraw		mm3,YUV_SHIFT
		packuswb	mm4,mm5				; mm4 = y7:y6:y5:y4:y3:y2:y1:y0
		packuswb	mm0,mm3				; mm0 = y7:y6:y5:y4:y3:y2:y1:y0
		movq		mm5,mm4				; mm5 = y7:y6:y5:y4:y3:y2:y1:y0
		movq		mm3,mm0				; mm3 = y7:y6:y5:y4:y3:y2:y1:y0
		punpcklbw	mm4,mm1				; mm4 = v1:y3:u1:y2:v0:y1:u0:y0
		punpckhbw	mm5,mm1				; mm5 = v3:y7:u3:y6:v2:y5:u2:y4
		punpcklbw	mm0,mm2				; mm4 = v0:y3:u0:y2:v0:y1:u0:y0
		punpckhbw	mm3,mm2				; mm5 = v1:y7:u1:y6:v1:y5:u1:y4
		movq		[edi+edx+ 0*SIZE_YUY2],mm4
		movq		[edi+edx+ 4*SIZE_YUY2],mm5
		movq		[edi+edx+ 8*SIZE_YUY2],mm0
		movq		[edi+edx+12*SIZE_YUY2],mm3

		lea			edi,[edi+edx*2]
		add			esi,DCT_BUFFER_STRIDE*2*2
		add			ebx,DCT_BUFFER_STRIDE*2
		dec			cl
		jnz			LoopY
		emms
	}
}
