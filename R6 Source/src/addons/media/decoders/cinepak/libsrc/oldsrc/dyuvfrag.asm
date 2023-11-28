; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/dyuvfrag.asm 2.9 1994/06/23 14:12:51 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: dyuvfrag.asm $
; Revision 2.9  1994/06/23 14:12:51  bog
; Change movzx into xor/mov pair.
; Revision 2.8  1993/10/12  17:25:04  bog
; RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
; 
; Revision 2.7  1993/08/04  19:07:42  timr
; Both compressor and decompressor now work on NT.
; 
; Revision 2.6  1993/07/27  09:32:52  timr
; Finally assembles under NT MASM386.
; 
; Revision 2.5  1993/07/24  11:10:22  bog
; Crashed high half of ebx.
; 
; Revision 2.4  1993/07/09  22:33:42  timr
; 2nd pass at Win32-izing.
; 
; Revision 2.3  1993/07/06  09:11:31  geoffs
; 1st pass WIN32'izing
; 
; Revision 2.2  93/07/03  11:43:25  geoffs
; All _DATA segment declarations needed USE16 if WIN16 build
; 
; Revision 2.1  93/06/10  09:18:39  geoffs
; Add 32 bit DIB support
; 
; Revision 2.0  93/06/01  14:14:37  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.18  93/04/21  15:48:27  bog
; Fix up copyright and disclaimer.
; 
; Revision 1.17  93/02/18  09:21:19  geoffs
; Corrected some of the -> checking code
; 
; Revision 1.16  93/02/16  14:48:35  geoffs
; Added recreate of smooth vectors from detail vectors
; 
; Revision 1.15  93/01/27  13:14:05  geoffs
; Added more pointer checking code, fixed up gpf in 2nd pass vertical filter
; 
; Revision 1.14  93/01/27  08:00:59  geoffs
; Added debug check for pointer ranges; added fix so that last input scan + 1 not processed
; 
; Revision 1.13  93/01/26  10:50:36  geoffs
; Fixed up non-0mod4 width,height
; 
; Revision 1.12  93/01/25  21:46:45  geoffs
; Non 0 mod 4 input.
; 
; Revision 1.11  93/01/25  14:25:24  geoffs
; Allow non 0 mod 4 frames.
; 
; Revision 1.10  93/01/25  09:33:24  geoffs
; Add DEBUG code at head of code to check for USE32/USE16
; 
; Revision 1.9  93/01/21  15:50:06  geoffs
; Fixed bug in 8 bpp fetch
; 
; Revision 1.8  93/01/16  16:53:31  geoffs
; First pass filtering had faulty 134 filtering
; 
; Revision 1.7  93/01/16  16:14:00  geoffs
; Rearranged the Y,U,V outputs to VECTOR format rather than the previous raster format
; 
; Revision 1.6  93/01/13  17:20:55  geoffs
; Changed so now flatSel is fetch from DGROUP
; 
; Revision 1.5  93/01/13  10:36:23  geoffs
; The detail and smooth lists appear to be outputting consistent data now
; 
; Revision 1.4  93/01/12  17:16:04  geoffs
; TILEs now have VECTORs instead of YYYYUVs.
; 
; Revision 1.3  93/01/11  18:44:57  geoffs
; Have all the fragment code in now, but not yet fully debugged
; 
; Revision 1.2  93/01/11  09:40:46  geoffs
; Don't get impatient -- we're almost there. Have only the 2nd vertical
; filtering pass on UV to go.
; 
; Revision 1.1  93/01/10  17:06:56  geoffs
; Initial revision
; 

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ifndef	WIN32
	.model	small,c
	.386
else
	.386
	.model	small,c
endif

	include	cv.inc
	include	dyuvfrag.inc

; -------------------------------------------------------
;               DATA SEGMENT DECLARATIONS
; -------------------------------------------------------

ifndef	WIN32
	.data
	extrn	flatSel:WORD
endif

ifndef	WIN32
_FRAGTEXT32	segment	para	public	USE32	'CODE'
        assume	cs:_FRAGTEXT32
        assume	ds:DGROUP
        assume	es:nothing
	assume	fs:nothing
	assume	gs:nothing
else
	.code
endif

;DIBTOYUVPRIVATE		struc
;--------------------------------------------------------------------------
; code at head of segment used to get segment loaded
;
; proc name also used as reference for fixups in code
;--------------------------------------------------------------------------

	public	baseFRAGTEXT32
ifndef	WIN32
baseFRAGTEXT32	proc	far
else
baseFRAGTEXT32	proc
endif
	ret				; we are called in USE16 mode
	nop
	nop
	nop				; for dword padding
baseFRAGTEXT32 endp
	.errnz	(offset $ - baseFRAGTEXT32 + 1) EQ 4

;--------------------------------------------------------------------------
; reserve space allocated for the private data which precedes code
;--------------------------------------------------------------------------

pDIBToYUV	dd	?		; -> compiled function
pRecreateSmoothFromDetail label dword
		dd	?		; -> compiled function

tileHeight	dw	?		; scans in current tile
hzSwizzle	db	?		; swizzle for nerp'ing along VECTOR
		db	?		; filler

oBits		dd	?		; 32-bit offset of input pixels

oDetail		dd	?		; base -> of detail list
oDetail_0Mod2	label	dword		; 32-bit offset of 0 mod 2 detail scan
oDetail_0Mod4	dd	?		; 32-bit offset of 0 mod 4 detail scan
oDetail_1Mod2	label	dword		; 32-bit offset of 1 mod 2 detail scan
oDetail_1Mod4	dd	?		; 32-bit offset of 1 mod 4 detail scan
oDetail_2Mod4	dd	?		; 32-bit offset of 2 mod 4 detail scan
oDetail_3Mod4	dd	?		; 32-bit offset of 3 mod 4 detail scan

oSmooth		dd	?		; 32-bit offset of smooth list
oSmooth_0Mod2	dd	?		; 32-bit offset of 0 mod 2 smooth scan
oSmooth_1Mod2	dd	?		; 32-bit offset of 1 mod 2 smooth scan
oSmoothUV	dd	?		; 32-bit offset of output smooth UV

oInterU2Work	dd	?		; working 32-bit offset of U2
oInterV2Work	dd	?		; working 32-bit offset of V2

oWork		dd	?		; 32-bit offset of work buffers
oWS0Current	dd	?		; -> current work scan 0
oWS0Effective	dd	?		; -> effective work scan 0
oWS1Current	dd	?		; -> current work scan 1
oWS2Current	dd	?		; -> current work scan 2
oWS3Current	dd	?		; -> current work scan 3
oWS3Effective	dd	?		; -> effective work scan 3

srcWidth	dd	?		; real DIB width (possibly not 0 mod 4)
cntHLoop	dd	?		; loop counter for horizontal filter

srcHeight	dd	?		; real DIB height (possibly not 0 mod 4)
cntVLoop	dd	?		; loop counter for vertical filter

firstTime	dd	?		; first time through flag

lookUp8		label	dword		; lookup table for 8 bit palettized
		dd	256 dup (?)

divBy7		label	byte		; divide by 7 table
ifndef	WIN32
OFF_DIVBY7	equ	cs:byte ptr (offset divBy7 - offset baseFRAGTEXT32)
endif
dividend	=	0
	rept	256 * 7
result 		=	(dividend + 3 + (dividend and 1)) / 7
	if	result	GT 255
result		=	255
	endif
	db	result
dividend	=	dividend + 1
	endm

ifdef	DEBUG				; for pointer range checking
bitsBase	dd	?
bitsLimit	dd	?
bitsYStep	dd	?

privBase	dd	?
privLimit	dd	?

detailBase	dd	?
detailLimit	dd	?

smoothBase	dd	?
smoothLimit	dd	?
endif
;DIBTOYUVPRIVATE		ends

;--------------------------------------------------------------------------
; Code fragment that pushes registers that must be preserved and sets
; up registers needed in the main body of code
;--------------------------------------------------------------------------

FRAGPROC	cEntry

ifndef	WIN32
ifdef	DEBUG
; Make sure that the segment is USE32 or else bomb out to debugger
	xor	eax,eax
	mov	ah,080h
	add	eax,eax
	jnc	@F			; it's ok, we're USE32...
int3:	int	3			; USE16 but shouldn't be!!!
	int	0			; cause divide by 0 if not in debug
	jmp	int3			; loop forever
@@:
endif

	push	ds
	push	esi
	push	edi
	push	ebp

	mov	ds,flatSel		; load selector to all memory
	assume	ds:nothing
else
	push	ebx
	push	esi
	push	edi
	push	ebp
endif
FRAGENDP	cEntry

;--------------------------------------------------------------------------
; Code fragments to process the input pixels in their 8, 16, or 24 bpp
; form into the internal YUV formats. Here we just do the horizontal
; filtering that will produce the Y, U, V filtered in x.
;--------------------------------------------------------------------------

; Register allocations
;
;	EAX		=	loop scratch
;	EBX.hi		=	building U2
;	EBX.lo		=	new Y
;	ECX.hi		=	building V2
;	ECX.lo		=	new U
;	EDX.hi		=	building Y2
;	EDX.lo		=	new V
;	ESI = rRGB	=	-> source pixels
;	EDI = rWork	=	-> work buffers
;	EBP = rDetail	=	-> detail list

; Register aliases

rYUV		equ	EAX
rYUV_16		equ	AX
rYUV_16_lo	equ	AL

rY		equ	AH
rY2_16_lo	equ	BL
rY2_16_hi	equ	BH
rY2_16		equ	BX
rY2_32		equ	EBX

rU		equ	AH
rU2_16_lo	equ	CL
rU2_16_hi	equ	CH
rU2_16		equ	CX
rU2_32		equ	ECX

rV		equ	AL
rV2_16_lo	equ	DL
rV2_16_hi	equ	DH
rV2_16		equ	DX
rV2_32		equ	EDX

rRGB		equ	ESI
rWork		equ	EDI
rDetail		equ	EBP

FRAGPROC	H1331Init
	movzx	eax,ds:tileHeight	; get # scans in this tile
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<4>
	shr	eax,1			; # out scans is 1/2 input
	mov	ds:cntVLoop,eax
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<4>

	mov	ds:oInterU2Work,0
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<H1331Init>,<FIXUP_INTERU2>,<4>
	mov	ds:oInterV2Work,0
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<H1331Init>,<FIXUP_INTERV2>,<4>

	mov	ds:oWork,0
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<H1331Init>,<FIXUP_WORKPLUS>,<4>

	mov	eax,ds:oDetail		; fetch -> detail list
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<4>
	mov	ds:oDetail_0Mod4,eax	; 0 mod 4 scan -> in detail list
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<4>
	inc	eax
	inc	eax			; skip YY
	mov	ds:oDetail_1Mod4,eax	; 1 mod 4 scan -> in detail list
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<4>
	add	eax,6+8			; skip YYUV + YYYYUV
	mov	ds:oDetail_2Mod4,eax	; 2 mod 4 scan -> in detail list
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<4>
	inc	eax
	inc	eax			; skip YY
	mov	ds:oDetail_3Mod4,eax	; 3 mod 4 scan -> in detail list
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<4>

	mov	eax,ds:oSmooth		; fetch -> smooth list
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<4>
	mov	ds:oSmooth_0Mod2,eax	; 0 mod 2 scan -> in smooth list
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<4>
	inc	eax
	inc	eax			; skip YY
	mov	ds:oSmooth_1Mod2,eax	; 1 mod 2 scan -> in smooth list
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<4>

	mov	ds:oWS1Current,0
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<H1331Init>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS2Current,1
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<H1331Init>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS3Current,2
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<H1331Init>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS0Current,3
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<H1331Init>,<FIXUP_WORKPLUS>,<4>

	mov	ds:firstTime,1
	FRAGFIXUP	<H1331Init>,<FIXUP_DSREL32>,<8>
FRAGENDP	H1331Init


FRAGPROC	H1331Start
	mov	rRGB,ds:oBits
	FRAGFIXUP	<H1331Start>,<FIXUP_DSREL32>,<4>
	dec	ds:srcHeight		; are we all out of input scans?
	FRAGFIXUP	<H1331Start>,<FIXUP_DSREL32>,<4>
	js	H1331Start_0		; yes -- process previous pixels...
	add	ds:oBits,80000001h	; point to next scanline of input
	FRAGFIXUP	<H1331Start>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<H1331Start>,<FIXUP_SRCYSTEP>,<4>
H1331Start_0:

	mov	rWork,ds:oWork		; fetch -> current scan in work buffer
	FRAGFIXUP	<H1331Start>,<FIXUP_DSREL32>,<4>
	lea	eax,[rWork][80000001h]	; calculate -> next scan
	FRAGFIXUP	<H1331Start>,<FIXUP_WORKYSTEP>,<4>
	cmp	eax,80000004h		; have we rolled around?
	FRAGFIXUP	<H1331Start>,<FIXUP_WORKPLUS>,<4>
	jb	H1331Start_1		; no -- haven't rolled around yet...
	mov	eax,80000000h		; yes -- get -> to start of work buffer
	FRAGFIXUP	<H1331Start>,<FIXUP_WORKPLUS>,<4>
H1331Start_1:
	mov	ds:oWork,eax		; save -> next scan in work buffer
	FRAGFIXUP	<H1331Start>,<FIXUP_DSREL32>,<4>

	mov	ds:hzSwizzle,10101010b	; every other time through loop
	FRAGFIXUP	<H1331Start>,<FIXUP_DSREL32>,<5>
	mov	rDetail,ds:oDetail_0Mod4; -> next scan in detail list
	FRAGFIXUP	<H1331Start>,<FIXUP_DSREL32>,<4>

	mov	ds:srcWidth,80000000h	; fetch,save real source width
	FRAGFIXUP	<H1331Start>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<H1331Start>,<FIXUP_SRCWIDTH>,<4>
	dec	ds:srcWidth		; 0-based
	FRAGFIXUP	<H1331Start>,<FIXUP_DSREL32>,<4>

	mov	ds:cntHLoop,80000000h	; save for loop counter
	FRAGFIXUP	<H1331Start>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<H1331Start>,<FIXUP_WORKWIDTH>,<4>
FRAGENDP	H1331Start

FRAGPROC	H1331End
;FRAGPROC	H134
	movzx	eax,rY2_16_lo		; get last fetched Y
	shr	rY2_32,16		; access to component we'll write
	add	rY2_32,rYUV		; add last component
	add	rY2_32,3		; normalize
	shr	rY2_32,3
	CheckPtr	CHECKPRIV,rWork,0,<H1331End>,<>
	mov	[rWork],rY2_16_lo	; save Y2

	movzx	eax,rV2_16_lo		; get last fetched V
	shr	rV2_32,16		; access to component we'll write
	add	rV2_32,rYUV		; add last component
	add	rV2_32,3		; normalize
	shr	rV2_32,3
	CheckPtr CHECKPRIV,rWork,<80000008h>,<H1331End>,<FIXUP_WORKYSTEP>
	mov	[rWork][80000008h],rV2_16_lo; save V2
	FRAGFIXUP	<H1331End>,<FIXUP_WORKYSTEP>,<4>

	movzx	eax,rU2_16_lo		; get last fetch U
	shr	rU2_32,16		; access to component we'll write
	add	rU2_32,rYUV		; add last component
	add	rU2_32,3		; normalize
	shr	rU2_32,3
	CheckPtr CHECKPRIV,rWork,<80000004h>,<H1331End>,<FIXUP_WORKYSTEP>
	mov	[rWork][80000004h],rU2_16_lo; save U2
	FRAGFIXUP	<H1331End>,<FIXUP_WORKYSTEP>,<4>
;FRAGENDP	H134

	xchg	rDetail,ds:oDetail_3Mod4; percolate detail ->s up
	FRAGFIXUP	<H1331End>,<FIXUP_DSREL32>,<4>
	xchg	rDetail,ds:oDetail_2Mod4
	FRAGFIXUP	<H1331End>,<FIXUP_DSREL32>,<4>
	xchg	rDetail,ds:oDetail_1Mod4
	FRAGFIXUP	<H1331End>,<FIXUP_DSREL32>,<4>
	mov	ds:oDetail_0Mod4,rDetail; will be next -> detail list Ys
	FRAGFIXUP	<H1331End>,<FIXUP_DSREL32>,<4>
FRAGENDP	H1331End

FRAGPROC	H1331FetchPixel8
	CheckPtr	CHECKBITS,rRGB,0,<H1331FetchPixel8>,<>
	xor	eax,eax
	mov	al,byte ptr [rRGB]	; fetch a pixel
	dec	ds:srcWidth		; fetched all pixels this scan?
	FRAGFIXUP	<H1331FetchPixel8>,<FIXUP_DSREL32>,<4>
	js	@F			; yes -- use same pixel next time...
	inc	rRGB			; bump for next pixel
@@:

	mov	eax,ds:lookUp8[eax * 4]	; fetch RGB for the pixel
	FRAGFIXUP	<H1331FetchPixel8>,<FIXUP_DSREL32>,<4>
FRAGENDP	H1331FetchPixel8

FRAGPROC	H1331FetchPixel16
	CheckPtr	CHECKBITS,rRGB,0,<H1331FetchPixel15>,<>
	mov	ax,[rRGB]		; fetch a pixel
	dec	ds:srcWidth		; fetched all pixels this scan?
	FRAGFIXUP	<H1331FetchPixel16>,<FIXUP_DSREL32>,<4>
	js	@F			; yes -- use same pixel next time...
	add	rRGB,2			; bump for next pixel
@@:

	shl	eax,3			; ????????|?????RRR|RRGGGGGG|BBBBB000
	ror	eax,8			; BBBBB000|????????|?????RRR|RRGGGGGG
	shl	ax,2			; BBBBB000|????????|???RRRRR|GGGGGG00
	shl	ah,3			; BBBBB000|????????|RRRRR000|GGGGGG00
	rol	eax,8			; ????????|RRRRR000|GGGGGG00|BBBBB000
FRAGENDP	H1331FetchPixel16

FRAGPROC	H1331FetchPixel15
	CheckPtr	CHECKBITS,rRGB,0,<H1331FetchPixel15>,<>
	mov	ax,[rRGB]		; fetch a pixel
	dec	ds:srcWidth		; fetched all pixels this scan?
	FRAGFIXUP	<H1331FetchPixel15>,<FIXUP_DSREL32>,<4>
	js	@F			; yes -- use same pixel next time...
	add	rRGB,2			; bump for next pixel
@@:

	shl	eax,3			; ????????|??????RR|RRRGGGGG|BBBBB000
	ror	eax,8			; BBBBB000|????????|??????RR|RRRGGGGG
	shl	ax,3			; BBBBB000|????????|???RRRRR|GGGGG000
	shl	ah,3			; BBBBB000|????????|RRRRR000|GGGGG000
	rol	eax,8			; ????????|RRRRR000|GGGGG000|BBBBB000
FRAGENDP	H1331FetchPixel15

FRAGPROC	H1331FetchPixel24
	CheckPtr	CHECKBITS,rRGB,0,<H1331FetchPixel24>,<>
	mov	eax,[rRGB]		; fetch a pixel
	dec	ds:srcWidth		; fetched all pixels this scan?
	FRAGFIXUP	<H1331FetchPixel24>,<FIXUP_DSREL32>,<4>
	js	@F			; yes -- use same pixel next time...
	add	rRGB,3			; bump for next pixel
@@:
FRAGENDP	H1331FetchPixel24

FRAGPROC	H1331FetchPixel32
	CheckPtr	CHECKBITS,rRGB,0,<H1331FetchPixel32>,<>
	mov	eax,[rRGB]		; fetch a pixel
	dec	ds:srcWidth		; fetched all pixels this scan?
	FRAGFIXUP	<H1331FetchPixel32>,<FIXUP_DSREL32>,<4>
	js	@F			; yes -- use same pixel next time...
	add	rRGB,4			; bump for next pixel
@@:
FRAGENDP	H1331FetchPixel32

FRAGPROC	H1331ToYUV

;     eax	?|R|G|B

	push	ebx
	push	ecx

	xor	ebx,ebx
	mov	bl,ah			; zext G into BX
	xor	ecx,ecx
	mov	cl,al			; zext B into CX
	shr	eax,16			; zext R into EAX
	xor	ah,ah

	shl	ebx,2			; 4G
	add	ebx,eax
	add	ebx,eax			; 2R + 4G
	add	ebx,ecx			; 2R + 4G + B
ifndef	WIN32
	movzx	ebx,OFF_DIVBY7[ebx]	; Y = (2R + 4G + B) / 7
else
	movzx	ebx,ds:divBy7[ebx]
	FRAGFIXUP	<H1331ToYUV>,<FIXUP_DSREL32>,<4>
endif

	sub	ecx,ebx			; B - Y
	sar	ecx,1			; (B - Y) / 2
	add	ecx,128			; U = 128 + ((B - Y) / 2)

	sub	eax,ebx			; R - Y
	sar	eax,1			; (R - Y) / 2
	add	eax,128			; V = 128 + ((R - Y) / 2)

	shl	eax,24			; V|0|0|0
	mov	ah,cl			; combine YUV into EAX
	mov	al,bl			; V|0|U|Y
	rol	eax,8			; 0|U|Y|V

	pop	ecx
	pop	ebx
FRAGENDP	H1331ToYUV

FRAGPROC	H1331StoreY0
; Store the 1st Y of a pair on the same line to the detail list...
	CheckPtr	CHECKDETAIL,rDetail,0,<H1331StoreY0>,<>
	mov	ds:[rDetail],rY
FRAGENDP	H1331StoreY0

FRAGPROC	H1331StoreY1
; Store the 2nd Y of a pair on the same line to the detail list...
	CheckPtr	CHECKDETAIL,rDetail,1,<H1331StoreY1>,<>
	mov	ds:[rDetail][1],rY
FRAGENDP	H1331StoreY1

FRAGPROC	H431
	movzx	rY2_32,rY		; the 4 of the 4:3:1
	shl	rY2_32,2

	movzx	rV2_32,rV		; the 4 of the 4:3:1
	shl	rV2_32,2

	ror	eax,8			; access to U
	movzx	rU2_32,rU		; the 4 of the 4:3:1
	shl	rU2_32,2

	jmp	near ptr baseFRAGTEXT32; jump into loop...
	FRAGFIXUP	<H431>,<FIXUP_JMPLP1>,<4>
FRAGENDP	H431

FRAGPROC	H1331B
	ror	rY2_32,16		; access to component we'll write
	add	rY2_16_lo,rY		; add last component
	adc	rY2_16_hi,0
	add	rY2_16,3		; normalize
	shr	rY2_16,3
	CheckPtr	CHECKPRIV,rWork,0,<H1331B>,<>
	mov	[rWork],rY2_16_lo	; save Y2
	ror	rY2_32,16
	add	rY2_16_lo,rY		; 3 * new Y added into old
	adc	rY2_16_hi,0
	add	rY2_16_lo,rY
	adc	rY2_16_hi,0
	add	rY2_16_lo,rY
	adc	rY2_16_hi,0

	ror	rV2_32,16		; access to component we'll write
	add	rV2_16_lo,rV		; add last component
	adc	rV2_16_hi,0
	add	rV2_16,3		; normalize
	shr	rV2_16,3
	CheckPtr	CHECKPRIV,rWork,<80000008h>,<H1331B>,<FIXUP_WORKYSTEP>
	mov	[rWork][80000008h],rV2_16_lo; save V2
	FRAGFIXUP	<H1331B>,<FIXUP_WORKYSTEP>,<4>
	ror	rV2_32,16
	add	rV2_16_lo,rV		; 3 * new V added into old
	adc	rV2_16_hi,0
	add	rV2_16_lo,rV
	adc	rV2_16_hi,0
	add	rV2_16_lo,rV
	adc	rV2_16_hi,0

	ror	rYUV,8			; access to new U
	ror	rU2_32,16		; access to component we'll write
	add	rU2_16_lo,rU		; add last component
	adc	rU2_16_hi,0
	add	rU2_16,3		; normalize
	shr	rU2_16,3
	CheckPtr	CHECKPRIV,rWork,<80000004h>,<H1331B>,<FIXUP_WORKYSTEP>
	mov	[rWork][80000004h],rU2_16_lo; save U2
	FRAGFIXUP	<H1331B>,<FIXUP_WORKYSTEP>,<4>
	ror	rU2_32,16
	add	rU2_16_lo,rU		; 3 * new U added into old
	adc	rU2_16_hi,0
	add	rU2_16_lo,rU
	adc	rU2_16_hi,0
	add	rU2_16_lo,rU
	adc	rU2_16_hi,0

	inc	rWork			; point to next work scan pixel
FRAGENDP	H1331B

FRAGPROC	H1331C
	add	rY2_16_lo,rY		; 3 * new Y added into old
	adc	rY2_16_hi,0
	add	rY2_16_lo,rY
	adc	rY2_16_hi,0
	add	rY2_16_lo,rY
	adc	rY2_16_hi,0
	shl	rY2_32,16		; push it down the pipeline
	mov	rY2_16_lo,rY		; save new component

	add	rV2_16_lo,rV		; 3 * new V added into old
	adc	rV2_16_hi,0
	add	rV2_16_lo,rV
	adc	rV2_16_hi,0
	add	rV2_16_lo,rV
	adc	rV2_16_hi,0
	shl	rV2_32,16		; push it down the pipeline
	mov	rV2_16_lo,rV		; save new component

	ror	rYUV,16			; access to 0|U
	add	rU2_16,rYUV_16		; 3 * new U added into old
	add	rU2_16,rYUV_16
	add	rU2_16,rYUV_16
	shl	rU2_32,16		; push it down the pipeline
	mov	rU2_16_lo,rYUV_16_lo	; save new U component
	rol	rYUV,16			; access to YV once again
FRAGENDP	H1331C

FRAGPROC	H1331Loop
	add	rDetail,8		; point to next Y pair for this scan
	ror	ds:hzSwizzle,1		; time to skip over 2 VECTOR fields?
	FRAGFIXUP	<H1331Loop>,<FIXUP_DSREL32>,<4>
	sbb	eax,eax			; (yes) ? 255 : 0
	and	eax,16			; (yes) ? (2 * sizeof(VECTOR)) : 0
	add	rDetail,eax		; update -> next VECTOR field

	dec	ds:cntHLoop		; drop count
	FRAGFIXUP	<H1331Loop>,<FIXUP_DSREL32>,<4>
	jnz	near ptr baseFRAGTEXT32
	FRAGFIXUP	<H1331Loop>,<FIXUP_USE_STOREDPC0>,<4>
FRAGENDP	H1331Loop

;----------------------------------------------------------------------------
; Fragments for generating vertical filtering code for Y2,U2,V2
;----------------------------------------------------------------------------

rWork0		equ	ECX		; -> top scan of a quadruple of scans
rWork1		equ	EDX		; -> next scan
rWork2		equ	ESI		; -> next scan
rWork3		equ	EDI		; -> bottom scan of a quadruple of scans
rDest		equ	EBP		; -> place to store output pixels

FRAGPROC	V1331EarlyOut
	cmp	ds:cntVLoop,1		; processing last pair of scans?
	FRAGFIXUP	<V1331EarlyOut>,<FIXUP_DSREL32>,<5>
	je	near ptr baseFRAGTEXT32; yes -- early out...
	FRAGFIXUP	<V1331EarlyOut>,<FIXUP_JMPLP1>,<4>
FRAGENDP	V1331EarlyOut

FRAGPROC	V1331FetchY2Dest
	mov	rDest,ds:oSmooth_0Mod2	; fetch -> smooth Y's
	FRAGFIXUP	<V1331FetchY2Dest>,<FIXUP_DSREL32>,<4>

	mov	rWork0,ds:oWS0Current	; load up ->s to each of 4 scans
	FRAGFIXUP	<V1331FetchY2Dest>,<FIXUP_DSREL32>,<4>
	mov	rWork1,ds:oWS1Current
	FRAGFIXUP	<V1331FetchY2Dest>,<FIXUP_DSREL32>,<4>
	mov	rWork2,ds:oWS2Current
	FRAGFIXUP	<V1331FetchY2Dest>,<FIXUP_DSREL32>,<4>
	mov	rWork3,ds:oWS3Current
	FRAGFIXUP	<V1331FetchY2Dest>,<FIXUP_DSREL32>,<4>

	dec	ds:firstTime		; first time through?
	FRAGFIXUP	<V1331FetchY2Dest>,<FIXUP_DSREL32>,<4>
	jnz	@F			; no...
	mov	rWork0,rWork1		; first,next the same
@@:	mov	ds:oWS0Effective,rWork0	; save for U2,V2
	FRAGFIXUP	<V1331FetchY2Dest>,<FIXUP_DSREL32>,<4>

	cmp	ds:cntVLoop,1		; last time through?
	FRAGFIXUP	<V1331FetchY2Dest>,<FIXUP_DSREL32>,<5>
	jne	@F			; no...
	mov	rWork3,rWork2		; last,previous the same
@@:	mov	ds:oWS3Effective,rWork3	; save for U2,V2
	FRAGFIXUP	<V1331FetchY2Dest>,<FIXUP_DSREL32>,<4>

	mov	eax,1			; ystep in work buffers
	FRAGFIXUP	<V1331FetchY2Dest>,<FIXUP_WORKYSTEP>,<4>
FRAGENDP	V1331FetchY2Dest

FRAGPROC	V1331FetchU2Dest
	mov	rDest,ds:oInterU2Work
	FRAGFIXUP	<V1331FetchU2Dest>,<FIXUP_DSREL32>,<4>

	mov	rWork0,ds:oWS0Effective	; load up ->s to each of 4 scans
	FRAGFIXUP	<V1331FetchU2Dest>,<FIXUP_DSREL32>,<4>
	mov	rWork1,ds:oWS1Current
	FRAGFIXUP	<V1331FetchU2Dest>,<FIXUP_DSREL32>,<4>
	mov	rWork2,ds:oWS2Current
	FRAGFIXUP	<V1331FetchU2Dest>,<FIXUP_DSREL32>,<4>
	mov	rWork3,ds:oWS3Effective
	FRAGFIXUP	<V1331FetchU2Dest>,<FIXUP_DSREL32>,<4>

	mov	eax,4			; ystep * 4 in work buffers
	FRAGFIXUP	<V1331FetchU2Dest>,<FIXUP_WORKYSTEP>,<4>
	add	rWork0,eax
	add	rWork1,eax
	add	rWork2,eax
	add	rWork3,eax
	shr	eax,2			; just ystep now
FRAGENDP	V1331FetchU2Dest

FRAGPROC	V1331FetchV2Dest
	mov	rDest,ds:oInterV2Work
	FRAGFIXUP	<V1331FetchV2Dest>,<FIXUP_DSREL32>,<4>

	mov	rWork0,ds:oWS0Effective	; load up ->s to each of 4 scans
	FRAGFIXUP	<V1331FetchV2Dest>,<FIXUP_DSREL32>,<4>
	mov	rWork1,ds:oWS1Current
	FRAGFIXUP	<V1331FetchV2Dest>,<FIXUP_DSREL32>,<4>
	mov	rWork2,ds:oWS2Current
	FRAGFIXUP	<V1331FetchV2Dest>,<FIXUP_DSREL32>,<4>
	mov	rWork3,ds:oWS3Effective
	FRAGFIXUP	<V1331FetchV2Dest>,<FIXUP_DSREL32>,<4>

	mov	eax,8			; ystep * 8 in work buffer
	FRAGFIXUP	<V1331FetchV2Dest>,<FIXUP_WORKYSTEP>,<4>
	add	rWork0,eax
	add	rWork1,eax
	add	rWork2,eax
	add	rWork3,eax
	shr	eax,3			; just ystep now
FRAGENDP	V1331FetchV2Dest

FRAGPROC	V1331Body
	mov	ds:cntHLoop,eax		; save ystep (== pixels) for loop cnt
	FRAGFIXUP	<V1331Body>,<FIXUP_DSREL32>,<4>

	FRAGFIXUP	<V1331Body>,<FIXUP_STOREPC0>,<0>

	CheckPtr	CHECKPRIV,<rWork1>,<0>,<V1331Body>,<>
	xor	eax,eax
	mov	al,byte ptr [rWork1]	; b
	CheckPtr	CHECKPRIV,<rWork2>,<0>,<V1331Body>,<>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork2]	; c
	add	ebx,eax			; b + c
	lea	ebx,[ebx][ebx * 2]	; 3b + 3c
	CheckPtr	CHECKPRIV,<rWork0>,<0>,<V1331Body>,<>
	xor	eax,eax
	mov	al,byte ptr [rWork0]	; a
	add	eax,ebx			; a + 3b + 3c
	CheckPtr	CHECKPRIV,<rWork3>,<0>,<V1331Body>,<>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork3]	; d
	add	eax,ebx			; a + 3b + 3c + d

	add	ax,3			; normalize
	shr	eax,3			; AL = pixel to write
	CheckPtr	CHECKPRIV,<rDest>,<0>,<V1331Body>,<>
	mov	ds:[rDest],al		; store it

	inc	rWork0			; bump pointers
	inc	rWork1
	inc	rWork2
	inc	rWork3
FRAGENDP	V1331Body
ifdef	DEBUG
FRAGPROC	V1331Body_Y2
	mov	ds:cntHLoop,eax		; save ystep (== pixels) for loop cnt
	FRAGFIXUP	<V1331Body_Y2>,<FIXUP_DSREL32>,<4>

	FRAGFIXUP	<V1331Body_Y2>,<FIXUP_STOREPC0>,<0>

	CheckPtr	CHECKPRIV,<rWork1>,<0>,<V1331Body_Y2>,<>
	xor	eax,eax
	mov	al,byte ptr [rWork1]	; b
	CheckPtr	CHECKPRIV,<rWork2>,<0>,<V1331Body_Y2>,<>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork2]	; c
	add	ebx,eax			; b + c
	lea	ebx,[ebx][ebx * 2]	; 3b + 3c
	CheckPtr	CHECKPRIV,<rWork0>,<0>,<V1331Body_Y2>,<>
	xor	eax,eax
	mov	al,byte ptr [rWork0]	; a
	add	eax,ebx			; a + 3b + 3c
	CheckPtr	CHECKPRIV,<rWork3>,<0>,<V1331Body_Y2>,<>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork3]	; d
	add	eax,ebx			; a + 3b + 3c + d

	add	ax,3			; normalize
	shr	eax,3			; AL = pixel to write
	CheckPtr	CHECKSMOOTH,<rDest>,<0>,<V1331Body_Y2>,<>
	mov	ds:[rDest],al		; store it

	inc	rWork0			; bump pointers
	inc	rWork1
	inc	rWork2
	inc	rWork3
FRAGENDP	V1331Body_Y2
endif

FRAGPROC	V1331IncY2Dest
	mov	eax,rDest
	shr	eax,1
	sbb	eax,eax
	and	eax,6			; (currently on odd pixel) ? 6 : 0
	inc	rDest			; point to next output pixel
	add	rDest,eax		; adjust to next pair if last was odd
FRAGENDP	V1331IncY2Dest

FRAGPROC	V1331IncU2V2Dest
	inc	rDest			; simple progressing ->
FRAGENDP	V1331IncU2V2Dest

FRAGPROC	V1331Loop
	dec	ds:cntHLoop		; drop count
	FRAGFIXUP	<V1331Loop>,<FIXUP_DSREL32>,<4>
	jnz	near ptr baseFRAGTEXT32
	FRAGFIXUP	<V1331Loop>,<FIXUP_USE_STOREDPC0>,<4>
FRAGENDP	V1331Loop

FRAGPROC	V1331StoreY2Dest
	xchg	rDest,ds:oSmooth_1Mod2	; percolate smooth ->s up
	FRAGFIXUP	<V1331StoreY2Dest>,<FIXUP_DSREL32>,<4>
	mov	ds:oSmooth_0Mod2,rDest	; will be next -> smooth list Ys
	FRAGFIXUP	<V1331StoreY2Dest>,<FIXUP_DSREL32>,<4>
FRAGENDP	V1331StoreY2Dest

FRAGPROC	V1331StoreU2Dest
	mov	ds:oInterU2Work,rDest	; store -> next U2 intermediate ->
	FRAGFIXUP	<V1331StoreU2Dest>,<FIXUP_DSREL32>,<4>
FRAGENDP	V1331StoreU2Dest

FRAGPROC	V1331StoreV2Dest
	mov	ds:oInterV2Work,rDest	; store -> next V2 intermediate ->
	FRAGFIXUP	<V1331StoreV2Dest>,<FIXUP_DSREL32>,<4>

	mov	eax,ds:oWS0Current	; swizzle ->s for next time
	FRAGFIXUP	<V1331StoreV2Dest>,<FIXUP_DSREL32>,<4>
	xchg	eax,ds:oWS2Current
	FRAGFIXUP	<V1331StoreV2Dest>,<FIXUP_DSREL32>,<4>
	mov	ds:oWS0Current,eax
	FRAGFIXUP	<V1331StoreV2Dest>,<FIXUP_DSREL32>,<4>

	mov	eax,ds:oWS1Current	; swizzle ->s for next time
	FRAGFIXUP	<V1331StoreV2Dest>,<FIXUP_DSREL32>,<4>
	xchg	eax,ds:oWS3Current
	FRAGFIXUP	<V1331StoreV2Dest>,<FIXUP_DSREL32>,<4>
	mov	ds:oWS1Current,eax
	FRAGFIXUP	<V1331StoreV2Dest>,<FIXUP_DSREL32>,<4>
FRAGENDP	V1331StoreV2Dest

FRAGPROC	HVLoop
	dec	ds:cntVLoop		; drop count
	FRAGFIXUP	<HVLoop>,<FIXUP_DSREL32>,<4>
	jnz	near ptr baseFRAGTEXT32
	FRAGFIXUP	<HVLoop>,<FIXUP_USE_STOREDPC1>,<4>
FRAGENDP	HVLoop

;----------------------------------------------------------------------------
; Fragments for generating filtering code for U2,V2 from the U,V filtered
; pixels that now reside in the intermediate buffers. We will filter in a
; similar manner to the first filtering producing a twice filter product.
;----------------------------------------------------------------------------

rUV_32		equ	EAX
rUV_16		equ	AX
rUV_16_lo	equ	AL
rUV_16_hi	equ	AH

riUV		equ	EBX
roUV		equ	ESI

FRAGPROC	U2V2HInit
	movzx	eax,ds:tileHeight	; get # scans in this tile
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<4>
	shr	eax,2			; # out scans is 1/4 input
	mov	ds:cntVLoop,eax
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<4>

	mov	ds:oInterU2Work,0
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<U2V2HInit>,<FIXUP_INTERU2>,<4>

	mov	ds:oWork,0
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<U2V2HInit>,<FIXUP_WORKPLUS>,<4>

	mov	eax,ds:oDetail		; -> detail list
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<4>
	add	eax,4
	mov	ds:oDetail_0Mod2,eax	; -> UV for 0 mod 2 scans
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<4>
	add	eax,16
	mov	ds:oDetail_1Mod2,eax	; -> UV for 1 mod 2 scans
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<4>

	mov	eax,ds:oSmooth		; -> smooth list
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<4>
	add	eax,4			; -> 1st smooth UV
	mov	ds:oSmoothUV,eax	; save for in loop
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<4>

	mov	ds:oWS1Current,0
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<U2V2HInit>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS2Current,1
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<U2V2HInit>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS3Current,2
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<U2V2HInit>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS0Current,3
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<U2V2HInit>,<FIXUP_WORKPLUS>,<4>

	mov	ds:firstTime,1
	FRAGFIXUP	<U2V2HInit>,<FIXUP_DSREL32>,<8>
FRAGENDP	U2V2HInit

FRAGPROC	U2V2HStart
	mov	riUV,ds:oInterU2Work
	FRAGFIXUP	<U2V2HStart>,<FIXUP_DSREL32>,<4>

	mov	roUV,ds:oDetail_0Mod2
	FRAGFIXUP	<U2V2HStart>,<FIXUP_DSREL32>,<4>

	mov	rWork,ds:oWork		; fetch -> current scan in work buffer
	FRAGFIXUP	<U2V2HStart>,<FIXUP_DSREL32>,<4>
	lea	eax,[rWork][80000001h]	; calculate -> next scan
	FRAGFIXUP	<U2V2HStart>,<FIXUP_WORKYSTEP>,<4>
	cmp	eax,80000004h		; have we rolled around?
	FRAGFIXUP	<U2V2HStart>,<FIXUP_WORKPLUS>,<4>
	jb	@F			; no -- haven't rolled around yet...
	mov	eax,80000000h		; yes -- get -> to start of work buffer
	FRAGFIXUP	<U2V2HStart>,<FIXUP_WORKPLUS>,<4>
@@:	mov	ds:oWork,eax		; save -> next scan in work buffer
	FRAGFIXUP	<U2V2HStart>,<FIXUP_DSREL32>,<4>

	mov	ds:cntHLoop,80000001h	; save for loop counter
	FRAGFIXUP	<U2V2HStart>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<U2V2HStart>,<FIXUP_WORKWIDTH>,<4>
FRAGENDP	U2V2HStart

FRAGPROC	U2V2HEnd
;FRAGPROC	U2V2H134
	shr	rU2_32,16		; access to component we'll write
	add	rU2_16,rUV_16		; add last component
	add	rU2_16,3		; normalize
	shr	rU2_16,3
	CheckPtr	CHECKPRIV,<rWork>,<0>,<U2V2HEnd>,<>
	mov	[rWork],rU2_16_lo	; save U2

	shr	rUV_32,16		; access to V
	shr	rV2_32,16		; access to component we'll write
	add	rV2_16,rUV_16		; add last V component
	add	rV2_16,3		; normalize
	shr	rV2_16,3
	CheckPtr CHECKPRIV,<rWork>,<80000004h>,<U2V2HEnd>,<FIXUP_WORKYSTEP>
	mov	[rWork][80000004h],rV2_16_lo; save V2
	FRAGFIXUP	<U2V2HEnd>,<FIXUP_WORKYSTEP>,<4>
;FRAGENDP	U2V2H134

	mov	ds:oInterU2Work,riUV	; save -> incoming pixels
	FRAGFIXUP	<U2V2HEnd>,<FIXUP_DSREL32>,<4>

	xchg	roUV,ds:oDetail_1Mod2	; percolate ->s up
	FRAGFIXUP	<U2V2HEnd>,<FIXUP_DSREL32>,<4>
	mov	ds:oDetail_0Mod2,roUV	; save -> next detail UV
	FRAGFIXUP	<U2V2HEnd>,<FIXUP_DSREL32>,<4>
FRAGENDP	U2V2HEnd

FRAGPROC	U2V2H431
	CheckPtr	CHECKPRIV,<riUV>,<0>,<U2V2H431>,<>
	mov	rUV_16_lo,[riUV]	; fetch a U
	CheckPtr CHECKPRIV,<riUV>,<80000000h>,<U2V2H431>,<FIXUP_UVOFF>
	mov	rUV_16_hi,[riUV][80000000h]; fetch a V
	FRAGFIXUP	<U2V2H431>,<FIXUP_UVOFF>,<4>
	inc	riUV			; -> next U

	CheckPtr	CHECKDETAIL,<roUV>,<0>,<U2V2H431>,<>
	mov	[roUV],ax		; store to detail UV spot
	add	roUV,8			; -> next detail UV

	movzx	rU2_32,rUV_16_lo	; get U to pipelined register
	shl	rU2_32,2		; the 4 of the 4:3:1

	movzx	rV2_32,rUV_16_hi	; get V to pipelined register
	shl	rV2_32,2		; the 4 of the 4:3:1

	jmp	near ptr baseFRAGTEXT32; jump into loop...
	FRAGFIXUP	<U2V2H431>,FIXUP_JMPLP1,<4>
FRAGENDP	U2V2H431

FRAGPROC	U2V2H1331B
	CheckPtr CHECKPRIV,<riUV>,<80000000h>,<U2V2H1331B>,<FIXUP_UVOFF>
	movzx	rUV_32,byte ptr [riUV][80000000h]; fetch V
	FRAGFIXUP	<U2V2H1331B>,<FIXUP_UVOFF>,<4>
	ror	rV2_32,16		; access to component we'll write
	add	rV2_16,rUV_16		; add last component
	add	rV2_16,3		; normalize
	shr	rV2_16,3
	CheckPtr CHECKPRIV,<rWork>,<80000004h>,<U2V2H1331B>,<FIXUP_WORKYSTEP>
	mov	[rWork][80000004h],rV2_16_lo; save V2
	FRAGFIXUP	<U2V2H1331B>,<FIXUP_WORKYSTEP>,<4>
	ror	rV2_32,16
	add	rV2_16,rUV_16		; 3 * new V added into old
	add	rV2_16,rUV_16
	add	rV2_16,rUV_16

	shl	rUV_32,8		; 0|0|V|0
	CheckPtr	CHECKPRIV,<riUV>,<0>,<U2V2H1331B>,<>
	mov	rUV_16_lo,[riUV]	; 0|0|V|U -- fetch U
	inc	riUV			; point to next U

	CheckPtr	CHECKDETAIL,<roUV>,<0>,<U2V2H1331B>,<>
	mov	[roUV],rUV_16		; store UV to detail list
	add	roUV,8			; point to next detail list spot

	movzx	rUV_16,rUV_16_lo	; 0|0|0|U
	ror	rU2_32,16		; access to component we'll write
	add	rU2_16,rUV_16		; add last component
	add	rU2_16,3		; normalize
	shr	rU2_16,3
	CheckPtr	CHECKPRIV,<rWork>,<0>,<U2V2H1331B>,<>
	mov	[rWork],rU2_16_lo	; save U2
	inc	rWork			; point to next U2
	ror	rU2_32,16
	add	rU2_16,rUV_16		; 3 * new U added into old
	add	rU2_16,rUV_16
	add	rU2_16,rUV_16
FRAGENDP	U2V2H1331B

FRAGPROC	U2V2H1331C
	CheckPtr CHECKPRIV,<riUV>,<80000000h>,<U2V2H1331C>,<FIXUP_UVOFF>
	movzx	rUV_32,byte ptr [riUV][80000000h]; fetch V
	FRAGFIXUP	<U2V2H1331C>,<FIXUP_UVOFF>,<4>
	add	rV2_16,rUV_16		; 3 * new V added into old
	add	rV2_16,rUV_16
	add	rV2_16,rUV_16
	shl	rV2_32,16		; push it down the pipeline
	mov	rV2_16,rUV_16		; save new component

	shl	rUV_32,8		; 0|0|V|0
	CheckPtr	CHECKPRIV,<riUV>,<0>,<U2V2H1331C>,<>
	mov	rUV_16_lo,[riUV]	; 0|0|V|U -- fetch U
	inc	riUV			; point to next U

	CheckPtr	CHECKDETAIL,<roUV>,<0>,<U2V2H1331C>,<>
	mov	[roUV],rUV_16		; store new UV to detail list
	add	roUV,24			; point to next detail list spot

	shl	rUV_32,8
	shr	rUV_16,8		; 0|V|0|U
	add	rU2_16,rUV_16		; 3 * new U added into old
	add	rU2_16,rUV_16
	add	rU2_16,rUV_16
	shl	rU2_32,16		; push it down the pipeline
	mov	rU2_16,rUV_16		; save new component
FRAGENDP	U2V2H1331C

FRAGPROC	U2V2HLoop
	dec	ds:cntHLoop		; drop count
	FRAGFIXUP	<U2V2HLoop>,<FIXUP_DSREL32>,<4>
	jnz	near ptr baseFRAGTEXT32
	FRAGFIXUP	<U2V2HLoop>,<FIXUP_USE_STOREDPC0>,<4>
FRAGENDP	U2V2HLoop

FRAGPROC	U2V2VBody
	mov	rDest,ds:oSmoothUV	; fetch -> smooth U's
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>

	mov	rWork0,ds:oWS0Current	; load up ->s to each of 4 scans
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
	mov	rWork1,ds:oWS1Current
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
	mov	rWork2,ds:oWS2Current
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
	mov	rWork3,ds:oWS3Current
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>

	dec	ds:firstTime		; first time through?
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
	jnz	@F			; no...
	mov	rWork0,rWork1		; first,next the same
@@:	mov	ds:oWS0Effective,rWork0	; save for U2,V2
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>

	cmp	ds:cntVLoop,1		; last time through?
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<5>
	jne	@F			; no...
	mov	rWork3,rWork2		; last,previous the same
@@:	mov	ds:oWS3Effective,rWork3	; save for U2,V2
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>

	mov	ds:cntHLoop,80000001h	; save for loop counter
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<U2V2VBody>,<FIXUP_WORKWIDTH>,<4>

U2V2VBody_HLoop:

	CheckPtr	CHECKPRIV,<rWork1>,<0>,<U2V2VBody>,<>
	xor	eax,eax
	mov	al,byte ptr [rWork1]	; b
	CheckPtr	CHECKPRIV,<rWork2>,<0>,<U2V2VBody>,<>
	add	al,[rWork2]		; b + c
	adc	ah,0
	lea	eax,[eax][eax * 2]	; 3b + 3c
	CheckPtr	CHECKPRIV,<rWork0>,<0>,<U2V2VBody>,<>
	add	al,[rWork0]		; a + 3b + 3c
	adc	ah,0
	CheckPtr	CHECKPRIV,<rWork3>,<0>,<U2V2VBody>,<>
	add	al,[rWork3]		; a + 3b + 3c + d
	adc	ah,0			; vertically filtered U2
	add	ax,3			; normalize
	shr	ax,3			; AL = U2 pixel to write

	CheckPtr CHECKPRIV,<rWork1>,<80000004h>,<U2V2VBody>,<FIXUP_WORKYSTEP>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork1][80000004h]; b
	FRAGFIXUP	<U2V2VBody>,<FIXUP_WORKYSTEP>,<4>
	CheckPtr CHECKPRIV,<rWork2>,<80000004h>,<U2V2VBody>,<FIXUP_WORKYSTEP>
	add	bl,[rWork2][80000004h]	; b + c
	FRAGFIXUP	<U2V2VBody>,<FIXUP_WORKYSTEP>,<4>
	adc	bh,0
	lea	ebx,[ebx][ebx * 2]	; 3b + 3c
	CheckPtr CHECKPRIV,<rWork0>,<80000004h>,<U2V2VBody>,<FIXUP_WORKYSTEP>
	add	bl,[rWork0][80000004h]	; a + 3b + 3c
	FRAGFIXUP	<U2V2VBody>,<FIXUP_WORKYSTEP>,<4>
	adc	bh,0
	CheckPtr CHECKPRIV,<rWork3>,<80000004h>,<U2V2VBody>,<FIXUP_WORKYSTEP>
	add	bl,[rWork3][80000004h]	; a + 3b + 3c + d
	FRAGFIXUP	<U2V2VBody>,<FIXUP_WORKYSTEP>,<4>
	adc	bh,0
	add	bx,3			; normalize
	shr	bx,3			; BL = V2 pixel to write

	mov	ah,bl			; combine U2,V2
	CheckPtr	CHECKSMOOTH,<rDest>,<0>,<U2V2VBody>,<>
	mov	ds:[rDest],ax		; store U2,V2 to smooth list
	add	rDest,8			; -> next U2,V2 spot

	inc	rWork0			; bump pointers in work scans
	inc	rWork1
	inc	rWork2
	inc	rWork3

	dec	ds:cntHLoop		; drop count
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
	jnz	U2V2VBody_HLoop		; loop for all pixels along scan...

	mov	ds:oSmoothUV,rDest	; store -> next spot in smooth list
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>

	mov	eax,ds:oWS0Current	; swizzle ->s for next time
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
	xchg	eax,ds:oWS2Current
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
	mov	ds:oWS0Current,eax
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
	mov	eax,ds:oWS1Current	; swizzle ->s for next time
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
	xchg	eax,ds:oWS3Current
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
	mov	ds:oWS1Current,eax
	FRAGFIXUP	<U2V2VBody>,<FIXUP_DSREL32>,<4>
FRAGENDP	U2V2VBody

;----------------------------------------------------------------------------
; The code that cleans up and returns from the whole wad of compiled code
;----------------------------------------------------------------------------

FRAGPROC	cExit
	pop	ebp
	pop	edi
	pop	esi
ifndef	WIN32
	pop	ds
___cExit	proc	far
	Use32Ret
___cExit	endp
else
	pop	ebx
___cExit	proc	near
	ret
___cExit	endp
endif
FRAGENDP	cExit

;----------------------------------------------------------------------------
; Code to generate smooth vector Y's from detail vector Y's
;----------------------------------------------------------------------------

FRAGPROC	rY_H1331Init
	movzx	eax,ds:tileHeight	; get # scans in this tile
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<4>
	shr	eax,1			; # out scans is 1/2 input
	mov	ds:cntVLoop,eax
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<4>

	mov	ds:oWork,0
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_WORKPLUS>,<4>

	mov	eax,ds:oDetail		; fetch -> detail list
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<4>
	mov	ds:oDetail_0Mod4,eax	; 0 mod 4 scan -> in detail list
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<4>
	inc	eax
	inc	eax			; skip YY
	mov	ds:oDetail_1Mod4,eax	; 1 mod 4 scan -> in detail list
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<4>
	add	eax,6+8			; skip YYUV + YYYYUV
	mov	ds:oDetail_2Mod4,eax	; 2 mod 4 scan -> in detail list
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<4>
	inc	eax
	inc	eax			; skip YY
	mov	ds:oDetail_3Mod4,eax	; 3 mod 4 scan -> in detail list
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<4>

	mov	eax,ds:oSmooth		; fetch -> smooth list
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<4>
	mov	ds:oSmooth_0Mod2,eax	; 0 mod 2 scan -> in smooth list
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<4>
	inc	eax
	inc	eax			; skip YY
	mov	ds:oSmooth_1Mod2,eax	; 1 mod 2 scan -> in smooth list
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<4>

	mov	ds:oWS1Current,0
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS2Current,1
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS3Current,2
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS0Current,3
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_WORKPLUS>,<4>

	mov	ds:firstTime,1
	FRAGFIXUP	<rY_H1331Init>,<FIXUP_DSREL32>,<8>
FRAGENDP	rY_H1331Init

FRAGPROC	rY_H1331Start
	mov	rWork,ds:oWork		; fetch -> current scan in work buffer
	FRAGFIXUP	<rY_H1331Start>,<FIXUP_DSREL32>,<4>
	lea	eax,[rWork][80000001h]	; calculate -> next scan
	FRAGFIXUP	<rY_H1331Start>,<FIXUP_WORKYSTEP>,<4>
	cmp	eax,80000004h		; have we rolled around?
	FRAGFIXUP	<rY_H1331Start>,<FIXUP_WORKPLUS>,<4>
	jb	rY_H1331Start_0		; no -- haven't rolled around yet...
	mov	eax,80000000h		; yes -- get -> to start of work buffer
	FRAGFIXUP	<rY_H1331Start>,<FIXUP_WORKPLUS>,<4>
rY_H1331Start_0:
	mov	ds:oWork,eax		; save -> next scan in work buffer
	FRAGFIXUP	<rY_H1331Start>,<FIXUP_DSREL32>,<4>

	mov	ds:hzSwizzle,10101010b	; every other time through loop
	FRAGFIXUP	<rY_H1331Start>,<FIXUP_DSREL32>,<5>
	mov	rDetail,ds:oDetail_0Mod4; -> next scan in detail list
	FRAGFIXUP	<rY_H1331Start>,<FIXUP_DSREL32>,<4>

	mov	ds:cntHLoop,80000000h	; save for loop counter
	FRAGFIXUP	<rY_H1331Start>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rY_H1331Start>,<FIXUP_WORKWIDTH>,<4>
FRAGENDP	rY_H1331Start

FRAGPROC	rY_H1331End
;FRAGPROC	rY_H134
	movzx	rYUV,rY2_16		; get last component
	shr	rY2_32,16		; access to component we'll write
	add	rY2_32,rYUV		; add last component
	add	rY2_32,3		; normalize
	shr	rY2_32,3
	CheckPtr	CHECKPRIV,rWork,0,<rY_H1331End>,<>
	mov	[rWork],rY2_16_lo	; save Y2
;FRAGENDP	rY_H134

	xchg	rDetail,ds:oDetail_3Mod4; percolate detail ->s up
	FRAGFIXUP	<rY_H1331End>,<FIXUP_DSREL32>,<4>
	xchg	rDetail,ds:oDetail_2Mod4
	FRAGFIXUP	<rY_H1331End>,<FIXUP_DSREL32>,<4>
	xchg	rDetail,ds:oDetail_1Mod4
	FRAGFIXUP	<rY_H1331End>,<FIXUP_DSREL32>,<4>
	mov	ds:oDetail_0Mod4,rDetail; will be next -> detail list Ys
	FRAGFIXUP	<rY_H1331End>,<FIXUP_DSREL32>,<4>
FRAGENDP	rY_H1331End

FRAGPROC	rY_H431
	CheckPtr	CHECKDETAIL,<rDetail>,<0>,<rY_H431>,<>
	movzx	rY2_32,ds:byte ptr [rDetail]; the 4 of the 4:3:1
	shl	rY2_32,2

	jmp	near ptr baseFRAGTEXT32; jump into loop...
	FRAGFIXUP	<rY_H431>,<FIXUP_JMPLP1>,<4>
FRAGENDP	rY_H431

FRAGPROC	rY_H1331B
	CheckPtr	CHECKDETAIL,<rDetail>,<0>,<rY_H1331B>,<>
	movzx	rYUV,ds:byte ptr [rDetail]; fetch next Y
	ror	rY2_32,16		; access to component we'll write
	add	rY2_16,rYUV_16		; add last component
	add	rY2_16,3		; normalize
	shr	rY2_16,3
	CheckPtr	CHECKPRIV,<rWork>,<0>,<rY_H1331B>,<>
	mov	[rWork],rY2_16_lo	; save Y2
	ror	rY2_32,16
	add	rY2_16,rYUV_16		; 3 * new Y added into old
	add	rY2_16,rYUV_16
	add	rY2_16,rYUV_16

	inc	rWork			; -> next work buffer location
FRAGENDP	rY_H1331B

FRAGPROC	rY_H1331C
	CheckPtr	CHECKDETAIL,<rDetail>,<1>,<rY_H1331C>,<>
	movzx	rYUV,ds:byte ptr [rDetail][1]; fetch next Y
	add	rY2_16,rYUV_16		; 3 * new Y added into old
	add	rY2_16,rYUV_16
	add	rY2_16,rYUV_16
	shl	rY2_32,16		; push it down the pipeline
	mov	rY2_16,rYUV_16		; save new component
FRAGENDP	rY_H1331C

FRAGPROC	rY_V1331Body
	mov	rDest,ds:oSmooth_0Mod2	; fetch -> smooth Y's
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>

	mov	rWork0,ds:oWS0Current	; load up ->s to each of 4 scans
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	rWork1,ds:oWS1Current
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	rWork2,ds:oWS2Current
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	rWork3,ds:oWS3Current
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>

	dec	ds:firstTime		; first time through?
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	jnz	@F			; no...
	mov	rWork0,rWork1		; first,next the same
@@:

	cmp	ds:cntVLoop,1		; last time through?
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<5>
	jne	@F			; no...
	mov	rWork3,rWork2		; last,previous the same
@@:

	mov	ds:cntHLoop,[80000001h]	; save ystep (== pixels) for loop cnt
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_WORKYSTEP>,<4>

	FRAGFIXUP	<rY_V1331Body>,<FIXUP_STOREPC0>,<0>

	CheckPtr	CHECKPRIV,<rWork1>,<0>,<rY_V1331Body>,<>
	xor	eax,eax
	mov	al,byte ptr [rWork1]	; b
	CheckPtr	CHECKPRIV,<rWork2>,<0>,<rY_V1331Body>,<>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork2]	; c
	add	ebx,eax			; b + c
	lea	ebx,[ebx][ebx * 2]	; 3b + 3c
	CheckPtr	CHECKPRIV,<rWork0>,<0>,<rY_V1331Body>,<>
	xor	eax,eax
	mov	al,byte ptr [rWork0]	; a
	add	eax,ebx			; a + 3b + 3c
	CheckPtr	CHECKPRIV,<rWork3>,<0>,<rY_V1331Body>,<>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork3]	; d
	add	eax,ebx			; a + 3b + 3c + d

	add	ax,3			; normalize
	shr	eax,3			; AL = pixel to write
	CheckPtr	CHECKSMOOTH,<rDest>,<0>,<rY_V1331Body>,<>
	mov	ds:[rDest],al		; store it

	inc	rWork0			; bump pointers
	inc	rWork1
	inc	rWork2
	inc	rWork3

	mov	eax,rDest
	shr	eax,1
	sbb	eax,eax
	and	eax,6			; (currently on odd pixel) ? 6 : 0
	inc	rDest			; point to next output pixel
	add	rDest,eax		; adjust to next pair if last was odd

	dec	ds:cntHLoop		; drop count
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	jnz	near ptr baseFRAGTEXT32
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_USE_STOREDPC0>,<4>

	xchg	rDest,ds:oSmooth_1Mod2	; percolate smooth ->s up
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	ds:oSmooth_0Mod2,rDest	; will be next -> smooth list Ys
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>

	mov	eax,ds:oWS0Current	; swizzle ->s for next time
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	xchg	eax,ds:oWS2Current
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	ds:oWS0Current,eax
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>

	mov	eax,ds:oWS1Current	; swizzle ->s for next time
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	xchg	eax,ds:oWS3Current
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	ds:oWS1Current,eax
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>

	dec	ds:cntVLoop		; drop count
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_DSREL32>,<4>
	jnz	near ptr baseFRAGTEXT32
	FRAGFIXUP	<rY_V1331Body>,<FIXUP_USE_STOREDPC1>,<4>
FRAGENDP	rY_V1331Body

;----------------------------------------------------------------------------
; Code to generate smooth vector UV's from detail vector UV's
;----------------------------------------------------------------------------

FRAGPROC	rUV_H1331Init
	movzx	eax,ds:tileHeight	; get # scans in this tile
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<4>
	shr	eax,2			; # out scans is 1/2 input
	mov	ds:cntVLoop,eax
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<4>

	mov	ds:oWork,0
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_WORKPLUS>,<4>

	mov	eax,ds:oDetail		; fetch -> detail list
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<4>
	add	eax,4			; -> U,V in first detail vector
	mov	ds:oDetail_0Mod2,eax	; 0 mod 2 scan -> in detail list
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<4>
	add	eax,16			; skip UV + YYYYUV + YYYY
	mov	ds:oDetail_1Mod2,eax	; 1 mod 2 scan -> in detail list
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<4>

	mov	eax,ds:oSmooth		; fetch -> smooth list
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<4>
	add	eax,4			; -> U,V in first smooth vector
	mov	ds:oSmooth_0Mod2,eax	; 0 mod 1 scan -> in smooth list
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<4>

	mov	ds:oWS1Current,0
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS2Current,1
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS3Current,2
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_WORKPLUS>,<4>
	mov	ds:oWS0Current,3
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_WORKPLUS>,<4>

	mov	ds:firstTime,1
	FRAGFIXUP	<rUV_H1331Init>,<FIXUP_DSREL32>,<8>
FRAGENDP	rUV_H1331Init

FRAGPROC	rUV_H1331Start
	mov	rWork,ds:oWork		; fetch -> current scan in work buffer
	FRAGFIXUP	<rUV_H1331Start>,<FIXUP_DSREL32>,<4>
	lea	eax,[rWork][80000001h]	; calculate -> next scan
	FRAGFIXUP	<rUV_H1331Start>,<FIXUP_WORKYSTEP>,<4>
	cmp	eax,80000004h		; have we rolled around?
	FRAGFIXUP	<rUV_H1331Start>,<FIXUP_WORKPLUS>,<4>
	jb	rUV_H1331Start_0	; no -- haven't rolled around yet...
	mov	eax,80000000h		; yes -- get -> to start of work buffer
	FRAGFIXUP	<rUV_H1331Start>,<FIXUP_WORKPLUS>,<4>
rUV_H1331Start_0:
	mov	ds:oWork,eax		; save -> next scan in work buffer
	FRAGFIXUP	<rUV_H1331Start>,<FIXUP_DSREL32>,<4>

	mov	ds:hzSwizzle,10101010b	; every other time through loop
	FRAGFIXUP	<rUV_H1331Start>,<FIXUP_DSREL32>,<5>
	mov	rDetail,ds:oDetail_0Mod2; -> next scan in detail list
	FRAGFIXUP	<rUV_H1331Start>,<FIXUP_DSREL32>,<4>

	mov	ds:cntHLoop,80000001h	; 1/4 original width # pairs
	FRAGFIXUP	<rUV_H1331Start>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rUV_H1331Start>,<FIXUP_WORKWIDTH>,<4>
FRAGENDP	rUV_H1331Start

FRAGPROC	rUV_H1331End
;FRAGPROC	rUV_H134
	mov	rYUV_16,rU2_16		; get last component
	shr	rU2_32,16		; access to component we'll write
	add	rU2_16,rYUV_16		; add last component
	add	rU2_32,3		; normalize
	shr	rU2_32,3
	CheckPtr	CHECKPRIV,rWork,<0>,<rUV_H1331End>,<>
	mov	[rWork],rU2_16_lo	; save U

	mov	rYUV_16,rV2_16		; get last component
	shr	rV2_32,16		; access to component we'll write
	add	rV2_16,rYUV_16		; add last component
	add	rV2_32,3		; normalize
	shr	rV2_32,3
	CheckPtr CHECKPRIV,rWork,<80000004h>,<rUV_H1331End>,<FIXUP_WORKYSTEP>
	mov	[rWork][80000004h],rV2_16_lo; save V
	FRAGFIXUP	<rUV_H1331End>,<FIXUP_WORKYSTEP>,<4>
;FRAGENDP	rUV_H134

	xchg	rDetail,ds:oDetail_1Mod2
	FRAGFIXUP	<rUV_H1331End>,<FIXUP_DSREL32>,<4>
	mov	ds:oDetail_0Mod2,rDetail; will be next -> detail list Ys
	FRAGFIXUP	<rUV_H1331End>,<FIXUP_DSREL32>,<4>
FRAGENDP	rUV_H1331End

FRAGPROC	rUV_H431
	CheckPtr	CHECKDETAIL,<rDetail>,<0>,<rUV_H431>,<>
	movzx	rU2_32,ds:byte ptr [rDetail]; the 4 of the 4:3:1
	shl	rU2_32,2

	CheckPtr	CHECKDETAIL,<rDetail>,<1>,<rUV_H431>,<>
	movzx	rV2_32,ds:byte ptr [rDetail][1]; the 4 of the 4:3:1
	shl	rV2_32,2

	add	rDetail,8		; point to next UV pair for this scan
	ror	ds:hzSwizzle,1		; time to skip over 2 VECTOR fields?
	FRAGFIXUP	<rUV_H431>,<FIXUP_DSREL32>,<4>
	sbb	eax,eax			; (yes) ? 255 : 0
	and	eax,16			; (yes) ? (2 * sizeof(VECTOR)) : 0
	add	rDetail,eax		; update -> next VECTOR field

	jmp	near ptr baseFRAGTEXT32; jump into loop...
	FRAGFIXUP	<rUV_H431>,<FIXUP_JMPLP1>,<4>
FRAGENDP	rUV_H431

FRAGPROC	rUV_H1331B
	CheckPtr	CHECKDETAIL,<rDetail>,<1>,<rUV_H1331B>,<>
	mov	rYUV_16,ds:[rDetail]	; fetch next UV

	ror	rU2_32,16		; access to component we'll write
	add	rU2_16_lo,rV		; add last component
	adc	rU2_16_hi,0
	add	rU2_16,3		; normalize
	shr	rU2_16,3
	CheckPtr	CHECKPRIV,rWork,<0>,<rUV_H1331B>,<>
	mov	[rWork],rU2_16_lo	; save U2
	ror	rU2_32,16
	add	rU2_16_lo,rV		; 3 * new U added into old
	adc	rU2_16_hi,0
	add	rU2_16_lo,rV
	adc	rU2_16_hi,0
	add	rU2_16_lo,rV
	adc	rU2_16_hi,0

	ror	rV2_32,16		; access to component we'll write
	add	rV2_16_lo,rU		; add last component
	adc	rV2_16_hi,0
	add	rV2_16,3		; normalize
	shr	rV2_16,3
	CheckPtr CHECKPRIV,rWork,<80000004h>,<rUV_H1331B>,<FIXUP_WORKYSTEP>
	mov	[rWork][80000004h],rV2_16_lo; save V2
	FRAGFIXUP	<rUV_H1331B>,<FIXUP_WORKYSTEP>,<4>
	ror	rV2_32,16
	add	rV2_16_lo,rU		; 3 * new V added into old
	adc	rV2_16_hi,0
	add	rV2_16_lo,rU
	adc	rV2_16_hi,0
	add	rV2_16_lo,rU
	adc	rV2_16_hi,0

	inc	rWork			; -> next work buffer location

	add	rDetail,8		; point to next UV pair for this scan
	ror	ds:hzSwizzle,1		; time to skip over 2 VECTOR fields?
	FRAGFIXUP	<rUV_H1331B>,<FIXUP_DSREL32>,<4>
	sbb	eax,eax			; (yes) ? 255 : 0
	and	eax,16			; (yes) ? (2 * sizeof(VECTOR)) : 0
	add	rDetail,eax		; update -> next VECTOR field
FRAGENDP	rUV_H1331B

FRAGPROC	rUV_H1331C
	CheckPtr	CHECKDETAIL,rDetail,<1>,<rUV_H1331C>,<>
	mov	rYUV_16,ds:[rDetail]	; fetch next UV

	add	rU2_16_lo,rV		; 3 * new U added into old
	adc	rU2_16_hi,0
	add	rU2_16_lo,rV
	adc	rU2_16_hi,0
	add	rU2_16_lo,rV
	adc	rU2_16_hi,0
	shl	rU2_32,16		; push it down the pipeline
	movzx	rU2_16,rV		; save new component

	add	rV2_16_lo,rU		; 3 * new V added into old
	adc	rV2_16_hi,0
	add	rV2_16_lo,rU
	adc	rV2_16_hi,0
	add	rV2_16_lo,rU
	adc	rV2_16_hi,0
	shl	rV2_32,16		; push it down the pipeline
	movzx	rV2_16,rU		; save new component
FRAGENDP	rUV_H1331C

FRAGPROC	rUV_V1331Body
	mov	rDest,ds:oSmooth_0Mod2	; fetch -> smooth UV's
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>

	mov	rWork0,ds:oWS0Current	; load up ->s to each of 4 scans
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	rWork1,ds:oWS1Current
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	rWork2,ds:oWS2Current
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	rWork3,ds:oWS3Current
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>

	dec	ds:firstTime		; first time through?
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>
	jnz	@F			; no...
	mov	rWork0,rWork1		; first,next the same
@@:

	cmp	ds:cntVLoop,1		; last time through?
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<5>
	jne	@F			; no...
	mov	rWork3,rWork2		; last,previous the same
@@:

	mov	ds:cntHLoop,80000001h	; save ystep (== pixels) for loop cnt
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<8>
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_WORKWIDTH>,<4>

	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_STOREPC0>,<0>

	CheckPtr CHECKPRIV,<rWork1>,<0>,<rUV_V1331Body>,<>
	xor	eax,eax
	mov	al,byte ptr [rWork1]	; b
	CheckPtr	CHECKPRIV,<rWork2>,<0>,<rUV_V1331Body>,<>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork2]	; c
	add	ebx,eax			; b + c
	lea	ebx,[ebx][ebx * 2]	; 3b + 3c
	CheckPtr	CHECKPRIV,<rWork0>,<0>,<rUV_V1331Body>,<>
	xor	eax,eax
	mov	al,byte ptr [rWork0]	; a
	add	eax,ebx			; a + 3b + 3c
	CheckPtr	CHECKPRIV,<rWork3>,<0>,<rUV_V1331Body>,<>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork3]	; d
	add	eax,ebx			; a + 3b + 3c + d

	add	ax,3			; normalize
	shr	eax,3			; AL = pixel to write
	CheckPtr	CHECKSMOOTH,<rDest>,<0>,<rUV_V1331Body>,<>
	mov	ds:[rDest],al		; store U

    CheckPtr CHECKPRIV,<rWork1>,<80000004h>,<rUV_V1331Body>,<FIXUP_WORKYSTEP>
	xor	eax,eax
	mov	al,byte ptr [rWork1][80000004h]; b
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_WORKYSTEP>,<4>
    CheckPtr CHECKPRIV,<rWork2>,<80000004h>,<rUV_V1331Body>,<FIXUP_WORKYSTEP>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork2][80000004h]; c
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_WORKYSTEP>,<4>
	add	ebx,eax			; b + c
	lea	ebx,[ebx][ebx * 2]	; 3b + 3c
    CheckPtr CHECKPRIV,<rWork0>,<80000004h>,<rUV_V1331Body>,<FIXUP_WORKYSTEP>
	xor	eax,eax
	mov	al,byte ptr [rWork0][80000004h]; a
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_WORKYSTEP>,<4>
	add	eax,ebx			; a + 3b + 3c
    CheckPtr CHECKPRIV,<rWork3>,<80000004h>,<rUV_V1331Body>,<FIXUP_WORKYSTEP>
	xor	ebx,ebx
	mov	bl,byte ptr [rWork3][80000004h]; d
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_WORKYSTEP>,<4>
	add	eax,ebx			; a + 3b + 3c + d

	add	ax,3			; normalize
	shr	eax,3			; AL = pixel to write
	CheckPtr	CHECKSMOOTH,<rDest>,<1>,<rUV_V1331Body>,<>
	mov	ds:[rDest][1],al	; store V

	inc	rWork0			; bump work buffer pointers
	inc	rWork1
	inc	rWork2
	inc	rWork3

	add	rDest,8			; skip UV + YYYY in smooth list
	mov	ds:oSmooth_0Mod2,rDest	; will be next -> smooth list UVs
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>

	dec	ds:cntHLoop		; drop count
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>
	jnz	near ptr baseFRAGTEXT32
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_USE_STOREDPC0>,<4>

	mov	eax,ds:oWS0Current	; swizzle ->s for next time
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>
	xchg	eax,ds:oWS2Current
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	ds:oWS0Current,eax
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>

	mov	eax,ds:oWS1Current	; swizzle ->s for next time
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>
	xchg	eax,ds:oWS3Current
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>
	mov	ds:oWS1Current,eax
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>

	dec	ds:cntVLoop		; drop count
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_DSREL32>,<4>
	jnz	near ptr baseFRAGTEXT32
	FRAGFIXUP	<rUV_V1331Body>,<FIXUP_USE_STOREDPC1>,<4>
FRAGENDP	rUV_V1331Body


;--------------------------------------------------------------------------
;
; Master tables of fragment names, lengths, fixups
;
; This data is what is seen externally by the compiler
;
;--------------------------------------------------------------------------

FRAGDECLARE	cEntry

FRAGDECLARE	H1331Init
FRAGDECLARE	H1331Start
FRAGDECLARE	H1331FetchPixel8
FRAGDECLARE	H1331FetchPixel15
FRAGDECLARE	H1331FetchPixel16
FRAGDECLARE	H1331FetchPixel24
FRAGDECLARE	H1331FetchPixel32
FRAGDECLARE	H1331ToYUV
FRAGDECLARE	H1331StoreY0
FRAGDECLARE	H1331StoreY1
FRAGDECLARE	H431
FRAGDECLARE	H1331B
FRAGDECLARE	H1331C
FRAGDECLARE	H1331Loop
FRAGDECLARE	H1331End

FRAGDECLARE	V1331EarlyOut

FRAGDECLARE	V1331FetchY2Dest
FRAGDECLARE	V1331FetchU2Dest
FRAGDECLARE	V1331FetchV2Dest
FRAGDECLARE	V1331Body
ifdef	DEBUG
FRAGDECLARE	V1331Body_Y2
endif
FRAGDECLARE	V1331IncY2Dest
FRAGDECLARE	V1331IncU2V2Dest
FRAGDECLARE	V1331Loop
FRAGDECLARE	V1331StoreY2Dest
FRAGDECLARE	V1331StoreU2Dest
FRAGDECLARE	V1331StoreV2Dest

FRAGDECLARE	HVLoop

FRAGDECLARE	U2V2HInit
FRAGDECLARE	U2V2HStart
FRAGDECLARE	U2V2H431
FRAGDECLARE	U2V2H1331B
FRAGDECLARE	U2V2H1331C
FRAGDECLARE	U2V2HLoop
FRAGDECLARE	U2V2HEnd

FRAGDECLARE	U2V2VBody

FRAGDECLARE	cExit

FRAGDECLARE	rY_H1331Init
FRAGDECLARE	rY_H1331Start
FRAGDECLARE	rY_H1331End
FRAGDECLARE	rY_H431
FRAGDECLARE	rY_H1331B
FRAGDECLARE	rY_H1331C
FRAGDECLARE	rY_V1331Body

FRAGDECLARE	rUV_H1331Init
FRAGDECLARE	rUV_H1331Start
FRAGDECLARE	rUV_H1331End
FRAGDECLARE	rUV_H431
FRAGDECLARE	rUV_H1331B
FRAGDECLARE	rUV_H1331C
FRAGDECLARE	rUV_V1331Body

ifndef	WIN32
_FRAGTEXT32	ends
else
_TEXT		ends
endif

	end
