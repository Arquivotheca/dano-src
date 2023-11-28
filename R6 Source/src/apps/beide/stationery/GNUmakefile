
# Makefile using DataTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

ifeq ($(TARGET_PLATFORM), intel)
BEIDE_PLATFORM_DIR := x86
BEIDE_XDEV_PREFIX := ppc
else
BEIDE_PLATFORM_DIR := ppc
BEIDE_XDEV_PREFIX := x86
endif

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/beide/stationery/$(BEIDE_PLATFORM_DIR)

dirs := $(addprefix $(call FULL_SRC_DIR, $(SRC_DIR))/, \
	BeApp \
	BeEverythingApp	\
	SharedLibrary \
	StaticLibrary \
	BeSTL \
	KernelDriver \
	$(BEIDE_XDEV_PREFIX)_BeApp \
)

#  FILES - the name of the files to copy. (can not be empty)
FILES := $(wildcard \
	$(addsuffix /*.proj, $(dirs)) \
	$(addsuffix /*.c, $(dirs)) \
	$(addsuffix /*.h, $(dirs)) \
	$(addsuffix /*.cpp, $(dirs)) \
	$(addsuffix /*.rsrc, $(dirs)) \
)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BEIDE_STATIONERY)/$(BEIDE_PLATFORM_DIR)

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_SYSTEM_DATA)

MIMESET_FILES := true

include $(SDIR_TEMPLATES)/DataTemplate.mk

