/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved. 
***
*** The information and source code contained herein is the exclusive 
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization 
*** from the company.
**/

/*
 * Needs to compile without -traditional for gcc
 */

#define __ASM__


#define	ENTRY(name)				\
	.globl	name;				\
	.align	16;					\
	name:

#define	ENTRY_EXPORT_CODE(name)		\
	.section .exports;			\
	.string #name;				\
	.section .text ;						\
	.globl	name;				\
	.align	16;					\
	name:

#define	ENTRY_EXPORT_DATA(name)		\
	.section .exports;			\
	.string #name;				\
	.section .data ;						\
	.globl	name;				\
	.align	16;					\
	name:

#define	ENTRYNOALIGN(name)				\
	.globl	name;				\
	name:

#define	GLOBAL(name)	.globl	name
#define	EXTERN(name)		name

#define START_PERFORMANCE_COUNTER(name)	\
	rdtsc;								\
	movl %eax,_perfTempl;				\
	movl %edx,_perfTemph;				\

#define STOP_PERFORMANCE_COUNTER(name)	\
	rdtsc;								\
	subl _perfTempl,%eax;				\
	sbbl _perfTemph,%edx;				\
	addl %eax,_cycleCount##name;		\
	movl _hitCount##name,%eax;			\
	inc %eax;							\
	movl %eax,_hitCount##name;			\

#define FLOAT_1_0 1065353216
#define	SARG0	4(%esp)
#define	SARG1	8(%esp)
#define	SARG2	12(%esp)
#define	SARG3	16(%esp)
#define	SARG4	20(%esp)
#define	SARG5	24(%esp)

