; ex:set sw=2 ts=8 wm=0:
; $Header: u:/rcs/cv/rcs/dyuvfrag.inc 2.6 1994/05/12 10:43:55 unknown Exp $

; (C) Copyright 1992-1993 SuperMac Technology, Inc.
; All rights reserved

; This source code and any compilation or derivative thereof is the sole
; property of SuperMac Technology, Inc. and is provided pursuant to a
; Software License Agreement.  This code is the proprietary information
; of SuperMac Technology and is confidential in nature.  Its use and
; dissemination by any party other than SuperMac Technology is strictly
; limited by the confidential information provisions of the Agreement
; referenced above.

; $Log: dyuvfrag.inc $
; Revision 2.6  1994/05/12 10:43:55  unknown
; We really DO need cs: fixed up, for NT x86.
; Revision 2.5  1994/04/13  08:04:19  timr
; Don't fixup references to CS:-relative data items.
; 
; Revision 2.4  1994/03/02  16:42:38  timr
; Allow up to 50 fixups per fragment.
; 
; Revision 2.3  1993/08/10  15:17:05  timr
; Remove extraneous comments.
; 
; Revision 2.2  1993/08/04  19:07:30  timr
; Both compressor and decompressor now work on NT.
; 
; Revision 2.1  1993/07/27  09:33:50  timr
; Finally assembles under NT MASM386.
; 
; Revision 2.0  93/06/01  14:14:47  bog
; Version 1.0 Release 1.3.0.1 of 1 June 1993.
; 
; Revision 1.12  93/04/21  15:48:40  bog
; Fix up copyright and disclaimer.
; 
; Revision 1.11  93/02/18  09:21:16  geoffs
; Corrected some of the -> checking code
; 
; Revision 1.10  93/02/16  14:48:42  geoffs
; Added recreate of smooth vectors from detail vectors
; 
; Revision 1.9  93/01/27  13:14:03  geoffs
; Added more pointer checking code, fixed up gpf in 2nd pass vertical filter
; 
; Revision 1.8  93/01/27  08:06:36  geoffs
; Too many endifs in Check macro
; 
; Revision 1.7  93/01/27  08:00:57  geoffs
; Added debug check for pointer ranges; added fix so that last input scan + 1 not processed
; 
; Revision 1.6  93/01/16  16:14:06  geoffs
; Rearranged the Y,U,V outputs to VECTOR format rather than the previous raster format
; 
; Revision 1.5  93/01/13  17:21:26  geoffs
; Add another fixup type (but not used yet)
; 
; Revision 1.4  93/01/13  10:36:29  geoffs
; The detail and smooth lists appear to be outputting consistent data now
; 
; Revision 1.3  93/01/11  18:45:02  geoffs
; Have all the fragment code in now, but not yet fully debugged
; 
; Revision 1.2  93/01/11  09:40:43  geoffs
; Don't get impatient -- we're almost there. Have only the 2nd vertical
; filtering pass on UV to go.
; 
; Revision 1.1  93/01/10  17:06:52  geoffs
; Initial revision

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

nested		=	0		; no nested FRAGPROCs yet

; types of fixups that compiler has to handle

FIXUP_EOT		equ	255	; end of fixup table
FIXUP_WORKYSTEP		equ	0	; instruction value * work buffer ystep
FIXUP_DSREL32		equ	1	; DS relative 32-bit fixup
FIXUP_JMPLP1		equ	2	; place to insert fixup for jmp to loop1
FIXUP_STOREPC0		equ	3	; temporarily save PC
FIXUP_USE_STOREDPC0	equ	4	; fixup with temporarily saved PC
FIXUP_USE_STOREDPC1	equ	5	; fixup with temporarily saved PC
FIXUP_INTERU2		equ	6	; store -> U2 intermediate
FIXUP_INTERV2		equ	7	; store -> V2 intermediate
FIXUP_WORKPLUS		equ	8	; store -> work buffer + offset to scan
FIXUP_WORKWIDTH		equ	9	; workystep >> instruction value
FIXUP_UVOFF		equ	10	; offset of V from U intermediate
FIXUP_SRCYSTEP		equ	11	; add src ystep * instruction value
FIXUP_SRCWIDTH		equ	12	; src width >> instruction value


; Macros that generate the fragment tables for the compiling code

genFIXUPS	macro	procName,num
	ifdef	_FIXUP_&procName&_&num
		db	_FIXUP_&procName&_&num&_TYPE_
ifdef	DEBUG
		dw	_FIXUP_&procName&_&num
else
		db	_FIXUP_&procName&_&num
endif
	endif
	endm

FRAGDECLARE	macro	procName
	public	procName
procName	label	far
ifndef	WIN32
	dd	__&procName		;; -> code fragment
	org	$-2			;; overlay hi word of previous
else
	dd	__&procName-baseFRAGTEXT32
endif
	dw	_SIZE_&procName		;; size of fragment

y	=	0
	repeat	50
	genFIXUPS	procName,%y
y	=	y+1
	endm
	db	-1			;; signals end of fixup table
ifdef	DEBUG
	dw	-1
else
	db	-1
endif
		endm


; Define a label for the start of the fragment

FRAGPROC	macro	procName
	if	nested EQ 0
fixupNum	=	0
	endif
nested		=	nested + 1
__&procName	label	far
OFF_&procName	=	offset $
		endm

; Define an assembler symbol that contains the length of the fragment

FRAGENDP	macro	procName
_SIZE_&procName	=	offset $ - offset __&procName
nested		=	nested - 1
		endm


; Define assembler symbols that contain the fixup type and offset from
; fragment begin

genFIXUP	macro	procName,fuNum,fixupType,fixupOffset
_FIXUP_&procName&_&fuNum&_TYPE_ =	fixupType
_FIXUP_&procName&_&fuNum	=	(offset $ - OFF_&procName - fixupOffset)
ifndef	DEBUG
	if	_FIXUP_&procName&_&fuNum GT 255
	%out	Fixup greater than 255 bytes!!!!
	endif
endif
		endm

FRAGFIXUP	macro	procName,fixupType,fixupOffset
	genFIXUP	procName,%fixupNum,fixupType,fixupOffset
fixupNum	=	fixupNum + 1
		endm


;-----------------------------------------------------------------------------
; Following defines and macro for pointer range checking
;-----------------------------------------------------------------------------

CHECKPRIV	equ	0
CHECKBITS	equ	1
CHECKDETAIL	equ	2
CHECKSMOOTH	equ	3

CheckPtr	macro	checkAgainst,rWho,incr,procName,fixupType
ifdef	DEBUG
if	checkAgainst EQ CHECKPRIV
if	incr EQ 0
	cmp	rWho,privBase
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jae	@F
	int	3
	int	0
@@:	cmp	rWho,privLimit
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jbe	@F
	int	3
	int	0
@@:
else
	push	eax
	mov	eax,rWho
	add	eax,incr
	ifnb	<fixupType>
	FRAGFIXUP	<procName>,<fixupType>,<4>
	endif
	cmp	eax,privBase
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jae	@F
	int	3
	int	0
@@:	cmp	eax,privLimit
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jbe	@F
	int	3
	int	0
@@:	pop	eax
endif
endif
if	checkAgainst EQ CHECKBITS
	cmp	rWho,bitsBase
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jae	@F
	int	3
	int	0
@@:	cmp	rWho,bitsLimit
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jbe	@F
	int	3
	int	0
@@:
endif
if	checkAgainst EQ CHECKDETAIL
if	incr EQ 0
	cmp	rWho,detailBase
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jae	@F
	int	3
	int	0
@@:	cmp	rWho,detailLimit
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jbe	@F
	int	3
	int	0
@@:
else
	push	eax
	mov	eax,rWho
	add	eax,incr
	ifnb	<fixupType>
	FRAGFIXUP	<procName>,<fixupType>,<4>
	endif
	cmp	eax,detailBase
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jae	@F
	int	3
	int	0
@@:	cmp	eax,detailLimit
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jbe	@F
	int	3
	int	0
@@:	pop	eax
endif
endif
if	checkAgainst EQ CHECKSMOOTH
if	incr EQ 0
	cmp	rWho,smoothBase
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jae	@F
	int	3
	int	0
@@:	cmp	rWho,smoothLimit
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jbe	@F
	int	3
	int	0
@@:
else
	push	eax
	mov	eax,rWho
	add	eax,incr
	ifnb	<fixupType>
	FRAGFIXUP	<procName>,<fixupType>,<4>
	endif
	cmp	eax,smoothBase
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jae	@F
	int	3
	int	0
@@:	cmp	eax,smoothLimit
ifdef WIN32
	FRAGFIXUP	<procName>,<FIXUP_DSREL32>,<4>
endif
	jbe	@F
	int	3
	int	0
@@:	pop	eax
endif
endif
endif
		endm
