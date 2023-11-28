; linear.inc

;**********************************************************************
;
; Data structure containing state of linear interpolator
; 
; This matches the linear_state structure defined 
; in resampler_linear.h
;
;**********************************************************************

%define freq_in					ebp+00h
%define freq_out				ebp+04h

%define left0					ebp+08h
%define left1					ebp+0ch
%define right0					ebp+010h
%define right1					ebp+014h

%define fl_xd_step				ebp+018h
%define i_xd					ebp+020h
%define freq_out_reciprocal		ebp+024h

%define right_in_channel_offset	ebp+02ch

%define left_gain				ebp+030h
%define right_gain				ebp+034h

%define in						ebp+038h
%define in_cnt					ebp+03ch
%define out						ebp+040h
%define out_cnt					ebp+044h


;**********************************************************************
;
; Defines for parameters passed on stack & local vars
;
; ebp is used to point to the linear_state struct, so all
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

