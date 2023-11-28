/********************************************************************
*                      x86ctl
*            --------------------------
* x86ct.h:
*    Header file for the X86 CTL users.
*    
*
*********************************************************************/
#ifndef _X86CT_H
#define _X86CT_H

/**************/
/** tbl type **/
/**************/
/* Table info for instrucion */
typedef enum
{
    NO_TBL=0,
    R,    /* Regular - use Op1 Op2 and Xop and size to choose table */

    Only, /* Only inst. - size is 16 or 32, but use both size entries */

    I8,   /* OP2+i - used by some inst. , Reg(i) or ST(i) are operands in OP2,
             so use all needed entries in OP2 table (0<=i<=7) */

    F24,   /* XOP will be in OP2 table in 24 entries only 
             for float instruction that has both OP2 and XOP from same
             OP1 byte (e.g D8 /0 == fadd and D8 C0 == fadd
          */
	T1,                            /* VX instructions */
	T21,T22, 
	T41, T42, T43, T44 
} tbl_type;

/**************/
/** Operands **/
/**************/
typedef enum
{
    X86CT_NO_OPER=0,
    OAL, /* AL */
    OAX, /* AX */
    OEAX,/* XEAX*/
    OCL, /* CL */
    ODX, /* DX */
    ODS,
    OES,
    OSS,
    OFS,
    OGS,
    OCS,
    RM8,
    RM16,
    RM32,
    RM64,
    IMM8,
    IMM16,
    IMM32,
    R8,
    R16,
    R32,
    IMMS8, /* Sign extended IMM8 */
    CONST_3, /* constant 3*/
    CONST_1, /* constant 1*/
    SREG,    /* Sreg */
    CR,    /* Control reg */  
    DR,    /* Debug reg */
    REL8,
    REL16,
    REL32,
    OFF8,
    OFF16,
    OFF32,
    PORT8,
    ST0,    /* ST(0) */
    STI,    /* ST(i) */
    M8,M16,M32,MA,OFF,RM80,M14B,M28B,M32RL,M64RL,M32INT,M16INT,M94B,M108B, 
    mm,
#ifdef VX_TOOLS
	xmm,
	M128,
	M160B,
#endif
    iM32, iRM32, M64, M80,
    AI_AL, AI_AX, AI_EAX, AI_DX, AI_CL, 
    MODX, REL8lp
} X86CT_Operand_type;

typedef  X86CT_Operand_type opr1_type;
typedef  X86CT_Operand_type opr2_type;
typedef  X86CT_Operand_type opr3_type;

/*****************/
/** encode type **/
/*****************/
typedef enum
{
    X86CT_NO_ENCODE=0,
    NONE, /* opcode only     */
    IMAB, /* AL   <- IMM8    */
    IMAW, /* AX   <- IMM16   */
    IMAL, /* EAX  <- IMM32   */
    IPAL, /* IMM8 port  <- AL  */
    IPAX, /* IMM8 port  <- AX  */
    IPEAX,/* IMM8 port  <- EAX */
    ALIP, /* AL <- IMM8 port */
    AXIP, /* AX <- IMM8 port */
    EAXIP,/* EAX <-IMM8 port */
    DXAL, /* DX port <- AL */
    DXAX, /* DX port <- AX */
    DXEAX,/* DX port <- EAX */
    ALDX, /* AL  <- DX port */
    AXDX, /* AX  <- DX port */
    EAXDX,/* EAX <- DX port */
    DX,  /* ES:(E)DI <- DX port  &  DX port <- ES:(E)DI */
    AXR16,/* AX <- R16 */
    EAXR32,/* EAX <- R32 */
    AXO,   /* AX only */
    W_AXO, /* AX only, wait prefix */
    IMB,  /* RM   <- IMM8    */
    IMW,  /* RM   <- IMM16   */
    IML,  /* RM   <- IMM32   */
    IMSB, /* RM  <- IMMS8    */
    RMR8 ,/* RM8  <- R8      */
    RMR16,/* RM16 <- R16     */
    RMR32,/* RM32 <- R32     */
    RRM8, /* R8   <- RM8     */
    RRM16,/* R16  <- RM16    */
    RRM32,/* R32  <- RM32    */
    SHFT_1, /* RM <- 1       */
    SHFT_CL,/* RM <- CL      */
    RM,     /* RM only       */
    W_RM,   /* RM only, wait prefix */
    RM_O16, /* RM only , only 16 bit */
    ALRM,   /* AL <- RM      */
    AXRM,   /* AX <- RM      */
    EAXRM,   /*EAX <- RM      */
    RRM16I8S,/*R16<-RM16*IMM8S*/
    RRM32I8S,/*R32<-RM32*IMM8S*/
    RRM16I  ,/*R16<-RM16*IMM16*/
    RRM32I  ,/*R32<-RM32*IMM32*/
    RMR16I8 ,/*RM16<-R16,IMM8*/
    RMR32I8 ,/*RM32<-R32,IMM8*/
    RMR16CL ,/*RM16<-R16,CL*/
    RMR32CL ,/*RM32<-R32,CL*/
    R16_OP1, /*R16 in opcode byte */
    R32_OP1, /*R32 in opcode byte */
    R8_OP1IM,  /*R8  in opcode byte ,IMM8 */
    R16_OP1IM, /*R16 in opcode byte,IMM16 */
    R32_OP1IM, /*R32 in opcode byte,IMM32 */
    IMOB,    /* IMM8 only */
    IMOW,    /* IMM16 only */
    IMOL,    /* IMM32 only */
    RL8,    /* REL8 only */
    RL16,    /* REL16 only */
    RL32,    /* REL32 only */
    SEG_OF16,/* seg:off16   */
    SEG_OF32,/* seg:off32   */
    RRM16_M, /* R16 <- RM16 (Memory ONLY!!!) */
    RRM32_M, /* R16 <- RM32 (Memory ONLY!!!) */
    RM8_M,   /* RM8  (Memory ONLY!!!) */
    RM16_M,  /* RM16 (Memory ONLY!!!) */
    RM32_M,  /* RM32 (Memory ONLY!!!) */
    RM64_M,  /* RM64 (Memory ONLY!!!) */
    RM80_M,  /* RM80 (Memory ONLY!!!) */
    IM16_8,  /* IMM16 + IMM8 */
    CONST1,  /* Imm 1 */
    CONST3,  /* Imm 3 */
    RMS16,   /* RM16 <- Sreg */
    RMS32,   /* RM32 <- Sreg */
    SRM16,   /* Sreg <- RM16 */
    RCR,     /* R32  <- Creg */
    CRR,     /* Creg <- R32  */
    RDR,     /* R32  <- Dreg */
    DRR,     /* Dreg <- R32  */
    ALOFF,   /* AL <- OFFSET */
    EAXOFF,  /* EAX <- OFFSET */
    OFFAL,   /* OFFSET <- AL */
    OFFEAX,  /* OFFSET <- EAX */
    R16_RMB, /* R16 <- RM8 */
    R32_RMB, /* R32 <- RMB */
    R32_RMW, /* R32 < RM16 */
    RRM16_R, /* R16 <- RM16 (Reg only) */
    RRM32_R, /* R32 <- RM32 (Reg only) */
    RDS,     /* Reg DS */
    RES,     /* Reg ES */
    RSS,     /* Reg SS */
    RFS,     /* Reg FS */
    RGS,     /* Reg GS */
    RCS,     /* Reg CS */
    STI_ST,  /* ST(i) <- ST */
    ST_STI,  /* ST <- ST(i) */
    STIO,    /* ST(i) */ 
    mmRM32,  /* mm <- RM32  */
    mmRM64,  /* mm <- RM64  */ 
    mmI8,    /* mm <- IMM8  */
    RM32mm,  /* RM32 <- mm  */
    RM64mm,   /* RM64 <- mm  */
#ifdef VX_TOOLS
	xmmRM128,  /* mm <- RM128 */
	RM128xmm,  /* RM128 <- mm */
    M128xmm,   /* M128 <- mm */
	xmmM32_xmm,
	xmmM64_xmm,
	R32_xmmM32,
	R32_xmmM64,
	xmmRM64,
	xmm_xmmM64,
	RM64xmm,
	xmmRM32,
	xmm_xmmM32,
	RM32xmm,
	R32xmm,
	xmmRM16,
	RM16xmm,
	xmmM128,
	xmmxmm,
	xmmI8,
	M160Byte,
    RRM128I8S,      /*R128 <- RM128, IMM8S*/
	R32_xmm_I8,
	xmm_M32_I8,
#endif
	OPMRM,
	M
} X86CT_Encode_type;


typedef unsigned char X86CT_byte;

/* X86CT flags definitions and macros */

#define X86CT_FLAG_8086        0x00000001   /* 0 - inst valid in 8086 mode */
#define X86CT_FLAG_V86         0x00000002   /* 1 - inst valid in  V86 mode */
#define X86CT_FLAG_P5          0x00000004   /* 2 - inst valid on P5 */
#define X86CT_FLAG_P6          0x00000008   /* 3 - inst valid on P6 */
#define X86CT_FLAG_P5MM        0x40000000   /* 30 - inst valid on SIMD P5MM */
#define X86CT_FLAG_P6MM        0x80000000   /* 31 - inst valid on SIMD P6MM */
#define X86CT_FLAG_P7          0x00000010   /* 4 - inst valid on P7 */
#define X86CT_FLAG_PRIVILIGE   0x00000060   /* 5 & 6 - int privilige level */
#define X86CT_FLAG_PRIV_POSITION 5 
#define X86CT_FLAG_LOCK        0x00000080   /* 7 - inst can use lock prefix*/
#define X86CT_FLAG_OPER_ERR    0x00000100   /* 8 - inst invalid with certain
                                               opetrans */
#define X86CT_FLAG_IMPLIED_OPR 0x00000200   /* 9 - inst has implied operands*/
#define X86CT_FLAG_TYPE        0x00003C00   /* 10,11,12,13 - inst type
                                               (jmp,call,alu..)*/
#define X86CT_FLAG_TYPE_JMP      0x00002000
#define X86CT_FLAG_TYPE_COND_JMP 0x00001000

#define X86CT_FLAG_TYPE_ALU      0x00000800
#define X86CT_FLAG_TYPE_FLOAT    0x00000400
#define X86CT_FLAG_TYPE_MM       0x00000000
#define X86CT_FLAG_TYPE_SYS      0x00000C00
#define X86CT_FLAG_INSTRUCTION   0x00004000   /* 14- instruction (Not alias) */
#define X86CT_FLAG_W_NEED_PREFIX 0x00008000   /* 15- 16bit needs size-prefix */
#define X86CT_FLAG_L_NEED_PREFIX 0x00010000   /* 16- 32bit needs size-prefix */
#define X86CT_FLAG_STOP_TRANS    0x00020000   /* Gambit - stop translation   */
#define X86CT_FLAG_IAS_VALID     0x00800000   /* 23- producable by ias       */
#define X86CT_FLAG_OPRNDS_ORDER  0x0f000000   /* 24-27 operands order   */

#define X86CT_FLAG_NO_OPRNDS     0x00000000   /* 0    No operands       */

#define X86CT_FLAG_1SRC1         0x01000000   /* 8    1st - src1        */
#define X86CT_FLAG_1DST          0x02000000   /* 4    1st - dst         */
#define X86CT_FLAG_1DST_SRC      0x03000000   /* C    1st - src1 & dst  */

#define X86CT_FLAG_1SRC1_2SRC2   0x04000000   /* 2    1st - src1        */
                                              /*      2nd - src2        */
#define X86CT_FLAG_1DST_2SRC1    0x05000000   /* A    1st - dest        */
                                              /*      2nd - src1        */
#define X86CT_FLAG_1DST_SRC1_2SRC2 \
                                 0x06000000   /* 6    1st - src1 & dst  */
                                              /*      2nd - src2        */
#define X86CT_FLAG_1DST_2SRC1_3SRC2 \
                                 0x07000000   /* E    1st - dest        */
                                              /*      2nd - src1        */
                                              /*      3nd - src2        */
#define X86CT_FLAG_1DST_SRC1_2SRC2_3SRC3  \
                                 0x08000000   /* 1    1st - src1 & dst  */
                                              /*      2nd - src2        */
                                              /*      3nd - src3        */
#define X86CT_FLAG_1DST1_SRC1_2DST2_SRC2 \
                                 0x09000000   /* 9    1st - dst1 &src1  */
                                              /*      2nd - dst2 &src2  */

#define X86CT_FLAG_ENCODER       0x10000000   /* 28- producable by encoder   */

#define X86CT_FLAG_OPRNDS_PRINT_RVRS \
                                 0x20000000   /* 29   Operands print    */
                                              /*      order is reverse  */


/*** Extended Flags (ext_flags member) ***/

#define X86CT_FLAG_TYPE_COND_INSTR 0x00000001 /* conditional instruction */

/***********  macros for obtaining some characteristics of instruction ****/
/**********   TRUE if true and FALSE otherwise ***************************/
#define VALID_8086(it)      ((it)->flags & X86CT_FLAG_8086)
#define VALID_V86(it)       ((it)->flags & X86CT_FLAG_V86)
#define VALID_P5(it)        ((it)->flags & X86CT_FLAG_P5)
#define VALID_P6(it)        ((it)->flags & X86CT_FLAG_P6)
#define VALID_P7(it)        ((it)->flags & X86CT_FLAG_P7)
#define VALID_P5MM(it)      ((it)->flags & X86CT_FLAG_P5MM)
#define VALID_P6MM(it)      ((it)->flags & X86CT_FLAG_P6MM)
#define PRIVILIGE(it)       ((it)->flags & X86CT_FLAG_PRIVILIGE)\
                             >> X86CT_FLAG_PRIV_POSITION)
#define VALID_LOCK(it)      ((it)->flags & X86CT_FLAG_LOCK)
#define OPER_ERR(it)        ((it)->flags & X86CT_FLAG_OPER_ERR)
#define IMPLIED_OPER(it)    ((it)->flags & X86CT_FLAG_IMPLIED_OPR)
#define INST_FLOW(it)       ((it)->flags & (X86CT_FLAG_TYPE_JMP | \
                                            X86CT_FLAG_TYPE_COND_JMP))
#define INST_COND_JUMP(it)  ((it)->flags & X86CT_FLAG_TYPE_COND_JMP)
#define INST_JUMP(it)       ((it)->flags & X86CT_FLAG_TYPE_JMP)
#define STOP_TRANS(it)      ((it)->flags & X86CT_FLAG_STOP_TRANS)
  
#define INST_TYPE(it)       ((it)->flags &  X86CT_FLAG_TYPE)
#define INST_ALU(it)        (INST_TYPE(it) == X86CT_FLAG_TYPE_ALU)
#define INST_FLOAT(it)      (INST_TYPE(it) == X86CT_FLAG_TYPE_FLOAT)
#define INST_SYS(it)        (INST_TYPE(it) == X86CT_FLAG_TYPE_SYS)
#define INST_MM(it)         (INST_TYPE(it) == X86CT_FLAG_TYPE_MM)
  
#define INSTRUCTION(it)     ((it)->flags &  X86CT_FLAG_INSTRUCTION)
#define IAS_INSTRUCTION(it) ((it)->flags &  X86CT_FLAG_IAS_VALID)

#define INST_W_NEEDS_PREFIX(it) ((it)->flags & X86CT_FLAG_W_NEED_PREFIX)
#define INST_L_NEEDS_PREFIX(it) ((it)->flags & X86CT_FLAG_L_NEED_PREFIX) 
#define INST_ENCODER(it)    ((it)->flags & X86CT_FLAG_ENCODER) 
#define OPERAND_IS_DEST(it) ((it)->flags & X86CT_FLAG_1DST)

/*** operand order is reversed ***/
#define SECOND_OPERAND_IS_DEST(it) (((it)->flags & X86CT_FLAG_1DST_2SRC1)||        \
                              ((it)->flags & X86CT_FLAG_1DST_SRC1_2SRC2)||     \
                              ((it)->flags & X86CT_FLAG_1DST1_SRC1_2DST2_SRC2))

#define FIRST_OPERAND_IS_DEST(it) ((it)->flags & X86CT_FLAG_1DST1_SRC1_2DST2_SRC2)

#define INST_COND_INSTR(it) ((it)->ext_flags & X86CT_FLAG_TYPE_COND_INSTR)

#endif










