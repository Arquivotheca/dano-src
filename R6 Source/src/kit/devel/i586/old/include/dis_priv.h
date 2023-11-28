#ifndef DISASM_PRIV_H
#define DISASM_PRIV_H

#include "decp62.h"


#define ALIGN 4
#define FIRST (IA_DECODER_REG_LAST+1)

/***************************************/
/***   valid types checking macros   ***/
/***************************************/

#define		legal_type(type)      \
	((((type) >= 0) && ((type) < IA_DECODER_CPU_LAST) )? 1:0)

#define		legal_mode(mode)      \
	((((mode) >= 0) && ((mode) < IA_DECODER_MODE_LAST) )? 1:0)

#define		legal_aliases(aliases) \
	 (1)

#define		legal_radix(radix)    \
	(((radix) < IA_DIS_RADIX_LAST) ? 1:0)

#define		legal_style(style)    \
	(((style) < IA_DIS_STYLE_LAST) ? 1:0)


/************************************************************************/
/***   registers names and aliases ( keep order with ia_decoder.h )   ***/
/************************************************************************/

typedef struct _dis_regs
{
	IA_Decoder_Reg_Name reg_name;
	char * 				reg_str;
	char *              reg_alias;      /* Total number of registers from this type */
	char *              reg_masm_str;
} Dis_Regs;

typedef struct _dis_regs_files
{
	IA_Decoder_Reg_Name regfile_name;
	char * 				regfile_str;
	char *              regfile_alias;      /* Total number of registers from this type */
	char *              regfile_masm_str;
} Dis_Regfile; 


typedef enum
{
	DIS_EMPTY,
	DIS_ADDR,
	DIS_PREG,
	DIS_MNEM,
	DIS_OPER
} Operand_Type;

#define	DIS_MEM_BASE  1
#define DIS_MEM_INDEX 2
#define DIS_MEM_SCALE 4
	

#define ADDR_SIZE 8

/* Tab1 - after <%p> */
#define DIS_TAB1 (tabstop+ADDR_SIZE)
/* Tab 2 - after mnemonic */
#define DIS_TAB2 (DIS_TAB1+8)


struct TOOL_X86CT_INFO_STR
{
	IA_Decoder_Inst_Id inst_id;
	char               opr1;
	char               opr2;
	char               opr3;
	char *             Mnem;
	char *             Mnem1;
};

typedef struct TOOL_X86CT_INFO_STR TOOL_X86CT_INFO;


#endif		/*** DISASM_PRIV_H ***/



