; zero.inc

;**********************************************************************
;
; Data structure containing state of zeroth-order interpolator
; 
; This matches the zero_state structure defined 
; in resampler_zero.h
;
;**********************************************************************

%define freq_in					ebp+00h
%define freq_out				ebp+04h

%define left					ebp+08h
%define right					ebp+0ch

%define i_xd					ebp+010h

%define right_in_channel_offset	ebp+014h

%define left_gain				ebp+018h
%define right_gain				ebp+01ch

%define in						ebp+020h
%define in_cnt					ebp+024h
%define out						ebp+028h
%define out_cnt					ebp+02ch


;**********************************************************************
;
; Defines for parameters passed on stack & local vars
;
; ebp is used to point to the zero_state struct, so all
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
;
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

