
TARGET_NAME:= cifsmount
SRC_DIR := src/commands/be/$(TARGET_NAME)

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).c nmblookup.c

MY_CFLAGS:= 

MY_INCLUDES	:= \
			   -I$(BUILDHOME)/src/inc/bdb_p

SYSTEM_LIBS:= libbe.so libroot.so \
	libbind.so libsocket.so

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk

# Makefile using DataTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

#  FILES - the name of the files to copy. (can not be empty)
FILES := ezmount

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=$(SRC_DIR_IN)

# MY_INSTALL_DIR - the full path of the directory you want
#   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk.
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BIN)

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_SYSTEM_APP)

include $(SDIR_TEMPLATES)/DataTemplate.mk

