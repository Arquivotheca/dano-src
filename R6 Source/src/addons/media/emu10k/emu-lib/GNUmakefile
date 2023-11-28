# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=emu10k.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media/emu10k/emu-lib

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	fxcbkman.c \
	fxparman.c \
	fxpatman.c \
	fxpgmman.c \
	fxresman.c \
	fxsystem.c \
	hal8210.c \
	hrb8210.c \
	hrm8210.c \
	ip8210.c \
	itm8210.c \
	mxr8210.c \
	se8210.c \
	sm8210.c \
	osbeos.c

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

MY_DFLAGS += -DBUILDING_LIB=1

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/inc/media_p
#

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. 
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.a
# LINKS :=

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

