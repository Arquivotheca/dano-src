TARGET_NAME:=keyboard

SRC_DIR:=src/addons/input_server/devices/keyboard

SRCS		:= keyboard.cpp kb_device.cpp ktable.cpp

MY_CFLAGS := -DUSE_OFFS=0

ifneq ($(call MFLAG_IS_SET, SUPPORTS_TASK_MANAGER),)
	SRCS	+=	TeamMonitorWindow.cpp
	MY_CFLAGS += -DSUPPORTS_TASK_MANAGER=1
endif


MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/inc/os_p \
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

# copy AT_Keyboard_Info

SRC_DIR:=src/addons/input_server/devices/keyboard/kb_maps

SOURCE:= $(call MVAR_TO_TARGET_NAME, USER_SETTING_AT_KEYBOARD_INFO, AT_Keyboard_Info.generic)
TARGET:=$(INSTALL_USER_SETTINGS)/AT_Keyboard_Info
ATTRIBUTES:=
PERMISSIONS:=$(PERMS_USER_DATA)

include $(SDIR_TEMPLATES)/DataFileTemplate.mk

