; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/draw32.asm 2.6 1994/06/23 14:12:41 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: draw32.asm $
; Revision 2.6  1994/06/23 14:12:41  bog
; Change movzx into xor/mov pair.
; Revision 2.5  1993/08/10  15:43:44  timr
; MASM386 textual equates need angle brackets.
; 
; Revision 2.4  1993/08/10  11:04:27  bog
; Use SEG_pB to clean up ifndefs.
; 
; Revision 2.3  1993/07/03  11:42:04  geoffs
; All _DATA segment declarations needed USE16 if WIN16 build
; 
; Revision 2.2  93/07/02  16:19:11  geoffs
; Now compiles,runs under Windows NT
; 
; Revision 2.1  93/06/13  11:27:05  bog
; Roll to 2.1.
; 
; Revision 1.1  93/06/09  17:58:53  bog
; Initial revision
; 

; cloned from draw24.asm 9 June 1993

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

; In the case of 32 bit DIBs, detail and smooth codebooks are identical:

;   ------------+-----------+-----------+------------
;   |B0|G0|R0|00|R0|B1|G1|00|B2|G2|R2|00|R2|B3|G3|00|
;   ------------+-----------+-----------+------------

; These correspond to screen patches that are 2x2 for detail:

;   +-------------+
;   | RGB0 | RGB1 |
;   |-------------|
;   | RGB2 | RGB3 |
;   +-------------+

; and 4x4 for smooth:

;   +---------------------------+
;   | RGB0 | RGB0 | RGB1 | RGB1 |
;   |---------------------------|
;   | RGB0 | RGB0 | RGB1 | RGB1 |
;   |---------------------------|
;   | RGB2 | RGB2 | RGB3 | RGB3 |
;   |---------------------------|
;   | RGB2 | RGB2 | RGB3 | RGB3 |
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
; void DrawKey32(
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


CVDecompEntry	DrawKey32,far

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

	mov	eax,[esi]		; I3 I2 I1 I0
	add	esi,4

  ; We place the 16 corresponding indices:
  ;
  ;   RGB0  RGB1   RGB4  RGB5
  ;   RGB2  RGB3   RGB6  RGB7
  ;
  ;   RGB8  RGB9   RGBc  RGBd
  ;   RGBa  RGBb   RGBe  RGBf

;     eax	I3 I2 I1 I0
;     ebx	working Ii*16
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	working RGB

	xor	ebx,ebx
	mov	bl,al			; I0, RGB0..3
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; B0 G0 R0 alpha0
	Ref	Codebook

	mov	SEG_pB:[edi],ebp

	mov	ebp,ds:Codebook[ebx][4]	; B1 G1 R1 alpha1
	Ref	Codebook

	mov	SEG_pB:[edi][4],ebp

	mov	ebp,ds:Codebook[ebx][8]	; B2 G2 R2 alpha2
	Ref	Codebook

	mov	SEG_pB:YStep[edi],ebp
	Ref	YStep1

	mov	ebp,ds:Codebook[ebx][12]; B3 G3 R3 alpha3
	Ref	Codebook

	mov	SEG_pB:YStep[edi][4],ebp
	Ref	YStep1


	xor	ebx,ebx
	mov	bl,ah			; I1, RGB4..7
	shl	ebx,4			; *16
	shr	eax,16			; ....i3i2

	mov	ebp,ds:Codebook[ebx]	; B4 G4 R4 alpha4
	Ref	Codebook

	mov	SEG_pB:[edi][8],ebp

	mov	ebp,ds:Codebook[ebx][4]	; B5 G5 R5 alpha5
	Ref	Codebook

	mov	SEG_pB:[edi][12],ebp

	mov	ebp,ds:Codebook[ebx][8]	; B6 G6 R6 alpha6
	Ref	Codebook

	mov	SEG_pB:YStep[edi][8],ebp
	Ref	YStep1

	mov	ebp,ds:Codebook[ebx][12]; B7 G7 R7 alpha7
	Ref	Codebook

	mov	SEG_pB:YStep[edi][12],ebp
	Ref	YStep1


	xor	ebx,ebx
	mov	bl,al			; I2, RGB8..b
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; B8 G8 R8 alpha8
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[ebx][4]	; B9 G9 R9 alpha9
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi][4],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[ebx][8]	; Ba Ga Ra alphaa
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi],ebp
	Ref	YStep3

	mov	ebp,ds:Codebook[ebx][12]; Bb Gb Rb alphab
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi][4],ebp
	Ref	YStep3


	xor	ebx,ebx
	mov	bl,ah			; I3, RGBc..f
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; Bc Gc Rc alphac
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi][8],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[ebx][4]	; Bd Gd Rd alphad
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi][12],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[ebx][8]	; Be Ge Re alphae
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi][8],ebp
	Ref	YStep3

	mov	ebp,ds:Codebook[ebx][12]; Bf Gf Rf alphaf
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi][12],ebp
	Ref	YStep3


	add	edi,16			; account for 4 pixels in width

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
;     (es):edi	-> outgoing DIB

	xor	ebx,ebx
	mov	bl,byte ptr[esi]	; I
	inc	esi
	shl	ebx,4

  ; We place the 16 corresponding indices:
  ;
  ;   RGB0  RGB0   RGB1  RGB1
  ;   RGB0  RGB0   RGB1  RGB1
  ;
  ;   RGB2  RGB2   RGB3  RGB3
  ;   RGB2  RGB2   RGB3  RGB3

;     eax	working RGB
;     ebx	working Ii*16
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	working RGB

	mov	eax,ds:Codebook[ebx][4096]	; B0 G0 R0 alpha0
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][4096][4]	; B1 G1 R1 alpha1
	Ref	Codebook

	mov	SEG_pB:[edi],eax
	mov	SEG_pB:[edi][4],eax

	mov	SEG_pB:[edi][8],ebp
	mov	SEG_pB:[edi][12],ebp

	mov	SEG_pB:YStep[edi],eax
	Ref	YStep1

	mov	SEG_pB:YStep[edi][4],eax
	Ref	YStep1

	mov	SEG_pB:YStep[edi][8],ebp
	Ref	YStep1

	mov	SEG_pB:YStep[edi][12],ebp
	Ref	YStep1


	mov	eax,ds:Codebook[ebx][4096][8]	; B2 G2 R2 alpha2
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][4096][12]	; B3 G3 R3 alpha3
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi],eax
	Ref	YStep2

	mov	SEG_pB:(YStep*2)[edi][4],eax
	Ref	YStep2

	mov	SEG_pB:(YStep*2)[edi][8],ebp
	Ref	YStep2

	mov	SEG_pB:(YStep*2)[edi][12],ebp
	Ref	YStep2

	mov	SEG_pB:(YStep*3)[edi],eax
	Ref	YStep3

	mov	SEG_pB:(YStep*3)[edi][4],eax
	Ref	YStep3

	mov	SEG_pB:(YStep*3)[edi][8],ebp
	Ref	YStep3

	mov	SEG_pB:(YStep*3)[edi][12],ebp
	Ref	YStep3


	add	edi,16			; account for 4 pixels in width

	dec	ecx			; account for 4 pixels written
	jnz	KeyLoop			; jump if more to do

	jmp	KeyEndLine		; handle end of line


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawKey32	endp



;--------------------------------------------------------------------------
;
; void DrawSmooth32(
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


CVDecompEntry	DrawSmooth32,far

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

	xor	ebx,ebx
	mov	bl,byte ptr[esi]	; I
	inc	esi
	shl	ebx,4

  ; We place the 16 corresponding indices:
  ;
  ;   RGB0  RGB0   RGB1  RGB1
  ;   RGB0  RGB0   RGB1  RGB1
  ;
  ;   RGB2  RGB2   RGB3  RGB3
  ;   RGB2  RGB2   RGB3  RGB3

;     eax	working RGB
;     ebx	working Ii*16
;     ecx	counting down width
;     edx	counting down height/4
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	working RGB

	mov	eax,ds:Codebook[ebx][4096]	; B0 G0 R0 alpha0
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][4096][4]	; B1 G1 R1 alpha1
	Ref	Codebook

	mov	SEG_pB:[edi],eax
	mov	SEG_pB:[edi][4],eax

	mov	SEG_pB:[edi][8],ebp
	mov	SEG_pB:[edi][12],ebp

	mov	SEG_pB:YStep[edi],eax
	Ref	YStep1

	mov	SEG_pB:YStep[edi][4],eax
	Ref	YStep1

	mov	SEG_pB:YStep[edi][8],ebp
	Ref	YStep1

	mov	SEG_pB:YStep[edi][12],ebp
	Ref	YStep1


	mov	eax,ds:Codebook[ebx][4096][8]	; B2 G2 R2 alpha2
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][4096][12]	; B3 G3 R3 alpha3
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi],eax
	Ref	YStep2

	mov	SEG_pB:(YStep*2)[edi][4],eax
	Ref	YStep2

	mov	SEG_pB:(YStep*2)[edi][8],ebp
	Ref	YStep2

	mov	SEG_pB:(YStep*2)[edi][12],ebp
	Ref	YStep2

	mov	SEG_pB:(YStep*3)[edi],eax
	Ref	YStep3

	mov	SEG_pB:(YStep*3)[edi][4],eax
	Ref	YStep3

	mov	SEG_pB:(YStep*3)[edi][8],ebp
	Ref	YStep3

	mov	SEG_pB:(YStep*3)[edi][12],ebp
	Ref	YStep3


	add	edi,16			; account for 4 pixels in width

	dec	ecx			; account for 4 pixels written
	jnz	SmoothLoop		; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SmoothEndLine:
	mov	ecx,bWidth/4		; for next swath
	Ref	Width4

	add	edi,YDelta
	Ref	YDelta

	dec	edx			; account for 4 more scanlines
	jnz	SmoothLoop		; jump if more lines to do

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawSmooth32	endp



;--------------------------------------------------------------------------
;
; void DrawInter32(
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


CVDecompEntry	DrawInter32,far

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

	mov	eax,[esi]		; I3 I2 I1 I0
	add	esi,4

  ; We place the 16 corresponding indices:
  ;
  ;   RGB0  RGB1   RGB4  RGB5
  ;   RGB2  RGB3   RGB6  RGB7
  ;
  ;   RGB8  RGB9   RGBc  RGBd
  ;   RGBa  RGBb   RGBe  RGBf

;     eax	I3 I2 I1 I0
;     ebx	working Ii*16
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	working RGB

	xor	ebx,ebx
	mov	bl,al			; I0, RGB0..3
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; B0 G0 R0 alpha0
	Ref	Codebook

	mov	SEG_pB:[edi],ebp

	mov	ebp,ds:Codebook[ebx][4]	; B1 G1 R1 alpha1
	Ref	Codebook

	mov	SEG_pB:[edi][4],ebp

	mov	ebp,ds:Codebook[ebx][8]	; B2 G2 R2 alpha2
	Ref	Codebook

	mov	SEG_pB:YStep[edi],ebp
	Ref	YStep1

	mov	ebp,ds:Codebook[ebx][12]; B3 G3 R3 alpha3
	Ref	Codebook

	mov	SEG_pB:YStep[edi][4],ebp
	Ref	YStep1


	xor	ebx,ebx
	mov	bl,ah			; I1, RGB4..7
	shl	ebx,4			; *16
	shr	eax,16			; ....i3i2

	mov	ebp,ds:Codebook[ebx]	; B4 G4 R4 alpha4
	Ref	Codebook

	mov	SEG_pB:[edi][8],ebp

	mov	ebp,ds:Codebook[ebx][4]	; B5 G5 R5 alpha5
	Ref	Codebook

	mov	SEG_pB:[edi][12],ebp

	mov	ebp,ds:Codebook[ebx][8]	; B6 G6 R6 alpha6
	Ref	Codebook

	mov	SEG_pB:YStep[edi][8],ebp
	Ref	YStep1

	mov	ebp,ds:Codebook[ebx][12]; B7 G7 R7 alpha7
	Ref	Codebook

	mov	SEG_pB:YStep[edi][12],ebp
	Ref	YStep1


	xor	ebx,ebx
	mov	bl,al			; I2, RGB8..b
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; B8 G8 R8 alpha8
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[ebx][4]	; B9 G9 R9 alpha9
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi][4],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[ebx][8]	; Ba Ga Ra alphaa
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi],ebp
	Ref	YStep3

	mov	ebp,ds:Codebook[ebx][12]; Bb Gb Rb alphab
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi][4],ebp
	Ref	YStep3


	xor	ebx,ebx
	mov	bl,ah			; I3, RGBc..f
	shl	ebx,4			; *16

	mov	ebp,ds:Codebook[ebx]	; Bc Gc Rc alphac
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi][8],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[ebx][4]	; Bd Gd Rd alphad
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi][12],ebp
	Ref	YStep2

	mov	ebp,ds:Codebook[ebx][8]	; Be Ge Re alphae
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi][8],ebp
	Ref	YStep3

	mov	ebp,ds:Codebook[ebx][12]; Bf Gf Rf alphaf
	Ref	Codebook

	mov	SEG_pB:(YStep*3)[edi][12],ebp
	Ref	YStep3


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterSkip:

	add	edi,16			; account for 4 pixels in width

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

	xor	ebx,ebx
	mov	bl,byte ptr[esi]	; I
	inc	esi
	shl	ebx,4

  ; We place the 16 corresponding indices:
  ;
  ;   RGB0  RGB0   RGB1  RGB1
  ;   RGB0  RGB0   RGB1  RGB1
  ;
  ;   RGB2  RGB2   RGB3  RGB3
  ;   RGB2  RGB2   RGB3  RGB3

;     eax	working RGB
;     ebx	working Ii*16
;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB
;     ebp	working RGB

	mov	eax,ds:Codebook[ebx][4096]	; B0 G0 R0 alpha0
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][4096][4]	; B1 G1 R1 alpha1
	Ref	Codebook

	mov	SEG_pB:[edi],eax
	mov	SEG_pB:[edi][4],eax

	mov	SEG_pB:[edi][8],ebp
	mov	SEG_pB:[edi][12],ebp

	mov	SEG_pB:YStep[edi],eax
	Ref	YStep1

	mov	SEG_pB:YStep[edi][4],eax
	Ref	YStep1

	mov	SEG_pB:YStep[edi][8],ebp
	Ref	YStep1

	mov	SEG_pB:YStep[edi][12],ebp
	Ref	YStep1


	mov	eax,ds:Codebook[ebx][4096][8]	; B2 G2 R2 alpha2
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx][4096][12]	; B3 G3 R3 alpha3
	Ref	Codebook

	mov	SEG_pB:(YStep*2)[edi],eax
	Ref	YStep2

	mov	SEG_pB:(YStep*2)[edi][4],eax
	Ref	YStep2

	mov	SEG_pB:(YStep*2)[edi][8],ebp
	Ref	YStep2

	mov	SEG_pB:(YStep*2)[edi][12],ebp
	Ref	YStep2

	mov	SEG_pB:(YStep*3)[edi],eax
	Ref	YStep3

	mov	SEG_pB:(YStep*3)[edi][4],eax
	Ref	YStep3

	mov	SEG_pB:(YStep*3)[edi][8],ebp
	Ref	YStep3

	mov	SEG_pB:(YStep*3)[edi][12],ebp
	Ref	YStep3


	add	edi,16			; account for 4 pixels in width

	dec	ecx			; account for 4 pixels written
	jnz	InterLoop		; jump if more to do

	jmp	InterEndLine		; handle end of line


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawInter32	endp


	EndMotion

	ENDSEG32


ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	Draw32

_DATA	ends

	end
