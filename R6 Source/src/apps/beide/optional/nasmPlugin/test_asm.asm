	SECTION .text
	GLOBAL my_copy
my_copy:
	push	edi
	push	esi

	mov		edi, [esp+12]	; dst
	mov		esi, [esp+16] 	; src
	mov		ecx, [esp+20] 	; len
	
	rep		movsb
	
	pop		esi
	pop		edi
	ret
