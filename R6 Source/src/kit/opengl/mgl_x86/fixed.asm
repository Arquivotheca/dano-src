%include "asm.inc"


%MACRO MARK_FUNC 1
	ALIGN 16
	GLOBAL %1
	%1:
%ENDMACRO	

SECTION .data progbits alloc noexec write align=16
fpVal_65536:	dd 65536.0

SECTION .text


;GLint fixedMul_16_16( GLint p1, GLint p2 );
MARK_FUNC fixedMul_16_16
		mov eax, [esp + 4]
		imul dword [esp + 8]
		shrd eax, edx, 16
		ret



;GLint floatToFixed_16_16( float f );
MARK_FUNC floatToFixed_16_16
		fld dword [esp + 4]
		fmul dword [fpVal_65536]
		push eax
		fistp dword [esp]
		pop eax
		ret



