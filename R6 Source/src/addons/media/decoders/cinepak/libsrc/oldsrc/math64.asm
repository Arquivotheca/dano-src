
; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/math64.asm 1.2 1993/10/05 10:34:16 geoffs Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: math64.asm $
; Revision 1.2  1993/10/05 10:34:16  geoffs
; Result returned in DX:AX for WIN16, in EAX for WIN32
; Revision 1.1  1993/10/01  12:39:16  geoffs
; Initial revision
; 


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; -------------------------------------------------------
;               DATA SEGMENT DECLARATIONS
; -------------------------------------------------------

;ifndef	WIN32
;	.model	small
;endif

	.386

ifndef	WIN32

_TEXT	segment	word	USE16	public	'CODE'
        assume cs:_TEXT
        assume ds:nothing
        assume es:nothing
else
_TEXT	segment	dword	USE32	public	'CODE'
	assume	ds:FLAT,es:FLAT,ss:NOTHING,fs:NOTHING,gs:NOTHING
endif

;--------------------------------------------------------------------------
;
; unsigned long muldiv32(
;	unsigned long top1,
;	unsigned long top2,
;	unsigned long bottom
; )
;
;   return ((top1 * top2) / bottom)
;   
;	Crashed:
;	    ES,FS,EAX,BX,CX,DX,flags
;
;--------------------------------------------------------------------------


ifndef	WIN32
OldBp	equ	(word ptr ss:[bp])
RetAddr	equ	(word ptr OldBp[2])
top1	equ	(dword ptr RetAddr[2])
top2	equ	(dword ptr top1[4])
bottom	equ	(dword ptr top2[4])
else
OldBp	equ	(dword ptr ss:[ebp])
RetAddr	equ	(dword ptr OldBp[4])
top1	equ	(dword ptr RetAddr[4])
top2	equ	(dword ptr top1[4])
bottom	equ	(dword ptr top2[4])
endif

	public	_muldiv32
_muldiv32	proc	near

ifndef	WIN32
	push	bp
	mov	bp,sp
else
	push	ebp
	mov	ebp,esp
endif

	sub	eax,eax			; EAX = 0 in case 0 dividend

	mov	ecx,bottom
	jecxz	@F

	mov	eax,top1
	mul	top2
	div	ecx
@@:

ifndef	WIN32
	shld    edx,eax,16		; result to DX:AX

	mov	sp,bp
	pop	bp
else
	mov	esp,ebp
	pop	ebp
endif
	ret

_muldiv32	endp

_TEXT	ends
	end
