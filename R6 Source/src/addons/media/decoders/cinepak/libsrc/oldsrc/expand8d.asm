; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/expand8d.asm 1.6 1994/09/22 17:17:17 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: expand8d.asm $
; Revision 1.6  1994/09/22 17:17:17  bog
; Fix up align and refs to kGreyBookBit.
; Revision 1.5  1994/06/23  14:13:19  bog
; Change movzx into xor/mov pair.
; 
; Revision 1.4  1994/05/06  13:41:40  bog
; Play back black and white movies.
; 
; Revision 1.3  1994/03/04  14:01:36  bog
; No need to initialize RgbLookup since it's copied from master in all cases.
; 
; Revision 1.2  1994/03/02  16:08:20  timr
; Move RgbLookup into the BeginMotion/EndMotion bracket.
; 
; Revision 1.1  1993/11/29  16:06:53  geoffs
; Pixel doubling done for 8bpp decompress
; 
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.xlist
	include	cv.inc
	include	cvdecomp.inc
	include	palette.inc
	.list


	.386

	SEG32

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


; Our 8 bpp dither scheme divides YUV space into prisms.  There are 16
; slots in Y and 8 in U and V.  Of the 16*8*8=1024 possible colors, only
; 232 are valid in RGB space; this number is conveniently less than the
; 236 slots we have available in a Windows palette since Windows
; reserves 20 colors.

; The dither happens as we quantize, when we map Y[0..255] into 0..15,
; U[0..255] into 0..7 and Y[0..255] into 0..7.  There are 4 versions of
; each of these mappings.  Version A is (Y1,U2,V0), B is (Y3,U1,V2), C
; is (Y0,U3,V1) and D is (Y2,U0,V3).  Across a scanline, we simply cycle
; through A,B,C,D,... and start each scanline with A,B,C,D,... so that
; we dither the same on 4x4 patches:

;   YUV
;    120 312 031 203
;    031 203 120 312
;    312 031 203 120
;    203 120 312 031

;   Y
;    1   3   0   2
;    0   2   1   3
;    3   0   2   1
;    2   1   3   0

;   U
;     2   1   3   0
;     3   0   2   1
;     1   3   0   2
;     0   2   1   3

;   V
;      0   2   1   3
;      1   3   0   2
;      2   1   3   0
;      3   0   2   1

; Once we have quantized the YUV into a 0..1023 index, we look up the
; dithered color in our palette table.  This produces an index into our
; standard dither palette.

; We save 2x2 detail pre dithered for the 4 possible positions in the
; destination 4x4.  But the upper left 2x2 and the lower right 2x2 are
; saved rotated by 16 to speed up the inner loop in draw8.


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	extrn	YLookup		:dword
	extrn	ULookup		:dword
	extrn	VLookup		:dword
;	extrn	RgbLookup	:byte
	extrn	GreyDwordDither	:dword

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


; RgbLookup
;   Palette index indexed by (Y*nU*nV + U*nV + V)


	BeginMotion

RgbLookup	label	byte

	db	(nY*nU*nV) DUP (?)


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


CVDecompEntry	ExpandDetailCodeBook8Doubled,far

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
	ja	short DetailQuit

	mov	edi,pCB

;     eax	-> last possible valid codebook entry
;     bl	codebook type
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

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	DetailKeyDoneCompare[-4],eax
	Ref	Motion

	align	4

DetailKeyLoop:

	mov	eax,[esi]	; Y3 Y2 Y1 Y0
	xor	ebx,ebx
	xor	edx,edx
	mov	bl,[esi][4]	; 0  0  0  U
	mov	dl,[esi][5]	; 0  0  0  V
	add	esi,6

;     eax	Y3 Y2 Y1 Y0
;     ebx	0  0  0  U
;     edx	0  0  0  V
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,VLookup[edx*4]; 00000 U3 00000 U1 00000 U2 00000 U0
	Ref	TEXT32
	or	edx,ULookup[ebx*4]; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
	Ref	TEXT32

;     eax	Y3 Y2 Y1 Y0
;     ebx	00000000 00000000 00000000 xxxxxxxx
;     ecx	working Y0n
;     edx	00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
;     esi	-> incoming codebook
;     edi	-> building codebook

	xor	ecx,ecx
	mov	cl,al		; separate Y0
	mov	ecx,YLookup[ecx*4]	; 0000 Y02 0000 Y00 0000 Y03 0000 Y01
	Ref	TEXT32

	mov	bl,cl		; Y01
	shl	ebx,6
	or	bl,dl		; Y01U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]		; 0 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y03
	shl	ebx,6
	or	bl,dh		; Y03U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi],al			; 0 120
	mov	al,RgbLookup[ebx]		; 0 312
	Ref	Motion

	rol	ecx,16			; 0000 Y03 0000 Y01 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y00
	shl	ebx,6
	or	bl,dl		; Y00U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][1],al			; 0 312
	mov	al,RgbLookup[ebx]		; 0 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y02
	shl	ebx,6
	or	bl,dh		; Y02U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][4],al			; 0 031

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ecx,ecx
	mov	cl,ah		; separate Y1

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y12 0000 Y10 0000 Y13 0000 Y11
	Ref	TEXT32

	mov	al,RgbLookup[ebx]		; 0 203
	Ref	Motion

	xor	ebx,ebx
	mov	bl,cl		; Y11
	shl	ebx,6
	or	bl,dl		; Y11U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][5],al			; 0 203
	mov	al,RgbLookup[ebx]		; 1 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y13
	shl	ebx,6
	or	bl,dh		; Y13U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]		; 1 312
	Ref	Motion

	rol	ecx,16			; 0000 Y13 0000 Y11 0000 Y12 0000 Y10
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y10
	shl	ebx,6
	or	bl,dl		; Y10U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][6],ax			; 1 120, 1 312
	mov	al,RgbLookup[ebx]		; 1 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y12
	shl	ebx,6
	or	bl,dh		; Y12U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]		; 1 203
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
	mov	[edi][2],ax			; 1 031, 1 203
	rol	eax,16		; Y1 Y0 Y3 Y2

	xor	ecx,ecx
	mov	cl,al		; separate Y2
	mov	ecx,YLookup[ecx*4]	; 0000 Y22 0000 Y20 0000 Y23 0000 Y21
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y21
	shl	ebx,6
	or	bl,dl		; Y21U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]		; 2 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y23
	shl	ebx,6
	or	bl,dh		; Y23U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][13],al			; 2 120
	mov	al,RgbLookup[ebx]		; 2 312
	Ref	Motion

	rol	ecx,16			; 0000 Y23 0000 Y21 0000 Y22 0000 Y20
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y20
	shl	ebx,6
	or	bl,dl		; Y20U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][8],al			; 2 312
	mov	al,RgbLookup[ebx]		; 2 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y22
	shl	ebx,6
	or	bl,dh		; Y22U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][9],al			; 2 031

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ecx,ecx
	mov	cl,ah		; separate Y3

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y32 0000 Y30 0000 Y33 0000 Y31
	Ref	TEXT32

	mov	al,RgbLookup[ebx]		; 2 203
	Ref	Motion

	xor	ebx,ebx
	mov	bl,cl		; Y31
	shl	ebx,6
	or	bl,dl		; Y31U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][12],al			; 2 203
	mov	ah,RgbLookup[ebx]		; 3 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y33
	shl	ebx,6
	or	bl,dh		; Y33U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]		; 3 312
	Ref	Motion

	shr	ecx,16			; 00000000 00000000 0000 Y32 0000 Y30
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	xchg	bl,cl		; Y30
	shl	ebx,6
	or	bl,dl		; Y30U3V1 00000000 00000000 000000 yyyy uuu vvv

	mov	dl,al				; 3 312

	shr	ecx,2		; Y32
	or	cl,dh		; Y32U0V3 00000000 00000000 000000 yyyy uuu vvv

	mov	dh,RgbLookup[ebx]		; 3 031
	Ref	Motion

	mov	al,RgbLookup[ecx]		; 3 203
	Ref	Motion

	mov	[edi][14],dx			; 3 312, 3 031
	mov	[edi][10],ax			; 3 203, 3 120

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	edi,16		; bump to next RGB patch

	cmp	esi,012345678h	; any more codebook entries?
DetailKeyDoneCompare	label	dword

	jbe	DetailKeyLoop	; jump if more to do
	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DetailPartial:

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	DetailPartialDoneCompare[-4],eax
	Ref	Motion

	align	4

DetailPartialLoadSwitches:

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

	jmp	short DetailPartialTestSwitch

	align	4


DetailPartialYUVLoop:

;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	add	ebp,ebp		; replace this index?

DetailPartialTestSwitch:
	jnc	DetailPartialYUVSkip
	jz	DetailPartialLoadSwitches

	mov	eax,[esi]	; Y3 Y2 Y1 Y0
	xor	ebx,ebx
	xor	edx,edx
	mov	bl,[esi][4]	; 0  0  0  U
	mov	dl,[esi][5]	; 0  0  0  V
	add	esi,6

;     eax	Y3 Y2 Y1 Y0
;     ebx	0  0  0  U
;     edx	0  0  0  V
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	mov	edx,VLookup[edx*4]; 00000 U3 00000 U1 00000 U2 00000 U0
	Ref	TEXT32
	or	edx,ULookup[ebx*4]; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
	Ref	TEXT32

;     eax	Y3 Y2 Y1 Y0
;     ebx	00000000 00000000 00000000 xxxxxxxx
;     ecx	working Y0n
;     edx	00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	xor	ecx,ecx
	mov	cl,al		; separate Y0
	mov	ecx,YLookup[ecx*4]	; 0000 Y02 0000 Y00 0000 Y03 0000 Y01
	Ref	TEXT32

	mov	bl,cl		; Y01
	shl	ebx,6
	or	bl,dl		; Y01U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]		; 0 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y03
	shl	ebx,6
	or	bl,dh		; Y03U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi],al			; 0 120
	mov	al,RgbLookup[ebx]		; 0 312
	Ref	Motion

	rol	ecx,16			; 0000 Y03 0000 Y01 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y00
	shl	ebx,6
	or	bl,dl		; Y00U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][1],al			; 0 312
	mov	al,RgbLookup[ebx]		; 0 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y02
	shl	ebx,6
	or	bl,dh		; Y02U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][4],al			; 0 031

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ecx,ecx
	mov	cl,ah		; separate Y1

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y12 0000 Y10 0000 Y13 0000 Y11
	Ref	TEXT32

	mov	al,RgbLookup[ebx]		; 0 203
	Ref	Motion

	xor	ebx,ebx
	mov	bl,cl		; Y11
	shl	ebx,6
	or	bl,dl		; Y11U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][5],al			; 0 203
	mov	al,RgbLookup[ebx]		; 1 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y13
	shl	ebx,6
	or	bl,dh		; Y13U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]		; 1 312
	Ref	Motion

	rol	ecx,16			; 0000 Y13 0000 Y11 0000 Y12 0000 Y10
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y10
	shl	ebx,6
	or	bl,dl		; Y10U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][6],ax			; 1 120, 1 312
	mov	al,RgbLookup[ebx]		; 1 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y12
	shl	ebx,6
	or	bl,dh		; Y12U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]		; 1 203
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
	mov	[edi][2],ax			; 1 031, 1 203
	rol	eax,16		; Y1 Y0 Y3 Y2

	xor	ecx,ecx
	mov	cl,al		; separate Y2
	mov	ecx,YLookup[ecx*4]	; 0000 Y22 0000 Y20 0000 Y23 0000 Y21
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y21
	shl	ebx,6
	or	bl,dl		; Y21U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]		; 2 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y23
	shl	ebx,6
	or	bl,dh		; Y23U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][13],al			; 2 120
	mov	al,RgbLookup[ebx]		; 2 312
	Ref	Motion

	rol	ecx,16			; 0000 Y23 0000 Y21 0000 Y22 0000 Y20
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y20
	shl	ebx,6
	or	bl,dl		; Y20U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][8],al			; 2 312
	mov	al,RgbLookup[ebx]		; 2 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y22
	shl	ebx,6
	or	bl,dh		; Y22U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][9],al			; 2 031

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ecx,ecx
	mov	cl,ah		; separate Y3

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y32 0000 Y30 0000 Y33 0000 Y31
	Ref	TEXT32

	mov	al,RgbLookup[ebx]		; 2 203
	Ref	Motion

	xor	ebx,ebx
	mov	bl,cl		; Y31
	shl	ebx,6
	or	bl,dl		; Y31U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][12],al			; 2 203
	mov	ah,RgbLookup[ebx]		; 3 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y33
	shl	ebx,6
	or	bl,dh		; Y33U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]		; 3 312
	Ref	Motion

	shr	ecx,16			; 00000000 00000000 0000 Y32 0000 Y30
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	xchg	bl,cl		; Y30
	shl	ebx,6
	or	bl,dl		; Y30U3V1 00000000 00000000 000000 yyyy uuu vvv

	mov	dl,al				; 3 312

	shr	ecx,2		; Y32
	or	cl,dh		; Y32U0V3 00000000 00000000 000000 yyyy uuu vvv

	mov	dh,RgbLookup[ebx]		; 3 031
	Ref	Motion

	mov	al,RgbLookup[ecx]		; 3 203
	Ref	Motion

	mov	[edi][14],dx			; 3 312, 3 031
	mov	[edi][10],ax			; 3 203, 3 120

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

DetailPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,012345678h	; any more codebook entries?
DetailPartialDoneCompare	label	dword

	jbe	DetailPartialYUVLoop
	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ifndef	NOBLACKWHITE

; Detail codebooks simply contain the 16 bytes properly dithered:

DoGreyDetail:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,2		; last possible code

	cmp	bl,kFullDBookType + kGreyBookBit
	jne	GreyDPartial

	align	4

GreyDKeyLoop:

	mov	edx,[esi]	; Y3 Y2 Y1 Y0
	add	esi,4

;     eax	-> last possible valid codebook entry
;     edx	Y3 Y2 Y1 Y0
;     esi	-> incoming codebook
;     edi	-> building codebook

	xor	ebx,ebx
	mov	bl,dl		; separate Y0
	xor	ecx,ecx
	mov	cl,dh		; separate Y1

	mov	ebx,GreyDwordDither[ebx*4]; Y03 Y02 Y01 Y00
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y13 Y12 Y11 Y10
	Ref	TEXT32

	rol	ebx,16		; Y01 Y00 Y03 Y02
	xchg	bx,cx		; ebx: Y01 Y00 Y11 Y10, ecx: Y13 Y12 Y03 Y02
	rol	ebx,16		; Y11 Y10 Y01 Y00

	mov	[edi][4],ecx
	rol	edx,16		; position for Y2 and Y3
	mov	[edi],ebx

	xor	ebx,ebx
	mov	bl,dl		; separate Y2
	xor	ecx,ecx
	mov	cl,dh		; separate Y3

	mov	ebx,GreyDwordDither[ebx*4]; Y23 Y22 Y21 Y20
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y33 Y32 Y31 Y30
	Ref	TEXT32

	rol	ebx,16		; Y21 Y20 Y23 Y22
	xchg	bx,cx		; ebx: Y21 Y20 Y31 Y30, ecx: Y33 Y32 Y23 Y22
	rol	ebx,16		; Y31 Y30 Y21 Y20

	mov	[edi][8],ebx
	mov	[edi][12],ecx

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyDKeyLoop	; jump if more to do

	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreyDPartial:

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	cmp	bl,kPartialDBookType + kGreyBookBit
	jne	DetailHuh	; invalid code

	align	4

GreyDPartialLoadSwitches:

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

	jmp	short GreyDPartialTestSwitch


	align	4

GreyDPartialYUVLoop:

;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	add	ebp,ebp		; replace this index?

GreyDPartialTestSwitch:
	jnc	GreyDPartialYUVSkip
	jz	GreyDPartialLoadSwitches

	mov	edx,[esi]	; Y3 Y2 Y1 Y0
	add	esi,4

;     eax	-> last possible valid codebook entry
;     edx	Y3 Y2 Y1 Y0
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	xor	ebx,ebx
	mov	bl,dl		; separate Y0
	xor	ecx,ecx
	mov	cl,dh		; separate Y1

	mov	ebx,GreyDwordDither[ebx*4]; Y03 Y02 Y01 Y00
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y13 Y12 Y11 Y10
	Ref	TEXT32

	rol	ebx,16		; Y01 Y00 Y03 Y02
	xchg	bx,cx		; ebx: Y01 Y00 Y11 Y10, ecx: Y13 Y12 Y03 Y02
	rol	ebx,16		; Y11 Y10 Y01 Y00

	rol	edx,16		; position for Y2 and Y3
	mov	[edi],ebx
	mov	[edi][4],ecx

	xor	ebx,ebx
	mov	bl,dl		; separate Y2
	xor	ecx,ecx
	mov	cl,dh		; separate Y3

	mov	ebx,GreyDwordDither[ebx*4]; Y23 Y22 Y21 Y20
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y33 Y32 Y31 Y30
	Ref	TEXT32

	rol	ebx,16		; Y21 Y20 Y23 Y22
	xchg	bx,cx		; ebx: Y21 Y20 Y31 Y30, ecx: Y33 Y32 Y23 Y22
	rol	ebx,16		; Y31 Y30 Y21 Y20

	mov	[edi][8],ebx
	mov	[edi][12],ecx

GreyDPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyDPartialYUVLoop

	jmp	DetailQuit

	endif


_ExpandDetailCodeBook8Doubled	endp



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


CVDecompEntry	ExpandSmoothCodeBook8Doubled,far

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
	ja	short SmoothQuit

	mov	edi,pCB

;     eax	-> last possible valid codebook entry
;     bl	codebook type
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

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	SmoothKeyDoneCompare[-4],eax
	Ref	Motion

	align	4

SmoothKeyLoop:

	mov	eax,[esi]	; Y3 Y2 Y1 Y0
	xor	ebx,ebx
	xor	edx,edx
	mov	bl,[esi][4]	; 0  0  0  U
	mov	dl,[esi][5]	; 0  0  0  V
	add	esi,6

;     eax	Y3 Y2 Y1 Y0
;     ebx	0  0  0  U
;     edx	0  0  0  V
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,VLookup[edx*4]; 00000 U3 00000 U1 00000 U2 00000 U0
	Ref	TEXT32
	or	edx,ULookup[ebx*4]; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
	Ref	TEXT32

;     eax	Y3 Y2 Y1 Y0
;     ebx	00000000 00000000 00000000 xxxxxxxx
;     ecx	working Y0n
;     edx	00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
;     esi	-> incoming codebook
;     edi	-> building codebook

	xor	ecx,ecx
	mov	cl,al		; separate Y0
	mov	ecx,YLookup[ecx*4]	; 0000 Y02 0000 Y00 0000 Y03 0000 Y01
	Ref	TEXT32

	mov	bl,cl		; Y01
	shl	ebx,6
	or	bl,dl		; Y01U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 0 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y03
	shl	ebx,6
	or	bl,dh		; Y03U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 0 312
	Ref	Motion

	rol	ecx,16			; 0 312 0 120 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y00
	shl	ebx,6
	or	bl,dl		; Y00U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 0 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y02
	shl	ebx,6
	or	bl,dh		; Y02U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 0 203
	Ref	Motion

	mov	[edi],ecx			; 0 031, 0 203, 0 120, 0 312

	xor	ecx,ecx
	mov	cl,ah		; separate Y1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y12 0000 Y10 0000 Y13 0000 Y11
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y11
	shl	ebx,6
	or	bl,dl		; Y11U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 1 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y13
	shl	ebx,6
	or	bl,dh		; Y13U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 1 312
	Ref	Motion

	rol	ecx,16			; 1 312 1 120 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y10
	shl	ebx,6
	or	bl,dl		; Y10U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 1 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y12
	shl	ebx,6
	or	bl,dh		; Y12U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 1 203
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	shr	eax,16		; 00 00 Y3 Y2

	mov	[edi][4],ecx			; 1 031, 1 203, 1 120, 1 312

	xor	ecx,ecx
	mov	cl,al		; separate Y2

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y22 0000 Y20 0000 Y23 0000 Y21
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y21
	shl	ebx,6
	or	bl,dl		; Y21U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 2 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y23
	shl	ebx,6
	or	bl,dh		; Y23U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 2 312
	Ref	Motion

	rol	ecx,16			; 2 312 2 120 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y20
	shl	ebx,6
	or	bl,dl		; Y20U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 2 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y22
	shl	ebx,6
	or	bl,dh		; Y22U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 2 203
	Ref	Motion

	mov	[edi][8],ecx			; 2 031, 2 203, 2 120, 2 312

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ecx,ecx
	mov	cl,ah		; separate Y3

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y32 0000 Y30 0000 Y33 0000 Y31
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y31
	shl	ebx,6
	or	bl,dl		; Y31U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 3 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y33
	shl	ebx,6
	or	bl,dh		; Y33U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 3 312
	Ref	Motion

	rol	ecx,16			; 3 312 3 120 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y30
	shl	ebx,6
	or	bl,dl		; Y30U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 3 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y32
	shl	ebx,6
	or	bl,dh		; Y32U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 3 203
	Ref	Motion

	mov	[edi][12],ecx			; 3 031, 3 203, 3 120, 3 312

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	edi,16		; bump to next RGB patch

	cmp	esi,012345678h	; any more codebook entries?
SmoothKeyDoneCompare	label	dword

	jbe	SmoothKeyLoop	; jump if more to do
	jmp	SmoothQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SmoothPartial:

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	SmoothPartialDoneCompare[-4],eax
	Ref	Motion

	align	4

SmoothPartialLoadSwitches:

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

	jmp	short SmoothPartialTestSwitch

	align	4


SmoothPartialYUVLoop:

;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	add	ebp,ebp		; replace this index?

SmoothPartialTestSwitch:
	jnc	SmoothPartialYUVSkip
	jz	SmoothPartialLoadSwitches

	mov	eax,[esi]	; Y3 Y2 Y1 Y0
	xor	ebx,ebx
	xor	edx,edx
	mov	bl,[esi][4]	; 0  0  0  U
	mov	dl,[esi][5]	; 0  0  0  V
	add	esi,6

;     eax	Y3 Y2 Y1 Y0
;     ebx	0  0  0  U
;     edx	0  0  0  V
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	mov	edx,VLookup[edx*4]; 00000 U3 00000 U1 00000 U2 00000 U0
	Ref	TEXT32
	or	edx,ULookup[ebx*4]; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
	Ref	TEXT32

;     eax	Y3 Y2 Y1 Y0
;     ebx	00000000 00000000 00000000 xxxxxxxx
;     ecx	working Y0n
;     edx	00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	xor	ecx,ecx
	mov	cl,al		; separate Y0
	mov	ecx,YLookup[ecx*4]	; 0000 Y02 0000 Y00 0000 Y03 0000 Y01
	Ref	TEXT32

	mov	bl,cl		; Y01
	shl	ebx,6
	or	bl,dl		; Y01U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 0 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y03
	shl	ebx,6
	or	bl,dh		; Y03U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 0 312
	Ref	Motion

	rol	ecx,16			; 0 312 0 120 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y00
	shl	ebx,6
	or	bl,dl		; Y00U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 0 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y02
	shl	ebx,6
	or	bl,dh		; Y02U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 0 203
	Ref	Motion

	mov	[edi],ecx			; 0 031, 0 203, 0 120, 0 312

	xor	ecx,ecx
	mov	cl,ah		; separate Y1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y12 0000 Y10 0000 Y13 0000 Y11
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y11
	shl	ebx,6
	or	bl,dl		; Y11U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 1 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y13
	shl	ebx,6
	or	bl,dh		; Y13U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 1 312
	Ref	Motion

	rol	ecx,16			; 1 312 1 120 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y10
	shl	ebx,6
	or	bl,dl		; Y10U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 1 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y12
	shl	ebx,6
	or	bl,dh		; Y12U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 1 203
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	shr	eax,16		; 00 00 Y3 Y2

	mov	[edi][4],ecx			; 1 031, 1 203, 1 120, 1 312

	xor	ecx,ecx
	mov	cl,al		; separate Y2

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y22 0000 Y20 0000 Y23 0000 Y21
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y21
	shl	ebx,6
	or	bl,dl		; Y21U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 2 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y23
	shl	ebx,6
	or	bl,dh		; Y23U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 2 312
	Ref	Motion

	rol	ecx,16			; 2 312 2 120 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y20
	shl	ebx,6
	or	bl,dl		; Y20U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 2 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y22
	shl	ebx,6
	or	bl,dh		; Y22U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 2 203
	Ref	Motion

	mov	[edi][8],ecx			; 2 031, 2 203, 2 120, 2 312

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ecx,ecx
	mov	cl,ah		; separate Y3

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y32 0000 Y30 0000 Y33 0000 Y31
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y31
	shl	ebx,6
	or	bl,dl		; Y31U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 3 120
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y33
	shl	ebx,6
	or	bl,dh		; Y33U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 3 312
	Ref	Motion

	rol	ecx,16			; 3 312 3 120 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y30
	shl	ebx,6
	or	bl,dl		; Y30U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	cl,RgbLookup[ebx]		; 3 031
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y32
	shl	ebx,6
	or	bl,dh		; Y32U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ch,RgbLookup[ebx]		; 3 203
	Ref	Motion

	mov	[edi][12],ecx			; 3 031, 3 203, 3 120, 3 312

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

SmoothPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,012345678h	; any more codebook entries?
SmoothPartialDoneCompare	label	dword

	jbe	SmoothPartialYUVLoop
	jmp	SmoothQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	ifndef	NOBLACKWHITE

DoGreySmooth:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,2		; last possible code

	cmp	bl,kFullSBookType + kGreyBookBit
	jne	GreySPartial

	align	4

GreySKeyLoop:

	mov	edx,[esi]	; Y3 Y2 Y1 Y0
	add	esi,4

;     eax	-> last possible valid codebook entry
;     edx	Y3 Y2 Y1 Y0
;     esi	-> incoming codebook
;     edi	-> building codebook

;   ------------+-----------+-----------+------------
;   |02|03|00|01|12|13|10|11|22|23|20|21|32|33|30|31|
;   ------------+-----------+-----------+------------


	xor	ebx,ebx
	mov	bl,dl		; separate Y0
	xor	ecx,ecx
	mov	cl,dh		; separate Y1

	rol	edx,16		; Y1 Y0 Y3 Y2

	mov	ebx,GreyDwordDither[ebx*4]; Y03 Y02 Y01 Y00
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y13 Y12 Y11 Y10
	Ref	TEXT32

	rol	ebx,16		; Y01 Y00 Y03 Y02
	rol	ecx,16		; Y11 Y10 Y13 Y12

	mov	[edi],ebx	; Y01 Y00 Y03 Y02

	rol	edx,16		; Y1 Y0 Y3 Y2

	mov	[edi][4],ecx	; Y11 Y10 Y13 Y12

	xor	ebx,ebx
	mov	bl,dl		; separate Y2
	xor	ecx,ecx
	mov	cl,dh		; separate Y3

	mov	ebx,GreyDwordDither[ebx*4]; Y23 Y22 Y21 Y20
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y33 Y32 Y31 Y30
	Ref	TEXT32

	rol	ebx,16		; Y21 Y20 Y23 Y22
	rol	ecx,16		; Y31 Y30 Y33 Y32

	mov	[edi][8],ebx	; Y21 Y20 Y23 Y22
	mov	[edi][12],ecx	; Y31 Y30 Y33 Y32

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreySKeyLoop	; jump if more to do

	jmp	SmoothQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreySPartial:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	cmp	bl,kPartialSBookType + kGreyBookBit
	jne	SmoothHuh	; invalid code

	align	4

GreySPartialLoadSwitches:

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

	jmp	short GreySPartialTestSwitch


	align	4

GreySPartialYUVLoop:

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	add	ebp,ebp		; replace this index?

GreySPartialTestSwitch:
	jnc	GreySPartialYUVSkip
	jz	GreySPartialLoadSwitches

	mov	edx,[esi]	; Y3 Y2 Y1 Y0
	add	esi,4

;     eax	-> last possible valid codebook entry
;     edx	Y3 Y2 Y1 Y0
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	xor	ebx,ebx
	mov	bl,dl		; separate Y0
	xor	ecx,ecx
	mov	cl,dh		; separate Y1

	mov	ebx,GreyDwordDither[ebx*4]; Y03 Y02 Y01 Y00
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y13 Y12 Y11 Y10
	Ref	TEXT32

	rol	ebx,16		; Y01 Y00 Y03 Y02
	rol	ecx,16		; Y11 Y10 Y13 Y12

	mov	[edi],ebx	; Y01 Y00 Y03 Y02

	rol	edx,16		; Y1 Y0 Y3 Y2

	mov	[edi][4],ecx	; Y11 Y10 Y13 Y12

	xor	ebx,ebx
	mov	bl,dl		; separate Y2
	xor	ecx,ecx
	mov	cl,dh		; separate Y3

	mov	ebx,GreyDwordDither[ebx*4]; Y23 Y22 Y21 Y20
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y33 Y32 Y31 Y30
	Ref	TEXT32

	rol	ebx,16		; Y21 Y20 Y23 Y22
	rol	ecx,16		; Y31 Y30 Y33 Y32

	mov	[edi][8],ebx	; Y21 Y20 Y23 Y22
	mov	[edi][12],ecx	; Y31 Y30 Y33 Y32

GreySPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreySPartialYUVLoop

	jmp	SmoothQuit

	endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandSmoothCodeBook8Doubled	endp

	EndMotion

	ENDSEG32

ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	Expand8Doubled

_DATA	ends

	end
