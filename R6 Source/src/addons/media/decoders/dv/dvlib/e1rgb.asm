SECTION .text

DCT_SHIFT equ $3
SIZE_RGBQ equ $4
SIZE_RGB16 equ $2
DCT_BUFFER_STRIDE equ $20
SIZE_DCT_DATA equ $2
RGB32_IMAGE_STRIDE equ $0b40
RGB16_IMAGE_STRIDE equ $05a0

; void mmxGetImage525_RGB15( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
GLOBAL mmxGetImage525_RGB15
ALIGN 16
mmxGetImage525_RGB15:
		push		edi
		push		esi
		push		eax
		push		ecx
		push		edx
		
;	SkipRGB = RGB16_IMAGE_STRIDE- SizeX * SIZE_RGB16;
;	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX*2)
;	SkipUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX/2)
;	SizeX >>= 3;
		sub			esp,16						; room for four variables
		mov			edi,[esp+16+20+4]			; pSrcBuffer
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

		mov			edx,[esp+16+20+16]	; SizeY
		pxor		mm4,mm4						; mm4 = 0
	LoopY4:
		mov			eax,[esp+12]				; SizeX
	LoopX4:
		movq		mm1,[edi+SIZE_RGB16*0]		; r3g3b3:r2g2b2:r1g1b1:r0g0b0
		
		movq		mm2,mm1						; mm2=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		movq		mm5,mm1						; mm5=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		psrlw		mm2,7						; mm2=r3.:r2.:r1.:r0.
		pand		mm2,[HIGH5MASK]				; mm2=r3:r2:r1:r0
		psllw		mm5,3						; mm2=.b3:.b2:.b1:.b0
		pand		mm5,[HIGH5MASK]				; mm5=b3:b2:b1:b0
		psrlw		mm1,2						; mm1=.g3.:.g2.:.g1.:.g0.
		pand		mm1,[HIGH5MASK]				; mm1=g3:g2:g1:g0

		movq		mm3,mm2
		punpcklwd	mm3,mm1						; mm2=g1:r1:g0:r0
		movq		mm4,mm2
		punpckhwd	mm4,mm1						; mm4=g3:r3:g2:r2
		paddw		mm4,mm3						; mm4=g13:r13:g02:r02
		movq		mm3,mm5
		punpcklwd	mm3,mm1						; mm3=g1:b1:g0:b0
		movq		mm0,mm5
		punpckhwd	mm0,mm1						; mm0=g3:b3:g2:b2
		paddw		mm0,mm3						; mm0=g13:b13:g02:b02
		movq		mm7,mm0
		punpckldq	mm7,mm4						; mm7=g02:r02:g02:b02
		punpckhdq	mm0,mm4						; mm0=g13:r13:g13:b13
		paddw		mm7,mm0						; mm7=tg:tr:tg:tb
		psllw		mm7,2						; mm7 = 0123:::tr::tg::tb

		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi],mm2					; y3:y2:y1:y0

; second 8 pixels
		movq		mm1,[edi+SIZE_RGB16*4]		; mm1 = :r5:g5:b5::r4:g4:b4

		movq		mm2,mm1						; mm2=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		movq		mm5,mm1						; mm5=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		psrlw		mm2,7						; mm2=r3.:r2.:r1.:r0.
		pand		mm2,[HIGH5MASK]				; mm2=r3:r2:r1:r0
		psllw		mm5,3						; mm2=.b3:.b2:.b1:.b0
		pand		mm5,[HIGH5MASK]				; mm5=b3:b2:b1:b0
		psrlw		mm1,2						; mm1=.g3.:.g2.:.g1.:.g0.
		pand		mm1,[HIGH5MASK]				; mm1=g3:g2:g1:g0

		movq		mm3,mm2
		punpcklwd	mm3,mm1						; mm2=g1:r1:g0:r0
		movq		mm4,mm2
		punpckhwd	mm4,mm1						; mm4=g3:r3:g2:r2
		paddw		mm4,mm3						; mm4=g13:r13:g02:r02
		movq		mm3,mm5
		punpcklwd	mm3,mm1						; mm3=g1:b1:g0:b0
		movq		mm0,mm5
		punpckhwd	mm0,mm1						; mm0=g3:b3:g2:b2
		paddw		mm0,mm3						; mm0=g13:b13:g02:b02
		movq		mm6,mm0
		punpckldq	mm6,mm4						; mm6=g02:r02:g02:b02
		punpckhdq	mm0,mm4						; mm0=g13:r13:g13:b13
		paddw		mm6,mm0						; mm6=tg:tr:tg:tb
		psllw		mm6,2						; mm6=4567:::tr::tg::tb

		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi+8],mm2					; y3:y2:y1:y0


		movq		mm5,mm7						; mm5 = 0123:::tr::tg::tb
		movq		mm0,mm6						; mm0 = 4567:::tr::tg::tb
		movq		mm2,[mmxVFromRGB]
		pmulhw		mm7,[mmxUFromRGB]			; mm7 = 0:::u1::u2::u3
		pmulhw		mm6,[mmxUFromRGB]			; mm6 = 4:::U1::U2::U3
		pmulhw		mm5,mm2						; mm5 = 0:::v1::v2::v3
		pmulhw		mm0,mm2						; mm0 = 4:::V1::V2::V3
		
		movq		mm2,mm5						; mm2 = 0:::v1::v2::v3
		punpcklwd	mm2,mm0						; mm2 = :V2::v2::V3::v3
		punpckhwd	mm5,mm0						; mm5 = :::::V1::v1
		movq		mm0,mm7						; mm0 = :::u1::u2::u3
		punpcklwd	mm0,mm6						; mm0 = :U2::u2::U3::u3
		punpckhwd	mm7,mm6						; mm7 = :::::U1::u1

		punpckldq	mm7,mm5						; mm7 = :V1::v1::U1::u1
		movq		mm5,mm0						; mm5 = :U2::u2::U3::u3
		punpckldq	mm5,mm2						; mm5 = :V3::v3::U3::u3
		punpckhdq	mm0,mm2						; mm0 = :V2::v2::U2::u2
		paddw		mm0,mm5
		paddw		mm0,mm7						; mm0 = :V::v::U::u
		psllw		mm0,$1
		movd		[ecx+8*SIZE_DCT_DATA],mm0
		punpckhdq	mm0,mm0						; mm0 = :V::v::V::v
		movd		[ecx],mm0

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

; void mmxGetImage525_RGB16( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
GLOBAL mmxGetImage525_RGB16
ALIGN 16
mmxGetImage525_RGB16:
		push		edi
		push		esi
		push		eax
		push		ecx
		push		edx
		
;	SkipRGB = RGB16_IMAGE_STRIDE- SizeX * SIZE_RGB16;
;	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX*2)
;	SkipUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX/2)
;	SizeX >>= 3;
		sub			esp,16						; room for four variables
		mov			edi,[esp+16+20+4]			; pSrcBuffer
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

		mov			edx,DWORD[esp+$14+16+16]	; SizeY
		pxor		mm4,mm4						; mm4 = 0
	LoopY3:
		mov			eax,[esp+12]				; SizeX
	LoopX3:
	
		movq		mm1,[edi+SIZE_RGB16*0]		; r3g3b3:r2g2b2:r1g1b1:r0g0b0
		
		movq		mm2,mm1						; mm2=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		movq		mm5,mm1						; mm5=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		psrlw		mm2,8						; mm2=r3.:r2.:r1.:r0.
		pand		mm2,[HIGH5MASK]				; mm2=r3:r2:r1:r0
		psllw		mm5,3						; mm2=.b3:.b2:.b1:.b0
		pand		mm5,[HIGH5MASK]				; mm5=b3:b2:b1:b0
		psrlw		mm1,3						; mm1=.g3.:.g2.:.g1.:.g0.
		pand		mm1,[HIGH6MASK]				; mm1=g3:g2:g1:g0

		movq		mm3,mm2
		punpcklwd	mm3,mm1						; mm2=g1:r1:g0:r0
		movq		mm4,mm2
		punpckhwd	mm4,mm1						; mm4=g3:r3:g2:r2
		paddw		mm4,mm3						; mm4=g13:r13:g02:r02
		movq		mm3,mm5
		punpcklwd	mm3,mm1						; mm3=g1:b1:g0:b0
		movq		mm0,mm5
		punpckhwd	mm0,mm1						; mm0=g3:b3:g2:b2
		paddw		mm0,mm3						; mm0=g13:b13:g02:b02
		movq		mm7,mm0
		punpckldq	mm7,mm4						; mm7=g02:r02:g02:b02
		punpckhdq	mm0,mm4						; mm0=g13:r13:g13:b13
		paddw		mm7,mm0						; mm7=tg:tr:tg:tb
		psllw		mm7,2						; mm7 = 0123:::tr::tg::tb

		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi],mm2					; y3:y2:y1:y0

; second 8 pixels
		movq		mm1,[edi+SIZE_RGB16*4]		; mm1 = :r5:g5:b5::r4:g4:b4

		movq		mm2,mm1						; mm2=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		movq		mm5,mm1						; mm5=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		psrlw		mm2,8						; mm2=r3.:r2.:r1.:r0.
		pand		mm2,[HIGH5MASK]				; mm2=r3:r2:r1:r0
		psllw		mm5,3						; mm2=.b3:.b2:.b1:.b0
		pand		mm5,[HIGH5MASK]				; mm5=b3:b2:b1:b0
		psrlw		mm1,3						; mm1=.g3.:.g2.:.g1.:.g0.
		pand		mm1,[HIGH6MASK]				; mm1=g3:g2:g1:g0

		movq		mm3,mm2
		punpcklwd	mm3,mm1						; mm2=g1:r1:g0:r0
		movq		mm4,mm2
		punpckhwd	mm4,mm1						; mm4=g3:r3:g2:r2
		paddw		mm4,mm3						; mm4=g13:r13:g02:r02
		movq		mm3,mm5
		punpcklwd	mm3,mm1						; mm3=g1:b1:g0:b0
		movq		mm0,mm5
		punpckhwd	mm0,mm1						; mm0=g3:b3:g2:b2
		paddw		mm0,mm3						; mm0=g13:b13:g02:b02
		movq		mm6,mm0
		punpckldq	mm6,mm4						; mm6=g02:r02:g02:b02
		punpckhdq	mm0,mm4						; mm0=g13:r13:g13:b13
		paddw		mm6,mm0						; mm6=tg:tr:tg:tb
		psllw		mm6,2						; mm6=4567:::tr::tg::tb

		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi+8],mm2					; y3:y2:y1:y0


		movq		mm5,mm7						; mm5 = 0123:::tr::tg::tb
		movq		mm0,mm6						; mm0 = 4567:::tr::tg::tb
		movq		mm2,[mmxVFromRGB]
		pmulhw		mm7,[mmxUFromRGB]			; mm7 = 0:::u1::u2::u3
		pmulhw		mm6,[mmxUFromRGB]			; mm6 = 4:::U1::U2::U3
		pmulhw		mm5,mm2						; mm5 = 0:::v1::v2::v3
		pmulhw		mm0,mm2						; mm0 = 4:::V1::V2::V3
		
		movq		mm2,mm5						; mm2 = 0:::v1::v2::v3
		punpcklwd	mm2,mm0						; mm2 = :V2::v2::V3::v3
		punpckhwd	mm5,mm0						; mm5 = :::::V1::v1
		movq		mm0,mm7						; mm0 = :::u1::u2::u3
		punpcklwd	mm0,mm6						; mm0 = :U2::u2::U3::u3
		punpckhwd	mm7,mm6						; mm7 = :::::U1::u1

		punpckldq	mm7,mm5						; mm7 = :V1::v1::U1::u1
		movq		mm5,mm0						; mm5 = :U2::u2::U3::u3
		punpckldq	mm5,mm2						; mm5 = :V3::v3::U3::u3
		punpckhdq	mm0,mm2						; mm0 = :V2::v2::U2::u2
		paddw		mm0,mm5
		paddw		mm0,mm7						; mm0 = :V::v::U::u
		psllw		mm0,$1
		movd		[ecx+8*SIZE_DCT_DATA],mm0
		punpckhdq	mm0,mm0						; mm0 = :V::v::V::v
		movd		[ecx],mm0

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


; void mmxGetImage525_RGBQ( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
; void mmxGetImage525_RGBQ( PBYTE GetImageBuffer,int StartX, int StartY, int SizeX, int SizeY, int GetImageStride,PDCT_DATA pY, PDCT_DATA pUV );
GLOBAL mmxGetImage525_RGBQ
ALIGN 16
mmxGetImage525_RGBQ:
		push		edi
		push		esi
		push		eax
		push		ecx
		push		edx
		
;	pSrcBuffer = GetImageBuffer + StartY * RGB32_IMAGE_STRIDE + StartX * SIZE_RGBQ;
;	SkipRGB = RGB32_IMAGE_STRIDE- SizeX * SIZE_RGBQ;
;	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX*2)
;	SkipUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * sizeof( DCT_DATA ) = (DCT_BUFFER_STRIDE*2)-(SizeX/2)
;	SizeX >>= 3;
		sub			esp,16						; room for four variables
		mov			edi,[esp+16+20+4]					; pSrcBuffer
		mov			esi,[esp+16+20+20]				; pY
		mov			ecx,[esp+16+20+24]				; pUV
		
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
		pxor		mm4,mm4						; mm4 = 0
	LoopY5:
		mov			eax,[esp+12]				; SizeX
	LoopX5:
	
		movq		mm3,[edi+SIZE_RGBQ*0]		; :r1:g1:b1::r0:g0:b0
		movq		mm1,[edi+SIZE_RGBQ*2]		; :r3:g3:b3::r2:g2:b2
		
		movq		mm2,mm3						; mm2 = :r1:g1:b1::r0:g0:b0
		movq		mm5,mm1						; mm5 = :r3:g3:b3::r2:g2:b2
		punpcklbw	mm2,mm4						; mm2 = :::r0::g0::b0
		punpckhbw	mm3,mm4						; mm3 = :::r1::g1::b1
		punpcklbw	mm5,mm4						; mm5 = :::r2::g2::b2
		punpckhbw	mm1,mm4						; mm1 = :::r3::g3::b3
		movq		mm7,mm1
		paddw		mm7,mm2
		paddw		mm7,mm3
		paddw		mm7,mm5						; mm7 = 0123 totals
		psllw		mm7,2						; mm7 = 0123:::tr::tg::tb

		movq		mm0,mm5						; mm0 = :::r2::g2::b2
		punpcklwd	mm0,mm1						; mm0 = :g3::g2::b3::b2
		punpckhwd	mm5,mm1						; mm5 = :::::r3::r2
		movq		mm1,mm2						; mm1 = :::r0::g0::b0
		punpcklwd	mm1,mm3						; mm1 = :g1::g0::b1::b0
		punpckhwd	mm2,mm3						; mm2 = :::::r1::r0
		punpckldq	mm2,mm5						; mm2 = :r3::r2::r1::r0
		movq		mm5,mm1						; mm5 = :g1::g0::b1::b0
		punpckldq	mm5,mm0						; mm5 = :b3::b2::b1::b0
		punpckhdq	mm1,mm0						; mm1 = :g3::g2::g1::g0
		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi],mm2					; y3:y2:y1:y0

; second 8 pixels
		movq		mm3,[edi+SIZE_RGBQ*4]		; mm3 = :r5:g5:b5::r4:g4:b4
		movq		mm1,[edi+SIZE_RGBQ*6]		; mm1 = :r7:g7:b7::r6:g6:b6
		
		movq		mm2,mm3						; mm2 = :r5:g5:b5::r4:g4:b4
		movq		mm5,mm1						; mm5 = :r7:g7:b7::r6:g6:b6
		punpcklbw	mm2,mm4						; mm2 = :::r4::g4::b4
		punpckhbw	mm3,mm4						; mm3 = :::r5::g5::b5
		punpcklbw	mm5,mm4						; mm5 = :::r6::g6::b6
		punpckhbw	mm1,mm4						; mm1 = :::r7::g7::b7
		movq		mm6,mm1
		paddw		mm6,mm2
		paddw		mm6,mm3
		paddw		mm6,mm5						; mm6 = 5678 totals
		psllw		mm6,2						; mm6 = 5678:::tr::tg::tb

		movq		mm0,mm5						; mm0 = :::r6::g6::b6
		punpcklwd	mm0,mm1						; mm0 = :g7::g6::b7::b6
		punpckhwd	mm5,mm1						; mm5 = :::::r7::r6
		movq		mm1,mm2						; mm1 = :::r4::g4::b4
		punpcklwd	mm1,mm3						; mm1 = :g5::g4::b5::b4
		punpckhwd	mm2,mm3						; mm2 = :::::r5::r4
		punpckldq	mm2,mm5						; mm2 = :r7::r6::r5::r4
		movq		mm5,mm1						; mm5 = :g5::g4::b5::b4
		punpckldq	mm5,mm0						; mm5 = :b7::b6::b5::b4
		punpckhdq	mm1,mm0						; mm1 = :g7::g6::g5::g4
		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi+8],mm2					; y3:y2:y1:y0

		movq		mm5,mm7						; mm5 = 0123:::tr::tg::tb
		movq		mm0,mm6						; mm0 = 4567:::tr::tg::tb
		movq		mm2,[mmxVFromRGB]
		pmulhw		mm7,[mmxUFromRGB]			; mm7 = 0:::u1::u2::u3
		pmulhw		mm6,[mmxUFromRGB]			; mm6 = 4:::U1::U2::U3
		pmulhw		mm5,mm2						; mm5 = 0:::v1::v2::v3
		pmulhw		mm0,mm2						; mm0 = 4:::V1::V2::V3
		
		movq		mm2,mm5						; mm2 = 0:::v1::v2::v3
		punpcklwd	mm2,mm0						; mm2 = :V2::v2::V3::v3
		punpckhwd	mm5,mm0						; mm5 = :::::V1::v1
		movq		mm0,mm7						; mm0 = :::u1::u2::u3
		punpcklwd	mm0,mm6						; mm0 = :U2::u2::U3::u3
		punpckhwd	mm7,mm6						; mm7 = :::::U1::u1

		punpckldq	mm7,mm5						; mm7 = :V1::v1::U1::u1
		movq		mm5,mm0						; mm5 = :U2::u2::U3::u3
		punpckldq	mm5,mm2						; mm5 = :V3::v3::U3::u3
		punpckhdq	mm0,mm2						; mm0 = :V2::v2::U2::u2
		paddw		mm0,mm5
		paddw		mm0,mm7						; mm0 = :V::v::U::u
		psllw		mm0,$1
		movd		[ecx+8*SIZE_DCT_DATA],mm0
		punpckhdq	mm0,mm0						; mm0 = :V::v::V::v
		movd		[ecx],mm0
		
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
		

; void    PASCAL    GetImage625_RGBQ( PBYTE pImage, int Stride, PDCT_DATA pY )
GLOBAL mmxGetImage625_RGBQ
ALIGN 16
mmxGetImage625_RGBQ:
		push		edi
		push		esi
		push		eax
		push		ebx
		push		ecx
		push		edx
		
;	pUV = pY + 16;
;	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * SIZE_RGBQ;
;	SkipRGB = PutImageStride * 2 - 8 * 2 * SIZE_RGBQ;
		sub			esp,4				; room for one variable
		mov			edi,[esp+24+4+4]			; PutImageBuffer

		mov			eax,[esp+24+4+8]	; PutImageStride
		lea			eax,[eax+eax-(8*2*SIZE_RGBQ)]		; PutImageStride*2
		mov			[esp+0],eax			; SkipRGB

		mov			esi,[esp+24+4+12]	; pY
		lea			ecx,[esi+16*SIZE_DCT_DATA]		; pUV
		mov			edx,[esp+24+4+8]	; PutImageStride
		mov			ebx,[SIZEY625]		; SizeY
		pxor		mm4,mm4
	LoopY2:
		mov			eax,[SIZEX625]	; SizeX
	LoopX2:

; code here...
		movq		mm3,[edi+SIZE_RGBQ*0]		; :r1:g1:b1::r0:g0:b0
		movq		mm1,[edi+SIZE_RGBQ*2]		; :r3:g3:b3::r2:g2:b2
		
		movq		mm2,mm3						; mm2 = :r1:g1:b1::r0:g0:b0
		movq		mm5,mm1						; mm5 = :r3:g3:b3::r2:g2:b2
		punpcklbw	mm2,mm4						; mm2 = :::r0::g0::b0
		punpckhbw	mm3,mm4						; mm3 = :::r1::g1::b1
		punpcklbw	mm5,mm4						; mm5 = :::r2::g2::b2
		punpckhbw	mm1,mm4						; mm1 = :::r3::g3::b3
		movq		mm7,mm1
		paddw		mm7,mm2
		paddw		mm7,mm3
		paddw		mm7,mm5						; mm7 = 0123 totals
		psllw		mm7,2						; mm7 = 0123:::tr::tg::tb

		movq		mm0,mm5						; mm0 = :::r2::g2::b2
		punpcklwd	mm0,mm1						; mm0 = :g3::g2::b3::b2
		punpckhwd	mm5,mm1						; mm5 = :::::r3::r2
		movq		mm1,mm2						; mm1 = :::r0::g0::b0
		punpcklwd	mm1,mm3						; mm1 = :g1::g0::b1::b0
		punpckhwd	mm2,mm3						; mm2 = :::::r1::r0
		punpckldq	mm2,mm5						; mm2 = :r3::r2::r1::r0
		movq		mm5,mm1						; mm5 = :g1::g0::b1::b0
		punpckldq	mm5,mm0						; mm5 = :b3::b2::b1::b0
		punpckhdq	mm1,mm0						; mm1 = :g3::g2::g1::g0
		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi],mm2					; y3:y2:y1:y0

; second 8 pixels
		movq		mm3,[edi+edx+SIZE_RGBQ*0]		; mm3 = :r5:g5:b5::r4:g4:b4
		movq		mm1,[edi+edx+SIZE_RGBQ*2]		; mm1 = :r7:g7:b7::r6:g6:b6
		
		movq		mm2,mm3						; mm2 = :r5:g5:b5::r4:g4:b4
		movq		mm5,mm1						; mm5 = :r7:g7:b7::r6:g6:b6
		punpcklbw	mm2,mm4						; mm2 = :::r4::g4::b4
		punpckhbw	mm3,mm4						; mm3 = :::r5::g5::b5
		punpcklbw	mm5,mm4						; mm5 = :::r6::g6::b6
		punpckhbw	mm1,mm4						; mm1 = :::r7::g7::b7
		movq		mm6,mm1
		paddw		mm6,mm2
		paddw		mm6,mm3
		paddw		mm6,mm5						; mm6 = 5678 totals
		psllw		mm6,2						; mm6 = 5678:::tr::tg::tb

		movq		mm0,mm5						; mm0 = :::r6::g6::b6
		punpcklwd	mm0,mm1						; mm0 = :g7::g6::b7::b6
		punpckhwd	mm5,mm1						; mm5 = :::::r7::r6
		movq		mm1,mm2						; mm1 = :::r4::g4::b4
		punpcklwd	mm1,mm3						; mm1 = :g5::g4::b5::b4
		punpckhwd	mm2,mm3						; mm2 = :::::r5::r4
		punpckldq	mm2,mm5						; mm2 = :r7::r6::r5::r4
		movq		mm5,mm1						; mm5 = :g5::g4::b5::b4
		punpckldq	mm5,mm0						; mm5 = :b7::b6::b5::b4
		punpckhdq	mm1,mm0						; mm1 = :g7::g6::g5::g4
		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi+DCT_BUFFER_STRIDE*2],mm2					; y3:y2:y1:y0

		movq		mm5,mm7						; mm5 = 0123:::tr::tg::tb
		movq		mm0,mm6						; mm0 = 4567:::tr::tg::tb
		movq		mm2,[mmxVFromRGB]
		pmulhw		mm7,[mmxUFromRGB]			; mm7 = 0:::u1::u2::u3
		pmulhw		mm6,[mmxUFromRGB]			; mm6 = 4:::U1::U2::U3
		pmulhw		mm5,mm2						; mm5 = 0:::v1::v2::v3
		pmulhw		mm0,mm2						; mm0 = 4:::V1::V2::V3
		
		movq		mm2,mm5						; mm2 = 0:::v1::v2::v3
		punpcklwd	mm2,mm0						; mm2 = :V2::v2::V3::v3
		punpckhwd	mm5,mm0						; mm5 = :::::V1::v1
		movq		mm0,mm7						; mm0 = :::u1::u2::u3
		punpcklwd	mm0,mm6						; mm0 = :U2::u2::U3::u3
		punpckhwd	mm7,mm6						; mm7 = :::::U1::u1

		punpckldq	mm7,mm5						; mm7 = :V1::v1::U1::u1
		movq		mm5,mm0						; mm5 = :U2::u2::U3::u3
		punpckldq	mm5,mm2						; mm5 = :V3::v3::U3::u3
		punpckhdq	mm0,mm2						; mm0 = :V2::v2::U2::u2
		paddw		mm0,mm5
		paddw		mm0,mm7						; mm0 = :V::v::U::u
		psllw		mm0,$1
		movd		[ecx+8*SIZE_DCT_DATA],mm0
		punpckhdq	mm0,mm0						; mm0 = :V::v::V::v
		movd		[ecx],mm0

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
		

; void mmxGetImage625_RGB16( PBYTE pImage, int Stride, PDCT_DATA pY )
GLOBAL mmxGetImage625_RGB16
ALIGN 16
mmxGetImage625_RGB16:
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

		movq		mm1,[edi+SIZE_RGB16*0]		; r3g3b3:r2g2b2:r1g1b1:r0g0b0
		
		movq		mm2,mm1						; mm2=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		movq		mm5,mm1						; mm5=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		psrlw		mm2,8						; mm2=r3.:r2.:r1.:r0.
		pand		mm2,[HIGH5MASK]				; mm2=r3:r2:r1:r0
		psllw		mm5,3						; mm2=.b3:.b2:.b1:.b0
		pand		mm5,[HIGH5MASK]				; mm5=b3:b2:b1:b0
		psrlw		mm1,3						; mm1=.g3.:.g2.:.g1.:.g0.
		pand		mm1,[HIGH6MASK]				; mm1=g3:g2:g1:g0

		movq		mm3,mm2
		punpcklwd	mm3,mm1						; mm2=g1:r1:g0:r0
		movq		mm4,mm2
		punpckhwd	mm4,mm1						; mm4=g3:r3:g2:r2
		paddw		mm4,mm3						; mm4=g13:r13:g02:r02
		movq		mm3,mm5
		punpcklwd	mm3,mm1						; mm3=g1:b1:g0:b0
		movq		mm0,mm5
		punpckhwd	mm0,mm1						; mm0=g3:b3:g2:b2
		paddw		mm0,mm3						; mm0=g13:b13:g02:b02
		movq		mm7,mm0
		punpckldq	mm7,mm4						; mm7=g02:r02:g02:b02
		punpckhdq	mm0,mm4						; mm0=g13:r13:g13:b13
		paddw		mm7,mm0						; mm7=tg:tr:tg:tb
		psllw		mm7,2						; mm7 = 0123:::tr::tg::tb

		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi],mm2					; y3:y2:y1:y0

; second 8 pixels
		movq		mm1,[edi+edx]		; mm1 = :r5:g5:b5::r4:g4:b4

		movq		mm2,mm1						; mm2=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		movq		mm5,mm1						; mm5=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		psrlw		mm2,8						; mm2=r3.:r2.:r1.:r0.
		pand		mm2,[HIGH5MASK]				; mm2=r3:r2:r1:r0
		psllw		mm5,3						; mm2=.b3:.b2:.b1:.b0
		pand		mm5,[HIGH5MASK]				; mm5=b3:b2:b1:b0
		psrlw		mm1,3						; mm1=.g3.:.g2.:.g1.:.g0.
		pand		mm1,[HIGH6MASK]				; mm1=g3:g2:g1:g0

		movq		mm3,mm2
		punpcklwd	mm3,mm1						; mm2=g1:r1:g0:r0
		movq		mm4,mm2
		punpckhwd	mm4,mm1						; mm4=g3:r3:g2:r2
		paddw		mm4,mm3						; mm4=g13:r13:g02:r02
		movq		mm3,mm5
		punpcklwd	mm3,mm1						; mm3=g1:b1:g0:b0
		movq		mm0,mm5
		punpckhwd	mm0,mm1						; mm0=g3:b3:g2:b2
		paddw		mm0,mm3						; mm0=g13:b13:g02:b02
		movq		mm6,mm0
		punpckldq	mm6,mm4						; mm6=g02:r02:g02:b02
		punpckhdq	mm0,mm4						; mm0=g13:r13:g13:b13
		paddw		mm6,mm0						; mm6=tg:tr:tg:tb
		psllw		mm6,2						; mm6=4567:::tr::tg::tb

		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi+DCT_BUFFER_STRIDE*2],mm2					; y3:y2:y1:y0


		movq		mm5,mm7						; mm5 = 0123:::tr::tg::tb
		movq		mm0,mm6						; mm0 = 4567:::tr::tg::tb
		movq		mm2,[mmxVFromRGB]
		pmulhw		mm7,[mmxUFromRGB]			; mm7 = 0:::u1::u2::u3
		pmulhw		mm6,[mmxUFromRGB]			; mm6 = 4:::U1::U2::U3
		pmulhw		mm5,mm2						; mm5 = 0:::v1::v2::v3
		pmulhw		mm0,mm2						; mm0 = 4:::V1::V2::V3
		
		movq		mm2,mm5						; mm2 = 0:::v1::v2::v3
		punpcklwd	mm2,mm0						; mm2 = :V2::v2::V3::v3
		punpckhwd	mm5,mm0						; mm5 = :::::V1::v1
		movq		mm0,mm7						; mm0 = :::u1::u2::u3
		punpcklwd	mm0,mm6						; mm0 = :U2::u2::U3::u3
		punpckhwd	mm7,mm6						; mm7 = :::::U1::u1

		punpckldq	mm7,mm5						; mm7 = :V1::v1::U1::u1
		movq		mm5,mm0						; mm5 = :U2::u2::U3::u3
		punpckldq	mm5,mm2						; mm5 = :V3::v3::U3::u3
		punpckhdq	mm0,mm2						; mm0 = :V2::v2::U2::u2
		paddw		mm0,mm5
		paddw		mm0,mm7						; mm0 = :V::v::U::u
		psllw		mm0,$1
		movd		[ecx+8*SIZE_DCT_DATA],mm0
		punpckhdq	mm0,mm0						; mm0 = :V::v::V::v
		movd		[ecx],mm0


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

; void mmxGetImage625_RGB15( PBYTE pImage, int Stride, PDCT_DATA pY )
GLOBAL mmxGetImage625_RGB15
ALIGN 16
mmxGetImage625_RGB15:
		push		edi
		push		esi
		push		eax
		push		ebx
		push		ecx
		push		edx
		
;	pUV = pY + 16;
;	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * SIZE_RGBQ;
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

		movq		mm1,[edi+SIZE_RGB16*0]		; r3g3b3:r2g2b2:r1g1b1:r0g0b0
		
		movq		mm2,mm1						; mm2=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		movq		mm5,mm1						; mm5=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		psrlw		mm2,7						; mm2=r3.:r2.:r1.:r0.
		pand		mm2,[HIGH5MASK]				; mm2=r3:r2:r1:r0
		psllw		mm5,3						; mm2=.b3:.b2:.b1:.b0
		pand		mm5,[HIGH6MASK]				; mm5=b3:b2:b1:b0
		psrlw		mm1,2						; mm1=.g3.:.g2.:.g1.:.g0.
		pand		mm1,[HIGH6MASK]				; mm1=g3:g2:g1:g0

		movq		mm3,mm2
		punpcklwd	mm3,mm1						; mm2=g1:r1:g0:r0
		movq		mm4,mm2
		punpckhwd	mm4,mm1						; mm4=g3:r3:g2:r2
		paddw		mm4,mm3						; mm4=g13:r13:g02:r02
		movq		mm3,mm5
		punpcklwd	mm3,mm1						; mm3=g1:b1:g0:b0
		movq		mm0,mm5
		punpckhwd	mm0,mm1						; mm0=g3:b3:g2:b2
		paddw		mm0,mm3						; mm0=g13:b13:g02:b02
		movq		mm7,mm0
		punpckldq	mm7,mm4						; mm7=g02:r02:g02:b02
		punpckhdq	mm0,mm4						; mm0=g13:r13:g13:b13
		paddw		mm7,mm0						; mm7=tg:tr:tg:tb
		psllw		mm7,2						; mm7 = 0123:::tr::tg::tb

		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi],mm2					; y3:y2:y1:y0

; second 8 pixels
		movq		mm1,[edi+edx]		; mm1 = :r5:g5:b5::r4:g4:b4

		movq		mm2,mm1						; mm2=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		movq		mm5,mm1						; mm5=r3g3b3:r2g2b2:r1g1b1:r0g0b0
		psrlw		mm2,7						; mm2=r3.:r2.:r1.:r0.
		pand		mm2,[HIGH5MASK]				; mm2=r3:r2:r1:r0
		psllw		mm5,3						; mm2=.b3:.b2:.b1:.b0
		pand		mm5,[HIGH6MASK]				; mm5=b3:b2:b1:b0
		psrlw		mm1,2						; mm1=.g3.:.g2.:.g1.:.g0.
		pand		mm1,[HIGH6MASK]				; mm1=g3:g2:g1:g0

		movq		mm3,mm2
		punpcklwd	mm3,mm1						; mm2=g1:r1:g0:r0
		movq		mm4,mm2
		punpckhwd	mm4,mm1						; mm4=g3:r3:g2:r2
		paddw		mm4,mm3						; mm4=g13:r13:g02:r02
		movq		mm3,mm5
		punpcklwd	mm3,mm1						; mm3=g1:b1:g0:b0
		movq		mm0,mm5
		punpckhwd	mm0,mm1						; mm0=g3:b3:g2:b2
		paddw		mm0,mm3						; mm0=g13:b13:g02:b02
		movq		mm6,mm0
		punpckldq	mm6,mm4						; mm6=g02:r02:g02:b02
		punpckhdq	mm0,mm4						; mm0=g13:r13:g13:b13
		paddw		mm6,mm0						; mm6=tg:tr:tg:tb
		psllw		mm6,2						; mm6=4567:::tr::tg::tb

		psllw		mm2,4
		psllw		mm1,4
		psllw		mm5,4
		pmulhw		mm2,[mmxYFromR]				;
		pmulhw		mm1,[mmxYFromG]				;
		pmulhw		mm5,[mmxYFromB]				;

		paddw		mm2,mm1
		paddw		mm2,mm5						; mm2 = :y3:y2:y1:y0
		psubw		mm2,[add1024]
		psllw		mm2,$1
		movq		[esi+DCT_BUFFER_STRIDE*2],mm2					; y3:y2:y1:y0


		movq		mm5,mm7						; mm5 = 0123:::tr::tg::tb
		movq		mm0,mm6						; mm0 = 4567:::tr::tg::tb
		movq		mm2,[mmxVFromRGB]
		pmulhw		mm7,[mmxUFromRGB]			; mm7 = 0:::u1::u2::u3
		pmulhw		mm6,[mmxUFromRGB]			; mm6 = 4:::U1::U2::U3
		pmulhw		mm5,mm2						; mm5 = 0:::v1::v2::v3
		pmulhw		mm0,mm2						; mm0 = 4:::V1::V2::V3
		
		movq		mm2,mm5						; mm2 = 0:::v1::v2::v3
		punpcklwd	mm2,mm0						; mm2 = :V2::v2::V3::v3
		punpckhwd	mm5,mm0						; mm5 = :::::V1::v1
		movq		mm0,mm7						; mm0 = :::u1::u2::u3
		punpcklwd	mm0,mm6						; mm0 = :U2::u2::U3::u3
		punpckhwd	mm7,mm6						; mm7 = :::::U1::u1

		punpckldq	mm7,mm5						; mm7 = :V1::v1::U1::u1
		movq		mm5,mm0						; mm5 = :U2::u2::U3::u3
		punpckldq	mm5,mm2						; mm5 = :V3::v3::U3::u3
		punpckhdq	mm0,mm2						; mm0 = :V2::v2::U2::u2
		paddw		mm0,mm5
		paddw		mm0,mm7						; mm0 = :V::v::U::u
		psllw		mm0,$1
		movd		[ecx+8*SIZE_DCT_DATA],mm0
		punpckhdq	mm0,mm0						; mm0 = :V::v::V::v
		movd		[ecx],mm0


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
alphamask:			dd 0xff000000,0xff000000
add1024:			dw 0x0400,0x0400,0x0400,0x0400
BLUEMASK:			dd 0x000000ff,0x000000ff
SIZEX625:			dd 0x00000004
SIZEY625:			dd 0x00000008
IMAGE_STRIDE_RGB32:	dd	RGB32_IMAGE_STRIDE
IMAGE_STRIDE_RGB16:	dd	RGB16_IMAGE_STRIDE
HIGH5MASK:			dw 0x00f8,0x00f8,0x00f8,0x00f8
HIGH6MASK:			dw 0x00fc,0x00fc,0x00fc,0x00fc
mmxVFromRGB:		dw 0xf599,0xca69,0x4000,0x0000
mmxUFromRGB:		dw 0x4000,0xd59a,0xea68,0x0000
mmxYFromR:			dw 0x25f9,0x25f9,0x25f9,0x25f9
mmxYFromG:			dw 0x4a8d,0x4a8d,0x4a8d,0x4a8d
mmxYFromB:			dw 0x0e7a,0x0e7a,0x0e7a,0x0e7a
