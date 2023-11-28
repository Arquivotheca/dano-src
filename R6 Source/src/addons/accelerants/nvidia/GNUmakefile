TARGET_NAME:=nvidia.accelerant

SRC_DIR:=src/addons/accelerants/nvidia

SRCS		:= nvidia.c thunk.c accel.c overlay.c

MY_CFLAGS := -DDEBUG=1 -DCOMPILING_ACCELERANT

MY_INCLUDES:= \
		-I$(BUILDHOME)/src/inc \
		   -I$(BUILDHOME)/src/kit/dinky/include \
		   -I$(BUILDHOME)/src/kit/surface/include

SYSTEM_LIBS:=libroot.so libdinky.a

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

SIGNATURE	:= "application/x-vnd.Be-nvidia.accelerant"

include $(SDIR_TEMPLATES)/AddonTemplate.mk
