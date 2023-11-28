; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/cv.inc 2.6 1995/04/29 12:31:41 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: cv.inc $
; Revision 2.6  1995/04/29 12:31:41  bog
; Explicitly name the CodeBook types.
; Revision 2.5  1994/05/06  13:41:01  bog
; Play back black and white movies.
; 
; Revision 2.4  1993/09/13  13:38:28  timr
; Can't "assume cs" in ML flat model.
; 
; Revision 2.3  1993/07/06  16:47:59  geoffs
; Fix up typo in WIN32 assumes
; 
; Revision 2.2  93/07/02  17:09:38  geoffs
; Now compiles,runs under Windows NT
; 
; Revision 2.1  93/06/01  14:47:59  bog
; Cuter entry check.  Too bad no one will use it.
; 
; Revision 2.0  93/06/01  14:13:26  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.5  93/04/21  15:46:51  bog
; Fix up copyright and disclaimer.
; 
; Revision 1.4  92/12/21  11:35:35  bog
; Split into IC and CV layers.
; 
; Revision 1.3  92/11/29  15:53:17  bog
; Cleaned up 24bpp path.  Now it's faster.
;
; Revision 1.2  92/11/05  16:56:47  bog
; USE32 segment for Draw and Expand works.
;
; Revision 1.1  92/11/01  14:45:17  bog
; Initial revision
;

; Compact Video Codec structs and definitions



; CV constants

kMaxTileCount		equ	3

kFrameType		equ	000h
kKeyFrameType		equ	(000h + kFrameType)
kInterFrameType		equ	(001h + kFrameType)
kEmptyFrameType		equ	(002h + kFrameType)

kTileType		equ	010h
kKeyTileType		equ	(000h + kTileType)
kInterTileType		equ	(001h + kTileType)

kCodeBookType		equ	020h
kPartialBookBit		equ	001h
kSmoothBookBit		equ	002h
kGreyBookBit		equ	004h
kFullDBookType		equ	\
  kCodeBookType
kPartialDBookType	equ	\
  kCodeBookType + kPartialBookBit
kFullSBookType		equ	\
  kCodeBookType + kSmoothBookBit
kPartialSBookType	equ	\
  kCodeBookType + kPartialBookBit + kSmoothBookBit
kFullDGreyBookType	equ	\
  kCodeBookType + kGreyBookBit
kPartialDGreyBookType	equ	\
  kCodeBookType + kPartialBookBit + kGreyBookBit
kFullSGreyBookType	equ	\
  kCodeBookType + kSmoothBookBit + kGreyBookBit
kPartialSGreyBookType	equ	\
  kCodeBookType + kPartialBookBit + kSmoothBookBit + kGreyBookBit

kFullDBookType		equ	(000h + kCodeBookType)
kPartialDBookType	equ	(001h + kCodeBookType)
kFullSBookType		equ	(002h + kCodeBookType)
kPartialSBookType	equ	(003h + kCodeBookType)

kCodesType		equ	030h
kIntraCodesType		equ	(000h + kCodesType)
kInterCodesType		equ	(001h + kCodesType)
kAllSmoothCodesType	equ	(002h + kCodesType)


; USE32 segment entry code

Use32Proc	macro	name, nearness
		local	FixUp

FixUp:	push	edi		;; really	push	di
	sub	esp,8		;;		sub	sp,8
	mov	ebx,cs		;;		mov	bx,cs
	mov	eax,ss		;;		mov	ax,ss
	mov	es,eax		;;		mov	es,ax
	mov	edi,esp		;;		mov	di,sp
	xor	eax,eax		;;		xor	ax,ax
	mov	al,00bh		;; Get Desc
	int	031h
	db	026h,080h,04dh,006h,040h;;	or	es:[di][6],040h
	xor	eax,eax		;;		xor	ax,ax
	mov	al,00ch		;; Set Desc
	int	031h
	add	sp,8		;; now in 32 bit
	pop	di

name	proc	nearness

	db	03dh, 00h, 00h	;;		cmp	[e]ax,0000
	jmp	short FixUp	;; skipped if 32 bit

	endm

Use32Ret	macro	toss
ifndef	WIN32
	db	066h
endif
  ifb	<toss>
	ret
  else
	ret	toss
  endif
	endm

SEG32	macro
ifndef	WIN32
_TEXT32	segment	dword	public	USE32	'CODE'
        assume cs:_TEXT32
        assume ds:nothing
        assume es:nothing
else
_TEXT   segment dword	public 	use32	'CODE'
        assume  ds:FLAT, es:FLAT, ss:NOTHING, fs:NOTHING, gs:NOTHING
endif
	endm

ENDSEG32	macro
ifndef	WIN32
_TEXT32	ends
else
_TEXT	ends
endif
	endm

