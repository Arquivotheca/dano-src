; 31 by 15-bit multiply (MMX)
; em 22jun00

pmmtext:		db "mm%i = [ %8x, %8x ] ", 10, 0
EXTERN printf
%macro PRINT_MMX_REG 1
	pusha
	sub esp, 40
	mov eax, pmmtext
	mov [esp],eax
	mov [esp+4], dword %1
	movq [esp+8], mm%1
	call printf
	add esp, 40
	popa
%endmacro

SECTION .data

GLOBAL mx31x16_sign_mask
ALIGN 16
mx31x16_sign_mask:		db 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80

SECTION .text

; mx31x16_single
;
; a31 references a single 15.15 fixed-point value
; b15 references a single 0.15 fixed-point value
;
; void mx31x16_single(int32* a31, int16* b15)
;                            4           8

%define STK 4

GLOBAL mx31x16_single
ALIGN 16
mx31x16_single:

	push ebx
	
	mov eax, [esp +4  +STK]
	mov ebx, [esp +8  +STK]
	
	movd mm6, [eax]

	; load sign bit
	movq mm7, mm6
	pand mm7, [mx31x16_sign_mask]

	; load factor
	movd mm4, [ebx]
	movq mm5, mm4
	pslld mm5, 16	
	
	; lo
	movq mm0, mm6
	psrlw mm0, 1
	; restore sign bit
	por mm0, mm7
	pmaddwd mm0, mm4
	psrad mm0, 15	
	
	; hi
	movq mm1, mm6
	pmaddwd mm1, mm5
	
	; compose
	paddd mm0, mm1
	
	; correct
	pslld mm0, 1
	
	movd [eax], mm0

	pop ebx
	emms
	ret

; mx31x16_dual
; void mx31x16_dual(int32* a31, int16* bl16, int16* bh16)
;                          4           8            12

%define STK 4

GLOBAL mx31x16_dual
ALIGN 16
mx31x16_dual:

	push ebx
	
	mov eax, [esp +4  +STK]
	mov ebx, [esp +8  +STK]
	mov ecx, [esp +12 +STK]
	
	movq mm6, [eax]
	
	movq mm0, mm6
	psrlw mm0, 1
	pmaddwd mm0, [ebx]
	psrad mm0, 15	
	
	movq mm1, mm6
	pmaddwd mm1, [ecx]
	paddd mm0, mm1
	pslld mm0, 1
	
	movq [eax], mm0

	pop ebx
	emms
	ret

	