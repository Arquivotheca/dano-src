# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libpjpeg-jpeg.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media/jpeglib

JPEGDIR := jpeglib/jpeg-6a

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	jpeg_io_routines.cpp \
	jpeg_interface.cpp \
	mjpega_interface.cpp \
	$(JPEGDIR)//jdmerge.c \
	$(JPEGDIR)/jdhuff.c \
	$(JPEGDIR)/jdmaster.c \
	$(JPEGDIR)/jdapimin.c \
	$(JPEGDIR)/jdsample.c \
	$(JPEGDIR)/jdapistd.c \
	$(JPEGDIR)/jdatadst.c \
	$(JPEGDIR)/jdatasrc.c \
	$(JPEGDIR)/jdcoefct.c \
	$(JPEGDIR)/jdcolor.c \
	$(JPEGDIR)/jddctmgr.c \
	$(JPEGDIR)/jdinput.c \
	$(JPEGDIR)/jdmainct.c \
	$(JPEGDIR)/jdmarker.c \
	$(JPEGDIR)/jdphuff.c \
	$(JPEGDIR)/jdpostct.c \
	$(JPEGDIR)/jdtrans.c \
	$(JPEGDIR)/jcapimin.c \
	$(JPEGDIR)/jcapistd.c \
	$(JPEGDIR)/jccoefct.c \
	$(JPEGDIR)/jccolor.c \
	$(JPEGDIR)/jcdctmgr.c \
	$(JPEGDIR)/jcinit.c \
	$(JPEGDIR)/jcmainct.c \
	$(JPEGDIR)/jcmarker.c \
	$(JPEGDIR)/jcmaster.c \
	$(JPEGDIR)/jcomapi.c \
	$(JPEGDIR)/jcparam.c \
	$(JPEGDIR)/jcphuff.c \
	$(JPEGDIR)/jcprepct.c \
	$(JPEGDIR)/jcsample.c \
	$(JPEGDIR)/jctrans.c \
	$(JPEGDIR)/jerror.c \
	$(JPEGDIR)/jidctint.c \
	$(JPEGDIR)/jidctred.c \
	$(JPEGDIR)/jquant1.c \
	$(JPEGDIR)/jquant2.c \
	$(JPEGDIR)/jchuff.c \
	$(JPEGDIR)/jfdctint.c \
	$(JPEGDIR)/jidctflt.c \
	$(JPEGDIR)/jidctfst.c \
	$(JPEGDIR)/jfdctflt.c \
	$(JPEGDIR)/jfdctfst.c \
	$(JPEGDIR)/jmemansi.c \
	$(JPEGDIR)/jutils.c \
	$(JPEGDIR)/jmemmgr.c \
#

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := $(FORCE_CPP)

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I. \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(JPEGDIR) \
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

