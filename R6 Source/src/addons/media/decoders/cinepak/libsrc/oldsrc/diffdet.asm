; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/diffdet.asm 2.13 1994/10/23 17:22:41 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: diffdet.asm $
; Revision 2.13  1994/10/23 17:22:41  bog
; Try to allow big frames.
; Revision 2.12  1994/06/23  14:06:15  bog
; Difference of 8 bit is 9 bit.  Movsx was incorrect.
; 
; Revision 2.11  1994/05/25  13:18:27  bog
; Handle overflow in computing detail sharpness error.
; 
; Revision 2.10  1994/05/17  12:52:17  unknown
; Return longs in eax for WIN32.
; 
; Revision 2.9  1994/04/30  11:28:28  bog
; Note Mac 1.5 change, but ignore it.
; 
; Revision 2.8  1994/04/29  10:11:22  bog
; Flag difference to Peter code.
; 
; Revision 2.7  1993/09/02  16:44:52  timr
; Convert from MASM386 to ML 6.11.
; 
; Revision 2.6  1993/09/02  16:39:44  timr
; Handle potential overflow in detail computation.
; referenced above.
;
; Revision 2.5  1993/08/04  19:07:38  timr
; Both compressor and decompressor now work on NT.
; 
; Revision 2.4  1993/07/27  09:28:59  timr
; Finally assembles under NT MASM386.
; 
; Revision 2.3  1993/07/09  14:33:27  timr
; 2nd pass at Win32-izing.
; 
; Revision 2.2  1993/07/06  09:11:28  geoffs
; 1st pass WIN32'izing
; 
; Revision 2.1  93/06/09  14:32:17  bog
; Preserve FS and top halves of ESI and EDI.
; 
; Revision 2.0  93/06/01  14:14:08  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.9  93/04/21  15:47:49  bog
; Fix up copyright and disclaimer.
; 
; Revision 1.8  93/04/02  16:58:43  bog
; Oops.  Indexing with wrong register.
; 
; Revision 1.7  93/03/13  15:31:16  bog
; IMUL delivered wrong value when prev Y was 00 and this was FC.
; 
; Revision 1.6  93/01/27  17:27:38  geoffs
; Make sure assembly .model has farstack specified (DS != SS)
; 
; Revision 1.5  93/01/20  15:57:15  timr
; Do detail & difference list computations for the whole list of patches,
; not line by line.  Don't be stupid.
; 
; Revision 1.4  93/01/19  13:46:33  geoffs
; Computing YUVDetailInner stuff now like Peter had it
; 
; Revision 1.3  93/01/12  17:18:56  timr
; Missed one spot in converting YYYYUVs to VECTORs.
; 

ifndef	WIN32
FARPOINTER      STRUCT
off		WORD	?
sel		WORD	?
FARPOINTER      ENDS

	.model	small, c, farstack
	.386

	extrn	flatSel:WORD
	extrn	PASCAL GetSelectorBase:FAR
else
	.386
	.model	small,c
endif

	.code

	extrn	LongSquares:	DWORD

ifndef	WIN32
SrcSEG	textequ	<fs>
DiffSEG	textequ	<es>
else
SrcSEG	textequ	<ds>
DiffSEG	textequ	<ds>
endif

; DiffDet.ASM - Assembly helpers for the Difference list and Detail list
;   computations.

; Copyright (C) 1993, SuperMac Technology.  All Rights Reserved.

DIFFLIMIT	equ	07fffh

SumDiff	Macro	op, n, dbl
	mov	bl, ds:[esi][n]
	sub	bl, SrcSEG:[eax][n]
	sbb	bh,bh
ifndef	WIN32
	shl	bx,2
	op	edx,LongSquares[bx]	;; add to patch running sum
else
	inc	bh		;; 0000ffxx->000000xx, 000000xx->000001xx
	op	edx,LongSquares[ebx*4][-00100h*4];; add to patch running sum
endif
	endm


ifndef	WIN32
YUVDifferenceInnerLoop	PROC	NEAR C PUBLIC uses ds esi edi fs,
	pOld: FAR PTR BYTE,
	pNew: FAR PTR BYTE,
	pDiffList: FAR PTR BYTE,
	count: DWORD

	xor	esi,esi
	xor	edi,edi
	xor	eax,eax
	lds	si, pOld
	les	di, pDiffList
	lfs	ax, pNew
else

YUVDifferenceInnerLoop	PROC	NEAR C PUBLIC uses ebx esi edi,
	pOld: PTR BYTE,
	pNew: PTR BYTE,
	pDiffList: PTR BYTE,
	count: DWORD

	mov	esi, pOld
	mov	edi, pDiffList
	mov	eax, pNew
endif

;     (fs:)eax	-> new source
;     ebx	working difference
;     ecx	running sum for entire stream
;     edx	running sum for each block
;     ds:esi	-> old source
;     (es:)edi	-> DiffList

; Byte offsets of the bytes in the patches are:

;      Y          U       V
;  0  1  8  9	 4 12	 5 13
;  2  3 10 11
; 16 17 24 25   20 28   21 29
; 18 19 26 27

	xor	ecx, ecx

innerloop:

ifdef	WIN32
	xor	ebx,ebx			; high half must be clear
endif

; add up the 3/16 guys

	SumDiff	mov, 0
	SumDiff	add, 9
	SumDiff	add, 18
	SumDiff	add, 27

	push	edx			; save 3/16 sum

; add in the 4/16 guys

	SumDiff	add, 4			; U
	SumDiff	add, 12
	SumDiff	add, 20
	SumDiff	add, 28
	SumDiff	add, 5			; V
	SumDiff	add, 13
	SumDiff	add, 21
	SumDiff	add, 29

	add	edx,edx

; add in the 2/16 guys

	SumDiff	add, 1
	SumDiff	add, 8
	SumDiff	add, 2
	SumDiff	add, 11
	SumDiff	add, 16
	SumDiff	add, 25
	SumDiff	add, 19
	SumDiff	add, 26

	add	edx,edx
	pop	ebx
	sub	edx,ebx

ifdef	WIN32
	xor	ebx,ebx			; high half must be clear
endif

; add in the 1/16 guys

	SumDiff	add, 3
	SumDiff	add, 10
	SumDiff	add, 17
	SumDiff	add, 24

	shr	edx, 4

;!!!!!!!!!!!!! sumY/8 if gray

	cmp	edx, DIFFLIMIT
	jl	@F
	mov	edx, DIFFLIMIT

@@:
	mov	DiffSEG:[edi], dx	; store in the difference list
	add	edi, 2
	add	ecx, edx		; add in to running sum

	add	esi, 32			; move to next patch
	add	eax, 32			; 4 * sizeof (VECTOR)

	dec	count
	jnz	innerloop

ifndef	WIN32
	mov	ax, cx			; DX:AX <= ECX
	shld	edx, ecx, 16		; pass running sum to caller
else
	mov	eax, ecx		; pass running sum to caller
endif

	ret

YUVDifferenceInnerLoop	ENDP


ifndef	WIN32
YUVDetailInnerLoop	PROC	NEAR C PUBLIC uses ds esi edi,
	pDetail:FARPOINTER,
	pSmooth:FARPOINTER,
	pDetailList:FARPOINTER,
	count: DWORD

; Map all ->s coming in to flat model

	push	pDetail.sel		; selector to map
	call	GetSelectorBase		; get its linear base
	shl	eax,16
	shrd	eax,edx,16		; EAX = 32 bit base of src ->
	movzx	esi,pDetail.off
	add	esi,eax			; ESI = linear -> detail list

	push	pSmooth.sel		; selector to map
	call	GetSelectorBase		; get its linear base
	shl	eax,16
	shrd	eax,edx,16		; EAX = 32 bit base of src ->
	movzx	edi,pSmooth.off
	add	edi,eax			; ESI = linear -> smooth list

	push	pDetailList.sel		; selector to map
	call	GetSelectorBase		; get its linear base
	shl	eax,16
	shrd	eax,edx,16		; EAX = 32 bit base of src ->
	mov	pDetailList.sel,0
	add	pDetailList,eax		; linear -> detail error list

	mov	ds,flatSel		; access to all memory
	assume	ds:nothing
else

YUVDetailInnerLoop	PROC	NEAR C PUBLIC uses ebx esi edi,
	pDetail:	PTR BYTE,
	pSmooth:	PTR BYTE,
	pDetailList:	PTR BYTE,
	count:		DWORD

; Map all ->s coming in to flat model

	mov	esi,pDetail		; -> detail list
	mov	edi,pSmooth		; -> smooth list
endif

; For each 4x4 patch in the tile:

foreachpatch:

; For each 2x2 within the 4x4:

	xor	ebx,ebx			; max error at start

	mov	ecx,00030000h		; we'll count until negative
	mov	cx,[edi][4]		; cl = U, ch = V

	push	ebp

innerloop:

	xor	edx,edx
	mov	dl,[esi]		; calculate mean of the 4 detail Y's
	add	dl,[esi][1]
	adc	dh,0
	add	dl,[esi][2]
	adc	dh,0
	add	dl,[esi][3]
	adc	dh,0
ifdef	WIN32
	shr	edx,2			; dl = mean of 4 detail Y's
else
	shr	dx,2			; dl = mean of 4 detail Y's
endif

;     ebx	max sharpness err this patch so far
;     ecx.lo	U, V
;     ecx.hi	count down 4 details in patch
;     edx	mean of Ys
;     ebp	accumulating sharpness error for this detail Y
;     esi	-> incoming detail vectors
;     edi	-> incoming smooth vectors

	xor	eax,eax			; clear high half

	mov	al,[esi]		; get Y0
	sub	al,dl			; Y0 - mean Y
	sbb	ah,ah			; 16 bit signed result
	imul	ax,ax			; square of diff; 16 bit pos result
	mov	ebp,eax			; accum in ebp

	mov	al,[esi][1]		; get Y1
	sub	al,dl			; Y1 - mean Y
	sbb	ah,ah			; 16 bit signed result
	imul	ax,ax			; square of diff; 16 bit pos result
	add	ebp,eax			; accum in ebp

	mov	al,[esi][2]		; get Y2
	sub	al,dl			; Y1 - mean Y
	sbb	ah,ah			; 16 bit signed result
	imul	ax,ax			; square of diff; 16 bit pos result
	add	ebp,eax			; accum in ebp

	mov	al,[esi][3]		; get Y3
	sub	al,dl			; Y1 - mean Y
	sbb	ah,ah			; 16 bit signed result
	imul	ax,ax			; square of diff; 16 bit pos result
	add	ebp,eax			; accum in ebp

	mov	al,[esi][4]		; get U
	sub	al,cl			; U - smooth U
	sbb	ah,ah			; sign extend
	imul	ax,ax			; square of diff; 16 bit pos result
	lea	ebp,[ebp][eax*4]	; accum in ebp

	mov	al,[esi][5]		; get V
	sub	al,ch			; V - smooth V
	sbb	ah,ah			; sign extend
	imul	ax,ax			; square of diff; 16 bit pos result
	lea	ebp,[ebp][eax*4]	; accum in ebp

	cmp	ebp,DIFFLIMIT
	jb	@F

	mov	ebp,DIFFLIMIT

@@:
	cmp	ebp,ebx
	jb	@F

	mov	ebx,ebp
@@:

	add	esi,8			; sizeof(VECTOR)
	sub	ecx,000010000h
	jge	innerloop

	pop	ebp
	mov	eax,pDetailList		; -> error list
	mov	[eax],bx		; store max error for 4x4 patch
	add	pDetailList,2		; bump to next error entry

	add	edi,8			; sizeof(VECTOR)
	dec	count
	jnz	foreachpatch

	ret
YUVDetailInnerLoop	ENDP

	end
