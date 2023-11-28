;**********************************************************************
;
; sinc_core.asm
;
; This code creates sinc interpolator functions for different input
; and output formats
;
; eax	Points to sinc table
; ebx	in_cnt
; ecx	out_cnt
; edx	i_xd
; ebp	Points to sinc_state structure
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
		
		mov		eax,[curr_entry]
		mov		ebx,[in_cnt]
		mov		ecx,[out_cnt]
		mov		edx,[i_xd]
		
;----------------------------------------------------------------------------
; Make sure that the input count is greater than or equal to the # of taps
;----------------------------------------------------------------------------

		cmp		ebx,[sinc_taps]
		jl		near .exit
		
;----------------------------------------------------------------------------
; Make sure out_cnt is not zero
;----------------------------------------------------------------------------

		cmp		ecx,0
		je		near .exit
		
;----------------------------------------------------------------------------
; Go to input loop if (i_xd <= 0)
;----------------------------------------------------------------------------

		cmp		edx,0
		jle		near .get_input_samples
		
		
;============================================================================
; Main loop
;============================================================================

;----------------------------------------------------------------------------
; Make new output samples as long as the position stays between the current
; sample pair (xd < 1.0)
;----------------------------------------------------------------------------

.make_output_samples:

; Run through the sinc table and do the multiply-accumulate sum
; Do both channels at once to take advantage of pipelining
		fld		dword [esi]
		fmul	dword [eax]
		
		fld		dword [esi+4]
		fmul	dword [eax]
		
%assign i 1
%rep SINC_TAPS-1
		stereo_mac	i
%assign i i+1
%endrep

; At this point st0 = right, st = left
; Mix and store
		MIX_RIGHT
		STORE_RIGHT
		MIX_LEFT
		STORE_LEFT

; out += bytes per frame
		add		edi,OUT_BPF
		
; curr_entry += bytes_per_table_entry
		add		eax,[bytes_per_table_entry]
		
; if (curr_entry == sinc_table_end), reset curr_entry
		cmp		eax,[sinc_table_end]
		jl		.bump_i_xd		
		mov		eax,[sinc_table]

; i_xd -= freq_in

.bump_i_xd:
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
		
; in += 2 * sizeof(float)
		add		esi,8
		
; i_xd += freq_out		
		add		edx,[freq_out]

; in_cnt--
		dec		ebx
		cmp		ebx,[sinc_taps]
		jl		.exit

; if (i_xd > 0) go to the output loop
		cmp		edx,0
		jg		near .make_output_samples
		
		jmp		.get_input_samples


;----------------------------------------------------------------------------		
; The output buffer is full; get more input samples if necessary.  There
; will be at least one input frame left. 
;
; The code from the get_input_samples loop is duplicated here for speed
; reasons.
;----------------------------------------------------------------------------

.out_buffer_full:

; Punt if (i_xd > 0)
		cmp		edx,0
		jg		.exit

; in += 2 * sizeof(float)
		add		esi,8
		
; i_xd += freq_out		
		add		edx,[freq_out]

; in_cnt--
		dec		ebx
		cmp		ebx,[sinc_taps]	
		jge		.out_buffer_full


;============================================================================
; All done; copy samples from end of input buffer to 
; start of input bufer and put return values in structure
;============================================================================

.exit:

; Save state variables
		mov		[curr_entry],eax
		mov		[in_cnt],ebx
		mov		[out_cnt],ecx
		mov		[i_xd],edx
		
; Copy samples
		mov		edi,[in]
		
.copy_samples:	; hardcode this loop for 17 taps

; Left		
		mov		eax,dword [esi]
		add		esi,4
		mov		dword [edi],eax
		add		edi,4

; Right
		mov		eax,dword [esi]
		add		esi,4
		mov		dword [edi],eax
		add		edi,4
		
		dec		ebx
		jg		.copy_samples

;============================================================================
; Stack cleanup and return
;============================================================================

.cleanup:
        add		esp,LOCAL_SPACE
        pop     esi
        pop     edi
        pop     ebp

        retn    

