# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libindeo_enc.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media/encoders/indeo5/encode.cpk

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS :=  \
	bsdbg.c \
	bsutil.c \
	cmpress.c \
	common.c \
	const.c \
	convert.c \
	convtabs.c \
	decomp.c \
	enbs.c \
	encpia.c \
	enme.c \
	enmesrch.c \
	enntryns.c \
	enseg.c \
	entrans.c \
	enwav.c \
	errhand.c \
	hufftbls.c \
	icconv.c \
	matrix.c \
	mc.c \
	prefilt.c \
	qnttbls.c \
	recomp.c \
	tksys.c \
	xfrm.c

MY_DFLAGS := -DC_PORT_KIT -D__NO_WINDOWS__

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I. \
	-I$(BUILDHOME)/src/kit/devel/i586/lib/include \
	-I$(BUILDHOME)/src/addons/media/decoders/indeo5/decode.cpk \
	-I$(BUILDHOME)/src/addons/media/decoders/indeo5/include 
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
# MY_DEBUG_FLAGS := -mno-fp-ret-in-387 -g -DDEBUG

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

