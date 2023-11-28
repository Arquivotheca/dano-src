; linear.asm - shell to create linear interpolators for various formats
;
; If you want to debug one these routines, it's easiest to comment out
; the %include "linear_core.asm" line and just paste the entire text of
; linear_core.inc into this file at the same place.  NASM doesn't give 
; error messages by the line number in the included file.
;
; I also tried making the core function a macro, but NASM doesn't give
; useful error messages that way either.
;

SECTION .text

%include "asmdebug.inc"
%include "convert.inc"
%include "linear.inc"

;**********************************************************************
;
; linear_uint8_to_float
; 
;**********************************************************************

%define CORE_LABEL		linear_uint8_to_float
%define IN_BPS			1
%define LOAD_SAMPLE		LOAD_UINT8
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_UINT8
%define MIX

%include "linear_core.asm"


;**********************************************************************
;
; linear_uint8_to_float_mix
; 
;**********************************************************************

%define CORE_LABEL		linear_uint8_to_float_mix
%define IN_BPS			1
%define LOAD_SAMPLE		LOAD_UINT8
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_UINT8
%define MIX				MIX_FLOAT

%include "linear_core.asm"


;**********************************************************************
;
; linear_int16_to_float
; 
;**********************************************************************

%define CORE_LABEL		linear_int16_to_float
%define IN_BPS			2
%define LOAD_SAMPLE		LOAD_INT16
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_INT16
%define MIX

%include "linear_core.asm"


;**********************************************************************
;
; linear_int16_to_float_mix
; 
;**********************************************************************

%define CORE_LABEL		linear_int16_to_float_mix
%define IN_BPS			2
%define LOAD_SAMPLE		LOAD_INT16
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_INT16
%define MIX				MIX_FLOAT

%include "linear_core.asm"


;**********************************************************************
;
; linear_int32_to_float
; 
;**********************************************************************

%define CORE_LABEL		linear_int32_to_float
%define IN_BPS			4
%define LOAD_SAMPLE		LOAD_INT32
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_INT32
%define MIX

%include "linear_core.asm"


;**********************************************************************
;
; linear_int32_to_float_mix
; 
;**********************************************************************

%define CORE_LABEL		linear_int32_to_float_mix
%define IN_BPS			4
%define LOAD_SAMPLE		LOAD_INT32
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE		GAIN_SCALE_INT32
%define MIX				MIX_FLOAT

%include "linear_core.asm"


;**********************************************************************
;
; linear_float_to_float
; 
;**********************************************************************

%define CORE_LABEL		linear_float_to_float
%define IN_BPS			4
%define LOAD_SAMPLE		LOAD_FLOAT
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE
%define MIX

%include "linear_core.asm"


;**********************************************************************
;
; linear_float_to_float_mix
; 
;**********************************************************************

%define CORE_LABEL		linear_float_to_float_mix
%define IN_BPS			4
%define LOAD_SAMPLE		LOAD_FLOAT
%define OUT_BPS			4
%define STORE_SAMPLE	STORE_FLOAT
%define GAIN_SCALE
%define MIX				MIX_FLOAT

%include "linear_core.asm"

