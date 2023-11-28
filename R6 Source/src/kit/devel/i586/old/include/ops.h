/***                                                                  ***/
/***   INTEL CORPORATION PROPRIETARY INFORMATION                      ***/
/***                                                                  ***/
/***   This software is supplied under the terms of a license         ***/
/***   agreement or nondisclosure agreement with Intel Corporation    ***/
/***   and may not be copied or disclosed except in accordance with   ***/
/***   the terms of that agreement.                                   ***/
/***   Copyright (c) 1992,1993,1994,1995  Intel Corporation.          ***/
/***                                                                  ***/

#ifndef _OPS_H_
#define _OPS_H_

/*
 *	ops.h - EX86 instruction formats
 */
enum	inst_grp	/** keep order compatible with voplist in pre-Alpha ias **/
{
    OP_ADDIP,   /*  0 */
    OP_ALU,    
    OP_ALU1,
    OP_ALU12,
    OP_ALU2,
    OP_ALU2D,   /*  5 */
    OP_BFTCH,   
    OP_CALL,    
    OP_CJMP,           
    OP_FCJMP,          
    OP_FETCH,   /* 10 */       
    OP_FLDX,    
    OP_FMOV,	
    OP_FP,             
    OP_FP1_I,          
    OP_FP2,     /* 15 */       
    OP_FSTX,    
    OP_I_FP1,   
    OP_JMP,            
    OP_JMPR,           
    OP_KCALL,   /* 20 */      
    OP_LDX,     
    OP_MFAR,    
    OP_MFKR,           
    OP_MFPR,           
    OP_MFSR,    /* 25 */
    OP_MOVI, 	
    OP_MTAR,    
    OP_MTKR,           
    OP_MTPR,           
    OP_MTSR,    /* 30 */
    OP_NONE,    
    OP_NOP,     
    OP_NOT,            
    OP_PSUB,    
    OP_STX,     /* 35 */
    OP_SUB,     
    OP_SUBO,    
    OP_X3,      
	OP_X86FMF,
	OP_X86FMT,	/* 40 */
	OP_X86JMP,
	OP_X86MF,
	OP_X86MT,
	OP_X86SMF,
	OP_X86SMT,	/* 45 */
    OP_ZXTB,
	OP_EVRET,
    OP_MF
} /* EX86_as_unit type */;

#endif /* _OPS_H_ */
