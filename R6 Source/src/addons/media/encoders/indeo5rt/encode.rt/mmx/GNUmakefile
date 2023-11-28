SRC_DIR:=src/addons/media/encoders/indeo5rt/encode.rt/common
MY_OBJ_OFFSET := libindeo_enc_mmx.a.f
SRCS :=  \
	beosalloc.c
#
TYPE := STATIC

MY_DFLAGS := \
				-D__NO_WINDOWS__ -DC_PORT_KIT

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, src/addons/media/encoders/indeo5rt/encode.rt/mmx) \
	-I$(BUILDHOME)/src/kit/devel/i586/lib/include \
#

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libindeo_enc_mmx.a

MY_DFLAGS := $(MY_DFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media/encoders/indeo5rt/encode.rt/mmx

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)

SRCS += \
	brc.s \
	bswrite.s \
	bswrtx.s \
	cintfc.s \
	common.s \
	const.s \
	encuvx.s \
	encyx.s \
	enyplanx.s \
	fgtx.s \
	fgtxkl.s \
	metable.s \
	motionx.s \
	noxformx.s \
	qdelta.s \
	qntscan.s \
	qrvtable.s \
	qtbls.s \
	rtenc.s \
	vlc.s \
	vlce.s \
	ycopyx.s


# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS := -mno-fp-ret-in-387 -g -DDEBUG

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

