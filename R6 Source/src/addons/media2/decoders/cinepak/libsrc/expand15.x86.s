SECTION .text

%include "cv.i"


GLOBAL ExpandDetailCodeBook15
;GLOBAL ScaleTable5
;GLOBAL ScaleTable6

ALIGN 16
ExpandDetailCodeBook15:
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
		ja	short DetailQuit
		
		mov	edi,[esp+16+8+8]		; pCB

;     eax	-> last possible valid codebook entry
;     bl	codebook type
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

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov [esp+4],eax
	align	4

DetailKeyLoop:

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
	mov	al,byte [esi+1]	; eax:  Y1

	xor	edx,edx
	mov	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi]	; eax:  Y0

	shl edx,6
	or	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xx000RRR RRGGGGGG BBBBBrrr rrgggggg

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb

	mov	[edi],edx		; save RGB1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+3]	; eax:  Y3

	xor	edx,edx
	mov	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+2]	; eax:  Y2

	shl	edx,6
	or	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xx000RRR RRGGGGGG BBBBBrrr rrgggggg

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb

	mov	[edi+4],edx		; save it

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV
	add	edi,$10		; bump to next RGB patch

	cmp	esi,[esp+4]	; any more codebook entries?

	jbe	near DetailKeyLoop	; jump if more to do
	jmp	DetailQuit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DetailPartial:

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[esp+4],eax

	align	4

DetailPartialLoadSwitches:

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

	jmp	short DetailPartialTestSwitch


	align	4

DetailPartialYUVLoop:

	mov	eax,[esp]	; self mod switches here
;PartialSwitches	label	dword

;     eax	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,eax		; replace this index?

DetailPartialTestSwitch:

	mov	[esp],eax; save for next iteration

	jnc	near DetailPartialYUVSkip

	jz	DetailPartialLoadSwitches

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
	mov	al,byte [esi+1]	; eax:  Y1

	mov	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi]	; eax:  Y0

	shl	edx,6
	or	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xx000RRR RRGGGGGG BBBBBrrr rrgggggg

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb

	mov	[edi],edx		; save RGB1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+3]	; eax:  Y3

	mov	dl,[ScaleTable5+ebx+eax]	; edx:  000000R2

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; edx:  00R2G200

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; edx:  00R2G2B2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+2]	; eax:  Y2

	shl	edx,6
	or	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xx000RRR RRGGGGGG BBBBBrrr rrgggggg

	shl	edx,5
	or	dl,[ScaleTable5+ebp+eax]	; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb

	mov	[edi+4],edx		; save it

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV

DetailPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp esi,[esp+4]
	
	jbe	near DetailPartialYUVLoop
	jmp	DetailQuit

GLOBAL ExpandSmoothCodeBook15

ALIGN 16
ExpandSmoothCodeBook15:
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

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov [esp+4],eax
	align	4

SmoothKeyLoop:

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
	mov	al,byte [esi]	; eax:  Y0

	xor	edx,edx
	mov	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi],eax		; save RGB0:RGB0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+1]	; eax:  Y1

	shl edx,6
	mov	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xx000RRR RRGGGGGG BBBBBrrr rrgggggg

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi+4],eax		; save RGB1:RGB1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+2]	; eax:  Y2

	xor	edx,edx
	mov	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi+8],eax		; save RGB2:RGB2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+3]	; eax:  Y3

	shl	edx,6
	mov	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xx000RRR RRGGGGGG BBBBBrrr rrgggggg

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi+12],eax		; save RGB3:RGB3

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV
	add	edi,$10		; bump to next RGB patch

	cmp	esi,[esp+4]	; any more codebook entries?

	jbe	near SmoothKeyLoop	; jump if more to do
	jmp	SmoothQuit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SmoothPartial:

;     eax	-> last possible valid codebook entry
;     esi	-> incoming codebook
;     edi	-> building codebook

	mov	[esp+4],eax

	align	4

SmoothPartialLoadSwitches:

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

	jmp	short SmoothPartialTestSwitch


	align	4

SmoothPartialYUVLoop:

	mov	eax,[esp]	; self mod switches here
;PartialSwitches	label	dword

;     eax	swizzled bit switches for codes we do
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,eax		; replace this index?

SmoothPartialTestSwitch:

	mov	[esp],eax; save for next iteration

	jnc	near SmoothPartialYUVSkip

	jz	SmoothPartialLoadSwitches

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

	mov	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx xxxxxxxx xxxxxxxx 000RRRRR

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xxxxxxxx xxxxxxxx xx000RRR RRGGGGGG

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; xxxxxxxx xxxxx000 RRRRRGGG GGGBBBBB

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi],eax		; save RGB0:RGB0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+1]	; eax:  Y1

	mov	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xx000RRR RRGGGGGG BBBBBrrr rrgggggg

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi+4],eax		; save RGB1:RGB1

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+2]	; eax:  Y2

	mov	dl,[ScaleTable5+ebx+eax]	; edx:  000000R2

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; edx:  00R2G200

	shl edx,5
	or	dl,[ScaleTable5+ebp+eax]	; edx:  00R2G2B2

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi+8],eax		; save RGB2:RGB2

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	xor	eax,eax
	mov	al,byte [esi+3]	; eax:  Y3

	mov	dl,[ScaleTable5+ebx+eax]	; xxxxxxxx 000RRRRR GGGGGGBB BBBrrrrr

	shl	edx,5
	or	dl,[ScaleTable5+ecx+eax]	; xx000RRR RRGGGGGG BBBBBrrr rrgggggg

	shl	edx,5
	or	dl,[ScaleTable5+ebp+eax]	; RRRRRGGG GGGBBBBB rrrrrggg gggbbbbb

	mov	eax,edx
	shl	eax,16
	mov	ax,dx
	mov	[edi+12],eax		; save RGB3:RGB3

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

	add	esi,6		; bump to next incoming YYYYUV

SmoothPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp esi,[esp+4]
	
	jbe	near SmoothPartialYUVLoop
	jmp	SmoothQuit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%ifndef	NOBLACKWHITE

DoGreyDetail:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,2		; last possible code

	cmp	bl,kFullDBookType + kGreyBookBit
	jne	GreyDPartial

	align	4

GreyDKeyLoop:

	mov	edx,[esi]	; Y3 Y2 Y1 Y0
	add	esi,4

;     eax	-> last possible valid codebook entry
;     edx	Y3 Y2 Y1 Y0
;     esi	-> incoming codebook
;     edi	-> building codebook

  ; we take the incoming YYYYUV at [esi] and develop a 2x2 patch:
  ;
  ;     RGB0  RGB1
  ;     RGB2  RGB3
  ;
  ; as seen on the screen.

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB.3:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB.3:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB.3:  ........ ........ 0rrrrrgg gggbbbbb

	shl	edx,8		;         22222222 11111111 00000000 ........

	shl	ebx,1		;         ........ .......0 rrrrrggg rrbbbbb0
	shld	ebx,edx,5	; RGB32:  ........ ..0rrrrr gggggbbb bb0rrrrr
	shld	ebx,edx,5	; RGB32:  .....0rr rrrggggg bbbbb0rr rrrggggg
	shld	ebx,edx,5	; RGB32:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	shl	edx,8		;         11111111 00000000 ........ ........

	mov	[edi+4],ebx	; RGB3, RGB2

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB.1:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB.1:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB.1:  ........ ........ 0rrrrrgg gggbbbbb

	shl	edx,8		;         00000000 ........ ........ ........

	shl	ebx,1		;         ........ .......0 rrrrrggg rrbbbbb0
	shld	ebx,edx,5	; RGB10:  ........ ..0rrrrr gggggbbb bb0rrrrr
	shld	ebx,edx,5	; RGB10:  .....0rr rrrggggg bbbbb0rr rrrggggg
	shld	ebx,edx,5	; RGB10:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	mov	[edi+0],ebx	; RGB1, RGB0

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyDKeyLoop	; jump if more to do

	jmp	DetailQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreyDPartial:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	cmp	bl,kPartialDBookType + kGreyBookBit
	jne	near DetailHuh	; invalid code

	align	4

GreyDPartialLoadSwitches:

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

	jmp	short GreyDPartialTestSwitch


	align	4

GreyDPartialYUVLoop:

;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	add	ebp,ebp		; replace this index?

GreyDPartialTestSwitch:
	jnc	GreyDPartialYUVSkip
	jz	GreyDPartialLoadSwitches

	mov	edx,[esi]	; Y3 Y2 Y1 Y0
	add	esi,4

;     eax	-> last possible valid codebook entry
;     edx	Y3 Y2 Y1 Y0
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

  ; we take the incoming YYYYUV at [esi] and develop a 2x2 patch:
  ;
  ;     RGB0  RGB1
  ;     RGB2  RGB3
  ;
  ; as seen on the screen.

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB.3:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB.3:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB.3:  ........ ........ 0rrrrrgg gggbbbbb

	shl	edx,8		;         22222222 11111111 00000000 ........

	shl	ebx,1		;         ........ .......0 rrrrrggg rrbbbbb0
	shld	ebx,edx,5	; RGB32:  ........ ..0rrrrr gggggbbb bb0rrrrr
	shld	ebx,edx,5	; RGB32:  .....0rr rrrggggg bbbbb0rr rrrggggg
	shld	ebx,edx,5	; RGB32:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	shl	edx,8		;         11111111 00000000 ........ ........

	mov	[edi+4],ebx	; RGB3, RGB2

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB.1:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB.1:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB.1:  ........ ........ 0rrrrrgg gggbbbbb

	shl	edx,8		;         00000000 ........ ........ ........

	shl	ebx,1		;         ........ .......0 rrrrrggg rrbbbbb0
	shld	ebx,edx,5	; RGB10:  ........ ..0rrrrr gggggbbb bb0rrrrr
	shld	ebx,edx,5	; RGB10:  .....0rr rrrggggg bbbbb0rr rrrggggg
	shld	ebx,edx,5	; RGB10:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	mov	[edi+0],ebx	; RGB1, RGB0

GreyDPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreyDPartialYUVLoop

	jmp	DetailQuit


DoGreySmooth:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	add	eax,2		; last possible code

	cmp	bl,kFullSBookType + kGreyBookBit
	jne	near GreySPartial

	align	4

GreySKeyLoop:

	mov	edx,[esi]	; Y3 Y2 Y1 Y0
	add	esi,4

;     eax	-> last possible valid codebook entry
;     edx	Y3 Y2 Y1 Y0
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

  ; we take the incoming YYYYUV at [esi] and develop a 4x4 patch:
  ;
  ;     RGB0 RGB0  RGB1 RGB1
  ;     RGB0 RGB0  RGB1 RGB1

  ;     RGB2 RGB2  RGB3 RGB3
  ;     RGB2 RGB2  RGB3 RGB3
  ;
  ; as seen on the screen.

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB3:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB3:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB3:  ........ ........ 0rrrrrgg gggbbbbb

	mov	ecx,ebx		; RGB3:  ........ ........ 0rrrrrgg gggbbbbb
	shl	ecx,16		; RGB3:  0rrrrrgg gggbbbbb ........ ........
	mov	cx,bx		; RGB3:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	mov	[edi+12],ecx	; RGB3

	shl	edx,8		;         22222222 11111111 00000000 ........

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB2:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB2:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB2:  ........ ........ 0rrrrrgg gggbbbbb

	mov	ecx,ebx		; RGB2:  ........ ........ 0rrrrrgg gggbbbbb
	shl	ecx,16		; RGB2:  0rrrrrgg gggbbbbb ........ ........
	mov	cx,bx		; RGB2:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	mov	[edi+8],ecx	; RGB2

	shl	edx,8		;         11111111 00000000 ........ ........

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB1:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB1:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB1:  ........ ........ 0rrrrrgg gggbbbbb

	mov	ecx,ebx		; RGB1:  ........ ........ 0rrrrrgg gggbbbbb
	shl	ecx,16		; RGB1:  0rrrrrgg gggbbbbb ........ ........
	mov	cx,bx		; RGB1:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	mov	[edi+4],ecx	; RGB1

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB0:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB0:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB0:  ........ ........ 0rrrrrgg gggbbbbb

	mov	ecx,ebx		; RGB0:  ........ ........ 0rrrrrgg gggbbbbb
	shl	ecx,16		; RGB0:  0rrrrrgg gggbbbbb ........ ........
	mov	cx,bx		; RGB0:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	mov	[edi],ecx	; RGB0

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	GreySKeyLoop	; jump if more to do

	jmp	SmoothQuit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GreySPartial:

;     eax	-> last possible valid codebook entry
;     bl	codebook type
;     esi	-> incoming codebook
;     edi	-> building codebook

	cmp	bl,kPartialSBookType + kGreyBookBit
	jne	near SmoothHuh	; invalid code

	align	4

GreySPartialLoadSwitches:

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

	jmp	short GreySPartialTestSwitch


	align	4

GreySPartialYUVLoop:

;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

	add	ebp,ebp		; replace this index?

GreySPartialTestSwitch:
	jnc	GreySPartialYUVSkip
	jz	GreySPartialLoadSwitches

	mov	edx,[esi]	; Y3 Y2 Y1 Y0
	add	esi,4

;     eax	-> last possible valid codebook entry
;     edx	Y3 Y2 Y1 Y0
;     esi	-> incoming codebook
;     edi	-> building codebook
;     ebp	swizzled bit switches for codes we do

  ; we take the incoming YYYYUV at [esi] and develop a 4x4 patch:
  ;
  ;     RGB0 RGB0  RGB1 RGB1
  ;     RGB0 RGB0  RGB1 RGB1

  ;     RGB2 RGB2  RGB3 RGB3
  ;     RGB2 RGB2  RGB3 RGB3
  ;
  ; as seen on the screen.

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB3:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB3:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB3:  ........ ........ 0rrrrrgg gggbbbbb

	mov	ecx,ebx		; RGB3:  ........ ........ 0rrrrrgg gggbbbbb
	shl	ecx,16		; RGB3:  0rrrrrgg gggbbbbb ........ ........
	mov	cx,bx		; RGB3:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	mov	[edi+12],ecx	; RGB3

	shl	edx,8		;         22222222 11111111 00000000 ........

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB2:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB2:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB2:  ........ ........ 0rrrrrgg gggbbbbb

	mov	ecx,ebx		; RGB2:  ........ ........ 0rrrrrgg gggbbbbb
	shl	ecx,16		; RGB2:  0rrrrrgg gggbbbbb ........ ........
	mov	cx,bx		; RGB2:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	mov	[edi+8],ecx	; RGB2

	shl	edx,8		;         11111111 00000000 ........ ........

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB1:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB1:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB1:  ........ ........ 0rrrrrgg gggbbbbb

	mov	ecx,ebx		; RGB1:  ........ ........ 0rrrrrgg gggbbbbb
	shl	ecx,16		; RGB1:  0rrrrrgg gggbbbbb ........ ........
	mov	cx,bx		; RGB1:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	mov	[edi+4],ecx	; RGB1

	xor	bl,bl		; start with cleared high bit

	shld	ebx,edx,5	; RGB0:  ........ ........ ........ ..0rrrrr
	shld	ebx,edx,5	; RGB0:  ........ ........ .....0rr rrrggggg
	shld	ebx,edx,5	; RGB0:  ........ ........ 0rrrrrgg gggbbbbb

	mov	ecx,ebx		; RGB0:  ........ ........ 0rrrrrgg gggbbbbb
	shl	ecx,16		; RGB0:  0rrrrrgg gggbbbbb ........ ........
	mov	cx,bx		; RGB0:  0rrrrrgg gggbbbbb 0rrrrrgg gggbbbbb

	mov	[edi],ecx	; RGB0

GreySPartialYUVSkip:

	add	edi,16		; bump to next RGB patch

	cmp	esi,eax		; any more codebook entries?
	jbe	near GreySPartialYUVLoop

	jmp	SmoothQuit


%endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.data
	db 000h,000h,000h,000h,000h,000h,000h,000h ; underflow
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h
	db 000h,000h,000h,000h,000h,000h,000h,000h

ScaleTable5:
	db	000h,000h,000h,000h,000h,000h,000h,000h
	db	001h,001h,001h,001h,001h,001h,001h,001h
	db	002h,002h,002h,002h,002h,002h,002h,002h
	db	003h,003h,003h,003h,003h,003h,003h,003h
	db	004h,004h,004h,004h,004h,004h,004h,004h
	db	005h,005h,005h,005h,005h,005h,005h,005h
	db	006h,006h,006h,006h,006h,006h,006h,006h
	db	007h,007h,007h,007h,007h,007h,007h,007h
	db	008h,008h,008h,008h,008h,008h,008h,008h
	db	009h,009h,009h,009h,009h,009h,009h,009h
	db	00ah,00ah,00ah,00ah,00ah,00ah,00ah,00ah
	db	00bh,00bh,00bh,00bh,00bh,00bh,00bh,00bh
	db	00ch,00ch,00ch,00ch,00ch,00ch,00ch,00ch
	db	00dh,00dh,00dh,00dh,00dh,00dh,00dh,00dh
	db	00eh,00eh,00eh,00eh,00eh,00eh,00eh,00eh
	db	00fh,00fh,00fh,00fh,00fh,00fh,00fh,00fh
	db	010h,010h,010h,010h,010h,010h,010h,010h
	db	011h,011h,011h,011h,011h,011h,011h,011h
	db	012h,012h,012h,012h,012h,012h,012h,012h
	db	013h,013h,013h,013h,013h,013h,013h,013h
	db	014h,014h,014h,014h,014h,014h,014h,014h
	db	015h,015h,015h,015h,015h,015h,015h,015h
	db	016h,016h,016h,016h,016h,016h,016h,016h
	db	017h,017h,017h,017h,017h,017h,017h,017h
	db	018h,018h,018h,018h,018h,018h,018h,018h
	db	019h,019h,019h,019h,019h,019h,019h,019h
	db	01ah,01ah,01ah,01ah,01ah,01ah,01ah,01ah
	db	01bh,01bh,01bh,01bh,01bh,01bh,01bh,01bh
	db	01ch,01ch,01ch,01ch,01ch,01ch,01ch,01ch
	db	01dh,01dh,01dh,01dh,01dh,01dh,01dh,01dh
	db	01eh,01eh,01eh,01eh,01eh,01eh,01eh,01eh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh

	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh;overflow
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh
	db	01fh,01fh,01fh,01fh,01fh,01fh,01fh,01fh


