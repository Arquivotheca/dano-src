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
#	Sub-makefile for string portion of library.
#
subdir	:= string

headers	:= string.h strings.h memory.h endian.h bits/endian.h \
	   byteswap.h bits/byteswap.h bits/string.h \
	   bits/string2.h

routines	:= strcat strchr strcmp strcoll strcpy strcspn		\
		   strlen strdup strnlen stpcpy			\
		   strncat strncmp strncpy				\
		   strrchr strpbrk strspn strstr strtok	\
		   strtok_r strxfrm memchr memcmp memmove \
		   bcopy bzero ffs 		\
		   strcasecmp

tests		:= tester inl-tester noinl-tester testcopy test-ffs	\
		   tst-strlen stratcliff tst-svc tst-inlcall
distribute	:= memcopy.h pagecopy.h tst-svc.expect


include ../Rules

tester-ENV = LANGUAGE=C
inl-tester-ENV = LANGUAGE=C
noinl-tester-ENV = LANGUAGE=C
CFLAGS-noinl-tester.c = -fno-builtin
CFLAGS-tst-strlen.c = -fno-builtin
CFLAGS-stratcliff.c = -fno-builtin
CFLAGS-test-ffs.c = -fno-builtin

ifeq ($(cross-compiling),no)
tests: $(objpfx)tst-svc.out
	cmp tst-svc.expect $(objpfx)tst-svc.out
endif
