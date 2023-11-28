; Fraunhofer MP3 decoder
; subband antialiasing algorithm

; void antialias_sb_int_m_asm(
;4	int32 *sb1,
;8	int32 *sb2);
; void antialias_sb_int_s_asm(
;4	int32 *sb1,
;8	int32 *sb2);

%include "mx31x16.mac"

pmmtext:		db "mm%i = [ %8x, %8x ] ", 10, 0

EXTERN debugger
EXTERN printf
EXTERN dump_dword

%macro DUMP_MMX_LO 1
	pusha
	sub esp, 40
	movd [esp], mm%1
	call dump_dword
	add esp, 40
	popa
%endmacro

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

SECTION .data ;progbits alloc noexec write align=16

; -------------------------------------------------------------------------

but_cs_lo:
dw	0x6dc2, 0, 0x6dc2, 0, 0x70dc, 0, 0x70dc, 0, 0x798d, 0, 0x798d, 0, 0x7ddd, 0, 0x7ddd, 0, 
dw	0x7f6d, 0, 0x7f6d, 0, 0x7fe4, 0, 0x7fe4, 0, 0x7ffc, 0, 0x7ffc, 0, 0x7fff, 0, 0x7fff, 0, 
but_cs_hi:
dw	0, 0x6dc2, 0, 0x6dc2, 0, 0x70dc, 0, 0x70dc, 0, 0x798d, 0, 0x798d, 0, 0x7ddd, 0, 0x7ddd,
dw	0, 0x7f6d, 0, 0x7f6d, 0, 0x7fe4, 0, 0x7fe4, 0, 0x7ffc, 0, 0x7ffc, 0, 0x7fff, 0, 0x7fff,

but_ca_lo:
dw	0xbe26, 0, 0xbe26, 0, 0xc39f, 0, 0xc39f, 0, 0xd7e4, 0, 0xd7e4, 0, 0xe8b8, 0, 0xe8b8, 0, 
dw	0xf3e5, 0, 0xf3e5, 0, 0xfac2, 0, 0xfac2, 0, 0xfe2f, 0, 0xfe2f, 0, 0xff87, 0, 0xff87, 0, 
but_ca_hi:
dw	0, 0xbe26, 0, 0xbe26, 0, 0xc39f, 0, 0xc39f, 0, 0xd7e4, 0, 0xd7e4, 0, 0xe8b8, 0, 0xe8b8,
dw	0, 0xf3e5, 0, 0xf3e5, 0, 0xfac2, 0, 0xfac2, 0, 0xfe2f, 0, 0xfe2f, 0, 0xff87, 0, 0xff87, 

sign_mask:		db 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80

SECTION .text

; -------------------------------------------------------------------------
; void antialias_sb_int_m_asm(
;4	int32 *sb1,
;8	int32 *sb2);

struc antialias_stack
.begin
;.tmp0	resq 1
endstruc

%macro aa_block_m	3
; %1 sb1Index
; %2 sb2Index
; %3 coefIndex

	;; +++++ optimize:
	;; - schedule multiply pairs

	movd	mm0, [ebx +%1 *8]
	movq	mm4, mm0
	movq	mm1, mm0
	pand	mm1, [sign_mask]			; sign1
	movq	mm2, mm0
	psrlw	mm2, 1
	por		mm2, mm1
	pmaddwd	mm2, [but_cs_lo +%3 *8]
	psrad	mm2, 15
	pmaddwd	mm0, [but_cs_hi +%3 *8]
	paddd	mm0, mm2					; res1a
	movd	mm2, [ecx +%2 *8]
	movq	mm5, mm2
	movq	mm3, mm2
	pand	mm3, [sign_mask]			; sign2
	movq	mm6, mm2
	psrlw	mm6, 1
	por		mm6, mm3
	pmaddwd	mm6, [but_ca_lo +%3 *8]
	psrad	mm6, 15
	pmaddwd	mm2, [but_ca_hi +%3 *8]
	paddd	mm2, mm6					; res2a
	psubd	mm0, mm2
	pslld	mm0, 1
	movd	[ebx +%1 *8], mm0

	movq	mm0, mm4
	psrlw	mm0, 1
	por		mm0, mm1
	pmaddwd	mm0, [but_ca_lo +%3 *8]
	psrad	mm0, 15
	pmaddwd	mm4, [but_ca_hi +%3 *8]
	paddd	mm0, mm4					; res1b
	movq	mm1, mm5
	psrlw	mm1, 1
	por		mm1, mm3
	pmaddwd	mm1, [but_cs_lo +%3 *8]
	psrad	mm1, 15
	pmaddwd	mm5, [but_cs_hi +%3 *8]
	paddd	mm1, mm5					; res2b
	paddd	mm0, mm1
	pslld	mm0, 1
	movd	[ecx +%2 *8], mm0

%endmacro

GLOBAL antialias_sb_int_m_asm
ALIGN 16
antialias_sb_int_m_asm:

;;	sub		esp, antialias_stack_size
	
	mov		ebx, [esp +4 +antialias_stack_size]		; sb1
	mov		ecx, [esp +8 +antialias_stack_size]		; sb2
	
	; ibby butterfly!

	aa_block_m	17, 0, 0
	aa_block_m	16, 1, 1
	aa_block_m	15, 2, 2
	aa_block_m	14, 3, 3
	aa_block_m	13, 4, 4
	aa_block_m	12, 5, 5
	aa_block_m	11, 6, 6
	aa_block_m	10, 7, 7

;;	add		esp, antialias_stack_size
	emms
	ret

; -------------------------------------------------------------------------
; void antialias_sb_int_s_asm(
;4	int32 *sb1,
;8	int32 *sb2);

%macro aa_block_s	3
; %1 sb1Index
; %2 sb2Index
; %3 coefIndex

	;; +++++ optimize:
	;; - schedule multiply pairs

	movq	mm0, [ebx +%1 *8]
	movq	mm4, mm0
	movq	mm1, mm0
	pand	mm1, [sign_mask]			; sign1
	movq	mm2, mm0
	psrlw	mm2, 1
	por		mm2, mm1
	pmaddwd	mm2, [but_cs_lo +%3 *8]
	psrad	mm2, 15
	pmaddwd	mm0, [but_cs_hi +%3 *8]
	paddd	mm0, mm2					; res1a
	movq	mm2, [ecx +%2 *8]
	movq	mm5, mm2
	movq	mm3, mm2
	pand	mm3, [sign_mask]			; sign2
	movq	mm6, mm2
	psrlw	mm6, 1
	por		mm6, mm3
	pmaddwd	mm6, [but_ca_lo +%3 *8]
	psrad	mm6, 15
	pmaddwd	mm2, [but_ca_hi +%3 *8]
	paddd	mm2, mm6					; res2a
	psubd	mm0, mm2
	pslld	mm0, 1
	movq	[ebx +%1 *8], mm0

	movq	mm0, mm4
	psrlw	mm0, 1
	por		mm0, mm1
	pmaddwd	mm0, [but_ca_lo +%3 *8]
	psrad	mm0, 15
	pmaddwd	mm4, [but_ca_hi +%3 *8]
	paddd	mm0, mm4					; res1b
	movq	mm1, mm5
	psrlw	mm1, 1
	por		mm1, mm3
	pmaddwd	mm1, [but_cs_lo +%3 *8]
	psrad	mm1, 15
	pmaddwd	mm5, [but_cs_hi +%3 *8]
	paddd	mm1, mm5					; res2b
	paddd	mm0, mm1
	pslld	mm0, 1
	movq	[ecx +%2 *8], mm0

%endmacro

GLOBAL antialias_sb_int_s_asm
ALIGN 16
antialias_sb_int_s_asm:

;;	sub		esp, antialias_stack_size
	
	mov		ebx, [esp +4 +antialias_stack_size]		; sb1
	mov		ecx, [esp +8 +antialias_stack_size]		; sb2
	
	; ibby butterfly!

	aa_block_s	17, 0, 0
	aa_block_s	16, 1, 1
	aa_block_s	15, 2, 2
	aa_block_s	14, 3, 3
	aa_block_s	13, 4, 4
	aa_block_s	12, 5, 5
	aa_block_s	11, 6, 6
	aa_block_s	10, 7, 7

;;	add		esp, antialias_stack_size
	emms
	ret
