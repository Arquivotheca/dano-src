#ifndef _X86DIS_TABLES_H_
#define _X86DIS_TABLES_H_

extern const operand_t no_operand;
extern const char *reg8[8];
extern const char *reg16[8];
extern const char *reg32[8];
extern const char *regmmx[8];
extern const char *regxmm[8];
extern const char *regseg[8];
extern const char *regfp[8];
extern const char *regc[8];
extern const char *regd[8];
extern const char *regt[8];

extern const instr0_desc_t instr0[];
extern const instr1_desc_t instr1[];
extern const instr2_desc_t instr2[];
extern const instr3_desc_t instr3[];
extern const opcode_extension_t opcode_extensions[];

extern const instr0_desc_t instr0_0f[];
extern const instr1_desc_t instr1_0f[];
extern const instr2_desc_t instr2_0f[];
extern const instr3_desc_t instr3_0f[];
extern const opcode_extension_t opcode_extensions_0f[];

extern const instr0_desc_t instr0_f30f[];
extern const instr1_desc_t instr1_f30f[];
extern const instr2_desc_t instr2_f30f[];
extern const instr3_desc_t instr3_f30f[];
extern const opcode_extension_t opcode_extensions_f30f[];

extern const no_arg_fp_instr_t no_arg_fp_instr[];
extern const two_args_fp_instr_t two_args_fp_instr[];

extern const char *intel_instrs[];

#endif
