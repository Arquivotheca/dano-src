#ifndef _X86DIS_P_H_
#define _X86DIS_P_H_

/* operand types */

typedef enum {
	op_addr_none,
	op_addr_A, op_addr_C, op_addr_D, op_addr_E, op_addr_F, op_addr_G,
	op_addr_I, op_addr_J, op_addr_M, op_addr_O, op_addr_P, op_addr_Q,
	op_addr_Qmmx, op_addr_R, op_addr_S, op_addr_T, op_addr_X, op_addr_Y,
	op_addr_reg, op_addr_immed,
} op_addr_t;

typedef enum {
	op_type_a, op_type_b, op_type_c, op_type_d, op_type_p,
	op_type_o, op_type_q, op_type_s, op_type_v, op_type_w,

	op_type_sreg, op_type_fpreg,
	
	op_type_last
} op_type_t;

typedef enum {
	reg_al, reg_cl, reg_dl, reg_bl, reg_ah, reg_ch, reg_dh, reg_bh
} reg8_t;

typedef enum {
	reg_ax, reg_cx, reg_dx, reg_bx, reg_sp, reg_bp, reg_si, reg_di
} reg16_t;

typedef enum {
	reg_eax, reg_ecx, reg_edx, reg_ebx, reg_esp, reg_ebp, reg_esi, reg_edi
} reg32_t;

typedef enum {
	reg_mmx0, reg_mmx1, reg_mmx2, reg_mmx3,
	reg_mmx4, reg_mmx5, reg_mmx6, reg_mmx7
} regmmx_t;

typedef enum {
	reg_es, reg_cs, reg_ss, reg_ds, reg_fs, reg_gs
} regseg_t;

typedef enum {
	reg_st0, reg_st1, reg_st2, reg_st3, reg_st4, reg_st5, reg_st6, reg_st7
} regfp_t;

typedef enum {
	reg_cr0, reg_cr1, reg_cr2, reg_cr3, reg_cr4, reg_cr5, reg_cr6, reg_cr7
} regc_t;

typedef enum {
	reg_dr0, reg_dr1, reg_dr2, reg_dr3, reg_dr4, reg_dr5, reg_dr6, reg_dr7
} regd_t;

typedef struct {
	unsigned int addr : 5;
	unsigned int type : 4;
	unsigned int data : 23;
} operand_t;

#define NO_OPERAND { op_addr_none, 0, 0 }

typedef enum {
	instr_aaa,
	instr_aad,
	instr_aam,
	instr_adc,
	instr_add,
	instr_and,
	instr_arpl,
	instr_bound,
	instr_bsf,
	instr_bsr,
	instr_bswap,
	instr_bt,
	instr_btc,
	instr_btr,
	instr_bts,
	instr_call,
	instr_cbw,
	instr_cflsh,
	instr_clc,
	instr_cld,
	instr_cli,
	instr_clts,
	instr_cmc,

	/* these must be in this order */
	instr_cmovo,
	instr_cmovno,
	instr_cmovc,
	instr_cmovnc,
	instr_cmovz,
	instr_cmovnz,
	instr_cmovbe,
	instr_cmova,
	instr_cmovs,
	instr_cmovns,
	instr_cmovp,
	instr_cmovnp,
	instr_cmovl,
	instr_cmovnl,
	instr_cmovle,
	instr_cmovg,

	instr_cmp,
	instr_cmps,
	instr_cmpxchg,
	instr_cmpxchg8b,
	instr_cpuid,
	instr_cwd,
	instr_daa,
	instr_das,
	instr_dec,
	instr_div,
	instr_emms,
	instr_enter,
	instr_esc0,
	instr_esc1,
	instr_esc2,
	instr_esc3,
	instr_esc4,
	instr_esc5,
	instr_esc6,
	instr_esc7,

	/* these must be in this order */
	instr_fadd,
	instr_fmul,
	instr_fcom,
	instr_fcomp,
	instr_fsub,
	instr_fsubr,
	instr_fdiv,
	instr_fdivr,

	/* these too */
	instr_fld,
	instr_0, /* placeholder */
	instr_fst,
	instr_fstp,
	instr_fldenv,
	instr_fldcw,
	instr_fstenv,
	instr_fstcw,

	/* again */
	instr_fiadd,
	instr_fimul,
	instr_ficom,
	instr_ficomp,
	instr_fisub,
	instr_fisubr,
	instr_fidiv,
	instr_fidivr,

	/* these must be in this order */
	instr_fld1,
	instr_fldl2t,
	instr_fldl2e,
	instr_fldpi,
	instr_fldlg2,
	instr_fldln2,
	instr_fldz,
	instr_1,		/* another placeholder */
	instr_f2xm1,
	instr_fyl2x,
	instr_fptan,
	instr_fpatan,
	instr_fxtract,
	instr_fprem1,
	instr_fdecstp,
	instr_fincstp,
	instr_fprem,
	instr_fyl2xp1,
	instr_fsqrt,
	instr_fsincos,
	instr_frndint,
	instr_fscale,
	instr_fsin,
	instr_fcos,

	instr_fabs,
	instr_faddp,
	instr_fbld,
	instr_fbstp,
	instr_fchs,
	instr_fclex,
	instr_fcmovb,
	instr_fcmovbe,
	instr_fcmove,
	instr_fcmovnb,
	instr_fcmovnbe,
	instr_fcmovne,
	instr_fcmovnu,
	instr_fcmovu,
	instr_fcomi,
	instr_fcomip,
	instr_fcompp,
	instr_fdivp,
	instr_fdivrp,
	instr_ffree,
	instr_fild,
	instr_finit,
	instr_fist,
	instr_fistp,
	instr_fmulp,
	instr_fnop,
	instr_frstor,
	instr_fsave,
	instr_fstsw,
	instr_fsubp,
	instr_fsubrp,
	instr_ftst,
	instr_fucom,
	instr_fucomi,
	instr_fucomip,
	instr_fucomp,
	instr_fucompp,
	instr_fxam,
	instr_fxch,

	instr_hlt,
	instr_idiv,
	instr_imul,
	instr_in,
	instr_inc,
	instr_ins,
	instr_int,
	instr_into,
	instr_invd,
	instr_invlpg,
	instr_iret,

	/* these must be in this order */
	instr_jo,
	instr_jno,
	instr_jc,
	instr_jnc,
	instr_jz,
	instr_jnz,
	instr_jbe,
	instr_ja,
	instr_js,
	instr_jns,
	instr_jp,
	instr_jnp,
	instr_jl,
	instr_jnl,
	instr_jle,
	instr_jg,
	
	instr_jcxz,
	instr_jmp,
	instr_lahf,
	instr_lar,
	instr_lds,
	instr_lea,
	instr_leave,
	instr_les,
	instr_lfs,
	instr_lgdt,
	instr_lgs,
	instr_lidt,
	instr_lldt,
	instr_lmsw,
	instr_loadall,
	instr_loadall286,
	instr_lock,
	instr_lods,
	instr_loop,
	instr_loopnz,
	instr_loopz,
	instr_lsl,
	instr_lss,
	instr_ltr,
	instr_mov,
	instr_movd,
	instr_movq,
	instr_movs,
	instr_movsx,
	instr_movzx,
	instr_mul,
	instr_neg,
	instr_nop,
	instr_not,
	instr_or,
	instr_out,
	instr_outs,
	instr_pcmpeqb,
	instr_pcmpeqw,
	instr_pcmpeqd,
	instr_pop,
	instr_popa,
	instr_popf,
	
	/* these must be in this order */
	instr_punpcklbw,
	instr_punpcklwd,
	instr_punpckldq,
	instr_packsswb,
	instr_pcmpgtb,
	instr_pcmpgtw,
	instr_pcmpgtd,
	instr_packuswb,
	instr_punpckhbw,
	instr_punpckhwd,
	instr_punpckhdq,
	instr_packssdw,
	
	instr_psrlw,
	instr_psrld,
	instr_psrlq,
	instr_pmullw,
	instr_psubusb,
	instr_psubusw,
	instr_pand,
	instr_paddusb,
	instr_paddusw,
	instr_pandn,
	instr_psraw,
	instr_psrad,
	instr_pmulhw,
	instr_psubsb,
	instr_psubsw,
	instr_por,
	instr_paddsb,
	instr_paddsw,
	instr_pxor,
	instr_psllw,
	instr_pslld,
	instr_psllq,
	instr_pmaddwd,
	instr_psubb,
	instr_psubw,
	instr_psubd,
	instr_paddb,
	instr_paddw,
	instr_paddd,
	
	instr_push,
	instr_pusha,
	instr_pushf,
	instr_rcl,
	instr_rcr,
	instr_rdtsc,
	instr_rdmsr,
	instr_rdpmc,
	instr_rep,
	instr_repnz,
	instr_retf,
	instr_retn,
	instr_rol,
	instr_ror,
	instr_rsm,
	instr_sahf,
	instr_sar,
	instr_sbb,
	instr_scas,
	instr_seg,
	instr_setalc,

	/* these must be in this order */
	instr_seto,
	instr_setno,
	instr_setc,
	instr_setnc,
	instr_setz,
	instr_setnz,
	instr_setbe,
	instr_seta,
	instr_sets,
	instr_setns,
	instr_setp,
	instr_setnp,
	instr_setl,
	instr_setnl,
	instr_setle,
	instr_setg,

	instr_sgdt,
	instr_shl,
	instr_shld,
	instr_shr,
	instr_shrd,
	instr_sidt,
	instr_sldt,
	instr_smsw,
	instr_stc,
	instr_std,
	instr_sti,
	instr_stos,
	instr_str,
	instr_sub,
	instr_sysenter,
	instr_sysexit,
	instr_test,
	instr_umov,
	instr_verr,
	instr_verw,
	instr_wait,
	instr_xadd,
	instr_xchg,
	instr_xlat,
	instr_xor,
	instr_wbinvd,
	instr_wrmsr,
	
	/* KNI instructions */
	instr_addps,
	instr_addss,
	instr_andnps,
	instr_andps,
	instr_cmpps,
	instr_cmpss,
	instr_comiss,
	instr_cvtpi2ps,
	instr_cvtps2pi,
	instr_cvtsi2ss,
	instr_cvtss2si,
	instr_cvttps2pi,
	instr_cvttss2si,
	instr_divps,
	instr_divss,
	instr_fxrstor,
	instr_fxsave,
	instr_ldmxcsr,
	instr_maskmovq,
	instr_maxps,
	instr_maxss,
	instr_minps,
	instr_minss,
	instr_movaps,
	instr_movhps,
	instr_movlps,
	instr_movmskps,
	instr_movntps,
	instr_movntq,
	instr_movss,
	instr_movups,
	instr_mulps,
	instr_mulss,
	instr_orps,
	instr_pavgb,
	instr_pavgw,
	instr_pextrw,
	instr_pinsrw,
	instr_pmaxsw,
	instr_pmaxub,
	instr_pminsw,
	instr_pminub,
	instr_pmovmskb,
	instr_pmulhuw,
	instr_prefetchnta,
	instr_prefetcht0,
	instr_prefetcht1,
	instr_prefetcht2,
	instr_psadbw,
	instr_pshufw,
	instr_rcpps,
	instr_rcpss,
	instr_rsqrtps,
	instr_rsqrtss,
	instr_sfence,
	instr_shufps,
	instr_sqrtps,
	instr_sqrtss,
	instr_stmxcsr,
	instr_subps,
	instr_subss,
	instr_ucomiss,
	instr_unpckhps,
	instr_unpcklps,
	instr_xorps,

	instr_invalid,

	instr_last
} instr_t;

typedef struct {
	uchar opcode;
	instr_t instruction;
} _PACKED instr0_desc_t;

typedef struct {
	uchar opcode;
	instr_t instruction;
	operand_t op1;
} _PACKED instr1_desc_t;

typedef struct {
	uchar opcode;
	instr_t instruction;
	operand_t op1, op2;
} _PACKED instr2_desc_t;

typedef struct {
	uchar opcode;
	instr_t instruction;
	operand_t op1, op2, op3;
} _PACKED instr3_desc_t;

typedef struct {
	uint8 opcode;
	operand_t op1, op2;
	const instr_t *instrs;
	const operand_t *op1s;
} _PACKED opcode_extension_t;

typedef struct {
	uint8 opcode1;
	uint8 opcode2;
	instr_t instruction;
} _PACKED no_arg_fp_instr_t;

typedef struct {
	uint8 opcode1;
	uint8 opcode2_base;
	instr_t instruction;
} _PACKED two_args_fp_instr_t;

#define END_EXTENSIONS 0x20

#define DISASM_USER_FLAGS \
	(B_DISASM_FLAG_OP_SIZE_16 | B_DISASM_FLAG_ADDR_SIZE_16 | \
	 B_DISASM_FLAG_INTEL_STYLE | B_DISASM_FLAG_RELATIVE_ADDRESSES | \
	 B_DISASM_FLAG_DEBUG_SERVER_PRIVATE)

#endif
