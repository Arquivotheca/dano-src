%if _PROCESSOR_MMX_==1

			global yuv2YCbCr422

			section .text

yuv2YCbCr422_inner_private:

%if _PROCESSOR_CMOV_==1 ; // this should be something like _PROCESSOR_PREFETCH_ instead
			prefetchnta [ebx+32]
%endif

			movd mm1,[ecx]			; mm1==00 00 00 00 u3 u2 u1 u0
			punpcklbw mm1,[edx]		; mm1==v3 u3 v2 u2 v1 u1 v0 u0

			movq mm0,[ebx]			; mm0==y7 y6 y5 y4 y3 y2 y1 y0
			movq mm2,mm0
			punpckhbw mm0,mm1		; mm0==v3 y7 u3 y6 v2 y5 u2 y4
			punpcklbw mm2,mm1		; mm2==v1 y3 u1 y2 v0 y1 u0 y0

			add ecx,4
			add edx,4												
			add ebx,8
			
%if _PROCESSOR_CMOV_==1 ; // this should be something like _PROCESSOR_MOVNT_ instead
			movntq [edi],mm2
			movntq [edi+8],mm0
%else
			movq [edi],mm2
			movq [edi+8],mm0
%endif
			add edi,16

			dec eax
			jnz yuv2YCbCr422_inner_private
			ret

; void yuv2YCbCr422asm (uint8 *dst, const uint8 *y, const uint8 *u, const uint8 *v,
;						int32 width_by_8, int32 height_by_2);
						
yuv2YCbCr422:
			push ebx
			push edi
			push esi
			push ebp
						
			mov edi,[esp+20]	; edi=dst
			mov ebx,[esp+24]	; ebx=y
			mov ecx,[esp+28]	; ecx=u
			mov edx,[esp+32]	; edx=v
			mov eax,[esp+36]	; eax=width by 8
			
			mov esi,[esp+40]	; esi=height by 2
			mov ebp,eax
			
yuv2YCbCr422_line:
			push ecx
			push edx
			call yuv2YCbCr422_inner_private
			mov eax,ebp
			pop edx
			pop ecx
			
			call yuv2YCbCr422_inner_private
			mov eax,ebp
			
			dec esi
			jnz yuv2YCbCr422_line

			pop ebp
			pop esi
			pop edi
			pop ebx
			
			emms
			ret

%endif
