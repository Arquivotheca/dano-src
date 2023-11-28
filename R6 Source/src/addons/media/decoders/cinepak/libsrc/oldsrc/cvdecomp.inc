; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/cvdecomp.inc 2.10 1994/07/29 15:31:22 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: cvdecomp.inc $
; Revision 2.10  1994/07/29 15:31:22  bog
; Weitek YUV420 works.
; Revision 2.9  1994/07/22  16:24:42  bog
; Add BuildInitialFixup macro for Weitek YUV.
; 
; Revision 2.8  1994/03/24  16:56:31  timr
; For DCI, allow the DIB YStep to change without requiring End/Begin.
; 
; Revision 2.7  1993/11/29  16:06:11  geoffs
; Pixel doubling done for 8bpp decompress
; 
; Revision 2.6  1993/07/06  16:52:54  geoffs
; Now compatible with ml,masm386
; 
; Revision 2.5  93/07/06  09:43:24  bog
; Masm386 does not like "repeat".
; 
; Revision 2.4  93/07/03  11:46:58  geoffs
; All _DATA segment declarations needed USE16 if WIN16 build
; 
; Revision 2.3  93/07/02  17:23:49  geoffs
; Had unclosed macro
; 
; Revision 2.2  93/07/02  17:10:35  geoffs
; Now compiles,runs under Windows NT
; 
; Revision 2.1  93/06/01  14:48:53  bog
; Compiled, flat decompress assembler.
; 
; Revision 2.0  93/06/01  14:13:49  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.3  93/04/21  15:47:22  bog
; Fix up copyright and disclaimer.
; 
; Revision 1.2  92/12/21  18:42:42  bog
; Decompressor now works split.
; 
; Revision 1.1  92/12/21  11:37:06  bog
; Initial revision
; 

; Compact Video decompression structs and definitions


ifndef	WIN32
_DATA	segment	dword	USE16	public	'DATA'

	extrn	_flatSel	:word	; selector to 4GB

_DATA	ends
DGROUP	group	_DATA
endif


; Macros for Expand and Draw code motion and fixup

BeginMotion	macro

		align	4

_nRefsCodebook	=	0
_nRefsYStep1	=	0		;; YStep
_nRefsYStep2	=	0		;; YStep*2
_nRefsYStep3	=	0		;; YStep*3
_nRefsYStep4	=	0		;; YStep*4
_nRefsYStep5	=	0		;; YStep*5
_nRefsYStep6	=	0		;; YStep*6
_nRefsYStep7	=	0		;; YStep*7
_nRefsYDelta	=	0		;; YStep*4 - Width
_nRefsMotion	=	0
_nRefsWidth4	=	0		;; Width/4
_nRefsWidth8	=	0		;; Width/8
_nRefsTEXT32	=	0
_nRefsYScan0	=	0
_nRefsUScan0	=	0
_nRefsUScan1	=	0
_nRefsVScan0	=	0
_nRefsVScan1	=	0

Codebook	=	010000000h
bWidth		=	010000000h
YStep		=	010000000h
YDelta		=	010000000h
YScan0		=	010000000h
UScan0		=	010000000h
UScan1		=	010000000h
VScan0		=	010000000h
VScan1		=	010000000h

MotionBase	label	byte

		endm


CatStrEQU	macro	Fixup,FixupNum

_Ref&Fixup&FixupNum	equ	$-MotionBase-4

		endm

Ref					macro	Fixup
		CatStrEQU	Fixup,%(_nRefs&Fixup)
_nRefs&Fixup				=	_nRefs&Fixup + 1
		endm


EndMotion	macro

MotionLength	equ	$-MotionBase

		endm


CatStrDW	macro	Fixup,i

		dw	_Ref&Fixup&i

		endm

DumpMotion	macro	Fixup

i		=	0
		rept	_nRefs&Fixup
		CatStrDW	Fixup,%(i)
i		=	i+1
		endm
		endm


BuildMotion	macro		Tag

		public		_&Tag&Base
_&Tag&Base	=		MotionBase

		public		_&Tag&Table
_&Tag&Table	dw		MotionLength
		dw		_nRefsCodebook
		dw		_nRefsYStep1
		dw		_nRefsYStep2
		dw		_nRefsYStep3
		dw		_nRefsYStep4
		dw		_nRefsYStep5
		dw		_nRefsYStep6
		dw		_nRefsYStep7
		dw		_nRefsYDelta
		dw		_nRefsMotion
		dw		_nRefsWidth4
		dw		_nRefsWidth8
		dw		_nRefsTEXT32
		dw		_nRefsYScan0
		dw		_nRefsUScan0
		dw		_nRefsUScan1
		dw		_nRefsVScan0
		dw		_nRefsVScan1

		DumpMotion	Codebook
		DumpMotion	YStep1
		DumpMotion	YStep2
		DumpMotion	YStep3
		DumpMotion	YStep4
		DumpMotion	YStep5
		DumpMotion	YStep6
		DumpMotion	YStep7
		DumpMotion	YDelta
		DumpMotion	Motion
		DumpMotion	Width4
		DumpMotion	Width8
		DumpMotion	TEXT32
		DumpMotion	YScan0
		DumpMotion	UScan0
		DumpMotion	UScan1
		DumpMotion	VScan0
		DumpMotion	VScan1

		endm

BuildInitialFixup	macro

		dd	Codebook
		Ref	Codebook
		dd	YStep
		Ref	YStep1
		dd	YStep*2
		Ref	YStep2
		dd	YStep*3
		Ref	YStep3
		dd	YStep*4
		Ref	YStep4
		dd	YStep*5
		Ref	YStep5
		dd	YStep*6
		Ref	YStep6
		dd	YStep*7
		Ref	YStep7
		dd	YDelta
		Ref	YDelta
		dd	0
		Ref	Motion
		dd	bWidth/4
		Ref	Width4
		dd	bWidth/8
		Ref	Width8
		dd	0
		Ref	TEXT32
		dd	YScan0
		Ref	YScan0
		dd	UScan0
		Ref	UScan0
		dd	UScan1
		Ref	UScan1
		dd	VScan0
		Ref	VScan0
		dd	VScan1
		Ref	VScan1

		endm


CVDecompEntry	macro	name, nearness

	public	_&name
ifndef	WIN32
_&name	proc	nearness
else
_&name	proc	near
endif

	push	ebp
ifndef	WIN32
	movzx	ebp,sp

	push	ds
else
	mov	ebp,esp

	push	ebx
endif

	push	esi
	push	edi

ifndef	WIN32
	mov	ds,_flatSel		; get flat 32 selector
	assume	ds:_TEXT32
endif
	endm

CVDecompExit	macro	toss
	pop	edi
	pop	esi
ifndef	WIN32
	pop	ds
else
	pop	ebx
endif
	pop	ebp

  ifb	<toss>
	Use32Ret
  else
	Use32Ret	toss
  endif
	endm
