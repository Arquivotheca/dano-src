; getbits.asm
; fraunhofer mpeg-audio decoder
; bit unpacker

EXTERN printf
EXTERN CBitStream__overflow

pmmtext:		db "mm%i = [ %8x, %8x ] ", 10, 0
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

_64_minus_index:
dd 64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,
dd 48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,
dd 32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,
dd 16,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1,

SECTION .text

; initialize the mm0 buffer with data starting at the given bit offset
; void CBitStream__init_asm(
;       int32* i_bit_count, uint8** io_buf_ptr);
;              4                    8

GLOBAL CBitStream__init_asm
ALIGN 16
CBitStream__init_asm:

	; advance buf_ptr
	mov		ecx, [esp +8]
	mov		edx, [ecx]
	add		edx, 8
	mov		[ecx], edx

	; load 8 bytes	
	mov		ecx, [edx -2 *4]
	mov		edx, [edx -1 *4]	
	bswap	edx
	bswap	ecx
	movd	mm0, ecx
	psllq	mm0, 32
	movd	mm1, edx
	por		mm0, mm1

	; skip (64-i_bit_count) bits if necessary
	mov		eax, [esp +4]
	cmp		eax, 64
	jne		.skip
	ret

.skip:
	movd	mm1, [_64_minus_index +eax *4]
	psllq	mm0, mm1
	ret

; int32 CBitStream__getbits_asm(
;       int32 count, int32* io_bit_count, uint8** io_buf_ptr, uint8* buf_end_ptr, int32 buf_size);
;             4             8                     12                 16                 20
GLOBAL CBitStream__getbits_asm
ALIGN 16
CBitStream__getbits_asm:

	mov		edx, [esp +8]
	mov		eax, [edx]
	mov		ecx, [esp +4]

	sub		eax, ecx
	jl		.new64bit
	
	movd	mm3, [_64_minus_index +ecx *4]
	movq	mm2, mm0
	movd	mm1, ecx
	psrlq	mm2, mm3

	mov		ecx, [esp +8]
	mov		[ecx], al
	
	movd	eax, mm2
	psllq	mm0, mm1
	
	ret
	
.new64bit:
	movd	mm3, [_64_minus_index +ecx *4]
	movq	mm2, mm0
	
	mov		ecx, [esp +12]
	mov		edx, [ecx]
	
	add		edx, 8
	add		eax, 64
	
	mov		[ecx], edx
	mov		ecx, [esp +16]
	cmp		edx, ecx

	; boundary check
	jg		.wrap
	
.refill:
	
	mov		ecx, [edx -2 *4]	; hi
	mov		edx, [edx -1 *4]	; lo
	
	bswap	edx
	bswap	ecx
	
	movd	mm4, ecx
	psrlq	mm2, mm3
	
	movd	mm1, edx
	psllq	mm4, 32
	
	movd	mm3, eax
	por		mm4, mm1
	
	movq	mm0, mm4
	psrlq	mm4, mm3
	
	mov		ecx, [esp +8]
	mov		[ecx], al
	por		mm2, mm4

	movd	mm1, [_64_minus_index +eax *4]
	
	movd	eax, mm2
	psllq	mm0, mm1

	ret
	
.wrap:

;;	; call hook
;;	pusha
;;	sub		esp, 8
;;	mov		[esp], edx
;;	mov		[esp+4], ecx
;;	call	CBitStream__overflow
;;	add		esp, 8
;;	popa

	; wrap buf_ptr & store
	sub		edx, [esp +20]
	mov		ecx, [esp +12]
	mov		[ecx], edx
	
	jmp		.refill


; void CBitStream__reset_asm()
GLOBAL CBitStream__reset_asm
ALIGN 16
CBitStream__reset_asm:
	emms
	ret
