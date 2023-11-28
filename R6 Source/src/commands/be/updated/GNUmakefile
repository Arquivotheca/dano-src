#
# begin build for updated script
#
SOURCE := updated
SRC_DIR := src/commands/be/updated/scripts
TARGET := $(INSTALL_BIN)/updated
PERMISSIONS := $(PERMS_SYSTEM_APP)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk         


# updated makefile
TARGET_NAME:= updated-bin
SRC_DIR := src/commands/be/updated
RESOURCE_FORK:=updated.rsrc
MY_INSTALL_DIR:=$(INSTALL_BIN)
SRCS := updated.cpp MCABinderNode.cpp
MY_CFLAGS:=
MY_INCLUDES	:= -I$(BUILDHOME)/headers/www
SYSTEM_LIBS:= libroot.so libbe.so libwww.so libsettings.so
INK_DIRS:=
include $(SDIR_TEMPLATES)/AppTemplate.mk

#
# end updated script build

SRC_DIR := $(SRC_DIR_IN)

FILES:= cookie
MY_INSTALL_DIR := $(INSTALL_UPDATE)
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataTemplate.mk

SRC_DIR := $(SRC_DIR_IN)

 
#
# PLEASE NOTE: AT THIS POINT SRC_DIR IS NO LONGER CONSTANT (steven)
#

SRC_DIR := src/commands/be/updated/scripts
MY_INSTALL_DIR := $(INSTALL_UPDATE)
FILES:= \
	updt \
	updt.html \
	doupdate \
	doupgrade \
	doscript \
	updated

PERMISSIONS:=$(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataTemplate.mk

SRC_DIR := src/commands/be/updated/recovery
MY_INSTALL_DIR := $(INSTALL_UPDATE)/recovery
FILES:= \
	Bootscript \
	dhclient-script \
	kernel \
	recovery.data.english \
	recovery.data.french \
	recovery.script \
	recovery.txt
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataTemplate.mk

SRC_DIR := src/commands/be/updated/keys
SOURCE := $(call MVAR_TO_TARGET_NAME, UPDATE_KEY, master.pub)
TARGET := $(INSTALL_UPDATE_KEYS)/master.pub
MY_DEPS:=$(call SRC_DIR_OUT_FROM_ARG, src/commands/be/updated)/$(MAKEFILE_LEAFNAME)
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR := src/commands/be/updated/keys
SOURCE := $(call MVAR_TO_TARGET_NAME, UPDATE_PARAMS, params)
TARGET := $(INSTALL_UPDATE_KEYS)/params
MY_DEPS:=$(call SRC_DIR_OUT_FROM_ARG, src/commands/be/updated)/$(MAKEFILE_LEAFNAME)
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR := src/commands/be/updated/update_url
SOURCE := $(call MVAR_TO_TARGET_NAME, UPDATE_UPDATE_URL, update_url.generic)
TARGET:= $(INSTALL_UPDATE)/update_url
ATTRIBUTES :=
PERMISSIONS := $(PERMS_SYSTEM_DATA)
MY_DEPS:=$(call SRC_DIR_OUT_FROM_ARG, src/commands/be/updated)/$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR := src/commands/be/updated/revisions
SOURCE := $(call MVAR_TO_TARGET_NAME, UPDATE_REVISION, revision.generic)
TARGET:= $(INSTALL_UPDATE)/revision
ATTRIBUTES :=
PERMISSIONS := $(PERMS_SYSTEM_DATA)
MY_DEPS:=$(call SRC_DIR_OUT_FROM_ARG, src/commands/be/updated)/$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk



