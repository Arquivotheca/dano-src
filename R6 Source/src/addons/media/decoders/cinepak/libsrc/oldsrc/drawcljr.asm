; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/drawcljr.asm 1.1 1994/10/05 10:38:54 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: drawcljr.asm $
; Revision 1.1  1994/10/05 10:38:54  bog
; Initial revision

; Code extracted from draw15.asm:
; Revision 2.8  1994/06/23  14:12:09  bog
; Change movzx into xor/mov pair.


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; -------------------------------------------------------
;               DATA SEGMENT DECLARATIONS
; -------------------------------------------------------

	include	cv.inc
	include	cvdecomp.inc

	.386

	SEG32

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; An incoming frame is composed of up to three tiles.  A tile is a
; horizontal band of the image.  Although the data format allows the
; possibility of tiles being arbitrary rectangles on the image, the
; current implementation on the Mac and PC will barf if the tiles are
; anything other than rectangles with a common left edge, abutting in Y,
; that sequentially cover the frame.

; A tile has two parts:

;   1.  Codebooks
;   2.  Code stream

; Incoming codebook entries are in a 2x2 YUV format:

;   -----------------
;   |Y3|Y2|Y1|Y0|U|V|
;   -----------------

; There are two kinds of codebooks.  A detail codebook entry covers a
; 2x2 patch:


;   +---------+
;   | Y0 | Y1 |
;   |---------|    with the U and V coloring the entire patch.
;   | Y2 | Y3 |
;   +---------+

; A smooth codebook entry covers a 4x4 patch:

;   +-------------------+
;   | Y0 | Y0 | Y1 | Y1 |
;   |-------------------|
;   | Y0 | Y0 | Y1 | Y1 |
;   |-------------------|  with U and V again coloring the whole patch.
;   | Y2 | Y2 | Y3 | Y3 |
;   |-------------------|
;   | Y2 | Y2 | Y3 | Y3 |
;   +-------------------+

; The internal codebook (translation table) has room for 512 16-byte
; entries.  The first 256 are for detail codebook entries and the 2nd
; 256 are for smooth entries.
;
; The entries are stored in a form convenient to the decompressor for
; the particular depth to which we are decompressing.  Some of the
; decompressors have internal formats that differ between the two
; groups.

; In the case of Cirrus Logic Pack Jr., detail and smooth codebooks are
; not identical.  Detail:

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

; Smooth:

;       31 27 26 22 21 17 16 12 11   6 5    0
;      +-----+-----+-----+-----+------+------+
;   0: |  y1 |  y1 |  y0 |  y0 |   u  |   v  |
;      +-----+-----+-----+-----+------+------+
;   4: |  y3 |  y3 |  y2 |  y2 |   u  |   v  |
;      +-----+-----+-----+-----+------+------+

; These correspond to screen patches that are 2x2 for detail:

;   +-------------+
;   | yuv0 | yuv1 |
;   |-------------|
;   | yuv2 | yuv3 |
;   +-------------+

; and 4x4 for smooth:

;   +---------------------------+
;   | yuv0 | yuv0 | yuv1 | yuv1 |
;   |---------------------------|
;   | yuv0 | yuv0 | yuv1 | yuv1 |
;   |---------------------------|
;   | yuv2 | yuv2 | yuv3 | yuv3 |
;   |---------------------------|
;   | yuv2 | yuv2 | yuv3 | yuv3 |
;   +---------------------------+

; There are two kinds of codebook streams that fill entries in the
; internal codebooks.
;
; We maintain a current codebook for each tile.  On a key frame, the
; first tile contains a key codebook, kFull[DS]BookType.  Subsequent
; tiles in the key frame can contain partial codebooks that are
; updates to the previous tile.  Frames other than key frames can
; contain tiles with partial codebooks, too.  Those update the
; corresponding codebook from the previous frame.

; A key codebook is used to replace a codebook with a new one.  It
; simply consists of the sequence of 6 byte entries; there is no
; interleaved control bit stream.  The number of entries is determined
; from the size field of the SizeType dword at the front of the stream.

; A partial codebook is used to update the entries in a codebook without
; replacing them all.  The stream is interleaved from two separate
; streams.  The first is a control bit stream and the second is the
; stream of the 6-byte codebook entries.  The first dword after the
; SizeType dword has in it 32 bit switches that control whether the
; codebook should be updated from the next 6 bytes in the stream.  When
; those 32 bits run out, the next dword continues the control stream.
; Thus the control stream and the codebook bytes are interleaved.


; The code stream itself is also interleaved from two separate streams.
; When a code stream contains the control bit stream, the first dword
; has in it 32 bit switches that control interpretation of subsequent
; bytes.  When those 32 bits run out, the next dword continues the
; control stream.  Note that the interleaving continues through the
; entire code stream for a tile; there is no flush at scanline breaks.

; Sequencing through the code stream occurs on 4x4 clumps of pixels; we
; thus process 4 scanlines at a time.  If a clump is a detail clump,
; four bytes in the stream specify the four 2x2 patches making up the
; clump (I<i> are the bytes in the stream):

;   +---------------------------+
;   | I0          | I1          |
;   |   Y00 | Y01 |   Y10 | Y11 |
;   |   --------- |   --------- |
;   |   Y02 | Y03 |   Y11 | Y11 |
;   |---------------------------|
;   | I2          | I3          |
;   |   Y20 | Y21 |   Y30 | Y31 |
;   |   --------- |   --------- |
;   |   Y22 | Y23 |   Y31 | Y33 |
;   |---------------------------|

; A smooth clump paints the 4x4 patch from a single byte in the code
; stream:

;   +-------------------+
;   | Y0 | Y0 | Y1 | Y1 |
;   |-------------------|
;   | Y0 | Y0 | Y1 | Y1 |
;   |-------------------|
;   | Y2 | Y2 | Y3 | Y3 |
;   |-------------------|
;   | Y2 | Y2 | Y3 | Y3 |
;   +-------------------+

; The bytes in the code byte stream are indices into the codebook.  A
; detail code byte indexes into the detail codebook and a smooth byte
; indexes into the smooth codebook.

; There are three kinds of code stream.  A key code stream
; (kIntraCodesType) describes a key tile.  An all-smooth code stream
; (kAllSmoothCodesType) describes a key tile that is composed of only
; smooth codes.  A partial code stream (kInterCodesType) contains
; differences from the previous frame.

; In a key code stream, the bits in the control bit stream describe
; whether the corresponding 4x4 clump ought to be a detail clump,
; requiring four bytes of the code byte stream to specify the colors, or
; a smooth clump, requiring but a single byte to describe the colors of
; the clump.

; In an all-smooth code stream, there is no control stream.  All code
; bytes are smooth indices.

; In a partial code stream, a bit in the control stream indicates
; whether or not the corresponding clump should be changed.  If zero,
; the clump is left unchanged from the previous frame.  If one, the next
; bit in the control bit stream describes whether the clump is a detail
; clump, with four detail index bytes, or smooth, with only one smooth
; index byte.


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	BeginMotion

nVPatches	dd	?		; for counting down Height

;--------------------------------------------------------------------------
;
; void DrawKeyCLJR (
;   unsigned long pIx,		/* -> incoming compressed indices */
;   unsigned long oBits,	/* offset for DIB bits to fill */
;   unsigned short sBits,	/* selector for DIB bits to fill */
;   short Height		/* height (pixels) of tile */
; );
; YStep was built into the code
;
;--------------------------------------------------------------------------

ifndef	WIN32

	assume	ds:DGROUP
	assume	es:nothing
	assume	fs:nothing
	assume	gs:nothing

SEG_pB	equ	es

else

SEG_pB	equ	<ds>

endif

OldBp		equ	(dword ptr ss:[ebp])
tOldBp		=	type OldBp
RetAddr		equ	(dword ptr OldBp[tOldBp])
tRetAddr	=	type RetAddr
pIx		equ	(dword ptr RetAddr[tRetAddr])
tpIx		=	type pIx
ifndef	WIN32
pB		equ	(fword ptr pIx[tpIx])
else
pB		equ	(dword ptr pIx[tpIx])
endif
tpB		=	type pB
Height		equ	(word ptr pB[tpB])
tHeight		=	type Height


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


CVDecompEntry	DrawKeyCLJR,far

	movzx	eax,Height		; for counting down Height
	shr	eax,2			; patches
	mov	nVPatches,eax
	Ref	Motion

	mov	ecx,bWidth/4
	Ref	Width4

	mov	esi,pIx			; -> incoming indices
ifndef	WIN32
	les	edi,pB			; -> outgoing DIB
else
	mov	edi,pB			; -> outgoing DIB
endif

;     ecx	counting down width/4
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


KeyNewSwitch:

  ; We use edx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	edx,[esi]		; get detail/smooth switches
	add	esi,4

	xchg	dl,dh			; swiz
	rol	edx,16
	xchg	dl,dh

	stc
	adc	edx,edx

  ;  carry, edx: s ssss ssss ssss ssss ssss ssss ssss sss1

  	jmp	short KeyTest


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


KeyLoop:

	add	edx,edx			; detail/smooth to carry

KeyTest:

	jnc	KeySmooth		; jump if smooth

	jz	KeyNewSwitch		; jump if need new switch

;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

  ; We place the 16 colors:

  ;               31 27 26 22 21 17 16 12 11   6 5    0
  ;              +-----+-----+-----+-----+------+------+
  ;           0: | y11 | y10 | y01 | y00 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;       YStep: | y13 | y12 | y03 | y02 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;     YStep*2: | y31 | y30 | y21 | y20 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;     YStep*3: | y33 | y32 | y23 | y22 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+

	xor	ebx,ebx
	mov	bl,[esi]		; I0
	shl	ebx,4			; *16

	mov	eax,ds:Codebook[ebx]	; ... ... y01 y00 u/2 v/2
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][8]	; ... ... y03 y02 u/2 v/2
	Ref	Codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ebx,ebx
	mov	bl,[esi][1]		; I1
	shl	ebx,4			; *16

	add	eax,ds:Codebook[ebx][4]	; y11 y10 y01 y00 u v
	Ref	Codebook

	add	ebp,ds:Codebook[ebx][12]; y13 y12 y03 y02 u v
	Ref	Codebook

;     eax	y11 y10 y01 y00 u v
;     ebx	Ii*16 indexing into codebook
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	y13 y12 y03 y02 u v

	mov	SEG_pB:[edi],eax

	mov	SEG_pB:YStep[edi],ebp
	Ref	YStep1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ebx,ebx
	mov	bl,[esi][2]		; I2
	shl	ebx,4			; *16

	mov	eax,ds:Codebook[ebx]	; ... ... y21 y20 u/2 v/2
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][8]	; ... ... y23 y22 u/2 v/2
	Ref	Codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ebx,ebx
	mov	bl,[esi][3]		; I2
	shl	ebx,4			; *16

	add	eax,ds:Codebook[ebx][4]	; y31 y30 y21 y20 u v
	Ref	Codebook

	add	ebp,ds:Codebook[ebx][12]; y33 y32 y23 y22 u v
	Ref	Codebook

;     eax	y31 y30 y21 y20 u v
;     ebx	Ii*16 indexing into codebook
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	y33 y32 y23 y22 u v

	mov	SEG_pB:(YStep*2)[edi],eax
	Ref	YStep2

	mov	SEG_pB:(YStep*3)[edi],ebp
	Ref	YStep3

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4
	add	edi,4			; account for 4 pixels in width

	dec	ecx			; account for 4 pixels written
	jnz	KeyLoop			; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


KeyEndLine:
	mov	ecx,bWidth/4		; for next swath
	Ref	Width4

	add	edi,YDelta
	Ref	YDelta

	dec	nVPatches		; account for 4 more scanlines
	Ref	Motion
	jnz	KeyLoop			; jump if more lines to do

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


KeySmooth:

;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

  ; We place the 4 colors:

  ;               31 27 26 22 21 17 16 12 11   6 5    0
  ;              +-----+-----+-----+-----+------+------+
  ;           0: |  y1 |  y1 |  y0 |  y0 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;       YStep: |  y1 |  y1 |  y0 |  y0 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;     YStep*2: |  y3 |  y3 |  y2 |  y2 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;     YStep*3: |  y3 |  y3 |  y2 |  y2 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+

	xor	ebx,ebx
	mov	bl,[esi]			; I
	inc	esi
	shl	ebx,4				; *16

	mov	eax,ds:Codebook[ebx][4096]	; y1 y1 y0 y0 u v
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][4096][4]	; y3 y3 y2 y2 u v
	Ref	Codebook


;     eax	y1 y1 y0 y0 u v
;     ebx	Ii*16 indexing into codebook
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	y3 y3 y2 y2 u v

	mov	SEG_pB:[edi],eax

	mov	SEG_pB:YStep[edi],eax
	Ref	YStep1

	mov	SEG_pB:(YStep*2)[edi],ebp
	Ref	YStep2

	mov	SEG_pB:(YStep*3)[edi],ebp
	Ref	YStep3

	add	edi,4			; account for 4 pixels in width

	dec	ecx			; account for 4 pixels written
	jnz	KeyLoop			; jump if more to do

	jmp	KeyEndLine		; handle end of line


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawKeyCLJR	endp



;--------------------------------------------------------------------------
;
; void DrawSmoothCLJR(
;   unsigned long pIx,		/* -> incoming compressed indices */
;   unsigned long oBits,	/* offset for DIB bits to fill */
;   unsigned short sBits,	/* selector for DIB bits to fill */
;   short Height		/* height (pixels) of tile */
; );
; YStep was built into the code
;
;--------------------------------------------------------------------------

ifndef	WIN32

	assume	ds:DGROUP
	assume	es:nothing
	assume	fs:nothing
	assume	gs:nothing

SEG_pB	equ	es

else

SEG_pB	equ	<ds>

endif

OldBp		equ	(dword ptr ss:[ebp])
tOldBp		=	type OldBp
RetAddr		equ	(dword ptr OldBp[tOldBp])
tRetAddr	=	type RetAddr
pIx		equ	(dword ptr RetAddr[tRetAddr])
tpIx		=	type pIx
ifndef	WIN32
pB		equ	(fword ptr pIx[tpIx])
else
pB		equ	(dword ptr pIx[tpIx])
endif
tpB		=	type pB
Height		equ	(word ptr pB[tpB])
tHeight		=	type Height


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


CVDecompEntry	DrawSmoothCLJR,far

	movzx	edx,Height		; for counting down Height
	shr	edx,2			; patches

	mov	ecx,bWidth/4
	Ref	Width4

	mov	esi,pIx			; -> incoming indices
ifndef	WIN32
	les	edi,pB			; -> outgoing DIB
else
	mov	edi,pB			; -> outgoing DIB
endif

;     ecx	counting down width/4
;     edx	counting down height/4
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SmoothLoop:

;     ecx	counting down width
;     edx	counting down height/4
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

  ; We place the 4 colors:

  ;               31 27 26 22 21 17 16 12 11   6 5    0
  ;              +-----+-----+-----+-----+------+------+
  ;           0: |  y1 |  y1 |  y0 |  y0 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;       YStep: |  y1 |  y1 |  y0 |  y0 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;     YStep*2: |  y3 |  y3 |  y2 |  y2 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;     YStep*3: |  y3 |  y3 |  y2 |  y2 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+

	xor	ebx,ebx
	mov	bl,[esi]			; I
	inc	esi
	shl	ebx,4				; *16

	mov	eax,ds:Codebook[ebx][4096]	; y1 y1 y0 y0 u v
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][4096][4]	; y3 y3 y2 y2 u v
	Ref	Codebook


;     eax	y1 y1 y0 y0 u v
;     ebx	Ii*16 indexing into codebook
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	y3 y3 y2 y2 u v

	mov	SEG_pB:[edi],eax

	mov	SEG_pB:YStep[edi],eax
	Ref	YStep1

	mov	SEG_pB:(YStep*2)[edi],ebp
	Ref	YStep2

	mov	SEG_pB:(YStep*3)[edi],ebp
	Ref	YStep3

	add	edi,4			; account for 4 pixels in width

	dec	ecx			; account for 4 pixels written
	jnz	SmoothLoop		; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SmoothEndLine:
	mov	ecx,bWidth/4		; for next swath
	Ref	Width4

	add	edi,YDelta
	Ref	YDelta

	dec	edx			; account for 4 more scanlines
	jnz	SmoothLoop			; jump if more lines to do

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawSmoothCLJR	endp



;--------------------------------------------------------------------------
;
; void DrawInterCLJR(
;   unsigned long pIx,		/* -> incoming compressed indices */
;   unsigned long oBits,	/* offset for DIB bits to fill */
;   unsigned short sBits,	/* selector for DIB bits to fill */
;   short Height		/* height (pixels) of tile */
; );
; YStep was built into the code
;
;--------------------------------------------------------------------------

ifndef	WIN32

	assume	ds:DGROUP
	assume	es:nothing
	assume	fs:nothing
	assume	gs:nothing

SEG_pB	equ	es

else

SEG_pB	equ	<ds>

endif

OldBp		equ	(dword ptr ss:[ebp])
tOldBp		=	type OldBp
RetAddr		equ	(dword ptr OldBp[tOldBp])
tRetAddr	=	type RetAddr
pIx		equ	(dword ptr RetAddr[tRetAddr])
tpIx		=	type pIx
ifndef	WIN32
pB		equ	(fword ptr pIx[tpIx])
else
pB		equ	(dword ptr pIx[tpIx])
endif
tpB		=	type pB
Height		equ	(word ptr pB[tpB])
tHeight		=	type Height


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


CVDecompEntry	DrawInterCLJR,far

	movzx	eax,Height		; for counting down Height
	shr	eax,2			; patches
	mov	nVPatches,eax
	Ref	Motion

	mov	ecx,bWidth/4
	Ref	Width4

	mov	esi,pIx			; -> incoming indices
ifndef	WIN32
	les	edi,pB			; -> outgoing DIB
else
	mov	edi,pB			; -> outgoing DIB
endif

;     ecx	counting down width/4
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterNewDoSwitch:

  ; We use edx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	edx,[esi]		; get detail/smooth switches
	add	esi,4

	xchg	dl,dh			; swiz
	rol	edx,16
	xchg	dl,dh

	stc
	adc	edx,edx

  ;  carry, edx: s ssss ssss ssss ssss ssss ssss ssss sss1

  	jmp	short InterDoTest


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterNewDetailSwitch:

	mov	edx,[esi]		; get detail/smooth switches
	add	esi,4

	xchg	dl,dh			; swiz
	rol	edx,16
	xchg	dl,dh

	stc
	adc	edx,edx

  ;  carry, edx: s ssss ssss ssss ssss ssss ssss ssss sss1

  	jmp	short InterDetailTest


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterLoop:

	add	edx,edx			; do/no do patch to carry

InterDoTest:

	jnc	InterSkip		; jump if skip patch

	jz	InterNewDoSwitch	; jump if need new switch


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	add	edx,edx			; detail/smooth to carry

InterDetailTest:

	jnc	InterSmooth		; jump if smooth

	jz	InterNewDetailSwitch	; jump if need new switch

;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

  ; We place the 16 colors:

  ;               31 27 26 22 21 17 16 12 11   6 5    0
  ;              +-----+-----+-----+-----+------+------+
  ;           0: | y11 | y10 | y01 | y00 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;       YStep: | y13 | y12 | y03 | y02 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;     YStep*2: | y31 | y30 | y21 | y20 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;     YStep*3: | y33 | y32 | y23 | y22 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+

	xor	ebx,ebx
	mov	bl,[esi]		; I0
	shl	ebx,4			; *16

	mov	eax,ds:Codebook[ebx]	; ... ... y01 y00 u/2 v/2
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][8]	; ... ... y03 y02 u/2 v/2
	Ref	Codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ebx,ebx
	mov	bl,[esi][1]		; I1
	shl	ebx,4			; *16

	add	eax,ds:Codebook[ebx][4]	; y11 y10 y01 y00 u v
	Ref	Codebook

	add	ebp,ds:Codebook[ebx][12]; y13 y12 y03 y02 u v
	Ref	Codebook

;     eax	y11 y10 y01 y00 u v
;     ebx	Ii*16 indexing into codebook
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	y13 y12 y03 y02 u v

	mov	SEG_pB:[edi],eax

	mov	SEG_pB:YStep[edi],ebp
	Ref	YStep1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ebx,ebx
	mov	bl,[esi][2]		; I2
	shl	ebx,4			; *16

	mov	eax,ds:Codebook[ebx]	; ... ... y21 y20 u/2 v/2
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][8]	; ... ... y23 y22 u/2 v/2
	Ref	Codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	ebx,ebx
	mov	bl,[esi][3]		; I2
	shl	ebx,4			; *16

	add	eax,ds:Codebook[ebx][4]	; y31 y30 y21 y20 u v
	Ref	Codebook

	add	ebp,ds:Codebook[ebx][12]; y33 y32 y23 y22 u v
	Ref	Codebook

;     eax	y31 y30 y21 y20 u v
;     ebx	Ii*16 indexing into codebook
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	y33 y32 y23 y22 u v

	mov	SEG_pB:(YStep*2)[edi],eax
	Ref	YStep2

	mov	SEG_pB:(YStep*3)[edi],ebp
	Ref	YStep3

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4

InterSkip:

	add	edi,4			; account for 4 pixels in width

	dec	ecx			; account for 4 pixels written
	jnz	InterLoop		; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterEndLine:
	mov	ecx,bWidth/4		; for next swath
	Ref	Width4

	add	edi,YDelta
	Ref	YDelta

	dec	nVPatches		; account for 4 more scanlines
	Ref	Motion
	jnz	InterLoop		; jump if more lines to do

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterSmooth:

;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

  ; We place the 4 colors:

  ;               31 27 26 22 21 17 16 12 11   6 5    0
  ;              +-----+-----+-----+-----+------+------+
  ;           0: |  y1 |  y1 |  y0 |  y0 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;       YStep: |  y1 |  y1 |  y0 |  y0 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;     YStep*2: |  y3 |  y3 |  y2 |  y2 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+
  ;     YStep*3: |  y3 |  y3 |  y2 |  y2 |   u  |   v  |
  ;              +-----+-----+-----+-----+------+------+

	xor	ebx,ebx
	mov	bl,[esi]			; I
	inc	esi
	shl	ebx,4				; *16

	mov	eax,ds:Codebook[ebx][4096]	; y1 y1 y0 y0 u v
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][4096][4]	; y3 y3 y2 y2 u v
	Ref	Codebook


;     eax	y1 y1 y0 y0 u v
;     ebx	Ii*16 indexing into codebook
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	y3 y3 y2 y2 u v

	mov	SEG_pB:[edi],eax

	mov	SEG_pB:YStep[edi],eax
	Ref	YStep1

	mov	SEG_pB:(YStep*2)[edi],ebp
	Ref	YStep2

	mov	SEG_pB:(YStep*3)[edi],ebp
	Ref	YStep3

	add	edi,4			; account for 4 pixels in width

	dec	ecx			; account for 4 pixels written
	jnz	InterLoop		; jump if more to do

	jmp	InterEndLine		; handle end of line


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawInterCLJR	endp


	EndMotion

	ENDSEG32


ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	DrawCLJR

_DATA	ends

	end
