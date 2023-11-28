/* ++++++++++

	hobbit.h

	Copyright (C) 1991 Be Labs, Inc.  All Rights Reserved.


	Definitions for the hobbit processor.

	Mod history (reverse chron):

	08 apr 91	rwh		moved reg#'s to asm.h, added psw, config masks.
	?? feb 91	s?s		created

+++++ */

#ifndef _HOBBIT_H
#define _HOBBIT_H

/* psw bit definitions */

#define pswVPmask	0x00010000
#define pswULmask	0x00008000
#define pswIPLmask	0x00007000
#define pswEmask	0x00000800
#define pswXmask	0x00000400
#define pswSmask	0x00000200
#define pswTBmask	0x00000100
#define pswTImask	0x00000080
#define pswVmask	0x00000040
#define pswCmask	0x00000020
#define pswFmask	0x00000010

#define pswIPL0		0x00000000
#define pswIPL1		0x00001000
#define pswIPL2		0x00002000
#define pswIPL3		0x00003000
#define pswIPL4		0x00004000
#define pswIPL5		0x00005000
#define pswIPL6		0x00006000
#define pswIPL7		0x00007000


/* configuration register bit definitions */

#define configT2mask	0xFE000000
#define configT1mask	0x01C00000
#define configPMmask	0x00200000
#define configPEmask	0x00100000
#define configIEmask	0x00080000
#define configSEmask	0x00040000
#define configPXmask	0x00020000
#define configKLmask	0x00010000

/* timer configuration bit masks */

#define t1CountCycles		0x00000000
#define t1CountInstructions	0x00400000
#define t1CountAlways		0x00000000
#define t1CountInUserMode	0x00800000
#define t1InterruptEnable	0x01000000

#define t2CountCycles		0x00000000
#define t2CountInstructions	0x02000000
#define t2DoNotCount		0x3E000000
#define t2CountAlways		0x00000000
#define t2CountInUserMode	0x40000000
#define t2InterruptEnable	0x80000000


/* vector table entry numbers */

#define vectorNumKcall			0
#define vectorNumException		1
#define vectorNumNiladic		2
#define vectorNumUnimplemented		3
#define vectorNumNMI			4
#define vectorNumInterrupt1		5
#define vectorNumInterrupt2		6
#define vectorNumInterrupt3		7
#define vectorNumInterrupt4		8
#define vectorNumInterrupt5		9
#define vectorNumInterrupt6		10
#define vectorNumTimer1			11
#define vectorNumTimer2			12
#define vectorNumFloatException		13

/* exception identifiers */

#define exceptionZeroDivide		1
#define exceptionTrace			2
#define exceptionIllegalInstruction	3
#define exceptionAlignmentFault		4
#define exceptionPrivilegeViolation	5
#define exceptionUnimplementedReg	6
#define exceptionFetchFault		7
#define exceptionReadFault		8
#define exceptionWriteFault		9
#define exceptionTextBusError		10
#define exceptionDataBusError		11

/* mmu definitions */

#define PAGESIZE		(4096)
#define segNum(addr) (addr>>22)
#define segOffset(addr) (addr&0x003FFFFF)

#define pageNum(addr) ((addr&0x003FF000)>>12)
#define pageOffset(addr) (addr&0x00000FFF)

#define segVmask		0x00000001
#define segWmask		0x00000002
#define segUmask		0x00000004
#define segSmask		0x00000008
#define segCmask		0x00000800

#define segBaseMask		0xFFC00000
#define segBoundMask		0x003FF000
#define segPageTableMask	0xFFFFF000
#define segBound(segEntry)	((segEntry&0x003FF000)>>12)

#define pageVmask		0x00000001
#define pageWmask		0x00000002
#define pageUmask		0x00000004
#define pageRmask		0x00000008
#define pageMmask		0x00000010
#define pageCmask		0x00000800

#define pageFrameMask		0xFFFFF000

#endif

