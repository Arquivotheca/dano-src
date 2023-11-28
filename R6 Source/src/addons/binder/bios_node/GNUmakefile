
TARGET_NAME:=bios_node

SRC_DIR:=src/addons/binder/bios_node

SRCS := BIOSNode.cpp

MY_CFLAGS := -DUSE_OFFS=0

MY_INCLUDES:= \
	-I$(BUILDHOME)/headers/www \
	-I$(BUILDHOME)/src/inc/support_p \
	-I$(BUILDHOME)/src/inc/www_p \
	-I$(BUILDHOME)/src/inc/os_p \
	-I.

SYSTEM_LIBS:= libroot.so libbe.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/binder

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
