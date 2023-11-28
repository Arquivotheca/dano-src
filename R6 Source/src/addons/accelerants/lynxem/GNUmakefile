TARGET_NAME:=lynxem.accelerant

SRC_DIR:=src/addons/accelerants/lynxem

SRCS		:= hooks.c thunk.c render.c init.c

MY_CFLAGS := -DDEBUG=1

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/kit/dinky/include \
	-I$(BUILDHOME)/src/kit/surface/include

SYSTEM_LIBS:=libroot.so libdinky.a

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

LINK_DIRS:=

SIGNATURE:= "application/x-vnd.Be-lynxem.accelerant"

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
