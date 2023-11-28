# Makefile using SimpleSharedLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libmpeg2video.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media2/decoders/mpeg2video/libmpeg2video

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS:=\
		mpeg2video.c		\
		mpeg2bitstream.c	\
		mpeg2video_seq_header.c	\
		mpeg2video_picture.c \
		mpeg2video_slice.c \
		mpeg2video_motion_compensation.c \
		mpeg2video_motion_vectors.c \
		mpeg2video_block.c \
		mpeg2video_extension.c \
		SmartBitStream_mmx.asm \
#

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
				-I$(call FULL_SRC_DIR, $(SRC_DIR))/../mpeglib-multiplatform \
#

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. 
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
MY_INSTALL_DIR := $(INSTALL_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.so
# LINKS :=

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

