%ifndef __ASM_INC__
%define __ASM_INC__

%MACRO EXPORT_FUNC 1
	ALIGN 16
	GLOBAL %1
	%1:
%ENDMACRO	


%endif