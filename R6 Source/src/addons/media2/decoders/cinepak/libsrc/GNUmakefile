# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# DIBS is bit switches for turning on support of peculiar DIBs:
#   1	support of normal DIBs (switchability not particularly tested)
#	  8 bpp palettized
#	  8 bpp palettized, doubled
#	  RGB555
#	  RGB565
#	  RGB888
#	  RGBa8888
#   2	support of CPLA, Weitek YUV420 DIBs
#   4	support of YUY2, various YUV422
#   8	support of UYVY, Cirrus Logic 5440 YUV422 DIBs
#  16	support of CLJR, Cirrus Logic AcuPak DIBs
DIBS :=		5

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libcinepak.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media2/decoders/cinepak/libsrc

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := iccv.c cvdecomp.c cvcompre.c \
	beos.cpp keycodeb.c converge.c matchand.c \
	makeneig.c splitcod.c intercod.c match.c \
	draw32.c draw24.c draw15.c draw8.c draw8d.c \
	expand32.c expand24.c expand15.c expand16.c \
	endingco.c dib2yuv2.c expand8.c expand8d.c \
	butthead.c dibtovec.c diffdet.c rate.c \
	generate.c writetil.c expandyuy2.c

MY_DFLAGS := -DWIN32 -DDIBS=$(DIBS) -DNOASM

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I. \
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

