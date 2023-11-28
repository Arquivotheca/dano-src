# Makefile using CompileObjectsTemplate.mk

SRC_DIR		:= src/kit/glue/common

SRCS :=  \
		init_term_dyn.c \
		start_dyn.c \

MY_CFLAGS:= -D_BUILDING_glue

MY_INCLUDES:=	\
				-I$(BUILDHOME)/src/kit/runtime/common \
				-I$(BUILDHOME)/src/kit/runtime/$(TARGET_CPU) \
				-I$(BUILDHOME)/src/nukernel/cpu/$(TARGET_CPU) \
				-I$(BUILDHOME)/src/inc/os_p

OBJ_DIR := $(CREATED_LIBRARY_PATH)
MY_TRASH_ID := glue/common

MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

