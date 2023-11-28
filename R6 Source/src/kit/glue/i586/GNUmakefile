# Makefile using CompileObjectsTemplate.mk

SRC_DIR		:= src/kit/glue/i586

SRCS :=  \
		crti.S \
		crtn.S \
		i386-mcount.S

MY_CFLAGS:= -D_BUILDING_glue

MY_INCLUDES:=	\
				-I$(BUILDHOME)/src/kit/runtime/common \
				-I$(BUILDHOME)/src/kit/runtime/$(TARGET_CPU) \
				-I$(BUILDHOME)/src/nukernel/cpu/$(TARGET_CPU) \
				-I$(BUILDHOME)/src/inc/os_p

MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)

OBJ_DIR := $(CREATED_LIBRARY_PATH)
MY_TRASH_ID := glue/i586

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

ifneq ($(call MFLAG_IS_SET, SUPPORTS_PROFILING),)
install: $(filter %-mcount.o,$(OBJS_OUT))
endif

