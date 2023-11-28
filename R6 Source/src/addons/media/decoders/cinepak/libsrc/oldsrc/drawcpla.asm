; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/drawcpla.asm 1.7 1995/02/22 14:44:20 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: drawcpla.asm $
; Revision 1.7  1995/02/22 14:44:20  bog
; _DATA needs to be USE32 for Win32 x86.
; Revision 1.6  1994/09/22  17:08:49  bog
; Change from Draw420 to DrawCPLA, the 4CC.
; 
; Revision 1.5  1994/07/29  15:31:23  bog
; Weitek YUV420 works.
; 
; Revision 1.4  1994/07/22  16:25:13  bog
; Integrate into standard codec.
; 
; Revision 1.3  1994/02/04  14:30:30  bog
; Multiple tiles.
; 
; Revision 1.2  1993/11/12  20:39:19  bog
; First running.
; 
; Revision 1.1  1993/11/11  23:19:02  bog
; Initial revision
; 

; Clone from codec's draw24 1 September 1993.


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

; In the case of YUV 4:2:0, detail and smooth codebooks are not
; identical.  We organize the saved codebooks so that they make best use
; of dword alignment.  Detail:

;   ------------+-----------+-----------+------------
;   |Y0|Y1|Y2|Y3|Y2|Y3|Y0|Y1| U| V|--|--|--|--|--|--|
;   ------------+-----------+-----------+------------

; Smooth:

;   ------------+-----------+-----------+------------
;   |Y0|Y0|Y1|Y1|Y2|Y2|Y3|Y3| U| U| V| V|--|--|--|--|
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

cVPatches	dd	?		; for counting down Height

;--------------------------------------------------------------------------
;
; void DrawKeyCPLA(
;   unsigned long pIx,		/* -> incoming compressed indices */
;   unsigned long oBits,	/* offset for DIB bits to fill */
;   unsigned short sBits,	/* selector for DIB bits to fill */
;   short Height		/* height of tile */
; );
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


CVDecompEntry	DrawKeyCPLA,far

	movzx	eax,Height		; for counting down Height
	shr	eax,2			; patches
	mov	cVPatches,eax
	Ref	Motion

	mov	ecx,YScan0		; -> Y (0,0) of frame
	Ref	YScan0

ifndef	WIN32
	les	edx,pB			; -> Y (0,0) of this tile
else
	mov	edx,pB			; -> Y (0,0) of this tile
endif

	mov	edi,edx
	sub	edi,ecx			; offset in Y frame to (0,0) of tile

	shr	edi,1
	sub	edx,edi

	shr	edi,1			; offset in U,V frame to (0,0) of tile

	mov	esi,YStep
	Ref	YStep1

;     edx	-> such that [edx][edi*2] -> Y (0,0) of this tile
;     esi	YStep
;     edi	offset in U,V frame to (0,0) of this tile

	mov	KeyDetailYScan0[-4],edx	; Y scanline 0 fix
	Ref	Motion

	mov	KeySmoothYScan0[-4],edx
	Ref	Motion

	add	edx,esi			; Y scanline 1

	mov	KeyDetailYScan1[-4],edx
	Ref	Motion

	mov	KeySmoothYScan1[-4],edx
	Ref	Motion

	add	edx,esi			; Y scanline 2

	mov	KeyDetailYScan2[-4],edx
	Ref	Motion

	mov	KeySmoothYScan2[-4],edx
	Ref	Motion

	add	edx,esi			; Y scanline 3

	mov	KeyDetailYScan3[-4],edx
	Ref	Motion

	mov	KeySmoothYScan3[-4],edx
	Ref	Motion

	mov	ecx,bWidth/4		; for next swath
	Ref	Width4

	dec	ecx
	shl	ecx,16

	mov	esi,pIx			; -> incoming indices

;     ecx.hi	counting down width in patches
;     esi	-> incoming indices
;     edi	indexing into outgoing YUV 4:2:0


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

;     ecx.hi	counting down width in patches
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     edi	indexing into outgoing YUV 4:2:0

	mov	eax,[esi]		; I3 I2 I1 I0
	add	esi,4

  ; We place the 16 corresponding luma:
  ;
  ;   Y00  Y01   Y10  Y11
  ;   Y02  Y03   Y12  Y13
  ;
  ;   Y20  Y21   Y30  Y31
  ;   Y22  Y23   Y32  Y33

  ; and the two sets of 4 corresponding chroma:
  ;
  ;   U0   U1
  ;   U2   U3
  ;
  ;   V0   V1
  ;   V2   U3

;     eax	I3 I2 I1 I0
;     ebx	working Ii*16
;     ecx.lo	working UV
;     ecx.hi	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     edi	indexing into outgoing YUV 4:2:0
;     ebp	working Y

	xor	ebx,ebx
	mov	bl,al			; I0
	shl	ebx,4			; *16

	mov	cx,ds:Codebook[8][ebx]	; U0 V0
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx]	; Y00 Y01 Y02 Y03
	Ref	Codebook

	xor	ebx,ebx
	mov	bl,ah			; I1
	shl	ebx,4			; *16

	mov	ax,ds:Codebook[8][ebx]	; U1 V1
	Ref	Codebook

	mov	ebx,ds:Codebook[4][ebx]	; Y12 Y13 Y10 Y11
	Ref	Codebook

	xchg	al,ch			; cx: U0 U1, ax: V0 V1

	mov	SEG_pB:VScan0[edi],ax	; V0 V1
	Ref	VScan0			; V scanline 0

	shr	eax,16			; ....I3I2

	xchg	bp,bx			; ebp: 12 13 02 03, ebx: 00 01 10 11

	mov	SEG_pB:UScan0[edi],cx	; U0 U1
	Ref	UScan0			; U scanline 0

	rol	ebp,16			; Y02 Y03 Y12 Y13

	mov	SEG_pB:[10000000h][edi*2],ebx; Y00 Y01 Y10 Y11
KeyDetailYScan0	label	dword

	mov	SEG_pB:[10000000h][edi*2],ebp; Y02 Y03 Y12 Y13
KeyDetailYScan1	label	dword


	xor	ebx,ebx
	mov	bl,al			; I2
	shl	ebx,4			; *16

	mov	cx,ds:Codebook[8][ebx]	; U2 V2
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx]	; Y20 Y21 Y22 Y23
	Ref	Codebook

	xor	ebx,ebx
	mov	bl,ah			; I3
	shl	ebx,4			; *16

	mov	ax,ds:Codebook[8][ebx]	; U3 V3
	Ref	Codebook

	mov	ebx,ds:Codebook[4][ebx]	; Y32 Y33 Y30 Y31
	Ref	Codebook

	xchg	al,ch			; cx: U2 U3, ax: V2 V3

	mov	SEG_pB:VScan1[edi],ax	; V2 V3
	Ref	VScan1

	xchg	bp,bx			; ebp: 32 33 22 23, ebx: 20 21 30 31

	mov	SEG_pB:UScan1[edi],cx	; U2 U3
	Ref	UScan1

	rol	ebp,16			; Y22 Y23 Y32 Y33

	mov	SEG_pB:[10000000h][edi*2],ebx; Y20 Y21 Y30 Y31
KeyDetailYScan2	label	dword

	mov	SEG_pB:[10000000h][edi*2],ebp; Y22 Y23 Y32 Y33
KeyDetailYScan3	label	dword


	add	edi,2			; account for 4 pixels in width

	sub	ecx,000010000h		; account for 4 pixels written
	jnc	KeyLoop			; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


KeyEndLine:
	mov	ecx,bWidth/4		; for next swath
	Ref	Width4

	lea	eax,[ecx*2]		; U width

	dec	ecx
	shl	ecx,16			; for counting down width

	add	edi,eax			; bump to next U scanline

	shl	eax,2			; 2 Y scans; 2 more in edi

	add	KeyDetailYScan0[-4],eax	; adjust for next swath
	Ref	Motion

	add	KeyDetailYScan1[-4],eax	; adjust for next swath
	Ref	Motion

	add	KeyDetailYScan2[-4],eax	; adjust for next swath
	Ref	Motion

	add	KeyDetailYScan3[-4],eax	; adjust for next swath
	Ref	Motion

	add	KeySmoothYScan0[-4],eax	; adjust for next swath
	Ref	Motion

	add	KeySmoothYScan1[-4],eax	; adjust for next swath
	Ref	Motion

	add	KeySmoothYScan2[-4],eax	; adjust for next swath
	Ref	Motion

	add	KeySmoothYScan3[-4],eax	; adjust for next swath
	Ref	Motion

	dec	cVPatches		; account for 4 more scanlines
	Ref	Motion
	jnz	KeyLoop			; jump if more lines to do

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


KeySmooth:

;     ecx.hi	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     edi	-> outgoing DIB

	xor	ebx,ebx
	mov	bl,[esi]		; I
	inc	esi
	shl	ebx,4

  ; We place the 16 corresponding luma:
  ;
  ;   Y0  Y0   Y1  Y1
  ;   Y0  Y0   Y1  Y1
  ;
  ;   Y2  Y2   Y3  Y3
  ;   Y2  Y2   Y3  Y3

  ; and the two sets of 4 corresponding chroma:
  ;
  ;   U   U
  ;   U   U
  ;
  ;   V   V
  ;   V   V

;     eax	working Y	
;     ebx	working I*16
;     ecx.hi	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     edi	indexing into outgoing YUV 4:2:0
;     ebp	working UV

	mov	eax,ds:Codebook[4096][ebx]; Y0 Y0 Y1 Y1
	Ref	Codebook

	mov	ebp,ds:Codebook[4096][8][ebx]; U U V V
	Ref	Codebook

	mov	SEG_pB:[10000000h][edi*2],eax; Y0 Y0 Y1 Y1
KeySmoothYScan0	label	dword

	mov	SEG_pB:[10000000h][edi*2],eax; Y0 Y0 Y1 Y1
KeySmoothYScan1	label	dword

	mov	eax,ds:Codebook[4096][4][ebx]; Y2 Y2 Y3 Y3
	Ref	Codebook

	mov	SEG_pB:UScan0[edi],bp	; U U
	Ref	UScan0

	mov	SEG_pB:UScan1[edi],bp	; U U
	Ref	UScan1

	rol	ebp,16			; V V U U

	mov	SEG_pB:[10000000h][edi*2],eax; Y2 Y2 Y3 Y3
KeySmoothYScan2	label	dword

	mov	SEG_pB:[10000000h][edi*2],eax; Y2 Y2 Y3 Y3
KeySmoothYScan3	label	dword

	mov	SEG_pB:VScan0[edi],bp	; V V
	Ref	VScan0

	mov	SEG_pB:VScan1[edi],bp; V V
	Ref	VScan1


	add	edi,2			; account for 4 pixels in width

	sub	ecx,000010000h		; account for 4 pixels written
	jnc	KeyLoop			; jump if more to do

	jmp	KeyEndLine		; handle end of line


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawKeyCPLA	endp



;--------------------------------------------------------------------------
;
; void DrawSmoothCPLA(
;   unsigned long pIx,		/* -> incoming compressed indices */
;   unsigned long oBits,	/* offset for DIB bits to fill */
;   unsigned short sBits,	/* selector for DIB bits to fill */
;   short Height		/* height of tile */
; );
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


CVDecompEntry	DrawSmoothCPLA,far

	mov	ecx,YScan0		; -> Y (0,0) of frame
	Ref	YScan0

ifndef	WIN32
	les	edx,pB			; -> Y (0,0) of this tile
else
	mov	edx,pB			; -> Y (0,0) of this tile
endif

	mov	edi,edx
	sub	edi,ecx			; offset in Y frame to (0,0) of tile

	shr	edi,1
	sub	edx,edi

	shr	edi,1			; offset in U,V frame to (0,0) of tile

	mov	esi,YStep
	Ref	YStep1

;     edx	-> such that [edx][edi*2] -> Y (0,0) of this tile
;     esi	YStep
;     edi	offset in U,V frame to (0,0) of this tile

	mov	SmoothYScan0[-4],edx	; Y scanline 0
	Ref	Motion

	add	edx,esi			; Y scanline 1

	mov	SmoothYScan1[-4],edx
	Ref	Motion

	add	edx,esi			; Y scanline 2

	mov	SmoothYScan2[-4],edx
	Ref	Motion

	add	edx,esi			; Y scanline 3

	mov	SmoothYScan3[-4],edx
	Ref	Motion

	mov	ecx,bWidth/4
	Ref	Width4

	movzx	edx,Height		; for counting down Height
	shr	edx,2			; patches

	mov	esi,pIx			; -> incoming indices

;     ecx	counting down width in patches
;     edx	counting down height in patches
;     esi	-> incoming indices
;     edi	indexing into outgoing YUV 4:2:0


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SmoothLoop:

	xor	ebx,ebx
	mov	bl,[esi]		; I
	inc	esi
	shl	ebx,4

  ; We place the 16 corresponding luma:
  ;
  ;   Y0  Y0   Y1  Y1
  ;   Y0  Y0   Y1  Y1
  ;
  ;   Y2  Y2   Y3  Y3
  ;   Y2  Y2   Y3  Y3

  ; and the two sets of 4 corresponding chroma:
  ;
  ;   U   U
  ;   U   U
  ;
  ;   V   V
  ;   V   V

;     eax	working Y	
;     ebx	working I*16
;     ecx	counting down width in patches
;     edx	counting down height in patches
;     esi	-> incoming indices
;     edi	indexing into outgoing YUV 4:2:0
;     ebp	working UV

	mov	eax,ds:Codebook[4096][ebx]; Y0 Y0 Y1 Y1
	Ref	Codebook

	mov	ebp,ds:Codebook[4096][8][ebx]; U U V V
	Ref	Codebook

	mov	SEG_pB:[10000000h][edi*2],eax; Y0 Y0 Y1 Y1
SmoothYScan0	label	dword

	mov	SEG_pB:[10000000h][edi*2],eax; Y0 Y0 Y1 Y1
SmoothYScan1	label	dword

	mov	eax,ds:Codebook[4096][4][ebx]; Y2 Y2 Y3 Y3
	Ref	Codebook

	mov	SEG_pB:UScan0[edi],bp	; U U
	Ref	UScan0

	mov	SEG_pB:UScan1[edi],bp; U U
	Ref	UScan1

	rol	ebp,16			; V V U U

	mov	SEG_pB:[10000000h][edi*2],eax; Y2 Y2 Y3 Y3
SmoothYScan2	label	dword

	mov	SEG_pB:[10000000h][edi*2],eax; Y2 Y2 Y3 Y3
SmoothYScan3	label	dword

	mov	SEG_pB:VScan0[edi],bp	; V V
	Ref	VScan0

	mov	SEG_pB:VScan1[edi],bp; V V
	Ref	VScan1


	add	edi,2			; account for 4 pixels in width

	dec	ecx			; account for 4 pixels written
	jnz	SmoothLoop		; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SmoothEndLine:
	mov	ecx,bWidth/4
	Ref	Width4

	lea	eax,[ecx*2]		; U width

	add	edi,eax			; bump to next U scanline

	shl	eax,2			; 2 Y scans; 2 more in edi

	add	SmoothYScan0[-4],eax	; adjust for next swath
	Ref	Motion

	add	SmoothYScan1[-4],eax	; adjust for next swath
	Ref	Motion

	add	SmoothYScan2[-4],eax	; adjust for next swath
	Ref	Motion

	add	SmoothYScan3[-4],eax	; adjust for next swath
	Ref	Motion

	dec	edx			; account for 4 more scanlines
	jnz	SmoothLoop		; jump if more lines to do

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawSmoothCPLA	endp



;--------------------------------------------------------------------------
;
; void DrawInterCPLA(
;   unsigned long pIx,		/* -> incoming compressed indices */
;   unsigned long oBits,	/* offset for DIB bits to fill */
;   unsigned short sBits,	/* selector for DIB bits to fill */
;   short Height		/* height of tile */
; );
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


CVDecompEntry	DrawInterCPLA,far

	movzx	eax,Height		; for counting down Height
	shr	eax,2			; patches
	mov	cVPatches,eax
	Ref	Motion

	mov	ecx,YScan0		; -> Y (0,0) of frame
	Ref	YScan0

ifndef	WIN32
	les	edx,pB			; -> Y (0,0) of this tile
else
	mov	edx,pB			; -> Y (0,0) of this tile
endif

	mov	edi,edx
	sub	edi,ecx			; offset in Y frame to (0,0) of tile

	shr	edi,1
	sub	edx,edi

	shr	edi,1			; offset in U,V frame to (0,0) of tile

	mov	esi,YStep
	Ref	YStep1

;     edx	-> such that [edx][edi*2] -> Y (0,0) of this tile
;     esi	YStep
;     edi	offset in U,V frame to (0,0) of this tile

	mov	InterDetailYScan0[-4],edx; Y scanline 0 fix
	Ref	Motion

	mov	InterSmoothYScan0[-4],edx
	Ref	Motion

	add	edx,esi			; Y scanline 1

	mov	InterDetailYScan1[-4],edx
	Ref	Motion

	mov	InterSmoothYScan1[-4],edx
	Ref	Motion

	add	edx,esi			; Y scanline 2

	mov	InterDetailYScan2[-4],edx
	Ref	Motion

	mov	InterSmoothYScan2[-4],edx
	Ref	Motion

	add	edx,esi			; Y scanline 3

	mov	InterDetailYScan3[-4],edx
	Ref	Motion

	mov	InterSmoothYScan3[-4],edx
	Ref	Motion

	mov	ecx,bWidth/4		; for next swath
	Ref	Width4

	dec	ecx
	shl	ecx,16

	mov	esi,pIx			; -> incoming indices

;     ecx.hi	counting down width in patches
;     esi	-> incoming indices
;     edi	indexing into outgoing YUV 4:2:0


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

;     ecx.hi	counting down width in patches
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     edi	indexing into outgoing YUV 4:2:0

	mov	eax,[esi]		; I3 I2 I1 I0
	add	esi,4

  ; We place the 16 corresponding luma:
  ;
  ;   Y00  Y01   Y10  Y11
  ;   Y02  Y03   Y12  Y13
  ;
  ;   Y20  Y21   Y30  Y31
  ;   Y22  Y23   Y32  Y33

  ; and the two sets of 4 corresponding chroma:
  ;
  ;   U0   U1
  ;   U2   U3
  ;
  ;   V0   V1
  ;   V2   U3

;     eax	I3 I2 I1 I0
;     ebx	working Ii*16
;     ecx.lo	working UV
;     ecx.hi	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     edi	indexing into outgoing YUV 4:2:0
;     ebp	working Y

	xor	ebx,ebx
	mov	bl,al			; I0
	shl	ebx,4			; *16

	mov	cx,ds:Codebook[8][ebx]	; U0 V0
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx]	; Y00 Y01 Y02 Y03
	Ref	Codebook

	xor	ebx,ebx
	mov	bl,ah			; I1
	shl	ebx,4			; *16

	mov	ax,ds:Codebook[8][ebx]	; U1 V1
	Ref	Codebook

	mov	ebx,ds:Codebook[4][ebx]	; Y12 Y13 Y10 Y11
	Ref	Codebook

	xchg	al,ch			; cx: U0 U1, ax: V0 V1

	mov	SEG_pB:VScan0[edi],ax	; V0 V1
	Ref	VScan0			; V scanline 0

	shr	eax,16			; ....I3I2

	xchg	bp,bx			; ebp: 12 13 02 03, ebx: 00 01 10 11

	mov	SEG_pB:UScan0[edi],cx	; U0 U1
	Ref	UScan0			; U scanline 0

	rol	ebp,16			; Y02 Y03 Y12 Y13

	mov	SEG_pB:[10000000h][edi*2],ebx; Y00 Y01 Y10 Y11
InterDetailYScan0	label	dword

	mov	SEG_pB:[10000000h][edi*2],ebp; Y02 Y03 Y12 Y13
InterDetailYScan1	label	dword


	xor	ebx,ebx
	mov	bl,al			; I2
	shl	ebx,4			; *16

	mov	cx,ds:Codebook[8][ebx]	; U2 V2
	Ref	Codebook

	mov	ebp,ds:Codebook[ebx]	; Y20 Y21 Y22 Y23
	Ref	Codebook

	xor	ebx,ebx
	mov	bl,ah			; I3
	shl	ebx,4			; *16

	mov	ax,ds:Codebook[8][ebx]	; U3 V3
	Ref	Codebook

	mov	ebx,ds:Codebook[4][ebx]	; Y32 Y33 Y30 Y31
	Ref	Codebook

	xchg	al,ch			; cx: U2 U3, ax: V2 V3

	mov	SEG_pB:VScan1[edi],ax	; V2 V3
	Ref	VScan1

	xchg	bp,bx			; ebp: 32 33 22 23, ebx: 20 21 30 31

	mov	SEG_pB:UScan1[edi],cx	; U2 U3
	Ref	UScan1

	rol	ebp,16			; Y22 Y23 Y32 Y33

	mov	SEG_pB:[10000000h][edi*2],ebx; Y20 Y21 Y30 Y31
InterDetailYScan2	label	dword

	mov	SEG_pB:[10000000h][edi*2],ebp; Y22 Y23 Y32 Y33
InterDetailYScan3	label	dword


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterSkip:
	add	edi,2			; account for 4 pixels in width

	sub	ecx,000010000h		; account for 4 pixels written
	jnc	InterLoop			; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterEndLine:
	mov	ecx,bWidth/4		; for next swath
	Ref	Width4

	lea	eax,[ecx*2]		; U width

	dec	ecx
	shl	ecx,16

	add	edi,eax			; bump to next U scanline

	shl	eax,2			; 2 Y scans; 2 more in edi

	add	InterDetailYScan0[-4],eax; adjust for next swath
	Ref	Motion

	add	InterDetailYScan1[-4],eax; adjust for next swath
	Ref	Motion

	add	InterDetailYScan2[-4],eax; adjust for next swath
	Ref	Motion

	add	InterDetailYScan3[-4],eax; adjust for next swath
	Ref	Motion

	add	InterSmoothYScan0[-4],eax; adjust for next swath
	Ref	Motion

	add	InterSmoothYScan1[-4],eax; adjust for next swath
	Ref	Motion

	add	InterSmoothYScan2[-4],eax; adjust for next swath
	Ref	Motion

	add	InterSmoothYScan3[-4],eax; adjust for next swath
	Ref	Motion

	dec	cVPatches		; account for 4 more scanlines
	Ref	Motion
	jnz	InterLoop		; jump if more lines to do

	CVDecompExit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterSmooth:

;     ecx.hi	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     edi	-> outgoing DIB

	xor	ebx,ebx
	mov	bl,[esi]		; I
	inc	esi
	shl	ebx,4

  ; We place the 16 corresponding luma:
  ;
  ;   Y0  Y0   Y1  Y1
  ;   Y0  Y0   Y1  Y1
  ;
  ;   Y2  Y2   Y3  Y3
  ;   Y2  Y2   Y3  Y3

  ; and the two sets of 4 corresponding chroma:
  ;
  ;   U   U
  ;   U   U
  ;
  ;   V   V
  ;   V   V

;     eax	working Y	
;     ebx	working I*16
;     ecx.hi	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     edi	indexing into outgoing YUV 4:2:0
;     ebp	working UV

	mov	eax,ds:Codebook[4096][ebx]; Y0 Y0 Y1 Y1
	Ref	Codebook

	mov	ebp,ds:Codebook[4096][8][ebx]; U U V V
	Ref	Codebook

	mov	SEG_pB:[10000000h][edi*2],eax; Y0 Y0 Y1 Y1
InterSmoothYScan0	label	dword

	mov	SEG_pB:[10000000h][edi*2],eax; Y0 Y0 Y1 Y1
InterSmoothYScan1	label	dword

	mov	eax,ds:Codebook[4096][4][ebx]; Y2 Y2 Y3 Y3
	Ref	Codebook

	mov	SEG_pB:UScan0[edi],bp	; U U
	Ref	UScan0

	mov	SEG_pB:UScan1[edi],bp	; U U
	Ref	UScan1

	rol	ebp,16			; V V U U

	mov	SEG_pB:[10000000h][edi*2],eax; Y2 Y2 Y3 Y3
InterSmoothYScan2	label	dword

	mov	SEG_pB:[10000000h][edi*2],eax; Y2 Y2 Y3 Y3
InterSmoothYScan3	label	dword

	mov	SEG_pB:VScan0[edi],bp	; V V
	Ref	VScan0

	mov	SEG_pB:VScan1[edi],bp	; V V
	Ref	VScan1


	add	edi,2			; account for 4 pixels in width

	sub	ecx,000010000h		; account for 4 pixels written
	jnc	InterLoop		; jump if more to do

	jmp	InterEndLine		; handle end of line


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_DrawInterCPLA	endp


	EndMotion

	ENDSEG32


ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	DrawCPLA

_DATA	ends

	end
