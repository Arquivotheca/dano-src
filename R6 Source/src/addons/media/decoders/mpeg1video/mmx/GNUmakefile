# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=mpegmmx.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media/decoders/mpeg1video/mmx

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS:= \
			idct_mmx.asm\
			idct_mmx_3dnow.asm\
			block_mmx.asm\
			mc_mmx8.asm\
			mc_mmx16.asm\
			mc2_mmx8.asm\
			mc2_mmx16.asm\
			mc.cpp \
			ycbcr_mmx.asm \
#

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I. \
			-Icorelibs/mp3dec \
            -Icommon/include \
            -Icommon/util \
            -Itools/synctools/mp3bufferparse
#

# MY_OPTIMIZER - the prefered optimization level


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

