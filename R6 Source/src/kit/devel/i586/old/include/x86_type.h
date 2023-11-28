/***************************************/
/*****	 X86 INSTRUCTION HEADER  *****/
/***************************************/
#ifndef _X86_decoder_X86CT_H_
#define _X86_decoder_X86CT_H_
typedef char *mnem_type;
typedef  int opc1_type;
typedef  int opc2_type;
typedef  int xopc_type;
typedef  int opr_sz_type;
typedef  int mem_sz_type;
typedef  int iopr_sz_type;
typedef  unsigned long flags_type;
typedef  void *func_type;
typedef func_type emit_r_type;
typedef func_type emit_m_type;
typedef X86CT_Encode_type encode_type;
typedef int inst_encode_type;


typedef struct X86_decoder_X86CT_INFO_TYPE {
	x86_enum_type		x86_enum;
	mem_sz_type		mem_sz;
	iopr_sz_type		iopr_sz;
	flags_type		flags;
	flags_type		ext_flags;
	opr_sz_type		opr_sz;
	encode_type		encode;
} X86_decoder_X86CT_INFO;

#endif  /* _X86_decoder_X86CT_H_ */
