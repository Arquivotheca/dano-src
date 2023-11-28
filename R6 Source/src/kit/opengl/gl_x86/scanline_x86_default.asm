%include "asm.inc"
%include "externs.mac"
%include "layout.mac"
%include "katmai.mac"

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


%define _USE_TEX_ 0
%define _USE_DEPTH_ 0
%define _USE_SMOOTH_ 0
%define _USE_FOG_ 0



SECTION .text

; Replacement for FillSubTriangle

; mm0 iterated color BG
; mm1 iterated color RA
; mm2 iterated color dBdx dGdx
; mm3 iterated color dRdx dAdx
; mm4 temp color

STRUC stack_data
.frag_B				resd 1		; 16:16 fixed
.frag_G				resd 1		; 16:16 fixed
.frag_R				resd 1		; 16:16 fixed
.frag_A				resd 1		; 16:16 fixed

.dbdx				resd 1		; 16:16 fixed
.dgdx				resd 1		; 16:16 fixed
.drdx				resd 1		; 16:16 fixed
.dadx				resd 1		; 16:16 fixed

.frag_X				resd 1
.frag_Y				resd 1
.frag_Z				resd 1
.frag_S				resd 1
.frag_T				resd 1
.frag_QW			resd 1
.frag_F				resd 1


.dsdx				resd 1		; 16:16 fixed
.dtdx				resd 1		; 16:16 fixed
.dqwdx				resd 1		; 16:16 fixed

.xLeft				resd 1		; 16:16 fixed
.xRight				resd 1		; 16:16 fixed
.ixLeft				resd 1
.ixRight			resd 1
.spanWidth			resd 1
.dxdyLeft			resd 1		; 16:16 fixed
.dxdyRight			resd 1		; 16:16 fixed
.yBottom			resd 1
.yTop				resd 1
.Z					resd 1
.S					resd 1
.T					resd 1
.QW					resd 1
.F					resd 1
.flatColor			resd 1
.tex_base			resd 1		; base address of texture data
.tex_stride			resd 1		; Stride in bytes
.tex_width			resd 1
.tex_height			resd 1
.tex_bpp			resd 1
.end
ENDSTRUC

%define local_size 16 + stack_data.end

%define stack_gc		esp+local_size+4


MARK_FUNC fillsub_Init
		push ebx
		push edi
		push esi
		push ebp
		sub esp, stack_data.end

		mov ebx, [stack_gc]

		mov eax, [ebx + __glContext.softScanProcs_shade_xLeftFixed]
		mov [esp + stack_data.xLeft], eax
		mov eax, [ebx + __glContext.softScanProcs_shade_xRightFixed]
		mov [esp + stack_data.xRight], eax
		
		fld dword [ebx + __glContext.softScanProcs_shade_dxdyLeft]
		fmul dword [fpConst_65536_0]
		fistp dword [esp + stack_data.dxdyLeft]

		fld dword [ebx + __glContext.softScanProcs_shade_dxdyRight]
		fmul dword [fpConst_65536_0]
		fistp dword [esp + stack_data.dxdyRight]
		
		mov eax, [ebx + __glContext.softScanProcs_shade_iyBottom]
		mov [esp + stack_data.yBottom], eax
		mov eax, [ebx + __glContext.softScanProcs_shade_iyTop]
		mov [esp + stack_data.yTop], eax
MARK_FUNC fillsub_Init_end


MARK_FUNC fillsub_Init_Shade
		fld dword [ebx + __glContext.softScanProcs_shade_r0]
		fistp dword [esp + stack_data.frag_R]

		fld dword [ebx + __glContext.softScanProcs_shade_g0]
		fistp dword [esp + stack_data.frag_G]

		fld dword [ebx + __glContext.softScanProcs_shade_b0]
		fistp dword [esp + stack_data.frag_B]

		fld dword [ebx + __glContext.softScanProcs_shade_a0]
		fistp dword [esp + stack_data.frag_A]
		
		fld dword [ebx + __glContext.softScanProcs_shade_drdx]
		fistp dword [esp + stack_data.drdx]

		fld dword [ebx + __glContext.softScanProcs_shade_dgdx]
		fistp dword [esp + stack_data.dgdx]

		fld dword [ebx + __glContext.softScanProcs_shade_dbdx]
		fistp dword [esp + stack_data.dbdx]

		fld dword [ebx + __glContext.softScanProcs_shade_dadx]
		fistp dword [esp + stack_data.dadx]
MARK_FUNC fillsub_Init_Shade_end

MARK_FUNC fillsub_Init_Z
		mov eax, [ebx + __glContext.softScanProcs_shade_z0]
		mov [esp + stack_data.Z], eax
MARK_FUNC fillsub_Init_Z_end

MARK_FUNC fillsub_Init_F
		mov eax, [ebx + __glContext.softScanProcs_shade_f0]
		mov [esp + stack_data.F], eax
MARK_FUNC fillsub_Init_F_end

MARK_FUNC fillsub_Init_Tex1
		mov eax, [ebx + __glContext.softScanProcs_shade_s0]
		mov [esp + stack_data.S], eax

		mov eax, [ebx + __glContext.softScanProcs_shade_t0]
		mov [esp + stack_data.T], eax

		mov eax, [ebx + __glContext.softScanProcs_shade_qw0]
		mov [esp + stack_data.QW], eax


		fld dword [ebx + __glContext.softScanProcs_shade_dsdx]
		fistp dword [esp + stack_data.dsdx]

		fld dword [ebx + __glContext.softScanProcs_shade_dtdx]
		fistp dword [esp + stack_data.dtdx]

		fld dword [ebx + __glContext.softScanProcs_shade_dqwdx]
		fistp dword [esp + stack_data.dqwdx]
MARK_FUNC fillsub_Init_Tex1_end

MARK_FUNC fillsub_Init_Tex1_non_mip
		mov eax, [ebx + __glContext.texture_Active]
		mov edx, [eax + __glTexture.level + __glTextureLevel.data]
		mov [esp + stack_data.tex_base], edx
		
		mov edx, [eax + __glTexture.level + __glTextureLevel.width]
		imul edx, [eax + __glTexture.level + __glTextureLevel.bytesPerTexel]
		mov [esp + stack_data.tex_stride], edx

		mov edx, [eax + __glTexture.level + __glTextureLevel.bytesPerTexel]
		mov [esp + stack_data.tex_bpp], edx
		
		mov edx, [eax + __glTexture.level + __glTextureLevel.width]
		mov [esp + stack_data.tex_width], edx

		mov edx, [eax + __glTexture.level + __glTextureLevel.height]
		mov [esp + stack_data.tex_height], edx
MARK_FUNC fillsub_Init_Tex1_non_mip_end


MARK_FUNC fillsub_YLoopInit
.loop
		mov eax, [esp + stack_data.yTop]
		cmp eax, [esp + stack_data.yBottom]
MARK_FUNC fillsub_YLoopInit_end
	;	jle near .done
		
MARK_FUNC fillsub_PerLineInit
			mov edx, [esp + stack_data.xLeft]
			shr edx, 16
			mov ecx, [esp + stack_data.xRight]
			mov [esp + stack_data.ixLeft], edx
			shr ecx, 16
			mov [esp + stack_data.ixRight], ecx
			sub ecx, edx
MARK_FUNC fillsub_PerLineInit_end
	;		jle  near .next
			
MARK_FUNC fillsub_PerLineInit2
				mov [esp + stack_data.spanWidth], ecx
				
				fild dword [esp + stack_data.ixLeft]
				fmul dword [fpConst_65536_0]
				fild dword [esp + stack_data.xLeft]
				fsubp st1, st0
				fadd dword [fpConst_65535_0]
				fmul dword [fpConst_Over_65536]
				

				mov eax, [esp + stack_data.ixLeft]
				mov [esp + stack_data.frag_X], eax
				mov eax, [esp + stack_data.yBottom]
				mov [esp + stack_data.frag_Y], eax
MARK_FUNC fillsub_PerLineInit2_end
				
MARK_FUNC fillsub_PerLine_Smooth
				fld dword [ebx + __glContext.softScanProcs_shade_drdx]
				fmul st0, st1
				fadd dword [ebx + __glContext.softScanProcs_shade_r0]
				fistp dword [esp + stack_data.frag_R]
				
				fld dword [ebx + __glContext.softScanProcs_shade_dgdx]
				fmul st0, st1
				fadd dword [ebx + __glContext.softScanProcs_shade_g0]
				fistp dword [esp + stack_data.frag_G]
				
				fld dword [ebx + __glContext.softScanProcs_shade_dbdx]
				fmul st0, st1
				fadd dword [ebx + __glContext.softScanProcs_shade_b0]
				fistp dword [esp + stack_data.frag_B]
				
				fld dword [ebx + __glContext.softScanProcs_shade_dadx]
				fmul st0, st1
				fadd dword [ebx + __glContext.softScanProcs_shade_a0]
				fistp dword [esp + stack_data.frag_A]
MARK_FUNC fillsub_PerLine_Smooth_end

MARK_FUNC fillsub_PerLine_Z
				fld dword [ebx + __glContext.softScanProcs_shade_dzdx]
				fmul st0, st1
				fadd dword [esp + stack_data.Z]
				fstp dword [esp + stack_data.frag_Z]
MARK_FUNC fillsub_PerLine_Z_end
				
MARK_FUNC fillsub_PerLine_Tex1
				fld dword [ebx + __glContext.softScanProcs_shade_dsdx]
				fmul st0, st1
				fadd dword [esp + stack_data.S]
				fistp dword [esp + stack_data.frag_S]

				fld dword [ebx + __glContext.softScanProcs_shade_dtdx]
				fmul st0, st1
				fadd dword [esp + stack_data.T]
				fistp dword [esp + stack_data.frag_T]

				fld dword [ebx + __glContext.softScanProcs_shade_dqwdx]
				fmul st0, st1
				fadd dword [esp + stack_data.QW]
				fistp dword [esp + stack_data.frag_QW]
MARK_FUNC fillsub_PerLine_Tex1_end

MARK_FUNC fillsub_PerLine_F
				fld dword [ebx + __glContext.softScanProcs_shade_dfdx]
				fmul st0, st1
				fadd dword [esp + stack_data.F]
				fstp dword [esp + stack_data.frag_F]
MARK_FUNC fillsub_PerLine_F_end
				
MARK_FUNC fillsub_PerLineInit3
				fstp st0
				movq mm0, [esp + stack_data.frag_B]
				movq mm1, [esp + stack_data.frag_R]
MARK_FUNC fillsub_PerLineInit3_end
		
MARK_FUNC fillsub_PerLine_Smooth2
				movq mm2, [esp + stack_data.dbdx]
				movq mm3, [esp + stack_data.drdx]
MARK_FUNC fillsub_PerLine_Smooth2_end
			
				;;;;;;;; Draw a scanline
				;;;;;;;;

				
.next
MARK_FUNC fillsub_PerLineNext
			emms
			mov eax, [esp + stack_data.xLeft]
			add eax, [esp + stack_data.dxdyLeft]
			mov [esp + stack_data.xLeft], eax

			mov eax, [esp + stack_data.xRight]
			add eax, [esp + stack_data.dxdyRight]
			mov [esp + stack_data.xRight], eax
MARK_FUNC fillsub_PerLineNext_end

MARK_FUNC fillsub_PerLineNext_Smooth
			fld dword [ebx + __glContext.softScanProcs_shade_r0]
			fadd dword [ebx + __glContext.softScanProcs_shade_drdxdy]
			fstp dword [ebx + __glContext.softScanProcs_shade_r0]

			fld dword [ebx + __glContext.softScanProcs_shade_g0]
			fadd dword [ebx + __glContext.softScanProcs_shade_dgdxdy]
			fstp dword [ebx + __glContext.softScanProcs_shade_g0]

			fld dword [ebx + __glContext.softScanProcs_shade_b0]
			fadd dword [ebx + __glContext.softScanProcs_shade_dbdxdy]
			fstp dword [ebx + __glContext.softScanProcs_shade_b0]

			fld dword [ebx + __glContext.softScanProcs_shade_a0]
			fadd dword [ebx + __glContext.softScanProcs_shade_dadxdy]
			fstp dword [ebx + __glContext.softScanProcs_shade_a0]
MARK_FUNC fillsub_PerLineNext_Smooth_end
			
MARK_FUNC fillsub_PerLineNext_Z
			fld dword [esp + stack_data.Z]
			fadd dword [ebx + __glContext.softScanProcs_shade_dzdxdy]
			fstp dword [esp + stack_data.Z]
MARK_FUNC fillsub_PerLineNext_Z_end
			
MARK_FUNC fillsub_PerLineNext_Tex1
			fld dword [esp + stack_data.S]
			fadd dword [ebx + __glContext.softScanProcs_shade_dsdxdy]
			fstp dword [esp + stack_data.S]
			fistp dword [esp + stack_data.frag_S]

			fld dword [esp + stack_data.T]
			fadd dword [ebx + __glContext.softScanProcs_shade_dtdxdy]
			fstp dword [esp + stack_data.T]
			fistp dword [esp + stack_data.frag_T]

			fld dword [esp + stack_data.QW]
			fadd dword [ebx + __glContext.softScanProcs_shade_dqwdxdy]
			fstp dword [esp + stack_data.QW]
			fistp dword [esp + stack_data.frag_QW]
MARK_FUNC fillsub_PerLineNext_Tex1_end
			
MARK_FUNC fillsub_PerLineNext_F
			fld dword [esp + stack_data.F]
			fadd dword [ebx + __glContext.softScanProcs_shade_dfdxdy]
			fstp dword [esp + stack_data.F]
MARK_FUNC fillsub_PerLineNext_F_end

MARK_FUNC fillsub_PerLineCopyFront
		mov eax, [esp + stack_data.yBottom]
		mov edx, [ebx + __glContext.buffer_visible]
		mov [edx + __glBufferSurface.ScanlineY], eax
		
		imul eax, [edx + __glBufferSurface.Width]
		shl eax, 2
		add eax, [edx + __glBufferSurface.ColorFront]
		mov [edx + __glBufferSurface.ScanlineFBIN], eax
		
		
		mov eax, [esp + stack_data.ixRight]
		mov edx, [esp + stack_data.ixLeft]
		push eax
		push edx
		push ebx
		call [ebx + __glContext.procs_callback_StoreFrontScanline]
		add esp, 12
MARK_FUNC fillsub_PerLineCopyFront_end
			
MARK_FUNC fillsub_PerLineEnd
			inc dword [esp + stack_data.yBottom]
			emms
MARK_FUNC fillsub_PerLineEnd_end
	;		jmp .loop

.done

MARK_FUNC fillsub_Done
		mov eax, [esp + stack_data.xLeft]
		mov [ebx + __glContext.softScanProcs_shade_xLeftFixed], eax
		mov eax, [esp + stack_data.xRight]
		mov [ebx + __glContext.softScanProcs_shade_xRightFixed], eax
MARK_FUNC fillsub_Done_end
		
		
		
MARK_FUNC fillsub_Done_Z
		mov eax, [esp + stack_data.Z]
		mov [ebx + __glContext.softScanProcs_shade_z0], eax
MARK_FUNC fillsub_Done_Z_end
		
MARK_FUNC fillsub_Done_Tex1
		mov eax, [esp + stack_data.S]
		mov [ebx + __glContext.softScanProcs_shade_s0], eax
		mov eax, [esp + stack_data.T]
		mov [ebx + __glContext.softScanProcs_shade_t0], eax
		mov eax, [esp + stack_data.QW]
		mov [ebx + __glContext.softScanProcs_shade_qw0], eax
MARK_FUNC fillsub_Done_Tex1_end
		
MARK_FUNC fillsub_Done_F
		mov eax, [esp + stack_data.F]
		mov [ebx + __glContext.softScanProcs_shade_f0], eax
MARK_FUNC fillsub_Done_F_end


MARK_FUNC fillsub_Cleanup
		add esp, stack_data.end
		pop ebp
		pop esi
		pop edi
		pop ebx
		ret
MARK_FUNC fillsub_Cleanup_end



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


MARK_FUNC fillsub_Init_Flat
		fld dword [ebx + __glContext.softScanProcs_shade_b0]
		fistp dword [esp + stack_data.frag_B]

		fld dword [ebx + __glContext.softScanProcs_shade_g0]
		fistp dword [esp + stack_data.frag_G]

		fld dword [ebx + __glContext.softScanProcs_shade_r0]
		fistp dword [esp + stack_data.frag_R]

		fld dword [ebx + __glContext.softScanProcs_shade_a0]
		fistp dword [esp + stack_data.frag_A]


		movq mm0, [esp + stack_data.frag_B]
		movq mm1, [esp + stack_data.frag_R]

		movq mm4, mm0
		movq mm5, mm1

		psrld mm4, 16
		psrld mm5, 16
			
		packssdw mm4, mm5

%if 0
		mov edx, [esp + stack_data.frag_B]
		shr edx, 16
		and edx, 0x000000ff
		
		mov eax, [esp + stack_data.frag_G]
		shr eax, 8
		and eax, 0x0000ff00
		or edx, eax
		
		mov eax, [esp + stack_data.frag_R]
		and eax, 0x00ff0000
		or edx, eax

		mov eax, [esp + stack_data.frag_A]
		shl eax, 8
		and eax, 0xff000000
		or edx, eax
		mov [esp + stack_data.flatColor], edx
%endif
MARK_FUNC fillsub_Init_Flat_end


MARK_FUNC fillsub_scan_CalcOffset
		mov ecx, [ebx + __glContext.buffer_current]
		mov eax, [esp + stack_data.frag_Y]
		imul eax, dword [ecx + __glBufferSurface.Width]
		add eax, [esp + stack_data.frag_X]
MARK_FUNC fillsub_scan_CalcOffset_end

MARK_FUNC fillsub_scan_CalcOffset_color32
		mov edi, eax
		shl edi, 2
MARK_FUNC fillsub_scan_CalcOffset_color32_end

MARK_FUNC fillsub_scan_CalcOffset_color16
		mov edi, eax
		shl edi, 1
MARK_FUNC fillsub_scan_CalcOffset_color16_end

MARK_FUNC fillsub_scan_CalcOffset_colorFront
		add edi, [ecx + __glBufferSurface.ColorFront]
MARK_FUNC fillsub_scan_CalcOffset_colorFront_end

MARK_FUNC fillsub_scan_CalcOffset_colorBack
		add edi, [ecx + __glBufferSurface.ColorBack]
MARK_FUNC fillsub_scan_CalcOffset_colorBack_end

	;	mov ebp, eax
	;	add ebp, [ecx + __glBufferSurface.Depth]
		
		
MARK_FUNC fillsub_scan_loop_init
		mov ecx, [esp + stack_data.spanWidth]
MARK_FUNC fillsub_scan_loop_init_end
		
	;	fld dword [esp + stack_data.frag_Z]
		
.spanLoop
MARK_FUNC fillsub_scan_loop_smooth
%if 1
			movq mm4, mm0
			movq mm5, mm1

			psrld mm4, 16
			psrld mm5, 16
			
			paddd mm0, mm2
			paddd mm1, mm3
			
			packssdw mm4, mm5
%else
			mov edx, [esp + stack_data.frag_B]
			mov esi, edx
			add esi, [esp + stack_data.dbdx]
			mov [esp + stack_data.frag_B], esi
			shr edx, 16
			and edx, 0x000000ff
			
			mov eax, [esp + stack_data.frag_G]
			mov esi, eax
			add esi, [esp + stack_data.dgdx]
			mov [esp + stack_data.frag_G], esi
			shr eax, 8
			and eax, 0x0000ff00
			or edx, eax
			
			mov eax, [esp + stack_data.frag_R]
			mov esi, eax
			add esi, [esp + stack_data.drdx]
			mov [esp + stack_data.frag_R], esi
			and eax, 0x00ff0000
			or edx, eax

			mov eax, [esp + stack_data.frag_A]
			mov esi, eax
			add esi, [esp + stack_data.dadx]
			mov [esp + stack_data.frag_A], esi
			shl eax, 8
			and eax, 0xff000000
			or edx, eax
%endif
MARK_FUNC fillsub_scan_loop_smooth_end

MARK_FUNC fillsub_scan_loop_flat
		movq mm4, mm0
		movq mm5, mm1

		psrld mm4, 16
		psrld mm5, 16
			
		packssdw mm4, mm5
	;		mov edx, [esp + stack_data.flatColor]
MARK_FUNC fillsub_scan_loop_flat_end

MARK_FUNC fillsub_scan_loop_tex_fetch
		mov eax, [esp + stack_data.frag_T]
		mov esi, eax
		add eax, [esp + stack_data.dtdx]
		mov [esp + stack_data.frag_T], eax
		
		and esi, 0x0000ffff			;; mask of high bits for repeat
		imul esi, [esp + stack_data.tex_height]
		shr esi, 16
		imul esi, [esp + stack_data.tex_width]
		
		mov eax, [esp + stack_data.frag_S]
		mov edx, eax
		add eax, [esp + stack_data.dsdx]
		mov [esp + stack_data.frag_S], eax
		
		and edx, 0xffff					; repeat
		imul edx, [esp + stack_data.tex_width]
		shr edx, 16
		add esi, edx

		imul esi, [esp + stack_data.tex_bpp]
		add esi, [esp + stack_data.tex_base]
		
		pxor mm7, mm7
		movd mm4, [esi]
		punpcklbw mm4, mm7
	
	
		
	;	mov edx, [esi]
	;	and edx, 0x000000ff
	;	shl edx, 16

	;	mov eax, [esi]
	;	and eax, 0x00ff0000
	;	shr eax, 16
	;	or edx, eax

	;	mov eax, [esi]
	;	and eax, 0xff000000
	;	or edx, eax

	;	mov eax, [esi]
	;	and eax, 0x0000ff00
	;	or edx, eax
		
		
MARK_FUNC fillsub_scan_loop_tex_fetch_end
	
	
	;		fcom dword [ebp]
	;;		fnstsw ax
	;		sahf
	;		ja .noWrite
MARK_FUNC fillsub_scan_loop_color_write
			packuswb mm4, mm4
			movd [edi], mm4
MARK_FUNC fillsub_scan_loop_color_write_end

MARK_FUNC fillsub_scan_loop_depth_write
				fst dword [ebp]
MARK_FUNC fillsub_scan_loop_depth_write_end
.noWrite
			
			
			
	;		fadd dword [ebx + __glContext.softScanProcs_shade_dzdx]
			
	;		add ebp, 4

MARK_FUNC fillsub_scan_loop_finish
			add edi, 4
			dec ecx
MARK_FUNC fillsub_scan_loop_finish_end
	;		jne .spanLoop
			
	;		fstp st0
			



