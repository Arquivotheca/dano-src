//==============================================================================
//  Check running on MMX processor.
//
//  Checking for ability to set/clear ID flag (Bit 21) in EFLAGS
//  which indicates the presence of a processor with the ability
//  to use the CPUID instruction. Then check CPUID.
//
//  Copyright(C) 1997 CANOPUS Co.,Ltd.  <<<<< Don't change copyright!
//  1997/1/27 Tom created.		<<<<< Don't change originality!
//
//  <!!! WARNING !!!>
//  Don't change original comments. It's the morality for programmer.
//  Don't modify a lot. You can modify minimum.
//
//==============================================================================

#define	CPU_PRO		0		// don't use.
#define	CPU_MMX		1
#define	CPU_XMM		2

int PASCAL GetCpuType( void );		// TRUE if MMX processor.

