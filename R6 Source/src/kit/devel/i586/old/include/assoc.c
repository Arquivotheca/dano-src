/***************************************/
/*****	 X86 INSTRUCTION INFO	*****/
/***************************************/
static Associate_Info  ASSOCIATE_INST_INFO[]={
/*0*/  {AAA,X86_AAA}
/*2*/ ,{AAD,X86_AAD}
/*4*/ ,{AAM,X86_AAM}
/*5*/ ,{AAS,X86_AAS}
/*6*/ ,{ADCB,X86_ADCB_MI_IMB}
/*7*/ ,{ADCB,X86_ADCB_MI_ALIAS}
/*8*/ ,{ADCB,X86_ADCB_RI_AL}
/*9*/ ,{ADCW,X86_ADCW_RI_AX}
/*10*/ ,{ADCL,X86_ADCL_RI_EAX}
/*12*/ ,{ADCW,X86_ADCW_MI}
/*13*/ ,{ADCL,X86_ADCL_MI}
/*15*/ ,{ADCW,X86_ADCW_MI_B}
/*16*/ ,{ADCL,X86_ADCL_MI_B}
/*18*/ ,{ADCB,X86_ADCB_MR}
/*19*/ ,{ADCW,X86_ADCW_MR_RMR16}
/*20*/ ,{ADCL,X86_ADCL_MR}
/*22*/ ,{ADCB,X86_ADCB_RM}
/*23*/ ,{ADCW,X86_ADCW_RM_RRM16}
/*24*/ ,{ADCL,X86_ADCL_RM}
/*26*/ ,{ADDB,X86_ADDB_RI_AL}
/*27*/ ,{ADDW,X86_ADDW_RI_AX}
/*28*/ ,{ADDL,X86_ADDL_RI_EAX}
/*30*/ ,{ADDB,X86_ADDB_MI_IMB}
/*31*/ ,{ADDB,X86_ADDB_MI_ALIAS}
/*32*/ ,{ADDW,X86_ADDW_MI}
/*33*/ ,{ADDL,X86_ADDL_MI}
/*35*/ ,{ADDW,X86_ADDW_MI_B}
/*36*/ ,{ADDL,X86_ADDL_MI_B}
/*38*/ ,{ADDB,X86_ADDB_MR}
/*39*/ ,{ADDW,X86_ADDW_MR_RMR16}
/*40*/ ,{ADDL,X86_ADDL_MR}
/*42*/ ,{ADDB,X86_ADDB_RM}
/*43*/ ,{ADDW,X86_ADDW_RM_RRM16}
/*44*/ ,{ADDL,X86_ADDL_RM}
/*46*/ ,{ANDB,X86_ANDB_RI_AL}
/*47*/ ,{ANDW,X86_ANDW_RI_AX}
/*48*/ ,{ANDL,X86_ANDL_RI_EAX}
/*50*/ ,{ANDB,X86_ANDB_MI_IMB}
/*51*/ ,{ANDB,X86_ANDB_MI_ALIAS}
/*52*/ ,{ANDW,X86_ANDW_MI}
/*53*/ ,{ANDL,X86_ANDL_MI}
/*55*/ ,{ANDW,X86_ANDW_MI_B}
/*56*/ ,{ANDL,X86_ANDL_MI_B}
/*58*/ ,{ANDB,X86_ANDB_MR}
/*59*/ ,{ANDW,X86_ANDW_MR_RMR16}
/*60*/ ,{ANDL,X86_ANDL_MR}
/*62*/ ,{ANDB,X86_ANDB_RM}
/*63*/ ,{ANDW,X86_ANDW_RM_RRM16}
/*64*/ ,{ANDL,X86_ANDL_RM}
/*66*/ ,{ARPL,X86_ARPL_MR_RMR16}
/*67*/ ,{BOUNDW,X86_BOUNDW_RM}
/*68*/ ,{BOUNDL,X86_BOUNDL_RM}
/*70*/ ,{BSFW,X86_BSFW_RM_RRM16}
/*71*/ ,{BSFL,X86_BSFL_RM}
/*73*/ ,{BSRW,X86_BSRW_RM_RRM16}
/*74*/ ,{BSRL,X86_BSRL_RM}
/*76*/ ,{BSWAP,X86_BSWAP_R_R32_OP1}
/*77*/ ,{BTW,X86_BTW_MR_RMR16}
/*78*/ ,{BTL,X86_BTL_MR}
/*80*/ ,{BTW,X86_BTW_MI_IMB}
/*81*/ ,{BTL,X86_BTL_MI_IMB}
/*83*/ ,{BTCW,X86_BTCW_MR_RMR16}
/*84*/ ,{BTCL,X86_BTCL_MR}
/*86*/ ,{BTCW,X86_BTCW_MI_IMB}
/*87*/ ,{BTCL,X86_BTCL_MI_IMB}
/*89*/ ,{BTRW,X86_BTRW_MR_RMR16}
/*90*/ ,{BTRL,X86_BTRL_MR}
/*92*/ ,{BTRW,X86_BTRW_MI_IMB}
/*93*/ ,{BTRL,X86_BTRL_MI_IMB}
/*95*/ ,{BTSW,X86_BTSW_MR_RMR16}
/*96*/ ,{BTSL,X86_BTSL_MR}
/*98*/ ,{BTSW,X86_BTSW_MI_IMB}
/*99*/ ,{BTSL,X86_BTSL_MI_IMB}
/*101*/ ,{LCALL,X86_CALLFW_M}
/*102*/ ,{LCALL,X86_CALLFL_M}
/*104*/ ,{CALL,X86_CALLNW_M_RM}
/*105*/ ,{CALL,X86_CALLNL_M_RM}
/*106*/ ,{CALL,X86_CALLNW}
/*107*/ ,{CALL,X86_CALLNL}
/*108*/ ,{LCALL,X86_CALLFW}
/*109*/ ,{LCALL,X86_CALLFL}
/*110*/ ,{CBTW,X86_CBW}
/*111*/ ,{CWTL,X86_CWDE}
/*113*/ ,{CFLSH,X86_CFLSH}
/*114*/ ,{CLC,X86_CLC}
/*115*/ ,{CLD,X86_CLD}
/*116*/ ,{CLI,X86_CLI}
/*117*/ ,{CLTS,X86_CLTS}
/*118*/ ,{CMC,X86_CMC}
/*119*/ ,{CMOVAW,X86_CMOVAW}
/*120*/ ,{CMOVAEW,X86_CMOVAEW}
/*121*/ ,{CMOVBW,X86_CMOVBW}
/*122*/ ,{CMOVBEW,X86_CMOVBEW}
/*124*/ ,{CMOVEW,X86_CMOVEW}
/*125*/ ,{CMOVGW,X86_CMOVGW}
/*126*/ ,{CMOVGEW,X86_CMOVGEW}
/*127*/ ,{CMOVLW,X86_CMOVLW}
/*128*/ ,{CMOVLEW,X86_CMOVLEW}
/*134*/ ,{CMOVNEW,X86_CMOVNEW}
/*139*/ ,{CMOVNOW,X86_CMOVNOW}
/*140*/ ,{CMOVNPW,X86_CMOVNPW}
/*141*/ ,{CMOVNSW,X86_CMOVNSW}
/*143*/ ,{CMOVOW,X86_CMOVOW}
/*144*/ ,{CMOVPW,X86_CMOVAW_W}
/*147*/ ,{CMOVSW,X86_CMOVSW}
/*149*/ ,{CMOVAL,X86_CMOVAL}
/*150*/ ,{CMOVAEL,X86_CMOVAEL}
/*151*/ ,{CMOVBL,X86_CMOVBL}
/*152*/ ,{CMOVBEL,X86_CMOVBEL}
/*154*/ ,{CMOVEL,X86_CMOVEL}
/*155*/ ,{CMOVGL,X86_CMOVGL}
/*156*/ ,{CMOVGEL,X86_CMOVGEL}
/*157*/ ,{CMOVLL,X86_CMOVLL}
/*158*/ ,{CMOVLEL,X86_CMOVLEL}
/*164*/ ,{CMOVNEL,X86_CMOVNEL}
/*169*/ ,{CMOVNOL,X86_CMOVNOL}
/*170*/ ,{CMOVNPL,X86_CMOVNPL}
/*171*/ ,{CMOVNSL,X86_CMOVNSL}
/*173*/ ,{CMOVOL,X86_CMOVOL}
/*174*/ ,{CMOVPL,X86_CMOVAL_L}
/*177*/ ,{CMOVSL,X86_CMOVSL}
/*209*/ ,{CMPB,X86_CMPB_RI_AL}
/*210*/ ,{CMPW,X86_CMPW_RI_AX}
/*211*/ ,{CMPL,X86_CMPL_RI_EAX}
/*213*/ ,{CMPB,X86_CMPB_MI_IMB}
/*214*/ ,{CMPB,X86_CMPB_MI_ALIAS}
/*215*/ ,{CMPW,X86_CMPW_MI}
/*216*/ ,{CMPL,X86_CMPL_MI}
/*218*/ ,{CMPW,X86_CMPW_MI_B}
/*219*/ ,{CMPL,X86_CMPL_MI_B}
/*221*/ ,{CMPB,X86_CMPB_MR}
/*222*/ ,{CMPW,X86_CMPW_MR_RMR16}
/*223*/ ,{CMPL,X86_CMPL_MR}
/*225*/ ,{CMPB,X86_CMPB_RM}
/*226*/ ,{CMPW,X86_CMPW_RM_RRM16}
/*227*/ ,{CMPL,X86_CMPL_RM}
/*229*/ ,{CMPSB,X86_CMPSB}
/*232*/ ,{CMPSW,X86_CMPSW}
/*235*/ ,{CMPSL,X86_CMPSL}
/*239*/ ,{CMPXCHGB,X86_CMPXCHGB_MR}
/*240*/ ,{CMPXCHGW,X86_CMPXCHGW_MR_RMR16}
/*241*/ ,{CMPXCHGL,X86_CMPXCHGL_MR}
/*243*/ ,{CMPXCHG8B,X86_CMPXCHG8B}
/*244*/ ,{CPUID,X86_CPUID}
/*245*/ ,{CWTD,X86_CWD}
/*246*/ ,{CLTD,X86_CDQ}
/*247*/ ,{DAA,X86_DAA}
/*248*/ ,{DAS,X86_DAS}
/*249*/ ,{DECB,X86_DECB_M_RM}
/*250*/ ,{DECW,X86_DECW_M_RM}
/*251*/ ,{DECL,X86_DECL_M_RM}
/*253*/ ,{DECW,X86_DECW_M_R16_OP1}
/*254*/ ,{DECL,X86_DECL_M_R32_OP1}
/*256*/ ,{DIVB,X86_DIVB_RM_AL}
/*257*/ ,{DIVW,X86_DIVW_RM_AX}
/*258*/ ,{DIVL,X86_DIVL_RM_EAX}
/*260*/ ,{ENTER,X86_ENTER}
/*261*/ ,{HLT,X86_HLT}
/*262*/ ,{IDIVB,X86_IDIVB_RM_AL}
/*263*/ ,{IDIVW,X86_IDIVW_RM_AX}
/*264*/ ,{IDIVL,X86_IDIVL_RM_EAX}
/*266*/ ,{IMULB,X86_IMULB_M_RM}
/*267*/ ,{IMULW,X86_IMULW_M_RM}
/*268*/ ,{IMULL,X86_IMULL_M_RM}
/*270*/ ,{IMULW,X86_IMULW_RM_RRM16}
/*271*/ ,{IMULL,X86_IMULL_RM}
/*273*/ ,{IMULW,X86_IMULW_RMI_RRM16I8S}
/*274*/ ,{IMULL,X86_IMULL_RMI_RRM32I8S}
/*276*/ ,{IMULW,X86_IMULW_RMI_RRM16I}
/*277*/ ,{IMULL,X86_IMULL_RMI}
/*285*/ ,{INB,X86_INB_I_AL}
/*286*/ ,{INW,X86_INW_I_AX}
/*287*/ ,{INL,X86_INL_I_EAX}
/*289*/ ,{INB,X86_INB_R_AL}
/*290*/ ,{INW,X86_INW_R_AX}
/*291*/ ,{INL,X86_INL_R_EAX}
/*293*/ ,{INCB,X86_INCB_M_RM}
/*294*/ ,{INCW,X86_INCW_M_RM}
/*295*/ ,{INCL,X86_INCL_M_RM}
/*297*/ ,{INCW,X86_INCW_M_R16_OP1}
/*298*/ ,{INCL,X86_INCL_M_R32_OP1}
/*300*/ ,{INSB,X86_INSB}
/*302*/ ,{INSW,X86_INSW}
/*304*/ ,{INSL,X86_INSL}
/*307*/ ,{INT_x86,X86_INT1}
/*308*/ ,{INT_x86,X86_INT3}
/*309*/ ,{INT_x86,X86_INT}
/*310*/ ,{INTO_x86,X86_INTO}
/*311*/ ,{INVD,X86_INVD}
/*312*/ ,{INVLPG,X86_INVLPG}
/*313*/ ,{IRETW,X86_IRET}
/*314*/ ,{IRET,X86_IRETD}
/*315*/ ,{JA,X86_JA}
/*316*/ ,{JAE,X86_JAE}
/*317*/ ,{JB,X86_JB}
/*318*/ ,{JBE,X86_JBE}
/*320*/ ,{JCXZ,X86_JCXZ}
/*321*/ ,{JE,X86_JE}
/*323*/ ,{JG,X86_JG}
/*324*/ ,{JGE,X86_JGE}
/*326*/ ,{JLE,X86_JLE}
/*334*/ ,{JNGE,X86_JNGE}
/*337*/ ,{JNO,X86_JNO}
/*338*/ ,{JNP,X86_JNP}
/*339*/ ,{JNS,X86_JNS}
/*340*/ ,{JNZ,X86_JNZ}
/*341*/ ,{JO,X86_JO}
/*342*/ ,{JP,X86_JP}
/*345*/ ,{JS,X86_JS}
/*346*/ ,{JA,X86_JAFW}
/*347*/ ,{JAE,X86_JAEFW}
/*348*/ ,{JB,X86_JBFW}
/*349*/ ,{JBE,X86_JBEFW}
/*351*/ ,{JE,X86_JEFW}
/*353*/ ,{JG,X86_JGFW}
/*354*/ ,{JGE,X86_JGEFW}
/*356*/ ,{JLE,X86_JLEFW}
/*364*/ ,{JNGE,X86_JNGEFW}
/*367*/ ,{JNO,X86_JNOFW}
/*368*/ ,{JNP,X86_JNPFW}
/*369*/ ,{JNS,X86_JNSFW}
/*370*/ ,{JNZ,X86_JNZFW}
/*371*/ ,{JO,X86_JOFW}
/*372*/ ,{JP,X86_JPFW}
/*375*/ ,{JS,X86_JSFW}
/*376*/ ,{JA,X86_JAFL}
/*377*/ ,{JAE,X86_JAEFL}
/*378*/ ,{JB,X86_JBFL}
/*379*/ ,{JBE,X86_JBEFL}
/*381*/ ,{JE,X86_JEFL}
/*383*/ ,{JG,X86_JGFL}
/*384*/ ,{JGE,X86_JGEFL}
/*386*/ ,{JLE,X86_JLEFL}
/*394*/ ,{JNGE,X86_JNGEFL}
/*397*/ ,{JNO,X86_JNOFL}
/*398*/ ,{JNP,X86_JNPFL}
/*399*/ ,{JNS,X86_JNSFL}
/*400*/ ,{JNZ,X86_JNZFL}
/*401*/ ,{JO,X86_JOFL}
/*402*/ ,{JP,X86_JPFL}
/*405*/ ,{JS,X86_JSFL}
/*406*/ ,{JMP,X86_JMPB}
/*407*/ ,{JMP,X86_JMPW}
/*408*/ ,{JMP,X86_JMPL}
/*409*/ ,{LJMP,X86_JMPWP}
/*410*/ ,{LJMP,X86_JMPLP}
/*411*/ ,{JMP,X86_JMPW_M_RM}
/*412*/ ,{JMP,X86_JMPL_M_RM}
/*413*/ ,{JMPF,X86_JMPWI}
/*414*/ ,{JMPF,X86_JMPLI}
/*415*/ ,{LAHF,X86_LAHF}
/*416*/ ,{LARW,X86_LARW_M_RRM16}
/*417*/ ,{LARL,X86_LARL_M}
/*418*/ ,{LDSW,X86_LDSW}
/*419*/ ,{LDSL,X86_LDSL}
/*421*/ ,{LESW,X86_LESW}
/*422*/ ,{LESL,X86_LESL}
/*424*/ ,{LSSW,X86_LSSW}
/*425*/ ,{LSSL,X86_LSSL}
/*427*/ ,{LFSW,X86_LFSW}
/*428*/ ,{LFSL,X86_LFSL}
/*430*/ ,{LGSW,X86_LGSW}
/*431*/ ,{LGSL,X86_LGSL}
/*433*/ ,{LEAW,X86_LEAW}
/*434*/ ,{LEAL,X86_LEAL}
/*436*/ ,{LEAVEN,X86_LEAVEW}
/*437*/ ,{LEAVE,X86_LEAVEL}
/*438*/ ,{LGDT,X86_LGDT}
/*439*/ ,{LIDT,X86_LIDT}
/*440*/ ,{LLDT,X86_LLDT_M}
/*441*/ ,{LMSW,X86_LMSW_M}
/*442*/ ,{LODSB,X86_LODSB}
/*444*/ ,{LODSW,X86_LODSW}
/*446*/ ,{LODSL,X86_LODSL}
/*450*/ ,{LOOP,X86_LOOP}
/*451*/ ,{LOOPZ,X86_LOOPZ}
/*452*/ ,{LOOPNZ,X86_LOOPNZ}
/*455*/ ,{LSLW,X86_LSLW_RRM16}
/*456*/ ,{LSLL,X86_LSLL}
/*458*/ ,{LTR,X86_LTRW_M}
/*459*/ ,{MOVB,X86_MOVB_RM_AL}
/*460*/ ,{MOVW,X86_MOVW_RM_AX}
/*461*/ ,{MOVL,X86_MOVL_RM_EAX}
/*463*/ ,{MOVB,X86_MOVB_MR_AL}
/*464*/ ,{MOVW,X86_MOVW_MR_AX}
/*465*/ ,{MOVL,X86_MOVL_MR_EAX}
/*467*/ ,{MOVB,X86_MOVB_MR}
/*468*/ ,{MOVW,X86_MOVW_MR_RMR16}
/*469*/ ,{MOVL,X86_MOVL_MR}
/*471*/ ,{MOVB,X86_MOVB_RM}
/*472*/ ,{MOVW,X86_MOVW_RM_RRM16}
/*473*/ ,{MOVL,X86_MOVL_RM}
/*475*/ ,{MOVW,X86_MOVW_SM_RMS16}
/*476*/ ,{MOVL,X86_MOVW_SM_RMS32}
/*477*/ ,{MOVW,X86_MOVW_MS_SRM16}
/*478*/ ,{MOVB,X86_MOVB_RI}
/*479*/ ,{MOVW,X86_MOVW_RI}
/*480*/ ,{MOVL,X86_MOVL_RI}
/*482*/ ,{MOVB,X86_MOVB_MI_IMB}
/*483*/ ,{MOVW,X86_MOVW_MI}
/*484*/ ,{MOVL,X86_MOVL_MI}
/*486*/ ,{MOVL,X86_MOVL_CR}
/*488*/ ,{MOVL,X86_MOVL_RC}
/*490*/ ,{MOVL,X86_MOVL_DR}
/*492*/ ,{MOVL,X86_MOVL_RD}
/*494*/ ,{MOVSB,X86_MOVSB}
/*496*/ ,{MOVSW,X86_MOVSW}
/*498*/ ,{MOVSL,X86_MOVSL}
/*501*/ ,{MOVSBW,X86_MOVSXBW_M}
/*502*/ ,{MOVSBL,X86_MOVSXBL_M}
/*503*/ ,{MOVSWW,X86_MOVSXWW_M}
/*504*/ ,{MOVSWL,X86_MOVSXWL_M}
/*505*/ ,{MOVZBW,X86_MOVZXBW_M}
/*506*/ ,{MOVZBL,X86_MOVZXBL_M}
/*507*/ ,{MOVZWW,X86_MOVZXWW_M}
/*508*/ ,{MOVZWL,X86_MOVZXWL_M}
/*509*/ ,{MULB,X86_MULB_RM_AL}
/*510*/ ,{MULW,X86_MULW_RM_AX}
/*511*/ ,{MULL,X86_MULL_RM_EAX}
/*513*/ ,{NEGB,X86_NEGB_M_RM}
/*514*/ ,{NEGW,X86_NEGW_M_RM}
/*515*/ ,{NEGL,X86_NEGL_M_RM}
/*517*/ ,{NOTB,X86_NOTB_M_RM}
/*518*/ ,{NOTW,X86_NOTW_M_RM}
/*519*/ ,{NOTL,X86_NOTL_M_RM}
/*521*/ ,{ORB,X86_ORB_RI_AL}
/*522*/ ,{ORW,X86_ORW_RI_AX}
/*523*/ ,{ORL,X86_ORL_RI_EAX}
/*525*/ ,{ORB,X86_ORB_MI_IMB}
/*526*/ ,{ORB,X86_ORB_MI_ALIAS}
/*527*/ ,{ORW,X86_ORW_MI}
/*528*/ ,{ORL,X86_ORL_MI}
/*530*/ ,{ORW,X86_ORW_MI_B}
/*531*/ ,{ORL,X86_ORL_MI_B}
/*533*/ ,{ORB,X86_ORB_MR}
/*534*/ ,{ORW,X86_ORW_MR_RMR16}
/*535*/ ,{ORL,X86_ORL_MR}
/*537*/ ,{ORB,X86_ORB_RM}
/*538*/ ,{ORW,X86_ORW_RM_RRM16}
/*539*/ ,{ORL,X86_ORL_RM}
/*541*/ ,{OUTB,X86_OUTB_I_AL}
/*542*/ ,{OUTW,X86_OUTW_I_AX}
/*543*/ ,{OUTL,X86_OUTL_I_EAX}
/*545*/ ,{OUTB,X86_OUTB_R_AL}
/*546*/ ,{OUTW,X86_OUTW_R_AX}
/*547*/ ,{OUTL,X86_OUTL_R_EAX}
/*549*/ ,{OUTSB,X86_OUTSB}
/*551*/ ,{OUTSW,X86_OUTSW}
/*553*/ ,{OUTSL,X86_OUTSL}
/*556*/ ,{POPW,X86_POPW_M_RM}
/*557*/ ,{POPL,X86_POPL_M_RM}
/*559*/ ,{POPW,X86_POPW_R_R16_OP1}
/*560*/ ,{POPL,X86_POPL_R_R32_OP1}
/*562*/ ,{POPW,X86_POPW_DS_RDS}
/*563*/ ,{POPL,X86_POPL_DS_RDS}
/*565*/ ,{POPW,X86_POPW_ES}
/*566*/ ,{POPL,X86_POPL_ES}
/*568*/ ,{POPW,X86_POPW_FS}
/*569*/ ,{POPL,X86_POPL_FS}
/*571*/ ,{POPW,X86_POPW_DS_RFS}
/*572*/ ,{POPL,X86_POPL_DS_RFS}
/*574*/ ,{POPW,X86_POPW_GS_RGS}
/*575*/ ,{POPL,X86_POPL_GS_RGS}
/*577*/ ,{POPAW,X86_POPAW}
/*578*/ ,{POPAL,X86_POPAL}
/*580*/ ,{POPFW,X86_POPFW}
/*581*/ ,{POPFL,X86_POPFL}
/*583*/ ,{PUSHW,X86_PUSHW_M_RM}
/*584*/ ,{PUSHL,X86_PUSHL_M_RM}
/*586*/ ,{PUSHW,X86_PUSHW_R_R16_OP1}
/*587*/ ,{PUSHL,X86_PUSHL_R_R32_OP1}
/*589*/ ,{PUSHW,X86_PUSHW_DS_RDS}
/*590*/ ,{PUSHL,X86_PUSHL_DS_RDS}
/*592*/ ,{PUSHW,X86_PUSHW_ES}
/*593*/ ,{PUSHL,X86_PUSHL_ES}
/*595*/ ,{PUSHW,X86_PUSHW_SS}
/*596*/ ,{PUSHL,X86_PUSHL_SS}
/*598*/ ,{PUSHW,X86_PUSHW_FS_RFS}
/*599*/ ,{PUSHL,X86_PUSHL_FS_RFS}
/*601*/ ,{PUSHW,X86_PUSHW_GS_RGS}
/*602*/ ,{PUSHL,X86_PUSHL_GS_RGS}
/*604*/ ,{PUSHW,X86_PUSHW_CS_RCS}
/*605*/ ,{PUSHL,X86_PUSHL_CS_RCS}
/*607*/ ,{PUSHW,X86_PUSHBW_I}
/*608*/ ,{PUSHL,X86_PUSHB_I}
/*610*/ ,{PUSHW,X86_PUSHW_I}
/*611*/ ,{PUSHL,X86_PUSHL_I}
/*613*/ ,{PUSHAW,X86_PUSHAW}
/*614*/ ,{PUSHAL,X86_PUSHAL}
/*616*/ ,{PUSHFW,X86_PUSHFW}
/*617*/ ,{PUSHFL,X86_PUSHFL}
/*619*/ ,{RCLB,X86_RCLB_MI_SHFT_1}
/*620*/ ,{RCLB,X86_RCLB_MR}
/*622*/ ,{RCLB,X86_RCLB_MI_IMB}
/*623*/ ,{RCLW,X86_RCLW_MI_SHFT_1}
/*625*/ ,{RCLW,X86_RCLW_MR}
/*626*/ ,{RCLW,X86_RCLW_MI_IMB}
/*627*/ ,{RCLL,X86_RCLL_MI_SHFT_1}
/*631*/ ,{RCLL,X86_RCLL_MR}
/*633*/ ,{RCLL,X86_RCLL_MI_IMB}
/*635*/ ,{RCRB,X86_RCRB_MI_SHFT_1}
/*636*/ ,{RCRB,X86_RCRB_MR}
/*638*/ ,{RCRB,X86_RCRB_MI_IMB}
/*639*/ ,{RCRW,X86_RCRW_MI_SHFT_1}
/*641*/ ,{RCRW,X86_RCRW_MR}
/*642*/ ,{RCRW,X86_RCRW_MI_IMB}
/*643*/ ,{RCRL,X86_RCRL_MI_SHFT_1}
/*647*/ ,{RCRL,X86_RCRL_MR}
/*649*/ ,{RCRL,X86_RCRL_MI_IMB}
/*651*/ ,{ROLB,X86_ROLB_MI_SHFT_1}
/*652*/ ,{ROLB,X86_ROLB_MR}
/*654*/ ,{ROLB,X86_ROLB_MI_IMB}
/*655*/ ,{ROLW,X86_ROLW_MI_SHFT_1}
/*657*/ ,{ROLW,X86_ROLW_MR}
/*658*/ ,{ROLW,X86_ROLW_MI_IMB}
/*659*/ ,{ROLL,X86_ROLL_MI_SHFT_1}
/*663*/ ,{ROLL,X86_ROLL_MR}
/*665*/ ,{ROLL,X86_ROLL_MI_IMB}
/*667*/ ,{RORB,X86_RORB_MI_SHFT_1}
/*668*/ ,{RORB,X86_RORB_MR}
/*670*/ ,{RORB,X86_RORB_MI_IMB}
/*671*/ ,{RORW,X86_RORW_MI_SHFT_1}
/*673*/ ,{RORW,X86_RORW_MR}
/*674*/ ,{RORW,X86_RORW_MI_IMB}
/*675*/ ,{RORL,X86_RORL_MI_SHFT_1}
/*679*/ ,{RORL,X86_RORL_MR}
/*681*/ ,{RORL,X86_RORL_MI_IMB}
/*683*/ ,{RDMSR,X86_RDMSR}
/*684*/ ,{RDPMC,X86_RDPMC}
/*685*/ ,{RDTSC,X86_RDTSC}
/*686*/ ,{RSM,X86_RSM}
/*692*/ ,{RET,X86_RETN}
/*693*/ ,{LRET,X86_RETF}
/*694*/ ,{RET,X86_RETN_I}
/*695*/ ,{LRET,X86_RETF_I}
/*696*/ ,{SAHF,X86_SAHF}
/*713*/ ,{SARB,X86_SARB_MI_SHFT_1}
/*714*/ ,{SARB,X86_SARB_MR}
/*716*/ ,{SARB,X86_SARB_MI_IMB}
/*717*/ ,{SARW,X86_SARW_MI_SHFT_1}
/*719*/ ,{SARW,X86_SARW_MR}
/*720*/ ,{SARW,X86_SARW_MI_IMB}
/*721*/ ,{SARL,X86_SARL_MI_SHFT_1}
/*725*/ ,{SARL,X86_SARL_MR}
/*727*/ ,{SARL,X86_SARL_MI_IMB}
/*729*/ ,{SHLB,X86_SHLB_MI_SHFT_1}
/*730*/ ,{SHLB,X86_SHLB_MR}
/*732*/ ,{SHLB,X86_SHLB_MI_IMB}
/*733*/ ,{SHLW,X86_SHLW_MI_SHFT_1}
/*735*/ ,{SHLW,X86_SHLW_MR}
/*736*/ ,{SHLW,X86_SHLW_MI_IMB}
/*737*/ ,{SHLL,X86_SHLL_MI_SHFT_1}
/*741*/ ,{SHLL,X86_SHLL_MR}
/*743*/ ,{SHLL,X86_SHLL_MI_IMB}
/*745*/ ,{SHLB,X86_SHLB_MI_1_ALIAS}
/*746*/ ,{SHLB,X86_SHLB_MR_ALIAS}
/*748*/ ,{SHLB,X86_SHLB_MI_I_ALIAS}
/*749*/ ,{SHLW,X86_SHLW_MI_1_ALIAS}
/*751*/ ,{SHLW,X86_SHLW_MR_ALIAS}
/*752*/ ,{SHLW,X86_SHLW_MI_I_ALIAS}
/*753*/ ,{SHLL,X86_SHLL_MI_1_ALIAS}
/*757*/ ,{SHLL,X86_SHLL_MR_ALIAS}
/*759*/ ,{SHLL,X86_SHLL_MI_I_ALIAS}
/*761*/ ,{SHRB,X86_SHRB_MI_SHFT_1}
/*762*/ ,{SHRB,X86_SHRB_MR}
/*764*/ ,{SHRB,X86_SHRB_MI_IMB}
/*765*/ ,{SHRW,X86_SHRW_MI_SHFT_1}
/*767*/ ,{SHRW,X86_SHRW_MR}
/*768*/ ,{SHRW,X86_SHRW_MI_IMB}
/*769*/ ,{SHRL,X86_SHRL_MI_SHFT_1}
/*773*/ ,{SHRL,X86_SHRL_MR}
/*775*/ ,{SHRL,X86_SHRL_MI_IMB}
/*777*/ ,{SBBB,X86_SBBB_RI_AL}
/*778*/ ,{SBBW,X86_SBBW_RI_AX}
/*779*/ ,{SBBL,X86_SBBL_RI_EAX}
/*781*/ ,{SBBB,X86_SBBB_MI_IMB}
/*782*/ ,{SBBB,X86_SBBB_MI_ALIAS}
/*783*/ ,{SBBW,X86_SBBW_MI}
/*784*/ ,{SBBL,X86_SBBL_MI}
/*786*/ ,{SBBW,X86_SBBW_MI_B}
/*787*/ ,{SBBL,X86_SBBL_MI_B}
/*789*/ ,{SBBB,X86_SBBB_MR}
/*790*/ ,{SBBW,X86_SBBW_MR_RMR16}
/*791*/ ,{SBBL,X86_SBBL_MR}
/*793*/ ,{SBBB,X86_SBBB_RM}
/*794*/ ,{SBBW,X86_SBBW_RM_RRM16}
/*795*/ ,{SBBL,X86_SBBL_RM}
/*797*/ ,{SCASB,X86_SCASB}
/*800*/ ,{SCASW,X86_SCASW}
/*803*/ ,{SCASL,X86_SCASL}
/*807*/ ,{SALC,X86_SALC}
/*808*/ ,{SETA,X86_SETA_M_RM}
/*809*/ ,{SETAE,X86_SETAE_M_RM}
/*810*/ ,{SETB,X86_SETB_M_RM}
/*811*/ ,{SETBE,X86_SETBE_M_RM}
/*813*/ ,{SETE,X86_SETE_M_RM}
/*814*/ ,{SETG,X86_SETG_M_RM}
/*815*/ ,{SETGE,X86_SETGE_M_RM}
/*816*/ ,{SETL,X86_SETL_M_RM}
/*817*/ ,{SETLE,X86_SETLE_R_RM}
/*823*/ ,{SETNE,X86_SETNE_M_RM}
/*828*/ ,{SETNO,X86_SETNO_M_RM}
/*829*/ ,{SETNP,X86_SETNP_M_RM}
/*830*/ ,{SETNS,X86_SETNS_M_RM}
/*832*/ ,{SETO,X86_SETO_M_RM}
/*833*/ ,{SETP,X86_SETA_M_P}
/*836*/ ,{SETS,X86_SETS_M_RM}
/*838*/ ,{SGDT,X86_SGDT}
/*839*/ ,{SIDT,X86_SIDT}
/*840*/ ,{SHLDW,X86_SHLDW_MI}
/*841*/ ,{SHLDL,X86_SHLDL_MI}
/*843*/ ,{SHLDW,X86_SHLDW_MR}
/*844*/ ,{SHLDL,X86_SHLDL_MR}
/*846*/ ,{SHRDW,X86_SHRDW_MI}
/*847*/ ,{SHRDL,X86_SHRDL_MI}
/*849*/ ,{SHRDW,X86_SHRDW_MR}
/*850*/ ,{SHRDL,X86_SHRDL_MR}
/*852*/ ,{SLDTW,X86_SLDTW_M}
/*853*/ ,{SLDTL,X86_SLDTL_M}
/*855*/ ,{SMSWW,X86_SMSWW_M}
/*856*/ ,{SMSWL,X86_SMSWL_M}
/*858*/ ,{STC,X86_STC}
/*859*/ ,{STD,X86_STD}
/*860*/ ,{STI,X86_STI}
/*861*/ ,{STOSB,X86_STOSB}
/*863*/ ,{STOSW,X86_STOSW}
/*865*/ ,{STOSL,X86_STOSL}
/*868*/ ,{STRW,X86_STRW_M}
/*869*/ ,{STRL,X86_STRL_M}
/*871*/ ,{SUBB,X86_SUBB_RI_AL}
/*872*/ ,{SUBW,X86_SUBW_RI_AX}
/*873*/ ,{SUBL,X86_SUBL_RI_EAX}
/*875*/ ,{SUBB,X86_SUBB_MI_IMB}
/*876*/ ,{SUBB,X86_SUBB_MI_ALIAS}
/*877*/ ,{SUBW,X86_SUBW_MI}
/*878*/ ,{SUBL,X86_SUBL_MI}
/*880*/ ,{SUBW,X86_SUBW_MI_B}
/*881*/ ,{SUBL,X86_SUBL_MI_B}
/*883*/ ,{SUBB,X86_SUBB_MR}
/*884*/ ,{SUBW,X86_SUBW_MR_RMR16}
/*885*/ ,{SUBL,X86_SUBL_MR}
/*887*/ ,{SUBB,X86_SUBB_RM}
/*888*/ ,{SUBW,X86_SUBW_RM_RRM16}
/*889*/ ,{SUBL,X86_SUBL_RM}
/*891*/ ,{TESTB,X86_TESTB_RI_AL}
/*892*/ ,{TESTW,X86_TESTW_RI_AX}
/*893*/ ,{TESTL,X86_TESTL_RI_EAX}
/*895*/ ,{TESTB,X86_TESTB_MI_IMB}
/*896*/ ,{TESTB,X86_TESTB_MI_ALIAS}
/*897*/ ,{TESTW,X86_TESTW_MI}
/*898*/ ,{TESTW,X86_TESTW_MI_ALIAS}
/*899*/ ,{TESTL,X86_TESTL_MI}
/*901*/ ,{TESTL,X86_TESTL_MI_ALIAS}
/*903*/ ,{TESTB,X86_TESTB_MR}
/*904*/ ,{TESTW,X86_TESTW_MR_RMR16}
/*905*/ ,{TESTL,X86_TESTL_MR}
/*907*/ ,{VERR,X86_VERR_M}
/*908*/ ,{VERW,X86_VERW_M}
/*909*/ ,{WAIT,X86_WAIT}
/*910*/ ,{WBINVD,X86_WBINVD}
/*911*/ ,{WRMSR,X86_WRMSR}
/*912*/ ,{XADDB,X86_XADDB_MR}
/*913*/ ,{XADDW,X86_XADDW_MR_RMR16}
/*914*/ ,{XADDL,X86_XADDL_MR}
/*916*/ ,{XCHGW,X86_XCHGW_RR_AX}
/*918*/ ,{XCHGL,X86_XCHGL_RR_EAX}
/*924*/ ,{XCHGB,X86_XCHGB_RM}
/*926*/ ,{XCHGW,X86_XCHGW_RM_RRM16}
/*927*/ ,{XCHGL,X86_XCHGL_RM}
/*931*/ ,{XLAT,X86_XLATB}
/*932*/ ,{XORB,X86_XORB_RI_AL}
/*933*/ ,{XORW,X86_XORW_RI_AX}
/*934*/ ,{XORL,X86_XORL_RI_EAX}
/*936*/ ,{XORB,X86_XORB_MI_IMB}
/*937*/ ,{XORB,X86_XORB_MI_ALIAS}
/*938*/ ,{XORW,X86_XORW_MI}
/*939*/ ,{XORL,X86_XORL_MI}
/*941*/ ,{XORW,X86_XORW_MI_B}
/*942*/ ,{XORL,X86_XORL_MI_B}
/*944*/ ,{XORB,X86_XORB_MR}
/*945*/ ,{XORW,X86_XORW_MR_RMR16}
/*946*/ ,{XORL,X86_XORL_MR}
/*948*/ ,{XORB,X86_XORB_RM}
/*949*/ ,{XORW,X86_XORW_RM_RRM16}
/*950*/ ,{XORL,X86_XORL_RM}
/*952*/ ,{F2XM1,X86_F2XM1}
/*953*/ ,{FABS,X86_FABS}
/*954*/ ,{FADDS,X86_FADDS_M}
/*955*/ ,{FADDL,X86_FADDL_M}
/*956*/ ,{FADD,X86_FADD_0I}
/*957*/ ,{FADD,X86_FADD_I0}
/*958*/ ,{FADDP,X86_FADDP_I0}
/*960*/ ,{FIADDL,X86_FIADDL_M}
/*961*/ ,{FIADD,X86_FIADDS_M}
/*962*/ ,{FBLD,X86_FBLD}
/*963*/ ,{FBSTP,X86_FBSTP}
/*964*/ ,{FCHS,X86_FCHS}
/*966*/ ,{FNCLEX,X86_FNCLEX}
/*967*/ ,{FCOMS,X86_FCOMS_M}
/*968*/ ,{FCOML,X86_FCOML_M}
/*969*/ ,{FCOM,X86_FCOM_0I}
/*970*/ ,{FCOM,X86_FCOM_0I_ALIAS}
/*972*/ ,{FCOMPS,X86_FCOMPS_M}
/*973*/ ,{FCOMPL,X86_FCOMPL_M}
/*974*/ ,{FCOMP,X86_FCOMP_0I}
/*975*/ ,{FCOMP,X86_FCOMP_0I_ALIAS1}
/*976*/ ,{FCOMP,X86_FCOMP_0I_ALIAS2}
/*978*/ ,{FCOMPP,X86_FCOMPP}
/*979*/ ,{FCOMI,X86_FCOMI_0I}
/*981*/ ,{FCOMIP,X86_FCOMIP_0I}
/*983*/ ,{FCOS,X86_FCOS}
/*984*/ ,{FDECSTP,X86_FDECSTP}
/*985*/ ,{FCMOVB,X86_FCMOVB}
/*986*/ ,{FCMOVE,X86_FCMOVE}
/*987*/ ,{FCMOVBE,X86_FCMOVBE}
/*988*/ ,{FCMOVU,X86_FCMOVU}
/*989*/ ,{FCMOVNB,X86_FCMOVNB}
/*990*/ ,{FCMOVNE,X86_FCMOVNE}
/*991*/ ,{FCMOVNBE,X86_FCMOVNBE}
/*992*/ ,{FCMOVNU,X86_FCMOVNU}
/*993*/ ,{FDIVS,X86_FDIVS_M}
/*994*/ ,{FDIVL,X86_FDIVL_M}
/*995*/ ,{FDIV,X86_FDIV_0I}
/*996*/ ,{FDIV,X86_FDIVR_I0}
/*997*/ ,{FDIVP,X86_FDIVRP_I0}
/*999*/ ,{FIDIV,X86_FIDIV_M}
/*1000*/ ,{FIDIVL,X86_FIDIVL_M}
/*1001*/ ,{FDISI,X86_FDISI}
/*1002*/ ,{FENI,X86_FENI}
/*1003*/ ,{FDIVRS,X86_FDIVRS_M}
/*1004*/ ,{FDIVRL,X86_FDIVRL_M}
/*1005*/ ,{FDIVR,X86_FDIVR_0I}
/*1006*/ ,{FDIVR,X86_FDIV_I0}
/*1007*/ ,{FDIVRP,X86_FDIVP_I0}
/*1009*/ ,{FIDIVR,X86_FIDIVR_M}
/*1010*/ ,{FIDIVRL,X86_FIDIVRL_M}
/*1011*/ ,{FFREE,X86_FFREE}
/*1012*/ ,{FFREEP,X86_FFREEP}
/*1013*/ ,{FICOM,X86_FICOM}
/*1014*/ ,{FICOML,X86_FICOML}
/*1015*/ ,{FICOMP,X86_FICOMP}
/*1016*/ ,{FICOMPL,X86_FICOMPL}
/*1017*/ ,{FILD,X86_FILD}
/*1018*/ ,{FILDL,X86_FILDL}
/*1019*/ ,{FILDLL,X86_FILDLL}
/*1020*/ ,{FINCSTP,X86_FINCSTP}
/*1022*/ ,{FNINIT,X86_FNINIT}
/*1023*/ ,{FIST,X86_FIST}
/*1024*/ ,{FISTL,X86_FISTL}
/*1025*/ ,{FISTP,X86_FISTP}
/*1026*/ ,{FISTPL,X86_FISTPL}
/*1027*/ ,{FISTPLL,X86_FISTPLL}
/*1028*/ ,{FLDS,X86_FLDS}
/*1029*/ ,{FLDL,X86_FLDL}
/*1030*/ ,{FLDT,X86_FLDT}
/*1031*/ ,{FLD,X86_FLD}
/*1032*/ ,{FLD1,X86_FLD1}
/*1033*/ ,{FLDL2T,X86_FLDL2T}
/*1034*/ ,{FLDL2E,X86_FLDL2E}
/*1035*/ ,{FLDPI,X86_FLDPI}
/*1036*/ ,{FLDLG2,X86_FLDLG2}
/*1037*/ ,{FLDLN2,X86_FLDLN2}
/*1038*/ ,{FLDZ,X86_FLDZ}
/*1039*/ ,{FLDCW,X86_FLDCW_RM}
/*1040*/ ,{FLDENV,X86_FLDENV_W_M14B}
/*1041*/ ,{FLDENV,X86_FLDENV_L_M28B}
/*1042*/ ,{FMULS,X86_FMULS_M}
/*1043*/ ,{FMULL,X86_FMULL_M}
/*1044*/ ,{FMUL,X86_FMUL_0I}
/*1045*/ ,{FMUL,X86_FMUL_I0}
/*1046*/ ,{FMULP,X86_FMULP_I0}
/*1048*/ ,{FIMULL,X86_FIMULL_M}
/*1049*/ ,{FIMUL,X86_FIMUL_M}
/*1050*/ ,{FNOP,X86_FNOP}
/*1051*/ ,{FPATAN,X86_FPATAN}
/*1052*/ ,{FPREM,X86_FPREM}
/*1053*/ ,{FPREM1,X86_FPREM1}
/*1054*/ ,{FPTAN,X86_FPTAN}
/*1055*/ ,{FRNDINT,X86_FRNDINT}
/*1056*/ ,{FRSTOR,X86_FRSTOR_W_RM}
/*1057*/ ,{FRSTOR,X86_FRSTOR_L_RM}
/*1059*/ ,{FNSAVE,X86_FNSAVE_W_RM}
/*1060*/ ,{FNSAVE,X86_FNSAVE_L_RM}
/*1061*/ ,{FSETPM,X86_FSETPM}
/*1062*/ ,{FSCALE,X86_FSCALE}
/*1063*/ ,{FSIN,X86_FSIN}
/*1064*/ ,{FSINCOS,X86_FSINCOS}
/*1065*/ ,{FSQRT,X86_FSQRT}
/*1066*/ ,{FSTS,X86_FSTS}
/*1067*/ ,{FSTL,X86_FSTL}
/*1068*/ ,{FST,X86_FST}
/*1069*/ ,{FSTPS,X86_FSTPS}
/*1070*/ ,{FSTPL,X86_FSTPL}
/*1071*/ ,{FSTPT,X86_FSTPT}
/*1072*/ ,{FSTP,X86_FSTP}
/*1073*/ ,{FSTPNCE,X86_FSTP_ALIAS1}
/*1074*/ ,{FSTP,X86_FSTP_ALIAS2}
/*1075*/ ,{FSTP,X86_FSTP_ALIAS3}
/*1077*/ ,{FNSTCW,X86_FNSTCW_RM}
/*1079*/ ,{FNSTENV,X86_FSTENV_W_M14B}
/*1080*/ ,{FNSTENV,X86_FSTENV_L_M28B}
/*1083*/ ,{FNSTSW,X86_FNSTSW_RM}
/*1084*/ ,{FNSTSW,X86_FNSTSW_A_16}
/*1085*/ ,{FSUBS,X86_FSUBS_M}
/*1086*/ ,{FSUBL,X86_FSUBL_M}
/*1087*/ ,{FSUB,X86_FSUB_0I}
/*1088*/ ,{FSUB,X86_FSUBR_I0}
/*1089*/ ,{FSUBP,X86_FSUBRP_I0}
/*1091*/ ,{FISUBL,X86_FISUBL_M}
/*1092*/ ,{FISUB,X86_FISUB_M}
/*1093*/ ,{FSUBRS,X86_FSUBRS_M}
/*1094*/ ,{FSUBRL,X86_FSUBRL_M}
/*1095*/ ,{FSUBR,X86_FSUBR_0I}
/*1096*/ ,{FSUBR,X86_FSUB_I0}
/*1097*/ ,{FSUBRP,X86_FSUBP_I0}
/*1099*/ ,{FISUBRL,X86_FISUBRL_M}
/*1100*/ ,{FISUBR,X86_FISUBR_M}
/*1101*/ ,{FTST,X86_FTST}
/*1102*/ ,{FUCOM,X86_FUCOM}
/*1104*/ ,{FUCOMP,X86_FUCOMP}
/*1106*/ ,{FUCOMPP,X86_FUCOMPP}
/*1107*/ ,{FUCOMI,X86_FUCOMI}
/*1109*/ ,{FUCOMIP,X86_FUCOMIP}
/*1112*/ ,{FXAM,X86_FXAM}
/*1113*/ ,{FXCH,X86_FXCH}
/*1115*/ ,{FXCH,X86_FXCH_ALIAS1}
/*1116*/ ,{FXCH,X86_FXCH_ALIAS2}
/*1117*/ ,{FXTRACT,X86_FXTRACT}
/*1118*/ ,{FYL2X,X86_FYL2X}
/*1119*/ ,{FYL2XP1,X86_FYL2XP1}
/*1122*/ ,{ZALLOC,X86_ZALLOC}
/*1123*/ ,{EMMS,X86_EMMS_MM}
/*1124*/ ,{MOVD,X86_MOVDL_MM}
/*1125*/ ,{MOVD,X86_MOVDL_MRR_MM}
/*1126*/ ,{MOVQ,X86_MOVQ_RM_MM}
/*1127*/ ,{MOVQ,X86_MOVQ_MR_MM}
/*1128*/ ,{PACKSSWB,X86_PACKSSWB_MM}
/*1129*/ ,{PACKSSDW,X86_PACKSSDW_MM}
/*1130*/ ,{PACKUSWB,X86_PACKUSWB_MM}
/*1131*/ ,{PADDB,X86_PADDB_MM}
/*1132*/ ,{PADDW,X86_PADDW_MM}
/*1133*/ ,{PADDD,X86_PADDD_MM}
/*1134*/ ,{PADDSB,X86_PADDSB_MM}
/*1135*/ ,{PADDSW,X86_PADDSW_MM}
/*1136*/ ,{PADDUSB,X86_PADDUSB_MM}
/*1137*/ ,{PADDUSW,X86_PADDUSW_MM}
/*1138*/ ,{PAND,X86_PAND_MM}
/*1139*/ ,{PANDN,X86_PANDN_MM}
/*1140*/ ,{PCMPEQB,X86_PCMPEQB_MM}
/*1141*/ ,{PCMPEQW,X86_PCMPEQW_MM}
/*1142*/ ,{PCMPEQD,X86_PCMPEQD_MM}
/*1143*/ ,{PCMPGTB,X86_PCMPGTB_MM}
/*1144*/ ,{PCMPGTW,X86_PCMPGTW_MM}
/*1145*/ ,{PCMPGTD,X86_PCMPGTD_MM}
/*1146*/ ,{PMADDWD,X86_PMADDWD_MM}
/*1147*/ ,{PMULHW,X86_PMULHW_MM}
/*1148*/ ,{PMULLW,X86_PMULLW_MM}
/*1149*/ ,{POR,X86_POR_MM}
/*1150*/ ,{PSLLW,X86_PSLLW_MM}
/*1151*/ ,{PSLLW,X86_PSLLW_I_MM}
/*1152*/ ,{PSLLD,X86_PSLLD_MM}
/*1153*/ ,{PSLLD,X86_PSLLD_I_MM}
/*1154*/ ,{PSLLQ,X86_PSLLQ_MM}
/*1155*/ ,{PSLLQ,X86_PSLLQ_I_MM}
/*1156*/ ,{PSRAW,X86_PSRAW_MM}
/*1157*/ ,{PSRAW,X86_PSRAW_I_MM}
/*1158*/ ,{PSRAD,X86_PSRAD_MM}
/*1159*/ ,{PSRAD,X86_PSRAD_I_MM}
/*1160*/ ,{PSRLW,X86_PSRLW__MM}
/*1161*/ ,{PSRLW,X86_PSRLW_I_MM}
/*1162*/ ,{PSRLD,X86_PSRLD_MM}
/*1163*/ ,{PSRLD,X86_PSRLD_I_MM}
/*1164*/ ,{PSRLQ,X86_PSRLQ_MM}
/*1165*/ ,{PSRLQ,X86_PSRLQ_I_MM}
/*1166*/ ,{PSUBB,X86_PSUBB_MM}
/*1167*/ ,{PSUBW,X86_PSUBW_MM}
/*1168*/ ,{PSUBD,X86_PSUBD_MM}
/*1169*/ ,{PSUBSB,X86_PSUBSB_MM}
/*1170*/ ,{PSUBSW,X86_PSUBSW_MM}
/*1171*/ ,{PSUBUSB,X86_PSUBUSB_MM}
/*1172*/ ,{PSUBUSW,X86_PSUBUSW_MM}
/*1173*/ ,{PUNPCKLBW,X86_PUNPCKLBW_MM}
/*1174*/ ,{PUNPCKLWD,X86_PUNPCKLWD_MM}
/*1175*/ ,{PUNPCKLDQ,X86_PUNPCKLDQ_MM}
/*1176*/ ,{PUNPCKHBW,X86_PUNPCKHBW_MM}
/*1177*/ ,{PUNPCKHWD,X86_PUNPCKHWD_MM}
/*1178*/ ,{PUNPCKHDQ,X86_PUNPCKHDQ_MM}
/*1179*/ ,{PXOR,X86_PXOR_MM}
};
