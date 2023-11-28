SECTION .text

%include "cv.i"

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

; In the case of 16 bit DIBs, detail and smooth codebooks are not
; identical.  We organize the saved codebooks so that they make best use
; of dword alignment.  Detail:

;   ------------+-----------+-----------+------------
;   |RGB0 |RGB1 |RGB2 |RGB3 |--|--|--|--|--|--|--|--|
;   ------------+-----------+-----------+------------

; Smooth:

;   ------------+-----------+-----------+------------
;   |RGB0 |RGB0 |RGB1 |RGB1 |RGB2 |RGB2 |RGB3 |RGB3 |
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

EXTERN 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; void DrawKey15(
;   unsigned long pD,		// -> DCONTEXT context
;   unsigned long pIx,		/* -> incoming compressed indices */
;   unsigned long oBits,	/* offset for DIB bits to fill */
;   short Height			/* height (pixels) of tile */
; );
GLOBAL DrawKey15
ALIGN 16
DrawKey15:
		push ebp
		mov ebp,esp
		push ebx
		push esi
		push edi
		
		sub esp,16

		mov	eax,[esp+16+16+16]		; for counting down Height
		shr eax,2					; patched
		mov [esp+12],eax
		mov esi,[esp+16+16+4]		; pD

		mov ecx,[esi+$34]			; pThisCodeBook
		mov [Codebooka0],ecx
		mov [Codebookb0],ecx
		mov	[Codebookc0],ecx
		mov [Codebookd0],ecx
		
		add ecx,$4
		mov [Codebooka4],ecx
		mov [Codebookb4],ecx
		mov [Codebookc4],ecx
		mov [Codebookd4],ecx
		
		add ecx,$8

		add ecx,01000h			; CodeBook+4096+12
		mov	[Codebook4096_12],ecx
		sub ecx,$4
		mov [Codebook4096_8],ecx
		sub ecx,$4
		mov [Codebook4096_4],ecx
		sub ecx,$4
		mov [Codebook4096_0],ecx

;	long yStep1 = pDC->YStep;
;	long yStep2 = pDC->YStep * 2;
;	long yStep3 = pDC->YStep * 3;
;	long yStep4 = pDC->YStep * 4;
		mov ecx,[esi+$38]			; YStep
		mov edx,ecx
		mov [YStep1a],ecx
		mov [YStep1e],ecx
		add ecx,$4
		mov [YStep1b],ecx
		mov [YStep1f],ecx

		add ecx,edx					; YStep*2
		mov [YStep2b],ecx
		mov [YStep2f],ecx
		sub ecx,$4
		mov [YStep2a],ecx
		mov [YStep2e],ecx

		add ecx,edx					; YStep*3
		mov [YStep3a],ecx
		mov [YStep3e],ecx
		add ecx,$4
		mov [YStep3b],ecx
		mov [YStep3f],ecx

; YDelta= yStep4 - pDC->Width * 2;
		add ecx,edx					; YStep*4
		sub ecx,04h
		mov edx,$0
		mov	dx,[esi+$28]			; pDC->Width
		shl edx,$1					; Width*2
		sub ecx,edx					; YStep*4-Width*2
		mov [YDelta],ecx

		mov ecx,$0
		mov	cx, word [esi+$28]		; bWidth
		shr cx,$2					; bWidth/4
		mov [bWidth4],ecx			; store in our loop

		mov	esi,[esp+16+16+8]		; -> incoming indices
		mov	edi,[esp+16+16+12]		; -> outgoing DIB

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

		jnc	near KeySmooth		; jump if smooth
		
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
		
		mov	ebp,[012345678h+ebx]	; RGB1:RGB0
Codebooka0	equ	$-4
		mov	[edi],ebp
		
		mov	ebp,[012345678h+ebx]	; RGB3:RGB2
Codebooka4	equ $-4					; Codebook+4
		mov	[012345678+edi],ebp
YStep1a	equ	$-4
		
		
		xor	ebx,ebx
		mov	bl,ah			; I1, RGB4..7
		shl	ebx,4			; *16
		shr	eax,16			; ....i3i2
		
		mov	ebp,[012345678h+ebx]	; RGB5:RGB4
Codebookb0 equ $-4					; Codebook+0
		mov	[edi+4],ebp
		
		mov	ebp,[012345678h+ebx]	; RGB7:RGB6
Codebookb4 equ $-4
		mov	[012345678h+edi],ebp
YStep1b equ $-4
		
		
		xor	ebx,ebx
		mov	bl,al			; I2, RGB8..b
		shl	ebx,4			; *16
		
		mov	ebp,[012345678h+ebx]	; RGB9:RGB8
Codebookc0 equ $-4
		mov	[012345678h+edi],ebp
YStep2a equ $-4
	
		mov	ebp,[012345678h+ebx]	; RGBb:RGBa
Codebookc4 equ $-4
		mov	[012345678h+edi],ebp
YStep3a equ $-4
		
		
		xor	ebx,ebx
		mov	bl,ah			; I3, RGBc..f
		shl	ebx,4			; *16
		
		mov	ebp,[012345678h+ebx]	; RGBd:RGBc
Codebookd0 equ $-4
		mov	[012345678h+edi],ebp
YStep2b equ $-4
		
		mov	ebp,[012345678h+ebx]	; Be Ge Re alphae
Codebookd4 equ $-4
		mov	[012345678h+edi],ebp
YStep3b equ $-4
		
		
		add	edi,8			; account for 4 pixels in width
		
		dec	ecx			; account for 4 pixels written
		jnz	near KeyLoop			; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


KeyEndLine:
		mov	ecx,012345678h		; iWidth4 -> for next swath
bWidth4 equ $-4
		
		add	edi,012345678h
YDelta equ $-4
		
		dec	word [esp+12]		; account for 4 more scanlines
		jnz	near KeyLoop			; jump if more lines to do

		add esp,16
		
		pop edi
		pop esi
		pop ebx
		pop ebp
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


KeySmooth:

		;     ecx	counting down width
		;     edx	detail/smooth switches
		;     esi	-> incoming indices
		;     (es):edi	-> outgoing DIB
		
		xor	ebx,ebx
		mov	bl,byte [esi]	; I
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
		
		mov	eax,[012345678h+ebx]	; RGB0:RGB0
Codebook4096_0 equ $-4
		mov	[edi],eax
		mov	[012345678h+edi],eax
YStep1e equ $-4
		
		mov	eax,[012345678h+ebx]	; RGB1:RGB1
Codebook4096_4 equ $-4
		mov	[edi+4],eax
		mov	[012345678h+edi],eax
YStep1f equ $-4
		
		mov	eax,[012345678h+ebx]	; RGB2:RGB2
Codebook4096_8 equ $-4
		mov	[012345678h+edi],eax
YStep2e equ $-4
		mov	[012345678h+edi],eax
YStep3e equ $-4
		
		mov	eax,[012345678h+ebx]	; RGB3:RGB3
Codebook4096_12 equ $-4
		mov	[012345678h+edi],eax
YStep2f equ $-4
		mov	[012345678h+edi],eax
YStep3f equ $-4

		add	edi,8			; account for 4 pixels in width
		
		dec	ecx			; account for 4 pixels written
		jnz	near KeyLoop			; jump if more to do
		
		jmp	KeyEndLine		; handle end of line

GLOBAL DrawSmooth15:
ALIGN 16
DrawSmooth15:
		push ebp
		mov ebp,esp
		push ebx
		push esi
		push edi

		sub esp,16

		mov esi,[esp+16+16+4]		; pD

		mov ecx,[esi+$34]			; pThisCodeBook
		add ecx,01000h				; CodeBook+4096
		mov	[CodeBook4096_0],ecx
		add ecx,$4
		mov [CodeBook4096_4],ecx
		add ecx,$4
		mov [CodeBook4096_8],ecx
		add ecx,$4
		mov [CodeBook4096_12],ecx

		mov ecx,[esi+$38]			; YStep
		mov edx,ecx
		mov [Ystep1a],ecx
		add ecx,$4
		mov [Ystep1b],ecx

		add ecx,edx					; YStep*2
		mov [Ystep2b],ecx
		sub ecx,$4
		mov [Ystep2a],ecx

		add ecx,edx					; YStep*3
		mov [Ystep3a],ecx
		add ecx,$4
		mov [Ystep3b],ecx

; YDelta= yStep4 - pDC->Width * 2;
		add ecx,edx					; YStep*4
		sub ecx,04h
		mov edx,$0
		mov	dx,[esi+$28]			; pDC->Width
		shl edx,$1					; Width*2
		sub ecx,edx					; YStep*4-Width*2
		mov [Ydelta],ecx

		mov ecx,$0
		mov	cx, word [esi+$28]		; bWidth
		shr cx,$2					; bWidth/4
		mov [bWidth4b],ecx			; store in our loop

		mov	edx,[esp+16+16+16]		; for counting down Height
		shr	edx,2			; patches
		
		mov	esi,[esp+16+16+8]		; -> incoming indices
		mov	edi,[esp+16+16+12]		; -> outgoing DIB

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
		mov	bl,byte [esi]	; I
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

		mov	eax,[012345678h+ebx]	; RGB0:RGB0
CodeBook4096_0 equ $-4
		mov	[edi],eax
		mov	[edi+012345678h],eax
Ystep1a equ $-4
		
		mov	eax,[012345678h+ebx]	; RGB1:RGB1
CodeBook4096_4 equ $-4
		mov	[edi+4],eax
		mov	[edi+012345678h],eax
Ystep1b equ $-4

		mov	eax,[012345678h+ebx]	; RGB2:RGB2
CodeBook4096_8 equ $-4
		mov	[edi+012345678h],eax
Ystep2a equ $-4
		mov	[edi+012345678h],eax
Ystep3a equ $-4
		
		mov	eax,[012345678h+ebx]	; RGB3:RGB3
CodeBook4096_12 equ $-4
		mov	[edi+012345678h],eax
Ystep2b equ $-4
		mov	[edi+012345678h],eax
Ystep3b equ $-4
		
		
		add	edi,8			; account for 4 pixels in width
		
		dec	ecx			; account for 4 pixels written
		jnz	SmoothLoop		; jump if more to do


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


SmoothEndLine:
		mov	ecx,012345678h		; for next swath
bWidth4b equ $-4
		
		add	edi,012345678h
Ydelta equ $-4
	
		dec	edx			; account for 4 more scanlines
		jnz	near SmoothLoop		; jump if more lines to do
		
		add esp,16

		pop edi
		pop esi
		pop ebx
		pop ebp
		ret

GLOBAL DrawInter15:
ALIGN 16
DrawInter15:
		push ebp
		mov ebp,esp
		push ebx
		push esi
		push edi

		sub esp,16

		mov	eax,[esp+16+16+16]		; for counting down Height
		shr	eax,2			; patches
		mov [esp+12],eax
		mov esi,[esp+16+16+4]		; pD
		
		mov ecx,[esi+$34]			; pThisCodeBook
		mov [codebooka0],ecx
		mov [codebookb0],ecx
		mov	[codebookc0],ecx
		mov [codebookd0],ecx
		
		add ecx,$4
		mov [codebooka4],ecx
		mov [codebookb4],ecx
		mov [codebookc4],ecx
		mov [codebookd4],ecx
		
		add ecx,$8

		add ecx,01000h			; CodeBook+4096+12
		mov	[codebook4096_12],ecx
		sub ecx,$4
		mov [codebook4096_8],ecx
		sub ecx,$4
		mov [codebook4096_4],ecx
		sub ecx,$4
		mov [codebook4096_0],ecx

;	long yStep1 = pDC->YStep;
;	long yStep2 = pDC->YStep * 2;
;	long yStep3 = pDC->YStep * 3;
;	long yStep4 = pDC->YStep * 4;
		mov ecx,[esi+$38]			; YStep
		mov edx,ecx
		mov [ystep1a],ecx
		mov [ystep1e],ecx
		add ecx,$4
		mov [ystep1b],ecx
		mov [ystep1f],ecx

		add ecx,edx					; YStep*2
		mov [ystep2b],ecx
		mov [ystep2f],ecx
		sub ecx,$4
		mov [ystep2a],ecx
		mov [ystep2e],ecx

		add ecx,edx					; YStep*3
		mov [ystep3a],ecx
		mov [ystep3e],ecx
		add ecx,$4
		mov [ystep3b],ecx
		mov [ystep3f],ecx

; YDelta= yStep4 - pDC->Width * 2;
		add ecx,edx					; YStep*4
		sub ecx,04h
		mov edx,$0
		mov	dx,[esi+$28]			; pDC->Width
		shl edx,$1					; Width*2
		sub ecx,edx					; YStep*4-Width*2
		mov [ydelta],ecx

		mov ecx,$0
		mov	cx, word [esi+$28]		; bWidth
		shr cx,$2					; bWidth/4
		mov [bwidth4],ecx			; store in our loop

		mov	esi,[esp+16+16+8]		; -> incoming indices
		mov	edi,[esp+16+16+12]		; -> outgoing DIB
		
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

		jnc	near InterSkip		; jump if skip patch
		
		jz	near InterNewDoSwitch	; jump if need new switch


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


		add	edx,edx			; detail/smooth to carry

InterDetailTest:

		jnc	near InterSmooth		; jump if smooth
		
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
		
		mov	ebp,[012345678h+ebx]	; B0 G0 R0 alpha0
codebooka0	equ	$-4
		mov	[edi],ebp

		mov	ebp,[012345678h+ebx]	; B1 G1 R1 alpha1
codebooka4	equ $-4					; Codebook+4
		mov	[012345678+edi],ebp
ystep1a	equ	$-4

		xor	ebx,ebx
		mov	bl,ah			; I1, RGB4..7
		shl	ebx,4			; *16
		shr	eax,16			; ....i3i2
		
		mov	ebp,[012345678h+ebx]	; B4 G4 R4 alpha4
codebookb0 equ $-4					; Codebook+0
		mov	[edi+4],ebp
		
		mov	ebp,[012345678h+ebx]	; B5 G5 R5 alpha5
codebookb4 equ $-4
		mov	[012345678h+edi],ebp
ystep1b equ $-4
		
		
		xor	ebx,ebx
		mov	bl,al			; I2, RGB8..b
		shl	ebx,4			; *16
		
		mov	ebp,[012345678h+ebx]	; RGB9:RGB8
codebookc0 equ $-4
		mov	[012345678h+edi],ebp
ystep2a equ $-4
	
		mov	ebp,[012345678h+ebx]	; B9 G9 R9 alpha9
codebookc4 equ $-4
		mov	[012345678h+edi],ebp
ystep3a equ $-4

		xor	ebx,ebx
		mov	bl,ah			; I3, RGBc..f
		shl	ebx,4			; *16
		
		mov	ebp,[012345678h+ebx]	; Bc Gc Rc alphac
codebookd0 equ $-4
		mov	[012345678h+edi],ebp
ystep2b equ $-4
		
		mov	ebp,[012345678h+ebx]	; Bd Gd Rd alphad
codebookd4 equ $-4
		mov	[012345678h+edi],ebp
ystep3b equ $-4
		
		
		
		
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterSkip:
		
		add	edi,8			; account for 4 pixels in width
		
		dec	ecx			; account for 4 pixels written
		jnz	near InterLoop		; jump if more to do
		
		
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterEndLine:
		mov	ecx,012345678h		; iWidth4 -> for next swath
bwidth4 equ $-4
		
		add	edi,012345678h
ydelta equ $-4
		
		dec	word [esp+12]		; account for 4 more scanlines
		jnz	near InterLoop		; jump if more lines to do

		add esp,16

		pop edi
		pop esi
		pop ebx
		pop ebp
		ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


InterSmooth:

;     ecx	counting down width
;     edx	detail/smooth switches
;     esi	-> incoming indices
;     (es:)edi	-> outgoing DIB

	xor	ebx,ebx
	mov	bl,byte [esi]	; I
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

		mov	eax,[012345678h+ebx]	; B0 G0 R0 alpha0
codebook4096_0 equ $-4
		mov	[edi],eax
		mov	[012345678h+edi],eax
ystep1e equ $-4
		
		mov	eax,[012345678h+ebx]	; B1 G1 R1 alpha1
codebook4096_4 equ $-4
		mov	[edi+4],eax
		mov	[012345678h+edi],eax
ystep1f equ $-4

		
		mov	eax,[012345678h+ebx]	; B2 G2 R2 alpha2
codebook4096_8 equ $-4
		mov	[012345678h+edi],eax
ystep2e equ $-4
		mov	[012345678h+edi],eax
ystep3e equ $-4
		
		mov	eax,[012345678h+ebx]	; B3 G3 R3 alpha3
codebook4096_12 equ $-4
		mov	[012345678h+edi],eax
ystep2f equ $-4
		mov	[012345678h+edi],eax
ystep3f equ $-4

		add	edi,8			; account for 4 pixels in width
		
		dec	ecx					; account for 4 pixels written
		jnz	near InterLoop		; jump if more to do
		
		jmp	InterEndLine		; handle end of line
			