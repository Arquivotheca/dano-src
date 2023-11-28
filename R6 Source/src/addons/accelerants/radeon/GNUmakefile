TARGET_NAME:=radeon.accelerant

SRC_DIR:=src/addons/accelerants/radeon

SRCS		:= core.c init.c memMgrInc.c cursor.c initengine.c 2d.c 3d.c cce.c R6mer.c

MY_CFLAGS		:= -DDEBUG=1 -DCOMPILING_ACCELERANT


MY_INCLUDES:= \
		-I$(SRC_DIR_OUT_FROM_SRC_DIR)

MY_INCLUDES 	+= -I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/kit/surface/include

SYSTEM_LIBS:=libroot.so libbe.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

SIGNATURE	:= "application/x-vnd.Be-radeon.accelerant"

include $(SDIR_TEMPLATES)/AddonTemplate.mk
