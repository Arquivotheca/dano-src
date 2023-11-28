SRC_DIR:=src/addons/media/decoders/ccittadpcm
MY_OBJ_OFFSET := g723_5.encoder.f
SRCS := \
		g72x.cpp \
		g711.cpp \
		g723_40.cpp
#
TYPE := SHARED

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := \
	-I$(call FULL_SRC_DIR, src/addons/media/encoders/g723_5) \
	-I$(call FULL_SRC_DIR, src/addons/media/decoders/ccittadpcm) \
	-I$(call FULL_SRC_DIR, src/addons/media/encoders/g72x) \
	-I$(BUILDHOME)/src/inc/media_p
#

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

# The rest of the sources
SRC_DIR:=src/addons/media/encoders/g72x
MY_OBJ_OFFSET := g723_5.encoder.f
SRCS := \
		g72xEncoder.cpp
#
TYPE := SHARED

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := $(MY_INCLUDES_IN)
#

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS += $(OBJS_OUT)


# Makefile using AddonTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the addon. (can not be empty)
TARGET_NAME:=g723_5.encoder

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media/encoders/g723_5

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your addon links against.
SYSTEM_LIBS:= libroot.so libbe.so \
	libmedia.so
#

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_ADD_ONS)/media/encoders

include $(SDIR_TEMPLATES)/AddonTemplate.mk

