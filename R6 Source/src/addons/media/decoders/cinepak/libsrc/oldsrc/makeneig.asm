; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/makeneig.asm 2.8 1994/06/23 14:13:27 bog Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: makeneig.asm $
; Revision 2.8  1994/06/23 14:13:27  bog
; Change movzx into xor/mov pair.
; Revision 2.7  1994/05/12  11:09:40  timr
; Oops; longs are 4 bytes.
; 
; Revision 2.6  1994/04/30  13:03:58  bog
; Avoid unnecessary calls to InsertNeighbor.
; 
; Revision 2.5  1993/09/02  16:33:04  timr
; Change distance computation.  I was mishandling carry.
; 
; Revision 2.4  1993/08/09  16:43:10  timr
; Correct typo introduced in Win32-izing, causing eventual crash in Match.
; 
; Revision 2.3  1993/08/04  19:07:55  timr
; Both compressor and decompressor now work on NT.
; 
; Revision 2.2  1993/07/09  14:34:01  timr
; 2nd pass at Win32-izing.
; 
; Revision 2.1  1993/07/06  09:11:45  geoffs
; 1st pass WIN32'izing
; 
; Revision 2.0  93/06/01  14:16:21  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.9  93/04/21  15:57:26  bog
; Fix up copyright and disclaimer.
; 
; Revision 1.8  93/02/05  17:01:06  bog
; Behave correctly when NEIGHBORS is 0.
; 
; Revision 1.7  93/02/03  13:55:56  bog
; CX was getting crashed by InsertNeighbor.
; 
; Revision 1.6  93/02/03  09:50:33  bog
; Make NEIGHBORS a SYSTEM.INI parameter.
; 
; Revision 1.5  93/02/01  12:15:07  bog
; One of the searches was not excluding fINVALID.
; 
; Revision 1.4  93/01/31  13:11:34  bog
; Add logic to refine only vectors and codebook entries that are unlocked on
; interframes.  Peter calls it "MSEFractional".
; 
; Revision 1.3  93/01/25  14:26:45  geoffs
; Match() needs valid Neighbor[] count.
; 
; Revision 1.2  93/01/25  09:35:14  geoffs
; First working version
; 
; Revision 1.1  93/01/22  11:46:37  bog
; Initial revision
; 
;
; CompactVideo Key CodeBook Build and Refine

; -------------------------------------------------------
;               DATA SEGMENT DECLARATIONS
; -------------------------------------------------------

ifndef	WIN32
	.model	small,c
	.386
else
	.386
	.model	small,c
endif

	.code

ifndef	WIN32

XAX	textequ	<ax>
XBX	textequ	<bx>
XSI	textequ	<si>
XDI	textequ	<di>
XDX	textequ	<dx>

else

XAX	textequ	<eax>
XBX	textequ	<ebx>
XSI	textequ	<esi>
XDI	textequ	<edi>
XDX	textequ	<edx>

endif

pecSrc	textequ	<(EXTCODE PTR [XSI])>
pecSrce	textequ	<(EXTCODE PTR [esi])>
pecDst	textequ	<(EXTCODE PTR [XDI])>
pecDste	textequ	<(EXTCODE PTR [edi])>
pneSrc	textequ	<(NEIGHBOR PTR [XSI])>

        assume	cs:@code
	assume	ds:@data
        assume	es:nothing
        assume	fs:nothing
        assume	gs:nothing

	include	cvcompre.inc

	extrn	LongSquares:DWORD

; We make assumptions that the number of neighbors will never be > 255 and
; that the size of a NEIGHBOR struct is equal to a dword

	.errnz	NEIGHBORS GT 255
ifndef	WIN32
	.errnz	type NEIGHBOR - 4
else
	.errnz	type NEIGHBOR - 8
endif

; DiffXBX produces in XBX the difference between bytes at one and two.
; and then operates on ecx with op

DiffXBX	macro	one, two, op
ifdef	WIN32
	mov	al,one
	sub	al,two
	sbb	ebx,ebx
	mov	bl,al
	op	ecx, LongSquares[ebx*4]
else
	mov	bl,one
	sub	bl,two
	sbb	bh,bh
	shl	bx,2
	op	ecx, LongSquares[bx]
endif
	endm

;**********************************************************************
;*
;* MakeNeighbors()
;*
;* Build the nearest neighbors part of the codebook extension
;*
;* Return # neighbors in neighbor tables
;*
;**********************************************************************/
;
;int MakeNeighbors(
;  EXTCODE far *pEC,		/* extended codebook */
;  short nCodes,		// number of entries
;  unsigned short nNeighbors,	// maximum Neighbors in list
;  unsigned char Flags		// whether to look at codes that are locked
;)
;/*
;  Run through the codes in pEC, building a nearest neighbors list for
;  each entry.
;
;  Each codebook entry has a list of that code's nearest neighbors.  We
;  maintain this list to speed up the "find nearest match" process.
;  There are NEIGHBORS slots in an EXTCODE for neighbors.  Each slot
;  holds the code for a neighbor and the distance to that neighbor.
; */
;{

ifndef	WIN32

MakeNeighbors	PROC	USES ds esi edi,
		pEC:FAR PTR EXTCODE,
		nCodes:SWORD,
		nNeighbors:WORD,
		Flags:BYTE

	mov	ax,nNeighbors
else

	public	MakeNeighbors

MakeNeighbors	PROC	NEAR USES ebx esi edi,	\
		pEC:	ptr EXTCODE,		\
		nCodes:	DWORD,		\
		nNeighbors:WORD,	\
		Flags:	BYTE

	movzx	eax,nNeighbors
endif

	or	XAX,XAX			; if nNeighbors is zero,
	jz	ZeroQuit		; we do nothing

	std				; our string moves work backwards
	xor	esi,esi			; make sure hi halves zero'ed
	xor	edi,edi

; Remember offsets to base,(last + 1) of extended code lists

ifndef	WIN32
	lds	si,pEC			; DS:SI is -> EXTCODEs
	assume	ds:nothing
	mov	ax,ds
	mov	es,ax
	assume	es:nothing		; ES:?? is -> EXTCODEs
else
	mov	esi,pEC
endif

	mov	XAX, nCodes
	mov	XBX,size EXTCODE
	mul	XBX
	add	XAX,XSI			; EAX is -> 1 EXTCODE beyond end
	mov	nCodes,XAX		; (last + 1)

;  /*
;    Skip any invalid codes at the beginning
;   */
;  for (; nCodes; nCodes--, pEC++) {
;    if (!(pEC->Flags & fINVALID)) {
;      break;
;    }
;  }
;  if (nCodes) {		/* if there are some valid codes */
;    auto unsigned short iNew;	/* new code to consider */
;    auto unsigned short iValidNew;/* counting valid codes */
;    auto EXTCODE far *pNew;	/* -> new code to consider */
;    auto unsigned char iPrev;	/* already built code we're looking at */
;    auto EXTCODE far *pPrev;	/* -> already built code we're looking at */

	mov	bl,fINVALID		; for valid and (!Flags or unlocked)
	mov	cl,Flags
	or	bl,cl			; fINVALID or (fINVALID | fUNLOCKED)

@@:	cmp	XSI,XAX			; cycled through all EXTCODEs?
	jae	mn_exit			; yes -- none valid or empty list...

	mov	ch,pecSrc.ecFlags
	xor	ch,cl			; fUNLOCKED now 1 if locked and Flags
	test	ch,bl			; z if valid and (!Flags or fUNLOCKED)
	jz	@F			; have 1st valid code...

	add	XSI,size EXTCODE	; -> next extended code
	jmp	@B			; for codes in list or until valid...
@@:
	
;    for (
;      iNew = 1,
;	iValidNew = 1,
;	pNew = pEC + 1;
;      iNew < (unsigned) nCodes;
;      iNew++,
;	pNew++
;    ) {
;      auto int PrevToCheck;	/* counting down already built codes */
;      auto int nSlotsSearch;	/* number of slots to search in old */
;      auto int nSlotsNew;	/* number of slots to build in new */
;      auto int nSlotsNewSoFar;	/* number of slots built in new */

	mov	XDI,XSI			; -> 1st valid code, will be next code
	xor	edx,edx			; initial nSlotsNew

; The code below is placed at loop top to make more efficient the assembler
; loops. It is equivalent to the iValidNew++ line way below...

mn_bumpValidCodes:

	shr	edx,16			; access to [nSlotsSearch,nSlotsNew]

	inc	dl			; bump nSlotsNew
	mov	dh,dl
	dec	dh			; nSlotsSearch = nSlotsNew - 1

	cmp	dl,byte ptr nNeighbors	; reached limit?
	jbe	@F			; no...
	mov	dl,byte ptr nNeighbors	; yes -- saturated value
	mov	dh,dl
@@:

	mov	al,dl
	shl	edx,16			; put them back in safe place
	mov	dl,al			; clone nSlotsNew for InsertNeighbor

	mov	bl,fINVALID		; for valid and (!Flags or unlocked)
	mov	cl,Flags
	or	bl,cl			; fINVALID or (fINVALID | fUNLOCKED)

mn_nextCode:

	add	XDI,size EXTCODE	; -> next code
	cmp	XDI,nCodes		; looked at all codes?
	jae	mn_exit			; yes -- we're done...

;      if (!(pNew->Flags & fINVALID)) {/* look only at valid codes */

	mov	ch,pecDst.ecFlags
	xor	ch,cl			; fUNLOCKED now 1 if locked and Flags
	test	ch,bl			; z if valid and (!Flags or fUNLOCKED)
	jnz	mn_nextCode		; find next valid code...

;	/*
;	  We cannot build more than iValidNew slots in the new guy nor
;	  search more than iValidNew-1 slots in the previous guys.
;	 */
;	nSlotsNew = (iValidNew <= NEIGHBORS) ? iValidNew : NEIGHBORS;
;	nSlotsSearch = (iValidNew <= NEIGHBORS) ? (iValidNew - 1) : NEIGHBORS;

;	for (
;	  iPrev = 0,
;	    pPrev = pEC,
;	    PrevToCheck = iNew,
;	    nSlotsNewSoFar = 0;
;	  PrevToCheck--;
;	  iPrev++,
;	    pPrev++
;	) {
;	  auto unsigned long LDist;/* distance between pPrev-> and pNew-> */
;	  auto unsigned short Distance;/* pinned at ffff */

ifndef	WIN32
	mov	si,word ptr pEC[0]	; -> start of extended codes
else
	mov	esi,pEC			; -> start of extended codes
endif
	sub	XSI,size EXTCODE	; seed loop

mn_searchCodes:

	mov	bl,fINVALID		; for valid and (!Flags or unlocked)
	mov	cl,Flags
	or	bl,cl			; fINVALID or (fINVALID | fUNLOCKED)

TrySearchCodes:

	add	XSI,size EXTCODE	; -> next code to search
	cmp	XSI,XDI			; searched all previous?
	jae	mn_bumpValidCodes	; yes -- do next code...

	mov	ch,pecSrc.ecFlags
	xor	ch,cl			; fUNLOCKED now 1 if locked and Flags
	test	ch,bl			; z if valid and (!Flags or fUNLOCKED)
	jnz	TrySearchCodes		; find next valid code...

;	  LDist = LongDistance(pPrev->yuv, pNew->yuv);

	DiffXBX	pecSrc.ecyuv.yuvu, pecDst.ecyuv.yuvu, mov
	DiffXBX	pecSrc.ecyuv.yuvv, pecDst.ecyuv.yuvv, add

	shl	ecx,2		; chroma weighted per pixel, not per patch

	DiffXBX	pecSrc.ecyuv.yuvy[0], pecDst.ecyuv.yuvy[0], add
	DiffXBX	pecSrc.ecyuv.yuvy[1], pecDst.ecyuv.yuvy[1], add
	DiffXBX	pecSrc.ecyuv.yuvy[2], pecDst.ecyuv.yuvy[2], add
	DiffXBX	pecSrc.ecyuv.yuvy[3], pecDst.ecyuv.yuvy[3], add

;	  Distance = (LDist > 0x0000ffffL) ? 0xffff : (unsigned short) LDist;

	add	ecx,0ffff0000h		; 'C' = (> 64k-1) ? 1 : 0
	sbb	ax,ax			; AX = (> 64k-1) ? 64k-1 : 0
	or	cx,ax			; Dist = (> 64k-1) ? 64k-1 : Dist

;	  /*
;	    Check and insert new in previous neighbor list
;	   */
;	  if (
;	    (nSlotsSearch < nSlotsNew) ||
;	    (Distance < pPrev->Neighbor[nSlotsSearch - 1].HowFar)
;	  ) {
;	    InsertNeighbor(
;	      pPrev->Neighbor,	/* nearest neighbor table getting insert */
;	      nSlotsSearch,	/* number of slots now in table */
;	      nSlotsNew,	/* maximum number slots allowed in table */
;	      Distance,		/* how far to new code */
;	      (unsigned char) iNew/* new code */
;	    );
;	  }

	rol	edx,16			; EDX.lo = [nSlotsSearch,nSlotsNew]

	cmp	dh,dl			; nSlotsSearch - nSlotsNew
	jl	InsertInPrev

	xor	eax,eax
	mov	al,dh			; nSlotsSearch
	cmp	pecSrce.ecNeighbor[eax * (size NEIGHBOR)][-(size NEIGHBOR)].neHowFar,cx
	jbe	AfterInsertInPrev	; jump if too big to insert

InsertInPrev:
	call	InsertNeighbor		; insert new into previous

AfterInsertInPrev:
	rol	edx,16			; EDX.hi = [nSlotsSearch,nSlotsNew]

;	  /*
;	    Check and insert previous in new neighbor list
;	   */
;	  if (
;	    (nSlotsNewSoFar < nSlotsNew) ||
;	    (Distance < pNew->Neighbor[nSlotsNewSoFar - 1].HowFar)
;	  ) {
;	    nSlotsNewSoFar += InsertNeighbor(
;	      pNew->Neighbor,	/* nearest neighbor table getting insert */
;	      nSlotsNewSoFar,	/* number of slots now in table */
;	      nSlotsNew,	/* maximum number slots allowed in table */
;	      Distance,		/* how far to new code */
;	      iPrev		/* new code */
;	    );
;	  }

	cmp	dh,dl			; nSlotsNewSoFar - nSlotsNew
	jl	InsertInNew		; jump if will insert

	xor	eax,eax
	mov	al,dh			; nSlotsNewSoFar
	cmp	pecDste.ecNeighbor[eax * (size NEIGHBOR)][-(size NEIGHBOR)].neHowFar,cx
	jbe	mn_searchCodes		; jump if too big to insert

InsertInNew:
	xchg	XSI,XDI			; new neighbor getting the insert
	call	InsertNeighbor
	add	dh,al			; bump new slots in table if grew
	xchg	XSI,XDI

	jmp	mn_searchCodes

;	}
;	iValidNew++;		/* note another valid entry */
;      }
;    }
;  }

mn_exit:

	shr	edx,24			; return # entries in neighbor tables
	mov	XAX,XDX

ZeroQuit:
	cld
	ret
MakeNeighbors	ENDP
;}



;/**********************************************************************
; *
; * InsertNeighbor()
; *
; * Looks up dNew in the table of size n at pN.  If dNew is less than any
; * entry in the table, a hole is dug and dNew and iNew are inserted.
; *
; * Returns 1 iff the table grew.
; *
; **********************************************************************/
;
;int InsertNeighbor(
;  NEIGHBOR far *pNeighbors,	/* 0th entry in neighbor table */
;  int nNow,			/* number of entries now in table */
;  int nMax,			/* maximum number of entries in table */
;  unsigned short dNew,		/* distance of entry to insert */
;  unsigned char iNew		/* code for insert */
;) {
;  auto int iBottom, iMid, iAbove;/* bisection indices */

InsertNeighbor	PROC
;		pNeighbors:NEIGHBOR FAR PTR,	; -> pEC in DS:SI
;		nNow:WORD,			; in DX.hi
;		nMax:WORD,			; in DX.lo
;		dNew:WORD,			; in CX
;		iNew:WORD			; in DI (really offset of)

nNow	equ	DH
nMax	equ	DL

iBottom	equ	BL
iAbove	equ	BH

iMid	equ	AL
iMid_32	equ	EAX

;  /*
;    We might insert at iBottom but not before.  We will insert below
;    iAbove.
;
;    We find the insertion spot via bisection.
;   */
;  for (
;    iBottom = 0,
;      iAbove = nNow;
;    iAbove > iBottom;
;  ) {
;    iMid = (iAbove + iBottom - 1) >> 1;
;
;    if (dNew >= pNeighbors[iMid].HowFar) {/* equal inserts after */
;
;      iBottom = iMid + 1;
;
;    } else {
;
;      iAbove = iMid;
;    }
;  }

	xor	iBottom,iBottom		;
	xor	iMid_32,iMid_32
	mov	iMid,nNow

ShrinkTop:
	mov	iAbove,iMid

bisect:	cmp	iAbove,iBottom
	jbe	in_searched

	mov	iMid,iAbove
	add	iMid,iBottom
	dec	iMid
	shr	iMid,1

	cmp	pecSrce.ecNeighbor[eax * (size NEIGHBOR)].neHowFar,cx
	ja	ShrinkTop

	mov	iBottom,iMid
	inc	iBottom
	jmp	bisect

in_searched:

;  /*
;    We insert in front of pNeighbors[iBottom].
;   */
;  if (iBottom < nMax) { /* if not trying to insert beyond end of table */

	xor	ax,ax

	cmp	iBottom,nMax
	jae	in_exit

;    iAbove = (nNow < nMax) ? nNow : (nMax - 1);

	mov	iAbove,nNow		; assume nNow < nMax
	cmp	nNow,nMax
	jb	@F
	mov	iAbove,nMax
	dec	iAbove
@@:

;    /*
;      Copy pNeighbors[iBottom..iAbove-1] to
;      pNeighbors[iBottom+1..iAbove] then put the new guy in
;      pNeighbors[iBottom].
;     */
;    for (
;      pNeighbors += iAbove,
;	iMid = iAbove - iBottom;
;      iMid--;
;      pNeighbors--,
;	pNeighbors[1] = pNeighbors[0]
;    );

	push	XSI
	push	XDI

;    eax.hi, ah	still 0

	mov	al,iAbove
; Because the rep movs below works backwards, we must start the copy with
; the final dword of the NEIGHBORS array.
	lea	edi,pecSrce.ecNeighbor[eax * (type NEIGHBOR)][type NEIGHBOR - 4]
	lea	XSI,[XDI][-(size NEIGHBOR)]

	sub	iAbove,iBottom
	mov	al,iAbove		; # NEIGHBORs to move

	xchg	cx,ax			; dwords to move in CX, distance in AX
ifdef	WIN32
 	movzx	ecx,cx
 	shl	ecx,1
endif
  rep	movs	es:dword ptr [XDI],ds:dword ptr [XSI]
	mov	cx,ax

; esi now points one dword PRIOR to the new insertion spot.

	pop	XDI

;    pNeighbors[0].HowFar = dNew;
;    pNeighbors[0].Who = iNew;

	mov	(pneSrc[4]).neHowFar,ax	; new distance
	mov	(pneSrc[4]).nepWho,XDI	; new NEIGHBOR

	pop	XSI

;    return(nMax > nNow);

	cmp	nMax,nNow
	seta	al
;  }
;  return(0);

in_exit:

	ret				; AX = (entry added) ? 1 : 0
InsertNeighbor	ENDP
;}

	end
