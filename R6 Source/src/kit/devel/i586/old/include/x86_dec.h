/********************************************************************
*                      X86CTL
*            --------------------------
* decode.h:
*    Header file for the X86 decoder users.
*    
*
*  Defines: 
*  ==================
*  o  Oper_type - operand type (to be returned by decoder)
*  o  oper_struct - opernad info from decoder.
*  o  decoder_info structure;
*  o  the decoder functions
*  o  Decode_errno enumaretor.
*
*  Note: INFO_STRUCT should be define before including this file.   
*
* 
*  IMPORTANT : you must include the file X86CT.h before this file.
*
*********************************************************************/
#ifndef _X86_decoder_DECODE_H
#define _X86_decoder_DECODE_H

/*****************/
/** Oper_type ****/
/*****************/
typedef enum
{
    X86CT_OPER_TYPE_NONE=0,
    X86CT_OPER_TYPE_REG,
    X86CT_OPER_TYPE_IMM,
    X86CT_OPER_TYPE_MEM,
    X86CT_OPER_TYPE_ST,
    X86CT_OPER_TYPE_MM,
    X86CT_OPER_TYPE_PORT=7,
	X86CT_OPER_TYPE_REL,
    X86CT_OPER_TYPE_SEG_OFF
} X86CT_Oper_type;

#define X86CT_OPER_FLAG_NONE             0x000
#define X86CT_OPER_FLAG_SIGN_EXTENDED    0x001
#define X86CT_OPER_FLAG_IMPLIED          0x002
#define X86CT_OPER_FLAG_ABS_ADDR         0x004
#define X86CT_OPER_FLAG_PORT_IN_DX       0x008

/*****************/
/** Oper_size ****/
/*****************/
typedef enum
{
    X86CT_OPER_SIZE_BYTE=8,
    X86CT_OPER_SIZE_WORD=16,
    X86CT_OPER_SIZE_LONG=32,
    X86CT_OPER_SIZE_QUAD=64,
    X86CT_OPER_SIZE_EXT=80,
	X86CT_OPER_SIZE_DQUAD = 128
} X86CT_Oper_size;
/******************************/
/** X86CT_Active_oper_size ****/
/******************************/
typedef enum
{
    X86CT_ACTIVE_OPER_SIZE_WORD=16,
    X86CT_ACTIVE_OPER_SIZE_LONG=32
} X86CT_Active_oper_size;
/******************************/
/** X86CT_Active_addr_size ****/
/******************************/
typedef enum
{
    X86CT_ACTIVE_ADDR_SIZE_WORD=16,
    X86CT_ACTIVE_ADDR_SIZE_LONG=32
} X86CT_Active_addr_size;

/*****************/
/** Decode_errno */
/*****************/
typedef enum
{
    X86CT_DECODE_NO_ERROR=0,
    X86CT_DECODE_SET_ERR,
    X86CT_DECODE_MODE_ERR,
    X86CT_DECODE_INVALID_OPCODE,
    X86CT_DECODE_INVALID_PRM_OPCODE,
    X86CT_DECODE_TOO_LONG_OPCODE,
    X86CT_DECODE_LOCK_ERR,
    X86CT_DECODE_OPERAND_ERR,
    X86CT_DECODE_TOO_SHORT_ERR
} X86CT_Decode_errno;
/*******************/
/*** prefix info ***/
/*******************/
typedef struct
{
	unsigned char repeat_type;
	unsigned char n_prefixes;
	unsigned char n_rep_pref;
	unsigned char n_lock_pref;
	unsigned char n_seg_pref;
	unsigned char n_oper_size_pref;
	unsigned char n_addr_size_pref;
} X86CT_Prefix_info;

typedef enum
{
	X86CT_OPOCDE_TYPE_NONE=0,
	X86CT_OPOCDE_TYPE_OP1,
	X86CT_OPOCDE_TYPE_OP1_XOP,
	X86CT_OPOCDE_TYPE_OP1_OP2,
	X86CT_OPOCDE_TYPE_OP1_OP2_XOP	
} X86CT_Opcode_type;
/*****************/
/** oper_info   **/
/*****************/
typedef struct
{
    X86CT_Oper_type type;
    UL              value;
    X86_Register mem_seg;
    UL             mem_offset;
    X86_Register mem_base;
    X86_Register mem_index;
    UL             mem_scale;
	X86CT_Oper_size size;
	UL              flags;
	X86CT_Oper_size dis_size;
} X86CT_Oper_info;
/*****************/
/** ModR/M info **/
/*****************/
typedef enum
{
  X86CT_MODRM_NONE = 0,
  X86CT_MODRM_REG = 1,
  X86CT_MODRM_XOP = 2,
  X86CT_MODRM_OP2 = 3,
  X86CT_MODRM_OP1 = 4
} X86CT_ModRM_Type;

typedef struct
{
	unsigned char         modrm;
	unsigned char         sib;
	X86CT_ModRM_Type      mrm_type;
	int                   mrm_opr_size;
}X86CT_Decode_ModRM_Info;

/*****************/
/** decoder_info */
/*****************/
typedef struct
{
    int rep;
    int lock;
	int opcode_1byte;
	int opcode_2byte;
    X86_Register seg;
    X86CT_Active_oper_size operand_size;
    X86CT_Active_addr_size address_size;
	int memory;
    X86CT_Oper_info operands[3];
    int inst_size;
	X86CT_Prefix_info prefix_info;
	X86CT_Opcode_type opcode_type;
    X86_decoder_X86CT_INFO *info;
	X86CT_Decode_ModRM_Info    mrm_info;
} X86CT_Decoder_info;

/*******************/
/** Operand_role  **/
/*******************/
typedef enum
{   /***
         This is index to operands. The operands array above will be filled
         by order (if you have one operand only it will always be in
         operands[0]). But when the decoder finds both SRC operand and DEST
         operand , it will put them in the array by these names.
     ***/
    X86CT_OPER_DEST=0, /* destination      */
    X86CT_OPER_SRC1=1, /* source 1         */
    X86CT_OPER_SRC2=2  /* source 2         */
} X86CT_Operand_role;

/****************/
/** table type **/
/****************/
/* CT tables types (used by decoder) */
typedef enum 
{
    X86CT_NO_TYPE=0,
    X86CT_TYPE_OP1,
    X86CT_TYPE_OP2,
    X86CT_TYPE_XOP,
    X86CT_TYPE_INFO,
    X86CT_TYPE_INFO_XOP,
	X86CT_TYPE_PREFIX
} X86CT_Table_type;

/****************/
/** opcode tbl **/
/****************/
typedef struct
{
    void *table;
    X86CT_Table_type type;
} X86CT_Opcode_Tbl;



/*****************/
/** FUNCTIONS ****/
/*****************/
extern X86CT_Decode_errno X86_decoder_set_decoder(unsigned long machine_flag,
                                     X86CT_Active_oper_size size);
extern X86CT_Decode_errno X86_decoder_decode(X86CT_byte *code,int code_size,
									  X86CT_Decoder_info *d_info);

#endif /* _X86_decoder_DECODE_H */



