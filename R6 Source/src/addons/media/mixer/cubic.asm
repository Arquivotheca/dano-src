; cubic.asm - shell to create cubic interpolators for various formats
;
; If you want to debug one these routines, it's easiest to comment out
; the %include "cubic_core.asm" line and just paste the entire text of
; cubic_core.inc into this file at the same place.  NASM doesn't give 
; error messages by the line number in the included file.
;
; I also tried making the core function a macro, but NASM doesn't give
; useful error messages that way either.
;

SECTION .text

%include "asmdebug.inc"
%include "convert.inc"
%include "cubic.inc"

;
; Constants used by assembly functions
; 

one_half:	dd	0.5
one:		dd	1.0
two:		dd	2.0
three:		dd	3.0
five:		dd	5.0

;**********************************************************************
;
; cubic_uint8_to_float
; 
;**********************************************************************

%define CORE_LABEL		cubic_uint8_to_float
%define IN_BPS			1
%define LOAD_SAMPLE		LOAD_UINT8
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_UINT8
%define MIX

%include "cubic_core.asm"


;**********************************************************************
;
; cubic_uint8_to_float_mix
; 
;**********************************************************************

%define CORE_LABEL		cubic_uint8_to_float_mix
%define IN_BPS			1
%define LOAD_SAMPLE		LOAD_UINT8
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_UINT8
%define MIX				MIX_FLOAT

%include "cubic_core.asm"


;**********************************************************************
;
; cubic_int16_to_float
; 
;**********************************************************************

%define CORE_LABEL		cubic_int16_to_float
%define IN_BPS			2
%define LOAD_SAMPLE		LOAD_INT16
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_INT16
%define MIX

%include "cubic_core.asm"


;**********************************************************************
;
; cubic_int16_to_float_mix
; 
;**********************************************************************

%define CORE_LABEL		cubic_int16_to_float_mix
%define IN_BPS			2
%define LOAD_SAMPLE		LOAD_INT16
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_INT16
%define MIX				MIX_FLOAT

%include "cubic_core.asm"


;**********************************************************************
;
; cubic_int32_to_float
; 
;**********************************************************************

%define CORE_LABEL		cubic_int32_to_float
%define IN_BPS			4
%define LOAD_SAMPLE		LOAD_INT32
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_INT32
%define MIX

%include "cubic_core.asm"


;**********************************************************************
;
; cubic_int32_to_float_mix
; 
;**********************************************************************

%define CORE_LABEL		cubic_int32_to_float_mix
%define IN_BPS			4
%define LOAD_SAMPLE		LOAD_INT32
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_INT32
%define MIX				MIX_FLOAT

%include "cubic_core.asm"


;**********************************************************************
;
; cubic_float_to_float
; 
;**********************************************************************

%define CORE_LABEL		cubic_float_to_float
%define IN_BPS			4
%define LOAD_SAMPLE		LOAD_FLOAT
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE
%define MIX

%include "cubic_core.asm"


;**********************************************************************
;
; cubic_float_to_float_mix
; 
;**********************************************************************

%define CORE_LABEL		cubic_float_to_float_mix
%define IN_BPS			4
%define LOAD_SAMPLE		LOAD_FLOAT
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE
%define MIX				MIX_FLOAT

%include "cubic_core.asm"

