; sinc_lowpass.inc

; The sinc code uses EAX

%define PRESERVE_EAX	1

%define IIR_TAPS		13

;**********************************************************************
;
; Data structure containing state of sinc+lowpass interpolator
; 
; This matches the sinc_lowpass_state structure defined 
; in sinc_lowpass_state.h
;
;**********************************************************************

%define over					ebp+00h		; ptr to oversampled data
%define over_cnt				ebp+04h		; # of frames in over

%define hist					ebp+08h		; ptr to IIR output history data
%define hist_cnt				ebp+0ch		; # of frames stored in hist
%define hist_size				ebp+10h		; size of hist buffer in frames

%define coeff_a					ebp+14h		; ptr to "a" coefficients
%define coeff_b					ebp+18h		; ptr to "b" coefficients

%define out						ebp+1ch
%define out_cnt					ebp+20h


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
%define iir_cnt			esp+4
%define decimate_cnt	esp+8


;**********************************************************************
;
; multiply-accumulate macro
;
; The parameter is the tap number
;
;**********************************************************************

%macro stereo_mac 1
	fld		dword [esi + %1*8]
	fmul	dword [ebx + %1*4]		; stack holds L' R L
	
	fld		dword [esi + %1*8+4]
	fmul	dword [ebx + %1*4]		; stack holds R' L' R L
	
	fxch	st0,st1					; L' R' R L
	faddp	st3,st0					; R' R L
	faddp	st1,st0					; R L
%endmacro


;**********************************************************************
;
; multiply-subtract macro
;
; The parameter is the tap number
;
;**********************************************************************

%macro stereo_msub 1
	fld		dword [edx + %1*8]
	fmul	dword [eax + %1*4]		; stack holds L' R L
	
	fld		dword [edx + %1*8+4]
	fmul	dword [eax + %1*4]		; stack holds R' L' R L
	
	fxch	st0,st1					; L' R' R L
	fsubp	st3,st0					; R' R L
	fsubp	st1,st0					; R L
%endmacro
