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
#	Sub-makefile for misc portion of the library.
#

subdir	:= misc

headers	:= sys/uio.h bits/uio.h sys/ioctl.h bits/ioctls.h bits/ioctl-types.h \
	   sys/file.h sys/dir.h sys/cdefs.h \
	   ar.h a.out.h stab.h bits/stab.def \
	   ttyent.h paths.h sys/reboot.h sys/select.h \
	   sys/param.h fstab.h err.h \
	   sys/queue.h sysexits.h syscall.h sys/syscall.h sys/swap.h \
	   sys/sysinfo.h \
	   bits/select.h

routines := brk \
	    mktemp mkstemp \
	    efgcvt efgcvt_r
	    
aux := init-misc
#install-lib := libbsd-compat.a libg.a
gpl2lgpl := error.c error.h

tests := tst-dirname tst-tsearch tst-fdset tst-efgcvt tst-mntent

CFLAGS-tsearch.c = $(exceptions)
CFLAGS-lsearch.c = $(exceptions)

include ../Rules

#$(objpfx)libbsd-compat.a: $(dep-dummy-lib); $(make-dummy-lib)
#$(objpfx)libg.a: $(dep-dummy-lib); $(make-dummy-lib)

CFLAGS-init-misc.c = -fkeep-inline-functions

ifeq ($(build-shared),yes)
$(objpfx)tst-tsearch: $(common-objpfx)math/libm.so$(libm.so-version)
else
$(objpfx)tst-tsearch: $(common-objpfx)math/libm.a
endif
