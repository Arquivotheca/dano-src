
TARGET_NAME:=net_endpoint_node

#SRC_DIR:=dsandler/addons/binder/net_endpoint_node
SRC_DIR:=src/addons/binder/net_endpoint_node

SRCS := NetServerNode.cpp NetEndpointNode.cpp NetSwitchboardNode.cpp

MY_CFLAGS := 

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/os_p \
#	-I.

SYSTEM_LIBS:= libroot.so libbe.so libbnetapi.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/binder

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=

MY_OPTIMIZER:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk

