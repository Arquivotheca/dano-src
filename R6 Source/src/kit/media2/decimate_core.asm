;**********************************************************************
;
; decimate_core.asm
;
; This code creates decimate functions.  Used with sinc+lowpass
; resampler
;
; eax	
; ebx	
; ecx	Loop counter
; edx	
; ebp	Pointer to sinc_lowpass_state
; esi	Pointer to IIR output history
; edi	Pointer to output data
;
;**********************************************************************

GLOBAL	CORE_LABEL

CORE_LABEL:

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
		mov		esi,[hist]
		mov		edi,[out]
		
;----------------------------------------------------------------------------
; Figure out how many frames to decimate		
;----------------------------------------------------------------------------

		mov		ecx,[out_cnt]
		shl		ecx,1
		
		mov		eax,[hist_cnt]
		sub		eax,IIR_TAPS - 2
		
		cmp		ecx,eax
		jle		.set_decimate_cnt
		mov		ecx,eax		

.set_decimate_cnt:
		and		ecx,0fffffffeh	; round loop count down to nearest two frames
		
		mov		dword [decimate_cnt],ecx

;============================================================================
; Decimate loop
;============================================================================

		shr		ecx,1
		
.decimate_loop:
		fld		dword [esi]
		MIX_LEFT
		fld		dword [esi+4]
		MIX_RIGHT
		
		add		esi,8
		
		fxch	st0,st1
		STORE_LEFT
		STORE_RIGHT
		
		add		edi,OUT_BPF
		
		loop	.decimate_loop

;============================================================================
; Cleanup
;============================================================================

;----------------------------------------------------------------------------
; Buffer housekeeping
;----------------------------------------------------------------------------

		mov		eax,[hist_cnt]
		mov		ecx,dword [decimate_cnt]
		sub		eax,ecx
		
		shr		ecx,1		; adjust for out_cnt
		
		mov		dword [hist_cnt],eax
		
		mov		ebx,[out_cnt]
		sub		ebx,ecx
		mov		dword [out_cnt],ebx

;----------------------------------------------------------------------------
; Move data at end of hist buffer to start of hist buffer
;----------------------------------------------------------------------------

		mov		edi,[hist]		
		shl		ecx,2
		
.hist_move_loop:
	
		mov		ebx,dword [esi]
		add		esi,4
		mov		dword [edi],ebx
		add		edi,4
		
		loop	.hist_move_loop
	
		
;============================================================================
; Stack cleanup and return
;============================================================================

.cleanup:
        add		esp,LOCAL_SPACE
        pop     esi
        pop     edi
        pop     ebp

        retn    

