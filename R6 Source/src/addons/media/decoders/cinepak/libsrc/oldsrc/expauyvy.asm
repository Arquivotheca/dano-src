; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/expauyvy.asm 1.1 1994/09/26 08:46:40 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: expauyvy.asm $
; Revision 1.1  1994/09/26 08:46:40  bog
; Initial revision

; Cloned from codec's expayuy2 22 September 1994.


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	include	cv.inc
	include	cvdecomp.inc


	.386

	SEG32


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; See expayuy2 for commentary.

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


CVDecompEntry	ExpandDetailCodeBookUYVY,far

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
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |U |Y0|V |Y1|U |Y2|V |Y3|  |  |  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

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
	mov	cl,ah				; V
	shl	ecx,16
	mov	cl,dh				; U 0 V 0

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ecx	U 0 V 0
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[26 shl (7+8)][eax][edx*8]
	sar	ebp,7

;     ecx	U 0 V 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	eax,ebp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	xor	al,al
	shl	eax,16		; Y1 in 31..24, zeros in 23..0

;     eax	0 0 0 Y1
;     ecx	U 0 V 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	ax,bp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	ax,dx

	xor	al,al
	or	eax,ecx

;     eax	U Y0 V Y1
;     ecx	U 0 V 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	eax,ebp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	xor	al,al
	shl	eax,16		; Y3 in 31..24, zeros in 23..0

;     eax	0 0 0 Y3
;     ecx	U 0 V 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	ax,bp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	ax,dx

	xor	al,al
	or	eax,ecx

;     eax	U Y2 V Y3
;     ecx	U 0 V 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][4],eax

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
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |U |Y0|V |Y1|U |Y2|V |Y3|  |  |  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

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
	mov	cl,ah				; V
	shl	ecx,16
	mov	cl,dh				; U 0 V 0

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ecx	U 0 V 0
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[26 shl (7+8)][eax][edx*8]
	sar	ebp,7

;     ecx	U 0 V 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	eax,ebp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	xor	al,al
	shl	eax,16		; Y1 in 31..24, zeros in 23..0

;     eax	0 0 0 Y1
;     ecx	U 0 V 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	ax,bp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	ax,dx

	xor	al,al
	or	eax,ecx

;     eax	U Y0 V Y1
;     ecx	U 0 V 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	eax,ebp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	xor	al,al
	shl	eax,16		; Y3 in 31..24, zeros in 23..0

;     eax	0 0 0 Y3
;     ecx	U 0 V 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	ax,bp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	ax,dx

	xor	al,al
	or	eax,ecx

;     eax	U Y2 V Y3
;     ecx	U 0 V 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][4],eax

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

;   We take the incoming Y3 Y2 Y1 Y0 in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|80|Y1|80|Y2|80|Y3|80|  |  |  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	xor	eax,eax

	mov	ah,[esi][1]	; Yc1
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	sar	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	mov	al,128
	shl	eax,16		; Y1 in 31..24, 128 in 23..16, zeros in 15..0

;     eax	0 0 128 Y1
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi]	; Yc0
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	sar	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	ax,dx

	mov	al,128		; 128 Y0 128 Y1

;     eax	128 Y0 128 Y1
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][3]	; Yc3
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	sar	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	mov	al,128
	shl	eax,16		; Y3 in 31..24, 128 in 23..16, zeros in 15..0

;     eax	0 0 128 Y3
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi][2]	; Yc2
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	sar	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	ax,dx

	mov	al,128		; 128 Y2 128 Y3

;     eax	128 Y2 128 Y3
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][4],eax

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

;   We take the incoming Y3 Y2 Y1 Y0 in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|80|Y1|80|Y2|80|Y3|80|  |  |  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	xor	eax,eax

	mov	ah,[esi][1]	; Yc1
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	sar	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	mov	al,128
	shl	eax,16		; Y1 in 31..24, 128 in 23..16, zeros in 15..0

;     eax	0 0 128 Y1
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi]	; Yc0
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	sar	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	ax,dx

	mov	al,128		; 128 Y0 128 Y1

;     eax	128 Y0 128 Y1
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][3]	; Yc3
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	sar	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	mov	al,128
	shl	eax,16		; Y3 in 31..24, 128 in 23..16, zeros in 15..0

;     eax	0 0 128 Y3
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi][2]	; Yc2
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	sar	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	ax,dx

	mov	al,128		; 128 Y2 128 Y3

;     eax	128 Y2 128 Y3
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4		; bump to next incoming YYYY

GreyDPartialYUVSkip:

	add	edi,16		; bump to next building table entry

	cmp	esi,ecx		; any more codebook entries?

	jbe	GreyDPartialYUVLoop
	jmp	DetailQuit

	endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandDetailCodeBookUYVY	endp



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


CVDecompEntry	ExpandSmoothCodeBookUYVY,far

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
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |U |Y0|V |Y0|U |Y1|V |Y1|U |Y2|V |Y2|U |Y3|V |Y3|
;   +-----------+-----------+-----------+-----------+

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
	mov	cl,dh				; U
	shl	ecx,16
	mov	cl,ah				; V 0 U 0

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ecx	V 0 U 0
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[26 shl (7+8)][eax][edx*8]
	sar	ebp,7

;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     ecx	V 0 U 0
;     esi	-> incoming codebook
;     edi	-> building codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

;     ah	Y0
;     ecx	V 0 U 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; V 0 U 0
	mov	dh,ah		; Y0
	rol	edx,16
	mov	dh,ah		; Y0

	mov	[edi],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	eax,ebp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

;     ah	Y1
;     ecx	V 0 U 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; V 0 U 0
	mov	dh,ah		; Y1
	rol	edx,16
	mov	dh,ah		; Y1

	mov	[edi][4],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

;     ah	Y2
;     ecx	V 0 U 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; V 0 U 0
	mov	dh,ah		; Y2
	rol	edx,16
	mov	dh,ah		; Y2

	mov	[edi][8],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	eax,ebp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

;     ah	Y3
;     ecx	V 0 U 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ch,ah		; Y3
	rol	ecx,16
	mov	ch,ah		; Y3

	mov	[edi][12],ecx

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
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |U |Y0|V |Y0|U |Y1|V |Y1|U |Y2|V |Y2|U |Y3|V |Y3|
;   +-----------+-----------+-----------+-----------+

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
	mov	cl,dh				; U
	shl	ecx,16
	mov	cl,ah				; V 0 U 0

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ecx	V 0 U 0
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[26 shl (7+8)][eax][edx*8]
	sar	ebp,7

;     ecx	V 0 U 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

;     ah	Y0
;     ecx	V 0 U 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; V 0 U 0
	mov	dh,ah		; Y0
	rol	edx,16
	mov	dh,ah		; Y0

	mov	[edi],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][1]	; Yc1
	lea	edx,[eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	eax,ebp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

;     ah	Y1
;     ecx	V 0 U 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; V 0 U 0
	mov	dh,ah		; Y1
	rol	edx,16
	mov	dh,ah		; Y1

	mov	[edi][4],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

;     ah	Y2
;     ecx	V 0 U 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; V 0 U 0
	mov	dh,ah		; Y2
	rol	edx,16
	mov	dh,ah		; Y2

	mov	[edi][8],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][3]	; Yc3
	lea	edx,[eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	eax,ebp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

;     ah	Y3
;     ecx	V 0 U 0
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ch,ah		; Y3
	rol	ecx,16
	mov	ch,ah		; Y3

	mov	[edi][12],ecx

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

;   We take the incoming Y3 Y2 Y1 Y0 in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|80|Y0|80|Y1|80|Y1|80|Y2|80|Y2|80|Y3|80|Y3|80|
;   +-----------+-----------+-----------+-----------+

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	sar	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y0 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx

;     eax	128 Y0 128 Y0
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][1]	; Yc1
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	sar	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y1 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx

;     eax	128 Y1 128 Y1
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	sar	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y2 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx

;     eax	128 Y2 128 Y2
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][8],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][3]	; Yc3
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	sar	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y3 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx

;     eax	128 Y3 128 Y3
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][12],eax

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

;   We take the incoming Y3 Y2 Y1 Y0 in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|80|Y0|80|Y1|80|Y1|80|Y2|80|Y2|80|Y3|80|Y3|80|
;   +-----------+-----------+-----------+-----------+

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc0<<8 << 2) + Yc0<<8
	sar	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y0 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx

;     eax	128 Y0 128 Y0
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][1]	; Yc1
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc1<<8 << 2) + Yc1<<8
	sar	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y1 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx

;     eax	128 Y1 128 Y1
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][2]	; Yc2
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc2<<8 << 2) + Yc2<<8
	sar	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y2 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx

;     eax	128 Y2 128 Y2
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][8],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi][3]	; Yc3
	lea	edx,[(-20) shl (8+5)][eax][eax*4]; (Yc3<<8 << 2) + Yc3<<8
	sar	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y3 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx

;     eax	128 Y3 128 Y3
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][12],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4		; bump to next incoming YYYYUV

GreySPartialYUVSkip:

	add	edi,16		; bump to next building table entry

	cmp	esi,ecx		; any more codebook entries?

	jbe	GreySPartialYUVLoop
	jmp	SmoothQuit

	endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandSmoothCodeBookUYVY	endp

	EndMotion

	ENDSEG32


ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	ExpandUYVY

_DATA	ends

	end
