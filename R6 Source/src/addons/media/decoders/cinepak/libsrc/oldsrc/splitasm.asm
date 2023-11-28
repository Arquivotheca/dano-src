; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/splitasm.asm 2.9 1994/10/23 17:23:05 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: splitasm.asm $
; Revision 2.9  1994/10/23 17:23:05  bog
; Try to allow big frames.
; Revision 2.8  1994/07/14  09:09:33  bog
; Remove SS: if WIN32.
; 
; Revision 2.7  1994/07/14  08:54:55  bog
; High half of ebx was nonzero.
; 
; Revision 2.6  1994/06/23  14:05:46  bog
; Difference of 8 bit is 9 bit.  Movsx was incorrect.
; 
; Revision 2.5  1994/05/17  12:52:51  timr
; Return longs in EAX for WIN32.
; 
; Revision 2.4  1993/09/13  13:38:52  timr
; Convert back to ML.
; 
; Revision 2.3  1993/08/05  17:07:08  timr
; CL386 assumes ebx is preserved across procedure calls.
; 
; Revision 2.1  93/06/09  14:31:41  bog
; Preserve FS and top halves of ESI and EDI.
; 
; Revision 2.0  93/06/01  14:17:21  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.6  93/04/21  15:59:15  bog
; Fix up copyright and disclaimer.
; 
; Revision 1.5  93/02/17  10:58:20  geoffs
; SplitTwoEntries needed to access vector list via 16:32 ->
; 
; Revision 1.4  93/02/17  09:28:29  geoffs
; Fix up some esi,si,di register crashes.
; 
; Revision 1.3  93/02/02  17:46:45  timr
; VECTORs are huge pointers.  Fixes Dave's hang.
; 
; Revision 1.2  93/01/26  11:22:45  timr
; The SplitTwoEntries return area needs to be in SS, not in the shared DS.
; 
; Revision 1.1  93/01/26  11:15:59  timr
; Initial revision
; 
;

ifndef	WIN32
	.model	small, c
	.386

XBX	textequ	<bx>
XDI	textequ	<di>

	extern	_AHSHIFT: ABS			; from KERNEL
else
	.386
	.model	small,c

XBX	textequ	<ebx>
XDI	textequ	<edi>
endif

	include	cvcompre.inc

; SplitCodeBook assembly helpers.

	.code

	extrn	LongSquares: DWORD		; in MATCH.ASM

; 
;  SplitCodeBook()
; 
;  Fill out a codebook by splitting the code with the maximum error
; 



; VECTOR huge * FindNonIdentical (VECTOR huge *, EXTCODE _far *, BYTE, BYTE);
;
; Find the first nonidentical matching vector.

ifndef	WIN32

FindNonIdentical	PROC	NEAR C PUBLIC uses esi di fs,
		lpV	: FAR PTR VECTOR,
		lpOldC	: FAR PTR EXTCODE,
		class	: BYTE,
		oldcode	: BYTE

	assume	ds: @data

	xor	esi,esi
	les	si, lpV
	lfs	di, lpOldC

pV	textequ	<(VECTOR PTR es:[esi])>
pOldC	textequ	<(EXTCODE PTR fs:[di])>

else	;	WIN32			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

FindNonIdentical	PROC	NEAR PUBLIC uses ebx esi edi,
		lpV	: PTR VECTOR,
		lpOldC	: PTR EXTCODE,
		class	: BYTE,
		oldcode	: BYTE

	assume	ds: @data

	mov	esi, lpV
	mov	edi, lpOldC

pV	equ	<(VECTOR PTR [esi])>
pOldC	equ	<(EXTCODE PTR [edi])>

endif


	mov	ecx, pOldC.ecnMatches	; loop counter
	xor	dx, dx

; For each code:

      .REPEAT

; We return upon finding the first vector which:
;  - is in the correct class
;  - is matched to the current codebook entry,
;  - is not identical to the current YUV.

	mov	ax, WORD PTR pV.vCode
	.errnz	VECTOR.vClass - VECTOR.vCode - 1
	.IF (ah == class && al == oldcode)

	  mov	eax, DWORD PTR pV.vyuv
	  cmp	eax, DWORD PTR pOldC.ecyuv
	  jnz	foundone
	  mov	ax, WORD PTR pV.vyuv [4]
	  cmp	ax, WORD PTR pOldC.ecyuv [4]
	  .IF	(!ZERO?)

foundone:
ifndef	WIN32
	    shld eax, esi, 16	; eax = count of 64ks
	    shl	ax, _AHSHIFT
	    mov	dx, es
	    add	dx, ax
	    mov	ax, si		; found one - return it
else
	    mov	eax, esi	; found one - return it
endif
	    ret
	  .ENDIF

	  dec	ecx
	.ENDIF

	add	esi, TYPE VECTOR
      .UNTIL (ecx == 0)

ifndef	WIN32
	xor	ax, ax
	mov	dx, ax			; return null pointer if failure
else
	xor	eax, eax		; return null pointer if failure
endif

	ret

FindNonIdentical	ENDP

pV	textequ	<>
pOldC	textequ	<>

S2E_PARMS	STRUC

OnMatches	dd	?
HnMatches	dd	?
OError		DD	?
HError		DD	?

S2E_PARMS	ENDS


; We now split vectors in this class matching OldCode between the code at 
; pOldC and the code at pHoles.  We accumulate the yuv for each match so 
; that we can move the codes to the mean of the matching vectors.

ifndef	WIN32

SplitTwoEntries	PROC	NEAR C PUBLIC uses esi di,
			lpV	: FAR PTR VECTOR,	; starting vector
			yOldC	: YYYYUV,		; YUV being split
			Code	: BYTE,			; code being split
			yHole	: YYYYUV,		; YUV to split into
			HoleCode: BYTE,			; code being split into
			Class	: BYTE,			; target class
			count	: DWORD,		; number of vectors
			Old	: PTR LYYYYUV,		; accumulated error
			Hole	: PTR LYYYYUV,		; accumulated error
			rtnval	: PTR S2E_PARMS		; return values

else	; WIN32				;;;;;;;;;;;;;;;;;;;;;;;;;;;

SplitTwoEntries	PROC	NEAR PUBLIC uses ebx esi edi,
			lpV	: PTR VECTOR,
			yOldC	: YYYYUV,
			Code	: BYTE,
			yHole	: YYYYUV,
			HoleCode: BYTE,
			Class	: BYTE,
			count	: DWORD,
			Old	: PTR LYYYYUV,
			Hole	: PTR LYYYYUV,
			rtnval	: PTR S2E_PARMS
endif

	assume	ds: @data

; Clear Old and Hole.  For Win16, both are SS: relative offsets.

ifndef	WIN32
	mov	ax, ss
	mov	es, ax
endif

	xor	eax, eax
	mov	ecx, (TYPE LYYYYUV)/4	; should be 6
	.errnz	(TYPE LYYYYUV)/4 NE 6
	mov	XDI, Old
    rep stos dword ptr es:[XDI]

	mov	ecx, (TYPE LYYYYUV)/4
	.errnz	(TYPE LYYYYUV)/4 NE 6
	mov	XDI, Hole
    rep stos dword ptr es:[XDI]

ifndef	WIN32

	xor	esi,esi
	les	si, lpV
pV	textequ	<(VECTOR PTR es:[esi])>
	mov	di, rtnval
pParms	textequ <(S2E_PARMS PTR ss:[di])>

else	; WIN32

	mov	esi, lpV
	mov	edi, rtnval
pV	textequ	<(VECTOR PTR [esi])>
pParms	textequ	<(S2E_PARMS PTR [edi])>

endif	; WIN32

; es:esi -> Vector book
; ss:di -> nMatches & Error return area

	mov	pParms.OnMatches, eax
	mov	pParms.HnMatches, eax
	mov	pParms.OError, eax
	mov	pParms.HError, eax

    .REPEAT			; for each vector in count
      mov	al, pV.vClass
      mov	ah, pV.vCode
      .IF (al == Class && ah == Code)

DiffXBX	macro	dest, one, two, op
	mov	bl,one
	sub	bl,two
	sbb	bh,bh

ifdef	WIN32

	inc	bh
	op	dest,LongSquares[ebx*4][-00100h*4]

else

	shl	bx,2
	op	dest,LongSquares[bx]
endif

	endm

; Compute distance from pV to pOldC into edx.

ifdef	WIN32
	xor	ebx,ebx		; clear high half
endif

	DiffXBX	edx, pV.vyuv.yuvu, yOldC.yuvu, mov
	DiffXBX	edx, pV.vyuv.yuvv, yOldC.yuvv, add
	shl	edx,2
	DiffXBX	edx, pV.vyuv.yuvy[0], yOldC.yuvy[0], add
	DiffXBX	edx, pV.vyuv.yuvy[1], yOldC.yuvy[1], add
	DiffXBX	edx, pV.vyuv.yuvy[2], yOldC.yuvy[2], add
	DiffXBX	edx, pV.vyuv.yuvy[3], yOldC.yuvy[3], add

; Compute distance from pV to pHole into eax.

	DiffXBX	eax, pV.vyuv.yuvu, yHole.yuvu, mov
	DiffXBX	eax, pV.vyuv.yuvv, yHole.yuvv, add
	shl	eax,2
	DiffXBX	eax, pV.vyuv.yuvy[0], yHole.yuvy[0], add
	DiffXBX	eax, pV.vyuv.yuvy[1], yHole.yuvy[1], add
	DiffXBX	eax, pV.vyuv.yuvy[2], yHole.yuvy[2], add
	DiffXBX	eax, pV.vyuv.yuvy[3], yHole.yuvy[3], add

; Add this vector into whichever side was closer.

	.IF (edx > eax)
	  inc	pParms.HnMatches
	  add	pParms.HError, eax		; eax = NewDist

	  mov	XBX, Hole
	  assume	XBX: PTR LYYYYUV

ifndef	WIN32
SS16	textequ	<ss>
else
SS16	textequ	<ds>
endif

	  xor	edx, edx
	  mov	eax, DWORD PTR yHole.yuvy
	  mov	dl, al
	  add	SS16:[XBX].lyuvy[0], edx
	  mov	dl, ah
	  add	SS16:[XBX].lyuvy[4], edx
	  rol	eax, 16
	  mov	dl, al
	  add	SS16:[XBX].lyuvy[8], edx
	  mov	dl, ah
	  add	SS16:[XBX].lyuvy[12], edx
	  mov	ax, WORD PTR yHole.yuvu 
	  mov	dl, al
	  add	SS16:[XBX].lyuvu, edx
	  mov	dl, ah
	  add	SS16:[XBX].lyuvv, edx
	  mov	al, HoleCode			; attach to new codebook entry
	  mov	pV.vCode, al

	.ELSE
	  inc	pParms.OnMatches
	  add	pParms.OError, edx		; edx = OldDist

	  mov	XBX, Old
	  assume	XBX: PTR LYYYYUV
	  xor	edx, edx
	  mov	eax, DWORD PTR yOldC.yuvy
	  mov	dl, al
	  add	SS16:[XBX].lyuvy[0], edx
	  mov	dl, ah
	  add	SS16:[XBX].lyuvy[4], edx
	  rol	eax, 16
	  mov	dl, al
	  add	SS16:[XBX].lyuvy[8], edx
	  mov	dl, ah
	  add	SS16:[XBX].lyuvy[12], edx
	  mov	ax, WORD PTR yOldC.yuvu 
	  mov	dl, al
	  add	SS16:[XBX].lyuvu, edx
	  mov	dl, ah
	  add	SS16:[XBX].lyuvv, edx
	.ENDIF
      .ENDIF

steContinue:
        add	esi, TYPE VECTOR
	dec	count
    .UNTIL (ZERO?)

	assume	XBX: nothing

	ret

SplitTwoEntries	ENDP

	end
