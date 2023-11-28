
TARGET_NAME:=user_node

SRC_DIR:=src/addons/binder/user_node

SRCS := UserNode.cpp UserRootNode.cpp

MY_CFLAGS := -DUSE_OFFS=0

MY_INCLUDES:= \
	-I$(BUILDHOME)/headers/www \
	-I$(BUILDHOME)/headers/xml \
	-I$(BUILDHOME)/src/inc/support_p \
	-I$(BUILDHOME)/src/inc/www_p \
	-I.

SYSTEM_LIBS:= libroot.so libbe.so libxml.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/binder

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
