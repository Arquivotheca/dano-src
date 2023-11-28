# Generated automatically from config.make.in by configure.
# From $Id: config.make.in,v 1.58 1998/06/26 14:41:45 drepper Exp $.
# Don't edit this file.  Put configuration parameters in configparms instead.

version = 2.0.94
release = experimental

# Installation prefixes.
install_root =
prefix = /usr/local
exec_prefix = ${prefix}
slibdir = 
sysconfdir = 
libexecdir = ${exec_prefix}/libexec
rootsbindir = 

# If ldconfig exists.  This will go away as soon as `ldconfig' is available
# in GNU libc.
has-ldconfig = 

# Maybe the `ldd' script must be rewritten.
ldd-rewrite-script = no

# System configuration.
config-machine = i586
base-machine = i386
config-vendor = pc
config-os = beos
config-sysdirs =  sysdeps/i386/elf sysdeps/beos sysdeps/posix sysdeps/i386/i586 sysdeps/i386/i486 sysdeps/i386/fpu sysdeps/libm-i387 sysdeps/i386 sysdeps/wordsize-32 sysdeps/ieee754 sysdeps/libm-ieee754 sysdeps/generic/elf sysdeps/generic

defines =  -D_LIBC_REENTRANT
sysincludes = 

elf = yes
have-initfini = 
need-nopic-initfini = 
with-cvs = yes
old-glibc-headers = no

versioning = no
no-whole-archive = -Wl,--no-whole-archive
exceptions = -fexceptions

have-bash2 = yes
have-ksh = yes

# Configuration options.
gnu-as = yes
gnu-ld = yes
build-static = yes
build-shared = yes
build-pic-default= no
build-profile = yes
build-omitfp = no
build-bounded = no
build-static-nss = no
stdio = libio
add-ons = 
cross-compiling = no
force-install = yes

# Build tools.
CC = gcc
BUILD_CC = 
CFLAGS = 
AR = ar
RANLIB = :
MAKEINFO = :
AS = $(CC) -c
MIG = mig

# Installation tools.
INSTALL = /bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
LN_S = ln -s
MSGFMT = msgfmt

# Script execution tools.
BASH = /bin/sh
KSH = /bin/sh
AWK = gawk
PERL = /boot/apps/GeekGadgets/bin/perl

# More variables may be inserted below by configure.

override stddef.h = # The installed <stddef.h> seems to be libc-friendly.
