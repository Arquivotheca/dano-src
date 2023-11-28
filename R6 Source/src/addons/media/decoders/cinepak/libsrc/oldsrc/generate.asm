; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/generate.asm 2.6 1995/03/14 08:24:12 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: generate.asm $
; Revision 2.6  1995/03/14 08:24:12  bog
; 1.  No one was looking at the remembered previous frame's smooth
;     vectors, so there is no point in remembering them.
; 2.  We update the previous frame's remembered detail vectors to be as
;     refined (quantized) rather than as incoming, improving the decision
;     about what to update in the next frame.
; Revision 2.5  1994/10/23  17:22:48  bog
; Try to allow big frames.
; 
; Revision 2.4  1993/09/02  16:44:45  timr
; Convert from MASM386 to ML 6.11.
; 
; Revision 2.3  1993/08/04  19:07:50  timr
; Both compressor and decompressor now work on NT.
; 
; Revision 2.2  1993/07/09  14:33:54  timr
; 2nd pass at Win32-izing.
; 
; Revision 2.1  1993/07/06  09:11:42  geoffs
; 1st pass WIN32'izing
; 
; Revision 2.0  93/06/01  14:15:10  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.5  93/04/21  15:49:08  bog
; Fix up copyright and disclaimer.
; 
; Revision 1.4  93/01/26  09:32:09  geoffs
; Protect EBX around calls to GetSelectorBase
; 
; Revision 1.3  93/01/21  21:14:43  timr
; Clear the VECTOR.Class field as we copy.
; 
; Revision 1.2  93/01/21  20:58:17  timr
; Correct simple typos.  Now works.
; 
; Revision 1.1  93/01/21  18:17:05  timr
; Initial revision
; 
;
; CompactVideo Codec Toss unused vectors

ifndef	WIN32
	.model	small, c
	.386
else
	.386
	.model	small, c
endif

	include	cvcompre.inc

ifndef	WIN32
	extern	flatSel: WORD
	extern	PASCAL GetSelectorBase: FAR

OFF	textequ	<WORD PTR [0]>
SEL	textequ	<WORD PTR [2]>

	.code

GenerateVectors	PROC	NEAR C PUBLIC uses ds esi edi,
		lpC : PTR CCONTEXT,
		lpT : PTR TILECONTEXT

	LOCAL	pDetailIn : FAR PTR VECTOR
	LOCAL	pDetailOut: FAR PTR VECTOR
	LOCAL	pSmoothIn : FAR PTR VECTOR
	LOCAL	pSmoothOut: FAR PTR VECTOR
	LOCAL	nPatches  : DWORD
	LOCAL	FrameType : BYTE

	mov	si, lpC
	mov	di, lpT

pC	textequ	<(CCONTEXT PTR [si])>
pT	textequ	<(TILECONTEXT PTR [di])>

; Swap the pointers in pC->VBook [Detail].pVectors and pT->Detail.

	.errnz	Detail
	mov	eax, pC.ccVBook.vbpVectors
	mov	ebx, pT.tcpDetail
	mov	pT.tcpDetail, eax
	mov	pC.ccVBook.vbpVectors, ebx

; Convert the pointers to flat32.  Note the assumption that the OFFSETs
; in VBook.pVectors and pT.pDetail are 0.

	shr	ebx,16
	push	bx			; selector for 2nd call

	shr	eax, 16
	push	ax
	call	GetSelectorBase		; get its linear base -> dx:ax
	mov	OFF [pDetailIn], ax
	mov	SEL [pDetailIn], dx

	call	GetSelectorBase		; get its linear base -> dx:ax
	mov	OFF [pDetailOut], ax
	mov	SEL [pDetailOut], dx

; Get the pointer in pC->VBook [Smooth].pVectors

	.errnz	Smooth - 1

; Convert the pointer to flat32.

	push	SEL pC.ccVBook[type VECTORBOOK].vbpVectors
	call	GetSelectorBase		; get its linear base -> dx:ax
	mov	OFF [pSmoothIn], ax
	mov	SEL [pSmoothIn], dx
	mov	OFF [pSmoothOut], ax
	mov	SEL [pSmoothOut], dx

else

	.code

GenerateVectors	PROC	PUBLIC uses ebx esi edi,
		lpC : PTR CCONTEXT,
		lpT : PTR TILECONTEXT

	LOCAL	pDetailIn : PTR VECTOR
	LOCAL	pDetailOut: PTR VECTOR
	LOCAL	pSmoothIn : PTR VECTOR
	LOCAL	pSmoothOut: PTR VECTOR
	LOCAL	nPatches  : DWORD
	LOCAL	FrameType : BYTE

	mov	esi, lpC
	mov	edi, lpT

pC	textequ	<(CCONTEXT PTR [esi])>
pT	textequ	<(TILECONTEXT PTR [edi])>

; Swap the pointers in pC->VBook [Detail].pVectors and pT->Detail.

	.errnz	Detail

	mov	eax, pC.ccVBook.vbpVectors
	mov	ebx, pT.tcpDetail

	mov	pDetailIn,eax
	mov	pT.tcpDetail, eax

	mov	pDetailOut,ebx
	mov	pC.ccVBook.vbpVectors, ebx

; Get the pointer in pC->VBook [Smooth].pVectors

	.errnz	Smooth - 1

	mov	eax, pC.ccVBook[size VECTORBOOK].vbpVectors

	mov	pSmoothIn,eax
	mov	pSmoothOut,eax

endif

; Tuck the frame type in the stack frame.

	mov	al, pC.ccfType
	mov	FrameType, al
	mov	eax, pT.tcnPatches
	mov	nPatches, eax

; Set vector counts into pC->VBook [x].nVectors.

;    pC->VBook [Detail].nVectors = pC->DetailCount * 4;

	mov	eax, pC.ccDetailCount
	mov	ebx, eax
	shl	eax, 2
	mov	pC.ccVBook.vbnVectors, eax

;    pC->VBook [Smooth].nVectors = pC->DiffCount - pC->DetailCount;

	mov	eax, pC.ccDiffCount
	sub	eax, ebx

ifndef	WIN32

	mov	pC.ccVBook [type VECTORBOOK].vbnVectors, eax

; Convert DiffMap and DetailMap pointers to flat32.  Again, offset == 0.

	push	SEL pC.ccpDiffList
	call	GetSelectorBase		; get its linear base -> dx:ax
	shrd	ebx, edx, 16
	mov	bx, ax			; ebx => FLAT DiffMap

	push	SEL pC.ccpDetailList
	call	GetSelectorBase		; get its linear base -> dx:ax
	shl	edx, 16
	mov	dx, ax			; edx -> FLAT DetailMap

	mov	ds,flatSel		; access to all memory
	assume	ds:nothing

else

	mov	pC.ccVBook [size VECTORBOOK].vbnVectors, eax

;    Setup remainder of registers

	mov	ebx,pC.ccpDiffList	; -> DiffMap
	mov	edx,pC.ccpDetailList	; -> DetailMap

endif

	xor	ecx, ecx

; Within the loop:
;
; eax = work transfer register
; ebx -> DiffMap
; ecx = count of patches
; edx -> DetailMap
; esi -> pDetailIn[ecx*32] or pSmoothIn
; edi -> pDetailOut or pSmoothOut

LoopTop:
	cmp	WORD PTR [edx][ecx*2], 0
	je	CheckSmooth

; Copy 32 bytes (4 VECTORs) from DetailIn to DetailOut.

	mov	esi, ecx
	shl	esi, 5			; 4 8-byte vectors per patch
	add	esi, pDetailIn
	mov	edi, pDetailOut

	.errnz	(type VECTOR) - 8
	mov	eax, [esi]		; push ecx/mov ecx,8/rep movsd
	mov	[edi], eax
	mov	eax, [esi][4]
	.errnz	VECTOR.vClass - 7
	and	eax, 0ffffffh		; clear Class field
	mov	[edi][4], eax

	mov	eax, [esi][8]
	mov	[edi][8], eax
	mov	eax, [esi][12]
	and	eax, 0ffffffh
	mov	[edi][12], eax

	mov	eax, [esi][16]
	mov	[edi][16], eax
	mov	eax, [esi][20]
	and	eax, 0ffffffh
	mov	[edi][20], eax

	mov	eax, [esi][24]
	mov	[edi][24], eax
	mov	eax, [esi][28]
	and	eax, 0ffffffh
	mov	[edi][28], eax

	add	edi, 4*(type VECTOR)
	mov	pDetailOut, edi
	jmp	EndLoop

CheckSmooth:
	cmp	[FrameType], kKeyFrameType
	je	DoSmooth
	cmp	WORD PTR [ebx][ecx*2], 0
	je	EndLoop

; Copy 8 bytes (1 VECTOR) from SmoothIn to SmoothOut.

DoSmooth:
	mov	esi, pSmoothIn
	mov	edi, pSmoothOut

	mov	eax, [esi][ecx*8]
	mov	[edi], eax
	mov	eax, [esi][ecx*8][4]
	.errnz	VECTOR.vClass - 7
	and	eax, 0ffffffh
	mov	[edi][4], eax

	add	edi, (type VECTOR)
	mov	pSmoothOut, edi

EndLoop:
	inc	ecx
	cmp	ecx, [nPatches]
	jl	LoopTop

	ret

GenerateVectors	ENDP

	end
