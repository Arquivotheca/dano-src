; cubic.inc

;**********************************************************************
;
; Data structure containing state of cubic interpolator
; 
; This matches the cubic_state structure defined 
; in resampler_cubic.h
;
;**********************************************************************

%define freq_in					ebp+00h
%define freq_out				ebp+04h

%define left0					ebp+08h
%define left1					ebp+0ch
%define left2					ebp+010h
%define left3					ebp+014h

%define right0					ebp+018h
%define right1					ebp+01ch
%define right2					ebp+020h
%define right3					ebp+024h

%define fl_xd_step				ebp+028h
%define i_xd					ebp+030h
%define freq_out_reciprocal		ebp+034h

%define right_in_channel_offset	ebp+03ch

%define left_gain				ebp+040h
%define right_gain				ebp+044h

%define in						ebp+048h
%define in_cnt					ebp+04ch
%define out						ebp+050h
%define out_cnt					ebp+054h


;**********************************************************************
;
; Defines for parameters passed on stack & local vars
;
; ebp is used to point to the cubic_state struct, so all
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

