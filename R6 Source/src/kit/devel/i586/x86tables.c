#include <support/SupportDefs.h>

#include "dis_p.h"

const operand_t no_operand = NO_OPERAND;

const char *reg8[8] = {
	"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

const char *reg16[8] = {
	"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

const char *reg32[8] = {
	"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
};

const char *regmmx[8] = {
	"mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7"
};

const char *regxmm[8] = {
	"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
};

const char *regseg[8] = {
	"es", "cs", "ss", "ds", "fs", "gs"
};

const char *regfp[8] = {
	"st(0)", "st(1)", "st(2)", "st(3)",
	"st(4)", "st(5)", "st(6)", "st(7)",
};

const char *regc[8] = {
	"cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7"
};

const char *regd[8] = {
	"dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7"
};

const char *regt[8] = {
	"tr0", "tr1", "tr2", "tr3", "tr4", "tr5", "tr6", "tr7"
};

const instr0_desc_t instr0[] = {
	{ 0x27, instr_daa },
	{ 0x2f, instr_das },
	{ 0x37, instr_aaa },

	{ 0x60, instr_pusha },
	{ 0x61, instr_popa },

	{ 0x90, instr_nop },
	
	{ 0x98, instr_cbw },
	{ 0x99, instr_cwd },
	{ 0x9b, instr_wait },
	{ 0x9c, instr_pushf },
	{ 0x9d, instr_popf },
	{ 0x9e, instr_sahf },
	{ 0x9f, instr_lahf },
	
	{ 0xc3, instr_retn },
	{ 0xc9, instr_leave },
	{ 0xcb, instr_retf },
	{ 0xce, instr_into },
	{ 0xcf, instr_iret },
	
	{ 0xd6, instr_setalc },
	{ 0xd7, instr_xlat },

	{ 0xf0, instr_lock },
	{ 0xf2, instr_repnz },
	{ 0xf3, instr_rep },
	{ 0xf4, instr_hlt },
	{ 0xf5, instr_cmc },
	{ 0xf8, instr_clc },
	{ 0xf9, instr_stc },
	{ 0xfa, instr_cli },
	{ 0xfb, instr_sti },
	{ 0xfc, instr_cld },
	{ 0xfd, instr_std },

	{ 0,	instr_last }
};

const instr1_desc_t instr1[] = {
	{ 0x06, instr_push, { op_addr_reg, op_type_sreg, reg_es } },
	{ 0x07, instr_pop,	{ op_addr_reg, op_type_sreg, reg_es } },
	{ 0x0e, instr_push, { op_addr_reg, op_type_sreg, reg_cs } },
	{ 0x16, instr_push, { op_addr_reg, op_type_sreg, reg_ss } },
	{ 0x17, instr_pop,	{ op_addr_reg, op_type_sreg, reg_ss } },
	{ 0x1e, instr_push, { op_addr_reg, op_type_sreg, reg_ds } },
	{ 0x1f, instr_pop,	{ op_addr_reg, op_type_sreg, reg_ds } },
	{ 0x26, instr_seg,	{ op_addr_reg, op_type_sreg, reg_es } },
	{ 0x2e, instr_seg,	{ op_addr_reg, op_type_sreg, reg_cs } },
	{ 0x36, instr_seg,	{ op_addr_reg, op_type_sreg, reg_ss } },
	{ 0x3e, instr_seg,	{ op_addr_reg, op_type_sreg, reg_ds } },

	{ 0x64, instr_seg,	{ op_addr_reg, op_type_sreg, reg_fs } },
	{ 0x65, instr_seg,	{ op_addr_reg, op_type_sreg, reg_gs } },
	{ 0x68, instr_push,	{ op_addr_I, op_type_v, 0 } },
	{ 0x6a, instr_push,	{ op_addr_I, op_type_b, 0 } },

#if 0
	{ 0x70, instr_jo,	{ op_addr_J, op_type_b, 0 } },
	{ 0x71, instr_jno,	{ op_addr_J, op_type_b, 0 } },
	{ 0x72, instr_jc,	{ op_addr_J, op_type_b, 0 } },
	{ 0x73, instr_jnc,	{ op_addr_J, op_type_b, 0 } },
	{ 0x74, instr_jz,	{ op_addr_J, op_type_b, 0 } },
	{ 0x75, instr_jnz,	{ op_addr_J, op_type_b, 0 } },
	{ 0x76, instr_jbe,	{ op_addr_J, op_type_b, 0 } },
	{ 0x77, instr_ja,	{ op_addr_J, op_type_b, 0 } },
	{ 0x78, instr_js,	{ op_addr_J, op_type_b, 0 } },
	{ 0x79, instr_jns,	{ op_addr_J, op_type_b, 0 } },
	{ 0x7a, instr_jp,	{ op_addr_J, op_type_b, 0 } },
	{ 0x7b, instr_jnp,	{ op_addr_J, op_type_b, 0 } },
	{ 0x7c, instr_jl,	{ op_addr_J, op_type_b, 0 } },
	{ 0x7d, instr_jnl,	{ op_addr_J, op_type_b, 0 } },
	{ 0x7e, instr_jle,	{ op_addr_J, op_type_b, 0 } },
	{ 0x7f, instr_jg,	{ op_addr_J, op_type_b, 0 } },
#endif

	{ 0x8f, instr_pop,	{ op_addr_E, op_type_v, 0 } },

	{ 0x9a, instr_call,	{ op_addr_A, op_type_p, 0 } },
	
	{ 0xc2, instr_retn, { op_addr_I, op_type_w, 0 } },
	{ 0xca, instr_retf, { op_addr_I, op_type_w, 0 } },
	{ 0xcc, instr_int,	{ op_addr_immed, op_type_b, 3 } },
	{ 0xcd, instr_int,	{ op_addr_I, op_type_b, 0 } },

	{ 0xd4, instr_aam,	{ op_addr_I, op_type_b, 0 } },
	{ 0xd5, instr_aad,	{ op_addr_I, op_type_b, 0 } },

	{ 0xe0, instr_loopnz,{ op_addr_J, op_type_b, 0 } },
	{ 0xe1, instr_loopz,{ op_addr_J, op_type_b, 0 } },
	{ 0xe2, instr_loop,	{ op_addr_J, op_type_b, 0 } },
	{ 0xe3, instr_jcxz,	{ op_addr_J, op_type_b, 0 } },
	{ 0xe8, instr_call,	{ op_addr_J, op_type_v, 0 } },
	{ 0xe9, instr_jmp,	{ op_addr_J, op_type_v, 0 } },
	{ 0xea, instr_jmp,	{ op_addr_A, op_type_p, 0 } },
	{ 0xeb, instr_jmp,	{ op_addr_J, op_type_b, 0 } },

	{ 0xf1, instr_int,	{ op_addr_immed, op_type_b, 1 } },

	{ 0,	instr_last, NO_OPERAND }
};
	
const instr2_desc_t instr2[] = {
	{ 0x62, instr_bound, { op_addr_G, op_type_v, 0 }, { op_addr_M, op_type_a, 0 } },
	{ 0x63, instr_arpl, { op_addr_E, op_type_w, 0 }, { op_addr_G, op_type_w, 0 } },

	{ 0x6c, instr_ins, { op_addr_Y, op_type_b, 0 }, { op_addr_reg, op_type_w, reg_dx } },
	{ 0x6d, instr_ins, { op_addr_Y, op_type_v, 0 }, { op_addr_reg, op_type_w, reg_dx } },
	{ 0x6e, instr_outs, { op_addr_reg, op_type_w, reg_dx }, { op_addr_X, op_type_b, 0 } },
	{ 0x6f, instr_outs, { op_addr_reg, op_type_w, reg_dx }, { op_addr_X, op_type_v, 0 } },

	{ 0x84, instr_test, { op_addr_E, op_type_b, 0 }, { op_addr_G, op_type_b, 0 } },
	{ 0x85, instr_test, { op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 } },
	{ 0x86, instr_xchg, { op_addr_E, op_type_b, 0 }, { op_addr_G, op_type_b, 0 } },
	{ 0x87, instr_xchg, { op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 } },
	{ 0x88, instr_mov,	{ op_addr_E, op_type_b, 0 }, { op_addr_G, op_type_b, 0 } },
	{ 0x89, instr_mov,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 } },
	{ 0x8a, instr_mov,	{ op_addr_G, op_type_b, 0 }, { op_addr_E, op_type_b, 0 } },
	{ 0x8b, instr_mov,	{ op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x8c, instr_mov,	{ op_addr_E, op_type_w, 0 }, { op_addr_S, op_type_w, 0 } },
	{ 0x8d, instr_lea,	{ op_addr_G, op_type_v, 0 }, { op_addr_M, op_type_v, 0 } },
	{ 0x8e, instr_mov,	{ op_addr_S, op_type_w, 0 }, { op_addr_E, op_type_w, 0 } },

	{ 0xa0, instr_mov,	{ op_addr_reg, op_type_b, reg_al }, { op_addr_O, op_type_b, 0 } },
	{ 0xa1, instr_mov,	{ op_addr_reg, op_type_v, reg_eax }, { op_addr_O, op_type_v, 0 } },
	{ 0xa2, instr_mov,	{ op_addr_O, op_type_b, reg_al }, { op_addr_reg, op_type_b, 0 } },
	{ 0xa3, instr_mov,	{ op_addr_O, op_type_v, reg_eax }, { op_addr_reg, op_type_v, 0 } },
	{ 0xa4, instr_movs,	{ op_addr_X, op_type_b, 0 }, { op_addr_Y, op_type_b, 0 } },
	{ 0xa5, instr_movs,	{ op_addr_X, op_type_v, 0 }, { op_addr_Y, op_type_v, 0 } },
	{ 0xa6, instr_cmps,	{ op_addr_X, op_type_b, 0 }, { op_addr_Y, op_type_b, 0 } },
	{ 0xa7, instr_cmps,	{ op_addr_X, op_type_v, 0 }, { op_addr_Y, op_type_v, 0 } },
	{ 0xa8, instr_test, { op_addr_reg, op_type_b, reg_al }, { op_addr_I, op_type_b, 0 } },
	{ 0xa9, instr_test, { op_addr_reg, op_type_v, reg_eax }, { op_addr_I, op_type_v, 0 } },
	{ 0xaa, instr_stos, { op_addr_Y, op_type_b, 0 }, { op_addr_reg, op_type_b, reg_al } },
	{ 0xab, instr_stos, { op_addr_Y, op_type_v, 0 }, { op_addr_reg, op_type_v, reg_eax } },
	{ 0xac, instr_lods, { op_addr_reg, op_type_b, reg_al }, { op_addr_X, op_type_b, 0 } },
	{ 0xad, instr_lods, { op_addr_reg, op_type_v, reg_eax }, { op_addr_X, op_type_v, 0 } },
	{ 0xae, instr_scas, { op_addr_reg, op_type_b, reg_al }, { op_addr_Y, op_type_b, 0 } },
	{ 0xaf, instr_scas, { op_addr_reg, op_type_v, reg_eax }, { op_addr_Y, op_type_v, 0 } },

	{ 0xc4, instr_les,	{ op_addr_G, op_type_v, reg_eax }, { op_addr_M, op_type_p, 0 } },
	{ 0xc5, instr_lds,	{ op_addr_G, op_type_v, reg_eax }, { op_addr_M, op_type_p, 0 } },
	{ 0xc6, instr_mov,	{ op_addr_E, op_type_b, 0 }, { op_addr_I, op_type_b, 0 } },
	{ 0xc7, instr_mov,	{ op_addr_E, op_type_v, 0 }, { op_addr_I, op_type_v, 0 } },
	{ 0xc8, instr_enter,{ op_addr_I, op_type_w, 0 }, { op_addr_I, op_type_b, 0 } },

	{ 0xe4, instr_in,	{ op_addr_reg, op_type_b, reg_al }, { op_addr_I, op_type_b, 0 } },
	{ 0xe5, instr_in,	{ op_addr_reg, op_type_v, reg_eax }, { op_addr_I, op_type_b, 0 } },
	{ 0xe6, instr_out,	{ op_addr_I, op_type_b, 0 }, { op_addr_reg, op_type_b, reg_al } },
	{ 0xe7, instr_out,	{ op_addr_I, op_type_b, 0 }, { op_addr_reg, op_type_v, reg_eax } },
	{ 0xec, instr_in,	{ op_addr_reg, op_type_b, reg_al }, { op_addr_reg, op_type_w, reg_dx } },
	{ 0xed, instr_in,	{ op_addr_reg, op_type_v, reg_eax }, { op_addr_reg, op_type_w, reg_dx } },
	{ 0xee, instr_out,	{ op_addr_reg, op_type_w, reg_dx }, { op_addr_reg, op_type_b, reg_al } },
	{ 0xef, instr_out,	{ op_addr_reg, op_type_w, reg_dx }, { op_addr_reg, op_type_v, reg_eax } },

	{ 0,	instr_last, NO_OPERAND, NO_OPERAND }
};

const instr3_desc_t instr3[] = {
	{ 0x69, instr_imul, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 }, { op_addr_I, op_type_v, 0 } },
	{ 0x6b, instr_imul, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 }, { op_addr_I, op_type_b, 0 } },

	{ 0,	instr_last, NO_OPERAND, NO_OPERAND, NO_OPERAND }
};

static const instr_t group1[8] = {
	instr_add, instr_or, instr_adc, instr_sbb,
	instr_and, instr_sub, instr_xor, instr_cmp
};

static const instr_t group2[8] = {
	instr_rol, instr_ror, instr_rcl, instr_rcr,
	instr_shl, instr_shr, instr_last, instr_sar
};

static const instr_t group3[8] = {
	instr_test, instr_last, instr_not, instr_neg,
	instr_mul, instr_imul, instr_div, instr_idiv
};

static const instr_t group4[8] = {
	instr_inc, instr_dec, instr_last, instr_last,
	instr_last, instr_last, instr_last, instr_last
};

static const instr_t group5[8] = {
	instr_inc, instr_dec, instr_call, instr_call,
	instr_jmp, instr_jmp, instr_push, instr_last
};

static const operand_t group5ops[8] = {
	{ op_addr_E, op_type_v },
	{ op_addr_E, op_type_v },
	{ op_addr_E, op_type_v },
	{ op_addr_E, op_type_p },
	{ op_addr_E, op_type_v },
	{ op_addr_E, op_type_p },
	{ op_addr_E, op_type_v },
	NO_OPERAND
};

static const instr_t group6[8] = {
	instr_sldt, instr_str, instr_lldt, instr_ltr,
	instr_verr, instr_verw, instr_last, instr_last
};

static const operand_t group7ops[8] = {
	{ op_addr_M, op_type_s },
	{ op_addr_M, op_type_s },
	{ op_addr_M, op_type_s },
	{ op_addr_M, op_type_s },
	{ op_addr_E, op_type_w },
	NO_OPERAND,
	{ op_addr_E, op_type_w },
	NO_OPERAND
};

static const instr_t group7[8] = {
	instr_sgdt, instr_sidt, instr_lgdt, instr_lidt,
	instr_smsw, instr_last, instr_lmsw, instr_invlpg
};

static const instr_t group7a[8] = {
	instr_fxsave, instr_fxrstor, instr_ldmxcsr, instr_stmxcsr,
	instr_last, instr_last, instr_last, instr_sfence
};

static const instr_t group8[8] = {
	instr_last, instr_last, instr_last, instr_last,
	instr_bt, instr_bts, instr_btr, instr_btc
};

static const instr_t group9[8] = {
	instr_last, instr_cmpxchg8b, instr_last, instr_last,
	instr_last, instr_last, instr_last, instr_last
};

static const instr_t groupAa[8] = {
	instr_last, instr_last, instr_psrlw, instr_last,
	instr_psraw, instr_last, instr_psllw, instr_last
};

static const instr_t groupAb[8] = {
	instr_last, instr_last, instr_psrld, instr_last,
	instr_psrad, instr_last, instr_pslld, instr_last
};

static const instr_t groupAc[8] = {
	instr_last, instr_last, instr_psrlq, instr_last,
	instr_last, instr_last, instr_psllq, instr_last
};

static const instr_t groupKNI[8] = {
	instr_prefetchnta, instr_prefetcht0,
	instr_prefetcht1,  instr_prefetcht2,
	instr_last, instr_last, instr_last, instr_last
};

const opcode_extension_t opcode_extensions[] = {
	{ 0x80, { op_addr_E, op_type_b, 0 }, { op_addr_I, op_type_b, 0 }, group1, NULL },
	{ 0x81, { op_addr_E, op_type_v, 0 }, { op_addr_I, op_type_v, 0 }, group1, NULL },
	{ 0x82, { op_addr_E, op_type_v, 0 }, { op_addr_I, op_type_b, 0 }, group1, NULL },
	{ 0x83, { op_addr_E, op_type_v, 0 }, { op_addr_I, op_type_b, 0 }, group1, NULL },

	{ 0xc0, { op_addr_E, op_type_b, 0 }, { op_addr_I, op_type_b, 0 }, group2, NULL },
	{ 0xc1, { op_addr_E, op_type_v, 0 }, { op_addr_I, op_type_b, 0 }, group2, NULL },

	{ 0xd0, { op_addr_E, op_type_b, 0 }, { op_addr_immed, op_type_b, 1 }, group2, NULL },
	{ 0xd1, { op_addr_E, op_type_v, 0 }, { op_addr_immed, op_type_b, 1 }, group2, NULL },
	{ 0xd2, { op_addr_E, op_type_b, 0 }, { op_addr_reg, op_type_b, reg_cl }, group2, NULL },
	{ 0xd3, { op_addr_E, op_type_v, 0 }, { op_addr_reg, op_type_b, reg_cl }, group2, NULL },
	
	{ 0xf6, { op_addr_E, op_type_b, 0 }, NO_OPERAND, group3, NULL },
	{ 0xf7, { op_addr_E, op_type_v, 0 }, NO_OPERAND, group3, NULL },

	{ 0xfe, { op_addr_E, op_type_b, 0 }, NO_OPERAND, group4, NULL },
	{ 0xff, NO_OPERAND, NO_OPERAND, group5, group5ops },
	
	{ END_EXTENSIONS, NO_OPERAND, NO_OPERAND, NULL }
};


const opcode_extension_t opcode_extensions_0f[] = {
	{ 0, { op_addr_E, op_type_w, 0 }, NO_OPERAND, group6, NULL },
	{ 1, NO_OPERAND, NO_OPERAND, group7, group7ops },
	{ 0x18, { op_addr_M, op_type_b, 0 }, NO_OPERAND, groupKNI, NULL },
	{ 0xae, { op_addr_M, op_type_v, 0 }, NO_OPERAND, group7a, NULL }, /* vyt: size incorrect */
	{ 0xba, { op_addr_E, op_type_v, 0 }, { op_addr_I, op_type_b, 0 }, group8, NULL },
	{ 0xc7, { op_addr_M, op_type_q, 0 }, NO_OPERAND, group9, NULL },
	{ 0x71, { op_addr_Qmmx, op_type_q, 0 }, { op_addr_I, op_type_b, 0 }, groupAa, NULL },
	{ 0x72, { op_addr_Qmmx, op_type_q, 0 }, { op_addr_I, op_type_b, 0 }, groupAb, NULL },
	{ 0x73, { op_addr_Qmmx, op_type_q, 0 }, { op_addr_I, op_type_b, 0 }, groupAc, NULL },
	{ END_EXTENSIONS, NO_OPERAND, NO_OPERAND, NULL }
};

const opcode_extension_t opcode_extensions_f30f[] = {
	{ END_EXTENSIONS, NO_OPERAND, NO_OPERAND, NULL }
};

const instr0_desc_t instr0_0f[] = {
	{ 0x05, instr_loadall286 },
	{ 0x06, instr_clts },
	{ 0x07, instr_loadall },
	{ 0x08, instr_invd },
	{ 0x09, instr_wbinvd },
	{ 0x0a, instr_cflsh },
	{ 0x0b, instr_invalid },

	{ 0x30, instr_wrmsr },
	{ 0x31, instr_rdtsc },
	{ 0x32, instr_rdmsr },
	{ 0x33, instr_rdpmc },
	{ 0x34, instr_sysenter },
	{ 0x35, instr_sysexit },

	{ 0x77, instr_emms },

	{ 0xa2, instr_cpuid },
	{ 0xaa, instr_rsm },

	{ 0xb9, instr_invalid },
	
	{ 0, instr_last }
};

const instr1_desc_t instr1_0f[] = {
#if 0
	{ 0x80, instr_jo,	{ op_addr_J, op_type_v, 0 } },
	{ 0x81, instr_jno,	{ op_addr_J, op_type_v, 0 } },
	{ 0x82, instr_jc,	{ op_addr_J, op_type_v, 0 } },
	{ 0x83, instr_jnc,	{ op_addr_J, op_type_v, 0 } },
	{ 0x84, instr_jz,	{ op_addr_J, op_type_v, 0 } },
	{ 0x85, instr_jnz,	{ op_addr_J, op_type_v, 0 } },
	{ 0x86, instr_jbe,	{ op_addr_J, op_type_v, 0 } },
	{ 0x87, instr_ja,	{ op_addr_J, op_type_v, 0 } },
	{ 0x88, instr_js,	{ op_addr_J, op_type_v, 0 } },
	{ 0x89, instr_jns,	{ op_addr_J, op_type_v, 0 } },
	{ 0x8a, instr_jp,	{ op_addr_J, op_type_v, 0 } },
	{ 0x8b, instr_jnp,	{ op_addr_J, op_type_v, 0 } },
	{ 0x8c, instr_jl,	{ op_addr_J, op_type_v, 0 } },
	{ 0x8d, instr_jnl,	{ op_addr_J, op_type_v, 0 } },
	{ 0x8e, instr_jle,	{ op_addr_J, op_type_v, 0 } },
	{ 0x8f, instr_jg,	{ op_addr_J, op_type_v, 0 } },

	{ 0x90, instr_seto,	{ op_addr_E, op_type_b, 0 } },
	{ 0x91, instr_setno,{ op_addr_E, op_type_b, 0 } },
	{ 0x92, instr_setc,	{ op_addr_E, op_type_b, 0 } },
	{ 0x93, instr_setnc,{ op_addr_E, op_type_b, 0 } },
	{ 0x94, instr_setz,	{ op_addr_E, op_type_b, 0 } },
	{ 0x95, instr_setnz,{ op_addr_E, op_type_b, 0 } },
	{ 0x96, instr_setbe,{ op_addr_E, op_type_b, 0 } },
	{ 0x97, instr_seta,	{ op_addr_E, op_type_b, 0 } },
	{ 0x98, instr_sets,	{ op_addr_E, op_type_b, 0 } },
	{ 0x99, instr_setns,{ op_addr_E, op_type_b, 0 } },
	{ 0x9a, instr_setp,	{ op_addr_E, op_type_b, 0 } },
	{ 0x9b, instr_setnp,{ op_addr_E, op_type_b, 0 } },
	{ 0x9c, instr_setl,	{ op_addr_E, op_type_b, 0 } },
	{ 0x9d, instr_setnl,{ op_addr_E, op_type_b, 0 } },
	{ 0x9e, instr_setle,{ op_addr_E, op_type_b, 0 } },
	{ 0x9f, instr_setg,	{ op_addr_E, op_type_b, 0 } },
#endif

	{ 0xa0, instr_push, { op_addr_reg, op_type_sreg, reg_fs } },
	{ 0xa1, instr_pop,	{ op_addr_reg, op_type_sreg, reg_fs } },
	{ 0xa8, instr_push, { op_addr_reg, op_type_sreg, reg_gs } },
	{ 0xa9, instr_pop,	{ op_addr_reg, op_type_sreg, reg_gs } },

	{ 0xb2, instr_lss,	{ op_addr_M, op_type_p, 0 } },
	{ 0xb4, instr_lfs,	{ op_addr_M, op_type_p, 0 } },
	{ 0xb5, instr_lgs,	{ op_addr_M, op_type_p, 0 } },

#if 0
	{ 0xc8, instr_bswap,{ op_addr_reg, op_type_d, reg_eax } },
	{ 0xc9, instr_bswap,{ op_addr_reg, op_type_d, reg_ecx } },
	{ 0xca, instr_bswap,{ op_addr_reg, op_type_d, reg_edx } },
	{ 0xcb, instr_bswap,{ op_addr_reg, op_type_d, reg_ebx } },
	{ 0xcc, instr_bswap,{ op_addr_reg, op_type_d, reg_esp } },
	{ 0xcd, instr_bswap,{ op_addr_reg, op_type_d, reg_ebp } },
	{ 0xce, instr_bswap,{ op_addr_reg, op_type_d, reg_esi } },
	{ 0xcf, instr_bswap,{ op_addr_reg, op_type_d, reg_edi } },
#endif

	{ 0, instr_last, NO_OPERAND }
};

const instr2_desc_t instr2_0f[] = {
	{ 0x02, instr_lar, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_w, 0 } },
	{ 0x03, instr_lsl, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_w, 0 } },

//	{ 0x10, instr_umov,{ op_addr_M, op_type_b, 0 }, { op_addr_G, op_type_b, 0 } },
//	{ 0x11, instr_umov,{ op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
//	{ 0x12, instr_umov,{ op_addr_M, op_type_b, 0 }, { op_addr_G, op_type_b, 0 } },
//	{ 0x13, instr_umov,{ op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x10, instr_movups, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x11, instr_movups, { op_addr_Q, op_type_o, 0 }, { op_addr_P, op_type_o, 0 } },
	{ 0x12, instr_movlps, { op_addr_P, op_type_o, 0 }, { op_addr_M, op_type_q, 0 } },
	{ 0x13, instr_movlps, { op_addr_M, op_type_q, 0 }, { op_addr_P, op_type_o, 0 } },
	{ 0x14, instr_unpcklps, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_q, 0 } },
	{ 0x15, instr_unpckhps, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_q, 0 } },
	{ 0x16, instr_movhps, { op_addr_P, op_type_o, 0 }, { op_addr_M, op_type_q, 0 } },
	{ 0x17, instr_movhps, { op_addr_M, op_type_q, 0 }, { op_addr_P, op_type_o, 0 } },

	{ 0x20, instr_mov, { op_addr_R, op_type_d, 0 }, { op_addr_C, op_type_d, 0 } },
	{ 0x21, instr_mov, { op_addr_R, op_type_d, 0 }, { op_addr_D, op_type_d, 0 } },
	{ 0x22, instr_mov, { op_addr_C, op_type_d, 0 }, { op_addr_R, op_type_d, 0 } },
	{ 0x23, instr_mov, { op_addr_D, op_type_d, 0 }, { op_addr_R, op_type_d, 0 } },
	{ 0x28, instr_movaps, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x29, instr_movaps, { op_addr_Q, op_type_o, 0 }, { op_addr_P, op_type_o, 0 } },
	{ 0x2a, instr_cvtpi2ps, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_q, 0 } },
	{ 0x2b, instr_movntps, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_q, 0 } },
	{ 0x2c, instr_cvttps2pi, { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x2d, instr_cvtps2pi, { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x2e, instr_ucomiss, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x2f, instr_comiss, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },

#if 0
	{ 0x40, instr_cmovo, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x41, instr_cmovno, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x42, instr_cmovc, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x43, instr_cmovnc, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x44, instr_cmovz, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x45, instr_cmovnz, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x46, instr_cmovbe, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x47, instr_cmova, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x48, instr_cmovs, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x49, instr_cmovns, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x4a, instr_cmovp, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x4b, instr_cmovnp, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x4c, instr_cmovl, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x4d, instr_cmovnl, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x4e, instr_cmovle, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0x4f, instr_cmovg, { op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
#endif

	{ 0x50, instr_movmskps,	{ op_addr_G, op_type_d, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x51, instr_sqrtps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x52, instr_rsqrtps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x53, instr_rcpps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x54, instr_andps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x55, instr_andnps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x56, instr_orps,		{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x57, instr_xorps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x58, instr_addps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x59, instr_mulps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x5c, instr_subps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x5d, instr_minps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x5e, instr_divps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x5f, instr_maxps,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },

#if 0
	{ 0x60, instr_punpcklbw, { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x61, instr_punpcklwd, { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x62, instr_punpckldq, { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x63, instr_packsswb,  { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x64, instr_pcmpgtb,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x65, instr_pcmpgtw,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x66, instr_pcmpgtd,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x67, instr_packuswb,  { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x68, instr_punpckhbw, { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x69, instr_punpckhwd, { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x6a, instr_punpckhdq, { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x6b, instr_packssdw,  { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
#endif

	{ 0x6e, instr_movd,	{ op_addr_P, op_type_d, 0 }, { op_addr_E, op_type_d, 0 } },
	{ 0x6f, instr_movq,	{ op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_q, 0 } },

	{ 0x74, instr_pcmpeqb,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x75, instr_pcmpeqw,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x76, instr_pcmpeqd,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0x7e, instr_movd,	{ op_addr_E, op_type_d, 0 }, { op_addr_P, op_type_d, 0 } },
	{ 0x7f, instr_movq,	{ op_addr_Q, op_type_q, 0 }, { op_addr_P, op_type_q, 0 } },

	{ 0xa3, instr_bt,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 } },
	{ 0xab, instr_bts,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 } },
	{ 0xaf, instr_imul,	{ op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },

	{ 0xb0, instr_cmpxchg,	{ op_addr_E, op_type_b, 0 }, { op_addr_G, op_type_b, 0 } },
	{ 0xb1, instr_cmpxchg,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 } },
	{ 0xb3, instr_btr,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 } },
	{ 0xb6, instr_movzx,	{ op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_b, 0 } },
	{ 0xb7, instr_movzx,	{ op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_w, 0 } },
	{ 0xbb, instr_btc,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 } },
	{ 0xbc, instr_bsf,	{ op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0xbd, instr_bsr,	{ op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_v, 0 } },
	{ 0xbe, instr_movsx,	{ op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_b, 0 } },
	{ 0xbf, instr_movsx,	{ op_addr_G, op_type_v, 0 }, { op_addr_E, op_type_w, 0 } },

	{ 0xc0, instr_xadd,	{ op_addr_E, op_type_b, 0 }, { op_addr_G, op_type_b, 0 } },
	{ 0xc1, instr_xadd,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 } },

	{ 0xd1, instr_psrlw,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xd2, instr_psrld,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xd3, instr_psrlq,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xd5, instr_pmullw,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xd7, instr_pmovmskb,  { op_addr_G, op_type_d, 0 }, { op_addr_Q, op_type_q, 0 } },
	{ 0xd8, instr_psubusb,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xd9, instr_psubusw,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xda, instr_pminub,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xdb, instr_pand,      { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xdc, instr_paddusb,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xdd, instr_paddusw,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xde, instr_pmaxub,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xdf, instr_pandn,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },

	{ 0xe0, instr_pavgb,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xe1, instr_psraw,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xe2, instr_psrad,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xe3, instr_pavgw,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xe4, instr_pmulhuw,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xe5, instr_pmulhw,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xe7, instr_movntq,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xe8, instr_psubsb,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xe9, instr_psubsw,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xea, instr_pminsw,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xeb, instr_por,       { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xec, instr_paddsb,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xed, instr_paddsw,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xee, instr_pmaxsw,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xef, instr_pxor,      { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },

	{ 0xf1, instr_psllw,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xf2, instr_pslld,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xf3, instr_psllq,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xf5, instr_pmaddwd,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xf6, instr_psadbw,    { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xf7, instr_maskmovq,  { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xf8, instr_psubb,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xf9, instr_psubw,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xfa, instr_psubd,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xfc, instr_paddb,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xfd, instr_paddw,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },
	{ 0xfe, instr_paddd,     { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 } },

	{ 0, instr_last, NO_OPERAND, NO_OPERAND }
};

const instr3_desc_t instr3_0f[] = {
	{ 0x70, instr_pshufw,   { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_d, 0 }, { op_addr_I, op_type_b, 0 } },

	{ 0xa4, instr_shld,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 }, { op_addr_I, op_type_b, 0 } },
	{ 0xa5, instr_shld,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 }, { op_addr_reg, op_type_b, reg_cl } },
	{ 0xac, instr_shrd,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 }, { op_addr_I, op_type_b, 0 } },
	{ 0xad, instr_shrd,	{ op_addr_E, op_type_v, 0 }, { op_addr_G, op_type_v, 0 }, { op_addr_reg, op_type_b, reg_cl } },

	{ 0xc2, instr_cmpps, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 }, { op_addr_I, op_type_b, 0 } },
	{ 0xc4, instr_pinsrw, { op_addr_P, op_type_q, 0 }, { op_addr_E, op_type_d, 0 }, { op_addr_I, op_type_b, 0 } },
	{ 0xc5, instr_pextrw, { op_addr_R, op_type_d, 0 }, { op_addr_P, op_type_q, 0 }, { op_addr_I, op_type_b, 0 } },
	{ 0xc6, instr_shufps, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 }, { op_addr_I, op_type_b, 0 } },

	{ 0, instr_last, NO_OPERAND, NO_OPERAND, NO_OPERAND }
};

const instr0_desc_t instr0_f30f[] = {
	{ 0, instr_last }
};

const instr1_desc_t instr1_f30f[] = {
	{ 0, instr_last, NO_OPERAND }
};

const instr2_desc_t instr2_f30f[] = {
	{ 0x10, instr_movss, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x11, instr_movss, { op_addr_Q, op_type_o, 0 }, { op_addr_P, op_type_o, 0 } },

	{ 0x2a, instr_cvtsi2ss, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_q, 0 } },
	{ 0x2c, instr_cvttss2si, { op_addr_G, op_type_d, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x2d, instr_cvtss2si, { op_addr_P, op_type_q, 0 }, { op_addr_Q, op_type_o, 0 } },

	{ 0x51, instr_sqrtss,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x52, instr_rsqrtss,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x53, instr_rcpss,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x58, instr_addss,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x59, instr_mulss,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x5c, instr_subss,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x5d, instr_minss,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x5e, instr_divss,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },
	{ 0x5f, instr_maxss,	{ op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 } },

	{ 0, instr_last, NO_OPERAND, NO_OPERAND }
};

const instr3_desc_t instr3_f30f[] = {
	{ 0xc2, instr_cmpss, { op_addr_P, op_type_o, 0 }, { op_addr_Q, op_type_o, 0 }, { op_addr_I, op_type_b, 0 } },

	{ 0, instr_last, NO_OPERAND, NO_OPERAND, NO_OPERAND }
};

const no_arg_fp_instr_t no_arg_fp_instr[] = {
	{ 0xd9, 0xd0, instr_fnop },
	{ 0xd9, 0xe0, instr_fchs },
	{ 0xd9, 0xe1, instr_fabs },
	{ 0xd9, 0xe4, instr_ftst },
	{ 0xd9, 0xe5, instr_fxam },
	{ 0xda, 0xe1, instr_fucompp },
	{ 0xdb, 0xe2, instr_fclex },
	{ 0xdb, 0xe3, instr_finit },
	{ 0xde, 0xd9, instr_fcompp },
	{ 0x00, 0x00, instr_last }
};

const two_args_fp_instr_t two_args_fp_instr[] = {
	{ 0xd9, 0xc0, instr_fld },
	{ 0xd9, 0xc8, instr_fxch },
	{ 0xda, 0xc0, instr_fcmovb },
	{ 0xda, 0xc8, instr_fcmove },
	{ 0xda, 0xd0, instr_fcmovbe },
	{ 0xda, 0xd8, instr_fcmovu },
	{ 0xdb, 0xc0, instr_fcmovnb },
	{ 0xdb, 0xc8, instr_fcmovne },
	{ 0xdb, 0xd0, instr_fcmovnbe },
	{ 0xdb, 0xd8, instr_fcmovnu },
	{ 0xdb, 0xe8, instr_fucomi },
	{ 0xdb, 0xf0, instr_fcomi },
	{ 0xdf, 0xe8, instr_fucomip },
	{ 0xdf, 0xf0, instr_fcomip },
	{ 0x00, 0x00, instr_last }
};

const char *intel_instrs[] = {
	"aaa",
	"aad",
	"aam",
	"Adc",
	"Add",
	"And",
	"arpl",
	"bound",
	"bsf",
	"bsr",
	"bswap",
	"bt",
	"btc",
	"btr",
	"bts",
	"call",
	"cbw",
	"cflsh",
	"clc",
	"cld",
	"cli",
	"clts",
	"cmc",

	/* these must be in this order */
	"cmovo",
	"cmovno",
	"cmovc",
	"cmovnc",
	"cmovz",
	"cmovnz",
	"cmovbe",
	"cmova",
	"cmovs",
	"cmovns",
	"cmovp",
	"cmovnp",
	"cmovl",
	"cmovnl",
	"cmovle",
	"cmovg",

	"Cmp",
	"Cmps",
	"cmpxchg",
	"cmpxchg8b",
	"cpuid",
	"cwd",
	"daa",
	"das",
	"Dec",
	"Div",
	"emms",
	"enter",
	"esc0",
	"esc1",
	"esc2",
	"esc3",
	"esc4",
	"esc5",
	"esc6",
	"esc7",
	
	/* these must be in this order */
	"fadd",
	"fmul",
	"fcom",
	"fcomp",
	"fsub",
	"fsubr",
	"fdiv",
	"fdivr",

	/* these too */
	"fld",
	"0", /* placeholder */
	"fst",
	"fstp",
	"fldenv",
	"fldcw",
	"fstenv",
	"fstcw",

	/* again */
	"fiadd",
	"fimul",
	"ficom",
	"ficomp",
	"fisub",
	"fisubr",
	"fidiv",
	"fidivr",

	/* these must be in this order */
	"fld1",
	"fldl2t",
	"fldl2e",
	"fldpi",
	"fldlg2",
	"fldln2",
	"fldz",
	"1",		/* another placeholder */
	"f2xm1",
	"fyl2x",
	"fptan",
	"fpatan",
	"fxtract",
	"fprem1",
	"fdecstp",
	"fincstp",
	"fprem",
	"fyl2xp1",
	"fsqrt",
	"fsincos",
	"frndint",
	"fscale",
	"fsin",
	"fcos",

	"fabs",
	"faddp",
	"fbld",
	"fbstp",
	"fchs",
	"fclex",
	"fcmovb",
	"fcmovbe",
	"fcmove",
	"fcmovnb",
	"fcmovnbe",
	"fcmovne",
	"fcmovnu",
	"fcmovu",
	"fcomi",
	"fcomip",
	"fcompp",
	"fdivp",
	"fdivrp",
	"ffree",
	"fild",
	"finit",
	"fist",
	"fistp",
	"fmulp",
	"fnop",
	"frstor",
	"fsave",
	"fstsw",
	"fsubp",
	"fsubrp",
	"ftst",
	"fucom",
	"fucomi",
	"fucomip",
	"fucomp",
	"fucompp",
	"fxam",
	"fxch",

	"hlt",
	"Idiv",
	"Imul",
	"In",
	"Inc",
	"Ins",
	"int",
	"into",
	"invd",
	"invlpg",
	"iret",

	/* these must be in this order */
	"jo",
	"jno",
	"jc",
	"jnc",
	"jz",
	"jnz",
	"jbe",
	"ja",
	"js",
	"jns",
	"jp",
	"jnp",
	"jl",
	"jnl",
	"jle",
	"jg",
	
	"jcxz",
	"jmp",
	"lahf",
	"lar",
	"lds",
	"Lea",
	"leave",
	"les",
	"lfs",
	"lgdt",
	"lgs",
	"lidt",
	"lldt",
	"lmsw",
	"loadall",
	"loadall286",
	"lock",
	"Lods",
	"loop",
	"loopnz",
	"loopz",
	"lsl",
	"lss",
	"ltr",
	"Mov",
	"movd",
	"movq",
	"Movs",
	"movsx",
	"movzx",
	"Mul",
	"Neg",
	"nop",
	"Not",
	"Or",
	"Out",
	"Outs",
	"pcmpeqb",
	"pcmpeqw",
	"pcmpeqd",
	"Pop",
	"Popa",
	"Popf",
	
	/* these must be in this order */
	"punpcklbw",
	"punpcklwd",
	"punpckldq",
	"packsswb",
	"pcmpgtb",
	"pcmpgtw",
	"pcmpgtd",
	"packuswb",
	"punpckhbw",
	"punpckhwd",
	"punpckhdq",
	"packssdw",
	
	"psrlw",
	"psrld",
	"psrlq",
	"pmullw",
	"psubusb",
	"psubusw",
	"pand",
	"paddusb",
	"paddusw",
	"pandn",
	"psraw",
	"psrad",
	"pmulhw",
	"psubsb",
	"psubsw",
	"por",
	"paddsb",
	"paddsw",
	"pxor",
	"psllw",
	"pslld",
	"psllq",
	"pmaddwd",
	"psubb",
	"psubw",
	"psubd",
	"paddb",
	"paddw",
	"paddd",
	
	"Push",
	"Pusha",
	"Pushf",
	"Rcl",
	"Rcr",
	"rdtsc",
	"rdmsr",
	"rdpmc",
	"rep",
	"repnz",
	"retf",
	"retn",
	"Rol",
	"Ror",
	"rsm",
	"sahf",
	"Sar",
	"Sbb",
	"Scas",
	"seg",
	"setalc",

	/* these must be in this order */
	"seto",
	"setno",
	"setc",
	"setnc",
	"setz",
	"setnz",
	"setbe",
	"seta",
	"sets",
	"setns",
	"setp",
	"setnp",
	"setl",
	"setnl",
	"setle",
	"setg",

	"sgdt",
	"Shl",
	"Shld",
	"Shr",
	"Shrd",
	"sidt",
	"sldt",
	"smsw",
	"stc",
	"std",
	"sti",
	"stos",
	"str",
	"Sub",
	"sysenter",
	"sysexit",
	"Test",
	"umov",
	"verr",
	"verw",
	"wait",
	"xadd",
	"Xchg",
	"xlat",
	"Xor",
	"wbinvd",
	"wrmsr",

	"addps",
	"addss",
	"andnps",
	"andps",
	"cmpps",
	"cmpss",
	"comiss",
	"cvtpi2ps",
	"cvtps2pi",
	"cvtsi2ss",
	"cvtss2si",
	"cvttps2pi",
	"cvttss2si",
	"divps",
	"divss",
	"fxrstor",
	"fxsave",
	"ldmxcsr",
	"maskmovq",
	"maxps",
	"maxss",
	"minps",
	"minss",
	"movaps",
	"movhps",
	"movlps",
	"movmskps",
	"movntps",
	"movntq",
	"movss",
	"movups",
	"mulps",
	"mulss",
	"orps",
	"pavgb",
	"pavgw",
	"pextrw",
	"pinsrw",
	"pmaxsw",
	"pmaxub",
	"pminsw",
	"pminub",
	"pmovmskb",
	"pmulhuw",
	"prefetchnta",
	"prefetcht0",
	"prefetcht1",
	"prefetcht2",
	"psadbw",
	"pshufw",
	"rcpps",
	"rcpss",
	"rsqrtps",
	"rsqrtss",
	"sfence",
	"shufps",
	"sqrtps",
	"sqrtss",
	"stmxcsr",
	"subps",
	"subss",
	"ucomiss",
	"unpckhps",
	"unpcklps",
	"xorps",

	"invalid"
};
