
; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/expacljr.asm 1.2 1994/10/05 11:07:33 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: expacljr.asm $
; Revision 1.2  1994/10/05 11:07:33  bog
; Fix black & white movies.
; Revision 1.1  1994/10/05  10:38:55  bog
; Initial revision
; 

; Cloned from expayuy2.asm 30 September 1994:
; Revision 1.4  1994/09/26  08:46:16  bog
; Seems healthy.


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	include	cv.inc
	include	cvdecomp.inc


	.386

	SEG32


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; See expayuy2 for commentary.

; CLJR is Cirrus Logic Pack Junior, or AcuPak.  It is a YUV411 format:

;  31    27 26    22 21    17 16    12 11    6 5     0
; +--------+--------+--------+--------+-------+-------+
; |   y3   |   y2   |   y1   |   y0   |   u   |   v   |
; +--------+--------+--------+--------+-------+-------+

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	BeginMotion


;--------------------------------------------------------------------------
;
; void (*pExpandDetailCodeBook)(
;   unsigned long,		// flat -> input codebook
;   unsigned long 		// flat -> where to put translate table
; );
;
;--------------------------------------------------------------------------

ifndef	WIN32
	assume	ds:DGROUP
	assume	es:nothing
	assume	fs:nothing
	assume	gs:nothing
endif

OldEbp		equ	(dword ptr ss:[ebp])
tOldEbp		=	type OldEbp
RetAddr		equ	(dword ptr OldEbp[tOldEbp])
tRetAddr	=	type RetAddr

pCBIn		equ	(dword ptr RetAddr[tRetAddr])
tpCBIn		=	type pCBIn
pCB		equ	(dword ptr pCBIn[tpCBIn])
tpCB		=	type pCB


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


CVDecompEntry	ExpandDetailCodeBookCLJR,far

	mov	esi,pCBIn		; -> incoming codebook

	mov	ecx,[esi]		; get type & size

	mov	bl,cl			; remember type

	shr	ecx,16
	xchg	cl,ch			; ecx:  size of incoming chunk

	add	ecx,esi			; ecx:  first byte beyond chunk

	add	esi,4			; esi:  first codebook entry

	sub	ecx,6			; ecx:  last possible valid entry

;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook

	cmp	esi,ecx			; empty codebook?
	ja	short DetailQuit

	mov	edi,pCB

;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	ifndef	NOBLACKWHITE

	test	bl,kGreyBookBit	; grey scale codebook?
	jnz	DoGreyDetail

	endif

	cmp	bl,kFullDBookType
	je	short DetailKey

	cmp	bl,kPartialDBookType
	je	DetailPartial

DetailHuh:

; not recognized so we just return

  ifdef	DEBUG
	int	3
  endif

DetailQuit:

	CVDecompExit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DetailKey:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	DetailKeyDoneCompare[-4],ecx
	Ref	Motion

	align	4

DetailKeyLoop:

;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV411 as CLJR dwords and remember it in the
;   table:

;       31 27 26 22 21 17 16 12 11   6 5    0
;      +-----+-----+-----+-----+------+------+
;   0: |   0 |   0 |  y1 |  y0 |  u/2 |  v/2 |
;      +-----+-----+-----+-----+------+------+
;   4: |  y1 |  y0 |   0 |   0 |  u/2 |  v/2 |
;      +-----+-----+-----+-----+------+------+
;   8: |   0 |   0 |  y3 |  y2 |  u/2 |  v/2 |
;      +-----+-----+-----+-----+------+------+
;  12: |  y3 |  y2 |   0 |   0 |  u/2 |  v/2 |
;      +-----+-----+-----+-----+------+------+

	movsx	eax,byte ptr[esi][4]		; Uc
	lea	edx,[eax][eax*2]		; Uc*3
	shl	eax,8				; Uc*256
	lea	edx,[000008094h][eax][edx*2]	; Uc*256 + Uc*6 + 128*256

;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	movsx	eax,byte ptr[esi][5]		; Vc
	shl	eax,5+8
	add	eax,edx				; Vc<<8 << 5 + U<<8
	lea	eax,[03da4e4h][eax][eax*4]	; 5 * (Vc<<8 << 5 + U<<8)
	sar	eax,7

	xor	ecx,ecx
	mov	cl,ah			; V
	shr	cl,1			; ........ ........ ........ .vvvvvvv
	mov	ch,dh			; ........ ........ uuuuuuuu .vvvvvvv
	shr	ch,3			; ........ ........ ...uuuuu .vvvvvvv
	shr	ecx,2			; ........ ........ .....uuu uu.vvvvv

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ecx	........ ........ .....uuu uu.vvvvv
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[26 shl (7+8)][eax][edx*8]
	sar	ebp,7

;     ecx	........ ........ .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 00000000 ........ ........ ........
	and	eax,0f8000000h	; 00000... ........ ........ ........

;     eax	00000... ........ ........ ........
;     ecx	........ ........ .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	ax,bp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	ax,dx

	shr	ax,11		; 00000... ........ ........ ...11111
	mov	edx,eax
	rol	eax,17		; ........ ..111110 0000.... ........
	rol	edx,27		; 11111000 00...... ........ ........
	or	eax,ecx		; ........ ..111110 0000.uuu uu.vvvvv
	or	edx,ecx		; 11111000 00...... .....uuu uu.vvvvv

;     eax	........ ..111110 0000.uuu uu.vvvvv
;     ecx	........ ........ .....uuu uu.vvvvv
;     edx	11111000 00...... .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi],eax	; ........ ..111110 0000.uuu uu.vvvvv
	mov	[edi][4],edx	; 11111000 00...... .....uuu uu.vvvvv

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 22222222 ........ ........ ........
	and	eax,0f8000000h	; 22222... ........ ........ ........

;     eax	22222... ........ ........ ........
;     ecx	........ ........ .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	ax,bp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	ax,dx

	shr	ax,11		; 22222... ........ ........ ...33333
	mov	edx,eax
	rol	eax,17		; ........ ..333332 2222.... ........
	rol	edx,27		; 33333222 22...... ........ ........
	or	eax,ecx		; ........ ..333332 2222.uuu uu.vvvvv
	or	edx,ecx		; 33333222 22...... .....uuu uu.vvvvv

;     eax	........ ..333332 2222.uuu uu.vvvvv
;     ecx	........ ........ .....uuu uu.vvvvv
;     edx	33333222 22...... .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi][8],eax	; ........ ..333332 2222.uuu uu.vvvvv
	mov	[edi][12],edx	; 33333222 22...... .....uuu uu.vvvvv

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV
	add	edi,16		; bump to next RGB patch

	cmp	esi,012345678h	; any more codebook entries?
DetailKeyDoneCompare	label	dword

	jbe	DetailKeyLoop	; jump if more to do
	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DetailPartial:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	DetailPartialDoneCompare[-4],ecx
	Ref	Motion

	align	4

DetailPartialLoadSwitches:

  ; We use ebx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	ebx,[esi]	; get next set of 32 switches
	add	esi,4		; bump source

	xchg	bl,bh		; swiz
	rol	ebx,16
	xchg	bl,bh

	stc
	adc	ebx,ebx

  ;  carry, ebx: s ssss ssss ssss ssss ssss ssss ssss sss1

	jmp	short DetailPartialTestSwitch

	align	4


DetailPartialYUVLoop:

;     ebx	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ebx,ebx		; replace this index?

DetailPartialTestSwitch:

	jnc	DetailPartialYUVSkip

	jz	DetailPartialLoadSwitches

;     ebx	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV411 as CLJR dwords and remember it in the
;   table:

;       31 27 26 22 21 17 16 12 11   6 5    0
;      +-----+-----+-----+-----+------+------+
;   0: |   0 |   0 |  y1 |  y0 |  u/2 |  v/2 |
;      +-----+-----+-----+-----+------+------+
;   4: |  y1 |  y0 |   0 |   0 |  u/2 |  v/2 |
;      +-----+-----+-----+-----+------+------+
;   8: |   0 |   0 |  y3 |  y2 |  u/2 |  v/2 |
;      +-----+-----+-----+-----+------+------+
;  12: |  y3 |  y2 |   0 |   0 |  u/2 |  v/2 |
;      +-----+-----+-----+-----+------+------+

	movsx	eax,byte ptr[esi][4]		; Uc
	lea	edx,[eax][eax*2]		; Uc*3
	shl	eax,8				; Uc*256
	lea	edx,[000008094h][eax][edx*2]	; Uc*256 + Uc*6 + 128*256

;     ebx	swizzled bit switches for codes we do
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	movsx	eax,byte ptr[esi][5]		; Vc
	shl	eax,5+8
	add	eax,edx				; Vc<<8 << 5 + U<<8
	lea	eax,[03da4e4h][eax][eax*4]	; 5 * (Vc<<8 << 5 + U<<8)
	sar	eax,7

	xor	ecx,ecx
	mov	cl,ah			; V
	shr	cl,1			; ........ ........ ........ .vvvvvvv
	mov	ch,dh			; ........ ........ uuuuuuuu .vvvvvvv
	shr	ch,3			; ........ ........ ...uuuuu .vvvvvvv
	shr	ecx,2			; ........ ........ .....uuu uu.vvvvv

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ .....uuu uu.vvvvv
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[26 shl (7+8)][eax][edx*8]
	sar	ebp,7

;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 00000000 ........ ........ ........
	and	eax,0f8000000h	; 00000... ........ ........ ........

;     eax	00000... ........ ........ ........
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	ax,bp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	ax,dx

	shr	ax,11		; 00000... ........ ........ ...11111
	mov	edx,eax
	rol	eax,17		; ........ ..111110 0000.... ........
	rol	edx,27		; 11111000 00...... ........ ........
	or	eax,ecx		; ........ ..111110 0000.uuu uu.vvvvv
	or	edx,ecx		; 11111000 00...... .....uuu uu.vvvvv

;     eax	........ ..111110 0000.uuu uu.vvvvv
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ .....uuu uu.vvvvv
;     edx	11111000 00...... .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi],eax	; ........ ..111110 0000.uuu uu.vvvvv
	mov	[edi][4],edx	; 11111000 00...... .....uuu uu.vvvvv

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 22222222 ........ ........ ........
	and	eax,0f8000000h	; 22222... ........ ........ ........

;     eax	22222... ........ ........ ........
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	ax,bp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	ax,dx

	shr	ax,11		; 22222... ........ ........ ...33333
	mov	edx,eax
	rol	eax,17		; ........ ..333332 2222.... ........
	rol	edx,27		; 33333222 22...... ........ ........
	or	eax,ecx		; ........ ..333332 2222.uuu uu.vvvvv
	or	edx,ecx		; 33333222 22...... .....uuu uu.vvvvv

;     eax	........ ..333332 2222.uuu uu.vvvvv
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ .....uuu uu.vvvvv
;     edx	33333222 22...... .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi][8],eax	; ........ ..333332 2222.uuu uu.vvvvv
	mov	[edi][12],edx	; 33333222 22...... .....uuu uu.vvvvv

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV

DetailPartialYUVSkip:

	add	edi,16		; bump to next building table entry

	cmp	esi,012345678h	; any more codebook entries?
DetailPartialDoneCompare	label	dword

	jbe	DetailPartialYUVLoop
	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	ifndef	NOBLACKWHITE

DoGreyDetail:

;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ecx,2		; last possible code

	cmp	bl,kFullDBookType + kGreyBookBit
	jne	GreyDPartial

	align	4

GreyDKeyLoop:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV411 as CLJR dwords and remember it in the
;   table:

;       31 27 26 22 21 17 16 12 11   6 5    0
;      +-----+-----+-----+-----+------+------+
;   0: |   0 |   0 |  y1 |  y0 |   10 |   10 |
;      +-----+-----+-----+-----+------+------+
;   4: |  y1 |  y0 |   0 |   0 |   10 |   10 |
;      +-----+-----+-----+-----+------+------+
;   8: |   0 |   0 |  y3 |  y2 |   10 |   10 |
;      +-----+-----+-----+-----+------+------+
;  12: |  y3 |  y2 |   0 |   0 |   10 |   10 |
;      +-----+-----+-----+-----+------+------+

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 00000000 ........ ........ ........
	and	eax,0f8000000h	; 00000... ........ ........ ........

;     eax	00000... ........ ........ ........
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	ax,bp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	ax,dx

	shr	ax,11		; 00000... ........ ........ ...11111
	mov	edx,eax
	rol	eax,17		; ........ ..111110 0000.... ........
	rol	edx,27		; 11111000 00...... ........ ........
	or	eax,000000410h	; ........ ..111110 0000.uuu uu.vvvvv
	or	edx,000000410h	; 11111000 00...... .....uuu uu.vvvvv

;     eax	........ ..111110 0000.uuu uu.vvvvv
;     ecx	-> last possible valid codebook entry
;     edx	11111000 00...... .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi],eax	; ........ ..111110 0000.uuu uu.vvvvv
	mov	[edi][4],edx	; 11111000 00...... .....uuu uu.vvvvv

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 22222222 ........ ........ ........
	and	eax,0f8000000h	; 22222... ........ ........ ........

;     eax	22222... ........ ........ ........
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	ax,bp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	ax,dx

	shr	ax,11		; 22222... ........ ........ ...33333
	mov	edx,eax
	rol	eax,17		; ........ ..333332 2222.... ........
	rol	edx,27		; 33333222 22...... ........ ........
	or	eax,000000410h	; ........ ..333332 2222.uuu uu.vvvvv
	or	edx,000000410h	; 33333222 22...... .....uuu uu.vvvvv

;     eax	........ ..333332 2222.uuu uu.vvvvv
;     ecx	-> last possible valid codebook entry
;     edx	33333222 22...... .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi][8],eax	; ........ ..333332 2222.uuu uu.vvvvv
	mov	[edi][12],edx	; 33333222 22...... .....uuu uu.vvvvv

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4		; bump to next incoming YYYY
	add	edi,16		; bump to next code patch

	cmp	esi,ecx		; any more codebook entries?

	jbe	GreyDKeyLoop	; jump if more to do
	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreyDPartial:

;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	cmp	bl,kPartialDBookType + kGreyBookBit
	jne	DetailHuh

	align	4

GreyDPartialLoadSwitches:

  ; We use ebx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	ebx,[esi]	; get next set of 32 switches
	add	esi,4		; bump source

	xchg	bl,bh		; swiz
	rol	ebx,16
	xchg	bl,bh

	stc
	adc	ebx,ebx

  ;  carry, ebx: s ssss ssss ssss ssss ssss ssss ssss sss1

	jmp	short GreyDPartialTestSwitch

	align	4


GreyDPartialYUVLoop:

;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ebx,ebx		; replace this index?

GreyDPartialTestSwitch:

	jnc	GreyDPartialYUVSkip

	jz	GreyDPartialLoadSwitches

;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV411 as CLJR dwords and remember it in the
;   table:

;       31 27 26 22 21 17 16 12 11   6 5    0
;      +-----+-----+-----+-----+------+------+
;   0: |   0 |   0 |  y1 |  y0 |   10 |   10 |
;      +-----+-----+-----+-----+------+------+
;   4: |  y1 |  y0 |   0 |   0 |   10 |   10 |
;      +-----+-----+-----+-----+------+------+
;   8: |   0 |   0 |  y3 |  y2 |   10 |   10 |
;      +-----+-----+-----+-----+------+------+
;  12: |  y3 |  y2 |   0 |   0 |   10 |   10 |
;      +-----+-----+-----+-----+------+------+

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 00000000 ........ ........ ........
	and	eax,0f8000000h	; 00000... ........ ........ ........

;     eax	00000... ........ ........ ........
;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	ax,bp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	ax,dx

	shr	ax,11		; 00000... ........ ........ ...11111
	mov	edx,eax
	rol	eax,17		; ........ ..111110 0000.... ........
	rol	edx,27		; 11111000 00...... ........ ........
	or	eax,000000410h	; ........ ..111110 0000.uuu uu.vvvvv
	or	edx,000000410h	; 11111000 00...... .....uuu uu.vvvvv

;     eax	........ ..111110 0000.uuu uu.vvvvv
;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     edx	11111000 00...... .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi],eax	; ........ ..111110 0000.uuu uu.vvvvv
	mov	[edi][4],edx	; 11111000 00...... .....uuu uu.vvvvv

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 22222222 ........ ........ ........
	and	eax,0f8000000h	; 22222... ........ ........ ........

;     eax	22222... ........ ........ ........
;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	ax,bp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	ax,dx

	shr	ax,11		; 22222... ........ ........ ...33333
	mov	edx,eax
	rol	eax,17		; ........ ..333332 2222.... ........
	rol	edx,27		; 33333222 22...... ........ ........
	or	eax,000000410h	; ........ ..333332 2222.uuu uu.vvvvv
	or	edx,000000410h	; 33333222 22...... .....uuu uu.vvvvv

;     eax	........ ..333332 2222.uuu uu.vvvvv
;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     edx	33333222 22...... .....uuu uu.vvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi][8],eax	; ........ ..333332 2222.uuu uu.vvvvv
	mov	[edi][12],edx	; 33333222 22...... .....uuu uu.vvvvv

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4		; bump to next incoming YYYY

GreyDPartialYUVSkip:

	add	edi,16		; bump to next building table entry

	cmp	esi,ecx		; any more codebook entries?

	jbe	GreyDPartialYUVLoop
	jmp	DetailQuit

	endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandDetailCodeBookCLJR	endp



;--------------------------------------------------------------------------
;
; void (*pExpandSmoothCodeBook)(
;   unsigned long,		// flat -> input codebook
;   unsigned long 		// flat -> where to put translate table
; );
;
;--------------------------------------------------------------------------

ifndef	WIN32
	assume	ds:DGROUP
	assume	es:nothing
	assume	fs:nothing
	assume	gs:nothing
endif

OldEbp		equ	(dword ptr ss:[ebp])
tOldEbp		=	type OldEbp
RetAddr		equ	(dword ptr OldEbp[tOldEbp])
tRetAddr	=	type RetAddr

pCBIn		equ	(dword ptr RetAddr[tRetAddr])
tpCBIn		=	type pCBIn
pCB		equ	(dword ptr pCBIn[tpCBIn])
tpCB		=	type pCB


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


CVDecompEntry	ExpandSmoothCodeBookCLJR,far

	mov	esi,pCBIn		; -> incoming codebook

	mov	ecx,[esi]		; get type & size

	mov	bl,cl			; remember type

	shr	ecx,16
	xchg	cl,ch			; ecx:  size of incoming chunk

	add	ecx,esi			; ecx:  first byte beyond chunk

	add	esi,4			; esi:  first codebook entry

	sub	ecx,6			; ecx:  last possible valid entry

;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook

	cmp	esi,ecx			; empty codebook?
	ja	short SmoothQuit

	mov	edi,pCB

;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	ifndef	NOBLACKWHITE

	test	bl,kGreyBookBit	; grey scale codebook?
	jnz	DoGreySmooth

	endif

	cmp	bl,kFullSBookType
	je	short SmoothKey

	cmp	bl,kPartialSBookType
	je	SmoothPartial

SmoothHuh:

; not recognized so we just return

  ifdef	DEBUG
	int	3
  endif

SmoothQuit:

	CVDecompExit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SmoothKey:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	SmoothKeyDoneCompare[-4],ecx
	Ref	Motion

	align	4

SmoothKeyLoop:

;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV411 as CLJR dwords and remember it in the
;   table:

;       31 27 26 22 21 17 16 12 11   6 5    0
;      +-----+-----+-----+-----+------+------+
;   0: |  y1 |  y1 |  y0 |  y0 |   u  |   v  |
;      +-----+-----+-----+-----+------+------+
;   4: |  y3 |  y3 |  y2 |  y2 |   u  |   v  |
;      +-----+-----+-----+-----+------+------+

	movsx	eax,byte ptr[esi][4]		; Uc
	lea	edx,[eax][eax*2]		; Uc*3
	shl	eax,8				; Uc*256
	lea	edx,[000008094h][eax][edx*2]	; Uc*256 + Uc*6 + 128*256

;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	movsx	eax,byte ptr[esi][5]		; Vc
	shl	eax,5+8
	add	eax,edx				; Vc<<8 << 5 + U<<8
	lea	eax,[03da4e4h][eax][eax*4]	; 5 * (Vc<<8 << 5 + U<<8)
	sar	eax,7

	xor	ecx,ecx
	mov	cl,ah			; V
	mov	ch,dh			; ........ ........ uuuuuuuu vvvvvvvv
	shr	ch,2			; ........ ........ ..uuuuuu vvvvvvvv
	shr	ecx,2			; ........ ........ ....uuuu uuvvvvvv

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ecx	........ ........ ....uuuu uuvvvvvv
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[26 shl (7+8)][eax][edx*8]
	sar	ebp,7

;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; yyyyyyyy ........ ........ ........
	and	eax,0f8000000h	; yyyyy... ........ ........ ........

;     eax	YYYYY... ........ ........ ........
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	ax,bp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	ax,dx		; Y1

	shr	ax,6		; YYYYY... ........ ......yy yyy?????
	and	al,0e0h		; YYYYY... ........ ......yy yyy.....

	rol	eax,17		; .....yyy yy.....Y YYYY.... ........
	lea	edx,[eax*8]	; ..yyyyy. ....YYYY Y....... ........
	lea	eax,[eax][edx*4]; yyyyyyyy yyYYYYYY YYYY.... ........
	or	eax,ecx		; yyyyyyyy yyYYYYYY YYYYuuuu uuvvvvvv

;     eax	yyyyyyyy yyYYYYYY YYYYuuuu uuvvvvvv
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; yyyyyyyy ........ ........ ........
	and	eax,0f8000000h	; yyyyy... ........ ........ ........

;     eax	YYYYY... ........ ........ ........
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	ax,bp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	ax,dx		; Y3

	shr	ax,6		; YYYYY... ........ ......yy yyy?????
	and	al,0e0h		; YYYYY... ........ ......yy yyy.....

	rol	eax,17		; .....yyy yy.....Y YYYY.... ........
	lea	edx,[eax*8]	; ..yyyyy. ....YYYY Y....... ........
	lea	eax,[eax][edx*4]; yyyyyyyy yyYYYYYY YYYY.... ........
	or	eax,ecx		; yyyyyyyy yyYYYYYY YYYYuuuu uuvvvvvv

;     eax	yyyyyyyy yyYYYYYY YYYYuuuu uuvvvvvv
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi][4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV
	add	edi,16		; bump to next RGB patch

	cmp	esi,012345678h	; any more codebook entries?
SmoothKeyDoneCompare	label	dword

	jbe	SmoothKeyLoop	; jump if more to do
	jmp	SmoothQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SmoothPartial:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	SmoothPartialDoneCompare[-4],ecx
	Ref	Motion

	align	4

SmoothPartialLoadSwitches:

  ; We use ebx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	ebx,[esi]	; get next set of 32 switches
	add	esi,4		; bump source

	xchg	bl,bh		; swiz
	rol	ebx,16
	xchg	bl,bh

	stc
	adc	ebx,ebx

  ;  carry, ebx: s ssss ssss ssss ssss ssss ssss ssss sss1

	jmp	short SmoothPartialTestSwitch

	align	4


SmoothPartialYUVLoop:

;     ebx	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ebx,ebx		; replace this index?

SmoothPartialTestSwitch:

	jnc	SmoothPartialYUVSkip

	jz	SmoothPartialLoadSwitches

;     ebx	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV411 as CLJR dwords and remember it in the
;   table:

;       31 27 26 22 21 17 16 12 11   6 5    0
;      +-----+-----+-----+-----+------+------+
;   0: |  y1 |  y1 |  y0 |  y0 |   u  |   v  |
;      +-----+-----+-----+-----+------+------+
;   4: |  y3 |  y3 |  y2 |  y2 |   u  |   v  |
;      +-----+-----+-----+-----+------+------+

	movsx	eax,byte ptr[esi][4]		; Uc
	lea	edx,[eax][eax*2]		; Uc*3
	shl	eax,8				; Uc*256
	lea	edx,[000008094h][eax][edx*2]	; Uc*256 + Uc*6 + 128*256

;     ebx	swizzled bit switches for codes we do
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	movsx	eax,byte ptr[esi][5]		; Vc
	shl	eax,5+8
	add	eax,edx				; Vc<<8 << 5 + U<<8
	lea	eax,[03da4e4h][eax][eax*4]	; 5 * (Vc<<8 << 5 + U<<8)
	sar	eax,7

	xor	ecx,ecx
	mov	cl,ah			; V
	mov	ch,dh			; ........ ........ uuuuuuuu vvvvvvvv
	shr	ch,2			; ........ ........ ..uuuuuu vvvvvvvv
	shr	ecx,2			; ........ ........ ....uuuu uuvvvvvv

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ ....uuuu uuvvvvvv
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[26 shl (7+8)][eax][edx*8]
	sar	ebp,7

;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; yyyyyyyy ........ ........ ........
	and	eax,0f8000000h	; yyyyy... ........ ........ ........

;     eax	YYYYY... ........ ........ ........
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	ax,bp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	ax,dx		; Y1

	shr	ax,6		; YYYYY... ........ ......yy yyy?????
	and	al,0e0h		; YYYYY... ........ ......yy yyy.....

	rol	eax,17		; .....yyy yy.....Y YYYY.... ........
	lea	edx,[eax*8]	; ..yyyyy. ....YYYY Y....... ........
	lea	eax,[eax][edx*4]; yyyyyyyy yyYYYYYY YYYY.... ........
	or	eax,ecx		; yyyyyyyy yyYYYYYY YYYYuuuu uuvvvvvv

;     eax	yyyyyyyy yyYYYYYY YYYYuuuu uuvvvvvv
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; yyyyyyyy ........ ........ ........
	and	eax,0f8000000h	; yyyyy... ........ ........ ........

;     eax	YYYYY... ........ ........ ........
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	ax,bp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	ax,dx		; Y3

	shr	ax,6		; YYYYY... ........ ......yy yyy?????
	and	al,0e0h		; YYYYY... ........ ......yy yyy.....

	rol	eax,17		; .....yyy yy.....Y YYYY.... ........
	lea	edx,[eax*8]	; ..yyyyy. ....YYYY Y....... ........
	lea	eax,[eax][edx*4]; yyyyyyyy yyYYYYYY YYYY.... ........
	or	eax,ecx		; yyyyyyyy yyYYYYYY YYYYuuuu uuvvvvvv

;     eax	yyyyyyyy yyYYYYYY YYYYuuuu uuvvvvvv
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi][4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV

SmoothPartialYUVSkip:

	add	edi,16		; bump to next building table entry

	cmp	esi,012345678h	; any more codebook entries?
SmoothPartialDoneCompare	label	dword

	jbe	SmoothPartialYUVLoop
	jmp	SmoothQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	ifndef	NOBLACKWHITE

DoGreySmooth:

;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ecx,2		; last possible codebook entry

	cmp	bl,kFullSBookType + kGreyBookBit
	jne	GreySPartial

	align	4

GreySKeyLoop:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV411 as CLJR dwords and remember it in the
;   table:

;       31 27 26 22 21 17 16 12 11   6 5    0
;      +-----+-----+-----+-----+------+------+
;   0: |  y1 |  y1 |  y0 |  y0 |  20  |  20  |
;      +-----+-----+-----+-----+------+------+
;   4: |  y3 |  y3 |  y2 |  y2 |  20  |  20  |
;      +-----+-----+-----+-----+------+------+

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 00000000 ........ ........ ........
	and	eax,0f8000000h	; 00000... ........ ........ ........

;     eax	YYYYY... ........ ........ ........
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	ax,bp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	ax,dx		; Y1

	shr	ax,6		; 00000... ........ ......11 111?????
	and	al,0e0h		; 00000... ........ ......11 111.....

	rol	eax,17		; .....111 11.....0 0000.... ........
	lea	edx,[eax*8]	; ..11111. ....0000 0....... ........
	lea	eax,[00820h][eax][edx*4]

;     eax	11111111 11000000 0000uuuu uuvvvvvv
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 22222222 ........ ........ ........
	and	eax,0f8000000h	; 22222... ........ ........ ........

;     eax	22222... ........ ........ ........
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	ax,bp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	ax,dx		; Y3

	shr	ax,6		; 22222... ........ ......33 333?????
	and	al,0e0h		; 22222... ........ ......33 333.....

	rol	eax,17		; .....333 33.....2 2222.... ........
	lea	edx,[eax*8]	; ..33333. ....2222 2....... ........
	lea	eax,[00820h][eax][edx*4]

;     eax	33333333 33222222 2222uuuu uuvvvvvv
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi][4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4		; bump to next incoming YYYYUV
	add	edi,16		; bump to next RGB patch

	cmp	esi,ecx		; any more codebook entries?

	jbe	GreySKeyLoop	; jump if more to do
	jmp	SmoothQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreySPartial:

	cmp	bl,kPartialSBookType + kGreyBookBit
	jne	SmoothHuh

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	align	4

GreySPartialLoadSwitches:

  ; We use ebx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	ebx,[esi]	; get next set of 32 switches
	add	esi,4		; bump source

	xchg	bl,bh		; swiz
	rol	ebx,16
	xchg	bl,bh

	stc
	adc	ebx,ebx

  ;  carry, ebx: s ssss ssss ssss ssss ssss ssss ssss sss1

	jmp	short GreySPartialTestSwitch

	align	4


GreySPartialYUVLoop:

;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ebx,ebx		; replace this index?

GreySPartialTestSwitch:

	jnc	GreySPartialYUVSkip

	jz	GreySPartialLoadSwitches

;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV411 as CLJR dwords and remember it in the
;   table:

;       31 27 26 22 21 17 16 12 11   6 5    0
;      +-----+-----+-----+-----+------+------+
;   0: |  y1 |  y1 |  y0 |  y0 |  20  |  20  |
;      +-----+-----+-----+-----+------+------+
;   4: |  y3 |  y3 |  y2 |  y2 |  20  |  20  |
;      +-----+-----+-----+-----+------+------+

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 00000000 ........ ........ ........
	and	eax,0f8000000h	; 00000... ........ ........ ........

;     eax	YYYYY... ........ ........ ........
;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	ax,bp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	ax,dx		; Y1

	shr	ax,6		; 00000... ........ ......11 111?????
	and	al,0e0h		; 00000... ........ ......11 111.....

	rol	eax,17		; .....111 11.....0 0000.... ........
	lea	edx,[eax*8]	; ..11111. ....0000 0....... ........
	lea	eax,[00820h][eax][edx*4]

;     eax	11111111 11000000 0000uuuu uuvvvvvv
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	shl	eax,16		; 22222222 ........ ........ ........
	and	eax,0f8000000h	; 22222... ........ ........ ........

;     eax	22222... ........ ........ ........
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	ax,bp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	ax,dx		; Y3

	shr	ax,6		; 22222... ........ ......33 333?????
	and	al,0e0h		; 22222... ........ ......33 333.....

	rol	eax,17		; .....333 33.....2 2222.... ........
	lea	edx,[eax*8]	; ..33333. ....2222 2....... ........
	lea	eax,[00820h][eax][edx*4]

;     eax	33333333 33222222 2222uuuu uuvvvvvv
;     ebx	swizzled bit switches for codes we do
;     ecx	........ ........ ....uuuu uuvvvvvv
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8

	mov	[edi][4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4		; bump to next incoming YYYYUV

GreySPartialYUVSkip:

	add	edi,16		; bump to next building table entry

	cmp	esi,ecx		; any more codebook entries?

	jbe	GreySPartialYUVLoop
	jmp	SmoothQuit

	endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandSmoothCodeBookCLJR	endp

	EndMotion

	ENDSEG32


ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	ExpandCLJR

_DATA	ends

	end
