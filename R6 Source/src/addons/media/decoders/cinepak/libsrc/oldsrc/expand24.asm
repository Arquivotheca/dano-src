; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/expand24.asm 2.9 1994/09/22 17:17:05 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: expand24.asm $
; Revision 2.9  1994/09/22 17:17:05  bog
; Fix up align and refs to kGreyBookBit.
; Revision 2.8  1994/06/23  14:13:04  bog
; Change movzx into xor/mov pair.
; 
; Revision 2.7  1994/05/06  13:41:24  bog
; Play back black and white movies.
; 
; Revision 2.6  1993/08/10  11:04:47  bog
; Fix some comments.
; 
; Revision 2.5  1993/08/04  17:09:04  bog
; A little faster.
; 
; Revision 2.4  1993/07/03  11:44:01  geoffs
; All _DATA segment declarations needed USE16 if WIN16 build
; 
; Revision 2.3  93/07/02  16:18:38  geoffs
; Now compiles,runs under Windows NT
; 
; Revision 2.2  93/06/09  17:58:55  bog
; Support decompression to 32 bit DIBs.
; 
; Revision 2.1  93/06/01  14:49:38  bog
; Compiled, flat decompress assembler.
; 
; Revision 2.0  93/06/01  14:14:56  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.15  93/04/21  15:48:51  bog
; Fix up copyright and disclaimer.
;
; Revision 1.14  92/12/21  11:37:32  bog
; Split into IC and CV layers.
;
; Revision 1.13  92/12/17  16:36:04  timr
; Correct computation of the "B" delta.  Fixes 24-bit color shift.
;
; Revision 1.12  92/12/01  11:07:00  bog
; 8 bpp dither path working; colors are a little strong.
;
; Revision 1.11  92/11/29  15:53:02  bog
; Cleaned up 24bpp path.  Now it's faster.
;
; Revision 1.10  92/11/07  17:00:45  bog
; Problems with 2nd tile; USE32 works.
;
; Revision 1.9  92/11/05  16:56:30  bog
; USE32 segment for Draw and Expand works.
;
; Revision 1.8  92/11/02  09:23:34  bog
; Fix colors.  U and V were switched.
;
; Revision 1.7  92/11/02  01:26:15  bog
; CompactVideo healthy except that colors are wacked out.
;
; Revision 1.6  92/11/01  23:46:40  bog
; CompactVideo decompression works!
; 1.  Colors are incorrect.
; 2.  Still need to do DrawSmooth and DrawInter.
;
; Revision 1.5  92/11/01  21:00:06  bog
; DrawKey24 compiles.
;
; Revision 1.4  92/11/01  15:37:11  bog
; Expanding codebooks seems to work.  At least it traverses a file.
;
; Revision 1.3  92/11/01  14:44:55  bog
; First successful compile with Expand24 done.
;
; Revision 1.2  92/11/01  13:13:16  bog
; Version with ScaleTable.
;
; Revision 1.1  92/10/31  23:11:56  bog
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

	public	Bounding24
Bounding24	label byte
	db	000h,001h,002h,003h,004h,005h,006h,007h
	db	008h,009h,00ah,00bh,00ch,00dh,00eh,00fh
	db	010h,011h,012h,013h,014h,015h,016h,017h
	db	018h,019h,01ah,01bh,01ch,01dh,01eh,01fh
	db	020h,021h,022h,023h,024h,025h,026h,027h
	db	028h,029h,02ah,02bh,02ch,02dh,02eh,02fh
	db	030h,031h,032h,033h,034h,035h,036h,037h
	db	038h,039h,03ah,03bh,03ch,03dh,03eh,03fh
	db	040h,041h,042h,043h,044h,045h,046h,047h
	db	048h,049h,04ah,04bh,04ch,04dh,04eh,04fh
	db	050h,051h,052h,053h,054h,055h,056h,057h
	db	058h,059h,05ah,05bh,05ch,05dh,05eh,05fh
	db	060h,061h,062h,063h,064h,065h,066h,067h
	db	068h,069h,06ah,06bh,06ch,06dh,06eh,06fh
	db	070h,071h,072h,073h,074h,075h,076h,077h
	db	078h,079h,07ah,07bh,07ch,07dh,07eh,07fh
	db	080h,081h,082h,083h,084h,085h,086h,087h
	db	088h,089h,08ah,08bh,08ch,08dh,08eh,08fh
	db	090h,091h,092h,093h,094h,095h,096h,097h
	db	098h,099h,09ah,09bh,09ch,09dh,09eh,09fh
	db	0a0h,0a1h,0a2h,0a3h,0a4h,0a5h,0a6h,0a7h
	db	0a8h,0a9h,0aah,0abh,0ach,0adh,0aeh,0afh
	db	0b0h,0b1h,0b2h,0b3h,0b4h,0b5h,0b6h,0b7h
	db	0b8h,0b9h,0bah,0bbh,0bch,0bdh,0beh,0bfh
	db	0c0h,0c1h,0c2h,0c3h,0c4h,0c5h,0c6h,0c7h
	db	0c8h,0c9h,0cah,0cbh,0cch,0cdh,0ceh,0cfh
	db	0d0h,0d1h,0d2h,0d3h,0d4h,0d5h,0d6h,0d7h
	db	0d8h,0d9h,0dah,0dbh,0dch,0ddh,0deh,0dfh
	db	0e0h,0e1h,0e2h,0e3h,0e4h,0e5h,0e6h,0e7h
	db	0e8h,0e9h,0eah,0ebh,0ech,0edh,0eeh,0efh
	db	0f0h,0f1h,0f2h,0f3h,0f4h,0f5h,0f6h,0f7h
	db	0f8h,0f9h,0fah,0fbh,0fch,0fdh,0feh,0ffh

	db	0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh;overflow
	db	0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh
	db	0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh
	db	0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh
	db	0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh
	db	0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh
	db	0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh
	db	0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh


	ifndef	NOBLACKWHITE

; GreyDwordLookup
;   Map Y in 0..255 to 4 identical indices in 10.245 in a dword.

	public	GreyDwordLookup
GreyDwordLookup	label	dword

i	=	0
	rept	256
	db	i,i,i,i
i	=	i+1
	endm

	endif


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


CVDecompEntry	ExpandDetailCodeBook24,far

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
	mov	al,byte ptr[esi]	; eax:  Y0

	mov	dh,Bounding24[ecx][eax]	; edx:  xxxxG0xx
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  xxxxG0B0
	Ref	TEXT32

	rol	edx,16

	mov	dl,Bounding24[ebx][eax]	; edx:  G0B0xxR0
	Ref	TEXT32

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][1]	; eax:  Y1

	mov	dh,Bounding24[ebp][eax]	; edx:  G0B0B1R0
	Ref	TEXT32

	rol	edx,16			; edx:  B1R0G0B0

	mov	[edi],edx

	mov	dh,Bounding24[ebx][eax]	; edx:  B1R0R1B0
	Ref	TEXT32

	mov	dl,Bounding24[ecx][eax]	; edx:  B1R0R1G1
	Ref	TEXT32

	rol	edx,16			; edx:  R1G1B1R0

	mov	[edi][4],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][2]	; eax:  Y2

	mov	dh,Bounding24[ecx][eax]	; edx:  xxxxG2xx
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  xxxxG2B2
	Ref	TEXT32

	rol	edx,16

	mov	dl,Bounding24[ebx][eax]	; edx:  G2B2xxR2
	Ref	TEXT32

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][3]	; eax:  Y3

	mov	dh,Bounding24[ebp][eax]	; edx:  G2B2B3R2
	Ref	TEXT32

	rol	edx,16			; edx:  B3R2G2B2

	mov	[edi][8],edx

	mov	dh,Bounding24[ebx][eax]	; edx:  B3R2R3B2
	Ref	TEXT32

	mov	dl,Bounding24[ecx][eax]	; edx:  B3R2R3G3
	Ref	TEXT32

	rol	edx,16			; edx:  R3G3B3R2

	mov	[edi][12],edx

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
	mov	al,byte ptr[esi]	; eax:  Y0

	mov	dh,Bounding24[ecx][eax]	; edx:  xxxxG0xx
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  xxxxG0B0
	Ref	TEXT32

	rol	edx,16

	mov	dl,Bounding24[ebx][eax]	; edx:  G0B0xxR0
	Ref	TEXT32

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][1]	; eax:  Y1

	mov	dh,Bounding24[ebp][eax]	; edx:  G0B0B1R0
	Ref	TEXT32

	rol	edx,16			; edx:  B1R0G0B0

	mov	[edi],edx

	mov	dh,Bounding24[ebx][eax]	; edx:  B1R0R1B0
	Ref	TEXT32

	mov	dl,Bounding24[ecx][eax]	; edx:  B1R0R1G1
	Ref	TEXT32

	rol	edx,16			; edx:  R1G1B1R0

	mov	[edi][4],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][2]	; eax:  Y2

	mov	dh,Bounding24[ecx][eax]	; edx:  xxxxG2xx
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  xxxxG2B2
	Ref	TEXT32

	rol	edx,16

	mov	dl,Bounding24[ebx][eax]	; edx:  G2B2xxR2
	Ref	TEXT32

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][3]	; eax:  Y3

	mov	dh,Bounding24[ebp][eax]	; edx:  G2B2B3R2
	Ref	TEXT32

	rol	edx,16			; edx:  B3R2G2B2

	mov	[edi][8],edx

	mov	dh,Bounding24[ebx][eax]	; edx:  B3R2R3B2
	Ref	TEXT32

	mov	dl,Bounding24[ecx][eax]	; edx:  B3R2R3G3
	Ref	TEXT32

	rol	edx,16			; edx:  R3G3B3R2

	mov	[edi][12],edx

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

	shrd	ebx,ecx,8		; Y1 Y0 Y0 Y0
	mov	cl,bl			; Y1 Y1 Y1 Y0

	mov	[edi],ebx		; Y1 Y0 Y0 Y0
	rol	edx,16
	mov	[edi][4],ecx		; Y1 Y1 Y1 Y0

	xor	ebx,ebx
	mov	bl,dl			; Y2
	xor	ecx,ecx
	mov	cl,dh			; Y3

	mov	ebx,GreyDwordLookup[ebx*4]; Y2 Y2 Y2 Y2
	Ref	TEXT32

	mov	ecx,GreyDwordLookup[ecx*4]; Y3 Y3 Y3 Y3
	Ref	TEXT32

	shrd	ebx,ecx,8		; Y3 Y2 Y2 Y2
	mov	cl,bl			; Y3 Y3 Y3 Y2

	mov	[edi][8],ebx		; Y3 Y2 Y2 Y2
	mov	[edi][12],ecx		; Y3 Y3 Y3 Y2


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

	shrd	ebx,ecx,8		; Y1 Y0 Y0 Y0
	mov	cl,bl			; Y1 Y1 Y1 Y0

	mov	[edi],ebx		; Y1 Y0 Y0 Y0
	rol	edx,16
	mov	[edi][4],ecx		; Y1 Y1 Y1 Y0

	xor	ebx,ebx
	mov	bl,dl			; Y2
	xor	ecx,ecx
	mov	cl,dh			; Y3

	mov	ebx,GreyDwordLookup[ebx*4]; Y2 Y2 Y2 Y2
	Ref	TEXT32

	mov	ecx,GreyDwordLookup[ecx*4]; Y3 Y3 Y3 Y3
	Ref	TEXT32

	shrd	ebx,ecx,8		; Y3 Y2 Y2 Y2
	mov	cl,bl			; Y3 Y3 Y3 Y2

	mov	[edi][8],ebx		; Y3 Y2 Y2 Y2
	mov	[edi][12],ecx		; Y3 Y3 Y3 Y2


GreyDPartialYUVSkip:
	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyDPartialYUVLoop

	jmp	DetailQuit

	endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandDetailCodeBook24	endp



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


CVDecompEntry	ExpandSmoothCodeBook24,far

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

	mov	dh,Bounding24[ebx][eax]	; edx:  xxxxR0xx
	Ref	TEXT32

	mov	dl,Bounding24[ecx][eax]	; edx:  xxxxR0G0
	Ref	TEXT32

	rol	edx,16			; edx:  R0G0xxxx

	mov	dl,Bounding24[ebp][eax]	; edx:  R0G0xxB0
	Ref	TEXT32

	mov	dh,dl			; edx:  R0G0B0B0

	ror	edx,8			; edx:  B0R0G0B0

	mov	[edi],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][1]	; eax:  Y1

	mov	dh,Bounding24[ecx][eax]	; edx:  xxxxG1xx
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  xxxxG1B1
	Ref	TEXT32

	rol	edx,16			; edx:  G1B1xxxx

	mov	dl,Bounding24[ebx][eax]	; edx:  G1B1xxR1
	Ref	TEXT32

	mov	dh,dl			; edx:  G1B1R1R1

	ror	edx,8			; edx:  R1G1B1R1

	mov	[edi][4],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][2]	; eax:  Y2

	mov	dh,Bounding24[ebx][eax]	; edx:  xxxxR2xx
	Ref	TEXT32

	mov	dl,Bounding24[ecx][eax]	; edx:  xxxxR2G2
	Ref	TEXT32

	rol	edx,16			; edx:  R2G2xxxx

	mov	dl,Bounding24[ebp][eax]	; edx:  R2G2xxB2
	Ref	TEXT32

	mov	dh,dl			; edx:  R2G2B2B2

	ror	edx,8			; edx:  B2R2G2B2

	mov	[edi][8],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][3]	; eax:  Y3

	mov	dh,Bounding24[ecx][eax]	; edx:  xxxxG3xx
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  xxxxG3B3
	Ref	TEXT32

	rol	edx,16			; edx:  G3B3xxxx

	mov	dl,Bounding24[ebx][eax]	; edx:  G3B3xxR3
	Ref	TEXT32

	mov	dh,dl			; edx:  G3B3R3R3

	ror	edx,8			; edx:  R3G3B3R3

	mov	[edi][12],edx

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

	mov	dh,Bounding24[ebx][eax]	; edx:  xxxxR0xx
	Ref	TEXT32

	mov	dl,Bounding24[ecx][eax]	; edx:  xxxxR0G0
	Ref	TEXT32

	rol	edx,16			; edx:  R0G0xxxx

	mov	dl,Bounding24[ebp][eax]	; edx:  R0G0xxB0
	Ref	TEXT32

	mov	dh,dl			; edx:  R0G0B0B0

	ror	edx,8			; edx:  B0R0G0B0

	mov	[edi],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][1]	; eax:  Y1

	mov	dh,Bounding24[ecx][eax]	; edx:  xxxxG1xx
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  xxxxG1B1
	Ref	TEXT32

	rol	edx,16			; edx:  G1B1xxxx

	mov	dl,Bounding24[ebx][eax]	; edx:  G1B1xxR1
	Ref	TEXT32

	mov	dh,dl			; edx:  G1B1R1R1

	ror	edx,8			; edx:  R1G1B1R1

	mov	[edi][4],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][2]	; eax:  Y2

	mov	dh,Bounding24[ebx][eax]	; edx:  xxxxR2xx
	Ref	TEXT32

	mov	dl,Bounding24[ecx][eax]	; edx:  xxxxR2G2
	Ref	TEXT32

	rol	edx,16			; edx:  R2G2xxxx

	mov	dl,Bounding24[ebp][eax]	; edx:  R2G2xxB2
	Ref	TEXT32

	mov	dh,dl			; edx:  R2G2B2B2

	ror	edx,8			; edx:  B2R2G2B2

	mov	[edi][8],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte ptr[esi][3]	; eax:  Y3

	mov	dh,Bounding24[ecx][eax]	; edx:  xxxxG3xx
	Ref	TEXT32

	mov	dl,Bounding24[ebp][eax]	; edx:  xxxxG3B3
	Ref	TEXT32

	rol	edx,16			; edx:  G3B3xxxx

	mov	dl,Bounding24[ebx][eax]	; edx:  G3B3xxR3
	Ref	TEXT32

	mov	dh,dl			; edx:  G3B3R3R3

	ror	edx,8			; edx:  R3G3B3R3

	mov	[edi][12],edx

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

	mov	[edi],ebx		; Y0 Y0 Y0 Y0
	rol	edx,16
	mov	[edi][4],ecx		; Y1 Y1 Y1 Y1

	xor	ebx,ebx
	mov	bl,dl			; Y2
	xor	ecx,ecx
	mov	cl,dh			; Y3

	mov	ebx,GreyDwordLookup[ebx*4]; Y2 Y2 Y2 Y2
	Ref	TEXT32

	mov	ecx,GreyDwordLookup[ecx*4]; Y3 Y3 Y3 Y3
	Ref	TEXT32

	mov	[edi][8],ebx		; Y2 Y2 Y2 Y2
	mov	[edi][12],ecx		; Y3 Y3 Y3 Y3


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

	mov	[edi],ebx		; Y0 Y0 Y0 Y0
	rol	edx,16
	mov	[edi][4],ecx		; Y1 Y1 Y1 Y1

	xor	ebx,ebx
	mov	bl,dl			; Y2
	xor	ecx,ecx
	mov	cl,dh			; Y3

	mov	ebx,GreyDwordLookup[ebx*4]; Y2 Y2 Y2 Y2
	Ref	TEXT32

	mov	ecx,GreyDwordLookup[ecx*4]; Y3 Y3 Y3 Y3
	Ref	TEXT32

	mov	[edi][8],ebx		; Y2 Y2 Y2 Y2
	mov	[edi][12],ecx		; Y3 Y3 Y3 Y3


GreySPartialYUVSkip:
	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreySPartialYUVLoop

	jmp	SmoothQuit

	endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_ExpandSmoothCodeBook24	endp

	EndMotion

	ENDSEG32


ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'
else
_DATA	segment	dword	USE32	public	'DATA'
endif

	BuildMotion	Expand24

_DATA	ends

	end
