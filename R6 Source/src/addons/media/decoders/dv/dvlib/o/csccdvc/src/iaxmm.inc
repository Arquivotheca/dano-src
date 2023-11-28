
;/*
;*    INTEL CORPORATION PROPRIETARY INFORMATION
;*
;*    This software is supplied under the terms of a license
;*    agreement or nondisclosure agreement with Intel Corporation
;*    and may not be copied or disclosed except in accordance with
;*    the terms of that agreement.
;*    Copyright (c) 1997  Intel Corporation.
;*
;*/
;NOTE:NOTE:NOTE:


;Please use XMMWORD and not DWORD  for 128 bit data
XMMWORD 	TEXTEQU	<DWORD>
xmmword	TEXTEQU	<DWORD>
mm2word	TEXTEQU	<DWORD>
MM2WORD	TEXTEQU	<DWORD>

;Please use MMWORD and not DWORD  for 64 bit data in MMX code
; It will be "MMWORD TEXTEQU <QWORD>" if you are using MASM 6.12 and not 6.11d.
IF @Version EQ 613
MMWORD	TEXTEQU	<QWORD>
mmword	TEXTEQU	<QWORD>
.586
.mmx
ELSE
MMWORD	TEXTEQU	<DWORD>
mmword TEXTEQU	<DWORD>
.486P
ENDIF

; You can use this macro to create the shufps or pshufw constant
;
SELECT macro imm3:req,imm2:req,imm1:req,imm0:req
   	temp TEXTEQU %(imm3*64 +imm2*16+imm1*4+imm0)
	EXITM  %temp
endm


MMX2INT macro arg1:req
	if @SizeStr(arg1) NE 3
		EXITM <arg1>
	endif
	if @InStr(1,arg1,<mm>) NE 1
		if @InStr(1,arg1,<MM>) NE 1
			EXITM <arg1>
		endif
	endif
	if @InStr(1,arg1,<mm0>) EQ 1
		EXITM <eax>
	elseif @InStr(1,arg1,<mm1>) EQ 1
		EXITM <ecx>
	elseif @InStr(1,arg1,<mm2>) EQ 1
		EXITM <edx>
	elseif @InStr(1,arg1,<mm3>) EQ 1
		EXITM <ebx>
	elseif @InStr(1,arg1,<mm4>) EQ 1
		EXITM <esp>
	elseif @InStr(1,arg1,<mm5>) EQ 1
		EXITM <ebp>
	elseif @InStr(1,arg1,<mm6>) EQ 1
		EXITM <esi>
	elseif @InStr(1,arg1,<mm7>) EQ 1
		EXITM <edi>
	elseif @InStr(1,arg1,<MM0>) EQ 1
		EXITM <eax>
	elseif @InStr(1,arg1,<MM1>) EQ 1
		EXITM <ecx>
	elseif @InStr(1,arg1,<MM2>) EQ 1
		EXITM <edx>
	elseif @InStr(1,arg1,<MM3>) EQ 1
		EXITM <ebx>
	elseif @InStr(1,arg1,<MM4>) EQ 1
		EXITM <esp>
	elseif @InStr(1,arg1,<MM5>) EQ 1
		EXITM <ebp>
	elseif @InStr(1,arg1,<MM6>) EQ 1
		EXITM <esi>
	elseif @InStr(1,arg1,<MM7>) EQ 1
		EXITM <edi>
	endif
	EXITM <arg1>
endm




; START VX macro file
DefineXMMxRegs Macro
IFDEF APP_16BIT
	xmm0	TEXTEQU	<AX>
	xmm1	TEXTEQU	<CX>
	xmm2	TEXTEQU	<DX>
	xmm3	TEXTEQU	<BX>
	xmm4	TEXTEQU	<SP>
	xmm5	TEXTEQU	<BP>
	xmm6	TEXTEQU	<SI>
	xmm7	TEXTEQU	<DI>

	XMM0	TEXTEQU	<AX>
	XMM1	TEXTEQU	<CX>
	XMM2	TEXTEQU	<DX>
	XMM3	TEXTEQU	<BX>
	XMM4	TEXTEQU	<SP>
	XMM5	TEXTEQU	<BP>
	XMM6	TEXTEQU	<SI>
	XMM7	TEXTEQU	<DI>
ELSE
	xmm0	TEXTEQU	<EAX>
	xmm1	TEXTEQU	<ECX>
	xmm2	TEXTEQU	<EDX>
	xmm3	TEXTEQU	<EBX>
	xmm4	TEXTEQU	<ESP>
	xmm5	TEXTEQU	<EBP>
	xmm6	TEXTEQU	<ESI>
	xmm7	TEXTEQU	<EDI>

	XMM0	TEXTEQU	<EAX>
	XMM1	TEXTEQU	<ECX>
	XMM2	TEXTEQU	<EDX>
	XMM3	TEXTEQU	<EBX>
	XMM4	TEXTEQU	<ESP>
	XMM5	TEXTEQU	<EBP>
	XMM6	TEXTEQU	<ESI>
	XMM7	TEXTEQU	<EDI>
ENDIF
endm

UnDefineXMMxRegs Macro
	XMM0	TEXTEQU	<XMM0>
	XMM1	TEXTEQU	<XMM1>
	XMM2	TEXTEQU	<XMM2>
	XMM3	TEXTEQU	<XMM3>
	XMM4	TEXTEQU	<XMM4>
	XMM5	TEXTEQU	<XMM5>
	XMM6	TEXTEQU	<XMM6>
	XMM7	TEXTEQU	<XMM7>



	xmm0	TEXTEQU	<xmm0>
	xmm1	TEXTEQU	<xmm1>
	xmm2	TEXTEQU	<xmm2>
	xmm3	TEXTEQU	<xmm3>
	xmm4	TEXTEQU	<xmm4>
	xmm5	TEXTEQU	<xmm5>
	xmm6	TEXTEQU	<xmm6>
	xmm7	TEXTEQU	<xmm7>
EndM



EXT0	TEXTEQU	<EAX>
EXT1	TEXTEQU	<ECX>
EXT2	TEXTEQU	<EDX>
EXT3	TEXTEQU	<EBX>
EXT4	TEXTEQU	<ESP>
EXT5	TEXTEQU	<EBP>
EXT6	TEXTEQU	<ESI>
EXT7	TEXTEQU	<EDI>
EXTB0	TEXTEQU	<AL>
EXTB1	TEXTEQU	<CL>
EXTB2	TEXTEQU	<DL>
EXTB3	TEXTEQU	<BL>



opc_Movups_ld       = 010H
opc_Movss_ld        = 010H
opc_Movups_st       = 011H
opc_Movss_st        = 011H
opc_Movlps_ld       = 012H
opc_Movlps_st       = 013H
opc_Unpcklps        = 014H
opc_Unpckhps        = 015H
opc_Movhps_ld       = 016H
opc_Movhps_st       = 017H
opc_Prefetch        = 018H
opc_Movaps_ld       = 028H
opc_Movaps_st       = 029H
opc_Cvtpi2ps        = 02aH
opc_Cvtsi2ss        = 02aH
opc_Movntps         = 02bH
opc_Cvttps2pi       = 02cH
opc_Cvttss2si       = 02cH
opc_Cvtps2pi        = 02dH
opc_Cvtss2si        = 02dH
opc_Ucomiss         = 02eH
opc_Comiss          = 02fH
opc_Movmskps        = 050H
opc_Sqrtps          = 051H
opc_Sqrtss          = 051H
opc_Rsqrtps         = 052H
opc_Rsqrtss         = 052H
opc_Rcpps           = 053H
opc_Rcpss           = 053H
opc_Andps           = 054H
opc_Andnps          = 055H
opc_Orps            = 056H
opc_Xorps           = 057H
opc_Addps           = 058H
opc_Addss           = 058H
opc_Mulps           = 059H
opc_Mulss           = 059H
opc_Subps           = 05cH
opc_Subss           = 05cH
opc_Minps           = 05dH
opc_Minss           = 05dH
opc_Divps           = 05eH
opc_Divss           = 05eH
opc_Maxps           = 05fH
opc_Maxss           = 05fH
opc_Pshufw          = 070H
opc_Pgeneral        = 0aeH
opc_Cmpps           = 0c2H
opc_Cmpss           = 0c2H
opc_Pinsrw          = 0c4H
opc_Pextrw          = 0c5H
opc_Shufps          = 0c6H
opc_Pmovmskb        = 0d7H
opc_Pminub          = 0daH
opc_Pmaxub          = 0deH
opc_Pmulhuw         = 0e4H
opc_Movntq          = 0e7H
opc_Pminsw          = 0eaH
opc_Pmaxsw          = 0eeH
opc_Maskmovq        = 0f7H

KNIld_st   macro   op:req,dst:req, src:req
        local   x, y
		DefineXMMxRegs
IF (OPATTR(dst)) AND 00010000y ; register
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    op&_ld
        org     y
ELSE
x:
        cmpxchg   dst, src
y:
        org     x+1
        byte    op&_st
        org     y
ENDIF
		UnDefineXMMxRegs
        endm
KNIld_st_f3   macro   op:req,dst:req, src:req
        local   x, y
		DefineXMMxRegs
IF (OPATTR(dst)) AND 00010000y ; register
x:
        lock cmpxchg   src, dst
y:
        org     x
        byte    0F3H,0Fh, op&_ld
        org     y
ELSE
x:
        lock cmpxchg   dst, src
y:
        org     x
        byte    0F3H,0Fh, op&_st
        org     y
ENDIF
		UnDefineXMMxRegs
        endm

KNIst   macro   op:req,dst:req, src:req
        local   x, y
		DefineXMMxRegs
IF (OPATTR(dst)) AND 00010000y ; register
 .ERR <Illegal argument - usage: op mem,xmm >
ELSE
x:
        cmpxchg   dst, src
y:
        org     x+1
        byte    op
        org     y
ENDIF
		UnDefineXMMxRegs
        endm

KNIone_op   macro   op:req,dst:req, ext:req
        local   x, y
IF (OPATTR(dst)) AND 00010000y ; register
 .ERR <Illegal argument - usage: op mem >
ELSE
x:
        cmpxchg   dst, ext
y:
        org     x+1
        byte    op
        org     y
ENDIF
        endm

KNIprefetch   macro   op:req,dst:req, ext:req
        local   x, y
IF (OPATTR(dst)) AND 00010000y ; register
 .ERR <Illegal argument - usage: op mem >
ELSE
x:
        lock add   dst, ext
y:
        org     x
        byte    0fH, op
        org     y
ENDIF
        endm


KNIno_op   macro   op:req, ext:req
        local   x, y
x:
        cmpxchg   EXT0,ext
y:
        org     x+1
        byte    op
        org     y
        endm


KNImm_st   macro   op:req,dst:req, src:req
        local   x, y
;		DefineMMxRegs

IF (OPATTR(dst)) AND 00010000y ; register
 .ERR <Illegal argument - usage: op mem,xmm >
ELSE
x:
        cmpxchg   dst, MMX2INT(src)
y:
        org     x+1
        byte    op
        org     y
ENDIF
;		UnDefineMMxRegs
        endm

KNIld_st1   macro   op:req,dst:req, src:req
        local   x, y
		DefineXMMxRegs
IF (OPATTR(dst)) AND 00010000y ; register
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    op&_ld
        org     y
ELSE
x:
        cmpxchg   dst, src
y:
        org     x+1
        byte    op&_st
        org     y
ENDIF
		UnDefineXMMxRegs
        endm

KNIld_st1_f3   macro   op:req,dst:req, src:req
        local   x, y
		DefineXMMxRegs
IF (OPATTR(dst)) AND 00010000y ; register
x:
        lock cmpxchg   src, dst
y:
        org     x
        byte    0f3h,0fh,op&_ld
        org     y
ELSEIF (OPATTR(src)) AND 00010000y ; register
 .ERR <Illegal argument - usage: op xmm,mem or op mem, xmm >
ELSE
x:
        lock cmpxchg   dst, src
y:
        org     x
        byte    0f3h,0fh,op&_st
        org     y
ENDIF
		UnDefineXMMxRegs
        endm


KNIop	macro	op:req, dst:req, src:req
        local   x, y
		DefineXMMxRegs
x:
        cmpxchg   src,dst
y:
        org     x+1
        byte    op
        org     y
		UnDefineXMMxRegs
endm
KNIopmm	macro	op:req, dst:req, src:req
        local   x, y

x:
        cmpxchg   MMX2INT(src),MMX2INT(dst)
y:
        org     x+1
        byte    op
        org     y

endm
KNIopmm_mmx	macro	op:req, dst:req, src:req
        local   x, y

		DefineXMMxRegs
x:
        cmpxchg   MMX2INT(src),MMX2INT(dst)
y:
        org     x+1
        byte    op
        org     y
		UnDefineXMMxRegs
endm
KNIopmm_imm	macro	op:req, dst:req, src:req,imm:req
        local   x, y

x:
        cmpxchg   MMX2INT(src),MMX2INT(dst)
        byte	imm
y:
        org     x+1
        byte    op
        org     y

endm
KNIop_imm	macro	op:req, dst:req, src:req,imm:req
        local   x, y
		DefineXMMxRegs
x:
        cmpxchg   src,dst
        byte	imm
y:
        org     x+1
        byte    op
        org     y
		UnDefineXMMxRegs
endm
KNIop_imm_f3	macro	op:req, dst:req, src:req,imm:req
        local   x, y
		DefineXMMxRegs
x:
        lock cmpxchg   src,dst
        byte	imm
y:
        org     x
        byte    0F3H,0FH,op
        org     y
		UnDefineXMMxRegs
endm

KNIop_f3	macro	op:req, dst:req, src:req
        local   x, y
		DefineXMMxRegs
x:
        lock cmpxchg   src,dst
y:
        org     x
        byte    0f3h ,0fh, op
        org     y
		UnDefineXMMxRegs
endm
;0F 10 /r  movups xmm1, xmm2/m128
;0F 11 /r  movups xmm2/m128, xmm1
movups  macro   dst:req, src:req
        KNIld_st    opc_Movups, dst, src
endm

;F3 0F 10 /r  movss xmm1, xmm2/m32
;F3 0F 11 /r  movss xmm2/m32, xmm1
movss  macro   dst:req, src:req
        KNIld_st_f3    opc_Movss, dst, src
endm
;0F 12 /r  movlps xmm, m64
;0F 13 /r  movlps m64, xmm
movlps  macro   dst:req, src:req
        KNIld_st1    opc_Movlps, dst, src
endm
;0F 14 /r  unpcklps xmm1, xmm2/m128
unpcklps  macro   dst:req, src:req
        KNIop    opc_Unpcklps, dst, src
endm
;0F 15 /r  unpckhps xmm1, xmm2/m128
unpckhps  macro   dst:req, src:req
        KNIop    opc_Unpckhps, dst, src
endm
;0F 16 /r  movhps xmm, m64
;0F 17 /r  movhps m64, xmm
movhps  macro   dst:req, src:req
        KNIld_st1    opc_Movhps, dst, src
endm
;0F 28 /r  movaps xmm1, xmm2/m128
;0F 29 /r  movaps xmm2/m128, xmm1
movaps  macro   dst:req, src:req
        KNIld_st    opc_Movaps, dst, src
endm
;0F E7 /r  movntq m64, mm
movntq  macro   dst:req, src:req
        KNImm_st    opc_Movntq, dst, src
endm
;0F 2B /r  movntps m128, xmm
movntps  macro   dst:req, src:req
        KNIst    opc_Movntps, dst, src
endm
;0F 1C /r  cvttps2pi mm, xmm/m64
cvttps2pi  macro   dst:req, src:req
        KNIopmm_mmx    opc_Cvttps2pi, dst, src
endm
;F3 0F 1C /r  cvttss2si r32, xmm/m32
cvttss2si  macro   dst:req, src:req
        KNIop_f3    opc_Cvttss2si, dst, src
endm
;0F 1D /r  cvtps2pi mm, xmm/m64
cvtps2pi  macro   dst:req, src:req
        KNIopmm_mmx    opc_Cvtps2pi, dst, src
endm
;F3 0F 1D /r  cvtss2si r32, xmm/m32
cvtss2si  macro   dst:req, src:req
        KNIop_f3    opc_Cvtss2si, dst, src
endm
;0F 2E /r  ucomiss xmm1, xmm2/m32
ucomiss  macro   dst:req, src:req
        KNIop    opc_Ucomiss, dst, src
endm
;0F 2F /r  comiss xmm1, xmm2/m32
comiss  macro   dst:req, src:req
        KNIop    opc_Comiss, dst, src
endm
;0F F7 /r maskmovq mm1,mm2
maskmovq  macro   dst:req, src:req
        KNIopmm    opc_Maskmovq, dst, src
endm
;0F 2D /r  cvtpi2ps xmm, mm/m64
cvtpi2ps  macro   dst:req, src:req
      KNIopmm_mmx    opc_Cvtpi2ps, dst, src
endm
;F3 0F 2D /r  cvtsi2ss xmm, r/m32
cvtsi2ss  macro   dst:req, src:req
        KNIop_f3    opc_Cvtsi2ss, dst, src
endm
;0F 51 /r  sqrtps xmm1, xmm2/m128
sqrtps  macro   dst:req, src:req
        KNIop    opc_Sqrtps, dst, src
endm
;F3 0F 51 /r  sqrtss xmm1, xmm2/m32
sqrtss  macro   dst:req, src:req
        KNIop_f3    opc_Sqrtss, dst, src
endm
;0F 52 /r  rsqrtps xmm1, xmm2/m128
rsqrtps  macro   dst:req, src:req
        KNIop    opc_Rsqrtps, dst, src
endm
;F3 0F 52 /r  rsqrtss xmm1, xmm2/m128
rsqrtss  macro   dst:req, src:req
        KNIop_f3  opc_Rsqrtss, dst, src
endm
;0F 53 /r  rcpps xmm1, xmm2/m128
rcpps  macro   dst:req, src:req
        KNIop    opc_Rcpps, dst, src
endm
;F3 0F 53 /r  rcpps xmm1, xmm2/m128
rcpss  macro   dst:req, src:req
        KNIop_f3    opc_Rcpss, dst, src
endm
;0F 54 /r  andps xmm1, xmm2/m128
andps  macro   dst:req, src:req
        KNIop    opc_Andps, dst, src
endm
;0F 55 /r  andnps xmm1, xmm2/m128
andnps  macro   dst:req, src:req
        KNIop    opc_Andnps, dst, src
endm
;0F 56 /r  orps xmm1, xmm2/m128
orps  macro   dst:req, src:req
        KNIop    opc_Orps, dst, src
endm
;0F 57 /r  xorps xmm1, xmm2/m128
xorps  macro   dst:req, src:req
        KNIop    opc_Xorps, dst, src
endm
;0F 58 /r  addps xmm1, xmm2/m128
addps  macro   dst:req, src:req
        KNIop    opc_Addps, dst, src
endm
;F3 0F 58 /r  addss xmm1, xmm2/m32
addss  macro   dst:req, src:req
        KNIop_f3    opc_Addss, dst, src
endm
;0F 59 /r  mulps xmm1, xmm2/m128
mulps  macro   dst:req, src:req
        KNIop    opc_Mulps, dst, src
endm
;F3 0F 59 /r  mulss xmm1, xmm2/m32
mulss  macro   dst:req, src:req
        KNIop_f3    opc_Mulss, dst, src
endm
;0F 5C /r  subps xmm1, xmm2/m128
subps  macro   dst:req, src:req
        KNIop    opc_Subps, dst, src
endm
;F3 0F 5C /r  subss xmm1, xmm2/m128
subss  macro   dst:req, src:req
        KNIop_f3    opc_Subss, dst, src
endm
;0F 5D /r  minps xmm1, xmm2/m128
minps  macro   dst:req, src:req
        KNIop    opc_Minps, dst, src
endm
;F3 0F 5D /r  minss xmm1, xmm2/m32
minss  macro   dst:req, src:req
        KNIop_f3    opc_Minss, dst, src
endm
;0F 5E /r  divps xmm1, xmm2/m128
divps  macro   dst:req, src:req
        KNIop    opc_Divps, dst, src
endm
;F3 0F 5E /r  divss xmm1, xmm2/m32
divss  macro   dst:req, src:req
        KNIop_f3    opc_Divss, dst, src
endm
;0F 5F /r  maxps xmm1, xmm2/m128
maxps  macro   dst:req, src:req
        KNIop    opc_Maxps, dst, src
endm
;F3 0F 5F /r  maxss xmm1, xmm2/m32
maxss  macro   dst:req, src:req
        KNIop_f3    opc_Maxss, dst, src
endm
;0F 70 /r ib pshufw mm1, mm2, imm8
 pshufw macro   dst:req, src:req,imm:req
        KNIopmm_imm    opc_Pshufw, dst, src,imm
endm
;0F AE /0    fxsave m512byte
 fxsave  macro  dst:req
        KNIone_op    opc_Pgeneral, dst,EXT0
endm
;0F AE /1    fxrstor m512byte
 fxrstor macro   dst:req
        KNIone_op    opc_Pgeneral, dst,EXT1
endm
;0F AE /2    ldmxcsr m32
  ldmxcsr macro   dst:req
        KNIone_op    opc_Pgeneral, dst, EXT2
endm
;0F AE /3    stmxcsr m32
 stmxcsr  macro  dst:req
        KNIone_op    opc_Pgeneral, dst, EXT3
endm
;0F AE /7    sfence
sfence   macro
        KNIno_op    opc_Pgeneral, EXT7
endm
;0F 50 /r  movmskps r32, xmm
movmskps  macro   dst:req, src:req
        KNIop    opc_Movmskps, dst, src
endm
;Those TEXTEQU lines for only for backward compatibility and they
;will be removed in one of the next versions.
cmppseq TEXTEQU <cmpeqps>
cmppslt TEXTEQU <cmpltps>
cmppsle TEXTEQU <cmpleps>
cmppsunord TEXTEQU <cmpunordps>
cmppsneq TEXTEQU <cmpneqps>
cmppslt TEXTEQU <cmpltps>
cmppsnle TEXTEQU <cmpnleps>
cmppsord TEXTEQU <cmpordps>
cmpsseq TEXTEQU <cmpeqss>
cmpsslt TEXTEQU <cmpltss>
cmpssle TEXTEQU <cmpless>
cmpssunord TEXTEQU <cmpunordss>
cmpssneq TEXTEQU <cmpneqss>
cmpsslt TEXTEQU <cmpltss>
cmpssnle TEXTEQU <cmpnless>
cmpssord TEXTEQU <cmpordss>
prefetch0 TEXTEQU <prefetcht0>
prefetch1 TEXTEQU <prefetcht1>
prefetch2 TEXTEQU <prefetcht2>
;0F C2 /r  cmpps xmm1, xmm2/m128, imm8
cmpps  macro   dst:req, src:req,imm:req
        KNIop_imm    opc_Cmpps, dst, src,imm
endm
cmpeqps  macro   dst:req, src:req
        KNIop_imm    opc_Cmpps, dst, src,0
endm
cmpltps  macro   dst:req, src:req
        KNIop_imm    opc_Cmpps, dst, src,1
endm
cmpleps  macro   dst:req, src:req
        KNIop_imm    opc_Cmpps, dst, src,2
endm
cmpunordps  macro   dst:req, src:req
        KNIop_imm    opc_Cmpps, dst, src,3
endm
cmpneqps  macro   dst:req, src:req
        KNIop_imm    opc_Cmpps, dst, src,4
endm
cmpnltps  macro   dst:req, src:req
        KNIop_imm    opc_Cmpps, dst, src,5
endm
cmpnleps  macro   dst:req, src:req
        KNIop_imm    opc_Cmpps, dst, src,6
endm
cmpordps  macro   dst:req, src:req
        KNIop_imm    opc_Cmpps, dst, src,7
endm

;F3 0F C2 /r  cmpss xmm1, xmm2/m32, imm8
cmpss  macro   dst:req, src:req,imm:req
        KNIop_imm_f3    opc_Cmpss, dst, src,imm
endm
cmpeqss  macro   dst:req, src:req
        KNIop_imm_f3    opc_Cmpss, dst, src,0
endm
cmpltss  macro   dst:req, src:req
        KNIop_imm_f3    opc_Cmpss, dst, src,1
endm
cmpless  macro   dst:req, src:req
        KNIop_imm_f3    opc_Cmpss, dst, src,2
endm
cmpunordss  macro   dst:req, src:req
        KNIop_imm_f3    opc_Cmpss, dst, src,3
endm
cmpneqss  macro   dst:req, src:req
        KNIop_imm_f3    opc_Cmpss, dst, src,4
endm
cmpnltss  macro   dst:req, src:req
        KNIop_imm_f3    opc_Cmpss, dst, src,5
endm
cmpnless  macro   dst:req, src:req
        KNIop_imm_f3    opc_Cmpss, dst, src,6
endm
cmpordss  macro   dst:req, src:req
        KNIop_imm_f3    opc_Cmpss, dst, src,7
endm

;0F C4 /r  pinsrw mm, r32, imm8
pinsrw  macro   dst:req, src:req, imm:req
        KNIopmm_imm    opc_Pinsrw, dst, src, imm
endm
;0F C5 /r  pextrw r32, mm, imm8
pextrw  macro   dst:req, src:req, imm:req
        KNIopmm_imm    opc_Pextrw, dst, src, imm
endm
;0F C6 /r  shufps xmm1, xmm2/m128, imm8
shufps  macro   dst:req, src:req, imm:req
        KNIop_imm    opc_Shufps, dst, src, imm
endm
;0F D7 /r  pmovmskb r32, mm
pmovmskb  macro   dst:req, src:req
        KNIopmm    opc_Pmovmskb, dst, src
endm
;0F DA /r  pminub mm1, mm2/m64
pminub  macro   dst:req, src:req
        KNIopmm    opc_Pminub, dst, src
endm
;0F DE /r  pmaxub mm1, mm2/m64
pmaxub  macro   dst:req, src:req
        KNIopmm    opc_Pmaxub, dst, src
endm
;0F EA /r  pminsw mm1, mm2/m64
pminsw  macro   dst:req, src:req
       KNIopmm    opc_Pminsw, dst, src
endm
;0F EE /r  pmaxsw mm1, mm2/m64
pmaxsw  macro   dst:req, src:req
       KNIopmm    opc_Pmaxsw, dst, src
endm
;0F E4 /r  pmulhuw mm1, mm2/m64
pmulhuw  macro   dst:req, src:req
        KNIopmm    opc_Pmulhuw, dst, src
endm


;0F 18 /1 prefetcht0 mm8
prefetcht0  macro   dst:req
        KNIprefetch    opc_Prefetch, dst, EXTB1
endm
;0F 18 /2 prefetcht1 mm8
prefetcht1  macro   dst:req
        KNIprefetch    opc_Prefetch, dst, EXTB2
endm
;0F 18 /3 prefetcht2 mm8
prefetcht2  macro   dst:req
        KNIprefetch    opc_Prefetch, dst, EXTB3
endm
;0F 18 /0 prefetchnta mm8
prefetchnta  macro   dst:req
        KNIprefetch    opc_Prefetch, dst, EXTB0
endm
