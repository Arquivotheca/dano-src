/********************************************************************
*                      X86
*            --------------------------
* x86.h:
*    Heaer file for the X86 tools.
*    
*
*  Defines: 
*  ==================
*  o  
*  o  
*  o  
*  o  
*  o  
*
*  
*
*********************************************************************/
#ifndef _X86_H
#define _X86_H


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif


typedef enum
{
    NO_MODE = 0,
    REAL_ADDRESS_MODE,
    V86_MODE,
    PROTECTED_MODE
} x86_mode;

typedef enum
{
	X86_Cond_None = 0,
	X86_Cond_O ,
	X86_Cond_NO ,
	X86_Cond_B ,
	X86_Cond_NB ,
	X86_Cond_E ,
	X86_Cond_NE ,
	X86_Cond_A ,
	X86_Cond_NA ,
	X86_Cond_S ,
	X86_Cond_NS ,
	X86_Cond_P ,
	X86_Cond_NP ,
	X86_Cond_L ,
	X86_Cond_NL ,
	X86_Cond_G ,
	X86_Cond_NG ,
	X86_Cond_CXZ,
	X86_Cond_ECXZ,
	
	X86_Cond_True
		
} x86_conditions;

typedef enum
{
	X86_fp_Cond_None = 0,
	X86_fp_Cond_B ,
	X86_fp_Cond_NB ,
	X86_fp_Cond_E ,
	X86_fp_Cond_NE ,
	X86_fp_Cond_BE ,
	X86_fp_Cond_NBE ,
	X86_fp_Cond_U ,
	X86_fp_Cond_NU 
		
} x86_fp_conditions;


typedef enum
{
    NO_PREFIX=0,
    PREFIX_REP=0xF3,
    PREFIX_REPNE=0xF2,
    PREFIX_LOCK=0xF0,
    PREFIX_SEG_CS=0x2E,
    PREFIX_SEG_SS=0x36,
    PREFIX_SEG_DS=0x3E,
    PREFIX_SEG_ES=0x26,
    PREFIX_SEG_FS=0x64,
    PREFIX_SEG_GS=0x65,
    PREFIX_OPERAND_SIZE=0x66,
    PREFIX_ADDRESS_SIZE=0x67 
} prefixes;


typedef enum
{
    X86_OPER_SIZE_BYTE=8,
    X86_OPER_SIZE_WORD=16,
    X86_OPER_SIZE_LONG=32,
} x86_oper_size;


typedef enum
{
    X86_ACTIVE_OPER_SIZE_WORD=16,
    X86_ACTIVE_OPER_SIZE_LONG=32
} x86_active_oper_size;


typedef enum
{
    X86_ACTIVE_ADDR_SIZE_WORD=16,
    X86_ACTIVE_ADDR_SIZE_LONG=32
} x86_active_addr_size;

/* registers */
#include "x86regs.h"

/* The maximum area needed for the FP environment data for fldenv/fstenv */
#define FP_ENV_SAVE_AREA_SIZE 28

#define NA (-1)


/** General masking - No more than 32 bits - Returns an usigned long */
#if !defined(UL)
#define UL unsigned long
#endif

#if !defined(BIT)
/* return an unsigned long with bit number 'b' set */
#define BIT(number)      (((UL)1) << (number))
#endif


/* is bit 'b' set in 'w' */
#ifndef ISSET
#define ISSET(w, b)      ((w & BIT(b)) != 0)
#endif

/* set all bits from LSB 'start' to MSB 'end' */
#define MASK(start, end) ((((end)>=31?(UL)0xFFFFFFFFL:           \
                         ((((UL)1)<<(end+1))-1))>>(start))<< (start) )

#define MASK_MAX(start)   (0xFFFFFFFFL << (start) )
#define MASK_MIN(end)     (((UL)1<<(end+1))-1)


/** immediate/offset macros, good for everything **/

/* get 'bits' bits of 'val', right-justified */
#define TO_RANGE(bits, val)            ((val) & ((1<<(bits))-1))

/* is the signed value of 'val' representible in 'bits' bits */
#define SIGNED_RANGE(bits, val)        (val < (1<<((bits)-1)) &&            \
                                        val >= -(1<<((bits)-1)))
#define LL_SIGNED_RANGE(bits, val)     (val < ((1LL)<<((bits)-1LL)) && \
                                        val >= -((1LL)<<((bits)-1)))

/* is the unsigned value of 'val' representible in 'bits' bits */
#define UNSIGNED_RANGE(bits, val)  (val < (1<<(bits)) && val >= 0)
#define LL_UNSIGNED_RANGE(bits, val)  (val < ((1LL)<<(bits)) &&    \
                                    val >= 0LL)
/* integer data object alignment */

#define BYTE_ALIGN_BITS         0
#define WORD_ALIGN_BITS         1
#define DOUBLE_ALIGN_BITS       2


/* floating point data object alignment */

#define SINGLE_ALIGN_BITS       2
#define FDOUBLE_ALIGN_BITS      3
#define EXTENDED_ALIGN_BITS     4

#define MAX_INSTRUCTION_LENGTH 15

/* Sign extend */
#define SIGN_EXT(xx,b)  (((long)(xx) << (32-(b))) >> (32-(b)))

/* Get Sign */
#define GET_SIGN(xx,b) (((long)(xx) << (32-(b))) >> 31 )


    
#define	X86_EFLAGS_FIELD_CF		0x00000001	/* carry bit			         */
#define	X86_EFLAGS_FIELD_PF		0x00000004	/* parity bit			         */
#define	X86_EFLAGS_FIELD_AF		0x00000010	/* auxiliary carry bit		     */
#define	X86_EFLAGS_FIELD_ZF		0x00000040	/* zero bit			             */
#define	X86_EFLAGS_FIELD_SF		0x00000080	/* negative bit			         */
#define	X86_EFLAGS_FIELD_TF		0x00000100	/* trace enable bit		         */
#define	X86_EFLAGS_FIELD_IF		0x00000200	/* interrupt enable bit		     */
#define	X86_EFLAGS_FIELD_DF		0x00000400	/* direction bit		         */
#define	X86_EFLAGS_FIELD_OF		0x00000800	/* overflow bit			         */
#define	X86_EFLAGS_FIELD_IOPL	0x00003000	/* I/O privilege level		     */
#define	X86_EFLAGS_FIELD_NT		0x00004000	/* nested task flag		         */
#define	X86_EFLAGS_FIELD_RF		0x00010000	/* Reset flag			         */
#define	X86_EFLAGS_FIELD_VM		0x00020000	/* Virtual 86 mode flag		     */
#define X86_EFLAGS_FIELD_AC     0x00040000  /* Alignment Check               */
#define X86_EFLAGS_FIELD_VIF    0x00080000  /* Virtual Interrupt Flag        */
#define X86_EFLAGS_FIELD_VIP    0x00100000  /* Virtual Interrupt Pending     */
#define X86_EFLAGS_FIELD_ID     0x00200000  /* ID Flag                       */

#define X86_NOP_INSTRUCTION 0x90
/** END OF x86 ARCHITECTURE **/
#endif



