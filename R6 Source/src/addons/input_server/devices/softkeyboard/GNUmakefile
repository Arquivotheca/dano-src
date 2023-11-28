TARGET_NAME:=softkeyboard

SRC_DIR:=src/addons/input_server/devices/$(TARGET_NAME)

SRCS		:= SoftKeyboardDevice.cpp

MY_CFLAGS := -DUSE_OFFS=0

MY_INCLUDES:= \
	-I$(BUILDHOME)/headers/add-ons/input_server

SYSTEM_LIBS:= libroot.so libbe.so

PARENT_IMAGE:= $(INPUT_SERVER_OBJECT)

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/input_server/devices

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
