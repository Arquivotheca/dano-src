TARGET_NAME:=trident.accelerant

SRC_DIR:=src/addons/accelerants/trident

SRCS		:= trident.c thunk.c accel.c

MY_CFLAGS := -DCOMPILING_ACCELERANT

MY_INCLUDES:= -I$(BUILDHOME)/src/inc \
		   -I$(BUILDHOME)/src/kit/dinky/include \
		   -I$(BUILDHOME)/src/kit/surface/include

SYSTEM_LIBS:=libdinky.a libroot.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

LINK_DIRS:=

SIGNATURE:= "application/x-vnd.Be-trident.accelerant"

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
