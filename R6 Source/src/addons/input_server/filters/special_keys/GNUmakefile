TARGET_NAME:=special_keys

SRC_DIR:=src/addons/input_server/filters/special_keys

SRCS		:= 	special_keys.cpp volume_window.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(BUILDHOME)/headers/add-ons/input_server

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

PARENT_IMAGE:= $(INPUT_SERVER_OBJECT)

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/input_server/filters

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

# special keys scripts

ifneq ($(call MVAR_EXISTS, SPECIAL_KEYS),)

SRC_DIR:=src/addons/input_server/filters/special_keys/scripts

SOURCE:= $(call MVAR_TO_TARGET_NAME, SPECIAL_KEYS)

MY_INSTALL_DIR:=$(INSTALL_CUSTOM)/special_keys

PERMISSIONS:=$(PERMS_COMMON_APP)
include $(SDIR_TEMPLATES)/DataDirTemplate.mk

endif
