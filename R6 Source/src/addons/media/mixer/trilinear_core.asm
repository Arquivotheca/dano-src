;**********************************************************************
;
; trilinear_core.asm
;
; This code creates trilinear interpolator functions for different input
; and output formats
;
; eax	temp storage
; ebx	in_cnt
; ecx	out_cnt
; edx	i_xd
; ebp	Points to trilinear_state structure
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
; Calculate fl_xd
;----------------------------------------------------------------------------
		fild	dword [freq_out]
		fild	dword [i_xd]
		fsubp	st1,st0
		fmul	qword [freq_out_reciprocal]
	
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
; sample pair (xd < 1.0)
;
;	yd = 0.5 * ( (y1 + (y1-y0)*xd)*(1-xd) +
;				 (y1 + (y2-y1)*xd) +
;				 (y2 + (y3-y2)*(xd-1))*xd )
;
; For simplicity, rearrage this as yd = 0.5 * (P + Q + R) where
; 
; P = (y1 + (y1 - y0) * xd)*(1 - xd)
; Q = y1 + (y2 - y1) * xd
; R = (y2 + (y3 - y2)*(xd - 1))*xd
;
;----------------------------------------------------------------------------

.make_output_samples:

; *(out++) = result of large complicated formula above

; Compute R for left channel
		fld		dword [one]
		fsub	st0,st1
		fchs					; st0 now has xd - 1
		fld		dword [left3]
		fsub	dword [left2]	; st0 now has left3-left2, st1 has xd-1
		fmulp	st1,st0
		fadd	dword [left2]	; st0 almost has R, st1 has xd
		fmul	st0,st1			; st0 has R, st1 has xd (4 regs used on stack)
		
; Compute P for left channel
		fld		dword [left1]
		fsub	dword [left0]	; st0 has left-left0, st1 has R, st2 has xd
		fmul	st0,st2
		fadd	dword [left1]
		fld1					; st0 has 1.0, st1 has most of P, st2 has R, st3 has xd
		fsub	st0,st3
		fmulp	st1,st0			; st0 has P, st1 has R, st2 has xd
	
; Add P and R
		faddp	st1,st0			; st0 has P+R, st1 has xd
		
; Compute Q
		fld		dword [left2]
		fsub	dword [left1]	; st0 has left2-left1, st1 has P+R, st2 has xd
		fmul	st0,st2
		fadd	dword [left1]

; Add Q to P+R
		faddp	st1,st0
		
; Multiply the whole thing by 0.5
		fmul	dword [one_half]

; Mix and store left channel
		MIX
		STORE_SAMPLE
		add		edi,OUT_BPS		; out++
		
; Now, the same stuff for the right channel

; Compute R for right channel
		fld		dword [one]
		fsub	st0,st1
		fchs					; st0 now has xd - 1
		fld		dword [right3]
		fsub	dword [right2]	; st0 now has right3-right2, st1 has xd-1
		fmulp	st1,st0
		fadd	dword [right2]	; st0 almost has R, st1 has xd
		fmul	st0,st1			; st0 has R, st1 has xd (4 regs used on stack)
		
; Compute P for right channel
		fld		dword [right1]
		fsub	dword [right0]	; st0 has right-right0, st1 has R, st2 has xd
		fmul	st0,st2
		fadd	dword [right1]
		fld1					; st0 has 1.0, st1 has most of P, st2 has R, st3 has xd
		fsub	st0,st3
		fmulp	st1,st0			; st0 has P, st1 has R, st2 has xd
	
; Add P and R
		faddp	st1,st0			; st0 has P+R, st1 has xd
		
; Compute Q
		fld		dword [right2]
		fsub	dword [right1]	; st0 has right2-right1, st1 has P+R, st2 has xd
		fmul	st0,st2
		fadd	dword [right1]

; Add Q to P+R
		faddp	st1,st0
		
; Multiply the whole thing by 0.5
		fmul	dword [one_half]

; Mix and store right channel
		MIX
		STORE_SAMPLE
		add		edi,OUT_BPS		; out++

; fl_xd += fl_xd_step (xd in st0)
		fadd	qword [fl_xd_step]

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

; Move forward one sample pair; shift the cascade down by one (OK to use eax 
; for floating-point values here since this is just a copy).
		mov		eax,dword [left1]	; left0 = left1
		mov		dword [left0],eax
		mov		eax,dword [left2]	; left1 = left2
		mov		dword [left1], eax
		mov		eax,dword [left3]	; left2 = left3
		mov		dword [left2], eax
		
		mov		eax,dword [right1]	; right0 = right1
		mov		dword [right0],eax
		mov		eax,dword [right2]	; right1 = right2
		mov		dword [right1],eax
		mov		eax,dword [right3]	; right2 = right3
		mov		dword [right2],eax

; left3 = gain[0] * (float) *in
		LOAD_SAMPLE					; now st0 has (float) *in, 
									; st1 has fl_xd
									; st2 has right_gain, 
									; st3 has left_gain
									
		add		esi,[right_in_channel_offset]	; Mr. Pipeline - avoid address generation interlock
												; new esi value is used by LOAD_SAMPLE below
											
		fmul	st0,st3
		fstp	dword [left3]
		
; right3 = gain[1] * (float) in[channel_offset]
		LOAD_SAMPLE
		fmul	st0,st2
		fstp	dword [right3]
		
; in += bytes per sample
		add		esi,IN_BPS
		
; i_xd += freq_out		
		add		edx,[freq_out]

; xd -= 1.0
		fsub	dword [one]

; in_cnt--
		dec		ebx
		jz		.in_buffer_empty

; if (i_xd > 0) go to the output loop
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

; Punt if (i_xd <= 0)
		cmp		edx,0
		jle		near .exit

; *(out++) = result of large complicated formula above

; Compute R for left channel
		fld		dword [one]
		fsub	st0,st1
		fchs					; st0 now has xd - 1
		fld		dword [left3]
		fsub	dword [left2]	; st0 now has left3-left2, st1 has xd-1
		fmulp	st1,st0
		fadd	dword [left2]	; st0 almost has R, st1 has xd
		fmul	st0,st1			; st0 has R, st1 has xd (4 regs used on stack)
		
; Compute P for left channel
		fld		dword [left1]
		fsub	dword [left0]	; st0 has left-left0, st1 has R, st2 has xd
		fmul	st0,st2
		fadd	dword [left1]
		fld1					; st0 has 1.0, st1 has most of P, st2 has R, st3 has xd
		fsub	st0,st3
		fmulp	st1,st0			; st0 has P, st1 has R, st2 has xd
	
; Add P and R
		faddp	st1,st0			; st0 has P+R, st1 has xd
		
; Compute Q
		fld		dword [left2]
		fsub	dword [left1]	; st0 has left2-left1, st1 has P+R, st2 has xd
		fmul	st0,st2
		fadd	dword [left1]

; Add Q to P+R
		faddp	st1,st0
		
; Multiply the whole thing by 0.5
		fmul	dword [one_half]

; Mix and store left channel
		MIX
		STORE_SAMPLE
		add		edi,OUT_BPS		; out++
		
; Now, the same stuff for the right channel

; Compute R for right channel
		fld		dword [one]
		fsub	st0,st1
		fchs					; st0 now has xd - 1
		fld		dword [right3]
		fsub	dword [right2]	; st0 now has right3-right2, st1 has xd-1
		fmulp	st1,st0
		fadd	dword [right2]	; st0 almost has R, st1 has xd
		fmul	st0,st1			; st0 has R, st1 has xd (4 regs used on stack)
		
; Compute P for right channel
		fld		dword [right1]
		fsub	dword [right0]	; st0 has right-right0, st1 has R, st2 has xd
		fmul	st0,st2
		fadd	dword [right1]
		fld1					; st0 has 1.0, st1 has most of P, st2 has R, st3 has xd
		fsub	st0,st3
		fmulp	st1,st0			; st0 has P, st1 has R, st2 has xd
	
; Add P and R
		faddp	st1,st0			; st0 has P+R, st1 has xd
		
; Compute Q
		fld		dword [right2]
		fsub	dword [right1]	; st0 has right2-right1, st1 has P+R, st2 has xd
		fmul	st0,st2
		fadd	dword [right1]

; Add Q to P+R
		faddp	st1,st0
		
; Multiply the whole thing by 0.5
		fmul	dword [one_half]

; Mix and store right channel
		MIX
		STORE_SAMPLE
		add		edi,OUT_BPS		; out++

; fl_xd += fl_xd_step (xd in st0)
		fadd	qword [fl_xd_step]

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

; Punt if (i_xd > 0)
		cmp		edx,0
		jg		.exit

; Move forward one sample pair; shift the cascade down by one (OK to use eax 
; for floating-point values here since this is just a copy).
		mov		eax,dword [left1]	; left0 = left1
		mov		dword [left0],eax
		mov		eax,dword [left2]	; left1 = left2
		mov		dword [left1], eax
		mov		eax,dword [left3]	; left2 = left3
		mov		dword [left2], eax
		
		mov		eax,dword [right1]	; right0 = right1
		mov		dword [right0],eax
		mov		eax,dword [right2]	; right1 = right2
		mov		dword [right1],eax
		mov		eax,dword [right3]	; right2 = right3
		mov		dword [right2],eax

; left3 = gain[0] * (float) *in
		LOAD_SAMPLE					; now st0 has (float) *in, 
									; st1 has fl_xd
									; st2 has right_gain, 
									; st3 has left_gain
									
		add		esi,[right_in_channel_offset]	; Mr. Pipeline - avoid address generation interlock
												; new esi value is used by LOAD_SAMPLE below
											
		fmul	st0,st3
		fstp	dword [left3]
		
; right3 = gain[1] * (float) in[channel_offset]
		LOAD_SAMPLE
		fmul	st0,st2
		fstp	dword [right3]
		
; in += bytes per sample
		add		esi,IN_BPS
		
; i_xd += freq_out		
		add		edx,[freq_out]

; xd -= 1.0
		fsub	dword [one]

; in_cnt--
		dec		ebx
		jnz		.out_buffer_full


;============================================================================
; All done; put return values in structure
;============================================================================

.exit:
		fstp	dword [temp]	; Pop fl_xd
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
