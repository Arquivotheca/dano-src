# Copyright (C) 1991, 92, 93, 94, 95, 96, 97, 98 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU Library General Public
# License along with the GNU C Library; see the file COPYING.LIB.  If not,
# write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

#
#	Makefile for signal routines.
#
subdir	:= signal

headers	:= signal.h sys/signal.h bits/signum.h bits/sigcontext.h \
	   bits/sigaction.h bits/sigset.h bits/siginfo.h bits/sigstack.h \
	   bits/sigthread.h

routines	:= signal raise \
		   sigprocmask \
		   sigsetops sigempty sigfillset sigaddset sigdelset sigismem \
		   sysv_signal

tests		:= tst-signal

distribute	:= sigsetops.h testrtsig.h sigset-cvt-mask.h


include ../Rules
