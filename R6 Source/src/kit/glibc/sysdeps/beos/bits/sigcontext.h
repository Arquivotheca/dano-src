/* Structure describing state saved while handling a signal.  Stub version.
   Copyright (C) 1991, 1994, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _SIGNAL_H
# error "Never use <bits/sigcontext.h> directly; include <signal.h> instead."
#endif

#if __INTEL__

#include <SupportDefs.h>

/* State of the FPU.  */
typedef struct packed_fp_stack
{
	uint8	st0[10];
	uint8	st1[10];
	uint8	st2[10];
	uint8	st3[10];
	uint8	st4[10];
	uint8	st5[10];
	uint8	st6[10];
	uint8	st7[10];
} packed_fp_stack;

typedef struct packed_mmx_regs
{
	uint8	mm0[10];
	uint8	mm1[10];
	uint8	mm2[10];
	uint8	mm3[10];
	uint8	mm4[10];
	uint8	mm5[10];
	uint8	mm6[10];
	uint8	mm7[10];
} packed_mmx_regs;


typedef struct old_extended_regs
{
	uint16	fp_control;
	uint16	_reserved1;
	uint16	fp_status;
	uint16	_reserved2;
	uint16	fp_tag;
	uint16	_reserved3;
	uint32	fp_eip;
	uint16	fp_cs;
	uint16	fp_opcode;
	uint32	fp_datap;
	uint16	fp_ds;
	uint16	_reserved4;
	union {
		packed_fp_stack	fp;
		packed_mmx_regs	mmx;
	} fp_mmx;
} old_extended_regs;

typedef struct fp_stack
{
	uint8	st0[10];
	uint8	_reserved_42_47[6];
	uint8	st1[10];
	uint8	_reserved_58_63[6];
	uint8	st2[10];
	uint8	_reserved_74_79[6];
	uint8	st3[10];
	uint8	_reserved_90_95[6];
	uint8	st4[10];
	uint8	_reserved_106_111[6];
	uint8	st5[10];
	uint8	_reserved_122_127[6];
	uint8	st6[10];
	uint8	_reserved_138_143[6];
	uint8	st7[10];
	uint8	_reserved_154_159[6];
} fp_stack;

typedef struct mmx_regs
{
	uint8	mm0[10];
	uint8	_reserved_42_47[6];
	uint8	mm1[10];
	uint8	_reserved_58_63[6];
	uint8	mm2[10];
	uint8	_reserved_74_79[6];
	uint8	mm3[10];
	uint8	_reserved_90_95[6];
	uint8	mm4[10];
	uint8	_reserved_106_111[6];
	uint8	mm5[10];
	uint8	_reserved_122_127[6];
	uint8	mm6[10];
	uint8	_reserved_138_143[6];
	uint8	mm7[10];
	uint8	_reserved_154_159[6];
} mmx_regs;

	
typedef struct xmmx_regs
{
	uint8	xmm0[16];
	uint8	xmm1[16];
	uint8	xmm2[16];
	uint8	xmm3[16];
	uint8	xmm4[16];
	uint8	xmm5[16];
	uint8	xmm6[16];
	uint8	xmm7[16];
} xmmx_regs;


typedef struct new_extended_regs
{
	uint16	fp_control;
	uint16	fp_status;
	uint16	fp_tag; /* This is a different format from
					 * old_extended_regs.fp_tag
					 */
	uint16	fp_opcode;
	uint32	fp_eip;
	uint16	fp_cs;
	uint16	res_14_15;
	uint32	fp_datap;
	uint16	fp_ds;
	uint16	_reserved_22_23;
	uint32	mxcsr;
	uint32	_reserved_28_31;
	union {
		fp_stack fp;
		mmx_regs mmx;
	} fp_mmx;
	xmmx_regs xmmx;
	uint8	_reserved_288_511[224];
} new_extended_regs;

typedef struct extended_regs
{
	union {
		old_extended_regs	old_format;
		new_extended_regs	new_format;
	} state;
	uint32	format; /* 0 == old_format;  1 == new_format */
} extended_regs;

typedef struct vregs {
	uint32			eip;
	uint32			eflags;
	uint32			eax;
	uint32			ecx;
	uint32			edx;
	uint32			esp;
	uint32			ebp;
	uint32			_reserved_1;
	extended_regs	xregs;
	uint32			_reserved_2[3];
} vregs;

#endif /* __INTEL__ */

/* State of this thread when the signal was taken.  */
struct sigcontext
  {
    int sc_onstack;
    __sigset_t sc_mask;

    /* Registers and such.  */
  };

/* Signal subcodes should be defined here.  */
