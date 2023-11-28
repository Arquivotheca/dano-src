; lowpass.asm - assembly version of IIR lowpass filter
;

SECTION .text

%include "sinc_lowpass.inc"
%include "asmdebug.inc"
%include "convert.inc"


;**********************************************************************
;
; decimate_float_to_float_mix
; 
;**********************************************************************

%define CORE_LABEL		decimate_float_to_float_mix
%define OUT_BPF			8					; bytes per frame
%define STORE_LEFT		STORE_FLOAT_LEFT
%define STORE_RIGHT		STORE_FLOAT_RIGHT
%define MIX_LEFT		MIX_FLOAT_LEFT
%define MIX_RIGHT		MIX_FLOAT_RIGHT

%include "decimate_core.asm"


;**********************************************************************
;
; decimate_float_to_float
; 
;**********************************************************************

%define CORE_LABEL		decimate_float_to_float
%define OUT_BPF			8					; bytes per frame
%define STORE_LEFT		STORE_FLOAT_LEFT
%define STORE_RIGHT		STORE_FLOAT_RIGHT
%define MIX_LEFT
%define MIX_RIGHT

%include "decimate_core.asm"


;**********************************************************************
;
; lowpass_iir
;
; eax	Points to "a" coefficients
; ebx	Points to "b" coefficients
; ecx	Number of frames to filter
; edx	Points to IIR output history tail
; ebp	Points to sinc_state structure
; esi	Points to oversampled data
; edi	Points to end of hist buffer
; 
;**********************************************************************

GLOBAL lowpass_iir

lowpass_iir:

;============================================================================
; Stack setup
;============================================================================
		
		push    ebp
        push    edi
        push    esi
        sub     esp, LOCAL_SPACE
        
;============================================================================
; Init
;============================================================================

		mov		ebp,[p_state]
		mov		esi,[over]

;----------------------------------------------------------------------------
; Figure out how many frames to filter
;----------------------------------------------------------------------------
		
		mov		ecx,[out_cnt]
		shl		ecx,1			; need twice as many frames as caller wants
								; for later decimation
		mov		ebx,[over_cnt]
		sub		ebx,IIR_TAPS - 1 
		cmp		ecx,ebx
		jle		.check_hist_cnt
		mov		ecx,ebx
		
.check_hist_cnt:
		mov		ebx,[hist_size]
		sub		ebx,[hist_cnt]	; figure out free space in hist buffer
		cmp		ecx,ebx
		jle		.iir_cnt_done
		mov		ecx,ebx
		
.iir_cnt_done:
        mov		[iir_cnt],ecx
        
;----------------------------------------------------------------------------
; Figure out where to put newly filtered samples
;----------------------------------------------------------------------------

		mov		edi,[hist]
		mov		ebx,[hist_cnt]
		shl		ebx,3			; shift by one to convert to stereo
								; and two more to convert frames->bytes
		add		edi,ebx

;----------------------------------------------------------------------------
; Figure out IIR output history tail
;----------------------------------------------------------------------------

		mov		edx,edi
		mov		eax,IIR_TAPS - 1
		shl		eax,3			; convert frames to stereo bytes
		sub		edx,eax
		
;----------------------------------------------------------------------------
; Other misc setup
;----------------------------------------------------------------------------

		mov		eax,[coeff_a]
		mov		ebx,[coeff_b]

;============================================================================
; IIR filter loop
;============================================================================

.iir_loop:
		fld		dword [esi]		; Left b[0] * over[0]
		fmul	dword [ebx]
		
		fld		dword [esi+4]	; Right b[0] * over[1]
		fmul	dword [ebx]
		
%assign i 1
%rep IIR_TAPS-1
		stereo_mac	i
%assign i i+1
%endrep

%assign i 1
%rep IIR_TAPS-2
		stereo_msub	i
%assign i i+1
%endrep

; At this point st0 = right, st = left
		fstp	dword [edi+4]
		fstp	dword [edi]
		
		add		esi,8
		add		edx,8
		add		edi,8
		
		dec		ecx
		jg		near .iir_loop		

;============================================================================
; Tidy up
;============================================================================

;----------------------------------------------------------------------------
; Buffer housekeeping
;----------------------------------------------------------------------------
	
		mov		eax,[hist_cnt]
		add		eax,[iir_cnt]
		mov		dword [hist_cnt],eax
		
		mov		ecx,[over_cnt]
		sub		ecx,[iir_cnt]
		mov		dword [over_cnt],ecx
		
;----------------------------------------------------------------------------
; Move data at end of over buffer to start of over buffer
;----------------------------------------------------------------------------

		mov		edi,[over]
		shl		ecx,1			; convert frames to stereo

.over_move_loop:
		mov		ebx,dword [esi]
		add		esi,4
		mov		dword [edi],ebx
		add		edi,4
		
		loop	.over_move_loop


;============================================================================
; Stack cleanup and return
;============================================================================

.cleanup:
        add		esp,LOCAL_SPACE
        pop     esi
        pop     edi
        pop     ebp

        retn    

