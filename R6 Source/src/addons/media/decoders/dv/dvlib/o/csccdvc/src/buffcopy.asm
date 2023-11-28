;=============================================================================
; Description:
;
;
;
;
;
; Copyright:
;	Copyright (c) 1998 Canopus Co.,Ltd. All Rights Reserved.
;	Developed in San Jose, CA, U.S.A.
; History:
;	10/23/98 Tom > Creaded.
;
;=============================================================================

.nolist
include iaxmm.inc                   ; IAMMX Emulator Macros
.list

.586
.mmx
.model flat

_DATA SEGMENT PARA PUBLIC USE32 'DATA'


extrn	_PutImageBuffer	:dword
extrn	_PutImageStride	:dword

UVOffset		dw	128,128,128,128

_DATA ENDS



DCT_BUFFER_STRIDE	equ	32
DCT_DATA_SIZE		equ	word
YUV_SHIFT		equ	3
SIZE_YUY2		equ	2


_TEXT SEGMENT PARA PUBLIC USE32 'CODE'

;-----------------------------------------------------------------------------
; Put decompressed data to YUY2 surface. (use MMX)
;-----------------------------------------------------------------------------

public	_xmmPutImage525_YUY2_eo@28
StartX	equ [ebp+8]
StartY	equ [ebp+12]
SizeX	equ [ebp+16]
SizeY	equ [ebp+20]
pY	equ [ebp+24]
pUV	equ [ebp+28]
pParam	equ [ebp+32]
_xmmPutImage525_YUY2_eo@28 proc near
	push		ebp
	mov		ebp,esp
	push		esi
	push		edi
	push		ebx

	mov		ebx,_PutImageStride
	mov		eax,StartY
	mul		ebx
	mov		edx,StartX
	add		eax,_PutImageBuffer
	lea		edi,[eax+edx*2]

	mov		eax,SizeX
	sub		ebx,eax
	sub		ebx,eax
	mov		StartX,ebx

	shr		eax,1				; size / 2
	mov		esi,pY
	neg		eax				; - ( size / 2 )
	mov		ebx,pUV
	lea		edx,[DCT_BUFFER_STRIDE*2+eax*4]	; StrideY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	add		eax,DCT_BUFFER_STRIDE*2		; StrideUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * 2;
	movq		mm3,qword ptr UVOffset
LoopY:
	mov		ecx,SizeX
LoopX:
	movq		mm1,[ebx+0*DCT_DATA_SIZE]	; mm1 = v3:v2:v1:v0
	movq		mm0,[ebx+8*DCT_DATA_SIZE]	; mm0 = u3:u2:u1:u0
	psraw		mm0,YUV_SHIFT
	psraw		mm1,YUV_SHIFT
	paddw		mm0,mm3
	paddw		mm1,mm3
	packuswb	mm0,mm1				; mm0 = v3:v2:v1:v0:u3:u2:u1:u0
	movq		mm1,mm0
	punpcklbw	mm0,mm0				; mm0 = u3:u3:u2:u2:u1:u1:u0:u0
	punpckhbw	mm1,mm1				; mm1 = v3:v3:v2:v2:v1:v1:v0:v0
	movq		mm2,mm0				; mm2 = u3:u3:u2:u2:u1:u1:u0:u0
	movq		mm4,[esi+0*DCT_DATA_SIZE]
	punpcklbw	mm0,mm1				; mm0 = v1:u1:v1:u1:v0:u0:v0:u0
	punpckhbw	mm2,mm1				; mm2 = v3:u3:v3:u3:v2:u2:v2:u2
	movq		mm5,[esi+4*DCT_DATA_SIZE]
	psraw		mm4,YUV_SHIFT
	psraw		mm5,YUV_SHIFT
	movq		mm6,[esi+8*DCT_DATA_SIZE]
	packuswb	mm4,mm5				; mm4 = y7:y6:y5:y4:y3:y2:y1:y0
	movq		mm5,mm4				; mm5 = y7:y6:y5:y4:y3:y2:y1:y0
	movq		mm7,[esi+12*DCT_DATA_SIZE]
	punpcklbw	mm4,mm0				; mm4 = v0:y3:u0:y2:v0:y1:u0:y0
	punpckhbw	mm5,mm0				; mm5 = v1:y7:u1:y6:v1:y5:u1:y4
	movntq		[edi+0*SIZE_YUY2],mm4
	psraw		mm6,YUV_SHIFT
	psraw		mm7,YUV_SHIFT
	movntq		[edi+4*SIZE_YUY2],mm5
	packuswb	mm6,mm7				; mm4 = y7:y6:y5:y4:y3:y2:y1:y0
	movq		mm7,mm6				; mm5 = y7:y6:y5:y4:y3:y2:y1:y0
	punpcklbw	mm6,mm2				; mm4 = v0:y3:u0:y2:v0:y1:u0:y0
	punpckhbw	mm7,mm2				; mm5 = v1:y7:u1:y6:v1:y5:u1:y4
	movntq		[edi+ 8*SIZE_YUY2],mm6
	movntq		[edi+12*SIZE_YUY2],mm7

	add		ebx,2*4
	add		esi,2*4*4
	add		edi,2*4*4
	sub		ecx,4*4
	jnz		loopX
	add		ebx,eax					; StrideUV
	add		esi,edx
	add		edi,StartX
	dec		dword ptr SizeY
	jnz		LoopY
	emms

	pop		ebx
	pop		edi
	pop		esi
	pop		ebp
	ret		28
_xmmPutImage525_YUY2_eo@28 endp


_TEXT ENDS
END

