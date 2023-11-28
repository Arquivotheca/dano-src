TARGET_NAME:=i810.accelerant

SRC_DIR:=src/addons/accelerants/i810

SRCS		:= hooks.c thunk.c mode.c render.c init.c
SRCS		+= overlay.c

MY_CFLAGS := -DDEBUG=1

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/kit/dinky/include \
	-I$(BUILDHOME)/src/kit/surface/include \
	-I$(BUILDHOME)/src/kit/open_gl/gl_x8 \
	-I$(call FULL_OBJ_DIR, src/kit/opengl/gl_x86) \
#

SYSTEM_LIBS:=libroot.so libdinky.a

MY_DEPS := $(call FULL_OBJ_DIR, src/kit/opengl/gl_x86)/state.h

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

SIGNATURE:= "application/x-vnd.Be-i810.accelerant"

include $(SDIR_TEMPLATES)/AddonTemplate.mk
