%include "asm.inc"
%include "mgl_state.inc"
;%include "externs.mac"
;%include "layout.mac"
;%include "katmai.mac"

%MACRO MARK_FUNC 1
	GLOBAL %1
	%1:
%ENDMACRO	


SECTION .data

fpConst_0_0: dd 0.0
fpConst_0_5: dd 0.5
fpConst_65536_0: dd 65536.0
fpConst_65535_0: dd 65535.0

fpConst_Over_65536: dd 0.000015258789
fpConst_Over_65535: dd 0.000015259022

mmxConst_0000ffff_0000ffff: dd 0x0000ffff, 0x0000ffff

pnormtext1:		db "EAX=%8x  EBX=%8x  ECX=%8x  EDX=%8x ", 10, 0
pnormtext2:		db "ESI=%8x  EDI=%8x  EBP=%8x  ESP=%8x ", 10, 0




SECTION .text

extern printf
EXPORT_FUNC printRegs
	pusha
	push edx
	push ecx
	push ebx
	push eax
	push dword pnormtext1
	mov eax, printf
	call eax

	lea eax, [esp + 20]
	push eax
	push ebp
	push edi
	push esi
	push dword pnormtext2
	mov eax, printf
	call eax
	add esp, 24
	pop eax
	pop ebx
	pop ecx
	pop edx
	popa
	ret

; Replacement for FillSubTriangle

; mm0 iterated color BG
; mm1 iterated color RA
; mm2 iterated texture ST
; mm3 z/fog
; mm4 temp color
; mm5
; mm6
; mm7


MARK_FUNC fillsub_Init
		push ebx
		push esi
		push edi
		push ebp
		mov ebx, [esp + 20]		;; context
MARK_FUNC fillsub_Init_end

MARK_FUNC fillsub_Uninit
		pop ebp
		pop edi
		pop esi
		pop ebx
		emms
		ret
MARK_FUNC fillsub_Uninit_end

MARK_FUNC fillsub_YTest1
		mov eax, [ebx + __mglContext.shade_yBottom]
		cmp eax, [ebx + __mglContext.shade_yTop]
MARK_FUNC fillsub_YTest1_end

MARK_FUNC fillsub_YTest2
		mov ecx, eax
		inc ecx
		mov [ebx + __mglContext.shade_yBottom], ecx
MARK_FUNC fillsub_YTest2_end


MARK_FUNC fillsub_CalcOffset_load
		mov eax, [ebx + __mglContext.shade_yBottom]
MARK_FUNC fillsub_CalcOffset_load_end

MARK_FUNC fillsub_CalcOffset
		imul eax, dword [ebx + __mglContext.color_width]
		
		mov edx, [ebx + __mglContext.shade_xLeftFixed]
		mov ecx, [ebx + __mglContext.shade_xRightFixed]
		shr edx, 16
		shr ecx, 16
		
		add eax, edx
		sub ecx, edx
MARK_FUNC fillsub_CalcOffset_end

MARK_FUNC fillsub_CalcOffset_color32
		mov edi, [ebx + __mglContext.color_data]
		lea edi, [edi + eax*4]
MARK_FUNC fillsub_CalcOffset_color32_end

MARK_FUNC fillsub_CalcOffset_color16
		mov edi, [ebx + __mglContext.color_data]
		lea edi, [edi + eax*2]
MARK_FUNC fillsub_CalcOffset_color16_end

MARK_FUNC fillsub_CalcOffset_Z
		mov ebp, [ebx + __mglContext.depth_data]
		lea ebp, [ebp + eax*4]
MARK_FUNC fillsub_CalcOffset_Z_end

		
MARK_FUNC fillsub_loop_init_color
		movq mm0, [ebx + __mglContext.shade_b]
		movq mm1, [ebx + __mglContext.shade_r]
MARK_FUNC fillsub_loop_init_color_end

MARK_FUNC fillsub_init_depth_fog
		movq mm3, [ebx + __mglContext.shade_z]
MARK_FUNC fillsub_init_depth_fog_end
		

MARK_FUNC fillsub_loop_smooth
		movq mm4, mm0
		movq mm5, mm1
		paddd mm0, [ebx + __mglContext.shade_dbdx]
		paddd mm1, [ebx + __mglContext.shade_drdx]
		psrld mm4, 16
		psrld mm5, 16
		packssdw mm4, mm5
MARK_FUNC fillsub_loop_smooth_end


MARK_FUNC fillsub_loop_flat
		movq mm4, mm0
		movq mm5, mm1
		psrld mm4, 16
		psrld mm5, 16
		packssdw mm4, mm5
MARK_FUNC fillsub_loop_flat_end


MARK_FUNC fillsub_loop_tex_fetch_addr
		movq mm5, mm2
		paddd mm2, [ebx + __mglContext.shade_tex0_dsdx]
		
		pand mm5, [mmxConst_0000ffff_0000ffff]	; repeat
		
		pmulhuw mm5, [ebx + __mglContext.state_texture + __mglTexture.surface_width]
		movd esi, mm5
		psrlq mm5, 32
		movd edx, mm5
		imul edx, [ebx + __mglContext.state_texture + __mglTexture.surface_width]
		add edx, esi
MARK_FUNC fillsub_loop_tex_fetch_addr_end


MARK_FUNC fillsub_loop_tex_fetch_3
		lea esi, [edx + edx*2]		; multiply by 3
		add esi, [ebx + __mglContext.state_texture + __mglTexture.surface_data]
		pxor mm7, mm7
		movd mm4, [esi]
		punpcklbw mm4, mm7
MARK_FUNC fillsub_loop_tex_fetch_3_end

MARK_FUNC fillsub_loop_tex_fetch_4
		mov esi, [ebx + __mglContext.state_texture + __mglTexture.surface_data]
		lea esi, [esi + edx*4]		; multiply by 4
		pxor mm7, mm7
		movd mm4, [esi]
		punpcklbw mm4, mm7
MARK_FUNC fillsub_loop_tex_fetch_4_end
	
	
	;		fcom dword [ebp]
	;;		fnstsw ax
	;		sahf
	;		ja .noWrite
MARK_FUNC fillsub_loop_color_write
			packuswb mm4, mm4
			movd [edi], mm4
MARK_FUNC fillsub_loop_color_write_end

MARK_FUNC fillsub_loop_Z_test
			movd eax, mm3
			paddd mm3, [ebx + __mglContext.shade_dzdx]
			cmp eax, [ebp]
MARK_FUNC fillsub_loop_Z_test_end

MARK_FUNC fillsub_loop_Z_write
			movd [ebp], mm3
MARK_FUNC fillsub_loop_Z_write_end
			
			
MARK_FUNC fillsub_loop_finish
	add ebp, 4
			add edi, 4
			dec ecx
MARK_FUNC fillsub_loop_finish_end
			

MARK_FUNC fillsub_step_color
		movq mm0, [ebx + __mglContext.shade_b]
		movq mm1, [ebx + __mglContext.shade_r]
		paddd mm0, [ebx + __mglContext.shade_dbdxdy]
		paddd mm1, [ebx + __mglContext.shade_drdxdy]
		movq [ebx + __mglContext.shade_b], mm0
		movq [ebx + __mglContext.shade_r], mm1
MARK_FUNC fillsub_step_color_end

MARK_FUNC fillsub_step_x
		movq mm7, [ebx + __mglContext.shade_xLeftFixed]
		paddd mm7, [ebx + __mglContext.shade_dxdyLeftFixed]
		movq [ebx + __mglContext.shade_xLeftFixed], mm7
MARK_FUNC fillsub_step_x_end

MARK_FUNC fillsub_step_tex0
		movq mm2, [ebx + __mglContext.shade_tex0_s]
		paddd mm2, [ebx + __mglContext.shade_tex0_dsdxdy]
		movq [ebx + __mglContext.shade_tex0_s], mm2
MARK_FUNC fillsub_step_tex0_end


MARK_FUNC fillsub_step_zf
		movq mm3, [ebx + __mglContext.shade_z]
		paddd mm3, [ebx + __mglContext.shade_dzdxdy]
		movq [ebx + __mglContext.shade_z], mm3
MARK_FUNC fillsub_step_zf_end



