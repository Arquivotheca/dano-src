; sinc.asm - assembly version of sinc interpolator
;

SECTION .text

%include "sinc.inc"
%include "asmdebug.inc"
%include "convert.inc"


;**********************************************************************
;
; sinc_float_to_float_mix
; 
;**********************************************************************

%define CORE_LABEL		sinc_float_to_float_mix
%define OUT_BPF			8					; bytes per frame
%define STORE_LEFT		STORE_FLOAT_LEFT
%define STORE_RIGHT		STORE_FLOAT_RIGHT
%define MIX_LEFT		MIX_FLOAT_LEFT
%define MIX_RIGHT		MIX_FLOAT_RIGHT

%include "sinc_core.asm"


;**********************************************************************
;
; sinc_float_to_float
; 
;**********************************************************************

%define CORE_LABEL		sinc_float_to_float
%define OUT_BPF			8					; bytes per frame
%define STORE_LEFT		STORE_FLOAT_LEFT
%define STORE_RIGHT		STORE_FLOAT_RIGHT
%define MIX_LEFT		
%define MIX_RIGHT		

%include "sinc_core.asm"


