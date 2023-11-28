; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/expand32.asm 2.9 1994/09/22 17:17:09 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: expand32.asm $
; Revision 2.9  1994/09/22 17:17:09  bog
; Fix up align and refs to kGreyBookBit.
; Revision 2.8  1994/06/23  14:13:10  bog
; Change movzx into xor/mov pair.
; 
; Revision 2.7  1994/05/06  13:41:31  bog
; Play back black and white movies.
; 
; Revision 2.6  1993/08/10  11:04:22  bog
; Speed up a little.
; 
; Revision 2.5  1993/07/03  11:44:09  geoffs
; All _DATA segment declarations needed USE16 if WIN16 build
; 
; Revision 2.4  93/07/02  16:19:19  geoffs
; Now compiles,runs under Windows NT
; 
; Revision 2.3  93/06/16  14:47:51  timr
; Don't write into byte 11 of codebook entries.
; 
; Revision 2.2  93/06/15  15:54:39  timr
; (bog)  Combining Detail and Smooth expand means we have to change the test.
; 
; Revision 2.1  93/06/13  11:27:23  bog
; Roll to 2.1.
; 
; Revision 1.1  93/06/09  17:59:02  bog
; Initial revision
; 

; cloned from expand24.asm 9 June 1993

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	include	cv.inc
	include	cvdecomp.inc


	.386

	SEG32

	extrn	Bounding24	:byte
	extrn	GreyDwordLookup	:dword


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	BeginMotion


;--------------------------------------------------------------------------
;
; void (*pExpandCodeBook)(
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


CVDecompEntry	ExpandCodeBook32,far

	mov	esi,pCBIn	; -> incoming codebook

	mov	eax,[esi]	; get type & size

	mov	bl,al		; remember type

	shr	eax,16
	xchg	al,ah		; eax:  size of incoming chunk

	add	eax,esi		; eax:  first byte beyond chunk

	add	esi,4		; esi:  first codebook entry

	sub	eax,6		; eax:  last possible valid entry

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook

	cmp	esi,eax		; empty codebook?
	ja	short Quit

	mov	edi,pCB

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	and	bl,not 2	; turn off detail/smooth bit

	ifndef	NOBLACKWHITE

	test	bl,kGreyBookBit	; grey scale codebook?
	jnz	DoGrey

	endif

	cmp	bl,kFullDBookType and not 2
	je	short Key

	cmp	bl,kPartialDBookType and not 2
	je	Partial

Huh:

; not recognized so we just return

  ifdef	DEBUG
	int	3
  endif

Quit:

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Key:

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	KeyDoneCompare[-4],eax
	Ref	Motion

	align	4

KeyLoop:

  ; we take the incoming YYYYUV at [esi] and develop a 2x2 patch:
  ;
  ;     RGB0  RGB1
  ;     RGB2  RGB3
  ;
  ; as seen on the screen.
  ;
  ; The YUV is scaled:
  ;   Ri = Yi + V*2
  ;   Gi = Yi - (V+U/2)
  ;   Bi = Yi + U*2
  ;
  ; We put V*2 in ebx, -(V+U/2) in ecx, and U*2 in ebp

	movsx	ecx,byte ptr[esi][4]	; ecx:  U
	movsx	ebx,byte ptr[esi][5]	; ebx:  V
	mov	ebp,ecx			; ebp:  U
	sar	ecx,1			; ecx:  U/2
	shl	ebp,1			; ebp:  U*2
	add	ecx,ebx			; ecx:  V+U/2
	shl	ebx,1			; ebx:  V*2
	neg	ecx			; ecx:  -(V+U/2)

;     eax	develop unscaled RGB components to index into Bounding24
;     ebx	V*2
;     ecx	-(V+U/2)
;     edx	developing scaled RGB
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	U*2

	xor	eax,eax
	mov	al,byte ptr[esi]	; eax:  Y0

	xor	edx,edx
	mov	dl,Bounding24[ebx][eax]	; edx:  000000R0
	Ref	TEXT32

	shl	edx,16

	mov	dh,Bounding24[ecx][eax]	; edx:  00R0G000
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  00R0G0B0
	Ref	TEXT32

	mov	[edi],edx		; save RGB0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][1]	; eax:  Y1

	xor	edx,edx
	mov	dl,Bounding24[ebx][eax]	; edx:  000000R1
	Ref	TEXT32

	shl	edx,16

	mov	dh,Bounding24[ecx][eax]	; edx:  00R1G100
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  00R1G1B1
	Ref	TEXT32

	mov	[edi][4],edx		; save RGB1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][2]	; eax:  Y2

	xor	edx,edx
	mov	dl,Bounding24[ebx][eax]	; edx:  000000R2
	Ref	TEXT32

	shl	edx,16

	mov	dh,Bounding24[ecx][eax]	; edx:  00R2G200
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  00R2G2B2
	Ref	TEXT32

	mov	[edi][8],edx		; save RGB2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][3]	; eax:  Y3

	xor	edx,edx
	mov	dl,Bounding24[ebx][eax]	; edx:  000000R3
	Ref	TEXT32

	shl	edx,16

	mov	dh,Bounding24[ecx][eax]	; edx:  00R3G300
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  00R3G3B3
	Ref	TEXT32

	mov	[edi][12],edx		; save it

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV
	add	edi,16		; bump to next RGB patch

	cmp	esi,012345678h	; any more codebook entries?
KeyDoneCompare	label	dword

	jbe	KeyLoop	; jump if more to do
	jmp	Quit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Partial:

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	PartialDoneCompare[-4],eax
	Ref	Motion

	align	4

PartialLoadSwitches:

  ; We use ebx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	eax,[esi]	; get next set of 32 switches
	add	esi,4		; bump source

	xchg	al,ah		; swiz
	rol	eax,16
	xchg	al,ah

	stc
	adc	eax,eax

  ;  carry, eax: s ssss ssss ssss ssss ssss ssss ssss sss1

	jmp	short PartialTestSwitch


	align	4

PartialYUVLoop:

	mov	eax,012345678h	; self mod switches here
PartialSwitches	label	dword

;     eax	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,eax		; replace this index?

PartialTestSwitch:

	mov	PartialSwitches[-4],eax; save for next iteration
	Ref	Motion

	jnc	PartialYUVSkip

	jz	PartialLoadSwitches

  ; we take the incoming YYYYUV at [esi] and develop a 2x2 patch:
  ;
  ;     RGB0  RGB1
  ;     RGB2  RGB3
  ;
  ; as seen on the screen.
  ;
  ; The YUV is scaled:
  ;   Ri = Yi + V*2
  ;   Gi = Yi - (V+U/2)
  ;   Bi = Yi + U*2
  ;
  ; We put V*2 in ebx, -(V+U/2) in ecx, and U*2 in ebp

	movsx	ecx,byte ptr[esi][4]	; ecx:  U
	movsx	ebx,byte ptr[esi][5]	; ebx:  V
	mov	ebp,ecx			; ebp:  U
	sar	ecx,1			; ecx:  U/2
	shl	ebp,1			; ebp:  U*2
	add	ecx,ebx			; ecx:  V+U/2
	shl	ebx,1			; ebx:  V*2
	neg	ecx			; ecx:  -(V+U/2)

;     eax	develop unscaled RGB components to index into Bounding24
;     ebx	V*2
;     ecx	-(V+U/2)
;     edx	developing scaled RGB
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	U*2

	xor	eax,eax
	mov	al,byte ptr[esi]	; eax:  Y0

	xor	edx,edx
	mov	dl,Bounding24[ebx][eax]	; edx:  000000R0
	Ref	TEXT32

	shl	edx,16

	mov	dh,Bounding24[ecx][eax]	; edx:  00R0G000
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  00R0G0B0
	Ref	TEXT32

	mov	[edi],edx		; save RGB0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][1]	; eax:  Y1

	xor	edx,edx
	mov	dl,Bounding24[ebx][eax]	; edx:  000000R1
	Ref	TEXT32

	shl	edx,16

	mov	dh,Bounding24[ecx][eax]	; edx:  00R1G100
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  00R1G1B1
	Ref	TEXT32

	mov	[edi][4],edx		; save RGB1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][2]	; eax:  Y2

	xor	edx,edx
	mov	dl,Bounding24[ebx][eax]	; edx:  000000R2
	Ref	TEXT32

	shl	edx,16

	mov	dh,Bounding24[ecx][eax]	; edx:  00R2G200
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  00R2G2B2
	Ref	TEXT32

	mov	[edi][8],edx		; save RGB2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][3]	; eax:  Y3

	xor	edx,edx
	mov	dl,Bounding24[ebx][eax]	; edx:  000000R3
	Ref	TEXT32

	shl	edx,16

	mov	dh,Bounding24[ecx][eax]	; edx:  00R3G300
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  00R3G3B3
	Ref	TEXT32

	mov	[edi][12],edx		; save it

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV

PartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,012345678h	; any more codebook entries?
PartialDoneCompare	label	dword

	jbe	PartialYUVLoop
	jmp	Quit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DoGrey:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,2		; last possible code

	cmp	bl,kFullDBookType + kGreyBookBit
	jne	GreyPartial

	align	4

GreyKeyLoop:

  ; we take the incoming YYYY at [esi] and develop a 2x2 patch:
  ;
  ;     RGB0  RGB1
  ;     RGB2  RGB3
  ;
  ; as seen on the screen.

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,[esi]		; Y3 Y2 Y1 Y0
	add	esi,4

	xor	ebx,ebx
	mov	bl,dl			; Y0
	xor	ecx,ecx
	mov	cl,dh			; Y1

	mov	ebx,GreyDwordLookup[ebx*4]; Y0 Y0 Y0 Y0
	Ref	TEXT32

	mov	ecx,GreyDwordLookup[ecx*4]; Y1 Y1 Y1 Y1
	Ref	TEXT32

	and	ebx,000ffffffh		; 00 Y0 Y0 Y0
	and	ecx,000ffffffh		; 00 Y1 Y1 Y1

	mov	[edi],ebx		; 00 Y0 Y0 Y0
	rol	edx,16
	mov	[edi][4],ecx		; 01 Y1 Y1 Y1

	xor	ebx,ebx
	mov	bl,dl			; Y2
	xor	ecx,ecx
	mov	cl,dh			; Y3

	mov	ebx,GreyDwordLookup[ebx*4]; Y2 Y2 Y2 Y2
	Ref	TEXT32

	mov	ecx,GreyDwordLookup[ecx*4]; Y3 Y3 Y3 Y3
	Ref	TEXT32

	and	ebx,000ffffffh		; 00 Y2 Y2 Y2
	and	ecx,000ffffffh		; 00 Y3 Y3 Y3

	mov	[edi][8],ebx		; 00 Y2 Y2 Y2
	mov	[edi][12],ecx		; 00 Y3 Y3 Y3

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyKeyLoop		; jump if more to do

	jmp	Quit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreyPartial:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	cmp	bl,kPartialDBookType + kGreyBookBit
	jne	Huh		; invalid code

	align	4

GreyPartialLoadSwitches:

  ; We use ebp for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	ebp,[esi]	; get 32 switches
	add	esi,4		; bump source

	rol	bp,8		; swiz
	rol	ebp,16
	rol	bp,8

	stc
	adc	ebp,ebp

  ;  carry, ebp: s ssss ssss ssss ssss ssss ssss ssss sss1

	jmp	short GreyPartialTestSwitch


	align	4

GreyPartialYUVLoop:

;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	add	ebp,ebp		; replace this index?

GreyPartialTestSwitch:
	jnc	GreyPartialYUVSkip
	jz	GreyPartialLoadSwitches

  ; we take the incoming YYYY at [esi] and develop a 2x2 patch:
  ;
  ;     RGB0  RGB1
  ;     RGB2  RGB3
  ;
  ; as seen on the screen.

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	mov	edx,[esi]		; Y3 Y2 Y1 Y0
	add	esi,4

	xor	ebx,ebx
	mov	bl,dl			; Y0
	xor	ecx,ecx
	mov	cl,dh			; Y1

	mov	ebx,GreyDwordLookup[ebx*4]; Y0 Y0 Y0 Y0
	Ref	TEXT32

	mov	ecx,GreyDwordLookup[ecx*4]; Y1 Y1 Y1 Y1
	Ref	TEXT32

	and	ebx,000ffffffh		; 00 Y0 Y0 Y0
	and	ecx,000ffffffh		; 00 Y1 Y1 Y1

	mov	[edi],ebx		; 00 Y0 Y0 Y0
	rol	edx,16
	mov	[edi][4],ecx		; 01 Y1 Y1 Y1

	xor	ebx,ebx
	mov	bl,dl			; Y2
	xor	ecx,ecx
	mov	cl,dh			; Y3

	mov	ebx,GreyDwordLookup[ebx*4]; Y2 Y2 Y2 Y2
	Ref	TEXT32

	mov	ecx,GreyDwordLookup[ecx*4]; Y3 Y3 Y3 Y3
	Ref	TEXT32

	and	ebx,000ffffffh		; 00 Y2 Y2 Y2
	and	ecx,000ffffffh		; 00 Y3 Y3 Y3

	mov	[edi][8],ebx		; 00 Y2 Y2 Y2
	mov	[edi][12],ecx		; 00 Y3 Y3 Y3


GreyPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyPartialYUVLoop

	jmp	Quit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandCodeBook32	endp

	EndMotion

	ENDSEG32


ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	Expand32

_DATA	ends

	end
