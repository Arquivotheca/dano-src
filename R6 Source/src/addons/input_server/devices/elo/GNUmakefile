TARGET_NAME:=elo

SRC_DIR:=src/addons/input_server/devices/$(TARGET_NAME)

SRCS		:= elo.cpp controls.cpp utils.cpp

MY_CFLAGS := 

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/headers/add-ons/input_server \
	-I$(BUILDHOME)/src/inc/media_p


SYSTEM_LIBS:= libroot.so libbe.so libdevice.so libmedia.so

PARENT_IMAGE:= $(INPUT_SERVER_OBJECT)

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/input_server/devices

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
