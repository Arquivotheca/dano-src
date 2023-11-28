# This little bit of cruft causes libroot to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libroot/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/kernel

MY_DFLAGS:= -D_BUILDING_root=1

MY_INCLUDES:=	-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/support_p \
				-I$(BUILDHOME)/srcx/c/ANSI/Private_Headers \
				-I$(BUILDHOME)/src/nukernel/cpu/i586

# Support profiling in any build with FORCE_MFLAGS=SUPPORTS_PROFILING
# Only libroot_start.c needs this CFLAG set
ifneq ($(call MFLAG_IS_SET, SUPPORTS_PROFILING),)
MY_CFLAGS := -D_SUPPORTS_PROFILING=1 
endif

SRCS:= $(addprefix common/, \
	Debug.cpp \
	byteorder.c \
	environ.c \
	find_dir.c \
	fscalls.c \
	gen_malloc.c \
	image.c \
	libroot_start.c \
	mainlib.c \
	scheduler.c \
	start_thread.c \
)

SRCS+= $(addprefix i586/, \
	byteorder_x86.S \
	glue.S \
	misc.c \
	perfmon_user.S \
	scalls.S \
	scalls_gen.c \
	tls.c \
)

include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif
