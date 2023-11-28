#ifndef __ASM_H__
#define __ASM_H__

#include <OS.h>


#define EAX 1
#define ECX 2
#define EDX 3
#define EBX 4
#define ESP 5
#define EBP 6
#define ESI 7
#define EDI 8

#define AL 9
#define CL 10
#define DL 11
#define BL 12
#define AH 13
#define CH 14
#define DH 15
#define BH 16

#define ST0 1
#define ST1 2
#define ST2 3
#define ST3 4
#define ST4 5
#define ST5 6
#define ST6 7
#define ST7 8

#define MM0 1
#define MM1 2
#define MM2 3
#define MM3 4
#define MM4 5
#define MM5 6
#define MM6 7
#define MM7 8

#define XMM0 1
#define XMM1 2
#define XMM2 3
#define XMM3 4
#define XMM4 5
#define XMM5 6
#define XMM6 7
#define XMM7 8

#define MOD_BITS_MEM_0 1
#define MOD_BITS_MEM_8 2
#define MOD_BITS_MEM_32 3
#define MOD_BITS_REG 4

typedef struct asmContextRec
{
	uint8 *codeBase;
	uint8 *pc;
	int32 len;
	int32 writing;
	
} asmContext;

#define WRITE_BYTE( byte ) { if( c->writing ) *c->pc=(byte); c->len++; c->pc++; }

#define WRITE_DWORD( data ) { \
	WRITE_BYTE( (data) & 0xff ); \
	WRITE_BYTE( ((data) >> 8) & 0xff ); \
	WRITE_BYTE( ((data) >> 16) & 0xff ); \
	WRITE_BYTE( ((data) >> 24) & 0xff ); \
	}

#ifdef __cplusplus
extern "C" {
#endif

void  asm_ADC_RI( asmContext *c, int32 reg, int32 value );
void  asm_ADC_RR( asmContext *c, int32 regD, int32 regS );
void  asm_ADC_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void  asm_ADC_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void  asm_ADD_RI( asmContext *c, int32 reg, int32 value );
void  asm_ADD_RR( asmContext *c, int32 regD, int32 regS );
void  asm_ADD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void  asm_ADD_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void  asm_AND_RI( asmContext *c, int32 reg, int32 value );
void  asm_AND_RR( asmContext *c, int32 regD, int32 regS );
void  asm_AND_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void  asm_AND_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void  asm_BSWAP( asmContext *c, int32 reg );

void asm_CALL_I( asmContext *c, int32 offset );
void asm_CALL_R( asmContext *c, int32 reg );
void asm_CALL_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void  asm_CMP_RI( asmContext *c, int32 reg, int32 value );
void  asm_CMP_RR( asmContext *c, int32 regD, int32 regS );
void  asm_CMP_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void  asm_CMP_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_DEC_R( asmContext *c, int32 reg );
void asm_DEC_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_DIV_R( asmContext *c, int32 reg );
void asm_DIV_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_IDIV_R( asmContext *c, int32 reg );
void asm_IDIV_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_IMUL_AR( asmContext *c, int32 reg );
void asm_IMUL_AM( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_IMUL_RM( asmContext *c, int32 dreg, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_IMUL_RR( asmContext *c, int32 dreg, int32 sreg );
void asm_IMUL_RMI( asmContext *c, int32 dreg, int32 off_Base, int32 off_Index, int32 mult, int32 offset, int32 immed );
void asm_IMUL_RRI( asmContext *c, int32 dreg, int32 sreg, int32 immed );

void asm_INC_R( asmContext *c, int32 reg );
void asm_INC_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_JA_8( asmContext *c, int32 offset );
void asm_JAE_8( asmContext *c, int32 offset );
void asm_JB_8( asmContext *c, int32 offset );
void asm_JBE_8( asmContext *c, int32 offset );
void asm_JC_8( asmContext *c, int32 offset );
void asm_JECXZ_8( asmContext *c, int32 offset );
void asm_JE_8( asmContext *c, int32 offset );
void asm_JG_8( asmContext *c, int32 offset );
void asm_JGE_8( asmContext *c, int32 offset );
void asm_JL_8( asmContext *c, int32 offset );
void asm_JLE_8( asmContext *c, int32 offset );
void asm_JNA_8( asmContext *c, int32 offset );
void asm_JNAE_8( asmContext *c, int32 offset );
void asm_JNB_8( asmContext *c, int32 offset );
void asm_JNBE_8( asmContext *c, int32 offset );
void asm_JNC_8( asmContext *c, int32 offset );
void asm_JNE_8( asmContext *c, int32 offset );
void asm_JNG_8( asmContext *c, int32 offset );
void asm_JNGE_8( asmContext *c, int32 offset );
void asm_JNL_8( asmContext *c, int32 offset );
void asm_JNLE_8( asmContext *c, int32 offset );
void asm_JNO_8( asmContext *c, int32 offset );
void asm_JNP_8( asmContext *c, int32 offset );
void asm_JNS_8( asmContext *c, int32 offset );
void asm_JNZ_8( asmContext *c, int32 offset );
void asm_JO_8( asmContext *c, int32 offset );
void asm_JP_8( asmContext *c, int32 offset );
void asm_JPE_8( asmContext *c, int32 offset );
void asm_JPO_8( asmContext *c, int32 offset );
void asm_JS_8( asmContext *c, int32 offset );
void asm_JZ_8( asmContext *c, int32 offset );

void asm_JA_32( asmContext *c, int32 offset );
void asm_JAE_32( asmContext *c, int32 offset );
void asm_JB_32( asmContext *c, int32 offset );
void asm_JBE_32( asmContext *c, int32 offset );
void asm_JC_32( asmContext *c, int32 offset );
void asm_JE_32( asmContext *c, int32 offset );
void asm_JG_32( asmContext *c, int32 offset );
void asm_JGE_32( asmContext *c, int32 offset );
void asm_JL_32( asmContext *c, int32 offset );
void asm_JLE_32( asmContext *c, int32 offset );
void asm_JNA_32( asmContext *c, int32 offset );
void asm_JNAE_32( asmContext *c, int32 offset );
void asm_JNB_32( asmContext *c, int32 offset );
void asm_JNBE_32( asmContext *c, int32 offset );
void asm_JNC_32( asmContext *c, int32 offset );
void asm_JNE_32( asmContext *c, int32 offset );
void asm_JNG_32( asmContext *c, int32 offset );
void asm_JNGE_32( asmContext *c, int32 offset );
void asm_JNL_32( asmContext *c, int32 offset );
void asm_JNLE_32( asmContext *c, int32 offset );
void asm_JNO_32( asmContext *c, int32 offset );
void asm_JNP_32( asmContext *c, int32 offset );
void asm_JNS_32( asmContext *c, int32 offset );
void asm_JNZ_32( asmContext *c, int32 offset );
void asm_JO_32( asmContext *c, int32 offset );
void asm_JP_32( asmContext *c, int32 offset );
void asm_JPE_32( asmContext *c, int32 offset );
void asm_JPO_32( asmContext *c, int32 offset );
void asm_JS_32( asmContext *c, int32 offset );
void asm_JZ_32( asmContext *c, int32 offset );

void asm_JMP_8( asmContext *c, int32 offset );
void asm_JMP_32( asmContext *c, int32 offset );
void asm_JMP_32R( asmContext *c, int32 reg );
void asm_JMP_32M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_fix_JA_8( uint8 *c, int32 offset );
void asm_fix_JAE_8( uint8 *c, int32 offset );
void asm_fix_JB_8( uint8 *c, int32 offset );
void asm_fix_JBE_8( uint8 *c, int32 offset );
void asm_fix_JC_8( uint8 *c, int32 offset );
void asm_fix_JECXZ_8( uint8 *c, int32 offset );
void asm_fix_JE_8( uint8 *c, int32 offset );
void asm_fix_JG_8( uint8 *c, int32 offset );
void asm_fix_JGE_8( uint8 *c, int32 offset );
void asm_fix_JL_8( uint8 *c, int32 offset );
void asm_fix_JLE_8( uint8 *c, int32 offset );
void asm_fix_JNA_8( uint8 *c, int32 offset );
void asm_fix_JNAE_8( uint8 *c, int32 offset );
void asm_fix_JNB_8( uint8 *c, int32 offset );
void asm_fix_JNBE_8( uint8 *c, int32 offset );
void asm_fix_JNC_8( uint8 *c, int32 offset );
void asm_fix_JNE_8( uint8 *c, int32 offset );
void asm_fix_JNG_8( uint8 *c, int32 offset );
void asm_fix_JNGE_8( uint8 *c, int32 offset );
void asm_fix_JNL_8( uint8 *c, int32 offset );
void asm_fix_JNLE_8( uint8 *c, int32 offset );
void asm_fix_JNO_8( uint8 *c, int32 offset );
void asm_fix_JNP_8( uint8 *c, int32 offset );
void asm_fix_JNS_8( uint8 *c, int32 offset );
void asm_fix_JNZ_8( uint8 *c, int32 offset );
void asm_fix_JO_8( uint8 *c, int32 offset );
void asm_fix_JP_8( uint8 *c, int32 offset );
void asm_fix_JPE_8( uint8 *c, int32 offset );
void asm_fix_JPO_8( uint8 *c, int32 offset );
void asm_fix_JS_8( uint8 *c, int32 offset );
void asm_fix_JZ_8( uint8 *c, int32 offset );

void asm_fix_JA_32( uint8 *c, int32 offset );
void asm_fix_JAE_32( uint8 *c, int32 offset );
void asm_fix_JB_32( uint8 *c, int32 offset );
void asm_fix_JBE_32( uint8 *c, int32 offset );
void asm_fix_JC_32( uint8 *c, int32 offset );
void asm_fix_JE_32( uint8 *c, int32 offset );
void asm_fix_JG_32( uint8 *c, int32 offset );
void asm_fix_JGE_32( uint8 *c, int32 offset );
void asm_fix_JL_32( uint8 *c, int32 offset );
void asm_fix_JLE_32( uint8 *c, int32 offset );
void asm_fix_JNA_32( uint8 *c, int32 offset );
void asm_fix_JNAE_32( uint8 *c, int32 offset );
void asm_fix_JNB_32( uint8 *c, int32 offset );
void asm_fix_JNBE_32( uint8 *c, int32 offset );
void asm_fix_JNC_32( uint8 *c, int32 offset );
void asm_fix_JNE_32( uint8 *c, int32 offset );
void asm_fix_JNG_32( uint8 *c, int32 offset );
void asm_fix_JNGE_32( uint8 *c, int32 offset );
void asm_fix_JNL_32( uint8 *c, int32 offset );
void asm_fix_JNLE_32( uint8 *c, int32 offset );
void asm_fix_JNO_32( uint8 *c, int32 offset );
void asm_fix_JNP_32( uint8 *c, int32 offset );
void asm_fix_JNS_32( uint8 *c, int32 offset );
void asm_fix_JNZ_32( uint8 *c, int32 offset );
void asm_fix_JO_32( uint8 *c, int32 offset );
void asm_fix_JP_32( uint8 *c, int32 offset );
void asm_fix_JPE_32( uint8 *c, int32 offset );
void asm_fix_JPO_32( uint8 *c, int32 offset );
void asm_fix_JS_32( uint8 *c, int32 offset );
void asm_fix_JZ_32( uint8 *c, int32 offset );

void asm_fix_JMP_8( uint8 *c, int32 offset );
void asm_fix_JMP_32( uint8 *c, int32 offset );

void asm_LEA( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_MOV_RI( asmContext *c, int32 reg, int32 value );
void asm_MOV_MI( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset, int32 data );
void asm_MOV_RR( asmContext *c, int32 regD, int32 regS );
void asm_MOV_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_MOV_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_MOVSX_32_8_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_MOVSX_32_16_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_MOVZX_32_8_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_MOVZX_32_16_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_MUL_AR( asmContext *c, int32 reg );
void asm_MUL_AM( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_NEG_R( asmContext *c, int32 reg );
void asm_NEG_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_NOP( asmContext *c );

void asm_NOT_R( asmContext *c, int32 reg );
void asm_NOT_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_OR_RI( asmContext *c, int32 reg, int32 value );
void asm_OR_RR( asmContext *c, int32 regD, int32 regS );
void asm_OR_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_OR_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_POP( asmContext *c, int32 reg );

void asm_PUSH_R( asmContext *c, int32 reg );
void asm_PUSH_I( asmContext *c, int32 reg, int32 value );

void asm_RET( asmContext *c );

void asm_SAHF( asmContext *c );

void asm_SHL_R1( asmContext *c, int32 reg );
void asm_SHL_RC( asmContext *c, int32 reg );
void asm_SHL_RI( asmContext *c, int32 reg, int32 value );

void asm_SHR_R1( asmContext *c, int32 reg );
void asm_SHR_RC( asmContext *c, int32 reg );
void asm_SHR_RI( asmContext *c, int32 reg, int32 value );

void asm_SBB_RI( asmContext *c, int32 reg, int32 value );
void asm_SBB_RR( asmContext *c, int32 regD, int32 regS );
void asm_SBB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_SBB_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_SUB_RI( asmContext *c, int32 reg, int32 value );
void asm_SUB_RR( asmContext *c, int32 regD, int32 regS );
void asm_SUB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_SUB_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_XOR_RI( asmContext *c, int32 reg, int32 value );
void asm_XOR_RR( asmContext *c, int32 regD, int32 regS );
void asm_XOR_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_XOR_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );


// X87 Floating point stuff

void asm_F2XM1( asmContext *c );

void asm_FABS( asmContext *c );

void asm_FADD_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FADD_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FADD_RR( asmContext *c, int32 regD, int32 regS );
void asm_FADDP_R( asmContext *c, int32 reg );

void asm_FCHS( asmContext *c );

void asm_FCMOVB( asmContext *c, int32 reg );
void asm_FCMOVE( asmContext *c, int32 reg );
void asm_FCMOVBE( asmContext *c, int32 reg );
void asm_FCMOVU( asmContext *c, int32 reg );
void asm_FCMOVNB( asmContext *c, int32 reg );
void asm_FCMOVNE( asmContext *c, int32 reg );
void asm_FCMOVNBE( asmContext *c, int32 reg );
void asm_FCMOVNU( asmContext *c, int32 reg );

void asm_FCOM_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FCOM_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FCOM_R( asmContext *c, int32 reg );
void asm_FCOMP_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FCOMP_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FCOMP_R( asmContext *c, int32 reg );
void asm_FCOMPP( asmContext *c );

void asm_FCOMI( asmContext *c, int32 reg );
void asm_FCOMIP( asmContext *c, int32 reg );
void asm_FUCOMI( asmContext *c, int32 reg );
void asm_FUCOMIP( asmContext *c, int32 reg );

void asm_FCOS( asmContext *c );

void asm_FDIV_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FDIV_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FDIV_RR( asmContext *c, int32 regD, int32 regS );
void asm_FDIVP_R( asmContext *c, int32 reg );

void asm_FDIVR_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FDIVR_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FDIVR_RR( asmContext *c, int32 regD, int32 regS );
void asm_FDIVRP_R( asmContext *c, int32 reg );

void asm_FILD_16( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FILD_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FILD_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_FIST_16( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FIST_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_FISTP_16( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FISTP_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FISTP_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_FLD_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FLD_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FLD_80( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FLD_R( asmContext *c, int32 reg );

void asm_FLD1( asmContext *c );
void asm_FLDL2T( asmContext *c );
void asm_FLDL2E( asmContext *c );
void asm_FLDPI( asmContext *c );
void asm_FLDLG2( asmContext *c );
void asm_FLDLN2( asmContext *c );
void asm_FLDZ( asmContext *c );

void asm_FMUL_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FMUL_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FMUL_RR( asmContext *c, int32 regD, int32 regS );
void asm_FMULP_R( asmContext *c, int32 reg );

void asm_FNSTSW_A( asmContext *c );

void asm_FPATAN( asmContext *c );

void asm_FPREM( asmContext *c );
void asm_FPREM1( asmContext *c );

void asm_FPTAN( asmContext *c );

void asm_FRNDINT( asmContext *c );

void asm_FSIN( asmContext *c );

void asm_FSINCOS( asmContext *c );

void asm_FSQRT( asmContext *c );

void asm_FST_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FST_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FST_R( asmContext *c, int32 reg );

void asm_FSTP_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FSTP_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FSTP_80( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FSTP_R( asmContext *c, int32 reg );

void asm_FSUB_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FSUB_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FSUB_RR( asmContext *c, int32 regD, int32 regS );
void asm_FSUBP_R( asmContext *c, int32 reg );

void asm_FSUBR_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FSUBR_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_FSUBR_RR( asmContext *c, int32 regD, int32 regS );
void asm_FSUBRP_R( asmContext *c, int32 reg );

void asm_FTST( asmContext *c );

void asm_FUCOM_R( asmContext *c, int32 reg );
void asm_FUCOMP_R( asmContext *c, int32 reg );
void asm_FUCOMPP( asmContext *c );

void asm_FXAM( asmContext *c );

void asm_FXCH( asmContext *c, int32 reg );

void asm_FXTRACT( asmContext *c );

void asm_FYL2X( asmContext *c );

void asm_FYL2XP1( asmContext *c );


// MMX stuff
void asm_MOVQ_RR( asmContext *c, int32 regD, int32 regS );
void asm_MOVQ_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_MOVQ_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_MOVD_RR( asmContext *c, int32 regD, int32 regS );
void asm_MOVD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_MOVD_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PACKSSWB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PACKSSWB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PACKSSDW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PACKSSDW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PACKUSWB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PACKUSWB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PADDB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PADDB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PADDW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PADDW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PADDD_RR( asmContext *c, int32 regD, int32 regS );
void asm_PADDD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PADDSB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PADDSB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PADDSW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PADDSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PADDUSB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PADDUSB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PADDUSW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PADDUSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PAND_RR( asmContext *c, int32 regD, int32 regS );
void asm_PAND_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PANDN_RR( asmContext *c, int32 regD, int32 regS );
void asm_PANDN_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PAVGB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PAVGB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PAVGW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PAVGW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PCMPEQB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PCMPEQB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PCMPEQW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PCMPEQW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PCMPEQD_RR( asmContext *c, int32 regD, int32 regS );
void asm_PCMPEQD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PCMPGTB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PCMPGTB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PCMPGTW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PCMPGTW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PCMPGTD_RR( asmContext *c, int32 regD, int32 regS );
void asm_PCMPGTD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PEXTRW_RRI( asmContext *c, int32 regD, int32 regS, uint8 immed );

void asm_PINSRW_RRI( asmContext *c, int32 regD, int32 regS, uint8 immed );

void asm_PMADDWD_RR( asmContext *c, int32 regD, int32 regS );
void asm_PMADDWD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PMAXSW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PMAXSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PMAXUB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PMAXUB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PMINSW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PMINSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PMINUB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PMINUB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PMOVMSKB_RR( asmContext *c, int32 regD, int32 regS );

void asm_PMULHUW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PMULHUW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PMULHW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PMULHW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PMULLW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PMULLW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_POR_RR( asmContext *c, int32 regD, int32 regS );
void asm_POR_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PREFETCH0_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PREFETCH1_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PREFETCH2_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PREFETCHNTA_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
 
void asm_PSADBW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSADBW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PSHUFW_RRI( asmContext *c, int32 regD, int32 regS, uint8 immed );
void asm_PSHUFW_RMI( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset, uint8 immed );

void asm_PSLLW_RI( asmContext *c, int32 regD, int8 count );
void asm_PSLLW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSLLW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PSLLD_RI( asmContext *c, int32 regD, int8 count );
void asm_PSLLD_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSLLD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PSLLQ_RI( asmContext *c, int32 regD, int8 count );
void asm_PSLLQ_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSLLQ_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PSRAW_RI( asmContext *c, int32 regD, int8 count );
void asm_PSRAW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSRAW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PSRAD_RI( asmContext *c, int32 regD, int8 count );
void asm_PSRAD_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSRAD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PSRLW_RI( asmContext *c, int32 regD, int8 count );
void asm_PSRLW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSRLW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PSRLD_RI( asmContext *c, int32 regD, int8 count );
void asm_PSRLD_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSRLD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PSRLQ_RI( asmContext *c, int32 regD, int8 count );
void asm_PSRLQ_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSRLQ_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PSUBB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSUBB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PSUBW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSUBW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PSUBD_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSUBD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PSUBSB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSUBSB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PSUBSW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSUBSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PSUBUSB_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSUBUSB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PSUBUSW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PSUBUSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PUNPCKHBW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PUNPCKHBW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PUNPCKHWD_RR( asmContext *c, int32 regD, int32 regS );
void asm_PUNPCKHWD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PUNPCKHDQ_RR( asmContext *c, int32 regD, int32 regS );
void asm_PUNPCKHDQ_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PUNPCKLBW_RR( asmContext *c, int32 regD, int32 regS );
void asm_PUNPCKLBW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PUNPCKLWD_RR( asmContext *c, int32 regD, int32 regS );
void asm_PUNPCKLWD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );
void asm_PUNPCKLDQ_RR( asmContext *c, int32 regD, int32 regS );
void asm_PUNPCKLDQ_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_PXOR_RR( asmContext *c, int32 regD, int32 regS );
void asm_PXOR_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset );

void asm_EMMS( asmContext *c );







#ifdef __cplusplus
}
#endif

// KNI stuff

#endif

