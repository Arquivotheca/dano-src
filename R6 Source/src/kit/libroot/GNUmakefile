
TARGET_NAME := libroot.so

TYPE := SHARED

SRC_DIR:= src/kit/libroot

MY_INSTALL_DIR:= $(INSTALL_LIBS)

# XXX
MY_DIRS := glibc

# FULL_NAMES accepts absolute arguments in it's list.
MY_DIRS += $(call FULL_NAMES, \
	kernel \
	nuposix \
	runtime \
	$(SRC_BUILDHOME)/srcx/c-extra \
	malloc \
	madviser \
, $(SRC_BUILDHOME)/src/kit)

# This is used in EXP
ifeq ($(TARGET_CPU),i586)
SYSTEM_LIBS := librootfake.so
endif

include $(SDIR_TEMPLATES)/ComplexTemplate.mk

# absolute link to /boot/beos/system/lib
SOURCE	:= /boot/beos/system/lib/libroot.so

SRC_DIR := src/kit/libroot

LINK_PREFIX		:=
LINK_RELATION	:=
LINKS			:=$(INSTALL_DEV_LIBS)/libroot.so
MY_DEPS			:=$(MAKEFILE_LEAFNAME)

include $(SDIR_TEMPLATES)/SymlinkTemplate.mk

# Makefile using DataTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

#  FILES - the name of the files to copy. (can not be empty)
SOURCE := gnu-versions.h

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR :=src/kit/glibc/include

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
TARGET := $(INSTALL_DEV_GNU_HEADERS)/$(SOURCE)

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_SYSTEM_DATA)

include $(SDIR_TEMPLATES)/DataFileTemplate.mk

