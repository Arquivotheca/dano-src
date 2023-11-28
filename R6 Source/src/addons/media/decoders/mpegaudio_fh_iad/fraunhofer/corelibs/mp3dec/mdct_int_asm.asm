; Fraunhofer MP3 decoder
; MDCT algorithm

; void CMDCTInt__cost12_h_m_asm(
;	const int32 *vec,
;	int32 *f_vec,
;	const int16* win);
; void CMDCTInt__cost12_h_s_asm(
;	const int32 *vec,
;	int32 *f_vec,
;	const int16* winL,
;	const int16* winR);

; void CMDCTInt__cost_long_combine_m_asm(
;	int32 *vec);
; void CMDCTInt__cost_long_combine_s_asm(
;	int32 *vec);
; void CMDCTInt__cost9_m_asm(
;	const int32 *vec,
;	int32 *f_vec);
; void CMDCTInt__cost9_s_asm(
;	const int32 *vec,
;	int32 *f_vec);
; void CMDCTInt__cost9_oddscale_m_asm(
;	int32 *vec);
; void CMDCTInt__cost9_oddscale_s_asm(
;	int32 *vec);
; void CMDCTInt__overlap_hybrid18_m_asm(
;	int32 *prev,
;	int32 *dest,
;	const int32 *cost36_rese,
;	const int32 *cost36_reso,
;	const int16* win);
; void CMDCTInt__overlap_hybrid18_s_asm(
;	int32 *prev,
;	int32 *dest,
;	const int32 *cost36_rese,
;	const int32 *cost36_reso,
;	const int16* winL,
;	const int16* winR);

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
;;; COEFFICIENT TABLES
;;; +++++
;;; to do: consolidate lo/hi into interleaved tables: (v 0 0 v v 0 0 v)

; -------------------------------------------------------------------------
cost9_c_lo:
dw	0x7fff, 0, 0x7fff, 0, 0x7e0e, 0, 0x7e0e, 0, 0x7847, 0, 0x7847, 0
dw	0x6ed9, 0, 0x6ed9, 0, 0x620d, 0, 0x620d, 0, 0x5246, 0, 0x5246, 0
dw	0x4000, 0, 0x4000, 0, 0x2bc7, 0, 0x2bc7, 0, 0x163a, 0, 0x163a, 0
cost9_c_hi:
dw	0, 0x7fff, 0, 0x7fff, 0, 0x7e0e, 0, 0x7e0e, 0, 0x7847, 0, 0x7847
dw	0, 0x6ed9, 0, 0x6ed9, 0, 0x620d, 0, 0x620d, 0, 0x5246, 0, 0x5246
dw	0, 0x4000, 0, 0x4000, 0, 0x2bc7, 0, 0x2bc7, 0, 0x163a, 0, 0x163a

; -------------------------------------------------------------------------
; >> 1
cost12_c1_lo:
dw	0x7ba3, 0, 0x7ba3, 0, 0x5a82, 0, 0x5a82, 0, 0x2120, 0, 0x2120, 0
cost12_c1_hi:
dw	0, 0x7ba3, 0, 0x7ba3, 0, 0x5a82, 0, 0x5a82, 0, 0x2120, 0, 0x2120

; -------------------------------------------------------------------------
; >> 1
cost12_c2_lo:
dw	0x6ed9, 0, 0x6ed9, 0
cost12_c2_hi:
dw	0, 0x6ed9, 0, 0x6ed9

; -------------------------------------------------------------------------
; >> 1
cost36_c1_lo:
dw	0x7f83, 0, 0x7f83, 0, 0x7ba3, 0, 0x7ba3, 0, 0x7401, 0, 0x7401, 0
dw	0x68d9, 0, 0x68d9, 0, 0x5a82, 0, 0x5a82, 0, 0x496a, 0, 0x496a, 0
dw	0x3618, 0, 0x3618, 0, 0x2120, 0, 0x2120, 0, 0x0b27, 0, 0x0b27, 0
cost36_c1_hi:
dw	0, 0x7f83, 0, 0x7f83, 0, 0x7ba3, 0, 0x7ba3, 0, 0x7401, 0, 0x7401
dw	0, 0x68d9, 0, 0x68d9, 0, 0x5a82, 0, 0x5a82, 0, 0x496a, 0, 0x496a
dw	0, 0x3618, 0, 0x3618, 0, 0x2120, 0, 0x2120, 0, 0x0b27, 0, 0x0b27

; -------------------------------------------------------------------------
; used for 31x16 multiply
;
sign_mask:		db 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80

SECTION .text

; -------------------------------------------------------------------------
; void CMDCTInt__cost12_h_m(
;4	const int32 *vec,
;8	int32 *f_vec,
;12	const int16* win);

struc cost12_stack
.begin
.v0		resq 1
.v1		resq 1
.v2		resq 1
.v3		resq 1
.v4		resq 1
.v5		resq 1
.re0	resq 1
.re1	resq 1
.re2	resq 1
.ro0	resq 1
.ro1	resq 1
.ro2	resq 1
endstruc

GLOBAL CMDCTInt__cost12_h_m_asm
ALIGN 16
CMDCTInt__cost12_h_m_asm:

	push	esi
	sub		esp, cost12_stack_size

	mov		esi, [esp +4 +4 +cost12_stack_size]		; vec
	mov		ecx, [esp +12+4 +cost12_stack_size]		; win
	
	; stage 1
	movd	mm5, [esi +15*8]				; v5
	movd	[esp +cost12_stack.v5], mm5
	movd	mm1, [esi +12*8]
	psubd	mm1, mm5
	movd	[esp +cost12_stack.v4], mm1		; v4
	movd	mm3, [esi +9 *8]
	psubd	mm3, mm1
;;	movd	[esp +cost12_stack.v3], mm3		; *v3
	movd	mm0, [esi +6 *8]
	psubd	mm0, mm3
	movd	[esp +cost12_stack.v2], mm0		; v2
	movd	mm1, [esi +3 *8]
	psubd	mm1, mm0
;;	movd	[esp +cost12_stack.v1], mm1		; *v1
	movd	mm0, [esi +0 *8]
	psubd	mm0, mm1
	movd	[esp +cost12_stack.v0], mm0		; v0

	; stage 2
	psubd	mm3, mm5
	movd	[esp +cost12_stack.v3], mm3
	psubd	mm1, mm3
	movd	[esp +cost12_stack.v1], mm1
	
	movd	mm0, [esp +cost12_stack.v3]
	mx31x16	0, 1, 2, cost12_c2, 0, 1
	movd	[esp +cost12_stack.v3], mm0
	
	movd	mm0, [esp +cost12_stack.v2]
	mx31x16	0, 1, 2, cost12_c2, 0, 1
	movd	[esp +cost12_stack.v2], mm0

	; re0
	movd	mm3, [esp +cost12_stack.v0]
	movd	mm2, [esp +cost12_stack.v4]
	movq	mm1, mm2
	paddd	mm1, mm3
	paddd	mm1, mm0						; v2
	movd	[esp +cost12_stack.re0], mm1
	; re2
	movq	mm1, mm3						; v0
	paddd	mm1, mm2						; v0 + v4
	psubd	mm1, mm0						; (v0 + v4) - v2
	movd	[esp +cost12_stack.re2], mm1
	; re1
	pslld	mm2, 1							; v4 *2
	psubd	mm3, mm2
	movd	[esp +cost12_stack.re1], mm3	; v0 - (v4 * 2)

	; ro0
	movd	mm0, [esp +cost12_stack.v1]
	paddd	mm0, [esp +cost12_stack.v3]
	paddd	mm0, [esp +cost12_stack.v5]
	mx31x16	0, 1, 2, cost12_c1, 0, 1
	movd	[esp +cost12_stack.ro0], mm0
	; ro1
	movd	mm0, [esp +cost12_stack.v1]
	movd	mm1, [esp +cost12_stack.v5]
	pslld	mm1, 1
	psubd	mm0, mm1
	mx31x16	0, 1, 2, cost12_c1, 1, 1
	movd	[esp +cost12_stack.ro1], mm0
	; ro2
	movd	mm0, [esp +cost12_stack.v1]
	paddd	mm0, [esp +cost12_stack.v5]
	psubd	mm0, [esp +cost12_stack.v3]
	mx31x16	0, 1, 2, cost12_c1, 2, 1
	movd	[esp +cost12_stack.ro2], mm0
	; v2/v3
	movd	mm1, [esp +cost12_stack.re2]
	movq	mm2, mm1
	paddd	mm2, mm0						; re2 + ro2
	movd	[esp +cost12_stack.v2], mm2
	psubd	mm1, mm0
	movd	[esp +cost12_stack.v3], mm1		; re2 - ro2
	; v1/v4
	movd	mm0, [esp +cost12_stack.re1]
	movd	mm1, [esp +cost12_stack.ro1]
	movq	mm2, mm0
	paddd	mm2, mm1
	movd	[esp +cost12_stack.v1], mm2		; re1 + ro1
	psubd	mm0, mm1
	movd	[esp +cost12_stack.v4], mm0		; re1 - ro1
	; v0/v5
	movd	mm0, [esp +cost12_stack.re0]
	movd	mm1, [esp +cost12_stack.ro0]
	movq	mm2, mm0
	paddd	mm2, mm1
	movd	[esp +cost12_stack.v0], mm2		; re0 + ro0
	psubd	mm0, mm1
	movd	[esp +cost12_stack.v5], mm0		; re5 - ro5
	
	; windowing
	mov		esi, [esp +8 +4 +cost12_stack_size]		; f_vec
	
	mx31x16_load_cr	0, ecx, 8				; * win[8]
	mx31x16_cr		2, 0, 1, 3, 1			; << 1
	paddd	mm2, [esi +8 *4]
	movd	[esi +8 *4], mm2				; f_vec[8] +=

	movd	mm2, [esp +cost12_stack.v0]
	mx31x16_load_cr	0, ecx, 9
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +9 *4]
	movd	[esi +9 *4], mm2

	movd	mm2, [esp +cost12_stack.v1]
	mx31x16_load_cr	0, ecx, 7
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +7 *4]
	movd	[esi +7 *4], mm2
	
	movd	mm2, [esp +cost12_stack.v1]
	mx31x16_load_cr	0, ecx, 10
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +10*4]
	movd	[esi +10*4], mm2
	
	movd	mm2, [esp +cost12_stack.v2]
	mx31x16_load_cr	0, ecx, 6
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +6 *4]
	movd	[esi +6 *4], mm2
	
	movd	mm2, [esp +cost12_stack.v2]
	mx31x16_load_cr	0, ecx, 11
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +11*4]
	movd	[esi +11*4], mm2

	movd	mm2, [esp +cost12_stack.v3]
	mx31x16_load_cr	0, ecx, 0
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +0 *4]
	movd	[esi +0 *4], mm2
	
	movd	mm2, [esp +cost12_stack.v3]
	mx31x16_load_cr	0, ecx, 5
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +5 *4]
	movd	[esi +5 *4], mm2
	
	movd	mm2, [esp +cost12_stack.v4]
	mx31x16_load_cr	0, ecx, 1
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +1 *4]
	movd	[esi +1 *4], mm2
	
	movd	mm2, [esp +cost12_stack.v4]
	mx31x16_load_cr	0, ecx, 4
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +4 *4]
	movd	[esi +4 *4], mm2
	
	movd	mm2, [esp +cost12_stack.v5]
	mx31x16_load_cr	0, ecx, 2
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +2 *4]
	movd	[esi +2 *4], mm2
	
	movd	mm2, [esp +cost12_stack.v5]
	mx31x16_load_cr	0, ecx, 3
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +3 *4]
	movd	[esi +3 *4], mm2

	add		esp, cost12_stack_size
	pop		esi
	emms
	ret

; -------------------------------------------------------------------------
; void CMDCTInt__cost12_h_s_asm(
;4	const int32 *vec,
;8	int32 *f_vec,
;12	const int16* winL,
;16	const int16* winR);

GLOBAL CMDCTInt__cost12_h_s_asm
ALIGN 16
CMDCTInt__cost12_h_s_asm:

	push	esi
	sub		esp, cost12_stack_size

	mov		esi, [esp +4 +4 +cost12_stack_size]		; vec
	mov		ecx, [esp +12+4 +cost12_stack_size]		; winL
	mov		edx, [esp +16+4 +cost12_stack_size]		; winR
	
	
	; stage 1
	movq	mm5, [esi +15*8]				; v5
	movq	[esp +cost12_stack.v5], mm5
	movq	mm1, [esi +12*8]
	psubd	mm1, mm5
	movq	[esp +cost12_stack.v4], mm1		; v4
	movq	mm3, [esi +9 *8]
	psubd	mm3, mm1
;;	movq	[esp +cost12_stack.v3], mm3		; *v3
	movq	mm0, [esi +6 *8]
	psubd	mm0, mm3
	movq	[esp +cost12_stack.v2], mm0		; v2
	movq	mm1, [esi +3 *8]
	psubd	mm1, mm0
;;	movq	[esp +cost12_stack.v1], mm1		; *v1
	movq	mm0, [esi +0 *8]
	psubd	mm0, mm1
	movq	[esp +cost12_stack.v0], mm0		; v0

	; stage 2
	psubd	mm3, mm5
	movq	[esp +cost12_stack.v3], mm3
	psubd	mm1, mm3
	movq	[esp +cost12_stack.v1], mm1
	
	movq	mm0, [esp +cost12_stack.v3]
	mx31x16	0, 1, 2, cost12_c2, 0, 1
	movq	[esp +cost12_stack.v3], mm0
	
	movq	mm0, [esp +cost12_stack.v2]
	mx31x16	0, 1, 2, cost12_c2, 0, 1
	movq	[esp +cost12_stack.v2], mm0

	; re0
	movq	mm3, [esp +cost12_stack.v0]
	movq	mm2, [esp +cost12_stack.v4]
	movq	mm1, mm2
	paddd	mm1, mm3
	paddd	mm1, mm0						; v2
	movq	[esp +cost12_stack.re0], mm1
	; re2
	movq	mm1, mm3						; v0
	paddd	mm1, mm2						; v0 + v4
	psubd	mm1, mm0						; (v0 + v4) - v2
	movq	[esp +cost12_stack.re2], mm1
	; re1
	pslld	mm2, 1							; v4 *2
	psubd	mm3, mm2
	movq	[esp +cost12_stack.re1], mm3	; v0 - (v4 * 2)

	; ro0
	movq	mm0, [esp +cost12_stack.v1]
	paddd	mm0, [esp +cost12_stack.v3]
	paddd	mm0, [esp +cost12_stack.v5]
	mx31x16	0, 1, 2, cost12_c1, 0, 1
	movq	[esp +cost12_stack.ro0], mm0
	; ro1
	movq	mm0, [esp +cost12_stack.v1]
	movq	mm1, [esp +cost12_stack.v5]
	pslld	mm1, 1
	psubd	mm0, mm1
	mx31x16	0, 1, 2, cost12_c1, 1, 1
	movq	[esp +cost12_stack.ro1], mm0
	; ro2
	movq	mm0, [esp +cost12_stack.v1]
	paddd	mm0, [esp +cost12_stack.v5]
	psubd	mm0, [esp +cost12_stack.v3]
	mx31x16	0, 1, 2, cost12_c1, 2, 1
	movq	[esp +cost12_stack.ro2], mm0
	; v2/v3
	movq	mm1, [esp +cost12_stack.re2]
	movq	mm2, mm1
	paddd	mm2, mm0						; re2 + ro2
	movq	[esp +cost12_stack.v2], mm2
	psubd	mm1, mm0
	movq	[esp +cost12_stack.v3], mm1		; re2 - ro2
	; v1/v4
	movq	mm0, [esp +cost12_stack.re1]
	movq	mm1, [esp +cost12_stack.ro1]
	movq	mm2, mm0
	paddd	mm2, mm1
	movq	[esp +cost12_stack.v1], mm2		; re1 + ro1
	psubd	mm0, mm1
	movq	[esp +cost12_stack.v4], mm0		; re1 - ro1
	; v0/v5
	movq	mm0, [esp +cost12_stack.re0]
	movq	mm1, [esp +cost12_stack.ro0]
	movq	mm2, mm0
	paddd	mm2, mm1
	movq	[esp +cost12_stack.v0], mm2		; re0 + ro0
	psubd	mm0, mm1
	movq	[esp +cost12_stack.v5], mm0		; re5 - ro5
	
	; windowing
	mov		esi, [esp +8 +4 +cost12_stack_size]		; f_vec
	
	mx31x16_load_cr2	0, 1, ecx, edx, 8	; * win[8]
	mx31x16_cr			2, 0, 1, 3, 1		; << 1
	paddd	mm2, [esi +8 *8]
	movq	[esi +8 *8], mm2				; f_vec[8] +=

	movq	mm2, [esp +cost12_stack.v0]
	mx31x16_load_cr2	0, 1, ecx, edx, 9
	mx31x16_cr			2, 0, 1, 3, 1
	paddd	mm2, [esi +9 *8]
	movq	[esi +9 *8], mm2

	movq	mm2, [esp +cost12_stack.v1]
	mx31x16_load_cr2	0, 1, ecx, edx, 7
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +7 *8]
	movq	[esi +7 *8], mm2
	
	movq	mm2, [esp +cost12_stack.v1]
	mx31x16_load_cr2	0, 1, ecx, edx, 10
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +10*8]
	movq	[esi +10*8], mm2
	
	movq	mm2, [esp +cost12_stack.v2]
	mx31x16_load_cr2	0, 1, ecx, edx, 6
	mx31x16_cr		2, 0, 1, 3, 1
	paddd	mm2, [esi +6 *8]
	movq	[esi +6 *8], mm2
	
	movq	mm2, [esp +cost12_stack.v2]
	mx31x16_load_cr2	0, 1, ecx, edx, 11
	mx31x16_cr			2, 0, 1, 3, 1
	paddd	mm2, [esi +11*8]
	movq	[esi +11*8], mm2

	movq	mm2, [esp +cost12_stack.v3]
	mx31x16_load_cr2	0, 1, ecx, edx, 0
	mx31x16_cr			2, 0, 1, 3, 1
	paddd	mm2, [esi +0 *8]
	movq	[esi +0 *8], mm2
	
	movq	mm2, [esp +cost12_stack.v3]
	mx31x16_load_cr2	0, 1, ecx, edx, 5
	mx31x16_cr			2, 0, 1, 3, 1
	paddd	mm2, [esi +5 *8]
	movq	[esi +5 *8], mm2
	
	movq	mm2, [esp +cost12_stack.v4]
	mx31x16_load_cr2	0, 1, ecx, edx, 1
	mx31x16_cr			2, 0, 1, 3, 1
	paddd	mm2, [esi +1 *8]
	movq	[esi +1 *8], mm2
	
	movq	mm2, [esp +cost12_stack.v4]
	mx31x16_load_cr2	0, 1, ecx, edx, 4
	mx31x16_cr			2, 0, 1, 3, 1
	paddd	mm2, [esi +4 *8]
	movq	[esi +4 *8], mm2
	
	movq	mm2, [esp +cost12_stack.v5]
	mx31x16_load_cr2	0, 1, ecx, edx, 2
	mx31x16_cr			2, 0, 1, 3, 1
	paddd	mm2, [esi +2 *8]
	movq	[esi +2 *8], mm2
	
	movq	mm2, [esp +cost12_stack.v5]
	mx31x16_load_cr2	0, 1, ecx, edx, 3
	mx31x16_cr			2, 0, 1, 3, 1
	paddd	mm2, [esi +3 *8]
	movq	[esi +3 *8], mm2

	add		esp, cost12_stack_size
	pop		esi
	emms
	ret

; -------------------------------------------------------------------------
; vec is stereo-interleaved; f_vec is mono
; void CMDCTInt__cost9_m_asm(
;4	const int32 *vec,
;8	int32 *f_vec);
;

struc cost9_stack
.begin
;.t1		resq 1
;.t2		resq 1
;.t3		resq 1
endstruc

GLOBAL CMDCTInt__cost9_m_asm
ALIGN 16
CMDCTInt__cost9_m_asm:

;	no stack vars are in use yet; may be needed w/ scheduling work
;	sub		esp, cost9_stack_size
	
	mov		ecx, [esp +4 +cost9_stack_size]		; vec
	mov		edx, [esp +8 +cost9_stack_size]		; f_vec
	
	movd	mm1, [ecx +4 *8]
	movd	mm2, [ecx +8 *8]
	movd	mm4, [ecx +16*8]
	
	movq	mm5, [ecx +0 *8]
	psubd	mm5, mm1
	paddd	mm5, mm2
	psubd	mm5, [ecx +12*8]
	paddd	mm5, mm4
	movd	[edx +4 *4], mm5			; s[4]
	
	movq	mm5, mm1
	psubd	mm5, mm2
	psubd	mm5, mm4
	mx31x16	5, 0, 3, cost9_c, 6
;;	movd	[esp +cost9_stack.t3], mm5	; >>>tmp3
	movd	mm0, [ecx +0 *8]
	psubd	mm0, [ecx +12*8]
	paddd	mm0, mm5
;;	movd	[esp +cost9_stack.t1], mm0	; >>>tmp1

	movq	mm1, [ecx +2 *8]
	psubd	mm1, [ecx +10*8]
	psubd	mm1, [ecx +14*8]
	mx31x16	1, 2, 3, cost9_c, 3			; >>>tmp2
	
	movq	mm2, mm0
	paddd	mm2, mm1
	movd	[edx +1 *4], mm2			; s[1]
	psubd	mm0, mm1
	movd	[edx +7 *4], mm0			; s[7]
	
	;;; +++++ likely needs improvement scheduling-wise

	movd	mm0, [ecx +0 *8]
	movd	mm1, [ecx +4 *8]
	mx31x16	1, 2, 3, cost9_c, 2
	paddd	mm0, mm1
	movd	mm1, [ecx +8 *8]
	mx31x16	1, 2, 3, cost9_c, 4
	paddd	mm0, mm1
	movd	mm1, [ecx +12*8]
	mx31x16	1, 2, 3, cost9_c, 6
	paddd	mm0, mm1
	movd	mm4, [ecx +16*8]
	mx31x16	4, 2, 3, cost9_c, 8
	paddd	mm4, mm0					; >>>tmp1
	
	movd	mm0, [ecx +2 *8]
	mx31x16	0, 1, 2, cost9_c, 1
	movd	mm1, [ecx +6 *8]
	mx31x16	1, 2, 3, cost9_c, 3
	paddd	mm0, mm1
	movd	mm1, [ecx +10*8]
	mx31x16	1, 2, 3, cost9_c, 5
	paddd	mm0, mm1
	movd	mm5, [ecx +14*8]
	mx31x16	5, 2, 3, cost9_c, 7
	paddd	mm5, mm0					; >>>tmp2
	
	movq	mm0, mm5
	paddd	mm0, mm4
	movd	[edx +0 *4], mm0			; s[0]
	psubd	mm4, mm5
	movd	[edx +8 *4], mm4			; s[8]
	
	movd	mm0, [ecx +0 *0]
	movd	mm1, [ecx +4 *8]
	mx31x16	1, 2, 3, cost9_c, 8
	psubd	mm0, mm1
	movd	mm1, [ecx +8 *8]
	mx31x16	1, 2, 3, cost9_c, 2
	psubd	mm0, mm1
	movd	mm1, [ecx +12*8]
	mx31x16	1, 2, 3, cost9_c, 6
	paddd	mm0, mm1
	movd	mm4, [ecx +16*8]
	mx31x16	4, 2, 3, cost9_c, 4
	paddd	mm4, mm0					; >>>tmp1
	
	movd	mm0, [ecx +2 *8]
	mx31x16	0, 1, 2, cost9_c, 5
	movd	mm1, [ecx +6 *8]
	mx31x16	1, 2, 3, cost9_c, 3
	psubd	mm0, mm1
	movd	mm1, [ecx +10*8]
	mx31x16	1, 2, 3, cost9_c, 7
	psubd	mm0, mm1
	movd	mm5, [ecx +14*8]
	mx31x16	5, 2, 3, cost9_c, 1
	paddd	mm5, mm0					; >>>tmp2
	
	movq	mm0, mm5
	paddd	mm0, mm4
	movd	[edx +2 *4], mm0			; s[2]
	psubd	mm4, mm5
	movd	[edx +6 *4], mm4			; s[6]
	
	movd	mm4, [ecx +0 *8]
	movd	mm1, [ecx +4 *8]
	mx31x16	1, 2, 3, cost9_c, 4
	psubd	mm4, mm1
	movd	mm1, [ecx +8 *8]
	mx31x16	1, 2, 3, cost9_c, 8
	paddd	mm4, mm1
	movd	mm1, [ecx +12*8]
	mx31x16	1, 2, 3, cost9_c, 6
	paddd	mm4, mm1
	movd	mm1, [ecx +16*8]
	mx31x16	1, 2, 3, cost9_c, 2
	psubd	mm4, mm1					; >>>tmp1
	
	movd	mm0, [ecx +2 *8]
	mx31x16	0, 1, 2, cost9_c, 7
	movd	mm1, [ecx +6 *8]
	mx31x16	1, 2, 3, cost9_c, 3
	psubd	mm0, mm1
	movd	mm1, [ecx +10*8]
	mx31x16	1, 2, 3, cost9_c, 1
	paddd	mm0, mm1
	movd	mm1, [ecx +14*8]
	mx31x16	1, 2, 3, cost9_c, 5
	psubd	mm0, mm1					; >>>tmp2
	
	movq	mm1, mm0
	paddd	mm0, mm4
	movd	[edx +3 *4], mm0			; s[3]
	psubd	mm4, mm1
	movd	[edx +5 *4], mm4			; s[5]

;	no stack vars are in use yet; may be needed w/ scheduling work
;	add		esp, cost9_stack_size
	emms
	ret

; -------------------------------------------------------------------------
; vec and f_vec are stereo-interleaved
; void CMDCTInt__cost9_s_asm(
;4	const int32 *vec,
;8	int32 *f_vec);
;

GLOBAL CMDCTInt__cost9_s_asm
ALIGN 16
CMDCTInt__cost9_s_asm:

;	no stack vars are in use yet; may be needed w/ scheduling work
;	sub		esp, cost9_stack_size
	
	mov		ecx, [esp +4 +cost9_stack_size]		; vec
	mov		edx, [esp +8 +cost9_stack_size]		; f_vec
	
	movq	mm1, [ecx +4 *8]
	movq	mm2, [ecx +8 *8]
	movq	mm4, [ecx +16*8]
	
	movq	mm5, [ecx +0 *8]
	psubd	mm5, mm1
	paddd	mm5, mm2
	psubd	mm5, [ecx +12*8]
	paddd	mm5, mm4
	movq	[edx +4 *8], mm5			; s[4]
	
	movq	mm5, mm1
	psubd	mm5, mm2
	psubd	mm5, mm4
	mx31x16	5, 0, 3, cost9_c, 6
;;	movq	[esp +cost9_stack.t3], mm5	; >>>tmp3
	movq	mm0, [ecx +0 *8]
	psubd	mm0, [ecx +12*8]
	paddd	mm0, mm5
;;	movq	[esp +cost9_stack.t1], mm0	; >>>tmp1

	movq	mm1, [ecx +2 *8]
	psubd	mm1, [ecx +10*8]
	psubd	mm1, [ecx +14*8]
	mx31x16	1, 2, 3, cost9_c, 3			; >>>tmp2
	
	movq	mm2, mm0
	paddd	mm2, mm1
	movq	[edx +1 *8], mm2			; s[1]
	psubd	mm0, mm1
	movq	[edx +7 *8], mm0			; s[7]
	
	;;; +++++ likely needs improvement scheduling-wise

	movq	mm0, [ecx +0 *8]
	movq	mm1, [ecx +4 *8]
	mx31x16	1, 2, 3, cost9_c, 2
	paddd	mm0, mm1
	movq	mm1, [ecx +8 *8]
	mx31x16	1, 2, 3, cost9_c, 4
	paddd	mm0, mm1
	movq	mm1, [ecx +12*8]
	mx31x16	1, 2, 3, cost9_c, 6
	paddd	mm0, mm1
	movq	mm4, [ecx +16*8]
	mx31x16	4, 2, 3, cost9_c, 8
	paddd	mm4, mm0					; >>>tmp1
	
	movq	mm0, [ecx +2 *8]
	mx31x16	0, 1, 2, cost9_c, 1
	movq	mm1, [ecx +6 *8]
	mx31x16	1, 2, 3, cost9_c, 3
	paddd	mm0, mm1
	movq	mm1, [ecx +10*8]
	mx31x16	1, 2, 3, cost9_c, 5
	paddd	mm0, mm1
	movq	mm5, [ecx +14*8]
	mx31x16	5, 2, 3, cost9_c, 7
	paddd	mm5, mm0					; >>>tmp2
	
	movq	mm0, mm5
	paddd	mm0, mm4
	movq	[edx +0 *8], mm0			; s[0]
	psubd	mm4, mm5
	movq	[edx +8 *8], mm4			; s[8]
	
	movq	mm0, [ecx +0 *0]
	movq	mm1, [ecx +4 *8]
	mx31x16	1, 2, 3, cost9_c, 8
	psubd	mm0, mm1
	movq	mm1, [ecx +8 *8]
	mx31x16	1, 2, 3, cost9_c, 2
	psubd	mm0, mm1
	movq	mm1, [ecx +12*8]
	mx31x16	1, 2, 3, cost9_c, 6
	paddd	mm0, mm1
	movq	mm4, [ecx +16*8]
	mx31x16	4, 2, 3, cost9_c, 4
	paddd	mm4, mm0					; >>>tmp1
	
	movq	mm0, [ecx +2 *8]
	mx31x16	0, 1, 2, cost9_c, 5
	movq	mm1, [ecx +6 *8]
	mx31x16	1, 2, 3, cost9_c, 3
	psubd	mm0, mm1
	movq	mm1, [ecx +10*8]
	mx31x16	1, 2, 3, cost9_c, 7
	psubd	mm0, mm1
	movq	mm5, [ecx +14*8]
	mx31x16	5, 2, 3, cost9_c, 1
	paddd	mm5, mm0					; >>>tmp2
	
	movq	mm0, mm5
	paddd	mm0, mm4
	movq	[edx +2 *8], mm0			; s[2]
	psubd	mm4, mm5
	movq	[edx +6 *8], mm4			; s[6]
	
	movq	mm4, [ecx +0 *8]
	movq	mm1, [ecx +4 *8]
	mx31x16	1, 2, 3, cost9_c, 4
	psubd	mm4, mm1
	movq	mm1, [ecx +8 *8]
	mx31x16	1, 2, 3, cost9_c, 8
	paddd	mm4, mm1
	movq	mm1, [ecx +12*8]
	mx31x16	1, 2, 3, cost9_c, 6
	paddd	mm4, mm1
	movq	mm1, [ecx +16*8]
	mx31x16	1, 2, 3, cost9_c, 2
	psubd	mm4, mm1					; >>>tmp1
	
	movq	mm0, [ecx +2 *8]
	mx31x16	0, 1, 2, cost9_c, 7
	movq	mm1, [ecx +6 *8]
	mx31x16	1, 2, 3, cost9_c, 3
	psubd	mm0, mm1
	movq	mm1, [ecx +10*8]
	mx31x16	1, 2, 3, cost9_c, 1
	paddd	mm0, mm1
	movq	mm1, [ecx +14*8]
	mx31x16	1, 2, 3, cost9_c, 5
	psubd	mm0, mm1					; >>>tmp2
	
	movq	mm1, mm0
	paddd	mm0, mm4
	movq	[edx +3 *8], mm0			; s[3]
	psubd	mm4, mm1
	movq	[edx +5 *8], mm4			; s[5]

;	no stack vars are in use yet; may be needed w/ scheduling work
;	add		esp, cost9_stack_size
	emms
	ret


; -------------------------------------------------------------------------
; void CMDCTInt__cost_long_combine_m_asm(
;4	int32 *vec);
;

GLOBAL CMDCTInt__cost_long_combine_m_asm
ALIGN 16
CMDCTInt__cost_long_combine_m_asm:

	mov		ecx, [esp +4]				; vec
	
	movd	mm0, [ecx +32*4]
	psubd	mm0, [ecx +34*4]
	movd	mm1, [ecx +30*4]
	psubd	mm1, mm0
	movd	mm2, [ecx +28*4]
	psubd	mm2, mm1
	movd	mm3, [ecx +26*4]
	psubd	mm3, mm2
	movd	mm4, [ecx +24*4]
	psubd	mm4, mm3
	movd	mm5, [ecx +22*4]
	psubd	mm5, mm4
	movd	mm6, [ecx +20*4]
	psubd	mm6, mm5
	movd	[ecx +32*4], mm0
	movd	mm7, [ecx +18*4]
	psubd	mm7, mm6
	movd	[ecx +30*4], mm1
	movd	mm0, [ecx +16*4]
	psubd	mm0, mm7
	movd	[ecx +28*4], mm2
	movd	mm1, [ecx +14*4]
	psubd	mm1, mm0
	movd	[ecx +26*4], mm3
	movd	mm2, [ecx +12*4]
	psubd	mm2, mm1
	movd	[ecx +24*4], mm4
	movd	mm3, [ecx +10*4]
	psubd	mm3, mm2
	movd	[ecx +22*4], mm5
	movd	mm4, [ecx +8 *4]
	psubd	mm4, mm3
	movd	[ecx +20*4], mm6
	movd	mm5, [ecx +6 *4]
	psubd	mm5, mm4
	movd	[ecx +18*4], mm7
	movd	mm6, [ecx +4 *4]
	psubd	mm6, mm5
	movd	[ecx +16*4], mm0
	movd	mm7, [ecx +2 *4]
	psubd	mm7, mm6
	movd	[ecx +14*4], mm1
	movd	mm0, [ecx +0 *4]
	psubd	mm0, mm7
	psrad	mm0, 1
	movd	[ecx +12*4], mm2
	movd	[ecx +10*4], mm3
	movd	[ecx +8 *4], mm4
	movd	[ecx +6 *4], mm5
	movd	[ecx +4 *4], mm6
	movd	[ecx +2 *4], mm7
	movd	[ecx +0 *4], mm0
	
	movd	mm0, [ecx +30*4]
	psubd	mm0, [ecx +34*4]
	movd	mm1, [ecx +26*4]
	psubd	mm1, mm0
	movd	mm2, [ecx +22*4]
	psubd	mm2, mm1
	movd	mm3, [ecx +18*4]
	psubd	mm3, mm2
	movd	mm4, [ecx +14*4]
	psubd	mm4, mm3
	movd	mm5, [ecx +10*4]
	psubd	mm5, mm4
	movd	mm6, [ecx +6 *4]
	psubd	mm6, mm5
	movd	mm7, [ecx +2 *4]
	psubd	mm7, mm6
	psrad	mm7, 1
	movd	[ecx +30*4], mm0
	movd	[ecx +26*4], mm1
	movd	[ecx +22*4], mm2
	movd	[ecx +18*4], mm3
	movd	[ecx +14*4], mm4
	movd	[ecx +10*4], mm5
	movd	[ecx +6 *4], mm6
	movd	[ecx +2 *4], mm7

	emms
	ret

; -------------------------------------------------------------------------
; void CMDCTInt__cost_long_combine_s_asm(
;4	int32 *vec);
;

GLOBAL CMDCTInt__cost_long_combine_s_asm
ALIGN 16
CMDCTInt__cost_long_combine_s_asm:

	mov		ecx, [esp +4]				; vec
	
	movq	mm0, [ecx +32*4]
	psubd	mm0, [ecx +34*4]
	movq	mm1, [ecx +30*4]
	psubd	mm1, mm0
	movq	mm2, [ecx +28*4]
	psubd	mm2, mm1
	movq	mm3, [ecx +26*4]
	psubd	mm3, mm2
	movq	mm4, [ecx +24*4]
	psubd	mm4, mm3
	movq	mm5, [ecx +22*4]
	psubd	mm5, mm4
	movq	mm6, [ecx +20*4]
	psubd	mm6, mm5
	movq	[ecx +32*4], mm0
	movq	mm7, [ecx +18*4]
	psubd	mm7, mm6
	movq	[ecx +30*4], mm1
	movq	mm0, [ecx +16*4]
	psubd	mm0, mm7
	movq	[ecx +28*4], mm2
	movq	mm1, [ecx +14*4]
	psubd	mm1, mm0
	movq	[ecx +26*4], mm3
	movq	mm2, [ecx +12*4]
	psubd	mm2, mm1
	movq	[ecx +24*4], mm4
	movq	mm3, [ecx +10*4]
	psubd	mm3, mm2
	movq	[ecx +22*4], mm5
	movq	mm4, [ecx +8 *4]
	psubd	mm4, mm3
	movq	[ecx +20*4], mm6
	movq	mm5, [ecx +6 *4]
	psubd	mm5, mm4
	movq	[ecx +18*4], mm7
	movq	mm6, [ecx +4 *4]
	psubd	mm6, mm5
	movq	[ecx +16*4], mm0
	movq	mm7, [ecx +2 *4]
	psubd	mm7, mm6
	movq	[ecx +14*4], mm1
	movq	mm0, [ecx +0 *4]
	psubd	mm0, mm7
	psrad	mm0, 1
	movq	[ecx +12*4], mm2
	movq	[ecx +10*4], mm3
	movq	[ecx +8 *4], mm4
	movq	[ecx +6 *4], mm5
	movq	[ecx +4 *4], mm6
	movq	[ecx +2 *4], mm7
	movq	[ecx +0 *4], mm0
	
	movq	mm0, [ecx +30*4]
	psubd	mm0, [ecx +34*4]
	movq	mm1, [ecx +26*4]
	psubd	mm1, mm0
	movq	mm2, [ecx +22*4]
	psubd	mm2, mm1
	movq	mm3, [ecx +18*4]
	psubd	mm3, mm2
	movq	mm4, [ecx +14*4]
	psubd	mm4, mm3
	movq	mm5, [ecx +10*4]
	psubd	mm5, mm4
	movq	mm6, [ecx +6 *4]
	psubd	mm6, mm5
	movq	mm7, [ecx +2 *4]
	psubd	mm7, mm6
	psrad	mm7, 1
	movq	[ecx +30*4], mm0
	movq	[ecx +26*4], mm1
	movq	[ecx +22*4], mm2
	movq	[ecx +18*4], mm3
	movq	[ecx +14*4], mm4
	movq	[ecx +10*4], mm5
	movq	[ecx +6 *4], mm6
	movq	[ecx +2 *4], mm7

	emms
	ret


; -------------------------------------------------------------------------
; vec is mono
; void CMDCTInt__cost9_oddscale_m_asm(
;4	int32 *vec);
;

GLOBAL CMDCTInt__cost9_oddscale_m_asm
ALIGN 16
CMDCTInt__cost9_oddscale_m_asm:

	mov		ecx, [esp +4]				; cost36_reso
	
	; +++++ SCHEDULE! +++++
	
	movd	mm0, [ecx +0 *4]
	mx31x16	0, 1, 2, cost36_c1, 0, 1
	movd	[ecx +0 *4], mm0

	movd	mm0, [ecx +1 *4]
	mx31x16	0, 1, 2, cost36_c1, 1, 1
	movd	[ecx +1 *4], mm0

	movd	mm0, [ecx +2 *4]
	mx31x16	0, 1, 2, cost36_c1, 2, 1
	movd	[ecx +2 *4], mm0

	movd	mm0, [ecx +3 *4]
	mx31x16	0, 1, 2, cost36_c1, 3, 1
	movd	[ecx +3 *4], mm0

	movd	mm0, [ecx +4 *4]
	mx31x16	0, 1, 2, cost36_c1, 4, 1
	movd	[ecx +4 *4], mm0

	movd	mm0, [ecx +5 *4]
	mx31x16	0, 1, 2, cost36_c1, 5, 1
	movd	[ecx +5 *4], mm0

	movd	mm0, [ecx +6 *4]
	mx31x16	0, 1, 2, cost36_c1, 6, 1
	movd	[ecx +6 *4], mm0

	movd	mm0, [ecx +7 *4]
	mx31x16	0, 1, 2, cost36_c1, 7, 1
	movd	[ecx +7 *4], mm0

	movd	mm0, [ecx +8 *4]
	mx31x16	0, 1, 2, cost36_c1, 8, 1
	movd	[ecx +8 *4], mm0

	emms
	ret

; -------------------------------------------------------------------------
; vec is stereo
; void CMDCTInt__cost9_oddscale_s_asm(
;4	int32 *vec);
;

GLOBAL CMDCTInt__cost9_oddscale_s_asm
ALIGN 16
CMDCTInt__cost9_oddscale_s_asm:

	mov		ecx, [esp +4]				; cost36_reso
	
	; +++++ SCHEDULE! +++++
	
	movq	mm0, [ecx +0 *8]
	mx31x16	0, 1, 2, cost36_c1, 0, 1
	movq	[ecx +0 *8], mm0

	movq	mm0, [ecx +1 *8]
	mx31x16	0, 1, 2, cost36_c1, 1, 1
	movq	[ecx +1 *8], mm0

	movq	mm0, [ecx +2 *8]
	mx31x16	0, 1, 2, cost36_c1, 2, 1
	movq	[ecx +2 *8], mm0

	movq	mm0, [ecx +3 *8]
	mx31x16	0, 1, 2, cost36_c1, 3, 1
	movq	[ecx +3 *8], mm0

	movq	mm0, [ecx +4 *8]
	mx31x16	0, 1, 2, cost36_c1, 4, 1
	movq	[ecx +4 *8], mm0

	movq	mm0, [ecx +5 *8]
	mx31x16	0, 1, 2, cost36_c1, 5, 1
	movq	[ecx +5 *8], mm0

	movq	mm0, [ecx +6 *8]
	mx31x16	0, 1, 2, cost36_c1, 6, 1
	movq	[ecx +6 *8], mm0

	movq	mm0, [ecx +7 *8]
	mx31x16	0, 1, 2, cost36_c1, 7, 1
	movq	[ecx +7 *8], mm0

	movq	mm0, [ecx +8 *8]
	mx31x16	0, 1, 2, cost36_c1, 8, 1
	movq	[ecx +8 *8], mm0

	emms
	ret

; -------------------------------------------------------------------------
; prev, dest: stereo-interleaved
; cost36_resX: mono
; void CMDCTInt__overlap_hybrid18_m_asm(
;4	int32 *prev,
;8	int32 *dest,
;12	const int32 *cost36_rese,
;16	const int32 *cost36_reso,
;20	const int16* win);

;	mov		ecx, [esp +4 +12]			; prev
;	mov		edx, [esp +8 +12]			; dest
;	mov		esi, [esp +12+12]			; rese
;	mov		edi, [esp +16+12]			; reso
;	mov		ebx, [esp +20+12]			; win

%macro overlap_block	7
; %1	resIndex
; %2	destIndex1
; %3	destIndex2
; %4	winIndex1
; %5	winIndex2
; %6	winIndex3
; %7	winIndex4

	; dest
	movd	mm0, [esi +%1 *4]
	psubd	mm0, [edi +%1 *4]
	movq	mm4, mm0
	mx31x16_load_cr	1, ebx, %4
	mx31x16_cr		0, 1, 2, 3, 1
	paddd	mm0, [ecx +%2 *8]
	movd	[edx +%2 *8], mm0
	movq	mm0, mm4
	mx31x16_load_cr	1, ebx, %5
	mx31x16_cr		0, 1, 2, 3, 1
	paddd	mm0, [ecx +%3 *8]
	movd	[edx +%3 *8], mm0

	; prev
	movd	mm0, [esi +%1 *4]
	paddd	mm0, [edi +%1 *4]
	movq	mm4, mm0
	mx31x16_load_cr	1, ebx, %6
	mx31x16_cr		0, 1, 2, 3, 1
	movd	[ecx +%2 *8], mm0
	movq	mm0, mm4
	mx31x16_load_cr	1, ebx, %7
	mx31x16_cr		0, 1, 2, 3, 1
	movd	[ecx +%3 *8], mm0

%endmacro

GLOBAL CMDCTInt__overlap_hybrid18_m_asm
ALIGN 16
CMDCTInt__overlap_hybrid18_m_asm:

	push	esi
	push	edi
	push	ebx

	mov		ecx, [esp +4 +12]			; prev
	mov		edx, [esp +8 +12]			; dest
	mov		esi, [esp +12+12]			; rese
	mov		edi, [esp +16+12]			; reso
	mov		ebx, [esp +20+12]			; win

	overlap_block	8, 0, 17, 0, 17, 18, 35
	overlap_block	7, 1, 16, 1, 16, 19, 34
	overlap_block	6, 2, 15, 2, 15, 20, 33
	overlap_block	5, 3, 14, 3, 14, 21, 32
	overlap_block	4, 4, 13, 4, 13, 22, 31
	overlap_block	3, 5, 12, 5, 12, 23, 30
	overlap_block	2, 6, 11, 6, 11, 24, 29
	overlap_block	1, 7, 10, 7, 10, 25, 28
	overlap_block	0, 8, 9,  8, 9,  26, 27

	pop		ebx
	pop		edi
	pop		esi
	emms
	ret

; -------------------------------------------------------------------------
; prev, dest, cost36_resX: stereo
; void CMDCTInt__overlap_hybrid18_s_asm(
;4	int32 *prev,
;8	int32 *dest,
;12	const int32 *cost36_rese,
;16	const int32 *cost36_reso,
;20	const int16* winL,
;24 const int16* winR);

%macro overlap_block_s	7
; %1	resIndex
; %2	destIndex1
; %3	destIndex2
; %4	winIndex1
; %5	winIndex2
; %6	winIndex3
; %7	winIndex4

	; dest
	movq	mm0, [esi +%1 *8]
	psubd	mm0, [edi +%1 *8]
	movq	mm4, mm0
	mx31x16_load_cr2	1, 2, ebx, eax, %4
	mx31x16_cr			0, 1, 2, 3, 1
	paddd	mm0, [ecx +%2 *8]
	movq	[edx +%2 *8], mm0
	movq	mm0, mm4
	mx31x16_load_cr2	1, 2, ebx, eax, %5
	mx31x16_cr			0, 1, 2, 3, 1
	paddd	mm0, [ecx +%3 *8]
	movq	[edx +%3 *8], mm0

	; prev
	movq	mm0, [esi +%1 *8]
	paddd	mm0, [edi +%1 *8]
	movq	mm4, mm0
	mx31x16_load_cr2	1, 2, ebx, eax, %6
	mx31x16_cr			0, 1, 2, 3, 1
	movq	[ecx +%2 *8], mm0
	movq	mm0, mm4
	mx31x16_load_cr2	1, 2, ebx, eax, %7
	mx31x16_cr			0, 1, 2, 3, 1
	movq	[ecx +%3 *8], mm0

%endmacro

GLOBAL CMDCTInt__overlap_hybrid18_s_asm
ALIGN 16
CMDCTInt__overlap_hybrid18_s_asm:

	push	esi
	push	edi
	push	ebx
	push	eax

	mov		ecx, [esp +4 +16]			; prev
	mov		edx, [esp +8 +16]			; dest
	mov		esi, [esp +12+16]			; rese
	mov		edi, [esp +16+16]			; reso
	mov		ebx, [esp +20+16]			; winL
	mov		eax, [esp +24+16]			; winR

	overlap_block_s	8, 0, 17, 0, 17, 18, 35
	overlap_block_s	7, 1, 16, 1, 16, 19, 34
	overlap_block_s	6, 2, 15, 2, 15, 20, 33
	overlap_block_s	5, 3, 14, 3, 14, 21, 32
	overlap_block_s	4, 4, 13, 4, 13, 22, 31
	overlap_block_s	3, 5, 12, 5, 12, 23, 30
	overlap_block_s	2, 6, 11, 6, 11, 24, 29
	overlap_block_s	1, 7, 10, 7, 10, 25, 28
	overlap_block_s	0, 8, 9,  8, 9,  26, 27

	pop		eax
	pop		ebx
	pop		edi
	pop		esi
	emms
	ret


