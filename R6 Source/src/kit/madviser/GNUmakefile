# This little bit of cruft causes libroot to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libroot/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/madviser

MY_CFLAGS:= $(WARNINGS_ALL) -D_BUILDING_root=1

MY_INCLUDES:=	-I$(BUILDHOME)/src/inc


SRCS:=		madviser.c



include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif

