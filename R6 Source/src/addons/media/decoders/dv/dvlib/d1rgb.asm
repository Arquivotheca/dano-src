SECTION .text

DCT_SHIFT equ $3
SIZE_RGBQ equ $4
SIZE_RGB16 equ $2
DCT_BUFFER_STRIDE equ $20
SIZE_DCT_DATA equ $2
RGB32_IMAGE_STRIDE equ $0b40
RGB16_IMAGE_STRIDE equ $05a0

; void mmxPutImage525_YUY2( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
GLOBAL mmxPutImage525_YUY2
ALIGN 16
mmxPutImage525_YUY2:
;	SkipRGB = RGB16_IMAGE_STRIDE- SizeX * SIZE_RGB16;
;	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX*2)
;	SkipUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX/2)
;	SizeX >>= 3;

		push		edi
		push		esi
		push		eax
		push		ecx
		push		edx
		
		sub			esp,16						; room for four variables
		mov			edi,[esp+16+20+4]			; pDestBuffer
		mov			esi,[esp+16+20+20]			; pY
		mov			ecx,[esp+16+20+24]			; pUV
		
		mov			eax,[esp+16+20+8]
		mov			edx,[esp+16+20+12]
		shl			edx,$1
		sub			eax,edx
		mov			[esp],eax					; [esp]=SkipRGB=RGB16_IMAGE_STRIDE-SizeX*SIZE_RGB16

		mov			eax,DCT_BUFFER_STRIDE*2
		sub			eax,edx
		mov			[esp+4],eax					; [esp+4]=SkipY=DCT_BUFFER_STRIDE*2-SizeX*2

		mov			eax,DCT_BUFFER_STRIDE*2
		shr			edx,$2
		sub			eax,edx
		mov			[esp+8],eax					; [esp+8]=SkipUV=DCT_BUFFER_STRIDE*2-SizeX/2
		
		mov			eax,[esp+16+20+12]				; SizeX=SizeX>>3
		ror			eax,$3
		mov			[esp+12],eax

		mov			edx,[esp+16+20+16]			; SizeY
		movq		mm7,[ADD128]
	LoopY9:
		mov			eax,[esp+12]				; SizeX
	LoopX9:
		movd		mm2,[ecx]		; v1:v0
		movd		mm3,[ecx+8*2]	; ecx+8*(TYPE DCT_DATA) u1:u0
		psraw		mm3,$1
		psraw		mm2,$1
		psraw		mm2,$3
		psraw		mm3,$3
		paddw		mm2,mm7
		paddw		mm3,mm7
		punpcklwd	mm3,mm2			; mm3= v1:u1:v0:u0
		movq		mm2,mm3			; mm2= v1:u1:v0:u0
		punpckldq	mm3,mm3			; mm3= v0:u0:v0:u0
		punpckhdq	mm2,mm2			; mm2= v1:u1:v1:u1
		
		movq		mm1,[esi]		; mm1= y3:y2:y1:y0
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm1,$3
		movq		mm4,mm1			; mm4= y3:y2:y1:y0
		punpcklwd	mm4,mm3			; mm4= y1:v0:y0:u0
		punpckhwd	mm1,mm3			; mm2= y3:u1:y2:v1
		packuswb	mm4,mm1			; mm2= y3:u0:y2:v0:y1:u0:y0:v0

		movq		[edi],mm4

		movq		mm1,[esi+4*2]	; mm1= y3:y2:y1:y0
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm1,$3
		movq		mm4,mm1			; mm4= u1:v1:u0:v0
		punpcklwd	mm4,mm2			; mm4= y1:u1:y0:v1
		punpckhwd	mm1,mm2			; mm2= y3:u1:y2:v1
		packuswb	mm4,mm1			; mm2= y3:u1:y2:v1:y1:u1:y0:v1

		movq		[edi+8],mm4
		add			ecx,2*2				;2=(TYPE DCT_DATA)
		add			esi,8*2				;2=(TYPE DCT_DATA)
		add			edi,8*SIZE_RGB16
		dec			eax
		jne			near LoopX9

		add			ecx,[esp+8]
		add			esi,[esp+4]
		add			edi,[esp]
		dec			edx
		jne			near LoopY9
		emms
		add			esp,16						; restore four variable room
		
		pop			edx
		pop			ecx
		pop			eax
		pop			esi
		pop			edi
		ret

; void mmxPutImage525_RGB15( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
GLOBAL mmxPutImage525_RGB15
ALIGN 16
mmxPutImage525_RGB15:
;	pDestBuffer = PutImageBuffer + StartY * RGB16_IMAGE_STRIDE + StartX * SIZE_RGB16;
;	SkipRGB = RGB16_IMAGE_STRIDE- SizeX * SIZE_RGB16;
;	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX*2)
;	SkipUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX/2)
;	SizeX >>= 3;

		push		edi
		push		esi
		push		eax
		push		ecx
		push		edx
		
		sub			esp,16						; room for four variables
		mov			edi,[esp+16+20+4]			; pDestBuffer
		mov			esi,[esp+16+20+20]			; pY
		mov			ecx,[esp+16+20+24]			; pUV
		
		mov			eax,[esp+16+20+8]
		mov			edx,[esp+16+20+12]
		shl			edx,$1
		sub			eax,edx
		mov			[esp],eax					; [esp]=SkipRGB=RGB16_IMAGE_STRIDE-SizeX*SIZE_RGB16

		mov			eax,DCT_BUFFER_STRIDE*2
		sub			eax,edx
		mov			[esp+4],eax					; [esp+4]=SkipY=DCT_BUFFER_STRIDE*2-SizeX*2

		mov			eax,DCT_BUFFER_STRIDE*2
		shr			edx,$2
		sub			eax,edx
		mov			[esp+8],eax					; [esp+8]=SkipUV=DCT_BUFFER_STRIDE*2-SizeX/2
		
		mov			eax,[esp+16+20+12]				; SizeX=SizeX>>3
		ror			eax,$3
		mov			[esp+12],eax

		mov			edx,[esp+16+20+16]			; SizeY
	LoopY4:
		mov			eax,[esp+12]				; SizeX
	LoopX4:
		movd		mm2,[ecx]		; v1:v0
		movd		mm3,[ecx+8*2]	; ecx+8*(TYPE DCT_DATA) u1:u0
		psraw		mm3,$1
		psraw		mm2,$1
		movq		mm0,mm3
		movq		mm4,mm2
		pmulhw		mm2,[mmxRFromV]
		pmulhw		mm3,[mmxBFromU]
		pmulhw		mm4,[mmxGFromV]
		pmulhw		mm0,[mmxGFromU]
		paddw		mm4,mm0
		punpcklwd	mm2,mm2			; v1:v1:v0:v0
		punpcklwd	mm3,mm3			; v1:v1:v0:v0
		punpcklwd	mm4,mm4			; v1:v1:v0:v0
		
		movq		mm5,mm2
		movq		mm6,mm3
		movq		mm7,mm4
		punpcklwd	mm2,mm2			; uvR0:uvR0:uvR0:uvR0
		punpcklwd	mm3,mm3			; uvB0:uvB0:uvB0:uvB0
		punpcklwd	mm4,mm4			; uvG0:uvG0:uvG0:uvG0
		punpckhwd	mm5,mm5			; uvR1:uvR1:uvR1:uvR1
		punpckhwd	mm6,mm6			; uvB1:uvB1:uvB1:uvB1
		punpckhwd	mm7,mm7			; uvG1:uvG1:uvG1:uvG1

		movq		mm1,[esi]		; y3:y2:y1:y0
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm1,DCT_SHIFT
		paddw		mm2,mm1			; mm2 = r3:r2:r1:r0
		paddw		mm3,mm1			; mm3 = b3:b2:b1:b0
		paddw		mm4,mm1			; mm4 = g3:g2:g1:g0

		pxor		mm1,mm1			; zero mm1

		packuswb	mm2,mm2			; mm2 = ??:??:??:??:r3:r2:r1:r0
		psrlq		mm2,$3			; mm2 = ::::xxxr3:xxxr2:xxxr1:xxxr0
		punpcklbw	mm2,mm1			; mm2 = 00:xxxr3:00:xxxr2:00:xxxr1:00:xxxr0
		psllw		mm2,$0a			; mm2 = r3000:00:r2000:00:r1000:00:r0000:00
		
		packuswb	mm3,mm3			; mm3 = ??:??:??:??:b3:b2:b1:b0
		punpcklbw	mm3,mm1			; mm3 = 00b3:00b2:00b1:00b0
		psrlw		mm3,$3			; mm3 = 00:000b3:00:000b2:00:000b1:00:000b0
	
		packuswb	mm4,mm4			; mm4 = ??:??:??:??:g3:g2:g1:g0
		punpcklbw	mm4,mm1			; mm4 = 00g3:00g2:00g1:00g0
		psrlw		mm4,$3			; mm4 = 00:00g3:00:00g2:00:00g1:00:00g0
		psllw		mm4,$5			; mm4 = 00000g300000:00000g200000:...
	
		por			mm3,mm2			; mm3 = r3000000b3:r2000000b2:r1000000b1...
		por			mm3,mm4			; mm3 = r3g3b3:r2g2b2:r1g1b1:r0g0b1

		movq		[edi],mm3

		movq		mm1,[esi+4*2]	; 2==(TYPE DCT_DATA) y3:y2:y1:y0
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm1,DCT_SHIFT
		paddw		mm5,mm1
		paddw		mm6,mm1
		paddw		mm7,mm1
		
		pxor		mm1,mm1			; zero mm1

		packuswb	mm5,mm5			; mm5 = ??:??:??:??:r3:r2:r1:r0
		psrlq		mm5,$3			; mm5 = ::::xxxr3:xxxr2:xxxr1:xxxr0
		punpcklbw	mm5,mm1			; mm5 = 00:xxxr3:00:xxxr2:00:xxxr1:00:xxxr0
		psllw		mm5,$0a			; mm5 = r3000:00:r2000:00:r1000:00:r0000:00
		
		packuswb	mm6,mm6			; mm6 = ??:??:??:??:b3:b2:b1:b0
		punpcklbw	mm6,mm1			; mm6 = 00b3:00b2:00b1:00b0
		psrlw		mm6,$3			; mm6 = 00:000b3:00:000b2:00:000b1:00:000b0
	
		packuswb	mm7,mm7			; mm7 = ??:??:??:??:g3:g2:g1:g0
		punpcklbw	mm7,mm1			; mm7 = 00g3:00g2:00g1:00g0
		psrlw		mm7,$3			; mm7 = 00:00g3:00:00g2:00:00g1:00:00g0
		psllw		mm7,$5			; mm7 = 00000g300000:00000g200000:...
	
		por			mm6,mm5			; mm6 = r3000000b3:r2000000b2:r1000000b1...
		por			mm6,mm7			; mm6 = r3g3b3:r2g2b2:r1g1b1:r0g0b1

		movq		[edi+SIZE_RGB16*4],mm6

		add			ecx,2*2				;2=(TYPE DCT_DATA)
		add			esi,8*2				;2=(TYPE DCT_DATA)
		add			edi,8*SIZE_RGB16
		dec			eax
		jne			near LoopX4

		add			ecx,[esp+8]
		add			esi,[esp+4]
		add			edi,[esp]
		dec			edx
		jne			near LoopY4
		emms
		add			esp,16						; restore four variable room
		
		pop			edx
		pop			ecx
		pop			eax
		pop			esi
		pop			edi
		ret

; void mmxPutImage525_RGB16( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
GLOBAL mmxPutImage525_RGB16
ALIGN 16
mmxPutImage525_RGB16:
;	SkipRGB = RGB16_IMAGE_STRIDE- SizeX * SIZE_RGB16;
;	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX*2)
;	SkipUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX/2)
;	SizeX >>= 3;

		push		edi
		push		esi
		push		eax
		push		ecx
		push		edx
		
		sub			esp,16						; room for four variables
		mov			edi,[esp+16+20+4]			; pDestBuffer
		mov			esi,[esp+16+20+20]			; pY
		mov			ecx,[esp+16+20+24]			; pUV
		
		mov			eax,[esp+16+20+8]
		mov			edx,[esp+16+20+12]
		shl			edx,$1
		sub			eax,edx
		mov			[esp],eax					; [esp]=SkipRGB=RGB16_IMAGE_STRIDE-SizeX*SIZE_RGB16

		mov			eax,DCT_BUFFER_STRIDE*2
		sub			eax,edx
		mov			[esp+4],eax					; [esp+4]=SkipY=DCT_BUFFER_STRIDE*2-SizeX*2

		mov			eax,DCT_BUFFER_STRIDE*2
		shr			edx,$2
		sub			eax,edx
		mov			[esp+8],eax					; [esp+8]=SkipUV=DCT_BUFFER_STRIDE*2-SizeX/2
		
		mov			eax,[esp+16+20+12]				; SizeX=SizeX>>3
		ror			eax,$3
		mov			[esp+12],eax

		mov			edx,[esp+16+20+16]			; SizeY
	LoopY3:
		mov			eax,[esp+12]				; SizeX
	LoopX3:
		movd		mm2,[ecx]		; v1:v0
		movd		mm3,[ecx+8*2]	; ecx+8*(TYPE DCT_DATA) u1:u0
		psraw		mm3,$1
		psraw		mm2,$1
		movq		mm0,mm3
		movq		mm4,mm2
		pmulhw		mm2,[mmxRFromV]
		pmulhw		mm3,[mmxBFromU]
		pmulhw		mm4,[mmxGFromV]
		pmulhw		mm0,[mmxGFromU]
		paddw		mm4,mm0
		punpcklwd	mm2,mm2			; v1:v1:v0:v0
		punpcklwd	mm3,mm3			; v1:v1:v0:v0
		punpcklwd	mm4,mm4			; v1:v1:v0:v0
		
		movq		mm5,mm2
		movq		mm6,mm3
		movq		mm7,mm4
		punpcklwd	mm2,mm2			; uvR0:uvR0:uvR0:uvR0
		punpcklwd	mm3,mm3			; uvB0:uvB0:uvB0:uvB0
		punpcklwd	mm4,mm4			; uvG0:uvG0:uvG0:uvG0
		punpckhwd	mm5,mm5			; uvR1:uvR1:uvR1:uvR1
		punpckhwd	mm6,mm6			; uvB1:uvB1:uvB1:uvB1
		punpckhwd	mm7,mm7			; uvG1:uvG1:uvG1:uvG1

		movq		mm1,[esi]		; y3:y2:y1:y0
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm1,DCT_SHIFT
		paddw		mm2,mm1			; mm2 = r3:r2:r1:r0
		paddw		mm3,mm1			; mm3 = b3:b2:b1:b0
		paddw		mm4,mm1			; mm4 = g3:g2:g1:g0

		pxor		mm1,mm1			; zero mm1

		packuswb	mm2,mm2			; mm2 = ??:??:??:??:r3:r2:r1:r0
		psrlq		mm2,$3			; mm2 = ::::xxxr3:xxxr2:xxxr1:xxxr0
		punpcklbw	mm2,mm1			; mm2 = 00:xxxr3:00:xxxr2:00:xxxr1:00:xxxr0
		psllw		mm2,$0b			; mm2 = r3000:00:r2000:00:r1000:00:r0000:00
		
		packuswb	mm3,mm3			; mm3 = ??:??:??:??:b3:b2:b1:b0
		punpcklbw	mm3,mm1			; mm3 = 00b3:00b2:00b1:00b0
		psrlw		mm3,$3			; mm3 = 00:000b3:00:000b2:00:000b1:00:000b0
	
		packuswb	mm4,mm4			; mm4 = ??:??:??:??:g3:g2:g1:g0
		punpcklbw	mm4,mm1			; mm4 = 00g3:00g2:00g1:00g0
		psrlw		mm4,$2			; mm4 = 00:00g3:00:00g2:00:00g1:00:00g0
		psllw		mm4,$5			; mm4 = 00000g300000:00000g200000:...
	
		por			mm3,mm2			; mm3 = r3000000b3:r2000000b2:r1000000b1...
		por			mm3,mm4			; mm3 = r3g3b3:r2g2b2:r1g1b1:r0g0b1

		movq		[edi],mm3

		movq		mm1,[esi+4*2]	; 2==(TYPE DCT_DATA) y3:y2:y1:y0
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm1,DCT_SHIFT
		paddw		mm5,mm1
		paddw		mm6,mm1
		paddw		mm7,mm1
		
		pxor		mm1,mm1			; zero mm1

		packuswb	mm5,mm5			; mm5 = ??:??:??:??:r3:r2:r1:r0
		psrlq		mm5,$3			; mm5 = ::::xxxr3:xxxr2:xxxr1:xxxr0
		punpcklbw	mm5,mm1			; mm5 = 00:xxxr3:00:xxxr2:00:xxxr1:00:xxxr0
		psllw		mm5,$0b			; mm5 = r3000:00:r2000:00:r1000:00:r0000:00
		
		packuswb	mm6,mm6			; mm6 = ??:??:??:??:b3:b2:b1:b0
		punpcklbw	mm6,mm1			; mm6 = 00b3:00b2:00b1:00b0
		psrlw		mm6,$3			; mm6 = 00:000b3:00:000b2:00:000b1:00:000b0
	
		packuswb	mm7,mm7			; mm7 = ??:??:??:??:g3:g2:g1:g0
		punpcklbw	mm7,mm1			; mm7 = 00g3:00g2:00g1:00g0
		psrlw		mm7,$2			; mm7 = 00:00g3:00:00g2:00:00g1:00:00g0
		psllw		mm7,$5			; mm7 = 00000g300000:00000g200000:...
	
		por			mm6,mm5			; mm6 = r3000000b3:r2000000b2:r1000000b1...
		por			mm6,mm7			; mm6 = r3g3b3:r2g2b2:r1g1b1:r0g0b1

		movq		[edi+SIZE_RGB16*4],mm6

		add			ecx,2*2				;2=(TYPE DCT_DATA)
		add			esi,8*2				;2=(TYPE DCT_DATA)
		add			edi,8*SIZE_RGB16
		dec			eax
		jne			near LoopX3

		add			ecx,[esp+8]
		add			esi,[esp+4]
		add			edi,[esp]
		dec			edx
		jne			near LoopY3
		emms
		add			esp,16						; restore four variable room
		
		pop			edx
		pop			ecx
		pop			eax
		pop			esi
		pop			edi
		ret


; void mmxPutImage525_RGBQ( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
GLOBAL mmxPutImage525_RGBQ
ALIGN 16
mmxPutImage525_RGBQ:
		push		edi
		push		esi
		push		eax
		push		ecx
		push		edx
		
;	SkipRGB = RGB32_IMAGE_STRIDE- SizeX * SIZE_RGBQ;
;	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX*2)
;	SkipUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX/2)
;	SizeX >>= 3;
		sub			esp,16						; room for four variables
		mov			edi,[esp+16+20+4]				; pDestBuffer
		mov			esi,[esp+16+20+20]			; pY
		mov			ecx,[esp+16+20+24]			; pUV
		
		mov			eax,[esp+16+20+8]
		mov			edx,[esp+16+20+12]
		shl			edx,$2
		sub			eax,edx
		mov			[esp],eax					; [esp]=SkipRGB=RGB32_IMAGE_STRIDE-SizeX*4

		mov			eax,DCT_BUFFER_STRIDE*2
		shr			edx,$1
		sub			eax,edx
		mov			[esp+4],eax				; [esp+4]=SkipY=DCT_BUFFER_STRIDE*2-SizeX*2

		mov			eax,DCT_BUFFER_STRIDE*2
		shr			edx,$2
		sub			eax,edx
		mov			[esp+8],eax				; [esp+12]=SkipUV=DCT_BUFFER_STRIDE*2-SizeX/2
		
		mov			eax,[esp+16+20+12]				; SizeX=SizeX>>3
		ror			eax,$3
		mov			[esp+12],eax

		; the main loops...
		mov			edx,[esp+16+20+16]			; SizeY
	LoopY5:
		mov			eax,[esp+12]				; SizeX
	LoopX5:
		movd		mm2,[ecx]					; v1:v0
		movd		mm3,[ecx+8*SIZE_DCT_DATA]	; u1:u0
		psraw		mm3,$1
		psraw		mm2,$1
		movq		mm0,mm3
		movq		mm4,mm2
		pmulhw		mm2,[mmxRFromV]
		pmulhw		mm3,[mmxBFromU]
		pmulhw		mm4,[mmxGFromV]
		pmulhw		mm0,[mmxGFromU]
		paddw		mm4,mm0
		punpcklwd	mm2,mm2			; v1:v1:v0:v0
		punpcklwd	mm3,mm3			; v1:v1:v0:v0
		punpcklwd	mm4,mm4			; v1:v1:v0:v0
		
		movq		mm5,mm2
		movq		mm6,mm3
		movq		mm7,mm4
		punpcklwd	mm2,mm2			; uvR0:uvR0:uvR0:uvR0
		punpcklwd	mm3,mm3			; uvB0:uvB0:uvB0:uvB0
		punpcklwd	mm4,mm4			; uvG0:uvG0:uvG0:uvG0
		punpckhwd	mm5,mm5			; uvR1:uvR1:uvR1:uvR1
		punpckhwd	mm6,mm6			; uvB1:uvB1:uvB1:uvB1
		punpckhwd	mm7,mm7			; uvG1:uvG1:uvG1:uvG1

		movq		mm1,[esi]		; y3:y2:y1:y0
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm1,DCT_SHIFT
		paddw		mm2,mm1			; mm2 = r3:r2:r1:r0
		paddw		mm3,mm1			; mm3 = b3:b2:b1:b0
		paddw		mm4,mm1			; mm4 = g3:g2:g1:g0
		packuswb	mm2,mm2			; mm2 = ??:??:??:??:r3:r2:r1:r0
		packuswb	mm3,mm3			; mm3 = ??:??:??:??:b3:b2:b1:b0
		packuswb	mm4,mm4			; mm4 = ??:??:??:??:g3:g2:g1:g0

		punpcklbw	mm3,mm4			; mm3 = g3:b3:g2:b2:g1:b1:g0:b0
		punpcklbw	mm2,mm2			; mm2 = r3:r3:r2:r2:r1:r1:r0:r0
		movq		mm1,mm3
		punpcklwd	mm3,mm2			; mm3 = r1:r1:g1:b1:r0:r0:g0:b0
		por			mm3,[alphamask]	; mm3 = ff:r1:g1:b1:ff:r0:g0:b0
		punpckhwd	mm1,mm2			; mm1 = r3:r3:g3:b3:r2:r2:g2:b2
		por			mm1,[alphamask]	; mm1 = ff:r3:g3:b3:ff:r2:g2:b2

		movq		[edi+SIZE_RGBQ*0],mm3
		movq		[edi+SIZE_RGBQ*2],mm1

		movq		mm1,[esi+4*SIZE_DCT_DATA]	; y3:y2:y1:y0
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm1,DCT_SHIFT
		paddw		mm5,mm1
		paddw		mm6,mm1
		paddw		mm7,mm1
		packuswb	mm5,mm5
		packuswb	mm6,mm6
		packuswb	mm7,mm7

		punpcklbw	mm6,mm7			; mm6 = g3:b3:g2:b2:g1:b1:g0:b0
		punpcklbw	mm5,mm5			; mm5 = r3:r3:r2:r2:r1:r1:r0:r0
		movq		mm1,mm6
		punpcklwd	mm6,mm5			; mm6 = r1:r1:g1:b1:r0:r0:g0:b0
		por			mm6,[alphamask]	; mm6 = ff:r1:g1:b1:ff:r0:g0:b0
		punpckhwd	mm1,mm5			; mm1 = r3:r3:g3:b3:r2:r2:g2:b2
		por			mm1,[alphamask]	; mm1 = ff:r3:g3:b3:ff:r2:g2:b2
		movq		[edi+SIZE_RGBQ*4],mm6
		movq		[edi+SIZE_RGBQ*6],mm1

		add			ecx,2*SIZE_DCT_DATA
		add			esi,8*SIZE_DCT_DATA
		add			edi,8*SIZE_RGBQ
		dec			eax
		jne			near LoopX5

		add			ecx,[esp+8]
		add			esi,[esp+4]
		add			edi,[esp]
		dec			edx
		jne			near LoopY5
		emms
		add			esp,16						; restore four variable room

		pop			edx
		pop			ecx
		pop			eax
		pop			esi
		pop			edi
		
		ret
		

; void mmxPutImage625_RGBQ( PBYTE pImage, int Stride, PDCT_DATA pY )
GLOBAL mmxPutImage625_RGBQ
ALIGN 16
mmxPutImage625_RGBQ:
		push		edi
		push		esi
		push		eax
		push		ebx
		push		ecx
		push		edx
		
;	pUV = pY + 16;
;	SkipRGB = PutImageStride * 2 - 8 * 2 * SIZE_RGBQ;
		sub			esp,4				; room for one variable
		mov			edi,[esp+24+4+4]			; PutImageBuffer

		mov			eax,[esp+24+4+8]			; PutImageStride
		lea			eax,[eax+eax-(8*2*SIZE_RGBQ)]		; PutImageStride*2
		mov			[esp+0],eax			; SkipRGB

		mov			esi,[esp+24+4+12]	; pY
		lea			ecx,[esi+16*SIZE_DCT_DATA]		; pUV
		mov			edx,[esp+24+4+8]	; PutImageStride
		mov			ebx,[SIZEY625]		; SizeY
	LoopY2:
		mov			eax,[SIZEX625]	; SizeX
	LoopX2:
		movd		mm2,[ecx]		; v1:v0
		movd		mm3,[ecx+8*2]	; u1:u0
		psraw		mm3,$1
		psraw		mm2,$1
		movq		mm0,mm3
		movq		mm4,mm2
		pmulhw		mm2,[mmxRFromV]
		pmulhw		mm3,[mmxBFromU]
		pmulhw		mm4,[mmxGFromV]
		pmulhw		mm0,[mmxGFromU]
		paddw		mm4,mm0
		punpcklwd	mm2,mm2			; vR1:vR1:vR0:vR0
		punpcklwd	mm3,mm3			; uB1:uB1:uB0:uB0
		punpcklwd	mm4,mm4			; uvG1:uvG1:uvG0:uvG0

		movq		mm5,[esi]		; y03:y02:y01:y00
		movq		mm1,[esi+DCT_BUFFER_STRIDE*2]	; y13:y12:y11:y10
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm5,$1
		paddw		mm5,[add1024]
		psraw		mm5,DCT_SHIFT
		psraw		mm1,DCT_SHIFT
		movq		mm6,mm5			; y03:y02:y01:y00 shifted
		movq		mm7,mm5			; y03:y02:y01:y00 shifted

		paddw		mm5,mm2			; r03:r02:r01:r00
		paddw		mm6,mm3			; b03:b02:b01:b00
		paddw		mm7,mm4			; g03:g02:g01:g00
		paddw		mm2,mm1			; r13:r12:r11:r10
		paddw		mm3,mm1			; b13:b12:b11:b10
		paddw		mm4,mm1			; g13:g12:g11:g10

		packuswb	mm5,mm5			; mm5 = ??:??:??:??:r3:r2:r1:r0
		packuswb	mm6,mm6			; mm6 = ??:??:??:??:b3:b2:b1:b0
		packuswb	mm7,mm7			; mm7 = ??:??:??:??:g3:g2:g1:g0

		punpcklbw	mm6,mm7			; mm6 = g3:b3:g2:b2:g1:b1:g0:b0
		punpcklbw	mm5,mm5			; mm5 = r3:r3:r2:r2:r1:r1:r0:r0
		movq		mm1,mm6
		punpcklwd	mm6,mm5			; mm6 = r1:r1:g1:b1:r0:r0:g0:b0
		por			mm6,[alphamask]	; mm6 = ff:r1:g1:b1:ff:r0:g0:b0
		punpckhwd	mm1,mm5			; mm1 = r3:r3:g3:b3:r2:r2:g2:b2
		por			mm1,[alphamask]	; mm1 = ff:r3:g3:b3:ff:r2:g2:b2

		movq		[edi+0],mm6
 		movq		[edi+8],mm1

		packuswb	mm2,mm2			; mm2 = ??:??:??:??:r3:r2:r1:r0
		packuswb	mm3,mm3			; mm3 = ??:??:??:??:b3:b2:b1:b0
		packuswb	mm4,mm4			; mm4 = ??:??:??:??:g3:g2:g1:g0

		punpcklbw	mm3,mm4			; mm3 = g3:b3:g2:b2:g1:b1:g0:b0
		punpcklbw	mm2,mm2			; mm2 = r3:r3:r2:r2:r1:r1:r0:r0
		movq		mm1,mm3
		punpcklwd	mm3,mm2			; mm3 = r1:r1:g1:b1:r0:r0:g0:b0
		por			mm3,[alphamask]	; mm6 = ff:r1:g1:b1:ff:r0:g0:b0
		punpckhwd	mm1,mm2			; mm1 = r3:r3:g3:b3:r2:r2:g2:b2
		por			mm1,[alphamask]	; mm6 = ff:r1:g1:b1:ff:r0:g0:b0

		movq		[edi+edx+0],mm3
		movq		[edi+edx+8],mm1

		add			ecx,2*2
		add			esi,4*2
		add			edi,4*SIZE_RGBQ
		dec			eax
		jne			near LoopX2
	
		add			ecx,DWORD ( DCT_BUFFER_STRIDE - 8 ) *2
		add			esi,( DCT_BUFFER_STRIDE * 2 - 8 * 2 ) *2
		add			edi,[esp+0]				; SkipRGB
		dec			ebx
		jne			near LoopY2
		emms
		add			esp,4						; restore four variable room
		
		pop			edx
		pop			ecx
		pop			ebx
		pop			eax
		pop			esi
		pop			edi
		
		ret
		

; void mmxPutImage625_RGB16( PBYTE pImage, int Stride, PDCT_DATA pY )
GLOBAL mmxPutImage625_RGB16
ALIGN 16
mmxPutImage625_RGB16:
		push		edi
		push		esi
		push		eax
		push		ebx
		push		ecx
		push		edx
		
;	pUV = pY + 16;
;	SkipRGB = PutImageStride * 2 - 8 * 2 * SIZE_RGBQ;
		sub			esp,4				; room for one variable
		mov			edi,[esp+24+4+4]			; PutImageBuffer

		mov			eax,[esp+24+4+8]	; PutImageStride
		lea			eax,[eax+eax-(8*2*SIZE_RGB16)]		; PutImageStride*2
		mov			[esp+0],eax			; SkipRGB

		mov			esi,[esp+24+4+12]	; pY
		lea			ecx,[esi+16*SIZE_DCT_DATA]		; pUV
		mov			edx,[esp+24+4+8]	; PutImageStride
		mov			ebx,[SIZEY625]		; SizeY
	LoopY7:
		mov			eax,[SIZEX625]	; SizeX
	LoopX7:
		movd		mm2,[ecx]		; v1:v0
		movd		mm3,[ecx+8*2]	; u1:u0
		psraw		mm3,$1
		psraw		mm2,$1
		movq		mm0,mm3
		movq		mm4,mm2
		pmulhw		mm2,[mmxRFromV]
		pmulhw		mm3,[mmxBFromU]
		pmulhw		mm4,[mmxGFromV]
		pmulhw		mm0,[mmxGFromU]
		paddw		mm4,mm0
		punpcklwd	mm2,mm2			; vR1:vR1:vR0:vR0
		punpcklwd	mm3,mm3			; uB1:uB1:uB0:uB0
		punpcklwd	mm4,mm4			; uvG1:uvG1:uvG0:uvG0

		movq		mm5,[esi]		; y03:y02:y01:y00
		movq		mm1,[esi+DCT_BUFFER_STRIDE*2]	; y13:y12:y11:y10
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm5,$1
		paddw		mm5,[add1024]
		psraw		mm5,DCT_SHIFT
		psraw		mm1,DCT_SHIFT
		movq		mm6,mm5			; y03:y02:y01:y00 shifted
		movq		mm7,mm5			; y03:y02:y01:y00 shifted

		paddw		mm5,mm2			; r03:r02:r01:r00
		paddw		mm6,mm3			; b03:b02:b01:b00
		paddw		mm7,mm4			; g03:g02:g01:g00
		paddw		mm2,mm1			; r13:r12:r11:r10
		paddw		mm3,mm1			; b13:b12:b11:b10
		paddw		mm4,mm1			; g13:g12:g11:g10

		pxor		mm1,mm1			; zero mm1

		packuswb	mm5,mm5			; mm2 = ??:??:??:??:r3:r2:r1:r0
		psrlq		mm5,$3			; mm2 = ::::xxxr3:xxxr2:xxxr1:xxxr0
		punpcklbw	mm5,mm1			; mm2 = 00:xxxr3:00:xxxr2:00:xxxr1:00:xxxr0
		psllw		mm5,$0b			; mm2 = r3000:00:r2000:00:r1000:00:r0000:00
		
		packuswb	mm6,mm6			; mm3 = ??:??:??:??:b3:b2:b1:b0
		punpcklbw	mm6,mm1			; mm3 = 00b3:00b2:00b1:00b0
		psrlw		mm6,$3			; mm3 = 00:000b3:00:000b2:00:000b1:00:000b0
	
		packuswb	mm7,mm7			; mm4 = ??:??:??:??:g3:g2:g1:g0
		punpcklbw	mm7,mm1			; mm4 = 00g3:00g2:00g1:00g0
		psrlw		mm7,$2			; mm4 = 00:00g3:00:00g2:00:00g1:00:00g0
		psllw		mm7,$5			; mm4 = 00000g300000:00000g200000:...
	
		por			mm6,mm5			; mm3 = r3000000b3:r2000000b2:r1000000b1...
		por			mm6,mm7			; mm3 = r3g3b3:r2g2b2:r1g1b1:r0g0b1

		movq		[edi],mm6

		pxor		mm1,mm1			; zero mm1

		packuswb	mm2,mm2			; mm2 = ??:??:??:??:r3:r2:r1:r0
		psrlq		mm2,$3			; mm2 = ::::xxxr3:xxxr2:xxxr1:xxxr0
		punpcklbw	mm2,mm1			; mm2 = 00:xxxr3:00:xxxr2:00:xxxr1:00:xxxr0
		psllw		mm2,$0b			; mm2 = r3000:00:r2000:00:r1000:00:r0000:00
		
		packuswb	mm3,mm3			; mm3 = ??:??:??:??:b3:b2:b1:b0
		punpcklbw	mm3,mm1			; mm3 = 00b3:00b2:00b1:00b0
		psrlw		mm3,$3			; mm3 = 00:000b3:00:000b2:00:000b1:00:000b0
	
		packuswb	mm4,mm4			; mm4 = ??:??:??:??:g3:g2:g1:g0
		punpcklbw	mm4,mm1			; mm4 = 00g3:00g2:00g1:00g0
		psrlw		mm4,$2			; mm4 = 00:00g3:00:00g2:00:00g1:00:00g0
		psllw		mm4,$5			; mm4 = 00000g300000:00000g200000:...
	
		por			mm3,mm2			; mm3 = r3000000b3:r2000000b2:r1000000b1...
		por			mm3,mm4			; mm3 = r3g3b3:r2g2b2:r1g1b1:r0g0b1

		movq		[edi+edx],mm3

		add			ecx,2*2
		add			esi,4*2
		add			edi,4*SIZE_RGB16
		dec			eax
		jne			near LoopX7
	
		add			ecx,DWORD ( DCT_BUFFER_STRIDE - 8 ) *2
		add			esi,( DCT_BUFFER_STRIDE * 2 - 8 * 2 ) *2
		add			edi,[esp+0]				; SkipRGB
		dec			ebx
		jne			near LoopY7
		emms
		add			esp,4						; restore four variable room
		
		pop			edx
		pop			ecx
		pop			ebx
		pop			eax
		pop			esi
		pop			edi
		
		ret

; void mmxPutImage625_YUY2( PBYTE pImage, int Stride, PDCT_DATA pY )
GLOBAL mmxPutImage625_YUY2
ALIGN 16
mmxPutImage625_YUY2:
		push		edi
		push		esi
		push		eax
		push		ebx
		push		ecx
		push		edx
		
;	pUV = pY + 16;
;	SkipRGB = PutImageStride * 2 - 8 * 2 * SIZE_RGBQ;
		sub			esp,4				; room for one variable
		mov			edi,[esp+24+4+4]			; PutImageBuffer

		mov			eax,[esp+24+4+8]	; PutImageStride
		lea			eax,[eax+eax-(8*2*SIZE_RGB16)]		; PutImageStride*2
		mov			[esp+0],eax			; SkipRGB

		mov			esi,[esp+24+4+12]	; pY
		lea			ecx,[esi+16*SIZE_DCT_DATA]		; pUV
		mov			edx,[esp+24+4+8]	; PutImageStride
		mov			ebx,[SIZEY625]		; SizeY
		movq		mm7,[ADD128]
	LoopY11:
		mov			eax,[SIZEX625]	; SizeX
	LoopX11:
		movd		mm2,[ecx]		; v1:v0
		movd		mm3,[ecx+8*2]	; ecx+8*(TYPE DCT_DATA) u1:u0
		psraw		mm3,$1
		psraw		mm2,$1
		psraw		mm2,$3
		psraw		mm3,$3
		paddw		mm2,mm7
		paddw		mm3,mm7
		punpcklwd	mm3,mm2			; mm3= v1:u1:v0:u0
		movq		mm2,mm3			; mm2= v1:u1:v0:u0
		punpckldq	mm3,mm3			; mm3= v0:u0:v0:u0
		punpckhdq	mm2,mm2			; mm2= v1:u1:v1:u1
		
		movq		mm1,[esi]		; mm1= y3:y2:y1:y0
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm1,$3
		movq		mm4,mm1			; mm4= y3:y2:y1:y0
		punpcklwd	mm4,mm3			; mm4= y1:v0:y0:u0
		punpckhwd	mm1,mm3			; mm2= y3:u1:y2:v1
		packuswb	mm4,mm1			; mm2= y3:u0:y2:v0:y1:u0:y0:v0

		movq		[edi],mm4

		movq		mm1,[esi+DCT_BUFFER_STRIDE*2]	; mm1= y3:y2:y1:y0
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm1,$3
		movq		mm4,mm1			; mm4= u1:v1:u0:v0
		punpcklwd	mm4,mm2			; mm4= y1:u1:y0:v1
		punpckhwd	mm1,mm2			; mm2= y3:u1:y2:v1
		packuswb	mm4,mm1			; mm2= y3:u1:y2:v1:y1:u1:y0:v1

		movq		[edi+edx],mm4

		add			ecx,2*2
		add			esi,4*2
		add			edi,4*SIZE_RGB16
		dec			eax
		jne			near LoopX11
	
		add			ecx,DWORD ( DCT_BUFFER_STRIDE - 8 ) *2
		add			esi,( DCT_BUFFER_STRIDE * 2 - 8 * 2 ) *2
		add			edi,[esp+0]				; SkipRGB
		dec			ebx
		jne			near LoopY11
		emms
		add			esp,4						; restore four variable room
		
		pop			edx
		pop			ecx
		pop			ebx
		pop			eax
		pop			esi
		pop			edi
		
		ret

; void mmxPutImage625_RGB15( PBYTE pImage, int Stride, PDCT_DATA pY )
GLOBAL mmxPutImage625_RGB15
ALIGN 16
mmxPutImage625_RGB15:
		push		edi
		push		esi
		push		eax
		push		ebx
		push		ecx
		push		edx
		
;	pUV = pY + 16;
;	SkipRGB = PutImageStride * 2 - 8 * 2 * SIZE_RGBQ;
		sub			esp,4				; room for one variable
		mov			edi,[esp+24+4+4]			; PutImageBuffer

		mov			eax,[esp+24+4+8]	; PutImageStride
		lea			eax,[eax+eax-(8*2*SIZE_RGB16)]		; PutImageStride*2
		mov			[esp+0],eax			; SkipRGB

		mov			esi,[esp+24+4+12]	; pY
		lea			ecx,[esi+16*SIZE_DCT_DATA]		; pUV
		mov			edx,[esp+24+4+8]	; PutImageStride
		mov			ebx,[SIZEY625]		; SizeY
	LoopY8:
		mov			eax,[SIZEX625]	; SizeX
	LoopX8:
		movd		mm2,[ecx]		; v1:v0
		movd		mm3,[ecx+8*2]	; u1:u0
		psraw		mm3,$1
		psraw		mm2,$1
		movq		mm0,mm3
		movq		mm4,mm2
		pmulhw		mm2,[mmxRFromV]
		pmulhw		mm3,[mmxBFromU]
		pmulhw		mm4,[mmxGFromV]
		pmulhw		mm0,[mmxGFromU]
		paddw		mm4,mm0
		punpcklwd	mm2,mm2			; vR1:vR1:vR0:vR0
		punpcklwd	mm3,mm3			; uB1:uB1:uB0:uB0
		punpcklwd	mm4,mm4			; uvG1:uvG1:uvG0:uvG0

		movq		mm5,[esi]		; y03:y02:y01:y00
		movq		mm1,[esi+DCT_BUFFER_STRIDE*2]	; y13:y12:y11:y10
		psraw		mm1,$1
		paddw		mm1,[add1024]
		psraw		mm5,$1
		paddw		mm5,[add1024]
		psraw		mm5,DCT_SHIFT
		psraw		mm1,DCT_SHIFT
		movq		mm6,mm5			; y03:y02:y01:y00 shifted
		movq		mm7,mm5			; y03:y02:y01:y00 shifted

		paddw		mm5,mm2			; r03:r02:r01:r00
		paddw		mm6,mm3			; b03:b02:b01:b00
		paddw		mm7,mm4			; g03:g02:g01:g00
		paddw		mm2,mm1			; r13:r12:r11:r10
		paddw		mm3,mm1			; b13:b12:b11:b10
		paddw		mm4,mm1			; g13:g12:g11:g10

		pxor		mm1,mm1			; zero mm1

		packuswb	mm5,mm5			; mm2 = ??:??:??:??:r3:r2:r1:r0
		psrlq		mm5,$3			; mm2 = ::::xxxr3:xxxr2:xxxr1:xxxr0
		punpcklbw	mm5,mm1			; mm2 = 00:xxxr3:00:xxxr2:00:xxxr1:00:xxxr0
		psllw		mm5,$0a			; mm2 = r3000:00:r2000:00:r1000:00:r0000:00
		
		packuswb	mm6,mm6			; mm3 = ??:??:??:??:b3:b2:b1:b0
		punpcklbw	mm6,mm1			; mm3 = 00b3:00b2:00b1:00b0
		psrlw		mm6,$3			; mm3 = 00:000b3:00:000b2:00:000b1:00:000b0
	
		packuswb	mm7,mm7			; mm4 = ??:??:??:??:g3:g2:g1:g0
		punpcklbw	mm7,mm1			; mm4 = 00g3:00g2:00g1:00g0
		psrlw		mm7,$3			; mm4 = 00:00g3:00:00g2:00:00g1:00:00g0
		psllw		mm7,$5			; mm4 = 00000g300000:00000g200000:...
	
		por			mm6,mm5			; mm3 = r3000000b3:r2000000b2:r1000000b1...
		por			mm6,mm7			; mm3 = r3g3b3:r2g2b2:r1g1b1:r0g0b1

		movq		[edi],mm6

		pxor		mm1,mm1			; zero mm1

		packuswb	mm2,mm2			; mm2 = ??:??:??:??:r3:r2:r1:r0
		psrlq		mm2,$3			; mm2 = ::::xxxr3:xxxr2:xxxr1:xxxr0
		punpcklbw	mm2,mm1			; mm2 = 00:xxxr3:00:xxxr2:00:xxxr1:00:xxxr0
		psllw		mm2,$0a			; mm2 = r3000:00:r2000:00:r1000:00:r0000:00
		
		packuswb	mm3,mm3			; mm3 = ??:??:??:??:b3:b2:b1:b0
		punpcklbw	mm3,mm1			; mm3 = 00b3:00b2:00b1:00b0
		psrlw		mm3,$3			; mm3 = 00:000b3:00:000b2:00:000b1:00:000b0
	
		packuswb	mm4,mm4			; mm4 = ??:??:??:??:g3:g2:g1:g0
		punpcklbw	mm4,mm1			; mm4 = 00g3:00g2:00g1:00g0
		psrlw		mm4,$3			; mm4 = 00:00g3:00:00g2:00:00g1:00:00g0
		psllw		mm4,$5			; mm4 = 00000g300000:00000g200000:...
	
		por			mm3,mm2			; mm3 = r3000000b3:r2000000b2:r1000000b1...
		por			mm3,mm4			; mm3 = r3g3b3:r2g2b2:r1g1b1:r0g0b1

		movq		[edi+edx],mm3

		add			ecx,2*2
		add			esi,4*2
		add			edi,4*SIZE_RGB16
		dec			eax
		jne			near LoopX8
	
		add			ecx,DWORD ( DCT_BUFFER_STRIDE - 8 ) *2
		add			esi,( DCT_BUFFER_STRIDE * 2 - 8 * 2 ) *2
		add			edi,[esp+0]				; SkipRGB
		dec			ebx
		jne			near LoopY8
		emms
		add			esp,4						; restore four variable room
		
		pop			edx
		pop			ecx
		pop			ebx
		pop			eax
		pop			esi
		pop			edi
		
		ret



.data
mmxRFromV:			dw 0x2cdd,0x2cdd,0x2cdd,0x2cdd	; 2cdd
mmxBFromU:			dw 0x38b4,0x38b4,0x38b4,0x38b4	; f4fe
mmxGFromV:			dw 0xe927,0xe927,0xe927,0xe927	; e927
mmxGFromU:			dw 0xf4fe,0xf4fe,0xf4fe,0xf4fe	; 38b4
add1024:			dw 0x0400,0x0400,0x0400,0x0400
alphamask:			dd 0xff000000,0xff000000
SIZEX625:			dd 0x00000004
SIZEY625:			dd 0x00000008
ADD128:				dw 0x0080,0x0080,0x0080,0x0080
IMAGE_STRIDE_RGB32:	dd	RGB32_IMAGE_STRIDE
IMAGE_STRIDE_RGB16:	dd	RGB16_IMAGE_STRIDE
