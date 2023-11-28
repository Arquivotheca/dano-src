; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/expacpla.asm 1.5 1994/09/22 17:15:09 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: expacpla.asm $
; Revision 1.5  1994/09/22 17:15:09  bog
; Name change.  Expand420 now is ExpandCPLA.
; Fix up align and kGreyBookBit.
; Revision 1.4  1994/07/29  16:30:10  bog
; Get grey working.
; 
; Revision 1.3  1994/07/22  16:26:37  bog
; Integrate into standard codec.
; 
; Revision 1.2  1993/11/12  20:39:26  bog
; First running.
; 
; Revision 1.1  1993/11/11  23:19:07  bog
; Initial revision
; 

; Cloned from codec's expand24 1 September 1993.


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	include	cv.inc
	include	cvdecomp.inc


	.386

	SEG32


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


CVDecompEntry	ExpandDetailCodeBookCPLA,far

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

	align	4

DetailKey:

DetailKeyLoop:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi]
;   and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|Y1|Y2|Y3|Y2|Y3|Y0|Y1| U| V|  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	mov	eax,[esi]		; Y0 Y1 Y2 Y3

	mov	[edi],eax

	rol	eax,16			; Y2 Y3 Y0 Y1

	mov	[edi][4],eax

	mov	ax,[esi][4]		; U V

	mov	[edi][8],ax


	add	esi,6		; bump to next incoming YYYYUV
	add	edi,16		; bump to next RGB patch

	cmp	esi,ecx		; any more codebook entries?

	jbe	DetailKeyLoop	; jump if more to do
	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	align	4

DetailPartial:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

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
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ebx,ebx		; replace this index?

DetailPartialTestSwitch:

	jnc	DetailPartialYUVSkip

	jz	DetailPartialLoadSwitches

;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi]
;   and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|Y1|Y2|Y3|Y2|Y3|Y0|Y1| U| V|  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	mov	eax,[esi]		; Y0 Y1 Y2 Y3

	mov	[edi],eax

	rol	eax,16			; Y2 Y3 Y0 Y1

	mov	[edi][4],eax

	mov	ax,[esi][4]		; U V

	mov	[edi][8],ax


	add	esi,6		; bump to next incoming YYYYUV

DetailPartialYUVSkip:

	add	edi,16		; bump to next building table entry

	cmp	esi,ecx		; any more codebook entries?

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


;   We take the incoming Y3 Y2 Y1 Y0 in Cinepak YUV space at [esi]
;   and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|Y1|Y2|Y3|Y2|Y3|Y0|Y1|  |  |  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	eax,[esi]		; Y0 Y1 Y2 Y3

	mov	[edi],eax

	rol	eax,16			; Y2 Y3 Y0 Y1

	mov	[edi][4],eax


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

;   We take the incoming Y3 Y2 Y1 Y0 in Cinepak YUV space at [esi]
;   and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|Y1|Y2|Y3|Y2|Y3|Y0|Y1|  |  |  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	mov	eax,[esi]		; Y0 Y1 Y2 Y3

	mov	[edi],eax

	rol	eax,16			; Y2 Y3 Y0 Y1

	mov	[edi][4],eax


	add	esi,4		; bump to next incoming YYYY

GreyDPartialYUVSkip:

	add	edi,16		; bump to next building table entry

	cmp	esi,ecx		; any more codebook entries?

	jbe	GreyDPartialYUVLoop
	jmp	DetailQuit

	endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandDetailCodeBookCPLA	endp



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


CVDecompEntry	ExpandSmoothCodeBookCPLA,far

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

	align	4

SmoothKey:

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi]
;   and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|Y0|Y1|Y1|Y2|Y2|Y3|Y3| U| U| V| V|  |  |  |  |
;   +-----------+-----------+-----------+-----------+

SmoothKeyLoop:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	eax,[esi]		; Y0 Y1 Y2 Y3
	mov	edx,eax

	xchg	dl,ah			; eax: Y0 Y0 Y2 Y3, edx: Y1 Y1 Y2 Y3
	rol	edx,16			; eax: Y0 Y0 Y2 Y3, edx: Y2 Y3 Y1 Y1
	xchg	ax,dx			; eax: Y2 Y3 Y2 Y3, edx: Y0 Y0 Y1 Y1
	xchg	al,ah			; eax: Y3 Y2 Y2 Y3, edx: Y0 Y0 Y1 Y1
	ror	eax,8			; eax: Y2 Y2 Y3 Y3

	mov	[edi][4],eax
	mov	[edi],edx

	mov	ax,[esi][4]		; U V

	mov	edx,eax

	xchg	al,dh			; eax: V V x x, edx: U U x x
	rol	eax,16			; eax: x x V V
	mov	ax,dx			; eax: U U V V

	mov	[edi][8],eax


	add	esi,6		; bump to next incoming YYYYUV
	add	edi,16		; bump to next RGB patch

	cmp	esi,ecx		; any more codebook entries?

	jbe	SmoothKeyLoop	; jump if more to do
	jmp	SmoothQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	align	4

SmoothPartial:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

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
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ebx,ebx		; replace this index?

SmoothPartialTestSwitch:

	jnc	SmoothPartialYUVSkip

	jz	SmoothPartialLoadSwitches

;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi]
;   and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|Y0|Y1|Y1|Y2|Y2|Y3|Y3| U| U| V| V|  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	mov	eax,[esi]		; Y0 Y1 Y2 Y3
	mov	edx,eax

	xchg	dl,ah			; eax: Y0 Y0 Y2 Y3, edx: Y1 Y1 Y2 Y3
	rol	edx,16			; eax: Y0 Y0 Y2 Y3, edx: Y2 Y3 Y1 Y1
	xchg	ax,dx			; eax: Y2 Y3 Y2 Y3, edx: Y0 Y0 Y1 Y1
	xchg	al,ah			; eax: Y3 Y2 Y2 Y3, edx: Y0 Y0 Y1 Y1
	ror	eax,8			; eax: Y2 Y2 Y3 Y3

	mov	[edi][4],eax
	mov	[edi],edx

	mov	ax,[esi][4]		; U V

	mov	edx,eax

	xchg	al,dh			; eax: V V x x, edx: U U x x
	rol	eax,16			; eax: x x V V
	mov	ax,dx			; eax: U U V V

	mov	[edi][8],eax


	add	esi,6		; bump to next incoming YYYYUV

SmoothPartialYUVSkip:

	add	edi,16		; bump to next building table entry

	cmp	esi,ecx		; any more codebook entries?

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

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi]
;   and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|Y0|Y1|Y1|Y2|Y2|Y3|Y3| U| U| V| V|  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	mov	eax,[esi]		; Y0 Y1 Y2 Y3
	mov	edx,eax

	xchg	dl,ah			; eax: Y0 Y0 Y2 Y3, edx: Y1 Y1 Y2 Y3
	rol	edx,16			; eax: Y0 Y0 Y2 Y3, edx: Y2 Y3 Y1 Y1
	xchg	ax,dx			; eax: Y2 Y3 Y2 Y3, edx: Y0 Y0 Y1 Y1
	xchg	al,ah			; eax: Y3 Y2 Y2 Y3, edx: Y0 Y0 Y1 Y1
	ror	eax,8			; eax: Y2 Y2 Y3 Y3

	mov	[edi][4],eax
	mov	[edi],edx


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

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi]
;   and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|Y0|Y1|Y1|Y2|Y2|Y3|Y3| U| U| V| V|  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	mov	eax,[esi]		; Y0 Y1 Y2 Y3
	mov	edx,eax

	xchg	dl,ah			; eax: Y0 Y0 Y2 Y3, edx: Y1 Y1 Y2 Y3
	rol	edx,16			; eax: Y0 Y0 Y2 Y3, edx: Y2 Y3 Y1 Y1
	xchg	ax,dx			; eax: Y2 Y3 Y2 Y3, edx: Y0 Y0 Y1 Y1
	xchg	al,ah			; eax: Y3 Y2 Y2 Y3, edx: Y0 Y0 Y1 Y1
	ror	eax,8			; eax: Y2 Y2 Y3 Y3

	mov	[edi][4],eax
	mov	[edi],edx


	add	esi,4		; bump to next incoming YYYYUV

GreySPartialYUVSkip:

	add	edi,16		; bump to next building table entry

	cmp	esi,ecx		; any more codebook entries?

	jbe	GreySPartialYUVLoop
	jmp	SmoothQuit

	endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandSmoothCodeBookCPLA	endp

	EndMotion

	ENDSEG32


ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	ExpandCPLA

_DATA	ends

	end
