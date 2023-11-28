/* Low-level statistical profiling support function.  Linux/ARM version.
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
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

#include <signal.h>
#include <asm/ptrace.h>

union k_sigcontext
  {
    struct
      {
	unsigned long int trap_no;
	unsigned long int error_code;
	unsigned long int oldmask;
	unsigned long int arm_r0;
	unsigned long int arm_r1;
	unsigned long int arm_r2;
	unsigned long int arm_r3;
	unsigned long int arm_r4;
	unsigned long int arm_r5;
	unsigned long int arm_r6;
	unsigned long int arm_r7;
	unsigned long int arm_r8;
	unsigned long int arm_r9;
	unsigned long int arm_r10;
	unsigned long int arm_fp;
	unsigned long int arm_ip;
	unsigned long int arm_sp;
	unsigned long int arm_lr;
	unsigned long int arm_pc;
	unsigned long int arm_cpsr;
      } v21;
    struct
      {
	unsigned long int magic;
	struct pt_regs reg;
	unsigned long int trap_no;
	unsigned long int error_code;
	unsigned long int oldmask;
      } v20;
};

void
profil_counter (int signo, int _a2, int _a3, int _a4, union k_sigcontext sc)
{
  /* The format of struct sigcontext changed between 2.0 and 2.1 kernels.
     Fortunately 2.0 puts a magic number in the first word and this is not
     a legal value for `trap_no', so we can tell them apart.  */

  void *pc;
  if (sc.v20.magic == 0x4B534154)
    pc = (void *) sc.v20.reg.ARM_pc;
  else
    pc = (void *) sc.v21.arm_pc;
  profil_count (pc);
}
