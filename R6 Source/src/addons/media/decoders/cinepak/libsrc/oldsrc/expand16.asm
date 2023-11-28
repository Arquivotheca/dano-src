; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/expand16.asm 1.4 1994/09/22 17:17:00 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: expand16.asm $
; Revision 1.4  1994/09/22 17:17:00  bog
; Fix up align and refs to kGreyBookBit.
; Revision 1.3  1994/06/23  14:13:00  bog
; Change movzx into xor/mov pair.
; 
; Revision 1.2  1994/05/06  13:41:20  bog
; Play back black and white movies.
; 
; Revision 1.1  1993/10/12  17:25:22  bog
; Initial revision
; 


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	include	cv.inc
	include	cvdecomp.inc


	.386

	SEG32


	db	000h,000h,000h,000h,000h,000h,000h,000h;underflow
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h

ScaleTable5	label byte
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	001h,001h,001h,001h,001h,001h,001h,001h
	db	002h,002h,002h,002h,002h,002h,002h,002h
	db	003h,003h,003h,003h,003h,003h,003h,003h
	db	004h,004h,004h,004h,004h,004h,004h,004h
	db	005h,005h,005h,005h,005h,005h,005h,005h
	db	006h,006h,006h,006h,006h,006h,006h,006h
	db	007h,007h,007h,007h,007h,007h,007h,007h
	db	008h,008h,008h,008h,008h,008h,008h,008h
	db	009h,009h,009h,009h,009h,009h,009h,009h
	db	00ah,00ah,00ah,00ah,00ah,00ah,00ah,00ah
	db	00bh,00bh,00bh,00bh,00bh,00bh,00bh,00bh
	db	00ch,00ch,00ch,00ch,00ch,00ch,00ch,00ch
	db	00dh,00dh,00dh,00dh,00dh,00dh,00dh,00dh
	db	00eh,00eh,00eh,00eh,00eh,00eh,00eh,00eh
	db	00fh,00fh,00fh,00fh,00fh,00fh,00fh,00fh
	db	010h,010h,010h,010h,010h,010h,010h,010h
	db	011h,011h,011h,011h,011h,011h,011h,011h
	db	012h,012h,012h,012h,012h,012h,012h,012h
	db	013h,013h,013h,013h,013h,013h,013h,013h
	db	014h,014h,014h,014h,014h,014h,014h,014h
	db	015h,015h,015h,015h,015h,015h,015h,015h
	db	016h,016h,016h,016h,016h,016h,016h,016h
	db	017h,017h,017h,017h,017h,017h,017h,017h
	db	018h,018h,018h,018h,018h,018h,018h,018h
	db	019h,019h,019h,019h,019h,019h,019h,019h
	db	01ah,01ah,01ah,01ah,01ah,01ah,01ah,01ah
	db	01bh,01bh,01bh,01bh,01bh,01bh,01bh,01bh
	db	01ch,01ch,01ch,01ch,01ch,01ch,01ch,01ch
	db	01dh,01dh,01dh,01dh,01dh,01dh,01dh,01dh
	db	01eh,01eh,01eh,01eh,01eh,01eh,01eh,01eh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh

	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh;overflow
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh


	db	000h,000h,000h,000h,000h,000h,000h,000h;underflow
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	000h,000h,000h,000h,000h,000h,000h,000h

ScaleTable6	label byte
	db	000h,000h,000h,000h,001h,001h,001h,001h
	db	002h,002h,002h,002h,003h,003h,003h,003h
	db	004h,004h,004h,004h,005h,005h,005h,005h
	db	006h,006h,006h,006h,007h,007h,007h,007h
	db	008h,008h,008h,008h,009h,009h,009h,009h
	db	00ah,00ah,00ah,00ah,00bh,00bh,00bh,00bh
	db	00ch,00ch,00ch,00ch,00dh,00dh,00dh,00dh
	db	00eh,00eh,00eh,00eh,00fh,00fh,00fh,00fh
	db	010h,010h,010h,010h,011h,011h,011h,011h
	db	012h,012h,012h,012h,013h,013h,013h,013h
	db	014h,014h,014h,014h,015h,015h,015h,015h
	db	016h,016h,016h,016h,017h,017h,017h,017h
	db	018h,018h,018h,018h,019h,019h,019h,019h
	db	01ah,01ah,01ah,01ah,01bh,01bh,01bh,01bh
	db	01ch,01ch,01ch,01ch,01dh,01dh,01dh,01dh
	db	01eh,01eh,01eh,01eh,01fh,01fh,01fh,01fh
	db	020h,020h,020h,020h,021h,021h,021h,021h
	db	022h,022h,022h,022h,023h,023h,023h,023h
	db	024h,024h,024h,024h,025h,025h,025h,025h
	db	026h,026h,026h,026h,027h,027h,027h,027h
	db	028h,028h,028h,028h,029h,029h,029h,029h
	db	02ah,02ah,02ah,02ah,02bh,02bh,02bh,02bh
	db	02ch,02ch,02ch,02ch,02dh,02dh,02dh,02dh
	db	02eh,02eh,02eh,02eh,02fh,02fh,02fh,02fh
	db	030h,030h,030h,030h,031h,031h,031h,031h
	db	032h,032h,032h,032h,033h,033h,033h,033h
	db	034h,034h,034h,034h,035h,035h,035h,035h
	db	036h,036h,036h,036h,037h,037h,037h,037h
	db	038h,038h,038h,038h,039h,039h,039h,039h
	db	03ah,03ah,03ah,03ah,03bh,03bh,03bh,03bh
	db	03ch,03ch,03ch,03ch,03dh,03dh,03dh,03dh
	db	03eh,03eh,03eh,03eh,03fh,03fh,03fh,03fh

	db	03fh,03fh,03fh,03fh,03fh,03fh,03fh,03fh;overflow
	db	03fh,03fh,03fh,03fh,03fh,03fh,03fh,03fh
	db	03fh,03fh,03fh,03fh,03fh,03fh,03fh,03fh
	db	03fh,03fh,03fh,03fh,03fh,03fh,03fh,03fh
	db	03fh,03fh,03fh,03fh,03fh,03fh,03fh,03fh
	db	03fh,03fh,03fh,03fh,03fh,03fh,03fh,03fh
	db	03fh,03fh,03fh,03fh,03fh,03fh,03fh,03fh
	db	03fh,03fh,03fh,03fh,03fh,03fh,03fh,03fh


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


CVDecompEntry	ExpandDetailCodeBook16,far

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
	mov	al,byte ptr[esi][1]	; eax:  Y1

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi]	; eax:  Y0

	shl	edx,5
	or	dl,ScaleTable5[ebx][eax]; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xx000RRR RRGGGGGG BBBBBrrr rrgggggg
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb
	Ref	TEXT32

	mov	[edi],edx		; save RGB1:RGB0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][3]	; eax:  Y3

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][2]	; eax:  Y2

	shl	edx,5
	or	dl,ScaleTable5[ebx][eax]; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xx000RRR RRGGGGGG BBBBBrrr rrgggggg
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb
	Ref	TEXT32

	mov	[edi][4],edx		; save RGB3:RGB2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV
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

	jmp	short DetailPartialTestSwitch


	align	4

DetailPartialYUVLoop:

	mov	eax,012345678h	; self mod switches here
DetailPartialSwitches	label	dword

;     eax	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,eax		; replace this index?

DetailPartialTestSwitch:

	mov	DetailPartialSwitches[-4],eax; save for next iteration
	Ref	Motion

	jnc	DetailPartialYUVSkip

	jz	DetailPartialLoadSwitches

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
	mov	al,byte ptr[esi][1]	; eax:  Y1

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi]	; eax:  Y0

	shl	edx,5
	or	dl,ScaleTable5[ebx][eax]; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xx000RRR RRGGGGGG BBBBBrrr rrgggggg
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb
	Ref	TEXT32

	mov	[edi],edx		; save RGB1:RGB0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][3]	; eax:  Y3

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][2]	; eax:  Y2

	shl	edx,5
	or	dl,ScaleTable5[ebx][eax]; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xx000RRR RRGGGGGG BBBBBrrr rrgggggg
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb
	Ref	TEXT32

	mov	[edi][4],edx		; save RGB3:RGB2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV

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

  ; we take the incoming YYYYUV at [esi] and develop a 2x2 patch:
  ;
  ;     RGB0  RGB1
  ;     RGB2  RGB3
  ;
  ; as seen on the screen.

	shld	ebx,edx,5	; RGB.3:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB.3:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB.3:  ........ ........ rrrrrggg gggbbbbb

	shl	edx,8		;         22222222 11111111 00000000 ........

	shld	ebx,edx,5	; RGB32:  ........ ...rrrrr ggggggbb bbbrrrrr
	shld	ebx,edx,6	; RGB32:  .....rrr rrgggggg bbbbbrrr rrgggggg
	shld	ebx,edx,5	; RGB32:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	shl	edx,8		;         11111111 00000000 ........ ........

	mov	[edi][4],ebx	; RGB3, RGB2

	shld	ebx,edx,5	; RGB.1:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB.1:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB.1:  ........ ........ rrrrrggg gggbbbbb

	shl	edx,8		;         00000000 ........ ........ ........

	shld	ebx,edx,5	; RGB10:  ........ ...rrrrr ggggggbb bbbrrrrr
	shld	ebx,edx,6	; RGB10:  .....rrr rrgggggg bbbbbrrr rrgggggg
	shld	ebx,edx,5	; RGB10:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	mov	[edi][0],ebx	; RGB1, RGB0

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

  ; we take the incoming YYYYUV at [esi] and develop a 2x2 patch:
  ;
  ;     RGB0  RGB1
  ;     RGB2  RGB3
  ;
  ; as seen on the screen.

	shld	ebx,edx,5	; RGB.3:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB.3:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB.3:  ........ ........ rrrrrggg gggbbbbb

	shl	edx,8		;         22222222 11111111 00000000 ........

	shld	ebx,edx,5	; RGB32:  ........ ...rrrrr ggggggbb bbbrrrrr
	shld	ebx,edx,6	; RGB32:  .....rrr rrgggggg bbbbbrrr rrgggggg
	shld	ebx,edx,5	; RGB32:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	shl	edx,8		;         11111111 00000000 ........ ........

	mov	[edi][4],ebx	; RGB3, RGB2

	shld	ebx,edx,5	; RGB.1:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB.1:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB.1:  ........ ........ rrrrrggg gggbbbbb

	shl	edx,8		;         00000000 ........ ........ ........

	shld	ebx,edx,5	; RGB10:  ........ ...rrrrr ggggggbb bbbrrrrr
	shld	ebx,edx,6	; RGB10:  .....rrr rrgggggg bbbbbrrr rrgggggg
	shld	ebx,edx,5	; RGB10:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	mov	[edi][0],ebx	; RGB1, RGB0

GreyDPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyDPartialYUVLoop

	jmp	DetailQuit

	endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandDetailCodeBook16	endp



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


CVDecompEntry	ExpandSmoothCodeBook16,far

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

  ; we take the incoming YYYYUV at [esi] and develop a 4x4 patch:
  ;
  ;     RGB0 RGB0  RGB1 RGB1
  ;     RGB0 RGB0  RGB1 RGB1
  ;     RGB2 RGB2  RGB3 RGB3
  ;     RGB2 RGB2  RGB3 RGB3
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

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi],eax		; save RGB0:RGB0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][1]	; eax:  Y1

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi][4],eax		; save RGB1:RGB1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][2]	; eax:  Y2

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi][8],eax		; save RGB2:RGB2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][3]	; eax:  Y3

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi][12],eax		; save RGB3:RGB3

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV
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

	jmp	short SmoothPartialTestSwitch


	align	4

SmoothPartialYUVLoop:

	mov	eax,012345678h	; self mod switches here
SmoothPartialSwitches	label	dword

;     eax	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,eax		; replace this index?

SmoothPartialTestSwitch:

	mov	SmoothPartialSwitches[-4],eax; save for next iteration
	Ref	Motion

	jnc	SmoothPartialYUVSkip

	jz	SmoothPartialLoadSwitches

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

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi],eax		; save RGB0:RGB0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][1]	; eax:  Y1

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi][4],eax		; save RGB1:RGB1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][2]	; eax:  Y2

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi][8],eax		; save RGB2:RGB2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][3]	; eax:  Y3

	mov	dl,ScaleTable5[ebx][eax]; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR
	Ref	TEXT32

	shl	edx,6
	or	dl,ScaleTable6[ecx][eax]; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG
	Ref	TEXT32

	shl	edx,5
	or	dl,ScaleTable5[ebp][eax]; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB
	Ref	TEXT32

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi][12],eax		; save RGB3:RGB3

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV

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
;     ebp	swizzled bit switches for codes we do

  ; we take the incoming YYYYUV at [esi] and develop a 4x4 patch:
  ;
  ;     RGB0 RGB0  RGB1 RGB1
  ;     RGB0 RGB0  RGB1 RGB1

  ;     RGB2 RGB2  RGB3 RGB3
  ;     RGB2 RGB2  RGB3 RGB3
  ;
  ; as seen on the screen.

	shld	ebx,edx,5	; RGB3:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB3:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB3:  ........ ........ rrrrrggg gggbbbbb

	mov	ecx,ebx		; RGB3:  ........ ........ rrrrrggg gggbbbbb
	shl	ecx,16		; RGB3:  rrrrrggg gggbbbbb ........ ........
	mov	cx,bx		; RGB3:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	shl	edx,8		;        22222222 11111111 00000000 ........

	mov	[edi][12],ecx	; RGB3

	shld	ebx,edx,5	; RGB2:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB2:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB2:  ........ ........ rrrrrggg gggbbbbb

	mov	ecx,ebx		; RGB2:  ........ ........ rrrrrggg gggbbbbb
	shl	ecx,16		; RGB2:  rrrrrggg gggbbbbb ........ ........
	mov	cx,bx		; RGB2:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	shl	edx,8		;        11111111 00000000 ........ ........

	mov	[edi][8],ecx	; RGB2

	shld	ebx,edx,5	; RGB1:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB1:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB1:  ........ ........ rrrrrggg gggbbbbb

	mov	ecx,ebx		; RGB1:  ........ ........ rrrrrggg gggbbbbb
	shl	ecx,16		; RGB1:  rrrrrggg gggbbbbb ........ ........
	mov	cx,bx		; RGB1:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	shl	edx,8		;        00000000 ........ ........ ........

	mov	[edi][4],ecx	; RGB1

	shld	ebx,edx,5	; RGB0:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB0:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB0:  ........ ........ rrrrrggg gggbbbbb

	mov	ecx,ebx		; RGB0:  ........ ........ rrrrrggg gggbbbbb
	shl	ecx,16		; RGB0:  rrrrrggg gggbbbbb ........ ........
	mov	cx,bx		; RGB0:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	mov	[edi],ecx	; RGB0

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

  ; we take the incoming YYYYUV at [esi] and develop a 4x4 patch:
  ;
  ;     RGB0 RGB0  RGB1 RGB1
  ;     RGB0 RGB0  RGB1 RGB1

  ;     RGB2 RGB2  RGB3 RGB3
  ;     RGB2 RGB2  RGB3 RGB3
  ;
  ; as seen on the screen.

	shld	ebx,edx,5	; RGB3:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB3:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB3:  ........ ........ rrrrrggg gggbbbbb

	mov	ecx,ebx		; RGB3:  ........ ........ rrrrrggg gggbbbbb
	shl	ecx,16		; RGB3:  rrrrrggg gggbbbbb ........ ........
	mov	cx,bx		; RGB3:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	shl	edx,8		;        22222222 11111111 00000000 ........

	mov	[edi][12],ecx	; RGB3

	shld	ebx,edx,5	; RGB2:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB2:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB2:  ........ ........ rrrrrggg gggbbbbb

	mov	ecx,ebx		; RGB2:  ........ ........ rrrrrggg gggbbbbb
	shl	ecx,16		; RGB2:  rrrrrggg gggbbbbb ........ ........
	mov	cx,bx		; RGB2:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	shl	edx,8		;        11111111 00000000 ........ ........

	mov	[edi][8],ecx	; RGB2

	shld	ebx,edx,5	; RGB1:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB1:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB1:  ........ ........ rrrrrggg gggbbbbb

	mov	ecx,ebx		; RGB1:  ........ ........ rrrrrggg gggbbbbb
	shl	ecx,16		; RGB1:  rrrrrggg gggbbbbb ........ ........
	mov	cx,bx		; RGB1:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	shl	edx,8		;        00000000 ........ ........ ........

	mov	[edi][4],ecx	; RGB1

	shld	ebx,edx,5	; RGB0:  ........ ........ ........ ...rrrrr
	shld	ebx,edx,6	; RGB0:  ........ ........ .....rrr rrgggggg
	shld	ebx,edx,5	; RGB0:  ........ ........ rrrrrggg gggbbbbb

	mov	ecx,ebx		; RGB0:  ........ ........ rrrrrggg gggbbbbb
	shl	ecx,16		; RGB0:  rrrrrggg gggbbbbb ........ ........
	mov	cx,bx		; RGB0:  rrrrrggg gggbbbbb rrrrrggg gggbbbbb

	mov	[edi],ecx	; RGB0

GreySPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreySPartialYUVLoop

	jmp	SmoothQuit

	endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandSmoothCodeBook16	endp

	EndMotion

	ENDSEG32

ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	Expand16

_DATA	ends

	end
