;**********************************************************************
;
; Macros for sample format conversion
;
;**********************************************************************

;----------------------------------------------------------------------
; 
; Convert and load samples into the floating-point stack
;
;----------------------------------------------------------------------

%macro LOAD_UINT8 0

%ifdef PRESERVE_EAX
	push 	eax
%endif

	xor		ax,ax
	mov		ah,byte [esi]
	xor		ax,08000h	; reverse the sign bit to convert unsigned->signed
	mov		word [temp],ax
	
	fild	word [temp]

%ifdef PRESERVE_EAX
	pop		eax
%endif

%endm
		
%define LOAD_INT16		fild	word [esi]
%define LOAD_INT32		fild	dword [esi]
%define LOAD_FLOAT		fld		dword [esi]


;----------------------------------------------------------------------
; 
; Convert, pop, and store samples from the floating-point stack
;
;----------------------------------------------------------------------

;
; Unsigned 8 bit
;

%macro STORE_UINT8 0

%ifdef PRESERVE_EAX
	push 	eax
%endif

	xor		ax,ax
	fistp	word [temp]
	mov		ax,word [temp]
	xor		ax,08000h	; convert from signed->unsigned
	mov		byte [edi],ah
	
%ifdef PRESERVE_EAX
	pop		eax
%endif

%endm

%define STORE_UINT8_LEFT	STORE_UINT8

%macro STORE_UINT8_RIGHT	0

%ifdef PRESERVE_EAX
	push 	eax
%endif

	xor		ax,ax
	fistp	word [temp]
	mov		ax,word [temp]
	xor		ax,08000h	; convert from signed->unsigned
	mov		byte [edi + 1],ah

%ifdef PRESERVE_EAX
	pop		eax
%endif

%endm


;
; 16 bit
;

%define STORE_INT16			fistp	word [edi]
%define STORE_INT16_LEFT	STORE_INT16
%define STORE_INT16_RIGHT	fistp	word [edi + 2]


;
; 32 bit
;

%define STORE_INT32			fistp	dword [edi]
%define STORE_INT32_LEFT	STORE_INT32
%define STORE_INT32_RIGHT	fistp	dword [edi + 4]


;
; Floating-point
;

%define STORE_FLOAT			fstp	dword [edi]
%define STORE_FLOAT_LEFT	STORE_FLOAT
%define STORE_FLOAT_RIGHT	fstp	dword [edi + 4]


;----------------------------------------------------------------------
; 
; Convert and mix samples with the top of the floating-point stack
;
;----------------------------------------------------------------------

;
; Unsigned 8 bit
;

%macro MIX_UINT8 0

%ifdef PRESERVE_EAX
	push 	eax
%endif

	xor		ax,ax
	mov		ah,byte [edi]
	xor		ax,08000h
	mov		word [temp],ax
	fild	word [temp]
	faddp	st1,st0

%ifdef PRESERVE_EAX
	pop		eax
%endif

%endm

%define MIX_UINT8_LEFT	MIX_UINT8

%macro MIX_UINT8_RIGHT 0

%ifdef PRESERVE_EAX
	push 	eax
%endif

	xor		ax,ax
	mov		ah,byte [edi+1]
	xor		ax,08000h
	mov		word [temp],ax
	fild	word [temp]
	faddp	st1,st0

%ifdef PRESERVE_EAX
	pop		eax
%endif

%endm


;
; 16 bit
;

%macro MIX_INT16 0
	fild	word [edi]
	faddp	st1,st0
%endm

%define MIX_INT16_LEFT	MIX_INT16

%macro MIX_INT16_RIGHT 0
	fild	word [edi + 2]
	faddp	st1,st0
%endm


;
; 32 bit
;

%macro MIX_INT32 0
	fild	dword [edi]
	faddp	st1,st0
%endm

%define MIX_INT32_LEFT	MIX_INT32

%macro MIX_INT32_RIGHT 0
	fild	word [edi + 4]
	faddp	st1,st0
%endm


;
; Floating-point
;

%define MIX_FLOAT 			fadd	dword [edi]
%define MIX_FLOAT_LEFT		MIX_FLOAT
%define MIX_FLOAT_RIGHT 	fadd	dword [edi+4]


;----------------------------------------------------------------------
; 
; Scale a gain value on the top of the floating-point stack
;
; For 8 bit samples, the 16 bit value is used; this is because
; the 8 bit samples are converted to 16 bit and then converted
; to signed.
; 
; There's no need to gain scale if the input format is float
;
;----------------------------------------------------------------------

%define GAIN_SCALE_UINT8	fmul	dword [scale_int16]
%define GAIN_SCALE_INT16	fmul	dword [scale_int16]
%define GAIN_SCALE_INT32	fmul	dword [scale_int32]

EXTERN scale_int16,scale_int32
	
