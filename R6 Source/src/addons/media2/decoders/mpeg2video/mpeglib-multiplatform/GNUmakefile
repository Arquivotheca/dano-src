# Makefile using CPUFeaturesTemplate.mk and SimpleSharedLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

TARGET_NAME := libMPEG2.so

SRC_DIR :=  src/addons/media2/decoders/mpeg2video/mpeglib-multiplatform

SRCS := idct_mmx.asm \
		idct_mmx_3dnow.asm \
		mpeg2lib.cpp \
		misc_mmx.asm \
		yuv2YCbCr422.asm \
		downmix_sse.asm \
#

MY_WARNINGS :=

SYSTEM_LIBS:= libroot.so

MY_INSTALL_DIR := $(INSTALL_LIBS)

TEMPLATE := SimpleSharedLibTemplate.mk

CPU_FEATURES := CMK CM C M3

# A bit of a hack, but the only way to do the task at hand.
GL_STYLE_PROCESSOR_DEFINES := true

include $(SDIR_TEMPLATES)/CPUFeaturesTemplate.mk

SOURCE	:= /boot/beos/system/lib/libMPEG2.so

SRC_DIR := src/addons/media2/decoders/mpeg2video/mpeglib-multiplatform

LINKS :=$(INSTALL_DEV_LIBS)/libGL2.so

LINK_PREFIX		:=
LINK_RELATION	:=
LINKS			:=$(INSTALL_DEV_LIBS)/libMPEG2.so
MY_DEPS			:=$(MAKEFILE_LEAFNAME)

include $(SDIR_TEMPLATES)/SymlinkTemplate.mk
