; sinc.inc

; The sinc code uses EAX

%define PRESERVE_EAX	1

%define SINC_TAPS		17

;**********************************************************************
;
; Data structure containing state of sinc interpolator
; 
; This matches the sinc_state structure defined 
; in resampler_sinc.h
;
;**********************************************************************

%define freq_in					ebp+00h
%define freq_out				ebp+04h

%define sinc_taps				ebp+08h
%define sinc_table				ebp+0ch		; ptr to sinc table start
%define curr_entry				ebp+010h	; ptr to current entry in sinc table
%define sinc_table_end			ebp+014h	; ptr to end of sinc table
%define bytes_per_table_entry	ebp+018h	; = sinc_taps * sizeof(float)

%define i_xd					ebp+01ch

%define in						ebp+020h
%define in_cnt					ebp+024h
%define out						ebp+028h
%define out_cnt					ebp+02ch


;**********************************************************************
;
; Defines for parameters passed on stack & local vars
;
; ebp is used to point to the sinc_state struct, so all
; stack offsets must be calculated based off of esp.
;
; When first entering this routine, the stack looks like this:
;
; 4		p_state
; 0		return address
;
; Then, push ebp,edi,esi
; 
; 10	p_state
; c		return addr
; 8		ebp
; 4		edi
; 0	 	esi
;
; and then subtract 16 to make room for local variables.
; 20	p_state
; 1c	return addr
; 18	ebp
; 14	edi
; 10 	esi
; 0-f	local storage
; 
;**********************************************************************

%define LOCAL_SPACE		010h
%define p_state			esp+020h
%define temp			esp


;**********************************************************************
;
; multiply-accumulate macro
;
; The parameter is the tap number
;
;**********************************************************************

%macro stereo_mac 1
	fld		dword [esi + %1*8]
	fmul	dword [eax + %1*4]		; stack holds L' R L
	
	fld		dword [esi + %1*8+4]
	fmul	dword [eax + %1*4]		; stack holds R' L' R L
	
	fxch	st0,st1					; L' R' R L
	faddp	st3,st0					; R' R L
	faddp	st1,st0					; R L
%endmacro
