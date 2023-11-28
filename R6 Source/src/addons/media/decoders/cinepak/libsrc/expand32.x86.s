SECTION .text

%include "cv.i"


GLOBAL ExpandCodeBook32

ALIGN 16
ExpandCodeBook32:
		push ebp
		mov ebp,esp
		push ebx
		push esi
		push edi

		sub esp,8

		mov	esi,[esp+16+8+4]		; pCBIn -> incoming codebook
		mov	eax,[esi]			; get type & size
		mov bl,al				; remember type
		
		shr eax,$10				; 16 in hex
		xchg al,ah				; eax: size of incoming chunk
		add eax,esi				; eax: first byte beyond chunk
		add esi,$4				; esi: first codebook entry
		sub eax,6				; eax: last posssible valid entry
		
;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook

		cmp	esi,eax		; empty codebook?
		ja	short Quit
		
		mov	edi,[esp+16+8+8]		; pCB

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook
		
		and	bl,$0ffff	; turn off detail/smooth bit
		
%ifndef	NOBLACKWHITE
		
		test	bl,kGreyBookBit	; grey scale codebook?
		jnz	near DoGrey
		
%endif
		
		cmp	bl,kFullDBookType
		je	short Key
		
		cmp	bl,kFullSBookType
		je	short Key
		
		cmp	bl,kPartialDBookType
		je	near Partial
		
		cmp	bl,kPartialSBookType
		je	near Partial
		
Huh:

; not recognized so we just return

Quit:

		add			esp,8

		pop edi
		pop esi
		pop ebx
		pop ebp
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Key:

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov [esp+4],eax
	align	4

KeyLoop:

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

	movsx	ecx,byte [esi+4]	; ecx:  U
	movsx	ebx,byte [esi+5]	; ebx:  V
	mov	ebp,ecx			; ebp:  U
	sar	ecx,1			; ecx:  U/2
	shl	ebp,1			; ebp:  U*2       =deltaB
	add	ecx,ebx			; ecx:  V+U/2
	shl	ebx,1			; ebx:  V*2       =deltaR
	neg	ecx				; ecx:  -(V+U/2)  =deltaG

;     eax	develop unscaled RGB components to index into Bounding24
;     ebx	V*2
;     ecx	-(V+U/2)
;     edx	developing scaled RGB
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	U*2

	xor	eax,eax
	mov	al,[esi]	; eax:  Y0

	xor	edx,edx
	mov	dl,[Bounding24+ebx+eax]	; edx:  000000R0

	shl	edx,16

	mov	dh,[Bounding24+ecx+eax]	; edx:  00R0G000

	mov	dl,[Bounding24+ebp+eax]	; edx:  00R0G0B0

	mov	[edi],edx		; save RGB0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,[esi+1]	; eax:  Y1

	xor	edx,edx
	mov	dl,[Bounding24+ebx+eax]	; edx:  000000R1

	shl	edx,16

	mov	dh,[Bounding24+ecx+eax]	; edx:  00R1G100

	mov	dl,[Bounding24+ebp+eax]	; edx:  00R1G1B1

	mov	[edi+4],edx		; save RGB1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,[esi+2]	; eax:  Y2

	xor	edx,edx
	mov	dl,[Bounding24+ebx+eax]	; edx:  000000R2

	shl	edx,16

	mov	dh,[Bounding24+ecx+eax]	; edx:  00R2G200

	mov	dl,[Bounding24+ebp+eax]	; edx:  00R2G2B2

	mov	[edi+8],edx		; save RGB2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,[esi+3]	; eax:  Y3

	xor	edx,edx
	mov	dl,[Bounding24+ebx+eax]	; edx:  000000R3

	shl	edx,16

	mov	dh,[Bounding24+ecx+eax]	; edx:  00R3G300

	mov	dl,[Bounding24+ebp+eax]	; edx:  00R3G3B3

	mov	[edi+12],edx		; save it

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV
	add	edi,$10		; bump to next RGB patch

	cmp	esi,[esp+4]	; any more codebook entries?

	jbe	near KeyLoop	; jump if more to do
	jmp	Quit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Partial:

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[esp+4],eax

	align	4

PartialLoadSwitches:

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

	jmp	short PartialTestSwitch


	align	4

PartialYUVLoop:

	mov	eax,[esp]	; self mod switches here
;PartialSwitches	label	dword

;     eax	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,eax		; replace this index?

PartialTestSwitch:

	mov	[esp],eax; save for next iteration

	jnc	near PartialYUVSkip

	jz	PartialLoadSwitches

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

	movsx	ecx,byte [esi+4]	; ecx:  U
	movsx	ebx,byte [esi+5]	; ebx:  V
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
	mov	al,byte [esi]	; eax:  Y0

	xor	edx,edx
	mov	dl,[Bounding24+ebx+eax]	; edx:  000000R0

	shl	edx,16

	mov	dh,[Bounding24+ecx+eax]	; edx:  00R0G000

	mov	dl,[Bounding24+ebp+eax]	; edx:  00R0G0B0

	mov	[edi],edx		; save RGB0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+1]	; eax:  Y1

	xor	edx,edx
	mov	dl,[Bounding24+ebx+eax]	; edx:  000000R1

	shl	edx,16

	mov	dh,[Bounding24+ecx+eax]	; edx:  00R1G100

	mov	dl,[Bounding24+ebp+eax]	; edx:  00R1G1B1

	mov	[edi+4],edx		; save RGB1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+2]	; eax:  Y2

	xor	edx,edx
	mov	dl,[Bounding24+ebx+eax]	; edx:  000000R2

	shl	edx,16

	mov	dh,[Bounding24+ecx+eax]	; edx:  00R2G200

	mov	dl,[Bounding24+ebp+eax]	; edx:  00R2G2B2

	mov	[edi+8],edx		; save RGB2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+3]	; eax:  Y3

	xor	edx,edx
	mov	dl,[Bounding24+ebx+eax]	; edx:  000000R3

	shl	edx,16

	mov	dh,[Bounding24+ecx+eax]	; edx:  00R3G300

	mov	dl,[Bounding24+ebp+eax]	; edx:  00R3G3B3

	mov	[edi+12],edx		; save it

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV

PartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp esi,[esp+4]
	
	jbe	near PartialYUVLoop
	jmp	Quit

%ifndef	NOBLACKWHITE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DoGrey:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,2		; last possible code

	cmp	bl,kFullDBookType + kGreyBookBit
	jz GreyKeyLoop
	cmp bl,kFullSBookType + kGreyBookBit
	jne	GreyPartial

	align	4

GreyKeyLoop:

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

	mov	ebx,[GreyDwordLookup+ebx*4]; Y0 Y0 Y0 Y0
;	Ref	TEXT32

	mov	ecx,[GreyDwordLookup+ecx*4]; Y1 Y1 Y1 Y1
;	Ref	TEXT32

	and	ebx,000ffffffh		; 00 Y0 Y0 Y0
	and	ecx,000ffffffh		; 00 Y1 Y1 Y1

	mov	[edi],ebx		; 00 Y0 Y0 Y0
	rol	edx,16
	mov	[edi+4],ecx		; 01 Y1 Y1 Y1

	xor	ebx,ebx
	mov	bl,dl			; Y2
	xor	ecx,ecx
	mov	cl,dh			; Y3

	mov	ebx,[GreyDwordLookup+ebx*4]; Y2 Y2 Y2 Y2
;	Ref	TEXT32

	mov	ecx,[GreyDwordLookup+ecx*4]; Y3 Y3 Y3 Y3
;	Ref	TEXT32

	and	ebx,000ffffffh		; 00 Y2 Y2 Y2
	and	ecx,000ffffffh		; 00 Y3 Y3 Y3

	mov	[edi+8],ebx		; 00 Y2 Y2 Y2
	mov	[edi+12],ecx		; 00 Y3 Y3 Y3

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyKeyLoop		; jump if more to do

	jmp	Quit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreyPartial:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook
	cmp	bl,kPartialDBookType + kGreyBookBit
	jz GreyPartialLoadSwitches
	cmp bl,kPartialSBookType + kGreyBookBit
	jne	near Huh		; invalid code

	align	4

GreyPartialLoadSwitches:

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

	jmp	short GreyPartialTestSwitch


	align	4

GreyPartialYUVLoop:

;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	add	ebp,ebp		; replace this index?

GreyPartialTestSwitch:
	jnc	GreyPartialYUVSkip
	jz	GreyPartialLoadSwitches

  ; we take the incoming YYYY at [esi] and develop a 2x2 patch:
  ;
  ;     RGB0  RGB1
  ;     RGB2  RGB3
  ;
  ; as seen on the screen.

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	mov	edx,[esi]		; Y3 Y2 Y1 Y0
	add	esi,4

	xor	ebx,ebx
	mov	bl,dl			; Y0
	xor	ecx,ecx
	mov	cl,dh			; Y1

	mov	ebx,[GreyDwordLookup+ebx*4]; Y0 Y0 Y0 Y0
;	Ref	TEXT32

	mov	ecx,[GreyDwordLookup+ecx*4]; Y1 Y1 Y1 Y1
;	Ref	TEXT32

	and	ebx,000ffffffh		; 00 Y0 Y0 Y0
	and	ecx,000ffffffh		; 00 Y1 Y1 Y1

	mov	[edi],ebx		; 00 Y0 Y0 Y0
	rol	edx,16
	mov	[edi+4],ecx		; 01 Y1 Y1 Y1

	xor	ebx,ebx
	mov	bl,dl			; Y2
	xor	ecx,ecx
	mov	cl,dh			; Y3

	mov	ebx,[GreyDwordLookup+ebx*4]; Y2 Y2 Y2 Y2
;	Ref	TEXT32

	mov	ecx,[GreyDwordLookup+ecx*4]; Y3 Y3 Y3 Y3
;	Ref	TEXT32

	and	ebx,000ffffffh		; 00 Y2 Y2 Y2
	and	ecx,000ffffffh		; 00 Y3 Y3 Y3

	mov	[edi+8],ebx		; 00 Y2 Y2 Y2
	mov	[edi+12],ecx		; 00 Y3 Y3 Y3


GreyPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyPartialYUVLoop

	jmp	Quit
%endif

.data
	db 000h,000h,000h,000h,000h,000h,000h,000h ; underflow
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h

Bounding24:
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

GreyDwordLookup:
	db	000h,000h,000h,000h
	db	001h,001h,001h,001h
	db	002h,002h,002h,002h
	db	003h,003h,003h,003h
	db	004h,004h,004h,004h
	db	005h,005h,005h,005h
	db	006h,006h,006h,006h
	db	007h,007h,007h,007h
	db	008h,008h,008h,008h
	db	009h,009h,009h,009h
	db	00ah,00ah,00ah,00ah
	db	00bh,00bh,00bh,00bh
	db	00ch,00ch,00ch,00ch
	db	00dh,00dh,00dh,00dh
	db	00eh,00eh,00eh,00eh
	db	00fh,00fh,00fh,00fh
	db	010h,010h,010h,010h
	db	011h,011h,011h,011h
	db	012h,012h,012h,012h
	db	013h,013h,013h,013h
	db	014h,014h,014h,014h
	db	015h,015h,015h,015h
	db	016h,016h,016h,016h
	db	017h,017h,017h,017h
	db	018h,018h,018h,018h
	db	019h,019h,019h,019h
	db	01ah,01ah,01ah,01ah
	db	01bh,01bh,01bh,01bh
	db	01ch,01ch,01ch,01ch
	db	01dh,01dh,01dh,01dh
	db	01eh,01eh,01eh,01eh
	db	01fh,01fh,01fh,01fh
	db	020h,020h,020h,020h
	db	021h,021h,021h,021h
	db	022h,022h,022h,022h
	db	023h,023h,023h,023h
	db	024h,024h,024h,024h
	db	025h,025h,025h,025h
	db	026h,026h,026h,026h
	db	027h,027h,027h,027h
	db	028h,028h,028h,028h
	db	029h,029h,029h,029h
	db	02ah,02ah,02ah,02ah
	db	02bh,02bh,02bh,02bh
	db	02ch,02ch,02ch,02ch
	db	02dh,02dh,02dh,02dh
	db	02eh,02eh,02eh,02eh
	db	02fh,02fh,02fh,02fh
	db	030h,030h,030h,030h
	db	031h,031h,031h,031h
	db	032h,032h,032h,032h
	db	033h,033h,033h,033h
	db	034h,034h,034h,034h
	db	035h,035h,035h,035h
	db	036h,036h,036h,036h
	db	037h,037h,037h,037h
	db	038h,038h,038h,038h
	db	039h,039h,039h,039h
	db	03ah,03ah,03ah,03ah
	db	03bh,03bh,03bh,03bh
	db	03ch,03ch,03ch,03ch
	db	03dh,03dh,03dh,03dh
	db	03eh,03eh,03eh,03eh
	db	03fh,03fh,03fh,03fh
	db	040h,040h,040h,040h
	db	041h,041h,041h,041h
	db	042h,042h,042h,042h
	db	043h,043h,043h,043h
	db	044h,044h,044h,044h
	db	045h,045h,045h,045h
	db	046h,046h,046h,046h
	db	047h,047h,047h,047h
	db	048h,048h,048h,048h
	db	049h,049h,049h,049h
	db	04ah,04ah,04ah,04ah
	db	04bh,04bh,04bh,04bh
	db	04ch,04ch,04ch,04ch
	db	04dh,04dh,04dh,04dh
	db	04eh,04eh,04eh,04eh
	db	04fh,04fh,04fh,04fh
	db	050h,050h,050h,050h
	db	051h,051h,051h,051h
	db	052h,052h,052h,052h
	db	053h,053h,053h,053h
	db	054h,054h,054h,054h
	db	055h,055h,055h,055h
	db	056h,056h,056h,056h
	db	057h,057h,057h,057h
	db	058h,058h,058h,058h
	db	059h,059h,059h,059h
	db	05ah,05ah,05ah,05ah
	db	05bh,05bh,05bh,05bh
	db	05ch,05ch,05ch,05ch
	db	05dh,05dh,05dh,05dh
	db	05eh,05eh,05eh,05eh
	db	05fh,05fh,05fh,05fh
	db	060h,060h,060h,060h
	db	061h,061h,061h,061h
	db	062h,062h,062h,062h
	db	063h,063h,063h,063h
	db	064h,064h,064h,064h
	db	065h,065h,065h,065h
	db	066h,066h,066h,066h
	db	067h,067h,067h,067h
	db	068h,068h,068h,068h
	db	069h,069h,069h,069h
	db	06ah,06ah,06ah,06ah
	db	06bh,06bh,06bh,06bh
	db	06ch,06ch,06ch,06ch
	db	06dh,06dh,06dh,06dh
	db	06eh,06eh,06eh,06eh
	db	06fh,06fh,06fh,06fh
	db	070h,070h,070h,070h
	db	071h,071h,071h,071h
	db	072h,072h,072h,072h
	db	073h,073h,073h,073h
	db	074h,074h,074h,074h
	db	075h,075h,075h,075h
	db	076h,076h,076h,076h
	db	077h,077h,077h,077h
	db	078h,078h,078h,078h
	db	079h,079h,079h,079h
	db	07ah,07ah,07ah,07ah
	db	07bh,07bh,07bh,07bh
	db	07ch,07ch,07ch,07ch
	db	07dh,07dh,07dh,07dh
	db	07eh,07eh,07eh,07eh
	db	07fh,07fh,07fh,07fh
	db	080h,080h,080h,080h
	db	081h,081h,081h,081h
	db	082h,082h,082h,082h
	db	083h,083h,083h,083h
	db	084h,084h,084h,084h
	db	085h,085h,085h,085h
	db	086h,086h,086h,086h
	db	087h,087h,087h,087h
	db	088h,088h,088h,088h
	db	089h,089h,089h,089h
	db	08ah,08ah,08ah,08ah
	db	08bh,08bh,08bh,08bh
	db	08ch,08ch,08ch,08ch
	db	08dh,08dh,08dh,08dh
	db	08eh,08eh,08eh,08eh
	db	08fh,08fh,08fh,08fh
	db	090h,090h,090h,090h
	db	091h,091h,091h,091h
	db	092h,092h,092h,092h
	db	093h,093h,093h,093h
	db	094h,094h,094h,094h
	db	095h,095h,095h,095h
	db	096h,096h,096h,096h
	db	097h,097h,097h,097h
	db	098h,098h,098h,098h
	db	099h,099h,099h,099h
	db	09ah,09ah,09ah,09ah
	db	09bh,09bh,09bh,09bh
	db	09ch,09ch,09ch,09ch
	db	09dh,09dh,09dh,09dh
	db	09eh,09eh,09eh,09eh
	db	09fh,09fh,09fh,09fh
	db	0a0h,0a0h,0a0h,0a0h
	db	0a1h,0a1h,0a1h,0a1h
	db	0a2h,0a2h,0a2h,0a2h
	db	0a3h,0a3h,0a3h,0a3h
	db	0a4h,0a4h,0a4h,0a4h
	db	0a5h,0a5h,0a5h,0a5h
	db	0a6h,0a6h,0a6h,0a6h
	db	0a7h,0a7h,0a7h,0a7h
	db	0a8h,0a8h,0a8h,0a8h
	db	0a9h,0a9h,0a9h,0a9h
	db	0aah,0aah,0aah,0aah
	db	0abh,0abh,0abh,0abh
	db	0ach,0ach,0ach,0ach
	db	0adh,0adh,0adh,0adh
	db	0aeh,0aeh,0aeh,0aeh
	db	0afh,0afh,0afh,0afh
	db	0b0h,0b0h,0b0h,0b0h
	db	0b1h,0b1h,0b1h,0b1h
	db	0b2h,0b2h,0b2h,0b2h
	db	0b3h,0b3h,0b3h,0b3h
	db	0b4h,0b4h,0b4h,0b4h
	db	0b5h,0b5h,0b5h,0b5h
	db	0b6h,0b6h,0b6h,0b6h
	db	0b7h,0b7h,0b7h,0b7h
	db	0b8h,0b8h,0b8h,0b8h
	db	0b9h,0b9h,0b9h,0b9h
	db	0bah,0bah,0bah,0bah
	db	0bbh,0bbh,0bbh,0bbh
	db	0bch,0bch,0bch,0bch
	db	0bdh,0bdh,0bdh,0bdh
	db	0beh,0beh,0beh,0beh
	db	0bfh,0bfh,0bfh,0bfh
	db	0c0h,0c0h,0c0h,0c0h
	db	0c1h,0c1h,0c1h,0c1h
	db	0c2h,0c2h,0c2h,0c2h
	db	0c3h,0c3h,0c3h,0c3h
	db	0c4h,0c4h,0c4h,0c4h
	db	0c5h,0c5h,0c5h,0c5h
	db	0c6h,0c6h,0c6h,0c6h
	db	0c7h,0c7h,0c7h,0c7h
	db	0c8h,0c8h,0c8h,0c8h
	db	0c9h,0c9h,0c9h,0c9h
	db	0cah,0cah,0cah,0cah
	db	0cbh,0cbh,0cbh,0cbh
	db	0cch,0cch,0cch,0cch
	db	0cdh,0cdh,0cdh,0cdh
	db	0ceh,0ceh,0ceh,0ceh
	db	0cfh,0cfh,0cfh,0cfh
	db	0d0h,0d0h,0d0h,0d0h
	db	0d1h,0d1h,0d1h,0d1h
	db	0d2h,0d2h,0d2h,0d2h
	db	0d3h,0d3h,0d3h,0d3h
	db	0d4h,0d4h,0d4h,0d4h
	db	0d5h,0d5h,0d5h,0d5h
	db	0d6h,0d6h,0d6h,0d6h
	db	0d7h,0d7h,0d7h,0d7h
	db	0d8h,0d8h,0d8h,0d8h
	db	0d9h,0d9h,0d9h,0d9h
	db	0dah,0dah,0dah,0dah
	db	0dbh,0dbh,0dbh,0dbh
	db	0dch,0dch,0dch,0dch
	db	0ddh,0ddh,0ddh,0ddh
	db	0deh,0deh,0deh,0deh
	db	0dfh,0dfh,0dfh,0dfh
	db	0e0h,0e0h,0e0h,0e0h
	db	0e1h,0e1h,0e1h,0e1h
	db	0e2h,0e2h,0e2h,0e2h
	db	0e3h,0e3h,0e3h,0e3h
	db	0e4h,0e4h,0e4h,0e4h
	db	0e5h,0e5h,0e5h,0e5h
	db	0e6h,0e6h,0e6h,0e6h
	db	0e7h,0e7h,0e7h,0e7h
	db	0e8h,0e8h,0e8h,0e8h
	db	0e9h,0e9h,0e9h,0e9h
	db	0eah,0eah,0eah,0eah
	db	0ebh,0ebh,0ebh,0ebh
	db	0ech,0ech,0ech,0ech
	db	0edh,0edh,0edh,0edh
	db	0eeh,0eeh,0eeh,0eeh
	db	0efh,0efh,0efh,0efh
	db	0f0h,0f0h,0f0h,0f0h
	db	0f1h,0f1h,0f1h,0f1h
	db	0f2h,0f2h,0f2h,0f2h
	db	0f3h,0f3h,0f3h,0f3h
	db	0f4h,0f4h,0f4h,0f4h
	db	0f5h,0f5h,0f5h,0f5h
	db	0f6h,0f6h,0f6h,0f6h
	db	0f7h,0f7h,0f7h,0f7h
	db	0f8h,0f8h,0f8h,0f8h
	db	0f9h,0f9h,0f9h,0f9h
	db	0fah,0fah,0fah,0fah
	db	0fbh,0fbh,0fbh,0fbh
	db	0fch,0fch,0fch,0fch
	db	0fdh,0fdh,0fdh,0fdh
	db	0feh,0feh,0feh,0feh
	db	0ffh,0ffh,0ffh,0ffh
