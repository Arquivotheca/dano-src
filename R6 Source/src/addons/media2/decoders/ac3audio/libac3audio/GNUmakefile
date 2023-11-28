# Makefile using SimpleSharedLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libac3audio.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media2/decoders/ac3audio/libac3audio

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS:=\
		ac3audio.c \
		ac3audio_alloc_bits.c \
		ac3audio_audblk.c \
		ac3audio_bsi.c \
		ac3audio_dec_exponents.c \
		ac3audio_dec_mantissas.c \
		ac3audio_downmix.c \
		ac3audio_imdct.c \
		ac3audio_rematrix.c \
		ac3audio_uncouple.c \
		../../mpeg2video/libmpeg2video/mpeg2bitstream.c \
		../../mpeg2video/libmpeg2video/SmartBitStream_mmx.asm \
#

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
				-I$(call FULL_SRC_DIR, $(SRC_DIR))/../../mpeg2video/libmpeg2video \
				-I$(call FULL_SRC_DIR, $(SRC_DIR))/../../mpeg2video/mpeglib-multiplatform \
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

