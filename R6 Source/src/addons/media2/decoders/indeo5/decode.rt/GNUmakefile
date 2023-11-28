# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libindeo_dec.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media2/decoders/indeo5/decode.rt

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS :=  \
	qnttbls.c	\
	convtabs.c	\
	beosresource.c	\
	ftol.s \
	resbitmap.cpp \
	rtcout_cpk.c \
	dynclut_cpk.c

#	dynclut.s \

SRCS += \
	blktabs.s \
	blockdec.s \
	btyuv29p.s \
	c420.s \
	c422.s \
	cfgpal.s \
	clutap.s \
	clutcp.s \
	cluttab.s \
	controls.s \
	cp_tabs.s \
	currband.s \
	decbndh.s \
	decmbinf.s \
	decpl.s \
	dectile.s \
	gcompasm.s \
	hivedec.s \
	ifonine.s \
	makemask.s \
	mcasm.s \
	mmxslant.s \
	pentime.s \
	readbits.s \
	rgbtbls.s \
	rtcout.s \
	rtdec.s \
	xpardec.s \
	xrgb32.s \
	yrgb16.s \
	yrgb16z2.s \
	yrgb24.s \
	yrgb24z2.s \
	yuyv29p.s

#	meantime.s \

MY_DFLAGS := \
		-D__NO_WINDOWS__ -DC_PORT_KIT

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I. \
		-I$(BUILDHOME)/src/kit/devel/i586/lib/include \
		-I$(call FULL_SRC_DIR, src/addons/media2/decoders/indeo5) \
		-I$(call FULL_SRC_DIR, src/addons/media2/decoders/indeo5)/include \
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

