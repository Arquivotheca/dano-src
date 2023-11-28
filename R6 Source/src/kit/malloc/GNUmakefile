# This little bit of cruft causes libroot to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libroot/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/malloc

MY_CFLAGS:= -D_BUILDING_root=1

MY_INCLUDES:= -I$(BUILDHOME)/src/inc/os_p 


SRCS:=		malloc.c

ifneq ($(call MFLAG_IS_SET, SUPPORTS_MALLOC_DEBUG),)
	SRCS +=	mcheck.c
	# PURIFY_SRCS
	SRCS += avl_tree.c
else
	SRCS += mcheck_stubs.c
endif

include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif
