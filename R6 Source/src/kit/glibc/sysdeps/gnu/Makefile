# Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License
# as published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU Library General Public
# License along with the GNU C Library; see the file COPYING.LIB.  If not,
# write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

# Generate the list of strings for errno codes from the section of the
# manual which documents the codes.

$(..)sysdeps/gnu/errlist.c: $(..)sysdeps/gnu/errlist.awk \
			    $(..)manual/errno.texi
	$(AWK) -f $^ > $@-tmp
# Make it unwritable so noone will edit it by mistake.
	-chmod a-w $@-tmp
	mv -f $@-tmp $@
ifeq ($(with-cvs),yes)
	test ! -d CVS || cvs commit -m'Regenerated from $^' $@
endif

ifeq ($(subdir),login)
sysdep_routines += setutxent getutxent endutxent getutxid getutxline \
                   pututxline utmpxname updwtmpx

sysdep_headers += utmpx.h bits/utmpx.h
endif
