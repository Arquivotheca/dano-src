; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/expand8.asm 2.9 1994/09/22 17:17:11 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: expand8.asm $
; Revision 2.9  1994/09/22 17:17:11  bog
; Fix up align and refs to kGreyBookBit.
; Revision 2.8  1994/06/23  14:13:12  bog
; Change movzx into xor/mov pair.
; 
; Revision 2.7  1994/05/06  13:41:33  bog
; Play back black and white movies.
; 
; Revision 2.6  1994/03/04  14:57:09  bog
; Make nRgbLookup a double because int in NT x86 asm version.
; 
; Revision 2.5  1994/03/02  16:08:00  timr
; Move RgbLookup into the BeginMotion/EndMotion bracket.
; 
; Revision 2.4  1993/11/29  08:06:47  geoffs
; Pixel doubling done for 8bpp decompress
; 
; Revision 2.3  1993/07/03  11:43:34  geoffs
; All _DATA segment declarations needed USE16 if WIN16 build
; 
; Revision 2.2  93/07/02  16:16:41  geoffs
; Now compiles,runs under Windows NT
; 
; Revision 2.1  93/06/01  14:49:49  bog
; Compiled, flat decompress assembler.
; 
; Revision 2.0  93/06/01  14:15:03  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.5  93/04/21  15:48:59  bog
; Fix up copyright and disclaimer.
;
; Revision 1.4  93/01/11  09:41:26  timr
; Separate the U and V lookup tables.  Using different nonlinearities for
; U and V gives better rendering of flesh tones.
;
; Revision 1.3  92/12/21  11:37:40  bog
; Split into IC and CV layers.
;
; Revision 1.2  92/12/18  15:46:25  bog
; Final adjustments to signed UV.
;
; Revision 1.1  92/12/01  11:07:15  bog
; Initial revision
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.xlist
	include	cv.inc
	include	cvdecomp.inc
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

	include	palette.inc


	ifndef	NOBLACKWHITE

; GreyByteLookup
;   Map Y in 0..255 to palette index in 10..245.

	public	GreyByteLookup
GreyByteLookup	label	byte

i	=	0

	rept	256
	db	((((i * 236) + 128) shr 8) + 10) and 0ffh
i	=	i+1
	endm


; GreyDwordDither
;   Map Y in 0..255 to 4 dithered indices in 10.245 in a dword.

	public	GreyDwordDither
GreyDwordDither	label	dword

i	=	0
delta	=	3
	rept	256
i1	=	i+delta
	if	i1 ge 256
i1	=	255
	endif
i2	=	i-delta
	if	i2 lt 0
i2	=	0
	endif
	db	((((i * 236) + 128) shr 8) + 10) and 0ffh
	db	((((i1 * 236) + 128) shr 8) + 10) and 0ffh
	db	((((i2 * 236) + 128) shr 8) + 10) and 0ffh
	db	((((i * 236) + 128) shr 8) + 10) and 0ffh
i	=	i+1
	endm

	endif


FixRange	macro	ivp,which,sh
	if	ivp LT n&which
	if	i GE which&s&sh&l&ivp
i&sh	=	ivp
	endif
	endif
	endm

; YLookup
;   256 dwords.  The bytes are Ys2 Ys1 Ys3 Ys0 in each dword.

i	=	0		; for each entry, 0..255
i0	=	0		; value in table
i1	=	0		; value in table
i2	=	0		; value in table
i3	=	0		; value in table

	public	YLookup
YLookup	label	dword

	rept	256

	FixRange	%(i0+1),Y,0
	FixRange	%(i1+1),Y,1
	FixRange	%(i2+1),Y,2
	FixRange	%(i3+1),Y,3

	dd	(i2 shl 24) or (i0 shl 16) or (i3 shl 8) or i1

i	=	i+1

	endm

; ULookup
;   256 dwords.  The bytes are Us2 Us1 Us3 Us0 in each dword.  UV in
;   CompactVideo is signed, not offset by 128.  It also is scaled to
;   half the usual range.

	public	ULookup
ULookup	label	dword

i	=	0		; for each entry, 0..255
i0	=	0		; value in table
i1	=	0		; value in table
i2	=	0		; value in table
i3	=	0		; value in table

	rept	256		; map [0..1) ~ [00..3f]

	FixRange	%(i0+1),U,0
	FixRange	%(i1+1),U,1
	FixRange	%(i2+1),U,2
	FixRange	%(i3+1),U,3

	if	i GE 128
	dd	(i0 shl 27) or (i3 shl 19) or (i1 shl 11) or (i2 shl 3)
	endif

i	=	i+1

	endm

i	=	0		; for each entry, 0..255
i0	=	0		; value in table
i1	=	0		; value in table
i2	=	0		; value in table
i3	=	0		; value in table

	rept	256		; map [-1..0) ~ [c0..ff]

	FixRange	%(i0+1),U,0
	FixRange	%(i1+1),U,1
	FixRange	%(i2+1),U,2
	FixRange	%(i3+1),U,3

	if	i LT 128
	dd	(i0 shl 27) or (i3 shl 19) or (i1 shl 11) or (i2 shl 3)
	endif

i	=	i+1

	endm

; VLookup
;   256 dwords.  The bytes are Vs2 Vs1 Vs3 Vs0 in each dword.  UV in
;   CompactVideo is signed, not offset by 128.  It also is scaled to
;   half the usual range.

	public	VLookup
VLookup	label	dword

i	=	0		; for each entry, 0..255
i0	=	0		; value in table
i1	=	0		; value in table
i2	=	0		; value in table
i3	=	0		; value in table

	rept	256		; map [0..1) ~ [00..3f]

	FixRange	%(i0+1),V,0
	FixRange	%(i1+1),V,1
	FixRange	%(i2+1),V,2
	FixRange	%(i3+1),V,3

	if	i GE 128
	dd	(i3 shl 24) or (i1 shl 16) or (i2 shl 8) or i0
	endif

i	=	i+1

	endm

i	=	0		; for each entry, 0..255
i0	=	0		; value in table
i1	=	0		; value in table
i2	=	0		; value in table
i3	=	0		; value in table

	rept	256		; map [-1..0) ~ [c0..ff]

	FixRange	%(i0+1),V,0
	FixRange	%(i1+1),V,1
	FixRange	%(i2+1),V,2
	FixRange	%(i3+1),V,3

	if	i LT 128
	dd	(i3 shl 24) or (i1 shl 16) or (i2 shl 8) or i0
	endif

i	=	i+1

	endm


RgbVal	macro	val
	dd	Rgb&val
;	dd	(Rgb&val shr 16) or \
;		(Rgb&val and 0ff00h) or \
;		((Rgb&val shl 16) and 0ff0000h)
	endm

		public	_numStdPAL8
_numStdPAL8	dw	nRgb

	align	4

; stdPAL8
;   just our palette, as RGBQUADs

		public	_stdPAL8
_stdPAL8	label	dword

i	=	10		; first index is 10

	rept	nRgb

	RgbVal	%(i)

i	=	i+1

	endm


YUVVal	macro	y,u,v
ifndef	WIN32
	db	low Y&y&U&u&V&v + 10
else
	db	low ((Y&y&U&u&V&v + 10) and 0ffh)
endif
	endm

; RgbLookup
;   Palette index indexed by (Y*nU*nV + U*nV + V)

	public	_nRgbLookup
_nRgbLookup	dd	nY*nU*nV; double for NT x86 asm version


	BeginMotion


	public	_RgbLookup
_RgbLookup	label	byte

	public	RgbLookup
RgbLookup	label	byte

i	=	0		; 0..1023

	rept	nY*nU*nV

	YUVVal	%(i/(nU*nV)),%((i/nV)-((i/(nU*nV))*nU)),%(i-((i/nV)*nV))

i	=	i+1

	endm


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;	BeginMotion


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


CVDecompEntry	ExpandDetailCodeBook8,far

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
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y03
	shl	ebx,6
	or	bl,dh		; Y03U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y03 0000 Y01 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y00
	shl	ebx,6
	or	bl,dl		; Y00U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][8],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y02
	shl	ebx,6
	or	bl,dh		; Y02U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][6],al	; delayed, rot16

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ecx,ecx
	mov	cl,ah		; separate Y1

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y12 0000 Y10 0000 Y13 0000 Y11
	Ref	TEXT32

	mov	al,RgbLookup[ebx]; delayed
	Ref	Motion

	xor	ebx,ebx
	mov	bl,cl		; Y11
	shl	ebx,6
	or	bl,dl		; Y11U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y13
	shl	ebx,6
	or	bl,dh		; Y13U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][14],ax	; delayed, rot16
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y13 0000 Y11 0000 Y12 0000 Y10
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y10
	shl	ebx,6
	or	bl,dl		; Y10U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][1],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y12
	shl	ebx,6
	or	bl,dh		; Y12U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][9],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
	mov	[edi][7],al	; delayed,rot16
	rol	eax,16		; Y1 Y0 Y3 Y2

	xor	ecx,ecx
	mov	cl,al		; separate Y2
	mov	ecx,YLookup[ecx*4]	; 0000 Y22 0000 Y20 0000 Y23 0000 Y21
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y21
	shl	ebx,6
	or	bl,dl		; Y21U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y23
	shl	ebx,6
	or	bl,dh		; Y23U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][4],al	; delayed, rot16
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y23 0000 Y21 0000 Y22 0000 Y20
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y20
	shl	ebx,6
	or	bl,dl		; Y20U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][12],al	; delayed, rot16
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y22
	shl	ebx,6
	or	bl,dh		; Y22U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][2],al	; delayed

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ecx,ecx
	mov	cl,ah		; separate Y3

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y32 0000 Y30 0000 Y33 0000 Y31
	Ref	TEXT32

	mov	al,RgbLookup[ebx]; delayed
	Ref	Motion

	xor	ebx,ebx
	mov	bl,cl		; Y31
	shl	ebx,6
	or	bl,dl		; Y31U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y33
	shl	ebx,6
	or	bl,dh		; Y33U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][10],ax	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y33 0000 Y31 0000 Y32 0000 Y30
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y30
	shl	ebx,6
	or	bl,dl		; Y30U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][5],al	; delayed, rot16
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y32
	shl	ebx,6
	or	bl,dh		; Y32U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][13],al	; delayed, rot16
	mov	al,RgbLookup[ebx]
	Ref	Motion
	mov	[edi][3],al

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
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y03
	shl	ebx,6
	or	bl,dh		; Y03U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y03 0000 Y01 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y00
	shl	ebx,6
	or	bl,dl		; Y00U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][8],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y02
	shl	ebx,6
	or	bl,dh		; Y02U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][6],al	; delayed, rot16

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ecx,ecx
	mov	cl,ah		; separate Y1

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y12 0000 Y10 0000 Y13 0000 Y11
	Ref	TEXT32

	mov	al,RgbLookup[ebx]; delayed
	Ref	Motion

	xor	ebx,ebx
	mov	bl,cl		; Y11
	shl	ebx,6
	or	bl,dl		; Y11U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y13
	shl	ebx,6
	or	bl,dh		; Y13U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][14],ax	; delayed, rot16
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y13 0000 Y11 0000 Y12 0000 Y10
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y10
	shl	ebx,6
	or	bl,dl		; Y10U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][1],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y12
	shl	ebx,6
	or	bl,dh		; Y12U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][9],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
	mov	[edi][7],al	; delayed,rot16
	rol	eax,16		; Y1 Y0 Y3 Y2

	xor	ecx,ecx
	mov	cl,al		; separate Y2
	mov	ecx,YLookup[ecx*4]	; 0000 Y22 0000 Y20 0000 Y23 0000 Y21
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y21
	shl	ebx,6
	or	bl,dl		; Y21U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y23
	shl	ebx,6
	or	bl,dh		; Y23U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][4],al	; delayed, rot16
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y23 0000 Y21 0000 Y22 0000 Y20
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y20
	shl	ebx,6
	or	bl,dl		; Y20U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][12],al	; delayed, rot16
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y22
	shl	ebx,6
	or	bl,dh		; Y22U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][2],al	; delayed

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ecx,ecx
	mov	cl,ah		; separate Y3

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	ecx,YLookup[ecx*4]	; 0000 Y32 0000 Y30 0000 Y33 0000 Y31
	Ref	TEXT32

	mov	al,RgbLookup[ebx]; delayed
	Ref	Motion

	xor	ebx,ebx
	mov	bl,cl		; Y31
	shl	ebx,6
	or	bl,dl		; Y31U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y33
	shl	ebx,6
	or	bl,dh		; Y33U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][10],ax	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y33 0000 Y31 0000 Y32 0000 Y30
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y30
	shl	ebx,6
	or	bl,dl		; Y30U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][5],al	; delayed, rot16
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y32
	shl	ebx,6
	or	bl,dh		; Y32U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][13],al	; delayed, rot16
	mov	al,RgbLookup[ebx]
	Ref	Motion
	mov	[edi][3],al

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

DetailPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,012345678h	; any more codebook entries?
DetailPartialDoneCompare	label	dword

	jbe	DetailPartialYUVLoop
	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	ifndef	NOBLACKWHITE

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

	mov	dl,GreyByteLookup[ebx]; ..inY3.. ..inY2.. ..inY1.. .outY0..
	Ref	TEXT32

	mov	dh,GreyByteLookup[ecx]; ..inY3.. ..inY2.. .outY1.. .outY0..
	Ref	TEXT32

	rol	edx,16		; .outY1.. .outY0.. ..inY3.. ..inY2..

	xor	ebx,ebx
	mov	bl,dl		; separate Y2
	xor	ecx,ecx
	mov	cl,dh		; separate Y3

	mov	dl,GreyByteLookup[ebx]; .outY1.. .outY0.. ..inY3.. .outY2..
	Ref	TEXT32

	mov	dh,GreyByteLookup[ecx]; .outY1.. .outY0.. .outY3.. .outY2..
	Ref	TEXT32

;     eax	-> last possible valid codebook entry
;     edx	Ys:  .outY1.. .outY0.. .outY3.. .outY2..
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi][4],edx
	mov	[edi][12],edx

	rol	edx,16		; .outY3.. .outY2.. .outY1.. .outY0..

	mov	[edi],edx
	mov	[edi][8],edx

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyDKeyLoop	; jump if more to do

	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreyDPartial:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
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

	mov	dl,GreyByteLookup[ebx]; ..inY3.. ..inY2.. ..inY1.. .outY0..
	Ref	TEXT32

	mov	dh,GreyByteLookup[ecx]; ..inY3.. ..inY2.. .outY1.. .outY0..
	Ref	TEXT32

	rol	edx,16		; .outY1.. .outY0.. ..inY3.. ..inY2..

	xor	ebx,ebx
	mov	bl,dl		; separate Y2
	xor	ecx,ecx
	mov	cl,dh		; separate Y3

	mov	dl,GreyByteLookup[ebx]; .outY1.. .outY0.. ..inY3.. .outY2..
	Ref	TEXT32

	mov	dh,GreyByteLookup[ecx]; .outY1.. .outY0.. .outY3.. .outY2..
	Ref	TEXT32

;     eax	-> last possible valid codebook entry
;     edx	Ys:  .outY1.. .outY0.. .outY3.. .outY2..
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	mov	[edi][4],edx
	mov	[edi][12],edx

	rol	edx,16		; .outY3.. .outY2.. .outY1.. .outY0..

	mov	[edi],edx
	mov	[edi][8],edx

GreyDPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyDPartialYUVLoop

	jmp	DetailQuit

	endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandDetailCodeBook8	endp



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


CVDecompEntry	ExpandSmoothCodeBook8,far

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
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y03
	shl	ebx,6
	or	bl,dh		; Y03U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y03 0000 Y01 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y00
	shl	ebx,6
	or	bl,dl		; Y00U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][1],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y02
	shl	ebx,6
	or	bl,dh		; Y02U0V3 00000000 00000000 000000 yyyy uuu vvv
	xor	ecx,ecx
	mov	cl,ah		; separate Y1
	mov	ah,RgbLookup[ebx]
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	[edi][4],ax	; delayed

	mov	ecx,YLookup[ecx*4]	; 0000 Y12 0000 Y10 0000 Y13 0000 Y11
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y11
	shl	ebx,6
	or	bl,dl		; Y11U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y13
	shl	ebx,6
	or	bl,dh		; Y13U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y13 0000 Y11 0000 Y12 0000 Y10
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y10
	shl	ebx,6
	or	bl,dl		; Y10U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][6],ax	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y12
	shl	ebx,6
	or	bl,dh		; Y12U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
	mov	[edi][2],ax	; delayed
	shr	eax,16		; 00 00 Y3 Y2

	xor	ecx,ecx
	mov	cl,al		; separate Y2
	mov	ecx,YLookup[ecx*4]	; 0000 Y22 0000 Y20 0000 Y23 0000 Y21
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y21
	shl	ebx,6
	or	bl,dl		; Y21U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y23
	shl	ebx,6
	or	bl,dh		; Y23U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][13],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y23 0000 Y21 0000 Y22 0000 Y20
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y20
	shl	ebx,6
	or	bl,dl		; Y20U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][8],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y22
	shl	ebx,6
	or	bl,dh		; Y22U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][9],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	xor	ecx,ecx
	mov	cl,ah		; separate Y3
	mov	[edi][12],al	; delayed
	mov	ecx,YLookup[ecx*4]	; 0000 Y32 0000 Y30 0000 Y33 0000 Y31
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y31
	shl	ebx,6
	or	bl,dl		; Y31U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y33
	shl	ebx,6
	or	bl,dh		; Y33U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][11],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y33 0000 Y31 0000 Y32 0000 Y30
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y30
	shl	ebx,6
	or	bl,dl		; Y30U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y32
	shl	ebx,6
	or	bl,dh		; Y32U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][14],ax	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion
	mov	[edi][10],al

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
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y03
	shl	ebx,6
	or	bl,dh		; Y03U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y03 0000 Y01 0000 Y02 0000 Y00
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y00
	shl	ebx,6
	or	bl,dl		; Y00U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][1],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y02
	shl	ebx,6
	or	bl,dh		; Y02U0V3 00000000 00000000 000000 yyyy uuu vvv
	xor	ecx,ecx
	mov	cl,ah		; separate Y1
	mov	ah,RgbLookup[ebx]
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	mov	[edi][4],ax	; delayed

	mov	ecx,YLookup[ecx*4]	; 0000 Y12 0000 Y10 0000 Y13 0000 Y11
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y11
	shl	ebx,6
	or	bl,dl		; Y11U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y13
	shl	ebx,6
	or	bl,dh		; Y13U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y13 0000 Y11 0000 Y12 0000 Y10
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y10
	shl	ebx,6
	or	bl,dl		; Y10U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][6],ax	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y12
	shl	ebx,6
	or	bl,dh		; Y12U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0
	mov	[edi][2],ax	; delayed
	shr	eax,16		; 00 00 Y3 Y2

	xor	ecx,ecx
	mov	cl,al		; separate Y2
	mov	ecx,YLookup[ecx*4]	; 0000 Y22 0000 Y20 0000 Y23 0000 Y21
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y21
	shl	ebx,6
	or	bl,dl		; Y21U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y23
	shl	ebx,6
	or	bl,dh		; Y23U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][13],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y23 0000 Y21 0000 Y22 0000 Y20
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y20
	shl	ebx,6
	or	bl,dl		; Y20U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][8],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y22
	shl	ebx,6
	or	bl,dh		; Y22U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][9],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	rol	edx,16			; 00 U0 V3 00 U3 V1 00 U1 V2 00 U2 V0

	xor	ecx,ecx
	mov	cl,ah		; separate Y3
	mov	[edi][12],al	; delayed
	mov	ecx,YLookup[ecx*4]	; 0000 Y32 0000 Y30 0000 Y33 0000 Y31
	Ref	TEXT32

	xor	ebx,ebx
	mov	bl,cl		; Y31
	shl	ebx,6
	or	bl,dl		; Y31U2V0 00000000 00000000 000000 yyyy uuu vvv
	mov	al,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y33
	shl	ebx,6
	or	bl,dh		; Y33U1V2 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][11],al	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion

	rol	ecx,16			; 0000 Y33 0000 Y31 0000 Y32 0000 Y30
	rol	edx,16			; 00 U1 V2 00 U2 V0 00 U0 V3 00 U3 V1

	xor	ebx,ebx
	mov	bl,cl		; Y30
	shl	ebx,6
	or	bl,dl		; Y30U3V1 00000000 00000000 000000 yyyy uuu vvv
	mov	ah,RgbLookup[ebx]
	Ref	Motion

	xor	ebx,ebx
	mov	bl,ch		; Y32
	shl	ebx,6
	or	bl,dh		; Y32U0V3 00000000 00000000 000000 yyyy uuu vvv
	mov	[edi][14],ax	; delayed
	mov	al,RgbLookup[ebx]
	Ref	Motion
	mov	[edi][10],al

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

	xor	ebx,ebx
	mov	bl,dl		; separate Y0
	xor	ecx,ecx
	mov	cl,dh		; separate Y1

	rol	edx,16		; Y1 Y0 Y3 Y2

	mov	ebx,GreyDwordDither[ebx*4]; Y03 Y02 Y01 Y00
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y13 Y12 Y11 Y10
	Ref	TEXT32

	rol	ecx,16		; Y11 Y10 Y13 Y12
	xchg	bx,cx		; ebx: Y03 Y02 Y13 Y12, ecx: Y11 Y10 Y01 Y00
	rol	ebx,16		; Y13 Y12 Y03 Y02

	mov	[edi],ecx	; Y11 Y10 Y01 Y00
	mov	[edi][4],ebx	; Y13 Y12 Y03 Y02

	xor	ebx,ebx
	mov	bl,dl		; separate Y2
	xor	ecx,ecx
	mov	cl,dh		; separate Y3

	mov	ebx,GreyDwordDither[ebx*4]; Y23 Y22 Y21 Y20
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y33 Y32 Y31 Y30
	Ref	TEXT32

	rol	ecx,16		; Y31 Y30 Y33 Y32
	xchg	bx,cx		; ebx: Y23 Y22 Y33 Y32, ecx: Y31 Y30 Y21 Y20
	rol	ebx,16		; Y33 Y32 Y23 Y22

	mov	[edi][8],ecx	; Y31 Y30 Y21 Y20
	mov	[edi][12],ebx	; Y33 Y32 Y23 Y22

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

	rol	edx,16		; Y1 Y0 Y3 Y2

	mov	ebx,GreyDwordDither[ebx*4]; Y03 Y02 Y01 Y00
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y13 Y12 Y11 Y10
	Ref	TEXT32

	rol	ecx,16		; Y11 Y10 Y13 Y12
	xchg	bx,cx		; ebx: Y03 Y02 Y13 Y12, ecx: Y11 Y10 Y01 Y00
	rol	ebx,16		; Y13 Y12 Y03 Y02

	mov	[edi],ecx	; Y11 Y10 Y01 Y00
	mov	[edi][4],ebx	; Y13 Y12 Y03 Y02

	xor	ebx,ebx
	mov	bl,dl		; separate Y2
	xor	ecx,ecx
	mov	cl,dh		; separate Y3

	mov	ebx,GreyDwordDither[ebx*4]; Y23 Y22 Y21 Y20
	Ref	TEXT32

	mov	ecx,GreyDwordDither[ecx*4]; Y33 Y32 Y31 Y30
	Ref	TEXT32

	rol	ecx,16		; Y31 Y30 Y33 Y32
	xchg	bx,cx		; ebx: Y23 Y22 Y33 Y32, ecx: Y31 Y30 Y21 Y20
	rol	ebx,16		; Y33 Y32 Y23 Y22

	mov	[edi][8],ecx	; Y31 Y30 Y21 Y20
	mov	[edi][12],ebx	; Y33 Y32 Y23 Y22

GreySPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreySPartialYUVLoop

	jmp	SmoothQuit

	endif


_ExpandSmoothCodeBook8	endp

	EndMotion

	ENDSEG32

ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	Expand8

_DATA	ends

	end
