%if __PROCESSOR_KATMAI__==1

global downmix_5_plus_1
global multiply_complex
global interleaved_mult_imre
global interleaved_mult_reim

		section .data

lfe_gain		dd 0.707,0.707,0.707,0.707
mask1			dd 0xffffffff,0,0xffffffff,0
mask2			dd 0,0xffffffff,0,0xffffffff

		section .text

; void downmix_5_plus_1 (int16 *output, float *channel0,
;								const float *center, const float *surround, const float *c);

downmix_5_plus_1:
		push esi
		push edi
		
		mov esi,[esp+20];
		movss xmm4,[esi]
		shufps xmm4,xmm4,byte 0	; xmm4=center|center|center|center

		mov esi,[esp+24];
		movss xmm5,[esi]
		shufps xmm5,xmm5,byte 0	; xmm5=surround|surround|surround|surround

		mov esi,[esp+28];
		movss xmm6,[esi]
		shufps xmm6,xmm6,byte 0	; xmm6=c|c|c|c
		
		mov edi,[esp+12]		; edi=output
		mov esi,[esp+16]		; esi=channel0
		mov ecx,256/4			; ecx=frames

		movups xmm3,[lfe_gain]			; xmm3=lfe_gain		

downmix_loop:
		movaps xmm0,[esi]
		movaps xmm1,[esi+512*4]
		mulps xmm1,xmm4
		addps xmm0,xmm1
		movaps xmm1,[esi+512*4*3]
		mulps xmm1,xmm5
		addps xmm0,xmm1
		movaps xmm1,[esi+512*4*5]
		mulps xmm1,xmm3
		addps xmm0,xmm1
		mulps xmm0,xmm6

		movaps xmm2,[esi+512*4*2]
		movaps xmm1,[esi+512*4]
		mulps xmm1,xmm4
		addps xmm2,xmm1
		movaps xmm1,[esi+512*4*4]
		mulps xmm1,xmm5
		addps xmm2,xmm1
		movaps xmm1,[esi+512*4*5]
		mulps xmm1,xmm3
		addps xmm2,xmm1
		mulps xmm2,xmm6

		cvttps2pi mm0,xmm0			; mm0=i2 i0
		shufps xmm0,xmm0,byte 0x4e 	; 01001110	; xmm0=f2 f0 f6 f4		
		cvttps2pi mm1,xmm0			; mm1=i6 i4

		cvttps2pi mm2,xmm2			; mm2=i3 i1
		packssdw mm0,mm2			; mm0=i3i1i2i0
		pshufw mm0,mm0,byte 0xd8	; 11011000	; mm0=i3i2i1i0
		
		shufps xmm2,xmm2,byte 0x4e 	; 01001110	; xmm2=f3 f1 f7 f5		
		cvttps2pi mm2,xmm2			; mm2=i7 i5
		packssdw mm1,mm2			; mm1=i7i5i6i4
		pshufw mm1,mm1,byte 0xd8	; 11011000	; mm1=i7i6i5i4

		movq [edi],mm1
		movq [edi+8],mm0
		
		add esi,16
		add edi,16

		dec ecx
		jz downmix_done
		jmp downmix_loop
				
downmix_done:
		pop edi
		pop esi
		
		emms				
		ret

;	void multiply_complex (complex_t *dst, const complex_t *src, int32 count_by_2);

multiply_complex:

		push esi
		push edi

		mov edi,[esp+12]
		mov esi,[esp+16]
		mov ecx,[esp+20]
		
		movups xmm3,[mask1]
		movups xmm4,[mask2]

multiply_loop:						
		movaps xmm0,[edi]						;	xmm0=i1r1i0r0
		movaps xmm2,xmm0						;	xmm2=i1r1i0r0
		movaps xmm1,[esi]						;	xmm1=I1R1I0R0
		mulps xmm0,xmm1							;	xmm0=i1*I1 | r1*R1 | i0*I0 | r0*R0
		shufps xmm2,xmm2,byte 0xb1				; 	xmm2=r1i1r0i0				
		mulps xmm1,xmm2							;	xmm1=r1*I1 | i1*R1 | r0*I0 | i0*R0
		
		movaps xmm2,xmm0						;	xmm2=i1*I1 | r1*R1 | i0*I0 | r0*R0		
		shufps xmm0,xmm0,byte 0xb1				; 	xmm0=r1*R1 | i1*I1 | r0*R0 | i0*I0
		subps xmm2,xmm0							;	xmm2=xxx | r1*R1-i1*I1 | xxx | r0*R0-i0*I0 
		andps xmm2,xmm4							;   xmm2=xxx | r1*R1-i1*I1 | xxx | r0*R0-i0*I0 

		movaps xmm0,xmm1						;	xmm0=r1*I1 | i1*R1 | r0*I0 | i0*R0		
		shufps xmm1,xmm1,byte 0xb1				;	xmm1=i1*R1 | r1*I1 | i0*R0 | r0*I0
		addps xmm0,xmm1							;	xmm0=r1*I1+i1*R1 | xxx | r0*I0+i0*R0 | xxx
		andps xmm0,xmm3							;	xmm0=r1*I1+i1*R1 | 0 | r0*I0+i0*R0 | 0
		orps xmm0,xmm2							;	xmm0=r1*I1+i1*R1 | r1*R1-i1*I1 | r0*I0+i0*R0 | r0*R0-i0*I0
		movaps [edi],xmm0

		add esi,16
		add edi,16
		
		dec ecx
		jnz multiply_loop
		
		pop edi
		pop esi		

		emms
		ret

; void interleaved_mult_imre (float *dst,
;								const complex_t *src1,
;								const complex_t *src2,
;								const float *w,
;								int32 count_by_2);
								
interleaved_mult_imre:
		push esi
		push edi
		push ebx
		push ebp

		mov edi,[esp+20]
		mov esi,[esp+24]
		mov ebp,[esp+28]
		mov ebx,[esp+32]
		mov ecx,[esp+36]
				
interleaved_mult_imre_loop:
		xorps xmm0,xmm0
		subps xmm0,[esi]						;	xmm0=-i1|-r1|-i0|-r0
		movaps xmm1,[ebx]						;	xmm1= w3| w2| w1| w0
		movaps xmm2,[ebp]						;	xmm2= I1| R1| I0| R0

		shufps xmm0,xmm0,byte 11011000b			;	xmm0=-i1|-i0|-r1|-r0
		shufps xmm2,xmm2,byte 11011000b			;	xmm2= I1| I0| R1| R0

		shufps xmm0,xmm2,byte 01001110b			;	xmm0= R1| R0|-i1|-i0
		shufps xmm0,xmm0,byte 10011100b			;	xmm0= R0|-i1| R1|-i0
		mulps xmm0,xmm1							;	xmm0=R0*w3|-i1*w2|R1*w1|-i0*w0
		movaps [edi],xmm0
		
		add esi,16
		add ebx,16
		sub ebp,16
		add edi,16
		
		dec ecx
		jnz interleaved_mult_imre_loop
		
		pop ebp
		pop ebx
		pop edi
		pop esi
		ret

; void interleaved_mult_reim (float *dst,
;								const complex_t *src1,
;								const complex_t *src2,
;								const float *w,
;								int32 count_by_2);
								
interleaved_mult_reim:
		push esi
		push edi
		push ebx
		push ebp

		mov edi,[esp+20]
		mov esi,[esp+24]
		mov ebp,[esp+28]
		mov ebx,[esp+32]
		mov ecx,[esp+36]
				
interleaved_mult_reim_loop:
		xorps xmm0,xmm0
		subps xmm0,[esi]						;	xmm0=-i1|-r1|-i0|-r0
		movaps xmm1,[ebx]						;	xmm1= w3| w2| w1| w0
		movaps xmm2,[ebp]						;	xmm2= I1| R1| I0| R0

		shufps xmm0,xmm0,byte 11011000b			;	xmm0=-i1|-i0|-r1|-r0
		shufps xmm2,xmm2,byte 11011000b			;	xmm2= I1| I0| R1| R0

		shufps xmm0,xmm2,byte 11100100b			;	xmm0= I1| I0|-r1|-r0
		shufps xmm0,xmm0,byte 10011100b			;	xmm0= I0|-r1| I1|-r0
		mulps xmm0,xmm1							;	xmm0=I0*w3|-r1*w2|I1*w1|-r0*w0
		movaps [edi],xmm0
		
		add esi,16
		add ebx,16
		sub ebp,16
		add edi,16
		
		dec ecx
		jnz interleaved_mult_reim_loop
		
		pop ebp
		pop ebx
		pop edi
		pop esi
		ret

%endif
