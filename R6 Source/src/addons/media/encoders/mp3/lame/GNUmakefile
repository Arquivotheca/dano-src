# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libmp3lame.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media/encoders/mp3/lame

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS:= \
	brhist.c \
	bitstream.c \
	fft.c \
	get_audio.c \
	id3tag.c \
	ieeefloat.c \
	lame.c \
	newmdct.c \
	parse.c \
	portableio.c \
	psymodel.c \
	quantize.c \
	quantize-pvt.c \
	vbrquantize.c \
	reservoir.c \
	tables.c \
	takehiro.c \
	timestatus.c \
	util.c \
	vorbis_interface.c \
	VbrTag.c \
	version.c
#

MY_DFLAGS := -DNDEBUG -D__NO_MATH_INLINES

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
#

MY_OPTIMIZER := -O1 -fomit-frame-pointer -march=pentium \
   -mcpu=pentium -ffast-math -funroll-loops

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

