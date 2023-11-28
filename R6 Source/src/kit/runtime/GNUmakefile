# This little bit of cruft causes libroot to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libroot/${MAKEFILE_LEAFNAME}
else


SRC_DIR:=src/kit/runtime

MY_CFLAGS:= $(FORCE_EXCEPTIONS)  -D_BUILDING_root=1

MY_INCLUDES:=	-I$(BUILDHOME)/$(SRC_DIR)/common \
				-I$(BUILDHOME)/$(SRC_DIR)/i586 \
				-I$(BUILDHOME)/src/inc/support_p

ifeq ($(call MVAR_TO_TARGET_NAME, COMPILER_VERSION, gcc2),gcc2) 
	SRCS:=	i586/new.cpp
endif

ifneq ($(call MFLAG_IS_SET, SUPPORTS_LEAK_CHECKING),)
	SRCS+= common/LeakChecker.cpp
endif

ifneq ($(call MFLAG_IS_SET, SUPPORTS_PROFILING),)
	SRCS+= common/mcount.c
	MY_INCLUDES+= -I$(BUILDHOME)/src/inc/os_p 
endif

include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

# There is nothing about this section that I (steven) like. (2001-04-05)

LIBGCC_OBJS:= $(addprefix $(call FULL_OBJ_DIR, src/kit/runtime)/, $(shell $(AR) t $(LIBGCC) | grep '[^new.o]'))

$(COMPLEX_TARGET): $(LIBGCC_OBJS)

$(LIBGCC_OBJS): $(LIBGCC)
	$(call PrepareDir, Extracting)
	$(AR) p $(LIBGCC) $(notdir $@) > $@

endif
