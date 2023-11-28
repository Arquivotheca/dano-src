;*****************************************************************
;
; Global constants used by assembly code
;
;*****************************************************************

;
; Gain scaling constants for different audio formats
;
; For 8 bit samples, the 16 bit value is used; this is because
; the 8 bit samples are converted to 16 bit and then converted
; to signed.
;

GLOBAL scale_int16,scale_int32

scale_int16:	dd	0.000030518509476
scale_int32:	dd	4.65661287308e-10

;
; Debug strings
;

GLOBAL pfloat,pint,phex,pcr

pfloat: db " %.50f",0
pint:   db " %ld",0
phex:   db " 0x%lx",0 
pcr:    db 10,0
