# This little bit of cruft causes libroot to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libroot/${MAKEFILE_LEAFNAME}
else

SRC_DIR:=src/kit/nuposix

MY_DFLAGS:= -D_BUILDING_root=1

MY_INCLUDES:=	-I$(BUILDHOME)/src/nukernel/inc \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/srcx/c/ANSI/Private_Headers

SRCS:=	errno.c \
		posix.c \
		pwd.c \
		fork.c \
		exec.c \
		wait.c \
		ttyname.c

ifeq ($(TARGET_CPU), ppc) 
	SRCS += popen.c
endif

include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif
