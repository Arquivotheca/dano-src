# Makefile using DataTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

#  FILES - the name of the files to copy. (can not be empty)
SOURCE := g400.accelerant

SIGNATURE	:= "application/x-vnd.Be.g400.accelerant"

# SRC_DIR - the relative path from $(BUILDHOME) of the root of your
#           application source tree. (can not be empty)
SRC_DIR:=src/addons/accelerants/g400

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
TARGET := $(INSTALL_ADD_ONS)/accelerants/$(SOURCE)

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_SYSTEM_DATA)

MY_DEPS := $(MAKEFILE_LEAFNAME)

include $(SDIR_TEMPLATES)/DataFileTemplate.mk

ELF_FILES += $(TARGET_IN)

