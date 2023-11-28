/********************************************************************
* X86_decoder_decode.c:
*    this file includes the interface routine for the
*    decoding X86 instruction 
* 
*  the main routines: 
*  ================== 
*  1) X86_decoder_set_decoder - set decoder modes. 
*  2) X86_decoder_decode - will decode given buffer    
*     
*********************************************************************/

/*******************/
/***** include *****/
/*******************/
#include <stdio.h>
#include <string.h>

#include "x86_inc.h"
#include "x86_type.h"
#include "x86_dec.h"
#include "x86_info.h" 


/*********************/
/***** defines   *****/
/*********************/

#if DECODE_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

#define XOP_BITS(x)  ((char)(x & 0x38) >> 3)

#define GET_BYTE(code)  (((current_byte+=1) > MAX_INSTRUCTION_LENGTH) ?     \
                         Decode_error(X86CT_DECODE_TOO_LONG_OPCODE)   :     \
                         (current_byte > code_size)                   ?     \
                         Decode_error(X86CT_DECODE_TOO_SHORT_ERR)     :     \
                         code[current_byte-1])

#define GET_WORD(code)  (((current_byte+=2) > MAX_INSTRUCTION_LENGTH) ?     \
                         Decode_error(X86CT_DECODE_TOO_LONG_OPCODE)   :     \
                         (current_byte > code_size)                   ?     \
                         Decode_error(X86CT_DECODE_TOO_SHORT_ERR)     :     \
                         ((code[current_byte-1] << 8) +                     \
                           code[current_byte-2]))


#define GET_DWORD(code) (((current_byte+=4) > MAX_INSTRUCTION_LENGTH) ?       \
                         Decode_error(X86CT_DECODE_TOO_LONG_OPCODE)   :       \
                         (current_byte > code_size)                   ?       \
                         Decode_error(X86CT_DECODE_TOO_SHORT_ERR)     :       \
                         (unsigned long)(((long)code[current_byte-1] << 24) + \
                          ((long)code[current_byte-2] << 16) +                \
                          ((long)code[current_byte-3] <<  8) +                \
                           (long)code[current_byte-4]))

#define NEXT_BYTE(code)  (unsigned char)(((current_byte) >= MAX_INSTRUCTION_LENGTH) ?     \
                         Decode_error(X86CT_DECODE_TOO_LONG_OPCODE)   :     \
                         (current_byte >= code_size)                   ?     \
                         Decode_error(X86CT_DECODE_TOO_SHORT_ERR)     :     \
                         code[current_byte])
                
#define UNGET_BYTE()     current_byte--;

#define Decode_error(err)   (((decode_err == X86CT_DECODE_TOO_LONG_OPCODE) || \
                              (decode_err == X86CT_DECODE_TOO_SHORT_ERR))   ? \
                             0 :                                              \
                              ((d_info->inst_size = current_byte),            \
                               ((decode_err == X86CT_DECODE_NO_ERROR)       ? \
                                (decode_err = err) : 0), 0))

static  X86CT_Decode_errno  decode_err;

static  void    set_oper_modrm(X86CT_byte *code, int code_size,
                                X86CT_Decoder_info *d_info, int i, int size);

static  X86_Register reg8[8] ={ X86_AL,X86_CL,X86_DL,X86_BL,X86_AH,X86_CH,X86_DH,X86_BH };
static  X86_Register reg16[8]={ X86_AX,X86_CX,X86_DX,X86_BX,X86_SP,X86_BP,X86_SI,X86_DI };
static  X86_Register reg32[8]={ X86_EAX,X86_ECX,X86_EDX,X86_EBX,X86_ESP,X86_EBP,X86_ESI,X86_EDI };
static  X86_Register regmm[8]={ X86_MM0,X86_MM1,X86_MM2,X86_MM3,X86_MM4,X86_MM5,X86_MM6,X86_MM7 };
static  X86_Register dreg[8]={ X86_DR0,X86_DR1,X86_DR2,X86_DR3,X86_DR4,X86_DR5,X86_DR6,X86_DR7};
static  X86_Register creg[8]={ X86_CR0,X86_CR1,X86_CR2,X86_CR3,X86_CR4,NO_REG,NO_REG,NO_REG};
static  X86_Register sreg3[8]={ X86_ES,X86_CS,X86_SS,X86_DS,X86_FS,X86_GS,NO_REG,NO_REG};
static  X86_Register reg32_index[8]= { X86_EAX,X86_ECX,X86_EDX,X86_EBX,NO_REG,X86_EBP,X86_ESI,X86_EDI };
static  X86_Stack    st[8]={ X86_ST,X86_ST1,X86_ST2,X86_ST3,X86_ST4,X86_ST5,X86_ST6,X86_ST7 };
/***
  ModRM32 - structure that includes the 32 bit version of ModRM.
  index to the table is 5 bit long. The 2 MSB bits are Mod and 3 bits are
  RM. (Without SIB).
****/
static X86CT_Oper_info ModRM32[] = {
/*mod r/m     type        val  seg  off  base  index    scale  **/
/*00 000*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_EAX,   NO_REG,  1,0,0},
/*00 001*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_ECX,   NO_REG,  1,0,0},
/*00 010*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_EDX,   NO_REG,  1,0,0},
/*00 011*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_EBX,   NO_REG,  1,0,0},
/*00 100*/{X86CT_NO_OPER,        0,  NO_REG,  0,   NO_REG,    NO_REG,  1,0,0},
/*00 101*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   NO_REG,    NO_REG,  1,0,0},
/*00 110*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_ESI,   NO_REG,  1,0,0},
/*00 111*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_EDI,   NO_REG,  1,0,0},

/*mod r/m     type        val  seg  off  base  index    scale  **/
/*01 000*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_EAX,   NO_REG,  1,0,0},
/*01 001*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_ECX,   NO_REG,  1,0,0},
/*01 010*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_EDX,   NO_REG,  1,0,0},
/*01 011*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_EBX,   NO_REG,  1,0,0},
/*01 100*/{X86CT_NO_OPER,        0,  NO_REG,  0,   NO_REG,    NO_REG,  1,0,0},
/*01 101*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS,  8,   X86_EBP,   NO_REG,  1,0,0},
/*01 110*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_ESI,   NO_REG,  1,0,0},
/*01 111*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_EDI,   NO_REG,  1,0,0},

/*mod r/m     type        val  seg  off  base  index    scale  **/
/*10 000*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_EAX,   NO_REG,  1,0,0},
/*10 001*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_ECX,   NO_REG,  1,0,0},
/*10 010*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_EDX,   NO_REG,  1,0,0},
/*10 011*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_EBX,   NO_REG,  1,0,0},
/*10 100*/{X86CT_NO_OPER,        0,  NO_REG, 0,    NO_REG,    NO_REG,  1,0,0},
/*10 101*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS, 32,   X86_EBP,   NO_REG,  1,0,0},
/*10 110*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_ESI,   NO_REG,  1,0,0},
/*10 111*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_EDI,   NO_REG,  1,0,0},

/*mod r/m     type        val  seg  off  base  index    scale  **/
/*11 000*/{X86CT_OPER_TYPE_REG,  0,  NO_REG, 0,    NO_REG,   NO_REG,   0,0,0},
/*11 001*/{X86CT_OPER_TYPE_REG,  0,  NO_REG, 0,    NO_REG,   NO_REG,   0,0,0},
/*11 010*/{X86CT_OPER_TYPE_REG,  0,  NO_REG, 0,    NO_REG,   NO_REG,   0,0,0},
/*11 011*/{X86CT_OPER_TYPE_REG,  0,  NO_REG, 0,    NO_REG,   NO_REG,   0,0,0},
/*11 100*/{X86CT_OPER_TYPE_REG,  0,  NO_REG, 0,    NO_REG,   NO_REG,   0,0,0},
/*11 101*/{X86CT_OPER_TYPE_REG,  0,  NO_REG, 0,    NO_REG,   NO_REG,   0,0,0},
/*11 110*/{X86CT_OPER_TYPE_REG,  0,  NO_REG, 0,    NO_REG,   NO_REG,   0,0,0},
/*11 111*/{X86CT_OPER_TYPE_REG,  0,  NO_REG, 0,    NO_REG,   NO_REG,   0,0,0} 
};

/***
  SIB - Scale Index Base
  The index is 2 bits of mod and 3 bits from base.
  (The scale and index will be taken manualy.
***/
static X86CT_Oper_info SIB[] = {
/*mod base     type        val  seg  off  base  index    scale  **/
/*00 000*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_EAX,   NO_REG,  0,0,0},
/*00 001*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_ECX,   NO_REG,  0,0,0},
/*00 010*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_EDX,   NO_REG,  0,0,0},
/*00 011*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_EBX,   NO_REG,  0,0,0},
/*00 100*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS,  0,   X86_ESP,   NO_REG,  0,0,0},
/*00 101*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   NO_REG,    NO_REG,  0,0,0},
/*00 110*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_ESI,   NO_REG,  0,0,0},
/*00 111*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_EDI,   NO_REG,  0,0,0},

/*mod base    type        val  seg  off  base  index    scale  **/
/*01 000*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_EAX,   NO_REG,  0,0,0},
/*01 001*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_ECX,   NO_REG,  0,0,0},
/*01 010*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_EDX,   NO_REG,  0,0,0},
/*01 011*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_EBX,   NO_REG,  0,0,0},
/*01 100*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS,  8,   X86_ESP,   NO_REG,  0,0,0},
/*01 101*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS,  8,   X86_EBP,   NO_REG,  0,0,0},
/*01 110*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_ESI,   NO_REG,  0,0,0},
/*01 111*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_EDI,   NO_REG,  0,0,0},

/*mod base    type        val  seg  off  base  index    scale  **/
/*10 000*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_EAX,   NO_REG,  0,0,0},
/*10 001*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_ECX,   NO_REG,  0,0,0},
/*10 010*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_EDX,   NO_REG,  0,0,0},
/*10 011*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_EBX,   NO_REG,  0,0,0},
/*10 100*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS, 32,   X86_ESP,   NO_REG,  0,0,0},
/*10 101*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS, 32,   X86_EBP,   NO_REG,  0,0,0},
/*10 110*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_ESI,   NO_REG,  0,0,0},
/*10 111*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 32,   X86_EDI,   NO_REG,  0,0,0} 
};
/***
  ModRM16 - structure that includes the 16 bit version of ModRM.
  index to the table is 5 bit long. The 2 MSB bits are Mod and 3 bits are
  RM.
****/
static X86CT_Oper_info ModRM16[] = {
/*mod r/m     type               val  seg  off  base  index   scale sz flgs **/
/*00 000*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_BX,    X86_SI, 1,0,0},
/*00 001*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_BX,    X86_DI, 1,0,0},
/*00 010*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS,  0,   X86_BP,    X86_SI, 1,0,0},
/*00 011*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS,  0,   X86_BP,    X86_DI, 1,0,0},
/*00 100*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_SI,    NO_REG, 1,0,0},
/*00 101*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_DI,    NO_REG, 1,0,0},
/*00 110*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 16,   NO_REG,    NO_REG, 1,0,0},
/*00 111*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  0,   X86_BX,    NO_REG, 1,0,0},

/*mod r/m     type               val  seg  off  base  index   scale sz flgs **/
/*01 000*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_BX,    X86_SI, 1,0,0},
/*01 001*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_BX,    X86_DI, 1,0,0},
/*01 010*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS,  8,   X86_BP,    X86_SI, 1,0,0},
/*01 011*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS,  8,   X86_BP,    X86_DI, 1,0,0},
/*01 100*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_SI,    NO_REG, 1,0,0},
/*01 101*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_DI,    NO_REG, 1,0,0},
/*01 110*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS,  8,   X86_BP,    NO_REG, 1,0,0},
/*01 111*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS,  8,   X86_BX,    NO_REG, 1,0,0},

/*mod r/m     type               val  seg  off  base  index   scale sz flgs **/
/*10 000*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 16,   X86_BX,    X86_SI, 1,0,0},
/*10 001*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 16,   X86_BX,    X86_DI, 1,0,0},
/*10 010*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS, 16,   X86_BP,    X86_SI, 1,0,0},
/*10 011*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS, 16,   X86_BP,    X86_DI, 1,0,0},
/*10 100*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 16,   X86_SI,    NO_REG, 1,0,0},
/*10 101*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 16,   X86_DI,    NO_REG, 1,0,0},
/*10 110*/{X86CT_OPER_TYPE_MEM,  0,  X86_SS, 16,   X86_BP,    NO_REG, 1,0,0},
/*10 111*/{X86CT_OPER_TYPE_MEM,  0,  X86_DS, 16,   X86_BX,    NO_REG, 1,0,0},

/*mod r/m     type               val  seg  off  base  index   scale sz flgs **/
/*11 000*/{X86CT_OPER_TYPE_REG,  0,NO_REG, 0,NO_REG,   NO_REG,   0  ,0,0},
/*11 001*/{X86CT_OPER_TYPE_REG,  0,NO_REG, 0,NO_REG,   NO_REG,   0  ,0,0},
/*11 010*/{X86CT_OPER_TYPE_REG,  0,NO_REG, 0,NO_REG,   NO_REG,   0  ,0,0},
/*11 011*/{X86CT_OPER_TYPE_REG,  0,NO_REG, 0,NO_REG,   NO_REG,   0  ,0,0},
/*11 100*/{X86CT_OPER_TYPE_REG,  0,NO_REG, 0,NO_REG,   NO_REG,   0  ,0,0},
/*11 101*/{X86CT_OPER_TYPE_REG,  0,NO_REG, 0,NO_REG,   NO_REG,   0  ,0,0},
/*11 110*/{X86CT_OPER_TYPE_REG,  0,NO_REG, 0,NO_REG,   NO_REG,   0  ,0,0},
/*11 111*/{X86CT_OPER_TYPE_REG,  0,NO_REG, 0,NO_REG,   NO_REG,   0  ,0,0} 
};

#define MODRM_REG_START 3
#define MODRM_REG_LENGTH 3
#define MODRM_REG_MASK  0x7

#define MODRM_MOD_START 6
#define MODRM_MOD_LENGTH 2
#define MODRM_MOD_MASK  0x3

#define MODRM_RM_START  0
#define MODRM_RM_LENGTH 3
#define MODRM_RM_MASK   0x7

#define SIB_S_START    6
#define SIB_S_LENGTH   2
#define SIB_S_MASK    0x3

#define SIB_I_START    3
#define SIB_I_LENGTH   3
#define SIB_I_MASK    0x7

#define SIB_B_START    0
#define SIB_B_LENGTH   3
#define SIB_B_MASK    0x7

#define REG_MOD  3


#define GET_MODRM_REG(modrm) ((modrm >>MODRM_REG_START ) & MODRM_REG_MASK)
#define GET_MODRM_MOD(modrm) ((modrm >>MODRM_MOD_START ) & MODRM_MOD_MASK)
#define GET_MODRM_RM(modrm) ((modrm >>MODRM_RM_START ) & MODRM_RM_MASK)

#define GET_SIB_S(sib)  ((sib>>SIB_S_START) & SIB_S_MASK)
#define GET_SIB_I(sib)  ((sib>>SIB_I_START) & SIB_I_MASK)
#define GET_SIB_B(sib)  ((sib>>SIB_B_START) & SIB_B_MASK)

#define SET_OPER_IMM8(i)    { opers[i].type=X86CT_OPER_TYPE_IMM; \
                              opers[i].size=X86CT_OPER_SIZE_BYTE; \
                            opers[i].value=SIGN_EXT((X86CT_byte)GET_BYTE(code),8);}

#define SET_OPER_CONST(i,val) { opers[i].type=X86CT_OPER_TYPE_IMM; \
                                opers[i].size=X86CT_OPER_SIZE_BYTE; \
                                opers[i].flags=X86CT_OPER_FLAG_IMPLIED;\
                                opers[i].value=val;}

#define SET_OPER_IMM8S(i) { opers[i].type=X86CT_OPER_TYPE_IMM; \
                            opers[i].size=X86CT_OPER_SIZE_BYTE; \
                            opers[i].flags=X86CT_OPER_FLAG_SIGN_EXTENDED; \
                            opers[i].value=SIGN_EXT((X86CT_byte)GET_BYTE(code),8);}

#define SET_OPER_IMM16(i) { opers[i].type=X86CT_OPER_TYPE_IMM; \
                            opers[i].size=X86CT_OPER_SIZE_WORD; \
                            opers[i].value=SIGN_EXT(GET_WORD(code),16);}

#define SET_OPER_IMM16U(i) { opers[i].type=X86CT_OPER_TYPE_IMM; \
                            opers[i].size=X86CT_OPER_SIZE_WORD; \
                            opers[i].value=GET_WORD(code);}

#define SET_OPER_IMM32(i) { opers[i].type=X86CT_OPER_TYPE_IMM; \
                            opers[i].size=X86CT_OPER_SIZE_LONG; \
                            opers[i].value=GET_DWORD(code);}


#define SET_OPER_PORT(i)    { opers[i].type=X86CT_OPER_TYPE_PORT; \
                              opers[i].size=X86CT_OPER_SIZE_BYTE; \
                              opers[i].value=(X86CT_byte)GET_BYTE(code);}

#define SET_OPER_REL8(i)    { opers[i].type=X86CT_OPER_TYPE_REL; \
                              opers[i].size=X86CT_OPER_SIZE_BYTE; \
                            opers[i].value=SIGN_EXT((X86CT_byte)GET_BYTE(code),8);}


#define SET_OPER_REL16(i) { opers[i].type=X86CT_OPER_TYPE_REL; \
                            opers[i].size=X86CT_OPER_SIZE_WORD; \
                            opers[i].value=SIGN_EXT(GET_WORD(code),16);}

#define SET_OPER_REL32(i) { opers[i].type=X86CT_OPER_TYPE_REL; \
                            opers[i].size=X86CT_OPER_SIZE_LONG; \
                            opers[i].value=GET_DWORD(code);}


#define SET_OPER_SEG_OFF16(i) { opers[i].type=X86CT_OPER_TYPE_SEG_OFF; \
                            opers[i].size=X86CT_OPER_SIZE_WORD; \
                            opers[i].mem_offset=GET_WORD(code);\
                            opers[i].value=GET_WORD(code);}

#define SET_OPER_SEG_OFF32(i) { opers[i].type=X86CT_OPER_TYPE_SEG_OFF; \
                            opers[i].size=X86CT_OPER_SIZE_LONG; \
                            opers[i].mem_offset=GET_DWORD(code);\
                            opers[i].value=GET_WORD(code);}

#define SET_OP1_AS_MODRM(Sz)								  \
{															  \
   d_info->mrm_info.mrm_opr_size = (Sz);                      \
   d_info->mrm_info.mrm_type = X86CT_MODRM_OP1;				  \
   d_info->mrm_info.modrm = code[current_byte-1]; 			  \
}

#define SET_OP2_AS_MODRM(Sz)								  \
{															  \
   d_info->mrm_info.mrm_opr_size = (Sz);                      \
   d_info->mrm_info.mrm_type = X86CT_MODRM_OP2;				  \
   d_info->mrm_info.modrm = code[current_byte-1]; 			  \
}
	

#define SET_OPER_MODRM_REG8(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_BYTE; \
   opers[index].value=reg8[GET_MODRM_REG(NEXT_BYTE(code))];\
   d_info->mrm_info.mrm_opr_size = X86CT_OPER_SIZE_BYTE;      \
   d_info->mrm_info.mrm_type = X86CT_MODRM_REG;				  \
   d_info->mrm_info.modrm = NEXT_BYTE(code); 			  \
}

#define SET_OPER_MODRM_REG16(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].value=reg16[GET_MODRM_REG(NEXT_BYTE(code))];\
   d_info->mrm_info.mrm_opr_size = X86CT_OPER_SIZE_WORD;      \
   d_info->mrm_info.mrm_type = X86CT_MODRM_REG;				  \
   d_info->mrm_info.modrm = NEXT_BYTE(code); 			  \
}

#define SET_OPER_MODRM_REG32(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_LONG; \
   opers[index].value=reg32[GET_MODRM_REG(NEXT_BYTE(code))];\
   d_info->mrm_info.mrm_opr_size = X86CT_OPER_SIZE_LONG;      \
   d_info->mrm_info.mrm_type = X86CT_MODRM_REG;				  \
   d_info->mrm_info.modrm = NEXT_BYTE(code); 			  \
}

#define SET_OPER_MODRM_ST(index) 											\
{  opers[index].type=X86CT_OPER_TYPE_ST;									\
   opers[index].size=X86CT_OPER_SIZE_EXT; 									\
   opers[index].value=st[GET_MODRM_RM(code[current_byte-1])];               \
   d_info->mrm_info.mrm_opr_size = X86CT_OPER_SIZE_EXT;                     \
   d_info->mrm_info.mrm_type = X86CT_MODRM_OP2;					        		\
   d_info->mrm_info.modrm = code[current_byte-1];			    			\
}

#define SET_OPER_MODRM_SREG(index)                                          \
{                                                                           \
    opers[index].type=X86CT_OPER_TYPE_REG;                                  \
    opers[index].size=X86CT_OPER_SIZE_LONG;                                 \
    opers[index].value=sreg3[GET_MODRM_REG(NEXT_BYTE(code))];            \
    if (opers[index].value == NO_REG)                                       \
    {                                                                       \
        Decode_error(X86CT_DECODE_OPERAND_ERR);                             \
    }                                                                       \
   d_info->mrm_info.mrm_opr_size = X86CT_OPER_SIZE_LONG;       \
   d_info->mrm_info.mrm_type = X86CT_MODRM_REG;				   \
   d_info->mrm_info.modrm = NEXT_BYTE(code);                \
}

#define SET_OPER_OFF_32(index) \
{  opers[index].type=X86CT_OPER_TYPE_MEM;\
   opers[index].mem_offset=GET_DWORD(code);\
   opers[index].mem_seg=(d_info->seg==NO_REG)?X86_DS:d_info->seg;\
   d_info->memory=1; \
   opers[index].dis_size = X86CT_OPER_SIZE_LONG;\
   opers[index].mem_base=opers[index].mem_index=NO_REG; }

#define SET_OPER_OFF_16(index) \
{  opers[index].type=X86CT_OPER_TYPE_MEM;\
   opers[index].mem_offset=GET_WORD(code);\
   opers[index].mem_seg=(d_info->seg==NO_REG)?X86_DS:d_info->seg;\
   d_info->memory=1; \
   opers[index].dis_size = X86CT_OPER_SIZE_WORD;\
   opers[index].mem_base=opers[index].mem_index=NO_REG; }

#define SET_OPER_CL(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_BYTE; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_CL;}

#define SET_OPER_ST(index) \
{  opers[index].type=X86CT_OPER_TYPE_ST;\
   opers[index].size=X86CT_OPER_SIZE_EXT; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_ST;}

#define SET_OPER_AL(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_BYTE; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_AL;}

#define SET_OPER_AX(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_AX;}

#define SET_OPER_DX_PORT(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED|X86CT_OPER_FLAG_PORT_IN_DX; \
   opers[index].value=X86_DX;}

#define SET_OPER_DS(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_DS;}

#define SET_OPER_ES(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_ES;}

#define SET_OPER_SS(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_SS;}

#define SET_OPER_FS(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_FS;}

#define SET_OPER_GS(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_GS;}

#define SET_OPER_CS(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_CS;}

#define SET_OPER_EAX(index) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].flags=X86CT_OPER_FLAG_IMPLIED;\
   opers[index].value=X86_EAX;}

#define SET_OPER_R8(index,r) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_BYTE; \
   opers[index].value=reg8[r];}

#define SET_OPER_R16(index,r) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_WORD; \
   opers[index].value=reg16[r];}


#define SET_OPER_MODRM_MM(index)                               \
{  opers[index].type=X86CT_OPER_TYPE_MM;                       \
   opers[index].size=X86CT_OPER_SIZE_QUAD;                     \
   opers[index].value=regmm[GET_MODRM_REG(NEXT_BYTE(code))];\
   d_info->mrm_info.mrm_opr_size = X86CT_OPER_SIZE_QUAD;       \
   d_info->mrm_info.mrm_type = X86CT_MODRM_REG;				   \
   d_info->mrm_info.modrm = NEXT_BYTE(code);                \
}


#define SET_OPER_R32(index,r) \
{  opers[index].type=X86CT_OPER_TYPE_REG;\
   opers[index].size=X86CT_OPER_SIZE_LONG; \
   opers[index].value=reg32[r];}

#define SET_OPER_MODRM_DR(index)                               \
{  opers[index].type=X86CT_OPER_TYPE_REG;                      \
   opers[index].size=X86CT_OPER_SIZE_LONG;                     \
   opers[index].value=dreg[GET_MODRM_REG(NEXT_BYTE(code))]; \
   d_info->mrm_info.mrm_opr_size = X86CT_OPER_SIZE_LONG;       \
   d_info->mrm_info.mrm_type = X86CT_MODRM_REG;				   \
   d_info->mrm_info.modrm = NEXT_BYTE(code);                \
}
   

#define SET_OPER_MODRM_CR(index) \
{                                                                           \
    opers[index].type=X86CT_OPER_TYPE_REG;                                  \
    opers[index].size=X86CT_OPER_SIZE_LONG;                                 \
    opers[index].value=creg[GET_MODRM_REG(NEXT_BYTE(code))];             \
    if (opers[index].value == NO_REG || opers[index].value == X86_CR1)      \
    {                                                                       \
        Decode_error(X86CT_DECODE_OPERAND_ERR);                             \
    }                                                                       \
   d_info->mrm_info.mrm_opr_size = X86CT_OPER_SIZE_LONG;       \
   d_info->mrm_info.mrm_type = X86CT_MODRM_REG;				   \
   d_info->mrm_info.modrm = NEXT_BYTE(code);                \
}

#define GET_OPCODE_REG(code) ((X86CT_byte)GET_BYTE(code) & 0x7)

#define GET_MODRM_INDEX(modrm) ((GET_MODRM_MOD(modrm)<<MODRM_RM_LENGTH) + \
                                GET_MODRM_RM(modrm))
  
#define GET_SIB_INDEX(modrm,sib) ((GET_MODRM_MOD(modrm)<<SIB_B_LENGTH) + \
                                  GET_SIB_B(sib))

#define SET_OPER_MODRM(i,size)  set_oper_modrm(code,code_size,d_info,i,size)
#define SET_OPER_MODRM8(i)   SET_OPER_MODRM(i,X86CT_OPER_SIZE_BYTE)
#define SET_OPER_MODRM16(i)  SET_OPER_MODRM(i,X86CT_OPER_SIZE_WORD)
#define SET_OPER_MODRM32(i)  SET_OPER_MODRM(i,X86CT_OPER_SIZE_LONG)
#define SET_OPER_MODRM64(i)  SET_OPER_MODRM(i,X86CT_OPER_SIZE_QUAD)
#define SET_OPER_MODRM80(i)  SET_OPER_MODRM(i,X86CT_OPER_SIZE_EXT)
#define SET_OPER_MODRM128(i)  SET_OPER_MODRM(i,X86CT_OPER_SIZE_DQUAD)

/*********************/
/***** variables *****/
/*********************/
static unsigned long Decode_machine=X86CT_FLAG_P5;
static int Decode_size=32;
static int current_byte=0;
static X86CT_Oper_info *Default_ModRM=ModRM32;
static X86CT_Oper_info *ModRM=ModRM32;

/************************************************************************
* Decode_errno X86_decoder_set_decoder(unsigned long  machine,int size)
*                                                                       
* The routine is setting the current machine and operand size for the decoder
* 
* input: 
* ======
* 1) machine - can be one of : X86CT_FLAG_P5, X86CT_FLAG_P6, X86CT_FLAG_P7,
*                              X86CT_FLAG_P5MM, X86CT_FLAG_P6MM
*           OR'd with one of : X86CT_FLAG_8086, X86CT_FLAG_V86 
*
* 2) size - the current operand size (16 or 32)
*
* output:
* =======
* Might return DECODE_SET_ERROR for invalid arguments ..
* 
************************************************************************/
X86CT_Decode_errno X86_decoder_set_decoder(unsigned long  machine,
                                    X86CT_Active_oper_size size)
{
    int     n_machines = 0;

    /*** check for valid setting ***/

    /*** SIZE can be 16 or 32 only ***/
    if (size!=X86CT_ACTIVE_OPER_SIZE_WORD && size!=X86CT_ACTIVE_OPER_SIZE_LONG)
    {
        return(X86CT_DECODE_SET_ERR);
    }
    
    /*** machine type should be one and only one of the list above ***/
    n_machines += (machine & X86CT_FLAG_P5) ? 1 : 0;
    n_machines += (machine & X86CT_FLAG_P6) ? 1 : 0;
    n_machines += (machine & X86CT_FLAG_P7) ? 1 : 0;
    n_machines += (machine & X86CT_FLAG_P5MM) ? 1 : 0;  
    n_machines += (machine & X86CT_FLAG_P6MM) ? 1 : 0;  
    
    if (n_machines != 1)
    {
        return(X86CT_DECODE_SET_ERR);
    }

    /*** mode may be none or one of the above ***/
    if ((machine & (X86CT_FLAG_8086 | X86CT_FLAG_V86)) ==
                    (X86CT_FLAG_8086 | X86CT_FLAG_V86))
    {
        return(X86CT_DECODE_SET_ERR);
    }

    /*** If machine is V86, size must be 16 ***/ 
    if ((machine & X86CT_FLAG_V86) && (size != X86CT_ACTIVE_OPER_SIZE_WORD))
    {
        return(X86CT_DECODE_SET_ERR);
    }

    /*** here it's OK ***/ 
    Decode_size=size;
    Decode_machine=machine;
    /*** take current ModRM ***/
    if (Decode_size==X86CT_ACTIVE_OPER_SIZE_LONG)
    {
        Default_ModRM=ModRM32;
    }
    else
    {
        Default_ModRM=ModRM16;
    }

    return(X86CT_DECODE_NO_ERROR);
}

/************************************************************************
* Decode_errno X86_decoder_decode(X86CT_byte *code,int code_size,
*                                  X86CT_Decoder_info *d_info)
*                                                                       
* The decoder main function. decode one instrucion per call from the
* given buffer, The instruction information is written into a given
* structcure.
* 
* input: 
* ======
* 1) code - pointer to code buffer 
* 2) code_size - buffer size (might be less then instruction size)
* 3) d_info - pointer to decoder_info structure, to be filled by routine
*
* output:
* =======
* Information in the given structure, or one of the errors in
* the Decode_errno enum.
* 
************************************************************************/
X86CT_Decode_errno X86_decoder_decode(X86CT_byte *code,int code_size,
                               X86CT_Decoder_info  *d_info)
{
    /***** variables *****/

    X86CT_Opcode_Tbl *op2_tbl,*xop_tbl;
    int op2_index,op1_index,xop_index,size_bit,prefix;
    X86_decoder_X86CT_INFO *instruction_info;
    X86CT_byte prefix_byte;
    X86CT_Oper_info *opers;
	int vx_scalar_form = 0;
	
    /*******************/
    /***** process *****/
    /*******************/
    decode_err = X86CT_DECODE_NO_ERROR;
    current_byte=0;
    /*** init the structure ***/ 
    d_info->rep=NO_PREFIX;
    d_info->lock=NO_PREFIX;
    d_info->seg=NO_REG;
    d_info->operand_size=Decode_size;
    d_info->address_size=Decode_size;
    d_info->memory=0;
    d_info->mrm_info.mrm_type = X86CT_MODRM_NONE;
	d_info->mrm_info.sib = 0;
    memset(&d_info->prefix_info,0,sizeof(X86CT_Prefix_info));
	
    /*** set ModRM to ModRM16 or ModRM32 ***/
    ModRM=Default_ModRM;
    
    /*** get prefixes info into structure ***/
    prefix=TRUE;
    while (prefix)
    {
        prefix_byte=(X86CT_byte)GET_BYTE(code); 
        switch(prefix_byte)
        {
          case PREFIX_REP: /*F3*/
			if (d_info->rep == NO_PREFIX)
            {
                d_info->rep=prefix_byte;
                d_info->prefix_info.repeat_type=prefix_byte;
            }
            d_info->prefix_info.n_rep_pref++;
            break;
          case PREFIX_REPNE:
			if (d_info->rep == NO_PREFIX)
            {
                d_info->rep=prefix_byte;
                d_info->prefix_info.repeat_type=prefix_byte;
            }
            d_info->prefix_info.n_rep_pref++;
            break;
          case PREFIX_LOCK:
            d_info->lock=prefix_byte;
            d_info->prefix_info.n_lock_pref++;
            break;
          case PREFIX_SEG_CS:
            d_info->seg=X86_CS;
            d_info->prefix_info.n_seg_pref++;
            break;
          case PREFIX_SEG_SS:
            d_info->seg=X86_SS;
            d_info->prefix_info.n_seg_pref++;
            break;
          case PREFIX_SEG_DS:
            d_info->seg=X86_DS;
            d_info->prefix_info.n_seg_pref++;
            break;
          case PREFIX_SEG_ES:
            d_info->seg=X86_ES;
            d_info->prefix_info.n_seg_pref++;
            break;
          case PREFIX_SEG_FS:
            d_info->seg=X86_FS;
            d_info->prefix_info.n_seg_pref++;
            break;
          case PREFIX_SEG_GS:
            d_info->seg=X86_GS;
            d_info->prefix_info.n_seg_pref++;
            break;
          case PREFIX_OPERAND_SIZE:
			/*** flip size ***/
            if (Decode_size==X86CT_ACTIVE_OPER_SIZE_WORD)
            {
                d_info->operand_size=X86CT_ACTIVE_OPER_SIZE_LONG;
            }
            else
            {
                d_info->operand_size=X86CT_ACTIVE_OPER_SIZE_WORD;
            }
			d_info->prefix_info.n_oper_size_pref++;
           break;
          case PREFIX_ADDRESS_SIZE:
             /*** flip size ***/
            if (Decode_size==X86CT_ACTIVE_ADDR_SIZE_WORD)
            {
                d_info->address_size=X86CT_ACTIVE_ADDR_SIZE_LONG;
                ModRM=ModRM32;
            }
            else
            {
                d_info->address_size=X86CT_ACTIVE_ADDR_SIZE_WORD;
                ModRM=ModRM16;
            }
            d_info->prefix_info.n_addr_size_pref++;
            break;
          default:
            prefix=FALSE;
            break;
        }
    }
    d_info->prefix_info.n_prefixes =
            (unsigned char)(d_info->prefix_info.n_rep_pref +
            d_info->prefix_info.n_lock_pref +
            d_info->prefix_info.n_seg_pref +
            d_info->prefix_info.n_oper_size_pref +
            d_info->prefix_info.n_addr_size_pref);
	

    /*** return one byte back ***/
    UNGET_BYTE();

    /*** Begin search in tables ***/

    /*** get size bit ***/
    size_bit=(d_info->operand_size==X86CT_ACTIVE_OPER_SIZE_WORD)?0:1;

    /*** set index to op1 table ***/ 
    op1_index=(X86CT_byte)GET_BYTE(code)*2 + size_bit;
    d_info->opcode_1byte = (op1_index - size_bit) / 2;
    d_info->opcode_2byte = 0;   /*** init to 0 for the sake of fwait ***/
    d_info->opcode_type=X86CT_OPOCDE_TYPE_OP1;
    switch(op1[op1_index].type)
    {
      case X86CT_NO_TYPE:
        Decode_error(X86CT_DECODE_INVALID_OPCODE);
        break;
      case X86CT_TYPE_OP2:
        /*** Get ready for OP2 table ***/
        op2_index=(X86CT_byte)GET_BYTE(code)*2 + size_bit;
        d_info->opcode_2byte = (op2_index - size_bit) / 2;      
        op2_tbl=op1[op1_index].table;
        /*** Get OP2 info ***/
        switch(op2_tbl[op2_index].type)
        {
          case X86CT_NO_TYPE:
            Decode_error(X86CT_DECODE_INVALID_OPCODE);
            break;
            
          case X86CT_TYPE_XOP:
            /*** get ready for XOP table ***/
            xop_index=XOP_BITS((X86CT_byte)GET_BYTE(code))*2 +size_bit;
            xop_tbl=op2_tbl[op2_index].table;
			UNGET_BYTE();
			d_info->mrm_info.mrm_type = X86CT_MODRM_XOP;
			d_info->opcode_type=X86CT_OPOCDE_TYPE_OP1_OP2_XOP;
            /*** Get XOP info ***/
            if (xop_tbl[xop_index].type == X86CT_TYPE_INFO)
			{
				/*** Found !! ***/
				instruction_info=xop_tbl[xop_index].table;
				break;
			}
			Decode_error(X86CT_DECODE_INVALID_OPCODE);
            break;
          case X86CT_TYPE_INFO_XOP:
            /*** Found !!, but skipped one byte  ***/
            UNGET_BYTE();
            d_info->opcode_type=X86CT_OPOCDE_TYPE_OP1_XOP;
            instruction_info=op2_tbl[op2_index].table;
			d_info->mrm_info.mrm_type = X86CT_MODRM_XOP;
            break;
          case X86CT_TYPE_INFO:
            /*** Found !! ***/
            instruction_info=op2_tbl[op2_index].table;
            d_info->opcode_type=X86CT_OPOCDE_TYPE_OP1_OP2;
            break;
		  case X86CT_TYPE_OP1:
		  case X86CT_TYPE_OP2:
			break;
		  case X86CT_TYPE_PREFIX:
			Decode_error(X86CT_DECODE_INVALID_OPCODE);
			break;
		  }
        break;
        
      case X86CT_TYPE_XOP:
        /*** Get ready for XOP ***/
        xop_index=XOP_BITS((X86CT_byte)GET_BYTE(code))*2 +size_bit;
        xop_tbl=op1[op1_index].table;
        /*** Get XOP info ***/
        if (xop_tbl[xop_index].type != X86CT_TYPE_INFO)
          Decode_error(X86CT_DECODE_INVALID_OPCODE);
        /*** Found !! ***/
        instruction_info=xop_tbl[xop_index].table;
        d_info->opcode_type=X86CT_OPOCDE_TYPE_OP1_XOP;
        UNGET_BYTE();
		d_info->mrm_info.mrm_type = X86CT_MODRM_XOP;
        break;
        
      case X86CT_TYPE_INFO:
        /*** Found !! ***/
        instruction_info=op1[op1_index].table;
        d_info->opcode_type=X86CT_OPOCDE_TYPE_OP1;
        break;

	  case X86CT_TYPE_OP1:
	  case X86CT_TYPE_INFO_XOP:
	  case X86CT_TYPE_PREFIX:
		break;
    }

    if (decode_err != X86CT_DECODE_NO_ERROR)
    {
        return(decode_err);
    }

    /*** get table info ***/
    d_info->info=instruction_info;

    opers=&d_info->operands[0];

    /*** set default types ****/
    opers[X86CT_OPER_SRC1].type=X86CT_NO_OPER;
    opers[X86CT_OPER_SRC2].type=X86CT_NO_OPER;
    opers[X86CT_OPER_DEST].type=X86CT_NO_OPER;

    opers[X86CT_OPER_SRC1].flags=X86CT_OPER_FLAG_NONE;
    opers[X86CT_OPER_SRC2].flags=X86CT_OPER_FLAG_NONE;
    opers[X86CT_OPER_DEST].flags=X86CT_OPER_FLAG_NONE;

    
    /*** read operands (if any) ***/
    /**
        NOTE: The order of the SET_OPER_XXXX macros is important
              because of the decoding order in the X86. Each macro
              might advance the pointer to code buffer, after it
              took what it needs.

              SET_OPER_MODRM_REG8/16/31 comes before GET_OPER_MODRM
              SET_OPER_MODRM  comes before SET_OPER_IMM8/16/32
              SET_OPER_AL/X86_AX/X86_EAX has no order
    **/              
    
    switch(instruction_info->encode)
    {
      case NONE:  /*** opcode only ***/
        break;
      case IMAB: /*** Immidiate Byte to AL ***/
        SET_OPER_IMM8(X86CT_OPER_SRC1);
        SET_OPER_AL(X86CT_OPER_DEST);
        break;
      case IMAW: /*** Immediate Word to X86_AX ***/
        SET_OPER_IMM16(X86CT_OPER_SRC1);
        SET_OPER_AX(X86CT_OPER_DEST);
        break;
      case IMAL: /*** Immediate Long to X86_EAX ***/
        SET_OPER_IMM32(X86CT_OPER_SRC1);
        SET_OPER_EAX(X86CT_OPER_DEST);
        break;
      case IMB: /*** Immediate Byte to RM ***/
        SET_OPER_MODRM(X86CT_OPER_DEST,instruction_info->opr_sz);
        SET_OPER_IMM8(X86CT_OPER_SRC1);
        break;
      case IMW: /*** Immediate Word to RM ***/
        SET_OPER_MODRM(X86CT_OPER_DEST,instruction_info->opr_sz);
        SET_OPER_IMM16(X86CT_OPER_SRC1);
        break;
      case IML: /*** Immediate Dword to RM ***/
        SET_OPER_MODRM(X86CT_OPER_DEST,instruction_info->opr_sz);
        SET_OPER_IMM32(X86CT_OPER_SRC1);
        break;
      case IMSB: /*** Signed Immediate byte to RM ***/
        SET_OPER_MODRM(X86CT_OPER_DEST,instruction_info->opr_sz);
        SET_OPER_IMM8S(X86CT_OPER_SRC1);
        break;
      case RMR8: /*** Reg8 to RM8 ***/
        SET_OPER_MODRM_REG8(X86CT_OPER_SRC1);
        SET_OPER_MODRM8(X86CT_OPER_DEST);
        break;
      case RMR16: /*** Reg16 to RM16 ***/
        SET_OPER_MODRM_REG16(X86CT_OPER_SRC1);
        SET_OPER_MODRM16(X86CT_OPER_DEST);
        break;
      case RMR32: /*** Reg32 to RM32 ***/
        SET_OPER_MODRM_REG32(X86CT_OPER_SRC1);
        SET_OPER_MODRM32(X86CT_OPER_DEST);
        break;
      case RRM8: /*** RM8 to Reg8 ***/
        SET_OPER_MODRM_REG8(X86CT_OPER_DEST);
        SET_OPER_MODRM8(X86CT_OPER_SRC1);
        break;
      case RRM16: /*** RM16 to Reg16 ***/
        SET_OPER_MODRM_REG16(X86CT_OPER_DEST);
        SET_OPER_MODRM16(X86CT_OPER_SRC1);
        break;
      case RRM16_M: /*** M16 to Reg16 (R16 is ILLEGAL!!) ***/
        SET_OPER_MODRM_REG16(X86CT_OPER_DEST);
        SET_OPER_MODRM16(X86CT_OPER_SRC1);
        if (opers[X86CT_OPER_SRC1].type==X86CT_OPER_TYPE_REG)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        break;
      case RRM32_M: /*** M32 to Reg32 (R32 is ILLEGAL!!) ***/
        SET_OPER_MODRM_REG32(X86CT_OPER_DEST);
        SET_OPER_MODRM32(X86CT_OPER_SRC1);
        if (opers[X86CT_OPER_SRC1].type==X86CT_OPER_TYPE_REG)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        break;
      case RRM16_R: /*** R16 to Reg16 (M16 is ILLEGAL!!) ***/
        SET_OPER_MODRM_REG16(X86CT_OPER_DEST);
        SET_OPER_MODRM16(X86CT_OPER_SRC1);
        if (opers[X86CT_OPER_SRC1].type!=X86CT_OPER_TYPE_REG)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        break;
      case RRM32_R: /*** R32 to Reg32 (M32 is ILLEGAL!!) ***/
        SET_OPER_MODRM_REG32(X86CT_OPER_DEST);
        SET_OPER_MODRM32(X86CT_OPER_SRC1);
        if (opers[X86CT_OPER_SRC1].type!=X86CT_OPER_TYPE_REG)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        break;
      case RM16_M: /*** M16 (R16 is ILLEGAL!!) ***/
        SET_OPER_MODRM16(X86CT_OPER_DEST);
        if (opers[X86CT_OPER_DEST].type==X86CT_OPER_TYPE_REG)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        break;
      case RM32_M: /*** M32 (R32 is ILLEGAL!!) ***/
        SET_OPER_MODRM32(X86CT_OPER_DEST);
        if (opers[X86CT_OPER_DEST].type==X86CT_OPER_TYPE_REG)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        break;
      case RM64_M: /*** M64 (R64(multimedia) is ILLEGAL!!) ***/
        SET_OPER_MODRM64(X86CT_OPER_DEST);
        if (opers[X86CT_OPER_DEST].type==X86CT_OPER_TYPE_REG)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        if (opers[X86CT_OPER_DEST].type==X86CT_OPER_TYPE_MM)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        break;
      case RM80_M: /*** M80 (R80(?) is ILLEGAL!!) ***/
        SET_OPER_MODRM80(X86CT_OPER_DEST);
        if (opers[X86CT_OPER_DEST].type==X86CT_OPER_TYPE_REG)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        break;
      case RRM32: /*** RM32 to Reg32 ***/
        SET_OPER_MODRM_REG32(X86CT_OPER_DEST);
        SET_OPER_MODRM32(X86CT_OPER_SRC1);
        break;
      case CONST1: /*** const 1 (for int 1) ***/
        SET_OPER_CONST(X86CT_OPER_DEST,1);
        break;
      case CONST3: /*** const 3 (for int 3) ***/
        SET_OPER_CONST(X86CT_OPER_DEST,3);
        break;
      case SHFT_1: /*** const 1 shifts RM ***/
        SET_OPER_MODRM(X86CT_OPER_DEST,instruction_info->opr_sz);
        SET_OPER_CONST(X86CT_OPER_SRC1,1);
        break;
      case SHFT_CL: /*** const 1 shifts RM ***/
        SET_OPER_MODRM(X86CT_OPER_DEST,instruction_info->opr_sz);
        SET_OPER_CL(X86CT_OPER_SRC1);
        break;
      case RM:     /*** RM only ***/
        SET_OPER_MODRM(X86CT_OPER_DEST,instruction_info->opr_sz);
        break;
      case RM_O16:  /*** RM only , only 16 bit, operand size has no effect ***/
        SET_OPER_MODRM(X86CT_OPER_DEST,X86CT_OPER_SIZE_WORD);
        break;
      case ALRM:   /*** RM to AL ***/
        SET_OPER_MODRM(X86CT_OPER_SRC1,instruction_info->opr_sz);
        SET_OPER_AL(X86CT_OPER_DEST);
        break;
      case AXRM:  /*** RM to X86_AX  ***/
        SET_OPER_MODRM(X86CT_OPER_SRC1,instruction_info->opr_sz);
        SET_OPER_AX(X86CT_OPER_DEST);
        break;
      case EAXRM: /*** RM to X86_EAX ***/
        SET_OPER_MODRM(X86CT_OPER_SRC1,instruction_info->opr_sz);
        SET_OPER_EAX(X86CT_OPER_DEST);
        break;
      case AXO:  /***  X86_AX only  ***/
        SET_OPER_AX(X86CT_OPER_DEST);
        break;
      case RRM16I8S: /*** Signed IMM8 and RM16 to R16 ***/
        SET_OPER_MODRM_REG16(X86CT_OPER_DEST);
        SET_OPER_MODRM16(X86CT_OPER_SRC1);
        SET_OPER_IMM8S(X86CT_OPER_SRC2);
        break;
      case RRM32I8S: /*** Signed IMM8 and RM32 to R32 ***/
        SET_OPER_MODRM_REG32(X86CT_OPER_DEST);
        SET_OPER_MODRM32(X86CT_OPER_SRC1);
        SET_OPER_IMM8S(X86CT_OPER_SRC2);
        break;
      case RRM16I: /*** IMM16 and RM16 to R16 ***/
        SET_OPER_MODRM_REG16(X86CT_OPER_DEST);
        SET_OPER_MODRM16(X86CT_OPER_SRC1);
        SET_OPER_IMM16(X86CT_OPER_SRC2);
        break;
      case RRM32I: /*** IMM32 and RM32 to R32 ***/
        SET_OPER_MODRM_REG32(X86CT_OPER_DEST);
        SET_OPER_MODRM32(X86CT_OPER_SRC1);
        SET_OPER_IMM32(X86CT_OPER_SRC2);
        break;
      case R16_OP1: /*** R16 in opcode (e.g opcode = C8+i ) ***/
        UNGET_BYTE();
        SET_OPER_R16(X86CT_OPER_DEST,GET_OPCODE_REG(code));
		if (d_info->opcode_type==X86CT_OPOCDE_TYPE_OP1_OP2)
		{
			SET_OP2_AS_MODRM(X86CT_OPER_SIZE_WORD);
		}
		else  SET_OP1_AS_MODRM(X86CT_OPER_SIZE_WORD);
        break; 
      case R32_OP1: /*** R32 in opcode (e.g opcode = C8+i ) ***/
        UNGET_BYTE();
        SET_OPER_R32(X86CT_OPER_DEST,GET_OPCODE_REG(code));
		if (d_info->opcode_type==X86CT_OPOCDE_TYPE_OP1_OP2)
		{
			SET_OP2_AS_MODRM(X86CT_OPER_SIZE_WORD);
		}
		else  SET_OP1_AS_MODRM(X86CT_OPER_SIZE_WORD);
        break;
      case R8_OP1IM: /*** R8 in opcode (e.g opcode = C8+i ) and IMM8 ***/
        UNGET_BYTE();
        SET_OPER_R8(X86CT_OPER_DEST,GET_OPCODE_REG(code));
        SET_OPER_IMM8(X86CT_OPER_SRC1);
		if (d_info->opcode_type==X86CT_OPOCDE_TYPE_OP1_OP2)
		{
			SET_OP2_AS_MODRM(X86CT_OPER_SIZE_WORD);
		}
		else  SET_OP1_AS_MODRM(X86CT_OPER_SIZE_WORD);
       break; 
      case R16_OP1IM: /*** R16 in opcode (e.g opcode = C8+i ) and IMM16 ***/
        UNGET_BYTE();
        SET_OPER_R16(X86CT_OPER_DEST,GET_OPCODE_REG(code));
        SET_OPER_IMM16(X86CT_OPER_SRC1);
		if (d_info->opcode_type==X86CT_OPOCDE_TYPE_OP1_OP2)
		{
			SET_OP2_AS_MODRM(X86CT_OPER_SIZE_WORD);
		}
		else  SET_OP1_AS_MODRM(X86CT_OPER_SIZE_WORD);
        break; 
      case R32_OP1IM: /*** R32 in opcode (e.g opcode = C8+i ) and IMM32 ***/
        UNGET_BYTE();
        SET_OPER_R32(X86CT_OPER_DEST,GET_OPCODE_REG(code));
        SET_OPER_IMM32(X86CT_OPER_SRC1);
		if (d_info->opcode_type==X86CT_OPOCDE_TYPE_OP1_OP2)
		{
			SET_OP2_AS_MODRM(X86CT_OPER_SIZE_WORD);
		}
		else  SET_OP1_AS_MODRM(X86CT_OPER_SIZE_WORD);
       break;
      case IMOB:   /*** IMM8 only ***/
        SET_OPER_IMM8(X86CT_OPER_DEST);
        break;
      case IMOW:   /*** IMM16 only ***/
        SET_OPER_IMM16(X86CT_OPER_DEST);
        break;
      case IMOL:   /*** IMM32 only ***/
        SET_OPER_IMM32(X86CT_OPER_DEST);
        break;
      case RL8:   /*** REL8 only ***/
        SET_OPER_REL8(X86CT_OPER_DEST);
        break;
      case RL16:  /*** REL16 only ***/
        SET_OPER_REL16(X86CT_OPER_DEST);
        break;
      case RL32:  /*** REL32 only ***/
        SET_OPER_REL32(X86CT_OPER_DEST);
        break;
        
      case SEG_OF16: /*** IMM16 & IMM16 (selector:offset16) ***/
        SET_OPER_SEG_OFF16(X86CT_OPER_DEST);
        opers[X86CT_OPER_DEST].flags=X86CT_OPER_FLAG_ABS_ADDR;
        break;
        
      case SEG_OF32: /*** IMM16 & IMM32 (selector:offset32) ***/
        SET_OPER_SEG_OFF32(X86CT_OPER_DEST);
        opers[X86CT_OPER_DEST].flags=X86CT_OPER_FLAG_ABS_ADDR;
        break;
        
      case IM16_8: /*** IMM16 & IMM8 ***/
        SET_OPER_IMM16(X86CT_OPER_DEST);
        SET_OPER_IMM8(X86CT_OPER_SRC1);
        break;
      case RMS16: /*** Sreg to RM16  ***/ 
        SET_OPER_MODRM_SREG(X86CT_OPER_SRC1);
        SET_OPER_MODRM16(X86CT_OPER_DEST);
        break;
      case RMS32: /*** Sreg to RM32  ***/ 
        SET_OPER_MODRM_SREG(X86CT_OPER_SRC1);
        SET_OPER_MODRM32(X86CT_OPER_DEST);
        break;
      case SRM16: /*** RM16 to Sreg ***/
        SET_OPER_MODRM_SREG(X86CT_OPER_DEST);
        if (opers[X86CT_OPER_DEST].value == X86_CS)
        {
            Decode_error(X86CT_DECODE_OPERAND_ERR);
        }
        SET_OPER_MODRM16(X86CT_OPER_SRC1);
        break;
      case ALOFF:   /*** Mem operand with offset to AL ***/
        SET_OPER_AL(X86CT_OPER_DEST);
        if (d_info->address_size == X86CT_ACTIVE_ADDR_SIZE_WORD)
        {
            SET_OPER_OFF_16(X86CT_OPER_SRC1);
        }
        else
        {
            SET_OPER_OFF_32(X86CT_OPER_SRC1);
        }
        opers[X86CT_OPER_SRC1].size=X86CT_OPER_SIZE_BYTE;
        break;
      case EAXOFF:   /*** Mem operand with Offset to X86_AX ***/
        if (d_info->operand_size == X86CT_ACTIVE_OPER_SIZE_WORD)
        {
            SET_OPER_AX(X86CT_OPER_DEST);
            opers[X86CT_OPER_SRC1].size=X86CT_OPER_SIZE_WORD;
        }
        else
        {
            SET_OPER_EAX(X86CT_OPER_DEST);
            opers[X86CT_OPER_SRC1].size=X86CT_OPER_SIZE_LONG;
        }
        if (d_info->address_size == X86CT_ACTIVE_ADDR_SIZE_WORD)
        {
            SET_OPER_OFF_16(X86CT_OPER_SRC1);
        }
        else
        {
            SET_OPER_OFF_32(X86CT_OPER_SRC1);
        }
        break;
      case OFFAL:   /*** Mem operand with offset to AL ***/
        SET_OPER_AL(X86CT_OPER_SRC1);
        if (d_info->address_size == X86CT_ACTIVE_ADDR_SIZE_WORD)
        {
            SET_OPER_OFF_16(X86CT_OPER_DEST);
        }
        else
        {
            SET_OPER_OFF_32(X86CT_OPER_DEST);
        }
        opers[X86CT_OPER_DEST].size=X86CT_OPER_SIZE_BYTE;
        break;
      case OFFEAX:   /*** Mem operand with Offset to X86_AX ***/
        if (d_info->operand_size == X86CT_ACTIVE_OPER_SIZE_WORD)
        {
            SET_OPER_AX(X86CT_OPER_SRC1);
            opers[X86CT_OPER_DEST].size=X86CT_OPER_SIZE_WORD;
        }
        else
        {
            SET_OPER_EAX(X86CT_OPER_SRC1);
            opers[X86CT_OPER_DEST].size=X86CT_OPER_SIZE_LONG;
        }
        if (d_info->address_size == X86CT_ACTIVE_ADDR_SIZE_WORD)
        {
            SET_OPER_OFF_16(X86CT_OPER_DEST);
        }
        else
        {
            SET_OPER_OFF_32(X86CT_OPER_DEST);
        }
        break;
      case CRR:        /*** Reg to Creg ***/
        SET_OPER_MODRM_CR(X86CT_OPER_DEST);
        SET_OPER_R32(X86CT_OPER_SRC1, GET_OPCODE_REG(code));
        break;
      case RCR:       /*** Creg to Reg ***/
        SET_OPER_MODRM_CR(X86CT_OPER_SRC1);
        SET_OPER_R32(X86CT_OPER_DEST, GET_OPCODE_REG(code));
        break;
      case DRR:      /*** Reg to Dreg ***/
        SET_OPER_MODRM_DR(X86CT_OPER_DEST);
        SET_OPER_R32(X86CT_OPER_SRC1, GET_OPCODE_REG(code));
        break;
      case RDR:     /*** Dreg to Reg ***/
        SET_OPER_MODRM_DR(X86CT_OPER_SRC1);
        SET_OPER_R32(X86CT_OPER_DEST, GET_OPCODE_REG(code));
        break;
      case R16_RMB: /*** RM8 -> R16 ***/  
        SET_OPER_MODRM_REG16(X86CT_OPER_DEST);
        SET_OPER_MODRM8(X86CT_OPER_SRC1);
        break;
      case R32_RMB: /*** RM8 -> R32 ***/
        SET_OPER_MODRM_REG32(X86CT_OPER_DEST);
        SET_OPER_MODRM8(X86CT_OPER_SRC1);
        break;
      case R32_RMW: /*** RM16 -> R32 ***/
        SET_OPER_MODRM_REG32(X86CT_OPER_DEST);
        SET_OPER_MODRM16(X86CT_OPER_SRC1);
        break;
      case IPAL:
        SET_OPER_PORT(X86CT_OPER_DEST);
        SET_OPER_AL(X86CT_OPER_SRC1);
        break;
      case IPAX:
        SET_OPER_PORT(X86CT_OPER_DEST);
        SET_OPER_AX(X86CT_OPER_SRC1);
        break;
      case IPEAX:
        SET_OPER_PORT(X86CT_OPER_DEST);
        SET_OPER_EAX(X86CT_OPER_SRC1);
        break;
      case ALIP:
        SET_OPER_PORT(X86CT_OPER_SRC1);
        SET_OPER_AL(X86CT_OPER_DEST);
        break;
      case AXIP:
        SET_OPER_PORT(X86CT_OPER_SRC1);
        SET_OPER_AX(X86CT_OPER_DEST);
        break;
      case EAXIP:
        SET_OPER_PORT(X86CT_OPER_SRC1);
        SET_OPER_EAX(X86CT_OPER_DEST);
        break;
      case DXAL:
        SET_OPER_DX_PORT(X86CT_OPER_DEST);
        SET_OPER_AL(X86CT_OPER_SRC1);
        break;
      case DXAX:
        SET_OPER_DX_PORT(X86CT_OPER_DEST);
        SET_OPER_AX(X86CT_OPER_SRC1);
        break;
      case DXEAX:
        SET_OPER_DX_PORT(X86CT_OPER_DEST);
        SET_OPER_EAX(X86CT_OPER_SRC1);
        break;
      case ALDX:
        SET_OPER_AL(X86CT_OPER_DEST);
        SET_OPER_DX_PORT(X86CT_OPER_SRC1);
        break;
      case AXDX:
        SET_OPER_AX(X86CT_OPER_DEST);
        SET_OPER_DX_PORT(X86CT_OPER_SRC1);
        break;
      case EAXDX:
        SET_OPER_EAX(X86CT_OPER_DEST);
        SET_OPER_DX_PORT(X86CT_OPER_SRC1);
        break;
      case DX:
        SET_OPER_DX_PORT(X86CT_OPER_DEST);
        break;
      case RDS:
        SET_OPER_DS(X86CT_OPER_DEST);
        break;
      case RES:
        SET_OPER_ES(X86CT_OPER_DEST);
        break;
      case RSS:
        SET_OPER_SS(X86CT_OPER_DEST);
        break;
      case RFS:
        SET_OPER_FS(X86CT_OPER_DEST);
        break;
      case RGS:
        SET_OPER_GS(X86CT_OPER_DEST);
        break;
      case RCS:
        SET_OPER_CS(X86CT_OPER_DEST);
        break;
      case RMR16I8:
        SET_OPER_MODRM_REG16(X86CT_OPER_SRC1);
        SET_OPER_MODRM16(X86CT_OPER_DEST);
        SET_OPER_IMM8(X86CT_OPER_SRC2);
        break;
      case RMR32I8:
        SET_OPER_MODRM_REG32(X86CT_OPER_SRC1);
        SET_OPER_MODRM32(X86CT_OPER_DEST);
        SET_OPER_IMM8(X86CT_OPER_SRC2);
        break;
      case RMR16CL:
        SET_OPER_MODRM_REG16(X86CT_OPER_SRC1);
        SET_OPER_MODRM16(X86CT_OPER_DEST);
        SET_OPER_CL(X86CT_OPER_SRC2);
        break;
      case RMR32CL:
        SET_OPER_MODRM_REG32(X86CT_OPER_SRC1);
        SET_OPER_MODRM32(X86CT_OPER_DEST);
        SET_OPER_CL(X86CT_OPER_SRC2);
        break;
      case AXR16:
        UNGET_BYTE();
        SET_OPER_AX(X86CT_OPER_DEST);
        SET_OPER_R16(X86CT_OPER_SRC1,GET_OPCODE_REG(code));
        break;
      case EAXR32:
        UNGET_BYTE();
        SET_OPER_EAX(X86CT_OPER_DEST);
        SET_OPER_R32(X86CT_OPER_SRC1,GET_OPCODE_REG(code));
        break;
      case ST_STI:
        SET_OPER_ST(X86CT_OPER_DEST);
        SET_OPER_MODRM_ST(X86CT_OPER_SRC1);
        break;
      case STI_ST:
        SET_OPER_ST(X86CT_OPER_SRC1);
        SET_OPER_MODRM_ST(X86CT_OPER_DEST);
        break;
      case STIO:
        SET_OPER_MODRM_ST(X86CT_OPER_DEST);
        break;
      case RM8_M:
        SET_OPER_MODRM8(X86CT_OPER_DEST);
        if (opers[X86CT_OPER_DEST].type==X86CT_OPER_TYPE_REG)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        break;

      case mmRM64:  /*** RM64 to MMREG ***/
        SET_OPER_MODRM_MM(X86CT_OPER_DEST); 
        SET_OPER_MODRM64(X86CT_OPER_SRC1);
        break;
      case mmRM32:  /*** RM32 to MMREG ***/
        SET_OPER_MODRM_MM(X86CT_OPER_DEST);
        SET_OPER_MODRM32(X86CT_OPER_SRC1);
        break;
      case RM32mm:  /*** MMREG to RM32 ***/
        SET_OPER_MODRM_MM(X86CT_OPER_SRC1);
        SET_OPER_MODRM32(X86CT_OPER_DEST);
        break;
      case RM64mm:  /*** MMREG to RM64 ***/
        SET_OPER_MODRM_MM(X86CT_OPER_SRC1);
        SET_OPER_MODRM64(X86CT_OPER_DEST);
        break;
      case mmI8:    /*** Immidiate Byte change MMREG ***/
        SET_OPER_MODRM64(X86CT_OPER_DEST);
        if (opers[X86CT_OPER_DEST].type==X86CT_OPER_TYPE_MEM)
          Decode_error(X86CT_DECODE_OPERAND_ERR);
        SET_OPER_IMM8(X86CT_OPER_SRC1);
        break;

	  case X86CT_NO_ENCODE:
	  case W_AXO:
	  case W_RM:
		break;
    }

    /*** check for instruction size errors ***/

    /*** instruction size ****/ 
    if ((d_info->inst_size=current_byte)>MAX_INSTRUCTION_LENGTH)
    {
        Decode_error(X86CT_DECODE_TOO_LONG_OPCODE);
    }
    
    /*** buffer too short ***/
    if (current_byte > code_size)
    {
        Decode_error(X86CT_DECODE_TOO_SHORT_ERR);
    }

    /*** check for instruction properties errors ***/

    /*** Lock found before non-lock instruction ***/
    if (d_info->lock)
    {
        if (VALID_LOCK(d_info->info))
        {
            /*** destination must be a memory operand or
			     it is the XCHG instruction ***/
            if ((opers[X86CT_OPER_DEST].type != X86CT_OPER_TYPE_MEM) &&
                (d_info->opcode_1byte != 0x86) &&  /* XCHG opcode */
                (d_info->opcode_1byte != 0x87))    /* XCHG opcode */
            {
                Decode_error(X86CT_DECODE_LOCK_ERR);
            }
        }
        else
        {
            Decode_error(X86CT_DECODE_LOCK_ERR);
        }
    }

    /*** Illegal instruction for that mode (8086,V86,P5 etc) ***/
    if ((d_info->info->flags & Decode_machine) != Decode_machine)
    {
        Decode_error(X86CT_DECODE_MODE_ERR);
    }
    return(decode_err);
}
/************************************************************************
* set_oper_modrm(X86CT_byte *code,int code_size,
*                                decoder_info *d_info,int i,int size)
* Put the ModRM + [SIB] + [disp8/32] infromation in the operand
* strcuture while advancing teh code counter etc.
* 
* 
* 
* input: 
* ======
* 1) code - pointer to code buffer 
* 2) code_size - buffer size (might be less then we need)
* 3) d_info - pointer to decoder_info structure, to be filled by routine
* 5) i - index of operand to fill.
* 6) size - of operand
* 
* output:
* =======
* The operand in the given structure
* 
* 
************************************************************************/
static  void    set_oper_modrm(X86CT_byte *code, int code_size,
                                X86CT_Decoder_info *d_info, int i, int size)
{
    /***** variables *****/
     X86CT_byte modrm=(X86CT_byte)GET_BYTE(code);
     X86CT_Oper_info *opers=d_info->operands;
     
    /*******************/
    /***** process *****/
    /*******************/
	 if (d_info->mrm_info.mrm_type != X86CT_MODRM_REG)
	 {
		if (d_info->mrm_info.mrm_type == X86CT_MODRM_NONE)
		{
			d_info->mrm_info.mrm_type = X86CT_MODRM_REG;
		}
		d_info->mrm_info.modrm = modrm;
		d_info->mrm_info.mrm_opr_size = size;
	 }
     opers[i]=ModRM[GET_MODRM_INDEX(modrm)];           
     DPRINTF(("DCD:modrm=0(mod 0 reg 0 rm 0 index 0) ",
              modrm,GET_MODRM_MOD(modrm),GET_MODRM_REG(modrm),
              GET_MODRM_RM(modrm),GET_MODRM_INDEX(modrm)));
     /*** need SIB ? ***/
     if (opers[i].type==X86CT_NO_OPER)                       
     {                                                 
         X86CT_byte sib=(X86CT_byte)GET_BYTE(code);
		 d_info->mrm_info.sib = sib;
         opers[i]=SIB[GET_SIB_INDEX(modrm,sib)];       
         DPRINTF(("sib=0(s 0 i 0 b 0 index 0)",   
                  sib,GET_SIB_S(sib),GET_SIB_I(sib),   
                  GET_SIB_B(sib),GET_SIB_INDEX(sib,modrm)));
         opers[i].mem_scale=(1 << GET_SIB_S(sib));     
         opers[i].mem_index=reg32_index[GET_SIB_I(sib)];\
     }                                                 
     opers[i].size=(int)size;
     if (opers[i].type==X86CT_OPER_TYPE_REG)                 
     {                                                 
         DPRINTF(("Reg0[0]\n",d_info->info->opr_sz,GET_MODRM_RM(modrm)));
         switch(size)              
         {                                             
           case X86CT_OPER_SIZE_BYTE:
            opers[i].value=reg8[GET_MODRM_RM(modrm)];  
            break;                                     
           case X86CT_OPER_SIZE_WORD:                                    
            opers[i].value=reg16[GET_MODRM_RM(modrm)]; 
            break;                                     
           case X86CT_OPER_SIZE_LONG:                                    
            opers[i].value=reg32[GET_MODRM_RM(modrm)]; 
            break;                                     
          case X86CT_OPER_SIZE_QUAD:           /* assume that only MM register (in x86) is 64 bit reg */  
            opers[i].value=regmm[GET_MODRM_RM(modrm)];
            opers[i].type=X86CT_OPER_TYPE_MM;   /* use the old ModRM table and change the type to MM */
            break;
        }                                             
     }
     else
       d_info->memory=1;
     /*** if offset!=0 , the value is it's size ***/   
     DPRINTF(("OFF0=", opers[i].mem_offset));
	 opers[i].dis_size = (X86CT_Oper_size)opers[i].mem_offset;
     switch(opers[i].mem_offset)                       
     {                                                 
       case X86CT_OPER_SIZE_BYTE:                                         
         opers[i].mem_offset=SIGN_EXT((X86CT_byte)GET_BYTE(code),8);           
         break;                                        
       case X86CT_OPER_SIZE_WORD:                                        
         opers[i].mem_offset=SIGN_EXT(GET_WORD(code),16);           
         break;                                        
       case X86CT_OPER_SIZE_LONG:                                        
         opers[i].mem_offset=GET_DWORD(code);          
         break;
         
     };                                                
    DPRINTF(("0x0\n", opers[i].mem_offset));          
    if (d_info->seg!=NO_REG)                           
      opers[i].mem_seg=d_info->seg;                    
    return;
}                                                  



