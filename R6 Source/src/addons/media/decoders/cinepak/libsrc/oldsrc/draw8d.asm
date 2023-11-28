; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/draw8d.asm 1.2 1994/06/23 14:12:47 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: draw8d.asm $
; Revision 1.2  1994/06/23 14:12:47  bog
; Change movzx into xor/mov pair.
; Revision 1.1  1993/11/29  16:06:14  geoffs
; Pixel doubling done for 8bpp decompress
; 


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	.386p

	include	cv.inc
	include	cvdecomp.inc

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

; In the case of 8 bit pixel doubled DIBs, detail and smooth codebooks
; are not identical.  In the blocks below, the pixels are shown as CP, with C
; color and P position.
  
; Detail codebooks simply contain the 16 bytes properly dithered:

;   ------------+-----------+-----------+------------
;   |00|01|10|11|02|03|12|13|20|21|30|31|22|23|32|33|
;   ------------+-----------+-----------+------------

; Smooth codebooks have the 4 orientations of each color saved as they
; appear on the top scanline of each 4x4 block of color:

;   ------------+-----------+-----------+------------
;   |02|03|00|01|12|13|10|11|22|23|20|21|32|33|30|31|
;   ------------+-----------+-----------+------------

; These correspond to screen patches that are 4x4 for detail:

;   +-------------------+
;   | 00 | 01 | 10 | 11 |
;   |-------------------|
;   | 02 | 03 | 12 | 13 |
;   |-------------------|
;   | 20 | 21 | 30 | 31 |
;   |-------------------|
;   | 22 | 23 | 32 | 33 |
;   +-------------------+

; and 8x8 for smooth:

;   +-------------------+-------------------+
;   | 00 | 01 | 02 | 03 | 10 | 11 | 12 | 13 |
;   |-------------------|-------------------|
;   | 02 | 03 | 00 | 01 | 12 | 13 | 10 | 11 |
;   |-------------------|-------------------|
;   | 01 | 02 | 03 | 00 | 11 | 12 | 13 | 10 |
;   |-------------------|-------------------|
;   | 03 | 00 | 01 | 02 | 13 | 10 | 11 | 12 |
;   |-------------------+-------------------|
;   | 20 | 21 | 22 | 23 | 30 | 31 | 32 | 33 |
;   |-------------------|-------------------|
;   | 22 | 23 | 20 | 21 | 32 | 33 | 30 | 31 |
;   |-------------------|-------------------|
;   | 21 | 22 | 23 | 20 | 31 | 32 | 33 | 30 |
;   |-------------------|-------------------|
;   | 23 | 20 | 21 | 22 | 33 | 30 | 31 | 32 |
;   +-------------------+-------------------+


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
; void DrawKey8(
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


CVDecompEntry	DrawKey8Doubled,far

	movzx	eax,Height		; for counting down Height
	shr	eax,3			; patches
	mov	nVPatches,eax
	Ref	Motion

	mov	ecx,bWidth/8
	Ref	Width8

	mov	esi,pIx			; -> incoming indices
ifndef	WIN32
	les	edi,pB			; -> outgoing DIB
else
	mov	edi,pB			; -> outgoing DIB
endif

;     ecx	counting down width
;     edx	detail/smooth switches
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

;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

KeyLoop:

	add	edx,edx			; detail/smooth to carry

KeyTest:

	jnc	KeySmooth		; jump if smooth

	jz	KeyNewSwitch		; jump if need new switch

;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

	mov	eax,[esi]		; I3 I2 I1 I0
	add	esi,4

  ; We place the 64 corresponding indices:

  ;   000  001  010  011   100  101  110  111 
  ;   002  003  012  013   102  103  112  113 
  ;   020  021  030  031   120  121  130  131 
  ;   022  023  032  033   122  123  132  133 

  ;   200  201  210  211   300  301  310  311 
  ;   202  203  212  213   302  303  312  313 
  ;   220  221  230  231   320  321  330  331 
  ;   222  223  232  233   322  323  332  333 

;     eax	I3 I2 I1 I0
;     ebx	working Ii*16
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     edi	-> outgoing DIB
;     ebp	working RGB

	xor	ebx,ebx
	mov	bl,al			; I0, 00..03
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; 000 001 010 011
	Ref	Codebook

	mov	SEG_pB:[edi],ebp

	mov	ebp,ds:Codebook[4][ebx]	; 002 003 012 013
	Ref	Codebook

	mov	SEG_pB:YStep[edi],ebp
	Ref	YStep1

	mov	ebp,ds:Codebook[8][ebx]	; 020 021 030 031
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[12][ebx]; 022 023 032 033
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi],ebp
	Ref	YStep3


	xor	ebx,ebx
	mov	bl,ah			; I1, 10..13
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; 100 101 110 111
	Ref	Codebook

	mov	SEG_pB:[edi][4],ebp

	mov	ebp,ds:Codebook[4][ebx]	; 102 103 112 113
	Ref	Codebook

	mov	SEG_pB:YStep[edi][4],ebp
	Ref	YStep1

	mov	ebp,ds:Codebook[8][ebx]	; 120 121 130 131
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi][4],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[12][ebx]; 122 123 132 133
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi][4],ebp
	Ref	YStep3


	shr	eax,16			; ....I3I2

	xor	ebx,ebx
	mov	bl,al			; I2, 20..23
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; 200 201 210 211
	Ref	Codebook

	mov	SEG_pB:(YStep*4)[edi],ebp
	Ref	YStep4

	mov	ebp,ds:Codebook[4][ebx]	; 202 203 212 213
	Ref	Codebook

	mov	SEG_pB:(YStep*5)[edi],ebp
	Ref	YStep5

	mov	ebp,ds:Codebook[8][ebx]	; 220 221 230 231
	Ref	Codebook

	mov	SEG_pB:(YStep*6)[edi],ebp
	Ref	YStep6

	mov	ebp,ds:Codebook[12][ebx]; 222 223 232 233
	Ref	Codebook

	mov	SEG_pB:(YStep*7)[edi],ebp
	Ref	YStep7


	xor	ebx,ebx
	mov	bl,ah			; I3, 30..33
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; 300 301 310 311
	Ref	Codebook

	mov	SEG_pB:(YStep*4)[edi][4],ebp
	Ref	YStep4

	mov	ebp,ds:Codebook[4][ebx]	; 302 303 312 313
	Ref	Codebook

	mov	SEG_pB:(YStep*5)[edi][4],ebp
	Ref	YStep5

	mov	ebp,ds:Codebook[8][ebx]	; 320 321 330 331
	Ref	Codebook

	mov	SEG_pB:(YStep*6)[edi][4],ebp
	Ref	YStep6

	mov	ebp,ds:Codebook[12][ebx]; 322 323 332 333
	Ref	Codebook

	mov	SEG_pB:(YStep*7)[edi][4],ebp
	Ref	YStep7


	add	edi,8			; account for 8 pixels in width

	dec	ecx			; account for 8 pixels written
	jnz	KeyLoop			; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


KeyEndLine:
	mov	ecx,bWidth/8		; for next swath
	Ref	Width8

	add	edi,YDelta
	Ref	YDelta

	dec	nVPatches		; account for 8 more scanlines
	Ref	Motion
	jnz	KeyLoop			; jump if more lines to do

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


KeySmooth:

;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

	xor	ebx,ebx
	mov	bl,byte ptr[esi]	; I
	inc	esi
	shl	ebx,4

  ; We place the 64 corresponding indices:

  ;   00 01 02 03  10 11 12 13
  ;   02 03 00 01  12 13 10 11
  ;   01 02 03 00  11 12 13 10
  ;   03 00 01 02  13 10 11 12

  ;   20 21 22 23  30 31 32 33
  ;   22 23 20 21  32 33 30 31
  ;   21 22 23 20  31 32 33 30
  ;   23 20 21 22  33 30 31 32

;     eax	working RGB
;     ebx	working Ii*16
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

	mov	eax,ds:Codebook[ebx][4096]	; 02 03 00 01
	Ref	Codebook

	mov	SEG_pB:YStep[edi],eax
	Ref	YStep1

	ror	eax,8				; 03 00 01 02

	mov	SEG_pB:(YStep*3)[edi],eax
	Ref	YStep3

	ror	eax,8				; 00 01 02 03

	mov	SEG_pB:[edi],eax

	ror	eax,8				; 01 02 03 00

	mov	SEG_pB:(YStep*2)[edi],eax
	Ref	YStep2


	mov	eax,ds:Codebook[ebx][4096][4]	; 12 13 10 11
	Ref	Codebook

	mov	SEG_pB:YStep[edi][4],eax
	Ref	YStep1

	ror	eax,8				; 13 10 11 12

	mov	SEG_pB:(YStep*3)[edi][4],eax
	Ref	YStep3

	ror	eax,8				; 10 11 12 13

	mov	SEG_pB:[edi][4],eax

	ror	eax,8				; 11 12 13 10

	mov	SEG_pB:(YStep*2)[edi][4],eax
	Ref	YStep2


	mov	eax,ds:Codebook[ebx][4096][8]	; 22 23 20 21
	Ref	Codebook

	mov	SEG_pB:(YStep*5)[edi],eax
	Ref	YStep5

	ror	eax,8				; 23 20 21 22

	mov	SEG_pB:(YStep*7)[edi],eax
	Ref	YStep7

	ror	eax,8				; 20 21 22 23

	mov	SEG_pB:(YStep*4)[edi],eax
	Ref	YStep4

	ror	eax,8				; 21 22 23 20

	mov	SEG_pB:(YStep*6)[edi],eax
	Ref	YStep6


	mov	eax,ds:Codebook[ebx][4096][12]	; 32 33 30 31
	Ref	Codebook

	mov	SEG_pB:(YStep*5)[edi][4],eax
	Ref	YStep5

	ror	eax,8				; 33 30 31 32

	mov	SEG_pB:(YStep*7)[edi][4],eax
	Ref	YStep7

	ror	eax,8				; 30 31 32 33

	mov	SEG_pB:(YStep*4)[edi][4],eax
	Ref	YStep4

	ror	eax,8				; 31 32 33 30

	mov	SEG_pB:(YStep*6)[edi][4],eax
	Ref	YStep6


	add	edi,8			; account for 8 pixels in width

	dec	ecx			; account for 8 pixels written
	jnz	KeyLoop			; jump if more to do

	jmp	KeyEndLine		; handle end of line


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawKey8Doubled	endp



;--------------------------------------------------------------------------
;
; void DrawSmooth8Doubled(
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


CVDecompEntry	DrawSmooth8Doubled,far

	mov	ecx,bWidth/8		; for next swath
	Ref	Width8

	mov	esi,pIx			; -> incoming indices
ifndef	WIN32
	les	edi,pB			; -> outgoing DIB
else
	mov	edi,pB			; -> outgoing DIB
endif

;     ecx	counting down width/8
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

	movzx	edx,Height		; for counting down Height
	shr	edx,3			; patches

	mov	ebp,YDelta		; ebp:  YStep*8 - Width
	Ref	YDelta

;     ecx	counting down width/8
;     edx	counting down height/8
;     esi	-> incoming indices
;     edi	-> outgoing DIB
;     ebp	YStep*8 - width


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SmoothLoop:

;     ecx	counting down width
;     edx	counting down height/8
;     esi	-> incoming indices
;     edi	-> outgoing DIB
;     ebp	YStep*8 - width

	xor	ebx,ebx
	mov	bl,byte ptr[esi]	; I
	inc	esi
	shl	ebx,4

  ; We place the 16 corresponding indices:
  ;
  ;   0 1  2 3
  ;   4 5  6 7
  ;
  ;   8 9  a b
  ;   c d  e f

;     eax	working RGB
;     ebx	working Ii*16
;     ecx	counting down width
;     edx	counting down height/8
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	YStep*8 - width

	mov	eax,ds:Codebook[ebx][4096]	; 02 03 00 01
	Ref	Codebook

	mov	SEG_pB:YStep[edi],eax
	Ref	YStep1

	ror	eax,8				; 03 00 01 02

	mov	SEG_pB:(YStep*3)[edi],eax
	Ref	YStep3

	ror	eax,8				; 00 01 02 03

	mov	SEG_pB:[edi],eax

	ror	eax,8				; 01 02 03 00

	mov	SEG_pB:(YStep*2)[edi],eax
	Ref	YStep2


	mov	eax,ds:Codebook[ebx][4096][4]	; 12 13 10 11
	Ref	Codebook

	mov	SEG_pB:YStep[edi][4],eax
	Ref	YStep1

	ror	eax,8				; 13 10 11 12

	mov	SEG_pB:(YStep*3)[edi][4],eax
	Ref	YStep3

	ror	eax,8				; 10 11 12 13

	mov	SEG_pB:[edi][4],eax

	ror	eax,8				; 11 12 13 10

	mov	SEG_pB:(YStep*2)[edi][4],eax
	Ref	YStep2


	mov	eax,ds:Codebook[ebx][4096][8]	; 22 23 20 21
	Ref	Codebook

	mov	SEG_pB:(YStep*5)[edi],eax
	Ref	YStep5

	ror	eax,8				; 23 20 21 22

	mov	SEG_pB:(YStep*7)[edi],eax
	Ref	YStep7

	ror	eax,8				; 20 21 22 23

	mov	SEG_pB:(YStep*4)[edi],eax
	Ref	YStep4

	ror	eax,8				; 21 22 23 20

	mov	SEG_pB:(YStep*6)[edi],eax
	Ref	YStep6


	mov	eax,ds:Codebook[ebx][4096][12]	; 32 33 30 31
	Ref	Codebook

	mov	SEG_pB:(YStep*5)[edi][4],eax
	Ref	YStep5

	ror	eax,8				; 33 30 31 32

	mov	SEG_pB:(YStep*7)[edi][4],eax
	Ref	YStep7

	ror	eax,8				; 30 31 32 33

	mov	SEG_pB:(YStep*4)[edi][4],eax
	Ref	YStep4

	ror	eax,8				; 31 32 33 30

	mov	SEG_pB:(YStep*6)[edi][4],eax
	Ref	YStep6

	add	edi,8			; account for 8 pixels in width

	dec	ecx			; account for 8 pixels written
	jnz	SmoothLoop		; jump if more to do

	mov	ecx,bWidth/8		; for next swath
	Ref	Width8

	add	edi,ebp

	dec	edx			; account for 8 more scanlines
	jnz	SmoothLoop		; jump if more lines to do

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawSmooth8Doubled	endp


;--------------------------------------------------------------------------
;
; void DrawInter8Doubled(
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


CVDecompEntry	DrawInter8Doubled,far

	movzx	eax,Height		; for counting down Height
	shr	eax,3			; patches
	mov	nVPatches,eax
	Ref	Motion

	mov	ecx,bWidth/8
	Ref	Width8

	mov	esi,pIx			; -> incoming indices
ifndef	WIN32
	les	edi,pB			; -> outgoing DIB
else
	mov	edi,pB			; -> outgoing DIB
endif

;     ecx	counting down width/8
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     edi	-> outgoing DIB

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

	mov	eax,[esi]		; I3 I2 I1 I0
	add	esi,4

  ; We place the 16 corresponding indices:
  ;
  ;   00  01   10  11
  ;   02  03   12  13
  ;
  ;   20  21   30  31
  ;   22  23   32  33

;     eax	I3 I2 I1 I0
;     ebx	working Ii*16
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	working RGB

	xor	ebx,ebx
	mov	bl,al			; I0, 00..03
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; 000 001 010 011
	Ref	Codebook

	mov	SEG_pB:[edi],ebp

	mov	ebp,ds:Codebook[4][ebx]	; 002 003 012 013
	Ref	Codebook

	mov	SEG_pB:YStep[edi],ebp
	Ref	YStep1

	mov	ebp,ds:Codebook[8][ebx]	; 020 021 030 031
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[12][ebx]; 022 023 032 033
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi],ebp
	Ref	YStep3


	xor	ebx,ebx
	mov	bl,ah			; I1, 10..13
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; 100 101 110 111
	Ref	Codebook

	mov	SEG_pB:[edi][4],ebp

	mov	ebp,ds:Codebook[4][ebx]	; 102 103 112 113
	Ref	Codebook

	mov	SEG_pB:YStep[edi][4],ebp
	Ref	YStep1

	mov	ebp,ds:Codebook[8][ebx]	; 120 121 130 131
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi][4],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[12][ebx]; 122 123 132 133
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi][4],ebp
	Ref	YStep3


	shr	eax,16			; ....I3I2

	xor	ebx,ebx
	mov	bl,al			; I2, 20..23
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; 200 201 210 211
	Ref	Codebook

	mov	SEG_pB:(YStep*4)[edi],ebp
	Ref	YStep4

	mov	ebp,ds:Codebook[4][ebx]	; 202 203 212 213
	Ref	Codebook

	mov	SEG_pB:(YStep*5)[edi],ebp
	Ref	YStep5

	mov	ebp,ds:Codebook[8][ebx]	; 220 221 230 231
	Ref	Codebook

	mov	SEG_pB:(YStep*6)[edi],ebp
	Ref	YStep6

	mov	ebp,ds:Codebook[12][ebx]; 222 223 232 233
	Ref	Codebook

	mov	SEG_pB:(YStep*7)[edi],ebp
	Ref	YStep7


	xor	ebx,ebx
	mov	bl,ah			; I3, 30..33
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; 300 301 310 311
	Ref	Codebook

	mov	SEG_pB:(YStep*4)[edi][4],ebp
	Ref	YStep4

	mov	ebp,ds:Codebook[4][ebx]	; 302 303 312 313
	Ref	Codebook

	mov	SEG_pB:(YStep*5)[edi][4],ebp
	Ref	YStep5

	mov	ebp,ds:Codebook[8][ebx]	; 320 321 330 331
	Ref	Codebook

	mov	SEG_pB:(YStep*6)[edi][4],ebp
	Ref	YStep6

	mov	ebp,ds:Codebook[12][ebx]; 322 323 332 333
	Ref	Codebook

	mov	SEG_pB:(YStep*7)[edi][4],ebp
	Ref	YStep7


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterSkip:

	add	edi,8			; account for 8 pixels in width

	dec	ecx			; account for 8 pixels written
	jnz	InterLoop		; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterEndLine:
	mov	ecx,bWidth/8		; for next swath
	Ref	Width8

	add	edi,YDelta
	Ref	YDelta

	dec	nVPatches		; account for 8 more scanlines
	Ref	Motion
	jnz	InterLoop		; jump if more lines to do

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterSmooth:

;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

	xor	ebx,ebx
	mov	bl,byte ptr[esi]	; I
	inc	esi
	shl	ebx,4

  ; We place the 16 corresponding indices:
  ;
  ;   0 1  2 3
  ;   4 5  6 7
  ;
  ;   8 9  a b
  ;   c d  e f

;     eax	working RGB
;     ebx	working Ii*16
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

	mov	eax,ds:Codebook[ebx][4096]	; 02 03 00 01
	Ref	Codebook

	mov	SEG_pB:YStep[edi],eax
	Ref	YStep1

	ror	eax,8				; 03 00 01 02

	mov	SEG_pB:(YStep*3)[edi],eax
	Ref	YStep3

	ror	eax,8				; 00 01 02 03

	mov	SEG_pB:[edi],eax

	ror	eax,8				; 01 02 03 00

	mov	SEG_pB:(YStep*2)[edi],eax
	Ref	YStep2


	mov	eax,ds:Codebook[ebx][4096][4]	; 12 13 10 11
	Ref	Codebook

	mov	SEG_pB:YStep[edi][4],eax
	Ref	YStep1

	ror	eax,8				; 13 10 11 12

	mov	SEG_pB:(YStep*3)[edi][4],eax
	Ref	YStep3

	ror	eax,8				; 10 11 12 13

	mov	SEG_pB:[edi][4],eax

	ror	eax,8				; 11 12 13 10

	mov	SEG_pB:(YStep*2)[edi][4],eax
	Ref	YStep2


	mov	eax,ds:Codebook[ebx][4096][8]	; 22 23 20 21
	Ref	Codebook

	mov	SEG_pB:(YStep*5)[edi],eax
	Ref	YStep5

	ror	eax,8				; 23 20 21 22

	mov	SEG_pB:(YStep*7)[edi],eax
	Ref	YStep7

	ror	eax,8				; 20 21 22 23

	mov	SEG_pB:(YStep*4)[edi],eax
	Ref	YStep4

	ror	eax,8				; 21 22 23 20

	mov	SEG_pB:(YStep*6)[edi],eax
	Ref	YStep6


	mov	eax,ds:Codebook[ebx][4096][12]	; 32 33 30 31
	Ref	Codebook

	mov	SEG_pB:(YStep*5)[edi][4],eax
	Ref	YStep5

	ror	eax,8				; 33 30 31 32

	mov	SEG_pB:(YStep*7)[edi][4],eax
	Ref	YStep7

	ror	eax,8				; 30 31 32 33

	mov	SEG_pB:(YStep*4)[edi][4],eax
	Ref	YStep4

	ror	eax,8				; 31 32 33 30

	mov	SEG_pB:(YStep*6)[edi][4],eax
	Ref	YStep6

	add	edi,8			; account for 8 pixels in width

	dec	ecx			; account for 8 pixels written
	jnz	InterLoop		; jump if more to do

	jmp	InterEndLine		; handle end of line


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawInter8Doubled	endp


	EndMotion

	ENDSEG32


ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	Draw8Doubled

_DATA	ends

	end
