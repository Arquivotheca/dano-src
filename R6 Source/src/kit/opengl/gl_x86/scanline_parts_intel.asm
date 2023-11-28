%include "asm.inc"
%include "externs.mac"
%include "layout.mac"
%include "katmai.mac"

%MACRO MARK_FUNC 1
	GLOBAL %1
	%1:
%ENDMACRO	


EXTERN validateSoftScanProcs



; Shader
; Stipple Test
; Depth Test
; Alpha test
; Fog ?


; void rasProcessScanline( __glContext *gc, const __glFragment *start, __glShade *shade, GLint w )

stipple_data:
	dd 	0x01000000, 0x02000000, 0x04000000, 0x08000000
	dd 	0x10000000, 0x20000000, 0x40000000, 0x80000000

	dd 	0x00010000, 0x00020000, 0x00040000, 0x00080000
	dd 	0x00100000, 0x00200000, 0x00400000, 0x00800000

	dd 	0x00000100, 0x00000200, 0x00000400, 0x00000800
	dd 	0x00001000, 0x00002000, 0x00004000, 0x00008000

	dd 	0x00000001, 0x00000002, 0x00000004, 0x00000008
	dd 	0x00000010, 0x00000020, 0x00000040, 0x00000080




STRUC stack_data
.x				resd 1
.stipple		resd 1

.end
ENDSTRUC

%define local_size 12 + stack_data.end

%define stack_gc		esp+local_size+4
%define stack_start		esp+local_size+8
%define stack_shade		esp+local_size+12
%define stack_w	 		esp+local_size+16

MARK_FUNC scanline_init
		push ebx
		push edi
		push esi
		sub esp, stack_data.end
		
		mov ebx, [stack_gc]		; gc
		mov ecx, [ebx + __glContext.buffer_current]
		mov eax, [ecx + __glBufferSurface.ScanlineY]
		imul dword [ecx + __glBufferSurface.Width]
		mov ecx, [stack_start]											; start
		mov edx, [ecx + __glFragment.x]
		add eax, edx
MARK_FUNC scanline_init_end
		
MARK_FUNC scanline_init_color
		mov edi, [ebx + __glContext.buffer_current]
		mov edi, [edi + __glBufferSurface.ColorBack]
		lea edi, [edi + eax*4]
MARK_FUNC scanline_init_color_end

MARK_FUNC scanline_init_depth
		mov esi, [ebx + __glContext.buffer_current]
		mov esi, [esi + __glBufferSurface.Depth]
		lea esi, [esi + eax*4]
		prefetcht0 [esi]
MARK_FUNC scanline_init_depth_end
		
MARK_FUNC scanline_init_color_start
		movups xmm0, [ecx + __glFragment.color_R]
		shufps xmm0, xmm0, byte base4_0312
		cvtps2pi mm2, xmm0												; R G
		shufps xmm0, xmm0, byte base4_2323
		cvtps2pi mm1, xmm0												; B A
		packssdw mm2, mm1												; mm2 = color
		psllw mm2, 6													; scaled color
MARK_FUNC scanline_init_color_start_end

MARK_FUNC scanline_init_depth_start
		movss xmm7, [ecx + __glFragment.z]
MARK_FUNC scanline_init_depth_start_end

MARK_FUNC scanline_init_stipple_start
		mov eax, [ecx + __glFragment.x]
		mov [esp + stack_data.x], eax
		mov eax, [ebx + __glContext.buffer_current]
		mov eax, [eax + __glBufferSurface.ScanlineY]
		and eax, 0x1f
		mov eax, [ebx + eax*4 + __glContext.state_polyStippleMask]
		mov [esp + stack_data.stipple], eax
MARK_FUNC scanline_init_stipple_start_end

MARK_FUNC scanline_init_color_shade
		mov eax, [stack_shade]
	;	movq mm1, [eax + __glShade.mmxDB]
		pxor mm3, mm3
MARK_FUNC scanline_init_color_shade_end

MARK_FUNC scanline_init_depth_shade
		movss xmm6, [eax + __glShade.dzdx]
MARK_FUNC scanline_init_depth_shade_end
		

MARK_FUNC scanline_loop_init
		mov ecx, [stack_w]								
MARK_FUNC scanline_loop_init_end

loop1
MARK_FUNC scanline_loop_depth_fetch
		prefetcht0 [esi+20]
MARK_FUNC scanline_loop_depth_fetch_end

MARK_FUNC scanline_loop_color
		movq mm0, mm2
		psraw mm0, 6
		packuswb mm0, mm3
MARK_FUNC scanline_loop_color_end
		
MARK_FUNC scanline_depth_test
		comiss xmm7, [esi]
MARK_FUNC scanline_depth_test_end
		ja noStore

MARK_FUNC scanline_stipple_test
		mov eax, [esp + stack_data.x]
		and eax, 0x1f
		mov eax, [stipple_data + eax*4]
		and eax, [esp + stack_data.stipple]
MARK_FUNC scanline_stipple_test_end
		jz noStore
		
MARK_FUNC scanline_color_write
		movd [edi], mm0
MARK_FUNC scanline_color_write_end

MARK_FUNC scanline_depth_write
		movss [esi], xmm7
MARK_FUNC scanline_depth_write_end

noStore
MARK_FUNC scanline_depth_add
		addss xmm7, xmm6
		add esi, 4
MARK_FUNC scanline_depth_add_end

MARK_FUNC scanline_color_add
		paddsw mm2, mm1
		add edi, 4
MARK_FUNC scanline_color_add_end

MARK_FUNC scanline_stipple_add
		inc dword [esp + stack_data.x]
MARK_FUNC scanline_stipple_add_end

MARK_FUNC scanline_loop_inc
		dec ecx
MARK_FUNC scanline_loop_inc_end
		jnz loop1
	
MARK_FUNC scanline_finish
		add esp, stack_data.end
		pop esi
		pop edi
		pop ebx
		emms
		ret
MARK_FUNC scanline_finish_end
	
	


