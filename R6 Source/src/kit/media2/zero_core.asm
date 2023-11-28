;**********************************************************************
;
; zero_core.asm
;
; This code creates zeroth-order interpolator functions for different 
; input and output formats.
;
; eax	temp storage
; ebx	in_cnt
; ecx	out_cnt
; edx	i_xd
; ebp	Points to zero_state structure
; esi	Points to input buffer
; edi	Points to output buffer
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
		mov		esi,[in]
		mov		edi,[out]
		mov		ebx,[in_cnt]
		mov		ecx,[out_cnt]
		mov		edx,[i_xd]
		
		fld		dword [left_gain]
		GAIN_SCALE
		fld		dword [right_gain]
		GAIN_SCALE
		
;----------------------------------------------------------------------------
; Make sure that neither in_cnt nor out_cnt is zero
;----------------------------------------------------------------------------

		cmp		ebx,0
		je		near .exit
		
		cmp		ecx,0
		je		near .exit

;----------------------------------------------------------------------------		
; if (i_xd <= 0) get new input data
;----------------------------------------------------------------------------

		cmp		edx,0
		jle		near .get_input_samples

;============================================================================
; Main loop
;============================================================================

;----------------------------------------------------------------------------
; Make new output samples as long as the position stays between the current
; sample pair (i_xd > 0)
;----------------------------------------------------------------------------

.make_output_samples:

; *(out++) = left
		fld		dword [left]
		MIX
		STORE_SAMPLE
		add		edi,OUT_BPS		; out++

; *(out++) = right
		fld		dword [right]
		MIX
		STORE_SAMPLE
		add		edi,OUT_BPS		; out++

; i_xd -= freq_in
		sub		edx,[freq_in]

; out_cnt--
		dec		ecx
		jz		near .out_buffer_full

; if (i_xd > 0) go make more output samples
		cmp		edx,0
		jg		near .make_output_samples

;----------------------------------------------------------------------------		
; Move through the input buffer until the position (xd) is less than 1.0
;----------------------------------------------------------------------------

.get_input_samples:

; left = gain[0] * (float) *in
		LOAD_SAMPLE					; now st0 has (float) *in, 
									; st1 has right_gain, 
									; st2 has left_gain
									
		add		esi,[right_in_channel_offset]	; Mr. Pipeline - avoid address generation interlock
												; new esi value is used by LOAD_SAMPLE below
											
		fmul	st0,st2
		fstp	dword [left]
		
; right_1 = gain[1] * (float) in[channel_offset]
		LOAD_SAMPLE
		fmul	st0,st1
		fstp	dword [right]
		
; in += bytes per sample
		add		esi,IN_BPS
		
; i_xd += freq_out		
		add		edx,[freq_out]

; in_cnt--
		dec		ebx
		jz		.in_buffer_empty

; if i_xd < freq_out go to the output loop
		cmp		edx,0
		jg		near .make_output_samples
		
		jmp		.get_input_samples
	
;----------------------------------------------------------------------------		
; The input buffer is empty; make more input samples if necessary.  There
; will be at least one space left in the output buffer.
;
; The code from the make_output_samples loop is duplicated here for speed
; reasons.
;----------------------------------------------------------------------------

.in_buffer_empty:

; Punt if i_xd <= 0
		cmp		edx,0
		jle		.exit
		
; *(out++) = left
		fld		dword [left]
		MIX
		STORE_SAMPLE
		add		edi,OUT_BPS		; out++

; *(out++) = right
		fld		dword [right]
		MIX
		STORE_SAMPLE
		add		edi,OUT_BPS		; out++

; i_xd -= freq_in
		sub		edx,[freq_in]

; out_cnt--
		dec		ecx
		jz		near .exit

		jmp		.in_buffer_empty

;----------------------------------------------------------------------------		
; The output buffer is full; get more input samples if necessary.  There
; will be at least one input frame left. 
;
; The code from the get_input_samples loop is duplicated here for speed
; reasons.
;----------------------------------------------------------------------------

.out_buffer_full:

; Punt if i_xd > 0
		cmp		edx,0
		jg		.exit

; left = gain[0] * (float) *in
		LOAD_SAMPLE					; now st0 has (float) *in, 
									; st1 has right_gain, 
									; st2 has left_gain
		add		esi,[right_in_channel_offset]	; Mr. Pipeline - avoid address generation interlock
												; new esi value is used by LOAD_SAMPLE below
											
		fmul	st0,st2
		fstp	dword [left]
		
; right_1 = gain[1] * (float) in[channel_offset]
		LOAD_SAMPLE
		fmul	st0,st1
		fstp	dword [right]
		
; in += bytes per sample
		add		esi,IN_BPS
		
; i_xd += freq_out		
		add		edx,[freq_out]

; in_cnt--
		dec		ebx
		jnz		.out_buffer_full


;============================================================================
; All done; put return values in structure
;============================================================================

.exit:
		fstp	dword [temp]	; Pop right_gain
		fstp	dword [temp]	; Pop left_gain

		mov		[in_cnt],ebx
		mov		[out_cnt],ecx
		mov		[i_xd],edx

;============================================================================
; Stack cleanup and return
;============================================================================

        add		esp,LOCAL_SPACE
        pop     esi
        pop     edi
        pop     ebp
        
        retn
