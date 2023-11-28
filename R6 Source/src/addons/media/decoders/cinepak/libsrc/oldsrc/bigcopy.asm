; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/bigcopy.asm 2.6 1993/09/02 16:44:27 timr Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: bigcopy.asm $
; Revision 2.6  1993/09/02 16:44:27  timr
; Convert from MASM386 to ML 6.11.
; Revision 2.5  1993/07/03  03:41:44  geoffs
; All _DATA segment declarations needed USE16 if WIN16 build
; 
; Revision 2.4  93/07/02  17:14:43  geoffs
; Deleted WIN16 DGROUP definition by mistake
; 
; Revision 2.3  93/07/02  16:20:43  geoffs
; Now compiles,runs under Windows NT
; 
; Revision 2.2  93/06/08  16:49:27  geoffs
; Removed DRAW stuff, cleanup Decompress rectangles
; 
; Revision 2.1  93/06/02  15:37:00  bog
; Silly optimization.
; 
; Revision 2.0  93/06/01  14:13:14  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.5  93/04/21  15:46:30  bog
; Fix up copyright and disclaimer.
; 
; Revision 1.4  93/04/15  13:36:38  geoffs
; BigCopy now handles overlapping source,dest
; 
; Revision 1.3  93/01/25  14:23:46  geoffs
; Allow non 0 mod 4 frames.
; 
; Revision 1.2  92/11/01  23:45:19  bog
; Forgot to put carry from ax into cx for byte rep.
; 
; Revision 1.1  92/10/31  23:11:52  bog
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

	include cv.inc

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
; void BigCopy(char far *source, char far *dest, unsigned long size)
;
;   copy size bytes from source to destination
;   no effort is made to align nicely; this works best if source and
;     dest are both dword aligned.
;   
;	Crashed:
;	    ES,FS,EAX,BX,CX,DX,flags
;
;--------------------------------------------------------------------------


ifndef	WIN32
OldBp	equ	(word ptr ss:[bp])
RetAddr	equ	(word ptr OldBp[2])
pS	equ	(dword ptr RetAddr[2])
pD	equ	(dword ptr pS[4])
n	equ	(dword ptr pD[4])
else
OldBp	equ	(dword ptr ss:[ebp])
RetAddr	equ	(dword ptr OldBp[4])
pS	equ	(dword ptr RetAddr[4])
pD	equ	(dword ptr pS[4])
n	equ	(dword ptr pD[4])
endif

	public	_BigCopy
_BigCopy	proc	near

ifndef	WIN32
	push	bp
	mov	bp,sp

	push	ds
	push	esi
	push	edi

	mov	ax,word ptr pD[2]
	mov	es,ax
	assume	es:nothing
	movzx	edi,word ptr pD

	mov	bx,word ptr pS[2]
	mov	ds,bx
	assume	ds:nothing
	movzx	esi,word ptr pS

	mov	ecx,n

; Is there the possibility for overlapping chunks requiring a move in the
; opposite direction?

	cmp	ax,bx		; in same segments?
	jne	bg_no_overlap	; no -- assume no overlap can occur...

	lea	eax,[esi][ecx][-1]; -> last source address
	cmp	eax,edi		; is source chunk totally below destination?
	jb	bg_no_overlap	; yes -- no overlap possible...

	lea	eax,[edi][ecx][-1]; -> last destination address
	cmp	eax,esi		; is destination chunk totally below source?
	jb	bg_no_overlap	; yes -- no overlap possible...
else
	push	ebp
	mov	ebp,esp

	push	ebx
	push	esi
	push	edi

	mov	ecx,n
	mov	esi,pS
	mov	edi,pD

	cmp	esi,edi
	jae	bg_no_overlap	; if source is above dest, no overlap...
endif

; Backward-moving copy (ie: overlap is possible):

	lea	esi,[esi][ecx][-4]; -> last dword in source
	lea	edi,[edi][ecx][-4]; -> last dword in dest

	std			; work down in address

	mov	ax,cx		; save for odd byte

	shr	ecx,2		; odd word to carry
    rep	movs	dword ptr es:[edi],dword ptr ds:[esi]

	adc	cx,cx		; odd word to ecx
        add	esi,2
	add	edi,2		; compensate for dword move above
    rep	movs	word ptr es:[edi],word ptr ds:[esi]

	shr	ax,1		; odd byte to carry
	adc	cx,cx		; odd byte to ecx
	inc	esi
	inc	edi		; compensate for word move above
    rep	movs	byte ptr es:[edi],byte ptr ds:[esi]

    	cld

	jmp	short bg_done

; Forward-moving copy (ie: no overlap):

bg_no_overlap:

	mov	ax,cx		; save for odd byte

	shr	ecx,2		; odd word to carry
    rep	movs	dword ptr es:[edi],dword ptr ds:[esi]
	adc	cx,cx		; odd word to ecx
    rep	movs	word ptr es:[edi],word ptr ds:[esi]
	shr	ax,1		; odd byte to carry
	adc	cx,cx		; odd byte to ecx
    rep	movs	byte ptr es:[edi],byte ptr ds:[esi]

bg_done:

	pop	edi
	pop	esi
ifndef	WIN32
	pop	ds

	mov	sp,bp
	pop	bp
else
	pop	ebx

	mov	esp,ebp
	pop	ebp
endif
	ret

_BigCopy	endp

;--------------------------------------------------------------------------
;
; void ButtHeadCopyBits(
;	unsigned long srcOffset32,
;	unsigned short srcSelector,
;	long srcYStep,
;	unsigned long dstOffset32,
;	unsigned short dstSelector,
;	long dstYStep,
;	unsigned short width,
;	unsigned short height
; );
;
;   for height scans in source, copy width # bytes to dest
;   no effort is made to align nicely; this works best if source and
;     dest are both dword aligned.
;   
;	Crashed:
;	    ES,EAX,EBX,ECX,EDX,flags
;
;--------------------------------------------------------------------------

ifndef	WIN32
	assume	ds:nothing
	assume	es:nothing
	assume	fs:nothing
	assume	gs:nothing

OldEbp		equ	(dword ptr ss:[bp])
RetAddr		equ	(word ptr OldEbp[4])
srcOffset32	equ	(dword ptr RetAddr[2])
srcSelector	equ	(word ptr srcOffset32[4])
srcYStep	equ	(dword ptr srcSelector[2])
dstOffset32	equ	(dword ptr srcYStep[4])
dstSelector	equ	(word ptr dstOffset32[4])
dstYStep	equ	(dword ptr dstSelector[2])
nW		equ	(word ptr dstYStep[4])
nH		equ	(word ptr nW[2])

else

OldEbp		equ	(dword ptr ss:[ebp])
RetAddr		equ	(dword ptr OldEbp[4])
srcOffset32	equ	(dword ptr RetAddr[4])
srcYStep	equ	(dword ptr srcOffset32[4])
dstOffset32	equ	(dword ptr srcYStep[4])
dstYStep	equ	(dword ptr dstOffset32[4])
nW		equ	(word ptr dstYStep[4])
nH		equ	(word ptr nW[4])
endif

	public	_ButtHeadCopyBits
_ButtHeadCopyBits	proc	near

	push	ebp
ifndef	WIN32
	mov	bp,sp

	push	ds
else
	mov	ebp,esp

	push	ebx
endif
	push	esi
	push	edi

ifndef	WIN32
	mov	es,dstSelector
	assume	es:nothing
	mov	edi,dstOffset32

	mov	ds,srcSelector
	assume	ds:nothing
	mov	esi,srcOffset32
else
	mov	edi,dstOffset32
	mov	esi,srcOffset32
endif

	movzx	eax,nW		; # bytes to copy

	mov	bx,nH		; # scanlines
	dec	bx		; less one since looking for carry
	shl	ebx,16		; to top half for in loop

	mov	edx,srcYStep
	sub	edx,eax
	mov	ebp,dstYStep
	sub	ebp,eax

	shr	eax,1		; odd byte to carry
	adc	bl,bl		;  to bl
	shr	eax,1		; odd word to carry
	adc	bh,bh		;  to bh

cpscan:

;     eax	number of whole dwords to copy in scanline
;     bl	number of odd bytes copy in scanline
;     bh	number of odd words copy in scanline
;     ebx.hi	number of scanlines left to copy after this one
;     ecx	counting down rep movs
;     edx	srcYStep - nW
;     ds:esi	-> source bitmap
;     es:edi	-> dest bitmap
;     ebp	dstYStep - nW

	mov	ecx,eax		; # dwords to copy / scan
    rep	movs	dword ptr es:[edi],dword ptr ds:[esi]
	mov	cl,bh		; odd word to ecx
    rep	movs	word ptr es:[edi],word ptr ds:[esi]
	mov	cl,bl		; odd byte to ecx
    rep	movs	byte ptr es:[edi],byte ptr ds:[esi]

	add	esi,edx		; bump to next scan
	add	edi,ebp

	sub	ebx,000010000h	; 1 less scan to process
	jnc	cpscan		; for all scans to process...

	pop	edi
	pop	esi
ifndef	WIN32
	pop	ds
else
	pop	ebx
endif

	pop	ebp
	ret

_ButtHeadCopyBits	endp

_TEXT	ends
	end
