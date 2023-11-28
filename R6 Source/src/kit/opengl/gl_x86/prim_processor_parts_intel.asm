%include "asm.inc"
%include "externs.mac"
%include "layout.mac"

%MACRO MARK_FUNC 1
	GLOBAL %1
	%1:
%ENDMACRO	

MARK_FUNC primParts_KatmaiEndPrefix1
		mov eax, [ebx + __glContext.primitive_CurrentVertex]
		test eax, 3
MARK_FUNC primParts_KatmaiEndPrefix1_end
	;	jz .continue

MARK_FUNC primParts_KatmaiEndPrefix2
		call [ebx + __glContext.vertex_TransformGroupCurrent]
MARK_FUNC primParts_KatmaiEndPrefix2_end

MARK_FUNC primParts_pop_nop
	pop ebx
	ret
MARK_FUNC primParts_pop_nop_end


_fp255 dd 1.0

; Caclulate facing and culling for a triangle
; arg1 = gc
; arg2 = offset equ for A
; arg3 = offset equ for B
; arg4 = offset equ for C
%macro CALC_FACING_3_MACRO 4
%if __PROCESSOR_KATMAI__
		movss xmm2, [__glContext.vertices_WindowX + %2]
		movss xmm4, [__glContext.vertices_WindowX + %4]
		movss xmm3, [__glContext.vertices_WindowY + %2]
		movss xmm0, [__glContext.vertices_WindowY + %3]
		subss xmm4, xmm2										; xmm4= dx01
		subss xmm0, xmm3										; xmm0= dy21

		movss xmm1, [__glContext.vertices_WindowX + %3]
		movss xmm5, [__glContext.vertices_WindowY + %4]
		mulss xmm0, xmm4

		subss xmm1, xmm2										; xmm1= dx21
		subss xmm5, xmm3										; xmm5= dy01
		mulss xmm5, xmm1

		xor eax, eax
		comiss xmm5, xmm0										; area
%else
		fld dword [__glContext.vertices_WindowX + %4]
		fsub dword [__glContext.vertices_WindowX + %2]
		fld dword [__glContext.vertices_WindowY + %3]
		fsub dword [__glContext.vertices_WindowY + %2]

		fld dword [__glContext.vertices_WindowX + %3]
		fsub dword [__glContext.vertices_WindowX + %2]
		fxch st1
		fmulp st2, st0
		fld dword [__glContext.vertices_WindowY + %4]
		fsub dword [__glContext.vertices_WindowY + %2]
		fmulp st1, st0
%if __PROCESSOR_P6__
		fcomip st1
%else
		fcomp st1
		fnstsw ax
		sahf
%endif
		fstp st0
%endif


%ENDMACRO
; Caclulate facing and culling for a quad
; arg1 = gc
; arg2 = offset equ for A
; arg3 = offset equ for B
; arg4 = offset equ for C
; arg5 = offset equ for D
%macro CALC_FACING_4_MACRO 5
%if __PROCESSOR_KATMAI__
		movss xmm0, [__glContext.vertices_WindowX + %4]					; vertex 2
		movss xmm1, [__glContext.vertices_WindowY + %4]					; vertex 2
	
		movss xmm2, [__glContext.vertices_WindowX + %2]
		movss xmm3, [__glContext.vertices_WindowY + %2]
		movss xmm4, xmm0
		movss xmm5, xmm1
		movss xmm6, [__glContext.vertices_WindowX + %3]
		movss xmm7, [__glContext.vertices_WindowY + %3]
		subss xmm4, xmm2											; dx02
		subss xmm5, xmm3											; dy02
		movss xmm2, xmm0
		movss xmm3, xmm1
		subss xmm0, xmm6											; dx12
		subss xmm1, xmm7											; dy12
		subss xmm2, [__glContext.vertices_WindowX + %5]					; dx32
		subss xmm3, [__glContext.vertices_WindowY + %5]					; dy32

		mulss xmm1, xmm4											; dx02*dy12
		mulss xmm0, xmm5											; dx12*dy02

		mulss xmm3, xmm4											; dx02*dy32
		mulss xmm2, xmm5											; dx32*dy02

		subss xmm0, xmm1											; dx12*dy02 - dx02*dy12
		subss xmm3, xmm2											; dx02*dy32 - dx32*dy02
		addss xmm0, xmm3

		xorps xmm7, xmm7												; xmm7=0
		comiss xmm7, xmm0
%else
		fld dword [__glContext.vertices_WindowX + %4]
		fsub dword [__glContext.vertices_WindowX + %2]		; dx02
		fld dword [__glContext.vertices_WindowY + %4]
		fsub dword [__glContext.vertices_WindowY + %3]		; dy12  dx02

		fld dword [__glContext.vertices_WindowX + %4]
		fsub dword [__glContext.vertices_WindowX + %3]		; dx12  dy12  dx02
		fld dword [__glContext.vertices_WindowY + %4]
		fsub dword [__glContext.vertices_WindowY + %2]		; dy02  dx12  dy12  dx02
		fxch st2										; dy12  dx12  dy02  dx02
		fmul st0, st3									; dx02*dy12  dx12  dy02  dx02
		fxch st1										; dx12  dx02*dy12  dy02  dx02
		fmul st0, st2									; dx12*dy02  dx02*dy12  dy02  dx02
		
		fld dword [__glContext.vertices_WindowX + %4]		
		fsub dword [__glContext.vertices_WindowX + %5]		; dx32  dx12*dy02  dx02*dy12  dy02  dx02
		fld dword [__glContext.vertices_WindowY + %4]
		fsub dword [__glContext.vertices_WindowY + %5]		; dy32  dx32  dx12*dy02  dx02*dy12  dy02  dx02
		fxch st1										; dx32  dy32  dx12*dy02  dx02*dy12  dy02  dx02
		fmulp st4, st0									; dy32  dx12*dy02  dx02*dy12  dx32*dy02  dx02

		fxch st2										; dx02*dy12  dx12*dy02  dy32  dx32*dy02  dx02
		fsubp st1, st0									; dx12*dy02-dx02*dy12  dy32  dx32*dy02  dx02
		fxch st3										; dx02  dy32  dx32*dy02  dx12*dy02-dx02*dy12
		fmulp st1, st0									; dx02*dy32  dx32*dy02  dx12*dy02-dx02*dy12
		fsubrp st1, st0									; dx02*dy32-dx32*dy02  dx12*dy02-dx02*dy12
		faddp st1, st0
		fldz											; 0  Aera
%if __PROCESSOR_P6__
		fcomip st1
%else
		fcomp st1
		fnstsw ax
		sahf
%endif
		fstp st0
%endif
%endmacro


MARK_FUNC primParts_Clipper_Get_FillFront
		mov eax, [ebx + __glContext.procs_triangleFillFrontUnordered] 
MARK_FUNC primParts_Clipper_Get_FillFront_end

MARK_FUNC primParts_Clipper_Get_LineFront
		mov eax, [ebx + __glContext.procs_triangleLineFrontUnordered] 
MARK_FUNC primParts_Clipper_Get_LineFront_end

MARK_FUNC primParts_Clipper_Get_PointFront
		mov eax, [ebx + __glContext.procs_trianglePointFrontUnordered] 
MARK_FUNC primParts_Clipper_Get_PointFront_end

MARK_FUNC primParts_Clipper_Get_FillBack
		mov eax, [ebx + __glContext.procs_triangleFillBackUnordered] 
MARK_FUNC primParts_Clipper_Get_FillBack_end

MARK_FUNC primParts_Clipper_Get_LineBack
		mov eax, [ebx + __glContext.procs_triangleLineBackUnordered] 
MARK_FUNC primParts_Clipper_Get_LineBack_end

MARK_FUNC primParts_Clipper_Get_PointBack
		mov eax, [ebx + __glContext.procs_trianglePointBackUnordered] 
MARK_FUNC primParts_Clipper_Get_PointBack_end


MARK_FUNC primParts_Clipper_Put_Front
		mov [ebx + __glContext.primitive_ClippedPolyProcFront], eax
MARK_FUNC primParts_Clipper_Put_Front_end

MARK_FUNC primParts_Clipper_Put_Back
		mov [ebx + __glContext.primitive_ClippedPolyProcBack], eax
MARK_FUNC primParts_Clipper_Put_Back_end


;=========================================================================
; GL_TRIANGLE_FAN

MARK_FUNC primParts_Fan_Init
		push esi
		mov esi, [ebx + __glContext.primitive_StripFlag]
	;	xor esi, esi
		mov eax, [ebx + __glContext.primitive_CurrentVertex]
		
		cmp eax, 2
		jg .ok
		pop esi
		pop ebx
		ret
	
.ok
		push edi
		mov ecx, 4
		cmp eax, 12 ;+16

		push ebp
		mov ebp, [ebx + __glContext.primitive_Count]					; ct
		mov edi, [__glContext.vertices_ClipCode + ebx]					; always vx[0]
%if __PROCESSOR_P6__
		cmovz eax, ecx
%else
		jnz .noMove
		mov eax, ecx
	.noMove
%endif


		mov [ebx + __glContext.primitive_CurrentVertex], eax
		
		lea eax, [eax+4]
		mov [ebx + __glContext.primitive_CallSize], eax
		
		or edi, [__glContext.vertices_ClipCode + ebx+ebp*4+4 ]			; vx[ ct ]
MARK_FUNC primParts_Fan_Init_end


MARK_FUNC primParts_Fan_Loop1
		inc ebp
		shr esi, 1
		mov eax, 4
		cmp ebp, 12 ;+16
%if __PROCESSOR_P6__
		cmovz ebp, eax
%else
		jnz .noMove
		mov ebp, eax
	.noMove
%endif
		lea edx, [ebp+1]
		cmp edx, 12 ;+16
%if __PROCESSOR_P6__
		cmovz edx, eax
%else
		jnz .noMove2
		mov edx, eax
	.noMove2
%endif
		cmp edx, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_Fan_Loop1_end
	;	je near .endLoop
		
MARK_FUNC primParts_Fan_Loop2
		or edi, [__glContext.vertices_ClipCode + ebx+edx*4 ]				; vx[ct+1]
MARK_FUNC primParts_Fan_Loop2_end
	;	jnz near .clipped

MARK_FUNC primParts_Fan_SetProvoking
		mov [ebx + __glContext.primitive_Provoking], edx
MARK_FUNC primParts_Fan_SetProvoking_end

MARK_FUNC primParts_Fan_Facing
		CALC_FACING_3_MACRO ebx, ebx, ebx+ebp*4, ebx+edx*4
MARK_FUNC primParts_Fan_Facing_end

MARK_FUNC primParts_Fan_EWNT_Front
	push edx
		and edx, 0xC						; Do edx vertex first so we don't need to save it.
		mov eax, [ebx + __glContext.vertices_Has + edx*4]
		test eax, 1
		jnz near .noCalc2
		or eax, 1
		mov [ebx + __glContext.vertices_Has + edx*4], eax
		push edi
		lea edi, [ebx + edx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc2
		mov ecx, ebp
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc1
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc1
		mov eax, [ebx + __glContext.vertices_Has]	; Do vertex 0 last
		test eax, 1
		jnz near .noCalc0
		or eax, 1
		mov [ebx + __glContext.vertices_Has], eax
		push edi
		mov edi, ebx
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc0
	pop edx
MARK_FUNC primParts_Fan_EWNT_Front_end


MARK_FUNC primParts_Fan_EWNT_Back
		and edx, 0xC						; Do edx vertex first so we don't need to save it.
		mov eax, [ebx + __glContext.vertices_Has + edx*4]
		test eax, 2
		jnz near .noCalc2
		or eax, 2
		mov [ebx + __glContext.vertices_Has + edx*4], eax
		push edi
		lea edi, [ebx + edx*4]
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc2
		mov ecx, ebp
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 2
		jnz near .noCalc1
		or eax, 2
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc1
		mov eax, [ebx + __glContext.vertices_Has]	; Do vertex 0 last
		test eax, 2
		jnz near .noCalc0
		or eax, 2
		mov [ebx + __glContext.vertices_Has], eax
		push edi
		mov edi, ebx
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc0
MARK_FUNC primParts_Fan_EWNT_Back_end

MARK_FUNC primParts_Fan_Front_Fill
		call [ebx + __glContext.procs_triangleFillFrontFan3 + esi*4]
		mov esi, 2
MARK_FUNC primParts_Fan_Front_Fill_end
MARK_FUNC primParts_Fan_Back_Fill
		call [ebx + __glContext.procs_triangleFillBackFan3 + esi*4]
		mov esi, 2
MARK_FUNC primParts_Fan_Back_Fill_end

MARK_FUNC primParts_Fan_Front_Line
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+1]
		mov eax, 4
		cmp ecx, 12

%if __PROCESSOR_P6__
		cmovz ecx, eax
%else
		jnz .noMove
		mov ecx, eax
	.noMove
%endif

		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		
		call [ebx + __glContext.procs_triangleLineFrontFan3 + esi*4]
		mov esi, 2
		mov [ebx + __glContext.vertices_Edge], dword 0
MARK_FUNC primParts_Fan_Front_Line_end
MARK_FUNC primParts_Fan_Back_Line
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+1]
		mov eax, 4
		cmp ecx, 12

%if __PROCESSOR_P6__
		cmovz ecx, eax
%else
		jnz .noMove
		mov ecx, eax
	.noMove
%endif

		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		
		call [ebx + __glContext.procs_triangleLineBackFan3 + esi*4]
		mov esi, 2
		mov [ebx + __glContext.vertices_Edge], dword 0
MARK_FUNC primParts_Fan_Back_Line_end

MARK_FUNC primParts_Fan_Front_Point
		mov eax, [ebx + __glContext.vertices_Edge]
		mov [ebx + ebp*4 + __glContext.vertices_Edge], eax
		lea ecx, [ebp+1]
		mov eax, 4
		cmp ecx, 12

%if __PROCESSOR_P6__
		cmovz ecx, eax
%else
		jnz .noMove
		mov ecx, eax
	.noMove
%endif

		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1

		call [ebx + __glContext.procs_trianglePointFrontFan3 + esi*4]
		mov esi, 2
		mov [ebx + __glContext.vertices_Edge], dword 0
MARK_FUNC primParts_Fan_Front_Point_end
MARK_FUNC primParts_Fan_Back_Point
		mov eax, [ebx + __glContext.vertices_Edge]
		mov [ebx + ebp*4 + __glContext.vertices_Edge], eax
		lea ecx, [ebp+1]
		mov eax, 4
		cmp ecx, 12

%if __PROCESSOR_P6__
		cmovz ecx, eax
%else
		jnz .noMove
		mov ecx, eax
	.noMove
%endif

		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1

		call [ebx + __glContext.procs_trianglePointBackFan3 + esi*4]
		mov esi, 2
		mov [ebx + __glContext.vertices_Edge], dword 0
MARK_FUNC primParts_Fan_Back_Point_end
		jmp 0 ;primParts_Fan_Loop1

		
MARK_FUNC primParts_Fan_Clipped1
		mov edi, [__glContext.vertices_ClipCode + ebx]					; always vx[0]
		mov eax, edi
		mov ecx, [__glContext.vertices_ClipCode + ebx+edx*4]
		and eax, [__glContext.vertices_ClipCode + ebx+ebp*4]
		or edi, ecx
		and eax, ecx
MARK_FUNC primParts_Fan_Clipped1_end
	;	jnz near .loop

MARK_FUNC primParts_Fan_Clipped2
		sub esp, 12
		mov ecx, esp
		mov [esp], dword 0
		mov [esp+4], ebp
		mov [esp+8], edx
		mov eax, [__glContext.vertices_ClipCode + ebx+ebp*4]
		or eax, edi
		push eax							; orCode
		push dword 3
		push ecx							; &vertex[3]
		push ebx
MARK_FUNC primParts_Fan_Clipped2_end
	;	call __glDoPolygonClip

MARK_FUNC primParts_Fan_Clipped3
		add esp, 16 + 12
MARK_FUNC primParts_Fan_Clipped3_end
	;	jmp .loop

MARK_FUNC primParts_Fan_Finish
		sub ebp, 1
		mov [ebx + __glContext.primitive_Count], ebp			; ct
		shl esi, 1
		mov [ebx + __glContext.primitive_StripFlag], esi
		pop ebp
		pop edi
		pop esi
		pop ebx
		ret
MARK_FUNC primParts_Fan_Finish_end


;=========================================================================
; GL_POLYGON

MARK_FUNC primParts_Polygon_Init
;CALL_DEBUGGER
		push esi
		mov esi, [ebx + __glContext.primitive_StripFlag]
		mov eax, [ebx + __glContext.primitive_CurrentVertex]

		push edi
		mov edx, 4
		cmp eax, 12 ;+16

		push ebp
		mov ebp, [ebx + __glContext.primitive_Count]					; ct
		mov ecx, [__glContext.vertices_ClipCode + ebx]					; always vx[0]

%if __PROCESSOR_P6__
		cmovz eax, edx
%else
		jnz .noMove
		mov eax, edx
	.noMove
%endif

		mov edi, ecx

		mov [ebx + __glContext.primitive_CurrentVertex], eax
		
		lea eax, [eax+4]
		mov [ebx + __glContext.primitive_CallSize], eax
		
		or ecx, [__glContext.vertices_ClipCode + ebx+ebp*4+4 ]			; vx[ ct ]
		and edi, [__glContext.vertices_ClipCode + ebx+ebp*4+4 ]			; vx[ ct ]

ALIGN 16
.loop
		inc ebp
		mov eax, 4
		cmp ebp, 12 ;+16

%if __PROCESSOR_P6__
		cmovz ebp, eax
%else
		jnz .noMove2
		mov ebp, eax
	.noMove2
%endif

		lea edx, [ebp+1]
		cmp edx, 12 ;+16

%if __PROCESSOR_P6__
		cmovz edx, eax
%else
		jnz .noMove3
		mov edx, eax
	.noMove3
%endif

		or ecx, [__glContext.vertices_ClipCode + ebx+edx*4 ]				; vx[ct+1]
		and edi, [__glContext.vertices_ClipCode + ebx+edx*4 ]				; vx[ct+1]

		cmp edx, [ebx + __glContext.primitive_CurrentVertex]
		jne .loop

		test edi, edi
MARK_FUNC primParts_Polygon_Init_end
	;	jnz .endLoop
		
MARK_FUNC primParts_Polygon_TestClipCode
		shr esi, 1
		test ecx, ecx
MARK_FUNC primParts_Polygon_TestClipCode_end
	;	jnz .clipped

MARK_FUNC primParts_Polygon_Loop1
		mov ebp, [ebx + __glContext.primitive_Count]					; ct
MARK_FUNC primParts_Polygon_Loop1_end
	
MARK_FUNC primParts_Polygon_Loop2
		inc ebp
		mov eax, 4
		cmp ebp, 12 ;+16

%if __PROCESSOR_P6__
		cmovz ebp, eax
%else
		jnz .noMove
		mov ebp, eax
	.noMove
%endif

		lea edx, [ebp+1]
		cmp edx, 12 ;+16

%if __PROCESSOR_P6__
		cmovz edx, eax
%else
		jnz .noMove2
		mov edx, eax
	.noMove2
%endif

		cmp edx, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_Polygon_Loop2_end
	;	je near .endLoop
		
MARK_FUNC primParts_Polygon_SetProvoking
		mov [ebx + __glContext.primitive_Provoking], edx
MARK_FUNC primParts_Polygon_SetProvoking_end

MARK_FUNC primParts_Polygon_Facing
		CALC_FACING_3_MACRO ebx, ebx, ebx+ebp*4, ebx+edx*4
MARK_FUNC primParts_Polygon_Facing_end

MARK_FUNC primParts_Polygon_EWNT_Front
	push edx
		and edx, 0xC						; Do edx vertex first so we don't need to save it.
		mov eax, [ebx + __glContext.vertices_Has + edx*4]
		test eax, 1
		jnz near .noCalc2
		or eax, 1
		mov [ebx + __glContext.vertices_Has + edx*4], eax
		push edi
		lea edi, [ebx + edx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc2
		mov ecx, ebp
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc1
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc1
		mov eax, [ebx + __glContext.vertices_Has]	; Do vertex 0 last
		test eax, 1
		jnz near .noCalc0
		or eax, 1
		mov [ebx + __glContext.vertices_Has], eax
		push edi
		mov edi, ebx
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc0
	pop edx
MARK_FUNC primParts_Polygon_EWNT_Front_end


MARK_FUNC primParts_Polygon_EWNT_Back
		and edx, 0xC						; Do edx vertex first so we don't need to save it.
		mov eax, [ebx + __glContext.vertices_Has + edx*4]
		test eax, 2
		jnz near .noCalc2
		or eax, 2
		mov [ebx + __glContext.vertices_Has + edx*4], eax
		push edi
		lea edi, [ebx + edx*4]
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc2
		mov ecx, ebp
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 2
		jnz near .noCalc1
		or eax, 2
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc1
		mov eax, [ebx + __glContext.vertices_Has]	; Do vertex 0 last
		test eax, 2
		jnz near .noCalc0
		or eax, 2
		mov [ebx + __glContext.vertices_Has], eax
		push edi
		mov edi, ebx
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc0
MARK_FUNC primParts_Polygon_EWNT_Back_end

MARK_FUNC primParts_Polygon_Front_Fill
		call [ebx + __glContext.procs_triangleFillFrontFan3 + esi*4]
		mov esi, 2
MARK_FUNC primParts_Polygon_Front_Fill_end
MARK_FUNC primParts_Polygon_Back_Fill
		call [ebx + __glContext.procs_triangleFillBackFan3 + esi*4]
		mov esi, 2
MARK_FUNC primParts_Polygon_Back_Fill_end

MARK_FUNC primParts_Polygon_Front_Line
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+1]
		mov eax, 4
		cmp ecx, 12

%if __PROCESSOR_P6__
		cmovz ecx, eax
%else
		jnz .noMove
		mov ecx, eax
	.noMove
%endif

		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		
		call [ebx + __glContext.procs_triangleLineFrontFan3 + esi*4]
		mov esi, 2
		mov [ebx + __glContext.vertices_Edge], dword 0
MARK_FUNC primParts_Polygon_Front_Line_end
MARK_FUNC primParts_Polygon_Back_Line
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+1]
		mov eax, 4
		cmp ecx, 12

%if __PROCESSOR_P6__
		cmovz ecx, eax
%else
		jnz .noMove
		mov ecx, eax
	.noMove
%endif

		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		
		call [ebx + __glContext.procs_triangleLineBackFan3 + esi*4]
		mov esi, 2
		mov [ebx + __glContext.vertices_Edge], dword 0
MARK_FUNC primParts_Polygon_Back_Line_end

MARK_FUNC primParts_Polygon_Front_Point
		mov eax, [ebx + __glContext.vertices_Edge]
		mov [ebx + ebp*4 + __glContext.vertices_Edge], eax
		lea ecx, [ebp+1]
		mov eax, 4
		cmp ecx, 12

%if __PROCESSOR_P6__
		cmovz ecx, eax
%else
		jnz .noMove
		mov ecx, eax
	.noMove
%endif

		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1

		call [ebx + __glContext.procs_trianglePointFrontFan3 + esi*4]
		mov esi, 2
		mov [ebx + __glContext.vertices_Edge], dword 0
MARK_FUNC primParts_Polygon_Front_Point_end
MARK_FUNC primParts_Polygon_Back_Point
		mov eax, [ebx + __glContext.vertices_Edge]
		mov [ebx + ebp*4 + __glContext.vertices_Edge], eax
		lea ecx, [ebp+1]
		mov eax, 4
		cmp ecx, 12

%if __PROCESSOR_P6__
		cmovz ecx, eax
%else
		jnz .noMove
		mov ecx, eax
	.noMove
%endif

		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1

		call [ebx + __glContext.procs_trianglePointBackFan3 + esi*4]
		mov esi, 2
		mov [ebx + __glContext.vertices_Edge], dword 0
MARK_FUNC primParts_Polygon_Back_Point_end
	;	jmp 0 ;primParts_Polygon_Loop1

		
MARK_FUNC primParts_Polygon_Clipped1
;CALL_DEBUGGER
		sub esp, 64
		mov ebp, [ebx + __glContext.primitive_Count]					; ct
		mov [esp], dword 0
		lea eax, [ebp+1]
		mov [esp +4], eax
		mov edi, 1
.loop
		inc ebp
		inc edi
		mov eax, 4
		cmp ebp, 12 ;+16

%if __PROCESSOR_P6__
		cmovz ebp, eax
%else
		jnz .noMove
		mov ebp, eax
	.noMove
%endif

		lea edx, [ebp+1]
		cmp edx, 12 ;+16

%if __PROCESSOR_P6__
		cmovz edx, eax
%else
		jnz .noMove2
		mov edx, eax
	.noMove2
%endif

		cmp edx, [ebx + __glContext.primitive_CurrentVertex]
		mov [esp + edi*4], edx
		jne .loop

		push ecx							; orCode
		push edi							; vertex count
		lea ecx, [esp + 8]
		push ecx							; &vertex[3]
		push ebx							; gc
MARK_FUNC primParts_Polygon_Clipped1_end
	;	call __glDoPolygonClip

MARK_FUNC primParts_Polygon_Clipped2
		add esp, 16 + 64
MARK_FUNC primParts_Polygon_Clipped2_end
	;	jmp .loop

MARK_FUNC primParts_Polygon_Finish
		sub ebp, 1
		mov [ebx + __glContext.primitive_Count], ebp			; ct
		shl esi, 1
		mov [ebx + __glContext.primitive_StripFlag], esi
		pop ebp
		pop edi
		pop esi
		pop ebx
		ret
MARK_FUNC primParts_Polygon_Finish_end


;=========================================================================
; GL_QUADS


MARK_FUNC primParts_Quad_Init
		mov ecx, [__glContext.vertices_ClipCode + ebx]
		or ecx, [__glContext.vertices_ClipCode + ebx +4]
		or ecx, [__glContext.vertices_ClipCode + ebx +8]
		or ecx, [__glContext.vertices_ClipCode + ebx +12]
MARK_FUNC primParts_Quad_Init_end
	;	jnz near .clipped

MARK_FUNC primParts_Quad_SetProvoking
		mov [ebx + __glContext.primitive_Provoking], dword 3
MARK_FUNC primParts_Quad_SetProvoking_end

MARK_FUNC primParts_Quad_Facing
		CALC_FACING_4_MACRO ebx, ebx, ebx+4, ebx+8, ebx+12
MARK_FUNC primParts_Quad_Facing_end
	;	jnz near .end

MARK_FUNC primParts_Quad_EWNT_Front
		push edi
		mov edi, ebx
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
MARK_FUNC primParts_Quad_EWNT_Front_end

MARK_FUNC primParts_Quad_EWNT_Back
		push edi
		mov edi, ebx
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
MARK_FUNC primParts_Quad_EWNT_Back_end

MARK_FUNC primParts_Quad_Front_Fill
		push ebp
		mov ebp, 1
		call [ebx + __glContext.procs_triangleFillFrontFan3]
		inc ebp
		call [ebx + __glContext.procs_triangleFillFrontFan1]
		pop ebp
MARK_FUNC primParts_Quad_Front_Fill_end
MARK_FUNC primParts_Quad_Back_Fill
		push ebp
		mov ebp, 1
		call [ebx + __glContext.procs_triangleFillBackFan3]
		inc ebp
		call [ebx + __glContext.procs_triangleFillBackFan1]
		pop ebp
MARK_FUNC primParts_Quad_Back_Fill_end

MARK_FUNC primParts_Quad_Front_Line
		push ebp
		mov eax, [ebx + 8 + __glContext.vertices_Edge]
		push eax
		mov ebp, 1
		mov [ebx + 8 + __glContext.vertices_Edge], dword 0
		call [ebx + __glContext.procs_triangleLineFrontFan3]
		inc ebp
		pop eax
		mov [ebx + __glContext.vertices_Edge], dword 0
		mov [ebx + 8 + __glContext.vertices_Edge], eax
		call [ebx + __glContext.procs_triangleLineFrontFan1]
		pop ebp
MARK_FUNC primParts_Quad_Front_Line_end
MARK_FUNC primParts_Quad_Back_Line
		push ebp
		mov eax, [ebx + 8 + __glContext.vertices_Edge]
		push eax
		mov ebp, 1
		mov [ebx + 8 + __glContext.vertices_Edge], dword 0
		call [ebx + __glContext.procs_triangleLineBackFan3]
		inc ebp
		pop eax
		mov [ebx + __glContext.vertices_Edge], dword 0
		mov [ebx + 8 + __glContext.vertices_Edge], eax
		call [ebx + __glContext.procs_triangleLineBackFan1]
		pop ebp
MARK_FUNC primParts_Quad_Back_Line_end

MARK_FUNC primParts_Quad_Front_Point
		push ebp
		mov eax, [ebx + 8 + __glContext.vertices_Edge]
		push eax
		mov ebp, 1
		mov [ebx + 8 + __glContext.vertices_Edge], dword 0
		call [ebx + __glContext.procs_trianglePointFrontFan3]
		inc ebp
		pop eax
		mov [ebx + __glContext.vertices_Edge], dword 0
		mov [ebx + 8 + __glContext.vertices_Edge], eax
		call [ebx + __glContext.procs_trianglePointFrontFan1]
		pop ebp
MARK_FUNC primParts_Quad_Front_Point_end
MARK_FUNC primParts_Quad_Back_Point
		push ebp
		mov eax, [ebx + 8 + __glContext.vertices_Edge]
		push eax
		mov ebp, 1
		mov [ebx + 8 + __glContext.vertices_Edge], dword 0
		call [ebx + __glContext.procs_trianglePointBackFan3]
		inc ebp
		pop eax
		mov [ebx + __glContext.vertices_Edge], dword 0
		mov [ebx + 8 + __glContext.vertices_Edge], eax
		call [ebx + __glContext.procs_trianglePointBackFan1]
		pop ebp
MARK_FUNC primParts_Quad_Back_Point_end


MARK_FUNC primParts_Quad_Finish
		xor ecx, ecx
		mov [ebx + __glContext.primitive_CurrentVertex], ecx
		pop ebx
		ret
MARK_FUNC primParts_Quad_Finish_end

MARK_FUNC primParts_Quad_Clipped1
		mov eax, [__glContext.vertices_ClipCode + ebx]
		and eax, [__glContext.vertices_ClipCode + ebx +4]
		and eax, [__glContext.vertices_ClipCode + ebx +8]
		and eax, [__glContext.vertices_ClipCode + ebx +12]
MARK_FUNC primParts_Quad_Clipped1_end
	;	jnz near .end

MARK_FUNC primParts_Quad_Clipped2
		sub esp, 16
		mov [esp], dword 0
		mov [esp+4], dword 1
		mov [esp+8], dword 2
		mov [esp+12], dword 3

		mov eax, esp
		push ecx							; orCode
		push dword 4
		push eax							; &vertex[3]
		push ebx
MARK_FUNC primParts_Quad_Clipped2_end
	;	call __glDoPolygonClip
		
MARK_FUNC primParts_Quad_Clipped3
		add esp, 16 + 16
		xor ecx, ecx
		mov [ebx + __glContext.primitive_CurrentVertex], ecx
		pop ebx
		ret
MARK_FUNC primParts_Quad_Clipped3_end
		

;=========================================================================
; GL_QUAD_STRIP

MARK_FUNC primParts_QuadStrip_Init
		push esi
		push edi
		mov esi, [ebx + __glContext.primitive_StripFlag]

		mov eax, [ebx + __glContext.primitive_CurrentVertex]
		and eax, 15
		mov [ebx + __glContext.primitive_CurrentVertex], eax

		lea eax, [eax+4]
		mov [ebx + __glContext.primitive_CallSize], eax

		push ebp
		mov ebp, [ebx + __glContext.primitive_Count]					; ct
		sub ebp, 2
MARK_FUNC primParts_QuadStrip_Init_end


MARK_FUNC primParts_QuadStrip_Loop1
		lea edx, [ebp+4]
		lea edi, [ebp+5]
		lea ebp, [ebp+2]
		and edx, 15
		and edi, 15
		and ebp, 15
		shr esi, 1
		cmp edi, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_QuadStrip_Loop1_end
	;	je near .endLoop
	

MARK_FUNC primParts_QuadStrip_Loop2
		cmp edx, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_QuadStrip_Loop2_end
	;	je near .endLoop


MARK_FUNC primParts_QuadStrip_Loop3
		mov ecx, [__glContext.vertices_ClipCode + ebx+ebp*4]
		or ecx, [__glContext.vertices_ClipCode + ebx+ebp*4+4]
		or ecx, [__glContext.vertices_ClipCode + ebx+edx*4]
		or ecx, [__glContext.vertices_ClipCode + ebx+edi*4]
MARK_FUNC primParts_QuadStrip_Loop3_end
	;	jnz near .clipped

MARK_FUNC primParts_QuadStrip_SetProvoking
		mov [ebx + __glContext.primitive_Provoking], edi
MARK_FUNC primParts_QuadStrip_SetProvoking_end

MARK_FUNC primParts_QuadStrip_Facing
		CALC_FACING_4_MACRO ebx, ebx+ebp*4, ebx+ebp*4+4, ebx+edi*4, ebx+edx*4
MARK_FUNC primParts_QuadStrip_Facing_end
	;	jnz near .loop

MARK_FUNC primParts_QuadStrip_EWNT_Front
		mov ecx, ebp			; The 1st & 2nd vertex are always in same group.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc12
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc12
		mov ecx, edi			; The 3st & 4nd vertex are always in same group.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc34
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc34
MARK_FUNC primParts_QuadStrip_EWNT_Front_end

MARK_FUNC primParts_QuadStrip_EWNT_Back
		mov ecx, ebp			; The 1st & 2nd vertex are always in same group.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 2
		jnz near .noCalc12
		or eax, 2
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc12
		mov ecx, edi			; The 3st & 4nd vertex are always in same group.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 2
		jnz near .noCalc34
		or eax, 2
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc34
MARK_FUNC primParts_QuadStrip_EWNT_Back_end

MARK_FUNC primParts_QuadStrip_Front_Fill
		call [ebx + __glContext.procs_triangleFillFrontOdd3 + esi*4]
		inc ebp
		call [ebx + __glContext.procs_triangleFillFrontEven1]
		dec ebp
		mov esi, 2
MARK_FUNC primParts_QuadStrip_Front_Fill_end
MARK_FUNC primParts_QuadStrip_Back_Fill
		call [ebx + __glContext.procs_triangleFillBackOdd3]
		inc ebp
		call [ebx + __glContext.procs_triangleFillBackEven1]
		dec ebp
MARK_FUNC primParts_QuadStrip_Back_Fill_end
	;	jmp .loop

MARK_FUNC primParts_QuadStrip_Front_Line
		mov [ebx + ebp*4 + 4 + __glContext.vertices_Edge], dword 0
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_triangleLineFrontOdd3]
		inc ebp
		
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
		call [ebx + __glContext.procs_triangleLineFrontEven1]
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
		dec ebp
MARK_FUNC primParts_QuadStrip_Front_Line_end
MARK_FUNC primParts_QuadStrip_Back_Line
		mov [ebx + ebp*4 + 4 + __glContext.vertices_Edge], dword 0
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_triangleLineBackOdd3]
		inc ebp
		
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
		call [ebx + __glContext.procs_triangleLineBackEven1]
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
		dec ebp
MARK_FUNC primParts_QuadStrip_Back_Line_end
	;	jmp .loop

MARK_FUNC primParts_QuadStrip_Front_Point
		mov eax, [ebx + ebp*4 + __glContext.vertices_Edge]
		mov [ebx + ebp*4 + 4 + __glContext.vertices_Edge], eax
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
		call [ebx + __glContext.procs_trianglePointFrontOdd3]
		inc ebp
		
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 0
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_trianglePointFrontEven1]
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
		dec ebp
MARK_FUNC primParts_QuadStrip_Front_Point_end
MARK_FUNC primParts_QuadStrip_Back_Point
		mov eax, [ebx + ebp*4 + __glContext.vertices_Edge]
		mov [ebx + ebp*4 + 4 + __glContext.vertices_Edge], eax
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
		call [ebx + __glContext.procs_trianglePointBackOdd3]
		inc ebp
		
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 0
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_trianglePointBackEven1]
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
		dec ebp
MARK_FUNC primParts_QuadStrip_Back_Point_end
	;	jmp .loop

MARK_FUNC primParts_QuadStrip_Clipped1
		mov eax, [__glContext.vertices_ClipCode + ebx+ebp*4]
		and eax, [__glContext.vertices_ClipCode + ebx+ebp*4+4]
		and eax, [__glContext.vertices_ClipCode + ebx+edx*4]
		and eax, [__glContext.vertices_ClipCode + ebx+edi*4]
MARK_FUNC primParts_QuadStrip_Clipped1_end
	;	jnz near .loop

MARK_FUNC primParts_QuadStrip_Clipped2
		sub esp, 16
		mov [esp], ebp
		lea eax, [ebp+1]
		mov [esp+4], eax
		mov [esp+8], edi
		mov [esp+12], edx
		mov eax, esp
		push ecx							; orCode
		push dword 4
		push eax							; &vertex[3]
		push ebx
MARK_FUNC primParts_QuadStrip_Clipped2_end
	;	call __glDoPolygonClip

MARK_FUNC primParts_QuadStrip_Clipped3
		add esp, 16 + 16
MARK_FUNC primParts_QuadStrip_Clipped3_end
	;	jmp .loop


MARK_FUNC primParts_QuadStrip_Finish
		mov [ebx + __glContext.primitive_Count], ebp					; ct
		shl esi, 1
		mov [ebx + __glContext.primitive_StripFlag], esi

		pop ebp
		pop edi
		pop esi
		pop ebx
		ret
MARK_FUNC primParts_QuadStrip_Finish_end


;=========================================================================
; GL_TRIANGLES

		
MARK_FUNC primParts_Triangles_Init
		push ebp
		mov ebp, -3			; ct
MARK_FUNC primParts_Triangles_Init_end

MARK_FUNC primParts_Triangles_Loop1
		lea eax, [ebp+5]
		lea ebp, [ebp+3]
		cmp eax, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_Triangles_Loop1_end
	;	jge near .endLoop
	
MARK_FUNC primParts_Triangles_Loop2
		mov ecx, [__glContext.vertices_ClipCode + ebx+ebp*4]
		or ecx, [__glContext.vertices_ClipCode + ebx+ebp*4 +4]
		or ecx, [__glContext.vertices_ClipCode + ebx+ebp*4 +8]
MARK_FUNC primParts_Triangles_Loop2_end
	;	jnz near .clipped

MARK_FUNC primParts_Triangles_SetProvoking
		lea eax, [ebp+2]
		mov [ebx + __glContext.primitive_Provoking], eax
MARK_FUNC primParts_Triangles_SetProvoking_end

MARK_FUNC primParts_Triangles_Facing
		CALC_FACING_3_MACRO ebx, ebx+ebp*4, ebx+ebp*4+4, ebx+ebp*4+8
MARK_FUNC primParts_Triangles_Facing_end
	;	jnz near .loop
		
MARK_FUNC primParts_Triangles_EWNT_Front
		mov ecx, ebp				; Test the first vertex.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc1
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]

		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc1
		lea ecx, [ebp+2]			; Test the last vertex.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc3
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]

		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc3
MARK_FUNC primParts_Triangles_EWNT_Front_end

MARK_FUNC primParts_Triangles_EWNT_Back
		mov ecx, ebp			; The 1st & 2nd vertex are always in same group.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 2
		jnz near .noCalc12
		or eax, 2
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc12
		lea ecx, [ebp+2]			; The 3st & 4nd vertex are always in same group.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 2
		jnz near .noCalc34
		or eax, 2
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc34
MARK_FUNC primParts_Triangles_EWNT_Back_end

MARK_FUNC primParts_Triangles_Front_Fill
		call [ebx + __glContext.procs_triangleFillFront]
MARK_FUNC primParts_Triangles_Front_Fill_end
MARK_FUNC primParts_Triangles_Back_Fill
		call [ebx + __glContext.procs_triangleFillBack]
MARK_FUNC primParts_Triangles_Back_Fill_end
	;	jmp .loop

MARK_FUNC primParts_Triangles_Front_Line
		call [ebx + __glContext.procs_triangleLineFront]
MARK_FUNC primParts_Triangles_Front_Line_end
MARK_FUNC primParts_Triangles_Back_Line
		call [ebx + __glContext.procs_triangleLineBack]
MARK_FUNC primParts_Triangles_Back_Line_end
	;	jmp .loop

MARK_FUNC primParts_Triangles_Front_Point
		call [ebx + __glContext.procs_trianglePointFront]
MARK_FUNC primParts_Triangles_Front_Point_end
MARK_FUNC primParts_Triangles_Back_Point
		call [ebx + __glContext.procs_trianglePointBack]
MARK_FUNC primParts_Triangles_Back_Point_end
	;	jmp .loop
		


MARK_FUNC primParts_Triangles_Clipped1
		mov eax, [__glContext.vertices_ClipCode + ebx+ebp*4]
		and eax, [__glContext.vertices_ClipCode + ebx+ebp*4 +4]
		and eax, [__glContext.vertices_ClipCode + ebx+ebp*4 +8]
MARK_FUNC primParts_Triangles_Clipped1_end
	;	jnz near .loop

MARK_FUNC primParts_Triangles_Clipped2
		sub esp, 12
		mov edx, esp
		mov [esp], ebp
		lea eax, [ebp+1]
		mov [esp+4], eax
		lea eax, [ebp+2]
		mov [esp+8], eax
		
		push ecx							; orCode
		push dword 3
		push edx							; &vertex[3]
		push ebx
MARK_FUNC primParts_Triangles_Clipped2_end
	;	call __glDoPolygonClip

MARK_FUNC primParts_Triangles_Clipped3
		add esp, 16 + 12
MARK_FUNC primParts_Triangles_Clipped3_end
	;	jmp .loop

MARK_FUNC primParts_Triangles_Finish
		xor ecx, ecx
		pop ebp
		mov [ebx + __glContext.primitive_CurrentVertex], ecx
		pop ebx
		ret
MARK_FUNC primParts_Triangles_Finish_end


;=========================================================================
; GL_TRIANGLE_STRIPS

		
MARK_FUNC primParts_TriangleStrip_Init
		push esi
		push edi
		mov esi, [ebx + __glContext.primitive_StripFlag]

		mov eax, [ebx + __glContext.primitive_CurrentVertex]
		and eax, 15
		mov [ebx + __glContext.primitive_CurrentVertex], eax

		lea eax, [eax+4]
		mov [ebx + __glContext.primitive_CallSize], eax

		push ebp
		mov ebp, [ebx + __glContext.primitive_Count]					; ct
		dec ebp
MARK_FUNC primParts_TriangleStrip_Init_end

MARK_FUNC primParts_TriangleStrip_Loop1
		lea edi, [ebp+3]
		lea edx, [ebp+2]
		lea ebp, [ebp+1]
		and edi, 15
		and edx, 15
		and ebp, 15
		shr esi, 1
		cmp edi, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_TriangleStrip_Loop1_end
	;	je near .endLoop


MARK_FUNC primParts_TriangleStrip_Loop2
		mov ecx, [__glContext.vertices_ClipCode + ebx+ebp*4]
		or ecx, [__glContext.vertices_ClipCode + ebx+edx*4]
		or ecx, [__glContext.vertices_ClipCode + ebx+edi*4]
MARK_FUNC primParts_TriangleStrip_Loop2_end
	;	jnz near .clipped1
	
MARK_FUNC primParts_TriangleStrip_SetProvoking
		mov [ebx + __glContext.primitive_Provoking], edi
MARK_FUNC primParts_TriangleStrip_SetProvoking_end

MARK_FUNC primParts_TriangleStrip_Facing1
		CALC_FACING_3_MACRO ebx, ebx+ebp*4, ebx+edx*4, ebx+edi*4
MARK_FUNC primParts_TriangleStrip_Facing1_end
	;	jnz near .tri2
		
MARK_FUNC primParts_TriangleStrip_EWNT_Front
		mov ecx, ebp			; The 1st & 2nd vertex are always in same group.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc12
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc12
		mov ecx, edi			; The 3st & 4nd vertex are always in same group.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc34
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc34
MARK_FUNC primParts_TriangleStrip_EWNT_Front_end

MARK_FUNC primParts_TriangleStrip_EWNT_Back
		mov ecx, ebp			; The 1st & 2nd vertex are always in same group.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 2
		jnz near .noCalc12
		or eax, 2
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc12
		mov ecx, edi			; The 3st & 4nd vertex are always in same group.
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 2
		jnz near .noCalc34
		or eax, 2
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Back]
		pop edi
.noCalc34
MARK_FUNC primParts_TriangleStrip_EWNT_Back_end

MARK_FUNC primParts_TriangleStrip_Front_Fill1
		call [ebx + __glContext.procs_triangleFillFrontOdd3 + esi*4]
		mov esi, 2
MARK_FUNC primParts_TriangleStrip_Front_Fill1_end
MARK_FUNC primParts_TriangleStrip_Back_Fill1
		call [ebx + __glContext.procs_triangleFillBackOdd3 + esi*4]
		mov esi, 2
MARK_FUNC primParts_TriangleStrip_Back_Fill1_end

MARK_FUNC primParts_TriangleStrip_Front_Line1
		mov [ebx + ebp*4 + 4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_triangleLineFrontOdd3]
MARK_FUNC primParts_TriangleStrip_Front_Line1_end
MARK_FUNC primParts_TriangleStrip_Back_Line1
		mov [ebx + ebp*4 + 4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_triangleLineBackOdd3]
MARK_FUNC primParts_TriangleStrip_Back_Line1_end

MARK_FUNC primParts_TriangleStrip_Front_Point1
		mov eax, [ebx + ebp*4 + __glContext.vertices_Edge]
		mov [ebx + ebp*4 + 4 + __glContext.vertices_Edge], eax
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_trianglePointFrontOdd3]
MARK_FUNC primParts_TriangleStrip_Front_Point1_end
MARK_FUNC primParts_TriangleStrip_Back_Point1
		mov eax, [ebx + ebp*4 + __glContext.vertices_Edge]
		mov [ebx + ebp*4 + 4 + __glContext.vertices_Edge], eax
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_trianglePointBackOdd3]
MARK_FUNC primParts_TriangleStrip_Back_Point1_end

MARK_FUNC primParts_TriangleStrip_Loop3
		lea edi, [ebp+3]
		lea edx, [ebp+2]
		lea ebp, [ebp+1]
		and edi, 15
		and edx, 15
		and ebp, 15
		shr esi, 1
		cmp edi, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_TriangleStrip_Loop3_end
	;	je near .endLoop2			; We can only end here if we are in an glEnd

MARK_FUNC primParts_TriangleStrip_Loop4
		mov ecx, [__glContext.vertices_ClipCode +ebx + edi*4]
		or ecx, [__glContext.vertices_ClipCode + ebx + edx*4]
		or ecx, [__glContext.vertices_ClipCode + ebx + ebp*4]
MARK_FUNC primParts_TriangleStrip_Loop4_end
	;	jnz near .clipped2
	
MARK_FUNC primParts_TriangleStrip_Facing2
		CALC_FACING_3_MACRO ebx, ebx+ebp*4, ebx+edi*4, ebx+edx*4
MARK_FUNC primParts_TriangleStrip_Facing2_end
	;	jnz near .loop
		
MARK_FUNC primParts_TriangleStrip_Front_Fill2
		call [ebx + __glContext.procs_triangleFillFrontEven3 + esi*4]
		mov esi, 2
MARK_FUNC primParts_TriangleStrip_Front_Fill2_end
MARK_FUNC primParts_TriangleStrip_Back_Fill2
		call [ebx + __glContext.procs_triangleFillBackEven3 + esi*4]
		mov esi, 2
MARK_FUNC primParts_TriangleStrip_Back_Fill2_end

MARK_FUNC primParts_TriangleStrip_Front_Line2
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 0
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_triangleLineBackEven3]
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
MARK_FUNC primParts_TriangleStrip_Front_Line2_end
MARK_FUNC primParts_TriangleStrip_Back_Line2
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 0
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_triangleLineBackEven3]
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
MARK_FUNC primParts_TriangleStrip_Back_Line2_end

MARK_FUNC primParts_TriangleStrip_Front_Point2
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 0
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_trianglePointFrontEven3]
MARK_FUNC primParts_TriangleStrip_Front_Point2_end
MARK_FUNC primParts_TriangleStrip_Back_Point2
		mov [ebx + ebp*4 + __glContext.vertices_Edge], dword 0
		lea ecx, [ebp+1]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 0
		lea ecx, [ebp+2]
		and ecx, 0x0f
		mov [ebx + ecx*4 + __glContext.vertices_Edge], dword 1
		call [ebx + __glContext.procs_trianglePointBackEven3]
MARK_FUNC primParts_TriangleStrip_Back_Point2_end
	;	jmp .loop
		
MARK_FUNC primParts_TriangleStrip_Clipped11
		xor esi, esi
		mov eax, [__glContext.vertices_ClipCode + ebx+ebp*4]
		and eax, [__glContext.vertices_ClipCode + ebx+edx*4]
		and eax, [__glContext.vertices_ClipCode + ebx+edi*4]
MARK_FUNC primParts_TriangleStrip_Clipped11_end
	;	jnz near .tri2

MARK_FUNC primParts_TriangleStrip_Clipped12
		sub esp, 12
		mov eax, esp
		mov [esp], ebp
		mov [esp+4], edx
		mov [esp+8], edi
		
		push ecx							; orCode
		push dword 3
		push eax							; &vertex[3]
		push ebx
MARK_FUNC primParts_TriangleStrip_Clipped12_end
	;	call __glDoPolygonClip
	
MARK_FUNC primParts_TriangleStrip_Clipped13
		add esp, 16 + 12
MARK_FUNC primParts_TriangleStrip_Clipped13_end
	;	jmp .tri2


MARK_FUNC primParts_TriangleStrip_Clipped21
		xor esi, esi
		mov eax, [__glContext.vertices_ClipCode + ebx +ebp*4]
		and eax, [__glContext.vertices_ClipCode + ebx +edx*4]
		and eax, [__glContext.vertices_ClipCode + ebx +edi*4]
MARK_FUNC primParts_TriangleStrip_Clipped21_end
	;	jnz near .loop

MARK_FUNC primParts_TriangleStrip_Clipped22
		sub esp, 12
		mov eax, esp
		mov [esp], ebp
		mov [esp+4], edi
		mov [esp+8], edx
		
		push ecx							; orCode
		push dword 3
		push eax							; &vertex[3]
		push ebx
MARK_FUNC primParts_TriangleStrip_Clipped22_end
	;	call __glDoPolygonClip
	
MARK_FUNC primParts_TriangleStrip_Clipped23
		add esp, 16 + 12
MARK_FUNC primParts_TriangleStrip_Clipped23_end
	;	jmp .loop
		
MARK_FUNC primParts_TriangleStrip_Finish
		mov [ebx + __glContext.primitive_Count], ebp					; ct
		shl esi, 1
		mov [ebx + __glContext.primitive_StripFlag], esi
		pop ebp
		pop edi
		pop esi
		pop ebx
		ret
MARK_FUNC primParts_TriangleStrip_Finish_end

;=========================================================================
; GL_LINE_STRIP

MARK_FUNC primParts_LineStrip_Init
		push edi
		mov eax, [ebx + __glContext.primitive_CurrentVertex]
		and eax, 15
		mov [ebx + __glContext.primitive_CurrentVertex], eax
		lea eax, [eax+4]
		mov [ebx + __glContext.primitive_CallSize], eax

		push ebp
		mov edi, [ebx + __glContext.primitive_Count]					; ct
MARK_FUNC primParts_LineStrip_Init_end

MARK_FUNC primParts_LineStrip_Loop1
		mov ebp, edi
		inc edi
		and edi, 15
		cmp edi, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_LineStrip_Loop1_end
	;	je near .endLoop

MARK_FUNC primParts_LineStrip_Loop2
		mov ecx, [__glContext.vertices_ClipCode + ebx+ebp*4]
		or ecx, [__glContext.vertices_ClipCode + ebx+edi*4]
MARK_FUNC primParts_LineStrip_Loop2_end
	;	jnz near .clipped

MARK_FUNC primParts_LineStrip_SetProvoking
		mov [ebx + __glContext.primitive_Provoking], edi
MARK_FUNC primParts_LineStrip_SetProvoking_end

MARK_FUNC primParts_LineStrip_EWNT
		mov ecx, ebp
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc12
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc12
		lea ecx, [ebp+1]
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc34
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc34
MARK_FUNC primParts_LineStrip_EWNT_end

MARK_FUNC primParts_LineStrip_Line
		call [ebx + __glContext.procs_line]
MARK_FUNC primParts_LineStrip_Line_end
	;	jmp .loop

		
MARK_FUNC primParts_LineStrip_Clipped1
		mov eax, [__glContext.vertices_ClipCode + ebx+ebp*4]
		and eax, [__glContext.vertices_ClipCode + ebx+edi*4]
MARK_FUNC primParts_LineStrip_Clipped1_end
	;	jnz near .loop

MARK_FUNC primParts_LineStrip_Clipped2
		push ebp
		push edi
		push ebx
MARK_FUNC primParts_LineStrip_Clipped2_end
	;	call __glClipLine

MARK_FUNC primParts_LineStrip_Clipped3
		add esp, 12
MARK_FUNC primParts_LineStrip_Clipped3_end
	;	jmp .loop

MARK_FUNC primParts_LineStrip_Finish
		mov [ebx + __glContext.primitive_Count], ebp					; ct
		pop ebp
		pop edi
		pop ebx
		ret
MARK_FUNC primParts_LineStrip_Finish_end


;=========================================================================
; GL_LINES

MARK_FUNC primParts_Lines_Init
		push ebp
		mov ebp, -2
MARK_FUNC primParts_Lines_Init_end

MARK_FUNC primParts_Lines_Loop1
		lea eax, [ebp+3]
		lea ebp, [ebp+2]
		cmp eax, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_Lines_Loop1_end
	;	jge near .endLoop

MARK_FUNC primParts_Lines_Loop2
		mov ecx, [__glContext.vertices_ClipCode + ebx+ebp*4]
		or ecx, [__glContext.vertices_ClipCode + ebx+ebp*4+4]
MARK_FUNC primParts_Lines_Loop2_end
	;	jnz near .clipped

MARK_FUNC primParts_Lines_SetProvoking
		lea eax, [ebp+1]
		mov [ebx + __glContext.primitive_Provoking], eax
MARK_FUNC primParts_Lines_SetProvoking_end

MARK_FUNC primParts_Lines_EWNT
		mov ecx, ebp
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc
MARK_FUNC primParts_Lines_EWNT_end

MARK_FUNC primParts_Lines_Line
		call [ebx + __glContext.procs_line]
MARK_FUNC primParts_Lines_Line_end
	;	jmp .loop

		
MARK_FUNC primParts_Lines_Clipped1
		mov eax, [__glContext.vertices_ClipCode + ebx+ebp*4]
		and eax, [__glContext.vertices_ClipCode + ebx+ebp*4+4]
MARK_FUNC primParts_Lines_Clipped1_end
	;	jnz near .loop

MARK_FUNC primParts_Lines_Clipped2
		push ebp
		lea eax, [ebp+1]
		push eax
		push ebx
MARK_FUNC primParts_Lines_Clipped2_end
	;	call __glClipLine

MARK_FUNC primParts_Lines_Clipped3
		add esp, 12
MARK_FUNC primParts_Lines_Clipped3_end
	;	jmp .loop

MARK_FUNC primParts_Lines_Finish
		mov [ebx + __glContext.primitive_CurrentVertex], dword 0
		pop ebp
		pop ebx
		ret
MARK_FUNC primParts_Lines_Finish_end



;==================================================================
; GL_LINE_LOOP

MARK_FUNC primParts_LineLoop_CloseLoop1
		mov eax, [ebx + __glContext.primitive_CurrentVertex]
		cmp eax, [ebx + __glContext.primitive_Count]
		jge .noWrap
		
		cmp eax, 4
		jne .noWrap
		mov eax, 12
		
.noWrap
		cmp eax, 1
		jg .notEmpty
		pop ebx
		ret
.notEmpty
		lea eax, [eax-1]
		push eax
		push dword 0
		push ebx
MARK_FUNC primParts_LineLoop_CloseLoop1_end

MARK_FUNC primParts_LineLoop_CloseLoopEWNT
		mov eax, [ebx + __glContext.vertices_Has]
		test eax, 1
		jnz near .noCalc12
		or eax, 1
		mov [ebx + __glContext.vertices_Has], eax
		push edi
		mov edi, ebx
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc12
		mov ecx, [ebx + __glContext.primitive_CurrentVertex]
		dec ecx
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc34
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc34
MARK_FUNC primParts_LineLoop_CloseLoopEWNT_end

extern __glClipLine

MARK_FUNC primParts_LineLoop_CloseLoop2
		mov eax, [__glContext.vertices_ClipCode + ebx+eax*4]
		mov ecx, [__glContext.vertices_ClipCode + ebx]
		or ecx, eax
		jz .notClipped

		and eax, [__glContext.vertices_ClipCode + ebx]
		jnz .done
	
		mov eax, __glClipLine
		call eax

	.notClipped
		call [ebx + __glContext.procs_lineUnordered]
	.done
		add esp, 12
MARK_FUNC primParts_LineLoop_CloseLoop2_end
		

MARK_FUNC primParts_LineLoop_Init
		push edi
		mov eax, [ebx + __glContext.primitive_CurrentVertex]
		mov edi, 4
		cmp eax, 12

%if __PROCESSOR_P6__
		cmovz eax, edi
%else
		jnz .noMove
		mov eax, edi
	.noMove
%endif

		mov [ebx + __glContext.primitive_CurrentVertex], eax
		lea eax, [eax+4]
		mov [ebx + __glContext.primitive_CallSize], eax
		push ebp
		mov edi, [ebx + __glContext.primitive_Count]					; ct
MARK_FUNC primParts_LineLoop_Init_end

MARK_FUNC primParts_LineLoop_Loop1
		mov ebp, edi
		inc edi
		mov eax, 4
		cmp ebp, 11

%if __PROCESSOR_P6__
		cmovz edi, eax
%else
		jnz .noMove
		mov edi, eax
	.noMove
%endif

		cmp edi, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_LineLoop_Loop1_end
	;	je near .endLoop

MARK_FUNC primParts_LineLoop_Loop2
		mov ecx, [__glContext.vertices_ClipCode + ebx+ebp*4]
		or ecx, [__glContext.vertices_ClipCode + ebx+edi*4]
MARK_FUNC primParts_LineLoop_Loop2_end
	;	jnz near .clipped
	
MARK_FUNC primParts_LineLoop_SetProvoking
		mov [ebx + __glContext.primitive_Provoking], edi
MARK_FUNC primParts_LineLoop_SetProvoking_end

MARK_FUNC primParts_LineLoop_EWNT
		mov ecx, edi
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc12
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc12
		mov ecx, ebp
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc34
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc34
MARK_FUNC primParts_LineLoop_EWNT_end

MARK_FUNC primParts_LineLoop_Line
		call [ebx + __glContext.procs_lineLoop]
MARK_FUNC primParts_LineLoop_Line_end
	;	jmp .loop

		
MARK_FUNC primParts_LineLoop_Clipped1
		mov eax, [__glContext.vertices_ClipCode + ebx+ebp*4]
		and eax, [__glContext.vertices_ClipCode + ebx+edi*4]
MARK_FUNC primParts_LineLoop_Clipped1_end
	;	jnz near .loop

MARK_FUNC primParts_LineLoop_Clipped2
		push ebp
		push edi
		push ebx
MARK_FUNC primParts_LineLoop_Clipped2_end
	;	call __glClipLine

MARK_FUNC primParts_LineLoop_Clipped3
		add esp, 12
MARK_FUNC primParts_LineLoop_Clipped3_end
	;	jmp .loop

MARK_FUNC primParts_LineLoop_Finish
		mov [ebx + __glContext.primitive_Count], ebp					; ct
		pop ebp
		pop edi
		pop ebx
		ret
MARK_FUNC primParts_LineLoop_Finish_end



;=========================================================================
; GL_POINTS

MARK_FUNC primParts_Points_Init
		push ebp
		mov ebp, -1
MARK_FUNC primParts_Points_Init_end

MARK_FUNC primParts_Points_Loop1
		lea ebp, [ebp+1]
		cmp ebp, [ebx + __glContext.primitive_CurrentVertex]
MARK_FUNC primParts_Points_Loop1_end
	;	jge near .endLoop

MARK_FUNC primParts_Points_Loop2
		mov eax, [__glContext.vertices_ClipCode + ebx+ebp*4]
		test eax, eax
MARK_FUNC primParts_Points_Loop2_end
	;	jnz near .loop

MARK_FUNC primParts_Points_EWNT
		mov ecx, ebp
		and ecx, 0xC
		mov eax, [ebx + __glContext.vertices_Has + ecx*4]
		test eax, 1
		jnz near .noCalc
		or eax, 1
		mov [ebx + __glContext.vertices_Has + ecx*4], eax
		push edi
		lea edi, [ebx + ecx*4]
		call [ebx + __glContext.asmProc_EWNT_Front]
		pop edi
.noCalc
MARK_FUNC primParts_Points_EWNT_end

MARK_FUNC primParts_Points_Point
		call [ebx + __glContext.procs_point]
MARK_FUNC primParts_Points_Point_end
	;	jmp .loop

MARK_FUNC primParts_Points_Finish
		mov [ebx + __glContext.primitive_CurrentVertex], dword 0
		pop ebp
		pop ebx
		ret
MARK_FUNC primParts_Points_Finish_end




