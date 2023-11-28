; assembly debug macros

EXTERN printf
EXTERN pfloat,pint,phex,pcr

%macro print_cr 0
	pusha
	sub 	esp, 40
	mov		eax,pcr
	mov		[esp],eax
	call 	printf
	add		esp,40
	popa
%endmacro

%macro print_float 0
	pusha
	sub 	esp, 40
	fst		qword [esp+4]
	mov		eax,pfloat
	mov		[esp],eax
	call 	printf
	add		esp,40
	popa
%endmacro

%macro print_float_cr 0
	pusha
	sub 	esp, 40
	fst		qword [esp+4]
	mov		eax,pfloat
	mov		[esp],eax
	call 	printf
	add		esp,40
	popa
	print_cr
%endmacro

%macro print_float_pop 0
	pusha
	sub 	esp, 40
	fstp	qword [esp+4]
	mov		eax,pfloat
	mov		[esp],eax
	call 	printf
	add		esp,40
	popa
%endmacro

%macro print_float_pop_cr 0
	pusha
	sub 	esp, 40
	fstp	qword [esp+4]
	mov		eax,pfloat
	mov		[esp],eax
	call 	printf
	add		esp,40
	popa
	print_cr
%endmacro

%macro print_int 0
	pusha
	sub 	esp, 40
	mov		dword [esp+4],eax
	mov		eax,pint
	mov		[esp],eax
	call 	printf
	add		esp,40
	popa
%endmacro

%macro print_int_cr 0
	print_int
	print_cr
%endmacro

%macro print_hex 0
	pusha
	sub 	esp, 40
	mov		dword [esp+4],eax
	mov		eax,phex
	mov		[esp],eax
	call 	printf
	add		esp,40
	popa
%endmacro

%macro print_hex_cr 0
	print_hex
	print_cr
%endmacro

