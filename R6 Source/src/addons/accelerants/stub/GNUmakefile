TARGET_NAME:=stub.accelerant

SRC_DIR:=src/addons/accelerants/stub

SRCS		:= accelerant.c

MY_CFLAGS := -DCOMPILING_ACCELERANT

MY_INCLUDES:= \
		-I$(BUILDHOME)/src/inc \
		-I$(BUILDHOME)/src/inc/driver \
		-I$(BUILDHOME)/src/inc/os_p

SYSTEM_LIBS:=libroot.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

LINK_DIRS:=

SIGNATURE:= "application/x-vnd.Be-stub.accelerant"

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
