; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/match.asm 2.10 1995/03/07 14:50:38 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: match.asm $
; Revision 2.10  1995/03/07 14:50:38  bog
; Fix comments.
; Revision 2.9  1994/06/23  14:13:29  bog
; Change movzx into xor/mov pair.
; 
; Revision 2.8  1994/05/27  17:04:30  bog
; LongSquares now dword aligned in Win16 in libinit.asm.
; 
; Revision 2.7  1994/05/27  15:44:13  bog
; Experimental version.
; 
; Revision 2.6  1994/05/02  15:52:51  bog
; Undo pad for dword align.
; 
; Revision 2.5  1994/05/01  23:58:24  unknown
; Try dword aligning again.
; 
; Revision 2.4  1994/05/01  23:55:11  unknown
; Align LongSquares to DWORD.
; 
; Revision 2.3  1993/09/02  16:33:39  timr
; Change distance computation.  I was mishandling carry.
; 
; Revision 2.2  1993/08/05  17:06:52  timr
; CL386 assumes ebx is preserved across procedure calls.
; 
; Revision 2.0  93/06/01  14:16:28  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.11  93/04/21  15:57:32  bog
; Fix up copyright and disclaimer.
; 
; Revision 1.10  93/02/07  13:24:46  bog
; If incoming Code is fINVALID, the error we remember must be positive.
; 
; Revision 1.9  93/02/05  17:01:20  bog
; 1.  Fix early escapes.
; 2.  Ensure NEIGHBORS of 0 behaves correctly.
; 
; Revision 1.8  93/02/03  21:00:45  timr
; Ensure that returned match is lowest index of equidistant matches.
; (bog)
; 
; Revision 1.7  93/02/03  09:50:39  bog
; Make NEIGHBORS a SYSTEM.INI parameter.
; 
; Revision 1.6  93/02/01  12:15:26  bog
; First invocation from InterCodeBook could start with an fINVALID code.
; Changed to linear search if pEC->pECodes[pV->Code].Flags is fINVALID or
; we're doing unlocked only and fUNLOCKED is not set.
; 
; Revision 1.5  93/01/31  13:11:06  bog
; Add logic to refine only vectors and codebook entries that are unlocked on
; interframes.  Peter calls it "MSEFractional".
; 
; Revision 1.4  93/01/25  21:41:53  geoffs
; Add labels to narrow profiling buckets.
; 
; Revision 1.3  93/01/25  14:22:14  bog
; Can't search Neighbor[] beyond valid entries.
; 
; Revision 1.2  93/01/25  10:01:54  bog
; Limit Neighbor[] search to valid entries.
; 
; Revision 1.1  93/01/22  11:46:35  bog
; Initial revision
; 

; CompactVideo Match a vector to the nearest codebook entry


ifndef	WIN32
	.model	small, c
	.386

	extrn	flatSel:WORD
	extrn	PASCAL GetSelectorBase:FAR

XAX	textequ	<ax>
XBX	textequ	<bx>
XCX	textequ	<cx>
XDI	textequ	<di>
XSI	textequ	<si>
else
	.386
	.model	small, c

XAX	equ	<eax>
XBX	equ	<ebx>
XCX	equ	<ecx>
XDI	equ	<edi>
XSI	equ	<esi>
endif

	option	noscoped	; allow public labels within PROCs

	.code

	include	cvcompre.inc


; DiffXBX moves or adds to eax the difference between bytes at one and
; two.  It crashes (e)bx.  (E)bx.hi is assumed to be zero.

DiffXBX	macro	one, two, op
	mov	bl,one
	sub	bl,two
	sbb	bh,bh

ifdef	WIN32

	inc	bh
	op	eax,LongSquares[ebx*4][-00100h*4]

else

	shl	bx,2
	op	eax,LongSquares[bx]
endif

	endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


ifdef	WIN32

i	=	-255
	rept	511
	if i EQ 0
	public	LongSquares
LongSquares	LABEL	DWORD
	endif
	dd	i*i
i	=	i+1
	endm

else

	extrn	LongSquares	:dword	; in libinit.asm

endif



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; unsigned long Match(
;   VECTOR far *pV,		// vector to match
;   EXTCODE far *pEC,		// -> base of code table
;   short nCodes,		// number of codes against which to match
;   short nNeighbors,		// number of entries in the Neighbor[] table
;   unsigned char Flags		// whether to look at codes that are locked
; );

; Match a vector to the nearest valid entry in a codebook.
; Update pV->Code.

; nCodes assumed to be > 0; SplitCodes would run first.

; Return the match error.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ifndef	WIN32
Match	PROC	NEAR	C	PUBLIC	uses ds si di,
    pV:		FAR PTR VECTOR,
    pEC:	FAR PTR EXTCODE,; offset must be 0
    nCodes:	SWORD,		; number of entries in pEC
    nNeighbors:	WORD,		; number of entries in pEC->Neighbor[]
    Flags:	BYTE		; 0 or fUNLOCKED: nz if skip locked

  LOCAL \
    V:		VECTOR,		; local copy of vector we're matching
    CurrError:	DWORD,		; squared distance from V to *pCurr
    BestError:	DWORD,		; best error found so far
    pBest:	NEAR PTR EXTCODE,; best code found so far
    Best:	BYTE		; index to best so far
else
Match	PROC	NEAR	PUBLIC	uses ebx esi edi,
    pV:		PTR VECTOR,
    pEC:	PTR EXTCODE,
    nCodes:	WORD,
    nNeighbors:	WORD,
    Flags:	BYTE

    LOCAL	V:	VECTOR		; local copy of vector we're matching
    LOCAL	CurrError:	DWORD	; squared distance from V to *pCurr
    LOCAL	BestError:	DWORD	; best error found so far
    LOCAL	pBest:	PTR EXTCODE	; best code found so far
    LOCAL	Best:	BYTE		; best code so far
endif

    pCurr	textequ	<(EXTCODE PTR ds:[XSI])>
    pTry	textequ	<(EXTCODE PTR ds:[XDI])>

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Each codebook entry has in it a list of the NEIGHBORS nearest other
; codebook entries.  Since the incoming vector previously matched
; pV->Code, it is likely that it is still near that same entry since
; the only motion we've had is successive refinement.

; If the distance from the codebook entry we're looking at to its
; nearest neighbor is more than twice the disance (four times the
; squared distance) from the entry to the vector, we know that no
; other code can be closer than the one we're looking at, so we have
; our result.

; If the vector is farther from the current code than half the
; distance to the code's nearest neighbor, we look through the nearest
; neighbor list.  We divide the neighbors into those that are closer
; than twice the distance from the vector to the codebook entry and
; those that are equal in distance or farther.  If there are any
; nearer, we know that the closest codebook entry is among that set,
; so we linearly search for the closest entry.

; If every neighbor is less than twice as far from the codebook entry as
; the vector is, we search through the neighbors list for a codebook
; entry that is nearer the vector than the codebook entry.  If we find
; one, we make it the current entry and start again at the top.

; If no neighbor in the list is nearer the vector than the current
; code (an unusual circumstance) then we must grossly search the
; coodbook for the the first entry nearer the vector than the current
; one, and start again.

; If no other entry in the list is closer than the current one, then
; the current one is obviously the nearest code.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	public	MatchEntry
MatchEntry:
ifndef	WIN32
	shl	nNeighbors,2	; make it offset beyond top of Neighbor[]
	.errnz	4-(type NEIGHBOR)

	les	si,pV		; point at the vector
	assume	es:nothing
	mov	eax,es:[si]	; remember in local
	mov	dword ptr V,eax
	mov	eax,es:[si][4]
	mov	dword ptr V[4],eax

	lds	si,pEC		; point at codebook
else
	shl	nNeighbors,3	; make it offset beyond top of Neighbor[]
	.errnz	8-(size NEIGHBOR)

	mov	esi,pV		; point at the vector
	mov	eax,[esi]	; remember in local
	mov	dword ptr V,eax
	mov	eax,[esi][4]
	mov	dword ptr V[4],eax

	mov	esi,pEC		; point at codebook
endif

	xor	XAX,XAX
	mov	al, V.vCode
	imul	XAX,size EXTCODE	; index into current code
	add	XSI,XAX

	xor	ebx,ebx

	mov	bh,fINVALID	; for test invalid or (Flags and locked)
	mov	bl,Flags
	or	bh,bl		; fINVALID or (fINVALID | fUNLOCKED)

	mov	al,pCurr.ecFlags
	xor	al,bl		; fUNLOCKED now like fLOCKED if Flags
	test	al,bh		; z if valid and (!Flags or fUNLOCKED)
	jnz	CurrInvalid	; jump if invalid entry

	; Compute eax = distance from V to *pCurr

	DiffXBX	V.vyuv.yuvu, pCurr.ecyuv.yuvu, mov
	DiffXBX	V.vyuv.yuvv, pCurr.ecyuv.yuvv, add
	shl	eax,2
	DiffXBX	V.vyuv.yuvy[0],pCurr.ecyuv.yuvy[0], add
	DiffXBX	V.vyuv.yuvy[1],pCurr.ecyuv.yuvy[1], add
	DiffXBX	V.vyuv.yuvy[2],pCurr.ecyuv.yuvy[2], add
	DiffXBX	V.vyuv.yuvy[3],pCurr.ecyuv.yuvy[3], add

TryAgain:
	mov	pBest,XSI
	mov	CurrError,eax
	mov	BestError,eax
	mov	bl,pCurr.ecCode
	mov	Best,bl

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; If less than half the distance to Curr's nearest neighbor, we have
; found the nearest code.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	shl	eax,2		; 4 * squared error is twice distance

  ; if farthest Neighbor is nearer than twice the distance from the
  ; vector to Curr, then there is no guarantee that the best match is in
  ; the Neighbors table.  Nerping through the Neighbor table thus
  ; doesn't help us.

ifndef	WIN32
	mov	cx,nNeighbors
else
	xor	ecx,ecx
	mov	cx,nNeighbors
endif
	or	XCX,XCX
	jz	MustLinear	; jump if no Neighbors table

	cmp	eax,0ffffh	; beyond infinite?
	ja	MustLinear	; jump if can't find in Neighbor

  ; Note that equals need to find the nearest with lowest index

	cmp	ax,pCurr.ecNeighbor[0].neHowFar; got match?
	jb	QuickOut	; jump if Curr is closest

	mov	XBX,XCX		; pCurr[bx] is one beyond nNeighbors
	cmp	ax,pCurr.ecNeighbor[XBX][-(size NEIGHBOR)].neHowFar
	ja	MustWalk	; no Neighbor is close enough to hit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; If we are as close as half the distance to any neighbor, our
; closest match must be among those in the list from Curr up to but
; not including that neighbor.

; We find the dividing line by bisection.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;     eax	CurrError << 2, < ffff
;     (e)cx	nNeighbors * sizeof(NEIGHBORS)

	xor	XDI,XDI

	public	MatchTryNeighbor
MatchTryNeighbor:
TryNeighbor:

;     eax	CurrError << 2, < ffff
;     (e)cx	iAbove
;     ds:(e)si	-> current EXTCODE under consideration
;     (e)di	iBottom

	cmp	XCX,XDI
	jbe	MaybeGotNeighbor

	lea	XBX,[XDI][-(size NEIGHBOR)]
	add	XBX,XCX
	shr	XBX,1		; iMid
	and	bl,-(size NEIGHBOR)

;     eax	CurrError << 2, < ffff
;     bx	iMid
;     cx	iAbove
;     ds:si	-> current EXTCODE under consideration
;     di	iBottom

	cmp	ax,pCurr.ecNeighbor[XBX].neHowFar
	jb	@F		; jump if CurrError < Neighbor[iMid].HowFar

	lea	XDI,[XBX][size NEIGHBOR]
	jmp	TryNeighbor

@@:
	mov	XCX,XBX
	jmp	TryNeighbor


MaybeGotNeighbor:
	cmp	cx,nNeighbors	; iAbove < NEIGHBORS?
	jae	MustWalk	; jump if must walk Neighbor table

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Either Curr or one of the first iAbove codebook entries in the
; neighbor list is the winner.

; We simply grind through, looking for the best.

; BestError and pBest were initialized above

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	public	MatchGrind
MatchGrind:
	mov	edx,BestError
	neg	edx

GrindLoop:

;     (e)cx	iAbove
;     edx	-(error to beat)
;     ds:si	-> current EXTCODE under consideration

	sub	XCX,TYPE NEIGHBOR; iAbove--
	jc	BestIsMatch	; jump if looked at all iAbove neighbors

	mov	XBX,XCX
	mov	XDI,pCurr.ecNeighbor[XBX].nepWho

	; Compute eax = distance from V to *pTry

	DiffXBX	V.vyuv.yuvu, pTry.ecyuv.yuvu, mov
	DiffXBX	V.vyuv.yuvv, pTry.ecyuv.yuvv, add
	shl	eax,2
	add	eax,edx
	jg	GrindLoop

	DiffXBX	V.vyuv.yuvy[0],pTry.ecyuv.yuvy[0], add
	jg	GrindLoop

	DiffXBX	V.vyuv.yuvy[1],pTry.ecyuv.yuvy[1], add
	jg	GrindLoop

	DiffXBX	V.vyuv.yuvy[2],pTry.ecyuv.yuvy[2], add
	jg	GrindLoop

	DiffXBX	V.vyuv.yuvy[3],pTry.ecyuv.yuvy[3], add
	jg	GrindLoop	; jump if TryError > BestError

	mov	bl,pTry.ecCode
	jne	@F		; jump if TryError < BestError

	cmp	bl,Best		; TryError == BestError
	ja	GrindLoop	; jump if Try > Best

@@:
	sub	eax,edx
	mov	BestError,eax	; note new best
	mov	pBest,XDI
	mov	Best,bl
	mov	edx,eax
	neg	edx

	jmp	GrindLoop

BestIsMatch:
	mov	XSI,pBest	; pCurr <- pBest
	mov	eax,BestError	; return match error

QuickOut:
	shld	edx,eax,16

;     eax	BestError
;     ds:si	-> nearest EXTCODE

	mov	cl,pCurr.ecCode	; remember matching code in vector
ifndef	WIN32
	mov	si,word ptr pV
	mov	(VECTOR PTR es:[si]).vCode,cl
else
	mov	esi,pV
	mov	(VECTOR PTR [esi]).vCode,cl
endif

	ret




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; There might be some codebook entry not in the neighbor list that
; is closer to the vector than any in the neighbor list.  If it
; turns out that the vector is closer to some neighbor than to
; Curr, we just start over again with that one being Curr.
; Otherwise, we must linearly search.


; Walk through the Neighbors table until we either fall off the end
; or find a neighbor nearer than Curr.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	public	MatchWalk
MatchWalk:
MustWalk:

;     ds:si	-> current EXTCODE under consideration

	mov	XCX,-(size NEIGHBOR)
	mov	edx,BestError
	neg	edx

WalkLoop:
	add	XCX,size NEIGHBOR

;     (e)cx	indexing through the Neighbors table
;     edx	-(error to beat)
;     ds:si	-> current EXTCODE under consideration

	cmp	cx,nNeighbors
	jae	MustLinear	; jump if no neighbor is closer

	mov	XBX,XCX
	mov	XDI,pCurr.ecNeighbor[XBX].nepWho

	; Compute eax = distance from V to *pTry

	DiffXBX	V.vyuv.yuvu, pTry.ecyuv.yuvu, mov
	DiffXBX	V.vyuv.yuvv, pTry.ecyuv.yuvv, add
	shl	eax,2
	add	eax,edx
	jg	WalkLoop

	DiffXBX	V.vyuv.yuvy[0],pTry.ecyuv.yuvy[0], add
	jg	WalkLoop

	DiffXBX	V.vyuv.yuvy[1],pTry.ecyuv.yuvy[1], add
	jg	WalkLoop

	DiffXBX	V.vyuv.yuvy[2],pTry.ecyuv.yuvy[2], add
	jg	WalkLoop

	DiffXBX	V.vyuv.yuvy[3],pTry.ecyuv.yuvy[3], add
	jge	WalkLoop	; jump if TryError >= CurrError

  ; The code at pTry is closer than pCurr.  Make it pCurr and start
  ; again.

	sub	eax,edx
	mov	XSI,XDI		; pCurr <- pTry
	jmp	TryAgain




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Continue our linear search from wherever we last left off.

; We break out of our search if we run off the end of the codebook
; (in which case Curr is our best match) or if we find a code that
; is closer to our vector than Curr, in which case we switch to it
; and start again.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	public	MatchLinear
MatchLinear:

CurrInvalid:
	mov	CurrError,07fffffffh; no valid entry yet
	mov	BestError,07fffffffh

MustLinear:
	mov	edx,BestError
	neg	edx

LinearLoop:
	mov	bh,fINVALID	; for test invalid or (Flags and locked)
	mov	bl,Flags
	or	bh,bl		; fINVALID or (fINVALID | fUNLOCKED)

TryLinearAgain:

;     bl	Flags
;     bh	Flags ? (fINVALID | fUNLOCKED) : fINVALID
;     edx	-(error to beat)
;     ds:si	-> current EXTCODE under consideration

	dec	nCodes		; any linear searching undone?
	js	BestIsMatch	; jump since Curr (Best) is best match

ifndef	WIN32
	mov	di,word ptr pEC
	add	word ptr pEC,TYPE EXTCODE
else
	mov	edi,pEC		; pTry <- pEC++
	add	pEC,type EXTCODE
endif

	mov	al,pTry.ecFlags
	xor	al,bl		; fUNLOCKED now like fLOCKED if Flags
	test	al,bh		; z if valid and (!Flags or fUNLOCKED)
	jnz	TryLinearAgain	; jump if invalid entry

	mov	al,pCurr.ecCode
	cmp	al,pTry.ecCode
	je	TryLinearAgain	; jump if pCurr.ecCode == pTry.ecCode

	; Compute eax = distance from V to *pTry

	DiffXBX	V.vyuv.yuvu, pTry.ecyuv.yuvu, mov
	DiffXBX	V.vyuv.yuvv, pTry.ecyuv.yuvv, add
	shl	eax,2
	add	eax,edx
	jg	LinearLoop

	DiffXBX	V.vyuv.yuvy[0],pTry.ecyuv.yuvy[0], add
	jg	LinearLoop

	DiffXBX	V.vyuv.yuvy[1],pTry.ecyuv.yuvy[1], add
	jg	LinearLoop

	DiffXBX	V.vyuv.yuvy[2],pTry.ecyuv.yuvy[2], add
	jg	LinearLoop

	DiffXBX	V.vyuv.yuvy[3],pTry.ecyuv.yuvy[3], add
	jg	LinearLoop	; jump if TryError > BestError

	jne	@F		; jump if TryError < BestError

	mov	bl,pTry.ecCode	; TryError == BestError
	cmp	bl,Best
	jae	LinearLoop	; jump if Try >= Best

@@:

  ; The code at pTry is closer than pCurr.  Make it pCurr and start
  ; again.

	sub	eax,edx
	mov	XSI,XDI		; pCurr <- pTry
	jmp	TryAgain

Match	ENDP


	end
