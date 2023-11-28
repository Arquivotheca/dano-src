TARGET_NAME:=cop8

SRC_DIR:=src/addons/input_server/devices/$(TARGET_NAME)

SRCS		:= cop8.cpp

MY_CFLAGS := -DUSE_OFFS=0

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/headers/add-ons/input_server \
	-I$(BUILDHOME)/src/servers/input

SYSTEM_LIBS:= libroot.so libbe.so libdevice.so

PARENT_IMAGE:= $(INPUT_SERVER_OBJECT)

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/input_server/devices

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
