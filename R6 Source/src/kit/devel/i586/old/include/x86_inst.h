	X86_AAA,						/**  opcode:  37        **/
	X86_AAD,						/**  opcode:  D5        **/
	X86_AAM,						/**  opcode:  D4        **/
	X86_AAS,						/**  opcode:  3F        **/
	X86_ADCB_MI_IMB,				/**  opcode:  80 /2     **/
	X86_ADCB_MI_ALIAS,				/**  opcode:  82 /2     **/
	X86_ADCB_RI_AL,					/**  opcode:  14        **/
	X86_ADCW_RI_AX,					/**  opcode:  15        **/
	X86_ADCL_RI_EAX,				/**  opcode:  15        **/
	X86_ADCW_MI,					/**  opcode:  81 /2     **/
	X86_ADCL_MI,					/**  opcode:  81 /2     **/
	X86_ADCW_MI_B,					/**  opcode:  83 /2     **/
	X86_ADCL_MI_B,					/**  opcode:  83 /2     **/
	X86_ADCB_MR,					/**  opcode:  10        **/
	X86_ADCW_MR_RMR16,				/**  opcode:  11        **/
	X86_ADCL_MR,					/**  opcode:  11        **/
	X86_ADCB_RM,					/**  opcode:  12        **/
	X86_ADCW_RM_RRM16,				/**  opcode:  13        **/
	X86_ADCL_RM,					/**  opcode:  13        **/
	X86_ADDB_RI_AL,					/**  opcode:  04        **/
	X86_ADDW_RI_AX,					/**  opcode:  05        **/
	X86_ADDL_RI_EAX,				/**  opcode:  05        **/
	X86_ADDB_MI_IMB,				/**  opcode:  80 /0     **/
	X86_ADDB_MI_ALIAS,				/**  opcode:  82 /0     **/
	X86_ADDW_MI,					/**  opcode:  81 /0     **/
	X86_ADDL_MI,					/**  opcode:  81 /0     **/
	X86_ADDW_MI_B,					/**  opcode:  83 /0     **/
	X86_ADDL_MI_B,					/**  opcode:  83 /0     **/
	X86_ADDB_MR,					/**  opcode:  00        **/
	X86_ADDW_MR_RMR16,				/**  opcode:  01        **/
	X86_ADDL_MR,					/**  opcode:  01        **/
	X86_ADDB_RM,					/**  opcode:  02        **/
	X86_ADDW_RM_RRM16,				/**  opcode:  03        **/
	X86_ADDL_RM,					/**  opcode:  03        **/
	X86_ANDB_RI_AL,					/**  opcode:  24        **/
	X86_ANDW_RI_AX,					/**  opcode:  25        **/
	X86_ANDL_RI_EAX,				/**  opcode:  25        **/
	X86_ANDB_MI_IMB,				/**  opcode:  80 /4     **/
	X86_ANDB_MI_ALIAS,				/**  opcode:  82 /4     **/
	X86_ANDW_MI,					/**  opcode:  81 /4     **/
	X86_ANDL_MI,					/**  opcode:  81 /4     **/
	X86_ANDW_MI_B,					/**  opcode:  83 /4     **/
	X86_ANDL_MI_B,					/**  opcode:  83 /4     **/
	X86_ANDB_MR,					/**  opcode:  20        **/
	X86_ANDW_MR_RMR16,				/**  opcode:  21        **/
	X86_ANDL_MR,					/**  opcode:  21        **/
	X86_ANDB_RM,					/**  opcode:  22        **/
	X86_ANDW_RM_RRM16,				/**  opcode:  23        **/
	X86_ANDL_RM,					/**  opcode:  23        **/
	X86_ARPL_MR_RMR16,				/**  opcode:  63        **/
	X86_BOUNDW_RM,					/**  opcode:  62        **/
	X86_BOUNDL_RM,					/**  opcode:  62        **/
	X86_BSFW_RM_RRM16,				/**  opcode:  0F BC     **/
	X86_BSFL_RM,					/**  opcode:  0F BC     **/
	X86_BSRW_RM_RRM16,				/**  opcode:  0F BD     **/
	X86_BSRL_RM,					/**  opcode:  0F BD     **/
	X86_BSWAP_R_R32_OP1,			/**  opcode:  0F C8     **/
	X86_BTW_MR_RMR16,				/**  opcode:  0F A3     **/
	X86_BTL_MR,						/**  opcode:  0F A3     **/
	X86_BTW_MI_IMB,					/**  opcode:  0F BA /4  **/
	X86_BTL_MI_IMB,					/**  opcode:  0F BA /4  **/
	X86_BTCW_MR_RMR16,				/**  opcode:  0F BB     **/
	X86_BTCL_MR,					/**  opcode:  0F BB     **/
	X86_BTCW_MI_IMB,				/**  opcode:  0F BA /7  **/
	X86_BTCL_MI_IMB,				/**  opcode:  0F BA /7  **/
	X86_BTRW_MR_RMR16,				/**  opcode:  0F B3     **/
	X86_BTRL_MR,					/**  opcode:  0F B3     **/
	X86_BTRW_MI_IMB,				/**  opcode:  0F BA /6  **/
	X86_BTRL_MI_IMB,				/**  opcode:  0F BA /6  **/
	X86_BTSW_MR_RMR16,				/**  opcode:  0F AB     **/
	X86_BTSL_MR,					/**  opcode:  0F AB     **/
	X86_BTSW_MI_IMB,				/**  opcode:  0F BA /5  **/
	X86_BTSL_MI_IMB,				/**  opcode:  0F BA /5  **/
	X86_CALLFW_M,					/**  opcode:  FF /3     **/
	X86_CALLFL_M,					/**  opcode:  FF /3     **/
	X86_CALLNW_M_RM,				/**  opcode:  FF /2     **/
	X86_CALLNL_M_RM,				/**  opcode:  FF /2     **/
	X86_CALLNW,						/**  opcode:  E8        **/
	X86_CALLNL,						/**  opcode:  E8        **/
	X86_CALLFW,						/**  opcode:  9A        **/
	X86_CALLFL,						/**  opcode:  9A        **/
	X86_CBW,						/**  opcode:  98        **/
	X86_CWDE,						/**  opcode:  98        **/
	X86_CFLSH,						/**  opcode:  0F 0A     **/
	X86_CLC,						/**  opcode:  F8        **/
	X86_CLD,						/**  opcode:  FC        **/
	X86_CLI,						/**  opcode:  FA        **/
	X86_CLTS,						/**  opcode:  0F 06     **/
	X86_CMC,						/**  opcode:  F5        **/
	X86_CMOVAW,						/**  opcode:  0F 47     **/
	X86_CMOVAEW,					/**  opcode:  0F 43     **/
	X86_CMOVBW,						/**  opcode:  0F 42     **/
	X86_CMOVBEW,					/**  opcode:  0F 46     **/
	X86_CMOVEW,						/**  opcode:  0F 44     **/
	X86_CMOVGW,						/**  opcode:  0F 4F     **/
	X86_CMOVGEW,					/**  opcode:  0F 4D     **/
	X86_CMOVLW,						/**  opcode:  0F 4C     **/
	X86_CMOVLEW,					/**  opcode:  0F 4E     **/
	X86_CMOVNEW,					/**  opcode:  0F 45     **/
	X86_CMOVNOW,					/**  opcode:  0F 41     **/
	X86_CMOVNPW,					/**  opcode:  0F 4B     **/
	X86_CMOVNSW,					/**  opcode:  0F 49     **/
	X86_CMOVOW,						/**  opcode:  0F 40     **/
	X86_CMOVAW_W,					/**  opcode:  0F 4A     **/
	X86_CMOVSW,						/**  opcode:  0F 48     **/
	X86_CMOVAL,						/**  opcode:  0F 47     **/
	X86_CMOVAEL,					/**  opcode:  0F 43     **/
	X86_CMOVBL,						/**  opcode:  0F 42     **/
	X86_CMOVBEL,					/**  opcode:  0F 46     **/
	X86_CMOVEL,						/**  opcode:  0F 44     **/
	X86_CMOVGL,						/**  opcode:  0F 4F     **/
	X86_CMOVGEL,					/**  opcode:  0F 4D     **/
	X86_CMOVLL,						/**  opcode:  0F 4C     **/
	X86_CMOVLEL,					/**  opcode:  0F 4E     **/
	X86_CMOVNEL,					/**  opcode:  0F 45     **/
	X86_CMOVNOL,					/**  opcode:  0F 41     **/
	X86_CMOVNPL,					/**  opcode:  0F 4B     **/
	X86_CMOVNSL,					/**  opcode:  0F 49     **/
	X86_CMOVOL,						/**  opcode:  0F 40     **/
	X86_CMOVAL_L,					/**  opcode:  0F 4A     **/
	X86_CMOVSL,						/**  opcode:  0F 48     **/
	X86_CMPB_RI_AL,					/**  opcode:  3C        **/
	X86_CMPW_RI_AX,					/**  opcode:  3D        **/
	X86_CMPL_RI_EAX,				/**  opcode:  3D        **/
	X86_CMPB_MI_IMB,				/**  opcode:  80 /7     **/
	X86_CMPB_MI_ALIAS,				/**  opcode:  82 /7     **/
	X86_CMPW_MI,					/**  opcode:  81 /7     **/
	X86_CMPL_MI,					/**  opcode:  81 /7     **/
	X86_CMPW_MI_B,					/**  opcode:  83 /7     **/
	X86_CMPL_MI_B,					/**  opcode:  83 /7     **/
	X86_CMPB_MR,					/**  opcode:  38        **/
	X86_CMPW_MR_RMR16,				/**  opcode:  39        **/
	X86_CMPL_MR,					/**  opcode:  39        **/
	X86_CMPB_RM,					/**  opcode:  3A        **/
	X86_CMPW_RM_RRM16,				/**  opcode:  3B        **/
	X86_CMPL_RM,					/**  opcode:  3B        **/
	X86_CMPSB,						/**  opcode:  A6        **/
	X86_CMPSW,						/**  opcode:  A7        **/
	X86_CMPSL,						/**  opcode:  A7        **/
	X86_CMPXCHGB_MR,				/**  opcode:  0F B0     **/
	X86_CMPXCHGW_MR_RMR16,			/**  opcode:  0F B1     **/
	X86_CMPXCHGL_MR,				/**  opcode:  0F B1     **/
	X86_CMPXCHG8B,					/**  opcode:  0f C7 /1  **/
	X86_CPUID,						/**  opcode:  0F A2     **/
	X86_CWD,						/**  opcode:  99        **/
	X86_CDQ,						/**  opcode:  99        **/
	X86_DAA,						/**  opcode:  27        **/
	X86_DAS,						/**  opcode:  2F        **/
	X86_DECB_M_RM,					/**  opcode:  FE /1     **/
	X86_DECW_M_RM,					/**  opcode:  FF /1     **/
	X86_DECL_M_RM,					/**  opcode:  FF /1     **/
	X86_DECW_M_R16_OP1,				/**  opcode:  48        **/
	X86_DECL_M_R32_OP1,				/**  opcode:  48        **/
	X86_DIVB_RM_AL,					/**  opcode:  F6 /6     **/
	X86_DIVW_RM_AX,					/**  opcode:  F7 /6     **/
	X86_DIVL_RM_EAX,				/**  opcode:  F7 /6     **/
	X86_ENTER,						/**  opcode:  C8        **/
	X86_HLT,						/**  opcode:  F4        **/
	X86_IDIVB_RM_AL,				/**  opcode:  F6 /7     **/
	X86_IDIVW_RM_AX,				/**  opcode:  F7 /7     **/
	X86_IDIVL_RM_EAX,				/**  opcode:  F7 /7     **/
	X86_IMULB_M_RM,					/**  opcode:  F6 /5     **/
	X86_IMULW_M_RM,					/**  opcode:  F7 /5     **/
	X86_IMULL_M_RM,					/**  opcode:  F7 /5     **/
	X86_IMULW_RM_RRM16,				/**  opcode:  0F AF     **/
	X86_IMULL_RM,					/**  opcode:  0F AF     **/
	X86_IMULW_RMI_RRM16I8S,			/**  opcode:  6B        **/
	X86_IMULL_RMI_RRM32I8S,			/**  opcode:  6B        **/
	X86_IMULW_RMI_RRM16I,			/**  opcode:  69        **/
	X86_IMULL_RMI,					/**  opcode:  69        **/
	X86_INB_I_AL,					/**  opcode:  E4        **/
	X86_INW_I_AX,					/**  opcode:  E5        **/
	X86_INL_I_EAX,					/**  opcode:  E5        **/
	X86_INB_R_AL,					/**  opcode:  EC        **/
	X86_INW_R_AX,					/**  opcode:  ED        **/
	X86_INL_R_EAX,					/**  opcode:  ED        **/
	X86_INCB_M_RM,					/**  opcode:  FE /0     **/
	X86_INCW_M_RM,					/**  opcode:  FF /0     **/
	X86_INCL_M_RM,					/**  opcode:  FF /0     **/
	X86_INCW_M_R16_OP1,				/**  opcode:  40        **/
	X86_INCL_M_R32_OP1,				/**  opcode:  40        **/
	X86_INSB,						/**  opcode:  6C        **/
	X86_INSW,						/**  opcode:  6D        **/
	X86_INSL,						/**  opcode:  6D        **/
	X86_INT1,						/**  opcode:  F1        **/
	X86_INT3,						/**  opcode:  CC        **/
	X86_INT,						/**  opcode:  CD        **/
	X86_INTO,						/**  opcode:  CE        **/
	X86_INVD,						/**  opcode:  0F 08     **/
	X86_INVLPG,						/**  opcode:  0F 01 /7  **/
	X86_IRET,						/**  opcode:  CF        **/
	X86_IRETD,						/**  opcode:  CF        **/
	X86_JA,							/**  opcode:  77        **/
	X86_JAE,						/**  opcode:  73        **/
	X86_JB,							/**  opcode:  72        **/
	X86_JBE,						/**  opcode:  76        **/
	X86_JCXZ,						/**  opcode:  E3        **/
	X86_JE,							/**  opcode:  74        **/
	X86_JG,							/**  opcode:  7F        **/
	X86_JGE,						/**  opcode:  7D        **/
	X86_JLE,						/**  opcode:  7E        **/
	X86_JNGE,						/**  opcode:  7C        **/
	X86_JNO,						/**  opcode:  71        **/
	X86_JNP,						/**  opcode:  7B        **/
	X86_JNS,						/**  opcode:  79        **/
	X86_JNZ,						/**  opcode:  75        **/
	X86_JO,							/**  opcode:  70        **/
	X86_JP,							/**  opcode:  7A        **/
	X86_JS,							/**  opcode:  78        **/
	X86_JAFW,						/**  opcode:  0F 87     **/
	X86_JAEFW,						/**  opcode:  0F 83     **/
	X86_JBFW,						/**  opcode:  0F 82     **/
	X86_JBEFW,						/**  opcode:  0F 86     **/
	X86_JEFW,						/**  opcode:  0F 84     **/
	X86_JGFW,						/**  opcode:  0F 8F     **/
	X86_JGEFW,						/**  opcode:  0F 8D     **/
	X86_JLEFW,						/**  opcode:  0F 8E     **/
	X86_JNGEFW,						/**  opcode:  0F 8C     **/
	X86_JNOFW,						/**  opcode:  0F 81     **/
	X86_JNPFW,						/**  opcode:  0F 8B     **/
	X86_JNSFW,						/**  opcode:  0F 89     **/
	X86_JNZFW,						/**  opcode:  0F 85     **/
	X86_JOFW,						/**  opcode:  0F 80     **/
	X86_JPFW,						/**  opcode:  0F 8A     **/
	X86_JSFW,						/**  opcode:  0F 88     **/
	X86_JAFL,						/**  opcode:  0F 87     **/
	X86_JAEFL,						/**  opcode:  0F 83     **/
	X86_JBFL,						/**  opcode:  0F 82     **/
	X86_JBEFL,						/**  opcode:  0F 86     **/
	X86_JEFL,						/**  opcode:  0F 84     **/
	X86_JGFL,						/**  opcode:  0F 8F     **/
	X86_JGEFL,						/**  opcode:  0F 8D     **/
	X86_JLEFL,						/**  opcode:  0F 8E     **/
	X86_JNGEFL,						/**  opcode:  0F 8C     **/
	X86_JNOFL,						/**  opcode:  0F 81     **/
	X86_JNPFL,						/**  opcode:  0F 8B     **/
	X86_JNSFL,						/**  opcode:  0F 89     **/
	X86_JNZFL,						/**  opcode:  0F 85     **/
	X86_JOFL,						/**  opcode:  0F 80     **/
	X86_JPFL,						/**  opcode:  0F 8A     **/
	X86_JSFL,						/**  opcode:  0F 88     **/
	X86_JMPB,						/**  opcode:  EB        **/
	X86_JMPW,						/**  opcode:  E9        **/
	X86_JMPL,						/**  opcode:  E9        **/
	X86_JMPWP,						/**  opcode:  EA        **/
	X86_JMPLP,						/**  opcode:  EA        **/
	X86_JMPW_M_RM,					/**  opcode:  FF /4     **/
	X86_JMPL_M_RM,					/**  opcode:  FF /4     **/
	X86_JMPWI,						/**  opcode:  FF /5     **/
	X86_JMPLI,						/**  opcode:  FF /5     **/
	X86_LAHF,						/**  opcode:  9F        **/
	X86_LARW_M_RRM16,				/**  opcode:  0F 02     **/
	X86_LARL_M,						/**  opcode:  0F 02     **/
	X86_LDSW,						/**  opcode:  C5        **/
	X86_LDSL,						/**  opcode:  C5        **/
	X86_LESW,						/**  opcode:  C4        **/
	X86_LESL,						/**  opcode:  C4        **/
	X86_LSSW,						/**  opcode:  0F B2     **/
	X86_LSSL,						/**  opcode:  0F B2     **/
	X86_LFSW,						/**  opcode:  0F B4     **/
	X86_LFSL,						/**  opcode:  0F B4     **/
	X86_LGSW,						/**  opcode:  0F B5     **/
	X86_LGSL,						/**  opcode:  0F B5     **/
	X86_LEAW,						/**  opcode:  8D        **/
	X86_LEAL,						/**  opcode:  8D        **/
	X86_LEAVEW,						/**  opcode:  C9        **/
	X86_LEAVEL,						/**  opcode:  C9        **/
	X86_LGDT,						/**  opcode:  0F 01 /2  **/
	X86_LIDT,						/**  opcode:  0F 01 /3  **/
	X86_LLDT_M,						/**  opcode:  0F 00 /2  **/
	X86_LMSW_M,						/**  opcode:  0F 01 /6  **/
	X86_LODSB,						/**  opcode:  AC        **/
	X86_LODSW,						/**  opcode:  AD        **/
	X86_LODSL,						/**  opcode:  AD        **/
	X86_LOOP,						/**  opcode:  E2        **/
	X86_LOOPZ,						/**  opcode:  E1        **/
	X86_LOOPNZ,						/**  opcode:  E0        **/
	X86_LSLW_RRM16,					/**  opcode:  0F 03     **/
	X86_LSLL,						/**  opcode:  0F 03     **/
	X86_LTRW_M,						/**  opcode:  0F 00 /3  **/
	X86_MOVB_RM_AL,					/**  opcode:  A0        **/
	X86_MOVW_RM_AX,					/**  opcode:  A1        **/
	X86_MOVL_RM_EAX,				/**  opcode:  A1        **/
	X86_MOVB_MR_AL,					/**  opcode:  A2        **/
	X86_MOVW_MR_AX,					/**  opcode:  A3        **/
	X86_MOVL_MR_EAX,				/**  opcode:  A3        **/
	X86_MOVB_MR,					/**  opcode:  88        **/
	X86_MOVW_MR_RMR16,				/**  opcode:  89        **/
	X86_MOVL_MR,					/**  opcode:  89        **/
	X86_MOVB_RM,					/**  opcode:  8A        **/
	X86_MOVW_RM_RRM16,				/**  opcode:  8B        **/
	X86_MOVL_RM,					/**  opcode:  8B        **/
	X86_MOVW_SM_RMS16,				/**  opcode:  8C        **/
	X86_MOVW_SM_RMS32,				/**  opcode:  8C        **/
	X86_MOVW_MS_SRM16,				/**  opcode:  8E        **/
	X86_MOVB_RI,					/**  opcode:  B0        **/
	X86_MOVW_RI,					/**  opcode:  B8        **/
	X86_MOVL_RI,					/**  opcode:  B8        **/
	X86_MOVB_MI_IMB,				/**  opcode:  C6 /0     **/
	X86_MOVW_MI,					/**  opcode:  C7 /0     **/
	X86_MOVL_MI,					/**  opcode:  C7 /0     **/
	X86_MOVL_CR,					/**  opcode:  0F 22     **/
	X86_MOVL_RC,					/**  opcode:  0F 20     **/
	X86_MOVL_DR,					/**  opcode:  0F 23     **/
	X86_MOVL_RD,					/**  opcode:  0F 21     **/
	X86_MOVSB,						/**  opcode:  A4        **/
	X86_MOVSW,						/**  opcode:  A5        **/
	X86_MOVSL,						/**  opcode:  A5        **/
	X86_MOVSXBW_M,					/**  opcode:  0F BE     **/
	X86_MOVSXBL_M,					/**  opcode:  0F BE     **/
	X86_MOVSXWW_M,					/**  opcode:  0F BF     **/
	X86_MOVSXWL_M,					/**  opcode:  0F BF     **/
	X86_MOVZXBW_M,					/**  opcode:  0F B6     **/
	X86_MOVZXBL_M,					/**  opcode:  0F B6     **/
	X86_MOVZXWW_M,					/**  opcode:  0F B7     **/
	X86_MOVZXWL_M,					/**  opcode:  0F B7     **/
	X86_MULB_RM_AL,					/**  opcode:  F6 /4     **/
	X86_MULW_RM_AX,					/**  opcode:  F7 /4     **/
	X86_MULL_RM_EAX,				/**  opcode:  F7 /4     **/
	X86_NEGB_M_RM,					/**  opcode:  F6 /3     **/
	X86_NEGW_M_RM,					/**  opcode:  F7 /3     **/
	X86_NEGL_M_RM,					/**  opcode:  F7 /3     **/
	X86_NOTB_M_RM,					/**  opcode:  F6 /2     **/
	X86_NOTW_M_RM,					/**  opcode:  F7 /2     **/
	X86_NOTL_M_RM,					/**  opcode:  F7 /2     **/
	X86_ORB_RI_AL,					/**  opcode:  0C        **/
	X86_ORW_RI_AX,					/**  opcode:  0D        **/
	X86_ORL_RI_EAX,					/**  opcode:  0D        **/
	X86_ORB_MI_IMB,					/**  opcode:  80 /1     **/
	X86_ORB_MI_ALIAS,				/**  opcode:  82 /1     **/
	X86_ORW_MI,						/**  opcode:  81 /1     **/
	X86_ORL_MI,						/**  opcode:  81 /1     **/
	X86_ORW_MI_B,					/**  opcode:  83 /1     **/
	X86_ORL_MI_B,					/**  opcode:  83 /1     **/
	X86_ORB_MR,						/**  opcode:  08        **/
	X86_ORW_MR_RMR16,				/**  opcode:  09        **/
	X86_ORL_MR,						/**  opcode:  09        **/
	X86_ORB_RM,						/**  opcode:  0A        **/
	X86_ORW_RM_RRM16,				/**  opcode:  0B        **/
	X86_ORL_RM,						/**  opcode:  0B        **/
	X86_OUTB_I_AL,					/**  opcode:  E6        **/
	X86_OUTW_I_AX,					/**  opcode:  E7        **/
	X86_OUTL_I_EAX,					/**  opcode:  E7        **/
	X86_OUTB_R_AL,					/**  opcode:  EE        **/
	X86_OUTW_R_AX,					/**  opcode:  EF        **/
	X86_OUTL_R_EAX,					/**  opcode:  EF        **/
	X86_OUTSB,						/**  opcode:  6E        **/
	X86_OUTSW,						/**  opcode:  6F        **/
	X86_OUTSL,						/**  opcode:  6F        **/
	X86_POPW_M_RM,					/**  opcode:  8F /0     **/
	X86_POPL_M_RM,					/**  opcode:  8F /0     **/
	X86_POPW_R_R16_OP1,				/**  opcode:  58        **/
	X86_POPL_R_R32_OP1,				/**  opcode:  58        **/
	X86_POPW_DS_RDS,				/**  opcode:  1F        **/
	X86_POPL_DS_RDS,				/**  opcode:  1F        **/
	X86_POPW_ES,					/**  opcode:  07        **/
	X86_POPL_ES,					/**  opcode:  07        **/
	X86_POPW_FS,					/**  opcode:  17        **/
	X86_POPL_FS,					/**  opcode:  17        **/
	X86_POPW_DS_RFS,				/**  opcode:  0F A1     **/
	X86_POPL_DS_RFS,				/**  opcode:  0F A1     **/
	X86_POPW_GS_RGS,				/**  opcode:  0F A9     **/
	X86_POPL_GS_RGS,				/**  opcode:  0F A9     **/
	X86_POPAW,						/**  opcode:  61        **/
	X86_POPAL,						/**  opcode:  61        **/
	X86_POPFW,						/**  opcode:  9D        **/
	X86_POPFL,						/**  opcode:  9D        **/
	X86_PUSHW_M_RM,					/**  opcode:  FF /6     **/
	X86_PUSHL_M_RM,					/**  opcode:  FF /6     **/
	X86_PUSHW_R_R16_OP1,			/**  opcode:  50        **/
	X86_PUSHL_R_R32_OP1,			/**  opcode:  50        **/
	X86_PUSHW_DS_RDS,				/**  opcode:  1E        **/
	X86_PUSHL_DS_RDS,				/**  opcode:  1E        **/
	X86_PUSHW_ES,					/**  opcode:  06        **/
	X86_PUSHL_ES,					/**  opcode:  06        **/
	X86_PUSHW_SS,					/**  opcode:  16        **/
	X86_PUSHL_SS,					/**  opcode:  16        **/
	X86_PUSHW_FS_RFS,				/**  opcode:  0F A0     **/
	X86_PUSHL_FS_RFS,				/**  opcode:  0F A0     **/
	X86_PUSHW_GS_RGS,				/**  opcode:  0F A8     **/
	X86_PUSHL_GS_RGS,				/**  opcode:  0F A8     **/
	X86_PUSHW_CS_RCS,				/**  opcode:  0E        **/
	X86_PUSHL_CS_RCS,				/**  opcode:  0E        **/
	X86_PUSHBW_I,					/**  opcode:  6A        **/
	X86_PUSHB_I,					/**  opcode:  6A        **/
	X86_PUSHW_I,					/**  opcode:  68        **/
	X86_PUSHL_I,					/**  opcode:  68        **/
	X86_PUSHAW,						/**  opcode:  60        **/
	X86_PUSHAL,						/**  opcode:  60        **/
	X86_PUSHFW,						/**  opcode:  9C        **/
	X86_PUSHFL,						/**  opcode:  9C        **/
	X86_RCLB_MI_SHFT_1,				/**  opcode:  D0 /2     **/
	X86_RCLB_MR,					/**  opcode:  D2 /2     **/
	X86_RCLB_MI_IMB,				/**  opcode:  C0 /2     **/
	X86_RCLW_MI_SHFT_1,				/**  opcode:  D1 /2     **/
	X86_RCLW_MR,					/**  opcode:  D3 /2     **/
	X86_RCLW_MI_IMB,				/**  opcode:  C1 /2     **/
	X86_RCLL_MI_SHFT_1,				/**  opcode:  D1 /2     **/
	X86_RCLL_MR,					/**  opcode:  D3 /2     **/
	X86_RCLL_MI_IMB,				/**  opcode:  C1 /2     **/
	X86_RCRB_MI_SHFT_1,				/**  opcode:  D0 /3     **/
	X86_RCRB_MR,					/**  opcode:  D2 /3     **/
	X86_RCRB_MI_IMB,				/**  opcode:  C0 /3     **/
	X86_RCRW_MI_SHFT_1,				/**  opcode:  D1 /3     **/
	X86_RCRW_MR,					/**  opcode:  D3 /3     **/
	X86_RCRW_MI_IMB,				/**  opcode:  C1 /3     **/
	X86_RCRL_MI_SHFT_1,				/**  opcode:  D1 /3     **/
	X86_RCRL_MR,					/**  opcode:  D3 /3     **/
	X86_RCRL_MI_IMB,				/**  opcode:  C1 /3     **/
	X86_ROLB_MI_SHFT_1,				/**  opcode:  D0 /0     **/
	X86_ROLB_MR,					/**  opcode:  D2 /0     **/
	X86_ROLB_MI_IMB,				/**  opcode:  C0 /0     **/
	X86_ROLW_MI_SHFT_1,				/**  opcode:  D1 /0     **/
	X86_ROLW_MR,					/**  opcode:  D3 /0     **/
	X86_ROLW_MI_IMB,				/**  opcode:  C1 /0     **/
	X86_ROLL_MI_SHFT_1,				/**  opcode:  D1 /0     **/
	X86_ROLL_MR,					/**  opcode:  D3 /0     **/
	X86_ROLL_MI_IMB,				/**  opcode:  C1 /0     **/
	X86_RORB_MI_SHFT_1,				/**  opcode:  D0 /1     **/
	X86_RORB_MR,					/**  opcode:  D2 /1     **/
	X86_RORB_MI_IMB,				/**  opcode:  C0 /1     **/
	X86_RORW_MI_SHFT_1,				/**  opcode:  D1 /1     **/
	X86_RORW_MR,					/**  opcode:  D3 /1     **/
	X86_RORW_MI_IMB,				/**  opcode:  C1 /1     **/
	X86_RORL_MI_SHFT_1,				/**  opcode:  D1 /1     **/
	X86_RORL_MR,					/**  opcode:  D3 /1     **/
	X86_RORL_MI_IMB,				/**  opcode:  C1 /1     **/
	X86_RDMSR,						/**  opcode:  0F 32     **/
	X86_RDPMC,						/**  opcode:  0F 33     **/
	X86_RDTSC,						/**  opcode:  0F 31     **/
	X86_RSM,						/**  opcode:  0F AA     **/
	X86_RETN,						/**  opcode:  C3        **/
	X86_RETF,						/**  opcode:  CB        **/
	X86_RETN_I,						/**  opcode:  C2        **/
	X86_RETF_I,						/**  opcode:  CA        **/
	X86_SAHF,						/**  opcode:  9E        **/
	X86_SARB_MI_SHFT_1,				/**  opcode:  D0 /7     **/
	X86_SARB_MR,					/**  opcode:  D2 /7     **/
	X86_SARB_MI_IMB,				/**  opcode:  C0 /7     **/
	X86_SARW_MI_SHFT_1,				/**  opcode:  D1 /7     **/
	X86_SARW_MR,					/**  opcode:  D3 /7     **/
	X86_SARW_MI_IMB,				/**  opcode:  C1 /7     **/
	X86_SARL_MI_SHFT_1,				/**  opcode:  D1 /7     **/
	X86_SARL_MR,					/**  opcode:  D3 /7     **/
	X86_SARL_MI_IMB,				/**  opcode:  C1 /7     **/
	X86_SHLB_MI_SHFT_1,				/**  opcode:  D0 /4     **/
	X86_SHLB_MR,					/**  opcode:  D2 /4     **/
	X86_SHLB_MI_IMB,				/**  opcode:  C0 /4     **/
	X86_SHLW_MI_SHFT_1,				/**  opcode:  D1 /4     **/
	X86_SHLW_MR,					/**  opcode:  D3 /4     **/
	X86_SHLW_MI_IMB,				/**  opcode:  C1 /4     **/
	X86_SHLL_MI_SHFT_1,				/**  opcode:  D1 /4     **/
	X86_SHLL_MR,					/**  opcode:  D3 /4     **/
	X86_SHLL_MI_IMB,				/**  opcode:  C1 /4     **/
	X86_SHLB_MI_1_ALIAS,			/**  opcode:  D0 /6     **/
	X86_SHLB_MR_ALIAS,				/**  opcode:  D2 /6     **/
	X86_SHLB_MI_I_ALIAS,			/**  opcode:  C0 /6     **/
	X86_SHLW_MI_1_ALIAS,			/**  opcode:  D1 /6     **/
	X86_SHLW_MR_ALIAS,				/**  opcode:  D3 /6     **/
	X86_SHLW_MI_I_ALIAS,			/**  opcode:  C1 /6     **/
	X86_SHLL_MI_1_ALIAS,			/**  opcode:  D1 /6     **/
	X86_SHLL_MR_ALIAS,				/**  opcode:  D3 /6     **/
	X86_SHLL_MI_I_ALIAS,			/**  opcode:  C1 /6     **/
	X86_SHRB_MI_SHFT_1,				/**  opcode:  D0 /5     **/
	X86_SHRB_MR,					/**  opcode:  D2 /5     **/
	X86_SHRB_MI_IMB,				/**  opcode:  C0 /5     **/
	X86_SHRW_MI_SHFT_1,				/**  opcode:  D1 /5     **/
	X86_SHRW_MR,					/**  opcode:  D3 /5     **/
	X86_SHRW_MI_IMB,				/**  opcode:  C1 /5     **/
	X86_SHRL_MI_SHFT_1,				/**  opcode:  D1 /5     **/
	X86_SHRL_MR,					/**  opcode:  D3 /5     **/
	X86_SHRL_MI_IMB,				/**  opcode:  C1 /5     **/
	X86_SBBB_RI_AL,					/**  opcode:  1C        **/
	X86_SBBW_RI_AX,					/**  opcode:  1D        **/
	X86_SBBL_RI_EAX,				/**  opcode:  1D        **/
	X86_SBBB_MI_IMB,				/**  opcode:  80 /3     **/
	X86_SBBB_MI_ALIAS,				/**  opcode:  82 /3     **/
	X86_SBBW_MI,					/**  opcode:  81 /3     **/
	X86_SBBL_MI,					/**  opcode:  81 /3     **/
	X86_SBBW_MI_B,					/**  opcode:  83 /3     **/
	X86_SBBL_MI_B,					/**  opcode:  83 /3     **/
	X86_SBBB_MR,					/**  opcode:  18        **/
	X86_SBBW_MR_RMR16,				/**  opcode:  19        **/
	X86_SBBL_MR,					/**  opcode:  19        **/
	X86_SBBB_RM,					/**  opcode:  1A        **/
	X86_SBBW_RM_RRM16,				/**  opcode:  1B        **/
	X86_SBBL_RM,					/**  opcode:  1B        **/
	X86_SCASB,						/**  opcode:  AE        **/
	X86_SCASW,						/**  opcode:  AF        **/
	X86_SCASL,						/**  opcode:  AF        **/
	X86_SALC,						/**  opcode:  D6        **/
	X86_SETA_M_RM,					/**  opcode:  0F 97     **/
	X86_SETAE_M_RM,					/**  opcode:  0F 93     **/
	X86_SETB_M_RM,					/**  opcode:  0F 92     **/
	X86_SETBE_M_RM,					/**  opcode:  0F 96     **/
	X86_SETE_M_RM,					/**  opcode:  0F 94     **/
	X86_SETG_M_RM,					/**  opcode:  0F 9F     **/
	X86_SETGE_M_RM,					/**  opcode:  0F 9D     **/
	X86_SETL_M_RM,					/**  opcode:  0F 9C     **/
	X86_SETLE_R_RM,					/**  opcode:  0F 9E     **/
	X86_SETNE_M_RM,					/**  opcode:  0F 95     **/
	X86_SETNO_M_RM,					/**  opcode:  0F 91     **/
	X86_SETNP_M_RM,					/**  opcode:  0F 9B     **/
	X86_SETNS_M_RM,					/**  opcode:  0F 99     **/
	X86_SETO_M_RM,					/**  opcode:  0F 90     **/
	X86_SETA_M_P,					/**  opcode:  0F 9A     **/
	X86_SETS_M_RM,					/**  opcode:  0F 98     **/
	X86_SGDT,						/**  opcode:  0F 01 /0  **/
	X86_SIDT,						/**  opcode:  0F 01 /1  **/
	X86_SHLDW_MI,					/**  opcode:  0F A4     **/
	X86_SHLDL_MI,					/**  opcode:  0F A4     **/
	X86_SHLDW_MR,					/**  opcode:  0F A5     **/
	X86_SHLDL_MR,					/**  opcode:  0F A5     **/
	X86_SHRDW_MI,					/**  opcode:  0F AC     **/
	X86_SHRDL_MI,					/**  opcode:  0F AC     **/
	X86_SHRDW_MR,					/**  opcode:  0F AD     **/
	X86_SHRDL_MR,					/**  opcode:  0F AD     **/
	X86_SLDTW_M,					/**  opcode:  0F 00 /0  **/
	X86_SLDTL_M,					/**  opcode:  0F 00 /0  **/
	X86_SMSWW_M,					/**  opcode:  0F 01 /4  **/
	X86_SMSWL_M,					/**  opcode:  0F 01 /4  **/
	X86_STC,						/**  opcode:  F9        **/
	X86_STD,						/**  opcode:  FD        **/
	X86_STI,						/**  opcode:  FB        **/
	X86_STOSB,						/**  opcode:  AA        **/
	X86_STOSW,						/**  opcode:  AB        **/
	X86_STOSL,						/**  opcode:  AB        **/
	X86_STRW_M,						/**  opcode:  0F 00 /1  **/
	X86_STRL_M,						/**  opcode:  0F 00 /1  **/
	X86_SUBB_RI_AL,					/**  opcode:  2C        **/
	X86_SUBW_RI_AX,					/**  opcode:  2D        **/
	X86_SUBL_RI_EAX,				/**  opcode:  2D        **/
	X86_SUBB_MI_IMB,				/**  opcode:  80 /5     **/
	X86_SUBB_MI_ALIAS,				/**  opcode:  82 /5     **/
	X86_SUBW_MI,					/**  opcode:  81 /5     **/
	X86_SUBL_MI,					/**  opcode:  81 /5     **/
	X86_SUBW_MI_B,					/**  opcode:  83 /5     **/
	X86_SUBL_MI_B,					/**  opcode:  83 /5     **/
	X86_SUBB_MR,					/**  opcode:  28        **/
	X86_SUBW_MR_RMR16,				/**  opcode:  29        **/
	X86_SUBL_MR,					/**  opcode:  29        **/
	X86_SUBB_RM,					/**  opcode:  2A        **/
	X86_SUBW_RM_RRM16,				/**  opcode:  2B        **/
	X86_SUBL_RM,					/**  opcode:  2B        **/
	X86_TESTB_RI_AL,				/**  opcode:  A8        **/
	X86_TESTW_RI_AX,				/**  opcode:  A9        **/
	X86_TESTL_RI_EAX,				/**  opcode:  A9        **/
	X86_TESTB_MI_IMB,				/**  opcode:  F6 /0     **/
	X86_TESTB_MI_ALIAS,				/**  opcode:  F6 /1     **/
	X86_TESTW_MI,					/**  opcode:  F7 /0     **/
	X86_TESTW_MI_ALIAS,				/**  opcode:  F7 /1     **/
	X86_TESTL_MI,					/**  opcode:  F7 /0     **/
	X86_TESTL_MI_ALIAS,				/**  opcode:  F7 /1     **/
	X86_TESTB_MR,					/**  opcode:  84        **/
	X86_TESTW_MR_RMR16,				/**  opcode:  85        **/
	X86_TESTL_MR,					/**  opcode:  85        **/
	X86_VERR_M,						/**  opcode:  0F 00 /4  **/
	X86_VERW_M,						/**  opcode:  0F 00 /5  **/
	X86_WAIT,						/**  opcode:  9B        **/
	X86_WBINVD,						/**  opcode:  0F 09     **/
	X86_WRMSR,						/**  opcode:  0F 30     **/
	X86_XADDB_MR,					/**  opcode:  0F C0     **/
	X86_XADDW_MR_RMR16,				/**  opcode:  0F C1     **/
	X86_XADDL_MR,					/**  opcode:  0F C1     **/
	X86_XCHGW_RR_AX,				/**  opcode:  90        **/
	X86_XCHGL_RR_EAX,				/**  opcode:  90        **/
	X86_XCHGB_RM,					/**  opcode:  86        **/
	X86_XCHGW_RM_RRM16,				/**  opcode:  87        **/
	X86_XCHGL_RM,					/**  opcode:  87        **/
	X86_XLATB,						/**  opcode:  D7        **/
	X86_XORB_RI_AL,					/**  opcode:  34        **/
	X86_XORW_RI_AX,					/**  opcode:  35        **/
	X86_XORL_RI_EAX,				/**  opcode:  35        **/
	X86_XORB_MI_IMB,				/**  opcode:  80 /6     **/
	X86_XORB_MI_ALIAS,				/**  opcode:  82 /6     **/
	X86_XORW_MI,					/**  opcode:  81 /6     **/
	X86_XORL_MI,					/**  opcode:  81 /6     **/
	X86_XORW_MI_B,					/**  opcode:  83 /6     **/
	X86_XORL_MI_B,					/**  opcode:  83 /6     **/
	X86_XORB_MR,					/**  opcode:  30        **/
	X86_XORW_MR_RMR16,				/**  opcode:  31        **/
	X86_XORL_MR,					/**  opcode:  31        **/
	X86_XORB_RM,					/**  opcode:  32        **/
	X86_XORW_RM_RRM16,				/**  opcode:  33        **/
	X86_XORL_RM,					/**  opcode:  33        **/
	X86_F2XM1,						/**  opcode:  D9 F0     **/
	X86_FABS,						/**  opcode:  D9 E1     **/
	X86_FADDS_M,					/**  opcode:  D8 /0     **/
	X86_FADDL_M,					/**  opcode:  DC /0     **/
	X86_FADD_0I,					/**  opcode:  D8 C0     **/
	X86_FADD_I0,					/**  opcode:  DC C0     **/
	X86_FADDP_I0,					/**  opcode:  DE C0     **/
	X86_FIADDL_M,					/**  opcode:  DA /0     **/
	X86_FIADDS_M,					/**  opcode:  DE /0     **/
	X86_FBLD,						/**  opcode:  DF /4     **/
	X86_FBSTP,						/**  opcode:  DF /6     **/
	X86_FCHS,						/**  opcode:  D9 E0     **/
	X86_FNCLEX,						/**  opcode:  DB E2     **/
	X86_FCOMS_M,					/**  opcode:  D8 /2     **/
	X86_FCOML_M,					/**  opcode:  DC /2     **/
	X86_FCOM_0I,					/**  opcode:  D8 D0     **/
	X86_FCOM_0I_ALIAS,				/**  opcode:  DC D0     **/
	X86_FCOMPS_M,					/**  opcode:  D8 /3     **/
	X86_FCOMPL_M,					/**  opcode:  DC /3     **/
	X86_FCOMP_0I,					/**  opcode:  D8 D8     **/
	X86_FCOMP_0I_ALIAS1,			/**  opcode:  DC D8     **/
	X86_FCOMP_0I_ALIAS2,			/**  opcode:  DE D0     **/
	X86_FCOMPP,						/**  opcode:  DE D9     **/
	X86_FCOMI_0I,					/**  opcode:  DB F0     **/
	X86_FCOMIP_0I,					/**  opcode:  DF F0     **/
	X86_FCOS,						/**  opcode:  D9 FF     **/
	X86_FDECSTP,					/**  opcode:  D9 F6     **/
	X86_FCMOVB,						/**  opcode:  DA C0     **/
	X86_FCMOVE,						/**  opcode:  DA C8     **/
	X86_FCMOVBE,					/**  opcode:  DA D0     **/
	X86_FCMOVU,						/**  opcode:  DA D8     **/
	X86_FCMOVNB,					/**  opcode:  DB C0     **/
	X86_FCMOVNE,					/**  opcode:  DB C8     **/
	X86_FCMOVNBE,					/**  opcode:  DB D0     **/
	X86_FCMOVNU,					/**  opcode:  DB D8     **/
	X86_FDIVS_M,					/**  opcode:  D8 /6     **/
	X86_FDIVL_M,					/**  opcode:  DC /6     **/
	X86_FDIV_0I,					/**  opcode:  D8 F0     **/
	X86_FDIVR_I0,					/**  opcode:  DC F0     **/
	X86_FDIVRP_I0,					/**  opcode:  DE F0     **/
	X86_FIDIV_M,					/**  opcode:  DE /6     **/
	X86_FIDIVL_M,					/**  opcode:  DA /6     **/
	X86_FDISI,						/**  opcode:  DB E1     **/
	X86_FENI,						/**  opcode:  DB E0     **/
	X86_FDIVRS_M,					/**  opcode:  D8 /7     **/
	X86_FDIVRL_M,					/**  opcode:  DC /7     **/
	X86_FDIVR_0I,					/**  opcode:  D8 F8     **/
	X86_FDIV_I0,					/**  opcode:  DC F8     **/
	X86_FDIVP_I0,					/**  opcode:  DE F8     **/
	X86_FIDIVR_M,					/**  opcode:  DE /7     **/
	X86_FIDIVRL_M,					/**  opcode:  DA /7     **/
	X86_FFREE,						/**  opcode:  DD C0     **/
	X86_FFREEP,						/**  opcode:  DF C0     **/
	X86_FICOM,						/**  opcode:  DE /2     **/
	X86_FICOML,						/**  opcode:  DA /2     **/
	X86_FICOMP,						/**  opcode:  DE /3     **/
	X86_FICOMPL,					/**  opcode:  DA /3     **/
	X86_FILD,						/**  opcode:  DF /0     **/
	X86_FILDL,						/**  opcode:  DB /0     **/
	X86_FILDLL,						/**  opcode:  DF /5     **/
	X86_FINCSTP,					/**  opcode:  D9 F7     **/
	X86_FNINIT,						/**  opcode:  DB E3     **/
	X86_FIST,						/**  opcode:  DF /2     **/
	X86_FISTL,						/**  opcode:  DB /2     **/
	X86_FISTP,						/**  opcode:  DF /3     **/
	X86_FISTPL,						/**  opcode:  DB /3     **/
	X86_FISTPLL,					/**  opcode:  DF /7     **/
	X86_FLDS,						/**  opcode:  D9 /0     **/
	X86_FLDL,						/**  opcode:  DD /0     **/
	X86_FLDT,						/**  opcode:  DB /5     **/
	X86_FLD,						/**  opcode:  D9 C0     **/
	X86_FLD1,						/**  opcode:  D9 E8     **/
	X86_FLDL2T,						/**  opcode:  D9 E9     **/
	X86_FLDL2E,						/**  opcode:  D9 EA     **/
	X86_FLDPI,						/**  opcode:  D9 EB     **/
	X86_FLDLG2,						/**  opcode:  D9 EC     **/
	X86_FLDLN2,						/**  opcode:  D9 ED     **/
	X86_FLDZ,						/**  opcode:  D9 EE     **/
	X86_FLDCW_RM,					/**  opcode:  D9 /5     **/
	X86_FLDENV_W_M14B,				/**  opcode:  D9 /4     **/
	X86_FLDENV_L_M28B,				/**  opcode:  D9 /4     **/
	X86_FMULS_M,					/**  opcode:  D8 /1     **/
	X86_FMULL_M,					/**  opcode:  DC /1     **/
	X86_FMUL_0I,					/**  opcode:  D8 C8     **/
	X86_FMUL_I0,					/**  opcode:  DC C8     **/
	X86_FMULP_I0,					/**  opcode:  DE C8     **/
	X86_FIMULL_M,					/**  opcode:  DA /1     **/
	X86_FIMUL_M,					/**  opcode:  DE /1     **/
	X86_FNOP,						/**  opcode:  D9 D0     **/
	X86_FPATAN,						/**  opcode:  D9 F3     **/
	X86_FPREM,						/**  opcode:  D9 F8     **/
	X86_FPREM1,						/**  opcode:  D9 F5     **/
	X86_FPTAN,						/**  opcode:  D9 F2     **/
	X86_FRNDINT,					/**  opcode:  D9 FC     **/
	X86_FRSTOR_W_RM,				/**  opcode:  DD /4     **/
	X86_FRSTOR_L_RM,				/**  opcode:  DD /4     **/
	X86_FNSAVE_W_RM,				/**  opcode:  DD /6     **/
	X86_FNSAVE_L_RM,				/**  opcode:  DD /6     **/
	X86_FSETPM,						/**  opcode:  DB E4     **/
	X86_FSCALE,						/**  opcode:  D9 FD     **/
	X86_FSIN,						/**  opcode:  D9 FE     **/
	X86_FSINCOS,					/**  opcode:  D9 FB     **/
	X86_FSQRT,						/**  opcode:  D9 FA     **/
	X86_FSTS,						/**  opcode:  D9 /2     **/
	X86_FSTL,						/**  opcode:  DD /2     **/
	X86_FST,						/**  opcode:  DD D0     **/
	X86_FSTPS,						/**  opcode:  D9 /3     **/
	X86_FSTPL,						/**  opcode:  DD /3     **/
	X86_FSTPT,						/**  opcode:  DB /7     **/
	X86_FSTP,						/**  opcode:  DD D8     **/
	X86_FSTP_ALIAS1,				/**  opcode:  D9 D8     **/
	X86_FSTP_ALIAS2,				/**  opcode:  DF D0     **/
	X86_FSTP_ALIAS3,				/**  opcode:  DF D8     **/
	X86_FNSTCW_RM,					/**  opcode:  D9 /7     **/
	X86_FSTENV_W_M14B,				/**  opcode:  D9 /6     **/
	X86_FSTENV_L_M28B,				/**  opcode:  D9 /6     **/
	X86_FNSTSW_RM,					/**  opcode:  DD /7     **/
	X86_FNSTSW_A_16,				/**  opcode:  DF E0     **/
	X86_FSUBS_M,					/**  opcode:  D8 /4     **/
	X86_FSUBL_M,					/**  opcode:  DC /4     **/
	X86_FSUB_0I,					/**  opcode:  D8 E0     **/
	X86_FSUBR_I0,					/**  opcode:  DC E0     **/
	X86_FSUBRP_I0,					/**  opcode:  DE E0     **/
	X86_FISUBL_M,					/**  opcode:  DA /4     **/
	X86_FISUB_M,					/**  opcode:  DE /4     **/
	X86_FSUBRS_M,					/**  opcode:  D8 /5     **/
	X86_FSUBRL_M,					/**  opcode:  DC /5     **/
	X86_FSUBR_0I,					/**  opcode:  D8 E8     **/
	X86_FSUB_I0,					/**  opcode:  DC E8     **/
	X86_FSUBP_I0,					/**  opcode:  DE E8     **/
	X86_FISUBRL_M,					/**  opcode:  DA /5     **/
	X86_FISUBR_M,					/**  opcode:  DE /5     **/
	X86_FTST,						/**  opcode:  D9 E4     **/
	X86_FUCOM,						/**  opcode:  DD E0     **/
	X86_FUCOMP,						/**  opcode:  DD E8     **/
	X86_FUCOMPP,					/**  opcode:  DA E9     **/
	X86_FUCOMI,						/**  opcode:  DB E8     **/
	X86_FUCOMIP,					/**  opcode:  DF E8     **/
	X86_FXAM,						/**  opcode:  D9 E5     **/
	X86_FXCH,						/**  opcode:  D9 C8     **/
	X86_FXCH_ALIAS1,				/**  opcode:  DD C8     **/
	X86_FXCH_ALIAS2,				/**  opcode:  DF C8     **/
	X86_FXTRACT,					/**  opcode:  D9 F4     **/
	X86_FYL2X,						/**  opcode:  D9 F1     **/
	X86_FYL2XP1,					/**  opcode:  D9 F9     **/
	X86_ZALLOC,						/**  opcode:  0F C7 /2  **/
	X86_EMMS_MM,					/**  opcode:  0F 77     **/
	X86_MOVDL_MM,					/**  opcode:  0F 7E     **/
	X86_MOVDL_MRR_MM,				/**  opcode:  0F 6E     **/
	X86_MOVQ_RM_MM,					/**  opcode:  0F 6F     **/
	X86_MOVQ_MR_MM,					/**  opcode:  0F 7F     **/
	X86_PACKSSWB_MM,				/**  opcode:  0F 63     **/
	X86_PACKSSDW_MM,				/**  opcode:  0F 6B     **/
	X86_PACKUSWB_MM,				/**  opcode:  0F 67     **/
	X86_PADDB_MM,					/**  opcode:  0F FC     **/
	X86_PADDW_MM,					/**  opcode:  0F FD     **/
	X86_PADDD_MM,					/**  opcode:  0F FE     **/
	X86_PADDSB_MM,					/**  opcode:  0F EC     **/
	X86_PADDSW_MM,					/**  opcode:  0F ED     **/
	X86_PADDUSB_MM,					/**  opcode:  0F DC     **/
	X86_PADDUSW_MM,					/**  opcode:  0F DD     **/
	X86_PAND_MM,					/**  opcode:  0F DB     **/
	X86_PANDN_MM,					/**  opcode:  0F DF     **/
	X86_PCMPEQB_MM,					/**  opcode:  0F 74     **/
	X86_PCMPEQW_MM,					/**  opcode:  0F 75     **/
	X86_PCMPEQD_MM,					/**  opcode:  0F 76     **/
	X86_PCMPGTB_MM,					/**  opcode:  0F 64     **/
	X86_PCMPGTW_MM,					/**  opcode:  0F 65     **/
	X86_PCMPGTD_MM,					/**  opcode:  0F 66     **/
	X86_PMADDWD_MM,					/**  opcode:  0F F5     **/
	X86_PMULHW_MM,					/**  opcode:  0F E5     **/
	X86_PMULLW_MM,					/**  opcode:  0F D5     **/
	X86_POR_MM,						/**  opcode:  0F EB     **/
	X86_PSLLW_MM,					/**  opcode:  0F F1     **/
	X86_PSLLW_I_MM,					/**  opcode:  0F 71 /6  **/
	X86_PSLLD_MM,					/**  opcode:  0F F2     **/
	X86_PSLLD_I_MM,					/**  opcode:  0F 72 /6  **/
	X86_PSLLQ_MM,					/**  opcode:  0F F3     **/
	X86_PSLLQ_I_MM,					/**  opcode:  0F 73 /6  **/
	X86_PSRAW_MM,					/**  opcode:  0F E1     **/
	X86_PSRAW_I_MM,					/**  opcode:  0F 71 /4  **/
	X86_PSRAD_MM,					/**  opcode:  0F E2     **/
	X86_PSRAD_I_MM,					/**  opcode:  0F 72 /4  **/
	X86_PSRLW__MM,					/**  opcode:  0F D1     **/
	X86_PSRLW_I_MM,					/**  opcode:  0F 71 /2  **/
	X86_PSRLD_MM,					/**  opcode:  0F D2     **/
	X86_PSRLD_I_MM,					/**  opcode:  0F 72 /2  **/
	X86_PSRLQ_MM,					/**  opcode:  0F D3     **/
	X86_PSRLQ_I_MM,					/**  opcode:  0F 73 /2  **/
	X86_PSUBB_MM,					/**  opcode:  0F F8     **/
	X86_PSUBW_MM,					/**  opcode:  0F F9     **/
	X86_PSUBD_MM,					/**  opcode:  0F FA     **/
	X86_PSUBSB_MM,					/**  opcode:  0F E8     **/
	X86_PSUBSW_MM,					/**  opcode:  0F E9     **/
	X86_PSUBUSB_MM,					/**  opcode:  0F D8     **/
	X86_PSUBUSW_MM,					/**  opcode:  0F D9     **/
	X86_PUNPCKLBW_MM,				/**  opcode:  0F 60     **/
	X86_PUNPCKLWD_MM,				/**  opcode:  0F 61     **/
	X86_PUNPCKLDQ_MM,				/**  opcode:  0F 62     **/
	X86_PUNPCKHBW_MM,				/**  opcode:  0F 68     **/
	X86_PUNPCKHWD_MM,				/**  opcode:  0F 69     **/
	X86_PUNPCKHDQ_MM,				/**  opcode:  0F 6A     **/
	X86_PXOR_MM,					/**  opcode:  0F EF     **/
