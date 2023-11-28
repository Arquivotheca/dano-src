SECTION .text

%include "cv.i"
; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/expayuy2.asm 1.4 1994/09/26 08:46:16 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: expayuy2.asm $
; Revision 1.4  1994/09/26 08:46:16  bog
; Seems healthy.
; Revision 1.3  1994/09/25  15:18:36  bog
; Transform seems healthy.  As long as Cinepak YUV is monotonic, anyway.
; 
; Revision 1.2  1994/09/25  12:55:34  bog
; Produces pretty good YUV.  A couple of spots are out of range.
; 
; Revision 1.1  1994/09/22  17:19:20  bog
; Initial revision
; 

; Cloned from codec's expacpla 21 September 1994.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Mapping RGB to Cinepak YcUcVc

;   Yc =       2/7 R +      4/7 G +       1/7 B +   0
;   Uc =      -1/7 R +     -2/7 G +       3/7 B +   0
;   Vc =      5/14 R +     -2/7 G +     -1/14 B +   0

;     R,G,B	in    0..255
;     Yc	in    0..255
;     Uc	in -110..109
;     Vc	in  -91..91

;      R ,  G ,  B	 Yc,   Uc,   Vc
;     ---  ---  ---	---   ---   ---
;       0,   0,   0	  0,    0,    0
;	0,   0, 255	 37,  109, - 19
;	0, 255,   0	146, - 73, - 73
;	0, 255, 255	182,   36, - 91
;     255,   0,   0	 73, - 37,   91
;     255,   0, 255	109,   73,   73
;     255, 255,   0	219, -110,   18
;     255, 255, 255	255,    0,    0

; Mapping RGB to CCIR601 YUV

;   Y =  0.256647 R +  0.504034 G +  0.098182 B +  16
;   U = -0.148129 R + -0.290914 G +  0.439043 B + 128
;   V =  0.439322 R + -0.367697 G + -0.071625 B + 128

;     R,G,B	in   0..255
;     Y		in  16..235
;     U,V	in  16..240

;      R ,  G ,  B	 Y ,  U ,  V
;     ---  ---  ---	---  ---  ---
;       0,   0,   0	 16, 128, 128
;	0,   0, 255	 41, 240, 110
;	0, 255,   0	145,  54,  34
;	0, 255, 255	169, 166,  16
;     255,   0,   0	 81,  90, 240
;     255,   0, 255	106, 202, 222
;     255, 255,   0	210,  16, 146
;     255, 255, 255	235, 128, 128

; Mapping Cinepak YcUcVc to CCIR601 YUV

;   Y =  0.858863 Yc + -0.055653 Uc +  0.009261 Vc +  16
;   U =         0 Yc +  1.023543 Uc + -0.005345 Vc + 128
;   V =         0 Yc +  0.040600 Uc +  1.246342 Vc + 128

;     Yc	in    0..255
;     Uc	in -110..109
;     Vc	in  -91..91
;     Y		in   16..235
;     U,V	in   16..240

;      Yc,   Uc,   Uc	  Y ,  U ,  V
;     ---   ---   ---	 ---  ---  ---
;       0,    0,    0	  16, 128, 128
;      37,  109, - 19	  41, 240, 108
;     146, - 73, - 73	 144,  53,  34
;     182,   36, - 91	 169, 165,  16
;      73, - 37,   91	  81,  90, 240
;     109,   73,   73	 106, 203, 222
;     219, -110,   18	 210,  16, 146
;     255,    0,    0	 235, 128, 128

; We compute

;   U = Uc + (Uc >> 6) + (Uc >> 7) + 128
;   V = Vc + (Vc >> 2) + (U >> 5) + (U >> 7) + 123

;   then

;   Y = Yc - (Yc >> 3) - (Yc >> 5) - (U >> 4) + (U >> 7) + (V >> 7) + 26

; Instead of 128 for U, we use 128.578125 (8094) to ensure that yellow
; stays in range.

; Instead of 123 for V, we use 123.288208 (3da4e4) to ensure that cyan
; stays in range.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


GLOBAL ExpandDetailCodeBookYUY2
;GLOBAL ScaleTable5
;GLOBAL ScaleTable6

ALIGN 16
ExpandDetailCodeBookYUY2:
		push ebp
		mov ebp,esp
		push ebx
		push esi
		push edi

		sub esp,8

		mov	esi,[esp+16+8+4]		; pCBIn -> incoming codebook
		mov	ecx,[esi]			; get type & size
		mov bl,cl				; remember type
		
		shr ecx,16
		xchg cl,ch				; ecx: size of incoming chunk
		add ecx,esi				; ecx: first byte beyond chunk
		add esi,4				; esi: first codebook entry
		sub	ecx,6			; ecx:  last possible valid entry
		
;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook

		cmp	esi,ecx		; empty codebook?
		ja	short DetailQuit
		
		mov	edi,[esp+16+8+8]		; pCB

;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook
		
%ifndef	NOBLACKWHITE
		
		test	bl,kGreyBookBit	; grey scale codebook?
		jnz	near DoGreyDetail
		
%endif
		
		cmp	bl,kFullDBookType
		je	short DetailKey
		
		cmp	bl,kPartialDBookType
		je	near DetailPartial
		
DetailHuh:

; not recognized so we just return

DetailQuit:

		add			esp,8

		pop edi
		pop esi
		pop ebx
		pop ebp
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DetailKey:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov [esp+4],ecx
	align	4

DetailKeyLoop:

;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|U |Y1|V |Y2|U |Y3|V |  |  |  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	movsx	eax,byte [esi+4]		; Uc
	lea	edx,[eax+eax*2]		; Uc*3
	shl	eax,8				; Uc*256
	lea	edx,[000008094h+eax+edx*2]	; Uc*256 + Uc*6 + 128*256

;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	movsx	eax,byte[esi+5]		; Vc
	shl	eax,5+8
	add	eax,edx				; Vc<<8 << 5 + U<<8
	lea	eax,[03da4e4h+eax+eax*4]	; 5 * (Vc<<8 << 5 + U<<8)
	sar	eax,7

	mov	cl,ah				; V
	shl	ecx,24
	mov	ch,dh				; 0 U 0 V

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ecx	0 U 0 V
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[0d0000h+eax+edx*8]
	sar	ebp,7

;     ecx	0 U 0 V
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+1]	; Yc1
	lea	edx,[eax+eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	eax,ebp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	xor	al,al
	shl	eax,16		; Y1 in 31..24, zeros in 23..0

;     eax	0 0 0 Y1
;     ecx	0 U 0 V
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi]	; Yc0
	lea	edx,[eax+eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	ax,bp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	ax,dx

	shr	eax,8		; Y0 0 Y1 0
	or	eax,ecx		; Y0 U Y1 V

;     eax	Y0 U Y1 V
;     ecx	0 U 0 V
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+3]	; Yc3
	lea	edx,[eax+eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	eax,ebp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	xor	al,al
	shl	eax,16		; Y3 in 31..24, zeros in 23..0

;     eax	0 0 0 Y3
;     ecx	0 U 0 V
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi+2]	; Yc2
	lea	edx,[eax+eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	ax,bp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	ax,dx

	shr	eax,8		; Y2 0 Y3 0
	or	eax,ecx		; Y2 U Y3 V

;     eax	Y2 U Y3 V
;     ecx	0 U 0 V
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi+4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV
	add	edi,16		; bump to next RGB patch

	cmp	esi,[esp+4]	; any more codebook entries?

	jbe	near DetailKeyLoop	; jump if more to do
	jmp	DetailQuit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DetailPartial:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[esp+4],ecx

	align	4

DetailPartialLoadSwitches:

  ; We use ebx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	ebx,[esi]	; get next set of 32 switches
	add	esi,4		; bump source

	xchg	bl,bh		; swiz
	rol	ebx,16
	xchg	bl,bh

	stc
	adc	ebx,ebx

  ;  carry, ebx: s ssss ssss ssss ssss ssss ssss ssss sss1

	jmp	short DetailPartialTestSwitch

	align	4


DetailPartialYUVLoop:

;     ebx	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ebx,ebx		; replace this index?

DetailPartialTestSwitch:

	jnc	near DetailPartialYUVSkip

	jz	DetailPartialLoadSwitches

;     ebx	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|U |Y1|V |Y2|U |Y3|V |  |  |  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	movsx	eax,byte[esi+4]		; Uc
	lea	edx,[eax+eax*2]		; Uc*3
	shl	eax,8				; Uc*256
	lea	edx,[000008094h+eax+edx*2]	; Uc*256 + Uc*6 + 128*256

;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	movsx	eax,byte[esi+5]		; Vc
	shl	eax,5+8
	add	eax,edx				; Vc<<8 << 5 + U<<8
	lea	eax,[03da4e4h+eax+eax*4]	; 5 * (Vc<<8 << 5 + U<<8)
	sar	eax,7

	mov	cl,ah				; V
	shl	ecx,24
	mov	ch,dh				; 0 U 0 V

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ecx	0 U 0 V
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[0d0000h+eax+edx*8]
	sar	ebp,7

;     ecx	0 U 0 V
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+1]	; Yc1
	lea	edx,[eax+eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	eax,ebp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	xor	al,al
	shl	eax,16		; Y1 in 31..24, zeros in 23..0

;     eax	0 0 0 Y1
;     ecx	0 U 0 V
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi]	; Yc0
	lea	edx,[eax+eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	ax,bp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	ax,dx

	shr	eax,8		; Y0 0 Y1 0
	or	eax,ecx		; Y0 U Y1 V

;     eax	Y0 U Y1 V
;     ecx	0 U 0 V
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+3]	; Yc3
	lea	edx,[eax+eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	eax,ebp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	xor	al,al
	shl	eax,16		; Y3 in 31..24, zeros in 23..0

;     eax	0 0 0 Y3
;     ecx	0 U 0 V
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi+2]	; Yc2
	lea	edx,[eax+eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	ax,bp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	ax,dx

	shr	eax,8		; Y2 0 Y3 0
	or	eax,ecx		; Y2 U Y3 V

;     eax	Y2 U Y3 V
;     ecx	0 U 0 V
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi+4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV

DetailPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp esi,[esp+4]
	
	jbe	near DetailPartialYUVLoop
	jmp	DetailQuit

%ifndef	NOBLACKWHITE

DoGreyDetail:

;     ecx	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ecx,2		; last possible code

	cmp	bl,kFullDBookType + kGreyBookBit
	jne	GreyDPartial

	align	4

GreyDKeyLoop:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|80|Y1|80|Y2|80|Y3|80|  |  |  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	xor	eax,eax

	mov	ah,[esi+1]	; Yc1
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc1<<8 << 2) + Yc1<<8
	sar	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	mov	al,128
	shl	eax,16		; Y1 in 31..24, 128 in 23..16, zeros in 15..0

;     eax	0 0 128 Y1
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi]	; Yc0
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc0<<8 << 2) + Yc0<<8
	sar	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	ax,dx

	mov	al,128		; 128 Y0 128 Y1
	ror	eax,8		; Y0 128 Y1 128

;     eax	Y0 128 Y1 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+3]	; Yc3
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc3<<8 << 2) + Yc3<<8
	sar	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	mov	al,128
	shl	eax,16		; Y3 in 31..24, 128 in 23..16, zeros in 15..0

;     eax	0 0 128 Y3
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi+2]	; Yc2
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc2<<8 << 2) + Yc2<<8
	sar	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	ax,dx

	mov	al,128		; 128 Y2 128 Y3
	ror	eax,8		; Y2 128 Y3 128

;     eax	Y2 128 Y3 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi+4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4		; bump to next incoming YYYY
	add	edi,16		; bump to next RGB patch

	cmp	esi,ecx		; any more codebook entries?
	jbe	GreyDKeyLoop	; jump if more to do

	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreyDPartial:

;     ecx	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	cmp	bl,kPartialDBookType + kGreyBookBit
	jne	near DetailHuh	; invalid code

	align	4

GreyDPartialLoadSwitches:

  ; We use ebx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	ebx,[esi]	; get next set of 32 switches
	add	esi,4		; bump source

	xchg	bl,bh		; swiz
	rol	ebx,16
	xchg	bl,bh

	stc
	adc	ebx,ebx

  ;  carry, ebx: s ssss ssss ssss ssss ssss ssss ssss sss1

	jmp	short GreyDPartialTestSwitch

	align	4


GreyDPartialYUVLoop:

;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ebx,ebx		; replace this index?

GreyDPartialTestSwitch:

	jnc	GreyDPartialYUVSkip

	jz	GreyDPartialLoadSwitches

;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|80|Y1|80|Y2|80|Y3|80|  |  |  |  |  |  |  |  |
;   +-----------+-----------+-----------+-----------+

	xor	eax,eax

	mov	ah,[esi+1]	; Yc1
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc1<<8 << 2) + Yc1<<8
	sar	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	mov	al,128
	shl	eax,16		; Y1 in 31..24, 128 in 23..16, zeros in 15..0

;     eax	0 0 128 Y1
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi]	; Yc0
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc0<<8 << 2) + Yc0<<8
	sar	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	ax,dx

	mov	al,128		; 128 Y0 128 Y1
	ror	eax,8		; Y0 128 Y1 128

;     eax	Y0 128 Y1 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+3]	; Yc3
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc3<<8 << 2) + Yc3<<8
	sar	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	mov	al,128
	shl	eax,16		; Y3 in 31..24, 128 in 23..16, zeros in 15..0

;     eax	0 0 128 Y3
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	ah,[esi+2]	; Yc2
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc2<<8 << 2) + Yc2<<8
	sar	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	ax,dx

	mov	al,128		; 128 Y2 128 Y3
	ror	eax,8		; Y2 128 Y3 128

;     eax	Y2 128 Y3 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi+4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4		; bump to next incoming YYYY

GreyDPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,ecx		; any more codebook entries?
	jbe	GreyDPartialYUVLoop

	jmp	DetailQuit

%endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


GLOBAL ExpandSmoothCodeBookYUY2

ALIGN 16
ExpandSmoothCodeBookYUY2:
		push ebp
		mov ebp,esp
		push ebx
		push esi
		push edi

		sub esp,8

		mov	esi,[esp+16+8+4]		; pCBIn -> incoming codebook
		mov	ecx,[esi]			; get type & size
		mov bl,cl				; remember type
		
		shr ecx,16				; 16 in hex
		xchg cl,ch				; ecx: size of incoming chunk
		add ecx,esi				; ecx: first byte beyond chunk
		add esi,4				; esi: first codebook entry
		sub ecx,6				; ecx: last posssible valid entry
		
;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook

		cmp	esi,ecx		; empty codebook?
		ja	short SmoothQuit
		
		mov	edi,[esp+16+8+8]		; pCB

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook
		
%ifndef	NOBLACKWHITE
		
		test	bl,kGreyBookBit	; grey scale codebook?
		jnz	near DoGreySmooth
		
%endif
		
		cmp	bl,kFullSBookType
		je	short SmoothKey
		
		cmp	bl,kPartialSBookType
		je	near SmoothPartial
		
SmoothHuh:

; not recognized so we just return

SmoothQuit:

		add			esp,8

		pop edi
		pop esi
		pop ebx
		pop ebp
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SmoothKey:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov [esp+4],ecx

	align	4

SmoothKeyLoop:
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|U |Y0|V |Y1|U |Y1|V |Y2|U |Y2|V |Y3|U |Y3|V |
;   +-----------+-----------+-----------+-----------+

	movsx	eax,byte[esi+4]		; Uc
	lea	edx,[eax+eax*2]		; Uc*3
	shl	eax,8				; Uc*256
	lea	edx,[000008094h+eax+edx*2]	; Uc*256 + Uc*6 + 128*256

;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	movsx	eax,byte[esi+5]		; Vc
	shl	eax,5+8
	add	eax,edx				; Vc<<8 << 5 + U<<8
	lea	eax,[03da4e4h+eax+eax*4]	; 5 * (Vc<<8 << 5 + U<<8)
	sar	eax,7

	mov	cl,dh				; U
	shl	ecx,24
	mov	ch,ah				; 0 V 0 U

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ecx	0 V 0 U
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[0d0000h+eax+edx*8]
	sar	ebp,7

;     ecx	0 V 0 U
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax+eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

;     ah	Y0
;     ecx	0 V 0 U
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; 0 V 0 U
	mov	dl,ah		; Y0 V 0 U
	rol	edx,16		; 0 U Y0 V
	mov	dl,ah		; Y0 U Y0 V

	mov	[edi],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+1]	; Yc1
	lea	edx,[eax+eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	eax,ebp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

;     ah	Y1
;     ecx	0 V 0 U
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; 0 V 0 U
	mov	dl,ah		; Y1 V 0 U
	rol	edx,16		; 0 U Y1 V
	mov	dl,ah		; Y1 U Y1 V

	mov	[edi+4],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+2]	; Yc2
	lea	edx,[eax+eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

;     ah	Y2
;     ecx	0 V 0 U
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; 0 V 0 U
	mov	dl,ah		; Y2 V 0 U
	rol	edx,16		; 0 U Y2 V
	mov	dl,ah		; Y2 U Y2 V

	mov	[edi+8],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+3]	; Yc3
	lea	edx,[eax+eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	eax,ebp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

;     ah	Y3
;     ecx	0 V 0 U
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	cl,ah		; Y3 V 0 U
	rol	ecx,16		; 0 U Y3 V
	mov	cl,ah		; Y3 U Y3 V

	mov	[edi+12],ecx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV
	add	edi,16		; bump to next RGB patch

	cmp	esi,[esp+4]	; any more codebook entries?

	jbe	near SmoothKeyLoop	; jump if more to do
	jmp	SmoothQuit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SmoothPartial:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[esp+4],ecx

	align	4

SmoothPartialLoadSwitches:

  ; We use ebx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	ebx,[esi]	; get next set of 32 switches
	add	esi,4		; bump source

	xchg	bl,bh		; swiz
	rol	ebx,16
	xchg	bl,bh

	stc
	adc	ebx,ebx

  ;  carry, ebx: s ssss ssss ssss ssss ssss ssss ssss sss1

	jmp	short SmoothPartialTestSwitch

	align	4


SmoothPartialYUVLoop:

;     ebx	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ebx,ebx		; replace this index?

SmoothPartialTestSwitch:

	jnc	near SmoothPartialYUVSkip

	jz	SmoothPartialLoadSwitches

;     ebx	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 U V in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|U |Y0|V |Y1|U |Y1|V |Y2|U |Y2|V |Y3|U |Y3|V |
;   +-----------+-----------+-----------+-----------+

	movsx	eax,byte[esi+4]		; Uc
	lea	edx,[eax+eax*2]		; Uc*3
	shl	eax,8				; Uc*256
	lea	edx,[000008094h+eax+edx*2]	; Uc*256 + Uc*6 + 128*256

;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	movsx	eax,byte[esi+5]		; Vc
	shl	eax,5+8
	add	eax,edx				; Vc<<8 << 5 + U<<8
	lea	eax,[03da4e4h+eax+eax*4]	; 5 * (Vc<<8 << 5 + U<<8)
	sar	eax,7

	mov	cl,dh				; U
	shl	ecx,24
	mov	ch,ah				; 0 V 0 U

;     eax	V<<8 = Vc<<8 + (Vc<<8 >> 2) + (U<<8 >> 5) + (U<<8 >> 7) + 123<<8
;     ecx	0 V 0 U
;     edx	U<<8 = Uc<<8 + (Uc<<8 >> 6) + (Uc<<8 >> 7) + 128<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,edx
	neg	edx
	lea	ebp,[0d0000h+eax+edx*8]
	sar	ebp,7

;     ecx	0 V 0 U
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[eax+eax*4]; (Yc0<<8 << 2) + Yc0<<8
	add	eax,ebp		; Yc0 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

;     ah	Y0
;     ecx	0 V 0 U
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; 0 V 0 U
	mov	dl,ah		; Y0 V 0 U
	rol	edx,16		; 0 U Y0 V
	mov	dl,ah		; Y0 U Y0 V

	mov	[edi],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+1]	; Yc1
	lea	edx,[eax+eax*4]; (Yc1<<8 << 2) + Yc1<<8
	add	eax,ebp		; Yc1 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

;     ah	Y1
;     ecx	0 V 0 U
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; 0 V 0 U
	mov	dl,ah		; Y1 V 0 U
	rol	edx,16		; 0 U Y1 V
	mov	dl,ah		; Y1 U Y1 V

	mov	[edi+4],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+2]	; Yc2
	lea	edx,[eax+eax*4]; (Yc2<<8 << 2) + Yc2<<8
	add	eax,ebp		; Yc2 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

;     ah	Y2
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	edx,ecx		; 0 V 0 U
	mov	dl,ah		; Y2 V 0 U
	rol	edx,16		; 0 U Y2 V
	mov	dl,ah		; Y2 U Y2 V

	mov	[edi+8],edx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+3]	; Yc3
	lea	edx,[eax+eax*4]; (Yc3<<8 << 2) + Yc3<<8
	add	eax,ebp		; Yc3 - (U >> 4) + (U >> 7) + (V >> 7) + 26
	shr	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

;     ah	Y3
;     ecx	0 V 0 U
;     ebp	- (U<<8 >> 4) + (U<<8 >> 7) + 26<<8
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	cl,ah		; Y3 V 0 U
	rol	ecx,16		; 0 U Y3 V
	mov	cl,ah		; Y3 U Y3 V

	mov	[edi+12],ecx

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV

SmoothPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp esi,[esp+4]
	
	jbe	near SmoothPartialYUVLoop
	jmp	SmoothQuit

%ifndef NOBLACKWHITE

DoGreySmooth:

;     bl	codebook type
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ecx,2		; last possible code

	cmp	bl,kFullSBookType + kGreyBookBit
	jne	near GreySPartial

	align	4

GreySKeyLoop:

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|80|Y0|80|Y1|80|Y1|80|Y2|80|Y2|80|Y3|80|Y3|80|
;   +-----------+-----------+-----------+-----------+

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc0<<8 << 2) + Yc0<<8
	sar	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y0 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx
	ror	eax,8

;     eax	Y0 128 Y0 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+1]	; Yc1
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc1<<8 << 2) + Yc1<<8
	sar	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y1 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx
	ror	eax,8

;     eax	Y1 128 Y1 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi+4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+2]	; Yc2
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc2<<8 << 2) + Yc2<<8
	sar	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y2 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx
	ror	eax,8

;     eax	Y2 128 Y2 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi+8],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+3]	; Yc3
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc3<<8 << 2) + Yc3<<8
	sar	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y3 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx
	ror	eax,8

;     eax	Y3 128 Y3 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi+12],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4		; bump to next incoming YYYYUV
	add	edi,16		; bump to next RGB patch

	cmp	esi,ecx		; any more codebook entries?
	jbe	near GreySKeyLoop	; jump if more to do

	jmp	SmoothQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreySPartial:

	cmp	bl,kPartialSBookType + kGreyBookBit
	jne	near SmoothHuh

;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	align	4

GreySPartialLoadSwitches:

  ; We use ebx for our switch loop control.  We fetch a dword of
  ; switches, swiz to x86 order, then trot the bits out the top in
  ; order.  On the first shift, we bring in a 1; when it shifts out, we
  ; must fetch another switch dword.

	mov	ebx,[esi]	; get next set of 32 switches
	add	esi,4		; bump source

	xchg	bl,bh		; swiz
	rol	ebx,16
	xchg	bl,bh

	stc
	adc	ebx,ebx

  ;  carry, ebx: s ssss ssss ssss ssss ssss ssss ssss sss1

	jmp	short GreySPartialTestSwitch

	align	4


GreySPartialYUVLoop:

;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	ebx,ebx		; replace this index?

GreySPartialTestSwitch:

	jnc	near GreySPartialYUVSkip

	jz	GreySPartialLoadSwitches

;     ebx	swizzled bit switches for codes we do
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

;   We take the incoming Y3 Y2 Y1 Y0 in Cinepak YUV space at [esi],
;   convert to CCIR601 YUV422 and remember it in the table:

;   +-----------+-----------+-----------+-----------+
;   |Y0|80|Y0|80|Y1|80|Y1|80|Y2|80|Y2|80|Y3|80|Y3|80|
;   +-----------+-----------+-----------+-----------+

	xor	eax,eax

	mov	ah,[esi]	; Yc0
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc0<<8 << 2) + Yc0<<8
	sar	edx,5		; (Yc0<<8 >> 3) + (Yc0<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y0 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx
	ror	eax,8

;     eax	Y0 128 Y0 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+1]	; Yc1
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc1<<8 << 2) + Yc1<<8
	sar	edx,5		; (Yc1<<8 >> 3) + (Yc1<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y1 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx
	ror	eax,8

;     eax	Y1 128 Y1 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi+4],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+2]	; Yc2
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc2<<8 << 2) + Yc2<<8
	sar	edx,5		; (Yc2<<8 >> 3) + (Yc2<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y2 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx
	ror	eax,8

;     eax	Y2 128 Y2 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi+8],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax

	mov	ah,[esi+3]	; Yc3
	lea	edx,[0fffd8000h+eax+eax*4]; (Yc3<<8 << 2) + Yc3<<8
	sar	edx,5		; (Yc3<<8 >> 3) + (Yc3<<8 >> 5)
	sub	eax,edx

	mov	al,128
	mov	edx,eax
	shl	eax,16		; Y3 in 31..24, 128 in 23..16, zeros in 15..0
	mov	ax,dx
	ror	eax,8

;     eax	Y3 128 Y3 128
;     ecx	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[edi+12],eax

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,4		; bump to next incoming YYYYUV

GreySPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,ecx		; any more codebook entries?
	jbe	near GreySPartialYUVLoop

	jmp	SmoothQuit


%endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

